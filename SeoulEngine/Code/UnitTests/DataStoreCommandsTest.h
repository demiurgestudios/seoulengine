/**
 * \file DataStoreCommandsTest.h
 * \brief Unit test header file for DataStore
 * commands functionality.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef DATA_STORE_COMMANDS_TEST_H
#define DATA_STORE_COMMANDS_TEST_H

#include "Prereqs.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class DataStoreCommandsTest SEOUL_SEALED
{
public:
	void TestAppendToExistingArrayInArray();
	void TestAppendToImplicitArrayInArray();
	void TestBasic();
	void TestBasic2();
	void TestErrors();
	void TestInheritance();
	void TestMutations();
	void TestMutationImplicit();
	void TestOverwriteInArray();
	void TestOverwriteInTable();
	void TestSearch();
};

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif	// include guard
