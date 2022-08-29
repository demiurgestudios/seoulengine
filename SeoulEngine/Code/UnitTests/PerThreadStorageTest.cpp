/**
 * \file PerThreadStorageTest.cpp
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

#include "Atomic32.h"
#include "Logger.h"
#include "PerThreadStorageTest.h"
#include "ReflectionDefine.h"
#include "ScopedPtr.h"
#include "Thread.h"
#include "UnitTesting.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(PerThreadStorageTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestPerThreadStorage)
	SEOUL_METHOD(TestHeapAllocatedPerThreadStorage)
SEOUL_END_TYPE()

struct PerThreadStorageTestUtility
{
	SEOUL_DELEGATE_TARGET(PerThreadStorageTestUtility);

	Atomic32 m_Atomic;
	PerThreadStorage m_PerThreadStorage;

	Int Run(const Thread& thread)
	{
		SEOUL_UNITTESTING_ASSERT(!m_PerThreadStorage.GetPerThreadStorage());
		Int const iValue = (Int)(++m_Atomic);
		m_PerThreadStorage.SetPerThreadStorage((void*)((size_t)iValue));

		Int iReturn = (Int)((size_t)m_PerThreadStorage.GetPerThreadStorage());
		SEOUL_UNITTESTING_ASSERT(iValue == iReturn);

		return iReturn;
	}
}; // struct PerThreadStorageTestUtility

void PerThreadStorageTest::TestPerThreadStorage()
{
	static const Int32 kTestThreadCount = 50;
	PerThreadStorageTestUtility test;

	ScopedPtr<Thread> apThreads[kTestThreadCount];
	for (Int32 i = 0; i < kTestThreadCount; ++i)
	{
		apThreads[i].Reset(SEOUL_NEW(MemoryBudgets::TBD) Thread(
			SEOUL_BIND_DELEGATE(&PerThreadStorageTestUtility::Run, &test)));
	}

	for (Int32 i = 0; i < kTestThreadCount; ++i)
	{
		SEOUL_VERIFY(apThreads[i]->Start());
	}

	for (Int32 i = 0; i < kTestThreadCount; ++i)
	{
		apThreads[i]->WaitUntilThreadIsNotRunning();
	}

	SEOUL_UNITTESTING_ASSERT(kTestThreadCount == (Int32)test.m_Atomic);
	Vector<Int> vValues(kTestThreadCount);
	for (Int32 i = 0; i < kTestThreadCount; ++i)
	{
		vValues[i] = apThreads[i]->GetReturnValue();
	}
	std::sort(vValues.Begin(), vValues.End());

	for (Int32 i = 1; i < kTestThreadCount; ++i)
	{
		SEOUL_UNITTESTING_ASSERT(vValues[i] - 1 == vValues[i-1]);
	}
}

struct HeapAllocatedObject
{
	explicit HeapAllocatedObject(Atomic32Type index)
		: m_ThreadIndex(index)
		, m_iValue(0)
	{
	}

	Atomic32Type const m_ThreadIndex;
	Int m_iValue;
};

struct HeapAllocatedPerThreadStorageTest
{
	SEOUL_DELEGATE_TARGET(HeapAllocatedPerThreadStorageTest);

	Atomic32 m_Counter;
	HeapAllocatedPerThreadStorage<HeapAllocatedObject, 50u> m_HeapAllocatedPerThreadStorage;

	Int Run(const Thread& thread)
	{
		Int const iValue = (Int)(++m_Counter);
		m_HeapAllocatedPerThreadStorage.Get().m_iValue = iValue;

		return iValue;
	}
}; // struct HeapAllocatedPerThreadStorageTest

void PerThreadStorageTest::TestHeapAllocatedPerThreadStorage()
{
	static const Int32 kTestThreadCount = 50;
	HeapAllocatedPerThreadStorageTest test;

	ScopedPtr<Thread> apThreads[kTestThreadCount];
	for (Int32 i = 0; i < kTestThreadCount; ++i)
	{
		apThreads[i].Reset(SEOUL_NEW(MemoryBudgets::TBD) Thread(
			SEOUL_BIND_DELEGATE(&HeapAllocatedPerThreadStorageTest::Run, &test)));
	}

	for (Int32 i = 0; i < kTestThreadCount; ++i)
	{
		SEOUL_VERIFY(apThreads[i]->Start());
	}

	for (Int32 i = 0; i < kTestThreadCount; ++i)
	{
		apThreads[i]->WaitUntilThreadIsNotRunning();
	}

	SEOUL_UNITTESTING_ASSERT(kTestThreadCount == (Int32)test.m_Counter);

	const HeapAllocatedPerThreadStorage<HeapAllocatedObject, 50u>::Objects& vObjects =
		test.m_HeapAllocatedPerThreadStorage.GetAllObjects();
	SEOUL_UNITTESTING_ASSERT(kTestThreadCount == (Int)vObjects.GetSize());

	Vector<Int> vValuesA(kTestThreadCount);
	Vector<Int> vValuesB(kTestThreadCount);
	Vector<Atomic32Type> vValuesC(kTestThreadCount);
	for (Int32 i = 0; i < kTestThreadCount; ++i)
	{
		vValuesA[i] = apThreads[i]->GetReturnValue();
		vValuesB[i] = vObjects[i]->m_iValue;
		vValuesC[i] = vObjects[i]->m_ThreadIndex;
	}
	QuickSort(vValuesA.Begin(), vValuesA.End());
	QuickSort(vValuesB.Begin(), vValuesB.End());
	QuickSort(vValuesC.Begin(), vValuesC.End());

	for (Int32 i = 1; i < kTestThreadCount; ++i)
	{
		SEOUL_UNITTESTING_ASSERT(vValuesA[i] - 1 == vValuesA[i-1]);
		SEOUL_UNITTESTING_ASSERT(vValuesB[i] - 1 == vValuesB[i-1]);
		SEOUL_UNITTESTING_ASSERT(vValuesC[i] - 1 == vValuesC[i - 1]);
		SEOUL_UNITTESTING_ASSERT(vValuesA[i] == vValuesB[i]);
		SEOUL_UNITTESTING_ASSERT(vValuesC[i] == (Int)vValuesC[i]);
	}
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
