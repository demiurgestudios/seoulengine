/**
 * \file PerThreadStorageTest.h
 *
 * \brief Unit tests for Seoul engine per thread storage and heap
 * allocated per thread storage. Per thread storage is a special
 * feature that allows a single variable to be defined and have
 * the value differ per thread that access the value.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef PER_THREAD_STORAGE_TEST_H
#define PER_THREAD_STORAGE_TEST_H

#include "Prereqs.h"
#include "HeapAllocatedPerThreadStorage.h"
#include "PerThreadStorage.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class PerThreadStorageTest SEOUL_SEALED
{
public:
	void TestPerThreadStorage();
	void TestHeapAllocatedPerThreadStorage();
}; // class PerThreadStorageTest

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif  // include guard
