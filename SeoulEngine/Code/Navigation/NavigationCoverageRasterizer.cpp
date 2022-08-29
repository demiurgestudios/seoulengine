/**
 * \file CoverageRasterizer.cpp
 * 1-bit coverage rasterizer, intended for projection of 3D geometry
 * onto a 2D navigation grid.
 *
 * \sa https://github.com/GameTechDev/OcclusionCulling/blob/master/TransformedAABBoxScalar.cpp
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "NavigationCoverageRasterizer.h"
#include "NavigationGrid.h"

#if SEOUL_WITH_NAVIGATION

namespace Seoul::Navigation
{

// Sanity check - necessary to avoid integer overflow with an Int32.
SEOUL_STATIC_ASSERT((Int32)Grid::kuMaxDimension * (Int32)CoverageRasterizer::kiRasterRes <= 16384);

CoverageRasterizer::CoverageRasterizer(
	UInt32 uWidth,
	UInt32 uHeight,
	const Vector3D& vWorldOrigin,
	Float32 const* pfHeightData,
	Axis eUpAxis /*= Axis::kZ */)
	: m_vSurface((UInt32)((uWidth * kiRasterRes) * (uHeight * kiRasterRes)), false)
	, m_vfHeightData(pfHeightData, pfHeightData + (UInt32)((uWidth * kiRasterRes) * (uHeight * kiRasterRes)))
	, m_uWidth(uWidth)
	, m_uHeight(uHeight)
	, m_vWorldOrigin(vWorldOrigin)
	, m_eUpAxis(eUpAxis)
{
}

/**
 * Apply the current state of the rasterizer to
 * a Grid. If the grid's dimensions do not
 * match the rasterizer, output will be clamped as needed.
 */
void CoverageRasterizer::ApplyToGrid(
	Grid& grid,
	UInt32 uMinimumSampleCount,
	UInt32 uBitToSet) const
{
	UInt32 const uMinWidth = Min(grid.GetWidth(), m_uWidth);
	UInt32 const uMinHeight = Min(grid.GetHeight(), m_uHeight);

	// SetValue is all 0, with a 1 set at the specified bit.
	UInt8 const uSetValue = (UInt8)(1u << uBitToSet);
	for (UInt32 uY = 0u; uY < uMinHeight; ++uY)
	{
		for (UInt32 uX = 0u; uX < uMinWidth; ++uX)
		{
			// Cache sample count.
			UInt32 const uSampleCount = GetSampleCount(uX, uY);

			// Get existing cell value.
			UInt8 uCellValue = grid.GetCell(uX, uY);

			// Set or unset the bit based on sample
			if (uSampleCount >= uMinimumSampleCount)
			{
				uCellValue |= uSetValue;
			}
			else
			{
				uCellValue &= ~uSetValue;
			}
			grid.SetCell(uX, uY, uCellValue);
		}
	}
}

/** Rasterize a triangle into a pixel buffer to determine coverage. */
void CoverageRasterizer::RasterizeTriangle(
	const Vector3D& v0,
	const Vector3D& v1,
	const Vector3D& v2)
{
	// Quantize the world positions into our 2D pixel space.
	Point const p0 = Quantize(v0);
	Point p1 = Quantize(v1);
	Point p2 = Quantize(v2);

	// Compute signed area of the triangle.
	Int32 const iSignedArea = ComputeSignedArea(p0, p1, p2);

	// Early out if degenerate.
	if (iSignedArea <= 0)
	{
		return;
	}

	// Compute A/B edge deltas and barycentric factors of the triangle.
	Int32 const iA0 = (p1.m_iY - p2.m_iY);
	Int32 const iA1 = (p2.m_iY - p0.m_iY);
	Int32 const iA2 = (p0.m_iY - p1.m_iY);
	Int32 const iB0 = (p2.m_iX - p1.m_iX);
	Int32 const iB1 = (p0.m_iX - p2.m_iX);
	Int32 const iB2 = (p1.m_iX - p0.m_iX);
	Int32 const iC0 = (p1.m_iX * p2.m_iY) - (p2.m_iX * p1.m_iY);
	Int32 const iC1 = (p2.m_iX * p0.m_iY) - (p0.m_iX * p2.m_iY);
	Int32 const iC2 = (p0.m_iX * p1.m_iY) - (p1.m_iX * p0.m_iY);

	// Transform height on 02 and 01 edges into a normalized factor
	// used for barycentric interpolation.
	Float32 const fInverseArea = (1.0f / (Float32)iSignedArea);
	p1.m_fHeightFactor = (p1.m_fHeightFactor - p0.m_fHeightFactor) * fInverseArea;
	p2.m_fHeightFactor = (p2.m_fHeightFactor - p0.m_fHeightFactor) * fInverseArea;

	// Compute triangle bounding box
	Int32 iMinX = Min(p0.m_iX, p1.m_iX, p2.m_iX);
	Int32 iMinY = Min(p0.m_iY, p1.m_iY, p2.m_iY);
	Int32 iMaxX = Max(p0.m_iX, p1.m_iX, p2.m_iX);
	Int32 iMaxY = Max(p0.m_iY, p1.m_iY, p2.m_iY);

	// Cache pixel width/height.
	Int32 const iPixelWidth = (Int32)m_uWidth * kiRasterRes;
	Int32 const iPixelHeight = (Int32)m_uHeight * kiRasterRes;

	// Clip against grid bounds
	iMinX = Max(iMinX, 0);
	iMinY = Max(iMinY, 0);
	iMaxX = Min(iMaxX, iPixelWidth - 1);
	iMaxY = Min(iMaxY, iPixelHeight - 1);

	// Early out if entirely clipped.
	if (iMaxX < iMinX || iMaxY < iMinY)
	{
		return;
	}

	// Compute whether edges are top-left.
	Bool const bAlphaTopLeft = IsTopLeft(p1, p2);
	Bool const bBetaTopLeft = IsTopLeft(p2, p0);
	Bool const bGammaTopLeft = IsTopLeft(p0, p1);

	// Rasterize
	Int32 iRowIndex = (iMinY * iPixelWidth + iMinX);
	Int32 iCol = iMinX;
	Int32 iRow = iMinY;

	// Advancement of edge terms used to evaluate barycentric coordinates.
	Int32 iAlpha0 = (iA0 * iCol) + (iB0 * iRow) + iC0;
	Int32 iBeta0 = (iA1 * iCol) + (iB1 * iRow) + iC1;
	Int32 iGamma0 = (iA2 * iCol) + (iB2 * iRow) + iC2;

	// Height factor change per X step.
	Float32 fDeltaHeight = (iA1 * p1.m_fHeightFactor) + (iA2 * p2.m_fHeightFactor);

	// Iterate on the bounding box of the triangle, using barycentric
	// weights to determine if a pixel is on the triangle or not.
	Point p;
	for (p.m_iY = iMinY; p.m_iY <= iMaxY;
		p.m_iY++,
		iRow++,
		iRowIndex = iRowIndex + iPixelWidth,
		iAlpha0 += iB0,
		iBeta0 += iB1,
		iGamma0 += iB2)
	{
		// Compute barycentric coordinates at this pixel.
		Int32 iIndex = iRowIndex;
		Int32 iAlpha = iAlpha0;
		Int32 iBeta = iBeta0;
		Int32 iGamma = iGamma0;

		// Current height value - advanced by delta height.
		Float32 fHeight = p0.m_fHeightFactor + (p1.m_fHeightFactor * iBeta) + (p2.m_fHeightFactor * iGamma);
		for (p.m_iX = iMinX; p.m_iX <= iMaxX;
			p.m_iX++,
			iIndex++,
			iAlpha += iA0,
			iBeta += iA1,
			iGamma += iA2,
			fHeight += fDeltaHeight)
		{
			// Mask value determines if pixel is on the triangle.
			Int32 iMask = (iAlpha | iBeta | iGamma);
			if (iMask > 0)
			{
				// Evaluate fill rules - if we're on an edge, the
				// corresponding edge must be a top or left edge
				// to render the pixel.
				if (0 == iAlpha)
				{
					if (!bAlphaTopLeft)
					{
						continue;
					}
				}
				else if (0 == iBeta)
				{
					if (!bBetaTopLeft)
					{
						continue;
					}
				}
				else if (0 == iGamma)
				{
					if (!bGammaTopLeft)
					{
						continue;
					}
				}

				// Evaluate the surface we're projecting coverage onto - if
				// the height value of the triangle is above the surface, project
				// onto the surface.
				Float32 const fSurfaceHeight = GetHeight(p);
				if (fHeight >= fSurfaceHeight)
				{
					RenderPixel(p);
				}
			}
		}
	}
}

} // namespace Seoul::Navigation

#endif // /#if SEOUL_WITH_NAVIGATION
