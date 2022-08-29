/**
 * \file PhysicsTest.h
 * \brief Navigation unit test .
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef PHYSICS_TEST_H
#define PHYSICS_TEST_H

#include "Prereqs.h"

#if SEOUL_WITH_PHYSICS

namespace Seoul
{

#if SEOUL_UNIT_TESTS

/**
 * Text  class for Physics.
 */
class PhysicsTest SEOUL_SEALED
{
public:
	void TestDynamicsSingleBoxStack();
	void TestDynamicsSmallBoxStack();
	void TestDynamicsLargeBoxStack();
}; // class PhysicsTest

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // /#if SEOUL_WITH_PHYSICS

#endif // include guard
