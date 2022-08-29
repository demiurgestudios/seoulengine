/**
 * \file FrustumTest.h
 * \brief Unit test header file for the Frustum struct.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FRUSTUM_TEST_H
#define FRUSTUM_TEST_H

#include "Prereqs.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class FrustumTest SEOUL_SEALED
{
public:
	void TestBasic();
	void TestIntersectsAABB();
	void TestIntersectsPoint();
	void TestIntersectsSphere();
	void TestMiscMethods();
};

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
