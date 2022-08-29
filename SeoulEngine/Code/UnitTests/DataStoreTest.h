/**
 * \file DataStoreTest.h
 * \brief Unit test header file for DataStore class
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef DATA_STORE_TEST_H
#define DATA_STORE_TEST_H

#include "Prereqs.h"
#include "DataStore.h"
#include "SeoulTypes.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

/**
 * Text fixture class for HashTable
 */
class DataStoreTest SEOUL_SEALED
{
public:
	void TestDiffAdditive();
	void TestDiffArray();
	void TestDiffArray2();
	void TestDiffNullDiff();
	void TestDiffNullTable();
	void TestDiffSelf();
	void TestDiffSubtractive();
	void TestDiffSubtractiveToString();
	void TestDiffTableMultiple();
	void TestEqualsNaN();
	void TestMD5();
	void TestMD5Inf();
	void TestMD5NaN();
	void TestMD5Zero();
	void TestInstantiation();
	void TestBasicRobustness();
	void TestDeepCopy();
	void TestDeepCopyTable();
	void TestGarbageCollection();
	void TestRootArray();
	void TestRootTable();
	void TestArrayErase();
	void TestTableErase();
	void TestTableNullKey();
	void TestNumbersInArray();
	void TestNumbersInTable();
	void TestStress();
	void TestToString();
	void TestStringAlloc();
	void TestFloat();
	void TestVerifyIntegrity();
	void TestOldData();
	void TestMoveNodeBetweenTables();
	void TestNullAsSpecialErase();
	void TestEraseAgainstNoExist();
	void TestEraseAgainstNoExistFromNull();
	void TestEraseAgainstNoExistRegression();
	void TestEraseAgainstNoExistFromNullRegression();

	void TestLargeSerializedStringTableRegression();
	void TestVersion1Load();

	void TestDataStorePrinter();
	void TestDataStorePrinterOnModifiedFile();
	void TestDataStorePrinterOnResolvedCommandsFile();

	void TestDataStoreCompactHandlesEmpty();
	void TestDataStoreCompactHandlesLargeData();

	void TestDataStoreCompactHandlesRegression();

	void TestDataStoreBinaryDeterminismRegression();
};

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
