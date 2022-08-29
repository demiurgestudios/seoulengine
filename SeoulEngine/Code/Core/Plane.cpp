/**
 * \file Plane.cpp
 * \brief Geometric primitive presents an infinite 3D plane, often used
 * for splitting 3D space in half-spaces.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "AABB.h"
#include "Plane.h"
#include "Sphere.h"

namespace Seoul
{

/**
 * Returns PlaneTestResult::kBack if aabb is completely behind this
 * Plane, PlaneTestResult::kFront if aabb is completely in front of this
 * Plane, and PlaneTestResult::kIntersects otherwise.
 *
 * The front side of a plane is defined by the direction of the
 * Plane's normal vector.
 */
PlaneTestResult Plane::Intersects(const AABB& aabb) const
{
	// Apply epsilon to radius as a margin.
	const Float radius = aabb.GetEffectiveRadius(GetNormal()) - fEpsilon;
	const Float negRadius = -radius;

	Float d = DotCoordinate(aabb.GetCenter());

	if (d < negRadius) { return PlaneTestResult::kBack; }
	else if (d <= radius) { return PlaneTestResult::kIntersects; }
	else { return PlaneTestResult::kFront; }
}

/**
 * Returns PlaneTestResult::kBack if sphere is completely behind this
 * Plane, PlaneTestResult::kFront if sphere is completely in front of this
 * Plane, and PlaneTestResult::kIntersects otherwise.
 *
 * The front side of a plane is defined by the direction of the
 * Plane's normal vector.
 */
PlaneTestResult Plane::Intersects(const Sphere& sphere) const
{
	// Apply epsilon to radius as a margin.
	const Float radius = sphere.m_fRadius - fEpsilon;
	const Float negRadius = -radius;

	Float d = DotCoordinate(sphere.m_vCenter);

	if (d < negRadius) { return PlaneTestResult::kBack; }
	else if (d <= radius) { return PlaneTestResult::kIntersects; }
	else { return PlaneTestResult::kFront; }
}

/**
 * Returns PlaneTestResult::kBack if vPoint is completely behind this
 * Plane, PlaneTestResult::kFront if vPoint is completely in front of this
 * Plane, and PlaneTestResult::kIntersects otherwise.
 *
 * The front side of a plane is defined by the direction of the
 * Plane's normal vector.
 */
PlaneTestResult Plane::Intersects(const Vector3D& vPoint) const
{
	Float d = DotCoordinate(vPoint);

	if (d < -fEpsilon) { return PlaneTestResult::kBack; }
	else if (d > fEpsilon) { return PlaneTestResult::kFront; }
	else { return PlaneTestResult::kIntersects; }
}

} // namespace Seoul
