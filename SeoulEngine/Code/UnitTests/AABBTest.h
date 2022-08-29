/**
 * \file AABBTest.h
 * \brief Unit test header file for the Seoul AABB struct
 *
 * This file contains the unit test declarations for the Seoul AABB struct
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef AABB_TEST_H
#define AABB_TEST_H

#include "Prereqs.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class AABBTest SEOUL_SEALED
{
public:
	void TestBasic();
	void TestIntersect();
	void TestTransform();
	void TestUtilities();
}; // class AABBTest

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif  // include guard
