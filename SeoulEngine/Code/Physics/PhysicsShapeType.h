/**
 * \file PhysicsShapeType.h
 * \brief Enumeration of the high level shape types. Shape
 * define body collision.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef PHYSICS_SHAPE_TYPE_H
#define PHYSICS_SHAPE_TYPE_H

#include "Prereqs.h"

#if SEOUL_WITH_PHYSICS

namespace Seoul::Physics
{

enum class ShapeType
{
	/* Placeholder for no shape. */
	kNone,

	/* An AABB (in the local space of the body) defined by an origin and extents. */
	kBox,

	/* A cylinder with two half-sphere end caps, defined by two local origin points and a radius. */
	kCapsule,

	/* A collection of points and faces that form an arbitrary 3D convex hull. */
	kConvexHull,

	/* A perfect sphere, defined by an origin and a radius. */
	kSphere,
};

} // namespace Seoul::Physics

#endif // /#if SEOUL_WITH_PHYSICS

#endif // include guard
