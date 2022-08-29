/**
 * \file QueueTest.cpp
 * \brief Unit test code for the Seoul Queue<> class
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Atomic32.h"
#include "ContainerTestUtil.h"
#include "Core.h"
#include "ReflectionDefine.h"
#include "UnitTesting.h"
#include "Queue.h"
#include "QueueTest.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(QueueTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestBasic)
	SEOUL_METHOD(TestClearBuiltin)
	SEOUL_METHOD(TestClearComplex)
	SEOUL_METHOD(TestClearSimple)
	SEOUL_METHOD(TestConstructorBuiltin)
	SEOUL_METHOD(TestConstructorComplex)
	SEOUL_METHOD(TestConstructorSimple)
	SEOUL_METHOD(TestEmptyBuiltin)
	SEOUL_METHOD(TestEmptyComplex)
	SEOUL_METHOD(TestEmptySimple)
	SEOUL_METHOD(TestFind)
	SEOUL_METHOD(TestMethods)
	SEOUL_METHOD(TestIterators)
SEOUL_END_TYPE()

void QueueTest::TestBasic()
{
	{
		Queue<Int> testQueue;
		for (UInt i=0; i<10; i++)
		{
			testQueue.Push(i);
			SEOUL_UNITTESTING_ASSERT_EQUAL(i + 1, testQueue.GetSize());
		}

		for (UInt i=0; i<10; i++)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(10u - i, testQueue.GetSize());
			SEOUL_UNITTESTING_ASSERT_EQUAL((Int)i, testQueue.Front());
			testQueue.Pop();
		}
	}
}

void QueueTest::TestClearBuiltin()
{
	Queue<UInt16, MemoryBudgets::Audio> queue;
	queue.Push(UInt16(23));
	queue.Push(UInt16(194));
	queue.Push(UInt16(119));

	// Clear should destroy elements but leave capacity.
	queue.Clear();
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, queue.GetSize());
	SEOUL_UNITTESTING_ASSERT(queue.IsEmpty());

	queue.Push(UInt16(7));
	queue.Push(UInt16(1123));
	queue.Push(UInt16(434));
	queue.Push(UInt16(342));
	queue.Push(UInt16(23989));

	auto i = queue.Begin();
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, queue.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(*i++, 7);
	SEOUL_UNITTESTING_ASSERT_EQUAL(*i++, 1123);
	SEOUL_UNITTESTING_ASSERT_EQUAL(*i++, 434);
	SEOUL_UNITTESTING_ASSERT_EQUAL(*i++, 342);
	SEOUL_UNITTESTING_ASSERT_EQUAL(*i++, 23989);

	// Clear again.
	queue.Clear();
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, queue.GetSize());
	SEOUL_UNITTESTING_ASSERT(queue.IsEmpty());

	queue.Push(UInt16(3));
	queue.Push(UInt16(124));
	queue.Push(UInt16(342));
	queue.Push(UInt16(12));
	queue.Push(UInt16(33));
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, queue.GetSize());

	// Now do the "swap trick". This should completely free the memory.
	{
		Queue<UInt16, MemoryBudgets::Audio> queue2;
		queue2.Swap(queue);

		// queue is now empty
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, queue.GetSize());
		SEOUL_UNITTESTING_ASSERT(queue.IsEmpty());

		// queue2 has queue's state.
		i = queue2.Begin();
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, queue2.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(*i++, 3);
		SEOUL_UNITTESTING_ASSERT_EQUAL(*i++, 124);
		SEOUL_UNITTESTING_ASSERT_EQUAL(*i++, 342);
		SEOUL_UNITTESTING_ASSERT_EQUAL(*i++, 12);
		SEOUL_UNITTESTING_ASSERT_EQUAL(*i++, 33);
	}
}

void QueueTest::TestClearComplex()
{
	Queue<ContainerTestComplex, MemoryBudgets::Audio> queue;
	queue.Push(ContainerTestComplex(23));
	queue.Push(ContainerTestComplex(194));
	queue.Push(ContainerTestComplex(119));

	SEOUL_UNITTESTING_ASSERT_EQUAL(3, ContainerTestComplex::s_iCount);

	// Clear should destroy elements but leave capacity.
	queue.Clear();
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, queue.GetSize());
	SEOUL_UNITTESTING_ASSERT(queue.IsEmpty());

	queue.Push(ContainerTestComplex(7));
	queue.Push(ContainerTestComplex(1123));
	queue.Push(ContainerTestComplex(434));
	queue.Push(ContainerTestComplex(342));
	queue.Push(ContainerTestComplex(23989));

	auto i = queue.Begin();
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, ContainerTestComplex::s_iCount);
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, queue.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(*i++, 7);
	SEOUL_UNITTESTING_ASSERT_EQUAL(*i++, 1123);
	SEOUL_UNITTESTING_ASSERT_EQUAL(*i++, 434);
	SEOUL_UNITTESTING_ASSERT_EQUAL(*i++, 342);
	SEOUL_UNITTESTING_ASSERT_EQUAL(*i++, 23989);

	// Clear again.
	queue.Clear();
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, queue.GetSize());
	SEOUL_UNITTESTING_ASSERT(queue.IsEmpty());

	queue.Push(ContainerTestComplex(3));
	queue.Push(ContainerTestComplex(124));
	queue.Push(ContainerTestComplex(342));
	queue.Push(ContainerTestComplex(12));
	queue.Push(ContainerTestComplex(33));
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, ContainerTestComplex::s_iCount);
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, queue.GetSize());

	// Now do the "swap trick". This should completely free the memory.
	{
		Queue<ContainerTestComplex, MemoryBudgets::Audio> queue2;
		queue2.Swap(queue);

		// queue is now empty
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, ContainerTestComplex::s_iCount);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, queue.GetSize());
		SEOUL_UNITTESTING_ASSERT(queue.IsEmpty());

		// queue2 has queue's state.
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, ContainerTestComplex::s_iCount);
		i = queue2.Begin();
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, queue2.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(*i++, 3);
		SEOUL_UNITTESTING_ASSERT_EQUAL(*i++, 124);
		SEOUL_UNITTESTING_ASSERT_EQUAL(*i++, 342);
		SEOUL_UNITTESTING_ASSERT_EQUAL(*i++, 12);
		SEOUL_UNITTESTING_ASSERT_EQUAL(*i++, 33);
	}

	// All gone.
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
}

void QueueTest::TestClearSimple()
{
	Queue<ContainerTestSimple, MemoryBudgets::Audio> queue;
	ContainerTestSimple simple;
	simple.m_B = 33;
	simple.m_iA = 23;
	queue.Push(simple);
	simple.m_iA = 194;
	queue.Push(simple);
	simple.m_iA = 119;
	queue.Push(simple);

	// Clear should destroy elements but leave capacity.
	queue.Clear();
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, queue.GetSize());
	SEOUL_UNITTESTING_ASSERT(queue.IsEmpty());

	simple.m_iA = 7;
	queue.Push(simple);
	simple.m_iA = 1123;
	queue.Push(simple);
	simple.m_iA = 434;
	queue.Push(simple);
	simple.m_iA = 342;
	queue.Push(simple);
	simple.m_iA = 23989;
	queue.Push(simple);

	auto i = queue.Begin();
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, queue.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(i->m_iA, 7); ++i;
	SEOUL_UNITTESTING_ASSERT_EQUAL(i->m_iA, 1123); ++i;
	SEOUL_UNITTESTING_ASSERT_EQUAL(i->m_iA, 434); ++i;
	SEOUL_UNITTESTING_ASSERT_EQUAL(i->m_iA, 342); ++i;
	SEOUL_UNITTESTING_ASSERT_EQUAL(i->m_iA, 23989); ++i;

	// Clear again.
	queue.Clear();
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, queue.GetSize());
	SEOUL_UNITTESTING_ASSERT(queue.IsEmpty());

	simple.m_iA = 3;
	queue.Push(simple);
	simple.m_iA = 124;
	queue.Push(simple);
	simple.m_iA = 342;
	queue.Push(simple);
	simple.m_iA = 12;
	queue.Push(simple);
	simple.m_iA = 33;
	queue.Push(simple);
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, queue.GetSize());

	// Now do the "swap trick". This should completely free the memory.
	{
		Queue<ContainerTestSimple, MemoryBudgets::Audio> queue2;
		queue2.Swap(queue);

		// queue is now empty
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, queue.GetSize());
		SEOUL_UNITTESTING_ASSERT(queue.IsEmpty());

		// queue2 has queue's state.
		i = queue2.Begin();
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, queue2.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(i->m_iA, 3); ++i;
		SEOUL_UNITTESTING_ASSERT_EQUAL(i->m_iA, 124); ++i;
		SEOUL_UNITTESTING_ASSERT_EQUAL(i->m_iA, 342); ++i;
		SEOUL_UNITTESTING_ASSERT_EQUAL(i->m_iA, 12); ++i;
		SEOUL_UNITTESTING_ASSERT_EQUAL(i->m_iA, 33); ++i;
	}
}

void QueueTest::TestConstructorBuiltin()
{
	// Default.
	{
		Queue<Int64, MemoryBudgets::DataStore> queue;
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, queue.GetSize());
		SEOUL_UNITTESTING_ASSERT(queue.IsEmpty());
	}
}

void QueueTest::TestConstructorComplex()
{
	// Default.
	{
		Queue<ContainerTestComplex, MemoryBudgets::DataStore> queue;
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, queue.GetSize());
		SEOUL_UNITTESTING_ASSERT(queue.IsEmpty());
	}
}

void QueueTest::TestConstructorSimple()
{
	// Default.
	{
		Queue<ContainerTestSimple, MemoryBudgets::DataStore> queue;
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, queue.GetSize());
		SEOUL_UNITTESTING_ASSERT(queue.IsEmpty());
	}
}

void QueueTest::TestEmptyBuiltin()
{
	Queue<Int16, MemoryBudgets::DataStore> queue;
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, queue.GetSize());
	SEOUL_UNITTESTING_ASSERT(queue.IsEmpty());

	SEOUL_UNITTESTING_ASSERT_EQUAL(queue.Begin(), queue.End());
	SEOUL_UNITTESTING_ASSERT(!Contains(queue.Begin(), queue.End(), 5));
	SEOUL_UNITTESTING_ASSERT(!ContainsFromBack(queue.Begin(), queue.End(), 7));
	SEOUL_UNITTESTING_ASSERT_EQUAL(queue.End(), queue.Begin());

	SEOUL_UNITTESTING_ASSERT_EQUAL(queue.End(), Find(queue.Begin(), queue.End(), 37));
	SEOUL_UNITTESTING_ASSERT_EQUAL(queue.End(), FindFromBack(queue.Begin(), queue.End(), 37));

	{
		Queue<Int16, MemoryBudgets::DataStore> queue2;
		queue.Swap(queue2);
	}

	SEOUL_UNITTESTING_ASSERT_EQUAL(0, queue.GetSize());
	SEOUL_UNITTESTING_ASSERT(queue.IsEmpty());

	queue.Push(53);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, queue.GetSize());
	SEOUL_UNITTESTING_ASSERT(!queue.IsEmpty());

	SEOUL_UNITTESTING_ASSERT_EQUAL(53, *queue.Begin());
	SEOUL_UNITTESTING_ASSERT_EQUAL(53, *(--queue.End()));
	SEOUL_UNITTESTING_ASSERT_EQUAL(53, queue.Front());

	queue.Pop();
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, queue.GetSize());
	SEOUL_UNITTESTING_ASSERT(queue.IsEmpty());
}

void QueueTest::TestEmptyComplex()
{
	Queue<ContainerTestComplex, MemoryBudgets::DataStore> queue;
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, queue.GetSize());
	SEOUL_UNITTESTING_ASSERT(queue.IsEmpty());

	SEOUL_UNITTESTING_ASSERT_EQUAL(queue.Begin(), queue.End());
	SEOUL_UNITTESTING_ASSERT(!Contains(queue.Begin(), queue.End(), ContainerTestComplex(5)));
	SEOUL_UNITTESTING_ASSERT(!ContainsFromBack(queue.Begin(), queue.End(), ContainerTestComplex(7)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(queue.End(), queue.Begin());

	SEOUL_UNITTESTING_ASSERT_EQUAL(queue.End(), Find(queue.Begin(), queue.End(), ContainerTestComplex(37)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(queue.End(), FindFromBack(queue.Begin(), queue.End(), ContainerTestComplex(37)));

	{
		Queue<ContainerTestComplex, MemoryBudgets::DataStore> queue2;
		queue.Swap(queue2);
	}

	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, queue.GetSize());
	SEOUL_UNITTESTING_ASSERT(queue.IsEmpty());

	queue.Push(ContainerTestComplex(53));
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, ContainerTestComplex::s_iCount);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, queue.GetSize());
	SEOUL_UNITTESTING_ASSERT(!queue.IsEmpty());

	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(53), queue.Back());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(53), *queue.Begin());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(53), *(--queue.End()));
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(53), queue.Front());

	queue.Pop();
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, queue.GetSize());
	SEOUL_UNITTESTING_ASSERT(queue.IsEmpty());
}

void QueueTest::TestEmptySimple()
{
	ContainerTestSimple simple;

	Queue<ContainerTestSimple, MemoryBudgets::DataStore> queue;
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, queue.GetSize());
	SEOUL_UNITTESTING_ASSERT(queue.IsEmpty());

	SEOUL_UNITTESTING_ASSERT_EQUAL(queue.Begin(), queue.End());
	SEOUL_UNITTESTING_ASSERT(!Contains(queue.Begin(), queue.End(), simple));
	SEOUL_UNITTESTING_ASSERT(!ContainsFromBack(queue.Begin(), queue.End(), simple));
	SEOUL_UNITTESTING_ASSERT_EQUAL(queue.End(), queue.Begin());

	simple.m_B = 33;
	simple.m_iA = 23;

	SEOUL_UNITTESTING_ASSERT_EQUAL(queue.End(), Find(queue.Begin(), queue.End(), simple));
	SEOUL_UNITTESTING_ASSERT_EQUAL(queue.End(), FindFromBack(queue.Begin(), queue.End(), simple));

	{
		Queue<ContainerTestSimple, MemoryBudgets::DataStore> queue2;
		queue.Swap(queue2);
	}

	SEOUL_UNITTESTING_ASSERT_EQUAL(0, queue.GetSize());
	SEOUL_UNITTESTING_ASSERT(queue.IsEmpty());

	simple.m_iA = 53;
	queue.Push(simple);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, queue.GetSize());
	SEOUL_UNITTESTING_ASSERT(!queue.IsEmpty());

	SEOUL_UNITTESTING_ASSERT_EQUAL(53, queue.Back().m_iA);
	SEOUL_UNITTESTING_ASSERT_EQUAL(33, queue.Back().m_B);
	SEOUL_UNITTESTING_ASSERT_EQUAL(53, queue.Begin()->m_iA);
	SEOUL_UNITTESTING_ASSERT_EQUAL(33, queue.Begin()->m_B);
	SEOUL_UNITTESTING_ASSERT_EQUAL(53, (--queue.End())->m_iA);
	SEOUL_UNITTESTING_ASSERT_EQUAL(33, (--queue.End())->m_B);
	SEOUL_UNITTESTING_ASSERT_EQUAL(53, queue.Front().m_iA);
	SEOUL_UNITTESTING_ASSERT_EQUAL(33, queue.Front().m_B);

	queue.Pop();
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, queue.GetSize());
	SEOUL_UNITTESTING_ASSERT(queue.IsEmpty());
}

void QueueTest::TestFind()
{
	// Empty
	{
		Queue<ContainerTestComplex> queue;
		SEOUL_UNITTESTING_ASSERT(!Contains(queue.Begin(), queue.End(), ContainerTestComplex(25)));
		SEOUL_UNITTESTING_ASSERT(!Contains(queue.Begin(), queue.End(), 25));
		SEOUL_UNITTESTING_ASSERT(!ContainsFromBack(queue.Begin(), queue.End(), ContainerTestComplex(25)));
		SEOUL_UNITTESTING_ASSERT(!ContainsFromBack(queue.Begin(), queue.End(), 25));
		SEOUL_UNITTESTING_ASSERT_EQUAL(queue.End(), Find(queue.Begin(), queue.End(), ContainerTestComplex(25)));
		SEOUL_UNITTESTING_ASSERT_EQUAL(queue.End(), Find(queue.Begin(), queue.End(), 25));
		SEOUL_UNITTESTING_ASSERT_EQUAL(queue.End(), FindFromBack(queue.Begin(), queue.End(), ContainerTestComplex(25)));
		SEOUL_UNITTESTING_ASSERT_EQUAL(queue.End(), FindFromBack(queue.Begin(), queue.End(), 25));
	}

	// Not empty.
	{
		Int const aiNumbers[] = { 10, 123, 3, 98, 128, 1498, 3, 5 };

		Queue<ContainerTestComplex> queue;
		for (size_t i = 0; i < SEOUL_ARRAY_COUNT(aiNumbers); ++i)
		{
			queue.Push(ContainerTestComplex(aiNumbers[i]));
		}

		SEOUL_UNITTESTING_ASSERT(!Contains(queue.Begin(), queue.End(), ContainerTestComplex(25)));
		SEOUL_UNITTESTING_ASSERT(!Contains(queue.Begin(), queue.End(), 25));
		SEOUL_UNITTESTING_ASSERT(!ContainsFromBack(queue.Begin(), queue.End(), ContainerTestComplex(25)));
		SEOUL_UNITTESTING_ASSERT(!ContainsFromBack(queue.Begin(), queue.End(), 25));
		SEOUL_UNITTESTING_ASSERT_EQUAL(queue.End(), Find(queue.Begin(), queue.End(), ContainerTestComplex(25)));
		SEOUL_UNITTESTING_ASSERT_EQUAL(queue.End(), Find(queue.Begin(), queue.End(), 25));
		SEOUL_UNITTESTING_ASSERT_EQUAL(queue.End(), FindFromBack(queue.Begin(), queue.End(), ContainerTestComplex(25)));
		SEOUL_UNITTESTING_ASSERT_EQUAL(queue.End(), FindFromBack(queue.Begin(), queue.End(), 25));

		for (size_t i = 0; i < SEOUL_ARRAY_COUNT(aiNumbers); ++i)
		{
			SEOUL_UNITTESTING_ASSERT(Contains(queue.Begin(), queue.End(), ContainerTestComplex(aiNumbers[i])));
			SEOUL_UNITTESTING_ASSERT(Contains(queue.Begin(), queue.End(), aiNumbers[i]));
			SEOUL_UNITTESTING_ASSERT(ContainsFromBack(queue.Begin(), queue.End(), ContainerTestComplex(aiNumbers[i])));
			SEOUL_UNITTESTING_ASSERT(ContainsFromBack(queue.Begin(), queue.End(), aiNumbers[i]));

			if (aiNumbers[i] == 3)
			{
				auto iterA = queue.Begin();
				for (size_t j = 0; j < 2; ++j) { ++iterA; }
				auto iterB = queue.Begin();
				for (size_t j = 0; j < 6; ++j) { ++iterB; }

				SEOUL_UNITTESTING_ASSERT_EQUAL(iterA, Find(queue.Begin(), queue.End(), ContainerTestComplex(aiNumbers[i])));
				SEOUL_UNITTESTING_ASSERT_EQUAL(iterA, Find(queue.Begin(), queue.End(), aiNumbers[i]));
				SEOUL_UNITTESTING_ASSERT_EQUAL(iterB, FindFromBack(queue.Begin(), queue.End(), ContainerTestComplex(aiNumbers[i])));
				SEOUL_UNITTESTING_ASSERT_EQUAL(iterB, FindFromBack(queue.Begin(), queue.End(), aiNumbers[i]));
			}
			else
			{
				auto iter = queue.Begin();
				for (size_t j = 0; j < i; ++j) { ++iter; }

				SEOUL_UNITTESTING_ASSERT_EQUAL(iter, Find(queue.Begin(), queue.End(), ContainerTestComplex(aiNumbers[i])));
				SEOUL_UNITTESTING_ASSERT_EQUAL(iter, Find(queue.Begin(), queue.End(), aiNumbers[i]));
				SEOUL_UNITTESTING_ASSERT_EQUAL(iter, FindFromBack(queue.Begin(), queue.End(), ContainerTestComplex(aiNumbers[i])));
				SEOUL_UNITTESTING_ASSERT_EQUAL(iter, FindFromBack(queue.Begin(), queue.End(), aiNumbers[i]));
			}
		}

	}
}

void QueueTest::TestMethods()
{
	//test LList queue
	{
		Queue<Int> testQueue;

		//put in 5
		for (UInt i =0; i< 5; i++)
		{
			testQueue.Push(i);
			SEOUL_UNITTESTING_ASSERT_EQUAL(i + 1, testQueue.GetSize());
			SEOUL_UNITTESTING_ASSERT_EQUAL((Int)0, testQueue.Front());
		}

		//make a copies
		Queue<Int> copyQueue = testQueue;
		Queue<Int> assignQueue;
		assignQueue = testQueue;

		//take two off
		for (UInt i = 0; i< 2; i++)
		{
			testQueue.Pop();
			SEOUL_UNITTESTING_ASSERT_EQUAL(5 - i - 1, testQueue.GetSize());
		}

		//asert it has the value it should have(2) and size 3
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)2, testQueue.Front());
		SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)3, testQueue.GetSize());


		//pop another two on
		for (UInt i=0; i<2; i++)
		{
			testQueue.Push(i);
			SEOUL_UNITTESTING_ASSERT_EQUAL(3 + i + 1, testQueue.GetSize());
		}

		//pop everything off and put 5 new values
		while (!testQueue.IsEmpty())
		{
			testQueue.Pop();
		}

		for (Int i = 0; i< 5; i++)
		{
			testQueue.Push(10+i);
		}

		//make sure assignQueue and copyQueue have different valuse than
		//test stack
		for (Int i=0; i<5; i++)
		{
			int testVal = testQueue.Front();
			int copyVal = copyQueue.Front();
			int assignVal = assignQueue.Front();


			SEOUL_UNITTESTING_ASSERT( testVal != copyVal);
			SEOUL_UNITTESTING_ASSERT( testVal != assignVal);

			testQueue.Pop();
			copyQueue.Pop();
			assignQueue.Pop();
		}

		//assert that they're all empty
		SEOUL_UNITTESTING_ASSERT( testQueue.IsEmpty() );
		SEOUL_UNITTESTING_ASSERT( copyQueue.IsEmpty() );
		SEOUL_UNITTESTING_ASSERT( assignQueue.IsEmpty() );
	}
}

void QueueTest::TestIterators()
{
	Queue<Int> testQueue;

	for (Int i = 0; i < 6; i++)
	{
		testQueue.Push(i + 10);
	}

	// Test value reads through iterator
	auto iter = testQueue.Begin();
	Int i = 0;
	while (iter != testQueue.End())
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(i + 10, *iter);
		++iter;
		++i;
	}

	// Test value writes through iterator
	iter = testQueue.Begin();
	i = 0;
	while (iter != testQueue.End())
	{
		*iter = 3 * (i + 1);
		SEOUL_UNITTESTING_ASSERT_EQUAL(iter, Find(testQueue.Begin(), testQueue.End(), 3 * (i + 1)));
		++iter;
		++i;
	}
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
