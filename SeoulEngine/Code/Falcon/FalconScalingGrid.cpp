/**
 * \file FalconScalingGrid.cpp
 * \brief Utility, handles decomposition of a mesh into 9 pieces
 * for 9-slice scaling and render.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FalconClipper.h"
#include "FalconRenderDrawer.h"
#include "FalconScalingGrid.h"

namespace Seoul::Falcon
{

/**
 * @return m with horizontal scaling removed, and a local translation applied to
 * match the reference point. This is the correct transform for the left and right
 * vertical sides of 9-slice scaling.
 */
static inline Matrix2x3 GetNoHorizontalScale(
	const Matrix2x3& m,
	Float fReferencePointX,
	Bool bShrinkX)
{
	if (bShrinkX)
	{
		return m;
	}
	else
	{
		// Decompose the transform into its non-orthonormal scaling/skew pre-rotation
		// part, and its rotation part.
		Matrix2D mPreRotation;
		Matrix2D mRotation;
		Matrix2D::Decompose(m.GetUpper2x2(), mPreRotation, mRotation);

		// The returned transform contains rotation and vertical scale.
		Matrix2x3 mReturn(m);
		mReturn.SetUpper2x2(mRotation * Matrix2D::CreateScale(1.0f, mPreRotation.M11));
		mReturn = mReturn * Matrix2x3::CreateTranslation(mPreRotation.M00 * fReferencePointX - fReferencePointX, 0.0f);
		return mReturn;
	}
}

/**
 * @return m with all scaling removed, and a local translation applied to match
 * vReferencePoint. This is the correct transform to use for the 4 corners
 * of 9-slice scaling.
 */
static inline Matrix2x3 GetNoScale(
	const Matrix2x3& m,
	const Vector2D& vReferencePoint,
	Bool bShrinkX,
	Bool bShrinkY)
{
	if (bShrinkX && bShrinkY)
	{
		return m;
	}
	else
	{
		// Decompose the transform into its non-orthonormal scaling/skew pre-rotation
		// part, and its rotation part.
		Matrix2D mPreRotation;
		Matrix2D mRotation;
		Matrix2D::Decompose(m.GetUpper2x2(), mPreRotation, mRotation);

		// Kill scaling parts unless shrinking along that axis.
		Matrix2D mCancelledScale(mPreRotation);
		if (!bShrinkX) { mCancelledScale.M00 = 1.0f; mCancelledScale.M01 = 0.0f; }
		if (!bShrinkY) { mCancelledScale.M10 = 0.0f; mCancelledScale.M11 = 1.0f; }

		// The return transform contains only rotation.
		Matrix2x3 mReturn(m);
		mReturn.SetUpper2x2(mRotation * mCancelledScale);
		mReturn = mReturn * Matrix2x3::CreateTranslation(
			bShrinkX ? 0.0f : (mPreRotation.M00 * vReferencePoint.X - vReferencePoint.X),
			bShrinkY ? 0.0f : (mPreRotation.M11 * vReferencePoint.Y - vReferencePoint.Y));
		return mReturn;
	}
}

/**
 * @return m with vertical scaling removed, and a local translation applied to
 * match the reference point. This is the correct transform for the top and bottom
 * horizontal sides of 9-slice scaling.
 */
static inline Matrix2x3 GetNoVerticalScale(
	const Matrix2x3& m,
	Float fReferencePointY,
	Bool bShrinkY)
{
	if (bShrinkY)
	{
		return m;
	}
	else
	{
		// Decompose the transform into its non-orthonormal scaling/skew pre-rotation
		// part, and its rotation part.
		Matrix2D mPreRotation;
		Matrix2D mRotation;
		Matrix2D::Decompose(m.GetUpper2x2(), mPreRotation, mRotation);

		// The returned transform contains rotation and horizontal scale.
		Matrix2x3 mReturn(m);
		mReturn.SetUpper2x2(mRotation * Matrix2D::CreateScale(mPreRotation.M00, 1.0f));
		mReturn = mReturn * Matrix2x3::CreateTranslation(0.0f, mPreRotation.M11 * fReferencePointY - fReferencePointY);
		return mReturn;
	}
}

ScalingGrid::ScalingGrid(Render::Drawer& r)
	: m_r(r)
	, m_pMeshClipCache(Clipper::NewMeshClipCache<StandardVertex2D>())
	, m_vClipI()
	, m_vClipV()
	, m_vWorkArea()
{
}

ScalingGrid::~ScalingGrid()
{
	Clipper::DestroyMeshClipCache(m_pMeshClipCache);
}

/**
 * Used to compute an adjusted transform for cases where runtime size of
 * a 9-slice element does not match the same element as displayed in Flash.
 *
 * This occurs when the engine can 9-slice elements (e.g. Bitmaps) that cannot
 * be 9-sliced in Flash.
 */
static inline Matrix2x3 ComputeAdjustedScalingGridTransform(
	const Matrix2x3& m,
	const Rectangle& targetBounds,
	const Rectangle& scalingGrid,
	Bool& rbShrinkX,
	Bool& rbShrinkY)
{
	auto const vTargetUL = Matrix2x3::TransformPosition(m, Vector2D(targetBounds.m_fLeft, targetBounds.m_fTop));
	auto const vTargetUR = Matrix2x3::TransformPosition(m, Vector2D(targetBounds.m_fRight, targetBounds.m_fTop));
	auto const vTargetBL = Matrix2x3::TransformPosition(m, Vector2D(targetBounds.m_fLeft, targetBounds.m_fBottom));

	auto const vGridUL = Matrix2x3::TransformPosition(m, Vector2D(scalingGrid.m_fLeft, scalingGrid.m_fTop));
	auto const vGridUR = Matrix2x3::TransformPosition(m, Vector2D(scalingGrid.m_fRight, scalingGrid.m_fTop));
	auto const vGridBL = Matrix2x3::TransformPosition(m, Vector2D(scalingGrid.m_fLeft, scalingGrid.m_fBottom));

	auto const fTargetWidth = (vTargetUR - vTargetUL).Length();
	auto const fTargetHeight = (vTargetUL - vTargetBL).Length();
	auto const fGridWidth = (vGridUR - vGridUL).Length();
	auto const fGridHeight = (vGridUL - vGridBL).Length();

	auto const fMinTargetWidth = targetBounds.GetWidth();
	auto const fMinTargetHeight = targetBounds.GetHeight();

	// If scaling will make the object smaller along an axis than its
	// 9-slicing will allow, account for this. We apply no adjustment in this case.
	rbShrinkX = (fTargetWidth < fMinTargetWidth);
	rbShrinkY = (fTargetHeight < fMinTargetHeight);

	// Early out, just return m if shrinking in both directions.
	if (rbShrinkX && rbShrinkY)
	{
		return m;
	}

	Float fScaleX = 1.0f;
	Float fScaleY = 1.0f;
	if (!rbShrinkX)
	{
		auto const fBorderWidth = Max(fMinTargetWidth - scalingGrid.GetWidth(), 0.0f);
		auto const fOneOverGridWidth = (IsZero(fGridWidth) ? 0.0f : (1.0f / fGridWidth));
		fScaleX = Max(fTargetWidth - fBorderWidth, 1e-3f) * fOneOverGridWidth;
	}

	if (!rbShrinkY)
	{
		auto const fBorderHeight = Max(fMinTargetHeight - scalingGrid.GetHeight(), 0.0f);
		auto const fOneOverGridHeight = (IsZero(fGridHeight) ? 0.0f : (1.0f / fGridHeight));
		fScaleY = Max(fTargetHeight - fBorderHeight, 1e-3f) * fOneOverGridHeight;
	}

	auto const vCenter = scalingGrid.GetCenter();
	auto const mAdjustment =
		Matrix2x3::CreateTranslation(vCenter) *
		Matrix2x3::CreateScale(fScaleX, fScaleY) *
		Matrix2x3::CreateTranslation(-vCenter);
	return (m * mAdjustment);
}

void ScalingGrid::DrawTriangleList(
	const Rectangle& scalingGrid,
	const Rectangle& worldBoundsPreClip,
	const TextureReference& textureReference,
	const Matrix2x3& mInParentTransform,
	const Matrix2x3& mChildTransform,
	const ColorTransformWithAlpha& cxWorld,
	const Rectangle& localBounds,
	UInt16 const* pIndices,
	UInt32 uIndexCount,
	ShapeVertex const* pVertices,
	UInt32 uVertexCount,
	TriangleListDescription eDescription,
	Render::Feature::Enum eFeature)
{
	// Early out if nothing to process.
	if (uVertexCount == 0u || uIndexCount == 0u)
	{
		return;
	}

	// Falcon supports 9-slicing of Bitmaps, Flash does not. As a result, we need
	// to adjust the transform when 9-slicing Bitmaps, as Flash renders without the 9-slicing.
	// Without this correction, what the author sees in Flash will be bigger than what
	// she sees in game (because in Flash, the edge and corners will scale, but in-engine,
	// they will not).
	//
	// To account for this, we increase the scaling, so the outer edge of the corners
	// and edges of the 9-slice match the outer edge that would exist if the shape
	// was scaled normally.
	//
	// We need to use the shape rectangle from Flash for this, not the individual
	// computed shape bounds.
	Bool bShrinkX = false;
	Bool bShrinkY = false;
	auto const mParentTransform = ComputeAdjustedScalingGridTransform(
		mInParentTransform,
		TransformRectangle(mChildTransform, localBounds),
		scalingGrid,
		bShrinkX,
		bShrinkY);

	// Easy early out case, just use the original transform and draw normally.
	if (bShrinkX && bShrinkY)
	{
		m_r.DrawTriangleList(
			worldBoundsPreClip,
			textureReference,
			(mInParentTransform * mChildTransform),
			cxWorld,
			pIndices,
			uIndexCount,
			pVertices,
			uVertexCount,
			eDescription,
			eFeature);
		return;
	}

	// Populate the work area with vertices - we need
	// to transform the vertices into the parent's space for
	// further processing.
	m_vWorkArea.ResizeNoInitialize(uVertexCount);
	memcpy(m_vWorkArea.Data(), pVertices, m_vWorkArea.GetSizeInBytes());

	// Transform all vertices into parent space. Also compute the bounding rectangle
	// of the vertices in the parent's space.
	Rectangle bounding(Rectangle::InverseMax());
	{
		for (auto i = m_vWorkArea.Begin(); m_vWorkArea.End() != i; ++i)
		{
			i->m_vP = Matrix2x3::TransformPosition(mChildTransform, i->m_vP);
			bounding.AbsorbPoint(i->m_vP);
		}
	}

	// Also create an oversized bounds for no clip - these are the bounds we want
	// to use to avoid clipping the shape's vertices. The offset here is arbitrary,
	// any value > 1 should work.
	Rectangle noClip(bounding);
	noClip.Expand(2.0f);

	// Center.
	{
		m_vClipI.Assign(pIndices, pIndices + uIndexCount);
		m_vClipV = m_vWorkArea;
		Clipper::MeshClip(*m_pMeshClipCache, scalingGrid, eDescription, m_vClipI, m_vClipI.GetSize(), m_vClipV, m_vClipV.GetSize());

		if (!m_vClipI.IsEmpty())
		{
			m_r.DrawTriangleList(
				worldBoundsPreClip,
				textureReference,
				mParentTransform,
				cxWorld,
				m_vClipI.Data(),
				m_vClipI.GetSize(),
				m_vClipV.Data(),
				m_vClipV.GetSize(),
				eDescription,
				eFeature);
		}
	}

	// Left-right sides.
	{
		// Left
		{
			Matrix2x3 const m(GetNoHorizontalScale(mParentTransform, scalingGrid.m_fLeft, bShrinkX));

			Rectangle const clipRectangle(Rectangle::Create(noClip.m_fLeft, scalingGrid.m_fLeft, scalingGrid.m_fTop, scalingGrid.m_fBottom));
			m_vClipI.Assign(pIndices, pIndices + uIndexCount);
			m_vClipV = m_vWorkArea;
			Clipper::MeshClip(*m_pMeshClipCache, clipRectangle, eDescription, m_vClipI, m_vClipI.GetSize(), m_vClipV, m_vClipV.GetSize());

			if (!m_vClipI.IsEmpty())
			{
				m_r.DrawTriangleList(
					worldBoundsPreClip,
					textureReference,
					m,
					cxWorld,
					m_vClipI.Data(),
					m_vClipI.GetSize(),
					m_vClipV.Data(),
					m_vClipV.GetSize(),
					eDescription,
					eFeature);
			}
		}

		// Right
		{
			Matrix2x3 const m(GetNoHorizontalScale(mParentTransform, scalingGrid.m_fRight, bShrinkX));

			Rectangle const clipRectangle(Rectangle::Create(scalingGrid.m_fRight, noClip.m_fRight, scalingGrid.m_fTop, scalingGrid.m_fBottom));
			m_vClipI.Assign(pIndices, pIndices + uIndexCount);
			m_vClipV = m_vWorkArea;
			Clipper::MeshClip(*m_pMeshClipCache, clipRectangle, eDescription, m_vClipI, m_vClipI.GetSize(), m_vClipV, m_vClipV.GetSize());

			if (!m_vClipI.IsEmpty())
			{
				m_r.DrawTriangleList(
					worldBoundsPreClip,
					textureReference,
					m,
					cxWorld,
					m_vClipI.Data(),
					m_vClipI.GetSize(),
					m_vClipV.Data(),
					m_vClipV.GetSize(),
					eDescription,
					eFeature);
			}
		}
	}

	// Top-bottom sides.
	{
		// Top
		{
			Matrix2x3 const m(GetNoVerticalScale(mParentTransform, scalingGrid.m_fTop, bShrinkY));

			Rectangle const clipRectangle(Rectangle::Create(scalingGrid.m_fLeft, scalingGrid.m_fRight, noClip.m_fTop, scalingGrid.m_fTop));
			m_vClipI.Assign(pIndices, pIndices + uIndexCount);
			m_vClipV = m_vWorkArea;
			Clipper::MeshClip(*m_pMeshClipCache, clipRectangle, eDescription, m_vClipI, m_vClipI.GetSize(), m_vClipV, m_vClipV.GetSize());

			if (!m_vClipI.IsEmpty())
			{
				m_r.DrawTriangleList(
					worldBoundsPreClip,
					textureReference,
					m,
					cxWorld,
					m_vClipI.Data(),
					m_vClipI.GetSize(),
					m_vClipV.Data(),
					m_vClipV.GetSize(),
					eDescription,
					eFeature);
			}
		}

		// Bottom
		{
			Matrix2x3 const m(GetNoVerticalScale(mParentTransform, scalingGrid.m_fBottom, bShrinkY));

			Rectangle const clipRectangle(Rectangle::Create(scalingGrid.m_fLeft, scalingGrid.m_fRight, scalingGrid.m_fBottom, noClip.m_fBottom));
			m_vClipI.Assign(pIndices, pIndices + uIndexCount);
			m_vClipV = m_vWorkArea;
			Clipper::MeshClip(*m_pMeshClipCache, clipRectangle, eDescription, m_vClipI, m_vClipI.GetSize(), m_vClipV, m_vClipV.GetSize());

			if (!m_vClipI.IsEmpty())
			{
				m_r.DrawTriangleList(
					worldBoundsPreClip,
					textureReference,
					m,
					cxWorld,
					m_vClipI.Data(),
					m_vClipI.GetSize(),
					m_vClipV.Data(),
					m_vClipV.GetSize(),
					eDescription,
					eFeature);
			}
		}
	}

	// Corners
	{
		// TL
		{
			Matrix2x3 const m(GetNoScale(mParentTransform, Vector2D(scalingGrid.m_fLeft, scalingGrid.m_fTop), bShrinkX, bShrinkY));

			Rectangle const clipRectangle(Rectangle::Create(noClip.m_fLeft, scalingGrid.m_fLeft, noClip.m_fTop, scalingGrid.m_fTop));
			m_vClipI.Assign(pIndices, pIndices + uIndexCount);
			m_vClipV = m_vWorkArea;
			Clipper::MeshClip(*m_pMeshClipCache, clipRectangle, eDescription, m_vClipI, m_vClipI.GetSize(), m_vClipV, m_vClipV.GetSize());

			if (!m_vClipI.IsEmpty())
			{
				m_r.DrawTriangleList(
					worldBoundsPreClip,
					textureReference,
					m,
					cxWorld,
					m_vClipI.Data(),
					m_vClipI.GetSize(),
					m_vClipV.Data(),
					m_vClipV.GetSize(),
					eDescription,
					eFeature);
			}
		}

		// TR
		{
			Matrix2x3 const m(GetNoScale(mParentTransform, Vector2D(scalingGrid.m_fRight, scalingGrid.m_fTop), bShrinkX, bShrinkY));

			Rectangle const clipRectangle(Rectangle::Create(scalingGrid.m_fRight, noClip.m_fRight, noClip.m_fTop, scalingGrid.m_fTop));
			m_vClipI.Assign(pIndices, pIndices + uIndexCount);
			m_vClipV = m_vWorkArea;
			Clipper::MeshClip(*m_pMeshClipCache, clipRectangle, eDescription, m_vClipI, m_vClipI.GetSize(), m_vClipV, m_vClipV.GetSize());

			if (!m_vClipI.IsEmpty())
			{
				m_r.DrawTriangleList(
					worldBoundsPreClip,
					textureReference,
					m,
					cxWorld,
					m_vClipI.Data(),
					m_vClipI.GetSize(),
					m_vClipV.Data(),
					m_vClipV.GetSize(),
					eDescription,
					eFeature);
			}
		}

		// BL
		{
			Matrix2x3 const m(GetNoScale(mParentTransform, Vector2D(scalingGrid.m_fLeft, scalingGrid.m_fBottom), bShrinkX, bShrinkY));

			Rectangle const clipRectangle(Rectangle::Create(noClip.m_fLeft, scalingGrid.m_fLeft, scalingGrid.m_fBottom, noClip.m_fBottom));
			m_vClipI.Assign(pIndices, pIndices + uIndexCount);
			m_vClipV = m_vWorkArea;
			Clipper::MeshClip(*m_pMeshClipCache, clipRectangle, eDescription, m_vClipI, m_vClipI.GetSize(), m_vClipV, m_vClipV.GetSize());

			if (!m_vClipI.IsEmpty())
			{
				m_r.DrawTriangleList(
					worldBoundsPreClip,
					textureReference,
					m,
					cxWorld,
					m_vClipI.Data(),
					m_vClipI.GetSize(),
					m_vClipV.Data(),
					m_vClipV.GetSize(),
					eDescription,
					eFeature);
			}
		}

		// BR
		{
			Matrix2x3 const m(GetNoScale(mParentTransform, Vector2D(scalingGrid.m_fRight, scalingGrid.m_fBottom), bShrinkX, bShrinkY));

			Rectangle const clipRectangle(Rectangle::Create(scalingGrid.m_fRight, noClip.m_fRight, scalingGrid.m_fBottom, noClip.m_fBottom));
			m_vClipI.Assign(pIndices, pIndices + uIndexCount);
			m_vClipV = m_vWorkArea;
			Clipper::MeshClip(*m_pMeshClipCache, clipRectangle, eDescription, m_vClipI, m_vClipI.GetSize(), m_vClipV, m_vClipV.GetSize());

			if (!m_vClipI.IsEmpty())
			{
				m_r.DrawTriangleList(
					worldBoundsPreClip,
					textureReference,
					m,
					cxWorld,
					m_vClipI.Data(),
					m_vClipI.GetSize(),
					m_vClipV.Data(),
					m_vClipV.GetSize(),
					eDescription,
					eFeature);
			}
		}
	}
}

} // namespace Seoul::Falcon
