/**
 * \file PhysicsBody.cpp
 * \brief Represents a physical thing in a physics world. Defines
 * dynamics and references a collision shape.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "PhysicsBody.h"
#include "PhysicsUtil.h"

#if SEOUL_WITH_PHYSICS

#include <bounce/dynamics/body.h>

namespace Seoul::Physics
{

static inline BodyType Convert(b3BodyType eType)
{
	switch (eType)
	{
	case e_staticBody: return BodyType::kStatic;
	case e_kinematicBody: return BodyType::kKinematic;
	case e_dynamicBody: return BodyType::kDynamic;
	default:
		SEOUL_FAIL("Out-of-sync enum.");
		return BodyType::kStatic;
	};
}

Body::~Body()
{
	// Sanity check - simulator must have cleared out
	// pimpl prior to destroy.
	SEOUL_ASSERT(!m_pImpl.IsValid());
}

Vector3D Body::GetAngularVelocity() const
{
	return Convert(m_pImpl->GetAngularVelocity());
}

Vector3D Body::GetLinearVelocity() const
{
	return Convert(m_pImpl->GetLinearVelocity());
}

Quaternion Body::GetOrientation() const
{
	return Convert(m_pImpl->GetOrientation());
}

Vector3D Body::GetPosition() const
{
	return Convert(m_pImpl->GetPosition());
}

BodyType Body::GetType() const
{
	return Convert(m_pImpl->GetType());
}

Bool Body::IsSleeping() const
{
	return !m_pImpl->IsAwake();
}

void Body::SetTransform(const Vector3D& position, const Quaternion& orientation, Bool bWake)
{
	m_pImpl->SetTransform(Convert(position), Convert(orientation));
	if (bWake && !m_pImpl->IsAwake())
	{
		m_pImpl->SetAwake(true);
	}
}

Body::Body(CheckedPtr<b3Body> pImpl)
	: m_pImpl(pImpl)
{
}

} // namespace Seoul::Physics

#endif // /#if SEOUL_WITH_PHYSICS
