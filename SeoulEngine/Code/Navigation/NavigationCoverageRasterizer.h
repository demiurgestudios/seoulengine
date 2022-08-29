/**
 * \file NavigationCoverageRasterizer.h
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

#pragma once
#ifndef NAVIGATION_COVERAGE_RASTERIZER_H
#define NAVIGATION_COVERAGE_RASTERIZER_H

#include "Axis.h"
#include "Prereqs.h"
#include "Vector.h"
#include "Vector3D.h"
namespace Seoul { namespace Navigation { class Grid; } }

#if SEOUL_WITH_NAVIGATION

namespace Seoul::Navigation
{

/** Utility used to project 3D collision triangles into a 2D space for populating a Navlib.Grid. */
class CoverageRasterizer SEOUL_SEALED
{
public:
	/**
	 * Number of pixels per cell row - or (4 * 4) pixels per cell.
	 *
	 * Keep in-sync with NAVAPI_RASTER_RES.
	 */
	static const Int32 kiRasterRes = 4;

	CoverageRasterizer(
		UInt32 uWidth,
		UInt32 uHeight,
		const Vector3D& vWorldOrigin,
		Float32 const* pfHeightDataInPixels,
		Axis eUpAxis = Axis::kZ);

	// Apply the current state of the rasterizer to
	// a Grid. If the grid's dimensions do not
	// match the rasterizer, output will be clamped as needed.
	void ApplyToGrid(
		Grid& grid,
		UInt32 uMinimumSampleCount,
		UInt32 uBitToSet) const;

	void Clear()
	{
		m_vSurface.Fill(false);
	}

	/** Grid height used to configure this rasterizer. */
	UInt32 GetHeight() const
	{
		return m_uHeight;
	}

	/**
	 * Get the accumulated sample count of a grid cell
	 * rendered into the coverage buffer.
	 *
	 * Pre: uGridX/uGridY are on the grid.
	 */
	UInt32 GetSampleCount(UInt32 uGridX, UInt32 uGridY) const
	{
		SEOUL_ASSERT(uGridX < m_uWidth);
		SEOUL_ASSERT(uGridY < m_uHeight);

		UInt32 uCount = 0u;
		Int32 const iStartX = (Int32)uGridX * kiRasterRes;
		Int32 const iEndX = iStartX  + kiRasterRes;
		Int32 const iStartY = (Int32)uGridY * kiRasterRes;
		Int32 const iEndY = iStartY + kiRasterRes;

		Point p;
		for (p.m_iY = iStartY; p.m_iY < iEndY; ++p.m_iY)
		{
			for (p.m_iX = iStartX; p.m_iX < iEndX; ++p.m_iX)
			{
				uCount += (GetPixel(p) ? 1u : 0u);
			}
		}

		return uCount;
	}

	/** Grid width used to configure this rasterizer. */
	UInt32 GetWidth() const
	{
		return m_uWidth;
	}

	/** Rasterize a triangle into a pixel buffer to determine coverage. */
	void RasterizeTriangle(
		const Vector3D& v0,
		const Vector3D& v1,
		const Vector3D& v2);

private:
	Vector<Bool, MemoryBudgets::Navigation> m_vSurface;
	Vector<Float32, MemoryBudgets::Navigation> m_vfHeightData;
	UInt32 const m_uWidth;
	UInt32 const m_uHeight;
	Vector3D const m_vWorldOrigin;
	Axis const m_eUpAxis;

	struct Point SEOUL_SEALED
	{
		Point()
			: m_iX(0)
			, m_iY(0)
			, m_fHeightFactor(0.0f)
		{
		}

		Int32 m_iX;
		Int32 m_iY;
		Float32 m_fHeightFactor;
	}; // struct Point

	static Int32 ComputeSignedArea(const Point& p0, const Point& p1, const Point& p2)
	{
		return
			(p1.m_iX - p0.m_iX) *
			(p2.m_iY - p0.m_iY) -
			(p1.m_iY - p0.m_iY) *
			(p2.m_iX - p0.m_iX);
	}

	Float32 GetHeight(const Point& p) const
	{
		Int32 const iPixelWidth = (Int32)m_uWidth * kiRasterRes;
		return m_vfHeightData[p.m_iY * iPixelWidth + p.m_iX];
	}

	Bool GetPixel(const Point& p) const
	{
		Int32 const iPixelWidth = (Int32)m_uWidth * kiRasterRes;
		return m_vSurface[p.m_iY * iPixelWidth + p.m_iX];
	}

	static Bool IsTopLeft(const Point& p0, const Point& p1)
	{
		// Triangles are treated as clockwise - we don't
		// backface cull, so this doesn't matter, as long
		// as we're consistent.
		return
			(p0.m_iY > p1.m_iY) || // left edge
			(p0.m_iY == p1.m_iY && p0.m_iX < p1.m_iX); // top edge.
	}

	Point Quantize(const Vector3D& v) const
	{
		Vector3D const vRelative = (v - m_vWorldOrigin);

		Point ret;
		switch (m_eUpAxis)
		{
		case Axis::kX:
			ret.m_iX = (Int32)Round(vRelative.Y * (Float32)kiRasterRes);
			ret.m_iY = (Int32)Round(vRelative.Z * (Float32)kiRasterRes);
			ret.m_fHeightFactor = vRelative.X;
			break;
		case Axis::kY:
			ret.m_iX = (Int32)Round(vRelative.X * (Float32)kiRasterRes);
			ret.m_iY = -(Int32)Round(vRelative.Z * (Float32)kiRasterRes);
			ret.m_fHeightFactor = vRelative.Y;
			break;
		case Axis::kZ:
			ret.m_iX = (Int32)Round(vRelative.X * (Float32)kiRasterRes);
			ret.m_iY = (Int32)Round(vRelative.Y * (Float32)kiRasterRes);
			ret.m_fHeightFactor = vRelative.Z;
			break;
		default:
			SEOUL_FAIL("Out-of-sync enum.");
			break;
		};
		return ret;
	}

	void RenderPixel(const Point& p)
	{
		Int32 const iPixelWidth = (Int32)m_uWidth * kiRasterRes;
		m_vSurface[p.m_iY * iPixelWidth + p.m_iX] = true;
	}

	SEOUL_DISABLE_COPY(CoverageRasterizer);
}; // class CoverageRasterizer

} // namespace Seoul::Navigation

#endif // /#if SEOUL_WITH_NAVIGATION

#endif // include guard
