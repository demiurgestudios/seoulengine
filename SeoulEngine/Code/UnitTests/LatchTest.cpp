/**
 * \file LatchTest.cpp
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Latch.h"
#include "LatchTest.h"
#include "ReflectionDefine.h"
#include "UnitTesting.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(LatchTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestBasic)
	SEOUL_METHOD(TestConditions)
	SEOUL_METHOD(TestConditionsTrigger)
	SEOUL_METHOD(TestRequire)
SEOUL_END_TYPE()

namespace
{

class TestLatch SEOUL_SEALED : public Latch
{
public:
	TestLatch()
	{
	}

	virtual void Execute() SEOUL_OVERRIDE
	{
		++m_Executes;
	}

	Atomic32 m_Executes;

private:
	SEOUL_DISABLE_COPY(TestLatch);
}; // class TestLatch

} // namespace anonynmous

void LatchTest::TestBasic()
{
	TestLatch latch;
	SEOUL_UNITTESTING_ASSERT_EQUAL(eLatchNew, latch.GetStatus());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, latch.m_Executes);
	SEOUL_UNITTESTING_ASSERT(!latch.Check(String()));
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, latch.m_Executes);
	SEOUL_UNITTESTING_ASSERT_EQUAL(eLatchNew, latch.GetStatus());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, latch.m_Executes);
	SEOUL_UNITTESTING_ASSERT_EQUAL(eLatchClosed, latch.Step(String()));
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, latch.m_Executes);
	SEOUL_UNITTESTING_ASSERT_EQUAL(eLatchClosed, latch.GetStatus());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, latch.m_Executes);
	latch.Reset();
	SEOUL_UNITTESTING_ASSERT_EQUAL(eLatchNew, latch.GetStatus());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, latch.m_Executes);
}

void LatchTest::TestConditions()
{
	Byte const* cond[] = { "A", "B", "C" };

	TestLatch latch;
	latch.Reset(cond, 3);

	SEOUL_UNITTESTING_ASSERT_EQUAL(eLatchNew, latch.GetStatus());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, latch.m_Executes);
	SEOUL_UNITTESTING_ASSERT(latch.Check("A"));
	SEOUL_UNITTESTING_ASSERT(latch.Check("B"));
	SEOUL_UNITTESTING_ASSERT(latch.Check("C"));

	SEOUL_UNITTESTING_ASSERT_EQUAL(eLatchOpen, latch.Step("A"));
	SEOUL_UNITTESTING_ASSERT_EQUAL(eLatchOpen, latch.GetStatus());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, latch.m_Executes);
	SEOUL_UNITTESTING_ASSERT(!latch.Check("A"));
	SEOUL_UNITTESTING_ASSERT(latch.Check("B"));
	SEOUL_UNITTESTING_ASSERT(latch.Check("C"));

	SEOUL_UNITTESTING_ASSERT_EQUAL(eLatchOpen, latch.Step("C"));
	SEOUL_UNITTESTING_ASSERT_EQUAL(eLatchOpen, latch.GetStatus());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, latch.m_Executes);
	SEOUL_UNITTESTING_ASSERT(!latch.Check("A"));
	SEOUL_UNITTESTING_ASSERT(latch.Check("B"));
	SEOUL_UNITTESTING_ASSERT(latch.Check("C"));

	SEOUL_UNITTESTING_ASSERT_EQUAL(eLatchOpen, latch.Step("B"));
	SEOUL_UNITTESTING_ASSERT_EQUAL(eLatchOpen, latch.GetStatus());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, latch.m_Executes);
	SEOUL_UNITTESTING_ASSERT(!latch.Check("A"));
	SEOUL_UNITTESTING_ASSERT(!latch.Check("B"));
	SEOUL_UNITTESTING_ASSERT(latch.Check("C"));

	SEOUL_UNITTESTING_ASSERT_EQUAL(eLatchClosed, latch.Step("C"));
	SEOUL_UNITTESTING_ASSERT_EQUAL(eLatchClosed, latch.GetStatus());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, latch.m_Executes);
	SEOUL_UNITTESTING_ASSERT(!latch.Check("A"));
	SEOUL_UNITTESTING_ASSERT(!latch.Check("B"));
	SEOUL_UNITTESTING_ASSERT(!latch.Check("C"));

	latch.Reset(cond, 3);

	SEOUL_UNITTESTING_ASSERT_EQUAL(eLatchNew, latch.GetStatus());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, latch.m_Executes);
	SEOUL_UNITTESTING_ASSERT(latch.Check("A"));
	SEOUL_UNITTESTING_ASSERT(latch.Check("B"));
	SEOUL_UNITTESTING_ASSERT(latch.Check("C"));
}

void LatchTest::TestConditionsTrigger()
{
	Byte const* cond[] = { "A", "B", "C" };

	TestLatch latch;
	latch.Reset(cond, 3);

	SEOUL_UNITTESTING_ASSERT_EQUAL(eLatchNew, latch.GetStatus());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, latch.m_Executes);
	SEOUL_UNITTESTING_ASSERT(latch.Check("A"));
	SEOUL_UNITTESTING_ASSERT(latch.Check("B"));
	SEOUL_UNITTESTING_ASSERT(latch.Check("C"));

	SEOUL_UNITTESTING_ASSERT_EQUAL(eLatchOpen, latch.Trigger("B"));
	SEOUL_UNITTESTING_ASSERT_EQUAL(eLatchOpen, latch.GetStatus());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, latch.m_Executes);
	SEOUL_UNITTESTING_ASSERT(latch.Check("A"));
	SEOUL_UNITTESTING_ASSERT(!latch.Check("B"));
	SEOUL_UNITTESTING_ASSERT(latch.Check("C"));

	Vector<String> vTriggers;
	vTriggers.PushBack("C");
	vTriggers.PushBack("A");

	SEOUL_UNITTESTING_ASSERT_EQUAL(eLatchClosed, latch.Trigger(vTriggers));
	SEOUL_UNITTESTING_ASSERT_EQUAL(eLatchClosed, latch.GetStatus());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, latch.m_Executes);
	SEOUL_UNITTESTING_ASSERT(!latch.Check("A"));
	SEOUL_UNITTESTING_ASSERT(!latch.Check("B"));
	SEOUL_UNITTESTING_ASSERT(!latch.Check("C"));

	latch.Reset(cond, 3);

	SEOUL_UNITTESTING_ASSERT_EQUAL(eLatchNew, latch.GetStatus());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, latch.m_Executes);
	SEOUL_UNITTESTING_ASSERT(latch.Check("A"));
	SEOUL_UNITTESTING_ASSERT(latch.Check("B"));
	SEOUL_UNITTESTING_ASSERT(latch.Check("C"));
}

void LatchTest::TestRequire()
{
	TestLatch latch;
	SEOUL_UNITTESTING_ASSERT_EQUAL(eLatchNew, latch.GetStatus());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, latch.m_Executes);
	latch.Force();
	SEOUL_UNITTESTING_ASSERT_EQUAL(eLatchClosed, latch.Step(String()));
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, latch.m_Executes);
	latch.Require("A");
	SEOUL_UNITTESTING_ASSERT_EQUAL(eLatchNew, latch.GetStatus());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, latch.m_Executes);
	SEOUL_UNITTESTING_ASSERT_EQUAL(eLatchClosed, latch.Step("A"));
	SEOUL_UNITTESTING_ASSERT_EQUAL(eLatchClosed, latch.Step(String()));
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, latch.m_Executes);
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
