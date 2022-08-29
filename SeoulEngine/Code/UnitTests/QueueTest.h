/**
 * \file QueueTest.h
 * \brief Unit test header file for the Seoul Queue<> class
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef QUEUE_TEST_H
#define QUEUE_TEST_H

#include "Prereqs.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class QueueTest SEOUL_SEALED
{
public:
	void TestBasic();
	void TestClearBuiltin();
	void TestClearComplex();
	void TestClearSimple();
	void TestConstructorBuiltin();
	void TestConstructorComplex();
	void TestConstructorSimple();
	void TestEmptyBuiltin();
	void TestEmptyComplex();
	void TestEmptySimple();
	void TestFind();
	void TestMethods();
	void TestIterators();
};

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
