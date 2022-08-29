/**
 * \file IOSUtilTest.h
 * \brief Unit tests for iOS specific utilities.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef IOS_UTIL_TEST_H
#define IOS_UTIL_TEST_H

#include "Prereqs.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class IOSUtilTest SEOUL_SEALED
{
public:
	void TestConvertToDataStore();
	void TestConvertToNSDictionary();
	void TestConvertToDataStoreArrayFull();
	void TestConvertToNSDictionaryArrayFull();
	void TestToDataStoreFails();

	void TestConvertToDataStoreNumberKeys();
	void TestConvertToNSDictionaryNumberKeys();
};

#endif // /#if SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
