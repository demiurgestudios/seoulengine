/**
 * \file FalconBitmapInstance.cpp
 * \brief A direct instantiation of
 * a BitmapDefinition.
 *
 * Typically, the basic Falcon draw component
 * is a ShapeInstance, but occasionally Flash
 * can directly export bitmap definitions as
 * as BitmapInstance.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FalconBitmapDefinition.h"
#include "FalconBitmapInstance.h"
#include "FalconClipper.h"
#include "FalconDefinition.h"
#include "FalconFCNFile.h"
#include "FalconMovieClipDefinition.h"
#include "FalconMovieClipInstance.h"
#include "FalconRenderDrawer.h"
#include "FalconRenderPoser.h"
#include "FalconScalingGrid.h"
#include "FixedArray.h"
#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionDefine.h"

namespace Seoul
{

#if SEOUL_ENABLE_CHEATS
namespace Falcon { FilePath DevOnlyIndirectTextureLookup(FilePathRelativeFilename id); }
#endif

// Note: can't make this static or the compiler will complain
// Seems to be an issue in 15.6.6 VC.  Seems fixed in 15.8
// Making an anonymous namespace instead.
namespace
{
	String GetBitmapFilename(const Falcon::BitmapInstance& inst)
	{
		auto ret = (inst.GetBitmapDefinition().IsValid() ? inst.GetBitmapDefinition()->GetFilePath() : FilePath());

		// Indirect handling - only available in developer builds.
#if SEOUL_ENABLE_CHEATS
		if (ret.GetType() == FileType::kUnknown)
		{
			ret = Falcon::DevOnlyIndirectTextureLookup(ret.GetRelativeFilenameWithoutExtension());
		}
#endif

		return (ret.IsValid() ? ret.GetRelativeFilenameInSource() : String());
	}
}

SEOUL_BEGIN_TYPE(Falcon::BitmapInstance, TypeFlags::kDisableNew)
	SEOUL_PARENT(Falcon::Instance)
	SEOUL_PROPERTY_N_EXT("Definition", GetBitmapDefinition)
	SEOUL_PROPERTY_N_Q("Filename", GetBitmapFilename)
SEOUL_END_TYPE()

namespace Falcon
{

BitmapInstance::BitmapInstance()
	: Instance(0u)
	, m_pBitmap()
{
}

BitmapInstance::BitmapInstance(const SharedPtr<BitmapDefinition const>& pBitmap)
	: Instance(pBitmap.IsValid() ? pBitmap->GetDefinitionID() : 0u)
	, m_pBitmap(pBitmap)
{
}

BitmapInstance::~BitmapInstance()
{
}

Bool BitmapInstance::ComputeLocalBounds(Rectangle& rBounds)
{
	rBounds.m_fBottom = (Float)(m_pBitmap.IsValid() ? m_pBitmap->GetHeight() : 0u);
	rBounds.m_fLeft = 0;
	rBounds.m_fRight = (Float)(m_pBitmap.IsValid() ? m_pBitmap->GetWidth() : 0u);
	rBounds.m_fTop = 0;
	return true;
}

void BitmapInstance::ComputeMask(
	const Matrix2x3& mParent,
	const ColorTransformWithAlpha& cxParent,
	Render::Poser& rPoser)
{
	if (!m_pBitmap.IsValid())
	{
		return;
	}

	// TODO: Reconsider - we don't consider the alpha
	// to match Flash behavior. I've never double checked what
	// happens if you (just) set the visibility of a mask to
	// false and logically it makes sense for visibility and alpha==0.0
	// to have the same behavior (or, in other words, visibility should
	// possibly not be considered here).
	if (!GetVisible())
	{
		return;
	}
	
	// Unlike many code paths, alpha == 0.0 is not considered here. Flash
	// does not hide the mask (or the shapes it reveals) if the cumulative
	// alpha at that mask is 0.0.

	Matrix2x3 const mWorld = (mParent * GetTransform());
	Rectangle const rect = m_pBitmap->GetVisibleRectangle();

	rPoser.ClipStackAddRectangle(mWorld, rect);
}

void BitmapInstance::Pose(
	Render::Poser& rPoser,
	const Matrix2x3& mParent,
	const ColorTransformWithAlpha& cxParent)
{
	if (!m_pBitmap.IsValid())
	{
		return;
	}

	if (!GetVisible())
	{
		return;
	}

	// Early out for fully transparent/not visible bitmaps.
	if (!m_pBitmap->IsVisible())
	{
		return;
	}

	ColorTransformWithAlpha const cxWorld = (cxParent * GetColorTransformWithAlpha());
	if (cxWorld.m_fMulA == 0.0f)
	{
		return;
	}

	Matrix2x3 const mWorld = (mParent * GetTransform());
	Float const fWidth = (Float)m_pBitmap->GetWidth();
	Float const fHeight = (Float)m_pBitmap->GetHeight();
	auto const bounds(Rectangle::Create(0, fWidth, 0, fHeight));
	auto const visibleBounds(m_pBitmap->GetVisibleRectangle());
	auto const worldBounds(TransformRectangle(mWorld, visibleBounds));

	TextureReference reference;
	if (Render::PoserResolveResult::kSuccess != rPoser.ResolveTextureReference(
		worldBounds,
		this,
		rPoser.GetRenderThreshold(fWidth, fHeight, mWorld),
		m_pBitmap,
		reference,
		m_pBitmap->GetPreload()))
	{
		return;
	}

	auto const worldOcclusion(ComputeOcclusionRectangle(mWorld, reference, bounds));
	Bool const bScalingGrid = (nullptr != GetParent() && GetParent()->GetMovieClipDefinition()->HasScalingGrid());
	rPoser.Pose(
		worldBounds,
		this,
		(bScalingGrid ? mParent : mWorld),
		cxWorld,
		reference,
		worldOcclusion,
		Render::Feature::kNone);
}

#if SEOUL_ENABLE_CHEATS
void BitmapInstance::PoseInputVisualization(
	Render::Poser& rPoser,
	const Matrix2x3& mParent,
	RGBA color)
{
	if (!m_pBitmap.IsValid())
	{
		return;
	}

	Rectangle const bounds = Rectangle::Create(
		0,
		(Float32)m_pBitmap->GetWidth(),
		0,
		(Float32)m_pBitmap->GetHeight());

	// TODO: Draw the appropriate shape for exact hit testing.
	auto const mWorld(mParent * GetTransform());
	auto const worldBounds = TransformRectangle(mWorld, bounds);
	rPoser.PoseInputVisualization(
		worldBounds,
		bounds,
		mWorld,
		color);
}
#endif

void BitmapInstance::Draw(
	Render::Drawer& rDrawer,
	const Rectangle& worldBoundsPreClip,
	const Matrix2x3& mParentOrWorld,
	const ColorTransformWithAlpha& cxWorld,
	const TextureReference& textureReference,
	Int32 /*iSubInstanceId*/)
{
	auto const fTX = textureReference.m_vVisibleOffset.X;
	auto const fTY = textureReference.m_vVisibleOffset.Y;
	auto const fTZ = textureReference.m_vVisibleOffset.X + textureReference.m_vVisibleScale.X;
	auto const fTW = textureReference.m_vVisibleOffset.Y + textureReference.m_vVisibleScale.Y;

	Float const fWidth = (Float)m_pBitmap->GetWidth();
	Float const fHeight = (Float)m_pBitmap->GetHeight();

	Float32 const fTU0 = fTX;
	Float32 const fTV0 = fTY;
	Float32 const fTU1 = fTZ;
	Float32 const fTV1 = fTW;
	Float32 const fX0 = (fTU0 * fWidth);
	Float32 const fY0 = (fTV0 * fHeight);
	Float32 const fX1 = (fTU1 * fWidth);
	Float32 const fY1 = (fTV1 * fHeight);

	FixedArray<ShapeVertex, 4> aVertices;
	aVertices[0] = ShapeVertex::Create(fX0, fY0, RGBA::White(), RGBA::TransparentBlack(), fTU0, fTV0);
	aVertices[1] = ShapeVertex::Create(fX0, fY1, RGBA::White(), RGBA::TransparentBlack(), fTU0, fTV1);
	aVertices[2] = ShapeVertex::Create(fX1, fY1, RGBA::White(), RGBA::TransparentBlack(), fTU1, fTV1);
	aVertices[3] = ShapeVertex::Create(fX1, fY0, RGBA::White(), RGBA::TransparentBlack(), fTU1, fTV0);

	Bool const bScalingGrid = (nullptr != GetParent() && GetParent()->GetMovieClipDefinition()->HasScalingGrid());
	if (bScalingGrid)
	{
		auto const& scalingGrid = GetParent()->GetMovieClipDefinition()->GetScalingGrid();

		FixedArray<UInt16, 6> aIndices;
		aIndices[0] = 0;
		aIndices[1] = 1;
		aIndices[2] = 2;
		aIndices[3] = 0;
		aIndices[4] = 2;
		aIndices[5] = 3;

		rDrawer.GetScalingGrid().DrawTriangleList(
			scalingGrid,
			worldBoundsPreClip,
			textureReference,
			mParentOrWorld,
			GetTransform(),
			cxWorld,
			Rectangle::Create(0.0f, fWidth, 0.0f, fHeight),
			aIndices.Data(),
			aIndices.GetSize(),
			aVertices.Data(),
			aVertices.GetSize(),
			TriangleListDescription::kQuadList,
			Render::Feature::kNone);
	}
	else
	{
		rDrawer.DrawTriangleList(
			worldBoundsPreClip,
			textureReference,
			mParentOrWorld,
			cxWorld,
			aVertices.Data(),
			4,
			TriangleListDescription::kQuadList,
			Render::Feature::kNone);
	}
}

Bool BitmapInstance::HitTest(
	const Matrix2x3& mParent,
	Float32 fWorldX,
	Float32 fWorldY,
	Bool bIgnoreVisibility /*= false*/) const
{
	if (!m_pBitmap.IsValid())
	{
		return false;
	}

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
	Rectangle const bounds = Rectangle::Create(
		0,
		(Float32)m_pBitmap->GetWidth(),
		0,
		(Float32)m_pBitmap->GetHeight());

	Float32 const fObjectSpaceX = vObjectSpace.X;
	Float32 const fObjectSpaceY = vObjectSpace.Y;

	if (fObjectSpaceX < bounds.m_fLeft) { return false; }
	if (fObjectSpaceY < bounds.m_fTop) { return false; }
	if (fObjectSpaceX > bounds.m_fRight) { return false; }
	if (fObjectSpaceY > bounds.m_fBottom) { return false; }

	return true;
}

void BitmapInstance::SetBitmapDefinition(const SharedPtr<BitmapDefinition const>& pBitmap)
{
	m_pBitmap = pBitmap;
}

} // namespace Falcon

} // namespace Seoul
