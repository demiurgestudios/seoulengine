/**
 * \file FalconTriangulator.cpp
 * \brief Wraps the TESS triangulation/tesselation library,
 * used by Falcon::Tesselator to generated 2D triangle lists
 * from shape paths.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FalconTesselator.h"
#include "FalconTriangulator.h"
#include "HashSet.h"
#include <tesselator.h>

namespace Seoul::Falcon
{

// TODO: Move tesselation/triangulation into the cooker, switch
// to the C# implementation of libtess2: https://github.com/speps/LibTessDotNet
namespace Triangulator
{

typedef Vector<Vector2D, MemoryBudgets::Falcon> Points;

static void* TriangulatorAllocate(void* /*pUserData*/, size_t zRequestedSizeInBytes)
{
	return MemoryManager::Allocate(zRequestedSizeInBytes, MemoryBudgets::Falcon);
}

static void TriangulatorDeallocate(void* /*pUserData*/, void* pAddressToFree)
{
	MemoryManager::Deallocate(pAddressToFree);
}

static void* TriangulatorReallocate(void* /*pUserData*/, void* pAddressToReallocate, size_t zRequestedSizeInBytes)
{
	return MemoryManager::Reallocate(pAddressToReallocate, zRequestedSizeInBytes, MemoryBudgets::Falcon);
}

/** Part of finalize, determine if a shape is convex or not. */
static Bool IsConvex(
	Paths::ConstIterator iBegin,
	Paths::ConstIterator iEnd)
{
	// Must be only a single path to be convex.
	if ((iBegin + 1) != iEnd)
	{
		return false;
	}

	// Examine element data.
	auto const& vPoints = iBegin->m_vPoints;
	auto const uPoints = vPoints.GetSize();
	
	// Less than 3 vertices, cannot be convex.
	if (uPoints < 3u)
	{
		return false;
	}
	// Greater than 2 vertices, less than or equal to 4, must be convex.
	else if (uPoints <= 4)
	{
		return true;
	}

	// Now compute (complex) convexivity.

	// Compute the sign of the first 2 line segments of the curve.
	Vector2D vPrev = vPoints[1];
	Vector2D vPrevD = (vPrev - vPoints[0]);
	Int32 iPrevSign = (Int32)Sign(Vector2D::Cross(
		vPoints[0] - vPoints[uPoints-1u],
		vPrevD));

	// Now advance and check that each further line segment pair
	// has the same angle (sign of the cross product) of
	// the first.
	for (UInt32 i = 2u; i < uPoints; ++i)
	{
		Vector2D const v(vPoints[i]);
		Vector2D const vD(v - vPrev);

		Int32 const iSign = (Int32)Sign(Vector2D::Cross(vPrevD, vD));
		if (iPrevSign != 0 && iSign != 0 && (iSign != iPrevSign))
		{
			// Different sign, not convex.
			return false;
		}

		vPrev = v;
		vPrevD = vD;
		iPrevSign = iSign;
	}

	// Done, convex.
	return true;
}

/**
 * Utility to compute whether a pending tesselation shape is convex or
 * not.
 */
void Finalize(
	Paths::ConstIterator iBegin,
	Paths::ConstIterator iEnd,
	Vertices& rvVertices,
	Indices& rvIndices,
	Bool& rbConvex)
{
	// Done if no indices.
	if (rvIndices.IsEmpty())
	{
		rbConvex = false;
		return;
	}

	// Determine convexivity.
	rbConvex = IsConvex(iBegin, iEnd);
}

Bool Triangulate(
	Paths::ConstIterator iBegin,
	Paths::ConstIterator iEnd,
	Vertices& rvVertices,
	Indices& rvIndices)
{
	Bool bHasVertices = false;

	for (auto i = iBegin; i != iEnd; ++i)
	{
		if (!i->m_vPoints.IsEmpty())
		{
			bHasVertices = true;
			break;
		}
	}

	if (!bHasVertices)
	{
		rvIndices.Clear();
		rvVertices.Clear();
		return true;
	}

	TESSalloc allocator;
	memset(&allocator, 0, sizeof(allocator));
	allocator.memalloc = TriangulatorAllocate;
	allocator.memfree = TriangulatorDeallocate;
	allocator.memrealloc = TriangulatorReallocate;
	TESStesselator* pTesselator = tessNewTess(&allocator);

	// Add each path as a contour.
	for (auto i = iBegin; i != iEnd; ++i)
	{
		const Points& vPoints = i->m_vPoints;
		if (vPoints.IsEmpty())
		{
			continue;
		}

		tessAddContour(pTesselator, 2, vPoints.Get(0), sizeof(Vector2D), vPoints.GetSize());
	}

	// Tesselate
	Bool bResult = (1 == tessTesselate(
		pTesselator,
		TESS_WINDING_ODD,
		TESS_POLYGONS,
		3,
		2,
		nullptr));

	// If successful, output the vertices and indices.
	if (bResult)
	{
		// Get the total vertex count.
		int const nVertices = tessGetVertexCount(pTesselator);

		// If no vertices, output empty.
		if (nVertices <= 0)
		{
			rvIndices.Clear();
			rvVertices.Clear();
		}
		// Otherwise, convert vertices and incies.
		else
		{
			// Copy the vertices straight.
			Vertices vVertices((Vertices::SizeType)nVertices);
			memcpy(vVertices.Get(0), tessGetVertices(pTesselator), vVertices.GetSizeInBytes());

			// Indices is 3x the element count.
			int const nIndices = 3 * tessGetElementCount(pTesselator);

			// If we have no indices, something terrible happened.
			if (nIndices <= 0)
			{
				bResult = false;
			}
			else
			{
				TESSindex const* pSourceIndices = tessGetElements(pTesselator);

				// Enumerate indices and check for TESS_UNDEF, which indicates
				// a special value we should never receive (a polygon with
				// fewer than our target, which would be 2 vertices, which
				// is not a valid polygon).
				Indices vIndices((Indices::SizeType)nIndices);
				for (int i = 0; i < nIndices; ++i)
				{
					TESSindex const iIndex = pSourceIndices[i];
					if (TESS_UNDEF == iIndex || (iIndex > (TESSindex)65535))
					{
						bResult = false;
						break;
					}

					vIndices[i] = (UInt16)iIndex;
				}

				if (bResult)
				{
					rvIndices.Swap(vIndices);
					rvVertices.Swap(vVertices);
				}
			}
		}
	}

	tessDeleteTess(pTesselator);

	return bResult;
}

} // namespace Triangulator

} // namespace Seoul::Falcon
