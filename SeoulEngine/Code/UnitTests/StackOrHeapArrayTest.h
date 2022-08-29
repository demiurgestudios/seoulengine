/**
 * \file StackOrHeapArrayTest.h
 * \brief Unit test header file for the Seoul vector class
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef STACK_OR_HEAP_ARRAY_TEST_H
#define STACK_OR_HEAP_ARRAY_TEST_H

#include "Prereqs.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class StackOrHeapArrayTest SEOUL_SEALED
{
public:
	void TestIterators();
	void TestMethodsHeap();
	void TestMethodsStack();
	void TestStackSized();
};

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif  // def VECTOR_TEST_H
