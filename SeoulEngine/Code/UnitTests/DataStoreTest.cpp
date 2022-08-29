/**
 * \file DataStoreTest.cpp
 * \brief Unit test header file for DataStore class
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Compress.h"
#include "DataStoreParser.h"
#include "DataStoreTest.h"
#include "EncryptAES.h"
#include "FileManager.h"
#include "HashFunctions.h"
#include "Logger.h"
#include "Path.h"
#include "ReflectionDefine.h"
#include "SaveLoadManager.h"
#include "SaveLoadUtil.h"
#include "SeoulAssert.h"
#include "SeoulFile.h"
#include "SeoulString.h"
#include "UnitTesting.h"
#include "UnitTestsFileManagerHelper.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

static inline String UnitTestingToString(const DataStore::TableIterator& i)
{
	return String::Printf("(%p, %p)", std::addressof(i->First), std::addressof(i->Second));
}

SEOUL_BEGIN_TYPE(DataStoreTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestDiffAdditive)
	SEOUL_METHOD(TestDiffArray)
	SEOUL_METHOD(TestDiffArray2)
	SEOUL_METHOD(TestDiffNullDiff)
	SEOUL_METHOD(TestDiffNullTable)
	SEOUL_METHOD(TestDiffSelf)
	SEOUL_METHOD(TestDiffSubtractive)
	SEOUL_METHOD(TestDiffSubtractiveToString)
	SEOUL_METHOD(TestDiffTableMultiple)
	SEOUL_METHOD(TestEqualsNaN)
	SEOUL_METHOD(TestMD5)
	SEOUL_METHOD(TestMD5Inf)
	SEOUL_METHOD(TestMD5NaN)
	SEOUL_METHOD(TestMD5Zero)
	SEOUL_METHOD(TestInstantiation)
	SEOUL_METHOD(TestBasicRobustness)
	SEOUL_METHOD(TestDeepCopy)
	SEOUL_METHOD(TestDeepCopyTable)
	SEOUL_METHOD(TestGarbageCollection)
	SEOUL_METHOD(TestRootArray)
	SEOUL_METHOD(TestRootTable)
	SEOUL_METHOD(TestArrayErase)
	SEOUL_METHOD(TestTableErase)
	SEOUL_METHOD(TestTableNullKey)
	SEOUL_METHOD(TestNumbersInArray)
	SEOUL_METHOD(TestNumbersInTable)
	SEOUL_METHOD(TestStress)
	SEOUL_METHOD(TestToString)
	SEOUL_METHOD(TestStringAlloc)
	SEOUL_METHOD(TestFloat)
	SEOUL_METHOD(TestVerifyIntegrity)
	SEOUL_METHOD(TestOldData)
	SEOUL_METHOD(TestMoveNodeBetweenTables)
	SEOUL_METHOD(TestNullAsSpecialErase)
	SEOUL_METHOD(TestEraseAgainstNoExist)
	SEOUL_METHOD(TestEraseAgainstNoExistFromNull)
	SEOUL_METHOD(TestEraseAgainstNoExistRegression)
	SEOUL_METHOD(TestEraseAgainstNoExistFromNullRegression)
	SEOUL_METHOD(TestLargeSerializedStringTableRegression)
	SEOUL_METHOD(TestVersion1Load)
	SEOUL_METHOD(TestDataStorePrinter)
	SEOUL_METHOD(TestDataStorePrinterOnModifiedFile)
	SEOUL_METHOD(TestDataStorePrinterOnResolvedCommandsFile)
	SEOUL_METHOD(TestDataStoreCompactHandlesEmpty)
	SEOUL_METHOD(TestDataStoreCompactHandlesLargeData)
	SEOUL_METHOD(TestDataStoreCompactHandlesRegression)
	SEOUL_METHOD(TestDataStoreBinaryDeterminismRegression)
SEOUL_END_TYPE()

/**
* Utility - Float31 in DataStore loses 1 bit of precision
* (the lowest bit of the mantissa is always set to 0). This
* function replicates that behaviro for the sake of equality
* comparisons in tests.
*/
static inline Float32 GetMaskedFloat31(Float32 f)
{
	union
	{
		Float32 fInOutValue;
		UInt32 uMixValue;
	};
	fInOutValue = f;
	uMixValue &= ~0x1;
	return fInOutValue;
}

static void TestMD5CheckValues(const DataStore& dataStore)
{
	DataNode value;
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(dataStore.GetRootNode(), HString("A"), value));
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, dataStore.AssumeInt32Small(value));
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(dataStore.GetRootNode(), HString("a"), value));
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, dataStore.AssumeInt32Small(value));
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(dataStore.GetRootNode(), HString("B"), value));
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, dataStore.AssumeInt32Small(value));
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(dataStore.GetRootNode(), HString("b"), value));
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, dataStore.AssumeInt32Small(value));
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(dataStore.GetRootNode(), HString("C"), value));
	SEOUL_UNITTESTING_ASSERT_EQUAL(4, dataStore.AssumeInt32Small(value));
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(dataStore.GetRootNode(), HString("c"), value));
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, dataStore.AssumeInt32Small(value));
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(dataStore.GetRootNode(), HString("D"), value));
	SEOUL_UNITTESTING_ASSERT_EQUAL(6, dataStore.AssumeInt32Small(value));
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(dataStore.GetRootNode(), HString("d"), value));
	SEOUL_UNITTESTING_ASSERT_EQUAL(7, dataStore.AssumeInt32Small(value));
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(dataStore.GetRootNode(), HString("E"), value));
	SEOUL_UNITTESTING_ASSERT_EQUAL(8, dataStore.AssumeInt32Small(value));
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(dataStore.GetRootNode(), HString("e"), value));
	SEOUL_UNITTESTING_ASSERT_EQUAL(9, dataStore.AssumeInt32Small(value));
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(dataStore.GetRootNode(), HString("F"), value));
	SEOUL_UNITTESTING_ASSERT_EQUAL(4.1f, dataStore.AssumeFloat32(value));
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(dataStore.GetRootNode(), HString("f"), value));
	SEOUL_UNITTESTING_ASSERT_EQUAL(1.5f, dataStore.AssumeFloat31(value));
}

void DataStoreTest::TestDiffAdditive()
{
	DataStore dataStoreA;
	dataStoreA.MakeTable();
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToTable(dataStoreA.GetRootNode(), HString("A"), 0));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToTable(dataStoreA.GetRootNode(), HString("a"), 1));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToTable(dataStoreA.GetRootNode(), HString("B"), 2));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToTable(dataStoreA.GetRootNode(), HString("b"), 3));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToTable(dataStoreA.GetRootNode(), HString("C"), 4));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToTable(dataStoreA.GetRootNode(), HString("c"), 5));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToTable(dataStoreA.GetRootNode(), HString("D"), 6));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToTable(dataStoreA.GetRootNode(), HString("d"), 7));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToTable(dataStoreA.GetRootNode(), HString("E"), 8));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToTable(dataStoreA.GetRootNode(), HString("e"), 9));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetFloat32ValueToTable(dataStoreA.GetRootNode(), HString("H"), 5.1f));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetFloat32ValueToTable(dataStoreA.GetRootNode(), HString("h"), 2.5f));

	DataStore dataStoreB;
	dataStoreB.MakeTable();
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetFloat32ValueToTable(dataStoreB.GetRootNode(), HString("h"), 2.5f));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetFloat32ValueToTable(dataStoreB.GetRootNode(), HString("H"), 5.1f));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToTable(dataStoreB.GetRootNode(), HString("e"), 9));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToTable(dataStoreB.GetRootNode(), HString("E"), 8));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToTable(dataStoreB.GetRootNode(), HString("d"), 7));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToTable(dataStoreB.GetRootNode(), HString("D"), 6));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToTable(dataStoreB.GetRootNode(), HString("c"), 5));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToTable(dataStoreB.GetRootNode(), HString("C"), 4));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToTable(dataStoreB.GetRootNode(), HString("b"), 3));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToTable(dataStoreB.GetRootNode(), HString("B"), 2));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToTable(dataStoreB.GetRootNode(), HString("a"), 1));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToTable(dataStoreB.GetRootNode(), HString("A"), 0));

	// Add new keys.
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToTable(dataStoreB.GetRootNode(), HString("F"), 10));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToTable(dataStoreB.GetRootNode(), HString("e"), 10));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetFloat32ValueToTable(dataStoreB.GetRootNode(), HString("G"), 4.1f));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetFloat32ValueToTable(dataStoreB.GetRootNode(), HString("g"), 1.5f));

	DataStore out;
	SEOUL_UNITTESTING_ASSERT(ComputeDiff(dataStoreA, dataStoreB, out));

	// Apply patch - this should make a copied version of A equal to B.
	{
		DataStore res;
		res.CopyFrom(dataStoreA);
		SEOUL_UNITTESTING_ASSERT(ApplyDiff(out, res));

		// Verify, a and b are not equal.
		SEOUL_UNITTESTING_ASSERT(!DataStore::Equals(
			dataStoreA, dataStoreA.GetRootNode(),
			dataStoreB, dataStoreB.GetRootNode()));

		// Verify that res is no longer equal to A.
		SEOUL_UNITTESTING_ASSERT(!DataStore::Equals(
			res,
			res.GetRootNode(),
			dataStoreA,
			dataStoreA.GetRootNode()));
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(res.ComputeMD5(), dataStoreA.ComputeMD5());

		SEOUL_UNITTESTING_ASSERT(DataStore::Equals(
			res,
			res.GetRootNode(),
			dataStoreB,
			dataStoreB.GetRootNode()));
		SEOUL_UNITTESTING_ASSERT_EQUAL(res.ComputeMD5(), dataStoreB.ComputeMD5());
	}
}

void DataStoreTest::TestDiffArray()
{
	// Two data stores, only different is an array value.
	DataStore dataStoreA;
	dataStoreA.MakeTable();
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetArrayToTable(dataStoreA.GetRootNode(), HString("A")));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToTable(dataStoreA.GetRootNode(), HString("B"), 2));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToTable(dataStoreA.GetRootNode(), HString("b"), 3));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToTable(dataStoreA.GetRootNode(), HString("C"), 4));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToTable(dataStoreA.GetRootNode(), HString("c"), 5));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToTable(dataStoreA.GetRootNode(), HString("D"), 6));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToTable(dataStoreA.GetRootNode(), HString("d"), 7));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToTable(dataStoreA.GetRootNode(), HString("E"), 8));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToTable(dataStoreA.GetRootNode(), HString("e"), 9));

	DataNode arr;
	SEOUL_UNITTESTING_ASSERT(dataStoreA.GetValueFromTable(dataStoreA.GetRootNode(), HString("A"), arr));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToArray(arr, 0u, 1));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToArray(arr, 0u, 2));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToArray(arr, 0u, 3));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetFloat32ValueToArray(arr, 1u, 4.1f));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetFloat32ValueToArray(arr, 2u, 1.5f));

	DataStore dataStoreB;
	dataStoreB.MakeTable();
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetArrayToTable(dataStoreB.GetRootNode(), HString("A")));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToTable(dataStoreB.GetRootNode(), HString("B"), 2));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToTable(dataStoreB.GetRootNode(), HString("b"), 3));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToTable(dataStoreB.GetRootNode(), HString("C"), 4));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToTable(dataStoreB.GetRootNode(), HString("c"), 5));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToTable(dataStoreB.GetRootNode(), HString("D"), 6));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToTable(dataStoreB.GetRootNode(), HString("d"), 7));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToTable(dataStoreB.GetRootNode(), HString("E"), 8));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToTable(dataStoreB.GetRootNode(), HString("e"), 9));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.GetValueFromTable(dataStoreB.GetRootNode(), HString("A"), arr));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetFloat32ValueToArray(arr, 4u, 2.5f));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetFloat32ValueToArray(arr, 3u, 5.1f));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetFloat32ValueToArray(arr, 2u, 1.5f));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetFloat32ValueToArray(arr, 1u, 4.1f));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToArray(arr, 0u, 3));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToArray(arr, 0u, 2));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToArray(arr, 0u, 1));

	DataStore out;
	SEOUL_UNITTESTING_ASSERT(ComputeDiff(dataStoreA, dataStoreB, out));

	// Apply patch - this should make a copied version of A equal to B.
	{
		DataStore res;
		res.CopyFrom(dataStoreA);
		SEOUL_UNITTESTING_ASSERT(ApplyDiff(out, res));

		// Verify, a and b are not equal.
		SEOUL_UNITTESTING_ASSERT(!DataStore::Equals(
			dataStoreA, dataStoreA.GetRootNode(),
			dataStoreB, dataStoreB.GetRootNode()));

		// Verify that res is no longer equal to A.
		SEOUL_UNITTESTING_ASSERT(!DataStore::Equals(
			res,
			res.GetRootNode(),
			dataStoreA,
			dataStoreA.GetRootNode()));
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(res.ComputeMD5(), dataStoreA.ComputeMD5());

		SEOUL_UNITTESTING_ASSERT(DataStore::Equals(
			res,
			res.GetRootNode(),
			dataStoreB,
			dataStoreB.GetRootNode()));
		SEOUL_UNITTESTING_ASSERT_EQUAL(res.ComputeMD5(), dataStoreB.ComputeMD5());
	}
}

void DataStoreTest::TestDiffArray2()
{
	// Two data stores, only different is an array value.
	DataStore dataStoreA;
	dataStoreA.MakeArray();

	DataNode arr = dataStoreA.GetRootNode();
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToArray(arr, 0u, 1));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToArray(arr, 0u, 2));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToArray(arr, 0u, 3));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetFloat32ValueToArray(arr, 1u, 4.1f));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetFloat32ValueToArray(arr, 2u, 1.5f));

	DataStore dataStoreB;
	dataStoreB.MakeArray();

	arr = dataStoreB.GetRootNode();
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetFloat32ValueToArray(arr, 4u, 2.5f));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetFloat32ValueToArray(arr, 3u, 5.1f));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetFloat32ValueToArray(arr, 2u, 1.5f));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetFloat32ValueToArray(arr, 1u, 4.1f));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToArray(arr, 0u, 3));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToArray(arr, 0u, 2));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToArray(arr, 0u, 1));

	DataStore out;
	SEOUL_UNITTESTING_ASSERT(ComputeDiff(dataStoreA, dataStoreB, out));

	// Apply patch - this should make a copied version of A equal to B.
	{
		DataStore res;
		res.CopyFrom(dataStoreA);
		SEOUL_UNITTESTING_ASSERT(ApplyDiff(out, res));

		// Verify, a and b are not equal.
		SEOUL_UNITTESTING_ASSERT(!DataStore::Equals(
			dataStoreA, dataStoreA.GetRootNode(),
			dataStoreB, dataStoreB.GetRootNode()));

		// Verify that res is no longer equal to A.
		SEOUL_UNITTESTING_ASSERT(!DataStore::Equals(
			res,
			res.GetRootNode(),
			dataStoreA,
			dataStoreA.GetRootNode()));
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(res.ComputeMD5(), dataStoreA.ComputeMD5());

		SEOUL_UNITTESTING_ASSERT(DataStore::Equals(
			res,
			res.GetRootNode(),
			dataStoreB,
			dataStoreB.GetRootNode()));
		SEOUL_UNITTESTING_ASSERT_EQUAL(res.ComputeMD5(), dataStoreB.ComputeMD5());
	}
}

void DataStoreTest::TestDiffNullDiff()
{
	DataStore dataStore;
	dataStore.MakeTable();
	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToTable(dataStore.GetRootNode(), HString("A"), 0));
	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToTable(dataStore.GetRootNode(), HString("a"), 1));
	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToTable(dataStore.GetRootNode(), HString("B"), 2));
	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToTable(dataStore.GetRootNode(), HString("b"), 3));
	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToTable(dataStore.GetRootNode(), HString("C"), 4));
	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToTable(dataStore.GetRootNode(), HString("c"), 5));
	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToTable(dataStore.GetRootNode(), HString("D"), 6));
	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToTable(dataStore.GetRootNode(), HString("d"), 7));
	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToTable(dataStore.GetRootNode(), HString("E"), 8));
	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToTable(dataStore.GetRootNode(), HString("e"), 9));

	// Null diff.
	{
		DataStore diff;
		DataStore copy;
		copy.CopyFrom(dataStore);
		SEOUL_UNITTESTING_ASSERT(ApplyDiff(diff, dataStore));
		SEOUL_UNITTESTING_ASSERT(DataStore::UnitTestHook_ByteForByteEqual(dataStore, copy));
		SEOUL_UNITTESTING_ASSERT(DataStore::Equals(
			dataStore,
			dataStore.GetRootNode(),
			copy,
			copy.GetRootNode()));
	}

	// Null target.
	{
		DataStore target;
		SEOUL_UNITTESTING_ASSERT(ApplyDiff(dataStore, target));
		SEOUL_UNITTESTING_ASSERT(DataStore::Equals(
			dataStore,
			dataStore.GetRootNode(),
			target,
			target.GetRootNode()));
	}
}

void DataStoreTest::TestDiffNullTable()
{
	DataStore dataStore;
	dataStore.MakeTable();
	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToTable(dataStore.GetRootNode(), HString("A"), 0));
	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToTable(dataStore.GetRootNode(), HString("a"), 1));
	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToTable(dataStore.GetRootNode(), HString("B"), 2));
	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToTable(dataStore.GetRootNode(), HString("b"), 3));
	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToTable(dataStore.GetRootNode(), HString("C"), 4));
	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToTable(dataStore.GetRootNode(), HString("c"), 5));
	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToTable(dataStore.GetRootNode(), HString("D"), 6));
	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToTable(dataStore.GetRootNode(), HString("d"), 7));
	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToTable(dataStore.GetRootNode(), HString("E"), 8));
	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToTable(dataStore.GetRootNode(), HString("e"), 9));

	// Null target.
	{
		DataStore null;
		DataStore diff;
		SEOUL_UNITTESTING_ASSERT(ComputeDiff(dataStore, null, diff));
		SEOUL_UNITTESTING_ASSERT(diff.GetRootNode().IsTable());

		DataStore copy;
		copy.CopyFrom(dataStore);
		SEOUL_UNITTESTING_ASSERT(ApplyDiff(diff, copy));
		SEOUL_UNITTESTING_ASSERT(copy.GetRootNode().IsTable());
		SEOUL_UNITTESTING_ASSERT_EQUAL(copy.TableBegin(copy.GetRootNode()), copy.TableEnd(copy.GetRootNode()));
	}
}

void DataStoreTest::TestDiffSelf()
{
	DataStore dataStoreA;
	dataStoreA.MakeTable();
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToTable(dataStoreA.GetRootNode(), HString("A"), 0));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToTable(dataStoreA.GetRootNode(), HString("a"), 1));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToTable(dataStoreA.GetRootNode(), HString("B"), 2));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToTable(dataStoreA.GetRootNode(), HString("b"), 3));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToTable(dataStoreA.GetRootNode(), HString("C"), 4));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToTable(dataStoreA.GetRootNode(), HString("c"), 5));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToTable(dataStoreA.GetRootNode(), HString("D"), 6));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToTable(dataStoreA.GetRootNode(), HString("d"), 7));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToTable(dataStoreA.GetRootNode(), HString("E"), 8));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToTable(dataStoreA.GetRootNode(), HString("e"), 9));

	DataStore dataStoreB;
	dataStoreB.MakeTable();
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToTable(dataStoreB.GetRootNode(), HString("e"), 9));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToTable(dataStoreB.GetRootNode(), HString("E"), 8));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToTable(dataStoreB.GetRootNode(), HString("d"), 7));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToTable(dataStoreB.GetRootNode(), HString("D"), 6));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToTable(dataStoreB.GetRootNode(), HString("c"), 5));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToTable(dataStoreB.GetRootNode(), HString("C"), 4));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToTable(dataStoreB.GetRootNode(), HString("b"), 3));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToTable(dataStoreB.GetRootNode(), HString("B"), 2));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToTable(dataStoreB.GetRootNode(), HString("a"), 1));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToTable(dataStoreB.GetRootNode(), HString("A"), 0));

	// Self diff, should produce no difference.
	{
		DataStore out;
		SEOUL_UNITTESTING_ASSERT(ComputeDiff(dataStoreA, dataStoreB, out));

		{
			DataStore res;
			res.CopyFrom(dataStoreA);
			SEOUL_UNITTESTING_ASSERT(ApplyDiff(out, res));

			SEOUL_UNITTESTING_ASSERT(DataStore::Equals(
				res,
				res.GetRootNode(),
				dataStoreA,
				dataStoreA.GetRootNode()));
			SEOUL_UNITTESTING_ASSERT_EQUAL(res.ComputeMD5(), dataStoreA.ComputeMD5());
			SEOUL_UNITTESTING_ASSERT(DataStore::UnitTestHook_ByteForByteEqual(res, dataStoreA));
		}
		{
			DataStore res;
			res.CopyFrom(dataStoreB);
			SEOUL_UNITTESTING_ASSERT(ApplyDiff(out, res));

			SEOUL_UNITTESTING_ASSERT(DataStore::Equals(
				res,
				res.GetRootNode(),
				dataStoreB,
				dataStoreB.GetRootNode()));
			SEOUL_UNITTESTING_ASSERT_EQUAL(res.ComputeMD5(), dataStoreB.ComputeMD5());
			SEOUL_UNITTESTING_ASSERT(DataStore::UnitTestHook_ByteForByteEqual(res, dataStoreB));
		}
	}
}

void DataStoreTest::TestDiffSubtractive()
{
	DataStore dataStoreA;
	dataStoreA.MakeTable();
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToTable(dataStoreA.GetRootNode(), HString("A"), 0));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToTable(dataStoreA.GetRootNode(), HString("a"), 1));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToTable(dataStoreA.GetRootNode(), HString("B"), 2));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToTable(dataStoreA.GetRootNode(), HString("b"), 3));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToTable(dataStoreA.GetRootNode(), HString("C"), 4));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToTable(dataStoreA.GetRootNode(), HString("c"), 5));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToTable(dataStoreA.GetRootNode(), HString("D"), 6));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToTable(dataStoreA.GetRootNode(), HString("d"), 7));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToTable(dataStoreA.GetRootNode(), HString("E"), 8));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToTable(dataStoreA.GetRootNode(), HString("e"), 9));

	// B is missing some entries compared to A.
	DataStore dataStoreB;
	dataStoreB.MakeTable();
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToTable(dataStoreB.GetRootNode(), HString("e"), 9));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToTable(dataStoreB.GetRootNode(), HString("E"), 8));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToTable(dataStoreB.GetRootNode(), HString("d"), 7));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToTable(dataStoreB.GetRootNode(), HString("D"), 6));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToTable(dataStoreB.GetRootNode(), HString("c"), 5));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToTable(dataStoreB.GetRootNode(), HString("C"), 4));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToTable(dataStoreB.GetRootNode(), HString("b"), 3));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToTable(dataStoreB.GetRootNode(), HString("B"), 2));

	DataStore out;
	SEOUL_UNITTESTING_ASSERT(ComputeDiff(dataStoreA, dataStoreB, out));

	// Apply patch - this should make a copied version of A equal to B.
	{
		DataStore res;
		res.CopyFrom(dataStoreA);
		SEOUL_UNITTESTING_ASSERT(ApplyDiff(out, res));

		// Verify, a and b are not equal.
		SEOUL_UNITTESTING_ASSERT(!DataStore::Equals(
			dataStoreA, dataStoreA.GetRootNode(),
			dataStoreB, dataStoreB.GetRootNode()));

		// Verify that res is no longer equal to A.
		SEOUL_UNITTESTING_ASSERT(!DataStore::Equals(
			res,
			res.GetRootNode(),
			dataStoreA,
			dataStoreA.GetRootNode()));
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(res.ComputeMD5(), dataStoreA.ComputeMD5());

		SEOUL_UNITTESTING_ASSERT(DataStore::Equals(
			res,
			res.GetRootNode(),
			dataStoreB,
			dataStoreB.GetRootNode()));
		SEOUL_UNITTESTING_ASSERT_EQUAL(res.ComputeMD5(), dataStoreB.ComputeMD5());
	}
}

void DataStoreTest::TestDiffSubtractiveToString()
{
	static const String ksExpectedString = "{\"A\":null,\"a\":null}";

	DataStore dataStoreA;
	dataStoreA.MakeTable();
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToTable(dataStoreA.GetRootNode(), HString("A"), 0));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToTable(dataStoreA.GetRootNode(), HString("a"), 1));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToTable(dataStoreA.GetRootNode(), HString("B"), 2));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToTable(dataStoreA.GetRootNode(), HString("b"), 3));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToTable(dataStoreA.GetRootNode(), HString("C"), 4));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToTable(dataStoreA.GetRootNode(), HString("c"), 5));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToTable(dataStoreA.GetRootNode(), HString("D"), 6));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToTable(dataStoreA.GetRootNode(), HString("d"), 7));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToTable(dataStoreA.GetRootNode(), HString("E"), 8));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToTable(dataStoreA.GetRootNode(), HString("e"), 9));

	// B is missing some entries compared to A.
	DataStore dataStoreB;
	dataStoreB.MakeTable();
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToTable(dataStoreB.GetRootNode(), HString("e"), 9));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToTable(dataStoreB.GetRootNode(), HString("E"), 8));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToTable(dataStoreB.GetRootNode(), HString("d"), 7));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToTable(dataStoreB.GetRootNode(), HString("D"), 6));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToTable(dataStoreB.GetRootNode(), HString("c"), 5));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToTable(dataStoreB.GetRootNode(), HString("C"), 4));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToTable(dataStoreB.GetRootNode(), HString("b"), 3));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToTable(dataStoreB.GetRootNode(), HString("B"), 2));

	DataStore out;
	SEOUL_UNITTESTING_ASSERT(ComputeDiff(dataStoreA, dataStoreB, out));

	String s;
	out.ToString(out.GetRootNode(), s, false, 0, true);
	SEOUL_UNITTESTING_ASSERT_EQUAL(ksExpectedString, s);
}

static void TestDiffTableMultipleUtil(const DataStore& dataStoreA, const DataStore& dataStoreB, const String& sB)
{
	DataStore out;
	SEOUL_UNITTESTING_ASSERT(ComputeDiff(dataStoreA, dataStoreB, out));

	// Apply patch - this should make a copied version of A equal to B.
	{
		DataStore res;
		res.CopyFrom(dataStoreA);
		SEOUL_UNITTESTING_ASSERT(ApplyDiff(out, res));

		// Verify, a and b are not equal.
		SEOUL_UNITTESTING_ASSERT(!DataStore::Equals(
			dataStoreA, dataStoreA.GetRootNode(),
			dataStoreB, dataStoreB.GetRootNode(), true));

		// Verify that res is no longer equal to A.
		SEOUL_UNITTESTING_ASSERT(!DataStore::Equals(
			res,
			res.GetRootNode(),
			dataStoreA,
			dataStoreA.GetRootNode(), true));
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(res.ComputeMD5(), dataStoreA.ComputeMD5());

		SEOUL_UNITTESTING_ASSERT(DataStore::Equals(
			res,
			res.GetRootNode(),
			dataStoreB,
			dataStoreB.GetRootNode(),
			true));
		SEOUL_UNITTESTING_ASSERT_EQUAL(res.ComputeMD5(), dataStoreB.ComputeMD5());

		// String test.
		String s;
		res.ToString(res.GetRootNode(), s, false, 0, true);
		SEOUL_UNITTESTING_ASSERT_EQUAL(sB, s);
	}
}

void DataStoreTest::TestDiffTableMultiple()
{
	static const Pair<String, String> s_kaTests[] =
	{
		MakePair("{\"A\":{\"B\":true}}", "{\"A\":{\"B\":false}}"), // Nested table.
		MakePair("{\"A\":{\"B\":5},\"C\":1.5}", "{\"A\":{\"B\":7},\"C\":3}"), // Multiple nested tables.
		MakePair("{\"A\":{\"B\":true}}", "{\"A\":{\"B\":\"Hello World\"}}"), // Type change.
		MakePair("{}", "[]"), // Root type change.
		MakePair("{\"A\":{\"B\":true}}", "{\"A\":[true]}"), // Complex type change.
		MakePair("{\"A\":{\"B\":true}}", "{}"), //  Full delete.
		MakePair("{\"A\":{\"B\":true},\"Boo\":false}", "{}"), //  Full delete, with multiple keys.
		MakePair("{\"A\":{\"B\":true},\"Boo\":false}", "{\"Boo\":false}"), //  Single delete, with multiple keys.
		MakePair("{\"A\":0}", "{\"A\":5}"), // Value change.
		MakePair("{\"A\":0}", "{\"A\":NaN}"), // Value change, to a NaN.
		MakePair("{\"A\":5}", "{\"A\":Infinity}"), // Value change, infinity.
		MakePair("{\"A\":5}", "[NaN]"), // Value change, complex, with a NaN.
	};

	for (size_t i = 0u; i < SEOUL_ARRAY_COUNT(s_kaTests); ++i)
	{
		DataStore dataStoreA;
		SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromString(s_kaTests[i].First, dataStoreA));
		DataStore dataStoreB;
		SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromString(s_kaTests[i].Second, dataStoreB));

		// String tests
		{
			String s;
			dataStoreA.ToString(dataStoreA.GetRootNode(), s, false, 0, true);
			SEOUL_UNITTESTING_ASSERT_EQUAL(s_kaTests[i].First, s);
		}
		{
			String s;
			dataStoreB.ToString(dataStoreB.GetRootNode(), s, false, 0, true);
			SEOUL_UNITTESTING_ASSERT_EQUAL(s_kaTests[i].Second, s);
		}

		TestDiffTableMultipleUtil(dataStoreA, dataStoreB, s_kaTests[i].Second); // A -> B
		TestDiffTableMultipleUtil(dataStoreB, dataStoreA, s_kaTests[i].First); // B -> A
	}
}

// Regression for a bug in DataStore::Equals() when bNaNEquals is true.
void DataStoreTest::TestEqualsNaN()
{
	// Array.
	{
		DataStore dataStoreA;
		dataStoreA.MakeArray();
		dataStoreA.SetFloat32ValueToArray(dataStoreA.GetRootNode(), 5, std::numeric_limits<Float>::quiet_NaN());

		DataStore dataStoreB;
		dataStoreB.MakeArray();
		dataStoreB.SetFloat32ValueToArray(dataStoreB.GetRootNode(), 5, std::numeric_limits<Float>::signaling_NaN());

		SEOUL_UNITTESTING_ASSERT(!DataStore::Equals(
			dataStoreA,
			dataStoreA.GetRootNode(),
			dataStoreB,
			dataStoreB.GetRootNode()));
		SEOUL_UNITTESTING_ASSERT(DataStore::Equals(
			dataStoreA,
			dataStoreA.GetRootNode(),
			dataStoreB,
			dataStoreB.GetRootNode(),
			true));
	}

	// Table.
	{
		DataStore dataStoreA;
		dataStoreA.MakeTable();
		dataStoreA.SetFloat32ValueToTable(dataStoreA.GetRootNode(), HString("A"), std::numeric_limits<Float>::quiet_NaN());

		DataStore dataStoreB;
		dataStoreB.MakeTable();
		dataStoreB.SetFloat32ValueToTable(dataStoreB.GetRootNode(), HString("A"), std::numeric_limits<Float>::signaling_NaN());

		SEOUL_UNITTESTING_ASSERT(!DataStore::Equals(
			dataStoreA,
			dataStoreA.GetRootNode(),
			dataStoreB,
			dataStoreB.GetRootNode()));
		SEOUL_UNITTESTING_ASSERT(DataStore::Equals(
			dataStoreA,
			dataStoreA.GetRootNode(),
			dataStoreB,
			dataStoreB.GetRootNode(),
			true));
	}
}

void DataStoreTest::TestMD5()
{
	DataStore dataStoreA;
	dataStoreA.MakeTable();
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToTable(dataStoreA.GetRootNode(), HString("A"), 0));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToTable(dataStoreA.GetRootNode(), HString("a"), 1));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToTable(dataStoreA.GetRootNode(), HString("B"), 2));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToTable(dataStoreA.GetRootNode(), HString("b"), 3));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToTable(dataStoreA.GetRootNode(), HString("C"), 4));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToTable(dataStoreA.GetRootNode(), HString("c"), 5));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToTable(dataStoreA.GetRootNode(), HString("D"), 6));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToTable(dataStoreA.GetRootNode(), HString("d"), 7));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToTable(dataStoreA.GetRootNode(), HString("E"), 8));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetInt32ValueToTable(dataStoreA.GetRootNode(), HString("e"), 9));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetFloat32ValueToTable(dataStoreA.GetRootNode(), HString("F"), 4.1f));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetFloat32ValueToTable(dataStoreA.GetRootNode(), HString("f"), 1.5f));

	DataStore dataStoreB;
	dataStoreB.MakeTable();
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetFloat32ValueToTable(dataStoreB.GetRootNode(), HString("f"), 1.5f));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetFloat32ValueToTable(dataStoreB.GetRootNode(), HString("F"), 4.1f));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToTable(dataStoreB.GetRootNode(), HString("e"), 9));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToTable(dataStoreB.GetRootNode(), HString("E"), 8));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToTable(dataStoreB.GetRootNode(), HString("d"), 7));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToTable(dataStoreB.GetRootNode(), HString("D"), 6));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToTable(dataStoreB.GetRootNode(), HString("c"), 5));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToTable(dataStoreB.GetRootNode(), HString("C"), 4));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToTable(dataStoreB.GetRootNode(), HString("b"), 3));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToTable(dataStoreB.GetRootNode(), HString("B"), 2));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToTable(dataStoreB.GetRootNode(), HString("a"), 1));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetInt32ValueToTable(dataStoreB.GetRootNode(), HString("A"), 0));

	// At this point, the two datastores should be exactly equal, and their MD5s should be exactly
	// the same.
	SEOUL_UNITTESTING_ASSERT(DataStore::Equals(
		dataStoreA,
		dataStoreA.GetRootNode(),
		dataStoreB,
		dataStoreB.GetRootNode()));
	String const sA = dataStoreA.ComputeMD5();
	String const sB = dataStoreB.ComputeMD5();
	SEOUL_UNITTESTING_ASSERT_EQUAL(sA, sB);

	// Also test that all values are equal, manually.
	TestMD5CheckValues(dataStoreA);
	TestMD5CheckValues(dataStoreB);
}

void DataStoreTest::TestMD5Inf()
{
	DataStore dataStoreA;
	dataStoreA.MakeTable();
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetFloat32ValueToTable(dataStoreA.GetRootNode(), HString("A"), std::numeric_limits<Float>::infinity()));
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetFloat32ValueToTable(dataStoreA.GetRootNode(), HString("B"), -std::numeric_limits<Float>::infinity()));

	DataStore dataStoreB;
	dataStoreB.MakeTable();
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetFloat32ValueToTable(dataStoreB.GetRootNode(), HString("A"), std::numeric_limits<Float>::infinity()));
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetFloat32ValueToTable(dataStoreB.GetRootNode(), HString("B"), -std::numeric_limits<Float>::infinity()));

	// At this point, the two datastores should be exactly equal, and their MD5s should be exactly
	// the same.
	SEOUL_UNITTESTING_ASSERT(DataStore::Equals(
		dataStoreA,
		dataStoreA.GetRootNode(),
		dataStoreB,
		dataStoreB.GetRootNode()));
	String const sA = dataStoreA.ComputeMD5();
	String const sB = dataStoreB.ComputeMD5();
	SEOUL_UNITTESTING_ASSERT_EQUAL(sA, sB);
}

void DataStoreTest::TestMD5NaN()
{
	DataStore dataStoreA;
	dataStoreA.MakeTable();
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetFloat32ValueToTable(dataStoreA.GetRootNode(), HString("A"), std::numeric_limits<Float>::quiet_NaN()));

	DataStore dataStoreB;
	dataStoreB.MakeTable();
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetFloat32ValueToTable(dataStoreB.GetRootNode(), HString("A"), std::numeric_limits<Float>::signaling_NaN()));

	// Manual value checking.
	{
		DataNode value;
		dataStoreA.GetValueFromTable(dataStoreA.GetRootNode(), HString("A"), value);
		SEOUL_UNITTESTING_ASSERT(value.IsFloat31());
		SEOUL_UNITTESTING_ASSERT(IsNaN(dataStoreA.AssumeFloat31(value)));
	}
	{
		DataNode value;
		dataStoreB.GetValueFromTable(dataStoreB.GetRootNode(), HString("A"), value);
		SEOUL_UNITTESTING_ASSERT(value.IsFloat31());
		SEOUL_UNITTESTING_ASSERT(IsNaN(dataStoreB.AssumeFloat31(value)));
	}

	// DataStore equality should fail here, because NaN is never equal to NaN.
	SEOUL_UNITTESTING_ASSERT(!DataStore::Equals(
		dataStoreA,
		dataStoreA.GetRootNode(),
		dataStoreB,
		dataStoreB.GetRootNode()));

	// DataStore equality should succeed here, with the optional bNanEqual set to true.
	SEOUL_UNITTESTING_ASSERT(DataStore::Equals(
		dataStoreA,
		dataStoreA.GetRootNode(),
		dataStoreB,
		dataStoreB.GetRootNode(),
		true));

	// The checksum will be equal given how DataStore converts floats into a canonical form.
	String const sA = dataStoreA.ComputeMD5();
	String const sB = dataStoreB.ComputeMD5();
	SEOUL_UNITTESTING_ASSERT_EQUAL(sA, sB);
}

void DataStoreTest::TestMD5Zero()
{
	DataStore dataStoreA;
	dataStoreA.MakeTable();
	SEOUL_UNITTESTING_ASSERT(dataStoreA.SetFloat32ValueToTable(dataStoreA.GetRootNode(), HString("A"), 0.0f));

	DataStore dataStoreB;
	dataStoreB.MakeTable();
	SEOUL_UNITTESTING_ASSERT(dataStoreB.SetFloat32ValueToTable(dataStoreB.GetRootNode(), HString("A"), -0.0f));

	// At this point, the two datastores should be exactly equal, and their MD5s should be exactly
	// the same.
	SEOUL_UNITTESTING_ASSERT(DataStore::Equals(
		dataStoreA,
		dataStoreA.GetRootNode(),
		dataStoreB,
		dataStoreB.GetRootNode()));
	String const sA = dataStoreA.ComputeMD5();
	String const sB = dataStoreB.ComputeMD5();
	SEOUL_UNITTESTING_ASSERT_EQUAL(sA, sB);
}

/**
 * Make sure that we can create an empty DataStore and that it is
 * in the expected state.
 */
void DataStoreTest::TestInstantiation()
{
	{
		DataStore dataStore;

		SEOUL_UNITTESTING_ASSERT(dataStore.GetRootNode().IsNull());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0u, dataStore.GetHeapCapacityInBytes());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0u, dataStore.GetInUseHeapSizeInBytes());

		// Check a few values that should succeed on a nil root.
		String sTest("TEST TEST");
		SEOUL_UNITTESTING_ASSERT(dataStore.AsString(dataStore.GetRootNode(), sTest));
		SEOUL_UNITTESTING_ASSERT_EQUAL(sTest, String());
		FilePath testFilePath;
		testFilePath.SetDirectory(GameDirectory::kToolsBin);
		SEOUL_UNITTESTING_ASSERT(dataStore.AsFilePath(dataStore.GetRootNode(), testFilePath));
		SEOUL_UNITTESTING_ASSERT_EQUAL(testFilePath, FilePath());

		// Check that most operations fail on the root node.
		SEOUL_UNITTESTING_ASSERT_EQUAL(dataStore.TableBegin(dataStore.GetRootNode()), dataStore.TableEnd(dataStore.GetRootNode()));

		UInt32 u = 124u;
		SEOUL_UNITTESTING_ASSERT(!dataStore.GetArrayCapacity(dataStore.GetRootNode(), u));
		SEOUL_UNITTESTING_ASSERT_EQUAL(124u, u);
		SEOUL_UNITTESTING_ASSERT(!dataStore.GetArrayCount(dataStore.GetRootNode(), u));
		SEOUL_UNITTESTING_ASSERT_EQUAL(124u, u);

		SEOUL_UNITTESTING_ASSERT(!dataStore.ArrayContains(dataStore.GetRootNode(), HString("TestIt")));

		Bool b = false;
		SEOUL_UNITTESTING_ASSERT(!dataStore.AsBoolean(dataStore.GetRootNode(), b));
		SEOUL_UNITTESTING_ASSERT_EQUAL(false, b);

		Float32 f = 1.57f;
		SEOUL_UNITTESTING_ASSERT(!dataStore.AsFloat32(dataStore.GetRootNode(), f));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1.57f, f);

		Int32 i = 17532;
		SEOUL_UNITTESTING_ASSERT(!dataStore.AsInt32(dataStore.GetRootNode(), i));
		SEOUL_UNITTESTING_ASSERT_EQUAL(17532, i);

		Int64 i64 = 12345;
		SEOUL_UNITTESTING_ASSERT(!dataStore.AsInt64(dataStore.GetRootNode(), i64));
		SEOUL_UNITTESTING_ASSERT_EQUAL(12345, i64);

		UInt32 u32 = 12345;
		SEOUL_UNITTESTING_ASSERT(!dataStore.AsUInt32(dataStore.GetRootNode(), u32));
		SEOUL_UNITTESTING_ASSERT_EQUAL(12345, u32);

		UInt64 u64 = 12345;
		SEOUL_UNITTESTING_ASSERT(!dataStore.AsUInt64(dataStore.GetRootNode(), u64));
		SEOUL_UNITTESTING_ASSERT_EQUAL(12345, u64);

		SEOUL_UNITTESTING_ASSERT(!dataStore.EraseValueFromArray(dataStore.GetRootNode(), 0u));
		SEOUL_UNITTESTING_ASSERT(!dataStore.EraseValueFromTable(dataStore.GetRootNode(), HString("TestFoo")));

		SEOUL_UNITTESTING_ASSERT(!dataStore.GetTableCapacity(dataStore.GetRootNode(), u));
		SEOUL_UNITTESTING_ASSERT_EQUAL(124u, u);
		SEOUL_UNITTESTING_ASSERT(!dataStore.GetTableCount(dataStore.GetRootNode(), u));
		SEOUL_UNITTESTING_ASSERT_EQUAL(124u, u);

		DataNode testValue;
		SEOUL_UNITTESTING_ASSERT(!dataStore.GetValueFromArray(dataStore.GetRootNode(), 0u, testValue));
		SEOUL_UNITTESTING_ASSERT(testValue.IsNull());
		SEOUL_UNITTESTING_ASSERT(!dataStore.GetValueFromTable(dataStore.GetRootNode(), HString("TestFoooom"), testValue));
		SEOUL_UNITTESTING_ASSERT(testValue.IsNull());

		SEOUL_UNITTESTING_ASSERT(!dataStore.ResizeArray(dataStore.GetRootNode(), 1024u));
		SEOUL_UNITTESTING_ASSERT(!dataStore.SetArrayToArray(dataStore.GetRootNode(), 0u, 1024u));
		SEOUL_UNITTESTING_ASSERT(!dataStore.SetArrayToTable(dataStore.GetRootNode(), HString("TestBoom"), 1024u));
		SEOUL_UNITTESTING_ASSERT(!dataStore.SetBooleanValueToArray(dataStore.GetRootNode(), 0u, true));
		SEOUL_UNITTESTING_ASSERT(!dataStore.SetBooleanValueToTable(dataStore.GetRootNode(), HString("TestBoom"), true));
		SEOUL_UNITTESTING_ASSERT(!dataStore.SetFilePathToArray(dataStore.GetRootNode(), 0u, FilePath()));
		SEOUL_UNITTESTING_ASSERT(!dataStore.SetFilePathToTable(dataStore.GetRootNode(), HString("TestBoom"), FilePath()));
		SEOUL_UNITTESTING_ASSERT(!dataStore.SetFloat32ValueToArray(dataStore.GetRootNode(), 0u, 1.7f));
		SEOUL_UNITTESTING_ASSERT(!dataStore.SetFloat32ValueToTable(dataStore.GetRootNode(), HString("TestBoom"), 1.7f));
		SEOUL_UNITTESTING_ASSERT(!dataStore.SetInt32ValueToArray(dataStore.GetRootNode(), 0u, 523));
		SEOUL_UNITTESTING_ASSERT(!dataStore.SetInt32ValueToTable(dataStore.GetRootNode(), HString("TestBoom"), 523));
		SEOUL_UNITTESTING_ASSERT(!dataStore.SetInt64ValueToArray(dataStore.GetRootNode(), 0u, 523));
		SEOUL_UNITTESTING_ASSERT(!dataStore.SetInt64ValueToTable(dataStore.GetRootNode(), HString("TestBoom"), 523));
		SEOUL_UNITTESTING_ASSERT(!dataStore.SetNullValueToArray(dataStore.GetRootNode(), 0u));
		SEOUL_UNITTESTING_ASSERT(!dataStore.SetNullValueToTable(dataStore.GetRootNode(), HString("TestBoom")));
		SEOUL_UNITTESTING_ASSERT(!dataStore.SetStringToArray(dataStore.GetRootNode(), 0u, "Test test!"));
		SEOUL_UNITTESTING_ASSERT(!dataStore.SetStringToTable(dataStore.GetRootNode(), HString("TestBoom"), "Test test!"));
		SEOUL_UNITTESTING_ASSERT(!dataStore.SetTableToArray(dataStore.GetRootNode(), 0u, 1024));
		SEOUL_UNITTESTING_ASSERT(!dataStore.SetTableToTable(dataStore.GetRootNode(), HString("TestBoom"), 1024));
		SEOUL_UNITTESTING_ASSERT(!dataStore.SetUInt32ValueToArray(dataStore.GetRootNode(), 0u, 1024));
		SEOUL_UNITTESTING_ASSERT(!dataStore.SetUInt32ValueToTable(dataStore.GetRootNode(), HString("TestBoom"), 1024));
		SEOUL_UNITTESTING_ASSERT(!dataStore.SetUInt64ValueToArray(dataStore.GetRootNode(), 0u, 1024));
		SEOUL_UNITTESTING_ASSERT(!dataStore.SetUInt64ValueToTable(dataStore.GetRootNode(), HString("TestBoom"), 1024));
		SEOUL_UNITTESTING_ASSERT(!dataStore.TableContainsKey(dataStore.GetRootNode(), HString()));
		SEOUL_UNITTESTING_ASSERT(!dataStore.SetStringToArray(dataStore.GetRootNode(), 0u, "ASDF", 4u));
		SEOUL_UNITTESTING_ASSERT(!dataStore.SetStringToTable(dataStore.GetRootNode(), HString("Whatit"), "ASDF", 4u));
	}

	{
		DataStore dataStore(1024u);

		SEOUL_UNITTESTING_ASSERT(dataStore.GetRootNode().IsNull());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1024u, dataStore.GetHeapCapacityInBytes());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0u, dataStore.GetInUseHeapSizeInBytes());

		// Check a few values that should succeed on a nil root.
		String sTest("TEST TEST");
		SEOUL_UNITTESTING_ASSERT(dataStore.AsString(dataStore.GetRootNode(), sTest));
		SEOUL_UNITTESTING_ASSERT_EQUAL(sTest, String());
		FilePath testFilePath;
		testFilePath.SetDirectory(GameDirectory::kToolsBin);
		SEOUL_UNITTESTING_ASSERT(dataStore.AsFilePath(dataStore.GetRootNode(), testFilePath));
		SEOUL_UNITTESTING_ASSERT_EQUAL(testFilePath, FilePath());

		// Check that most operations fail on the root node.
		SEOUL_UNITTESTING_ASSERT_EQUAL(dataStore.TableBegin(dataStore.GetRootNode()), dataStore.TableEnd(dataStore.GetRootNode()));

		UInt32 u = 124u;
		SEOUL_UNITTESTING_ASSERT(!dataStore.GetArrayCapacity(dataStore.GetRootNode(), u));
		SEOUL_UNITTESTING_ASSERT_EQUAL(124u, u);
		SEOUL_UNITTESTING_ASSERT(!dataStore.GetArrayCount(dataStore.GetRootNode(), u));
		SEOUL_UNITTESTING_ASSERT_EQUAL(124u, u);

		SEOUL_UNITTESTING_ASSERT(!dataStore.ArrayContains(dataStore.GetRootNode(), HString("TestIt")));

		Bool b = false;
		SEOUL_UNITTESTING_ASSERT(!dataStore.AsBoolean(dataStore.GetRootNode(), b));
		SEOUL_UNITTESTING_ASSERT_EQUAL(false, b);

		Float32 f = 1.57f;
		SEOUL_UNITTESTING_ASSERT(!dataStore.AsFloat32(dataStore.GetRootNode(), f));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1.57f, f);

		Int32 i = 17532;
		SEOUL_UNITTESTING_ASSERT(!dataStore.AsInt32(dataStore.GetRootNode(), i));
		SEOUL_UNITTESTING_ASSERT_EQUAL(17532, i);

		Int64 i64 = 12345;
		SEOUL_UNITTESTING_ASSERT(!dataStore.AsInt64(dataStore.GetRootNode(), i64));
		SEOUL_UNITTESTING_ASSERT_EQUAL(12345, i64);

		UInt32 u32 = 12345;
		SEOUL_UNITTESTING_ASSERT(!dataStore.AsUInt32(dataStore.GetRootNode(), u32));
		SEOUL_UNITTESTING_ASSERT_EQUAL(12345, u32);

		UInt64 u64 = 12345;
		SEOUL_UNITTESTING_ASSERT(!dataStore.AsUInt64(dataStore.GetRootNode(), u64));
		SEOUL_UNITTESTING_ASSERT_EQUAL(12345, u64);

		SEOUL_UNITTESTING_ASSERT(!dataStore.EraseValueFromArray(dataStore.GetRootNode(), 0u));
		SEOUL_UNITTESTING_ASSERT(!dataStore.EraseValueFromTable(dataStore.GetRootNode(), HString("TestFoo")));

		SEOUL_UNITTESTING_ASSERT(!dataStore.GetTableCapacity(dataStore.GetRootNode(), u));
		SEOUL_UNITTESTING_ASSERT_EQUAL(124u, u);
		SEOUL_UNITTESTING_ASSERT(!dataStore.GetTableCount(dataStore.GetRootNode(), u));
		SEOUL_UNITTESTING_ASSERT_EQUAL(124u, u);

		DataNode testValue;
		SEOUL_UNITTESTING_ASSERT(!dataStore.GetValueFromArray(dataStore.GetRootNode(), 0u, testValue));
		SEOUL_UNITTESTING_ASSERT(testValue.IsNull());
		SEOUL_UNITTESTING_ASSERT(!dataStore.GetValueFromTable(dataStore.GetRootNode(), HString("TestFoooom"), testValue));
		SEOUL_UNITTESTING_ASSERT(testValue.IsNull());

		SEOUL_UNITTESTING_ASSERT(!dataStore.ResizeArray(dataStore.GetRootNode(), 1024u));
		SEOUL_UNITTESTING_ASSERT(!dataStore.SetArrayToArray(dataStore.GetRootNode(), 0u, 1024u));
		SEOUL_UNITTESTING_ASSERT(!dataStore.SetArrayToTable(dataStore.GetRootNode(), HString("TestBoom"), 1024u));
		SEOUL_UNITTESTING_ASSERT(!dataStore.SetBooleanValueToArray(dataStore.GetRootNode(), 0u, true));
		SEOUL_UNITTESTING_ASSERT(!dataStore.SetBooleanValueToTable(dataStore.GetRootNode(), HString("TestBoom"), true));
		SEOUL_UNITTESTING_ASSERT(!dataStore.SetFilePathToArray(dataStore.GetRootNode(), 0u, FilePath()));
		SEOUL_UNITTESTING_ASSERT(!dataStore.SetFilePathToTable(dataStore.GetRootNode(), HString("TestBoom"), FilePath()));
		SEOUL_UNITTESTING_ASSERT(!dataStore.SetFloat32ValueToArray(dataStore.GetRootNode(), 0u, 1.7f));
		SEOUL_UNITTESTING_ASSERT(!dataStore.SetFloat32ValueToTable(dataStore.GetRootNode(), HString("TestBoom"), 1.7f));
		SEOUL_UNITTESTING_ASSERT(!dataStore.SetInt32ValueToArray(dataStore.GetRootNode(), 0u, 523));
		SEOUL_UNITTESTING_ASSERT(!dataStore.SetInt32ValueToTable(dataStore.GetRootNode(), HString("TestBoom"), 523));
		SEOUL_UNITTESTING_ASSERT(!dataStore.SetInt64ValueToArray(dataStore.GetRootNode(), 0u, 523));
		SEOUL_UNITTESTING_ASSERT(!dataStore.SetInt64ValueToTable(dataStore.GetRootNode(), HString("TestBoom"), 523));
		SEOUL_UNITTESTING_ASSERT(!dataStore.SetNullValueToArray(dataStore.GetRootNode(), 0u));
		SEOUL_UNITTESTING_ASSERT(!dataStore.SetNullValueToTable(dataStore.GetRootNode(), HString("TestBoom")));
		SEOUL_UNITTESTING_ASSERT(!dataStore.SetStringToArray(dataStore.GetRootNode(), 0u, "Test test!"));
		SEOUL_UNITTESTING_ASSERT(!dataStore.SetStringToTable(dataStore.GetRootNode(), HString("TestBoom"), "Test test!"));
		SEOUL_UNITTESTING_ASSERT(!dataStore.SetTableToArray(dataStore.GetRootNode(), 0u, 1024));
		SEOUL_UNITTESTING_ASSERT(!dataStore.SetTableToTable(dataStore.GetRootNode(), HString("TestBoom"), 1024));
		SEOUL_UNITTESTING_ASSERT(!dataStore.SetUInt32ValueToArray(dataStore.GetRootNode(), 0u, 1024));
		SEOUL_UNITTESTING_ASSERT(!dataStore.SetUInt32ValueToTable(dataStore.GetRootNode(), HString("TestBoom"), 1024));
		SEOUL_UNITTESTING_ASSERT(!dataStore.SetUInt64ValueToArray(dataStore.GetRootNode(), 0u, 1024));
		SEOUL_UNITTESTING_ASSERT(!dataStore.SetUInt64ValueToTable(dataStore.GetRootNode(), HString("TestBoom"), 1024));
		SEOUL_UNITTESTING_ASSERT(!dataStore.TableContainsKey(dataStore.GetRootNode(), HString()));
		SEOUL_UNITTESTING_ASSERT(!dataStore.SetStringToArray(dataStore.GetRootNode(), 0u, "ASDF", 4u));
		SEOUL_UNITTESTING_ASSERT(!dataStore.SetStringToTable(dataStore.GetRootNode(), HString("Whatit"), "ASDF", 4u));
	}
}

void DataStoreTest::TestBasicRobustness()
{
	DataStore dataStore;

	// Check that calling MakeArray() after populating an existing
	// array results in a new empty array.
	{
		dataStore.MakeArray();
		SEOUL_UNITTESTING_ASSERT(dataStore.SetStringToArray(dataStore.GetRootNode(), 0u, "Hello World"));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetStringToArray(dataStore.GetRootNode(), 1u, "Hi There"));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetStringToArray(dataStore.GetRootNode(), 2u, "How are you?"));

		DataNode value;
		String sString;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(dataStore.GetRootNode(), 0u, value));
		SEOUL_UNITTESTING_ASSERT(dataStore.AsString(value, sString));
		SEOUL_UNITTESTING_ASSERT_EQUAL(String("Hello World"), sString);
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(dataStore.GetRootNode(), 1u, value));
		SEOUL_UNITTESTING_ASSERT(dataStore.AsString(value, sString));
		SEOUL_UNITTESTING_ASSERT_EQUAL(String("Hi There"), sString);
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(dataStore.GetRootNode(), 2u, value));
		SEOUL_UNITTESTING_ASSERT(dataStore.AsString(value, sString));
		SEOUL_UNITTESTING_ASSERT_EQUAL(String("How are you?"), sString);

		UInt32 nCapacity = 0u;
		UInt32 nCount = 0u;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetArrayCapacity(dataStore.GetRootNode(), nCapacity));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetArrayCount(dataStore.GetRootNode(), nCount));
		SEOUL_UNITTESTING_ASSERT_EQUAL(6u, nCapacity);
		SEOUL_UNITTESTING_ASSERT_EQUAL(3u, nCount);

		dataStore.MakeArray(1u);
		SEOUL_UNITTESTING_ASSERT(dataStore.GetArrayCapacity(dataStore.GetRootNode(), nCapacity));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetArrayCount(dataStore.GetRootNode(), nCount));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1u, nCapacity);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0u, nCount);

		dataStore.MakeArray(8u);
		SEOUL_UNITTESTING_ASSERT(dataStore.GetArrayCapacity(dataStore.GetRootNode(), nCapacity));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetArrayCount(dataStore.GetRootNode(), nCount));
		SEOUL_UNITTESTING_ASSERT_EQUAL(8u, nCapacity);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0u, nCount);
	}

	// Check that calling MakeTable() after populating an existing
	// table results in a new empty table.
	{
		static const HString kKey0("Hi There0");
		static const HString kKey1("Hi There1");
		static const HString kKey2("Hi There2");

		dataStore.MakeTable();
		SEOUL_UNITTESTING_ASSERT(dataStore.SetStringToTable(dataStore.GetRootNode(), kKey0, "Hello World"));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetStringToTable(dataStore.GetRootNode(), kKey1, "Hi There"));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetStringToTable(dataStore.GetRootNode(), kKey2, "How are you?"));

		DataNode value;
		String sString;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(dataStore.GetRootNode(), kKey0, value));
		SEOUL_UNITTESTING_ASSERT(dataStore.AsString(value, sString));
		SEOUL_UNITTESTING_ASSERT_EQUAL(String("Hello World"), sString);
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(dataStore.GetRootNode(), kKey1, value));
		SEOUL_UNITTESTING_ASSERT(dataStore.AsString(value, sString));
		SEOUL_UNITTESTING_ASSERT_EQUAL(String("Hi There"), sString);
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(dataStore.GetRootNode(), kKey2, value));
		SEOUL_UNITTESTING_ASSERT(dataStore.AsString(value, sString));
		SEOUL_UNITTESTING_ASSERT_EQUAL(String("How are you?"), sString);

		UInt32 nCapacity = 0u;
		UInt32 nCount = 0u;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetTableCapacity(dataStore.GetRootNode(), nCapacity));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetTableCount(dataStore.GetRootNode(), nCount));
		SEOUL_UNITTESTING_ASSERT_EQUAL(8u, nCapacity);
		SEOUL_UNITTESTING_ASSERT_EQUAL(3u, nCount);

		dataStore.MakeTable(1u);
		SEOUL_UNITTESTING_ASSERT(dataStore.GetTableCapacity(dataStore.GetRootNode(), nCapacity));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetTableCount(dataStore.GetRootNode(), nCount));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1u, nCapacity);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0u, nCount);

		dataStore.MakeTable(8u);
		SEOUL_UNITTESTING_ASSERT(dataStore.GetTableCapacity(dataStore.GetRootNode(), nCapacity));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetTableCount(dataStore.GetRootNode(), nCount));
		SEOUL_UNITTESTING_ASSERT_EQUAL(8u, nCapacity);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0u, nCount);
	}

	// Check that a stale DataNode to a by-reference type does not succeeded.
	{
		dataStore.MakeArray();
		DataNode oldRoot = dataStore.GetRootNode();
		dataStore.MakeArray();

		// All these array operations should fail.
		UInt32 u = 124u;
		SEOUL_UNITTESTING_ASSERT(!dataStore.GetArrayCapacity(oldRoot, u));
		SEOUL_UNITTESTING_ASSERT_EQUAL(124u, u);
		SEOUL_UNITTESTING_ASSERT(!dataStore.GetArrayCount(oldRoot, u));
		SEOUL_UNITTESTING_ASSERT_EQUAL(124u, u);

		SEOUL_UNITTESTING_ASSERT(!dataStore.ArrayContains(oldRoot, HString("TestIt")));

		SEOUL_UNITTESTING_ASSERT(!dataStore.EraseValueFromArray(oldRoot, 0u));

		DataNode testValue;
		SEOUL_UNITTESTING_ASSERT(!dataStore.GetValueFromArray(oldRoot, 0u, testValue));
		SEOUL_UNITTESTING_ASSERT(testValue.IsNull());

		SEOUL_UNITTESTING_ASSERT(!dataStore.ResizeArray(oldRoot, 1024u));
		SEOUL_UNITTESTING_ASSERT(!dataStore.SetArrayToArray(oldRoot, 0u, 1024u));
		SEOUL_UNITTESTING_ASSERT(!dataStore.SetBooleanValueToArray(oldRoot, 0u, true));
		SEOUL_UNITTESTING_ASSERT(!dataStore.SetFilePathToArray(oldRoot, 0u, FilePath()));
		SEOUL_UNITTESTING_ASSERT(!dataStore.SetFloat32ValueToArray(oldRoot, 0u, 1.7f));
		SEOUL_UNITTESTING_ASSERT(!dataStore.SetInt32ValueToArray(oldRoot, 0u, 523));
		SEOUL_UNITTESTING_ASSERT(!dataStore.SetInt64ValueToArray(oldRoot, 0u, 523));
		SEOUL_UNITTESTING_ASSERT(!dataStore.SetNullValueToArray(oldRoot, 0u));
		SEOUL_UNITTESTING_ASSERT(!dataStore.SetStringToArray(oldRoot, 0u, "Test test!"));
		SEOUL_UNITTESTING_ASSERT(!dataStore.SetTableToArray(oldRoot, 0u, 1024));
		SEOUL_UNITTESTING_ASSERT(!dataStore.SetUInt32ValueToArray(oldRoot, 0u, 1024));
		SEOUL_UNITTESTING_ASSERT(!dataStore.SetUInt64ValueToArray(oldRoot, 0u, 1024));
		SEOUL_UNITTESTING_ASSERT(!dataStore.SetStringToArray(oldRoot, 0u, "ASDF", 4u));
	}

	// Compaction around a table with just an empty key.
	{
		DataStore dataStore;
		dataStore.MakeTable();

		SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToTable(dataStore.GetRootNode(), HString(), 23));

		{
			UInt32 uTableCount = 0u;
			SEOUL_UNITTESTING_ASSERT(dataStore.GetTableCount(dataStore.GetRootNode(), uTableCount));
			SEOUL_UNITTESTING_ASSERT_EQUAL(1u, uTableCount);
		}

		{
			UInt32 uIterations = 0u;
			for (auto i = dataStore.TableBegin(dataStore.GetRootNode()); dataStore.TableEnd(dataStore.GetRootNode()) != i; ++i)
			{
				SEOUL_UNITTESTING_ASSERT_EQUAL(HString(), i->First);
				SEOUL_UNITTESTING_ASSERT_EQUAL(23, i->Second.GetInt32Small());
				++uIterations;
			}

			SEOUL_UNITTESTING_ASSERT_EQUAL(1u, uIterations);
		}

		dataStore.CollectGarbageAndCompactHeap();

		{
			UInt32 uIterations = 0u;
			for (auto i = dataStore.TableBegin(dataStore.GetRootNode()); dataStore.TableEnd(dataStore.GetRootNode()) != i; ++i)
			{
				SEOUL_UNITTESTING_ASSERT_EQUAL(HString(), i->First);
				SEOUL_UNITTESTING_ASSERT_EQUAL(23, i->Second.GetInt32Small());
				++uIterations;
			}

			SEOUL_UNITTESTING_ASSERT_EQUAL(1u, uIterations);
		}
	}
}

void DataStoreTest::TestDeepCopy()
{
	static const UInt32 kTotalArrayEntries = 1023u;

	DataStore srcDataStore;

	// Populate srcDataStore.
	{
		srcDataStore.MakeArray();

		for (UInt32 i = 0u; i < kTotalArrayEntries; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(srcDataStore.SetStringToArray(srcDataStore.GetRootNode(), i, String::Printf("Test String: %u", i)));
		}

		for (UInt32 i = 0u; i < kTotalArrayEntries; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(srcDataStore.SetUInt32ValueToArray(srcDataStore.GetRootNode(), i, i));
		}

		SEOUL_UNITTESTING_ASSERT(srcDataStore.SetFloat32ValueToArray(srcDataStore.GetRootNode(), kTotalArrayEntries, 5.1f));
		SEOUL_UNITTESTING_ASSERT(srcDataStore.SetFloat32ValueToArray(srcDataStore.GetRootNode(), kTotalArrayEntries + 1, 2.5f));

		srcDataStore.CollectGarbage();
	}

	// Validate srcDataStore.
	{
		for (UInt32 i = 0u; i < kTotalArrayEntries; ++i)
		{
			UInt32 uValue = UIntMax;
			DataNode value;
			SEOUL_UNITTESTING_ASSERT(srcDataStore.GetValueFromArray(srcDataStore.GetRootNode(), i, value));
			SEOUL_UNITTESTING_ASSERT(srcDataStore.AsUInt32(value, uValue));
			SEOUL_UNITTESTING_ASSERT_EQUAL(i, uValue);
		}

		Float32 f;
		DataNode value;
		SEOUL_UNITTESTING_ASSERT(srcDataStore.GetValueFromArray(srcDataStore.GetRootNode(), kTotalArrayEntries, value));
		SEOUL_UNITTESTING_ASSERT(srcDataStore.AsFloat32(value, f));
		SEOUL_UNITTESTING_ASSERT_EQUAL(f, 5.1f);

		SEOUL_UNITTESTING_ASSERT(srcDataStore.GetValueFromArray(srcDataStore.GetRootNode(), kTotalArrayEntries + 1, value));
		SEOUL_UNITTESTING_ASSERT(srcDataStore.AsFloat32(value, f));
		SEOUL_UNITTESTING_ASSERT_EQUAL(f, 2.5f);
	}

	// Deep copy to dstDataStore
	DataStore dstDataStore;
	dstDataStore.MakeArray();
	SEOUL_UNITTESTING_ASSERT(dstDataStore.DeepCopy(srcDataStore, srcDataStore.GetRootNode(), dstDataStore.GetRootNode()));

	// Validate dstDataStore.
	{
		for (UInt32 i = 0u; i < kTotalArrayEntries; ++i)
		{
			UInt32 uValue = UIntMax;
			DataNode value;
			SEOUL_UNITTESTING_ASSERT(dstDataStore.GetValueFromArray(dstDataStore.GetRootNode(), i, value));
			SEOUL_UNITTESTING_ASSERT(dstDataStore.AsUInt32(value, uValue));
			SEOUL_UNITTESTING_ASSERT_EQUAL(i, uValue);
		}

		Float32 f;
		DataNode value;
		SEOUL_UNITTESTING_ASSERT(dstDataStore.GetValueFromArray(dstDataStore.GetRootNode(), kTotalArrayEntries, value));
		SEOUL_UNITTESTING_ASSERT(dstDataStore.AsFloat32(value, f));
		SEOUL_UNITTESTING_ASSERT_EQUAL(f, 5.1f);

		SEOUL_UNITTESTING_ASSERT(dstDataStore.GetValueFromArray(dstDataStore.GetRootNode(), kTotalArrayEntries + 1, value));
		SEOUL_UNITTESTING_ASSERT(dstDataStore.AsFloat32(value, f));
		SEOUL_UNITTESTING_ASSERT_EQUAL(f, 2.5f);
	}

	// Deep copy dstDatStore to dstDataStore
	SEOUL_UNITTESTING_ASSERT(dstDataStore.DeepCopy(dstDataStore.GetRootNode(), dstDataStore.GetRootNode(), true));

	// Validate dstDataStore.
	{
		for (UInt32 i = 0u; i < kTotalArrayEntries; ++i)
		{
			UInt32 uValue = UIntMax;
			DataNode value;
			SEOUL_UNITTESTING_ASSERT(dstDataStore.GetValueFromArray(dstDataStore.GetRootNode(), i, value));
			SEOUL_UNITTESTING_ASSERT(dstDataStore.AsUInt32(value, uValue));
			SEOUL_UNITTESTING_ASSERT_EQUAL(i, uValue);
		}

		Float32 f;
		DataNode value;
		SEOUL_UNITTESTING_ASSERT(dstDataStore.GetValueFromArray(dstDataStore.GetRootNode(), kTotalArrayEntries, value));
		SEOUL_UNITTESTING_ASSERT(dstDataStore.AsFloat32(value, f));
		SEOUL_UNITTESTING_ASSERT_EQUAL(f, 5.1f);

		SEOUL_UNITTESTING_ASSERT(dstDataStore.GetValueFromArray(dstDataStore.GetRootNode(), kTotalArrayEntries + 1, value));
		SEOUL_UNITTESTING_ASSERT(dstDataStore.AsFloat32(value, f));
		SEOUL_UNITTESTING_ASSERT_EQUAL(f, 2.5f);
	}
}

void DataStoreTest::TestDeepCopyTable()
{
	static const UInt32 kTotalEntries = 1023u;

	DataStore srcDataStore;

	// Populate srcDataStore.
	{
		srcDataStore.MakeTable();

		for (UInt32 i = 0u; i < kTotalEntries; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(srcDataStore.SetStringToTable(srcDataStore.GetRootNode(), HString(String::Printf("%u", i)), String::Printf("Test String: %u", i)));
		}

		for (UInt32 i = 0u; i < kTotalEntries; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(srcDataStore.SetUInt32ValueToTable(srcDataStore.GetRootNode(), HString(String::Printf("%u", i)), i));
		}

		SEOUL_UNITTESTING_ASSERT(srcDataStore.SetFloat32ValueToTable(srcDataStore.GetRootNode(), HString(String::Printf("%u", kTotalEntries)), 5.1f));
		SEOUL_UNITTESTING_ASSERT(srcDataStore.SetFloat32ValueToTable(srcDataStore.GetRootNode(), HString(String::Printf("%u", kTotalEntries + 1)), 2.5f));

		srcDataStore.CollectGarbage();
	}

	// Validate srcDataStore.
	{
		for (UInt32 i = 0u; i < kTotalEntries; ++i)
		{
			UInt32 uValue = UIntMax;
			DataNode value;
			SEOUL_UNITTESTING_ASSERT(srcDataStore.GetValueFromTable(srcDataStore.GetRootNode(), HString(String::Printf("%u", i)), value));
			SEOUL_UNITTESTING_ASSERT(srcDataStore.AsUInt32(value, uValue));
			SEOUL_UNITTESTING_ASSERT_EQUAL(i, uValue);
		}

		Float32 f;
		DataNode value;
		SEOUL_UNITTESTING_ASSERT(srcDataStore.GetValueFromTable(srcDataStore.GetRootNode(), HString(String::Printf("%u", kTotalEntries)), value));
		SEOUL_UNITTESTING_ASSERT(srcDataStore.AsFloat32(value, f));
		SEOUL_UNITTESTING_ASSERT_EQUAL(f, 5.1f);

		SEOUL_UNITTESTING_ASSERT(srcDataStore.GetValueFromTable(srcDataStore.GetRootNode(), HString(String::Printf("%u", kTotalEntries + 1)), value));
		SEOUL_UNITTESTING_ASSERT(srcDataStore.AsFloat32(value, f));
		SEOUL_UNITTESTING_ASSERT_EQUAL(f, 2.5f);
	}

	// Deep copy to dstDataStore
	DataStore dstDataStore;
	dstDataStore.MakeTable();
	SEOUL_UNITTESTING_ASSERT(dstDataStore.DeepCopy(srcDataStore, srcDataStore.GetRootNode(), dstDataStore.GetRootNode()));

	// Validate dstDataStore.
	{
		for (UInt32 i = 0u; i < kTotalEntries; ++i)
		{
			UInt32 uValue = UIntMax;
			DataNode value;
			SEOUL_UNITTESTING_ASSERT(dstDataStore.GetValueFromTable(dstDataStore.GetRootNode(), HString(String::Printf("%u", i)), value));
			SEOUL_UNITTESTING_ASSERT(dstDataStore.AsUInt32(value, uValue));
			SEOUL_UNITTESTING_ASSERT_EQUAL(i, uValue);
		}

		Float32 f;
		DataNode value;
		SEOUL_UNITTESTING_ASSERT(dstDataStore.GetValueFromTable(dstDataStore.GetRootNode(), HString(String::Printf("%u", kTotalEntries)), value));
		SEOUL_UNITTESTING_ASSERT(dstDataStore.AsFloat32(value, f));
		SEOUL_UNITTESTING_ASSERT_EQUAL(f, 5.1f);

		SEOUL_UNITTESTING_ASSERT(dstDataStore.GetValueFromTable(dstDataStore.GetRootNode(), HString(String::Printf("%u", kTotalEntries + 1)), value));
		SEOUL_UNITTESTING_ASSERT(dstDataStore.AsFloat32(value, f));
		SEOUL_UNITTESTING_ASSERT_EQUAL(f, 2.5f);
	}

	// Deep copy dstDatStore to dstDataStore
	SEOUL_UNITTESTING_ASSERT(dstDataStore.DeepCopy(dstDataStore.GetRootNode(), dstDataStore.GetRootNode(), true));

	// Validate dstDataStore.
	{
		for (UInt32 i = 0u; i < kTotalEntries; ++i)
		{
			UInt32 uValue = UIntMax;
			DataNode value;
			SEOUL_UNITTESTING_ASSERT(dstDataStore.GetValueFromTable(dstDataStore.GetRootNode(), HString(String::Printf("%u", i)), value));
			SEOUL_UNITTESTING_ASSERT(dstDataStore.AsUInt32(value, uValue));
			SEOUL_UNITTESTING_ASSERT_EQUAL(i, uValue);
		}

		Float32 f;
		DataNode value;
		SEOUL_UNITTESTING_ASSERT(dstDataStore.GetValueFromTable(dstDataStore.GetRootNode(), HString(String::Printf("%u", kTotalEntries)), value));
		SEOUL_UNITTESTING_ASSERT(dstDataStore.AsFloat32(value, f));
		SEOUL_UNITTESTING_ASSERT_EQUAL(f, 5.1f);

		SEOUL_UNITTESTING_ASSERT(dstDataStore.GetValueFromTable(dstDataStore.GetRootNode(), HString(String::Printf("%u", kTotalEntries + 1)), value));
		SEOUL_UNITTESTING_ASSERT(dstDataStore.AsFloat32(value, f));
		SEOUL_UNITTESTING_ASSERT_EQUAL(f, 2.5f);
	}
}

void DataStoreTest::TestGarbageCollection()
{
	DataStore dataStore;

	static const UInt32 kTotalArrayEntries = 1022u;
	static const UInt32 kTotalTableEntries = 1000u;

	// Total space is 4 bytes for the root node, 8 bytes for the container header,
	// and then 1022u 4 byte DataNode entries (1022u is explicitly specified,
	// since DataStore grows in powers of 2, including space for the container
	// header).
	static const UInt32 kHeapSizeInBytesUsingArray = 4u + 8u + (1022u * 4u);

	// Total space is 4 bytes for the root node, 8 bytes for the container header,
	// 2048u 4 byte DataNode entries, then 2048u 4 byte HString entries, and then another 1000u 64-bit UInt entries.
	//
	// NOTE: The 2048u comes from a load factor of 0.8 on the table.
	static const UInt32 kHeapSizeInBytesUsingTable = 4u + 8u + (2048u * 4u) + (2048u * 4u) + (1000u * 8u);

	// Test with root as an array
	{
		dataStore.MakeArray();

		for (UInt32 i = 0u; i < kTotalArrayEntries; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(dataStore.SetStringToArray(dataStore.GetRootNode(), i, String::Printf("Test String: %u", i)));
		}

		for (UInt32 i = 0u; i < kTotalArrayEntries; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(dataStore.SetUInt32ValueToArray(dataStore.GetRootNode(), i, i));
		}

		dataStore.CollectGarbage();

		for (UInt32 i = 0u; i < kTotalArrayEntries; ++i)
		{
			UInt32 uValue = UIntMax;
			DataNode value;
			SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(dataStore.GetRootNode(), i, value));
			SEOUL_UNITTESTING_ASSERT(dataStore.AsUInt32(value, uValue));
			SEOUL_UNITTESTING_ASSERT_EQUAL(i, uValue);
		}

		SEOUL_UNITTESTING_ASSERT_EQUAL(kHeapSizeInBytesUsingArray, dataStore.GetInUseHeapSizeInBytes());

		dataStore.CompactHeap();

		for (UInt32 i = 0u; i < kTotalArrayEntries; ++i)
		{
			UInt32 uValue = UIntMax;
			DataNode value;
			SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(dataStore.GetRootNode(), i, value));
			SEOUL_UNITTESTING_ASSERT(dataStore.AsUInt32(value, uValue));
			SEOUL_UNITTESTING_ASSERT_EQUAL(i, uValue);
		}

		SEOUL_UNITTESTING_ASSERT_EQUAL(kHeapSizeInBytesUsingArray, dataStore.GetHeapCapacityInBytes());
		SEOUL_UNITTESTING_ASSERT_EQUAL(kHeapSizeInBytesUsingArray, dataStore.GetInUseHeapSizeInBytes());
	}

	// Test with root as a table
	{
		dataStore.MakeTable();

		for (UInt32 i = 0u; i < kTotalTableEntries; ++i)
		{
			HString key(String::Printf("key%u", i));
			SEOUL_UNITTESTING_ASSERT(dataStore.SetStringToTable(dataStore.GetRootNode(), key, String::Printf("Test String: %u", i)));
		}

		for (UInt32 i = 0u; i < kTotalTableEntries; ++i)
		{
			HString key(String::Printf("key%u", i));
			UInt64 uValue = (UInt64)Int64Max + i;
			SEOUL_UNITTESTING_ASSERT(dataStore.SetUInt64ValueToTable(dataStore.GetRootNode(), key, uValue));
		}

		dataStore.CollectGarbage();

		for (UInt32 i = 0u; i < kTotalTableEntries; ++i)
		{
			HString key(String::Printf("key%u", i));
			UInt64 const uTestValue = (UInt64)Int64Max + i;
			UInt64 uValue = UIntMax;
			DataNode value;
			SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(dataStore.GetRootNode(), key, value));
			SEOUL_UNITTESTING_ASSERT(dataStore.AsUInt64(value, uValue));
			SEOUL_UNITTESTING_ASSERT_EQUAL(uTestValue, uValue);
		}

		SEOUL_UNITTESTING_ASSERT_EQUAL(kHeapSizeInBytesUsingTable, dataStore.GetInUseHeapSizeInBytes());

		dataStore.CompactHeap();

		for (UInt32 i = 0u; i < kTotalTableEntries; ++i)
		{
			HString key(String::Printf("key%u", i));
			UInt64 const uTestValue = (UInt64)Int64Max + i;
			UInt64 uValue = UIntMax;
			DataNode value;
			SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(dataStore.GetRootNode(), key, value));
			SEOUL_UNITTESTING_ASSERT(dataStore.AsUInt64(value, uValue));
			SEOUL_UNITTESTING_ASSERT_EQUAL(uTestValue, uValue);
		}

		SEOUL_UNITTESTING_ASSERT_EQUAL(kHeapSizeInBytesUsingTable, dataStore.GetHeapCapacityInBytes());
		SEOUL_UNITTESTING_ASSERT_EQUAL(kHeapSizeInBytesUsingTable, dataStore.GetInUseHeapSizeInBytes());
	}
}

void DataStoreTest::TestRootArray()
{
	DataStore dataStore(1024);
	dataStore.MakeArray(8);

	SEOUL_UNITTESTING_ASSERT(dataStore.GetRootNode().IsArray());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1024u, dataStore.GetHeapCapacityInBytes());

	// 8 bytes for root node, 8 bytes for array header, and then 4 bytes for 8 nodes
	// worth of capacity in the root array.
	SEOUL_UNITTESTING_ASSERT_EQUAL((4u + 8u + (4u * 8u)), dataStore.GetInUseHeapSizeInBytes());

	DataNode root = dataStore.GetRootNode();

	DataNode value;
	SEOUL_UNITTESTING_ASSERT(value.IsNull());

	Float fValue = 1.1f;
	SEOUL_UNITTESTING_ASSERT(dataStore.SetFloat32ValueToArray(root, 0u, 1.3f));
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
	SEOUL_UNITTESTING_ASSERT(value.IsFloat31());
	SEOUL_UNITTESTING_ASSERT(dataStore.AsFloat32(value, fValue));
	SEOUL_UNITTESTING_ASSERT_EQUAL(1.3f, fValue);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1.3f, dataStore.AssumeFloat31(value));
}

void DataStoreTest::TestRootTable()
{
	DataStore dataStore(1024);
	dataStore.MakeTable(8);

	SEOUL_UNITTESTING_ASSERT(dataStore.GetRootNode().IsTable());
	SEOUL_UNITTESTING_ASSERT(dataStore.GetHeapCapacityInBytes() == 1024u);

	// 4 bytes for root node, 8 bytes for table header, 4 bytes for 8 nodes
	// worth of capacity in the value portion of the table, 4 bytes for 8 HString
	// keys in the key portion of the table - note that, to maintain DataNode multiples,
	// the storage used for the key area is always rounded up to mulitples of 4 bytes.
	SEOUL_UNITTESTING_ASSERT(dataStore.GetInUseHeapSizeInBytes() == (4u + 8u + (4u * 8u) + (4u * 8u)));

	DataNode root = dataStore.GetRootNode();

	DataNode value;
	SEOUL_UNITTESTING_ASSERT(value.IsNull());

	static const HString kKey("Hello_World");
	Float fValue = 1.1f;
	SEOUL_UNITTESTING_ASSERT(dataStore.SetFloat32ValueToTable(root, kKey, 1.3f));
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, kKey, value));
	SEOUL_UNITTESTING_ASSERT(value.IsFloat31());
	SEOUL_UNITTESTING_ASSERT(dataStore.AsFloat32(value, fValue));
	SEOUL_UNITTESTING_ASSERT_EQUAL(1.3f, fValue);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1.3f, dataStore.AssumeFloat31(value));

	static const HString kEmptyKey;
	SEOUL_UNITTESTING_ASSERT(dataStore.SetFloat32ValueToTable(root, kEmptyKey, 5.3f));
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, kEmptyKey, value));
	SEOUL_UNITTESTING_ASSERT(value.IsFloat31());
	SEOUL_UNITTESTING_ASSERT(dataStore.AsFloat32(value, fValue));
	SEOUL_UNITTESTING_ASSERT_EQUAL(5.3f, fValue);
	SEOUL_UNITTESTING_ASSERT_EQUAL(5.3f, dataStore.AssumeFloat31(value));
}

void DataStoreTest::TestArrayErase()
{
	DataStore dataStore;
	dataStore.MakeArray();

	DataNode root = dataStore.GetRootNode();

	// populate the array
	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(root, 0u, 9));
	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(root, 1u, 8));
	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(root, 2u, 7));
	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(root, 3u, 6));
	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(root, 4u, 5));
	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(root, 5u, 4));
	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(root, 6u, 3));
	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(root, 7u, 2));
	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(root, 8u, 1));
	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(root, 9u, 0));

	// make sure it counted
	UInt32 uArrayCount = 0u;
	SEOUL_UNITTESTING_ASSERT(dataStore.GetArrayCount(root, uArrayCount));
	SEOUL_UNITTESTING_ASSERT_EQUAL(10u, uArrayCount);

	// remove an entry
	SEOUL_UNITTESTING_ASSERT(dataStore.EraseValueFromArray(root, 4u));

	// make sure removal worked
	SEOUL_UNITTESTING_ASSERT(dataStore.GetArrayCount(root, uArrayCount));
	SEOUL_UNITTESTING_ASSERT_EQUAL(9u, uArrayCount);

	DataNode value;
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
	SEOUL_UNITTESTING_ASSERT(value.IsInt32Small());
	SEOUL_UNITTESTING_ASSERT_EQUAL(9, dataStore.AssumeInt32Small(value));

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 1u, value));
	SEOUL_UNITTESTING_ASSERT(value.IsInt32Small());
	SEOUL_UNITTESTING_ASSERT_EQUAL(8, dataStore.AssumeInt32Small(value));

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 2u, value));
	SEOUL_UNITTESTING_ASSERT(value.IsInt32Small());
	SEOUL_UNITTESTING_ASSERT_EQUAL(7, dataStore.AssumeInt32Small(value));

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 3u, value));
	SEOUL_UNITTESTING_ASSERT(value.IsInt32Small());
	SEOUL_UNITTESTING_ASSERT_EQUAL(6, dataStore.AssumeInt32Small(value));

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 4u, value));
	SEOUL_UNITTESTING_ASSERT(value.IsInt32Small());
	SEOUL_UNITTESTING_ASSERT_EQUAL(4, dataStore.AssumeInt32Small(value));

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 5u, value));
	SEOUL_UNITTESTING_ASSERT(value.IsInt32Small());
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, dataStore.AssumeInt32Small(value));

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 6u, value));
	SEOUL_UNITTESTING_ASSERT(value.IsInt32Small());
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, dataStore.AssumeInt32Small(value));

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 7u, value));
	SEOUL_UNITTESTING_ASSERT(value.IsInt32Small());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, dataStore.AssumeInt32Small(value));

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 8u, value));
	SEOUL_UNITTESTING_ASSERT(value.IsInt32Small());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, dataStore.AssumeInt32Small(value));

	// add and remove entries
	SEOUL_UNITTESTING_ASSERT(dataStore.EraseValueFromArray(root, 8u));
	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(root, 7u, 10));
	SEOUL_UNITTESTING_ASSERT(dataStore.EraseValueFromArray(root, 0u));
	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(root, 0u, 0));
	SEOUL_UNITTESTING_ASSERT(dataStore.EraseValueFromArray(root, 5u));
	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(root, 4u, 6));

	// Check that erase fails.
	SEOUL_UNITTESTING_ASSERT(!dataStore.EraseValueFromArray(root, 100u));
	SEOUL_UNITTESTING_ASSERT(!dataStore.EraseValueFromArray(root, 6u));
	SEOUL_UNITTESTING_ASSERT(!dataStore.EraseValueFromArray(root, 30u));

	// make sure removal worked
	SEOUL_UNITTESTING_ASSERT(dataStore.GetArrayCount(root, uArrayCount));
	SEOUL_UNITTESTING_ASSERT_EQUAL(6u, uArrayCount);

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
	SEOUL_UNITTESTING_ASSERT(value.IsInt32Small());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, dataStore.AssumeInt32Small(value));

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 1u, value));
	SEOUL_UNITTESTING_ASSERT(value.IsInt32Small());
	SEOUL_UNITTESTING_ASSERT_EQUAL(7, dataStore.AssumeInt32Small(value));

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 2u, value));
	SEOUL_UNITTESTING_ASSERT(value.IsInt32Small());
	SEOUL_UNITTESTING_ASSERT_EQUAL(6, dataStore.AssumeInt32Small(value));

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 3u, value));
	SEOUL_UNITTESTING_ASSERT(value.IsInt32Small());
	SEOUL_UNITTESTING_ASSERT_EQUAL(4, dataStore.AssumeInt32Small(value));

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 4u, value));
	SEOUL_UNITTESTING_ASSERT(value.IsInt32Small());
	SEOUL_UNITTESTING_ASSERT_EQUAL(6, dataStore.AssumeInt32Small(value));

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 5u, value));
	SEOUL_UNITTESTING_ASSERT(value.IsInt32Small());
	SEOUL_UNITTESTING_ASSERT_EQUAL(10, dataStore.AssumeInt32Small(value));

	// remove remaining entries
	SEOUL_UNITTESTING_ASSERT(dataStore.EraseValueFromArray(root, 5u));
	SEOUL_UNITTESTING_ASSERT(dataStore.EraseValueFromArray(root, 3u));
	SEOUL_UNITTESTING_ASSERT(dataStore.EraseValueFromArray(root, 3u));
	SEOUL_UNITTESTING_ASSERT(dataStore.EraseValueFromArray(root, 1u));
	SEOUL_UNITTESTING_ASSERT(dataStore.EraseValueFromArray(root, 0u));
	SEOUL_UNITTESTING_ASSERT(dataStore.EraseValueFromArray(root, 0u));

	// Check that erase fails.
	SEOUL_UNITTESTING_ASSERT(!dataStore.EraseValueFromArray(root, 0u));

	// Check size
	SEOUL_UNITTESTING_ASSERT(dataStore.GetArrayCount(root, uArrayCount));
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, uArrayCount);
}

void DataStoreTest::TestTableErase()
{
	DataStore dataStore;
	dataStore.MakeTable();

	DataNode root = dataStore.GetRootNode();

	// populate the table
	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToTable(root, HString("one"), 1));
	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToTable(root, HString("two"), 2));
	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToTable(root, HString("three"), 3));
	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToTable(root, HString("tremendous"), 4));
	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToTable(root, HString("terrific"), 5));
	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToTable(root, HString("toofreakinawesome"), 6));
	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToTable(root, HString("four"), 7));
	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToTable(root, HString("five"), 8));
	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToTable(root, HString("six"), 9));
	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToTable(root, HString("seven"), 10));

	// make sure it counted
	UInt32 nTableCount = 0u;
	SEOUL_UNITTESTING_ASSERT(dataStore.GetTableCount(root, nTableCount));
	SEOUL_UNITTESTING_ASSERT_EQUAL(10u, nTableCount);

	// remove an entry
	SEOUL_UNITTESTING_ASSERT(dataStore.EraseValueFromTable(root, HString("one")));

	// make sure removal worked
	SEOUL_UNITTESTING_ASSERT(dataStore.GetTableCount(root, nTableCount));
	SEOUL_UNITTESTING_ASSERT_EQUAL(9u, nTableCount);

	DataNode value;
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, HString("two"), value));
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, HString("three"), value));
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, HString("tremendous"), value));
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, HString("terrific"), value));
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, HString("toofreakinawesome"), value));
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, HString("four"), value));
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, HString("five"), value));
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, HString("six"), value));
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, HString("seven"), value));

	SEOUL_UNITTESTING_ASSERT(dataStore.EraseValueFromTable(root, HString("two")));
	SEOUL_UNITTESTING_ASSERT(dataStore.EraseValueFromTable(root, HString("seven")));

	SEOUL_UNITTESTING_ASSERT(dataStore.GetTableCount(root, nTableCount));
	SEOUL_UNITTESTING_ASSERT_EQUAL(7u, nTableCount);

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, HString("three"), value));
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, HString("tremendous"), value));
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, HString("terrific"), value));
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, HString("toofreakinawesome"), value));
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, HString("four"), value));
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, HString("five"), value));
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, HString("six"), value));

	SEOUL_UNITTESTING_ASSERT(dataStore.EraseValueFromTable(root, HString("three")));
	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToTable(root, HString("one"), 11));
	SEOUL_UNITTESTING_ASSERT(dataStore.EraseValueFromTable(root, HString("six")));

	SEOUL_UNITTESTING_ASSERT(dataStore.GetTableCount(root, nTableCount));
	SEOUL_UNITTESTING_ASSERT_EQUAL(6u, nTableCount);

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, HString("one"), value));
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, HString("tremendous"), value));
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, HString("terrific"), value));
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, HString("toofreakinawesome"), value));
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, HString("four"), value));
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, HString("five"), value));

	SEOUL_UNITTESTING_ASSERT(dataStore.EraseValueFromTable(root, HString("five")));
	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToTable(root, HString("two"), 12));
	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToTable(root, HString("three"), 13));
	SEOUL_UNITTESTING_ASSERT(dataStore.EraseValueFromTable(root, HString("tremendous")));

	SEOUL_UNITTESTING_ASSERT(dataStore.GetTableCount(root, nTableCount));
	SEOUL_UNITTESTING_ASSERT_EQUAL(6u, nTableCount);

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, HString("one"), value));
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, HString("two"), value));
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, HString("three"), value));
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, HString("terrific"), value));
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, HString("toofreakinawesome"), value));
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, HString("four"), value));

	SEOUL_UNITTESTING_ASSERT(dataStore.EraseValueFromTable(root, HString("one")));
	SEOUL_UNITTESTING_ASSERT(dataStore.EraseValueFromTable(root, HString("two")));
	SEOUL_UNITTESTING_ASSERT(dataStore.EraseValueFromTable(root, HString("three")));

	SEOUL_UNITTESTING_ASSERT(dataStore.GetTableCount(root, nTableCount));
	SEOUL_UNITTESTING_ASSERT_EQUAL(3u, nTableCount);

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, HString("terrific"), value));
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, HString("toofreakinawesome"), value));
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, HString("four"), value));

	SEOUL_UNITTESTING_ASSERT(dataStore.EraseValueFromTable(root, HString("terrific")));
	SEOUL_UNITTESTING_ASSERT(dataStore.EraseValueFromTable(root, HString("toofreakinawesome")));
	SEOUL_UNITTESTING_ASSERT(dataStore.EraseValueFromTable(root, HString("four")));

	SEOUL_UNITTESTING_ASSERT(dataStore.GetTableCount(root, nTableCount));
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, nTableCount);
}

void DataStoreTest::TestTableNullKey()
{
	DataStore dataStore;
	dataStore.MakeTable();
	DataNode root = dataStore.GetRootNode();
	DataNode value;
	UInt32 nCount = 0u;

	// Multiple passes to test integrity after filling the table and removing all entries.
	for (Int iPass = 0; iPass < 8; ++iPass)
	{
		{
			SEOUL_UNITTESTING_ASSERT(!dataStore.EraseValueFromTable(root, HString("1")));
			SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToTable(root, HString("1"), 1));
			SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, HString("1"), value));
			SEOUL_UNITTESTING_ASSERT(!dataStore.GetValueFromTable(root, HString("0"), value));
			SEOUL_UNITTESTING_ASSERT(!dataStore.GetValueFromTable(root, HString("2"), value));
			SEOUL_UNITTESTING_ASSERT(!dataStore.GetValueFromTable(root, HString("3"), value));
			SEOUL_UNITTESTING_ASSERT(dataStore.GetTableCount(root, nCount));
			SEOUL_UNITTESTING_ASSERT_EQUAL(1u, nCount);
			SEOUL_UNITTESTING_ASSERT(!dataStore.EraseValueFromTable(root, HString("2")));
			SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToTable(root, HString("2"), 2));
			SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, HString("2"), value));
			SEOUL_UNITTESTING_ASSERT(!dataStore.GetValueFromTable(root, HString("0"), value));
			SEOUL_UNITTESTING_ASSERT(!dataStore.GetValueFromTable(root, HString("3"), value));
			SEOUL_UNITTESTING_ASSERT(dataStore.GetTableCount(root, nCount));
			SEOUL_UNITTESTING_ASSERT_EQUAL(2u, nCount);
			SEOUL_UNITTESTING_ASSERT(!dataStore.EraseValueFromTable(root, HString("0")));
			SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToTable(root, HString("0"), 213));
			SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, HString("0"), value));
			SEOUL_UNITTESTING_ASSERT(!dataStore.GetValueFromTable(root, HString("3"), value));
			// nullptr key overwrite testing.
			SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToTable(root, HString("0"), 237));
			SEOUL_UNITTESTING_ASSERT(dataStore.GetTableCount(root, nCount));
			SEOUL_UNITTESTING_ASSERT_EQUAL(3u, nCount);
			SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, HString("0"), value));
			SEOUL_UNITTESTING_ASSERT(!dataStore.EraseValueFromTable(root, HString("3")));
			SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToTable(root, HString("3"), 3));
			SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, HString("3"), value));
			SEOUL_UNITTESTING_ASSERT(dataStore.GetTableCount(root, nCount));
			SEOUL_UNITTESTING_ASSERT_EQUAL(4u, nCount);

			// shouldn't be empty
			SEOUL_UNITTESTING_ASSERT(dataStore.GetTableCount(root, nCount));
			SEOUL_UNITTESTING_ASSERT_EQUAL(4u, nCount);

			// verify iteration behaves as expected with a nullptr key present.
			{
				Bool abSeen[4] = { false };
				Int iIterationCount = 0;
				auto const iBegin = dataStore.TableBegin(root);
				auto const iEnd = dataStore.TableEnd(root);
				for (auto i = iBegin; iEnd != i; ++i)
				{
					Int32 iKeyValue = 277;
					SEOUL_UNITTESTING_ASSERT(i->First.ToInt32(iKeyValue));
					SEOUL_UNITTESTING_ASSERT_LESS_THAN(iKeyValue, 4);
					Int32 iValueValue = -1;
					SEOUL_UNITTESTING_ASSERT(dataStore.AsInt32(i->Second, iValueValue));
					if (0 != iKeyValue)
					{
						SEOUL_UNITTESTING_ASSERT_EQUAL(iKeyValue, iValueValue);
					}
					else
					{
						SEOUL_UNITTESTING_ASSERT_EQUAL(237, iValueValue);
					}
					SEOUL_UNITTESTING_ASSERT(!abSeen[iKeyValue]);
					abSeen[iKeyValue] = true;
					++iIterationCount;
				}
				SEOUL_UNITTESTING_ASSERT_EQUAL(4, iIterationCount);
			}

			// erase and reinsert to verify integrity.
			SEOUL_UNITTESTING_ASSERT(dataStore.EraseValueFromTable(root, HString("2")));
			SEOUL_UNITTESTING_ASSERT(!dataStore.GetValueFromTable(root, HString("2"), value));
			SEOUL_UNITTESTING_ASSERT(dataStore.GetTableCount(root, nCount));
			SEOUL_UNITTESTING_ASSERT_EQUAL(3, nCount);
			SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(dataStore.TableBegin(root),dataStore.TableEnd(root));
			SEOUL_UNITTESTING_ASSERT(dataStore.EraseValueFromTable(root, HString("0")));
			SEOUL_UNITTESTING_ASSERT(!dataStore.GetValueFromTable(root, HString("0"), value));
			SEOUL_UNITTESTING_ASSERT(dataStore.GetTableCount(root, nCount));
			SEOUL_UNITTESTING_ASSERT_EQUAL(2, nCount);
			SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(dataStore.TableBegin(root),dataStore.TableEnd(root));
			SEOUL_UNITTESTING_ASSERT(dataStore.EraseValueFromTable(root, HString("3")));
			SEOUL_UNITTESTING_ASSERT(!dataStore.GetValueFromTable(root, HString("3"), value));
			SEOUL_UNITTESTING_ASSERT(dataStore.GetTableCount(root, nCount));
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, nCount);
			SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(dataStore.TableBegin(root),dataStore.TableEnd(root));
			// Attempt a reinsert of nullptr now.
			SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToTable(root, HString("0"), 819));
			SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, HString("0"), value));
			SEOUL_UNITTESTING_ASSERT(dataStore.GetTableCount(root, nCount));
			SEOUL_UNITTESTING_ASSERT_EQUAL(2, nCount);
			SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(dataStore.TableBegin(root),dataStore.TableEnd(root));
			SEOUL_UNITTESTING_ASSERT(dataStore.EraseValueFromTable(root, HString("1")));
			SEOUL_UNITTESTING_ASSERT(!dataStore.GetValueFromTable(root, HString("1"), value));
			SEOUL_UNITTESTING_ASSERT(dataStore.GetTableCount(root, nCount));
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, nCount);
			SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(dataStore.TableBegin(root),dataStore.TableEnd(root));
			// Sanity check that we can get nullptr when it's the last element.
			SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, HString("0"), value));
			SEOUL_UNITTESTING_ASSERT(dataStore.EraseValueFromTable(root, HString("0")));
			SEOUL_UNITTESTING_ASSERT(!dataStore.GetValueFromTable(root, HString("0"), value));
			SEOUL_UNITTESTING_ASSERT(dataStore.GetTableCount(root, nCount));
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, nCount);
			SEOUL_UNITTESTING_ASSERT_EQUAL(dataStore.TableBegin(root),dataStore.TableEnd(root));
		}

		// Add a big bucket of elements, then clear to stress test.
		for (Int32 iPadding = 0; iPadding < (iPass + 1) * 4; ++iPadding)
		{
			SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToTable(root, HString(String::Printf("%d", iPadding)), iPadding));
		}

		SEOUL_UNITTESTING_ASSERT(dataStore.GetTableCount(root, nCount));
		SEOUL_UNITTESTING_ASSERT_EQUAL((UInt32)((iPass + 1) * 4), nCount);

		{
			DataStore dataStoreTemp;
			dataStoreTemp.CopyFrom(dataStore);

			DataStore emptyDataStore;
			dataStore.Swap(emptyDataStore);

			dataStore.CopyFrom(dataStoreTemp);
			root = dataStore.GetRootNode();
		}

		// Every other pass, either clear or erase manually.
		if (0 == (iPass % 2))
		{
			dataStore.MakeTable();
			root = dataStore.GetRootNode();
		}
		else
		{
			for (Int32 iPadding = ((iPass + 1) * 4) - 1; iPadding >= 0; --iPadding)
			{
				SEOUL_UNITTESTING_ASSERT(dataStore.EraseValueFromTable(root, HString(String::Printf("%d", iPadding))));
			}
		}

		SEOUL_UNITTESTING_ASSERT(dataStore.GetTableCount(root, nCount));
		SEOUL_UNITTESTING_ASSERT_EQUAL(0u, nCount);
	}

	// Final count check.
	SEOUL_UNITTESTING_ASSERT(dataStore.GetTableCount(root, nCount));
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, nCount);
}

void DataStoreTest::TestNumbersInArray()
{
	DataStore dataStore;
	dataStore.MakeArray();

	DataNode root = dataStore.GetRootNode();

	DataNode value;

	// Int32 test - only one case (value is set and stored as Int32).
	{
		Int32 iValue = 103;
		SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(root, 0u, 105));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(value.IsInt32Small());
		SEOUL_UNITTESTING_ASSERT(dataStore.AsInt32(value, iValue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(105, iValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL(105, dataStore.AssumeInt32Small(value));
	}

	// UInt32 test - 2 cases, depending on value, can be set and stored as an Int32 or UInt32.
	{
		// Stored as Int32 case.
		UInt32 uValue = 102u;
		SEOUL_UNITTESTING_ASSERT(dataStore.SetUInt32ValueToArray(root, 0u, 107u));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(value.IsInt32Small());
		SEOUL_UNITTESTING_ASSERT(dataStore.AsUInt32(value, uValue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(107u, uValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL(107u, (UInt32)dataStore.AssumeInt32Small(value));

		// Stored as UInt32 case.
		uValue = 102u;
		SEOUL_UNITTESTING_ASSERT(dataStore.SetUInt32ValueToArray(root, 0u, UIntMax));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(value.IsUInt32());
		SEOUL_UNITTESTING_ASSERT(dataStore.AsUInt32(value, uValue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(UIntMax, uValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL(UIntMax, dataStore.AssumeUInt32(value));
	}

	// Float32 test - 3 cases, depending on value, can be set and stored as an Int32, Float32, or Float31.
	{
		// Stored as Float31 case.
		Float32 fValue = 105.0f;
		SEOUL_UNITTESTING_ASSERT(dataStore.SetFloat32ValueToArray(root, 0u, 1.5f));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(value.IsFloat31());
		SEOUL_UNITTESTING_ASSERT(dataStore.AsFloat32(value, fValue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1.5f, fValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1.5f, dataStore.AssumeFloat31(value));

		// Stored as Float32 case.
		fValue = 101.0f;
		SEOUL_UNITTESTING_ASSERT(dataStore.SetFloat32ValueToArray(root, 0u, 1.666f));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(value.IsFloat32());
		SEOUL_UNITTESTING_ASSERT(dataStore.AsFloat32(value, fValue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1.666f, fValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1.666f, dataStore.AssumeFloat32(value));

		// Stored as Int32 case.
		fValue = 101.0f;
		SEOUL_UNITTESTING_ASSERT(dataStore.SetFloat32ValueToArray(root, 0u, 1.0f));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(value.IsInt32Small());
		SEOUL_UNITTESTING_ASSERT(dataStore.AsFloat32(value, fValue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, fValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, dataStore.AssumeInt32Small(value));
	}

	// Int64 test - 3 cases, can be stored as an Int32, UInt32, or Int64 depending on value.
	{
		// Stored as Int32 case.
		Int64 iValue = 99;
		SEOUL_UNITTESTING_ASSERT(dataStore.SetInt64ValueToArray(root, 0u, 107));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(value.IsInt32Small());
		SEOUL_UNITTESTING_ASSERT(dataStore.AsInt64(value, iValue));
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int64)107, iValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL(107, dataStore.AssumeInt32Small(value));

		// Stored as UInt32 case.
		iValue = 99;
		SEOUL_UNITTESTING_ASSERT(dataStore.SetInt64ValueToArray(root, 0u, (Int64)UIntMax));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(value.IsUInt32());
		SEOUL_UNITTESTING_ASSERT(dataStore.AsInt64(value, iValue));
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int64)UIntMax, iValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL(UIntMax, dataStore.AssumeUInt32(value));

		// Stored as Int64 case.
		iValue = 99;
		SEOUL_UNITTESTING_ASSERT(dataStore.SetInt64ValueToArray(root, 0u, Int64Max));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(value.IsInt64());
		SEOUL_UNITTESTING_ASSERT(dataStore.AsInt64(value, iValue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(Int64Max, iValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL(Int64Max, dataStore.AssumeInt64(value));
	}

	// UInt64 test - 4 cases, can be stored as an Int32, UInt32, Int64, or UInt64 depending on value.
	{
		// Stored as Int32 case.
		UInt64 uValue = 73;
		SEOUL_UNITTESTING_ASSERT(dataStore.SetUInt64ValueToArray(root, 0u, 107));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(value.IsInt32Small());
		SEOUL_UNITTESTING_ASSERT(dataStore.AsUInt64(value, uValue));
		SEOUL_UNITTESTING_ASSERT_EQUAL((UInt64)107, uValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL(107, dataStore.AssumeInt32Small(value));

		// Stored as UInt32 case.
		uValue = 73;
		SEOUL_UNITTESTING_ASSERT(dataStore.SetUInt64ValueToArray(root, 0u, (Int64)UIntMax));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(value.IsUInt32());
		SEOUL_UNITTESTING_ASSERT(dataStore.AsUInt64(value, uValue));
		SEOUL_UNITTESTING_ASSERT_EQUAL((UInt64)UIntMax, uValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL(UIntMax, dataStore.AssumeUInt32(value));

		// Stored as Int64 case.
		uValue = 73;
		SEOUL_UNITTESTING_ASSERT(dataStore.SetUInt64ValueToArray(root, 0u, Int64Max));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(value.IsInt64());
		SEOUL_UNITTESTING_ASSERT(dataStore.AsUInt64(value, uValue));
		SEOUL_UNITTESTING_ASSERT_EQUAL((UInt64)Int64Max, uValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL(Int64Max, dataStore.AssumeInt64(value));

		// Stored as UInt64 case.
		uValue = 73;
		SEOUL_UNITTESTING_ASSERT(dataStore.SetUInt64ValueToArray(root, 0u, UInt64Max));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(value.IsUInt64());
		SEOUL_UNITTESTING_ASSERT(dataStore.AsUInt64(value, uValue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(UInt64Max, uValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL(UInt64Max, dataStore.AssumeUInt64(value));
	}
}

void DataStoreTest::TestNumbersInTable()
{
	DataStore dataStore;
	dataStore.MakeTable();

	DataNode root = dataStore.GetRootNode();

	DataNode value;

	static const HString kKey("Hello_This_Is_My_Key");

	// Int32 test - only one case (value is set and stored as Int32).
	{
		Int32 iValue = 103;
		SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToTable(root, kKey, 105));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, kKey, value));
		SEOUL_UNITTESTING_ASSERT(value.IsInt32Small());
		SEOUL_UNITTESTING_ASSERT(dataStore.AsInt32(value, iValue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(105, iValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL(105, dataStore.AssumeInt32Small(value));
	}

	// UInt32 test - 2 cases, depending on value, can be set and stored as an Int32 or UInt32.
	{
		// Stored as Int32 case.
		UInt32 uValue = 102u;
		SEOUL_UNITTESTING_ASSERT(dataStore.SetUInt32ValueToTable(root, kKey, 107u));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, kKey, value));
		SEOUL_UNITTESTING_ASSERT(value.IsInt32Small());
		SEOUL_UNITTESTING_ASSERT(dataStore.AsUInt32(value, uValue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(107u, uValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL(107u, (UInt32)dataStore.AssumeInt32Small(value));

		// Stored as UInt32 case.
		uValue = 102u;
		SEOUL_UNITTESTING_ASSERT(dataStore.SetUInt32ValueToTable(root, kKey, UIntMax));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, kKey, value));
		SEOUL_UNITTESTING_ASSERT(value.IsUInt32());
		SEOUL_UNITTESTING_ASSERT(dataStore.AsUInt32(value, uValue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(UIntMax, uValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL(UIntMax, dataStore.AssumeUInt32(value));
	}

	// Float32 test - 2 cases, depending on value, can be set and stored as an Int32 or Float32.
	{
		// Stored as Float31 case.
		Float32 fValue = 105.0f;
		SEOUL_UNITTESTING_ASSERT(dataStore.SetFloat32ValueToTable(root, kKey, 1.5f));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, kKey, value));
		SEOUL_UNITTESTING_ASSERT(value.IsFloat31());
		SEOUL_UNITTESTING_ASSERT(dataStore.AsFloat32(value, fValue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1.5f, fValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1.5f, dataStore.AssumeFloat31(value));

		// Stored as Float32 case.
		fValue = 101.0f;
		SEOUL_UNITTESTING_ASSERT(dataStore.SetFloat32ValueToTable(root, kKey, 1.1f));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, kKey, value));
		SEOUL_UNITTESTING_ASSERT(value.IsFloat32());
		SEOUL_UNITTESTING_ASSERT(dataStore.AsFloat32(value, fValue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1.1f, fValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1.1f, dataStore.AssumeFloat32(value));

		// Stored as Int32 case.
		fValue = 101.0f;
		SEOUL_UNITTESTING_ASSERT(dataStore.SetFloat32ValueToTable(root, kKey, 1.0f));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, kKey, value));
		SEOUL_UNITTESTING_ASSERT(value.IsInt32Small());
		SEOUL_UNITTESTING_ASSERT(dataStore.AsFloat32(value, fValue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, fValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, dataStore.AssumeInt32Small(value));
	}

	// Int64 test - 3 cases, can be stored as an Int32, UInt32, or Int64 depending on value.
	{
		// Stored as Int32 case.
		Int64 iValue = 99;
		SEOUL_UNITTESTING_ASSERT(dataStore.SetInt64ValueToTable(root, kKey, 107));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, kKey, value));
		SEOUL_UNITTESTING_ASSERT(value.IsInt32Small());
		SEOUL_UNITTESTING_ASSERT(dataStore.AsInt64(value, iValue));
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int64)107, iValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL(107, dataStore.AssumeInt32Small(value));

		// Stored as UInt32 case.
		iValue = 99;
		SEOUL_UNITTESTING_ASSERT(dataStore.SetInt64ValueToTable(root, kKey, (Int64)UIntMax));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, kKey, value));
		SEOUL_UNITTESTING_ASSERT(value.IsUInt32());
		SEOUL_UNITTESTING_ASSERT(dataStore.AsInt64(value, iValue));
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int64)UIntMax, iValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL(UIntMax, dataStore.AssumeUInt32(value));

		// Stored as Int64 case.
		iValue = 99;
		SEOUL_UNITTESTING_ASSERT(dataStore.SetInt64ValueToTable(root, kKey, Int64Max));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, kKey, value));
		SEOUL_UNITTESTING_ASSERT(value.IsInt64());
		SEOUL_UNITTESTING_ASSERT(dataStore.AsInt64(value, iValue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(Int64Max, iValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL(Int64Max, dataStore.AssumeInt64(value));
	}

	// UInt64 test - 4 cases, can be stored as an Int32, UInt32, Int64, or UInt64 depending on value.
	{
		// Stored as Int32 case.
		UInt64 uValue = 73;
		SEOUL_UNITTESTING_ASSERT(dataStore.SetUInt64ValueToTable(root, kKey, 107));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, kKey, value));
		SEOUL_UNITTESTING_ASSERT(value.IsInt32Small());
		SEOUL_UNITTESTING_ASSERT(dataStore.AsUInt64(value, uValue));
		SEOUL_UNITTESTING_ASSERT_EQUAL((UInt64)107, uValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL(107, dataStore.AssumeInt32Small(value));

		// Stored as UInt32 case.
		uValue = 73;
		SEOUL_UNITTESTING_ASSERT(dataStore.SetUInt64ValueToTable(root, kKey, (Int64)UIntMax));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, kKey, value));
		SEOUL_UNITTESTING_ASSERT(value.IsUInt32());
		SEOUL_UNITTESTING_ASSERT(dataStore.AsUInt64(value, uValue));
		SEOUL_UNITTESTING_ASSERT_EQUAL((UInt64)UIntMax, uValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL(UIntMax, dataStore.AssumeUInt32(value));

		// Stored as Int64 case.
		uValue = 73;
		SEOUL_UNITTESTING_ASSERT(dataStore.SetUInt64ValueToTable(root, kKey, Int64Max));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, kKey, value));
		SEOUL_UNITTESTING_ASSERT(value.IsInt64());
		SEOUL_UNITTESTING_ASSERT(dataStore.AsUInt64(value, uValue));
		SEOUL_UNITTESTING_ASSERT_EQUAL((UInt64)Int64Max, uValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL(Int64Max, dataStore.AssumeInt64(value));

		// Stored as UInt64 case.
		uValue = 73;
		SEOUL_UNITTESTING_ASSERT(dataStore.SetUInt64ValueToTable(root, kKey, UInt64Max));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, kKey, value));
		SEOUL_UNITTESTING_ASSERT(value.IsUInt64());
		SEOUL_UNITTESTING_ASSERT(dataStore.AsUInt64(value, uValue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(UInt64Max, uValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL(UInt64Max, dataStore.AssumeUInt64(value));
	}
}

void DataStoreTest::TestToString()
{
	DataStore dataStore;
	dataStore.MakeTable();
	DataNode const rootNode = dataStore.GetRootNode();

	{
		dataStore.SetBooleanValueToTable(rootNode, HString("one"), true);
		dataStore.SetBooleanValueToTable(rootNode, HString("two"), false);

		{
			FilePath filePath;
			dataStore.SetFilePathToTable(rootNode, HString("three"), filePath);
			filePath.SetDirectory(GameDirectory::kContent);
			dataStore.SetFilePathToTable(rootNode, HString("four"), filePath);
			filePath.SetRelativeFilenameWithoutExtension(FilePathRelativeFilename("Foo"));
			dataStore.SetFilePathToTable(rootNode, HString("five"), filePath);
			filePath.SetType(FileType::kFont);
			dataStore.SetFilePathToTable(rootNode, HString("six"), filePath);
		}

		dataStore.SetFloat32ValueToTable(rootNode, HString("seven"), FloatMax);
		dataStore.SetFloat32ValueToTable(rootNode, HString("nine"), -FloatMax);

		dataStore.SetStringToTable(rootNode, HString("ten"), "foooooooo");

		dataStore.SetInt32ValueToTable(rootNode, HString("eleven"), -1940923850);

		dataStore.SetInt64ValueToTable(rootNode, HString("twelve"), Int64Max);

		dataStore.SetNullValueToTable(rootNode, HString("thirteen"));

		dataStore.SetStringToTable(rootNode, HString("fourteen"), String("WAAAAAAAAAAAAAAAH"));

		dataStore.SetUInt32ValueToTable(rootNode, HString("fifteen"), UIntMax);

		dataStore.SetUInt64ValueToTable(rootNode, HString("sixteen"), UInt64Max);

		// Use hidden constant to suppress warnings about divide by zero.
		Float32 f = kfUnitTestZeroConstant;
		dataStore.SetFloat32ValueToTable(rootNode, HString("_ind_"), (0.0f / f));
		dataStore.SetFloat32ValueToTable(rootNode, HString("_nan_"), std::numeric_limits<Float32>::quiet_NaN());
		dataStore.SetFloat32ValueToTable(rootNode, HString("_nan2_"), std::numeric_limits<Float32>::signaling_NaN());
		dataStore.SetFloat32ValueToTable(rootNode, HString("_inf_"), std::numeric_limits<Float32>::infinity());
		dataStore.SetFloat32ValueToTable(rootNode, HString("_neg_inf_"), -std::numeric_limits<Float32>::infinity());

		dataStore.SetFloat32ValueToTable(rootNode, HString("big_int_float"), 67108864.0f);
		dataStore.SetFloat32ValueToTable(rootNode, HString("small_int_float"), -67108872.0f);
		dataStore.SetFloat32ValueToTable(rootNode, HString("big_int64_float"), 9.2233722e+017f);
		dataStore.SetFloat32ValueToTable(rootNode, HString("small_int64_float"), -9.2233722e+017f);
		dataStore.SetFloat32ValueToTable(rootNode, HString("big_uint64_float"), 9.2233720e+018f);
	}

	for (Int32 i = 0; i < 8; ++i)
	{
		String s;
		dataStore.ToString(rootNode, s, (i % 2) == 0, 0, false);

		DataStore testDataStore;
		if (i < 4)
		{
			SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromString(s, testDataStore));
		}
		else
		{
			SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromString(s.CStr(), s.GetSize(), testDataStore));
		}
		DataNode const testRootNode = testDataStore.GetRootNode();

		{
			DataNode value;
			Bool bValue = false;
			FilePath filePathValue;
			filePathValue.SetDirectory(GameDirectory::kToolsBin);
			Float32 fValue = -700.0f;
			HString identifierValue("12345");
			Int32 iValue = -123415;
			Int64 i64Value = -12151555;
			String sValue("125155");
			UInt32 uValue = 3580283508u;
			UInt64 u64Value = 802305982395;

			SEOUL_UNITTESTING_ASSERT(testDataStore.GetValueFromTable(testRootNode, HString("one"), value));
			SEOUL_UNITTESTING_ASSERT(testDataStore.AsBoolean(value, bValue));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bValue);
			SEOUL_UNITTESTING_ASSERT(testDataStore.GetValueFromTable(testRootNode, HString("two"), value));
			SEOUL_UNITTESTING_ASSERT(testDataStore.AsBoolean(value, bValue));
			SEOUL_UNITTESTING_ASSERT_EQUAL(false, bValue);

			{
				FilePath testFilePath;
				SEOUL_UNITTESTING_ASSERT(testDataStore.GetValueFromTable(testRootNode, HString("three"), value));
				SEOUL_UNITTESTING_ASSERT(testDataStore.AsFilePath(value, filePathValue));
				SEOUL_UNITTESTING_ASSERT_EQUAL(testFilePath, filePathValue);

				SEOUL_UNITTESTING_ASSERT(testDataStore.GetValueFromTable(testRootNode, HString("four"), value));
				SEOUL_UNITTESTING_ASSERT(testDataStore.AsFilePath(value, filePathValue));
				testFilePath.SetDirectory(GameDirectory::kContent);
				SEOUL_UNITTESTING_ASSERT_EQUAL(testFilePath, filePathValue);

				SEOUL_UNITTESTING_ASSERT(testDataStore.GetValueFromTable(testRootNode, HString("five"), value));
				SEOUL_UNITTESTING_ASSERT(testDataStore.AsFilePath(value, filePathValue));
				testFilePath.SetRelativeFilenameWithoutExtension(FilePathRelativeFilename("Foo"));
				SEOUL_UNITTESTING_ASSERT_EQUAL(testFilePath, filePathValue);

				SEOUL_UNITTESTING_ASSERT(testDataStore.GetValueFromTable(testRootNode, HString("six"), value));
				SEOUL_UNITTESTING_ASSERT(testDataStore.AsFilePath(value, filePathValue));
				testFilePath.SetType(FileType::kFont);
				SEOUL_UNITTESTING_ASSERT_EQUAL(testFilePath, filePathValue);
			}

			SEOUL_UNITTESTING_ASSERT(testDataStore.GetValueFromTable(testRootNode, HString("seven"), value));
			SEOUL_UNITTESTING_ASSERT(testDataStore.AsFloat32(value, fValue));
			SEOUL_UNITTESTING_ASSERT_EQUAL(3.4028200e+038f, fValue);

			SEOUL_UNITTESTING_ASSERT(testDataStore.GetValueFromTable(testRootNode, HString("nine"), value));
			SEOUL_UNITTESTING_ASSERT(testDataStore.AsFloat32(value, fValue));
			SEOUL_UNITTESTING_ASSERT_EQUAL(-3.4028200e+038f, fValue);

			SEOUL_UNITTESTING_ASSERT(testDataStore.GetValueFromTable(testRootNode, HString("ten"), value));
			SEOUL_UNITTESTING_ASSERT(testDataStore.AsString(value, sValue));
			SEOUL_UNITTESTING_ASSERT(testDataStore.AsString(value, identifierValue));
			SEOUL_UNITTESTING_ASSERT_EQUAL(String("foooooooo"), sValue);
			SEOUL_UNITTESTING_ASSERT_EQUAL(HString("foooooooo"), identifierValue);

			SEOUL_UNITTESTING_ASSERT(testDataStore.GetValueFromTable(testRootNode, HString("eleven"), value));
			SEOUL_UNITTESTING_ASSERT(testDataStore.AsInt32(value, iValue));
			SEOUL_UNITTESTING_ASSERT_EQUAL(-1940923850, iValue);

			SEOUL_UNITTESTING_ASSERT(testDataStore.GetValueFromTable(testRootNode, HString("twelve"), value));
			SEOUL_UNITTESTING_ASSERT(testDataStore.AsInt64(value, i64Value));
			SEOUL_UNITTESTING_ASSERT_EQUAL(Int64Max, i64Value);

			SEOUL_UNITTESTING_ASSERT(testDataStore.GetValueFromTable(testRootNode, HString("thirteen"), value));
			SEOUL_UNITTESTING_ASSERT(value.IsNull());

			SEOUL_UNITTESTING_ASSERT(testDataStore.GetValueFromTable(testRootNode, HString("fourteen"), value));
			SEOUL_UNITTESTING_ASSERT(testDataStore.AsString(value, sValue));
			SEOUL_UNITTESTING_ASSERT(testDataStore.AsString(value, identifierValue));
			SEOUL_UNITTESTING_ASSERT_EQUAL(String("WAAAAAAAAAAAAAAAH"), sValue);
			SEOUL_UNITTESTING_ASSERT_EQUAL(HString("WAAAAAAAAAAAAAAAH"), identifierValue);

			SEOUL_UNITTESTING_ASSERT(testDataStore.GetValueFromTable(testRootNode, HString("fifteen"), value));
			SEOUL_UNITTESTING_ASSERT(testDataStore.AsUInt32(value, uValue));
			SEOUL_UNITTESTING_ASSERT_EQUAL(UIntMax, uValue);

			SEOUL_UNITTESTING_ASSERT(testDataStore.GetValueFromTable(testRootNode, HString("sixteen"), value));
			SEOUL_UNITTESTING_ASSERT(testDataStore.AsUInt64(value, u64Value));
			SEOUL_UNITTESTING_ASSERT_EQUAL(UInt64Max, u64Value);

			SEOUL_UNITTESTING_ASSERT(testDataStore.GetValueFromTable(testRootNode, HString("_ind_"), value));
			SEOUL_UNITTESTING_ASSERT(testDataStore.AsFloat32(value, fValue));
			SEOUL_UNITTESTING_ASSERT(IsNaN(fValue));

			SEOUL_UNITTESTING_ASSERT(testDataStore.GetValueFromTable(testRootNode, HString("_nan_"), value));
			SEOUL_UNITTESTING_ASSERT(testDataStore.AsFloat32(value, fValue));
			SEOUL_UNITTESTING_ASSERT(IsNaN(fValue));

			SEOUL_UNITTESTING_ASSERT(testDataStore.GetValueFromTable(testRootNode, HString("_nan2_"), value));
			SEOUL_UNITTESTING_ASSERT(testDataStore.AsFloat32(value, fValue));
			SEOUL_UNITTESTING_ASSERT(IsNaN(fValue));

			SEOUL_UNITTESTING_ASSERT(testDataStore.GetValueFromTable(testRootNode, HString("_inf_"), value));
			SEOUL_UNITTESTING_ASSERT(testDataStore.AsFloat32(value, fValue));
			SEOUL_UNITTESTING_ASSERT(IsInf(fValue) && fValue > 0.0f);

			SEOUL_UNITTESTING_ASSERT(testDataStore.GetValueFromTable(testRootNode, HString("_neg_inf_"), value));
			SEOUL_UNITTESTING_ASSERT(testDataStore.AsFloat32(value, fValue));
			SEOUL_UNITTESTING_ASSERT(IsInf(fValue) && fValue < 0.0f);

			SEOUL_UNITTESTING_ASSERT(testDataStore.GetValueFromTable(testRootNode, HString("big_int_float"), value));
			SEOUL_UNITTESTING_ASSERT(testDataStore.AsFloat32(value, fValue));
			SEOUL_UNITTESTING_ASSERT_EQUAL(67108864.0f, fValue);

			// Also check that a big int value can parse to appropriate integer types.
			SEOUL_UNITTESTING_ASSERT(testDataStore.AsInt32(value, iValue));
			SEOUL_UNITTESTING_ASSERT_EQUAL(67108864, iValue);
			SEOUL_UNITTESTING_ASSERT(testDataStore.AsInt64(value, i64Value));
			SEOUL_UNITTESTING_ASSERT_EQUAL((Int64)67108864, i64Value);
			SEOUL_UNITTESTING_ASSERT(testDataStore.AsUInt32(value, uValue));
			SEOUL_UNITTESTING_ASSERT_EQUAL(67108864u, uValue);
			SEOUL_UNITTESTING_ASSERT(testDataStore.AsUInt64(value, u64Value));
			SEOUL_UNITTESTING_ASSERT_EQUAL((UInt64)67108864u, u64Value);

			SEOUL_UNITTESTING_ASSERT(testDataStore.GetValueFromTable(testRootNode, HString("small_int_float"), value));
			SEOUL_UNITTESTING_ASSERT(testDataStore.AsFloat32(value, fValue));
			SEOUL_UNITTESTING_ASSERT_EQUAL(-67108872.0f, fValue);

			// Also check that a big int value can parse to appropriate integer types.
			SEOUL_UNITTESTING_ASSERT(testDataStore.AsInt32(value, iValue));
			SEOUL_UNITTESTING_ASSERT_EQUAL((Int32)-67108872, iValue);
			SEOUL_UNITTESTING_ASSERT(testDataStore.AsInt64(value, i64Value));
			SEOUL_UNITTESTING_ASSERT_EQUAL((Int64)-67108872, i64Value);

			SEOUL_UNITTESTING_ASSERT(testDataStore.GetValueFromTable(testRootNode, HString("big_int64_float"), value));
			SEOUL_UNITTESTING_ASSERT(testDataStore.AsFloat32(value, fValue));
			SEOUL_UNITTESTING_ASSERT_EQUAL(9.2233722e+017f, fValue);

			// Also check that a big int64 value can parse to appropriate integer types.
			SEOUL_UNITTESTING_ASSERT(testDataStore.AsInt64(value, i64Value));
			SEOUL_UNITTESTING_ASSERT_EQUAL((Int64)922337217429372928, i64Value);
			SEOUL_UNITTESTING_ASSERT(testDataStore.AsUInt64(value, u64Value));
			SEOUL_UNITTESTING_ASSERT_EQUAL((UInt64)922337217429372928, u64Value);

			SEOUL_UNITTESTING_ASSERT(testDataStore.GetValueFromTable(testRootNode, HString("small_int64_float"), value));
			SEOUL_UNITTESTING_ASSERT(testDataStore.AsFloat32(value, fValue));
			SEOUL_UNITTESTING_ASSERT_EQUAL(-9.2233722e+017f, fValue);

			// Also check that a samll int64 value can parse to appropriate integer types.
			SEOUL_UNITTESTING_ASSERT(testDataStore.AsInt64(value, i64Value));
			SEOUL_UNITTESTING_ASSERT_EQUAL((Int64)-922337217429372928, i64Value);

			SEOUL_UNITTESTING_ASSERT(testDataStore.GetValueFromTable(testRootNode, HString("big_uint64_float"), value));
			SEOUL_UNITTESTING_ASSERT(testDataStore.AsFloat32(value, fValue));
			SEOUL_UNITTESTING_ASSERT_EQUAL(9.2233720e+018f, fValue);

			// Also check that a big uint64 value can parse to appropriate integer types.
			SEOUL_UNITTESTING_ASSERT(testDataStore.AsUInt64(value, u64Value));
			SEOUL_UNITTESTING_ASSERT_EQUAL((UInt64)9223372036854775808u, u64Value);
		}
	}
}

void DataStoreTest::TestStress()
{
	// Some keys and values that we use.
	static const HString kAbilityLevels("AbilityLevels");
	static const HString kCharacterLevel("CharacterLevel");
	static const HString kIdentifier("Identifier");
	static const HString kMessage("Message");
	static const HString kRating("Rating");
	static const String ksStringValue(
		"I am the very model of a modern Major-General,"
		"I've information vegetable, animal, and mineral,"
		"I know the kings of England, and I quote the fights historical"
		"From Marathon to Waterloo, in order categorical"
		"I'm very well acquainted, too, with matters mathematical,"
		"I understand equations, both the simple and quadratical,"
		"About binomial theorem I'm teeming with a lot o' news, (bothered for a rhyme)"
		"With many cheerful facts about the square of the hypotenuse.");
	static const HString kValue("Value_Of_Identifier_Key");
	static const HString kWeight("Weight");
	static const HString kNullValue("NullValue");
	static const HString kSpecialErase("SpecialErase");

	// Very large number of entries to add.
#if SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_IOS
	// Half as many on mobile for devices with too little
	// memory - this test can eat up a few hundred MBs,
	// which can cause the test to be killed due
	// to low memory.
	static UInt32 const kuEntries = 32889;
#else
	static UInt32 const kuEntries = 65781;
#endif
	static UInt32 const kuGarbageCollectionInterval = 8192u;

	DataStore dataStore;
	dataStore.MakeArray();

	// Add entries and run garbage collection every kuGarbageCollectionInterval
	DataNode rootNode = dataStore.GetRootNode();
	Bool bCompactContainers = false;
	Bool bVersion1Forced = false;
	for (UInt32 i = 0u; i < kuEntries; ++i)
	{
		SEOUL_UNITTESTING_ASSERT(dataStore.SetTableToArray(rootNode, i));

		DataNode table;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(rootNode, i, table));
		SEOUL_UNITTESTING_ASSERT(table.IsTable());

		// Add values to table
		SEOUL_UNITTESTING_ASSERT(dataStore.SetStringToTable(table, kIdentifier, kValue));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToTable(table, kRating, i + 225));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToTable(table, kCharacterLevel, i + 1));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetStringToTable(table, kMessage, ksStringValue));

		// Array value.
		SEOUL_UNITTESTING_ASSERT(dataStore.SetArrayToTable(table, kAbilityLevels));
		DataNode array;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(table, kAbilityLevels, array));
		SEOUL_UNITTESTING_ASSERT(array.IsArray());
		SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(array, 0u, 1));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(array, 1u, 1));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(array, 2u, 1));

		// Remaining value.
		SEOUL_UNITTESTING_ASSERT(dataStore.SetFloat32ValueToTable(table, kWeight, 0.99f));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetNullValueToTable(table, kNullValue));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetSpecialEraseToTable(table, kSpecialErase));

		// Collect garbage after interval passes.
		if (0u == (i % kuGarbageCollectionInterval))
		{
			dataStore.CollectGarbage(bCompactContainers);
			SEOUL_UNITTESTING_ASSERT(dataStore.VerifyIntegrity());

			bCompactContainers = !bCompactContainers;
			bVersion1Forced = !bVersion1Forced;

			// Also, perform a save/load. This should fully restore the DataStore,
			// which should leave the rootNode valid.
			{
				MemorySyncFile syncFile;
				SEOUL_UNITTESTING_ASSERT(dataStore.Save(syncFile, keCurrentPlatform));

				DataStore newDataStore;
				SEOUL_UNITTESTING_ASSERT(syncFile.Seek(0, File::kSeekFromStart));
				SEOUL_UNITTESTING_ASSERT(newDataStore.Load(syncFile));
				SEOUL_UNITTESTING_ASSERT(newDataStore.VerifyIntegrity());

				SEOUL_UNITTESTING_ASSERT(DataStore::Equals(
					dataStore,
					dataStore.GetRootNode(),
					newDataStore,
					newDataStore.GetRootNode()));

				dataStore.Swap(newDataStore);
				rootNode = dataStore.GetRootNode();
			}
		}
	}

	// Now check values.
	for (UInt32 i = 0u; i < kuEntries; ++i)
	{
		DataNode table;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(rootNode, i, table));
		SEOUL_UNITTESTING_ASSERT(table.IsTable());

		// Check values.
		DataNode value;

		// kIdentifier with kValue
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(table, kIdentifier, value));
		SEOUL_UNITTESTING_ASSERT(value.IsString());
		String sValue;
		HString testValue;
		SEOUL_UNITTESTING_ASSERT(dataStore.AsString(value, sValue));
		SEOUL_UNITTESTING_ASSERT(HString::Get(testValue, sValue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(kValue, testValue);
		testValue = HString();
		SEOUL_UNITTESTING_ASSERT(dataStore.AsString(value, testValue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(kValue, testValue);

		// kRating
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(table, kRating, value));
		SEOUL_UNITTESTING_ASSERT(value.IsInt32Small());
		Int32 iTestValue = 0;
		SEOUL_UNITTESTING_ASSERT(dataStore.AsInt32(value, iTestValue));
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int32)i + 225, iTestValue);

		// kCharacterLevel
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(table, kCharacterLevel, value));
		SEOUL_UNITTESTING_ASSERT(value.IsInt32Small());
		SEOUL_UNITTESTING_ASSERT(dataStore.AsInt32(value, iTestValue));
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int32)i + 1, iTestValue);

		// kMessage
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(table, kMessage, value));
		SEOUL_UNITTESTING_ASSERT(value.IsString());
		String sTestValue;
		HString hstringTestValue;
		SEOUL_UNITTESTING_ASSERT(dataStore.AsString(value, sTestValue));
		SEOUL_UNITTESTING_ASSERT(dataStore.AsString(value, hstringTestValue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(ksStringValue, sTestValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL(HString(ksStringValue), hstringTestValue);

		// Array value.
		DataNode array;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(table, kAbilityLevels, array));
		SEOUL_UNITTESTING_ASSERT(array.IsArray());
		UInt32 uArrayCount;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetArrayCount(array, uArrayCount));
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, uArrayCount);
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(array, 0u, value));
		SEOUL_UNITTESTING_ASSERT(value.IsInt32Small());
		SEOUL_UNITTESTING_ASSERT(dataStore.AsInt32(value, iTestValue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, iTestValue);
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(array, 1u, value));
		SEOUL_UNITTESTING_ASSERT(value.IsInt32Small());
		SEOUL_UNITTESTING_ASSERT(dataStore.AsInt32(value, iTestValue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, iTestValue);
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(array, 2u, value));
		SEOUL_UNITTESTING_ASSERT(value.IsInt32Small());
		SEOUL_UNITTESTING_ASSERT(dataStore.AsInt32(value, iTestValue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, iTestValue);

		// kWeight
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(table, kWeight, value));
		SEOUL_UNITTESTING_ASSERT(value.IsFloat31());
		Float32 fTestValue = 0.0f;
		SEOUL_UNITTESTING_ASSERT(dataStore.AsFloat32(value, fTestValue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(0.99f, fTestValue);

		// Specials
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(table, kNullValue, value));
		SEOUL_UNITTESTING_ASSERT(value.IsNull());
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(table, kSpecialErase, value));
		SEOUL_UNITTESTING_ASSERT(value.IsSpecialErase());
	}
}

// Regression for a DataStore bug. String were being allocated with the wrong capacity
// value, which didn't exhibit until garbage collection was engaged (the garbage collector
// would allocate too much space in the output, and could possibly read passed the
// end of the buffer).
void DataStoreTest::TestStringAlloc()
{
	// Regular string test.
	{
		// To reproduce, we need to create a DataStore that should be identical
		// after garbage collection that contains a string.
		DataStore dataStore;
		dataStore.MakeArray(1u);
		SEOUL_UNITTESTING_ASSERT(dataStore.SetStringToArray(dataStore.GetRootNode(), 0u, "Hello World"));

		DataStore copy;
		copy.CopyFrom(dataStore);

		dataStore.CollectGarbageAndCompactHeap();
		SEOUL_UNITTESTING_ASSERT_EQUAL(copy.GetHeapCapacityInBytes(), dataStore.GetHeapCapacityInBytes());
		SEOUL_UNITTESTING_ASSERT_EQUAL(copy.GetInUseHeapSizeInBytes(), dataStore.GetInUseHeapSizeInBytes());
		SEOUL_UNITTESTING_ASSERT(DataStore::Equals(
			copy,
			copy.GetRootNode(),
			dataStore,
			dataStore.GetRootNode()));

		SEOUL_UNITTESTING_ASSERT(DataStore::UnitTestHook_ByteForByteEqual(copy, dataStore));
	}
}

/**
 * Regression for some edge cases, make sure all floating point values are returned
 * as either identical, or nearly identical.
 */
void DataStoreTest::TestFloat()
{
	DataStore dataStore;
	dataStore.MakeArray();

	// Increment by 128 so the test finishes in a reasonable time. Not ideal, since we
	// don't get complete converage, but does hit all the main edge cases (denormals, NaN, inf).
	DataNode value;
	for (UInt64 i = 0u; i <= (UInt64)UIntMax; i += 128u)
	{
		union
		{
			UInt32 u;
			Float32 f;
		};
		u = (UInt32)i;

		Float32 fOutValue;

		SEOUL_UNITTESTING_ASSERT(dataStore.SetFloat32ValueToArray(dataStore.GetRootNode(), 0u, f));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(dataStore.GetRootNode(), 0u, value));
		SEOUL_UNITTESTING_ASSERT(dataStore.AsFloat32(value, fOutValue));

		if (IsNaN(f))
		{
			SEOUL_UNITTESTING_ASSERT(IsNaN(fOutValue));
		}
		else if (f != fOutValue)
		{
			// Take the output and check it - should be off by at most one bit.
			union
			{
				UInt32 u2;
				Float32 f2;
			};
			f2 = fOutValue;
			u2 = (u2 & (~(0x1))) | (u & 0x1);

			if (f != f2)
			{
				SEOUL_LOG("%u: %f != %f", (UInt32)i, f, fOutValue);
				SEOUL_UNITTESTING_ASSERT(false);
			}
		}
	}
}

void DataStoreTest::TestVerifyIntegrity()
{
	for (Int32 i = 0; i < DataStore::CORRUPTION_TYPES; ++i)
	{
		// Should be true.
		DataStore dataStore;
		SEOUL_UNITTESTING_ASSERT(dataStore.VerifyIntegrity());
		dataStore.MakeTable();
		SEOUL_UNITTESTING_ASSERT(dataStore.VerifyIntegrity());
		dataStore.MakeArray();

		// Intentionally corrupt the DataStore.
		dataStore.UnitTestHook_FillWithCorruptedData((DataStore::CorruptedDataType)i);
		SEOUL_UNITTESTING_ASSERT(!dataStore.VerifyIntegrity());
	}
}

// Verification that data created prior to the introduction of Float31 vs. Float32
// still loads correctly.
void DataStoreTest::TestOldData()
{
// #define SEOUL_GENERATE_DATA 1
#if SEOUL_GENERATE_DATA
	static Byte const* const ksData =
		"{\"TestValue\": [0, 1.0, 2.1, -3, -4.1, 5.0, 2147483648, -2147483649, 18446744073709551615, 81985529216486895, 1e-5, 7E7, 10E+8, -67108864, 67108863, -67108865, 67108864]}";

	DataStore dataStore;
	SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromString(ksData, dataStore));

	MemorySyncFile file;
	SEOUL_UNITTESTING_ASSERT(dataStore.Save(file, keCurrentPlatform));
	auto const s(Base64Encode(file.GetBuffer().GetBuffer(), file.GetBuffer().GetTotalDataSizeInBytes()));

	SEOUL_LOG("%s", s.CStr());
#else
	static Byte const* const ksDataBase64 = "605tur1m0ewBAAAAAQAAAAALAAAAAFRlc3RWYWx1ZQAKAAAAAQAAIAcAACAaAAAgGwAAIB0AACAfAAAgIQAAICIAACAjAAAgJAAAICUAAAAoAAAAAgAAAAEAAAAAAAAAKgEAAAAAAAABAAAAEQAAABEAAAAEAAAAJAAAAGdmBkCk////MzODwKQAAAAwAgAAMgMAADQEAAAyBQAArcUnNy4GAAAuBwAABAAAgOT//38uCAAALgkAAAAAAID///9/////////////////782riWdFIwGAHSwEAMqaO/////sAAAAEAAAIAAoAAAAKAAAAAAAAAA==";

#if SEOUL_PLATFORM_WINDOWS
	static const Int64 kBigInt64Test = 0x0123456789ABCDEFi64;
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
	static const Int64 kBigInt64Test = 0x0123456789ABCDEFll;
#else
#error "Define for this platform."
#endif

	DataStore dataStore;
	Vector<Byte> vData;
	SEOUL_UNITTESTING_ASSERT(Base64Decode(ksDataBase64, vData));
	FullyBufferedSyncFile file(vData.Data(), vData.GetSizeInBytes(), false);
	SEOUL_UNITTESTING_ASSERT(dataStore.Load(file));

	SEOUL_UNITTESTING_ASSERT(dataStore.GetRootNode().IsTable());

	DataNode testValue;
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(dataStore.GetRootNode(), HString("TestValue"), testValue));
	SEOUL_UNITTESTING_ASSERT(testValue.IsArray());

	UInt32 uArrayCount = 0u;
	SEOUL_UNITTESTING_ASSERT(dataStore.GetArrayCount(testValue, uArrayCount));
	SEOUL_UNITTESTING_ASSERT_EQUAL(17u, uArrayCount);

	DataNode number;
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(testValue, 0u, number));
	SEOUL_UNITTESTING_ASSERT(number.IsInt32Small());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, dataStore.AssumeInt32Small(number));

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(testValue, 1u, number));
	SEOUL_UNITTESTING_ASSERT(number.IsInt32Small());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, dataStore.AssumeInt32Small(number));

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(testValue, 2u, number));
	SEOUL_UNITTESTING_ASSERT(number.IsFloat31());
	SEOUL_UNITTESTING_ASSERT_EQUAL(2.1f, dataStore.AssumeFloat31(number));

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(testValue, 3u, number));
	SEOUL_UNITTESTING_ASSERT(number.IsInt32Small());
	SEOUL_UNITTESTING_ASSERT_EQUAL(-3, dataStore.AssumeInt32Small(number));

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(testValue, 4u, number));
	SEOUL_UNITTESTING_ASSERT(number.IsFloat31());
	SEOUL_UNITTESTING_ASSERT_EQUAL(GetMaskedFloat31(-4.1f), dataStore.AssumeFloat31(number));

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(testValue, 5u, number));
	SEOUL_UNITTESTING_ASSERT(number.IsInt32Small());
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, dataStore.AssumeInt32Small(number));

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(testValue, 6u, number));
	SEOUL_UNITTESTING_ASSERT(number.IsUInt32());
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt32)IntMax + 1u, dataStore.AssumeUInt32(number));

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(testValue, 7u, number));
	SEOUL_UNITTESTING_ASSERT(number.IsInt64());
	SEOUL_UNITTESTING_ASSERT_EQUAL((Int64)IntMin - 1, dataStore.AssumeInt64(number));

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(testValue, 8u, number));
	SEOUL_UNITTESTING_ASSERT(number.IsUInt64());
	SEOUL_UNITTESTING_ASSERT_EQUAL(UInt64Max, dataStore.AssumeUInt64(number));

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(testValue, 9u, number));
	SEOUL_UNITTESTING_ASSERT(number.IsInt64());
	SEOUL_UNITTESTING_ASSERT_EQUAL(kBigInt64Test, dataStore.AssumeInt64(number));

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(testValue, 10u, number));
	SEOUL_UNITTESTING_ASSERT(number.IsFloat31());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1e-5f, dataStore.AssumeFloat31(number));

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(testValue, 11u, number));
	SEOUL_UNITTESTING_ASSERT(number.IsInt32Big());
	SEOUL_UNITTESTING_ASSERT_EQUAL(70000000, dataStore.AssumeInt32Big(number));

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(testValue, 12u, number));
	SEOUL_UNITTESTING_ASSERT(number.IsInt32Big());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1000000000, dataStore.AssumeInt32Big(number));

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(testValue, 13u, number));
	SEOUL_UNITTESTING_ASSERT(number.IsInt32Small());
	SEOUL_UNITTESTING_ASSERT_EQUAL(kiDataNodeMinInt32SmallValue, dataStore.AssumeInt32Small(number));

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(testValue, 14u, number));
	SEOUL_UNITTESTING_ASSERT(number.IsInt32Small());
	SEOUL_UNITTESTING_ASSERT_EQUAL(kiDataNodeMaxInt32SmallValue, dataStore.AssumeInt32Small(number));

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(testValue, 15u, number));
	SEOUL_UNITTESTING_ASSERT(number.IsInt32Big());
	SEOUL_UNITTESTING_ASSERT_EQUAL(kiDataNodeMinInt32SmallValue - 1, dataStore.AssumeInt32Big(number));

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(testValue, 16u, number));
	SEOUL_UNITTESTING_ASSERT(number.IsInt32Big());
	SEOUL_UNITTESTING_ASSERT_EQUAL(kiDataNodeMaxInt32SmallValue + 1, dataStore.AssumeInt32Big(number));
#endif // /!SEOUL_GENERATE_DATA
}

void DataStoreTest::TestMoveNodeBetweenTables()
{
	DataStore dataStore;
	dataStore.MakeTable();

	SEOUL_UNITTESTING_ASSERT(dataStore.SetTableToTable(dataStore.GetRootNode(), HString("A")));
	SEOUL_UNITTESTING_ASSERT(dataStore.SetTableToTable(dataStore.GetRootNode(), HString("B")));

	DataNode tA;
	DataNode tB;
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(dataStore.GetRootNode(), HString("A"), tA));
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(dataStore.GetRootNode(), HString("B"), tB));

	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToTable(tA, HString("A"), 0));
	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToTable(tA, HString("B"), 1));
	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToTable(tA, HString("C"), 2));

	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToTable(tB, HString("C"), 7));


	// Different tables, same name
	SEOUL_UNITTESTING_ASSERT(dataStore.MoveNodeBetweenTables(tA, HString("A"), tB, HString("A")));
	// Same table, different name
	SEOUL_UNITTESTING_ASSERT(dataStore.MoveNodeBetweenTables(tA, HString("C"), tA, HString("A")));
	// Different tables, different names, overwrite destination table value
	SEOUL_UNITTESTING_ASSERT(dataStore.MoveNodeBetweenTables(tA, HString("B"), tB, HString("C")));

	static const String ksExpectedString = R"'({"A":{"A":2},"B":{"A":0,"C":1}})'";

	String s;
	dataStore.ToString(dataStore.GetRootNode(), s, false, 0, true);

	SEOUL_UNITTESTING_ASSERT_EQUAL_MESSAGE(ksExpectedString, s, "Got unexpected table result: %s", s.CStr());
}

void DataStoreTest::TestNullAsSpecialErase()
{
	static const String ksPatch = R"({"A": null, "C": null, "D": null, "F": null})";
	static const String ksBase = R"({"A": 1, "B": 2, "C": 3, "D": 4, "E": 5, "F": 6})";
	static const String ksExpected = R"({"B":2,"E":5})";

	DataStore dataStore;
	SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromString(ksBase, dataStore));
	DataStore patch;
	SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromString(ksPatch, patch, DataStoreParserFlags::kNullAsSpecialErase));
	SEOUL_UNITTESTING_ASSERT(ApplyDiff(patch, dataStore));

	String sResult;
	dataStore.ToString(dataStore.GetRootNode(), sResult, false, 0, true);
	SEOUL_UNITTESTING_ASSERT_EQUAL(ksExpected, sResult);
}

void DataStoreTest::TestEraseAgainstNoExist()
{
	static const String ksPatch = R"({"A": null, "C": null, "D": null, "F": null})";
	static const String ksBase = R"({"B": 2, "C": 3, "D": 4, "E": 5})";
	static const String ksExpected = R"({"B":2,"E":5})";

	DataStore dataStore;
	SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromString(ksBase, dataStore));
	DataStore patch;
	SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromString(ksPatch, patch, DataStoreParserFlags::kNullAsSpecialErase));
	SEOUL_UNITTESTING_ASSERT(ApplyDiff(patch, dataStore));

	String sResult;
	dataStore.ToString(dataStore.GetRootNode(), sResult, false, 0, true);
	SEOUL_UNITTESTING_ASSERT_EQUAL(ksExpected, sResult);
}

void DataStoreTest::TestEraseAgainstNoExistFromNull()
{
	static const String ksPatch = R"({"A": null, "C": null, "D": null, "F": null})";
	static const String ksExpected = R"({})";

	DataStore dataStore;
	DataStore patch;
	SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromString(ksPatch, patch, DataStoreParserFlags::kNullAsSpecialErase));
	SEOUL_UNITTESTING_ASSERT(ApplyDiff(patch, dataStore));

	String sResult;
	dataStore.ToString(dataStore.GetRootNode(), sResult, false, 0, true);
	SEOUL_UNITTESTING_ASSERT_EQUAL(ksExpected, sResult);
}

// Test for a regression where deep erase will erroneously generated inner
// table entries, leaving a null value at the end.
void DataStoreTest::TestEraseAgainstNoExistRegression()
{
	static const String ksPatch = R"(
		{
			"Currency": {
				"BossKey": {
					"Earned": 10,
					"LifetimeEarned" : 10,
					"LifetimePurchased" : null,
					"Purchased" : null
				}
			}
		})";
	static const String ksBase = R"({})";
	static const String ksExpected = R"({"Currency":{"BossKey":{"Earned":10,"LifetimeEarned":10}}})";

	DataStore dataStore;
	SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromString(ksBase, dataStore));
	DataStore patch;
	SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromString(ksPatch, patch, DataStoreParserFlags::kNullAsSpecialErase));
	SEOUL_UNITTESTING_ASSERT(ApplyDiff(patch, dataStore));

	String sResult;
	dataStore.ToString(dataStore.GetRootNode(), sResult, false, 0, true);
	SEOUL_UNITTESTING_ASSERT_EQUAL(ksExpected, sResult);
}

void DataStoreTest::TestEraseAgainstNoExistFromNullRegression()
{
	static const String ksPatch = R"(
		{
			"Currency": {
				"BossKey": {
					"Earned": 10,
					"LifetimeEarned" : 10,
					"LifetimePurchased" : null,
					"Purchased" : null
				}
			}
		})";
	static const String ksExpected = R"({"Currency":{"BossKey":{"Earned":10,"LifetimeEarned":10}}})";

	DataStore dataStore;
	DataStore patch;
	SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromString(ksPatch, patch, DataStoreParserFlags::kNullAsSpecialErase));
	SEOUL_UNITTESTING_ASSERT(ApplyDiff(patch, dataStore));

	String sResult;
	dataStore.ToString(dataStore.GetRootNode(), sResult, false, 0, true);
	SEOUL_UNITTESTING_ASSERT_EQUAL(ksExpected, sResult);
}

// Regression for a bug in version 1 of the serialized DataStore format,
// large strings (in bytes) could overflow the lookup offset and fail
// on load.
void DataStoreTest::TestLargeSerializedStringTableRegression()
{
	UnitTestsFileManagerHelper helper;

	DataStore dataStore;
	{
		void* p = nullptr;
		UInt32 u = 0u;
		SEOUL_UNITTESTING_ASSERT(FileManager::Get()->ReadAll(FilePath::CreateConfigFilePath("UnitTests/DataStore/LargeStringTable.dat"), p, u, 0u, MemoryBudgets::Developer));
		void* pU = nullptr;
		UInt32 uU = 0u;
		SEOUL_UNITTESTING_ASSERT(LZ4Decompress(p, u, pU, uU));
		MemoryManager::Deallocate(p);
		p = nullptr;
		u = 0u;

		FullyBufferedSyncFile file(pU, uU);
		SEOUL_UNITTESTING_ASSERT(dataStore.Load(file));
	}

	SEOUL_UNITTESTING_ASSERT(dataStore.VerifyIntegrity());
	SEOUL_UNITTESTING_ASSERT(dataStore.GetRootNode().IsArray());

	UInt32 uArrayCount = 0u;
	SEOUL_UNITTESTING_ASSERT(dataStore.GetArrayCount(dataStore.GetRootNode(), uArrayCount));
	SEOUL_UNITTESTING_ASSERT_EQUAL(35807u, uArrayCount);
}

// Verify that a serialized v1 data store is loadable.
void DataStoreTest::TestVersion1Load()
{
	UnitTestsFileManagerHelper helper;

	DataStore dataStore;
	{
		void* p = nullptr;
		UInt32 u = 0u;
		SEOUL_UNITTESTING_ASSERT(FileManager::Get()->ReadAll(FilePath::CreateConfigFilePath("UnitTests/DataStore/v1data.dat"), p, u, 0u, MemoryBudgets::Developer));

		SaveLoadUtil::SaveFileMetadata unusedMetadata;
		SEOUL_UNITTESTING_ASSERT(SaveLoadResult::kSuccess == SaveLoadUtil::FromBlob(p, u, unusedMetadata, dataStore));
		MemoryManager::Deallocate(p);
		p = nullptr;
		u = 0u;
	}

	SEOUL_UNITTESTING_ASSERT(dataStore.VerifyIntegrity());
	SEOUL_UNITTESTING_ASSERT(dataStore.GetRootNode().IsTable());

	DataStore expected;
	SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromFile(FilePath::CreateConfigFilePath("UnitTests/DataStore/v1data_expected.json"), expected));

	// String compare here, precision lost of floating point values on tostring.
	String sExpected;
	expected.ToString(expected.GetRootNode(), sExpected, false, 0, true);
	String sActual;
	dataStore.ToString(dataStore.GetRootNode(), sActual, false, 0, true);
	SEOUL_UNITTESTING_ASSERT_EQUAL(sExpected, sActual);
}

static void NormalizeLineEndings(void*& rp, UInt32& ru)
{
	if (0 == strcmp(SEOUL_EOL, "\r\n"))
	{
		return; // Nothing to do if Windows line endings.
	}

	// Repace \r\n with \n
	Byte* p = (Byte*)rp;
	UInt32 uOut = 0u;
	for (UInt32 i = 0u; i < ru; ++i)
	{
		auto ch = p[i];
		if ('\r' == ch)
		{
			continue;
		}

		p[uOut++] = ch;
	}

	ru = uOut;
}

static void NormalizeLineEndings(String& rs)
{
	void* p = nullptr;
	UInt32 u = 0u;
	rs.RelinquishBuffer(p, u);
	NormalizeLineEndings(p, u);
	rs.TakeOwnership(p, u);
}

void DataStoreTest::TestDataStorePrinter()
{
	UnitTestsFileManagerHelper helper;

	void* p = nullptr;
	UInt32 u = 0u;
	SEOUL_UNITTESTING_ASSERT(FileManager::Get()->ReadAll(
		FilePath::CreateConfigFilePath("UnitTests/DataStorePrinter/C.json"),
		p,
		u,
		0u,
		MemoryBudgets::Developer));
	NormalizeLineEndings(p, u);

	DataStore ds;
	SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromString((Byte const*)p, u, ds, DataStoreParserFlags::kLeaveFilePathAsString));

	// Reserialize to ensure.
	{
		String s;
		ds.ToString(ds.GetRootNode(), s, false, 0, true);

		DataStore newDs;
		SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromString(s, newDs, DataStoreParserFlags::kLeaveFilePathAsString));
		ds.Swap(newDs);
	}

	SharedPtr<DataStoreHint> pHint;
	SEOUL_UNITTESTING_ASSERT(DataStorePrinter::ParseHintsNoCopy((Byte const*)p, u, pHint));

	String s;
	DataStorePrinter::PrintWithHints(ds, pHint, s);

	SEOUL_UNITTESTING_ASSERT_EQUAL(s, String((Byte const*)p, u));

	MemoryManager::Deallocate(p);
	p = nullptr;
	u = 0u;
}

void DataStoreTest::TestDataStorePrinterOnModifiedFile()
{
	UnitTestsFileManagerHelper helper;

	String sActual;
	{
		void* p = nullptr;
		UInt32 u = 0u;
		SEOUL_UNITTESTING_ASSERT(FileManager::Get()->ReadAll(
			FilePath::CreateConfigFilePath("UnitTests/DataStorePrinter/C.json"),
			p,
			u,
			0u,
			MemoryBudgets::Developer));
		NormalizeLineEndings(p, u);

		DataStore ds;
		SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromString((Byte const*)p, u, ds, DataStoreParserFlags::kLeaveFilePathAsString));

		// Now modify.
		{
			DataNode value;
			SEOUL_UNITTESTING_ASSERT(ds.GetValueFromArray(ds.GetRootNode(), 18u, value));
			SEOUL_UNITTESTING_ASSERT(ds.GetValueFromArray(value, 3u, value));
			SEOUL_UNITTESTING_ASSERT(ds.SetStringToTable(value,
				HString("poergbtbghdgbaxg"),
				"pwtoosbygeekdadeqxlirziqpuavsybnutajjars"));

			SEOUL_UNITTESTING_ASSERT(ds.GetValueFromArray(ds.GetRootNode(), 19u, value));
			SEOUL_UNITTESTING_ASSERT(ds.GetValueFromArray(value, 3u, value));
			SEOUL_UNITTESTING_ASSERT(ds.SetBooleanValueToTable(value,
				HString("bjidlbdrg"),
				true));

			UInt32 uArrayCount = 0u;
			SEOUL_UNITTESTING_ASSERT(ds.GetArrayCount(ds.GetRootNode(), uArrayCount));

			// Erase the element 1 before last.
			SEOUL_UNITTESTING_ASSERT(ds.EraseValueFromArray(ds.GetRootNode(), uArrayCount - 2u));
			--uArrayCount;

			// Move all elements + 1.
			for (UInt32 i = uArrayCount - 1u; i >= 20u; --i)
			{
				SEOUL_UNITTESTING_ASSERT(ds.GetValueFromArray(ds.GetRootNode(), i, value));
				SEOUL_UNITTESTING_ASSERT(ds.SetNullValueToArray(ds.GetRootNode(), i + 1u));
				SEOUL_UNITTESTING_ASSERT(ds.DeepCopyToArray(ds, value, ds.GetRootNode(), i + 1u, true));
			}

			// "Insert" a new element at i.
			{
				DataStore ds2;
				SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromString(R"_feed_(
				[
					"$set",
					"ttnpfbpj",
					"bulucyqelq",
					{
						"zvnx": "bwewhktjfnuumh",
						"ezakyqot" : "socnyohyolfscsfvyfuqekfyyn",
						"ycecrebkyzajbnv" : "rgzcosmxwfdrxxwxrfrorugtammijvdmpxsq",
						"kbrocrdpfrjatkvv" : "yhgqgdvfhzqbuyl",
						"poergbtbghdgbaxg" : "oyfpwazkmeyicsarqnvlpe",
						"jgcdcgbszqlcskuk" : "gffqoujbfqmpftpwug",
						"fpsgv" : "yfmdhdohbqfywiuz",
						"eoldpznhfix" : 3,
						"zdkjhswqatbcwv" : 100,
						"kylmbhwdqwlam" : "vngjvvqwydtywkxxphasowmbleouppglcjmmnojttyjatzqhomcbozimzzvfmwfdtvubxp",
						"xvqdklihzjbukjzmu" : "qsmmlksdybyykqfmxeprvejndqhtqnwgivhlbjckbkhvavvewubbltlngohcyvaycbifchcpvscj",
						"rhmw" : "eyjlbjmgblbkhxktxdwueup",
						"gihrmzpoy" : "bulucyqelq",
						"bjidlbdrg" : true
					}
				])_feed_", ds2, DataStoreParserFlags::kLeaveFilePathAsString));
				SEOUL_UNITTESTING_ASSERT(ds.SetNullValueToArray(ds.GetRootNode(), 20u));
				SEOUL_UNITTESTING_ASSERT(ds.DeepCopyToArray(ds2, ds2.GetRootNode(), ds.GetRootNode(), 20u, true));
			}
		}

		SharedPtr<DataStoreHint> pHint;
		SEOUL_UNITTESTING_ASSERT(DataStorePrinter::ParseHintsNoCopy((Byte const*)p, u, pHint));

		DataStorePrinter::PrintWithHints(ds, pHint, sActual);
		MemoryManager::Deallocate(p);
		p = nullptr;
		u = 0u;
	}

	String sExpected;
	SEOUL_UNITTESTING_ASSERT(FileManager::Get()->ReadAll(
		FilePath::CreateConfigFilePath("UnitTests/DataStorePrinter/C_Modified.json"),
		sExpected));
	NormalizeLineEndings(sExpected);

	SEOUL_UNITTESTING_ASSERT_EQUAL(sActual, sExpected);
}

namespace
{

SharedPtr<DataStore> ResolveFail(const String& sFileName, Bool bResolveCommands)
{
	SEOUL_FAIL(String::Printf("Encountered unexpected $include '%s'", sFileName.CStr()).CStr());
	return SharedPtr<DataStore>();
}

} // namespace

void DataStoreTest::TestDataStorePrinterOnResolvedCommandsFile()
{
	UnitTestsFileManagerHelper helper;

	auto const aFiles = { "A", "B", "D" };

	for (auto const& sFile : aFiles)
	{
		String sActual;
		{
			auto const filePath(FilePath::CreateConfigFilePath(String::Printf("UnitTests/DataStorePrinter/%s.json", sFile)));

			void* p = nullptr;
			UInt32 u = 0u;
			SEOUL_UNITTESTING_ASSERT(FileManager::Get()->ReadAll(
				filePath,
				p,
				u,
				0u,
				MemoryBudgets::Developer));
			NormalizeLineEndings(p, u);

			DataStore ds;
			SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromString((Byte const*)p, u, ds, DataStoreParserFlags::kLeaveFilePathAsString));

			// Now resolve and print.
			SEOUL_UNITTESTING_ASSERT(DataStoreParser::ResolveCommandFile(
				SEOUL_BIND_DELEGATE(ResolveFail),
				filePath.GetAbsoluteFilename(),
				ds,
				ds,
				DataStoreParserFlags::kLeaveFilePathAsString));

			// Derive hinting from existing file.
			SharedPtr<DataStoreHint> pHint;
			SEOUL_UNITTESTING_ASSERT(DataStorePrinter::ParseHintsNoCopyWithFlattening((Byte const*)p, u, pHint));
			SEOUL_UNITTESTING_ASSERT(pHint.IsValid());

			// Pretty print with DataStorePrinter.
			DataStorePrinter::PrintWithHints(ds, pHint, sActual);
			MemoryManager::Deallocate(p);
			p = nullptr;
			u = 0u;
		}

		String sExpected;
		SEOUL_UNITTESTING_ASSERT(FileManager::Get()->ReadAll(
			FilePath::CreateConfigFilePath(String::Printf("UnitTests/DataStorePrinter/%s_Resolved.json", sFile)),
			sExpected));
		NormalizeLineEndings(sExpected);

		SEOUL_UNITTESTING_ASSERT_EQUAL(sActual, sExpected);
	}
}

void DataStoreTest::TestDataStoreCompactHandlesEmpty()
{
	DataStore datastore;
	datastore.UnitTestHook_CallInternalCompactHandleOffsets();
	SEOUL_UNITTESTING_ASSERT(datastore.VerifyIntegrity());
}

namespace
{

// Capture of SaveLoadManager::LoadLocalData for
// testing.
static const UInt32 kuSaveContainerSignature = 0x27eadb42;
static const Int32 kiMaxSaveContainerVersion = 3;
static const Int32 kiMinSaveContainerVersion = 3;

static const UInt8 kauKey[32] =
{
	0xba, 0x18, 0xf6, 0x75, 0xde, 0x71, 0xac, 0x61,
	0x5a, 0x3e, 0x15, 0xf8, 0xbc, 0x9f, 0xb0, 0xb3,
	0x25, 0x38, 0x58, 0xf3, 0x6f, 0x01, 0xa0, 0xd5,
	0xc7, 0xa2, 0x76, 0x45, 0x9c, 0x4f, 0xbf, 0x5f,
};

/**
 * Local utility used in a few loading paths. Reads and decompresses
 * a DataStore.
 *
 * @param[in] rBuffer Data blob to read the compressed DataStore from.
 * @param[out] rDataStore Output DataStore. Left unmodified on failure.
 *
 * @return kSuccess on success, or various error codes on failure.
 */
static SaveLoadResult ReadDataStore(StreamBuffer& rBuffer, DataStore& rDataStore)
{
	// Read header data.
	UInt32 uUncompressedDataSizeInBytes = 0u;
	UInt32 uCompressedDataSizeInBytes = 0u;
	if (!rBuffer.Read(uUncompressedDataSizeInBytes) ||
		uUncompressedDataSizeInBytes > SaveLoadUtil::kuMaxDataSizeInBytes ||
		!rBuffer.Read(uCompressedDataSizeInBytes) ||
		uCompressedDataSizeInBytes > SaveLoadUtil::kuMaxDataSizeInBytes)
	{
		return SaveLoadResult::kErrorTooBig;
	}

	// Decompress the data.
	Vector<Byte, MemoryBudgets::Saving> vUncompressedData(uUncompressedDataSizeInBytes);
	if (!vUncompressedData.IsEmpty())
	{
		if (!ZlibDecompress(
			rBuffer.GetBuffer() + rBuffer.GetOffset(),
			uCompressedDataSizeInBytes,
			vUncompressedData.Data(),
			uUncompressedDataSizeInBytes))
		{
			return SaveLoadResult::kErrorCompression;
		}
		else
		{
			// Advance passed the data we just consumed.
			rBuffer.SeekToOffset(rBuffer.GetOffset() + uCompressedDataSizeInBytes);
		}
	}

	// If we get here successfully, data is now an array of the uncompressed
	// DataStore serialized data, so we need to deserialize it into a
	// DataStore object.
	DataStore dataStore;
	if (!vUncompressedData.IsEmpty())
	{
		FullyBufferedSyncFile file(vUncompressedData.Data(), vUncompressedData.GetSizeInBytes(), false);
		if (!dataStore.Load(file))
		{
			return SaveLoadResult::kErrorSaveData;
		}

		if (!dataStore.VerifyIntegrity())
		{
			return SaveLoadResult::kErrorSaveCheck;
		}
	}

	// Done success, swap in the output DataStore.
	rDataStore.Swap(dataStore);
	return SaveLoadResult::kSuccess;
}

static SaveLoadResult LoadLocalDataPC(
	StreamBuffer& data,
	DataStore& rSaveData,
	DataStore& rPendingDelta)
{
	// Signature check.
	UInt32 uSignature = 0u;
	if (!data.Read(uSignature)) { return SaveLoadResult::kErrorSignatureData; }
	if (uSignature != kuSaveContainerSignature) { return SaveLoadResult::kErrorSignatureCheck; }

	// If signature check was succesful, version check.
	Int32 iVersion = -1;
	if (!data.Read(iVersion)) { return SaveLoadResult::kErrorVersionData; }
	if (!(iVersion >= kiMinSaveContainerVersion && iVersion <= kiMaxSaveContainerVersion)) { return SaveLoadResult::kErrorVersionCheck; }

	// If version check was successful, decrypt the data.
	UInt32 uChecksumOffset = 0u;
	UInt8 auNonce[EncryptAES::kEncryptionNonceLength];
	if (!data.Read(auNonce, sizeof(auNonce)))
	{
		return SaveLoadResult::kErrorEncryption;
	}
	else
	{
		uChecksumOffset = data.GetOffset();
		for (auto const& e : auNonce)
		{
			SEOUL_LOG("%d", (Int)e);
		}
		EncryptAES::DecryptInPlace(
			data.GetBuffer() + uChecksumOffset,
			data.GetTotalDataSizeInBytes() - uChecksumOffset,
			kauKey,
			sizeof(kauKey),
			auNonce);
	}

	// Read and verify the checksum.
	UInt8 auChecksum[EncryptAES::kSHA512DigestLength];
	if (!data.Read(auChecksum, sizeof(auChecksum)))
	{
		return SaveLoadResult::kErrorChecksumCheck;
	}
	else
	{
		// Verify the checksum -- since the checksum was originally computed
		// with the checksum bytes set to 0, we need to set them back to 0 to
		// verify.
		memset(data.GetBuffer() + uChecksumOffset, 0, sizeof(auChecksum));
		UInt8 auComputedChecksum[EncryptAES::kSHA512DigestLength];
		EncryptAES::SHA512Digest(
			data.GetBuffer(),
			data.GetTotalDataSizeInBytes(),
			auComputedChecksum);

		if (0 != memcmp(auChecksum, auComputedChecksum, sizeof(auChecksum)))
		{
			for (auto const& e : auChecksum)
			{
				SEOUL_LOG("%d", (Int)e);
			}

			return SaveLoadResult::kErrorChecksumCheck;
		}
	}

	// Discard metadata.
	{
		DataStore metadata;
		auto eResult = ReadDataStore(data, metadata);
		if (SaveLoadResult::kSuccess != eResult)
		{
			return eResult;
		}
	}

	// Read the checkpoint DataStore.
	DataStore saveData;
	{
		auto const eResult = ReadDataStore(data, saveData);
		if (SaveLoadResult::kSuccess != eResult)
		{
			return eResult;
		}
	}

	// Read the pending delta.
	DataStore pendingDelta;
	{
		auto const eResult = ReadDataStore(data, pendingDelta);
		if (SaveLoadResult::kSuccess != eResult)
		{
			return eResult;
		}
	}

	// Verify that we completely consumed the input data.
	if (data.GetOffset() != data.GetTotalDataSizeInBytes())
	{
		return SaveLoadResult::kErrorExtraData;
	}

	// Populate and return success.
	rSaveData.Swap(saveData);
	rPendingDelta.Swap(pendingDelta);
	return SaveLoadResult::kSuccess;
}

} // namespace anonymous

void DataStoreTest::TestDataStoreCompactHandlesLargeData()
{
	UnitTestsFileManagerHelper helper;

	DataStore serverSaveDataStore;
	SaveLoadUtil::SaveFileMetadata serverSaveMetadata;
	Vector<Byte> serverSaveBytes;
	SEOUL_UNITTESTING_ASSERT(FileManager::Get()->ReadAll(
		FilePath::CreateRawFilePath(Seoul::GameDirectory::kConfig, "UnitTests/DataStore/LargePersistentTestSave/serverSave.dsr"),
		serverSaveBytes));

	SEOUL_UNITTESTING_ASSERT(SaveLoadResult::kSuccess == SaveLoadUtil::FromBlob(
		serverSaveBytes.Data(),
		serverSaveBytes.GetSize(),
		serverSaveMetadata,
		serverSaveDataStore));

	Vector<Byte> localSaveBytes;
	SEOUL_UNITTESTING_ASSERT(FileManager::Get()->ReadAll(
		FilePath::CreateConfigFilePath("UnitTests/DataStore/LargePersistentTestSave/player-save-test.dat"),
		localSaveBytes));

	StreamBuffer saveDeltaBuffer;
	saveDeltaBuffer.Write(localSaveBytes.Data(), localSaveBytes.GetSize());
	saveDeltaBuffer.SeekToOffset(0);
	DataStore saveDataUNUSED;
	DataStore saveDeltaDataStore;

	SEOUL_UNITTESTING_ASSERT(SaveLoadResult::kSuccess == LoadLocalDataPC(saveDeltaBuffer, saveDataUNUSED, saveDeltaDataStore));
	SEOUL_UNITTESTING_ASSERT(ApplyDiff(saveDeltaDataStore, serverSaveDataStore));
}

extern Bool g_bUnitTestOnlyDisableDataStoreHandleCompactionOnLoad;

// Test for a regression - compact handles contained an unprotected call to Vector<>::Get()
// that could erroneously trigger an assertion failure if the target container at the offset
// of the Get() was at the end of the DataStore data.
void DataStoreTest::TestDataStoreCompactHandlesRegression()
{
	UnitTestsFileManagerHelper helper;

	// Must disable until a certain point to reproduce this bug.
	g_bUnitTestOnlyDisableDataStoreHandleCompactionOnLoad = true;

	DataStore dataStore;
	SaveLoadUtil::SaveFileMetadata unusedMetadata;
	Vector<Byte> bytes;
	SEOUL_UNITTESTING_ASSERT(FileManager::Get()->ReadAll(
		FilePath::CreateRawFilePath(Seoul::GameDirectory::kConfig, "UnitTests/DataStore/CompactHandlesRegression/container_at_data_end_after_gc.dsr"),
		bytes));

	SEOUL_UNITTESTING_ASSERT(SaveLoadResult::kSuccess == SaveLoadUtil::FromBlob(
		bytes.Data(),
		bytes.GetSize(),
		unusedMetadata,
		dataStore));

	// This part is necessary and sufficient but not strictly "minimal" - the step
	// that actually introduces the bug is trigger the GC on dataStore. Doing it
	// this way to reproduce the exact original steps of the bug.
	{
		String sTemp;
		SEOUL_UNITTESTING_ASSERT(SaveLoadResult::kSuccess == SaveLoadUtil::ToBase64(
			unusedMetadata,
			dataStore,
			sTemp));

		// Enable now to reproduce.
		g_bUnitTestOnlyDisableDataStoreHandleCompactionOnLoad = false;

		// With the bug present, this call will SEOUL_ASSERT() in DataStore::InternalCompactHandleOffsetsInner().
		SEOUL_UNITTESTING_ASSERT(SaveLoadResult::kSuccess == SaveLoadUtil::FromBase64(
			sTemp,
			unusedMetadata,
			dataStore));
	}

	// Just for completeness.
	SEOUL_UNITTESTING_ASSERT(dataStore.VerifyIntegrity());
}

// Test for a bug where DataStore::Save() would not consistently
// generate the same binary output given the same DataStore
// state.
void DataStoreTest::TestDataStoreBinaryDeterminismRegression()
{
	static const String ksData =
R"({
	"UpsellTextDataConfig": [
		/*
		{
		"UpsellTextToken": "UI_ConversionOfferUpsellText3",
		"UpsellTextXOffset": 360,
		"UpsellTextYOffset": 20,
		"UpsellTextRotation": 0,
		},
		*/
		{
			"UpsellTextToken": "UI_Bundle_5xValue",
			"UpsellTextXOffset": 95,
			"UpsellTextYOffset": -116,

			//negative values move UP
			"UpsellTextRotation": -10,
			"UpsellTextXScale": 1.5,
			"UpsellTextYScale": 1.5
		},
		{
			"UpsellTextToken": "UI_Bundle_SetValue",
			"UpsellTextXOffset": 125,
			"UpsellTextYOffset": -18,

			//negative values move UP
			"UpsellTextRotation": -10,
			"UpsellTextXScale": 1.5,
			"UpsellTextYScale": 1.5
		},
		{
			"UpsellTextToken": "UI_Bundle_GemConversionNameLevel40",
			"UpsellTextXOffset": 365,
			"UpsellTextYOffset": 22,

			//negative values move UP
			"UpsellTextRotation": 0,
			"UpsellTextXScale": 1,
			"UpsellTextYScale": 1
		}
	],
	"BackgroundArtConfig": [
		{
			"ImageFilePath": "content://Authored/Textures/BundleArt/Bundle_GemConversion_L20_A.png",
			"XSize": 532,
			"YSize": 862,
			"XOffset": -145,
			"YOffset": -638,
			"Rotation": 0
		},
		{
			"ImageFilePath": "content://Authored/Textures/SpriteSheet-assets/Panels/BannerRedYellow_BundleDetailsPanel.png",
			"XSize": 760,
			"YSize": 90,
			"XOffset": -253.93,
			"YOffset": -693.82,
			"Rotation": 0
		}
	],
	"DetailViewUpsellTextDataConfig": [
		{
			"UpsellTextToken": "UI_Bundle_GemConversionHeading",
			"UpsellTextXOffset": 0,
			"UpsellTextYOffset": -195.28,

			//negative values move UP
			"UpsellTextRotation": 0,
			"UpsellTextXScale": 1.5,
			"UpsellTextYScale": 1.5
		},
		{
			"UpsellTextToken": "UI_Bundle_GemConversionSubheading",
			"UpsellTextXOffset": 0,
			"UpsellTextYOffset": -130.28,

			//negative values move UP
			"UpsellTextRotation": 0,
			"UpsellTextXScale": 1.5,
			"UpsellTextYScale": 1.5
		},

		/*
		{
		"UpsellTextToken": "UI_Bundle_GemConversionTitle",
		"UpsellTextXOffset": 0,
		"UpsellTextYOffset": -69.28,  //negative values move UP
		"UpsellTextRotation": 0,
		"UpsellTextXScale": 1.85,
		"UpsellTextYScale": 1.85,  //Scale controls
		},
		*/
		{
			"UpsellTextToken": "UI_Bundle_GemBarrelQuantity2",
			"UpsellTextXOffset": -126.11,
			"UpsellTextYOffset": 200.75,

			//negative values move UP
			"UpsellTextRotation": 0,
			"UpsellTextXScale": 1.19,
			"UpsellTextYScale": 1.19
		},
		{
			"UpsellTextToken": "UI_Bundle_GemBarrelQuantity2",
			"UpsellTextXOffset": 108.81,
			"UpsellTextYOffset": 200.75,

			//negative values move UP
			"UpsellTextRotation": 0,
			"UpsellTextXScale": 1.19,
			"UpsellTextYScale": 1.19
		},
		{
			"UpsellTextToken": "UI_Bundle_GemBarrelName",
			"UpsellTextXOffset": -127.56,
			"UpsellTextYOffset": 140,

			//negative values move UP
			"UpsellTextRotation": 0,
			"UpsellTextXScale": 0.87,
			"UpsellTextYScale": 0.87
		},
		{
			"UpsellTextToken": "UI_Bundle_GemBarrelName",
			"UpsellTextXOffset": 109.02,
			"UpsellTextYOffset": 140,

			//negative values move UP
			"UpsellTextRotation": 0,
			"UpsellTextXScale": 0.87,
			"UpsellTextYScale": 0.87
		},
		{
			"UpsellTextToken": "UI_Bundle_GemConversionChest",
			"UpsellTextXOffset": 0.02,
			"UpsellTextYOffset": 459.15,

			//negative values move UP
			"UpsellTextRotation": 0,
			"UpsellTextXScale": 0.87,
			"UpsellTextYScale": 0.87
		},
		{
			"UpsellTextToken": "UI_Bundle_500Percent",
			"UpsellTextXOffset": 209.74,
			"UpsellTextYOffset": 665,

			//negative values move UP
			"UpsellTextRotation": 0,
			"UpsellTextXScale": 0.98,
			"UpsellTextYScale": 0.98
		},
		{
			"UpsellTextToken": "UI_Bundle_ValueLargeFontSize",
			"UpsellTextXOffset": 212.84,
			"UpsellTextYOffset": 746.23,

			//negative values move UP
			"UpsellTextRotation": 0,
			"UpsellTextXScale": 0.84,
			"UpsellTextYScale": 0.84
		}
	],
	"DetailBackgroundFrame": "Yellow",
	"FormatToken": "BundleBlue_Style",
	"StoreBundleImage": {
		"FilePath": "content://Authored/Textures/BundleSheet-assets/Bnd_1up_LevelOffer.png",
		"Offset": {"X": 0, "Y": -23},
		"Size": {"X": 1120, "Y": 576}
	}
})";

	DataStore dataStore;
	SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromString(ksData, dataStore));

	for (Int i = 0; i < 10; ++i)
	{
		MemorySyncFile a;
		SEOUL_UNITTESTING_ASSERT(dataStore.Save(a, keCurrentPlatform));
		SEOUL_UNITTESTING_ASSERT(a.Seek(0, File::kSeekFromStart));
		DataStore dataStore2;
		SEOUL_UNITTESTING_ASSERT(dataStore2.Load(a));
		MemorySyncFile b;
		SEOUL_UNITTESTING_ASSERT(dataStore2.Save(b, keCurrentPlatform));

		SEOUL_UNITTESTING_ASSERT_EQUAL(a.GetBuffer().GetTotalDataSizeInBytes(), b.GetBuffer().GetTotalDataSizeInBytes());
		SEOUL_UNITTESTING_ASSERT(0 == memcmp(
			a.GetBuffer().GetBuffer(),
			b.GetBuffer().GetBuffer(),
			a.GetBuffer().GetTotalDataSizeInBytes()));

		dataStore.Swap(dataStore2);
	}

	{
		DataStore dataStore2;
		SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromString(ksData, dataStore2));

		MemorySyncFile a;
		SEOUL_UNITTESTING_ASSERT(dataStore.Save(a, keCurrentPlatform));
		MemorySyncFile b;
		SEOUL_UNITTESTING_ASSERT(dataStore2.Save(b, keCurrentPlatform));

		SEOUL_UNITTESTING_ASSERT_EQUAL(a.GetBuffer().GetTotalDataSizeInBytes(), b.GetBuffer().GetTotalDataSizeInBytes());
		SEOUL_UNITTESTING_ASSERT(0 == memcmp(
			a.GetBuffer().GetBuffer(),
			b.GetBuffer().GetBuffer(),
			a.GetBuffer().GetTotalDataSizeInBytes()));
	}
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
