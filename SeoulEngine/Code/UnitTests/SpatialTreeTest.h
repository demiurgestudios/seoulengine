/**
 * \file SpatialTreeTest.h
 * \brief Unit test header file for the Seoul SpatialTree class
 * This file contains the unit test declarations for the Seoul SpatialTree class
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SPATIAL_TREE_TEST_H
#define SPATIAL_TREE_TEST_H

#include "Prereqs.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class SpatialTreeTest SEOUL_SEALED
{
public:
	void TestDefaultState();
	void TestAddRemoveUpdateEmptyTree();
	void TestBuildAndQuery();
}; // class SpatialTreeTest

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif  // include guard
