/**
 * \file AtomicTest.h
 * \brief Unit tests for Seoul engine Atomic types. These include
 * Atomic32, AtomicPointer, and AtomicRingBuffer, which provide
 * thread-safe, lockless types and data structures.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ATOMIC_TEST_H
#define ATOMIC_TEST_H

#include "Prereqs.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class AtomicTest SEOUL_SEALED
{
public:
	void TestAtomic32Basic();
	void TestAtomic32MultipleThread();
	void TestAtomic32ValueBasic();
	void TestAtomic32ValueMultipleThread();
	void TestAtomic32ValueFloat32Regression();
	void TestAtomic32ValueBoolRegression();
	void TestAtomic32ValueMax();
	void TestAtomic32ValueMin();
	void TestAtomic32ValueNeg0();
	void TestAtomic32ValueNaN();
	void TestAtomic64Basic();
	void TestAtomic64MultipleThread();
	void TestAtomic64ValueBasic();
	void TestAtomic64ValueMultipleThread();
	void TestAtomic64ValueFloat32Regression();
	void TestAtomic64ValueFloat64Regression();
	void TestAtomic64ValueBoolRegression();
	void TestAtomic64ValueMaxInt32();
	void TestAtomic64ValueMaxInt64();
	void TestAtomic64ValueMinInt32();
	void TestAtomic64ValueMinInt64();
	void TestAtomic64ValueFloat32Neg0();
	void TestAtomic64ValueFloat64Neg0();
	void TestAtomic64ValueFloat32NaN();
	void TestAtomic64ValueFloat64NaN();
	void TestAtomicPointer();
	void TestAtomicPointerMultipleThread();
	void TestAtomicRingBufferSingleThread();
	void TestAtomicRingBufferIdenticalValue();
	void TestAtomicRingBufferFull();
}; // class AtomicTest

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif  // include guard
