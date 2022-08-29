/**
 * \file SeoulUtilTest.h
 * \brief Unit test header file for general Seoul utility functions
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SEOUL_UTIL_TEST_H
#define SEOUL_UTIL_TEST_H

#include "Prereqs.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class SeoulUtilTest SEOUL_SEALED
{
public:
	void TestCompareVersionStrings();
	void TestMD5();
	void TestUUID();
};

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
