/**
 * \file PlaneTest.h
 * \brief Unit test header file for the Plane struct.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef PLANE_TEST_H
#define PLANE_TEST_H

#include "Prereqs.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class PlaneTest SEOUL_SEALED
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
