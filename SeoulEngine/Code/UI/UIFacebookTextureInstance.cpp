/**
 * \file UIFacebookTextureInstance.cpp
 * \brief SeoulEngine subclass/extension of Falcon::Instance for Facebook profile images.
 *
 * UI::FacebookTextureInstance is a subclass of Falcon::Instance that is mostly
 * similar to UI::TextureSubstitutionInstance, except textures are sourced
 * from FacebookProfileImageManager.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ContentLoadManager.h"
#include "Engine.h"
#include "FacebookImageManager.h"
#include "FalconBitmapDefinition.h"
#include "FalconRenderDrawer.h"
#include "FalconRenderPoser.h"
#include "ReflectionDefine.h"
#include "TextureManager.h"
#include "UIManager.h"
#include "UIRenderer.h"
#include "UIUtil.h"
#include "UIFacebookTextureInstance.h"

namespace Seoul
{

SEOUL_BEGIN_TYPE(UI::FacebookTextureInstance, TypeFlags::kDisableNew)
	SEOUL_PARENT(Falcon::Instance)
SEOUL_END_TYPE()

namespace UI
{

FacebookTextureInstance::FacebookTextureInstance(
	String& facebookUserGUID,
	FilePath defaultImageFilePath,
	Int32 iTextureWidth,
	Int32 iTextureHeight)
	: Falcon::Instance(0)
	, m_pBitmap()
	, m_pDefaultBitmap()
	, m_vTextureCoordinates(0.0f, 0.0f, 1.0f, 1.0f)
	, m_CachedFilePath()
	, m_CachedDefaultFilePath(defaultImageFilePath)
	, m_iTextureWidth(iTextureWidth)
	, m_iTextureHeight(iTextureHeight)
	, m_sUserFacebookGuid(facebookUserGUID)
{

}

FacebookTextureInstance::~FacebookTextureInstance()
{

}

Bool FacebookTextureInstance::ComputeLocalBounds(Falcon::Rectangle& rBounds)
{
	rBounds.m_fBottom = (Float32)m_iTextureHeight;
	rBounds.m_fLeft = 0;
	rBounds.m_fRight = (Float32)m_iTextureWidth;
	rBounds.m_fTop = 0;
	return true;
}

void FacebookTextureInstance::Pose(
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

	// The flow is as it states. If we dont have our
	// bitmap valid and no cached file path then we must render
	// our given default image path. Then it falls under.
	// Check if the default image path is correct or is it using a Hstring ID
	// and create the bitmap for that default image occordingly.
	if (!m_pBitmap.IsValid() || !m_CachedFilePath.IsValid())
	{
		if (!m_pDefaultBitmap.IsValid() && m_CachedDefaultFilePath.IsValid())
		{
			m_pDefaultBitmap.Reset(SEOUL_NEW(MemoryBudgets::UIRuntime) Falcon::BitmapDefinition(
				m_CachedDefaultFilePath,
				m_iTextureWidth,
				m_iTextureHeight,
				0));

			m_vTextureCoordinates = Vector4D(0.0f, 0.0f, 1.0f, 1.0f);

		}
	}

	if (!m_CachedFilePath.IsValid())
	{
		//make the request to facebookImage manager for the file path with our facebook guid
		FilePath imageFilePath = FacebookImageManager::Get()->RequestFacebookImageBitmap(m_sUserFacebookGuid);

		//if we got a correct file path, save it and use it for the life of the application
		if (imageFilePath.IsValid())
		{
			m_CachedFilePath = imageFilePath;

			//reset the bitmap once we got a correct filepath and the bitmap has not
			//been set up yet
			m_pBitmap.Reset(SEOUL_NEW(MemoryBudgets::UIRuntime) Falcon::BitmapDefinition(
					m_CachedFilePath,
					m_iTextureWidth,
					m_iTextureHeight,
					0));

			m_vTextureCoordinates = Vector4D(0.0f, 0.0f, 1.0f, 1.0f);
		}
	}

	// bail if we dont have the proper resources to draw the FB image
	if ((!m_pBitmap.IsValid() || !m_CachedFilePath.IsValid())
		&& (!m_pDefaultBitmap.IsValid() || !m_CachedDefaultFilePath.IsValid()))
	{
		return;
	}

	Matrix2x3 const mWorld = (mParent * GetTransform());
	Float const fWidth = (Float)m_iTextureWidth;
	Float const fHeight = (Float)m_iTextureHeight;
	auto const bounds(Falcon::Rectangle::Create(0, fWidth, 0, fHeight));
	auto const worldBounds(Falcon::TransformRectangle(mWorld, bounds));

	Falcon::TextureReference reference;
	auto const eResult = rPoser.ResolveTextureReference(
		worldBounds,
		this,
		rPoser.GetRenderThreshold(fWidth, fHeight, mWorld),
		(m_pBitmap.IsValid() ? m_pBitmap : m_pDefaultBitmap),
		reference);
	if (Falcon::Render::PoserResolveResult::kSuccess != eResult)
	{
		if (Falcon::Render::PoserResolveResult::kNotReady == eResult && !rPoser.InPlanarShadow())
		{
			m_vTextureCoordinates = Vector4D(0.0f, 0.0f, 1.0f, 1.0f);
		}
	}
	else
	{
		auto const worldOcclusion(Falcon::ComputeOcclusionRectangle(mWorld, reference, bounds));
		rPoser.Pose(
			worldBounds,
			this,
			mWorld,
			cxWorld,
			reference,
			worldOcclusion,
			Falcon::Render::Feature::kNone);
	}
}

void FacebookTextureInstance::Draw(
	Falcon::Render::Drawer& rDrawer,
	const Falcon::Rectangle& worldBoundsPreClip,
	const Matrix2x3& mWorld,
	const Falcon::ColorTransformWithAlpha& cxWorld,
	const Falcon::TextureReference& textureReference,
	Int32 /*iSubInstanceId*/)
{
	m_vTextureCoordinates.X = textureReference.m_vVisibleOffset.X;
	m_vTextureCoordinates.Y = textureReference.m_vVisibleOffset.Y;
	m_vTextureCoordinates.Z = textureReference.m_vVisibleOffset.X + textureReference.m_vVisibleScale.X;
	m_vTextureCoordinates.W = textureReference.m_vVisibleOffset.Y + textureReference.m_vVisibleScale.Y;

	Float const fWidth = (Float)m_iTextureWidth;
	Float const fHeight = (Float)m_iTextureHeight;

	Float const fTU0 = m_vTextureCoordinates.X;
	Float const fTV0 = m_vTextureCoordinates.Y;
	Float const fTU1 = m_vTextureCoordinates.Z;
	Float const fTV1 = m_vTextureCoordinates.W;
	Float const fX0 = (fTU0 * fWidth);
	Float const fY0 = (fTV0 * fHeight);
	Float const fX1 = (fTU1 * fWidth);
	Float const fY1 = (fTV1 * fHeight);

	FixedArray<Falcon::ShapeVertex, 4> aVertices;
	aVertices[0] = Falcon::ShapeVertex::Create(fX0, fY0, RGBA::White(), RGBA::TransparentBlack(), fTU0, fTV0);
	aVertices[1] = Falcon::ShapeVertex::Create(fX0, fY1, RGBA::White(), RGBA::TransparentBlack(), fTU0, fTV1);
	aVertices[2] = Falcon::ShapeVertex::Create(fX1, fY1, RGBA::White(), RGBA::TransparentBlack(), fTU1, fTV1);
	aVertices[3] = Falcon::ShapeVertex::Create(fX1, fY0, RGBA::White(), RGBA::TransparentBlack(), fTU1, fTV0);

	rDrawer.DrawTriangleList(
		worldBoundsPreClip,
		textureReference,
		mWorld,
		cxWorld,
		aVertices.Get(0),
		4,
		Falcon::TriangleListDescription::kQuadList,
		Falcon::Render::Feature::kNone);
}

#if SEOUL_ENABLE_CHEATS
void FacebookTextureInstance::PoseInputVisualization(
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

Bool FacebookTextureInstance::HitTest(
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
	Float32 const fObjectSpaceX = vObjectSpace.X;
	Float32 const fObjectSpaceY = vObjectSpace.Y;

	if (fObjectSpaceX < 0) { return false; }
	if (fObjectSpaceY < 0) { return false; }
	if (fObjectSpaceX > (Float)m_iTextureWidth) { return false; }
	if (fObjectSpaceY > (Float)m_iTextureHeight) { return false; }

	return true;
}

Falcon::InstanceType FacebookTextureInstance::GetType() const
{
	return Falcon::InstanceType::kCustom;
}

FacebookTextureInstance::FacebookTextureInstance()
	: Falcon::Instance(0)
{
}

} // namespace UI

} // namespace Seoul
