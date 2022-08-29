/**
 * \file MemoryManagerTest.h
 * \brief Memory Manager unit test fixture.
 *
 * This header defines the unit test fixture for Seoul's Memory Manager.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef MEMORY_MANAGER_TEST_H
#define MEMORY_MANAGER_TEST_H

#include "Prereqs.h"
#include "MemoryManager.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

/**
 * Text fixture class for Memory Manager.
 */
class MemoryManagerTest SEOUL_SEALED
{
public:
	void TestMemoryManager();
	void TestGetAllocationSizeAndAlignment();
	void TestReallocRegression();
}; // class ThreadFriendlyMemoryAllocatorTest

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
