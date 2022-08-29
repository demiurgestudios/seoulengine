/**
 * \file FalconClipper.h
 * \brief Interface for Falcon's clipping facility,
 * used primarily to implement masking.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FALCON_CLIPPER_H
#define FALCON_CLIPPER_H

#include "FalconTriangleListDescription.h"
#include "FalconTypes.h"
#include "FixedArray.h"
#include "UnsafeBuffer.h"
namespace Seoul { namespace Falcon { namespace Clipper { template <typename VERTEX> class MeshClipCacheT; } } }

namespace Seoul
{

namespace Falcon
{

namespace Clipper
{

// When clipping against a rectangle, this is the number
// of extra vertices that must be present in the output
// array to account for the worst case clipped vertex
// generation.
static const UInt32 kuRectangleClipVertexCount = 4u;

typedef UnsafeBuffer<UInt16, MemoryBudgets::Falcon> Indices;
typedef UnsafeBuffer<ShapeVertex, MemoryBudgets::Falcon> Vertices;

/**
 * General purpose vertex, that refers to external data,
 * currently used for "persistent" clipping, where
 * the clipped value (m_v here) is stable but the
 * additional values (referenced by the m_a[].m_u member)
 * can change.
 *
 * Clipping is computed once, and then applied via remapping
 * to changed vertex buffers.
 */
struct UtilityVertex SEOUL_SEALED
{
	UtilityVertex()
		: m_v()
		, m_a()
		, m_uCount(0u)
		, m_uReservedForClipper(0u)
	{
	}

	/**
	 * Apply a new state to this vertex, with main value v
	 * and additional attributes u
	 */
	void Reset(const Vector2D& v, UInt32 u)
	{
		m_v = v;
		m_a.Fill(Entry());
		m_a[0].m_f = 1.0f;
		m_a[0].m_u = u;
		m_uCount = 1u;
		m_uReservedForClipper = 0u;
	}

	Bool operator==(const UtilityVertex& b) const
	{
		if (m_v != b.m_v)
		{
			return false;
		}
		
		if (m_uCount != b.m_uCount)
		{
			return false;
		}
		
		for (UInt32 i = 0u; i < m_uCount; ++i)
		{
			if (m_a[i] != b.m_a[i])
			{
				return false;
			}
		}
		
		return true;
	}

	Bool operator!=(const UtilityVertex& b) const
	{
		return !(*this == b);
	}

	Bool Equals(
		const UtilityVertex& b,
		Float fTolerance = fEpsilon) const
	{
		if (!m_v.Equals(b.m_v, fTolerance))
		{
			return false;
		}
		
		if (m_uCount != b.m_uCount)
		{
			return false;
		}
		
		for (UInt32 i = 0u; i < m_uCount; ++i)
		{
			if (!m_a[i].Equals(b.m_a[i], fTolerance))
			{
				return false;
			}
		}
		
		return true;
	}

	/**
	 * A single additional attribute entry of this utility
	 * vertex. Because we do not compute final values,
	 * we need to accumulate weights and references to
	 * the original (completely unclipped) vertex
	 * set.
	 */
	struct Entry SEOUL_SEALED
	{
		Entry()
			: m_f(0.0f)
			, m_u(0u)
		{
		}

		Float m_f;
		UInt32 m_u;
		
		Bool operator==(const Entry& b) const
		{
			return 
				(m_f == b.m_f) &&
				(m_u == b.m_u);
		}
		
		Bool operator!=(const Entry& b) const
		{
			return !(*this == b);
		}
		
		Bool Equals(
			const Entry& b,
			Float fTolerance = fEpsilon) const
		{
			return 
				m_u == b.m_u &&
				Seoul::Equals(m_f, b.m_f, fTolerance);
		}
	};

	Vector2D m_v;
	FixedArray<Entry, 3u> m_a;
	UInt32 m_uCount;
	UInt32 m_uReservedForClipper;
}; // struct UtilityVertex

// Utility when using clip functions outside of a clip stack.
// Generates clipping planes from a list of convex points.
//
// Pre: pClipPlanes must be uClipVertexCount in length.
void ComputeClipPlanes(
	Vector2D const* pClipVertices,
	UInt32 uClipVertexCount,
	Vector3D* pClipPlanes);

// pOutVertices must have enough space to contain uClipVertexCount + uInVertexCount vertices.
//
// Return values:
// - negative - all vertices clipped, pOutVertices left unmodified.
// - zero - no vertices clipped, pOutVertices left unmodified.
// - positive - some vertices clipped, pOutVertices contains the output. Return
//              value is the number of vertices in the clipped output shape.
Int32 ConvexClip(
	Vector3D const* pClipPlanes,
	UInt32 uClipPlaneCount,
	Vector2D const* pInVertices,
	UInt32 uInVertexCount,
	Vector2D* pOutVertices,
	Float fTolerance = kfAboutEqualPosition);
Int32 ConvexClip(
	Vector3D const* pClipPlanes,
	UInt32 uClipPlaneCount,
	ShapeVertex const* pInVertices,
	UInt32 uInVertexCount,
	ShapeVertex* pOutVertices,
	Float fTolerance = kfAboutEqualPosition);
Int32 ConvexClip(
	const Rectangle& clipRectangle,
	Vector2D const* pInVertices,
	UInt32 uInVertexCount,
	Vector2D* pOutVertices,
	Float fTolerance = kfAboutEqualPosition);
Int32 ConvexClip(
	const Rectangle& clipRectangle,
	ShapeVertex const* pInVertices,
	UInt32 uInVertexCount,
	ShapeVertex* pOutVertices,
	Float fTolerance = kfAboutEqualPosition);

// Allocate and destroy the cache used by MeshClip() variations.
template <typename VERTEX>
MeshClipCacheT<VERTEX>* NewMeshClipCache();

template <typename VERTEX>
void DestroyMeshClipCache(MeshClipCacheT<VERTEX>*& rp);

// Applies full clipping logic to a mesh (index+vertex buffers). Applie in-place.
// If uIndexCount or uVertexCount are < the size of Indices or Vertices, the
// *last* n indices/vertices are processed.
void MeshClip(
	MeshClipCacheT<ShapeVertex>& rCache,
	Vector3D const* pClipPlanes,
	UInt32 uClipPlaneCount,
	TriangleListDescription eDescription,
	Indices& rvIndices,
	UInt32 uIndexCount,
	Clipper::Vertices& rvVertices,
	UInt32 uVertexCount,
	Float fTolerance = kfAboutEqualPosition);
void MeshClip(
	MeshClipCacheT<ShapeVertex>& rCache,
	const Rectangle& clipRectangle,
	TriangleListDescription eDescription,
	Indices& rvIndices,
	UInt32 uIndexCount,
	Clipper::Vertices& rvVertices,
	UInt32 uVertexCount,
	Float fTolerance = kfAboutEqualPosition);
void MeshClip(
	MeshClipCacheT<ShapeVertex>& rCache,
	const Rectangle& clipRectangle,
	TriangleListDescription eDescription,
	const Rectangle& vertexBounds,
	Indices& rvIndices,
	UInt32 uIndexCount,
	Clipper::Vertices& rvVertices,
	UInt32 uVertexCount,
	Float fTolerance = kfAboutEqualPosition);

void MeshClip(
	MeshClipCacheT<UtilityVertex>& rCache,
	Vector3D const* pClipPlanes,
	UInt32 uClipPlaneCount,
	TriangleListDescription eDescription,
	Indices& rvIndices,
	UInt32 uIndexCount,
	UnsafeBuffer<UtilityVertex, MemoryBudgets::Falcon>& rvVertices,
	UInt32 uVertexCount,
	Float fTolerance = kfAboutEqualPosition);
void MeshClip(
	MeshClipCacheT<UtilityVertex>& rCache,
	const Rectangle& clipRectangle,
	TriangleListDescription eDescription,
	Indices& rvIndices,
	UInt32 uIndexCount,
	UnsafeBuffer<UtilityVertex, MemoryBudgets::Falcon>& rvVertices,
	UInt32 uVertexCount,
	Float fTolerance = kfAboutEqualPosition);
void MeshClip(
	MeshClipCacheT<UtilityVertex>& rCache,
	const Rectangle& clipRectangle,
	TriangleListDescription eDescription,
	const Rectangle& vertexBounds,
	Indices& rvIndices,
	UInt32 uIndexCount,
	UnsafeBuffer<UtilityVertex, MemoryBudgets::Falcon>& rvVertices,
	UInt32 uVertexCount,
	Float fTolerance = kfAboutEqualPosition);

// Handles clipping of both ShapeVertex and Vector2D in
// Clip and MeshClip variations.

static inline const Vector2D& GetVector2D(const ShapeVertex& v)
{
	return v.m_vP;
}

static inline const Vector2D& GetVector2D(const UtilityVertex& v)
{
	return v.m_v;
}

static inline const Vector2D& GetVector2D(const Vector2D& v)
{
	return v;
}

} // namespace Clipper

/**
 * A utility structure that manages a push/pop stack of clipping
 * shapes.
 *
 * There are 3 clipping shapes supported by a ClipStack:
 * - simple - an axis-aligned bounding box.
 * - single convex - an arbitrary set of planes, convex.
 * - multi convex - used to clip with arbitrary polygons. Polygon
 *   must be divided into a set of non-overlapping convex regions.
 */
class ClipStack SEOUL_SEALED
{
public:
	// Utility, each frame of the stack is defined
	// by a state entry.
	struct State SEOUL_SEALED
	{
		State()
			: m_Bounds(Rectangle::InverseMax())
			, m_uFirstVertex(0u)
			, m_uVertices(0u)
			, m_uFirstHull(0u)
			, m_uHulls(0u)
			, m_bSimple(false)
		{
		}

		Rectangle m_Bounds;
		UInt16 m_uFirstVertex;
		UInt16 m_uVertices;
		UInt16 m_uFirstHull;
		UInt16 m_uHulls;
		Bool m_bSimple;
	}; // struct State

	typedef Vector<UInt8, MemoryBudgets::Falcon> Hulls;
	typedef Vector<Vector3D, MemoryBudgets::Falcon> Planes;
	typedef Vector<State, MemoryBudgets::Falcon> Stack;
	typedef Vector<Vector2D, MemoryBudgets::Falcon> Vertices;

	ClipStack();
	~ClipStack();

	/**
	 * Insert a new convex hull into the pending stack frame.
	 *
	 * Call all Add*() variations prior to Push().
	 */
	template <typename T>
	void AddConvexHull(
		T const* p,
		UInt32 u,
		Float fTolerance = kfAboutEqualPosition)
	{
		T const* const pEnd = (p + u);
		for (; p < pEnd; ++p)
		{
			m_vVertices.PushBack(Clipper::GetVector2D(*p));
		}

		ClipHull(u, fTolerance);
	}

	/**
	 * Insert a new convex hull into the pending stack frame.
	 *
	 * Call all Add*() variations prior to Push().
	 */
	template <typename T>
	void AddConvexHull(
		const Matrix2x3& m,
		T const* pIn,
		UInt32 u,
		Float fTolerance = kfAboutEqualPosition)
	{
		T const* const pEnd = (pIn + u);

		// Insert the vertices in reverse if the transform reflects.
		if (m.DeterminantUpper2x2() < 0.0f)
		{
			for (auto pV = (pEnd - 1); pV >= pIn; --pV)
			{
				m_vVertices.PushBack(Matrix2x3::TransformPosition(m, Clipper::GetVector2D(*pV)));
			}
		}
		else
		{
			for (auto pV = pIn; pV < pEnd; ++pV)
			{
				m_vVertices.PushBack(Matrix2x3::TransformPosition(m, Clipper::GetVector2D(*pV)));
			}
		}

		ClipHull(u, fTolerance);
	}

	/**
	 * Insert a new rectangle into the pending stack frame.
	 *
	 * Call all Add*() variations prior to Push().
	 */
	void AddRectangle(
		const Rectangle& rect,
		Float fTolerance = kfAboutEqualPosition)
	{
		Vector2D a[4];
		a[0] = Vector2D(rect.m_fRight, rect.m_fTop);
		a[1] = Vector2D(rect.m_fRight, rect.m_fBottom);
		a[2] = Vector2D(rect.m_fLeft, rect.m_fBottom);
		a[3] = Vector2D(rect.m_fLeft, rect.m_fTop);
		AddConvexHull(a, 4, fTolerance);
	}

	/**
	 * Insert a new rectangle into the pending stack frame.
	 *
	 * Call all Add*() variations prior to Push().
	 */
	void AddRectangle(
		const Matrix2x3& m,
		const Rectangle& rect,
		Float fTolerance = kfAboutEqualPosition)
	{
		Vector2D a[4];
		a[0] = Vector2D(rect.m_fRight, rect.m_fTop);
		a[1] = Vector2D(rect.m_fRight, rect.m_fBottom);
		a[2] = Vector2D(rect.m_fLeft, rect.m_fBottom);
		a[3] = Vector2D(rect.m_fLeft, rect.m_fTop);
		AddConvexHull(m, a, 4, fTolerance);
	}

	/**
	 * Used for estimating the worst case increase to a vertex/index count based
	 * on the current state of the clip stack. This is a fast, conservative estimate.
	 *
	 * If ruIndexCount is 0, ruVertexCount is considered convex.
	 */
	void AddWorstCaseClippingCounts(UInt32& ruIndexCount, UInt32& ruVertexCount)
	{
		// No adjustment if no clipping.
		if (!HasClips())
		{
			return;
		}

		auto const& e = GetTopClip();

		// Number of convex hulls - if the index count is not zero, it's
		// the number of indices / 3 (the number of triangles). Otherwise,
		// it's just one.
		UInt32 const uConvexHulls = (0u == ruIndexCount
			?  1u
			: (ruIndexCount / 3u));

		// Now use the hull count to compute the inflated vertex and index
		// counts.
		if (e.m_bSimple)
		{
			// Four sides, each can add at most 1 vertex for a convex hull.
			ruVertexCount += (4u * uConvexHulls);
			ruIndexCount += (0u == ruIndexCount ? 0u : (4u * uConvexHulls * 3u));
		}
		else if (e.m_uHulls == 1u)
		{
			// n sides, each can add at most 1 vertex for a convex hull.
			ruVertexCount += (e.m_uVertices * uConvexHulls);
			ruIndexCount += (0u == ruIndexCount ? 0u : (e.m_uVertices * uConvexHulls * 3u));
		}
		else
		{
			// Each hull can add n vertices to the output.
			for (UInt32 i = 0u; i < e.m_uHulls; ++i)
			{
				// Get the number of vertices in this clipping hull.
				UInt32 const uClipPlanes = m_vHulls[e.m_uFirstHull + i];
				ruVertexCount += (uClipPlanes * uConvexHulls);
				ruIndexCount += (0u == ruIndexCount ? 0u : (uClipPlanes * uConvexHulls * 3u));
			}
		}
	}

	/**
	 * Reset this ClipStack to its default state.
	 */
	void Clear()
	{
		m_vStack.Clear();
		m_vHulls.Clear();
		m_vPlanes.Clear();
		m_vVertices.Clear();
		m_Pending = State();
	}

	/**
	 * @return The top-most frame in the clip stack.
	 *
	 * \pre HasClips() must be true.
	 */
	const State& GetTopClip() const
	{
		return m_vStack.Back();
	}

	/** Direct read-only access to the current vertex set. */
	const Vertices& GetVertices() const { return m_vVertices; }

	/**
	 * @return true if the current stack has any frames.
	 */
	Bool HasClips() const
	{
		return !m_vStack.IsEmpty();
	}

	/**
	 * Apply the current clip stack to an arbitrary mesh
	 * defineds by index and vertex buffers. Clipping
	 * occurs in-place.
	 */
	void MeshClip(
		TriangleListDescription eDescription,
		Clipper::Indices& rvIndices,
		Clipper::Vertices& rvVertices,
		Int32 iIndexCount = -1,
		Int32 iVertexCount = -1,
		Float fTolerance = kfAboutEqualPosition);

	/**
	 * Apply the current clip stack to an arbitrary mesh
	 * defineds by index and vertex buffers. Clipping
	 * occurs in-place. vertexBounds is used to early out
	 * computations, so it should be cheaply computed.
	 */
	void MeshClip(
		TriangleListDescription eDescription,
		const Rectangle& vertexBounds,
		Clipper::Indices& rvIndices,
		Clipper::Vertices& rvVertices,
		Int32 iIndexCount = -1,
		Int32 iVertexCount = -1,
		Float fTolerance = kfAboutEqualPosition);

	// Modification of the clip stack.

	void Pop();

	// A Push() applies all pending clip shapes added by
	// Add*() methods.
	//
	// If Push() returns true, pending clip shapes have
	// been applied and a Pop() must be called. Otherwise,
	// they were discarded and a Pop() must *not* be called.
	//
	// Push() will return false if the stack of clip shapes
	// results in a no-clip (a zero-sized clip area, so that
	// everything is clipped).
	Bool Push();

	// Checks if the stack is in a fully clear state. This
	// might be a question on the unit tests really care
	// about.
	Bool IsFullyClear()
	{
		return m_vHulls.IsEmpty()
			&& m_vPlanes.IsEmpty()
			&& m_vStack.IsEmpty()
			&& m_vVertices.IsEmpty();
	}

private:
	friend class ClipCapture;
	ScopedPtr< Clipper::MeshClipCacheT<ShapeVertex> > m_pCache;
	State m_Pending;
	Hulls m_vHulls;
	Planes m_vPlanes;
	Stack m_vStack;
	Vertices m_vVertices;

	Clipper::Indices m_vScratchIndices;
	Clipper::Vertices m_vScratchVertices;
	Vertices m_vScratchVectors2D;

	void ClipHull(UInt32 uCount, Float fTolerance);

	SEOUL_DISABLE_COPY(ClipStack);
}; // class ClipStack

/**
 * Utility used to capture the exact state of the *top*
 * of the ClipStack.
 */
class ClipCapture SEOUL_SEALED
{
public:
	ClipCapture()
		: m_State()
		, m_vHulls()
		, m_vPlanes()
		, m_vVertices()
	{
	}

	~ClipCapture()
	{
	}

	/**
	 * Capture the state of the top of the clip stack.
	 *
	 * The *top* of this capture is important. If this
	 * capture is applied to a ClipStack with Overwrite(),
	 * the stack of the ClipStack will be trampled. Only
	 * the topmost element will be defined.
	 *
	 * In otherwords, calling Pop() on the ClipStack()
	 * after Overwrite() will place the clip stack
	 * in its default state.
	 */
	void Capture(const ClipStack& stack)
	{
		// Stack to capture has no stack,
		// place this capture in the default state.
		if (stack.m_vStack.IsEmpty())
		{
			m_State = ClipStack::State();
			m_vHulls.Clear();
			m_vPlanes.Clear();
			m_vVertices.Clear();
		}
		else
		{
			// Capture.
			m_State = stack.m_vStack.Back();
			
			// No hulls, clear out capture.
			if (0u == m_State.m_uHulls)
			{
				m_vHulls.Clear();
			}
			else
			{
				// Otherwise, copy the hull range.
				auto const iBegin = (stack.m_vHulls.Begin() + m_State.m_uFirstHull);
				auto const iEnd = (iBegin + m_State.m_uHulls);
				m_vHulls.Assign(iBegin, iEnd);
			}

			// No vertices or planes,  clear vertices and planes.
			if (0u == m_State.m_uVertices)
			{
				m_vPlanes.Clear();
				m_vVertices.Clear();
			}
			else
			{
				// Otherwise, copy the range of defined planes
				// and vertices.
				{
					auto const iBegin = (stack.m_vPlanes.Begin() + m_State.m_uFirstVertex);
					auto const iEnd = (iBegin + m_State.m_uVertices);
					m_vPlanes.Assign(iBegin, iEnd);
				}
				{
					auto const iBegin = (stack.m_vVertices.Begin() + m_State.m_uFirstVertex);
					auto const iEnd = (iBegin + m_State.m_uVertices);
					m_vVertices.Assign(iBegin, iEnd);
				}
			}

			// Since we copy only the topmost element, our capture
			// always starts at index 0.
			m_State.m_uFirstHull = 0u;
			m_State.m_uFirstVertex = 0u;
		}
	}

	/**
	 * Effectively, clear the ClipStack r and apply
	 * the topmost clip state that was captured with
	 * this ClipStack.
	 */
	void Overwrite(ClipStack& r) const
	{
		// Ovewrwrite ranges.
		r.m_vStack.Assign(1u, m_State);
		r.m_vHulls = m_vHulls;
		r.m_vPlanes = m_vPlanes;
		r.m_vVertices = m_vVertices;

		// Place pending in its proper expected state.
		r.m_Pending = ClipStack::State();
		r.m_Pending.m_uFirstHull = r.m_vHulls.GetSize();
		r.m_Pending.m_uFirstVertex = r.m_vVertices.GetSize();
	}

private:
	ClipStack::State m_State;
	ClipStack::Hulls m_vHulls;
	ClipStack::Planes m_vPlanes;
	ClipStack::Vertices m_vVertices;

	SEOUL_DISABLE_COPY(ClipCapture);
}; // class ClipCapture

} // namespace Falcon

SEOUL_STATIC_ASSERT(sizeof(Falcon::Clipper::UtilityVertex) == 40);
template <> struct CanMemCpy<Falcon::Clipper::UtilityVertex> { static const Bool Value = true; };
template <> struct CanZeroInit<Falcon::Clipper::UtilityVertex> { static const Bool Value = true; };

} // namespace Seoul

#endif // include guard
