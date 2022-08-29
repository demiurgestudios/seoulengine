/**
 * \file AtomicHandleTest.h
 * \brief Unit test implementations for the AtomicHandle and
 * AtomicHandleTable classes.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ATOMIC_HANDLE_TEST_H
#define ATOMIC_HANDLE_TEST_H

#include "Prereqs.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class AtomicHandleTest SEOUL_SEALED
{
public:
	void TestAllocation();
	void TestEquality();
	void TestHandleTable();
	void TestHandleTableThreaded();
}; // class AtomicHandleTest

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
