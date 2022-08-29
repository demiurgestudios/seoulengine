/**
 * \file MemoryManagerTest.cpp
 * \brief Contains the unit test implementations for the Memory Manager.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Logger.h"
#include "MemoryManagerTest.h"
#include "ScopedPtr.h"
#include "SeoulMath.h"
#include "Thread.h"
#include "ReflectionDefine.h"
#include "UnitTesting.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(MemoryManagerTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestMemoryManager)
	SEOUL_METHOD(TestGetAllocationSizeAndAlignment)
	SEOUL_METHOD(TestReallocRegression)
SEOUL_END_TYPE()

struct MemoryManagerTestUtility
{
	SEOUL_DELEGATE_TARGET(MemoryManagerTestUtility);

	PerThreadStorage m_PerThreadStorageHeapAllocatedUInt32;
	PerThreadStorage m_PerThreadStorageUInt32;

	MemoryManagerTestUtility()
	{
	}

	void Allocate()
	{
		SEOUL_UNITTESTING_ASSERT(!m_PerThreadStorageHeapAllocatedUInt32.GetPerThreadStorage());
		SEOUL_UNITTESTING_ASSERT(!m_PerThreadStorageUInt32.GetPerThreadStorage());
		void* pData = MemoryManager::AllocateAligned(
			sizeof(UInt32),
			MemoryBudgets::TBD,
			SEOUL_ALIGN_OF(UInt32));

		UInt32 uValue = GlobalRandom::UniformRandomUInt32();

		SEOUL_UNITTESTING_ASSERT(pData);
		*reinterpret_cast<UInt32*>(pData) = uValue;

		m_PerThreadStorageUInt32.SetPerThreadStorage((void*)((size_t)uValue));
		m_PerThreadStorageHeapAllocatedUInt32.SetPerThreadStorage(pData);
	}

	void Reallocate()
	{
		void* pData = m_PerThreadStorageHeapAllocatedUInt32.GetPerThreadStorage();
		SEOUL_UNITTESTING_ASSERT(pData);

		UInt32 uValue = (UInt32)((size_t)m_PerThreadStorageUInt32.GetPerThreadStorage());
		SEOUL_UNITTESTING_ASSERT(*reinterpret_cast<UInt32*>(pData) == uValue);

		void* pNewData = nullptr;

		// We don't control the underlying allocator on iOS, so this is not assured.
#if !SEOUL_PLATFORM_IOS
		pNewData = MemoryManager::ReallocateAligned(
			pData,
			sizeof(UInt32),
			SEOUL_ALIGN_OF(UInt32),
			MemoryBudgets::TBD);
		SEOUL_UNITTESTING_ASSERT(pData == pNewData);
#endif // /#if !SEOUL_PLATFORM_IOS

		pNewData = MemoryManager::ReallocateAligned(
			pData,
			sizeof(UInt32) + 1u,
			SEOUL_ALIGN_OF(UInt32),
			MemoryBudgets::TBD);

		SEOUL_UNITTESTING_ASSERT(pNewData);

		uValue = GlobalRandom::UniformRandomUInt32();
		*reinterpret_cast<UInt32*>(pNewData) = uValue;

		m_PerThreadStorageUInt32.SetPerThreadStorage((void*)((size_t)uValue));
		m_PerThreadStorageHeapAllocatedUInt32.SetPerThreadStorage(pNewData);
	}

	void Deallocate()
	{
		void* pData = m_PerThreadStorageHeapAllocatedUInt32.GetPerThreadStorage();
		SEOUL_UNITTESTING_ASSERT(pData);

		UInt32 uValue = (UInt32)((size_t)m_PerThreadStorageUInt32.GetPerThreadStorage());
		SEOUL_UNITTESTING_ASSERT(*reinterpret_cast<UInt32*>(pData) == uValue);

		m_PerThreadStorageUInt32.SetPerThreadStorage(nullptr);
		m_PerThreadStorageHeapAllocatedUInt32.SetPerThreadStorage(nullptr);

		MemoryManager::Deallocate(pData);
	}

	Int Run(const Thread& thread)
	{
		Allocate();
		Thread::YieldToAnotherThread();
		Reallocate();
		Thread::YieldToAnotherThread();
		Deallocate();
		Thread::YieldToAnotherThread();
		return 0;
	}
};

void MemoryManagerTest::TestMemoryManager()
{
	static const Int32 kTestThreadCount = 50;

	MemoryManagerTestUtility test;

	ScopedPtr<Thread> apThreads[kTestThreadCount];

	for (Int32 i = 0; i < kTestThreadCount; ++i)
	{
		apThreads[i].Reset(SEOUL_NEW(MemoryBudgets::TBD) Thread(
			SEOUL_BIND_DELEGATE(&MemoryManagerTestUtility::Run, &test)));
	}

	for (Int32 i = 0; i < kTestThreadCount; ++i)
	{
		SEOUL_VERIFY(apThreads[i]->Start());
	}

	for (Int32 i = 0; i < kTestThreadCount; ++i)
	{
		apThreads[i]->WaitUntilThreadIsNotRunning();
	}
}

void MemoryManagerTest::TestGetAllocationSizeAndAlignment()
{
	// Allocate a block and perform some API on it, and then destroy it.
	//
	// Doing this to make sure GetAllocationSizeInBytes() and reallocations with alignment work
	// as expected on all platforms - implementation of these methods became complicated and device
	// dependent when we stopped using a custom allocator on Android, and we don't want that testing
	// to get missed if (e.g.) unit tests are not running on Android, etc.
	Byte aTestData[] = { 5, 1, 2, 8, 2, 0, 3, 2, 3, 5, 9, 3, 27, 5, 9, 7 };
	for (size_t z = 0; z < sizeof(aTestData) + 1; ++z)
	{
		void* p = MemoryManager::AllocateAligned(z, MemoryBudgets::Developer, 16u);
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(nullptr, p);
		SEOUL_UNITTESTING_ASSERT_EQUAL((size_t)p % 16u, 0u);
		memcpy(p, aTestData, z);
		SEOUL_UNITTESTING_ASSERT_GREATER_EQUAL(MemoryManager::GetAllocationSizeInBytes(p), z);
		SEOUL_UNITTESTING_ASSERT_LESS_EQUAL(MemoryManager::GetAllocationSizeInBytes(p), 128);
		p = MemoryManager::ReallocateAligned(p, z, 32u, MemoryBudgets::Developer);
		if (0 == z)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(nullptr, p);
		}
		else
		{
			SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(nullptr, p);
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL((size_t)p % 32u, 0u);
		SEOUL_ASSERT(0 == memcmp(p, aTestData, z));
		p = MemoryManager::ReallocateAligned(p, z + 25, 64u, MemoryBudgets::Developer);
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(nullptr, p);
		SEOUL_UNITTESTING_ASSERT_EQUAL((size_t)p % 64u, 0u);
		SEOUL_ASSERT(0 == memcmp(p, aTestData, z));
		SEOUL_UNITTESTING_ASSERT_GREATER_EQUAL(MemoryManager::GetAllocationSizeInBytes(p), z + 25);
		SEOUL_UNITTESTING_ASSERT_LESS_EQUAL(MemoryManager::GetAllocationSizeInBytes(p), 128);
		MemoryManager::Deallocate(p);
	}
}

// Regression for an edge case of Reallocate() - calling standard realloc(p, 0)
// if p is not nullptr will return a nullptr, which prior to the fix would
// trigger an assertion failure/crash (due to MemoryManager treating this as
// OOM).
void MemoryManagerTest::TestReallocRegression()
{
	// No alignment.
	{
		void* p = MemoryManager::Allocate(1u, MemoryBudgets::TBD);
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(nullptr, p);
		p = MemoryManager::Reallocate(p, 0u, MemoryBudgets::TBD);
		SEOUL_UNITTESTING_ASSERT_EQUAL(nullptr, p);
	}
	// Alignment explicit, small alignment.
	{
		void* p = MemoryManager::AllocateAligned(1u, MemoryBudgets::TBD, 8u); // Need to be our known minimum of all minimum platform alignments.
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(nullptr, p);
		p = MemoryManager::ReallocateAligned(p, 0u, 8u, MemoryBudgets::TBD);
		SEOUL_UNITTESTING_ASSERT_EQUAL(nullptr, p);
	}
	// Alignment explicit, large alignment.
	{
		void* p = MemoryManager::AllocateAligned(32u, MemoryBudgets::TBD, 32u); // Need to be our known minimum of all minimum platform alignments.
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(nullptr, p);
		p = MemoryManager::ReallocateAligned(p, 0u, 32u, MemoryBudgets::TBD);
		SEOUL_UNITTESTING_ASSERT_EQUAL(nullptr, p);
	}
	// Alignment explicit, large alignment with alignment change.
	{
		void* p = MemoryManager::AllocateAligned(64u, MemoryBudgets::TBD, 64u); // Need to be our known minimum of all minimum platform alignments.
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(nullptr, p);
		p = MemoryManager::ReallocateAligned(p, 0u, 32u, MemoryBudgets::TBD);
		SEOUL_UNITTESTING_ASSERT_EQUAL(nullptr, p);
	}
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
