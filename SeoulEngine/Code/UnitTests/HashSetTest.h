/**
 * \file HashSetTest.h
 * \brief Unit test header file for HashSet class
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef HASH_SET_TEST_H
#define HASH_SET_TEST_H

#include "Prereqs.h"
#include "SeoulTypes.h"
#include "HashSet.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

/**
 * Text fixture class for HashSet
 */
class HashSetTest SEOUL_SEALED
{
public:
	void TestInstantiation();
	void TestInstantiationFromVector();
	void TestClear();
	void TestClusteringPrevention();
	void TestAssignment();
	void TestInsert();
	void TestSwap();
	void TestIntKeys();
	void TestHashableKeys();
	void TestNullKey();
	void TestFindNull();
	void TestSeoulStringKeys();
	void TestPointerKeys();
	void TestErase();
	void TestIterators();
	void TestRangedFor();
	void TestUtilities();
	void TestEquality();
	void TestContains();
	void TestDisjoint();
};

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
