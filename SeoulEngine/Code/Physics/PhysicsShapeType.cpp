/**
 * \file PhysicsShapeType.cpp
 * \brief Enumeration of the high level shape types. Shape
 * define body collision.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "PhysicsShapeType.h"
#include "ReflectionDefine.h"

#if SEOUL_WITH_PHYSICS

namespace Seoul
{

SEOUL_BEGIN_ENUM(Physics::ShapeType)
	SEOUL_ENUM_N("None", Physics::ShapeType::kNone)
	SEOUL_ENUM_N("Box", Physics::ShapeType::kBox)
	SEOUL_ENUM_N("Capsule", Physics::ShapeType::kCapsule)
	SEOUL_ENUM_N("ConvexHull", Physics::ShapeType::kConvexHull)
	SEOUL_ENUM_N("Sphere", Physics::ShapeType::kSphere)
SEOUL_END_ENUM()

} // namespace Seoul

#endif // /#if SEOUL_WITH_PHYSICS
