/**
 * \file UnsafeBufferTest.cpp
 * \brief Unit test code for the Seoul UnsafeBuffer<> class
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
#include "StandardVertex2D.h"
#include "UnitTesting.h"
#include "UnsafeBuffer.h"
#include "UnsafeBufferTest.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

// For UnitTestingToString
SEOUL_BEGIN_TEMPLATE_TYPE(
	UnsafeBuffer,
	(T, MEMORY_BUDGETS),
	(typename T, Int MEMORY_BUDGETS),
	("UnsafeBuffer<%s, %d>", SEOUL_GET_FULLY_QUALIFIED_TYPE_NAME(T), MEMORY_BUDGETS))
SEOUL_END_TYPE()

SEOUL_SPEC_TEMPLATE_TYPE(UnsafeBuffer<ContainerTestSimple, 48>)
SEOUL_SPEC_TEMPLATE_TYPE(UnsafeBuffer<StandardVertex2D, 19>)
SEOUL_SPEC_TEMPLATE_TYPE(UnsafeBuffer<UInt16, 19>)
SEOUL_SPEC_TEMPLATE_TYPE(UnsafeBuffer<UInt32, 48>)

SEOUL_BEGIN_TYPE(UnsafeBufferTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestAppendBuiltin)
	SEOUL_METHOD(TestAppendSimple)
	SEOUL_METHOD(TestAssignBuiltin)
	SEOUL_METHOD(TestAssignSimple)
	SEOUL_METHOD(TestBasic)
	SEOUL_METHOD(TestClearBuiltin)
	SEOUL_METHOD(TestClearSimple)
	SEOUL_METHOD(TestConstructorBuiltin)
	SEOUL_METHOD(TestConstructorSimple)
	SEOUL_METHOD(TestEmptyBuiltin)
	SEOUL_METHOD(TestEmptySimple)
	SEOUL_METHOD(TestEqualityBuiltin)
	SEOUL_METHOD(TestEqualitySimple)
	SEOUL_METHOD(TestMethods)
	SEOUL_METHOD(TestIterators)
SEOUL_END_TYPE()

void UnsafeBufferTest::TestAppendBuiltin()
{
	// To empty;
	{
		UnsafeBuffer<UInt64, MemoryBudgets::TBDContainer> v;

		UnsafeBuffer<UInt64, MemoryBudgets::TBDContainer> v2;
		v2.PushBack(UInt64(12));
		v2.PushBack(UInt64(3209));
		v2.PushBack(UInt64(3090));

		v.Append(v2);

		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[0], 12);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[1], 3209);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[2], 3090);
	}

	// To empty, different type.
	{
		UnsafeBuffer<UInt64, MemoryBudgets::StateMachine> v;

		UnsafeBuffer<UInt64, MemoryBudgets::OperatorNewArray> v2;
		v2.PushBack(UInt64(12));
		v2.PushBack(UInt64(3209));
		v2.PushBack(UInt64(3090));

		v.Append(v2);

		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[0], 12);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[1], 3209);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[2], 3090);
	}

	// To empty, iterators.
	{
		UnsafeBuffer<UInt64, MemoryBudgets::StateMachine> v;

		UnsafeBuffer<UInt64, MemoryBudgets::OperatorNewArray> v2;
		v2.PushBack(UInt64(12));
		v2.PushBack(UInt64(3209));
		v2.PushBack(UInt64(3090));

		v.Append(v2.Begin() + 1, v2.End());

		SEOUL_UNITTESTING_ASSERT_EQUAL(2, v.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(2, v.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[0], 3209);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[1], 3090);
	}

	// To partial;
	{
		UnsafeBuffer<UInt64, MemoryBudgets::TBDContainer> v;
		v.PushBack(UInt64(7));
		v.PushBack(UInt64(91));
		v.PushBack(UInt64(313));

		UnsafeBuffer<UInt64, MemoryBudgets::TBDContainer> v2;
		v2.PushBack(UInt64(11));
		v2.PushBack(UInt64(323));
		v2.PushBack(UInt64(112));

		v.Append(v2);

		SEOUL_UNITTESTING_ASSERT_EQUAL(6, v.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(6, v.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[0], 7);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[1], 91);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[2], 313);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[3], 11);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[4], 323);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[5], 112);
	}

	// To partial, different type.
	{
		UnsafeBuffer<UInt64, MemoryBudgets::TBDContainer> v;
		v.PushBack(UInt64(7));
		v.PushBack(UInt64(91));
		v.PushBack(UInt64(313));

		UnsafeBuffer<UInt64, MemoryBudgets::Threading> v2;
		v2.PushBack(UInt64(11));
		v2.PushBack(UInt64(323));
		v2.PushBack(UInt64(112));

		v.Append(v2);

		SEOUL_UNITTESTING_ASSERT_EQUAL(6, v.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(6, v.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[0], 7);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[1], 91);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[2], 313);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[3], 11);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[4], 323);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[5], 112);
	}

	// To partial, iterators.
	{
		UnsafeBuffer<UInt64, MemoryBudgets::TBDContainer> v;
		v.PushBack(UInt64(7));
		v.PushBack(UInt64(91));
		v.PushBack(UInt64(313));

		UnsafeBuffer<UInt64, MemoryBudgets::Threading> v2;
		v2.PushBack(UInt64(11));
		v2.PushBack(UInt64(323));
		v2.PushBack(UInt64(112));

		v.Append(v2.Begin() + 1, v2.End());

		SEOUL_UNITTESTING_ASSERT_EQUAL(5, v.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, v.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[0], 7);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[1], 91);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[2], 313);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[3], 323);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[4], 112);
	}
}

void UnsafeBufferTest::TestAppendSimple()
{
	// To empty;
	{
		UnsafeBuffer<ContainerTestSimple, MemoryBudgets::TBDContainer> v;

		UnsafeBuffer<ContainerTestSimple, MemoryBudgets::TBDContainer> v2;
		ContainerTestSimple simple;
		simple.m_B = 33;

		simple.m_iA = 12;
		v2.PushBack(simple);
		simple.m_iA = 3209;
		v2.PushBack(simple);
		simple.m_iA = 3090;
		v2.PushBack(simple);

		v.Append(v2);

		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[0].m_iA, 12);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[1].m_iA, 3209);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[2].m_iA, 3090);
	}

	// To empty, different type.
	{
		UnsafeBuffer<ContainerTestSimple, MemoryBudgets::StateMachine> v;

		UnsafeBuffer<ContainerTestSimple, MemoryBudgets::OperatorNewArray> v2;
		ContainerTestSimple simple;
		simple.m_B = 33;

		simple.m_iA = 12;
		v2.PushBack(simple);
		simple.m_iA = 3209;
		v2.PushBack(simple);
		simple.m_iA = 3090;
		v2.PushBack(simple);

		v.Append(v2);

		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[0].m_iA, 12);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[1].m_iA, 3209);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[2].m_iA, 3090);
	}

	// To empty, iterators.
	{
		UnsafeBuffer<ContainerTestSimple, MemoryBudgets::StateMachine> v;

		UnsafeBuffer<ContainerTestSimple, MemoryBudgets::OperatorNewArray> v2;
		ContainerTestSimple simple;
		simple.m_B = 33;

		simple.m_iA = 12;
		v2.PushBack(simple);
		simple.m_iA = 3209;
		v2.PushBack(simple);
		simple.m_iA = 3090;
		v2.PushBack(simple);

		v.Append(v2.Begin() + 1, v2.End());

		SEOUL_UNITTESTING_ASSERT_EQUAL(2, v.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(2, v.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[0].m_iA, 3209);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[1].m_iA, 3090);
	}

	// To partial;
	{
		UnsafeBuffer<ContainerTestSimple, MemoryBudgets::TBDContainer> v;
		ContainerTestSimple simple;
		simple.m_B = 33;

		simple.m_iA = 7;
		v.PushBack(simple);
		simple.m_iA = 91;
		v.PushBack(simple);
		simple.m_iA = 313;
		v.PushBack(simple);

		UnsafeBuffer<ContainerTestSimple, MemoryBudgets::TBDContainer> v2;
		simple.m_iA = 11;
		v2.PushBack(simple);
		simple.m_iA = 323;
		v2.PushBack(simple);
		simple.m_iA = 112;
		v2.PushBack(simple);

		v.Append(v2);

		SEOUL_UNITTESTING_ASSERT_EQUAL(6, v.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(6, v.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[0].m_iA, 7);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[1].m_iA, 91);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[2].m_iA, 313);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[3].m_iA, 11);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[4].m_iA, 323);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[5].m_iA, 112);
	}

	// To partial, different type.
	{
		UnsafeBuffer<ContainerTestSimple, MemoryBudgets::TBDContainer> v;
		ContainerTestSimple simple;
		simple.m_B = 33;

		simple.m_iA = 7;
		v.PushBack(simple);
		simple.m_iA = 91;
		v.PushBack(simple);
		simple.m_iA = 313;
		v.PushBack(simple);

		UnsafeBuffer<ContainerTestSimple, MemoryBudgets::Threading> v2;
		simple.m_iA = 11;
		v2.PushBack(simple);
		simple.m_iA = 323;
		v2.PushBack(simple);
		simple.m_iA = 112;
		v2.PushBack(simple);

		v.Append(v2);

		SEOUL_UNITTESTING_ASSERT_EQUAL(6, v.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(6, v.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[0].m_iA, 7);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[1].m_iA, 91);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[2].m_iA, 313);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[3].m_iA, 11);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[4].m_iA, 323);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[5].m_iA, 112);
	}

	// To partial, iterators.
	{
		UnsafeBuffer<ContainerTestSimple, MemoryBudgets::TBDContainer> v;
		ContainerTestSimple simple;
		simple.m_B = 33;

		simple.m_iA = 7;
		v.PushBack(simple);
		simple.m_iA = 91;
		v.PushBack(simple);
		simple.m_iA = 313;
		v.PushBack(simple);

		UnsafeBuffer<ContainerTestSimple, MemoryBudgets::Threading> v2;
		simple.m_iA = 11;
		v2.PushBack(simple);
		simple.m_iA = 323;
		v2.PushBack(simple);
		simple.m_iA = 112;
		v2.PushBack(simple);

		v.Append(v2.Begin() + 1, v2.End());

		SEOUL_UNITTESTING_ASSERT_EQUAL(5, v.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, v.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[0].m_iA, 7);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[1].m_iA, 91);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[2].m_iA, 313);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[3].m_iA, 323);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[4].m_iA, 112);
	}
}

void UnsafeBufferTest::TestAssignBuiltin()
{
	// Copy self
	{
		UnsafeBuffer<UInt16, MemoryBudgets::DataStore> v1;
		v1.PushBack(UInt16(7));
		v1.PushBack(UInt16(11));
		v1.PushBack(UInt16(25));

		v1 = v1;
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v1.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3 * sizeof(UInt16), v1.GetCapacityInBytes());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v1.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3 * sizeof(UInt16), v1.GetSizeInBytes());
		SEOUL_UNITTESTING_ASSERT(!v1.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(UInt16(25), v1.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(UInt16(7), v1.Front());
		SEOUL_UNITTESTING_ASSERT_EQUAL(UInt16(11), v1.At(1));
		SEOUL_UNITTESTING_ASSERT_EQUAL(UInt16(11), *(v1.Begin() + 1));
		SEOUL_UNITTESTING_ASSERT_EQUAL(UInt16(11), v1.Data()[1]);
		SEOUL_UNITTESTING_ASSERT_EQUAL(UInt16(11), *(v1.End() - 1 - 1));
		SEOUL_UNITTESTING_ASSERT_EQUAL(UInt16(11), *v1.Get(1));
		SEOUL_UNITTESTING_ASSERT_EQUAL(UInt16(11), v1[1]);
	}

	// Copy
	{
		UnsafeBuffer<UInt16, MemoryBudgets::DataStore> v1;
		v1.PushBack(UInt16(7));
		v1.PushBack(UInt16(11));
		v1.PushBack(UInt16(25));

		UnsafeBuffer<UInt16, MemoryBudgets::DataStore> v2 = v1;
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v2.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3 * sizeof(UInt16), v2.GetCapacityInBytes());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v2.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3 * sizeof(UInt16), v2.GetSizeInBytes());
		SEOUL_UNITTESTING_ASSERT(!v2.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(v1.Back(), v2.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(v1.Front(), v2.Front());
		for (UInt32 i = 0u; i < 3; ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(v1.At(i), v2.At(i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(*(v1.Begin() + i), *(v2.Begin() + i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(v1.Data()[i], v2.Data()[i]);
			SEOUL_UNITTESTING_ASSERT_EQUAL(*(v1.End() - 1 - i), *(v2.End() - 1 - i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(*v1.Get(i), *v2.Get(i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(v1[i], v2[i]);
		}
	}

	// Copy templated
	{
		UnsafeBuffer<UInt16, MemoryBudgets::Falcon> v1;
		v1.PushBack(UInt16(7));
		v1.PushBack(UInt16(11));
		v1.PushBack(UInt16(25));

		UnsafeBuffer<UInt16, MemoryBudgets::Physics> v2;
		v2.PushBack(UInt16(112));
		v2.PushBack(UInt16(12));

		v2 = v1;
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v2.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3 * sizeof(UInt16), v2.GetCapacityInBytes());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v2.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3 * sizeof(UInt16), v2.GetSizeInBytes());
		SEOUL_UNITTESTING_ASSERT(!v2.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(v1.Back(), v2.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(v1.Front(), v2.Front());
		for (UInt32 i = 0u; i < 3; ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(v1.At(i), v2.At(i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(*(v1.Begin() + i), *(v2.Begin() + i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(v1.Data()[i], v2.Data()[i]);
			SEOUL_UNITTESTING_ASSERT_EQUAL(*(v1.End() - 1 - i), *(v2.End() - 1 - i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(*v1.Get(i), *v2.Get(i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(v1[i], v2[i]);
		}
	}

	// Iterator full
	{
		UnsafeBuffer<UInt16, MemoryBudgets::Falcon> v1;
		v1.PushBack(UInt16(7));
		v1.PushBack(UInt16(11));
		v1.PushBack(UInt16(25));

		UnsafeBuffer<UInt16, MemoryBudgets::Physics> v2;
		v2.PushBack(UInt16(191));
		v2.PushBack(UInt16(3981));
		v2.PushBack(UInt16(1298));
		v2.PushBack(UInt16(787));
		v2.PushBack(UInt16(12));

		v2.Assign(v1.Begin(), v1.End());
		SEOUL_UNITTESTING_ASSERT_EQUAL(6, v2.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(6 * sizeof(UInt16), v2.GetCapacityInBytes());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v2.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3 * sizeof(UInt16), v2.GetSizeInBytes());
		SEOUL_UNITTESTING_ASSERT(!v2.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(v1.Back(), v2.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(v1.Front(), v2.Front());
		for (UInt32 i = 0u; i < 3; ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(v1.At(i), v2.At(i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(*(v1.Begin() + i), *(v2.Begin() + i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(v1.Data()[i], v2.Data()[i]);
			SEOUL_UNITTESTING_ASSERT_EQUAL(*(v1.End() - 1 - i), *(v2.End() - 1 - i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(*v1.Get(i), *v2.Get(i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(v1[i], v2[i]);
		}
	}

	// Iterator partial
	{
		UnsafeBuffer<UInt16, MemoryBudgets::Falcon> v1;
		v1.PushBack(UInt16(3));
		v1.PushBack(UInt16(7));
		v1.PushBack(UInt16(11));
		v1.PushBack(UInt16(25));
		v1.PushBack(UInt16(91));

		UnsafeBuffer<UInt16, MemoryBudgets::Physics> v2;
		v2.PushBack(UInt16(191));
		v2.PushBack(UInt16(3981));
		v2.PushBack(UInt16(1298));

		v2.Assign(v1.Begin() + 1, v1.End() - 1);
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v2.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3 * sizeof(UInt16), v2.GetCapacityInBytes());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v2.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3 * sizeof(UInt16), v2.GetSizeInBytes());
		SEOUL_UNITTESTING_ASSERT(!v2.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(v1[v1.GetSize() - 2], v2.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(v1[1], v2.Front());
		for (UInt32 i = 0u; i < 3; ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(v1.At(i + 1), v2.At(i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(*(v1.Begin() + i + 1), *(v2.Begin() + i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(v1.Data()[i + 1], v2.Data()[i]);
			SEOUL_UNITTESTING_ASSERT_EQUAL(*(v1.End() - 1 - i - 1), *(v2.End() - 1 - i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(*v1.Get(i + 1), *v2.Get(i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(v1[i + 1], v2[i]);
		}
	}
}

void UnsafeBufferTest::TestAssignSimple()
{
	// Copy self
	{
		UnsafeBuffer<ContainerTestSimple, MemoryBudgets::DataStore> v1;
		v1.PushBack(ContainerTestSimple::Create(7));
		v1.PushBack(ContainerTestSimple::Create(11));
		v1.PushBack(ContainerTestSimple::Create(25));

		v1 = v1;
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v1.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3 * sizeof(ContainerTestSimple), v1.GetCapacityInBytes());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v1.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3 * sizeof(ContainerTestSimple), v1.GetSizeInBytes());
		SEOUL_UNITTESTING_ASSERT(!v1.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(25), v1.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(7), v1.Front());
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(11), v1.At(1));
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(11), *(v1.Begin() + 1));
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(11), v1.Data()[1]);
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(11), *(v1.End() - 1 - 1));
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(11), *v1.Get(1));
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(11), v1[1]);
	}

	// Copy
	{
		ContainerTestSimple simple;
		simple.m_B = 33;
		simple.m_iA = 7;
		UnsafeBuffer<ContainerTestSimple, MemoryBudgets::DataStore> v1;
		v1.PushBack(simple);
		simple.m_iA = 11;
		v1.PushBack(simple);
		simple.m_iA = 25;
		v1.PushBack(simple);

		UnsafeBuffer<ContainerTestSimple, MemoryBudgets::DataStore> v2 = v1;
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v2.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3 * sizeof(ContainerTestSimple), v2.GetCapacityInBytes());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v2.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3 * sizeof(ContainerTestSimple), v2.GetSizeInBytes());
		SEOUL_UNITTESTING_ASSERT(!v2.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(v1.Back(), v2.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(v1.Front(), v2.Front());
		for (UInt32 i = 0u; i < 3; ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(v1.At(i), v2.At(i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(*(v1.Begin() + i), *(v2.Begin() + i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(v1.Data()[i], v2.Data()[i]);
			SEOUL_UNITTESTING_ASSERT_EQUAL(*(v1.End() - 1 - i), *(v2.End() - 1 - i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(*v1.Get(i), *v2.Get(i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(v1[i], v2[i]);
		}
	}

	// Copy templated
	{
		ContainerTestSimple simple;
		simple.m_B = 33;
		simple.m_iA = 7;
		UnsafeBuffer<ContainerTestSimple, MemoryBudgets::DataStore> v1;
		v1.PushBack(simple);
		simple.m_iA = 11;
		v1.PushBack(simple);
		simple.m_iA = 25;
		v1.PushBack(simple);

		UnsafeBuffer<ContainerTestSimple, MemoryBudgets::Physics> v2;
		simple.m_iA = 122;
		v2.PushBack(simple);
		simple.m_iA = 12;
		v2.PushBack(simple);

		v2 = v1;
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v2.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3 * sizeof(ContainerTestSimple), v2.GetCapacityInBytes());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v2.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3 * sizeof(ContainerTestSimple), v2.GetSizeInBytes());
		SEOUL_UNITTESTING_ASSERT(!v2.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(v1.Back(), v2.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(v1.Front(), v2.Front());
		for (UInt32 i = 0u; i < 3; ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(v1.At(i), v2.At(i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(*(v1.Begin() + i), *(v2.Begin() + i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(v1.Data()[i], v2.Data()[i]);
			SEOUL_UNITTESTING_ASSERT_EQUAL(*(v1.End() - 1 - i), *(v2.End() - 1 - i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(*v1.Get(i), *v2.Get(i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(v1[i], v2[i]);
		}
	}

	// Iterator full
	{
		ContainerTestSimple simple;
		simple.m_B = 33;
		simple.m_iA = 7;
		UnsafeBuffer<ContainerTestSimple, MemoryBudgets::DataStore> v1;
		v1.PushBack(simple);
		simple.m_iA = 11;
		v1.PushBack(simple);
		simple.m_iA = 25;
		v1.PushBack(simple);

		UnsafeBuffer<ContainerTestSimple, MemoryBudgets::Physics> v2;
		simple.m_iA = 191;
		v2.PushBack(simple);
		simple.m_iA = 3981;
		v2.PushBack(simple);
		simple.m_iA = 1298;
		v2.PushBack(simple);
		simple.m_iA = 787;
		v2.PushBack(simple);
		simple.m_iA = 12;
		v2.PushBack(simple);

		v2.Assign(v1.Begin(), v1.End());
		SEOUL_UNITTESTING_ASSERT_EQUAL(6, v2.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(6 * sizeof(ContainerTestSimple), v2.GetCapacityInBytes());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v2.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3 * sizeof(ContainerTestSimple), v2.GetSizeInBytes());
		SEOUL_UNITTESTING_ASSERT(!v2.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(v1.Back(), v2.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(v1.Front(), v2.Front());
		for (UInt32 i = 0u; i < 3; ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(v1.At(i), v2.At(i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(*(v1.Begin() + i), *(v2.Begin() + i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(v1.Data()[i], v2.Data()[i]);
			SEOUL_UNITTESTING_ASSERT_EQUAL(*(v1.End() - 1 - i), *(v2.End() - 1 - i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(*v1.Get(i), *v2.Get(i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(v1[i], v2[i]);
		}
	}

	// Iterator partial
	{
		ContainerTestSimple simple;
		simple.m_B = 33;
		UnsafeBuffer<ContainerTestSimple, MemoryBudgets::Falcon> v1;
		simple.m_iA = 3;
		v1.PushBack(simple);
		simple.m_iA = 7;
		v1.PushBack(simple);
		simple.m_iA = 11;
		v1.PushBack(simple);
		simple.m_iA = 25;
		v1.PushBack(simple);
		simple.m_iA = 91;
		v1.PushBack(simple);

		UnsafeBuffer<ContainerTestSimple, MemoryBudgets::Physics> v2;
		simple.m_iA = 191;
		v2.PushBack(simple);
		simple.m_iA = 3981;
		v2.PushBack(simple);
		simple.m_iA = 1298;
		v2.PushBack(simple);

		v2.Assign(v1.Begin() + 1, v1.End() - 1);
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v2.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3 * sizeof(ContainerTestSimple), v2.GetCapacityInBytes());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v2.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3 * sizeof(ContainerTestSimple), v2.GetSizeInBytes());
		SEOUL_UNITTESTING_ASSERT(!v2.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(v1[v1.GetSize() - 2], v2.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(v1[1], v2.Front());
		for (UInt32 i = 0u; i < 3; ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(v1.At(i + 1), v2.At(i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(*(v1.Begin() + i + 1), *(v2.Begin() + i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(v1.Data()[i + 1], v2.Data()[i]);
			SEOUL_UNITTESTING_ASSERT_EQUAL(*(v1.End() - 1 - i - 1), *(v2.End() - 1 - i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(*v1.Get(i + 1), *v2.Get(i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(v1[i + 1], v2[i]);
		}
	}

	// Size with default value.
	{
		ContainerTestSimple simple;
		simple.m_B = 33;
		UnsafeBuffer<ContainerTestSimple, MemoryBudgets::Falcon> v;
		simple.m_iA = 908;
		v.PushBack(simple);
		simple.m_iA = 124;
		v.PushBack(simple);
		simple.m_iA = 457;
		v.PushBack(simple);

		v.Assign(5);
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, v.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(5 * sizeof(ContainerTestSimple), v.GetCapacityInBytes());
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, v.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(5 * sizeof(ContainerTestSimple), v.GetSizeInBytes());
		SEOUL_UNITTESTING_ASSERT(!v.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple(), v.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple(), v.Front());
		for (UInt32 i = 0u; i < 3; ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple(), v.At(i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple(), *(v.Begin() + i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple(), v.Data()[i]);
			SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple(), *(v.End() - 1 - i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple(), *v.Get(i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple(), v[i]);
		}
	}

	// Size with value.
	{
		ContainerTestSimple simple;
		simple.m_B = 33;
		UnsafeBuffer<ContainerTestSimple, MemoryBudgets::Falcon> v;
		simple.m_iA = 3904;
		v.PushBack(simple);
		simple.m_iA = 144;
		v.PushBack(simple);
		simple.m_iA = 389;
		v.PushBack(simple);

		simple.m_iA = 77;
		v.Assign(5, simple);
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, v.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(5 * sizeof(ContainerTestSimple), v.GetCapacityInBytes());
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, v.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(5 * sizeof(ContainerTestSimple), v.GetSizeInBytes());
		SEOUL_UNITTESTING_ASSERT(!v.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(simple, v.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(simple, v.Front());
		for (UInt32 i = 0u; i < 3; ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(simple, v.At(i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(simple, *(v.Begin() + i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(simple, v.Data()[i]);
			SEOUL_UNITTESTING_ASSERT_EQUAL(simple, *(v.End() - 1 - i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(simple, *v.Get(i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(simple, v[i]);
		}
	}
}

void UnsafeBufferTest::TestBasic()
{
	UnsafeBuffer<Int> testVec;
	// GetCapacity() should return the initial size of the UnsafeBuffer
	SEOUL_UNITTESTING_ASSERT( testVec.GetCapacity() == 0);
	SEOUL_UNITTESTING_ASSERT( testVec.GetSize() == 0);

	for(Int i=0; i<10; i++)
	{
		testVec.PushBack(i);
	}
	SEOUL_UNITTESTING_ASSERT(testVec.GetSize() == 10);
}

void UnsafeBufferTest::TestClearBuiltin()
{
	UnsafeBuffer<UInt16, MemoryBudgets::Audio> v;
	v.PushBack(UInt16(23));
	v.PushBack(UInt16(194));
	v.PushBack(UInt16(119));

	// Clear should destroy elements but leave capacity.
	v.Clear();
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, v.GetCapacity());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSize());
	SEOUL_UNITTESTING_ASSERT(v.IsEmpty());

	v.PushBack(UInt16(7));
	v.PushBack(UInt16(1123));
	v.PushBack(UInt16(434));
	v.PushBack(UInt16(342));
	v.PushBack(UInt16(23989));

	SEOUL_UNITTESTING_ASSERT_EQUAL(6, v.GetCapacity());
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, v.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(v[0], 7);
	SEOUL_UNITTESTING_ASSERT_EQUAL(v[1], 1123);
	SEOUL_UNITTESTING_ASSERT_EQUAL(v[2], 434);
	SEOUL_UNITTESTING_ASSERT_EQUAL(v[3], 342);
	SEOUL_UNITTESTING_ASSERT_EQUAL(v[4], 23989);

	// Now shrink - this should get us a capacity of 5.
	v.ShrinkToFit();
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, v.GetCapacity());
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, v.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(v[0], 7);
	SEOUL_UNITTESTING_ASSERT_EQUAL(v[1], 1123);
	SEOUL_UNITTESTING_ASSERT_EQUAL(v[2], 434);
	SEOUL_UNITTESTING_ASSERT_EQUAL(v[3], 342);
	SEOUL_UNITTESTING_ASSERT_EQUAL(v[4], 23989);

	// Clear again.
	v.Clear();
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, v.GetCapacity());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSize());
	SEOUL_UNITTESTING_ASSERT(v.IsEmpty());

	// Now shrink - this should completely free the memory.
	v.ShrinkToFit();
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetCapacity());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSize());
	SEOUL_UNITTESTING_ASSERT(v.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(nullptr, v.Data());

	v.PushBack(UInt16(3));
	v.PushBack(UInt16(124));
	v.PushBack(UInt16(342));
	v.PushBack(UInt16(12));
	v.PushBack(UInt16(33));
	SEOUL_UNITTESTING_ASSERT_EQUAL(6, v.GetCapacity());
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, v.GetSize());

	// Now do the "swap trick". This should completely free the memory.
	{
		UnsafeBuffer<UInt16, MemoryBudgets::Audio> v2;
		v2.Swap(v);

		// v is now empty
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSize());
		SEOUL_UNITTESTING_ASSERT(v.IsEmpty());
		SEOUL_UNITTESTING_ASSERT_EQUAL(nullptr, v.Data());

		// v2 has v's state.
		SEOUL_UNITTESTING_ASSERT_EQUAL(6, v2.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, v2.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(v2[0], 3);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v2[1], 124);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v2[2], 342);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v2[3], 12);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v2[4], 33);
	}
}

void UnsafeBufferTest::TestClearSimple()
{
	UnsafeBuffer<ContainerTestSimple, MemoryBudgets::Audio> v;
	ContainerTestSimple simple;
	simple.m_B = 33;
	simple.m_iA = 23;
	v.PushBack(simple);
	simple.m_iA = 194;
	v.PushBack(simple);
	simple.m_iA = 119;
	v.PushBack(simple);

	// Clear should destroy elements but leave capacity.
	v.Clear();
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, v.GetCapacity());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSize());
	SEOUL_UNITTESTING_ASSERT(v.IsEmpty());

	simple.m_iA = 7;
	v.PushBack(simple);
	simple.m_iA = 1123;
	v.PushBack(simple);
	simple.m_iA = 434;
	v.PushBack(simple);
	simple.m_iA = 342;
	v.PushBack(simple);
	simple.m_iA = 23989;
	v.PushBack(simple);

	SEOUL_UNITTESTING_ASSERT_EQUAL(6, v.GetCapacity());
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, v.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(v[0].m_iA, 7);
	SEOUL_UNITTESTING_ASSERT_EQUAL(v[1].m_iA, 1123);
	SEOUL_UNITTESTING_ASSERT_EQUAL(v[2].m_iA, 434);
	SEOUL_UNITTESTING_ASSERT_EQUAL(v[3].m_iA, 342);
	SEOUL_UNITTESTING_ASSERT_EQUAL(v[4].m_iA, 23989);

	// Now shrink - this should get us a capacity of 5.
	v.ShrinkToFit();
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, v.GetCapacity());
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, v.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(v[0].m_iA, 7);
	SEOUL_UNITTESTING_ASSERT_EQUAL(v[1].m_iA, 1123);
	SEOUL_UNITTESTING_ASSERT_EQUAL(v[2].m_iA, 434);
	SEOUL_UNITTESTING_ASSERT_EQUAL(v[3].m_iA, 342);
	SEOUL_UNITTESTING_ASSERT_EQUAL(v[4].m_iA, 23989);

	// Clear again.
	v.Clear();
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, v.GetCapacity());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSize());
	SEOUL_UNITTESTING_ASSERT(v.IsEmpty());

	// Now shrink - this should completely free the memory.
	v.ShrinkToFit();
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetCapacity());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSize());
	SEOUL_UNITTESTING_ASSERT(v.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(nullptr, v.Data());

	simple.m_iA = 3;
	v.PushBack(simple);
	simple.m_iA = 124;
	v.PushBack(simple);
	simple.m_iA = 342;
	v.PushBack(simple);
	simple.m_iA = 12;
	v.PushBack(simple);
	simple.m_iA = 33;
	v.PushBack(simple);
	SEOUL_UNITTESTING_ASSERT_EQUAL(6, v.GetCapacity());
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, v.GetSize());

	// Now do the "swap trick". This should completely free the memory.
	{
		UnsafeBuffer<ContainerTestSimple, MemoryBudgets::Audio> v2;
		v2.Swap(v);

		// v is now empty
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSize());
		SEOUL_UNITTESTING_ASSERT(v.IsEmpty());
		SEOUL_UNITTESTING_ASSERT_EQUAL(nullptr, v.Data());

		// v2 has v's state.
		SEOUL_UNITTESTING_ASSERT_EQUAL(6, v2.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, v2.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(v2[0].m_iA, 3);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v2[1].m_iA, 124);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v2[2].m_iA, 342);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v2[3].m_iA, 12);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v2[4].m_iA, 33);
	}
}

void UnsafeBufferTest::TestConstructorBuiltin()
{
	// Default.
	{
		UnsafeBuffer<Int64, MemoryBudgets::DataStore> v;
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetCapacityInBytes());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSizeInBytes());
		SEOUL_UNITTESTING_ASSERT(v.IsEmpty());
	}

	// Copy
	{
		UnsafeBuffer<Int64, MemoryBudgets::DataStore> v1;
		v1.PushBack(Int64(7));
		v1.PushBack(Int64(11));
		v1.PushBack(Int64(25));

		UnsafeBuffer<Int64, MemoryBudgets::DataStore> v2(v1);
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v2.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3 * sizeof(Int64), v2.GetCapacityInBytes());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v2.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3 * sizeof(Int64), v2.GetSizeInBytes());
		SEOUL_UNITTESTING_ASSERT(!v2.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(v1.Back(), v2.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(v1.Front(), v2.Front());
		for (UInt32 i = 0u; i < 3; ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(v1.At(i), v2.At(i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(*(v1.Begin() + i), *(v2.Begin() + i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(v1.Data()[i], v2.Data()[i]);
			SEOUL_UNITTESTING_ASSERT_EQUAL(*(v1.End() - 1 - i), *(v2.End() - 1 - i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(*v1.Get(i), *v2.Get(i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(v1[i], v2[i]);
		}
	}

	// Copy templated
	{
		UnsafeBuffer<Int64, MemoryBudgets::Falcon> v1;
		v1.PushBack(Int64(7));
		v1.PushBack(Int64(11));
		v1.PushBack(Int64(25));

		UnsafeBuffer<Int64, MemoryBudgets::Physics> v2(v1);
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v2.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3 * sizeof(Int64), v2.GetCapacityInBytes());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v2.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3 * sizeof(Int64), v2.GetSizeInBytes());
		SEOUL_UNITTESTING_ASSERT(!v2.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(v1.Back(), v2.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(v1.Front(), v2.Front());
		for (UInt32 i = 0u; i < 3; ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(v1.At(i), v2.At(i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(*(v1.Begin() + i), *(v2.Begin() + i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(v1.Data()[i], v2.Data()[i]);
			SEOUL_UNITTESTING_ASSERT_EQUAL(*(v1.End() - 1 - i), *(v2.End() - 1 - i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(*v1.Get(i), *v2.Get(i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(v1[i], v2[i]);
		}
	}

	// Iterator full
	{
		UnsafeBuffer<Int64, MemoryBudgets::Falcon> v1;
		v1.PushBack(Int64(7));
		v1.PushBack(Int64(11));
		v1.PushBack(Int64(25));

		UnsafeBuffer<Int64, MemoryBudgets::Physics> v2(v1.Begin(), v1.End());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v2.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3 * sizeof(Int64), v2.GetCapacityInBytes());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v2.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3 * sizeof(Int64), v2.GetSizeInBytes());
		SEOUL_UNITTESTING_ASSERT(!v2.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(v1.Back(), v2.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(v1.Front(), v2.Front());
		for (UInt32 i = 0u; i < 3; ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(v1.At(i), v2.At(i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(*(v1.Begin() + i), *(v2.Begin() + i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(v1.Data()[i], v2.Data()[i]);
			SEOUL_UNITTESTING_ASSERT_EQUAL(*(v1.End() - 1 - i), *(v2.End() - 1 - i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(*v1.Get(i), *v2.Get(i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(v1[i], v2[i]);
		}
	}

	// Iterator partial
	{
		UnsafeBuffer<Int64, MemoryBudgets::Falcon> v1;
		v1.PushBack(Int64(3));
		v1.PushBack(Int64(7));
		v1.PushBack(Int64(11));
		v1.PushBack(Int64(25));
		v1.PushBack(Int64(91));

		UnsafeBuffer<Int64, MemoryBudgets::Physics> v2(v1.Begin() + 1, v1.End() - 1);
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v2.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3 * sizeof(Int64), v2.GetCapacityInBytes());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v2.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3 * sizeof(Int64), v2.GetSizeInBytes());
		SEOUL_UNITTESTING_ASSERT(!v2.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(v1[v1.GetSize() - 2], v2.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(v1[1], v2.Front());
		for (UInt32 i = 0u; i < 3; ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(v1.At(i + 1), v2.At(i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(*(v1.Begin() + i + 1), *(v2.Begin() + i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(v1.Data()[i + 1], v2.Data()[i]);
			SEOUL_UNITTESTING_ASSERT_EQUAL(*(v1.End() - 1 - i - 1), *(v2.End() - 1 - i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(*v1.Get(i + 1), *v2.Get(i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(v1[i + 1], v2[i]);
		}
	}
}

void UnsafeBufferTest::TestConstructorSimple()
{
	// Default.
	{
		UnsafeBuffer<ContainerTestSimple, MemoryBudgets::DataStore> v;
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetCapacityInBytes());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSizeInBytes());
		SEOUL_UNITTESTING_ASSERT(v.IsEmpty());
	}

	// Copy
	{
		ContainerTestSimple simple;
		simple.m_B = 33;
		simple.m_iA = 7;
		UnsafeBuffer<ContainerTestSimple, MemoryBudgets::DataStore> v1;
		v1.PushBack(simple);
		simple.m_iA = 11;
		v1.PushBack(simple);
		simple.m_iA = 25;
		v1.PushBack(simple);

		UnsafeBuffer<ContainerTestSimple, MemoryBudgets::DataStore> v2(v1);
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v2.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3 * sizeof(ContainerTestSimple), v2.GetCapacityInBytes());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v2.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3 * sizeof(ContainerTestSimple), v2.GetSizeInBytes());
		SEOUL_UNITTESTING_ASSERT(!v2.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(v1.Back(), v2.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(v1.Front(), v2.Front());
		for (UInt32 i = 0u; i < 3; ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(v1.At(i), v2.At(i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(*(v1.Begin() + i), *(v2.Begin() + i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(v1.Data()[i], v2.Data()[i]);
			SEOUL_UNITTESTING_ASSERT_EQUAL(*(v1.End() - 1 - i), *(v2.End() - 1 - i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(*v1.Get(i), *v2.Get(i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(v1[i], v2[i]);
		}
	}

	// Copy templated
	{
		ContainerTestSimple simple;
		simple.m_B = 33;
		simple.m_iA = 7;
		UnsafeBuffer<ContainerTestSimple, MemoryBudgets::Falcon> v1;
		v1.PushBack(simple);
		simple.m_iA = 11;
		v1.PushBack(simple);
		simple.m_iA = 25;
		v1.PushBack(simple);

		UnsafeBuffer<ContainerTestSimple, MemoryBudgets::Physics> v2(v1);
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v2.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3 * sizeof(ContainerTestSimple), v2.GetCapacityInBytes());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v2.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3 * sizeof(ContainerTestSimple), v2.GetSizeInBytes());
		SEOUL_UNITTESTING_ASSERT(!v2.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(v1.Back(), v2.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(v1.Front(), v2.Front());
		for (UInt32 i = 0u; i < 3; ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(v1.At(i), v2.At(i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(*(v1.Begin() + i), *(v2.Begin() + i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(v1.Data()[i], v2.Data()[i]);
			SEOUL_UNITTESTING_ASSERT_EQUAL(*(v1.End() - 1 - i), *(v2.End() - 1 - i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(*v1.Get(i), *v2.Get(i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(v1[i], v2[i]);
		}
	}

	// Iterator full
	{
		ContainerTestSimple simple;
		simple.m_B = 33;
		simple.m_iA = 7;
		UnsafeBuffer<ContainerTestSimple, MemoryBudgets::Falcon> v1;
		v1.PushBack(simple);
		simple.m_iA = 11;
		v1.PushBack(simple);
		simple.m_iA = 25;
		v1.PushBack(simple);

		UnsafeBuffer<ContainerTestSimple, MemoryBudgets::Physics> v2(v1.Begin(), v1.End());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v2.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3 * sizeof(ContainerTestSimple), v2.GetCapacityInBytes());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v2.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3 * sizeof(ContainerTestSimple), v2.GetSizeInBytes());
		SEOUL_UNITTESTING_ASSERT(!v2.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(v1.Back(), v2.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(v1.Front(), v2.Front());
		for (UInt32 i = 0u; i < 3; ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(v1.At(i), v2.At(i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(*(v1.Begin() + i), *(v2.Begin() + i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(v1.Data()[i], v2.Data()[i]);
			SEOUL_UNITTESTING_ASSERT_EQUAL(*(v1.End() - 1 - i), *(v2.End() - 1 - i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(*v1.Get(i), *v2.Get(i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(v1[i], v2[i]);
		}
	}

	// Iterator partial
	{
		ContainerTestSimple simple;
		simple.m_B = 33;
		simple.m_iA = 3;
		UnsafeBuffer<ContainerTestSimple, MemoryBudgets::Falcon> v1;
		v1.PushBack(simple);
		simple.m_iA = 7;
		v1.PushBack(simple);
		simple.m_iA = 11;
		v1.PushBack(simple);
		simple.m_iA = 25;
		v1.PushBack(simple);
		simple.m_iA = 91;
		v1.PushBack(simple);

		UnsafeBuffer<ContainerTestSimple, MemoryBudgets::Physics> v2(v1.Begin() + 1, v1.End() - 1);
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v2.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3 * sizeof(ContainerTestSimple), v2.GetCapacityInBytes());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v2.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3 * sizeof(ContainerTestSimple), v2.GetSizeInBytes());
		SEOUL_UNITTESTING_ASSERT(!v2.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(v1[v1.GetSize() - 2], v2.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(v1[1], v2.Front());
		for (UInt32 i = 0u; i < 3; ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(v1.At(i + 1), v2.At(i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(*(v1.Begin() + i + 1), *(v2.Begin() + i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(v1.Data()[i + 1], v2.Data()[i]);
			SEOUL_UNITTESTING_ASSERT_EQUAL(*(v1.End() - 1 - i - 1), *(v2.End() - 1 - i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(*v1.Get(i + 1), *v2.Get(i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(v1[i + 1], v2[i]);
		}
	}
}

void UnsafeBufferTest::TestEmptyBuiltin()
{
	UnsafeBuffer<Int16, MemoryBudgets::DataStore> v;
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetCapacity());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetCapacityInBytes());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSizeInBytes());
	SEOUL_UNITTESTING_ASSERT(v.IsEmpty());

	SEOUL_UNITTESTING_ASSERT_EQUAL(v.Begin(), v.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(nullptr, v.Data());
	SEOUL_UNITTESTING_ASSERT_EQUAL(v.End(), v.Begin());

	v.ResizeNoInitialize(0);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetCapacity());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetCapacityInBytes());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSizeInBytes());
	SEOUL_UNITTESTING_ASSERT(v.IsEmpty());

	v.ShrinkToFit();
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetCapacity());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetCapacityInBytes());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSizeInBytes());
	SEOUL_UNITTESTING_ASSERT(v.IsEmpty());

	{
		UnsafeBuffer<Int16, MemoryBudgets::DataStore> v2;
		v.Swap(v2);
	}

	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetCapacity());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetCapacityInBytes());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSizeInBytes());
	SEOUL_UNITTESTING_ASSERT(v.IsEmpty());

	v.PushBack(53);
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(nullptr, v.Data());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, v.GetCapacity());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1 * sizeof(Int16), v.GetCapacityInBytes());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, v.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1 * sizeof(Int16), v.GetSizeInBytes());
	SEOUL_UNITTESTING_ASSERT(!v.IsEmpty());

	SEOUL_UNITTESTING_ASSERT_EQUAL((size_t)1, (size_t)(v.End() - v.Begin()));

	SEOUL_UNITTESTING_ASSERT_EQUAL(53, v.At(0));
	SEOUL_UNITTESTING_ASSERT_EQUAL(53, v.Back());
	SEOUL_UNITTESTING_ASSERT_EQUAL(53, *v.Begin());
	SEOUL_UNITTESTING_ASSERT_EQUAL(53, v.Data()[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(53, *(v.End() - 1));
	SEOUL_UNITTESTING_ASSERT_EQUAL(53, v.Front());
	SEOUL_UNITTESTING_ASSERT_EQUAL(53, *v.Get(0));
	SEOUL_UNITTESTING_ASSERT_EQUAL(53, v[0]);

	v.ShrinkToFit();
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(nullptr, v.Data());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, v.GetCapacity());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1 * sizeof(Int16), v.GetCapacityInBytes());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, v.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1 * sizeof(Int16), v.GetSizeInBytes());
	SEOUL_UNITTESTING_ASSERT(!v.IsEmpty());

	auto i = v.Begin();
	v.PopBack();
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(nullptr, v.Data());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, v.GetCapacity());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1 * sizeof(Int16), v.GetCapacityInBytes());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSizeInBytes());
	SEOUL_UNITTESTING_ASSERT(v.IsEmpty());

	// Iterator should not have been invalidated by the PopBack.
	SEOUL_UNITTESTING_ASSERT_EQUAL(v.Begin(), i);
	SEOUL_UNITTESTING_ASSERT_EQUAL(v.End(), i);

	// Now Shrink - should give us a nullptr before again.
	v.ShrinkToFit();
	SEOUL_UNITTESTING_ASSERT_EQUAL(nullptr, v.Data());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetCapacity());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetCapacityInBytes());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSizeInBytes());
	SEOUL_UNITTESTING_ASSERT(v.IsEmpty());
}

void UnsafeBufferTest::TestEmptySimple()
{
	ContainerTestSimple simple;

	UnsafeBuffer<ContainerTestSimple, MemoryBudgets::DataStore> v;
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetCapacity());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetCapacityInBytes());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSizeInBytes());
	SEOUL_UNITTESTING_ASSERT(v.IsEmpty());

	SEOUL_UNITTESTING_ASSERT_EQUAL(v.Begin(), v.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(nullptr, v.Data());
	SEOUL_UNITTESTING_ASSERT_EQUAL(v.End(), v.Begin());

	v.ResizeNoInitialize(0);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetCapacity());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetCapacityInBytes());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSizeInBytes());
	SEOUL_UNITTESTING_ASSERT(v.IsEmpty());

	v.ShrinkToFit();
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetCapacity());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetCapacityInBytes());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSizeInBytes());
	SEOUL_UNITTESTING_ASSERT(v.IsEmpty());

	{
		UnsafeBuffer<ContainerTestSimple, MemoryBudgets::DataStore> v2;
		v.Swap(v2);
	}

	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetCapacity());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetCapacityInBytes());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSizeInBytes());
	SEOUL_UNITTESTING_ASSERT(v.IsEmpty());

	simple.m_B = 33;
	simple.m_iA = 53;
	v.PushBack(simple);
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(nullptr, v.Data());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, v.GetCapacity());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1 * sizeof(ContainerTestSimple), v.GetCapacityInBytes());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, v.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1 * sizeof(ContainerTestSimple), v.GetSizeInBytes());
	SEOUL_UNITTESTING_ASSERT(!v.IsEmpty());

	SEOUL_UNITTESTING_ASSERT_EQUAL((size_t)1, (size_t)(v.End() - v.Begin()));

	SEOUL_UNITTESTING_ASSERT_EQUAL(53, v.At(0).m_iA);
	SEOUL_UNITTESTING_ASSERT_EQUAL(33, v.At(0).m_B);
	SEOUL_UNITTESTING_ASSERT_EQUAL(53, v.Back().m_iA);
	SEOUL_UNITTESTING_ASSERT_EQUAL(33, v.Back().m_B);
	SEOUL_UNITTESTING_ASSERT_EQUAL(53, v.Begin()->m_iA);
	SEOUL_UNITTESTING_ASSERT_EQUAL(33, v.Begin()->m_B);
	SEOUL_UNITTESTING_ASSERT_EQUAL(53, v.Data()[0].m_iA);
	SEOUL_UNITTESTING_ASSERT_EQUAL(33, v.Data()[0].m_B);
	SEOUL_UNITTESTING_ASSERT_EQUAL(53, (v.End() - 1)->m_iA);
	SEOUL_UNITTESTING_ASSERT_EQUAL(33, (v.End() - 1)->m_B);
	SEOUL_UNITTESTING_ASSERT_EQUAL(53, v.Front().m_iA);
	SEOUL_UNITTESTING_ASSERT_EQUAL(33, v.Front().m_B);
	SEOUL_UNITTESTING_ASSERT_EQUAL(53, v.Get(0)->m_iA);
	SEOUL_UNITTESTING_ASSERT_EQUAL(33, v.Get(0)->m_B);
	SEOUL_UNITTESTING_ASSERT_EQUAL(53, v[0].m_iA);
	SEOUL_UNITTESTING_ASSERT_EQUAL(33, v[0].m_B);

	v.ShrinkToFit();
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(nullptr, v.Data());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, v.GetCapacity());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1 * sizeof(ContainerTestSimple), v.GetCapacityInBytes());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, v.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1 * sizeof(ContainerTestSimple), v.GetSizeInBytes());
	SEOUL_UNITTESTING_ASSERT(!v.IsEmpty());

	auto i = v.Begin();
	v.PopBack();
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(nullptr, v.Data());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, v.GetCapacity());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1 * sizeof(ContainerTestSimple), v.GetCapacityInBytes());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSizeInBytes());
	SEOUL_UNITTESTING_ASSERT(v.IsEmpty());

	// Iterator should not have been invalidated by the PopBack.
	SEOUL_UNITTESTING_ASSERT_EQUAL(v.Begin(), i);
	SEOUL_UNITTESTING_ASSERT_EQUAL(v.End(), i);

	// Now Shrink - should give us a nullptr before again.
	v.ShrinkToFit();
	SEOUL_UNITTESTING_ASSERT_EQUAL(nullptr, v.Data());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetCapacity());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetCapacityInBytes());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSizeInBytes());
	SEOUL_UNITTESTING_ASSERT(v.IsEmpty());
}

void UnsafeBufferTest::TestEqualityBuiltin()
{
	UnsafeBuffer<UInt32> vA;
	UnsafeBuffer<UInt32> vB;

	vA.PushBack(1);
	vB.PushBack(1);
	vB.PushBack(2);

	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(vA, vB);

	vA.PushBack(2);

	SEOUL_UNITTESTING_ASSERT_EQUAL(vA, vB);

	vA[1] = 3;

	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(vA, vB);
}

void UnsafeBufferTest::TestEqualitySimple()
{
	UnsafeBuffer<ContainerTestSimple> vA;
	UnsafeBuffer<ContainerTestSimple> vB;

	vA.PushBack(ContainerTestSimple::Create(1));
	vB.PushBack(ContainerTestSimple::Create(1));
	vB.PushBack(ContainerTestSimple::Create(2));

	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(vA, vB);

	vA.PushBack(ContainerTestSimple::Create(2));

	SEOUL_UNITTESTING_ASSERT_EQUAL(vA, vB);

	vA[1] = ContainerTestSimple::Create(3);

	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(vA, vB);
}

void UnsafeBufferTest::TestMethods()
{
	UnsafeBuffer<Int> testVec;
	// GetCapacity() should return the initial size of the UnsafeBuffer
	SEOUL_UNITTESTING_ASSERT( testVec.GetCapacity() == 0);
	SEOUL_UNITTESTING_ASSERT( testVec.GetSize() == 0);

	for(Int i=0; i<10; i++)
	{
		testVec.PushBack(i);
	}

	//pop everything off
	UInt currentCap = testVec.GetCapacity();
	for(Int i=0; i<10; i++)
	{
		SEOUL_UNITTESTING_ASSERT( testVec.GetSize() == 10-(UInt)i);
		Int val = testVec.Back();
		testVec.PopBack();
		SEOUL_UNITTESTING_ASSERT( val == 10-i-1);
		// GetCapacity() should return the same value as before the PopBack() calls
		SEOUL_UNITTESTING_ASSERT( testVec.GetCapacity() == currentCap );

	}

	// put 11 on and make sure capacity gets changed
	for(Int i=0; i<11; i++)
	{
		testVec.PushBack(i);
	}
	// GetCapacity() >= GetSize()
	SEOUL_UNITTESTING_ASSERT( testVec.GetCapacity() >= testVec.GetSize() );
	SEOUL_UNITTESTING_ASSERT( testVec.GetSize() == 11);

	//resize to 11
	testVec.ResizeNoInitialize(11);
	// GetCapacity() >= GetSize()
	SEOUL_UNITTESTING_ASSERT( testVec.GetCapacity() >= testVec.GetSize() );
	SEOUL_UNITTESTING_ASSERT( testVec.GetSize() == 11);

	SEOUL_UNITTESTING_ASSERT( testVec[5]==5);
	testVec[5] = 3;
	int j = testVec[9];

	SEOUL_UNITTESTING_ASSERT( j==9);


	// pop everything off (again to make sure)
	for(Int i=0; i<11; i++)
	{
		SEOUL_UNITTESTING_ASSERT( testVec.GetSize() == 11-(UInt)i);
		Int val = testVec.Back();
		testVec.PopBack();
		if( i!=5)
		{
			SEOUL_UNITTESTING_ASSERT( val == 11-i-1);
		}
		else
		{
			SEOUL_UNITTESTING_ASSERT( val == 3);
		}
		// GetCapacity() >= GetSize()
		SEOUL_UNITTESTING_ASSERT( testVec.GetCapacity() >= testVec.GetSize() );

	}

	//make sure you don't crash if I reset to 0
	for(Int i = 0; i< 10; i++)
	{
		testVec.PushBack(i);
	}
	testVec.ResizeNoInitialize(0);

	//set it back again for copy tests
	for(Int i = 0; i< 10; i++)
	{
		testVec.PushBack(i);
	}
	//testing copy constructor
	{
		UnsafeBuffer<Int> otherVec(testVec);
		SEOUL_UNITTESTING_ASSERT( testVec.GetSize() == otherVec.GetSize());
		//the capacity's wont be the same however
		SEOUL_UNITTESTING_ASSERT( testVec.GetSize() == otherVec.GetCapacity());

		otherVec[3]=24;
		for(UInt i = 0; i< testVec.GetSize(); i++)
		{
			if(i!=3)
			{
				SEOUL_UNITTESTING_ASSERT( testVec[i] == otherVec[i]);
			}
			else
			{
				SEOUL_UNITTESTING_ASSERT( otherVec[i] == 24);
				SEOUL_UNITTESTING_ASSERT( testVec[i] == 3);
			}
		}

		UnsafeBuffer<Int, MemoryBudgets::Debug> otherVec2(testVec);
		SEOUL_UNITTESTING_ASSERT( testVec.GetSize() == otherVec2.GetSize());
		//the capacity's wont be the same however
		SEOUL_UNITTESTING_ASSERT( testVec.GetSize() == otherVec2.GetCapacity());

		otherVec2[3]=24;
		for(UInt i = 0; i< testVec.GetSize(); i++)
		{
			if(i!=3)
			{
				SEOUL_UNITTESTING_ASSERT( testVec[i] == otherVec2[i]);
			}
			else
			{
				SEOUL_UNITTESTING_ASSERT( otherVec2[i] == 24);
				SEOUL_UNITTESTING_ASSERT( testVec[i] == 3);
			}
		}
	}

	//testing assignment
	{
		UnsafeBuffer<Int> otherVec;
		otherVec = testVec;
		SEOUL_UNITTESTING_ASSERT( testVec.GetSize() == otherVec.GetSize());
		//the capacity's wont be the same however
		SEOUL_UNITTESTING_ASSERT( testVec.GetSize() == otherVec.GetCapacity());

		otherVec[3]=24;
		for(UInt i = 0; i< testVec.GetSize(); i++)
		{
			if(i!=3)
			{
				SEOUL_UNITTESTING_ASSERT( testVec[i] == otherVec[i]);
			}
			else
			{
				SEOUL_UNITTESTING_ASSERT( otherVec[i] == 24);
				SEOUL_UNITTESTING_ASSERT( testVec[i] == 3);
			}
		}

		UnsafeBuffer<Int, MemoryBudgets::Debug> otherVec2;
		otherVec2 = testVec;
		SEOUL_UNITTESTING_ASSERT( testVec.GetSize() == otherVec2.GetSize());
		//the capacity's wont be the same however
		SEOUL_UNITTESTING_ASSERT( testVec.GetSize() == otherVec2.GetCapacity());

		otherVec2[3]=24;
		for(UInt i = 0; i< testVec.GetSize(); i++)
		{
			if(i!=3)
			{
				SEOUL_UNITTESTING_ASSERT( testVec[i] == otherVec2[i]);
			}
			else
			{
				SEOUL_UNITTESTING_ASSERT( otherVec2[i] == 24);
				SEOUL_UNITTESTING_ASSERT( testVec[i] == 3);
			}
		}
	}
}

/**
 * Tests the Iterator interfaces of the UnsafeBuffer class
 */
void UnsafeBufferTest::TestIterators()
{
	UnsafeBuffer<Int> testVec;

	for(Int i = 0; i < 6; i++)
	{
		testVec.PushBack(i + 10);
	}

	// Test value reads through iterator
	auto iter = testVec.Begin();
	Int i = 0;
	while(iter != testVec.End())
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(i + 10, *iter);
		++iter;
		++i;
	}

	// Test value writes through iterator
	iter = testVec.Begin();
	i = 0;
	while(iter != testVec.End())
	{
		*iter = 3 * i;
		SEOUL_UNITTESTING_ASSERT_EQUAL(3 * i, testVec[i]);
		++iter;
		++i;
	}
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
