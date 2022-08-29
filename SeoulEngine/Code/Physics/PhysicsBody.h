/**
 * \file PhysicsBody.h
 * \brief Represents a physical thing in a physics world. Defines
 * dynamics and references a collision shape.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef PHYSICS_BODY_H
#define PHYSICS_BODY_H

#include "CheckedPtr.h"
#include "PhysicsBodyType.h"
#include "Prereqs.h"
#include "Quaternion.h"
#include "SharedPtr.h"
#include "Vector3D.h"
class b3Body;

#if SEOUL_WITH_PHYSICS

namespace Seoul::Physics
{

class Body SEOUL_SEALED
{
public:
	~Body();

	Vector3D GetAngularVelocity() const;
	Vector3D GetLinearVelocity() const;
	Quaternion GetOrientation() const;
	Vector3D GetPosition() const;
	BodyType GetType() const;
	Bool IsSleeping() const;
	void SetTransform(const Vector3D& position, const Quaternion& orientation, Bool bWake);

private:
	SEOUL_REFERENCE_COUNTED(Body);

	friend class Simulator;

	Body(CheckedPtr<b3Body> pImpl);

	CheckedPtr<b3Body> m_pImpl;

	SEOUL_DISABLE_COPY(Body);
}; // class Body

} // namespace Seoul::Physics

#endif // /#if SEOUL_WITH_PHYSICS

#endif // include guard
