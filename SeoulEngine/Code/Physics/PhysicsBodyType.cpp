/**
 * \file PhysicsBodyType.cpp
 * \brief High-level type enumeration of a physics body
 * (e.g. dynamic vs. static).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "PhysicsBodyType.h"
#include "ReflectionDefine.h"

#if SEOUL_WITH_PHYSICS

namespace Seoul
{

SEOUL_BEGIN_ENUM(Physics::BodyType)
	SEOUL_ENUM_N("Dynamic", Physics::BodyType::kDynamic)
	SEOUL_ENUM_N("Kinematic", Physics::BodyType::kKinematic)
	SEOUL_ENUM_N("Static", Physics::BodyType::kStatic)
SEOUL_END_ENUM()

} // namespace Seoul

#endif // /#if SEOUL_WITH_PHYSICS
