/**
 * \file FixedArrayTest.h
 * \brief Unit test header file for the Seoul vector class
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FIXED_ARRAY_TEST_H
#define FIXED_ARRAY_TEST_H

#include "Prereqs.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class FixedArrayTest SEOUL_SEALED
{
public:
	void TestAssign();
	void TestCopyConstructor();
	void TestDefaultConstructor();
	void TestMethods();
	void TestIterators();
	void TestValueConstructor();
	void TestRangedFor();
};

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif  // def VECTOR_TEST_H
