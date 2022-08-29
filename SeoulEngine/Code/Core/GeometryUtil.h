/**
 * \file GeometryUtil.h
 * \brief Collection of utility functions on combined geometric types.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef GEOMETRY_UTIL_H
#define GEOMETRY_UTIL_H

#include "SeoulTypes.h"

namespace Seoul
{

struct Vector3D;
struct Plane;
struct Frustum;

// Calculates the intersection point of 3 planes
Bool GetIntersection(Vector3D & v3Intersection, const Plane & p1, const Plane & p2, const Plane & p3);

// Calculates the intersection point of a plane and a ray
Bool GetIntersection(Vector3D & v3Intersection, const Plane & plane, const Vector3D & vOrigin, const Vector3D & vDirection);

// Calculates the intersection point of a frustum and a ray
Bool GetIntersection(Vector3D & v3Intersection, const Frustum & frustum, const Vector3D & vOrigin, const Vector3D & vDirection);

} // namespace Seoul

#endif // include guard
