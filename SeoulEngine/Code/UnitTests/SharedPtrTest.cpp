/**
 * \file SharedPtrTest.cpp
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "BoxedValue.h"
#include "ReflectionDefine.h"
#include "SharedPtrTest.h"
#include "UnitTesting.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(SharedPtrTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestBasic)
	SEOUL_METHOD(TestBoxedValue)
SEOUL_END_TYPE()

namespace
{

static Atomic32 s_Count;

class TestObject
{
public:
	TestObject()
		: m_i(14098140)
	{
		++s_Count;
	}

	~TestObject()
	{
		--s_Count;
		m_i = -1;
	}

	Int32 m_i;

private:
	SEOUL_DISABLE_COPY(TestObject);
	SEOUL_REFERENCE_COUNTED(TestObject);
}; // class TestObject

class TestObject2
{
public:
	TestObject2()
		: m_i(14098140)
	{
		++s_Count;
	}

	~TestObject2()
	{
		--s_Count;
		m_i = -1;
	}

	Int32 m_i;

private:
	SEOUL_DISABLE_COPY(TestObject2);
}; // class TestObject2

} // namespace anonymous

void SharedPtrTest::TestBasic()
{
	{
		auto pT = SEOUL_NEW(MemoryBudgets::Developer) TestObject;
		SharedPtr<TestObject> p(pT);

		SEOUL_UNITTESTING_ASSERT_EQUAL(p.GetPtr(), pT);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, p.GetReferenceCount());
		SEOUL_UNITTESTING_ASSERT(p.IsUnique());
		SEOUL_UNITTESTING_ASSERT(p.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, s_Count);

		SharedPtr<TestObject> pB;
		p.Swap(pB);
		SEOUL_UNITTESTING_ASSERT_EQUAL(pB.GetPtr(), pT);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, pB.GetReferenceCount());
		SEOUL_UNITTESTING_ASSERT(pB.IsUnique());
		SEOUL_UNITTESTING_ASSERT(pB.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, s_Count);

		SEOUL_UNITTESTING_ASSERT_EQUAL(nullptr, p.GetPtr());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, p.GetReferenceCount());
		SEOUL_UNITTESTING_ASSERT(!p.IsUnique());
		SEOUL_UNITTESTING_ASSERT(!p.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, s_Count);

		SEOUL_UNITTESTING_ASSERT_EQUAL(14098140, pB->m_i);
		SEOUL_UNITTESTING_ASSERT_EQUAL(14098140, (*pB).m_i);

		pT = nullptr;
		pB.AtomicReplace(nullptr);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, s_Count);
	}
}

void SharedPtrTest::TestBoxedValue()
{
	{
		auto pTT = SEOUL_NEW(MemoryBudgets::Developer) BoxedValue<TestObject2>;
		auto pT = &pTT->GetBoxedValue();
		SharedPtr< BoxedValue<TestObject2> > p(pTT);

		SEOUL_UNITTESTING_ASSERT_EQUAL(p.GetPtr(), pTT);
		SEOUL_UNITTESTING_ASSERT_EQUAL(&(*p), pT);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, p.GetReferenceCount());
		SEOUL_UNITTESTING_ASSERT(p.IsUnique());
		SEOUL_UNITTESTING_ASSERT(p.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, s_Count);

		SharedPtr< BoxedValue<TestObject2> > pB;
		p.Swap(pB);
		SEOUL_UNITTESTING_ASSERT_EQUAL(pB.GetPtr(), pTT);
		SEOUL_UNITTESTING_ASSERT_EQUAL(&(*pB), pT);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, pB.GetReferenceCount());
		SEOUL_UNITTESTING_ASSERT(pB.IsUnique());
		SEOUL_UNITTESTING_ASSERT(pB.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, s_Count);

		SEOUL_UNITTESTING_ASSERT_EQUAL(nullptr, p.GetPtr());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, p.GetReferenceCount());
		SEOUL_UNITTESTING_ASSERT(!p.IsUnique());
		SEOUL_UNITTESTING_ASSERT(!p.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, s_Count);

		SEOUL_UNITTESTING_ASSERT_EQUAL(14098140, pB->m_i);
		SEOUL_UNITTESTING_ASSERT_EQUAL(14098140, (*pB).m_i);

		pT = nullptr;
		pB.AtomicReplace(nullptr);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, s_Count);
	}
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
