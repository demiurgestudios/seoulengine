/**
 * \file Frustum.h
 * \brief A Frustum is a 6-sided, convex bounding region defined by planes.
 * A Frustum is typically used to define the region formed by a camera
 * projection matrix but can also be used to define regions such as spot
 * light shadow regions.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FRUSTUM_H
#define FRUSTUM_H

#include "FixedArray.h"
#include "Geometry.h"
#include "Matrix4D.h"
#include "SeoulTypeTraits.h"
#include "Vector.h"

namespace Seoul
{

/**
 * Defines the result of an intersection test between a shape
 * and a frustum.
 */
enum class FrustumTestResult
{
	/** Shape is completely inside the Frustum (dot coordinate of all planes is positive). */
	kContains,

	/** Shape intersects one or more frustum planes (dot coordinates are negative and positive). */
	kIntersects,

	/** Shape is completely outside the Frustum (dot coordinate of all planes is negative). */
	kDisjoint,
};

/**
 * Frustum, a convex bounding volume defined by 6 planes, 4 lateral
 * planes and 2 capping planes.
 */
struct Frustum SEOUL_SEALED
{
	/** Plane indices in the plane array. */
	enum
	{
		kNear = 0,
		kFar,
		kLeft,
		kRight,
		kTop,
		kBottom,
		PLANE_COUNT,
	};

	// Explicit named Plane access.
	const Plane& GetBottomPlane() const { return m_aPlanes[kBottom]; }
	const Plane& GetFarPlane() const    { return m_aPlanes[kFar]; }
	const Plane& GetLeftPlane() const   { return m_aPlanes[kLeft]; }
	const Plane& GetNearPlane() const   { return m_aPlanes[kNear]; }
	const Plane& GetRightPlane() const  { return m_aPlanes[kRight]; }
	const Plane& GetTopPlane() const    { return m_aPlanes[kTop]; }

	// Plane by index - 0-6 planes.
	const Plane& GetPlane(Int ix) const
	{
		SEOUL_ASSERT(ix >= 0 && ix < PLANE_COUNT);
		return m_aPlanes[ix];
	}

	// Frustum descriptors.
	AABB GetAABB() const;
	void GetCornerVertices(Vector3D aCorners[8]) const;

	// Intersection with Frustum tests.
	FrustumTestResult Intersects(const AABB& aabb) const;
	FrustumTestResult Intersects(const Sphere& sphere) const;
	FrustumTestResult Intersects(const Vector3D& point) const;

	// Update the Frustum from a view * projection matrix.
	void Set(const Matrix4D& ProjectionMatrix, const Matrix4D& ViewMatrix);

	/**
	 * Constructs a new Frustum from 6 defining planes
	 */
	static Frustum CreateFromPlanes(
		const Plane& rNearPlane,
		const Plane& rFarPlane,
		const Plane& rLeftPlane,
		const Plane& rRightPlane,
		const Plane& rTopPlane,
		const Plane& rBottomPlane)
	{
		Frustum ret;
		ret.m_aPlanes[kNear] = rNearPlane;
		ret.m_aPlanes[kFar] = rFarPlane;
		ret.m_aPlanes[kLeft] = rLeftPlane;
		ret.m_aPlanes[kRight] = rRightPlane;
		ret.m_aPlanes[kTop] = rTopPlane;
		ret.m_aPlanes[kBottom] = rBottomPlane;
		return ret;
	}

	/**
	 * Constructs a new Frustum from view and projection matrices.
	 */
	static Frustum CreateFromViewProjection(
		const Matrix4D& mProjectionMatrix,
		const Matrix4D& mViewMatrix)
	{
		Frustum ret;
		ret.Set(mProjectionMatrix, mViewMatrix);
		return ret;
	}

private:
	typedef FixedArray<Plane, 6> Planes;
	Planes m_aPlanes;
}; // struct Frustum
template <> struct CanMemCpy<Frustum> { static const Bool Value = true; };
template <> struct CanZeroInit<Frustum> { static const Bool Value = true; };

// Verifying that the Frustum struct's size is what we expect.
SEOUL_STATIC_ASSERT(sizeof(Frustum) == sizeof(Plane) * 6u);

} // namespace Seoul

#endif // include guard
