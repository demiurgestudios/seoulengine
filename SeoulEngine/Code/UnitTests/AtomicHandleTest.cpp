/**
 * \file AtomicHandleTest.cpp
 * \brief Unit test implementations for the AtomicHandle and
 * AtomicHandleTable classes.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "AtomicHandle.h"
#include "AtomicHandleTest.h"
#include "Logger.h"
#include "MemoryManager.h"
#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionDefine.h"
#include "SeoulTypes.h"
#include "Thread.h"
#include "UnitTesting.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

template <typename T>
static inline String UnitTestingToString(const AtomicHandle<T>& h)
{
	return String::Printf("%d", (Int32)h.GetAtomicValue());
}

SEOUL_BEGIN_TYPE(AtomicHandleTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestAllocation)
	SEOUL_METHOD(TestEquality)
	SEOUL_METHOD(TestHandleTable)
	SEOUL_METHOD(TestHandleTableThreaded)
SEOUL_END_TYPE()

class AtomicHandleTestObject SEOUL_SEALED
{
public:
	AtomicHandleTestObject();
	~AtomicHandleTestObject();

	const AtomicHandle<AtomicHandleTestObject>& GetHandle() const
	{
		return m_hHandle;
	}

	Atomic32Type GetHandleValue() const { return m_hHandle.GetAtomicValue(); }

private:
	AtomicHandle<AtomicHandleTestObject> m_hHandle;
}; // class AtomicHandleTestObject

SEOUL_SPEC_TEMPLATE_TYPE(CheckedPtr<AtomicHandleTestObject>)
SEOUL_BEGIN_TYPE(AtomicHandleTestObject)
	SEOUL_PROPERTY_N_EXT("HandleValue", GetHandleValue)
SEOUL_END_TYPE()

typedef AtomicHandle<AtomicHandleTestObject> AtomicHandleTestObjectHandle;
typedef AtomicHandleTable<AtomicHandleTestObject> AtomicHandleTestObjectHandleTable;

// NOTE: Assignment here is necessary to convince the linker in our GCC toolchain
// to include this definition. Otherwise, it strips it.
template <>
AtomicHandleTableCommon::Data AtomicHandleTestObjectHandleTable::s_Data = AtomicHandleTableCommon::Data();

AtomicHandleTestObject::AtomicHandleTestObject()
	: m_hHandle()
{
	m_hHandle = AtomicHandleTestObjectHandleTable::Allocate(this);
}

AtomicHandleTestObject::~AtomicHandleTestObject()
{
	AtomicHandleTestObjectHandleTable::Free(m_hHandle);
	SEOUL_UNITTESTING_ASSERT_EQUAL(AtomicHandleTestObjectHandle(), m_hHandle);
}

/** Conversion to pointer convenience functions. */
template <typename T>
inline CheckedPtr<T> GetPtr(AtomicHandleTestObjectHandle h)
{
	CheckedPtr<T> pReturn((T*)((AtomicHandleTestObject*)AtomicHandleTestObjectHandleTable::Get(h)));
	return pReturn;
}

static inline CheckedPtr<AtomicHandleTestObject> GetPtr(AtomicHandleTestObjectHandle h)
{
	CheckedPtr<AtomicHandleTestObject> pReturn((AtomicHandleTestObject*)AtomicHandleTestObjectHandleTable::Get(h));
	return pReturn;
}

/**
 * Test Handle validity when doing object allocation/deletion
 */
void AtomicHandleTest::TestAllocation()
{
	// Test Handleable subclass constructor
	AtomicHandleTestObject* newObj = SEOUL_NEW(MemoryBudgets::TBD) AtomicHandleTestObject();
	SEOUL_UNITTESTING_ASSERT(newObj);
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(nullptr, GetPtr(newObj->GetHandle()));
	SEOUL_UNITTESTING_ASSERT_EQUAL(GetPtr(newObj->GetHandle()), newObj);

	// Test default constructor
	AtomicHandleTestObjectHandle hObj;
	SEOUL_UNITTESTING_ASSERT(!hObj.IsInternalValid());
	SEOUL_UNITTESTING_ASSERT_EQUAL(nullptr, GetPtr(hObj));

	// Test assignment and GetPtr
	hObj = newObj->GetHandle();
	SEOUL_UNITTESTING_ASSERT(hObj.IsInternalValid());
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(nullptr, GetPtr(hObj));
	SEOUL_UNITTESTING_ASSERT_EQUAL(GetPtr(hObj), newObj);

	// Test destructor - handle should still be internally valid (set to a non-zero value)
	// but resolve to a null object.
	SafeDelete(newObj);
	SEOUL_UNITTESTING_ASSERT(hObj.IsInternalValid());
	SEOUL_UNITTESTING_ASSERT_EQUAL(nullptr, GetPtr(hObj));

	// Test generation IDs - newObj->GetHandle() and hObj point to the same
	//   table slot
	newObj = SEOUL_NEW(MemoryBudgets::TBD) AtomicHandleTestObject();
	SEOUL_UNITTESTING_ASSERT(newObj);
	SEOUL_UNITTESTING_ASSERT(newObj->GetHandle().IsInternalValid());
	SEOUL_UNITTESTING_ASSERT_EQUAL(newObj, GetPtr(newObj->GetHandle()));
	SEOUL_UNITTESTING_ASSERT(hObj.IsInternalValid());
	SEOUL_UNITTESTING_ASSERT_EQUAL(nullptr, GetPtr(hObj));

	SafeDelete(newObj);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, AtomicHandleTestObjectHandleTable::GetAllocatedCount());
}

/**
 * Test Handle equality and void* conversion operators
 */
void AtomicHandleTest::TestEquality()
{
	AtomicHandleTestObject* newObj1 = SEOUL_NEW(MemoryBudgets::TBD) AtomicHandleTestObject();
	SEOUL_UNITTESTING_ASSERT(newObj1);

	AtomicHandleTestObject* newObj2 = SEOUL_NEW(MemoryBudgets::TBD) AtomicHandleTestObject();
	SEOUL_UNITTESTING_ASSERT(newObj2);

	// Test assignment operator
	{
		AtomicHandleTestObjectHandle hObj1 = newObj1->GetHandle();
		AtomicHandleTestObjectHandle hObj2 = newObj2->GetHandle();
		SEOUL_UNITTESTING_ASSERT_EQUAL(hObj1, newObj1->GetHandle());
		SEOUL_UNITTESTING_ASSERT_EQUAL(hObj2, newObj2->GetHandle());
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(hObj1, hObj2);
	}

	// Test copy constructor
	{
		AtomicHandleTestObjectHandle hObj1(newObj1->GetHandle());
		AtomicHandleTestObjectHandle hObj2(newObj2->GetHandle());
		SEOUL_UNITTESTING_ASSERT_EQUAL(hObj1, newObj1->GetHandle());
		SEOUL_UNITTESTING_ASSERT_EQUAL(hObj2, newObj2->GetHandle());
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(hObj1, hObj2);
	}

	// Test void* conversion
	{
		AtomicHandleTestObjectHandle hObj1;
		void* p = AtomicHandleTestObjectHandle::ToVoidStar(newObj1->GetHandle());
		SEOUL_ASSERT((size_t)p < UIntMax);
		hObj1 = AtomicHandleTestObjectHandle::ToHandle(p);
		SEOUL_UNITTESTING_ASSERT(hObj1.IsInternalValid());
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(nullptr, GetPtr(hObj1));
		SEOUL_UNITTESTING_ASSERT_EQUAL(hObj1, newObj1->GetHandle());
	}

	SafeDelete(newObj1);
	SafeDelete(newObj2);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, AtomicHandleTestObjectHandleTable::GetAllocatedCount());
}

/**
 * Stress test the handle table
 */
void AtomicHandleTest::TestHandleTable()
{
	AtomicHandleTestObject* newObj = SEOUL_NEW(MemoryBudgets::TBD) AtomicHandleTestObject();
	SEOUL_UNITTESTING_ASSERT(newObj);
	SEOUL_UNITTESTING_ASSERT(newObj->GetHandle().IsInternalValid());
	SEOUL_UNITTESTING_ASSERT_EQUAL(newObj, GetPtr(newObj->GetHandle()));

	// Verify entries being removed
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, AtomicHandleTestObjectHandleTable::GetAllocatedCount());
	SafeDelete(newObj);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, AtomicHandleTestObjectHandleTable::GetAllocatedCount());

	// Verify a non-empty table
	SEOUL_UNITTESTING_ASSERT_LESS_THAN(0u, AtomicHandleTableCommon::kGlobalArraySize);

	Vector< AtomicHandleTestObjectHandle > vhHandleArray(AtomicHandleTableCommon::kGlobalArraySize);
	Vector< AtomicHandleTestObject* > vpPtrArray(AtomicHandleTableCommon::kGlobalArraySize);

	// Fill the table
	for (UInt32 uIdx = 0; uIdx < AtomicHandleTableCommon::kGlobalArraySize; ++uIdx)
	{
		vpPtrArray[uIdx] = SEOUL_NEW(MemoryBudgets::TBD) AtomicHandleTestObject();
		vhHandleArray[uIdx] = vpPtrArray[uIdx]->GetHandle();
	}

	// Verify all entries are valid
	for (UInt32 uIdx = 0; uIdx < AtomicHandleTableCommon::kGlobalArraySize; ++uIdx)
	{
		SEOUL_UNITTESTING_ASSERT_MESSAGE(vhHandleArray[uIdx].IsInternalValid(), "Invalid Handle Entry at Index %#x", uIdx);
		SEOUL_UNITTESTING_ASSERT_EQUAL(vpPtrArray[uIdx], GetPtr(vhHandleArray[uIdx]));
	}

	// Empty the table
	for (UInt32 uIdx = 0; uIdx < AtomicHandleTableCommon::kGlobalArraySize; ++uIdx)
	{
		SafeDelete(vpPtrArray[uIdx]);
	}

	// Verify all saved handles are invalid
	for (UInt32 uIdx = 0; uIdx < AtomicHandleTableCommon::kGlobalArraySize; ++uIdx)
	{
		// Internal will still be valid (handle is still non-zero) but GetPtr() will be nullptr.
		SEOUL_UNITTESTING_ASSERT_MESSAGE(vhHandleArray[uIdx].IsInternalValid(), "Failed to Delete Entry at Index %#x", uIdx);
		SEOUL_UNITTESTING_ASSERT_EQUAL(nullptr, GetPtr(vhHandleArray[uIdx]));
	}

	// Refill table
	for (UInt32 uIdx = 0; uIdx < AtomicHandleTableCommon::kGlobalArraySize; ++uIdx)
	{
		vpPtrArray[uIdx] = SEOUL_NEW(MemoryBudgets::TBD) AtomicHandleTestObject();
	}

	// Verify saved handles are still invalid
	for (UInt32 uIdx = 0; uIdx < AtomicHandleTableCommon::kGlobalArraySize; ++uIdx)
	{
		// Internal will still be valid (handle is still non-zero) but GetPtr() will be nullptr.
		SEOUL_UNITTESTING_ASSERT_MESSAGE(vhHandleArray[uIdx].IsInternalValid(), "Failed to Delete Entry at Index %#x", uIdx);
		SEOUL_UNITTESTING_ASSERT_EQUAL(nullptr, GetPtr(vhHandleArray[uIdx]));
	}

	// Empty the table
	for (UInt32 uIdx = 0; uIdx < AtomicHandleTableCommon::kGlobalArraySize; ++uIdx)
	{
		SafeDelete(vpPtrArray[uIdx]);
	}

	// Verify that the table has actually been emptied
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, AtomicHandleTestObjectHandleTable::GetAllocatedCount());
}

class TestHandleTableThreadedUtil SEOUL_SEALED
{
public:
	SEOUL_DELEGATE_TARGET(TestHandleTableThreadedUtil);

	TestHandleTableThreadedUtil()
		: m_vhHandleArray(AtomicHandleTableCommon::kGlobalArraySize)
		, m_vpPtrArray(AtomicHandleTableCommon::kGlobalArraySize)
		, m_Index(0)
	{
	}

	Vector< AtomicHandleTestObjectHandle > m_vhHandleArray;
	Vector< AtomicHandleTestObject* > m_vpPtrArray;
	Atomic32 m_Index;

	Int FillAll(const Thread&)
	{
		while (true)
		{
			Atomic32Type const index = (++m_Index) - 1;
			if (index >= (Atomic32Type)AtomicHandleTableCommon::kGlobalArraySize)
			{
				break;
			}

			m_vpPtrArray[index] = SEOUL_NEW(MemoryBudgets::TBD) AtomicHandleTestObject();
			m_vhHandleArray[index] = m_vpPtrArray[index]->GetHandle();
		}

		return 0;
	}

	Int FillOne(const Thread&)
	{
		while (true)
		{
			Atomic32Type const index = (++m_Index) - 1;
			if (index >= (Atomic32Type)AtomicHandleTableCommon::kGlobalArraySize)
			{
				break;
			}

			m_vpPtrArray[index] = SEOUL_NEW(MemoryBudgets::TBD) AtomicHandleTestObject();
		}

		return 0;
	}

private:
	SEOUL_DISABLE_COPY(TestHandleTableThreadedUtil);
}; // class TestHandleTableThreadedUtil

/**
 * Stress test the handle table. Multiple threads.
 */
void AtomicHandleTest::TestHandleTableThreaded()
{
	static const UInt32 kuThreads = 8u;

	AtomicHandleTestObject* newObj = SEOUL_NEW(MemoryBudgets::TBD) AtomicHandleTestObject();
	SEOUL_UNITTESTING_ASSERT(newObj);
	SEOUL_UNITTESTING_ASSERT(newObj->GetHandle().IsInternalValid());
	SEOUL_UNITTESTING_ASSERT_EQUAL(newObj, GetPtr(newObj->GetHandle()));

	// Verify entries being removed
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, AtomicHandleTestObjectHandleTable::GetAllocatedCount());
	SafeDelete(newObj);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, AtomicHandleTestObjectHandleTable::GetAllocatedCount());

	// Verify a non-empty table
	SEOUL_UNITTESTING_ASSERT_LESS_THAN(0u, AtomicHandleTableCommon::kGlobalArraySize);

	TestHandleTableThreadedUtil util;
	{
		FixedArray<ScopedPtr<Thread>, kuThreads> aThreads;

		// Fill the table
		for (auto i = aThreads.Begin(); aThreads.End() != i; ++i)
		{
			i->Reset(SEOUL_NEW(MemoryBudgets::TBD) Thread(
				SEOUL_BIND_DELEGATE(&TestHandleTableThreadedUtil::FillAll, &util),
				true));
		}

		for (auto i = aThreads.Begin(); aThreads.End() != i; ++i)
		{
			(*i)->WaitUntilThreadIsNotRunning();
		}
	}
	util.m_Index.Reset();

	// Verify all entries are valid
	for (UInt32 uIdx = 0; uIdx < AtomicHandleTableCommon::kGlobalArraySize; ++uIdx)
	{
		SEOUL_UNITTESTING_ASSERT_MESSAGE(util.m_vhHandleArray[uIdx].IsInternalValid(), "Invalid Handle Entry at Index %#x", uIdx);
		SEOUL_UNITTESTING_ASSERT_EQUAL(util.m_vpPtrArray[uIdx], GetPtr(util.m_vhHandleArray[uIdx]));
	}

	// Empty the table
	for (UInt32 uIdx = 0; uIdx < AtomicHandleTableCommon::kGlobalArraySize; ++uIdx)
	{
		SafeDelete(util.m_vpPtrArray[uIdx]);
	}

	// Verify all saved handles are invalid
	for (UInt32 uIdx = 0; uIdx < AtomicHandleTableCommon::kGlobalArraySize; ++uIdx)
	{
		// Internal will still be valid (handle is still non-zero) but GetPtr() will be nullptr.
		SEOUL_UNITTESTING_ASSERT_MESSAGE(util.m_vhHandleArray[uIdx].IsInternalValid(), "Failed to Delete Entry at Index %#x", uIdx);
		SEOUL_UNITTESTING_ASSERT_EQUAL(nullptr, GetPtr(util.m_vhHandleArray[uIdx]));
	}

	// Refill table
	{
		FixedArray<ScopedPtr<Thread>, kuThreads> aThreads;

		// Fill the table
		for (auto i = aThreads.Begin(); aThreads.End() != i; ++i)
		{
			i->Reset(SEOUL_NEW(MemoryBudgets::TBD) Thread(
				SEOUL_BIND_DELEGATE(&TestHandleTableThreadedUtil::FillOne, &util),
				true));
		}

		for (auto i = aThreads.Begin(); aThreads.End() != i; ++i)
		{
			(*i)->WaitUntilThreadIsNotRunning();
		}
	}
	util.m_Index.Reset();

	// Verify saved handles are still invalid
	for (UInt32 uIdx = 0; uIdx < AtomicHandleTableCommon::kGlobalArraySize; ++uIdx)
	{
		// Internal will still be valid (handle is still non-zero) but GetPtr() will be nullptr.
		SEOUL_UNITTESTING_ASSERT_MESSAGE(util.m_vhHandleArray[uIdx].IsInternalValid(), "Failed to Delete Entry at Index %#x", uIdx);
		SEOUL_UNITTESTING_ASSERT_EQUAL(nullptr, GetPtr(util.m_vhHandleArray[uIdx]));
	}

	// Empty the table
	for (UInt32 uIdx = 0; uIdx < AtomicHandleTableCommon::kGlobalArraySize; ++uIdx)
	{
		SafeDelete(util.m_vpPtrArray[uIdx]);
	}

	// Verify that the table has actually been emptied
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, AtomicHandleTestObjectHandleTable::GetAllocatedCount());
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
