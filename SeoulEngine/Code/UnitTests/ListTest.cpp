/**
 * \file ListTest.cpp
 * \brief Unit test code for the Seoul List<> class
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
#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionDefine.h"
#include "UnitTesting.h"
#include "List.h"
#include "ListTest.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(ListTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestAssignBuiltin)
	SEOUL_METHOD(TestAssignComplex)
	SEOUL_METHOD(TestAssignSimple)
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
	SEOUL_METHOD(TestEqualityBuiltin)
	SEOUL_METHOD(TestEqualityComplex)
	SEOUL_METHOD(TestEqualitySimple)
	SEOUL_METHOD(TestFind)
	SEOUL_METHOD(TestMethods)
	SEOUL_METHOD(TestInsertBuiltin)
	SEOUL_METHOD(TestInsertComplex)
	SEOUL_METHOD(TestInsertSimple)
	SEOUL_METHOD(TestEraseBuiltin)
	SEOUL_METHOD(TestEraseComplex)
	SEOUL_METHOD(TestEraseSimple)
	SEOUL_METHOD(TestEraseReturnBuiltin)
	SEOUL_METHOD(TestEraseReturnComplex)
	SEOUL_METHOD(TestEraseReturnSimple)
	SEOUL_METHOD(TestEraseRangeReturnBuiltin)
	SEOUL_METHOD(TestEraseRangeReturnComplex)
	SEOUL_METHOD(TestEraseRangeReturnSimple)
	SEOUL_METHOD(TestIterators)
	SEOUL_METHOD(TestRangedFor)
	SEOUL_METHOD(TestRemoveBuiltin)
	SEOUL_METHOD(TestRemoveComplex)
	SEOUL_METHOD(TestRemoveComplexCoerce)
	SEOUL_METHOD(TestRemoveSimple)
	SEOUL_METHOD(TestReverseIterators)
	SEOUL_METHOD(TestRemoveRegressionBuiltin)
	SEOUL_METHOD(TestRemoveRegressionComplex)
	SEOUL_METHOD(TestRemoveRegressionSimple)
	SEOUL_METHOD(TestRemoveFirstInstanceBuiltin)
	SEOUL_METHOD(TestRemoveFirstInstanceComplex)
	SEOUL_METHOD(TestRemoveFirstInstanceComplexCoerce)
	SEOUL_METHOD(TestRemoveFirstInstanceSimple)
	SEOUL_METHOD(TestRemoveCountBuiltin)
	SEOUL_METHOD(TestRemoveCountComplex)
	SEOUL_METHOD(TestRemoveCountSimple)
SEOUL_END_TYPE()

void ListTest::TestAssignBuiltin()
{
	// Copy self
	{
		List<UInt16, MemoryBudgets::DataStore> list1;
		list1.PushBack(UInt16(7));
		list1.PushBack(UInt16(11));
		list1.PushBack(UInt16(25));

		list1 = list1;
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, list1.GetSize());
		SEOUL_UNITTESTING_ASSERT(!list1.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(UInt16(25), list1.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(UInt16(7), list1.Front());
		SEOUL_UNITTESTING_ASSERT_EQUAL(UInt16(11), *(++list1.Begin()));
	}

	// Copy
	{
		List<UInt16, MemoryBudgets::DataStore> list1;
		list1.PushBack(UInt16(7));
		list1.PushBack(UInt16(11));
		list1.PushBack(UInt16(25));

		List<UInt16, MemoryBudgets::DataStore> list2 = list1;
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, list2.GetSize());
		SEOUL_UNITTESTING_ASSERT(!list2.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(list1.Back(), list2.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(list1.Front(), list2.Front());

		auto i1 = list1.Begin();
		auto i2 = list2.Begin();
		for (; list1.End() != i1; ++i1, ++i2)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(*i1, *i2);
		}
	}

	// Copy templated
	{
		List<UInt16, MemoryBudgets::Falcon> list1;
		list1.PushBack(UInt16(7));
		list1.PushBack(UInt16(11));
		list1.PushBack(UInt16(25));

		List<UInt16, MemoryBudgets::Physics> list2;
		list2.PushBack(UInt16(112));
		list2.PushBack(UInt16(12));

		list2 = list1;
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, list2.GetSize());
		SEOUL_UNITTESTING_ASSERT(!list2.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(list1.Back(), list2.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(list1.Front(), list2.Front());

		auto i1 = list1.Begin();
		auto i2 = list2.Begin();
		for (; list1.End() != i1; ++i1, ++i2)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(*i1, *i2);
		}
	}

	// Iterator full
	{
		List<UInt16, MemoryBudgets::Falcon> list1;
		list1.PushBack(UInt16(7));
		list1.PushBack(UInt16(11));
		list1.PushBack(UInt16(25));

		List<UInt16, MemoryBudgets::Physics> list2;
		list2.PushBack(UInt16(191));
		list2.PushBack(UInt16(3981));
		list2.PushBack(UInt16(1298));
		list2.PushBack(UInt16(787));
		list2.PushBack(UInt16(12));

		list2.Assign(list1.Begin(), list1.End());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, list2.GetSize());
		SEOUL_UNITTESTING_ASSERT(!list2.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(list1.Back(), list2.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(list1.Front(), list2.Front());

		auto i1 = list1.Begin();
		auto i2 = list2.Begin();
		for (; list1.End() != i1; ++i1, ++i2)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(*i1, *i2);
		}
	}

	// Iterator partial
	{
		List<UInt16, MemoryBudgets::Falcon> list1;
		list1.PushBack(UInt16(3));
		list1.PushBack(UInt16(7));
		list1.PushBack(UInt16(11));
		list1.PushBack(UInt16(25));
		list1.PushBack(UInt16(91));

		List<UInt16, MemoryBudgets::Physics> list2;
		list2.PushBack(UInt16(191));
		list2.PushBack(UInt16(3981));
		list2.PushBack(UInt16(1298));

		list2.Assign((++list1.Begin()), (--list1.End()));
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, list2.GetSize());
		SEOUL_UNITTESTING_ASSERT(!list2.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(*(--(--list1.End())), list2.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(*(++list1.Begin()), list2.Front());

		auto i1 = list1.Begin();
		++i1;
		auto i2 = list2.Begin();
		for (; list2.End() != i2; ++i1, ++i2)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(*i1, *i2);
		}
	}

	// Size with default value.
	{
		List<UInt16, MemoryBudgets::Falcon> list;
		list.PushBack(UInt16(908));
		list.PushBack(UInt16(124));
		list.PushBack(UInt16(457));

		list.Assign(5);
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, list.GetSize());
		SEOUL_UNITTESTING_ASSERT(!list.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(UInt16(), list.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(UInt16(), list.Front());

		auto i1 = list.Begin();
		for (; list.End() != i1; ++i1)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(UInt16(), *i1);
		}
	}

	// Size with value.
	{
		List<UInt16, MemoryBudgets::Falcon> list;
		list.PushBack(UInt16(3409));
		list.PushBack(UInt16(144));
		list.PushBack(UInt16(389));

		list.Assign(5, UInt16(77));
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, list.GetSize());
		SEOUL_UNITTESTING_ASSERT(!list.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(UInt16(77), list.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(UInt16(77), list.Front());

		auto i1 = list.Begin();
		for (; list.End() != i1; ++i1)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(UInt16(77), *i1);
		}
	}
}
void ListTest::TestAssignComplex()
{
	// Copy self
	{
		List<ContainerTestComplex, MemoryBudgets::DataStore> list1;
		list1.PushBack(ContainerTestComplex(7));
		list1.PushBack(ContainerTestComplex(11));
		list1.PushBack(ContainerTestComplex(25));

		SEOUL_UNITTESTING_ASSERT_EQUAL(3, ContainerTestComplex::s_iCount);
		list1 = list1;
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, ContainerTestComplex::s_iCount);

		SEOUL_UNITTESTING_ASSERT_EQUAL(3, list1.GetSize());
		SEOUL_UNITTESTING_ASSERT(!list1.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(25), list1.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(7), list1.Front());
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(11), *(++list1.Begin()));
	}
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);

	// Copy
	{
		List<ContainerTestComplex, MemoryBudgets::DataStore> list1;
		list1.PushBack(ContainerTestComplex(7));
		list1.PushBack(ContainerTestComplex(11));
		list1.PushBack(ContainerTestComplex(25));

		List<ContainerTestComplex, MemoryBudgets::DataStore> list2 = list1;
		SEOUL_UNITTESTING_ASSERT_EQUAL(6, ContainerTestComplex::s_iCount);
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, list2.GetSize());
		SEOUL_UNITTESTING_ASSERT(!list2.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(list1.Back(), list2.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(list1.Front(), list2.Front());

		auto i1 = list1.Begin();
		auto i2 = list2.Begin();
		for (; list1.End() != i1; ++i1, ++i2)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(*i1, *i2);
		}
	}
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);

	// Copy templated
	{
		List<ContainerTestComplex, MemoryBudgets::Falcon> list1;
		list1.PushBack(ContainerTestComplex(7));
		list1.PushBack(ContainerTestComplex(11));
		list1.PushBack(ContainerTestComplex(25));

		List<ContainerTestComplex, MemoryBudgets::Physics> list2;
		list2.PushBack(ContainerTestComplex(112));
		list2.PushBack(ContainerTestComplex(12));

		list2 = list1;
		SEOUL_UNITTESTING_ASSERT_EQUAL(6, ContainerTestComplex::s_iCount);
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, list2.GetSize());
		SEOUL_UNITTESTING_ASSERT(!list2.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(list1.Back(), list2.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(list1.Front(), list2.Front());

		auto i1 = list1.Begin();
		auto i2 = list2.Begin();
		for (; list1.End() != i1; ++i1, ++i2)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(*i1, *i2);
		}
	}
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);

	// Iterator full
	{
		List<ContainerTestComplex, MemoryBudgets::Falcon> list1;
		list1.PushBack(ContainerTestComplex(7));
		list1.PushBack(ContainerTestComplex(11));
		list1.PushBack(ContainerTestComplex(25));

		List<ContainerTestComplex, MemoryBudgets::Physics> list2;
		list2.PushBack(ContainerTestComplex(191));
		list2.PushBack(ContainerTestComplex(3981));
		list2.PushBack(ContainerTestComplex(1298));
		list2.PushBack(ContainerTestComplex(787));
		list2.PushBack(ContainerTestComplex(12));

		list2.Assign(list1.Begin(), list1.End());
		SEOUL_UNITTESTING_ASSERT_EQUAL(6, ContainerTestComplex::s_iCount);
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, list2.GetSize());
		SEOUL_UNITTESTING_ASSERT(!list2.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(list1.Back(), list2.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(list1.Front(), list2.Front());

		auto i1 = list1.Begin();
		auto i2 = list2.Begin();
		for (; list1.End() != i1; ++i1, ++i2)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(*i1, *i2);
		}
	}
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);

	// Iterator partial
	{
		List<ContainerTestComplex, MemoryBudgets::Falcon> list1;
		list1.PushBack(ContainerTestComplex(3));
		list1.PushBack(ContainerTestComplex(7));
		list1.PushBack(ContainerTestComplex(11));
		list1.PushBack(ContainerTestComplex(25));
		list1.PushBack(ContainerTestComplex(91));

		List<ContainerTestComplex, MemoryBudgets::Physics> list2;
		list2.PushBack(ContainerTestComplex(191));
		list2.PushBack(ContainerTestComplex(3981));
		list2.PushBack(ContainerTestComplex(1298));

		list2.Assign((++list1.Begin()), (--list1.End()));
		SEOUL_UNITTESTING_ASSERT_EQUAL(8, ContainerTestComplex::s_iCount);
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, list2.GetSize());
		SEOUL_UNITTESTING_ASSERT(!list2.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(*(--(--list1.End())), list2.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(*(++list1.Begin()), list2.Front());

		auto i1 = list1.Begin();
		++i1;
		auto i2 = list2.Begin();
		for (; list2.End() != i2; ++i1, ++i2)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(*i1, *i2);
		}
	}
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);

	// Size with default value.
	{
		List<ContainerTestComplex, MemoryBudgets::Falcon> list;
		list.PushBack(ContainerTestComplex(908));
		list.PushBack(ContainerTestComplex(124));
		list.PushBack(ContainerTestComplex(457));

		list.Assign(5);
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, ContainerTestComplex::s_iCount);
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, list.GetSize());
		SEOUL_UNITTESTING_ASSERT(!list.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(), list.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(), list.Front());

		auto i1 = list.Begin();
		for (; list.End() != i1; ++i1)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(), *i1);
		}
	}

	// Size with value.
	{
		List<ContainerTestComplex, MemoryBudgets::Falcon> list;
		list.PushBack(ContainerTestComplex(3409));
		list.PushBack(ContainerTestComplex(144));
		list.PushBack(ContainerTestComplex(389));

		list.Assign(5, ContainerTestComplex(77));
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, ContainerTestComplex::s_iCount);
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, list.GetSize());
		SEOUL_UNITTESTING_ASSERT(!list.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(77), list.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(77), list.Front());

		auto i1 = list.Begin();
		for (; list.End() != i1; ++i1)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(77), *i1);
		}
	}
}

void ListTest::TestAssignSimple()
{
	// Copy self
	{
		List<ContainerTestSimple, MemoryBudgets::DataStore> list1;
		list1.PushBack(ContainerTestSimple::Create(7));
		list1.PushBack(ContainerTestSimple::Create(11));
		list1.PushBack(ContainerTestSimple::Create(25));

		list1 = list1;
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, list1.GetSize());
		SEOUL_UNITTESTING_ASSERT(!list1.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(25), list1.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(7), list1.Front());
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(11), *(++list1.Begin()));
	}

	// Copy
	{
		ContainerTestSimple simple;
		simple.m_B = 33;
		simple.m_iA = 7;
		List<ContainerTestSimple, MemoryBudgets::DataStore> list1;
		list1.PushBack(simple);
		simple.m_iA = 11;
		list1.PushBack(simple);
		simple.m_iA = 25;
		list1.PushBack(simple);

		List<ContainerTestSimple, MemoryBudgets::DataStore> list2 = list1;
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, list2.GetSize());
		SEOUL_UNITTESTING_ASSERT(!list2.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(list1.Back(), list2.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(list1.Front(), list2.Front());

		auto i1 = list1.Begin();
		auto i2 = list2.Begin();
		for (; list1.End() != i1; ++i1, ++i2)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(*i1, *i2);
		}
	}

	// Copy templated
	{
		ContainerTestSimple simple;
		simple.m_B = 33;
		simple.m_iA = 7;
		List<ContainerTestSimple, MemoryBudgets::DataStore> list1;
		list1.PushBack(simple);
		simple.m_iA = 11;
		list1.PushBack(simple);
		simple.m_iA = 25;
		list1.PushBack(simple);

		List<ContainerTestSimple, MemoryBudgets::Physics> list2;
		simple.m_iA = 122;
		list2.PushBack(simple);
		simple.m_iA = 12;
		list2.PushBack(simple);

		list2 = list1;
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, list2.GetSize());
		SEOUL_UNITTESTING_ASSERT(!list2.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(list1.Back(), list2.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(list1.Front(), list2.Front());

		auto i1 = list1.Begin();
		auto i2 = list2.Begin();
		for (; list1.End() != i1; ++i1, ++i2)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(*i1, *i2);
		}
	}

	// Iterator full
	{
		ContainerTestSimple simple;
		simple.m_B = 33;
		simple.m_iA = 7;
		List<ContainerTestSimple, MemoryBudgets::DataStore> list1;
		list1.PushBack(simple);
		simple.m_iA = 11;
		list1.PushBack(simple);
		simple.m_iA = 25;
		list1.PushBack(simple);

		List<ContainerTestSimple, MemoryBudgets::Physics> list2;
		simple.m_iA = 191;
		list2.PushBack(simple);
		simple.m_iA = 3981;
		list2.PushBack(simple);
		simple.m_iA = 1298;
		list2.PushBack(simple);
		simple.m_iA = 787;
		list2.PushBack(simple);
		simple.m_iA = 12;
		list2.PushBack(simple);

		list2.Assign(list1.Begin(), list1.End());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, list2.GetSize());
		SEOUL_UNITTESTING_ASSERT(!list2.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(list1.Back(), list2.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(list1.Front(), list2.Front());

		auto i1 = list1.Begin();
		auto i2 = list2.Begin();
		for (; list1.End() != i1; ++i1, ++i2)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(*i1, *i2);
		}
	}

	// Iterator partial
	{
		ContainerTestSimple simple;
		simple.m_B = 33;
		List<ContainerTestSimple, MemoryBudgets::Falcon> list1;
		simple.m_iA = 3;
		list1.PushBack(simple);
		simple.m_iA = 7;
		list1.PushBack(simple);
		simple.m_iA = 11;
		list1.PushBack(simple);
		simple.m_iA = 25;
		list1.PushBack(simple);
		simple.m_iA = 91;
		list1.PushBack(simple);

		List<ContainerTestSimple, MemoryBudgets::Physics> list2;
		simple.m_iA = 191;
		list2.PushBack(simple);
		simple.m_iA = 3981;
		list2.PushBack(simple);
		simple.m_iA = 1298;
		list2.PushBack(simple);

		list2.Assign((++list1.Begin()), (--list1.End()));
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, list2.GetSize());
		SEOUL_UNITTESTING_ASSERT(!list2.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(*(--(--list1.End())), list2.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(*(++list1.Begin()), list2.Front());

		auto i1 = list1.Begin();
		++i1;
		auto i2 = list2.Begin();
		for (; list2.End() != i2; ++i1, ++i2)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(*i1, *i2);
		}
	}

	// Size with default value.
	{
		ContainerTestSimple simple;
		simple.m_B = 33;
		List<ContainerTestSimple, MemoryBudgets::Falcon> list;
		simple.m_iA = 908;
		list.PushBack(simple);
		simple.m_iA = 124;
		list.PushBack(simple);
		simple.m_iA = 457;
		list.PushBack(simple);

		list.Assign(5);
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, list.GetSize());
		SEOUL_UNITTESTING_ASSERT(!list.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple(), list.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple(), list.Front());

		auto i1 = list.Begin();
		for (; list.End() != i1; ++i1)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple(), *i1);
		}
	}

	// Size with value.
	{
		ContainerTestSimple simple;
		simple.m_B = 33;
		List<ContainerTestSimple, MemoryBudgets::Falcon> list;
		simple.m_iA = 3904;
		list.PushBack(simple);
		simple.m_iA = 144;
		list.PushBack(simple);
		simple.m_iA = 389;
		list.PushBack(simple);

		simple.m_iA = 77;
		list.Assign(5, simple);
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, list.GetSize());
		SEOUL_UNITTESTING_ASSERT(!list.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(simple, list.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(simple, list.Front());

		auto i1 = list.Begin();
		for (; list.End() != i1; ++i1)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(simple, *i1);
		}
	}
}

void ListTest::TestBasic()
{
	List<Int> testList;
	SEOUL_UNITTESTING_ASSERT(testList.GetSize() == 0 );

	//add ten items
	for (Int i = 0; i< 10; i++)
	{
		testList.PushBack(i);
	}
	SEOUL_UNITTESTING_ASSERT(testList.GetSize() == 10 );

	//make sure they are in the right order
	auto iter = testList.Begin();
	Int j = 0;
	while ( iter != testList.End() )
	{
		SEOUL_UNITTESTING_ASSERT( *iter == j );
		++iter;
		j++;
	}

	SEOUL_UNITTESTING_ASSERT(j == 10 );

	//now do the same backwards
	testList.Clear();
	SEOUL_UNITTESTING_ASSERT(testList.GetSize() == 0 );

	//add ten items
	for (Int i = 0; i< 10; i++)
	{
		testList.PushFront(i);
	}
	SEOUL_UNITTESTING_ASSERT(testList.GetSize() == 10 );

	//make sure they are in the right order
	iter = testList.Begin();
	j = 0;
	while ( iter != testList.End() )
	{
		SEOUL_UNITTESTING_ASSERT( *iter == 10-j-1 );
		++iter;
		j++;
	}

	SEOUL_UNITTESTING_ASSERT(j == 10 );
}

void ListTest::TestClearBuiltin()
{
	List<UInt16, MemoryBudgets::Audio> list;
	list.PushBack(UInt16(23));
	list.PushBack(UInt16(194));
	list.PushBack(UInt16(119));

	// Clear should destroy elements but leave capacity.
	list.Clear();
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(list.IsEmpty());

	list.PushBack(UInt16(7));
	list.PushBack(UInt16(1123));
	list.PushBack(UInt16(434));
	list.PushBack(UInt16(342));
	list.PushBack(UInt16(23989));

	auto i = list.Begin();
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, list.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(*i++, 7);
	SEOUL_UNITTESTING_ASSERT_EQUAL(*i++, 1123);
	SEOUL_UNITTESTING_ASSERT_EQUAL(*i++, 434);
	SEOUL_UNITTESTING_ASSERT_EQUAL(*i++, 342);
	SEOUL_UNITTESTING_ASSERT_EQUAL(*i++, 23989);

	// Clear again.
	list.Clear();
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(list.IsEmpty());

	list.PushBack(UInt16(3));
	list.PushBack(UInt16(124));
	list.PushBack(UInt16(342));
	list.PushBack(UInt16(12));
	list.PushBack(UInt16(33));
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, list.GetSize());

	// Now do the "swap trick". This should completely free the memory.
	{
		List<UInt16, MemoryBudgets::Audio> list2;
		list2.Swap(list);

		// list is now empty
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
		SEOUL_UNITTESTING_ASSERT(list.IsEmpty());

		// list2 has list's state.
		i = list2.Begin();
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, list2.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(*i++, 3);
		SEOUL_UNITTESTING_ASSERT_EQUAL(*i++, 124);
		SEOUL_UNITTESTING_ASSERT_EQUAL(*i++, 342);
		SEOUL_UNITTESTING_ASSERT_EQUAL(*i++, 12);
		SEOUL_UNITTESTING_ASSERT_EQUAL(*i++, 33);
	}
}

void ListTest::TestClearComplex()
{
	List<ContainerTestComplex, MemoryBudgets::Audio> list;
	list.PushBack(ContainerTestComplex(23));
	list.PushBack(ContainerTestComplex(194));
	list.PushBack(ContainerTestComplex(119));

	SEOUL_UNITTESTING_ASSERT_EQUAL(3, ContainerTestComplex::s_iCount);

	// Clear should destroy elements but leave capacity.
	list.Clear();
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(list.IsEmpty());

	list.PushBack(ContainerTestComplex(7));
	list.PushBack(ContainerTestComplex(1123));
	list.PushBack(ContainerTestComplex(434));
	list.PushBack(ContainerTestComplex(342));
	list.PushBack(ContainerTestComplex(23989));

	auto i = list.Begin();
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, ContainerTestComplex::s_iCount);
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, list.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(*i++, 7);
	SEOUL_UNITTESTING_ASSERT_EQUAL(*i++, 1123);
	SEOUL_UNITTESTING_ASSERT_EQUAL(*i++, 434);
	SEOUL_UNITTESTING_ASSERT_EQUAL(*i++, 342);
	SEOUL_UNITTESTING_ASSERT_EQUAL(*i++, 23989);

	// Clear again.
	list.Clear();
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(list.IsEmpty());

	list.PushBack(ContainerTestComplex(3));
	list.PushBack(ContainerTestComplex(124));
	list.PushBack(ContainerTestComplex(342));
	list.PushBack(ContainerTestComplex(12));
	list.PushBack(ContainerTestComplex(33));
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, ContainerTestComplex::s_iCount);
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, list.GetSize());

	// Now do the "swap trick". This should completely free the memory.
	{
		List<ContainerTestComplex, MemoryBudgets::Audio> list2;
		list2.Swap(list);

		// list is now empty
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, ContainerTestComplex::s_iCount);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
		SEOUL_UNITTESTING_ASSERT(list.IsEmpty());

		// list2 has list's state.
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, ContainerTestComplex::s_iCount);
		i = list2.Begin();
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, list2.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(*i++, 3);
		SEOUL_UNITTESTING_ASSERT_EQUAL(*i++, 124);
		SEOUL_UNITTESTING_ASSERT_EQUAL(*i++, 342);
		SEOUL_UNITTESTING_ASSERT_EQUAL(*i++, 12);
		SEOUL_UNITTESTING_ASSERT_EQUAL(*i++, 33);
	}

	// All gone.
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
}

void ListTest::TestClearSimple()
{
	List<ContainerTestSimple, MemoryBudgets::Audio> list;
	ContainerTestSimple simple;
	simple.m_B = 33;
	simple.m_iA = 23;
	list.PushBack(simple);
	simple.m_iA = 194;
	list.PushBack(simple);
	simple.m_iA = 119;
	list.PushBack(simple);

	// Clear should destroy elements but leave capacity.
	list.Clear();
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(list.IsEmpty());

	simple.m_iA = 7;
	list.PushBack(simple);
	simple.m_iA = 1123;
	list.PushBack(simple);
	simple.m_iA = 434;
	list.PushBack(simple);
	simple.m_iA = 342;
	list.PushBack(simple);
	simple.m_iA = 23989;
	list.PushBack(simple);

	auto i = list.Begin();
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, list.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(i->m_iA, 7); ++i;
	SEOUL_UNITTESTING_ASSERT_EQUAL(i->m_iA, 1123); ++i;
	SEOUL_UNITTESTING_ASSERT_EQUAL(i->m_iA, 434); ++i;
	SEOUL_UNITTESTING_ASSERT_EQUAL(i->m_iA, 342); ++i;
	SEOUL_UNITTESTING_ASSERT_EQUAL(i->m_iA, 23989); ++i;

	// Clear again.
	list.Clear();
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(list.IsEmpty());

	simple.m_iA = 3;
	list.PushBack(simple);
	simple.m_iA = 124;
	list.PushBack(simple);
	simple.m_iA = 342;
	list.PushBack(simple);
	simple.m_iA = 12;
	list.PushBack(simple);
	simple.m_iA = 33;
	list.PushBack(simple);
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, list.GetSize());

	// Now do the "swap trick". This should completely free the memory.
	{
		List<ContainerTestSimple, MemoryBudgets::Audio> list2;
		list2.Swap(list);

		// list is now empty
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
		SEOUL_UNITTESTING_ASSERT(list.IsEmpty());

		// list2 has list's state.
		i = list2.Begin();
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, list2.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(i->m_iA, 3); ++i;
		SEOUL_UNITTESTING_ASSERT_EQUAL(i->m_iA, 124); ++i;
		SEOUL_UNITTESTING_ASSERT_EQUAL(i->m_iA, 342); ++i;
		SEOUL_UNITTESTING_ASSERT_EQUAL(i->m_iA, 12); ++i;
		SEOUL_UNITTESTING_ASSERT_EQUAL(i->m_iA, 33); ++i;
	}
}

void ListTest::TestConstructorBuiltin()
{
	// Default.
	{
		List<Int64, MemoryBudgets::DataStore> list;
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
		SEOUL_UNITTESTING_ASSERT(list.IsEmpty());
	}

	// Copy
	{
		List<Int64, MemoryBudgets::DataStore> list1;
		list1.PushBack(Int64(7));
		list1.PushBack(Int64(11));
		list1.PushBack(Int64(25));

		List<Int64, MemoryBudgets::DataStore> list2(list1);
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, list2.GetSize());
		SEOUL_UNITTESTING_ASSERT(!list2.IsEmpty());

		auto i1 = list1.Begin();
		auto i2 = list2.Begin();
		for (; list1.End() != i1; ++i1, ++i2)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(*i1, *i2);
		}
	}

	// Copy templated
	{
		List<Int64, MemoryBudgets::Falcon> list1;
		list1.PushBack(Int64(7));
		list1.PushBack(Int64(11));
		list1.PushBack(Int64(25));

		List<Int64, MemoryBudgets::Physics> list2(list1);
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, list2.GetSize());
		SEOUL_UNITTESTING_ASSERT(!list2.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(list1.Back(), list2.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(list1.Front(), list2.Front());

		auto i1 = list1.Begin();
		auto i2 = list2.Begin();
		for (; list1.End() != i1; ++i1, ++i2)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(*i1, *i2);
		}
	}

	// Size with default value.
	{
		List<Int64, MemoryBudgets::Falcon> list(5);
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, list.GetSize());
		SEOUL_UNITTESTING_ASSERT(!list.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(Int64(), list.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(Int64(), list.Front());

		auto i1 = list.Begin();
		for (; list.End() != i1; ++i1)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(Int64(), *i1);
		}
	}

	// Size with value.
	{
		List<Int64, MemoryBudgets::Falcon> list(5, Int64(77));
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, list.GetSize());
		SEOUL_UNITTESTING_ASSERT(!list.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(Int64(77), list.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(Int64(77), list.Front());

		auto i1 = list.Begin();
		for (; list.End() != i1; ++i1)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(Int64(77), *i1);
		}
	}
}

void ListTest::TestConstructorComplex()
{
	// Default.
	{
		List<ContainerTestComplex, MemoryBudgets::DataStore> list;
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
		SEOUL_UNITTESTING_ASSERT(list.IsEmpty());
	}

	// Copy
	{
		List<ContainerTestComplex, MemoryBudgets::DataStore> list1;
		list1.PushBack(ContainerTestComplex(7));
		list1.PushBack(ContainerTestComplex(11));
		list1.PushBack(ContainerTestComplex(25));

		List<ContainerTestComplex, MemoryBudgets::DataStore> list2(list1);
		SEOUL_UNITTESTING_ASSERT_EQUAL(6, ContainerTestComplex::s_iCount);
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, list2.GetSize());
		SEOUL_UNITTESTING_ASSERT(!list2.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(list1.Back(), list2.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(list1.Front(), list2.Front());

		auto i1 = list1.Begin();
		auto i2 = list2.Begin();
		for (; list1.End() != i1; ++i1, ++i2)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(*i1, *i2);
		}
	}
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);

	// Copy templated
	{
		List<ContainerTestComplex, MemoryBudgets::Falcon> list1;
		list1.PushBack(ContainerTestComplex(7));
		list1.PushBack(ContainerTestComplex(11));
		list1.PushBack(ContainerTestComplex(25));

		List<ContainerTestComplex, MemoryBudgets::Physics> list2(list1);
		SEOUL_UNITTESTING_ASSERT_EQUAL(6, ContainerTestComplex::s_iCount);
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, list2.GetSize());
		SEOUL_UNITTESTING_ASSERT(!list2.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(list1.Back(), list2.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(list1.Front(), list2.Front());

		auto i1 = list1.Begin();
		auto i2 = list2.Begin();
		for (; list1.End() != i1; ++i1, ++i2)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(*i1, *i2);
		}
	}
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);

	// Size with default value.
	{
		List<ContainerTestComplex, MemoryBudgets::Falcon> list(5);
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, ContainerTestComplex::s_iCount);
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, list.GetSize());
		SEOUL_UNITTESTING_ASSERT(!list.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(), list.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(), list.Front());

		auto i1 = list.Begin();
		for (; list.End() != i1; ++i1)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(), *i1);
		}
	}

	// Size with value.
	{
		List<ContainerTestComplex, MemoryBudgets::Falcon> list(5, ContainerTestComplex(77));
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, ContainerTestComplex::s_iCount);
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, list.GetSize());
		SEOUL_UNITTESTING_ASSERT(!list.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(77), list.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(77), list.Front());

		auto i1 = list.Begin();
		for (; list.End() != i1; ++i1)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(77), *i1);
		}
	}
}

void ListTest::TestConstructorSimple()
{
	// Default.
	{
		List<ContainerTestSimple, MemoryBudgets::DataStore> list;
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
		SEOUL_UNITTESTING_ASSERT(list.IsEmpty());
	}

	// Copy
	{
		ContainerTestSimple simple;
		simple.m_B = 33;
		simple.m_iA = 7;
		List<ContainerTestSimple, MemoryBudgets::DataStore> list1;
		list1.PushBack(simple);
		simple.m_iA = 11;
		list1.PushBack(simple);
		simple.m_iA = 25;
		list1.PushBack(simple);

		List<ContainerTestSimple, MemoryBudgets::DataStore> list2(list1);
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, list2.GetSize());
		SEOUL_UNITTESTING_ASSERT(!list2.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(list1.Back(), list2.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(list1.Front(), list2.Front());

		auto i1 = list1.Begin();
		auto i2 = list2.Begin();
		for (; list1.End() != i1; ++i1, ++i2)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(*i1, *i2);
		}
	}

	// Copy templated
	{
		ContainerTestSimple simple;
		simple.m_B = 33;
		simple.m_iA = 7;
		List<ContainerTestSimple, MemoryBudgets::Falcon> list1;
		list1.PushBack(simple);
		simple.m_iA = 11;
		list1.PushBack(simple);
		simple.m_iA = 25;
		list1.PushBack(simple);

		List<ContainerTestSimple, MemoryBudgets::Physics> list2(list1);
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, list2.GetSize());
		SEOUL_UNITTESTING_ASSERT(!list2.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(list1.Back(), list2.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(list1.Front(), list2.Front());

		auto i1 = list1.Begin();
		auto i2 = list2.Begin();
		for (; list1.End() != i1; ++i1, ++i2)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(*i1, *i2);
		}
	}

	// Size with default value.
	{
		List<ContainerTestSimple, MemoryBudgets::Falcon> list(5);
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, list.GetSize());
		SEOUL_UNITTESTING_ASSERT(!list.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple(), list.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple(), list.Front());

		auto i1 = list.Begin();
		for (; list.End() != i1; ++i1)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple(), *i1);
		}
	}

	// Size with value.
	{
		ContainerTestSimple simple;
		simple.m_B = 33;
		simple.m_iA = 77;
		List<ContainerTestSimple, MemoryBudgets::Falcon> list(5, simple);
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, list.GetSize());
		SEOUL_UNITTESTING_ASSERT(!list.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(simple, list.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(simple, list.Front());

		auto i1 = list.Begin();
		for (; list.End() != i1; ++i1)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(simple, *i1);
		}
	}
}

void ListTest::TestEmptyBuiltin()
{
	List<Int16, MemoryBudgets::DataStore> list;
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(list.IsEmpty());

	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Begin(), list.End());
	SEOUL_UNITTESTING_ASSERT(!Contains(list.Begin(), list.End(), 5));
	SEOUL_UNITTESTING_ASSERT(!ContainsFromBack(list.Begin(), list.End(), 7));
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.End(), list.Begin());
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Begin(), list.Erase(list.Begin(), list.Begin()));

	list.Assign(0, 23);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(list.IsEmpty());

	SEOUL_UNITTESTING_ASSERT_EQUAL(list.End(), Find(list.Begin(), list.End(), 37));
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.End(), FindFromBack(list.Begin(), list.End(), 37));

	list.Resize(0);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(list.IsEmpty());

	{
		List<Int16, MemoryBudgets::DataStore> list2;
		list.Swap(list2);
	}

	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(list.IsEmpty());

	list.PushBack(53);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(!list.IsEmpty());

	SEOUL_UNITTESTING_ASSERT_EQUAL(53, *list.Begin());
	SEOUL_UNITTESTING_ASSERT_EQUAL(53, *(--list.End()));
	SEOUL_UNITTESTING_ASSERT_EQUAL(53, list.Front());

	list.PopBack();
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(list.IsEmpty());
}

void ListTest::TestEmptyComplex()
{
	List<ContainerTestComplex, MemoryBudgets::DataStore> list;
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(list.IsEmpty());

	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Begin(), list.End());
	SEOUL_UNITTESTING_ASSERT(!Contains(list.Begin(), list.End(), ContainerTestComplex(5)));
	SEOUL_UNITTESTING_ASSERT(!ContainsFromBack(list.Begin(), list.End(), ContainerTestComplex(7)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.End(), list.Begin());
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Begin(), list.Erase(list.Begin(), list.Begin()));

	list.Assign(0, ContainerTestComplex(23));
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(list.IsEmpty());

	SEOUL_UNITTESTING_ASSERT_EQUAL(list.End(), Find(list.Begin(), list.End(), ContainerTestComplex(37)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.End(), FindFromBack(list.Begin(), list.End(), ContainerTestComplex(37)));

	list.Resize(0);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(list.IsEmpty());

	{
		List<ContainerTestComplex, MemoryBudgets::DataStore> list2;
		list.Swap(list2);
	}

	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(list.IsEmpty());

	list.PushBack(ContainerTestComplex(53));
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, ContainerTestComplex::s_iCount);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(!list.IsEmpty());

	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(53), list.Back());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(53), *list.Begin());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(53), *(--list.End()));
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(53), list.Front());

	list.PopBack();
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(list.IsEmpty());
}

void ListTest::TestEmptySimple()
{
	ContainerTestSimple simple;

	List<ContainerTestSimple, MemoryBudgets::DataStore> list;
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(list.IsEmpty());

	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Begin(), list.End());
	SEOUL_UNITTESTING_ASSERT(!Contains(list.Begin(), list.End(), simple));
	SEOUL_UNITTESTING_ASSERT(!ContainsFromBack(list.Begin(), list.End(), simple));
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.End(), list.Begin());
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Begin(), list.Erase(list.Begin(), list.Begin()));

	simple.m_B = 33;
	simple.m_iA = 23;
	list.Assign(0, simple);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(list.IsEmpty());

	SEOUL_UNITTESTING_ASSERT_EQUAL(list.End(), Find(list.Begin(), list.End(), simple));
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.End(), FindFromBack(list.Begin(), list.End(), simple));

	list.Resize(0);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(list.IsEmpty());

	{
		List<ContainerTestSimple, MemoryBudgets::DataStore> list2;
		list.Swap(list2);
	}

	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(list.IsEmpty());

	simple.m_iA = 53;
	list.PushBack(simple);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(!list.IsEmpty());

	SEOUL_UNITTESTING_ASSERT_EQUAL(53, list.Back().m_iA);
	SEOUL_UNITTESTING_ASSERT_EQUAL(33, list.Back().m_B);
	SEOUL_UNITTESTING_ASSERT_EQUAL(53, list.Begin()->m_iA);
	SEOUL_UNITTESTING_ASSERT_EQUAL(33, list.Begin()->m_B);
	SEOUL_UNITTESTING_ASSERT_EQUAL(53, (--list.End())->m_iA);
	SEOUL_UNITTESTING_ASSERT_EQUAL(33, (--list.End())->m_B);
	SEOUL_UNITTESTING_ASSERT_EQUAL(53, list.Front().m_iA);
	SEOUL_UNITTESTING_ASSERT_EQUAL(33, list.Front().m_B);

	list.PopBack();
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(list.IsEmpty());
}

void ListTest::TestEqualityBuiltin()
{
	List<UInt32> listA;
	List<UInt32> listB;

	listA.PushBack(1);
	listB.PushBack(1);
	listB.PushBack(2);

	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(listA, listB);

	listA.PushBack(2);

	SEOUL_UNITTESTING_ASSERT_EQUAL(listA, listB);

	*(++listA.Begin()) = 3;

	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(listA, listB);
}

void ListTest::TestEqualityComplex()
{
	List<ContainerTestComplex> listA;
	List<ContainerTestComplex> listB;

	listA.PushBack(ContainerTestComplex(1));
	listB.PushBack(ContainerTestComplex(1));
	listB.PushBack(ContainerTestComplex(2));

	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(listA, listB);

	listA.PushBack(ContainerTestComplex(2));

	SEOUL_UNITTESTING_ASSERT_EQUAL(listA, listB);

	*(++listA.Begin()) = ContainerTestComplex(3);

	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(listA, listB);
}

void ListTest::TestEqualitySimple()
{
	List<ContainerTestSimple> listA;
	List<ContainerTestSimple> listB;

	listA.PushBack(ContainerTestSimple::Create(1));
	listB.PushBack(ContainerTestSimple::Create(1));
	listB.PushBack(ContainerTestSimple::Create(2));

	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(listA, listB);

	listA.PushBack(ContainerTestSimple::Create(2));

	SEOUL_UNITTESTING_ASSERT_EQUAL(listA, listB);

	*(++listA.Begin()) = ContainerTestSimple::Create(3);

	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(listA, listB);
}

void ListTest::TestFind()
{
	// Empty
	{
		List<ContainerTestComplex> list;
		SEOUL_UNITTESTING_ASSERT(!list.Contains(ContainerTestComplex(25)));
		SEOUL_UNITTESTING_ASSERT(!list.Contains(25));
		SEOUL_UNITTESTING_ASSERT(!list.ContainsFromBack(ContainerTestComplex(25)));
		SEOUL_UNITTESTING_ASSERT(!list.ContainsFromBack(25));
		SEOUL_UNITTESTING_ASSERT_EQUAL(list.End(), list.Find(ContainerTestComplex(25)));
		SEOUL_UNITTESTING_ASSERT_EQUAL(list.End(), list.Find(25));
		SEOUL_UNITTESTING_ASSERT_EQUAL(list.End(), list.FindFromBack(ContainerTestComplex(25)));
		SEOUL_UNITTESTING_ASSERT_EQUAL(list.End(), list.FindFromBack(25));
	}

	// Not empty.
	{
		Int const aiNumbers[] = { 10, 123, 3, 98, 128, 1498, 3, 5 };

		List<ContainerTestComplex> list;
		for (size_t i = 0; i < SEOUL_ARRAY_COUNT(aiNumbers); ++i)
		{
			list.PushBack(ContainerTestComplex(aiNumbers[i]));
		}

		SEOUL_UNITTESTING_ASSERT(!list.Contains(ContainerTestComplex(25)));
		SEOUL_UNITTESTING_ASSERT(!list.Contains(25));
		SEOUL_UNITTESTING_ASSERT(!list.ContainsFromBack(ContainerTestComplex(25)));
		SEOUL_UNITTESTING_ASSERT(!list.ContainsFromBack(25));
		SEOUL_UNITTESTING_ASSERT_EQUAL(list.End(), list.Find(ContainerTestComplex(25)));
		SEOUL_UNITTESTING_ASSERT_EQUAL(list.End(), list.Find(25));
		SEOUL_UNITTESTING_ASSERT_EQUAL(list.End(), list.FindFromBack(ContainerTestComplex(25)));
		SEOUL_UNITTESTING_ASSERT_EQUAL(list.End(), list.FindFromBack(25));

		for (size_t i = 0; i < SEOUL_ARRAY_COUNT(aiNumbers); ++i)
		{
			SEOUL_UNITTESTING_ASSERT(list.Contains(ContainerTestComplex(aiNumbers[i])));
			SEOUL_UNITTESTING_ASSERT(list.Contains(aiNumbers[i]));
			SEOUL_UNITTESTING_ASSERT(list.ContainsFromBack(ContainerTestComplex(aiNumbers[i])));
			SEOUL_UNITTESTING_ASSERT(list.ContainsFromBack(aiNumbers[i]));

			if (aiNumbers[i] == 3)
			{
				auto iterA = list.Begin();
				for (size_t j = 0; j < 2; ++j) { ++iterA; }
				auto iterB = list.Begin();
				for (size_t j = 0; j < 6; ++j) { ++iterB; }

				SEOUL_UNITTESTING_ASSERT_EQUAL(iterA, list.Find(ContainerTestComplex(aiNumbers[i])));
				SEOUL_UNITTESTING_ASSERT_EQUAL(iterA, list.Find(aiNumbers[i]));
				SEOUL_UNITTESTING_ASSERT_EQUAL(iterB, list.FindFromBack(ContainerTestComplex(aiNumbers[i])));
				SEOUL_UNITTESTING_ASSERT_EQUAL(iterB, list.FindFromBack(aiNumbers[i]));
			}
			else
			{
				auto iter = list.Begin();
				for (size_t j = 0; j < i; ++j) { ++iter; }

				SEOUL_UNITTESTING_ASSERT_EQUAL(iter, list.Find(ContainerTestComplex(aiNumbers[i])));
				SEOUL_UNITTESTING_ASSERT_EQUAL(iter, list.Find(aiNumbers[i]));
				SEOUL_UNITTESTING_ASSERT_EQUAL(iter, list.FindFromBack(ContainerTestComplex(aiNumbers[i])));
				SEOUL_UNITTESTING_ASSERT_EQUAL(iter, list.FindFromBack(aiNumbers[i]));
			}
		}
	}
}

void ListTest::TestMethods()
{
	List<Int> testList;
	for (Int i = 0; i<10; i++)
	{
		testList.PushBack(i);
	}
	SEOUL_UNITTESTING_ASSERT( testList.GetSize() == 10);

	// get head
	SEOUL_UNITTESTING_ASSERT( *(testList.Begin()) == 0);

	// erase an Iterator
	auto iter = testList.Begin();
	for (Int i = 0; i < 5; i++)
	{
		++iter;
	}
	iter = testList.Erase(iter);
	SEOUL_UNITTESTING_ASSERT( testList.GetSize() == 9);
	SEOUL_UNITTESTING_ASSERT( *iter == 6 );
	--iter;
	SEOUL_UNITTESTING_ASSERT( *iter == 4);

	{
		auto iter2 = testList.Begin();
		for (Int i=0; i<10; i++)
		{
			if ( i == 5) //skip the element we took out
			{
				i++;
			}
			SEOUL_UNITTESTING_ASSERT(*iter2==i);

			++iter2;

		}
		SEOUL_UNITTESTING_ASSERT( testList.End() == iter2 );
	}
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, testList.Remove(8));
		SEOUL_UNITTESTING_ASSERT( testList.GetSize() == 8);
		auto iter2 = testList.Begin();
		for (Int i=0; i<10; i++)
		{
			if ( i == 5 || i==8) //skip the element we took out
			{
				i++;
			}
			SEOUL_UNITTESTING_ASSERT(*iter2 == i);

			++iter2;

		}
		SEOUL_UNITTESTING_ASSERT( testList.End() == iter2 );
	}
	//remove head
	{
		iter = testList.Erase(testList.Begin() );
		SEOUL_UNITTESTING_ASSERT( iter == testList.Begin() );
		SEOUL_UNITTESTING_ASSERT( testList.GetSize() == 7);
		auto iter2 = testList.Begin();
		for (Int i=0; i<10; i++)
		{
			if ( i == 5 || i==8 || i==0) //skip the element we took out
			{
				i++;
			}
			SEOUL_UNITTESTING_ASSERT(*iter2 == i);

			++iter2;

		}
		SEOUL_UNITTESTING_ASSERT( testList.End() == iter2);
	}

	//remove from a list with only two elements
	List<Int> smallList;
	smallList.PushBack(0);
	smallList.PushBack(1);
	SEOUL_UNITTESTING_ASSERT( smallList.GetSize() == 2);

	smallList.Erase( smallList.Begin() );
	SEOUL_UNITTESTING_ASSERT( smallList.GetSize() == 1);
	SEOUL_UNITTESTING_ASSERT( *(smallList.Begin()) == 1);

	smallList.Erase( smallList.Begin() );
	SEOUL_UNITTESTING_ASSERT( smallList.GetSize() == 0);
	SEOUL_UNITTESTING_ASSERT( smallList.Begin() == smallList.End() );

	//test copy constructor
	{
		List<Int> otherList(testList);
		SEOUL_UNITTESTING_ASSERT( otherList.GetSize() == 7);

		otherList.PushFront(29);
		SEOUL_UNITTESTING_ASSERT( *(otherList.Begin()) != *(testList.Begin()) );

		auto curIter = testList.Begin();
		auto otherIter = otherList.Begin();
		++otherIter;
		for (UInt i = 0; i<testList.GetSize(); i++)
		{
			SEOUL_UNITTESTING_ASSERT( *(curIter) == *(otherIter) );

			++curIter;
			++otherIter;
		}
	}

	//test assignment constructor
	{
		List<Int> otherList;
		otherList.PushFront(1);
		otherList = testList;
		SEOUL_UNITTESTING_ASSERT( otherList.GetSize() == 7);

		otherList.PushFront(29);
		SEOUL_UNITTESTING_ASSERT( *(otherList.Begin()) != *(testList.Begin()) );

		auto curIter = testList.Begin();
		auto otherIter = otherList.Begin();
		++otherIter;
		for (UInt i = 0; i<testList.GetSize(); i++)
		{
			SEOUL_UNITTESTING_ASSERT( *curIter == *otherIter );

			++curIter;
			++otherIter;
		}
	}

	//test fill constructor
	{
		List<Int> onesList(10, 1);
		auto onesIter = onesList.Begin();

		for (UInt i =0; i<10; i++)
		{
			SEOUL_UNITTESTING_ASSERT( *onesIter == 1);
			++onesIter;
		}
		SEOUL_UNITTESTING_ASSERT( onesIter == onesList.End() );

		List<Int> twosList(20, 2);
		auto twosIter = twosList.Begin();

		for (UInt i =0; i<20; i++)
		{
			SEOUL_UNITTESTING_ASSERT( *twosIter == 2);
			++twosIter;
		}
		SEOUL_UNITTESTING_ASSERT( twosIter == twosList.End() );
	}
}

void ListTest::TestInsertBuiltin()
{
	List<Int>::Iterator iter;

	List<Int> list;
	list.Insert(list.Begin(), 0);

	SEOUL_UNITTESTING_ASSERT_EQUAL(Int(0), *list.Begin());

	iter = list.Begin();
	++iter;
	list.Insert(iter, Int(3));

	SEOUL_UNITTESTING_ASSERT_EQUAL(Int(0), list.Front());
	SEOUL_UNITTESTING_ASSERT_EQUAL(Int(3), list.Back());

	iter = list.Begin();
	++iter;
	iter = list.Insert(iter, 1);
	SEOUL_UNITTESTING_ASSERT_EQUAL(Int(0), list.Front());
	SEOUL_UNITTESTING_ASSERT_EQUAL(Int(1), *iter);
	SEOUL_UNITTESTING_ASSERT_EQUAL(Int(3), list.Back());

	--iter;
	SEOUL_UNITTESTING_ASSERT_EQUAL(Int(0), *iter);
	++iter;
	++iter;
	SEOUL_UNITTESTING_ASSERT_EQUAL(Int(3), *iter);

	iter = list.Insert(iter, 2);
	SEOUL_UNITTESTING_ASSERT_EQUAL(Int(0), list.Front());
	SEOUL_UNITTESTING_ASSERT_EQUAL(Int(2), *iter);
	SEOUL_UNITTESTING_ASSERT_EQUAL(Int(3), list.Back());

	--iter;
	SEOUL_UNITTESTING_ASSERT_EQUAL(Int(1), *iter);
	iter--;
	SEOUL_UNITTESTING_ASSERT_EQUAL(Int(0), *iter);
	++iter;
	SEOUL_UNITTESTING_ASSERT_EQUAL(Int(1), *iter);
	iter++;
	SEOUL_UNITTESTING_ASSERT_EQUAL(Int(2), *iter);
	++iter;
	SEOUL_UNITTESTING_ASSERT_EQUAL(Int(3), *iter);
}

void ListTest::TestInsertComplex()
{
	{
		List<ContainerTestComplex>::Iterator iter;

		SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);

		List<ContainerTestComplex> list;
		list.Insert(list.Begin(), ContainerTestComplex(0));

		SEOUL_UNITTESTING_ASSERT_EQUAL(1, ContainerTestComplex::s_iCount);

		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(0), *list.Begin());

		iter = list.Begin();
		++iter;
		list.Insert(iter, ContainerTestComplex(3));

		SEOUL_UNITTESTING_ASSERT_EQUAL(2, ContainerTestComplex::s_iCount);

		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(0), list.Front());
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(3), list.Back());

		iter = list.Begin();
		++iter;
		iter = list.Insert(iter, ContainerTestComplex(1));
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, ContainerTestComplex::s_iCount);

		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(0), list.Front());
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(1), *iter);
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(3), list.Back());

		--iter;
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(0), *iter);
		++iter;
		++iter;
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(3), *iter);

		iter = list.Insert(iter, ContainerTestComplex(2));
		SEOUL_UNITTESTING_ASSERT_EQUAL(4, ContainerTestComplex::s_iCount);

		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(0), list.Front());
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(2), *iter);
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(3), list.Back());

		--iter;
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(1), *iter);
		iter--;
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(0), *iter);
		++iter;
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(1), *iter);
		iter++;
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(2), *iter);
		++iter;
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(3), *iter);
	}

	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
}

void ListTest::TestInsertSimple()
{
	List<ContainerTestSimple>::Iterator iter;

	List<ContainerTestSimple> list;
	list.Insert(list.Begin(), ContainerTestSimple::Create(0));

	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(0), *list.Begin());

	iter = list.Begin();
	++iter;
	list.Insert(iter, ContainerTestSimple::Create(3));

	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(0), list.Front());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(3), list.Back());

	iter = list.Begin();
	++iter;
	iter = list.Insert(iter, ContainerTestSimple::Create(1));
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(0), list.Front());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(1), *iter);
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(3), list.Back());

	--iter;
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(0), *iter);
	++iter;
	++iter;
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(3), *iter);

	iter = list.Insert(iter, ContainerTestSimple::Create(2));
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(0), list.Front());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(2), *iter);
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(3), list.Back());

	--iter;
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(1), *iter);
	iter--;
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(0), *iter);
	++iter;
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(1), *iter);
	iter++;
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(2), *iter);
	++iter;
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(3), *iter);
}

void ListTest::TestEraseBuiltin()
{
	// Built-in type.
	{
		List<Int> testList;

		for (Int i = 0; i < 6; i++)
		{
			testList.PushBack(i + 10);
		}

		auto i = testList.Begin();
		++i;
		i++;
		++i;
		testList.Erase(i);

		SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)5, testList.GetSize());

		i = testList.Begin();
		SEOUL_UNITTESTING_ASSERT_EQUAL(Int(10), *i); ++i;
		SEOUL_UNITTESTING_ASSERT_EQUAL(Int(11), *i); ++i;
		SEOUL_UNITTESTING_ASSERT_EQUAL(Int(12), *i); ++i;
		SEOUL_UNITTESTING_ASSERT_EQUAL(Int(14), *i); ++i;
		SEOUL_UNITTESTING_ASSERT_EQUAL(Int(15), *i); ++i;

		testList.Erase(testList.Begin());

		SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)4, testList.GetSize());

		i = testList.Begin();
		SEOUL_UNITTESTING_ASSERT_EQUAL(Int(11), *i); ++i;
		SEOUL_UNITTESTING_ASSERT_EQUAL(Int(12), *i); ++i;
		SEOUL_UNITTESTING_ASSERT_EQUAL(Int(14), *i); ++i;
		SEOUL_UNITTESTING_ASSERT_EQUAL(Int(15), *i); ++i;

		testList.Erase((++(++testList.Begin())), (++(++testList.Begin())));  // should not do anything
		SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)4, testList.GetSize());

		testList.Erase((++(++testList.Begin())), (++(++(++testList.Begin()))));  // should erase one element
		SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)3, testList.GetSize());

		i = testList.Begin();
		SEOUL_UNITTESTING_ASSERT_EQUAL(Int(11), *i); ++i;
		SEOUL_UNITTESTING_ASSERT_EQUAL(Int(12), *i); ++i;
		SEOUL_UNITTESTING_ASSERT_EQUAL(Int(15), *i); ++i;

		testList.Erase((++testList.Begin()), (++(++(++testList.Begin()))));
		SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)1, testList.GetSize());

		i = testList.Begin();
		SEOUL_UNITTESTING_ASSERT_EQUAL(Int(11), *i); ++i;

		testList.Erase(testList.Begin(), (++testList.Begin()));
		SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)0, testList.GetSize());
	}
}

void ListTest::TestEraseSimple()
{
	// Simple type.
	{
		List<ContainerTestSimple> testList;

		for (Int i = 0; i < 6; i++)
		{
			testList.PushBack(ContainerTestSimple::Create(i + 10));
		}

		auto i = testList.Begin();
		++i;
		i++;
		++i;
		testList.Erase(i);

		SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)5, testList.GetSize());

		i = testList.Begin();
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(10), *i); ++i;
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(11), *i); ++i;
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(12), *i); ++i;
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(14), *i); ++i;
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(15), *i); ++i;

		testList.Erase(testList.Begin());

		SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)4, testList.GetSize());

		i = testList.Begin();
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(11), *i); ++i;
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(12), *i); ++i;
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(14), *i); ++i;
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(15), *i); ++i;

		testList.Erase((++(++testList.Begin())), (++(++testList.Begin())));  // should not do anything
		SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)4, testList.GetSize());

		testList.Erase((++(++testList.Begin())), (++(++(++testList.Begin()))));  // should erase one element
		SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)3, testList.GetSize());

		i = testList.Begin();
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(11), *i); ++i;
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(12), *i); ++i;
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(15), *i); ++i;

		testList.Erase((++testList.Begin()), (++(++(++testList.Begin()))));
		SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)1, testList.GetSize());

		i = testList.Begin();
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(11), *i); ++i;

		testList.Erase(testList.Begin(), (++testList.Begin()));
		SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)0, testList.GetSize());
	}
}

void ListTest::TestEraseComplex()
{
	// Complex type.
	{
		List<ContainerTestComplex> testList;

		SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);

		for (Int i = 0; i < 6; i++)
		{
			testList.PushBack(ContainerTestComplex(i + 10));
		}

		SEOUL_UNITTESTING_ASSERT_EQUAL(6, ContainerTestComplex::s_iCount);

		auto i = testList.Begin();
		++i;
		i++;
		++i;
		testList.Erase(i);

		SEOUL_UNITTESTING_ASSERT_EQUAL(5, ContainerTestComplex::s_iCount);

		SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)5, testList.GetSize());

		i = testList.Begin();
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(10), *i); ++i;
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(11), *i); ++i;
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(12), *i); ++i;
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(14), *i); ++i;
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(15), *i); ++i;

		testList.Erase(testList.Begin());

		SEOUL_UNITTESTING_ASSERT_EQUAL(4, ContainerTestComplex::s_iCount);

		SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)4, testList.GetSize());

		i = testList.Begin();
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(11), *i); ++i;
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(12), *i); ++i;
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(14), *i); ++i;
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(15), *i); ++i;

		testList.Erase((++(++testList.Begin())), (++(++testList.Begin())));  // should not do anything
		SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)4, testList.GetSize());

		SEOUL_UNITTESTING_ASSERT_EQUAL(4, ContainerTestComplex::s_iCount);

		testList.Erase((++(++testList.Begin())), (++(++(++testList.Begin()))));  // should erase one element
		SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)3, testList.GetSize());

		SEOUL_UNITTESTING_ASSERT_EQUAL(3, ContainerTestComplex::s_iCount);

		i = testList.Begin();
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(11), *i); ++i;
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(12), *i); ++i;
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(15), *i); ++i;

		testList.Erase((++testList.Begin()), (++(++(++testList.Begin()))));
		SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)1, testList.GetSize());

		SEOUL_UNITTESTING_ASSERT_EQUAL(1, ContainerTestComplex::s_iCount);

		i = testList.Begin();
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(11), *i); ++i;

		testList.Erase(testList.Begin(), (++testList.Begin()));
		SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)0, testList.GetSize());

		SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
	}

	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
}

void ListTest::TestIterators()
{
	List<Int> testList;

	for (Int i = 0; i < 6; i++)
	{
		testList.PushBack(i + 10);
	}

	// Test value reads through iterator
	auto iter = testList.Begin();
	Int i = 0;
	while (iter != testList.End())
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(i + 10, *iter);
		++iter;
		++i;
	}

	// Test value writes through iterator
	iter = testList.Begin();
	i = 0;
	while (iter != testList.End())
	{
		*iter = 3 * (i + 1);
		SEOUL_UNITTESTING_ASSERT_EQUAL(iter, Find(testList.Begin(), testList.End(), 3 * (i + 1)));
		++iter;
		++i;
	}
}

void ListTest::TestEraseReturnBuiltin()
{
	List<Int> testList;

	for(Int i = 0; i < 6; i++)
	{
		testList.PushBack(i + 10);
	}

	SEOUL_UNITTESTING_ASSERT_EQUAL(6, testList.GetSize());

	auto i = testList.Erase(++(++testList.Begin()));
	SEOUL_UNITTESTING_ASSERT_EQUAL(13, *i);
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, testList.GetSize());

	i = testList.Erase(++(++(++(++testList.Begin()))));
	SEOUL_UNITTESTING_ASSERT_EQUAL(testList.End(), i);
	SEOUL_UNITTESTING_ASSERT_EQUAL(4, testList.GetSize());

	i = testList.Erase(testList.Begin());
	SEOUL_UNITTESTING_ASSERT_EQUAL(11, *i);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, testList.GetSize());

	i = testList.Erase(++testList.Begin());
	SEOUL_UNITTESTING_ASSERT_EQUAL(14, *i);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, testList.GetSize());

	i = testList.Erase(testList.Begin());
	SEOUL_UNITTESTING_ASSERT_EQUAL(14, *i);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, testList.GetSize());

	i = testList.Erase(testList.Begin());
	SEOUL_UNITTESTING_ASSERT_EQUAL(testList.End(), i);
	SEOUL_UNITTESTING_ASSERT(testList.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, testList.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(testList.Begin(), testList.End());
}

void ListTest::TestEraseReturnComplex()
{
	{
		List<ContainerTestComplex> testList;

		for(Int i = 0; i < 6; i++)
		{
			testList.PushBack(ContainerTestComplex(i + 10));
		}

		SEOUL_UNITTESTING_ASSERT_EQUAL(6, testList.GetSize());

		auto i = testList.Erase(++(++testList.Begin()));
		SEOUL_UNITTESTING_ASSERT_EQUAL(13, i->m_iVariableValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, testList.GetSize());

		i = testList.Erase(++(++(++(++testList.Begin()))));
		SEOUL_UNITTESTING_ASSERT_EQUAL(testList.End(), i);
		SEOUL_UNITTESTING_ASSERT_EQUAL(4, testList.GetSize());

		i = testList.Erase(testList.Begin());
		SEOUL_UNITTESTING_ASSERT_EQUAL(11, i->m_iVariableValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, testList.GetSize());

		i = testList.Erase(++testList.Begin());
		SEOUL_UNITTESTING_ASSERT_EQUAL(14, i->m_iVariableValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL(2, testList.GetSize());

		i = testList.Erase(testList.Begin());
		SEOUL_UNITTESTING_ASSERT_EQUAL(14, i->m_iVariableValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, testList.GetSize());

		i = testList.Erase(testList.Begin());
		SEOUL_UNITTESTING_ASSERT_EQUAL(testList.End(), i);
		SEOUL_UNITTESTING_ASSERT(testList.IsEmpty());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, testList.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(testList.Begin(), testList.End());
	}
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
}

void ListTest::TestEraseReturnSimple()
{
	List<ContainerTestSimple> testList;

	for(Int i = 0; i < 6; i++)
	{
		testList.PushBack(ContainerTestSimple::Create(i + 10));
	}

	SEOUL_UNITTESTING_ASSERT_EQUAL(6, testList.GetSize());

	auto i = testList.Erase(++(++testList.Begin()));
	SEOUL_UNITTESTING_ASSERT_EQUAL(13, i->m_iA);
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, testList.GetSize());

	i = testList.Erase(++(++(++(++testList.Begin()))));
	SEOUL_UNITTESTING_ASSERT_EQUAL(testList.End(), i);
	SEOUL_UNITTESTING_ASSERT_EQUAL(4, testList.GetSize());

	i = testList.Erase(testList.Begin());
	SEOUL_UNITTESTING_ASSERT_EQUAL(11, i->m_iA);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, testList.GetSize());

	i = testList.Erase(++testList.Begin());
	SEOUL_UNITTESTING_ASSERT_EQUAL(14, i->m_iA);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, testList.GetSize());

	i = testList.Erase(testList.Begin());
	SEOUL_UNITTESTING_ASSERT_EQUAL(14, i->m_iA);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, testList.GetSize());

	i = testList.Erase(testList.Begin());
	SEOUL_UNITTESTING_ASSERT_EQUAL(testList.End(), i);
	SEOUL_UNITTESTING_ASSERT(testList.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, testList.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(testList.Begin(), testList.End());
}

void ListTest::TestEraseRangeReturnBuiltin()
{
	List<Int> testList;

	for(Int i = 0; i < 6; i++)
	{
		testList.PushBack(i + 10);
	}

	SEOUL_UNITTESTING_ASSERT_EQUAL(6, testList.GetSize());

	auto i = testList.Erase(++testList.Begin(), ++(++(++testList.Begin())));
	SEOUL_UNITTESTING_ASSERT_EQUAL(13, *i);
	SEOUL_UNITTESTING_ASSERT_EQUAL(4, testList.GetSize());

	i = testList.Erase(testList.Begin(), ++(++testList.Begin()));
	SEOUL_UNITTESTING_ASSERT_EQUAL(14, *i);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, testList.GetSize());

	i = testList.Erase(testList.Begin(), ++testList.Begin());
	SEOUL_UNITTESTING_ASSERT_EQUAL(15, *i);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, testList.GetSize());

	i = testList.Erase(testList.Begin(), ++testList.Begin());
	SEOUL_UNITTESTING_ASSERT_EQUAL(testList.End(), i);
	SEOUL_UNITTESTING_ASSERT(testList.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, testList.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(testList.Begin(), testList.End());
}

void ListTest::TestEraseRangeReturnComplex()
{
	{
		List<ContainerTestComplex> testList;

		for(Int i = 0; i < 6; i++)
		{
			testList.PushBack(ContainerTestComplex(i + 10));
		}

		SEOUL_UNITTESTING_ASSERT_EQUAL(6, testList.GetSize());

		auto i = testList.Erase(++testList.Begin(), ++(++(++testList.Begin())));
		SEOUL_UNITTESTING_ASSERT_EQUAL(13, i->m_iVariableValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL(4, testList.GetSize());

		i = testList.Erase(testList.Begin(), ++(++testList.Begin()));
		SEOUL_UNITTESTING_ASSERT_EQUAL(14, i->m_iVariableValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL(2, testList.GetSize());

		i = testList.Erase(testList.Begin(), ++testList.Begin());
		SEOUL_UNITTESTING_ASSERT_EQUAL(15, i->m_iVariableValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, testList.GetSize());

		i = testList.Erase(testList.Begin(), ++testList.Begin());
		SEOUL_UNITTESTING_ASSERT_EQUAL(testList.End(), i);
		SEOUL_UNITTESTING_ASSERT(testList.IsEmpty());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, testList.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(testList.Begin(), testList.End());
	}
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
}

void ListTest::TestEraseRangeReturnSimple()
{
	List<ContainerTestSimple> testList;

	for(Int i = 0; i < 6; i++)
	{
		testList.PushBack(ContainerTestSimple::Create(i + 10));
	}

	SEOUL_UNITTESTING_ASSERT_EQUAL(6, testList.GetSize());

	auto i = testList.Erase(++testList.Begin(), ++(++(++testList.Begin())));
	SEOUL_UNITTESTING_ASSERT_EQUAL(13, i->m_iA);
	SEOUL_UNITTESTING_ASSERT_EQUAL(4, testList.GetSize());

	i = testList.Erase(testList.Begin(), ++(++testList.Begin()));
	SEOUL_UNITTESTING_ASSERT_EQUAL(14, i->m_iA);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, testList.GetSize());

	i = testList.Erase(testList.Begin(), ++testList.Begin());
	SEOUL_UNITTESTING_ASSERT_EQUAL(15, i->m_iA);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, testList.GetSize());

	i = testList.Erase(testList.Begin(), ++testList.Begin());
	SEOUL_UNITTESTING_ASSERT_EQUAL(testList.End(), i);
	SEOUL_UNITTESTING_ASSERT(testList.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, testList.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(testList.Begin(), testList.End());
}

/** Tests for ranged-based for loops (for a : b). */
void ListTest::TestRangedFor()
{
	List<Int> testList;
	testList.PushBack(3);
	testList.PushBack(7);
	testList.PushBack(2);

	Int i = 0;
	for (auto list : testList)
	{
		auto iExpected = testList.Begin();
		for (Int j = 0; j < i; ++j) { ++iExpected; }
		SEOUL_UNITTESTING_ASSERT_EQUAL(*iExpected, list);
		++i;
	}

	testList.Insert(testList.Begin(), 35);
	i = 0;
	for (auto list : testList)
	{
		auto iExpected = testList.Begin();
		for (Int j = 0; j < i; ++j) { ++iExpected; }
		SEOUL_UNITTESTING_ASSERT_EQUAL(*iExpected, list);
		++i;
	}

	testList.PushBack(77);
	i = 0;
	for (auto list : testList)
	{
		auto iExpected = testList.Begin();
		for (Int j = 0; j < i; ++j) { ++iExpected; }
		SEOUL_UNITTESTING_ASSERT_EQUAL(*iExpected, list);
		++i;
	}
}

static Bool UInt64AlwaysTrueFunctor(UInt64) { return true; }
static Bool UInt64LessThan(UInt64 a, UInt64 b) { return (a < b); }
void ListTest::TestRemoveBuiltin()
{
	List<UInt64, MemoryBudgets::TBDContainer> list;
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Begin(), list.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(list.IsEmpty());

	// Nop
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.Remove(UInt64(25)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Begin(), list.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(list.IsEmpty());

	// Nop
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.RemoveIf(UInt64AlwaysTrueFunctor));
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Begin(), list.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(list.IsEmpty());

	// Populate the list.
	list.PushBack(UInt64(25));
	list.PushFront(UInt64(13));
	list.PopBack();
	list.PushBack(UInt64(23));
	list.PopFront();
	list.PushFront(UInt64(15));

	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(list.Begin(), list.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(!list.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Front(), UInt64(15));
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Back(), UInt64(23));

	SEOUL_UNITTESTING_ASSERT_EQUAL(1, list.Remove(UInt64(23)));
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(list.Begin(), list.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(!list.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Front(), UInt64(15));
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Back(), UInt64(15));

	SEOUL_UNITTESTING_ASSERT_EQUAL(1, list.Remove(UInt64(15)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Begin(), list.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(list.IsEmpty());

	for (Int i = 0; i < 5; ++i)
	{
		list.PushBack(5);
		list.PushFront(5);
		list.PopFront();
		list.PopBack();
		list.PushBack(5);
	}
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(list.Begin(), list.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(!list.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Front(), UInt64(5));
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Back(), UInt64(5));

	SEOUL_UNITTESTING_ASSERT_EQUAL(5, list.RemoveIf(UInt64AlwaysTrueFunctor));
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Begin(), list.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(list.IsEmpty());

	for (Int i = 0; i < 5; ++i)
	{
		list.PushBack(i);
		list.PushFront(i);
	}
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(list.Begin(), list.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(10, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(!list.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Front(), UInt64(4));
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Back(), UInt64(4));

	SEOUL_UNITTESTING_ASSERT_EQUAL(2, list.Remove(UInt64(0)));
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(list.Begin(), list.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(8, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(!list.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Front(), UInt64(4));
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Back(), UInt64(4));

	list.Sort();
	auto iter = list.Begin();
	for (UInt64 i = 0; i < 4; ++i)
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(i + 1, *iter);
		++iter;
		SEOUL_UNITTESTING_ASSERT_EQUAL(i + 1, *iter);
		iter++;
	}

	list.Reverse();
	iter = list.End();
	for (UInt64 i = 0; i < 4; ++i)
	{
		iter--;
		SEOUL_UNITTESTING_ASSERT_EQUAL(i + 1, *iter);
		--iter;
		SEOUL_UNITTESTING_ASSERT_EQUAL(i + 1, *iter);
	}

	list.Sort(UInt64LessThan);
	iter = list.Begin();
	for (UInt64 i = 0; i < 4; ++i)
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(i + 1, *iter);
		++iter;
		SEOUL_UNITTESTING_ASSERT_EQUAL(i + 1, *iter);
		iter++;
	}
}

static Bool ContainerTestComplexAlwaysTrueFunctor(ContainerTestComplex) { return true; }
static Bool ContainerTestComplexLessThan(ContainerTestComplex a, ContainerTestComplex b) { return (a.m_iVariableValue < b.m_iVariableValue); }
void ListTest::TestRemoveComplex()
{
	List<ContainerTestComplex, MemoryBudgets::TBDContainer> list;
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Begin(), list.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(list.IsEmpty());

	// Nop
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.Remove(ContainerTestComplex(25)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Begin(), list.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(list.IsEmpty());

	// Nop
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.RemoveIf(ContainerTestComplexAlwaysTrueFunctor));
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Begin(), list.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(list.IsEmpty());

	// Populate the list.
	list.PushBack(ContainerTestComplex(25));
	list.PushFront(ContainerTestComplex(13));
	list.PopBack();
	list.PushBack(ContainerTestComplex(23));
	list.PopFront();
	list.PushFront(ContainerTestComplex(15));

	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(list.Begin(), list.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(!list.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Front(), ContainerTestComplex(15));
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Back(), ContainerTestComplex(23));

	SEOUL_UNITTESTING_ASSERT_EQUAL(1, list.Remove(ContainerTestComplex(23)));
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(list.Begin(), list.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(!list.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Front(), ContainerTestComplex(15));
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Back(), ContainerTestComplex(15));

	SEOUL_UNITTESTING_ASSERT_EQUAL(1, list.Remove(ContainerTestComplex(15)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Begin(), list.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(list.IsEmpty());

	for (Int i = 0; i < 5; ++i)
	{
		list.PushBack(ContainerTestComplex(5));
		list.PushFront(ContainerTestComplex(5));
		list.PopFront();
		list.PopBack();
		list.PushBack(ContainerTestComplex(5));
	}
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(list.Begin(), list.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(!list.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Front(), ContainerTestComplex(5));
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Back(), ContainerTestComplex(5));

	SEOUL_UNITTESTING_ASSERT_EQUAL(5, list.RemoveIf(ContainerTestComplexAlwaysTrueFunctor));
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Begin(), list.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(list.IsEmpty());

	for (Int i = 0; i < 5; ++i)
	{
		list.PushBack(ContainerTestComplex(i));
		list.PushFront(ContainerTestComplex(i));
	}
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(list.Begin(), list.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(10, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(!list.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Front(), ContainerTestComplex(4));
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Back(), ContainerTestComplex(4));

	SEOUL_UNITTESTING_ASSERT_EQUAL(2, list.Remove(ContainerTestComplex(0)));
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(list.Begin(), list.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(8, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(!list.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Front(), ContainerTestComplex(4));
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Back(), ContainerTestComplex(4));

	list.Sort();
	auto iter = list.Begin();
	for (Int i = 0; i < 4; ++i)
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(i + 1), *iter);
		++iter;
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(i + 1), *iter);
		iter++;
	}

	list.Reverse();
	iter = list.End();
	for (Int i = 0; i < 4; ++i)
	{
		iter--;
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(i + 1), *iter);
		--iter;
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(i + 1), *iter);
	}

	list.Sort(ContainerTestComplexLessThan);
	iter = list.Begin();
	for (Int i = 0; i < 4; ++i)
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(i + 1), *iter);
		++iter;
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(i + 1), *iter);
		iter++;
	}
}

void ListTest::TestRemoveComplexCoerce()
{
	List<ContainerTestComplex, MemoryBudgets::TBDContainer> list;
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Begin(), list.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(list.IsEmpty());

	// Nop
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.Remove(25));
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Begin(), list.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(list.IsEmpty());

	// Nop
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.RemoveIf(ContainerTestComplexAlwaysTrueFunctor));
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Begin(), list.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(list.IsEmpty());

	// Populate the list.
	list.PushBack(ContainerTestComplex(25));
	list.Insert(list.Begin(), ContainerTestComplex(13));
	list.PopBack();
	list.PushBack(ContainerTestComplex(23));
	list.Erase(list.Begin());
	list.Insert(list.Begin(), ContainerTestComplex(15));

	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(list.Begin(), list.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(!list.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Front(), ContainerTestComplex(15));
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Back(), ContainerTestComplex(23));

	SEOUL_UNITTESTING_ASSERT_EQUAL(1, list.Remove(23));
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(list.Begin(), list.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(!list.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Front(), ContainerTestComplex(15));
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Back(), ContainerTestComplex(15));

	SEOUL_UNITTESTING_ASSERT_EQUAL(1, list.Remove(15));
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Begin(), list.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(list.IsEmpty());

	for (Int i = 0; i < 5; ++i)
	{
		list.PushBack(ContainerTestComplex(5));
		list.Insert(list.Begin(), ContainerTestComplex(5));
		list.Erase(list.Begin());
		list.PopBack();
		list.PushBack(ContainerTestComplex(5));
	}
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(list.Begin(), list.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(!list.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Front(), ContainerTestComplex(5));
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Back(), ContainerTestComplex(5));

	SEOUL_UNITTESTING_ASSERT_EQUAL(5, list.RemoveIf(ContainerTestComplexAlwaysTrueFunctor));
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Begin(), list.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(list.IsEmpty());

	for (Int i = 0; i < 5; ++i)
	{
		list.PushBack(ContainerTestComplex(i));
		list.Insert(list.Begin(), ContainerTestComplex(i));
	}
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(list.Begin(), list.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(10, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(!list.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Front(), ContainerTestComplex(4));
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Back(), ContainerTestComplex(4));

	SEOUL_UNITTESTING_ASSERT_EQUAL(2, list.Remove(0));
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(list.Begin(), list.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(8, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(!list.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Front(), ContainerTestComplex(4));
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Back(), ContainerTestComplex(4));

	list.Sort();
	auto iter = list.Begin();
	for (Int i = 0; i < 4; ++i)
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(i + 1), *iter);
		++iter;
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(i + 1), *iter);
		iter++;
	}

	Reverse(list.Begin(), list.End());
	iter = list.End();
	for (Int i = 0; i < 4; ++i)
	{
		iter--;
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(i + 1), *iter);
		--iter;
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(i + 1), *iter);
	}

	list.Sort(ContainerTestComplexLessThan);
	iter = list.Begin();
	for (Int i = 0; i < 4; ++i)
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(i + 1), *iter);
		++iter;
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(i + 1), *iter);
		iter++;
	}
}

static Bool ContainerTestSimpleAlwaysTrueFunctor(ContainerTestSimple) { return true; }
static Bool ContainerTestSimpleLessThan(ContainerTestSimple a, ContainerTestSimple b) { return (a.m_iA < b.m_iA); }
void ListTest::TestRemoveSimple()
{
	List<ContainerTestSimple, MemoryBudgets::TBDContainer> list;
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Begin(), list.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(list.IsEmpty());

	// Nop
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.Remove(ContainerTestSimple::Create(25)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Begin(), list.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(list.IsEmpty());

	// Nop
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.RemoveIf(ContainerTestSimpleAlwaysTrueFunctor));
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Begin(), list.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(list.IsEmpty());

	// Populate the list.
	list.PushBack(ContainerTestSimple::Create(25));
	list.PushFront(ContainerTestSimple::Create(13));
	list.PopBack();
	list.PushBack(ContainerTestSimple::Create(23));
	list.PopFront();
	list.PushFront(ContainerTestSimple::Create(15));

	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(list.Begin(), list.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(!list.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Front(), ContainerTestSimple::Create(15));
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Back(), ContainerTestSimple::Create(23));

	SEOUL_UNITTESTING_ASSERT_EQUAL(1, list.Remove(ContainerTestSimple::Create(23)));
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(list.Begin(), list.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(!list.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Front(), ContainerTestSimple::Create(15));
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Back(), ContainerTestSimple::Create(15));

	SEOUL_UNITTESTING_ASSERT_EQUAL(1, list.Remove(ContainerTestSimple::Create(15)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Begin(), list.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(list.IsEmpty());

	for (Int i = 0; i < 5; ++i)
	{
		list.PushBack(ContainerTestSimple::Create(5));
		list.PushFront(ContainerTestSimple::Create(5));
		list.PopFront();
		list.PopBack();
		list.PushBack(ContainerTestSimple::Create(5));
	}
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(list.Begin(), list.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(!list.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Front(), ContainerTestSimple::Create(5));
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Back(), ContainerTestSimple::Create(5));

	SEOUL_UNITTESTING_ASSERT_EQUAL(5, list.RemoveIf(ContainerTestSimpleAlwaysTrueFunctor));
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Begin(), list.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(list.IsEmpty());

	for (Int i = 0; i < 5; ++i)
	{
		list.PushBack(ContainerTestSimple::Create(i));
		list.PushFront(ContainerTestSimple::Create(i));
	}
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(list.Begin(), list.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(10, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(!list.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Front(), ContainerTestSimple::Create(4));
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Back(), ContainerTestSimple::Create(4));

	SEOUL_UNITTESTING_ASSERT_EQUAL(2, list.Remove(ContainerTestSimple::Create(0)));
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(list.Begin(), list.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(8, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(!list.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Front(), ContainerTestSimple::Create(4));
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Back(), ContainerTestSimple::Create(4));

	list.Sort();
	auto iter = list.Begin();
	for (Int i = 0; i < 4; ++i)
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(i + 1), *iter);
		++iter;
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(i + 1), *iter);
		iter++;
	}

	list.Reverse();
	iter = list.End();
	for (Int i = 0; i < 4; ++i)
	{
		iter--;
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(i + 1), *iter);
		--iter;
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(i + 1), *iter);
	}

	list.Sort(ContainerTestSimpleLessThan);
	iter = list.Begin();
	for (Int i = 0; i < 4; ++i)
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(i + 1), *iter);
		++iter;
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(i + 1), *iter);
		iter++;
	}
}

void ListTest::TestReverseIterators()
{
	List<Int> testList;

	for (Int i = 0; i < 6; i++)
	{
		testList.PushBack(i + 10);
	}

	// Test value reads through iterator
	auto iter = testList.RBegin();
	Int i = 5;
	while (iter != testList.REnd())
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(i + 10, *iter);
		++iter;
		--i;
	}

	// Test value writes through iterator
	iter = testList.RBegin();
	i = 5;
	while (iter != testList.REnd())
	{
		*iter = 3 * (i + 1);
		auto fiter = testList.Begin();
		for (Int32 j = 0; j < i; ++j)
		{
			++fiter;
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL(*fiter, *iter);
		++iter;
		--i;
	}
}

// Regression tests for the case where Remove() is called on the list
// itself, which could result in erroneous removals (e.g. list of 2 elements,
// passed with (*(list.Begin())), this would remove all elements from the list).
void ListTest::TestRemoveRegressionBuiltin()
{
	List<Int> list;
	list.PushBack(1);
	list.PushBack(2);

	SEOUL_UNITTESTING_ASSERT_EQUAL(1, list.Remove((*(list.Begin()))));
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, list.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, (*(list.Begin())));
}

void ListTest::TestRemoveRegressionComplex()
{
	List<ContainerTestComplex> list;
	list.PushBack(ContainerTestComplex(1));
	list.PushBack(ContainerTestComplex(2));
	
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, list.Remove((*(list.Begin()))));
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, list.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(2), (*(list.Begin())));
}

void ListTest::TestRemoveRegressionSimple()
{
	List<ContainerTestSimple> list;
	list.PushBack(ContainerTestSimple::Create(1));
	list.PushBack(ContainerTestSimple::Create(2));
	
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, list.Remove((*(list.Begin()))));
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, list.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(2), (*(list.Begin())));
}

void ListTest::TestRemoveFirstInstanceBuiltin()
{
	List<UInt64, MemoryBudgets::TBDContainer> list;
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Begin(), list.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(list.IsEmpty());

	// Nop
	SEOUL_UNITTESTING_ASSERT(!list.RemoveFirstInstance(UInt64(25)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Begin(), list.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(list.IsEmpty());

	// Populate the list.
	list.PushBack(UInt64(25));
	list.PushBack(UInt64(23));
	list.PushBack(UInt64(25));
	list.PushBack(UInt64(25));
	list.PushBack(UInt64(17));

	// Removes.
	SEOUL_UNITTESTING_ASSERT(list.RemoveFirstInstance(UInt64(25)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(4, list.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(23, (*(list.Begin())));
	SEOUL_UNITTESTING_ASSERT_EQUAL(25, (*(++list.Begin())));
	SEOUL_UNITTESTING_ASSERT_EQUAL(25, (*(++(++list.Begin()))));
	SEOUL_UNITTESTING_ASSERT_EQUAL(17, (*(++(++(++list.Begin())))));
	
	SEOUL_UNITTESTING_ASSERT(!list.RemoveFirstInstance(UInt64(16)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(4, list.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(23, (*(list.Begin())));
	SEOUL_UNITTESTING_ASSERT_EQUAL(25, (*(++list.Begin())));
	SEOUL_UNITTESTING_ASSERT_EQUAL(25, (*(++(++list.Begin()))));
	SEOUL_UNITTESTING_ASSERT_EQUAL(17, (*(++(++(++list.Begin())))));
	
	SEOUL_UNITTESTING_ASSERT(list.RemoveFirstInstance(UInt64(25)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, list.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(23, (*(list.Begin())));
	SEOUL_UNITTESTING_ASSERT_EQUAL(25, (*(++list.Begin())));
	SEOUL_UNITTESTING_ASSERT_EQUAL(17, (*(++(++list.Begin()))));
	
	SEOUL_UNITTESTING_ASSERT(list.RemoveFirstInstance(UInt64(17)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, list.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(23, (*(list.Begin())));
	SEOUL_UNITTESTING_ASSERT_EQUAL(25, (*(++list.Begin())));
	
	SEOUL_UNITTESTING_ASSERT(list.RemoveFirstInstance(UInt64(23)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, list.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(25, (*(list.Begin())));
	
	SEOUL_UNITTESTING_ASSERT(list.RemoveFirstInstance(UInt64(25)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(list.IsEmpty());
}

void ListTest::TestRemoveFirstInstanceComplex()
{
	List<ContainerTestComplex, MemoryBudgets::TBDContainer> list;
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Begin(), list.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(list.IsEmpty());

	// Nop
	SEOUL_UNITTESTING_ASSERT(!list.RemoveFirstInstance(ContainerTestComplex(25)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Begin(), list.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(list.IsEmpty());

	// Populate the list.
	list.PushBack(ContainerTestComplex(25));
	list.PushBack(ContainerTestComplex(23));
	list.PushBack(ContainerTestComplex(25));
	list.PushBack(ContainerTestComplex(25));
	list.PushBack(ContainerTestComplex(17));

	// Removes.
	SEOUL_UNITTESTING_ASSERT(list.RemoveFirstInstance(ContainerTestComplex(25)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(4, list.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(23), (*(list.Begin())));
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(25), (*(++list.Begin())));
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(25), (*(++(++list.Begin()))));
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(17), (*(++(++(++list.Begin())))));
	
	SEOUL_UNITTESTING_ASSERT(!list.RemoveFirstInstance(ContainerTestComplex(16)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(4, list.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(23), (*(list.Begin())));
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(25), (*(++list.Begin())));
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(25), (*(++(++list.Begin()))));
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(17), (*(++(++(++list.Begin())))));
	
	SEOUL_UNITTESTING_ASSERT(list.RemoveFirstInstance(ContainerTestComplex(25)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, list.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(23), (*(list.Begin())));
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(25), (*(++list.Begin())));
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(17), (*(++(++list.Begin()))));
	
	SEOUL_UNITTESTING_ASSERT(list.RemoveFirstInstance(ContainerTestComplex(17)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, list.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(23), (*(list.Begin())));
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(25), (*(++list.Begin())));
	
	SEOUL_UNITTESTING_ASSERT(list.RemoveFirstInstance(ContainerTestComplex(23)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, list.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(25), (*(list.Begin())));
	
	SEOUL_UNITTESTING_ASSERT(list.RemoveFirstInstance(ContainerTestComplex(25)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(list.IsEmpty());
}

void ListTest::TestRemoveFirstInstanceComplexCoerce()
{
	List<ContainerTestComplex, MemoryBudgets::TBDContainer> list;
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Begin(), list.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(list.IsEmpty());

	// Nop
	SEOUL_UNITTESTING_ASSERT(!list.RemoveFirstInstance(25));
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Begin(), list.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(list.IsEmpty());

	// Populate the list.
	list.PushBack(ContainerTestComplex(25));
	list.PushBack(ContainerTestComplex(23));
	list.PushBack(ContainerTestComplex(25));
	list.PushBack(ContainerTestComplex(25));
	list.PushBack(ContainerTestComplex(17));

	// Removes.
	SEOUL_UNITTESTING_ASSERT(list.RemoveFirstInstance(25));
	SEOUL_UNITTESTING_ASSERT_EQUAL(4, list.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(23), (*(list.Begin())));
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(25), (*(++list.Begin())));
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(25), (*(++(++list.Begin()))));
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(17), (*(++(++(++list.Begin())))));
	
	SEOUL_UNITTESTING_ASSERT(!list.RemoveFirstInstance(16));
	SEOUL_UNITTESTING_ASSERT_EQUAL(4, list.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(23), (*(list.Begin())));
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(25), (*(++list.Begin())));
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(25), (*(++(++list.Begin()))));
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(17), (*(++(++(++list.Begin())))));
	
	SEOUL_UNITTESTING_ASSERT(list.RemoveFirstInstance(25));
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, list.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(23), (*(list.Begin())));
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(25), (*(++list.Begin())));
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(17), (*(++(++list.Begin()))));
	
	SEOUL_UNITTESTING_ASSERT(list.RemoveFirstInstance(17));
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, list.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(23), (*(list.Begin())));
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(25), (*(++list.Begin())));
	
	SEOUL_UNITTESTING_ASSERT(list.RemoveFirstInstance(23));
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, list.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(25), (*(list.Begin())));
	
	SEOUL_UNITTESTING_ASSERT(list.RemoveFirstInstance(25));
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(list.IsEmpty());
}

void ListTest::TestRemoveFirstInstanceSimple()
{
	List<ContainerTestSimple, MemoryBudgets::TBDContainer> list;
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Begin(), list.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(list.IsEmpty());

	// Nop
	SEOUL_UNITTESTING_ASSERT(!list.RemoveFirstInstance(ContainerTestSimple::Create(25)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Begin(), list.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(list.IsEmpty());

	// Populate the list.
	list.PushBack(ContainerTestSimple::Create(25));
	list.PushBack(ContainerTestSimple::Create(23));
	list.PushBack(ContainerTestSimple::Create(25));
	list.PushBack(ContainerTestSimple::Create(25));
	list.PushBack(ContainerTestSimple::Create(17));

	// Removes.
	SEOUL_UNITTESTING_ASSERT(list.RemoveFirstInstance(ContainerTestSimple::Create(25)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(4, list.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(23), (*(list.Begin())));
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(25), (*(++list.Begin())));
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(25), (*(++(++list.Begin()))));
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(17), (*(++(++(++list.Begin())))));
	
	SEOUL_UNITTESTING_ASSERT(!list.RemoveFirstInstance(ContainerTestSimple::Create(16)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(4, list.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(23), (*(list.Begin())));
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(25), (*(++list.Begin())));
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(25), (*(++(++list.Begin()))));
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(17), (*(++(++(++list.Begin())))));
	
	SEOUL_UNITTESTING_ASSERT(list.RemoveFirstInstance(ContainerTestSimple::Create(25)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, list.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(23), (*(list.Begin())));
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(25), (*(++list.Begin())));
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(17), (*(++(++list.Begin()))));
	
	SEOUL_UNITTESTING_ASSERT(list.RemoveFirstInstance(ContainerTestSimple::Create(17)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, list.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(23), (*(list.Begin())));
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(25), (*(++list.Begin())));
	
	SEOUL_UNITTESTING_ASSERT(list.RemoveFirstInstance(ContainerTestSimple::Create(23)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, list.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(25), (*(list.Begin())));
	
	SEOUL_UNITTESTING_ASSERT(list.RemoveFirstInstance(ContainerTestSimple::Create(25)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(list.IsEmpty());
}

void ListTest::TestRemoveCountBuiltin()
{
	List<Int, MemoryBudgets::TBDContainer> list;
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Begin(), list.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(list.IsEmpty());

	// Populate the list.
	list.PushBack(25);
	list.PushBack(23);
	list.PushBack(25);
	list.PushBack(25);
	list.PushBack(17);

	// Remove and test counts.
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, list.Remove(25));
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, list.Remove(17));
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, list.Remove(23));
	SEOUL_UNITTESTING_ASSERT(list.IsEmpty());
}

void ListTest::TestRemoveCountComplex()
{
	List<ContainerTestComplex, MemoryBudgets::TBDContainer> list;
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Begin(), list.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(list.IsEmpty());

	// Populate the list.
	list.PushBack(ContainerTestComplex(25));
	list.PushBack(ContainerTestComplex(23));
	list.PushBack(ContainerTestComplex(25));
	list.PushBack(ContainerTestComplex(25));
	list.PushBack(ContainerTestComplex(17));

	// Remove and test counts.
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, list.Remove(25));
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, list.Remove(17));
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, list.Remove(23));
	SEOUL_UNITTESTING_ASSERT(list.IsEmpty());
}

void ListTest::TestRemoveCountSimple()
{
	List<ContainerTestSimple, MemoryBudgets::TBDContainer> list;
	SEOUL_UNITTESTING_ASSERT_EQUAL(list.Begin(), list.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, list.GetSize());
	SEOUL_UNITTESTING_ASSERT(list.IsEmpty());

	// Populate the list.
	list.PushBack(ContainerTestSimple::Create(25));
	list.PushBack(ContainerTestSimple::Create(23));
	list.PushBack(ContainerTestSimple::Create(25));
	list.PushBack(ContainerTestSimple::Create(25));
	list.PushBack(ContainerTestSimple::Create(17));

	// Remove and test counts.
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, list.Remove(ContainerTestSimple::Create(25)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, list.Remove(ContainerTestSimple::Create(17)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, list.Remove(ContainerTestSimple::Create(23)));
	SEOUL_UNITTESTING_ASSERT(list.IsEmpty());
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
