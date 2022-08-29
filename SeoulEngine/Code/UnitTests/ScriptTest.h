/**
 * \file ScriptTest.h
 * \brief Declaration of unit tests for the Script functionality.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCRIPT_TEST_H
#define SCRIPT_TEST_H

#include "Prereqs.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class ScriptTest SEOUL_SEALED
{
public:
	void TestAny();
	void TestArrayIndex();
	void TestBasic();
	void TestBindStrongInstance();
	void TestBindStrongTable();
	void TestDataStore();
	void TestDataStoreNilConversion();
	void TestDataStorePrimitives();
	void TestDataStoreSpecial();
	void TestInterfaceArgs();
	void TestInterfaceArgsMultiple();
	void TestInterfaceFilePath();
	void TestInterfaceRaiseError();
	void TestInterfaceReturn();
	void TestInterfaceReturnMultiple();
	void TestInterfaceUserData();
	void TestInterfaceUserDataType();
	void TestInvokeArgs();
	void TestInvokeArgsMultiple();
	void TestInvokeFilePath();
	void TestInvokeReturn();
	void TestInvokeReturnMultiple();
	void TestInvokeUserData();
	void TestInvokeUserDataType();
	void TestMultiVmClone();
	void TestNullObject();
	void TestNullScriptVmObject();
	void TestNumberRanges();
	void TestReflectionArgs();
	void TestReflectionMultiSuccess();
	void TestReflectionReturn();
	void TestReflectionTypes();
	void TestSetGlobal();
	void TestWeakBinding();
	void TestRandom();

	void TestI32AddNV();
	void TestI32DivNV();
	void TestI32ModExtensionNV();
	void TestI32MulExtensionNV();
	void TestI32SubNV();

	void TestI32AddVN();
	void TestI32DivVN();
	void TestI32ModExtensionVN();
	void TestI32MulExtensionVN();
	void TestI32SubVN();

	void TestI32AddVV();
	void TestI32DivVV();
	void TestI32ModExtensionVV();
	void TestI32MulExtensionVV();
	void TestI32SubVV();

	void TestI32Truncate();

	void TestI32ModExtensionWrongTypes();
	void TestI32MulExtensionWrongTypes();
	void TestI32TruncateWrongTypes();
}; // class ScriptTest

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
