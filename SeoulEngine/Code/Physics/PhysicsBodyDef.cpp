/**
 * \file PhysicsBodyDef.cpp
 * \brief The definition of a body. Bodies are instanced from a def,
 * so a single def can be shared between multiple bodies.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "PhysicsBodyDef.h"
#include "ReflectionDefine.h"

#if SEOUL_WITH_PHYSICS

namespace Seoul
{

SEOUL_BEGIN_TYPE(Physics::BodyDef)
	SEOUL_PROPERTY_N("Type", m_eType)
		SEOUL_DEV_ONLY_ATTRIBUTE(Description, "Simulation type applied to the body.")
	SEOUL_PROPERTY_N("Orientation", m_qOrientation)
		SEOUL_DEV_ONLY_ATTRIBUTE(Description, "Initial rotation of the body.")
	SEOUL_PROPERTY_N("Position", m_vPosition)
		SEOUL_DEV_ONLY_ATTRIBUTE(Description, "Initial position of the body.")
	SEOUL_PROPERTY_N("Shape", m_Shape)
SEOUL_END_TYPE()

} // namespace Seoul

#endif // /#if SEOUL_WITH_PHYSICS
