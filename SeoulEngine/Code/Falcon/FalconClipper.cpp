/**
 * \file FalconClipper.cpp
 * \brief Interface for Falcon's clipping facility,
 * used primarily to implement masking.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FalconClipper.h"
#include "HashSet.h"
#include "StackOrHeapArray.h"

namespace Seoul
{

namespace
{

/**
 * Utility wrapper for tracking clipped vs.
 * original vertices.
 */
template <typename T>
struct VertexWrapper SEOUL_SEALED
{
	VertexWrapper()
		: m_v()
		, m_uOrigIndex(0u)
	{
	}

	explicit VertexWrapper(const T& v)
		: m_v(v)
		, m_uOrigIndex(0u)
	{
	}

	const T& GetVertex() const { return m_v; }

	T m_v;
	UInt32 m_uOrigIndex;
}; // struct VertexWrapper

/**
 * Entry used in the custom hashing table used
 * for vertex lookup in MeshBuilder.
 */
struct MeshBuilderLookupEntry SEOUL_SEALED
{
	MeshBuilderLookupEntry()
		: m_uHash(0u)
		, m_uIndex(0u)
		, m_bValid(false)
	{
	}

	UInt32 m_uHash;
	UInt16 m_uIndex;
	Bool m_bValid;
}; // struct MeshBuilderLookupEntry

} // namespace anonymous

template <typename VERTEX> struct CanMemCpy< VertexWrapper<VERTEX> > { static const Bool Value = true; };
template <typename VERTEX> struct CanZeroInit< VertexWrapper<VERTEX> > { static const Bool Value = true; };
template <> struct CanMemCpy<MeshBuilderLookupEntry> { static const Bool Value = true; };
template <> struct CanZeroInit<MeshBuilderLookupEntry> { static const Bool Value = true; };

namespace Falcon
{

namespace Clipper
{

static const UInt32 kuMaxStackVertices = 12;

/** Hook for VertexWrapper<>. */
template <typename T>
static inline const Vector2D& GetVector2D(const VertexWrapper<T>& v)
{
	return Clipper::GetVector2D(v.m_v);
}

/**
 * A MeshBuilder is needed when clipping arbitrary triangle mesh
 * buckets (that can not be specially classified as either convex
 * or a list of quads.
 */
template <typename VERTEX>
class MeshBuilderT SEOUL_SEALED
{
public:
	typedef Vector<MeshBuilderLookupEntry, MemoryBudgets::Falcon> Lookup;
	typedef UnsafeBuffer<VERTEX, MemoryBudgets::Falcon> Vertices;

	MeshBuilderT(Indices& rvIndices, Vertices& rvVertices)
		: m_vLookup()
		, m_rvIndices(rvIndices)
		, m_rvVertices(rvVertices)
		, m_uBaseVertex(0u)
	{
	}

	~MeshBuilderT()
	{
	}

	/**
	 * Consume a vertex into the mesh being built.
	 *
	 * If the input vertex is exactly equal to an existing vertex,
	 * the existing vertex will be reused.
	 */
	void MergeVertex(const VERTEX& vertex)
	{
		// Compute a hash for the vertex.
		UInt32 const uHash = Seoul::GetHash((Byte const*)&vertex, (UInt32)sizeof(vertex));

		// Resolve the vertex to get an index and then offset it.
		UInt16 const uIndexValue = (ResolveVertex(vertex, uHash) + m_uBaseVertex);

		m_rvIndices.PushBack(uIndexValue);
	}

	/**
	 * Clear the mesh builder internal start, restart with a new mesh.
	 *
	 * @param[in] uBaseVertex the offset that will be applied to all
	 * mesh indices.
	 */
	void Reset(UInt16 uBaseVertex, UInt32 uExpectedVertices)
	{
		m_vLookup.Clear();
		GrowLookup(uExpectedVertices);
		m_vLookupScratch.Clear();
		m_rvIndices.Clear();
		m_rvVertices.Clear();
		m_rvVertices.Reserve(uExpectedVertices);
		m_uBaseVertex = uBaseVertex;
	}

private:
	/**
	 * Increase the size of our lookup table for vertices to at least the
	 * specified capacity (will be rounded up to the nearest power of 2).
	 *
	 * Nop if the existing capacity is already >= uCapacity.
	 */
	void GrowLookup(UInt32 uCapacity)
	{
		// Round up and then check the capacity.
		uCapacity = GetNextPowerOf2(uCapacity);
		if (uCapacity <= m_vLookup.GetSize())
		{
			return;
		}

		// Setup the scratch area for the size increase.
		m_vLookupScratch.Clear();
		m_vLookupScratch.Resize(uCapacity);

		// Rehash and insert the entries.
		for (auto i = m_vLookup.Begin(); m_vLookup.End() != i; ++i)
		{
			// Skip invalid entries (these are placeholder values).
			if (!i->m_bValid)
			{
				continue;
			}

			// Loop indefinitely until we find an insertion slot.
			// The existence of one is guaranteed by the fact that
			// we increased the size, not decreased it.
			UInt32 uIndex = i->m_uHash;
			while (true)
			{
				// Mask "trick" - because capacity is power-of-2, we can mask
				// by it to mod the index.
				uIndex &= (uCapacity - 1u);

				// If we found an insertion point (invalid entry), insert and
				// finish.
				if (!m_vLookupScratch[uIndex].m_bValid)
				{
					m_vLookupScratch[uIndex] = *i;
					break;
				}

				++uIndex;
			}
		}

		m_vLookup.Swap(m_vLookupScratch);
	}

	/**
	 * @return The index of the vertex specified by vertex, given
	 * an already computed hash.
	 *
	 * Either returns the index of an existing vertex copy, or
	 * insert vertex and returns the new insertion index.
	 */
	UInt16 ResolveVertex(const VERTEX& vertex, UInt32 uHash)
	{
		// Must have enough room for at least one more entry.
		if (m_rvVertices.GetSize() + 1u > m_vLookup.GetSize())
		{
			GrowLookup(m_vLookup.GetSize() + 1u);
		}

		// Loop indefinitely until we find an insertion point or an
		// existing entry.
		UInt32 const uCapacity = m_vLookup.GetSize();
		UInt32 uIndex = uHash;
		while (true)
		{
			uIndex &= (uCapacity - 1u);
			auto& r = m_vLookup[uIndex];

			// Valid entry, check for equality.
			if (r.m_bValid)
			{
				// Avoid expensive comparison on collisions.
				if (uHash == r.m_uHash)
				{
					// Lookup and equality compare the vertices.
					UInt16 const uVertexIndex = r.m_uIndex;
					if (vertex == m_rvVertices[uVertexIndex])
					{
						return uVertexIndex;
					}
				}
			}
			// Invalid entry, need to insert.
			else
			{
				UInt16 const uVertexIndex = (UInt16)m_rvVertices.GetSize();
				m_rvVertices.PushBack(vertex);

				MeshBuilderLookupEntry entry;
				entry.m_bValid = true;
				entry.m_uHash = uHash;
				entry.m_uIndex = uVertexIndex;

				// Done, finish insertion and return the index of insertion.
				r = entry;
				return uVertexIndex;
			}

			++uIndex;
		}
	}

	Lookup m_vLookup;
	Lookup m_vLookupScratch;
	Indices& m_rvIndices;
	Vertices& m_rvVertices;
	UInt16 m_uBaseVertex;

	SEOUL_DISABLE_COPY(MeshBuilderT);
}; // class MeshBuilderT

/**
 * All MeshClip() methods take a clip cache to store intermediate
 * state. The cache should persist, to avoid memory allocations with
 * each clipping call.
 */
template <typename VERTEX>
class MeshClipCacheT SEOUL_SEALED
{
public:
	typedef MeshBuilderT<VERTEX> Builder;
	typedef Vector<Bool, MemoryBudgets::Falcon> Inside;
	typedef Vector<Int32, MemoryBudgets::Falcon> Remap;
	typedef UnsafeBuffer<VERTEX, MemoryBudgets::Falcon> Vertices;

	MeshClipCacheT()
		: m_vInside()
		, m_vRemap()
		, m_vClipIndices()
		, m_vClipVertices()
		, m_Builder(m_vClipIndices, m_vClipVertices)
	{
	}

	Inside m_vInside;
	Remap m_vRemap;
	Indices m_vClipIndices;
	Vertices m_vClipVertices;
	Builder m_Builder;

private:
	SEOUL_DISABLE_COPY(MeshClipCacheT);
}; // class MeshClipCacheT

// ShapeVertex MeshClipCache.

// Allocate and destroy a clip cache.

template <>
MeshClipCacheT<ShapeVertex>* NewMeshClipCache<ShapeVertex>()
{
	return SEOUL_NEW(MemoryBudgets::Falcon) MeshClipCacheT<ShapeVertex>;
}

template <>
void DestroyMeshClipCache<ShapeVertex>(MeshClipCacheT<ShapeVertex>*& rp)
{
	auto p = rp;
	rp = nullptr;

	SafeDelete(p);
}

template <>
MeshClipCacheT<UtilityVertex>* NewMeshClipCache<UtilityVertex>()
{
	return SEOUL_NEW(MemoryBudgets::Falcon) MeshClipCacheT<UtilityVertex>;
}

template <>
void DestroyMeshClipCache<UtilityVertex>(MeshClipCacheT<UtilityVertex>*& rp)
{
	auto p = rp;
	rp = nullptr;

	SafeDelete(p);
}

// Utility to store stages of clipping against 1D planes (planes along
// the x or y axes).
struct Plane1D
{
	static Plane1D Create(Int32 iComponent, Float fPlane, Float fSign)
	{
		Plane1D ret;
		ret.m_iComponent = iComponent;
		ret.m_fPlane = fPlane;
		ret.m_fSign = fSign;
		return ret;
	}

	Int32 m_iComponent;
	Float m_fPlane;
	Float m_fSign;
};

// Variation for linearly interpolating values based on the clip
// intermediate point.

static inline ShapeVertex DeriveVertex(const ShapeVertex& vertex1, const ShapeVertex& vertex2, Float fT)
{
	ShapeVertex ret;

	// TODO: Interpolate color values.
	ret.m_ColorAdd = vertex1.m_ColorAdd;
	ret.m_ColorMultiply = vertex1.m_ColorMultiply;
	ret.m_vP = Lerp(vertex1.m_vP, vertex2.m_vP, fT);
	ret.m_vT = Lerp(vertex1.m_vT, vertex2.m_vT, fT);

	return ret;
}

/**
 * This operation is notably more complex looking for a utility
 * vertex, since we're not just generating final values
 * immediately, but rather, maintaining a weight set of
 * references to original (unclipped) vertices that will
 * form final values.
 *
 * This allows a clipped vertex set to be reused with new
 * attribute data, as long as the main value (m_v) does
 * not change.
 */
static inline UtilityVertex DeriveVertex(const UtilityVertex& v1, const UtilityVertex& v2, Float fT)
{
	UtilityVertex ret;

	// Cache weights of 0 and 1.
	auto const f1 = (1.0f - fT);
	auto const f2 = fT;

	// Simple part - just lerp the v part.
	ret.m_v = Lerp(v1.m_v, v2.m_v, fT);

	// Complex part - need to merge the referenced vertices of v1 and v2. Initially,
	// values are equal to v1, rescaled by f1.
	ret.m_uCount = v1.m_uCount;
	for (UInt32 i = 0u; i < ret.m_uCount; ++i)
	{
		ret.m_a[i].m_u = v1.m_a[i].m_u;
		ret.m_a[i].m_f = v1.m_a[i].m_f * f1;
	}

	// Now merge in the values of v2 - for each entry in v2,
	// check if it already exists in v1. If so, merge. Otherwise,
	// need to add a new entry for v2. Sanity check count - must never
	// exceed the size of the fixed array (which is 3, as we can never
	// reference more than 3 original vertices no matter how much clipping,
	// with normally defined triangles).
	for (UInt32 i = 0u; i < v2.m_uCount; ++i)
	{
		Bool bDone = false;
		Float const f = v2.m_a[i].m_f * f2;
		for (UInt32 j = 0u; j < ret.m_uCount; ++j)
		{
			// Same reference, accumulate.
			if (ret.m_a[j].m_u == v2.m_a[i].m_u)
			{
				ret.m_a[j].m_f += f;
				bDone = true;
				break;
			}
		}

		// Need to add a new entry.
		if (!bDone)
		{
			// Sanity check.
			SEOUL_ASSERT(ret.m_uCount < ret.m_a.GetSize());
			ret.m_a[ret.m_uCount].m_f = f;
			ret.m_a[ret.m_uCount].m_u = v2.m_a[i].m_u;
			++ret.m_uCount;
		}
	}

	return ret;
}

static inline Vector2D DeriveVertex(const Vector2D& v1, const Vector2D& v2, Float fT)
{
	return Lerp(v1, v2, fT);
}

template <typename T>
static inline VertexWrapper<T> DeriveVertex(const VertexWrapper<T>& vertex1, const VertexWrapper<T>& vertex2, Float fT)
{
	VertexWrapper<T> ret;
	ret.m_v = DeriveVertex(vertex1.m_v, vertex2.m_v, fT);
	return ret;
}

//
// Given 2 vertex endpoints, compute the vertex of intersection
// against a plane.
//
template <typename T>
static inline T ComputeIntersection(
	const T& vertex1,
	const T& vertex2,
	const Vector3D& vPlane,
	Float fDotCoordinate)
{
	// Cache vertex points.
	const Vector2D& v1 = Clipper::GetVector2D(vertex1);
	const Vector2D& v2 = Clipper::GetVector2D(vertex2);

	// Delta 2D change between point.
	Vector2D const vDifference = (v2 - v1);

	// fT is a value on [0, 1] which defines a Lerp to
	// apply between the 2 vertices to compute the intersection
	// vertex. Clamp is used here to gracefully handle NaN generated
	// by fA == 0.0f.
	Float const fT = Seoul::Clamp(-fDotCoordinate / Vector2D::Dot(vDifference, vPlane.GetXY()), 0.0f, 1.0f);

	return DeriveVertex(vertex1, vertex2, fT);
}

//
// Given 2 vertex endpoints, compute the vertex of intersection
// against a one-dimensional (axis-aligned) plane.
//
template <typename T>
static inline T ComputeIntersection(
	const T& vertex1,
	const T& vertex2,
	const Plane1D& plane,
	Float fDotCoordinate)
{
	// Cache vertex points.
	const Vector2D& v1 = Clipper::GetVector2D(vertex1);
	const Vector2D& v2 = Clipper::GetVector2D(vertex2);

	// Length is the change along our desired axis.
	Float const fA = (v2[plane.m_iComponent] - v1[plane.m_iComponent]);

	// fT is a value on [0, 1] which defines a Lerp to
	// apply between the 2 vertices to compute the intersection
	// vertex. Clamp is used here to gracefully handle NaN generated
	// by fA == 0.0f.
	Float const fT = Seoul::Clamp((plane.m_fPlane - v1[plane.m_iComponent]) / fA, 0.0f, 1.0f);

	return DeriveVertex(vertex1, vertex2, fT);
}

// Dot coordinate plane computation in 2D and 1D.

static inline Float DotCoordinate(const Vector3D& vPlane, const Vector2D& vPoint)
{
	return Vector2D::Dot(vPoint, vPlane.GetXY()) + vPlane.Z;
}

static inline Float DotCoordinate(const Plane1D& plane, const Vector2D& vPoint)
{
	return (vPoint[plane.m_iComponent] - plane.m_fPlane) * plane.m_fSign;
}

/**
 * Clip an array of vertices against a plane. Templated
 * to allow specialization of 2D and 1D (axis-aligned) planes.
 */
template <typename PLANE, typename T>
static inline Int32 PlaneClip(
	const PLANE& plane,
	T const* pInVertices,
	UInt32 uInVertexCount,
	T* pOutVertices,
	Float fTolerance)
{
	// Used for caching plane intersection results.
	typedef StackOrHeapArray<Float, kuMaxStackVertices, MemoryBudgets::Falcon> StackOrHeapArrayType;
	StackOrHeapArrayType aPlaneIntersections(uInVertexCount);

	// Count vertices on the positive and negative side
	// of the clip plane - "intersecting" the plane is unique
	// and explicit here (an intersection is neither a positive
	// or negative result).
	Int32 iPositive = 0;
	Int32 iNegative = 0;
	for (UInt32 i = 0u; i < uInVertexCount; ++i)
	{
		// Compute the intersection type.
		Float const fDotCoordinate = DotCoordinate(
			plane,
			Clipper::GetVector2D(pInVertices[i]));

		// Increment counters.
		iPositive += (fDotCoordinate > fTolerance ? 1 : 0);
		iNegative += (fDotCoordinate < -fTolerance ? 1 : 0);

		// Cache the result.
		aPlaneIntersections[i] = fDotCoordinate;
	}

	// All points are either in the plane, or on the "positive" side
	// of the plane (where the positive direction is defined as pointing
	// to the inside of the convex clipping polygon), so nothing is clipped.
	if (0 == iNegative)
	{
		return 0;
	}
	// All points are either in the plane or on the "negative" side
	// of the plane (where the negative direction is defined as pointing
	// to the inside of the convex clipping polygon), so everything is clipped.
	else if (0 == iPositive)
	{
		return -1;
	}

	// Otherwise, some are outside, some are inside, so we need to clip.
	UInt32 uOutCount = 0u;
	Float fS = aPlaneIntersections[uInVertexCount - 1];
	T vertexS = pInVertices[uInVertexCount - 1];
	for (UInt32 uP = 0u; uP < uInVertexCount; ++uP)
	{
		// The first vertex is s and the second is p.
		Float const fP = aPlaneIntersections[uP];
		T const vertexP = pInVertices[uP];

		// If p is on the positive side of the clip plane, we
		// always include it.
		if (fP > fTolerance)
		{
			// If s is explicitly on the outside of the clip plane,
			// we generate a new vertex to be on the plane for the
			// intersection bewteen s and p.
			if (fS < -fTolerance)
			{
				pOutVertices[uOutCount++] = ComputeIntersection(
					vertexS,
					vertexP,
					plane,
					fS);
			}

			// Always include p if it is positive.
			pOutVertices[uOutCount++] = vertexP;
		}
		// Always include p if it is "inside"/intersects the clip
		// plane - don't handle s in this case, since it either
		// also intersects (and will be added when we consider it
		// as p) or is outside the clip plane, in which case
		// a projection would only generate a coincident point
		// with p.
		else if (fP >= -fTolerance)
		{
			pOutVertices[uOutCount++] = vertexP;
		}
		// If p is explicitly outside the clip plane, we generate
		// an intersection point if s is explicitly inside the
		// clip plane. Otherwise, we exclude p, since s is either
		// also outside the clip plane, or it intersects the clip
		// plane, in which case an intersection of p would be
		// coincident with s.
		else if (fS > fTolerance)
		{
			pOutVertices[uOutCount++] = ComputeIntersection(
				vertexP,
				vertexS,
				plane,
				fP);
		}

		// Moving on.
		vertexS = vertexP;
		fS = fP;
	}

	// Done - count will be a value > 0 at this point.
	return (Int32)uOutCount;
}

/**
 * Sutherland-Hodgman clipping - input polygon is required to be convex,
 * even though Sutherland-Hodgman clipping will also produce correct
 * results when clipping concave polygons. The input requirement is
 * reasonable for Falcon and allows a simple estimate of the max size
 * of pOutVertices (# of input vertices + clip plane count - when clipping
 * convex against convex, at most 1 vertex can be added to the input
 * vertices per clipping plane).
 */
template <typename PLANE, typename T>
static Int32 DoConvexClip(
	StackOrHeapArray<T, kuMaxStackVertices, MemoryBudgets::Falcon>& aVertices0,
	StackOrHeapArray<T, kuMaxStackVertices, MemoryBudgets::Falcon>& aVertices1,
	PLANE const* pClipPlanes,
	UInt32 uClipPlaneCount,
	T const* pInVertices,
	UInt32 uInVertexCount,
	T* pOutVertices,
	Float fTolerance)
{
	// Sanity check.
	SEOUL_ASSERT(aVertices0.GetSize() == aVertices1.GetSize());

	T const* pIn = pInVertices;
	T* pOut = aVertices0.Get(0);
	T* pNextOut = aVertices1.Get(0);

	UInt32 uCurrentCount = uInVertexCount;

	// Clip against all planes of the convex clip shape.
	Int32 iReturn = 0;
	for (UInt32 i = 0u; i < uClipPlaneCount; ++i)
	{
		// Get the clip plane.
		const PLANE& plane = pClipPlanes[i];

		// Perform clip against the plane.
		Int32 const iResult = PlaneClip(
			plane,
			pIn,
			uCurrentCount,
			pOut,
			fTolerance);

		// Done immediately if all are clipped against
		// one plane.
		if (iResult < 0)
		{
			return iResult;
		}
		// Otherwise, swap buffers if some vertices were clipped.
		else if (iResult > 0)
		{
			// The return value is the last clip result in this case.
			iReturn = iResult;

			// Update count and ping-pong input and output buffers.
			uCurrentCount = (UInt32)iResult;
			pIn = pOut;
			Swap(pOut, pNextOut);
		}
	}

	// Sanity check.
	SEOUL_ASSERT(iReturn <= (Int32)aVertices0.GetSize());

	// If iReturn > 0, append the output to the output buffer.
	// The last clipped output will be in the buffer at pIn.
	for (Int32 i = 0; i < iReturn; ++i)
	{
		pOutVertices[i] = pIn[i];
	}

	return iReturn;
}

template <typename PLANE, typename T>
static Int32 DoConvexClip(
	PLANE const* pClipPlanes,
	UInt32 uClipPlaneCount,
	T const* pInVertices,
	UInt32 uInVertexCount,
	T* pOutVertices,
	Float fTolerance)
{
	// Utility used for intermediate output.
	typedef StackOrHeapArray<T, kuMaxStackVertices, MemoryBudgets::Falcon> StackOrHeapArrayType;

	// If no clip planes, all clipped.
	if (0u == uClipPlaneCount)
	{
		return -1;
	}

	// If no input vertices, none clipped.
	if (0u == uInVertexCount)
	{
		return 0;
	}

	// Work area needs enough for max output, which is in + clip plane count.
	UInt32 const uWorkAreaCount = (uClipPlaneCount + uInVertexCount);
	StackOrHeapArrayType aVertices0(uWorkAreaCount);
	StackOrHeapArrayType aVertices1(uWorkAreaCount);

	return DoConvexClip(aVertices0, aVertices1, pClipPlanes, uClipPlaneCount, pInVertices, uInVertexCount, pOutVertices, fTolerance);
}

/**
  * Utility when using clip functions outside of a clip stack.
  * Generates clipping planes from a list of convex points.
  *
  * \pre pClipPlanes must be uClipVertexCount in length.
  */
void ComputeClipPlanes(
	Vector2D const* pClipVertices,
	UInt32 uClipVertexCount,
	Vector3D* pClipPlanes)
{
	// Nop if no vertices.
	if (0u == uClipVertexCount)
	{
		return;
	}

	// Compute a plane per line segment, starting at (uEnd-1 -> 0u)
	UInt32 uPrev = (uClipVertexCount - 1u);
	for (UInt32 uV = 0u; uV < uClipVertexCount; ++uV)
	{
		// Cache the two vertices of the segment.
		auto const& v0 = pClipVertices[uPrev];
		auto const& v1 = pClipVertices[uV];

		// Compute the plane.
		Vector2D const vNormal(Vector2D::Normalize(Vector2D::Perpendicular(v0 - v1)));
		Vector3D const vPlane(vNormal, Vector2D::Dot(-vNormal, v0));

		// Assign.
		pClipPlanes[uV] = vPlane;

		// Advance.
		uPrev = uV;
	}
}

// ConvexClip variations fo different plane and vertex
// types.

Int32 ConvexClip(
	Vector3D const* pClipPlanes,
	UInt32 uClipPlaneCount,
	Vector2D const* pInVertices,
	UInt32 uInVertexCount,
	Vector2D* pOutVertices,
	Float fTolerance /*= kfAboutEqualPosition*/)
{
	return DoConvexClip(pClipPlanes, uClipPlaneCount, pInVertices, uInVertexCount, pOutVertices, fTolerance);
}

Int32 ConvexClip(
	Vector3D const* pClipPlanes,
	UInt32 uClipPlaneCount,
	ShapeVertex const* pInVertices,
	UInt32 uInVertexCount,
	ShapeVertex* pOutVertices,
	Float fTolerance /*= kfAboutEqualPosition*/)
{
	return DoConvexClip(pClipPlanes, uClipPlaneCount, pInVertices, uInVertexCount, pOutVertices, fTolerance);
}

Int32 ConvexClip(
	const Rectangle& clipRectangle,
	Vector2D const* pInVertices,
	UInt32 uInVertexCount,
	Vector2D* pOutVertices,
	Float fTolerance /*= kfAboutEqualPosition*/)
{
	// Operations - clip the input polygon against each plane of the clip rectangle.
	Plane1D aPlanes[4] =
	{
		{ 0, clipRectangle.m_fLeft, 1.0f },
		{ 1, clipRectangle.m_fBottom, -1.0f },
		{ 0, clipRectangle.m_fRight, -1.0f },
		{ 1, clipRectangle.m_fTop, 1.0f },
	};

	return DoConvexClip(aPlanes, 4u, pInVertices, uInVertexCount, pOutVertices, fTolerance);
}

Int32 ConvexClip(
	const Rectangle& clipRectangle,
	ShapeVertex const* pInVertices,
	UInt32 uInVertexCount,
	ShapeVertex* pOutVertices,
	Float fTolerance /*= kfAboutEqualPosition*/)
{
	// Operations - clip the input polygon against each plane of the clip rectangle.
	Plane1D aPlanes[4] =
	{
		{ 0, clipRectangle.m_fLeft, 1.0f },
		{ 1, clipRectangle.m_fBottom, -1.0f },
		{ 0, clipRectangle.m_fRight, -1.0f },
		{ 1, clipRectangle.m_fTop, 1.0f },
	};

	return DoConvexClip(aPlanes, 4u, pInVertices, uInVertexCount, pOutVertices, fTolerance);
}

/**
 * Given a count, generates a triangle fan index sequence.
 * This is the appropriate index buffer for any convex shape.
 */
static inline void AppendConvexIndices(
	Indices& rvIndices,
	UInt32 uCount,
	UInt16 uBase)
{
	for (UInt32 i = 2; i < uCount; ++i)
	{
		rvIndices.PushBack((UInt16)(uBase + 0));
		rvIndices.PushBack((UInt16)(uBase + i - 1));
		rvIndices.PushBack((UInt16)(uBase + i - 0));
	}
}

/**
 * Variation of mesh clipping for a target shape that is convex.
 */
template <typename PLANE, typename VERTEX>
static void DoMeshClipConvex(
	MeshClipCacheT<VERTEX>& rCache,
	PLANE const* pClipPlanes,
	UInt32 uClipPlaneCount,
	Indices& rvIndices,
	UInt32 uIndexCount,
	UnsafeBuffer<VERTEX, MemoryBudgets::Falcon>& rvVertices,
	UInt32 uVertexCount,
	Float fTolerance)
{
	UInt32 const uStartingVertices = rvVertices.GetSize();
	UInt32 const uBegin = (uStartingVertices - uVertexCount);
	rvVertices.ResizeNoInitialize(uStartingVertices + uClipPlaneCount);

	Int32 const iResult = DoConvexClip(
		pClipPlanes,
		uClipPlaneCount,
		rvVertices.Get(uBegin),
		uVertexCount,
		rvVertices.Get(uBegin),
		fTolerance);

	// All clipped, shrink buffers and return.
	if (iResult < 0)
	{
		rvIndices.ResizeNoInitialize(rvIndices.GetSize() - uIndexCount);
		rvVertices.ResizeNoInitialize(uBegin);
	}
	// Some clipped, trim vertices and regenerate indices.
	else if (iResult > 0)
	{
		// Sanity check that our worst case estimate was correct.
		SEOUL_ASSERT(uBegin + (UInt32)iResult <= rvVertices.GetSize());

		// Trim vertices.
		rvVertices.ResizeNoInitialize(uBegin + (UInt32)iResult);

		// Generate indices for the clipped vertices. All triangle fans.
		rvIndices.ResizeNoInitialize(rvIndices.GetSize() - uIndexCount);
		AppendConvexIndices(rvIndices, (UInt32)iResult, uBegin);
	}
	// None clipped, just resize vertices back to what it was.
	else
	{
		rvVertices.ResizeNoInitialize(uStartingVertices);
	}
}

/**
 * Variation of mesh clipping for a target shape that is
 * a list of quads.
 */
template <typename PLANE, typename VERTEX>
static void DoMeshClipQuadList(
	MeshClipCacheT<VERTEX>& rCache,
	PLANE const* pClipPlanes,
	UInt32 uClipPlaneCount,
	Indices& rvIndices,
	UInt32 uIndexCount,
	UnsafeBuffer<VERTEX, MemoryBudgets::Falcon>& rvVertices,
	UInt32 uVertexCount,
	Float fTolerance)
{
	// Sanity check - uVertexCount must be a multiple of 4, and
	// uIndexCount must be a multiple of 6, or kQuadList was
	// an incorrect designation.
	SEOUL_ASSERT(uIndexCount % 6u == 0u);
	SEOUL_ASSERT(uVertexCount % 4u == 0u);

	UInt32 const uEndI = rvIndices.GetSize();
	UInt32 const uEndV = rvVertices.GetSize();

	SEOUL_ASSERT(uIndexCount <= uEndI);
	SEOUL_ASSERT(uVertexCount <= uEndV);

	UInt32 const uBeginI = (uEndI - uIndexCount);
	UInt32 const uBeginV = (uEndV - uVertexCount);

	// Prepare our cache.
	rCache.m_vClipIndices.Clear();
	rCache.m_vClipVertices.Clear();
	auto& vClipI(rCache.m_vClipIndices);
	auto& vClipV(rCache.m_vClipVertices);

	// Utility used for intermediate output.
	typedef StackOrHeapArray<VERTEX, kuMaxStackVertices, MemoryBudgets::Falcon> StackOrHeapArrayType;
	UInt32 const uWorkAreaCount = uClipPlaneCount + 4u;

	// Our work area.
	StackOrHeapArrayType aWorkArea(uWorkAreaCount);

	// Shared work area for DoConvexClip().
	StackOrHeapArrayType aConvexVertices0(uWorkAreaCount);
	StackOrHeapArrayType aConvexVertices1(uWorkAreaCount);

	// Process. We don't start populating vClipIndices or vClipVertices
	// until clipping actually occurs.
	UInt32 uI = uBeginI;
	UInt32 uV = uBeginV;
	UInt32 uClipI = uEndI;
	UInt32 uClipV = uEndV;
	Bool bClipping = false;
	UInt32 uTotalOut = 0u;
	for (; uI < uEndI; uI += 6u, uV += 4u)
	{
		Int32 const iResult = DoConvexClip(
			aConvexVertices0,
			aConvexVertices1,
			pClipPlanes,
			uClipPlaneCount,
			rvVertices.Get(uV),
			4u,
			aWorkArea.Data(),
			fTolerance);

		// If any clipping occured, or if we're already clipping,
		// need to populate vClipIndices and vClipVertices.
		if (bClipping || iResult != 0)
		{
			if (iResult >= 0)
			{
				auto const* pIn = (0 == iResult ? rvVertices.Get(uV) : aWorkArea.Data());
				UInt32 const uAppend = (0 == iResult ? 4u : (UInt32)iResult);
				UInt32 const uClipOut = (vClipV.GetSize());
				UInt32 const uBase = (uTotalOut + uBeginV);

				vClipV.ResizeNoInitialize(uClipOut + uAppend);
				memcpy(vClipV.Get(uClipOut), pIn, uAppend * sizeof(ShapeVertex));
				AppendConvexIndices(vClipI, uAppend, uBase);
			}

			// Setup the start of clipping.
			if (!bClipping)
			{
				uClipI = uI;
				uClipV = uV;
				bClipping = true;
			}
		}

		uTotalOut += (iResult == 0
			? 4u
			: (iResult > 0 ? (UInt32)iResult : 0));
	}

	// If bClipping, need to replace a subset of indices and vertices
	// with the clipped output.
	if (bClipping)
	{
		rvIndices.ResizeNoInitialize(uClipI);
		rvIndices.Append(vClipI);
		rvVertices.ResizeNoInitialize(uClipV);
		rvVertices.Append(vClipV);
	}
}

/**
 * (Most expensive case) Variation of mesh clipping for
 * an arbitrary triangle mesh.
 */
template <typename PLANE, typename VERTEX>
static void DoMeshClipNotSpecific(
	MeshClipCacheT<VERTEX>& rCache,
	PLANE const* pClipPlanes,
	UInt32 uClipPlaneCount,
	Indices& rvIndices,
	UInt32 uIndexCount,
	UnsafeBuffer<VERTEX, MemoryBudgets::Falcon>& rvVertices,
	UInt32 uVertexCount,
	Float fTolerance)
{
	// Sanity check - uIndexCount must be a multiple of 3
	// for arbitrary triangle lists.
	SEOUL_ASSERT(uIndexCount % 3u == 0u);

	UInt32 const uEndI = rvIndices.GetSize();
	UInt32 const uEndV = rvVertices.GetSize();

	SEOUL_ASSERT(uIndexCount <= uEndI);
	SEOUL_ASSERT(uVertexCount <= uEndV);

	UInt32 const uBeginI = (uEndI - uIndexCount);
	UInt32 const uBeginV = (uEndV - uVertexCount);

	// Prepare our cache.
	rCache.m_Builder.Reset(uBeginV, uVertexCount);
	auto& builder(rCache.m_Builder);
	auto& vClipI(rCache.m_vClipIndices);
	auto& vClipV(rCache.m_vClipVertices);

	// Utility used for intermediate output.
	typedef StackOrHeapArray< VertexWrapper<VERTEX>, kuMaxStackVertices, MemoryBudgets::Falcon> StackOrHeapArrayType;
	UInt32 const uWorkAreaCount = uClipPlaneCount + 3u;

	// Our work area.
	StackOrHeapArrayType aWorkArea(uWorkAreaCount);

	// Shared work area for DoConvexClip().
	StackOrHeapArrayType aConvexVertices0(uWorkAreaCount);
	StackOrHeapArrayType aConvexVertices1(uWorkAreaCount);

	// TODO: We use DotCoordinate() below on all vertices
	// and then throw the results away. Ideally, we'd cache these
	// and then use the data in DoConvexClip() as needed.

	// Populate our inside acceleration list. This is used to early out
	// of convex culling if all vertices of a triangle are considered
	// inside the clipping planes.
	rCache.m_vInside.Resize(uVertexCount);
	rCache.m_vInside.Fill(true);
	for (UInt32 i = 0u; i < uVertexCount; ++i)
	{
		auto const& v = GetVector2D(rvVertices[uBeginV + i]);
		for (UInt32 j = 0u; j < uClipPlaneCount; ++j)
		{
			// If a vertex is completely outside any candidate
			// plane, we set "inside" to false and break immediately.
			// This will force us to consider any triangles that use
			// this vertex with DoConvexClip().
			if (DotCoordinate(pClipPlanes[j], v) < -fTolerance)
			{
				rCache.m_vInside[i] = false;
				break;
			}
		}
	}

	// Process. We don't start populating vClipIndices or vClipVertices
	// until clipping actually occurs.
	UInt32 uI = uBeginI;
	Bool bClipping = false;
	for (; uI < uEndI; uI += 3u)
	{
		// Get the vertex offsets and their values in the work area.
		UInt16 const uV0 = rvIndices[uI + 0];
		UInt16 const uV1 = rvIndices[uI + 1];
		UInt16 const uV2 = rvIndices[uI + 2];
		aWorkArea[0] = VertexWrapper<VERTEX>(rvVertices[uV0]);
		aWorkArea[1] = VertexWrapper<VERTEX>(rvVertices[uV1]);
		aWorkArea[2] = VertexWrapper<VERTEX>(rvVertices[uV2]);

		// Track the inside/outside state of each vertex.
		Bool const bV0 = rCache.m_vInside[uV0 - uBeginV];
		Bool const bV1 = rCache.m_vInside[uV1 - uBeginV];
		Bool const bV2 = rCache.m_vInside[uV2 - uBeginV];

		// If all three vertices are inside all planes
		// (all booleans are == true), then we can
		// immediately consider this triangle not clipped.
		Int32 iResult;
		if (bV0 && bV1 && bV2)
		{
			iResult = 0;
		}
		// Otherwise, we need to perform standard clipping on
		// the triangle.
		else
		{
			// To help with regenerating the render mesh, we
			// use the reserved slot to give the input vertices
			// their index. On output, these values will only
			// be set if a vertex is an original input vertex
			// (vs. a clipped vertex).
			aWorkArea[0].m_uOrigIndex = (uV0 + 1u);
			aWorkArea[1].m_uOrigIndex = (uV1 + 1u);
			aWorkArea[2].m_uOrigIndex = (uV2 + 1u);
			iResult = DoConvexClip(
				aConvexVertices0,
				aConvexVertices1,
				pClipPlanes,
				uClipPlaneCount,
				aWorkArea.Data(),
				3u,
				aWorkArea.Data(),
				fTolerance);
		}

		// If clipping occured and we are not yet clipping,
		// need to "prime" the clip indices and vertices.
		if (iResult != 0 && !bClipping)
		{
			// Fill clip indices will all indices up to this point,
			// fill clip vertices with all existing vertices.
			vClipI.Append(rvIndices.Get(uBeginI), rvIndices.Get(uI));
			vClipV.Append(rvVertices.Get(uBeginV), rvVertices.End());
			bClipping = true;
		}

		// Skip vertices if all clipped.
		if (iResult < 0)
		{
			continue;
		}
		// Otherwise, compute count and process, if clipping.
		else if (bClipping)
		{
			// On a no-clip result, we can just copy through
			// the existing three indices.
			if (0 == iResult)
			{
				auto pBeginI = rvIndices.Get(uI);
				vClipI.Append(pBeginI, pBeginI + 3);
			}
			// Otherwise, we need to merge the generate vertex
			// set.
			else
			{
				UInt32 const uVertexCount = (UInt32)iResult;
				for (UInt32 j = 2u; j < uVertexCount; ++j)
				{
					// In all three cases,
					// if the reserved field is non-zero, we can
					// use it without a (more expensive) merge
					// operation. Otherwise, we need to merge,
					// which will hash the vertex and try
					// to eliminate duplicates.
					if (aWorkArea[0].m_uOrigIndex == 0u)
					{
						builder.MergeVertex(aWorkArea[0].GetVertex());
					}
					else
					{
						vClipI.PushBack((UInt16)(aWorkArea[0].m_uOrigIndex - 1u));
					}

					if (aWorkArea[j - 1].m_uOrigIndex == 0u)
					{
						builder.MergeVertex(aWorkArea[j - 1].GetVertex());
					}
					else
					{
						vClipI.PushBack((UInt16)(aWorkArea[j - 1].m_uOrigIndex - 1u));
					}

					if (aWorkArea[j].m_uOrigIndex == 0u)
					{
						builder.MergeVertex(aWorkArea[j].GetVertex());
					}
					else
					{
						vClipI.PushBack((UInt16)(aWorkArea[j].m_uOrigIndex - 1u));
					}
				}
			}
		}
	}

	// Done, replace output with clipped, if clipping occurred.
	if (bClipping)
	{
		// Need to optimize and compact the vertex and index buffers. This is akin to
		// a garbage collection pass. We want the final vertices to only contain used
		// vertices and for them to be ordered efficiently.

		// Prune existing indices and vertices.
		rvIndices.ResizeNoInitialize(uBeginI);
		rvVertices.ResizeNoInitialize(uBeginV);

		// Size our remap table.
		rCache.m_vRemap.Clear();
		rCache.m_vRemap.Resize(vClipV.GetSize(), -1);

		// Now remap and insert indices and vertices into the
		// final output.
		Int32 iNextRemap = (Int32)uBeginV;
		for (auto ii = vClipI.Begin(); vClipI.End() != ii; ++ii)
		{
			Int32 const iV = (Int32)*ii;
			Int32 const iVrel = iV - (Int32)uBeginV;
			Int32 iRemap = rCache.m_vRemap[iVrel];
			if (iRemap < 0)
			{
				rCache.m_vRemap[iVrel] = iRemap = iNextRemap;
				++iNextRemap;

				rvVertices.PushBack(vClipV[iVrel]);
			}

			rvIndices.PushBack((UInt16)iRemap);
		}
	}
}

/**
 * Variation handler for mesh clipping. Dispatches to a specialized
 * function depending on clipped target type.
 */
template <typename PLANE, typename VERTEX>
static void DoMeshClip(
	MeshClipCacheT<VERTEX>& rCache,
	PLANE const* pClipPlanes,
	UInt32 uClipPlaneCount,
	TriangleListDescription eDescription,
	Indices& rvIndices,
	UInt32 uIndexCount,
	UnsafeBuffer<VERTEX, MemoryBudgets::Falcon>& rvVertices,
	UInt32 uVertexCount,
	Float fTolerance)
{
	// Special cases - if convex, or if a quad list with 1 entry, perform the clipping in place.
	if (eDescription == TriangleListDescription::kConvex ||
		(eDescription == TriangleListDescription::kQuadList && uVertexCount == 4u) ||
		(eDescription == TriangleListDescription::kTextChunk && uVertexCount == 4u))
	{
		DoMeshClipConvex(rCache, pClipPlanes, uClipPlaneCount, rvIndices, uIndexCount, rvVertices, uVertexCount, fTolerance);
	}
	// Otherwise, if a quad list or text chunk, need temporary buffers but no merging.
	else if (TriangleListDescription::kQuadList == eDescription || TriangleListDescription::kTextChunk == eDescription)
	{
		DoMeshClipQuadList(rCache, pClipPlanes, uClipPlaneCount, rvIndices, uIndexCount, rvVertices, uVertexCount, fTolerance);
	}
	// Most complex case, need temporary buffers and merging.
	else
	{
		DoMeshClipNotSpecific(rCache, pClipPlanes, uClipPlaneCount, rvIndices, uIndexCount, rvVertices, uVertexCount, fTolerance);
	}
}

// Second tier.
template <typename PLANE, typename VERTEX>
static inline void MeshClipInner(
	MeshClipCacheT<VERTEX>& rCache,
	PLANE const* pClipPlanes,
	UInt32 uClipPlaneCount,
	TriangleListDescription eDescription,
	Indices& rvIndices,
	UInt32 uIndexCount,
	UnsafeBuffer<VERTEX, MemoryBudgets::Falcon>& rvVertices,
	UInt32 uVertexCount,
	Float fTolerance)
{
	DoMeshClip(rCache, pClipPlanes, uClipPlaneCount, eDescription, rvIndices, uIndexCount, rvVertices, uVertexCount, fTolerance);
}

template <typename VERTEX>
static inline void MeshClipInner(
	MeshClipCacheT<VERTEX>& rCache,
	const Rectangle& clipRectangle,
	TriangleListDescription eDescription,
	Indices& rvIndices,
	UInt32 uIndexCount,
	UnsafeBuffer<VERTEX, MemoryBudgets::Falcon>& rvVertices,
	UInt32 uVertexCount,
	Float fTolerance)
{
	// Operations - clip the input polygon against each plane of the clip rectangle.
	Plane1D aPlanes[4] =
	{
		{ 0, clipRectangle.m_fLeft, 1.0f },
		{ 1, clipRectangle.m_fBottom, -1.0f },
		{ 0, clipRectangle.m_fRight, -1.0f },
		{ 1, clipRectangle.m_fTop, 1.0f },
	};

	DoMeshClip(rCache, aPlanes, 4u, eDescription, rvIndices, uIndexCount, rvVertices, uVertexCount, fTolerance);
}

template <typename VERTEX>
static inline void MeshClipInner(
	MeshClipCacheT<VERTEX>& rCache,
	const Rectangle& clipRectangle,
	TriangleListDescription eDescription,
	const Rectangle& vertexBounds,
	Indices& rvIndices,
	UInt32 uIndexCount,
	UnsafeBuffer<VERTEX, MemoryBudgets::Falcon>& rvVertices,
	UInt32 uVertexCount,
	Float fTolerance)
{
	// Test each clip plane against the vertexBounds.
	UInt32 uPlanes = 0u;
	Plane1D aPlanes[4];

	// Left plane fully clips
	if (vertexBounds.m_fRight <= clipRectangle.m_fLeft)
	{
		rvIndices.ResizeNoInitialize(rvIndices.GetSize() - uIndexCount);
		rvVertices.ResizeNoInitialize(rvVertices.GetSize() - uVertexCount);
		return;
	}
	// Left plane partially clips.
	else if (vertexBounds.m_fLeft < clipRectangle.m_fLeft)
	{
		aPlanes[uPlanes++] = Plane1D::Create(0, clipRectangle.m_fLeft, 1.0f);
	}

	// Bottom plane fully clips
	if (vertexBounds.m_fTop >= clipRectangle.m_fBottom)
	{
		rvIndices.ResizeNoInitialize(rvIndices.GetSize() - uIndexCount);
		rvVertices.ResizeNoInitialize(rvVertices.GetSize() - uVertexCount);
		return;
	}
	// Left plane partially clips.
	else if (vertexBounds.m_fBottom > clipRectangle.m_fBottom)
	{
		aPlanes[uPlanes++] = Plane1D::Create(1, clipRectangle.m_fBottom, -1.0f);
	}

	// Right plane fully clips
	if (vertexBounds.m_fLeft >= clipRectangle.m_fRight)
	{
		rvIndices.ResizeNoInitialize(rvIndices.GetSize() - uIndexCount);
		rvVertices.ResizeNoInitialize(rvVertices.GetSize() - uVertexCount);
		return;
	}
	// Right plane partially clips.
	else if (vertexBounds.m_fRight > clipRectangle.m_fRight)
	{
		aPlanes[uPlanes++] = Plane1D::Create(0, clipRectangle.m_fRight, -1.0f);
	}

	// Top plane fully clips
	if (vertexBounds.m_fBottom <= clipRectangle.m_fTop)
	{
		rvIndices.ResizeNoInitialize(rvIndices.GetSize() - uIndexCount);
		rvVertices.ResizeNoInitialize(rvVertices.GetSize() - uVertexCount);
		return;
	}
	// Top plane partially clips.
	else if (vertexBounds.m_fTop < clipRectangle.m_fTop)
	{
		aPlanes[uPlanes++] = Plane1D::Create(1, clipRectangle.m_fTop, 1.0f);
	}

	// Nothing clipped.
	if (0u == uPlanes)
	{
		return;
	}

	DoMeshClip(rCache, aPlanes, uPlanes, eDescription, rvIndices, uIndexCount, rvVertices, uVertexCount, fTolerance);
}

// MeshClip variations fo different plane and vertex
// types.

void MeshClip(
	MeshClipCacheT<ShapeVertex>& rCache,
	Vector3D const* pClipPlanes,
	UInt32 uClipPlaneCount,
	TriangleListDescription eDescription,
	Indices& rvIndices,
	UInt32 uIndexCount,
	UnsafeBuffer<ShapeVertex, MemoryBudgets::Falcon>& rvVertices,
	UInt32 uVertexCount,
	Float fTolerance /*= kfAboutEqualPosition*/)
{
	MeshClipInner(rCache, pClipPlanes, uClipPlaneCount, eDescription, rvIndices, uIndexCount, rvVertices, uVertexCount, fTolerance);
}

void MeshClip(
	MeshClipCacheT<ShapeVertex>& rCache,
	const Rectangle& clipRectangle,
	TriangleListDescription eDescription,
	Indices& rvIndices,
	UInt32 uIndexCount,
	UnsafeBuffer<ShapeVertex, MemoryBudgets::Falcon>& rvVertices,
	UInt32 uVertexCount,
	Float fTolerance /*= kfAboutEqualPosition*/)
{
	MeshClipInner(rCache, clipRectangle, eDescription, rvIndices, uIndexCount, rvVertices, uVertexCount, fTolerance);
}

void MeshClip(
	MeshClipCacheT<ShapeVertex>& rCache,
	const Rectangle& clipRectangle,
	TriangleListDescription eDescription,
	const Rectangle& vertexBounds,
	Indices& rvIndices,
	UInt32 uIndexCount,
	UnsafeBuffer<ShapeVertex, MemoryBudgets::Falcon>& rvVertices,
	UInt32 uVertexCount,
	Float fTolerance /*= kfAboutEqualPosition*/)
{
	MeshClipInner(rCache, clipRectangle, eDescription, vertexBounds, rvIndices, uIndexCount, rvVertices, uVertexCount, fTolerance);
}

void MeshClip(
	MeshClipCacheT<UtilityVertex>& rCache,
	Vector3D const* pClipPlanes,
	UInt32 uClipPlaneCount,
	TriangleListDescription eDescription,
	Indices& rvIndices,
	UInt32 uIndexCount,
	UnsafeBuffer<UtilityVertex, MemoryBudgets::Falcon>& rvVertices,
	UInt32 uVertexCount,
	Float fTolerance /* = kfAboutEqualPosition*/)
{
	MeshClipInner(rCache, pClipPlanes, uClipPlaneCount, eDescription, rvIndices, uIndexCount, rvVertices, uVertexCount, fTolerance);
}

void MeshClip(
	MeshClipCacheT<UtilityVertex>& rCache,
	const Rectangle& clipRectangle,
	TriangleListDescription eDescription,
	Indices& rvIndices,
	UInt32 uIndexCount,
	UnsafeBuffer<UtilityVertex, MemoryBudgets::Falcon>& rvVertices,
	UInt32 uVertexCount,
	Float fTolerance /* = kfAboutEqualPosition*/)
{
	MeshClipInner(rCache, clipRectangle, eDescription, rvIndices, uIndexCount, rvVertices, uVertexCount, fTolerance);
}

void MeshClip(
	MeshClipCacheT<UtilityVertex>& rCache,
	const Rectangle& clipRectangle,
	TriangleListDescription eDescription,
	const Rectangle& vertexBounds,
	Indices& rvIndices,
	UInt32 uIndexCount,
	UnsafeBuffer<UtilityVertex, MemoryBudgets::Falcon>& rvVertices,
	UInt32 uVertexCount,
	Float fTolerance /* = kfAboutEqualPosition*/)
{
	MeshClipInner(rCache, clipRectangle, eDescription, vertexBounds, rvIndices, uIndexCount, rvVertices, uVertexCount, fTolerance);
}

} // namespace Clipper

ClipStack::ClipStack()
	: m_pCache(SEOUL_NEW(MemoryBudgets::Falcon) Clipper::MeshClipCacheT<ShapeVertex>)
	, m_Pending()
	, m_vHulls()
	, m_vPlanes()
	, m_vStack()
	, m_vVertices()
{
}

ClipStack::~ClipStack()
{
}

/**
 * Apply the current clip stack to an arbitrary mesh, defined
 * by index and vertex buffers.
 */
void ClipStack::MeshClip(
	TriangleListDescription eDescription,
	Clipper::Indices& rvIndices,
	Clipper::Vertices& rvVertices,
	Int32 iIndexCount /*= -1*/,
	Int32 iVertexCount /*= -1*/,
	Float fTolerance /*= kfAboutEqualPosition*/)
{
	iIndexCount = (iIndexCount >= 0 ? iIndexCount : (Int32)rvIndices.GetSize());
	iVertexCount = (iVertexCount >= 0 ? iVertexCount : (Int32)rvVertices.GetSize());

	if (m_vStack.IsEmpty() || 0 == iIndexCount || 0 == iVertexCount)
	{
		return;
	}

	auto const& e = m_vStack.Back();
	if (e.m_bSimple)
	{
		Clipper::MeshClip(
			*m_pCache,
			e.m_Bounds,
			eDescription,
			rvIndices,
			(UInt32)iIndexCount,
			rvVertices,
			(UInt32)iVertexCount);
	}
	else
	{
		// Simple case, only 1 convex hull.
		if (1u == e.m_uHulls)
		{
			Clipper::MeshClip(
				*m_pCache,
				m_vPlanes.Get(e.m_uFirstVertex),
				e.m_uVertices,
				eDescription,
				rvIndices,
				(UInt32)iIndexCount,
				rvVertices,
				(UInt32)iVertexCount);
		}
		// For multiple hulls:
		// - for each hull, copy the original mesh state into staging buffers.
		// - clip against the hull.
		// - if non-zero, merge the result into the final buffer
		else
		{
			UInt32 const uEndI = rvIndices.GetSize();
			UInt32 const uEndV = rvVertices.GetSize();
			UInt32 const uBeginI = (uEndI - (UInt32)iIndexCount);
			UInt32 const uBeginV = (uEndV - (UInt32)iVertexCount);

			UInt32 uFirstPlane = e.m_uFirstVertex;
			for (UInt32 i = 0u; i < e.m_uHulls; ++i)
			{
				// Get the number of vertices in this convex hull.
				UInt32 const uClipPlanes = m_vHulls[e.m_uFirstHull + i];

				// Cache current offset.
				auto const uStartI = m_vScratchIndices.GetSize();
				auto const uStartV = m_vScratchVertices.GetSize();

				// Append the full mesh we're about to clip to the current scratch buffers.
				m_vScratchIndices.Append(rvIndices.Begin() + uBeginI, rvIndices.Begin() + uEndI);
				m_vScratchVertices.Append(rvVertices.Begin() + uBeginV, rvVertices.Begin() + uEndV);

				// Offset indices.
				for (auto i = m_vScratchIndices.Get(uStartI); m_vScratchIndices.End() != i; ++i)
				{
					*i = (*i - uBeginV + uStartV);
				}

				// Clip the appended vertices.
				Clipper::MeshClip(
					*m_pCache,
					m_vPlanes.Get(uFirstPlane),
					uClipPlanes,
					eDescription,
					m_vScratchIndices,
					(UInt32)iIndexCount,
					m_vScratchVertices,
					(UInt32)iVertexCount);

				// Advance to the next convex hull.
				uFirstPlane += uClipPlanes;
			}

			// Final step, rebase final indices to output.
			for (auto i = m_vScratchIndices.Begin(); m_vScratchIndices.End() != i; ++i)
			{
				*i += uBeginV;
			}

			// Finally, replace the indices and vertices with the total contents of the scratch buffers.
			rvIndices.ResizeNoInitialize(uBeginI);
			rvIndices.Append(m_vScratchIndices);
			rvVertices.ResizeNoInitialize(uBeginV);
			rvVertices.Append(m_vScratchVertices);

			m_vScratchIndices.Clear();
			m_vScratchVertices.Clear();
		}
	}
}

/**
 * Apply the current clip stack to an arbitrary mesh, defined
 * by index and vertex buffers.
 */
void ClipStack::MeshClip(
	TriangleListDescription eDescription,
	const Rectangle& vertexBounds,
	Clipper::Indices& rvIndices,
	Clipper::Vertices& rvVertices,
	Int32 iIndexCount /*= -1*/,
	Int32 iVertexCount /*= -1*/,
	Float fTolerance /*= kfAboutEqualPosition*/)
{
	iIndexCount = (iIndexCount >= 0 ? iIndexCount : (Int32)rvIndices.GetSize());
	iVertexCount = (iVertexCount >= 0 ? iVertexCount : (Int32)rvVertices.GetSize());

	if (m_vStack.IsEmpty() || 0 == iIndexCount || 0 == iVertexCount)
	{
		return;
	}

	auto const& e = m_vStack.Back();

	// Use the overriden MeshClip() variation with bounds in the simple case.
	if (e.m_bSimple)
	{
		Clipper::MeshClip(
			*m_pCache,
			e.m_Bounds,
			eDescription,
			vertexBounds,
			rvIndices,
			(UInt32)iIndexCount,
			rvVertices,
			(UInt32)iVertexCount);
	}
	// Otherwise, clip normally.
	else
	{
		MeshClip(
			eDescription,
			rvIndices,
			rvVertices,
			iIndexCount,
			iVertexCount,
			fTolerance);
	}
}

/**
 * Remove a frame from the clip stack.
 *
 * \pre HasClips() must be true.
 */
void ClipStack::Pop()
{
	auto const e = m_vStack.Back();

	m_vStack.PopBack();
	m_vHulls.Resize(m_vHulls.GetSize() - e.m_uHulls);
	m_vPlanes.Resize(m_vPlanes.GetSize() - e.m_uVertices);
	m_vVertices.Resize(m_vVertices.GetSize() - e.m_uVertices);

	// Sanity check that we're managing members correctly.
	SEOUL_ASSERT(m_vVertices.GetSize() == m_vPlanes.GetSize());

	m_Pending.m_uFirstHull = m_vHulls.GetSize();
	m_Pending.m_uFirstVertex = m_vVertices.GetSize();
}

/**
 * Apply the pending clip to a new clip stack frame and return true,
 * or return false if the new frame would be zero sized.
 *
 * The caller *must* call a corresponding Pop() when
 * Push() returns true, and must *not* call a corresponding Pop()
 * when Push() return false.
 */
Bool ClipStack::Push()
{
	// Finalize the pending clip state.
	{
		auto& e = m_Pending;

		// Compute bounds of the compound shape.
		e.m_Bounds = Rectangle::InverseMax();
		auto const iBegin = m_vVertices.Begin() + e.m_uFirstVertex;
		auto const iEnd = (iBegin + e.m_uVertices);
		for (auto i = iBegin; iEnd != i; ++i)
		{
			e.m_Bounds.AbsorbPoint(*i);
		}

		// Compute if the shape is simple (all vertices fall on the bounds
		// of the shape, meaning it is an axis-aligned rectangle).
		e.m_bSimple = false;
		if (1u == e.m_uHulls && 4u == e.m_uVertices)
		{
			e.m_bSimple = true;
			for (auto i = iBegin; iEnd != i; ++i)
			{
				Float const fX = i->X;
				if (!Seoul::Equals(e.m_Bounds.m_fLeft, fX, kfAboutEqualPosition) &&
					!Seoul::Equals(e.m_Bounds.m_fRight, fX, kfAboutEqualPosition))
				{
					e.m_bSimple = false;
					break;
				}

				Float32 const fY = i->Y;
				if (!Seoul::Equals(e.m_Bounds.m_fBottom, fY, kfAboutEqualPosition) &&
					!Seoul::Equals(e.m_Bounds.m_fTop, fY, kfAboutEqualPosition))
				{
					e.m_bSimple = false;
					break;
				}
			}
		}
	}

	// Early out if pending has zero sized bounds or no data.
	if (m_Pending.m_uHulls == 0u || m_Pending.m_uVertices == 0u ||
		m_Pending.m_Bounds.GetHeight() <= 0.0f ||
		m_Pending.m_Bounds.GetWidth() <= 0.0f)
	{
		// If we early out here, clean up the hulls and vertices
		m_vHulls.Resize(m_vHulls.GetSize() - m_Pending.m_uHulls);
		m_vVertices.Resize(m_vVertices.GetSize() - m_Pending.m_uVertices);

		m_Pending = State();
		m_Pending.m_uFirstHull = m_vHulls.GetSize();
		m_Pending.m_uFirstVertex = m_vVertices.GetSize();
		return false;
	}

	// Sanity check - should be the last n hulls.
	SEOUL_ASSERT(m_Pending.m_uFirstHull + m_Pending.m_uHulls == m_vHulls.GetSize());

	// Sanity check - should be the last n vertices.
	SEOUL_ASSERT(m_Pending.m_uFirstVertex + m_Pending.m_uVertices == m_vVertices.GetSize());

	// If simple remove vertices and hulls. We want m_vPlanes and m_vVertices to be exactly
	// in sync, and we don't want any hull data associated with a simple (bounding volume only)
	// clipper.
	if (m_Pending.m_bSimple)
	{
		m_vHulls.Resize(m_vHulls.GetSize() - m_Pending.m_uHulls);
		m_Pending.m_uFirstHull = 0u;
		m_Pending.m_uHulls = 0u;
		m_vVertices.Resize(m_vVertices.GetSize() - m_Pending.m_uVertices);
		m_Pending.m_uFirstVertex = 0u;
		m_Pending.m_uVertices = 0u;
	}
	// Otherwise, generate planes for clipping.
	else
	{
		// Sanity check, should have been enforced aboved.
		SEOUL_ASSERT(m_Pending.m_uVertices > 0u);

		UInt32 const uEndV = m_vVertices.GetSize();
		UInt32 const uEndHull = (m_Pending.m_uFirstHull + m_Pending.m_uHulls);

		m_vPlanes.Resize(uEndV);
		UInt32 uV = m_Pending.m_uFirstVertex;
		for (UInt32 uHull = m_Pending.m_uFirstHull; uHull < uEndHull; ++uHull)
		{
			UInt32 const uEndHullV = (uV + m_vHulls[uHull]);

			// Sanity check - should not have been allowed.
			SEOUL_ASSERT(uEndHullV > uV);

			UInt32 uPrev = (uEndHullV - 1u);
			for (; uV < uEndHullV; ++uV)
			{
				auto const& v0 = m_vVertices[uPrev];
				auto const& v1 = m_vVertices[uV];
				Vector2D const vNormal(Vector2D::Normalize(Vector2D::Perpendicular(v0 - v1)));
				Vector3D const vPlane(vNormal, Vector2D::Dot(-vNormal, v0));

				m_vPlanes[uV] = vPlane;

				// Advance.
				uPrev = uV;
			}
		}

		// Sanity check that all vertices were consumed.
		SEOUL_ASSERT(uEndV == uV);
	}

	// Sanity check that we fixed up members correctly.
	SEOUL_ASSERT(m_vVertices.GetSize() == m_vPlanes.GetSize());

	m_vStack.PushBack(m_Pending);
	m_Pending = State();
	m_Pending.m_uFirstHull = m_vHulls.GetSize();
	m_Pending.m_uFirstVertex = m_vVertices.GetSize();
	return true;
}

void ClipStack::ClipHull(UInt32 uCount, Float fTolerance)
{
	SEOUL_ASSERT(uCount <= m_vVertices.GetSize());

	// If no existing stack, or if the count is 0, just add the hull,
	// there is nothing to clip.
	if (m_vStack.IsEmpty() || uCount == 0u)
	{
		if (uCount > 0u)
		{
			auto& e = m_Pending;
			e.m_uHulls++;
			e.m_uVertices += uCount;
			m_vHulls.PushBack(uCount);
		}
		return;
	}

	// Cache our clipping starting offset, and the clipping entry.
	UInt32 const uBegin = (m_vVertices.GetSize() - uCount);
	auto const& e = m_vStack.Back();

	// Simple cases - an axis-aligned rectangle or a single convexs hull.
	if (e.m_bSimple || 1u == e.m_uHulls)
	{
		// Expand rv by the worst case size.
		m_vVertices.Resize(m_vVertices.GetSize() + (e.m_bSimple ? 4u : e.m_uVertices));

		// Perform the clip.
		Int32 iResult = 0;
		if (e.m_bSimple)
		{
			iResult = Clipper::ConvexClip(
				e.m_Bounds,
				m_vVertices.Get(uBegin),
				uCount,
				m_vVertices.Get(uBegin),
				fTolerance);
		}
		else
		{
			iResult = Clipper::ConvexClip(
				m_vPlanes.Get(e.m_uFirstVertex),
				e.m_uVertices,
				m_vVertices.Get(uBegin),
				uCount,
				m_vVertices.Get(uBegin),
				fTolerance);
		}

		// Negative indicates all clipped. Hull is removed.
		if (iResult < 0)
		{
			m_vVertices.Resize(uBegin);
		}
		// Positive indicates a partial clip.
		else if (iResult > 0)
		{
			// Sanity check that our worst case estimate was correct.
			SEOUL_ASSERT(uBegin + (UInt32)iResult <= m_vVertices.GetSize());

			// Partial clip - add a new hull with the partial size.
			m_vVertices.Resize(uBegin + (UInt32)iResult);

			auto& e = m_Pending;
			e.m_uHulls++;
			e.m_uVertices += (UInt32)iResult;
			m_vHulls.PushBack((UInt32)iResult);
		}
		// Otherwise, no clip, add a hull for the entire shape.
		else
		{
			// The entire hull was left unclipped, so just add it unmodified.
			m_vVertices.Resize(uBegin + uCount);

			auto& e = m_Pending;
			e.m_uHulls++;
			e.m_uVertices += uCount;
			m_vHulls.PushBack(uCount);
		}
	}
	// Complex case, multiple hulls.
	else
	{
		// Multiple hull clipping applies each hull to
		// the original shape, and appends each clipped result
		// as a new hull.
		auto& vScratch = m_vScratchVectors2D;

		// Copy the shape into the scratch.
		vScratch.Assign(m_vVertices.Begin() + uBegin, m_vVertices.End());

		// Remove the shape from the main buffer.
		m_vVertices.Resize(uBegin);

		// Now clip the scratch and, possibly, append each as a new shape.
		UInt32 uFirstClipPlane = e.m_uFirstVertex;
		for (UInt32 uHull = 0u; uHull < e.m_uHulls; ++uHull)
		{
			// Prepare the output scratch for clipped result.
			UInt32 const uClipPlanes = m_vHulls[e.m_uFirstHull + uHull];
			UInt32 const uWorstCaseSize = (vScratch.GetSize() + uClipPlanes);

			UInt32 const uBaseSize = m_vVertices.GetSize();
			m_vVertices.Resize(uBaseSize + uWorstCaseSize);

			// Perform the clipped.
			Int32 const iResult = Clipper::ConvexClip(
				m_vPlanes.Get(uFirstClipPlane),
				uClipPlanes,
				vScratch.Data(),
				vScratch.GetSize(),
				m_vVertices.Get(uBaseSize),
				fTolerance);

			// Negative indicates all vertices were clipped
			// by this hull.
			if (iResult < 0)
			{
				m_vVertices.Resize(uBaseSize);
			}
			// Positive indicates a partial clip.
			else if (iResult > 0)
			{
				// Sanity check that our worst case estimate was correct.
				SEOUL_ASSERT(uBaseSize + (UInt32)iResult <= m_vVertices.GetSize());

				// Partial clip - add a new hull with the partial size.
				m_vVertices.Resize(uBaseSize + (UInt32)iResult);

				auto& e = m_Pending;
				e.m_uHulls++;
				e.m_uVertices += (UInt32)iResult;
				m_vHulls.PushBack((UInt32)iResult);
			}
			else
			{
				// Sanity check, assumed below.
				SEOUL_ASSERT(uCount == vScratch.GetSize());

				// The entire hull was left unclipped, so just add it unmodified.
				m_vVertices.Resize(uBaseSize);
				m_vVertices.Append(vScratch);

				auto& e = m_Pending;
				e.m_uHulls++;
				e.m_uVertices += uCount;
				m_vHulls.PushBack(uCount);

				// If a single hull did not clip any vertices,
				// then we can return immediately, as the original
				// mesh will remain unclipped, since we assume all
				// hulls do not overlap.
				return;
			}

			// Advance.
			uFirstClipPlane += uClipPlanes;
		}
	}
}

} // namespace Falcon

} // namespace Seoul
