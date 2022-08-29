/**
 * \file HashTableTest.h
 * \brief Unit test header file for HashTable class
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef HASH_TABLE_TEST_H
#define HASH_TABLE_TEST_H

#include "Prereqs.h"
#include "SeoulTypes.h"
#include "HashTable.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

/**
 * Text fixture class for HashTable
 */
class HashTableTest SEOUL_SEALED
{
public:
	void TestInstantiation();
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
};

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
