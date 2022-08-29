/**
 * \file StackOrHeapArrayTest.cpp
 * \brief Unit test code for the Seoul vector class
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
#include "StackOrHeapArray.h"
#include "StackOrHeapArrayTest.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(StackOrHeapArrayTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestIterators)
	SEOUL_METHOD(TestMethodsHeap)
	SEOUL_METHOD(TestMethodsStack)
	SEOUL_METHOD(TestStackSized)
SEOUL_END_TYPE()

void StackOrHeapArrayTest::TestIterators()
{
	StackOrHeapArray<Int, 6, MemoryBudgets::TBD> stackOrHeapArray(6);

	for (Int i = 0; i < 6; i++)
	{
		stackOrHeapArray[i] = (i + 10);
	}

	// Test value reads through iterator
	auto iter = stackOrHeapArray.Begin();
	Int i = 0;
	while (iter != stackOrHeapArray.End())
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(i + 10, *iter);
		++iter;
		++i;
	}

	// Test value writes through iterator
	iter = stackOrHeapArray.Begin();
	i = 0;
	while (iter != stackOrHeapArray.End())
	{
		*iter = 3 * i;
		SEOUL_UNITTESTING_ASSERT_EQUAL(3 * i, stackOrHeapArray[i]);
		++iter;
	}
}

void StackOrHeapArrayTest::TestMethodsHeap()
{
	{
		StackOrHeapArray<ContainerTestComplex, 126, MemoryBudgets::Compression> stackOrHeapArray(357);
		StackOrHeapArray<ContainerTestComplex, 126, MemoryBudgets::Compression> const& stackOrHeapArrayConst = stackOrHeapArray;
		SEOUL_UNITTESTING_ASSERT_EQUAL(126 + 357, ContainerTestComplex::s_iCount);

		SEOUL_UNITTESTING_ASSERT(!stackOrHeapArray.IsUsingStack());
		SEOUL_UNITTESTING_ASSERT_EQUAL(357, stackOrHeapArray.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(357, stackOrHeapArrayConst.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(357 * sizeof(ContainerTestComplex), stackOrHeapArray.GetSizeInBytes());
		SEOUL_UNITTESTING_ASSERT_EQUAL(357 * sizeof(ContainerTestComplex), stackOrHeapArrayConst.GetSizeInBytes());

		for (UInt32 i = 0u; i < stackOrHeapArray.GetSize(); ++i)
		{
			stackOrHeapArray.At(i) = ContainerTestComplex(i);
		}

		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(356), stackOrHeapArray.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(356), stackOrHeapArrayConst.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(0), *stackOrHeapArray.Begin());
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(0), *stackOrHeapArrayConst.Begin());
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(356), *(stackOrHeapArray.End() - 1));
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(356), *(stackOrHeapArrayConst.End() - 1));
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(0), stackOrHeapArray.Front());
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(0), stackOrHeapArrayConst.Front());

		for (UInt32 i = 0u; i < stackOrHeapArray.GetSize(); ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(
				ContainerTestComplex(i),
				stackOrHeapArray.At(i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(
				ContainerTestComplex(i),
				stackOrHeapArrayConst.At(i));

			SEOUL_UNITTESTING_ASSERT_EQUAL(
				ContainerTestComplex(i),
				*(stackOrHeapArray.Begin() + i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(
				ContainerTestComplex(i),
				*(stackOrHeapArrayConst.Begin() + i));

			SEOUL_UNITTESTING_ASSERT_EQUAL(
				ContainerTestComplex(i),
				*(stackOrHeapArray.Data() + i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(
				ContainerTestComplex(i),
				*(stackOrHeapArrayConst.Data() + i));

			SEOUL_UNITTESTING_ASSERT_EQUAL(
				ContainerTestComplex(i),
				*(stackOrHeapArray.End() - 357 + i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(
				ContainerTestComplex(i),
				*(stackOrHeapArrayConst.End() - 357 + i));

			SEOUL_UNITTESTING_ASSERT_EQUAL(
				ContainerTestComplex(i),
				*(stackOrHeapArray.Get(i)));
			SEOUL_UNITTESTING_ASSERT_EQUAL(
				ContainerTestComplex(i),
				*(stackOrHeapArrayConst.Get(i)));

			SEOUL_UNITTESTING_ASSERT_EQUAL(
				ContainerTestComplex(i),
				stackOrHeapArray[i]);
			SEOUL_UNITTESTING_ASSERT_EQUAL(
				ContainerTestComplex(i),
				stackOrHeapArrayConst[i]);
		}

		// Swap.
		SEOUL_UNITTESTING_ASSERT_EQUAL(126 + 357, ContainerTestComplex::s_iCount);
		StackOrHeapArray<ContainerTestComplex, 126, MemoryBudgets::Compression> stackOrHeapArray2(357);
		SEOUL_UNITTESTING_ASSERT_EQUAL(252 + 714, ContainerTestComplex::s_iCount);
		SEOUL_UNITTESTING_ASSERT(!stackOrHeapArray2.IsUsingStack());
		for (UInt32 i = 0u; i < stackOrHeapArray2.GetSize(); ++i)
		{
			stackOrHeapArray2.At(i) = ContainerTestComplex(357 - i);
		}

		stackOrHeapArray2.Swap(stackOrHeapArray);

		// Check
		for (UInt32 i = 0u; i < stackOrHeapArray2.GetSize(); ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(
				ContainerTestComplex(i),
				stackOrHeapArray2[i]);
			SEOUL_UNITTESTING_ASSERT_EQUAL(
				ContainerTestComplex(357 - i),
				stackOrHeapArray[i]);
		}
	}
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
}

void StackOrHeapArrayTest::TestMethodsStack()
{
	{
		StackOrHeapArray<ContainerTestComplex, 357, MemoryBudgets::Compression> stackOrHeapArray(357);
		StackOrHeapArray<ContainerTestComplex, 357, MemoryBudgets::Compression> const& stackOrHeapArrayConst = stackOrHeapArray;
		SEOUL_UNITTESTING_ASSERT_EQUAL(357, ContainerTestComplex::s_iCount);

		SEOUL_UNITTESTING_ASSERT(stackOrHeapArray.IsUsingStack());
		SEOUL_UNITTESTING_ASSERT_EQUAL(357, stackOrHeapArray.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(357, stackOrHeapArrayConst.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(357 * sizeof(ContainerTestComplex), stackOrHeapArray.GetSizeInBytes());
		SEOUL_UNITTESTING_ASSERT_EQUAL(357 * sizeof(ContainerTestComplex), stackOrHeapArrayConst.GetSizeInBytes());

		for (UInt32 i = 0u; i < stackOrHeapArray.GetSize(); ++i)
		{
			stackOrHeapArray.At(i) = ContainerTestComplex(i);
		}

		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(356), stackOrHeapArray.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(356), stackOrHeapArrayConst.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(0), *stackOrHeapArray.Begin());
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(0), *stackOrHeapArrayConst.Begin());
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(356), *(stackOrHeapArray.End() - 1));
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(356), *(stackOrHeapArrayConst.End() - 1));
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(0), stackOrHeapArray.Front());
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(0), stackOrHeapArrayConst.Front());

		for (UInt32 i = 0u; i < stackOrHeapArray.GetSize(); ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(
				ContainerTestComplex(i),
				stackOrHeapArray.At(i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(
				ContainerTestComplex(i),
				stackOrHeapArrayConst.At(i));

			SEOUL_UNITTESTING_ASSERT_EQUAL(
				ContainerTestComplex(i),
				*(stackOrHeapArray.Begin() + i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(
				ContainerTestComplex(i),
				*(stackOrHeapArrayConst.Begin() + i));

			SEOUL_UNITTESTING_ASSERT_EQUAL(
				ContainerTestComplex(i),
				*(stackOrHeapArray.Data() + i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(
				ContainerTestComplex(i),
				*(stackOrHeapArrayConst.Data() + i));

			SEOUL_UNITTESTING_ASSERT_EQUAL(
				ContainerTestComplex(i),
				*(stackOrHeapArray.End() - 357 + i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(
				ContainerTestComplex(i),
				*(stackOrHeapArrayConst.End() - 357 + i));

			SEOUL_UNITTESTING_ASSERT_EQUAL(
				ContainerTestComplex(i),
				*(stackOrHeapArray.Get(i)));
			SEOUL_UNITTESTING_ASSERT_EQUAL(
				ContainerTestComplex(i),
				*(stackOrHeapArrayConst.Get(i)));

			SEOUL_UNITTESTING_ASSERT_EQUAL(
				ContainerTestComplex(i),
				stackOrHeapArray[i]);
			SEOUL_UNITTESTING_ASSERT_EQUAL(
				ContainerTestComplex(i),
				stackOrHeapArrayConst[i]);
		}

		// Swap.
		SEOUL_UNITTESTING_ASSERT_EQUAL(357, ContainerTestComplex::s_iCount);
		StackOrHeapArray<ContainerTestComplex, 357, MemoryBudgets::Compression> stackOrHeapArray2(357);
		SEOUL_UNITTESTING_ASSERT_EQUAL(714, ContainerTestComplex::s_iCount);
		SEOUL_UNITTESTING_ASSERT(stackOrHeapArray2.IsUsingStack());
		for (UInt32 i = 0u; i < stackOrHeapArray2.GetSize(); ++i)
		{
			stackOrHeapArray2.At(i) = ContainerTestComplex(357 - i);
		}

		stackOrHeapArray2.Swap(stackOrHeapArray);

		// Check
		for (UInt32 i = 0u; i < stackOrHeapArray2.GetSize(); ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(
				ContainerTestComplex(i),
				stackOrHeapArray2[i]);
			SEOUL_UNITTESTING_ASSERT_EQUAL(
				ContainerTestComplex(357 - i),
				stackOrHeapArray[i]);
		}
	}

	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
}

void StackOrHeapArrayTest::TestStackSized()
{
	// Default initialization with Int
	{
		StackOrHeapArray<Int, 15, MemoryBudgets::TBD> stackOrHeapArray(15);

		// Should still be using the stack.
		SEOUL_UNITTESTING_ASSERT(stackOrHeapArray.IsUsingStack());

		// Test for zero initialized Simple.
		for (UInt32 i = 0; i < stackOrHeapArray.GetSize(); ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, stackOrHeapArray[i]);
		}

		SEOUL_UNITTESTING_ASSERT_EQUAL(15, stackOrHeapArray.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(15 * sizeof(Int), stackOrHeapArray.GetSizeInBytes());
		SEOUL_UNITTESTING_ASSERT(!stackOrHeapArray.IsEmpty());
	}

	// Default initialization with Simple
	{
		StackOrHeapArray<ContainerTestSimple, 33, MemoryBudgets::TBD> stackOrHeapArray(27);

		// Should still be using the stack.
		SEOUL_UNITTESTING_ASSERT(stackOrHeapArray.IsUsingStack());

		// Test for zero initialized Simple.
		for (UInt32 i = 0; i < stackOrHeapArray.GetSize(); ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, stackOrHeapArray[i].m_iA);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, stackOrHeapArray[i].m_B);
		}

		SEOUL_UNITTESTING_ASSERT_EQUAL(27, stackOrHeapArray.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(27 * sizeof(ContainerTestSimple), stackOrHeapArray.GetSizeInBytes());
		SEOUL_UNITTESTING_ASSERT(!stackOrHeapArray.IsEmpty());
	}

	// Default initialization with Complex
	{
		{
			StackOrHeapArray<ContainerTestComplex, 13, MemoryBudgets::TBD> stackOrHeapArray(13);

			// Should still be using the stack.
			SEOUL_UNITTESTING_ASSERT(stackOrHeapArray.IsUsingStack());

			// Check count.
			SEOUL_UNITTESTING_ASSERT_EQUAL(13, ContainerTestComplex::s_iCount);

			// Test for default constructed Simple.
			for (UInt32 i = 0; i < stackOrHeapArray.GetSize(); ++i)
			{
				SEOUL_UNITTESTING_ASSERT_EQUAL(33, stackOrHeapArray[i].m_iFixedValue);
			}

			SEOUL_UNITTESTING_ASSERT_EQUAL(13, stackOrHeapArray.GetSize());
			SEOUL_UNITTESTING_ASSERT_EQUAL(13 * sizeof(ContainerTestComplex), stackOrHeapArray.GetSizeInBytes());
			SEOUL_UNITTESTING_ASSERT(!stackOrHeapArray.IsEmpty());
		}

		// Check count.
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
	}
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
