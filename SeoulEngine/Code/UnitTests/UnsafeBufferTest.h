/**
 * \file UnsafeBufferTest.h
 * \brief Unit test header file for the Seoul UnsafeBuffer<> class
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef UNSAFE_BUFFER_TEST_H
#define UNSAFE_BUFFER_TEST_H

#include "Prereqs.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class UnsafeBufferTest SEOUL_SEALED
{
public:
	void TestAppendBuiltin();
	void TestAppendSimple();
	void TestAssignBuiltin();
	void TestAssignSimple();
	void TestBasic();
	void TestClearBuiltin();
	void TestClearSimple();
	void TestConstructorBuiltin();
	void TestConstructorSimple();
	void TestEmptyBuiltin();
	void TestEmptySimple();
	void TestEqualityBuiltin();
	void TestEqualitySimple();
	void TestMethods();
	void TestIterators();
};

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
