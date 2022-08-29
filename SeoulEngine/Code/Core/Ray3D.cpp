/**
* \file Ray3D.cpp
* \brief Geometric primitive presents an infinite ray in 3D
* sapce.
*
* Copyright (c) Demiurge Studios, Inc.
* 
* This source code is licensed under the MIT license.
* Full license details can be found in the LICENSE file
* in the root directory of this source tree.
*/

#include "AABB.h"
#include "Plane.h"
#include "Ray3D.h"

namespace Seoul
{

/**
 * Cast the ray and check for intersection with plane.
 * If a hit occurs, rfDistance is the displacement along
 * the ray to the hit point.
 *
 * @return True on intersection, false otherwise. On false,
 * rfDistance is left unmodified, otherwise it contains the
 * distance along the ray to the point of intersection.
 *
 * Treats the ray as a mathematical ray in one direction
 * (false is returned for hits "behind" the ray).
 */
Bool Ray3D::Intersects(const Plane& plane, Float& rfDistance) const
{
	// Perform a line intersection.
	Float f = 0.0f;
	if (!LineIntersects(plane, f))
	{
		return false;
	}

	// Require intersections at or above 0 ("in front of" the ray).
	if (f < 0.0f)
	{
		return false;
	}

	// Successful hit.
	rfDistance = f;
	return true;
}

/**
 * \sa From: Ericson, C. 2005. "Real-Time Collision Detection",
 *     Elsevier, Inc. ISBN: 1-55860-732-3, page 178
 */
Bool Ray3D::Intersects(const Sphere& sphere, Float& rfDistance) const
{
	Vector3D const v = (m_vPosition - sphere.m_vCenter);
	Float const fB = Vector3D::Dot(v, m_vDirection);
	Float const fC = v.LengthSquared() - (sphere.RadiusSquared());
	if (fC > 0.0f && fB > 0.0f)
	{
		return false;
	}

	Float const fDiscr = (fB * fB) - fC;
	if (fDiscr < 0.0f)
	{
		return false;
	}

	rfDistance = Max((-fB - Sqrt(fDiscr)), 0.0f);
	return true;
}

/**
 * Cast the ray and check for intersection with plane.
 * If a hit occurs, rfDistance is the displacement along
 * the ray to the hit point.
 *
 * @return True on intersection, false otherwise. On false,
 * rfDistance is left unmodified, otherwise it contains the
 * distance along the ray to the point of intersection.
 *
 * Treats the ray as an infinite line, which will
 * return true for intersections "behind" the ray (rfDistance
 * can be negative values).
 */
Bool Ray3D::LineIntersects(const Plane& plane, Float& rfDistance) const
{
	Float const fDotNormal = Vector3D::Dot(plane.GetNormal(), m_vDirection);

	// Failure if plane is perpendicular to the ray.
	if (IsZero(fDotNormal, 1e-4f))
	{
		return false;
	}

	// Distance is dot coordinate over dot normal, negated.
	rfDistance = -plane.DotCoordinate(m_vPosition) / fDotNormal;
	return true;
}

} // namespace Seoul
