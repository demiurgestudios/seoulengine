/**
 * \file GeometryUtil.cpp
 * \brief Collection of utility functions on combined geometric types.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "GeometryUtil.h"

#include "Geometry.h"
#include "Vector3D.h"
#include "Matrix4D.h"
#include "Frustum.h"
#include "SeoulMath.h"

namespace Seoul
{

/**
 * Calculates the intersection point of 3 planes.
 *
 *  Calculates the intersection point of 3 planes. Returns true if an intersection
 *  was found, otherwise false (when two or more of the planes are parallel).
 *
 * @param[out] v3Intersection	The point of intersection
 * @param[in]  p1				The first plane
 * @param[in]  p2				The second plane
 * @param[in]  p3				The third plane
 *
 * @return True if an intersection was found, otherwise false.
 */
Bool GetIntersection(Vector3D & v3Intersection, const Plane & p1, const Plane & p2, const Plane & p3)
{
	Matrix4D m(p1.A, p1.B, p1.C, p1.D,
			   p2.A, p2.B, p2.C, p2.D,
			   p3.A, p3.B, p3.C, p3.D,
			   0.f,  0.f,  0.f,  1.f);

	m = m.Inverse();

	if (m.Equals(Matrix4D::Zero(), fEpsilon))
	{
		v3Intersection = Vector3D::Zero();
		return false;
	}
	else
	{
		v3Intersection.X = m.M03;
		v3Intersection.Y = m.M13;
		v3Intersection.Z = m.M23;
		return true;
	}
}

/**
 * Calculates the intersection point of a plane and a ray.
 *
 *  Calculates the intersection point of a plane and a ray. Returns true if an intersection
 *  was found, otherwise false (they might be parallel).
 *
 * @param[out] v3Intersection	The point of intersection
 * @param[in]  plane				The plane
 * @param[in]  vOrigin			The origin of the ray
 * @param[in]  vDirection		The normalized direction of the ray
 *
 * @return True if an intersection was found, otherwise false.
 */
Bool GetIntersection(Vector3D & v3Intersection, const Plane & plane, const Vector3D & vOrigin, const Vector3D & vDirection)
{
	v3Intersection = Vector3D::Zero();

	// Check if they are parallel
	Float fDot = Vector3D::Dot(plane.GetNormal(), vDirection);
	if (IsZero(fDot))
	{
		return false;
	}

	Float t = -(Vector3D::Dot(plane.GetNormal(), vOrigin) + plane.D) / fDot;

	// Check if the intersection is in the wrong direction
	if (t < 0.f)
	{
		return false;
	}

	v3Intersection = vOrigin + vDirection * t;
	return true;
}

/**
 * Calculates the intersection point of a frustum and a ray.
 *
 * This is unambiguous if the origin is inside the frustum.
 * If it is outside, one of the intersection points will be arbitrarily returned.
 *
 * @param[out] v3Intersection	The point of intersection
 * @param[in]  frustum			The frustum
 * @param[in]  vOrigin			The origin of the ray
 * @param[in]  vDirection		The normalized direction of the ray
 */
Bool GetIntersection(Vector3D & v3Intersection, const Frustum & frustum, const Vector3D & vOrigin, const Vector3D & vDirection)
{
	// (copied from Frustum::Intersects(Point))
	//
	// For future development, Frusta with # of planes != 6
	// can be useful for Portal-Cell culling and other spatial queries.
	static const Int count = 6;
	for (Int i = 0; i < count; ++i)
	{
		Bool bIntersect = GetIntersection(v3Intersection, frustum.GetPlane(i), vOrigin, vDirection);
		if (!bIntersect) continue;

		// Now we know it intersects plane i -- make sure the intersect point
		// is inside the other 5 planes
		Bool bInsideAll = true;
		for (Int j = 0; j < count; ++j)
		{
			if (i == j) continue;

			const Float d = frustum.GetPlane(j).DotCoordinate(v3Intersection);

			if (d < -fEpsilon)
			{
				bInsideAll = false;
				break;
			}
		}

		// If it's inside the other 5 planes we can stop looking
		if (bInsideAll)
		{
			return true;
		}
	}

	return false;
}

} // namespace Seoul
