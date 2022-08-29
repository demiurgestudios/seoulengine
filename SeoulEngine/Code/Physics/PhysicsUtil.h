/**
 * \file PhysicsUtil.h
 * \brief Shared utilities for the Physics project. Internal header.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef PHYSICS_UTIL_H
#define PHYSICS_UTIL_H

#if SEOUL_WITH_PHYSICS

#include <bounce/common/math/quat.h>
#include <bounce/common/math/vec3.h>
#include "Quaternion.h"
#include "Vector3D.h"

namespace Seoul::Physics
{

static inline Quaternion Convert(const b3Quat& q)
{
	return Quaternion(q.x, q.y, q.z, q.w);
}

static inline Vector3D Convert(const b3Vec3& v)
{
	return Vector3D(v.x, v.y, v.z);
}

static inline b3Quat Convert(const Quaternion& q)
{
	return b3Quat(q.X, q.Y, q.Z, q.W);
}

static inline b3Vec3 Convert(const Vector3D& v)
{
	return b3Vec3(v.X, v.Y, v.Z);
}

} // namespace Seoul::Physics

#endif // /#if SEOUL_WITH_PHYSICS

#endif // include guard
