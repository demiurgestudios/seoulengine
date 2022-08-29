/**
 * \file PhysicsBodyDef.h
 * \brief The definition of a body. Bodies are instanced from a def,
 * so a single def can be shared between multiple bodies.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef PHYSICS_BODY_DEF_H
#define PHYSICS_BODY_DEF_H

#include "PhysicsBodyType.h"
#include "PhysicsShapeDef.h"
#include "Prereqs.h"
#include "Quaternion.h"
#include "Vector3D.h"

#if SEOUL_WITH_PHYSICS

namespace Seoul::Physics
{

struct BodyDef SEOUL_SEALED
{
	BodyDef()
		: m_eType(BodyType::kStatic)
		, m_qOrientation(Quaternion::Identity())
		, m_vPosition(Vector3D::Zero())
		, m_Shape()
	{
	}

	BodyType m_eType;
	Quaternion m_qOrientation;
	Vector3D m_vPosition;
	ShapeDef m_Shape;
}; // struct BodyDef

} // namespace Seoul::Physics

#endif // /#if SEOUL_WITH_PHYSICS

#endif // include guard
