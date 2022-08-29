/**
 * \file PhysicsBodyType.h
 * \brief High-level type enumeration of a physics body
 * (e.g. dynamic vs. static).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef PHYSICS_BODY_TYPE_H
#define PHYSICS_BODY_TYPE_H

#include "Prereqs.h"

#if SEOUL_WITH_PHYSICS

namespace Seoul::Physics
{

enum class BodyType
{
	/** Body is entirely immobile and has infinite mass. */
	kStatic,

	/**
	 * Body has infinite mass but is expected to be mobile -
	 * this is the body type to use for character controllers
	 * or other entities that will be code or player driven,
	 * should affect physical things, but should not obey
	 * the normal laws of physics.
	 */
	kKinematic,

	/**
	 * Fully simulated and dynamic body. Normal, physically
	 * based mass and dynamics characteristics.
	 */
	kDynamic,
};

} // namespace Seoul::Physics

#endif // /#if SEOUL_WITH_PHYSICS

#endif // include guard
