/**
 * \file UIStage3D.cpp
 * \brief Similar to UI::TextureSubstitution, except that perspective
 * effects can be applied, to create the illusion of depth. Intended
 * for background plates and similar images.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ContentLoadManager.h"
#include "Engine.h"
#include "FalconBitmapDefinition.h"
#include "FalconRenderDrawer.h"
#include "FalconRenderPoser.h"
#include "FalconStage3DSettings.h"
#include "ReflectionDefine.h"
#include "TextureManager.h"
#include "UIManager.h"
#include "UIRenderer.h"
#include "UIStage3D.h"
#include "UIUtil.h"

namespace Seoul
{

SEOUL_BEGIN_TYPE(UI::Stage3D, TypeFlags::kDisableNew)
	SEOUL_PARENT(Falcon::Instance)
SEOUL_END_TYPE()

namespace UI
{

Stage3D::Stage3D(
	FilePath filePath,
	Int32 iTextureWidth,
	Int32 iTextureHeight)
	: Falcon::Instance(0)
	, m_vTextureCoordinates(0.0f, 0.0f, 1.0f, 1.0f)
	, m_FilePath(filePath)
	, m_iTextureWidth(iTextureWidth)
	, m_iTextureHeight(iTextureHeight)
{
}

Stage3D::~Stage3D()
{
}

Bool Stage3D::ComputeLocalBounds(Falcon::Rectangle& rBounds)
{
	rBounds.m_fBottom = (Float32)m_iTextureHeight;
	rBounds.m_fLeft = 0.0f;
	rBounds.m_fRight = (Float32)m_iTextureWidth;
	rBounds.m_fTop = 0.0f;
	return true;
}

void Stage3D::Pose(
	Falcon::Render::Poser& rPoser,
	const Matrix2x3& mParent,
	const Falcon::ColorTransformWithAlpha& cxParent)
{
	if (!GetVisible())
	{
		return;
	}

	Falcon::ColorTransformWithAlpha cxWorld = (cxParent * GetColorTransformWithAlpha());
	if (cxWorld.m_fMulA == 0.0f)
	{
		return;
	}

	if (!m_FilePath.IsValid())
	{
		return;
	}

	Matrix2x3 const mWorld = (mParent * GetTransform());
	Float32 const fWidth = (Float32)m_iTextureWidth;
	Float32 const fHeight = (Float32)m_iTextureHeight;
	auto const bounds(Falcon::Rectangle::Create(0, fWidth, 0, fHeight));
	auto const worldBounds(Falcon::TransformRectangle(mWorld, bounds));
	auto const fRenderThreshold = rPoser.GetRenderThreshold(fWidth, fHeight, mWorld);

	Falcon::TextureReference reference;
	auto const eResult = rPoser.ResolveTextureReference(
		worldBounds,
		this,
		fRenderThreshold,
		m_FilePath,
		reference,
		true);
	if (Falcon::Render::PoserResolveResult::kSuccess != eResult)
	{
		if (Falcon::Render::PoserResolveResult::kNotReady == eResult)
		{
			m_vTextureCoordinates = Vector4D(0.0f, 0.0f, 1.0f, 1.0f);
		}
	}
	else
	{
		rPoser.PoseWithFarthestDepth(
			1.0f,
			worldBounds,
			this,
			mWorld,
			cxWorld,
			reference,
			// TODO: We may be able to use a sub-region as a valid
			// occlusion rectangle.
			Falcon::Rectangle(),
			Falcon::Render::Feature::kNone);

		if (rPoser.GetState().m_pStage3DSettings->m_Perspective.m_bDebugShowGridTexture)
		{
			PrepareDebugGrid();
			if (m_pGrid.IsValid())
			{
				Falcon::TextureReference gridReference;
				if (Falcon::Render::PoserResolveResult::kSuccess == rPoser.ResolveTextureReference(
					worldBounds,
					this,
					fRenderThreshold,
					m_pGrid,
					gridReference))
				{
					rPoser.PoseWithFarthestDepth(
						1.0f,
						worldBounds,
						this,
						mWorld,
						cxWorld,
						gridReference,
						Falcon::Rectangle(),
						Falcon::Render::Feature::kNone);
				}
			}
		}
	}
}

void Stage3D::Draw(
	Falcon::Render::Drawer& rDrawer,
	const Falcon::Rectangle& worldBoundsPreClip,
	const Matrix2x3& mWorld,
	const Falcon::ColorTransformWithAlpha& cxWorld,
	const Falcon::TextureReference& textureReference,
	Int32 /*iSubInstanceId*/)
{
	auto const& settings = rDrawer.GetState().m_pStage3DSettings->m_Perspective;

	m_vTextureCoordinates.X = textureReference.m_vVisibleOffset.X;
	m_vTextureCoordinates.Y = textureReference.m_vVisibleOffset.Y;
	m_vTextureCoordinates.Z = textureReference.m_vVisibleOffset.X + textureReference.m_vVisibleScale.X;
	m_vTextureCoordinates.W = textureReference.m_vVisibleOffset.Y + textureReference.m_vVisibleScale.Y;

	Float32 const fWidth = (Float32)m_iTextureWidth;
	Float32 const fHeight = (Float32)m_iTextureHeight;

	Float32 const fTU0 = m_vTextureCoordinates.X;
	Float32 const fTV0 = m_vTextureCoordinates.Y;
	Float32 const fTU1 = m_vTextureCoordinates.Z;
	// Intentional, we want the horizon line to be based on the image dimensions,
	// not the visible dimensions, so it is not content dependent.
	Float32 const fTV1 = Lerp(0.0f, 1.0f, settings.m_fHorizon);
	Float32 const fTV2 = m_vTextureCoordinates.W;
	Float32 const fX0 = (fTU0 * fWidth);
	Float32 const fY0 = (fTV0 * fHeight);
	Float32 const fX1 = (fTU1 * fWidth);
	// Intentional, we want the horizon line to be based on the image dimensions,
	// not the visible dimensions, so it is not content dependent.
	Float32 const fY1 = Lerp(0.0f, fHeight, settings.m_fHorizon);
	Float32 const fY2 = (fTV2 * fHeight);

	FixedArray<UInt16, 12> aIndices;
	aIndices[0]  = 0;
	aIndices[1]  = 1;
	aIndices[2]  = 2;
	aIndices[3]  = 0;
	aIndices[4]  = 2;
	aIndices[5]  = 3;
	aIndices[6]  = 1;
	aIndices[7]  = 4;
	aIndices[8]  = 5;
	aIndices[9]  = 1;
	aIndices[10] = 5;
	aIndices[11] = 2;

	FixedArray<Falcon::ShapeVertex, 6> aVertices;
	aVertices[0] = Falcon::ShapeVertex::Create(fX0, fY0, RGBA::White(), RGBA::TransparentBlack(), fTU0, fTV0);
	aVertices[1] = Falcon::ShapeVertex::Create(fX0, fY1, RGBA::White(), RGBA::TransparentBlack(), fTU0, fTV1);
	aVertices[2] = Falcon::ShapeVertex::Create(fX1, fY1, RGBA::White(), RGBA::TransparentBlack(), fTU1, fTV1);
	aVertices[3] = Falcon::ShapeVertex::Create(fX1, fY0, RGBA::White(), RGBA::TransparentBlack(), fTU1, fTV0);
	aVertices[4] = Falcon::ShapeVertex::Create(fX0, fY2, RGBA::White(), RGBA::TransparentBlack(), fTU0, fTV2);
	aVertices[5] = Falcon::ShapeVertex::Create(fX1, fY2, RGBA::White(), RGBA::TransparentBlack(), fTU1, fTV2);

	FixedArray<Float32, 6> aDepths;
	aDepths[0] = 0.0f;
	aDepths[1] = 0.0f;
	aDepths[2] = 0.0f;
	aDepths[3] = 0.0f;
	aDepths[4] = 1.0f;
	aDepths[5] = 1.0f;

	// Push our 3D staging to the renderer, so other components can be projected correctly.
	Manager::Get()->GetRenderer().SetStage3DProjectionBounds(
		Matrix2x3::TransformPosition(mWorld, Vector2D(0, fY1)).Y,
		Matrix2x3::TransformPosition(mWorld, Vector2D(0, fY2)).Y);

	rDrawer.DrawTriangleList(
		worldBoundsPreClip,
		textureReference,
		mWorld,
		cxWorld,
		aIndices.Data(),
		aIndices.GetSize(),
		aDepths.Data(),
		aVertices.Data(),
		aVertices.GetSize(),
		Falcon::TriangleListDescription::kNotSpecific,
		Falcon::Render::Feature::kNone);
}

Bool Stage3D::HitTest(
	const Matrix2x3& mParent,
	Float32 fWorldX,
	Float32 fWorldY,
	Bool bIgnoreVisibility /*= false*/) const
{
	if (!bIgnoreVisibility)
	{
		if (!GetVisible())
		{
			return false;
		}
	}

	Matrix2x3 const mWorld = (mParent * GetTransform());
	Matrix2x3 const mInverseWorld = mWorld.Inverse();

	Vector2D const vObjectSpace = Matrix2x3::TransformPosition(mInverseWorld, Vector2D(fWorldX, fWorldY));
	Float32 fObjectSpaceX = vObjectSpace.X;
	Float32 fObjectSpaceY = vObjectSpace.Y;

	if (fObjectSpaceX < 0) { return false; }
	if (fObjectSpaceY < 0) { return false; }
	if (fObjectSpaceX > (Float32)m_iTextureWidth) { return false; }
	if (fObjectSpaceY > (Float32)m_iTextureHeight) { return false; }

	return true;
}

#if SEOUL_ENABLE_CHEATS
void Stage3D::PoseInputVisualization(
	Falcon::Render::Poser& rPoser,
	const Matrix2x3& mParent,
	RGBA color)
{
	auto const bounds = Falcon::Rectangle::Create(
		0,
		(Float)m_iTextureWidth,
		0,
		(Float32)m_iTextureHeight);

	// TODO: Draw the appropriate shape for exact hit testing.
	auto const mWorld(mParent * GetTransform());
	auto const worldBounds = Falcon::TransformRectangle(mWorld, bounds);
	rPoser.PoseInputVisualization(
		worldBounds,
		bounds,
		mWorld,
		color);
}
#endif

Falcon::InstanceType Stage3D::GetType() const
{
	return Falcon::InstanceType::kCustom;
}

Stage3D::Stage3D()
	: Falcon::Instance(0)
	, m_pGrid()
	, m_vTextureCoordinates(0.0f, 0.0f, 1.0f, 1.0f)
	, m_FilePath()
	, m_iTextureWidth(0)
	, m_iTextureHeight(0)
{
}

void Stage3D::PrepareDebugGrid()
{
	if (m_pGrid.IsValid())
	{
		return;
	}

	RGBA* pColor = (RGBA*)MemoryManager::AllocateAligned(
		m_iTextureHeight * m_iTextureWidth * sizeof(RGBA),
		MemoryBudgets::Falcon,
		SEOUL_ALIGN_OF(RGBA));
	memset(pColor, 0, (m_iTextureHeight * m_iTextureWidth * 4));

	static const Int32 s_kaSteps[] =
	{
		1,
		1,
		1,
		1,
		1,
		15,
	};
	static const UInt8 s_kaAlpha[] =
	{
		64,
		128,
		255,
		128,
		64,
		0,
	};

	{
		Int iStep = 0;
		for (Int32 iY = 0; iY < m_iTextureHeight; ++iY)
		{
			for (Int32 iX = 0; iX < m_iTextureWidth; iX += s_kaSteps[iStep])
			{
				auto cColor = RGBA::Black();
				cColor.m_A = s_kaAlpha[iStep];
				pColor[iY * m_iTextureWidth + iX] = cColor;

				iStep = (iStep + 1) % SEOUL_ARRAY_COUNT(s_kaSteps);
			}
		}
	}

	{
		Int iStep = 0;
		for (Int32 iX = 0; iX < m_iTextureWidth; ++iX)
		{
			for (Int32 iY = 0; iY < m_iTextureHeight; iY += s_kaSteps[iStep])
			{
				auto cColor = RGBA::Black();
				cColor.m_A = Max(s_kaAlpha[iStep], pColor[iY * m_iTextureWidth + iX].m_A);
				pColor[iY * m_iTextureWidth + iX] = cColor;

				iStep = (iStep + 1) % SEOUL_ARRAY_COUNT(s_kaSteps);
			}
		}
	}

	m_pGrid.Reset(SEOUL_NEW(MemoryBudgets::Falcon) Falcon::BitmapDefinition(
		(UInt32)m_iTextureWidth,
		(UInt32)m_iTextureHeight,
		(UInt8*)pColor,
		false));
}

} // namespace UI

} // namespace Seoul
