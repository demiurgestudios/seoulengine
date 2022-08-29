/**
 * \file NavigationTest.h
 * \brief Navigation unit test .
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef NAVIGATION_TEST_H
#define NAVIGATION_TEST_H

#include "Prereqs.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

#if SEOUL_WITH_NAVIGATION

/**
 * Text  class for Memory Manager.
 */
class NavigationTest SEOUL_SEALED
{
public:
	void TestConnectedLargeData();
	void TestConnectedRandom();
	void TestCoverageRasterizerBasic();
	void TestFindNearestBasic();
	void TestFindNearestLarge();
	void TestFindNearestLargeData();
	void TestFindNearestConnectedBasic();
	void TestFindNearestConnectedLarge();
	void TestFindNearestConnectedLargeData();
	void TestFindPathBasic();
	void TestFindStraightPathBasic();
	void TestGridBasic();
	void TestRayTestBasic();
	void TestRobustFindStraightPathBasic();
}; // class NavigationTest

#endif // /#if SEOUL_WITH_NAVIGATION

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
