/**
 * \file FalconRenderPoser.cpp
 * The render poser flattens the scene graph into a list
 * of instances to render, which can be further optimized
 * and re-ordered prior to render submission.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FalconClipper.h"
#include "FalconRenderable.h"
#include "FalconRenderCommand.h"
#include "FalconRenderPoser.h"
#include "FalconRenderState.h"
#include "FalconStage3DSettings.h"
#include "FalconTextureCache.h"
#include "FilePath.h"
#include "Matrix2x3.h"

namespace Seoul::Falcon
{

namespace Render
{

Poser::Poser()
	: m_pState()
{
}

Poser::~Poser()
{
}

void Poser::Begin(State& rState)
{
	m_pState = &rState;
}

void Poser::End()
{
	m_pState->EndPhase();
	m_pState.Reset();
}

void Poser::BeginScissorClip(const Rectangle& worldRectangle)
{
	auto const top = (m_vScissorStack.IsEmpty() ? m_pState->m_WorldCullRectangle : m_vScissorStack.Back());

	auto clippedRect = worldRectangle;
	clippedRect.m_fLeft = Max(clippedRect.m_fLeft, top.m_fLeft);
	clippedRect.m_fRight = Min(clippedRect.m_fRight, top.m_fRight);
	clippedRect.m_fTop = Max(clippedRect.m_fTop, top.m_fTop);
	clippedRect.m_fBottom = Min(clippedRect.m_fBottom, top.m_fBottom);

	m_vScissorStack.PushBack(clippedRect);

	m_pState->m_Buffer.IssueBeginScissorClip(clippedRect);
}

void Poser::EndScissorClip()
{
	m_vScissorStack.PopBack();

	auto const top = (m_vScissorStack.IsEmpty() ? Rectangle() : m_vScissorStack.Back());

	m_pState->m_Buffer.IssueEndScissorClip(top);
}

void Poser::BeginPlanarShadows()
{
	if (1 == ++m_pState->m_iInPlanarShadowRender)
	{
		m_pState->m_Buffer.IssueBeginPlanarShadows();
	}
}

void Poser::EndPlanarShadows()
{
	if (1 == m_pState->m_iInPlanarShadowRender)
	{
		m_pState->m_Buffer.IssueEndPlanarShadows();
		--m_pState->m_iInPlanarShadowRender;
	}
}

void Poser::BeginDeferDraw()
{
	if (1 == ++m_pState->m_iInDeferredDrawingRender)
	{
		m_pState->m_Buffer.BeginDeferDraw();
	}
}

void Poser::EndDeferDraw()
{
	if (0 == --m_pState->m_iInDeferredDrawingRender)
	{
		m_pState->m_Buffer.EndDeferDraw();
	}
}

void Poser::FlushDeferredDraw()
{
	m_pState->m_Buffer.FlushDeferredDraw();
}

void Poser::ClipStackAddConvexHull(
	const Matrix2x3& m,
	StandardVertex2D const* p,
	UInt32 u,
	Float fTolerance /* = kfAboutEqualPosition*/)
{
	m_pState->m_pClipStack->AddConvexHull(m, p, u, fTolerance);
}

void Poser::ClipStackAddConvexHull(
	const Matrix2x3& m,
	Vector2D const* p,
	UInt32 u,
	Float fTolerance /* = kfAboutEqualPosition*/)
{
	m_pState->m_pClipStack->AddConvexHull(m, p, u, fTolerance);
}

void Poser::ClipStackAddRectangle(
	const Matrix2x3& m,
	const Rectangle& rect,
	Float fTolerance /*= kfAboutEqualPosition*/)
{
	m_pState->m_pClipStack->AddRectangle(m, rect, fTolerance);
}

void Poser::ClipStackPop()
{
	m_pState->m_Buffer.IssuePopClip();
	m_pState->m_pClipStack->Pop();
}

Bool Poser::ClipStackPush()
{
	if (!m_pState->m_pClipStack->Push())
	{
		return false;
	}

	m_pState->m_Buffer.IssuePushClip(*m_pState->m_pClipStack);
	return true;
}

void Poser::PopDepth3D(Float f, Bool bIgnoreDepthProjection)
{
	m_pState->m_fRawDepth3D -= f;
	if (bIgnoreDepthProjection) { --m_pState->m_iIgnoreDepthProjection; }
}

void Poser::PushDepth3D(Float f, Bool bIgnoreDepthProjection)
{
	if (bIgnoreDepthProjection) { ++m_pState->m_iIgnoreDepthProjection; }
	m_pState->m_fRawDepth3D += f;
}

Pair<Float, Int> Poser::ReplaceDepth3D(Float f)
{
	auto const fReturn = m_pState->m_fRawDepth3D;
	auto const iReturn = m_pState->m_iIgnoreDepthProjection;
	m_pState->m_fRawDepth3D = f;
	return MakePair(fReturn, iReturn);
}

Pair<Float, Int> Poser::ReplaceDepth3D(Float f, Int iIgnoreDepthProjection)
{
	auto const fReturn = m_pState->m_fRawDepth3D;
	auto const iReturn = m_pState->m_iIgnoreDepthProjection;
	m_pState->m_fRawDepth3D = f;
	m_pState->m_iIgnoreDepthProjection = iIgnoreDepthProjection;
	return MakePair(fReturn, iReturn);
}

Float Poser::GetRenderThreshold(
	Float fLocalRenderWidth,
	Float fLocalRenderHeight,
	const Matrix2x3& mWorldTransform) const
{
	// Adjust the threshold for perspective projection, if enabled.
	auto const fDepth3D = m_pState->GetModifiedDepth3D();
	if (SEOUL_UNLIKELY(fDepth3D > 1e-4f))
	{
		Float const fW = m_pState->ComputeCurrentOneOverW();
		if (fLocalRenderWidth > fLocalRenderHeight)
		{
			return Vector2D(
				mWorldTransform.M00 * fLocalRenderWidth * m_pState->m_fWorldWidthToScreenWidth * fW,
				mWorldTransform.M10 * fLocalRenderWidth * m_pState->m_fWorldHeightToScreenHeight * fW).Length();
		}
		else
		{
			return Vector2D(
				mWorldTransform.M01 * fLocalRenderHeight * m_pState->m_fWorldWidthToScreenWidth * fW,
				mWorldTransform.M11 * fLocalRenderHeight * m_pState->m_fWorldHeightToScreenHeight * fW).Length();
		}
	}
	else
	{
		if (fLocalRenderWidth > fLocalRenderHeight)
		{
			return Vector2D(
				mWorldTransform.M00 * fLocalRenderWidth * m_pState->m_fWorldWidthToScreenWidth,
				mWorldTransform.M10 * fLocalRenderWidth * m_pState->m_fWorldHeightToScreenHeight).Length();
		}
		else
		{
			return Vector2D(
				mWorldTransform.M01 * fLocalRenderHeight * m_pState->m_fWorldWidthToScreenWidth,
				mWorldTransform.M11 * fLocalRenderHeight * m_pState->m_fWorldHeightToScreenHeight).Length();
		}
	}
}

void Poser::Pose(
	const Rectangle& worldRectangle,
	Renderable* pRenderable,
	const Matrix2x3& mWorld,
	const ColorTransformWithAlpha& cxWorld,
	const TextureReference& reference,
	const Rectangle& worldOcclusionRectangle,
	Feature::Enum eFeature,
	Int32 iSubRenderableId /*= -1*/)
{
	// Amend color add to the input feature based on the color transform.
	if (0 != cxWorld.m_AddR ||
		0 != cxWorld.m_AddG ||
		0 != cxWorld.m_AddB)
	{
		eFeature = (Feature::Enum)((Int)eFeature | Feature::kColorAdd);
	}

	auto& r = m_pState->m_Buffer.IssuePose();
	if (0 != m_pState->m_iInPlanarShadowRender)
	{
		r.m_vShadowPlaneWorldPosition = pRenderable->GetShadowPlaneWorldPosition();

		// Pre-project the shadow vertices if shadow rendering.
		auto const& shadow = m_pState->m_pStage3DSettings->m_Shadow;
		auto const vPlane = shadow.ComputeShadowPlane(r.m_vShadowPlaneWorldPosition);

		// Set the preclip rectangle.
		r.m_WorldRectanglePreClip = Rectangle::InverseMax();
		r.m_WorldRectanglePreClip.AbsorbPoint(
			shadow.ShadowProject(vPlane, Vector3D(worldRectangle.m_fLeft, worldRectangle.m_fTop, 0.0f)).GetXY());
		r.m_WorldRectanglePreClip.AbsorbPoint(
			shadow.ShadowProject(vPlane, Vector3D(worldRectangle.m_fLeft, worldRectangle.m_fBottom, 0.0f)).GetXY());
		r.m_WorldRectanglePreClip.AbsorbPoint(
			shadow.ShadowProject(vPlane, Vector3D(worldRectangle.m_fRight, worldRectangle.m_fTop, 0.0f)).GetXY());
		r.m_WorldRectanglePreClip.AbsorbPoint(
			shadow.ShadowProject(vPlane, Vector3D(worldRectangle.m_fRight, worldRectangle.m_fBottom, 0.0f)).GetXY());

		// Planar projected shadows cannot occlude.
		r.m_WorldOcclusionRectangle = Rectangle();
	}
	else
	{
		// Set the pre-clip rectangle.
		r.m_WorldRectanglePreClip = worldRectangle;

		// Can only occlude with high enough opacity and standard blending.
		if (cxWorld.m_fMulA >= kfOcclusionAlphaThreshold && 0 == cxWorld.m_uBlendingFactor)
		{
			r.m_WorldOcclusionRectangle = ClipAndProjectOcclusion(worldOcclusionRectangle);
		}
		else
		{
			r.m_WorldOcclusionRectangle = Rectangle();
		}
	}

	r.m_fDepth3D = m_pState->GetModifiedDepth3D();
	r.m_cxWorld = cxWorld;
	r.m_iSubRenderableId = iSubRenderableId;
	r.m_TextureReference = reference;
	r.m_mWorld = mWorld;
	r.m_pRenderable = pRenderable;
	r.m_WorldRectangle = ClipAndProjectWorldCull(r.m_WorldRectanglePreClip);
	r.m_iClip = m_pState->m_Buffer.GetClipStackTop();
	r.m_eFeature = (Feature::Enum)Max(eFeature, Feature::kColorMultiply);
}

/**
 * Variation of Pose for shapes that have variable 3D depth.
 * Call this function with the highest depth value, to
 * ensure world culling and occlusion rectangles are large
 * enough for accurate culling.
 */
void Poser::PoseWithFarthestDepth(
	Float fDepth3D,
	const Rectangle& worldRectangle,
	Renderable* pRenderable,
	const Matrix2x3& mWorld,
	const ColorTransformWithAlpha& cxWorld,
	const TextureReference& reference,
	const Rectangle& worldOcclusionRectangle,
	Feature::Enum eFeature,
	Int32 iSubRenderableId /*= -1*/)
{
	// Amend color add to the input feature based on the color transform.
	if (0 != cxWorld.m_AddR ||
		0 != cxWorld.m_AddG ||
		0 != cxWorld.m_AddB)
	{
		eFeature = (Feature::Enum)((Int)eFeature | Feature::kColorAdd);
	}

	// Capture actual 3D depth - expected to be 0.0,
	// since this is the fixed depth value, and
	// PoseWithFarthestDepth() is meant for shapes
	// that define their own per-vertex depth.
	auto const fPlanarDepth3D = m_pState->m_fRawDepth3D;

	// Now temporarily set the 3D depth to the max
	// of its current value and fDepth3D,
	// for perspective projection of culling rectangles.
	m_pState->m_fRawDepth3D = Max(fPlanarDepth3D, fDepth3D);

	auto& r = m_pState->m_Buffer.IssuePose();
	if (0 != m_pState->m_iInPlanarShadowRender)
	{
		r.m_vShadowPlaneWorldPosition = pRenderable->GetShadowPlaneWorldPosition();

		// Pre-project the shadow vertices if shadow rendering.
		auto const& shadow = m_pState->m_pStage3DSettings->m_Shadow;
		auto const vPlane = shadow.ComputeShadowPlane(r.m_vShadowPlaneWorldPosition);

		// Set the preclip rectangle.
		r.m_WorldRectanglePreClip = Rectangle::InverseMax();
		r.m_WorldRectanglePreClip.AbsorbPoint(
			shadow.ShadowProject(vPlane, Vector3D(worldRectangle.m_fLeft, worldRectangle.m_fTop, 0.0f)).GetXY());
		r.m_WorldRectanglePreClip.AbsorbPoint(
			shadow.ShadowProject(vPlane, Vector3D(worldRectangle.m_fLeft, worldRectangle.m_fBottom, 0.0f)).GetXY());
		r.m_WorldRectanglePreClip.AbsorbPoint(
			shadow.ShadowProject(vPlane, Vector3D(worldRectangle.m_fRight, worldRectangle.m_fTop, 0.0f)).GetXY());
		r.m_WorldRectanglePreClip.AbsorbPoint(
			shadow.ShadowProject(vPlane, Vector3D(worldRectangle.m_fRight, worldRectangle.m_fBottom, 0.0f)).GetXY());

		// Planar projected shadows cannot occlude.
		r.m_WorldOcclusionRectangle = Rectangle();
	}
	else
	{
		// Set the pre-clip rectangle.
		r.m_WorldRectanglePreClip = worldRectangle;

		// Can only occlude with high enough opacity and standard blending.
		if (cxWorld.m_fMulA >= kfOcclusionAlphaThreshold && 0 == cxWorld.m_uBlendingFactor)
		{
			r.m_WorldOcclusionRectangle = ClipAndProjectOcclusion(worldOcclusionRectangle);
		}
		else
		{
			r.m_WorldOcclusionRectangle = Rectangle();
		}
	}

	// Always use the modified planar depth for actual rendering.
	r.m_fDepth3D = (0 == m_pState->m_iIgnoreDepthProjection ? fPlanarDepth3D : 0.0f);
	r.m_cxWorld = cxWorld;
	r.m_iSubRenderableId = iSubRenderableId;
	r.m_TextureReference = reference;
	r.m_mWorld = mWorld;
	r.m_pRenderable = pRenderable;
	r.m_WorldRectangle = ClipAndProjectWorldCull(r.m_WorldRectanglePreClip);
	r.m_iClip = m_pState->m_Buffer.GetClipStackTop();
	r.m_eFeature = (Feature::Enum)Max(eFeature, Feature::kColorMultiply);

	// Restore the planar depth.
	m_pState->m_fRawDepth3D = fPlanarDepth3D;
}

#if SEOUL_ENABLE_CHEATS
Bool Poser::PoseInputVisualization(
	const Rectangle& worldRectangle,
	const Rectangle& inputBounds,
	const Matrix2x3& mWorld,
	RGBA color)
{
	if (!Intersects(worldRectangle, m_pState->m_WorldCullRectangle))
	{
		return false;
	}

	TextureReference reference;
	if (PoserResolveResult::kSuccess != ResolveTextureReference(worldRectangle, nullptr, 1.0f, FilePath(), reference))
	{
		return false;
	}

	ColorTransformWithAlpha cxWorld(ColorTransformWithAlpha::Identity());
	cxWorld.m_fMulR = color.m_R / 255.0f;
	cxWorld.m_fMulG = color.m_G / 255.0f;
	cxWorld.m_fMulB = color.m_B / 255.0f;
	cxWorld.m_fMulA = color.m_A / 255.0f;

	auto& r = m_pState->m_Buffer.IssuePoseInputVisualization();
	r.m_fDepth3D = m_pState->GetModifiedDepth3D();
	r.m_InputBounds = inputBounds;
	r.m_TextureReference = reference;
	r.m_cxWorld = cxWorld;
	r.m_mWorld = mWorld;
	r.m_WorldRectangle = ClipAndProjectWorldCull(worldRectangle);
	r.m_WorldRectanglePreClip = worldRectangle;
	r.m_iClip = m_pState->m_Buffer.GetClipStackTop();

	return true;
}
#endif // /#if SEOUL_ENABLE_CHEATS

PoserResolveResult Poser::ResolveTextureReference(
	const Rectangle& worldRectangle,
	Renderable const* pRenderable,
	Float fRenderThreshold,
	const FilePath& filePath,
	TextureReference& rTextureReference,
	Bool bPrefetch /*= false*/,
	Bool bUsePacked /* = true*/)
{
	if (!Intersects(worldRectangle, m_pState->m_WorldCullRectangle))
	{
		// If prefetching, perform that now.
		if (bPrefetch)
		{
			if (m_pState->m_pCache->Prefetch(fRenderThreshold, filePath))
			{
				return PoserResolveResult::kCulledAndPrefetched;
			}
		}

		return PoserResolveResult::kCulled;
	}

	if (0 != m_pState->m_iInPlanarShadowRender)
	{
		if (nullptr == pRenderable || !pRenderable->CastShadow())
		{
			// If prefetching, perform that now.
			if (bPrefetch)
			{
				if (m_pState->m_pCache->Prefetch(fRenderThreshold, filePath))
				{
					return PoserResolveResult::kCulledAndPrefetched;
				}
			}

			return PoserResolveResult::kCulled;
		}

		fRenderThreshold *= m_pState->m_pStage3DSettings->m_Shadow.GetResolutionScale();
	}

	Bool const bReturn = m_pState->m_pCache->ResolveTextureReference(
		fRenderThreshold,
		filePath,
		rTextureReference,
		bUsePacked);
	return (bReturn ? PoserResolveResult::kSuccess : PoserResolveResult::kNotReady);
}

Bool Poser::InPlanarShadow() const
{
	return 0 != m_pState->m_iInPlanarShadowRender;
}

PoserResolveResult Poser::ResolveTextureReference(
	const Rectangle& worldRectangle,
	Renderable const* pRenderable,
	Float fRenderThreshold,
	const SharedPtr<BitmapDefinition const>& p,
	TextureReference& rTextureReference,
	Bool bPrefetch /*= false*/,
	Bool bUsePacked /*= true*/)
{
	if (!Intersects(worldRectangle, m_pState->m_WorldCullRectangle))
	{
		// If prefetching, perform that now.
		if (bPrefetch && p.IsValid() && p->GetFilePath().IsValid())
		{
			if (m_pState->m_pCache->Prefetch(fRenderThreshold, p->GetFilePath()))
			{
				return PoserResolveResult::kCulledAndPrefetched;
			}
		}

		return PoserResolveResult::kCulled;
	}

	if (0 != m_pState->m_iInPlanarShadowRender)
	{
		if (nullptr == pRenderable || !pRenderable->CastShadow())
		{
			// If prefetching, perform that now.
			if (bPrefetch && p.IsValid() && p->GetFilePath().IsValid())
			{
				if (m_pState->m_pCache->Prefetch(fRenderThreshold, p->GetFilePath()))
				{
					return PoserResolveResult::kCulledAndPrefetched;
				}
			}

			return PoserResolveResult::kCulled;
		}

		fRenderThreshold *= m_pState->m_pStage3DSettings->m_Shadow.GetResolutionScale();
	}

	Bool const bReturn = m_pState->m_pCache->ResolveTextureReference(fRenderThreshold, p, rTextureReference, bUsePacked);
	return (bReturn ? PoserResolveResult::kSuccess : PoserResolveResult::kNotReady);
}

/**
 * Called on occlusion rectangles
 * to account for masking/clipping and 3D projection, for
 * accurate visibility and occlusion tests.
 */
Rectangle Poser::ClipAndProjectOcclusion(const Rectangle& rect) const
{
	// Need to clip then project.
	if (m_pState->m_pClipStack->HasClips())
	{
		// Get the top state of the clip stack.
		auto const& top = m_pState->m_pClipStack->GetTopClip();

		// TODO: Anyway to support other more complex clips?

		// If the clipping is not simple, just disable occlusion on this shape.
		if (!top.m_bSimple)
		{
			return Rectangle();
		}

		// Clip the shape.
		auto const& clip = top.m_Bounds;
		auto ret = Rectangle::Create(
			Max(clip.m_fLeft, rect.m_fLeft),
			Min(clip.m_fRight, rect.m_fRight),
			Max(clip.m_fTop, rect.m_fTop),
			Min(clip.m_fBottom, rect.m_fBottom));

		// Now project the clipped shape.
		return DepthProject(ret);
	}
	// Otherwise, just depth project the shape.
	else
	{
		return DepthProject(rect);
	}
}

/**
 * Called on world cull rectangles
 * to account for masking/clipping and 3D projection, for
 * accurate visibility and occlusion tests.
 */
Rectangle Poser::ClipAndProjectWorldCull(const Rectangle& rect) const
{
	// Need to clip then project.
	if (m_pState->m_pClipStack->HasClips())
	{
		// Get the top state of the clip stack.
		auto const& top = m_pState->m_pClipStack->GetTopClip();

		// TODO: Anyway to support other more complex clips?

		// If the clipping is not simple, just project the unclipped shape.
		// Because the cull shape can only get smaller with clipping,
		// using the unclipped shape is more conservative, not less, and
		// is therefore ok.
		if (!top.m_bSimple)
		{
			return DepthProject(rect);
		}

		// Clip the shape.
		auto const& clip = top.m_Bounds;
		auto ret = Rectangle::Create(
			Max(clip.m_fLeft, rect.m_fLeft),
			Min(clip.m_fRight, rect.m_fRight),
			Max(clip.m_fTop, rect.m_fTop),
			Min(clip.m_fBottom, rect.m_fBottom));

		// Now project the clipped shape.
		return DepthProject(ret);
	}
	// Otherwise, just depth project the shape.
	else
	{
		return DepthProject(rect);
	}
}

Vector2D Poser::InverseDepthProject(Float fWorldX, Float fWorldY) const
{
	auto const fDepth3D = m_pState->GetModifiedDepth3D();
	if (SEOUL_UNLIKELY(fDepth3D > 1e-4f))
	{
		return m_pState->InverseProject(Vector2D(fWorldX, fWorldY));
	}
	else
	{
		return Vector2D(fWorldX, fWorldY);
	}
}

/** Shared utility of both culling rectangle clips. */
Rectangle Poser::DepthProject(const Rectangle& rect) const
{
	auto const fDepth3D = m_pState->GetModifiedDepth3D();
	if (SEOUL_UNLIKELY(fDepth3D > 1e-4f))
	{
		auto const vLT = m_pState->Project(Vector2D(rect.m_fLeft, rect.m_fTop));
		auto const vRB = m_pState->Project(Vector2D(rect.m_fRight, rect.m_fBottom));

		return Rectangle::Create(vLT.X, vRB.X, vLT.Y, vRB.Y);
	}
	else
	{
		return rect;
	}
}

} // namepsace Render

/**
 * Utility for computing occlusion rectangles. The input
 * rectangle is expected to be the tight fitting bounds
 * of the occlusion shape (the shape is a quad
 * and the corners line up with texture coordinates
 * of (0, 0) and (1, 1).
 */
Rectangle ComputeOcclusionRectangle(
	const Matrix2x3& m,
	const TextureReference& reference,
	const Rectangle& bounds)
{
	// Compute object space occlusion rectangle.
	Float const fWidth = bounds.GetWidth();
	Float const fHeight = bounds.GetHeight();

	Float const fU0 = reference.m_vOcclusionOffset.X;
	Float const fV0 = reference.m_vOcclusionOffset.Y;
	Float const fU1 = reference.m_vOcclusionOffset.X + reference.m_vOcclusionScale.X;
	Float const fV1 = reference.m_vOcclusionOffset.Y + reference.m_vOcclusionScale.Y;

	Rectangle ret;
	ret.m_fLeft = bounds.m_fLeft + (fU0 * fWidth);
	ret.m_fTop = bounds.m_fTop + (fV0 * fHeight);
	ret.m_fRight = bounds.m_fLeft + (fU1 * fWidth);
	ret.m_fBottom = bounds.m_fTop + (fV1 * fHeight);

	// Transform, check matches bounds.
	Bool bMatchesBounds = false;
	ret = TransformRectangle(m, ret, bMatchesBounds);

	// Occlusion rectangle is only valid if the transformed result is axis aligned.
	// In all other cases, make the rectangle zero sized.
	if (!bMatchesBounds)
	{
		ret = Rectangle();
	}

	return ret;
}
Rectangle ComputeOcclusionRectangle(
	const Matrix2x3& m,
	const TextureReference& reference,
	const Matrix2x3& mOcclusionTransform)
{
	Rectangle ret;
	ret.m_fLeft = reference.m_vOcclusionOffset.X;
	ret.m_fTop = reference.m_vOcclusionOffset.Y;
	ret.m_fRight = reference.m_vOcclusionOffset.X + reference.m_vOcclusionScale.X;
	ret.m_fBottom = reference.m_vOcclusionOffset.Y + reference.m_vOcclusionScale.Y;

	// Transform, check matches bounds.
	Bool bMatchesBounds = false;
	ret = TransformRectangle(m * mOcclusionTransform, ret, bMatchesBounds);

	// Occlusion rectangle is only valid if the transformed result is axis aligned.
	// In all other cases, make the rectangle zero sized.
	if (!bMatchesBounds)
	{
		ret = Rectangle();
	}

	return ret;
}

} // namespace Seoul::Falcon
