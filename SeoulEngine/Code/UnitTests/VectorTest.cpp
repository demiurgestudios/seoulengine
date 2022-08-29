/**
 * \file VectorTest.cpp
 * \brief Unit test code for the Seoul Vector<> class
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
#include "Vector.h"
#include "VectorTest.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(VectorTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestAppendBuiltin)
	SEOUL_METHOD(TestAppendComplex)
	SEOUL_METHOD(TestAppendSimple)
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
	SEOUL_METHOD(TestPushBackMoveBuiltin)
	SEOUL_METHOD(TestPushBackMoveComplex)
	SEOUL_METHOD(TestPushBackMoveSimple)
	SEOUL_METHOD(TestInsertMoveBuiltin)
	SEOUL_METHOD(TestInsertMoveComplex)
	SEOUL_METHOD(TestInsertMoveSimple)
	SEOUL_METHOD(TestAssignRegressBuiltin)
	SEOUL_METHOD(TestAssignRegressComplex)
	SEOUL_METHOD(TestAssignRegressSimple)
	SEOUL_METHOD(TestSelfAssignBuiltin)
	SEOUL_METHOD(TestSelfAssignComplex)
	SEOUL_METHOD(TestSelfAssignSimple)
	SEOUL_METHOD(TestRemoveRegressionBuiltin)
	SEOUL_METHOD(TestRemoveRegressionComplex)
	SEOUL_METHOD(TestRemoveRegressionSimple)
	SEOUL_METHOD(TestSelfAssignRegressionBuiltin)
	SEOUL_METHOD(TestSelfAssignRegressionComplex)
	SEOUL_METHOD(TestSelfAssignRegressionSimple)
	SEOUL_METHOD(TestSelfFillRegressionBuiltin)
	SEOUL_METHOD(TestSelfFillRegressionComplex)
	SEOUL_METHOD(TestSelfFillRegressionSimple)
	SEOUL_METHOD(TestSelfInsertRegressionBuiltin)
	SEOUL_METHOD(TestSelfInsertRegressionComplex)
	SEOUL_METHOD(TestSelfInsertRegressionSimple)
	SEOUL_METHOD(TestRemoveFirstInstanceBuiltin)
	SEOUL_METHOD(TestRemoveFirstInstanceComplex)
	SEOUL_METHOD(TestRemoveFirstInstanceComplexCoerce)
	SEOUL_METHOD(TestRemoveFirstInstanceSimple)
	SEOUL_METHOD(TestRemoveCountBuiltin)
	SEOUL_METHOD(TestRemoveCountComplex)
	SEOUL_METHOD(TestRemoveCountSimple)
SEOUL_END_TYPE()

void VectorTest::TestAppendBuiltin()
{
	// To empty;
	{
		Vector<UInt64, MemoryBudgets::TBDContainer> v;

		Vector<UInt64, MemoryBudgets::TBDContainer> v2;
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
		Vector<UInt64, MemoryBudgets::StateMachine> v;

		Vector<UInt64, MemoryBudgets::OperatorNewArray> v2;
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
		Vector<UInt64, MemoryBudgets::StateMachine> v;

		Vector<UInt64, MemoryBudgets::OperatorNewArray> v2;
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
		Vector<UInt64, MemoryBudgets::TBDContainer> v;
		v.PushBack(UInt64(7));
		v.PushBack(UInt64(91));
		v.PushBack(UInt64(313));

		Vector<UInt64, MemoryBudgets::TBDContainer> v2;
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
		Vector<UInt64, MemoryBudgets::TBDContainer> v;
		v.PushBack(UInt64(7));
		v.PushBack(UInt64(91));
		v.PushBack(UInt64(313));

		Vector<UInt64, MemoryBudgets::Threading> v2;
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
		Vector<UInt64, MemoryBudgets::TBDContainer> v;
		v.PushBack(UInt64(7));
		v.PushBack(UInt64(91));
		v.PushBack(UInt64(313));

		Vector<UInt64, MemoryBudgets::Threading> v2;
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

void VectorTest::TestAppendComplex()
{
	// To empty;
	{
		Vector<ContainerTestComplex, MemoryBudgets::TBDContainer> v;

		Vector<ContainerTestComplex, MemoryBudgets::TBDContainer> v2;
		v2.PushBack(ContainerTestComplex(12));
		v2.PushBack(ContainerTestComplex(3209));
		v2.PushBack(ContainerTestComplex(3090));

		SEOUL_UNITTESTING_ASSERT_EQUAL(3, ContainerTestComplex::s_iCount);
		v.Append(v2);
		SEOUL_UNITTESTING_ASSERT_EQUAL(6, ContainerTestComplex::s_iCount);

		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[0], 12);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[1], 3209);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[2], 3090);
	}
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);

	// To empty, different type.
	{
		Vector<ContainerTestComplex, MemoryBudgets::StateMachine> v;

		Vector<ContainerTestComplex, MemoryBudgets::OperatorNewArray> v2;
		v2.PushBack(ContainerTestComplex(12));
		v2.PushBack(ContainerTestComplex(3209));
		v2.PushBack(ContainerTestComplex(3090));

		SEOUL_UNITTESTING_ASSERT_EQUAL(3, ContainerTestComplex::s_iCount);
		v.Append(v2);
		SEOUL_UNITTESTING_ASSERT_EQUAL(6, ContainerTestComplex::s_iCount);

		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[0], 12);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[1], 3209);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[2], 3090);
	}
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);

	// To empty, iterators.
	{
		Vector<ContainerTestComplex, MemoryBudgets::StateMachine> v;

		Vector<ContainerTestComplex, MemoryBudgets::OperatorNewArray> v2;
		v2.PushBack(ContainerTestComplex(12));
		v2.PushBack(ContainerTestComplex(3209));
		v2.PushBack(ContainerTestComplex(3090));

		SEOUL_UNITTESTING_ASSERT_EQUAL(3, ContainerTestComplex::s_iCount);
		v.Append(v2.Begin() + 1, v2.End());
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, ContainerTestComplex::s_iCount);

		SEOUL_UNITTESTING_ASSERT_EQUAL(2, v.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(2, v.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[0], 3209);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[1], 3090);
	}
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);

	// To partial;
	{
		Vector<ContainerTestComplex, MemoryBudgets::TBDContainer> v;
		v.PushBack(ContainerTestComplex(7));
		v.PushBack(ContainerTestComplex(91));
		v.PushBack(ContainerTestComplex(313));

		Vector<ContainerTestComplex, MemoryBudgets::TBDContainer> v2;
		v2.PushBack(ContainerTestComplex(11));
		v2.PushBack(ContainerTestComplex(323));
		v2.PushBack(ContainerTestComplex(112));

		SEOUL_UNITTESTING_ASSERT_EQUAL(6, ContainerTestComplex::s_iCount);
		v.Append(v2);
		SEOUL_UNITTESTING_ASSERT_EQUAL(9, ContainerTestComplex::s_iCount);

		SEOUL_UNITTESTING_ASSERT_EQUAL(6, v.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(6, v.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[0], 7);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[1], 91);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[2], 313);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[3], 11);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[4], 323);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[5], 112);
	}
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);

	// To partial, different type.
	{
		Vector<ContainerTestComplex, MemoryBudgets::TBDContainer> v;
		v.PushBack(ContainerTestComplex(7));
		v.PushBack(ContainerTestComplex(91));
		v.PushBack(ContainerTestComplex(313));

		Vector<ContainerTestComplex, MemoryBudgets::Threading> v2;
		v2.PushBack(ContainerTestComplex(11));
		v2.PushBack(ContainerTestComplex(323));
		v2.PushBack(ContainerTestComplex(112));

		SEOUL_UNITTESTING_ASSERT_EQUAL(6, ContainerTestComplex::s_iCount);
		v.Append(v2);
		SEOUL_UNITTESTING_ASSERT_EQUAL(9, ContainerTestComplex::s_iCount);

		SEOUL_UNITTESTING_ASSERT_EQUAL(6, v.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(6, v.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[0], 7);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[1], 91);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[2], 313);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[3], 11);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[4], 323);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[5], 112);
	}
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);

	// To partial, iterators.
	{
		Vector<ContainerTestComplex, MemoryBudgets::TBDContainer> v;
		v.PushBack(ContainerTestComplex(7));
		v.PushBack(ContainerTestComplex(91));
		v.PushBack(ContainerTestComplex(313));

		Vector<ContainerTestComplex, MemoryBudgets::Threading> v2;
		v2.PushBack(ContainerTestComplex(11));
		v2.PushBack(ContainerTestComplex(323));
		v2.PushBack(ContainerTestComplex(112));

		SEOUL_UNITTESTING_ASSERT_EQUAL(6, ContainerTestComplex::s_iCount);
		v.Append(v2.Begin() + 1, v2.End());
		SEOUL_UNITTESTING_ASSERT_EQUAL(8, ContainerTestComplex::s_iCount);

		SEOUL_UNITTESTING_ASSERT_EQUAL(5, v.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, v.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[0], 7);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[1], 91);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[2], 313);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[3], 323);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v[4], 112);
	}
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
}

void VectorTest::TestAppendSimple()
{
	// To empty;
	{
		Vector<ContainerTestSimple, MemoryBudgets::TBDContainer> v;

		Vector<ContainerTestSimple, MemoryBudgets::TBDContainer> v2;
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
		Vector<ContainerTestSimple, MemoryBudgets::StateMachine> v;

		Vector<ContainerTestSimple, MemoryBudgets::OperatorNewArray> v2;
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
		Vector<ContainerTestSimple, MemoryBudgets::StateMachine> v;

		Vector<ContainerTestSimple, MemoryBudgets::OperatorNewArray> v2;
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
		Vector<ContainerTestSimple, MemoryBudgets::TBDContainer> v;
		ContainerTestSimple simple;
		simple.m_B = 33;

		simple.m_iA = 7;
		v.PushBack(simple);
		simple.m_iA = 91;
		v.PushBack(simple);
		simple.m_iA = 313;
		v.PushBack(simple);

		Vector<ContainerTestSimple, MemoryBudgets::TBDContainer> v2;
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
		Vector<ContainerTestSimple, MemoryBudgets::TBDContainer> v;
		ContainerTestSimple simple;
		simple.m_B = 33;

		simple.m_iA = 7;
		v.PushBack(simple);
		simple.m_iA = 91;
		v.PushBack(simple);
		simple.m_iA = 313;
		v.PushBack(simple);

		Vector<ContainerTestSimple, MemoryBudgets::Threading> v2;
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
		Vector<ContainerTestSimple, MemoryBudgets::TBDContainer> v;
		ContainerTestSimple simple;
		simple.m_B = 33;

		simple.m_iA = 7;
		v.PushBack(simple);
		simple.m_iA = 91;
		v.PushBack(simple);
		simple.m_iA = 313;
		v.PushBack(simple);

		Vector<ContainerTestSimple, MemoryBudgets::Threading> v2;
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

void VectorTest::TestAssignBuiltin()
{
	// Copy self
	{
		Vector<UInt16, MemoryBudgets::DataStore> v1;
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
		Vector<UInt16, MemoryBudgets::DataStore> v1;
		v1.PushBack(UInt16(7));
		v1.PushBack(UInt16(11));
		v1.PushBack(UInt16(25));

		Vector<UInt16, MemoryBudgets::DataStore> v2 = v1;
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
		Vector<UInt16, MemoryBudgets::Falcon> v1;
		v1.PushBack(UInt16(7));
		v1.PushBack(UInt16(11));
		v1.PushBack(UInt16(25));

		Vector<UInt16, MemoryBudgets::Physics> v2;
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
		Vector<UInt16, MemoryBudgets::Falcon> v1;
		v1.PushBack(UInt16(7));
		v1.PushBack(UInt16(11));
		v1.PushBack(UInt16(25));

		Vector<UInt16, MemoryBudgets::Physics> v2;
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
		Vector<UInt16, MemoryBudgets::Falcon> v1;
		v1.PushBack(UInt16(3));
		v1.PushBack(UInt16(7));
		v1.PushBack(UInt16(11));
		v1.PushBack(UInt16(25));
		v1.PushBack(UInt16(91));

		Vector<UInt16, MemoryBudgets::Physics> v2;
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

	// Size with default value.
	{
		Vector<UInt16, MemoryBudgets::Falcon> v;
		v.PushBack(UInt16(908));
		v.PushBack(UInt16(124));
		v.PushBack(UInt16(457));

		v.Assign(5);
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, v.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(5 * sizeof(UInt16), v.GetCapacityInBytes());
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, v.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(5 * sizeof(UInt16), v.GetSizeInBytes());
		SEOUL_UNITTESTING_ASSERT(!v.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(UInt16(), v.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(UInt16(), v.Front());
		for (UInt32 i = 0u; i < 3; ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(UInt16(), v.At(i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(UInt16(), *(v.Begin() + i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(UInt16(), v.Data()[i]);
			SEOUL_UNITTESTING_ASSERT_EQUAL(UInt16(), *(v.End() - 1 - i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(UInt16(), *v.Get(i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(UInt16(), v[i]);
		}
	}

	// Size with value.
	{
		Vector<UInt16, MemoryBudgets::Falcon> v;
		v.PushBack(UInt16(3409));
		v.PushBack(UInt16(144));
		v.PushBack(UInt16(389));

		v.Assign(5, UInt16(77));
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, v.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(5 * sizeof(UInt16), v.GetCapacityInBytes());
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, v.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(5 * sizeof(UInt16), v.GetSizeInBytes());
		SEOUL_UNITTESTING_ASSERT(!v.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(UInt16(77), v.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(UInt16(77), v.Front());
		for (UInt32 i = 0u; i < 3; ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(UInt16(77), v.At(i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(UInt16(77), *(v.Begin() + i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(UInt16(77), v.Data()[i]);
			SEOUL_UNITTESTING_ASSERT_EQUAL(UInt16(77), *(v.End() - 1 - i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(UInt16(77), *v.Get(i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(UInt16(77), v[i]);
		}
	}
}

void VectorTest::TestAssignComplex()
{
	// Copy self
	{
		Vector<ContainerTestComplex, MemoryBudgets::DataStore> v1;
		v1.PushBack(ContainerTestComplex(7));
		v1.PushBack(ContainerTestComplex(11));
		v1.PushBack(ContainerTestComplex(25));

		SEOUL_UNITTESTING_ASSERT_EQUAL(3, ContainerTestComplex::s_iCount);
		v1 = v1;
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, ContainerTestComplex::s_iCount);

		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v1.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3 * sizeof(ContainerTestComplex), v1.GetCapacityInBytes());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v1.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3 * sizeof(ContainerTestComplex), v1.GetSizeInBytes());
		SEOUL_UNITTESTING_ASSERT(!v1.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(25), v1.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(7), v1.Front());
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(11), v1.At(1));
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(11), *(v1.Begin() + 1));
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(11), v1.Data()[1]);
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(11), *(v1.End() - 1 - 1));
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(11), *v1.Get(1));
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(11), v1[1]);
	}
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);

	// Copy
	{
		Vector<ContainerTestComplex, MemoryBudgets::DataStore> v1;
		v1.PushBack(ContainerTestComplex(7));
		v1.PushBack(ContainerTestComplex(11));
		v1.PushBack(ContainerTestComplex(25));

		Vector<ContainerTestComplex, MemoryBudgets::DataStore> v2 = v1;
		SEOUL_UNITTESTING_ASSERT_EQUAL(6, ContainerTestComplex::s_iCount);
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v2.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3 * sizeof(ContainerTestComplex), v2.GetCapacityInBytes());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v2.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3 * sizeof(ContainerTestComplex), v2.GetSizeInBytes());
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
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);

	// Copy templated
	{
		Vector<ContainerTestComplex, MemoryBudgets::Falcon> v1;
		v1.PushBack(ContainerTestComplex(7));
		v1.PushBack(ContainerTestComplex(11));
		v1.PushBack(ContainerTestComplex(25));

		Vector<ContainerTestComplex, MemoryBudgets::Physics> v2;
		v2.PushBack(ContainerTestComplex(112));
		v2.PushBack(ContainerTestComplex(12));

		v2 = v1;
		SEOUL_UNITTESTING_ASSERT_EQUAL(6, ContainerTestComplex::s_iCount);
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v2.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3 * sizeof(ContainerTestComplex), v2.GetCapacityInBytes());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v2.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3 * sizeof(ContainerTestComplex), v2.GetSizeInBytes());
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
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);

	// Iterator full
	{
		Vector<ContainerTestComplex, MemoryBudgets::Falcon> v1;
		v1.PushBack(ContainerTestComplex(7));
		v1.PushBack(ContainerTestComplex(11));
		v1.PushBack(ContainerTestComplex(25));

		Vector<ContainerTestComplex, MemoryBudgets::Physics> v2;
		v2.PushBack(ContainerTestComplex(191));
		v2.PushBack(ContainerTestComplex(3981));
		v2.PushBack(ContainerTestComplex(1298));
		v2.PushBack(ContainerTestComplex(787));
		v2.PushBack(ContainerTestComplex(12));

		v2.Assign(v1.Begin(), v1.End());
		SEOUL_UNITTESTING_ASSERT_EQUAL(6, ContainerTestComplex::s_iCount);
		SEOUL_UNITTESTING_ASSERT_EQUAL(6, v2.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(6 * sizeof(ContainerTestComplex), v2.GetCapacityInBytes());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v2.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3 * sizeof(ContainerTestComplex), v2.GetSizeInBytes());
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
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);

	// Iterator partial
	{
		Vector<ContainerTestComplex, MemoryBudgets::Falcon> v1;
		v1.PushBack(ContainerTestComplex(3));
		v1.PushBack(ContainerTestComplex(7));
		v1.PushBack(ContainerTestComplex(11));
		v1.PushBack(ContainerTestComplex(25));
		v1.PushBack(ContainerTestComplex(91));

		Vector<ContainerTestComplex, MemoryBudgets::Physics> v2;
		v2.PushBack(ContainerTestComplex(191));
		v2.PushBack(ContainerTestComplex(3981));
		v2.PushBack(ContainerTestComplex(1298));

		v2.Assign(v1.Begin() + 1, v1.End() - 1);
		SEOUL_UNITTESTING_ASSERT_EQUAL(8, ContainerTestComplex::s_iCount);
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v2.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3 * sizeof(ContainerTestComplex), v2.GetCapacityInBytes());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v2.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3 * sizeof(ContainerTestComplex), v2.GetSizeInBytes());
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
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);

	// Size with default value.
	{
		Vector<ContainerTestComplex, MemoryBudgets::Falcon> v;
		v.PushBack(ContainerTestComplex(908));
		v.PushBack(ContainerTestComplex(124));
		v.PushBack(ContainerTestComplex(457));

		v.Assign(5);
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, ContainerTestComplex::s_iCount);
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, v.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(5 * sizeof(ContainerTestComplex), v.GetCapacityInBytes());
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, v.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(5 * sizeof(ContainerTestComplex), v.GetSizeInBytes());
		SEOUL_UNITTESTING_ASSERT(!v.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(), v.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(), v.Front());
		for (UInt32 i = 0u; i < 3; ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(), v.At(i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(), *(v.Begin() + i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(), v.Data()[i]);
			SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(), *(v.End() - 1 - i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(), *v.Get(i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(), v[i]);
		}
	}

	// Size with value.
	{
		Vector<ContainerTestComplex, MemoryBudgets::Falcon> v;
		v.PushBack(ContainerTestComplex(3409));
		v.PushBack(ContainerTestComplex(144));
		v.PushBack(ContainerTestComplex(389));

		v.Assign(5, ContainerTestComplex(77));
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, ContainerTestComplex::s_iCount);
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, v.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(5 * sizeof(ContainerTestComplex), v.GetCapacityInBytes());
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, v.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(5 * sizeof(ContainerTestComplex), v.GetSizeInBytes());
		SEOUL_UNITTESTING_ASSERT(!v.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(77), v.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(77), v.Front());
		for (UInt32 i = 0u; i < 3; ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(77), v.At(i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(77), *(v.Begin() + i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(77), v.Data()[i]);
			SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(77), *(v.End() - 1 - i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(77), *v.Get(i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(77), v[i]);
		}
	}
}

void VectorTest::TestAssignSimple()
{
	// Copy self
	{
		Vector<ContainerTestSimple, MemoryBudgets::DataStore> v1;
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
		Vector<ContainerTestSimple, MemoryBudgets::DataStore> v1;
		v1.PushBack(simple);
		simple.m_iA = 11;
		v1.PushBack(simple);
		simple.m_iA = 25;
		v1.PushBack(simple);

		Vector<ContainerTestSimple, MemoryBudgets::DataStore> v2 = v1;
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
		Vector<ContainerTestSimple, MemoryBudgets::DataStore> v1;
		v1.PushBack(simple);
		simple.m_iA = 11;
		v1.PushBack(simple);
		simple.m_iA = 25;
		v1.PushBack(simple);

		Vector<ContainerTestSimple, MemoryBudgets::Physics> v2;
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
		Vector<ContainerTestSimple, MemoryBudgets::DataStore> v1;
		v1.PushBack(simple);
		simple.m_iA = 11;
		v1.PushBack(simple);
		simple.m_iA = 25;
		v1.PushBack(simple);

		Vector<ContainerTestSimple, MemoryBudgets::Physics> v2;
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
		Vector<ContainerTestSimple, MemoryBudgets::Falcon> v1;
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

		Vector<ContainerTestSimple, MemoryBudgets::Physics> v2;
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
		Vector<ContainerTestSimple, MemoryBudgets::Falcon> v;
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
		Vector<ContainerTestSimple, MemoryBudgets::Falcon> v;
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

void VectorTest::TestBasic()
{
	Vector<Int> testVec(15);
	// GetCapacity() should return the initial size of the Vector
	SEOUL_UNITTESTING_ASSERT( testVec.GetCapacity() == 15);
	SEOUL_UNITTESTING_ASSERT( testVec.GetSize() == 15);

	for(Int i=0; i<10; i++)
	{
		testVec.PushBack(i);
	}
	SEOUL_UNITTESTING_ASSERT( testVec.GetSize() == 25);
}

void VectorTest::TestClearBuiltin()
{
	Vector<UInt16, MemoryBudgets::Audio> v;
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
		Vector<UInt16, MemoryBudgets::Audio> v2;
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

void VectorTest::TestClearComplex()
{
	Vector<ContainerTestComplex, MemoryBudgets::Audio> v;
	v.PushBack(ContainerTestComplex(23));
	v.PushBack(ContainerTestComplex(194));
	v.PushBack(ContainerTestComplex(119));

	SEOUL_UNITTESTING_ASSERT_EQUAL(3, ContainerTestComplex::s_iCount);

	// Clear should destroy elements but leave capacity.
	v.Clear();
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, v.GetCapacity());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSize());
	SEOUL_UNITTESTING_ASSERT(v.IsEmpty());

	v.PushBack(ContainerTestComplex(7));
	v.PushBack(ContainerTestComplex(1123));
	v.PushBack(ContainerTestComplex(434));
	v.PushBack(ContainerTestComplex(342));
	v.PushBack(ContainerTestComplex(23989));

	SEOUL_UNITTESTING_ASSERT_EQUAL(5, ContainerTestComplex::s_iCount);
	SEOUL_UNITTESTING_ASSERT_EQUAL(6, v.GetCapacity());
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, v.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(v[0], 7);
	SEOUL_UNITTESTING_ASSERT_EQUAL(v[1], 1123);
	SEOUL_UNITTESTING_ASSERT_EQUAL(v[2], 434);
	SEOUL_UNITTESTING_ASSERT_EQUAL(v[3], 342);
	SEOUL_UNITTESTING_ASSERT_EQUAL(v[4], 23989);

	// Now shrink - this should get us a capacity of 5.
	v.ShrinkToFit();
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, ContainerTestComplex::s_iCount);
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, v.GetCapacity());
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, v.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(v[0], 7);
	SEOUL_UNITTESTING_ASSERT_EQUAL(v[1], 1123);
	SEOUL_UNITTESTING_ASSERT_EQUAL(v[2], 434);
	SEOUL_UNITTESTING_ASSERT_EQUAL(v[3], 342);
	SEOUL_UNITTESTING_ASSERT_EQUAL(v[4], 23989);

	// Clear again.
	v.Clear();
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, v.GetCapacity());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSize());
	SEOUL_UNITTESTING_ASSERT(v.IsEmpty());

	// Now shrink - this should completely free the memory.
	v.ShrinkToFit();
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetCapacity());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSize());
	SEOUL_UNITTESTING_ASSERT(v.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(nullptr, v.Data());

	v.PushBack(ContainerTestComplex(3));
	v.PushBack(ContainerTestComplex(124));
	v.PushBack(ContainerTestComplex(342));
	v.PushBack(ContainerTestComplex(12));
	v.PushBack(ContainerTestComplex(33));
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, ContainerTestComplex::s_iCount);
	SEOUL_UNITTESTING_ASSERT_EQUAL(6, v.GetCapacity());
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, v.GetSize());

	// Now do the "swap trick". This should completely free the memory.
	{
		Vector<ContainerTestComplex, MemoryBudgets::Audio> v2;
		v2.Swap(v);

		// v is now empty
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, ContainerTestComplex::s_iCount);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSize());
		SEOUL_UNITTESTING_ASSERT(v.IsEmpty());
		SEOUL_UNITTESTING_ASSERT_EQUAL(nullptr, v.Data());

		// v2 has v's state.
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, ContainerTestComplex::s_iCount);
		SEOUL_UNITTESTING_ASSERT_EQUAL(6, v2.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, v2.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(v2[0], 3);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v2[1], 124);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v2[2], 342);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v2[3], 12);
		SEOUL_UNITTESTING_ASSERT_EQUAL(v2[4], 33);
	}

	// All gone.
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
}

void VectorTest::TestClearSimple()
{
	Vector<ContainerTestSimple, MemoryBudgets::Audio> v;
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
		Vector<ContainerTestSimple, MemoryBudgets::Audio> v2;
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

void VectorTest::TestConstructorBuiltin()
{
	// Default.
	{
		Vector<Int64, MemoryBudgets::DataStore> v;
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetCapacityInBytes());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSizeInBytes());
		SEOUL_UNITTESTING_ASSERT(v.IsEmpty());
	}

	// Copy
	{
		Vector<Int64, MemoryBudgets::DataStore> v1;
		v1.PushBack(Int64(7));
		v1.PushBack(Int64(11));
		v1.PushBack(Int64(25));

		Vector<Int64, MemoryBudgets::DataStore> v2(v1);
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
		Vector<Int64, MemoryBudgets::Falcon> v1;
		v1.PushBack(Int64(7));
		v1.PushBack(Int64(11));
		v1.PushBack(Int64(25));

		Vector<Int64, MemoryBudgets::Physics> v2(v1);
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

	// Initializer list
	{
		Vector<Int64, MemoryBudgets::Falcon> v1;
		v1.PushBack(Int64(7));
		v1.PushBack(Int64(11));
		v1.PushBack(Int64(25));

		Vector<Int64, MemoryBudgets::Physics> v2({ 7, 11, 25 });
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
		Vector<Int64, MemoryBudgets::Falcon> v1;
		v1.PushBack(Int64(7));
		v1.PushBack(Int64(11));
		v1.PushBack(Int64(25));

		Vector<Int64, MemoryBudgets::Physics> v2(v1.Begin(), v1.End());
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
		Vector<Int64, MemoryBudgets::Falcon> v1;
		v1.PushBack(Int64(3));
		v1.PushBack(Int64(7));
		v1.PushBack(Int64(11));
		v1.PushBack(Int64(25));
		v1.PushBack(Int64(91));

		Vector<Int64, MemoryBudgets::Physics> v2(v1.Begin() + 1, v1.End() - 1);
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

	// Size with default value.
	{
		Vector<Int64, MemoryBudgets::Falcon> v(5);
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, v.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(5 * sizeof(Int64), v.GetCapacityInBytes());
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, v.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(5 * sizeof(Int64), v.GetSizeInBytes());
		SEOUL_UNITTESTING_ASSERT(!v.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(Int64(), v.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(Int64(), v.Front());
		for (UInt32 i = 0u; i < 3; ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(Int64(), v.At(i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(Int64(), *(v.Begin() + i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(Int64(), v.Data()[i]);
			SEOUL_UNITTESTING_ASSERT_EQUAL(Int64(), *(v.End() - 1 - i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(Int64(), *v.Get(i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(Int64(), v[i]);
		}
	}

	// Size with value.
	{
		Vector<Int64, MemoryBudgets::Falcon> v(5, Int64(77));
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, v.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(5 * sizeof(Int64), v.GetCapacityInBytes());
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, v.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(5 * sizeof(Int64), v.GetSizeInBytes());
		SEOUL_UNITTESTING_ASSERT(!v.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(Int64(77), v.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(Int64(77), v.Front());
		for (UInt32 i = 0u; i < 3; ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(Int64(77), v.At(i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(Int64(77), *(v.Begin() + i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(Int64(77), v.Data()[i]);
			SEOUL_UNITTESTING_ASSERT_EQUAL(Int64(77), *(v.End() - 1 - i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(Int64(77), *v.Get(i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(Int64(77), v[i]);
		}
	}
}

void VectorTest::TestConstructorComplex()
{
	// Default.
	{
		Vector<ContainerTestComplex, MemoryBudgets::DataStore> v;
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetCapacityInBytes());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSizeInBytes());
		SEOUL_UNITTESTING_ASSERT(v.IsEmpty());
	}

	// Copy
	{
		Vector<ContainerTestComplex, MemoryBudgets::DataStore> v1;
		v1.PushBack(ContainerTestComplex(7));
		v1.PushBack(ContainerTestComplex(11));
		v1.PushBack(ContainerTestComplex(25));

		Vector<ContainerTestComplex, MemoryBudgets::DataStore> v2(v1);
		SEOUL_UNITTESTING_ASSERT_EQUAL(6, ContainerTestComplex::s_iCount);
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v2.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3 * sizeof(ContainerTestComplex), v2.GetCapacityInBytes());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v2.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3 * sizeof(ContainerTestComplex), v2.GetSizeInBytes());
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
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);

	// Copy templated
	{
		Vector<ContainerTestComplex, MemoryBudgets::Falcon> v1;
		v1.PushBack(ContainerTestComplex(7));
		v1.PushBack(ContainerTestComplex(11));
		v1.PushBack(ContainerTestComplex(25));

		Vector<ContainerTestComplex, MemoryBudgets::Physics> v2(v1);
		SEOUL_UNITTESTING_ASSERT_EQUAL(6, ContainerTestComplex::s_iCount);
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v2.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3 * sizeof(ContainerTestComplex), v2.GetCapacityInBytes());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v2.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3 * sizeof(ContainerTestComplex), v2.GetSizeInBytes());
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
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);

	// Initializer list
	{
		Vector<ContainerTestComplex, MemoryBudgets::Falcon> v1;
		v1.PushBack(ContainerTestComplex(7));
		v1.PushBack(ContainerTestComplex(11));
		v1.PushBack(ContainerTestComplex(25));

		Vector<ContainerTestComplex, MemoryBudgets::Physics> v2({ ContainerTestComplex(7), ContainerTestComplex(11), ContainerTestComplex(25) });
		SEOUL_UNITTESTING_ASSERT_EQUAL(6, ContainerTestComplex::s_iCount);
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v2.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3 * sizeof(ContainerTestComplex), v2.GetCapacityInBytes());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v2.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3 * sizeof(ContainerTestComplex), v2.GetSizeInBytes());
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
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);

	// Iterator full
	{
		Vector<ContainerTestComplex, MemoryBudgets::Falcon> v1;
		v1.PushBack(ContainerTestComplex(7));
		v1.PushBack(ContainerTestComplex(11));
		v1.PushBack(ContainerTestComplex(25));

		Vector<ContainerTestComplex, MemoryBudgets::Physics> v2(v1.Begin(), v1.End());
		SEOUL_UNITTESTING_ASSERT_EQUAL(6, ContainerTestComplex::s_iCount);
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v2.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3 * sizeof(ContainerTestComplex), v2.GetCapacityInBytes());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v2.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3 * sizeof(ContainerTestComplex), v2.GetSizeInBytes());
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
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);

	// Iterator partial
	{
		Vector<ContainerTestComplex, MemoryBudgets::Falcon> v1;
		v1.PushBack(ContainerTestComplex(3));
		v1.PushBack(ContainerTestComplex(7));
		v1.PushBack(ContainerTestComplex(11));
		v1.PushBack(ContainerTestComplex(25));
		v1.PushBack(ContainerTestComplex(91));

		Vector<ContainerTestComplex, MemoryBudgets::Physics> v2(v1.Begin() + 1, v1.End() - 1);
		SEOUL_UNITTESTING_ASSERT_EQUAL(8, ContainerTestComplex::s_iCount);
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v2.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3 * sizeof(ContainerTestComplex), v2.GetCapacityInBytes());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v2.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3 * sizeof(ContainerTestComplex), v2.GetSizeInBytes());
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
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);

	// Size with default value.
	{
		Vector<ContainerTestComplex, MemoryBudgets::Falcon> v(5);
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, ContainerTestComplex::s_iCount);
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, v.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(5 * sizeof(ContainerTestComplex), v.GetCapacityInBytes());
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, v.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(5 * sizeof(ContainerTestComplex), v.GetSizeInBytes());
		SEOUL_UNITTESTING_ASSERT(!v.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(), v.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(), v.Front());
		for (UInt32 i = 0u; i < 3; ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(), v.At(i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(), *(v.Begin() + i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(), v.Data()[i]);
			SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(), *(v.End() - 1 - i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(), *v.Get(i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(), v[i]);
		}
	}

	// Size with value.
	{
		Vector<ContainerTestComplex, MemoryBudgets::Falcon> v(5, ContainerTestComplex(77));
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, ContainerTestComplex::s_iCount);
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, v.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(5 * sizeof(ContainerTestComplex), v.GetCapacityInBytes());
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, v.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(5 * sizeof(ContainerTestComplex), v.GetSizeInBytes());
		SEOUL_UNITTESTING_ASSERT(!v.IsEmpty());

		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(77), v.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(77), v.Front());
		for (UInt32 i = 0u; i < 3; ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(77), v.At(i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(77), *(v.Begin() + i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(77), v.Data()[i]);
			SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(77), *(v.End() - 1 - i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(77), *v.Get(i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(77), v[i]);
		}
	}
}

void VectorTest::TestConstructorSimple()
{
	// Default.
	{
		Vector<ContainerTestSimple, MemoryBudgets::DataStore> v;
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
		Vector<ContainerTestSimple, MemoryBudgets::DataStore> v1;
		v1.PushBack(simple);
		simple.m_iA = 11;
		v1.PushBack(simple);
		simple.m_iA = 25;
		v1.PushBack(simple);

		Vector<ContainerTestSimple, MemoryBudgets::DataStore> v2(v1);
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
		Vector<ContainerTestSimple, MemoryBudgets::Falcon> v1;
		v1.PushBack(simple);
		simple.m_iA = 11;
		v1.PushBack(simple);
		simple.m_iA = 25;
		v1.PushBack(simple);

		Vector<ContainerTestSimple, MemoryBudgets::Physics> v2(v1);
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

	// Initializer list
	{
		ContainerTestSimple simple;
		simple.m_B = 33;
		simple.m_iA = 7;
		Vector<ContainerTestSimple, MemoryBudgets::Falcon> v1;
		v1.PushBack(simple);
		simple.m_iA = 11;
		v1.PushBack(simple);
		simple.m_iA = 25;
		v1.PushBack(simple);

		Vector<ContainerTestSimple, MemoryBudgets::Physics> v2({ v1[0], v1[1], v1[2] });
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
		Vector<ContainerTestSimple, MemoryBudgets::Falcon> v1;
		v1.PushBack(simple);
		simple.m_iA = 11;
		v1.PushBack(simple);
		simple.m_iA = 25;
		v1.PushBack(simple);

		Vector<ContainerTestSimple, MemoryBudgets::Physics> v2(v1.Begin(), v1.End());
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
		Vector<ContainerTestSimple, MemoryBudgets::Falcon> v1;
		v1.PushBack(simple);
		simple.m_iA = 7;
		v1.PushBack(simple);
		simple.m_iA = 11;
		v1.PushBack(simple);
		simple.m_iA = 25;
		v1.PushBack(simple);
		simple.m_iA = 91;
		v1.PushBack(simple);

		Vector<ContainerTestSimple, MemoryBudgets::Physics> v2(v1.Begin() + 1, v1.End() - 1);
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
		Vector<ContainerTestSimple, MemoryBudgets::Falcon> v(5);
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
		simple.m_iA = 77;
		Vector<ContainerTestSimple, MemoryBudgets::Falcon> v(5, simple);
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

void VectorTest::TestEmptyBuiltin()
{
	Vector<Int16, MemoryBudgets::DataStore> v;
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetCapacity());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetCapacityInBytes());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSizeInBytes());
	SEOUL_UNITTESTING_ASSERT(v.IsEmpty());

	SEOUL_UNITTESTING_ASSERT_EQUAL(v.Begin(), v.End());
	SEOUL_UNITTESTING_ASSERT(!v.Contains(5));
	SEOUL_UNITTESTING_ASSERT(!v.ContainsFromBack(7));
	SEOUL_UNITTESTING_ASSERT_EQUAL(nullptr, v.Data());
	SEOUL_UNITTESTING_ASSERT_EQUAL(v.End(), v.Begin());
	SEOUL_UNITTESTING_ASSERT_EQUAL(v.Begin(), v.Erase(v.Begin(), v.Begin()));

	v.Fill(23);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetCapacity());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetCapacityInBytes());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSizeInBytes());
	SEOUL_UNITTESTING_ASSERT(v.IsEmpty());

	SEOUL_UNITTESTING_ASSERT_EQUAL(v.End(), v.Find(37));
	SEOUL_UNITTESTING_ASSERT_EQUAL(v.End(), v.FindFromBack(37));

	v.Resize(0);
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
		Vector<Int16, MemoryBudgets::DataStore> v2;
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

void VectorTest::TestEmptyComplex()
{
	Vector<ContainerTestComplex, MemoryBudgets::DataStore> v;
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetCapacity());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetCapacityInBytes());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSizeInBytes());
	SEOUL_UNITTESTING_ASSERT(v.IsEmpty());

	SEOUL_UNITTESTING_ASSERT_EQUAL(v.Begin(), v.End());
	SEOUL_UNITTESTING_ASSERT(!v.Contains(ContainerTestComplex(5)));
	SEOUL_UNITTESTING_ASSERT(!v.ContainsFromBack(ContainerTestComplex(7)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(nullptr, v.Data());
	SEOUL_UNITTESTING_ASSERT_EQUAL(v.End(), v.Begin());
	SEOUL_UNITTESTING_ASSERT_EQUAL(v.Begin(), v.Erase(v.Begin(), v.Begin()));

	v.Fill(ContainerTestComplex(23));
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetCapacity());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetCapacityInBytes());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSizeInBytes());
	SEOUL_UNITTESTING_ASSERT(v.IsEmpty());

	SEOUL_UNITTESTING_ASSERT_EQUAL(v.End(), v.Find(ContainerTestComplex(37)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(v.End(), v.FindFromBack(ContainerTestComplex(37)));

	v.Resize(0);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetCapacity());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetCapacityInBytes());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSizeInBytes());
	SEOUL_UNITTESTING_ASSERT(v.IsEmpty());

	v.ShrinkToFit();
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetCapacity());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetCapacityInBytes());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSizeInBytes());
	SEOUL_UNITTESTING_ASSERT(v.IsEmpty());

	{
		Vector<ContainerTestComplex, MemoryBudgets::DataStore> v2;
		v.Swap(v2);
	}

	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetCapacity());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetCapacityInBytes());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSizeInBytes());
	SEOUL_UNITTESTING_ASSERT(v.IsEmpty());

	v.PushBack(ContainerTestComplex(53));
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(nullptr, v.Data());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, ContainerTestComplex::s_iCount);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, v.GetCapacity());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1 * sizeof(ContainerTestComplex), v.GetCapacityInBytes());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, v.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1 * sizeof(ContainerTestComplex), v.GetSizeInBytes());
	SEOUL_UNITTESTING_ASSERT(!v.IsEmpty());

	SEOUL_UNITTESTING_ASSERT_EQUAL((size_t)1, (size_t)(v.End() - v.Begin()));

	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(53), v.At(0));
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(53), v.Back());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(53), *v.Begin());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(53), v.Data()[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(53), *(v.End() - 1));
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(53), v.Front());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(53), *v.Get(0));
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(53), v[0]);

	v.ShrinkToFit();
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(nullptr, v.Data());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, ContainerTestComplex::s_iCount);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, v.GetCapacity());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1 * sizeof(ContainerTestComplex), v.GetCapacityInBytes());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, v.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1 * sizeof(ContainerTestComplex), v.GetSizeInBytes());
	SEOUL_UNITTESTING_ASSERT(!v.IsEmpty());

	auto i = v.Begin();
	v.PopBack();
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(nullptr, v.Data());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, v.GetCapacity());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1 * sizeof(ContainerTestComplex), v.GetCapacityInBytes());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSizeInBytes());
	SEOUL_UNITTESTING_ASSERT(v.IsEmpty());

	// Iterator should not have been invalidated by the PopBack.
	SEOUL_UNITTESTING_ASSERT_EQUAL(v.Begin(), i);
	SEOUL_UNITTESTING_ASSERT_EQUAL(v.End(), i);

	// Now Shrink - should give us a nullptr before again.
	v.ShrinkToFit();
	SEOUL_UNITTESTING_ASSERT_EQUAL(nullptr, v.Data());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetCapacity());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetCapacityInBytes());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSizeInBytes());
	SEOUL_UNITTESTING_ASSERT(v.IsEmpty());
}

void VectorTest::TestEmptySimple()
{
	ContainerTestSimple simple;

	Vector<ContainerTestSimple, MemoryBudgets::DataStore> v;
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetCapacity());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetCapacityInBytes());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSizeInBytes());
	SEOUL_UNITTESTING_ASSERT(v.IsEmpty());

	SEOUL_UNITTESTING_ASSERT_EQUAL(v.Begin(), v.End());
	SEOUL_UNITTESTING_ASSERT(!v.Contains(simple));
	SEOUL_UNITTESTING_ASSERT(!v.ContainsFromBack(simple));
	SEOUL_UNITTESTING_ASSERT_EQUAL(nullptr, v.Data());
	SEOUL_UNITTESTING_ASSERT_EQUAL(v.End(), v.Begin());
	SEOUL_UNITTESTING_ASSERT_EQUAL(v.Begin(), v.Erase(v.Begin(), v.Begin()));

	simple.m_B = 33;
	simple.m_iA = 23;
	v.Fill(simple);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetCapacity());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetCapacityInBytes());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSizeInBytes());
	SEOUL_UNITTESTING_ASSERT(v.IsEmpty());

	SEOUL_UNITTESTING_ASSERT_EQUAL(v.End(), v.Find(simple));
	SEOUL_UNITTESTING_ASSERT_EQUAL(v.End(), v.FindFromBack(simple));

	v.Resize(0);
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
		Vector<ContainerTestSimple, MemoryBudgets::DataStore> v2;
		v.Swap(v2);
	}

	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetCapacity());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetCapacityInBytes());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.GetSizeInBytes());
	SEOUL_UNITTESTING_ASSERT(v.IsEmpty());

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

void VectorTest::TestEqualityBuiltin()
{
	Vector<UInt32> vA;
	Vector<UInt32> vB;

	vA.PushBack(1);
	vB.PushBack(1);
	vB.PushBack(2);

	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(vA, vB);

	vA.PushBack(2);

	SEOUL_UNITTESTING_ASSERT_EQUAL(vA, vB);

	vA[1] = 3;

	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(vA, vB);
}

void VectorTest::TestEqualityComplex()
{
	Vector<ContainerTestComplex> vA;
	Vector<ContainerTestComplex> vB;

	vA.PushBack(ContainerTestComplex(1));
	vB.PushBack(ContainerTestComplex(1));
	vB.PushBack(ContainerTestComplex(2));

	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(vA, vB);

	vA.PushBack(ContainerTestComplex(2));

	SEOUL_UNITTESTING_ASSERT_EQUAL(vA, vB);

	vA[1] = ContainerTestComplex(3);

	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(vA, vB);
}

void VectorTest::TestEqualitySimple()
{
	Vector<ContainerTestSimple> vA;
	Vector<ContainerTestSimple> vB;

	vA.PushBack(ContainerTestSimple::Create(1));
	vB.PushBack(ContainerTestSimple::Create(1));
	vB.PushBack(ContainerTestSimple::Create(2));

	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(vA, vB);

	vA.PushBack(ContainerTestSimple::Create(2));

	SEOUL_UNITTESTING_ASSERT_EQUAL(vA, vB);

	vA[1] = ContainerTestSimple::Create(3);

	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(vA, vB);
}

void VectorTest::TestFind()
{
	// Empty
	{
		Vector<ContainerTestComplex> v;
		SEOUL_UNITTESTING_ASSERT(!v.Contains(ContainerTestComplex(25)));
		SEOUL_UNITTESTING_ASSERT(!v.Contains(25));
		SEOUL_UNITTESTING_ASSERT(!v.ContainsFromBack(ContainerTestComplex(25)));
		SEOUL_UNITTESTING_ASSERT(!v.ContainsFromBack(25));
		SEOUL_UNITTESTING_ASSERT_EQUAL(v.End(), v.Find(ContainerTestComplex(25)));
		SEOUL_UNITTESTING_ASSERT_EQUAL(v.End(), v.Find(25));
		SEOUL_UNITTESTING_ASSERT_EQUAL(v.End(), v.FindFromBack(ContainerTestComplex(25)));
		SEOUL_UNITTESTING_ASSERT_EQUAL(v.End(), v.FindFromBack(25));
	}

	// Not empty.
	{
		Int const aiNumbers[] = { 10, 123, 3, 98, 128, 1498, 3, 5 };

		Vector<ContainerTestComplex> v;
		for (size_t i = 0; i < SEOUL_ARRAY_COUNT(aiNumbers); ++i)
		{
			v.PushBack(ContainerTestComplex(aiNumbers[i]));
		}

		SEOUL_UNITTESTING_ASSERT(!v.Contains(ContainerTestComplex(25)));
		SEOUL_UNITTESTING_ASSERT(!v.Contains(25));
		SEOUL_UNITTESTING_ASSERT(!v.ContainsFromBack(ContainerTestComplex(25)));
		SEOUL_UNITTESTING_ASSERT(!v.ContainsFromBack(25));
		SEOUL_UNITTESTING_ASSERT_EQUAL(v.End(), v.Find(ContainerTestComplex(25)));
		SEOUL_UNITTESTING_ASSERT_EQUAL(v.End(), v.Find(25));
		SEOUL_UNITTESTING_ASSERT_EQUAL(v.End(), v.FindFromBack(ContainerTestComplex(25)));
		SEOUL_UNITTESTING_ASSERT_EQUAL(v.End(), v.FindFromBack(25));

		for (size_t i = 0; i < SEOUL_ARRAY_COUNT(aiNumbers); ++i)
		{
			SEOUL_UNITTESTING_ASSERT(v.Contains(ContainerTestComplex(aiNumbers[i])));
			SEOUL_UNITTESTING_ASSERT(v.Contains(aiNumbers[i]));
			SEOUL_UNITTESTING_ASSERT(v.ContainsFromBack(ContainerTestComplex(aiNumbers[i])));
			SEOUL_UNITTESTING_ASSERT(v.ContainsFromBack(aiNumbers[i]));

			if (aiNumbers[i] == 3)
			{
				SEOUL_UNITTESTING_ASSERT_EQUAL(v.Begin() + 2, v.Find(ContainerTestComplex(aiNumbers[i])));
				SEOUL_UNITTESTING_ASSERT_EQUAL(v.Begin() + 2, v.Find(aiNumbers[i]));
				SEOUL_UNITTESTING_ASSERT_EQUAL(v.Begin() + 6, v.FindFromBack(ContainerTestComplex(aiNumbers[i])));
				SEOUL_UNITTESTING_ASSERT_EQUAL(v.Begin() + 6, v.FindFromBack(aiNumbers[i]));
			}
			else
			{
				SEOUL_UNITTESTING_ASSERT_EQUAL(v.Begin() + i, v.Find(ContainerTestComplex(aiNumbers[i])));
				SEOUL_UNITTESTING_ASSERT_EQUAL(v.Begin() + i, v.Find(aiNumbers[i]));
				SEOUL_UNITTESTING_ASSERT_EQUAL(v.Begin() + i, v.FindFromBack(ContainerTestComplex(aiNumbers[i])));
				SEOUL_UNITTESTING_ASSERT_EQUAL(v.Begin() + i, v.FindFromBack(aiNumbers[i]));
			}
		}
	}
}

void VectorTest::TestMethods()
{
	Vector<Int> testVec;
	// GetCapacity() should return the initial size of the Vector
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
	testVec.Resize(11);
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
	testVec.Resize(0);

	//set it back again for copy tests
	for(Int i = 0; i< 10; i++)
	{
		testVec.PushBack(i);
	}
	//testing copy constructor
	{
		Vector<Int> otherVec(testVec);
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

		Vector<Int, MemoryBudgets::Debug> otherVec2(testVec);
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
		Vector<Int> otherVec;
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

		Vector<Int, MemoryBudgets::Debug> otherVec2;
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

void VectorTest::TestInsertBuiltin()
{
	Vector<Int>::Iterator iter;

	Vector<Int> vec;
	vec.Insert(vec.Begin(), 0, 0);  // still empty
	SEOUL_UNITTESTING_ASSERT(vec.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, vec.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, vec.GetSizeInBytes());

	vec.Reserve(2);
	SEOUL_UNITTESTING_ASSERT(vec.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, vec.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, vec.GetSizeInBytes());
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, vec.GetCapacity());
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt32)(2 * sizeof(Int)), vec.GetCapacityInBytes());
	for (int i = 0; i < 4; i++)
	{
		vec.PushBack(i);
	}
	// 0 1 2 3
	SEOUL_UNITTESTING_ASSERT_EQUAL(vec[0], 0);
	SEOUL_UNITTESTING_ASSERT_EQUAL(vec.At(0), 0);
	SEOUL_UNITTESTING_ASSERT_EQUAL(vec[1], 1);
	SEOUL_UNITTESTING_ASSERT_EQUAL(vec.At(1), 1);
	SEOUL_UNITTESTING_ASSERT_EQUAL(vec[2], 2);
	SEOUL_UNITTESTING_ASSERT_EQUAL(vec.At(2), 2);
	SEOUL_UNITTESTING_ASSERT_EQUAL(vec[3], 3);
	SEOUL_UNITTESTING_ASSERT_EQUAL(vec.At(3), 3);

	SEOUL_UNITTESTING_ASSERT_EQUAL(4, vec.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(4 * sizeof(Int), vec.GetSizeInBytes());

	// Test the different code paths of Insert(iterator, value)
	SEOUL_UNITTESTING_ASSERT(vec.GetCapacity() <= 6);
	vec.Reserve(6);
	iter = vec.Insert(vec.End(), 4);  // 0 1 2 3 4
	SEOUL_UNITTESTING_ASSERT_EQUAL(4, *iter);
	iter = vec.Insert(vec.Begin() + 2, 5);  // 0 1 5 2 3 4
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, *iter);
	iter = vec.Insert(vec.End(), 6);  // 0 1 5 2 3 4 6
	SEOUL_UNITTESTING_ASSERT_EQUAL(6, *iter);

	// Test the different code paths of Insert(iterator, count, value)
	SEOUL_UNITTESTING_ASSERT(vec.GetCapacity() <= 17);
	vec.Reserve(17);
	vec.Insert(vec.End(), 3, 7);  // 0 1 5 2 3 4 6 7 7 7
	vec.Insert(vec.Begin() + 3, 2, 8);  // 0 1 5 8 8 2 3 4 6 7 7 7
	vec.Insert(vec.End() - 4, 5, 9);  // 0 1 5 8 8 2 3 4 9 9 9 9 9 6 7 7 7
	vec.Insert(vec.Begin() + 2, 2, 10);  // 0 1 10 10 5 8 8 2 3 4 9 9 9 9 9 6 7 7 7
	vec.Insert(vec.Begin() + 1, 0, 11);  // same

	int expected[] = {0, 1, 10, 10, 5, 8, 8, 2, 3, 4, 9, 9, 9, 9, 9, 6, 7, 7, 7};
	SEOUL_UNITTESTING_ASSERT_EQUAL(SEOUL_ARRAY_COUNT(expected), (size_t)vec.GetSize());
	for(UInt i = 0; i < vec.GetSize(); i++)
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(expected[i], vec[i]);
	}

	// Test the different code paths of Insert(iterator, rangeStart, rangeEnd)
	Vector<Int> vec2;
	vec2.Insert(vec2.Begin(), vec.Begin(), vec.Begin());  // still empty
	vec2.Insert(vec2.Begin(), vec.Begin(), vec.Begin() + 4);  // 0 1 10 10

	vec2.Reserve(20);
	vec2.Insert(vec2.Begin() + 1, vec.Begin() + 4, vec.Begin() + 6);  // 0 5 8 1 10 10
	vec2.Insert(vec2.Begin() + 5, vec.Begin() + 6, vec.Begin() + 9);  // 0 5 8 1 10 8 2 3 10

	int expected2[] = {0, 5, 8, 1, 10, 8, 2, 3, 10};
	SEOUL_UNITTESTING_ASSERT_EQUAL(SEOUL_ARRAY_COUNT(expected2), (size_t)vec2.GetSize());
	for(UInt i = 0; i < vec2.GetSize(); i++)
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(expected2[i], vec2[i]);
	}
}

void VectorTest::TestInsertComplex()
{
	Vector<ContainerTestComplex>::Iterator iter;

	Vector<ContainerTestComplex> vec;
	vec.Insert(vec.Begin(), 0, 0);  // still empty
	SEOUL_UNITTESTING_ASSERT(vec.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, vec.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, vec.GetSizeInBytes());

	vec.Reserve(2);
	SEOUL_UNITTESTING_ASSERT(vec.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, vec.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, vec.GetSizeInBytes());
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, vec.GetCapacity());
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt32)(2 * sizeof(ContainerTestComplex)), vec.GetCapacityInBytes());
	for (int i = 0; i < 4; i++)
	{
		vec.PushBack(ContainerTestComplex(i));
	}
	// 0 1 2 3

	SEOUL_UNITTESTING_ASSERT_EQUAL(4, vec.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(4 * sizeof(ContainerTestComplex), vec.GetSizeInBytes());

	// Test the different code paths of Insert(iterator, value)
	SEOUL_UNITTESTING_ASSERT(vec.GetCapacity() <= 6);
	vec.Reserve(6);
	iter = vec.Insert(vec.End(), ContainerTestComplex(4));  // 0 1 2 3 4
	SEOUL_UNITTESTING_ASSERT_EQUAL(4, iter->m_iVariableValue);
	SEOUL_UNITTESTING_ASSERT_EQUAL(33, iter->m_iFixedValue);
	iter = vec.Insert(vec.Begin() + 2, ContainerTestComplex(5));  // 0 1 5 2 3 4
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, iter->m_iVariableValue);
	SEOUL_UNITTESTING_ASSERT_EQUAL(33, iter->m_iFixedValue);
	iter = vec.Insert(vec.End(), ContainerTestComplex(6));  // 0 1 5 2 3 4 6
	SEOUL_UNITTESTING_ASSERT_EQUAL(6, iter->m_iVariableValue);
	SEOUL_UNITTESTING_ASSERT_EQUAL(33, iter->m_iFixedValue);

	// Test the different code paths of Insert(iterator, count, value)
	SEOUL_UNITTESTING_ASSERT(vec.GetCapacity() <= 17);
	vec.Reserve(17);
	vec.Insert(vec.End(), 3, ContainerTestComplex(7));  // 0 1 5 2 3 4 6 7 7 7
	vec.Insert(vec.Begin() + 3, 2, ContainerTestComplex(8));  // 0 1 5 8 8 2 3 4 6 7 7 7
	vec.Insert(vec.End() - 4, 5, ContainerTestComplex(9));  // 0 1 5 8 8 2 3 4 9 9 9 9 9 6 7 7 7
	vec.Insert(vec.Begin() + 2, 2, ContainerTestComplex(10));  // 0 1 10 10 5 8 8 2 3 4 9 9 9 9 9 6 7 7 7
	vec.Insert(vec.Begin() + 1, 0, ContainerTestComplex(11));  // same

	int expected[] = {0, 1, 10, 10, 5, 8, 8, 2, 3, 4, 9, 9, 9, 9, 9, 6, 7, 7, 7};
	SEOUL_UNITTESTING_ASSERT_EQUAL(SEOUL_ARRAY_COUNT(expected), (size_t)vec.GetSize());
	for(UInt i = 0; i < vec.GetSize(); i++)
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(expected[i], vec[i].m_iVariableValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL(33, vec[i].m_iFixedValue);
	}

	SEOUL_UNITTESTING_ASSERT_EQUAL((int)vec.GetSize(), ContainerTestComplex::s_iCount);

	// Test the different code paths of Insert(iterator, rangeStart, rangeEnd)
	Vector<ContainerTestComplex> vec2;
	vec2.Insert(vec2.Begin(), vec.Begin(), vec.Begin());  // still empty
	vec2.Insert(vec2.Begin(), vec.Begin(), vec.Begin() + 4);  // 0 1 10 10

	vec2.Reserve(20);
	vec2.Insert(vec2.Begin() + 1, vec.Begin() + 4, vec.Begin() + 6);  // 0 5 8 1 10 10
	vec2.Insert(vec2.Begin() + 5, vec.Begin() + 6, vec.Begin() + 9);  // 0 5 8 1 10 8 2 3 10

	int expected2[] = {0, 5, 8, 1, 10, 8, 2, 3, 10};
	SEOUL_UNITTESTING_ASSERT_EQUAL(SEOUL_ARRAY_COUNT(expected2), (size_t)vec2.GetSize());
	for(UInt i = 0; i < vec2.GetSize(); i++)
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(expected2[i], vec2[i].m_iVariableValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL(33, vec[i].m_iFixedValue);
	}

	SEOUL_UNITTESTING_ASSERT_EQUAL((int)(vec.GetSize() + vec2.GetSize()), ContainerTestComplex::s_iCount);
}

void VectorTest::TestInsertSimple()
{
	Vector<ContainerTestSimple>::Iterator iter;

	Vector<ContainerTestSimple> vec;
	vec.Insert(vec.Begin(), 0, 0);  // still empty
	SEOUL_UNITTESTING_ASSERT(vec.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, vec.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, vec.GetSizeInBytes());

	vec.Reserve(2);
	SEOUL_UNITTESTING_ASSERT(vec.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, vec.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, vec.GetSizeInBytes());
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, vec.GetCapacity());
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt32)(2 * sizeof(ContainerTestSimple)), vec.GetCapacityInBytes());
	for (int i = 0; i < 4; i++)
	{
		ContainerTestSimple simple;
		simple.m_iA = (i);
		simple.m_B = 33;
		vec.PushBack(simple);
	}
	// 0 1 2 3

	SEOUL_UNITTESTING_ASSERT_EQUAL(4, vec.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(4 * sizeof(ContainerTestSimple), vec.GetSizeInBytes());

	// Test the different code paths of Insert(iterator, value)
	SEOUL_UNITTESTING_ASSERT(vec.GetCapacity() <= 6);
	vec.Reserve(6);
	ContainerTestSimple simple;
	simple.m_B = 33;

	simple.m_iA = 4;
	iter = vec.Insert(vec.End(), simple);  // 0 1 2 3 4
	SEOUL_UNITTESTING_ASSERT_EQUAL(4, iter->m_iA);
	SEOUL_UNITTESTING_ASSERT_EQUAL(33, iter->m_B);
	simple.m_iA = 5;
	iter = vec.Insert(vec.Begin() + 2, simple);  // 0 1 5 2 3 4
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, iter->m_iA);
	SEOUL_UNITTESTING_ASSERT_EQUAL(33, iter->m_B);
	simple.m_iA = 6;
	iter = vec.Insert(vec.End(), simple);  // 0 1 5 2 3 4 6
	SEOUL_UNITTESTING_ASSERT_EQUAL(6, iter->m_iA);
	SEOUL_UNITTESTING_ASSERT_EQUAL(33, iter->m_B);

	// Test the different code paths of Insert(iterator, count, value)
	SEOUL_UNITTESTING_ASSERT(vec.GetCapacity() <= 17);
	vec.Reserve(17);
	simple.m_iA = 7;
	vec.Insert(vec.End(), 3, simple);  // 0 1 5 2 3 4 6 7 7 7
	simple.m_iA = 8;
	vec.Insert(vec.Begin() + 3, 2, simple);  // 0 1 5 8 8 2 3 4 6 7 7 7
	simple.m_iA = 9;
	vec.Insert(vec.End() - 4, 5, simple);  // 0 1 5 8 8 2 3 4 9 9 9 9 9 6 7 7 7
	simple.m_iA = 10;
	vec.Insert(vec.Begin() + 2, 2, simple);  // 0 1 10 10 5 8 8 2 3 4 9 9 9 9 9 6 7 7 7
	simple.m_iA = 11;
	vec.Insert(vec.Begin() + 1, 0, simple);  // same

	int expected[] = {0, 1, 10, 10, 5, 8, 8, 2, 3, 4, 9, 9, 9, 9, 9, 6, 7, 7, 7};
	SEOUL_UNITTESTING_ASSERT_EQUAL(SEOUL_ARRAY_COUNT(expected), (size_t)vec.GetSize());
	for(UInt i = 0; i < vec.GetSize(); i++)
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(expected[i], vec[i].m_iA);
		SEOUL_UNITTESTING_ASSERT_EQUAL(33, vec[i].m_B);
	}

	// Test the different code paths of Insert(iterator, rangeStart, rangeEnd)
	Vector<ContainerTestSimple> vec2;
	vec2.Insert(vec2.Begin(), vec.Begin(), vec.Begin());  // still empty
	vec2.Insert(vec2.Begin(), vec.Begin(), vec.Begin() + 4);  // 0 1 10 10

	vec2.Reserve(20);
	vec2.Insert(vec2.Begin() + 1, vec.Begin() + 4, vec.Begin() + 6);  // 0 5 8 1 10 10
	vec2.Insert(vec2.Begin() + 5, vec.Begin() + 6, vec.Begin() + 9);  // 0 5 8 1 10 8 2 3 10

	int expected2[] = {0, 5, 8, 1, 10, 8, 2, 3, 10};
	SEOUL_UNITTESTING_ASSERT_EQUAL(SEOUL_ARRAY_COUNT(expected2), (size_t)vec2.GetSize());
	for(UInt i = 0; i < vec2.GetSize(); i++)
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(expected2[i], vec2[i].m_iA);
		SEOUL_UNITTESTING_ASSERT_EQUAL(33, vec[i].m_B);
	}
}

void VectorTest::TestEraseBuiltin()
{
	// Built-in type.
	{
		Vector<Int> testVec;

		for(Int i = 0; i < 6; i++)
		{
			testVec.PushBack(i + 10);
		}

		testVec.Erase(testVec.Begin() + 3);

		SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)5, testVec.GetSize());
		// GetCapacity() >= GetSize()
		SEOUL_UNITTESTING_ASSERT( testVec.GetCapacity() >= testVec.GetSize() );

		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)10, testVec[0]);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)11, testVec[1]);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)12, testVec[2]);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)14, testVec[3]);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)15, testVec[4]);

		testVec.Erase(testVec.Begin());

		SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)4, testVec.GetSize());
		// GetCapacity() >= GetSize()
		SEOUL_UNITTESTING_ASSERT( testVec.GetCapacity() >= testVec.GetSize() );

		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)11, testVec[0]);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)12, testVec[1]);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)14, testVec[2]);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)15, testVec[3]);

		testVec.Erase(testVec.Begin() + 2, testVec.Begin() + 2);  // should not do anything
		SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)4, testVec.GetSize());

		testVec.Erase(testVec.Begin() + 2, testVec.Begin() + 3);  // should erase one element
		SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)3, testVec.GetSize());
		// GetCapacity() >= GetSize()
		SEOUL_UNITTESTING_ASSERT( testVec.GetCapacity() >= testVec.GetSize() );

		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)11, testVec[0]);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)12, testVec[1]);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)15, testVec[2]);

		testVec.Erase(testVec.Begin() + 1, testVec.Begin() + 3);
		SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)1, testVec.GetSize());
		// GetCapacity() >= GetSize()
		SEOUL_UNITTESTING_ASSERT( testVec.GetCapacity() >= testVec.GetSize() );

		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)11, testVec[0]);

		testVec.Erase(testVec.Begin(), testVec.Begin() + 1);
		SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)0, testVec.GetSize());
		// GetCapacity() >= GetSize()
		SEOUL_UNITTESTING_ASSERT( testVec.GetCapacity() >= testVec.GetSize() );
	}
}

void VectorTest::TestEraseSimple()
{
	// Simple type.
	{
		Vector<ContainerTestSimple> testVec;

		for(Int i = 0; i < 6; i++)
		{
			ContainerTestSimple simple;
			simple.m_iA = (i + 10);
			simple.m_B = (i + 3);
			testVec.PushBack(simple);
		}

		testVec.Erase(testVec.Begin() + 3);

		SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)5, testVec.GetSize());
		// GetCapacity() >= GetSize()
		SEOUL_UNITTESTING_ASSERT( testVec.GetCapacity() >= testVec.GetSize() );

		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)10, testVec[0].m_iA);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)11, testVec[1].m_iA);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)12, testVec[2].m_iA);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)14, testVec[3].m_iA);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)15, testVec[4].m_iA);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)3, testVec[0].m_B);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)4, testVec[1].m_B);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)5, testVec[2].m_B);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)7, testVec[3].m_B);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)8, testVec[4].m_B);

		testVec.Erase(testVec.Begin());

		SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)4, testVec.GetSize());
		// GetCapacity() >= GetSize()
		SEOUL_UNITTESTING_ASSERT( testVec.GetCapacity() >= testVec.GetSize() );

		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)11, testVec[0].m_iA);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)12, testVec[1].m_iA);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)14, testVec[2].m_iA);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)15, testVec[3].m_iA);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)4, testVec[0].m_B);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)5, testVec[1].m_B);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)7, testVec[2].m_B);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)8, testVec[3].m_B);

		testVec.Erase(testVec.Begin() + 2, testVec.Begin() + 2);  // should not do anything
		SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)4, testVec.GetSize());

		testVec.Erase(testVec.Begin() + 2, testVec.Begin() + 3);  // should erase one element
		SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)3, testVec.GetSize());
		// GetCapacity() >= GetSize()
		SEOUL_UNITTESTING_ASSERT( testVec.GetCapacity() >= testVec.GetSize() );

		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)11, testVec[0].m_iA);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)12, testVec[1].m_iA);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)15, testVec[2].m_iA);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)4, testVec[0].m_B);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)5, testVec[1].m_B);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)8, testVec[2].m_B);

		testVec.Erase(testVec.Begin() + 1, testVec.Begin() + 3);
		SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)1, testVec.GetSize());
		// GetCapacity() >= GetSize()
		SEOUL_UNITTESTING_ASSERT( testVec.GetCapacity() >= testVec.GetSize() );

		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)11, testVec[0].m_iA);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)4, testVec[0].m_B);

		testVec.Erase(testVec.Begin(), testVec.Begin() + 1);
		SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)0, testVec.GetSize());
		// GetCapacity() >= GetSize()
		SEOUL_UNITTESTING_ASSERT( testVec.GetCapacity() >= testVec.GetSize() );
	}
}

void VectorTest::TestEraseComplex()
{
	// Complex type.
	{
		Vector<ContainerTestComplex> testVec;

		for(Int i = 0; i < 6; i++)
		{
			testVec.PushBack(ContainerTestComplex(i + 10));
		}

		testVec.Erase(testVec.Begin() + 3);

		SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)5, testVec.GetSize());
		// GetCapacity() >= GetSize()
		SEOUL_UNITTESTING_ASSERT( testVec.GetCapacity() >= testVec.GetSize() );

		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)10, testVec[0].m_iVariableValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)11, testVec[1].m_iVariableValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)12, testVec[2].m_iVariableValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)14, testVec[3].m_iVariableValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)15, testVec[4].m_iVariableValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)33, testVec[0].m_iFixedValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)33, testVec[1].m_iFixedValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)33, testVec[2].m_iFixedValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)33, testVec[3].m_iFixedValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)33, testVec[4].m_iFixedValue);

		testVec.Erase(testVec.Begin());

		SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)4, testVec.GetSize());
		// GetCapacity() >= GetSize()
		SEOUL_UNITTESTING_ASSERT( testVec.GetCapacity() >= testVec.GetSize() );

		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)11, testVec[0].m_iVariableValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)12, testVec[1].m_iVariableValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)14, testVec[2].m_iVariableValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)15, testVec[3].m_iVariableValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)33, testVec[0].m_iFixedValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)33, testVec[1].m_iFixedValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)33, testVec[2].m_iFixedValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)33, testVec[3].m_iFixedValue);

		testVec.Erase(testVec.Begin() + 2, testVec.Begin() + 2);  // should not do anything
		SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)4, testVec.GetSize());

		testVec.Erase(testVec.Begin() + 2, testVec.Begin() + 3);  // should erase one element
		SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)3, testVec.GetSize());
		// GetCapacity() >= GetSize()
		SEOUL_UNITTESTING_ASSERT( testVec.GetCapacity() >= testVec.GetSize() );

		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)11, testVec[0].m_iVariableValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)12, testVec[1].m_iVariableValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)15, testVec[2].m_iVariableValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)33, testVec[0].m_iFixedValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)33, testVec[1].m_iFixedValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)33, testVec[2].m_iFixedValue);

		testVec.Erase(testVec.Begin() + 1, testVec.Begin() + 3);
		SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)1, testVec.GetSize());
		// GetCapacity() >= GetSize()
		SEOUL_UNITTESTING_ASSERT( testVec.GetCapacity() >= testVec.GetSize() );

		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)11, testVec[0].m_iVariableValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)33, testVec[0].m_iFixedValue);

		testVec.Erase(testVec.Begin(), testVec.Begin() + 1);
		SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)0, testVec.GetSize());
		// GetCapacity() >= GetSize()
		SEOUL_UNITTESTING_ASSERT( testVec.GetCapacity() >= testVec.GetSize() );
	}
}

void VectorTest::TestEraseReturnBuiltin()
{
	Vector<Int> testVec;

	for(Int i = 0; i < 6; i++)
	{
		testVec.PushBack(i + 10);
	}

	SEOUL_UNITTESTING_ASSERT_EQUAL(6, testVec.GetSize());

	auto i = testVec.Erase(testVec.Begin() + 2);
	SEOUL_UNITTESTING_ASSERT_EQUAL(13, *i);
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, testVec.GetSize());

	i = testVec.Erase(testVec.Begin() + 4);
	SEOUL_UNITTESTING_ASSERT_EQUAL(testVec.End(), i);
	SEOUL_UNITTESTING_ASSERT_EQUAL(4, testVec.GetSize());

	i = testVec.Erase(testVec.Begin());
	SEOUL_UNITTESTING_ASSERT_EQUAL(11, *i);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, testVec.GetSize());

	i = testVec.Erase(testVec.Begin() + 1);
	SEOUL_UNITTESTING_ASSERT_EQUAL(14, *i);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, testVec.GetSize());

	i = testVec.Erase(testVec.Begin());
	SEOUL_UNITTESTING_ASSERT_EQUAL(14, *i);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, testVec.GetSize());

	i = testVec.Erase(testVec.Begin());
	SEOUL_UNITTESTING_ASSERT_EQUAL(testVec.End(), i);
	SEOUL_UNITTESTING_ASSERT(testVec.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, testVec.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(testVec.Begin(), testVec.End());
}

void VectorTest::TestEraseReturnComplex()
{
	{
		Vector<ContainerTestComplex> testVec;

		for(Int i = 0; i < 6; i++)
		{
			testVec.PushBack(ContainerTestComplex(i + 10));
		}

		SEOUL_UNITTESTING_ASSERT_EQUAL(6, testVec.GetSize());

		auto i = testVec.Erase(testVec.Begin() + 2);
		SEOUL_UNITTESTING_ASSERT_EQUAL(13, i->m_iVariableValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, testVec.GetSize());

		i = testVec.Erase(testVec.Begin() + 4);
		SEOUL_UNITTESTING_ASSERT_EQUAL(testVec.End(), i);
		SEOUL_UNITTESTING_ASSERT_EQUAL(4, testVec.GetSize());

		i = testVec.Erase(testVec.Begin());
		SEOUL_UNITTESTING_ASSERT_EQUAL(11, i->m_iVariableValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, testVec.GetSize());

		i = testVec.Erase(testVec.Begin() + 1);
		SEOUL_UNITTESTING_ASSERT_EQUAL(14, i->m_iVariableValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL(2, testVec.GetSize());

		i = testVec.Erase(testVec.Begin());
		SEOUL_UNITTESTING_ASSERT_EQUAL(14, i->m_iVariableValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, testVec.GetSize());

		i = testVec.Erase(testVec.Begin());
		SEOUL_UNITTESTING_ASSERT_EQUAL(testVec.End(), i);
		SEOUL_UNITTESTING_ASSERT(testVec.IsEmpty());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, testVec.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(testVec.Begin(), testVec.End());
	}
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
}

void VectorTest::TestEraseReturnSimple()
{
	Vector<ContainerTestSimple> testVec;

	for(Int i = 0; i < 6; i++)
	{
		testVec.PushBack(ContainerTestSimple::Create(i + 10));
	}

	SEOUL_UNITTESTING_ASSERT_EQUAL(6, testVec.GetSize());

	auto i = testVec.Erase(testVec.Begin() + 2);
	SEOUL_UNITTESTING_ASSERT_EQUAL(13, i->m_iA);
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, testVec.GetSize());

	i = testVec.Erase(testVec.Begin() + 4);
	SEOUL_UNITTESTING_ASSERT_EQUAL(testVec.End(), i);
	SEOUL_UNITTESTING_ASSERT_EQUAL(4, testVec.GetSize());

	i = testVec.Erase(testVec.Begin());
	SEOUL_UNITTESTING_ASSERT_EQUAL(11, i->m_iA);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, testVec.GetSize());

	i = testVec.Erase(testVec.Begin() + 1);
	SEOUL_UNITTESTING_ASSERT_EQUAL(14, i->m_iA);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, testVec.GetSize());

	i = testVec.Erase(testVec.Begin());
	SEOUL_UNITTESTING_ASSERT_EQUAL(14, i->m_iA);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, testVec.GetSize());

	i = testVec.Erase(testVec.Begin());
	SEOUL_UNITTESTING_ASSERT_EQUAL(testVec.End(), i);
	SEOUL_UNITTESTING_ASSERT(testVec.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, testVec.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(testVec.Begin(), testVec.End());
}

void VectorTest::TestEraseRangeReturnBuiltin()
{
	Vector<Int> testVec;

	for(Int i = 0; i < 6; i++)
	{
		testVec.PushBack(i + 10);
	}

	SEOUL_UNITTESTING_ASSERT_EQUAL(6, testVec.GetSize());

	auto i = testVec.Erase(testVec.Begin() + 1, testVec.Begin() + 3);
	SEOUL_UNITTESTING_ASSERT_EQUAL(13, *i);
	SEOUL_UNITTESTING_ASSERT_EQUAL(4, testVec.GetSize());

	i = testVec.Erase(testVec.Begin(), testVec.Begin() + 2);
	SEOUL_UNITTESTING_ASSERT_EQUAL(14, *i);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, testVec.GetSize());

	i = testVec.Erase(testVec.Begin(), testVec.Begin() + 1);
	SEOUL_UNITTESTING_ASSERT_EQUAL(15, *i);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, testVec.GetSize());

	i = testVec.Erase(testVec.Begin(), testVec.Begin() + 1);
	SEOUL_UNITTESTING_ASSERT_EQUAL(testVec.End(), i);
	SEOUL_UNITTESTING_ASSERT(testVec.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, testVec.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(testVec.Begin(), testVec.End());
}

void VectorTest::TestEraseRangeReturnComplex()
{
	{
		Vector<ContainerTestComplex> testVec;

		for(Int i = 0; i < 6; i++)
		{
			testVec.PushBack(ContainerTestComplex(i + 10));
		}

		SEOUL_UNITTESTING_ASSERT_EQUAL(6, testVec.GetSize());

		auto i = testVec.Erase(testVec.Begin() + 1, testVec.Begin() + 3);
		SEOUL_UNITTESTING_ASSERT_EQUAL(13, i->m_iVariableValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL(4, testVec.GetSize());

		i = testVec.Erase(testVec.Begin(), testVec.Begin() + 2);
		SEOUL_UNITTESTING_ASSERT_EQUAL(14, i->m_iVariableValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL(2, testVec.GetSize());

		i = testVec.Erase(testVec.Begin(), testVec.Begin() + 1);
		SEOUL_UNITTESTING_ASSERT_EQUAL(15, i->m_iVariableValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, testVec.GetSize());

		i = testVec.Erase(testVec.Begin(), testVec.Begin() + 1);
		SEOUL_UNITTESTING_ASSERT_EQUAL(testVec.End(), i);
		SEOUL_UNITTESTING_ASSERT(testVec.IsEmpty());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, testVec.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(testVec.Begin(), testVec.End());
	}
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
}

void VectorTest::TestEraseRangeReturnSimple()
{
	Vector<ContainerTestSimple> testVec;

	for(Int i = 0; i < 6; i++)
	{
		testVec.PushBack(ContainerTestSimple::Create(i + 10));
	}

	SEOUL_UNITTESTING_ASSERT_EQUAL(6, testVec.GetSize());

	auto i = testVec.Erase(testVec.Begin() + 1, testVec.Begin() + 3);
	SEOUL_UNITTESTING_ASSERT_EQUAL(13, i->m_iA);
	SEOUL_UNITTESTING_ASSERT_EQUAL(4, testVec.GetSize());

	i = testVec.Erase(testVec.Begin(), testVec.Begin() + 2);
	SEOUL_UNITTESTING_ASSERT_EQUAL(14, i->m_iA);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, testVec.GetSize());

	i = testVec.Erase(testVec.Begin(), testVec.Begin() + 1);
	SEOUL_UNITTESTING_ASSERT_EQUAL(15, i->m_iA);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, testVec.GetSize());

	i = testVec.Erase(testVec.Begin(), testVec.Begin() + 1);
	SEOUL_UNITTESTING_ASSERT_EQUAL(testVec.End(), i);
	SEOUL_UNITTESTING_ASSERT(testVec.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, testVec.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(testVec.Begin(), testVec.End());
}

/**
 * Tests the Iterator interfaces of the Vector class
 */
void VectorTest::TestIterators()
{
	Vector<Int> testVec;

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

/** Tests for ranged-based for loops (for a : b). */
void VectorTest::TestRangedFor()
{
	Vector<Int> testVec;
	testVec.PushBack(3);
	testVec.PushBack(7);
	testVec.PushBack(2);

	Int i = 0;
	for (auto v : testVec)
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(testVec[i], v);
		++i;
	}

	testVec.Insert(testVec.Begin(), 35);
	i = 0;
	for (auto v : testVec)
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(testVec[i], v);
		++i;
	}

	testVec.PushBack(77);
	i = 0;
	for (auto v : testVec)
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(testVec[i], v);
		++i;
	}
}

static Bool UInt64AlwaysTrueFunctor(UInt64) { return true; }
static Bool UInt64LessThan(UInt64 a, UInt64 b) { return (a < b); }
void VectorTest::TestRemoveBuiltin()
{
	Vector<UInt64, MemoryBudgets::TBDContainer> vector;
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Begin(), vector.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT(vector.IsEmpty());

	// Nop
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vector.Remove(UInt64(25)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Begin(), vector.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT(vector.IsEmpty());

	// Nop
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vector.RemoveIf(UInt64AlwaysTrueFunctor));
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Begin(), vector.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT(vector.IsEmpty());

	// Populate the vector.
	vector.PushBack(UInt64(25));
	vector.Insert(vector.Begin(), UInt64(13));
	vector.PopBack();
	vector.PushBack(UInt64(23));
	vector.Erase(vector.Begin());
	vector.Insert(vector.Begin(), UInt64(15));

	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(vector.Begin(), vector.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT(!vector.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Front(), UInt64(15));
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Back(), UInt64(23));

	SEOUL_UNITTESTING_ASSERT_EQUAL(1, vector.Remove(UInt64(23)));
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(vector.Begin(), vector.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT(!vector.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Front(), UInt64(15));
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Back(), UInt64(15));

	SEOUL_UNITTESTING_ASSERT_EQUAL(1, vector.Remove(UInt64(15)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Begin(), vector.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT(vector.IsEmpty());

	for (Int i = 0; i < 5; ++i)
	{
		vector.PushBack(5);
		vector.Insert(vector.Begin(), 5);
		vector.Erase(vector.Begin());
		vector.PopBack();
		vector.PushBack(5);
	}
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(vector.Begin(), vector.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT(!vector.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Front(), UInt64(5));
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Back(), UInt64(5));

	SEOUL_UNITTESTING_ASSERT_EQUAL(5, vector.RemoveIf(UInt64AlwaysTrueFunctor));
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Begin(), vector.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT(vector.IsEmpty());

	for (Int i = 0; i < 5; ++i)
	{
		vector.PushBack(i);
		vector.Insert(vector.Begin(), i);
	}
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(vector.Begin(), vector.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(10, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT(!vector.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Front(), UInt64(4));
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Back(), UInt64(4));

	SEOUL_UNITTESTING_ASSERT_EQUAL(2, vector.Remove(UInt64(0)));
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(vector.Begin(), vector.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(8, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT(!vector.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Front(), UInt64(4));
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Back(), UInt64(4));

	QuickSort(vector.Begin(), vector.End());
	auto iter = vector.Begin();
	for (UInt64 i = 0; i < 4; ++i)
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(i + 1, *iter);
		++iter;
		SEOUL_UNITTESTING_ASSERT_EQUAL(i + 1, *iter);
		iter++;
	}

	Reverse(vector.Begin(), vector.End());
	iter = vector.End();
	for (UInt64 i = 0; i < 4; ++i)
	{
		iter--;
		SEOUL_UNITTESTING_ASSERT_EQUAL(i + 1, *iter);
		--iter;
		SEOUL_UNITTESTING_ASSERT_EQUAL(i + 1, *iter);
	}

	QuickSort(vector.Begin(), vector.End(), UInt64LessThan);
	iter = vector.Begin();
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
void VectorTest::TestRemoveComplex()
{
	Vector<ContainerTestComplex, MemoryBudgets::TBDContainer> vector;
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Begin(), vector.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT(vector.IsEmpty());

	// Nop
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vector.Remove(ContainerTestComplex(25)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Begin(), vector.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT(vector.IsEmpty());

	// Nop
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vector.RemoveIf(ContainerTestComplexAlwaysTrueFunctor));
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Begin(), vector.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT(vector.IsEmpty());

	// Populate the vector.
	vector.PushBack(ContainerTestComplex(25));
	vector.Insert(vector.Begin(), ContainerTestComplex(13));
	vector.PopBack();
	vector.PushBack(ContainerTestComplex(23));
	vector.Erase(vector.Begin());
	vector.Insert(vector.Begin(), ContainerTestComplex(15));

	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(vector.Begin(), vector.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT(!vector.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Front(), ContainerTestComplex(15));
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Back(), ContainerTestComplex(23));

	SEOUL_UNITTESTING_ASSERT_EQUAL(1, vector.Remove(ContainerTestComplex(23)));
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(vector.Begin(), vector.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT(!vector.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Front(), ContainerTestComplex(15));
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Back(), ContainerTestComplex(15));

	SEOUL_UNITTESTING_ASSERT_EQUAL(1, vector.Remove(ContainerTestComplex(15)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Begin(), vector.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT(vector.IsEmpty());

	for (Int i = 0; i < 5; ++i)
	{
		vector.PushBack(ContainerTestComplex(5));
		vector.Insert(vector.Begin(), ContainerTestComplex(5));
		vector.Erase(vector.Begin());
		vector.PopBack();
		vector.PushBack(ContainerTestComplex(5));
	}
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(vector.Begin(), vector.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT(!vector.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Front(), ContainerTestComplex(5));
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Back(), ContainerTestComplex(5));

	SEOUL_UNITTESTING_ASSERT_EQUAL(5, vector.RemoveIf(ContainerTestComplexAlwaysTrueFunctor));
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Begin(), vector.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT(vector.IsEmpty());

	for (Int i = 0; i < 5; ++i)
	{
		vector.PushBack(ContainerTestComplex(i));
		vector.Insert(vector.Begin(), ContainerTestComplex(i));
	}
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(vector.Begin(), vector.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(10, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT(!vector.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Front(), ContainerTestComplex(4));
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Back(), ContainerTestComplex(4));

	SEOUL_UNITTESTING_ASSERT_EQUAL(2, vector.Remove(ContainerTestComplex(0)));
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(vector.Begin(), vector.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(8, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT(!vector.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Front(), ContainerTestComplex(4));
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Back(), ContainerTestComplex(4));

	QuickSort(vector.Begin(), vector.End());
	auto iter = vector.Begin();
	for (Int i = 0; i < 4; ++i)
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(i + 1), *iter);
		++iter;
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(i + 1), *iter);
		iter++;
	}

	Reverse(vector.Begin(), vector.End());
	iter = vector.End();
	for (Int i = 0; i < 4; ++i)
	{
		iter--;
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(i + 1), *iter);
		--iter;
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(i + 1), *iter);
	}

	QuickSort(vector.Begin(), vector.End(), ContainerTestComplexLessThan);
	iter = vector.Begin();
	for (Int i = 0; i < 4; ++i)
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(i + 1), *iter);
		++iter;
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(i + 1), *iter);
		iter++;
	}
}

void VectorTest::TestRemoveComplexCoerce()
{
	Vector<ContainerTestComplex, MemoryBudgets::TBDContainer> vector;
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Begin(), vector.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT(vector.IsEmpty());

	// Nop
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vector.Remove(25));
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Begin(), vector.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT(vector.IsEmpty());

	// Nop
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vector.RemoveIf(ContainerTestComplexAlwaysTrueFunctor));
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Begin(), vector.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT(vector.IsEmpty());

	// Populate the vector.
	vector.PushBack(ContainerTestComplex(25));
	vector.Insert(vector.Begin(), ContainerTestComplex(13));
	vector.PopBack();
	vector.PushBack(ContainerTestComplex(23));
	vector.Erase(vector.Begin());
	vector.Insert(vector.Begin(), ContainerTestComplex(15));

	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(vector.Begin(), vector.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT(!vector.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Front(), ContainerTestComplex(15));
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Back(), ContainerTestComplex(23));

	SEOUL_UNITTESTING_ASSERT_EQUAL(1, vector.Remove(23));
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(vector.Begin(), vector.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT(!vector.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Front(), ContainerTestComplex(15));
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Back(), ContainerTestComplex(15));

	SEOUL_UNITTESTING_ASSERT_EQUAL(1, vector.Remove(15));
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Begin(), vector.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT(vector.IsEmpty());

	for (Int i = 0; i < 5; ++i)
	{
		vector.PushBack(ContainerTestComplex(5));
		vector.Insert(vector.Begin(), ContainerTestComplex(5));
		vector.Erase(vector.Begin());
		vector.PopBack();
		vector.PushBack(ContainerTestComplex(5));
	}
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(vector.Begin(), vector.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT(!vector.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Front(), ContainerTestComplex(5));
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Back(), ContainerTestComplex(5));

	SEOUL_UNITTESTING_ASSERT_EQUAL(5, vector.RemoveIf(ContainerTestComplexAlwaysTrueFunctor));
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Begin(), vector.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT(vector.IsEmpty());

	for (Int i = 0; i < 5; ++i)
	{
		vector.PushBack(ContainerTestComplex(i));
		vector.Insert(vector.Begin(), ContainerTestComplex(i));
	}
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(vector.Begin(), vector.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(10, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT(!vector.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Front(), ContainerTestComplex(4));
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Back(), ContainerTestComplex(4));

	SEOUL_UNITTESTING_ASSERT_EQUAL(2, vector.Remove(0));
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(vector.Begin(), vector.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(8, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT(!vector.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Front(), ContainerTestComplex(4));
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Back(), ContainerTestComplex(4));

	QuickSort(vector.Begin(), vector.End());
	auto iter = vector.Begin();
	for (Int i = 0; i < 4; ++i)
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(i + 1), *iter);
		++iter;
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(i + 1), *iter);
		iter++;
	}

	Reverse(vector.Begin(), vector.End());
	iter = vector.End();
	for (Int i = 0; i < 4; ++i)
	{
		iter--;
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(i + 1), *iter);
		--iter;
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(i + 1), *iter);
	}

	QuickSort(vector.Begin(), vector.End(), ContainerTestComplexLessThan);
	iter = vector.Begin();
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
void VectorTest::TestRemoveSimple()
{
	Vector<ContainerTestSimple, MemoryBudgets::TBDContainer> vector;
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Begin(), vector.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT(vector.IsEmpty());

	// Nop
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vector.Remove(ContainerTestSimple::Create(25)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Begin(), vector.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT(vector.IsEmpty());

	// Nop
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vector.RemoveIf(ContainerTestSimpleAlwaysTrueFunctor));
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Begin(), vector.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT(vector.IsEmpty());

	// Populate the vector.
	vector.PushBack(ContainerTestSimple::Create(25));
	vector.Insert(vector.Begin(), ContainerTestSimple::Create(13));
	vector.PopBack();
	vector.PushBack(ContainerTestSimple::Create(23));
	vector.Erase(vector.Begin());
	vector.Insert(vector.Begin(), ContainerTestSimple::Create(15));

	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(vector.Begin(), vector.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT(!vector.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Front(), ContainerTestSimple::Create(15));
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Back(), ContainerTestSimple::Create(23));

	SEOUL_UNITTESTING_ASSERT_EQUAL(1, vector.Remove(ContainerTestSimple::Create(23)));
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(vector.Begin(), vector.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT(!vector.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Front(), ContainerTestSimple::Create(15));
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Back(), ContainerTestSimple::Create(15));

	SEOUL_UNITTESTING_ASSERT_EQUAL(1, vector.Remove(ContainerTestSimple::Create(15)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Begin(), vector.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT(vector.IsEmpty());

	for (Int i = 0; i < 5; ++i)
	{
		vector.PushBack(ContainerTestSimple::Create(5));
		vector.Insert(vector.Begin(), ContainerTestSimple::Create(5));
		vector.Erase(vector.Begin());
		vector.PopBack();
		vector.PushBack(ContainerTestSimple::Create(5));
	}
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(vector.Begin(), vector.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT(!vector.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Front(), ContainerTestSimple::Create(5));
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Back(), ContainerTestSimple::Create(5));

	SEOUL_UNITTESTING_ASSERT_EQUAL(5, vector.RemoveIf(ContainerTestSimpleAlwaysTrueFunctor));
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Begin(), vector.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT(vector.IsEmpty());

	for (Int i = 0; i < 5; ++i)
	{
		vector.PushBack(ContainerTestSimple::Create(i));
		vector.Insert(vector.Begin(), ContainerTestSimple::Create(i));
	}
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(vector.Begin(), vector.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(10, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT(!vector.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Front(), ContainerTestSimple::Create(4));
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Back(), ContainerTestSimple::Create(4));

	SEOUL_UNITTESTING_ASSERT_EQUAL(2, vector.Remove(ContainerTestSimple::Create(0)));
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(vector.Begin(), vector.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(8, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT(!vector.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Front(), ContainerTestSimple::Create(4));
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Back(), ContainerTestSimple::Create(4));

	QuickSort(vector.Begin(), vector.End());
	auto iter = vector.Begin();
	for (Int i = 0; i < 4; ++i)
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(i + 1), *iter);
		++iter;
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(i + 1), *iter);
		iter++;
	}

	Reverse(vector.Begin(), vector.End());
	iter = vector.End();
	for (Int i = 0; i < 4; ++i)
	{
		iter--;
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(i + 1), *iter);
		--iter;
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(i + 1), *iter);
	}

	QuickSort(vector.Begin(), vector.End(), ContainerTestSimpleLessThan);
	iter = vector.Begin();
	for (Int i = 0; i < 4; ++i)
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(i + 1), *iter);
		++iter;
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(i + 1), *iter);
		iter++;
	}
}

void VectorTest::TestPushBackMoveBuiltin()
{
	UInt64 a = 12;
	UInt64 b = 3209;
	UInt64 c = 3090;

	Vector<UInt64, MemoryBudgets::TBDContainer> v;
	v.PushBack(RvalRef(a));
	v.PushBack(RvalRef(b));
	v.PushBack(RvalRef(c));

	SEOUL_UNITTESTING_ASSERT_EQUAL(12, a);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3209, b);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3090, c);

	SEOUL_UNITTESTING_ASSERT_EQUAL(12, v[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3209, v[1]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3090, v[2]);
}

void VectorTest::TestPushBackMoveComplex()
{
	ContainerTestComplex a = ContainerTestComplex(12);
	ContainerTestComplex b = ContainerTestComplex(3209);
	ContainerTestComplex c = ContainerTestComplex(3090);

	Vector<ContainerTestComplex, MemoryBudgets::TBDContainer> v;
	v.PushBack(RvalRef(a));
	v.PushBack(RvalRef(b));
	v.PushBack(RvalRef(c));

	SEOUL_UNITTESTING_ASSERT_EQUAL(5235, a.m_iVariableValue);
	SEOUL_UNITTESTING_ASSERT_EQUAL(33, a.m_iFixedValue);
	SEOUL_UNITTESTING_ASSERT_EQUAL(5235, b.m_iVariableValue);
	SEOUL_UNITTESTING_ASSERT_EQUAL(33, b.m_iFixedValue);
	SEOUL_UNITTESTING_ASSERT_EQUAL(5235, c.m_iVariableValue);
	SEOUL_UNITTESTING_ASSERT_EQUAL(33, c.m_iFixedValue);

	SEOUL_UNITTESTING_ASSERT_EQUAL(12, v[0].m_iVariableValue);
	SEOUL_UNITTESTING_ASSERT_EQUAL(33, v[0].m_iFixedValue);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3209, v[1].m_iVariableValue);
	SEOUL_UNITTESTING_ASSERT_EQUAL(33, v[1].m_iFixedValue);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3090, v[2].m_iVariableValue);
	SEOUL_UNITTESTING_ASSERT_EQUAL(33, v[2].m_iFixedValue);
}

void VectorTest::TestPushBackMoveSimple()
{
	ContainerTestSimple a = ContainerTestSimple::Create(12);
	ContainerTestSimple b = ContainerTestSimple::Create(3209);
	ContainerTestSimple c = ContainerTestSimple::Create(3090);

	Vector<ContainerTestSimple, MemoryBudgets::TBDContainer> v;
	v.PushBack(RvalRef(a));
	v.PushBack(RvalRef(b));
	v.PushBack(RvalRef(c));

	SEOUL_UNITTESTING_ASSERT_EQUAL(12, a.m_iA);
	SEOUL_UNITTESTING_ASSERT_EQUAL(33, a.m_B);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3209, b.m_iA);
	SEOUL_UNITTESTING_ASSERT_EQUAL(33, b.m_B);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3090, c.m_iA);
	SEOUL_UNITTESTING_ASSERT_EQUAL(33, c.m_B);

	SEOUL_UNITTESTING_ASSERT_EQUAL(12, v[0].m_iA);
	SEOUL_UNITTESTING_ASSERT_EQUAL(33, v[0].m_B);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3209, v[1].m_iA);
	SEOUL_UNITTESTING_ASSERT_EQUAL(33, v[1].m_B);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3090, v[2].m_iA);
	SEOUL_UNITTESTING_ASSERT_EQUAL(33, v[2].m_B);
}

void VectorTest::TestInsertMoveBuiltin()
{
	UInt64 a = 12;
	UInt64 b = 3209;
	UInt64 c = 3090;

	Vector<UInt64, MemoryBudgets::TBDContainer> v;
	v.Insert(v.Begin(), RvalRef(a));
	v.Insert(v.Begin() + 1, RvalRef(b));
	v.Insert(v.Begin(), RvalRef(c));

	SEOUL_UNITTESTING_ASSERT_EQUAL(12, a);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3209, b);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3090, c);

	SEOUL_UNITTESTING_ASSERT_EQUAL(12, v[1]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3209, v[2]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3090, v[0]);
}

void VectorTest::TestInsertMoveComplex()
{
	ContainerTestComplex a = ContainerTestComplex(12);
	ContainerTestComplex b = ContainerTestComplex(3209);
	ContainerTestComplex c = ContainerTestComplex(3090);

	Vector<ContainerTestComplex, MemoryBudgets::TBDContainer> v;
	v.Insert(v.Begin(), RvalRef(a));
	v.Insert(v.Begin() + 1, RvalRef(b));
	v.Insert(v.Begin(), RvalRef(c));

	SEOUL_UNITTESTING_ASSERT_EQUAL(5235, a.m_iVariableValue);
	SEOUL_UNITTESTING_ASSERT_EQUAL(33, a.m_iFixedValue);
	SEOUL_UNITTESTING_ASSERT_EQUAL(5235, b.m_iVariableValue);
	SEOUL_UNITTESTING_ASSERT_EQUAL(33, b.m_iFixedValue);
	SEOUL_UNITTESTING_ASSERT_EQUAL(5235, c.m_iVariableValue);
	SEOUL_UNITTESTING_ASSERT_EQUAL(33, c.m_iFixedValue);

	SEOUL_UNITTESTING_ASSERT_EQUAL(12, v[1].m_iVariableValue);
	SEOUL_UNITTESTING_ASSERT_EQUAL(33, v[1].m_iFixedValue);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3209, v[2].m_iVariableValue);
	SEOUL_UNITTESTING_ASSERT_EQUAL(33, v[2].m_iFixedValue);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3090, v[0].m_iVariableValue);
	SEOUL_UNITTESTING_ASSERT_EQUAL(33, v[0].m_iFixedValue);
}

void VectorTest::TestInsertMoveSimple()
{
	ContainerTestSimple a = ContainerTestSimple::Create(12);
	ContainerTestSimple b = ContainerTestSimple::Create(3209);
	ContainerTestSimple c = ContainerTestSimple::Create(3090);

	Vector<ContainerTestSimple, MemoryBudgets::TBDContainer> v;
	v.Insert(v.Begin(), RvalRef(a));
	v.Insert(v.Begin() + 1, RvalRef(b));
	v.Insert(v.Begin(), RvalRef(c));

	SEOUL_UNITTESTING_ASSERT_EQUAL(12, a.m_iA);
	SEOUL_UNITTESTING_ASSERT_EQUAL(33, a.m_B);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3209, b.m_iA);
	SEOUL_UNITTESTING_ASSERT_EQUAL(33, b.m_B);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3090, c.m_iA);
	SEOUL_UNITTESTING_ASSERT_EQUAL(33, c.m_B);

	SEOUL_UNITTESTING_ASSERT_EQUAL(12, v[1].m_iA);
	SEOUL_UNITTESTING_ASSERT_EQUAL(33, v[1].m_B);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3209, v[2].m_iA);
	SEOUL_UNITTESTING_ASSERT_EQUAL(33, v[2].m_B);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3090, v[0].m_iA);
	SEOUL_UNITTESTING_ASSERT_EQUAL(33, v[0].m_B);
}

// Test for a regression in the (relatively unlikely, but apparently more likely
// on iOS) event that the memory of two vectors are adjacent in memory and
// those buffers are assigned. If the buffer being assigned is empty, it could
// incorrectly be identified as a "this" assignment and cause the assignment to
// be skipped.
void VectorTest::TestAssignRegressBuiltin()
{
	Vector<UInt64, MemoryBudgets::TBDContainer> v;
	v.PushBack(12);
	v.PushBack(3209);
	v.PushBack(3090);

	// Now create a hacked up secondary buffer that has
	// a buffer that is exactly at the end of the above buffer.
	Vector<UInt64, MemoryBudgets::TBDContainer> v2;
	auto pEnd = v.End();
	*((Vector<UInt64, MemoryBudgets::TBDContainer>::ValueType**)&v2 + 0) = pEnd;
	*((Vector<UInt64, MemoryBudgets::TBDContainer>::ValueType**)&v2 + 1) = pEnd;
	*((Vector<UInt64, MemoryBudgets::TBDContainer>::ValueType**)&v2 + 2) = pEnd + 3;

	// Now assign - v should now be empty.
	auto const vBack = v;
	v = v2;
	SEOUL_UNITTESTING_ASSERT(v.IsEmpty());
	v = vBack;

	v.Assign(v2.Begin(), v2.End());
	SEOUL_UNITTESTING_ASSERT(v.IsEmpty());
	v = vBack;

	// Very important - replace the pointers with nullptr or will crash trying to deallocate
	// memory it doesn't own.
	*((Vector<UInt64, MemoryBudgets::TBDContainer>::ValueType**)&v2 + 0) = nullptr;
	*((Vector<UInt64, MemoryBudgets::TBDContainer>::ValueType**)&v2 + 1) = nullptr;
	*((Vector<UInt64, MemoryBudgets::TBDContainer>::ValueType**)&v2 + 2) = nullptr;
}

void VectorTest::TestAssignRegressComplex()
{
	Vector<ContainerTestComplex, MemoryBudgets::TBDContainer> v;
	v.PushBack(ContainerTestComplex(12));
	v.PushBack(ContainerTestComplex(3209));
	v.PushBack(ContainerTestComplex(3090));

	// Now create a hacked up secondary buffer that has
	// a buffer that is exactly at the end of the above buffer.
	Vector<ContainerTestComplex, MemoryBudgets::TBDContainer> v2;
	auto pEnd = v.End();
	*((Vector<ContainerTestComplex, MemoryBudgets::TBDContainer>::ValueType**)&v2 + 0) = pEnd;
	*((Vector<ContainerTestComplex, MemoryBudgets::TBDContainer>::ValueType**)&v2 + 1) = pEnd;
	*((Vector<ContainerTestComplex, MemoryBudgets::TBDContainer>::ValueType**)&v2 + 2) = pEnd + 3;

	// Now assign - v should now be empty.
	auto const vBack = v;
	v = v2;
	SEOUL_UNITTESTING_ASSERT(v.IsEmpty());
	v = vBack;

	v.Assign(v2.Begin(), v2.End());
	SEOUL_UNITTESTING_ASSERT(v.IsEmpty());
	v = vBack;

	// Very important - replace the pointers with nullptr or will crash trying to deallocate
	// memory it doesn't own.
	*((Vector<ContainerTestComplex, MemoryBudgets::TBDContainer>::ValueType**)&v2 + 0) = nullptr;
	*((Vector<ContainerTestComplex, MemoryBudgets::TBDContainer>::ValueType**)&v2 + 1) = nullptr;
	*((Vector<ContainerTestComplex, MemoryBudgets::TBDContainer>::ValueType**)&v2 + 2) = nullptr;
}

void VectorTest::TestAssignRegressSimple()
{
	Vector<ContainerTestSimple, MemoryBudgets::TBDContainer> v;
	v.PushBack(ContainerTestSimple::Create(12));
	v.PushBack(ContainerTestSimple::Create(3209));
	v.PushBack(ContainerTestSimple::Create(3090));

	// Now create a hacked up secondary buffer that has
	// a buffer that is exactly at the end of the above buffer.
	Vector<ContainerTestSimple, MemoryBudgets::TBDContainer> v2;
	auto pEnd = v.End();
	*((Vector<ContainerTestSimple, MemoryBudgets::TBDContainer>::ValueType**)&v2 + 0) = pEnd;
	*((Vector<ContainerTestSimple, MemoryBudgets::TBDContainer>::ValueType**)&v2 + 1) = pEnd;
	*((Vector<ContainerTestSimple, MemoryBudgets::TBDContainer>::ValueType**)&v2 + 2) = pEnd + 3;

	// Now assign - v should now be empty.
	auto const vBack = v;
	v = v2;
	SEOUL_UNITTESTING_ASSERT(v.IsEmpty());
	v = vBack;

	v.Assign(v2.Begin(), v2.End());
	SEOUL_UNITTESTING_ASSERT(v.IsEmpty());
	v = vBack;

	// Very important - replace the pointers with nullptr or will crash trying to deallocate
	// memory it doesn't own.
	*((Vector<ContainerTestSimple, MemoryBudgets::TBDContainer>::ValueType**)&v2 + 0) = nullptr;
	*((Vector<ContainerTestSimple, MemoryBudgets::TBDContainer>::ValueType**)&v2 + 1) = nullptr;
	*((Vector<ContainerTestSimple, MemoryBudgets::TBDContainer>::ValueType**)&v2 + 2) = nullptr;
}

void VectorTest::TestSelfAssignBuiltin()
{
	Vector<UInt64, MemoryBudgets::TBDContainer> v;
	v.PushBack(12);
	v.PushBack(3209);
	v.PushBack(3090);

	auto const vBack = v;

	v = v;
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, v.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(12, v[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3209, v[1]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3090, v[2]);
	v = vBack;

	v.Assign(v.Begin(), v.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, v.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(12, v[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3209, v[1]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3090, v[2]);
	v = vBack;

	v.Assign(v.Begin() + 1, v.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, v.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(3209, v[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3090, v[1]);
	v = vBack;

	v.Assign(v.Begin(), v.End() - 1);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, v.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(12, v[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3209, v[1]);
	v = vBack;

	v.Assign(v.Begin() + 1, v.End() - 1);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, v.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(3209, v[0]);
	v = vBack;
}

void VectorTest::TestSelfAssignComplex()
{
	Vector<ContainerTestComplex, MemoryBudgets::TBDContainer> v;
	v.PushBack(ContainerTestComplex(12));
	v.PushBack(ContainerTestComplex(3209));
	v.PushBack(ContainerTestComplex(3090));

	auto const vBack = v;

	v = v;
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, v.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(12), v[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(3209), v[1]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(3090), v[2]);
	v = vBack;

	v.Assign(v.Begin(), v.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, v.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(12), v[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(3209), v[1]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(3090), v[2]);
	v = vBack;

	v.Assign(v.Begin() + 1, v.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, v.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(3209), v[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(3090), v[1]);
	v = vBack;

	v.Assign(v.Begin(), v.End() - 1);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, v.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(12), v[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(3209), v[1]);
	v = vBack;

	v.Assign(v.Begin() + 1, v.End() - 1);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, v.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(3209), v[0]);
	v = vBack;
}

void VectorTest::TestSelfAssignSimple()
{
	Vector<ContainerTestSimple, MemoryBudgets::TBDContainer> v;
	v.PushBack(ContainerTestSimple::Create(12));
	v.PushBack(ContainerTestSimple::Create(3209));
	v.PushBack(ContainerTestSimple::Create(3090));

	auto const vBack = v;

	v = v;
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, v.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(12), v[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(3209), v[1]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(3090), v[2]);
	v = vBack;

	v.Assign(v.Begin(), v.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, v.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(12), v[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(3209), v[1]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(3090), v[2]);
	v = vBack;

	v.Assign(v.Begin() + 1, v.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, v.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(3209), v[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(3090), v[1]);
	v = vBack;

	v.Assign(v.Begin(), v.End() - 1);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, v.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(12), v[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(3209), v[1]);
	v = vBack;

	v.Assign(v.Begin() + 1, v.End() - 1);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, v.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(3209), v[0]);
	v = vBack;
}

// Regression tests for the case where Remove() is called on the vector
// itself, which could result in erroneous removals (e.g. vector of 2 elements,
// passed with v[0], this would remove all elements from the vector).
void VectorTest::TestRemoveRegressionBuiltin()
{
	Vector<Int> v;
	v.PushBack(1);
	v.PushBack(2);

	SEOUL_UNITTESTING_ASSERT_EQUAL(1, v.Remove(v[0]));
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, v.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, v[0]);
}

void VectorTest::TestRemoveRegressionComplex()
{
	Vector<ContainerTestComplex> v;
	v.PushBack(ContainerTestComplex(1));
	v.PushBack(ContainerTestComplex(2));

	SEOUL_UNITTESTING_ASSERT_EQUAL(1, v.Remove(v[0]));
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, v.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(2), v[0]);
}

void VectorTest::TestRemoveRegressionSimple()
{
	Vector<ContainerTestSimple> v;
	v.PushBack(ContainerTestSimple::Create(1));
	v.PushBack(ContainerTestSimple::Create(2));

	SEOUL_UNITTESTING_ASSERT_EQUAL(1, v.Remove(v[0]));
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, v.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(2), v[0]);
}

// Regression tests for the self assignment case of a "fill" style assignment,
// if passed with an element of the vector.
void VectorTest::TestSelfAssignRegressionBuiltin()
{
	Vector<Int> v;
	v.PushBack(1);
	v.PushBack(2);

	v.Assign(3u, v[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, v.GetSize());
	for (auto const& e : v) { SEOUL_UNITTESTING_ASSERT_EQUAL(1, e); }
}

void VectorTest::TestSelfAssignRegressionComplex()
{
	Vector<ContainerTestComplex> v;
	v.PushBack(ContainerTestComplex(1));
	v.PushBack(ContainerTestComplex(2));

	v.Assign(3u, v[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, v.GetSize());
	for (auto const& e : v) { SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(1), e); }
}

void VectorTest::TestSelfAssignRegressionSimple()
{
	Vector<ContainerTestSimple> v;
	v.PushBack(ContainerTestSimple::Create(1));
	v.PushBack(ContainerTestSimple::Create(2));

	v.Assign(3u, v[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, v.GetSize());
	for (auto const& e : v) { SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(1), e); }
}

void VectorTest::TestSelfFillRegressionBuiltin()
{
	Vector<Int> v;
	v.PushBack(1);
	v.PushBack(2);

	v.Fill(v[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, v.GetSize());
	for (auto const& e : v) { SEOUL_UNITTESTING_ASSERT_EQUAL(1, e); }
}

void VectorTest::TestSelfFillRegressionComplex()
{
	Vector<ContainerTestComplex> v;
	v.PushBack(ContainerTestComplex(1));
	v.PushBack(ContainerTestComplex(2));

	v.Fill(v[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, v.GetSize());
	for (auto const& e : v) { SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(1), e); }
}

void VectorTest::TestSelfFillRegressionSimple()
{
	Vector<ContainerTestSimple> v;
	v.PushBack(ContainerTestSimple::Create(1));
	v.PushBack(ContainerTestSimple::Create(2));

	v.Fill(v[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, v.GetSize());
	for (auto const& e : v) { SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(1), e); }
}

// Regression tests for the self insert case of a single
// value or repeated value.
void VectorTest::TestSelfInsertRegressionBuiltin()
{
	Vector<Int> v;
	v.PushBack(1);
	v.PushBack(2);

	v.Insert(v.Begin(), v[1]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, v.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, v[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, v[1]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, v[2]);

	v.Insert(v.Begin() + 1, 10u, v[2]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(13, v.GetSize());
	for (UInt32 i = 0u; i < v.GetSize(); ++i)
	{
		if (i < 11) { SEOUL_UNITTESTING_ASSERT_EQUAL(2, v[i]); }
		else if (11 == i) { SEOUL_UNITTESTING_ASSERT_EQUAL(1, v[i]); }
		else { SEOUL_UNITTESTING_ASSERT_EQUAL(2, v[i]); }
	}
}

void VectorTest::TestSelfInsertRegressionComplex()
{
	Vector<ContainerTestComplex> v;
	v.PushBack(ContainerTestComplex(1));
	v.PushBack(ContainerTestComplex(2));

	v.Insert(v.Begin(), v[1]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, v.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(2), v[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(1), v[1]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(2), v[2]);

	v.Insert(v.Begin() + 1, 10u, v[2]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(13, v.GetSize());
	for (UInt32 i = 0u; i < v.GetSize(); ++i)
	{
		if (i < 11) { SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(2), v[i]); }
		else if (11 == i) { SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(1), v[i]); }
		else { SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(2), v[i]); }
	}
}

void VectorTest::TestSelfInsertRegressionSimple()
{
	Vector<ContainerTestSimple> v;
	v.PushBack(ContainerTestSimple::Create(1));
	v.PushBack(ContainerTestSimple::Create(2));

	v.Insert(v.Begin(), v[1]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, v.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(2), v[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(1), v[1]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(2), v[2]);

	v.Insert(v.Begin() + 1, 10u, v[2]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(13, v.GetSize());
	for (UInt32 i = 0u; i < v.GetSize(); ++i)
	{
		if (i < 11) { SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(2), v[i]); }
		else if (11 == i) { SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(1), v[i]); }
		else { SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(2), v[i]); }
	}
}

void VectorTest::TestRemoveFirstInstanceBuiltin()
{
	Vector<UInt64, MemoryBudgets::TBDContainer> vector;
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Begin(), vector.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT(vector.IsEmpty());

	// Nop
	SEOUL_UNITTESTING_ASSERT(!vector.RemoveFirstInstance(UInt64(25)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Begin(), vector.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT(vector.IsEmpty());

	// Populate the vector.
	vector.PushBack(UInt64(25));
	vector.PushBack(UInt64(23));
	vector.PushBack(UInt64(25));
	vector.PushBack(UInt64(25));
	vector.PushBack(UInt64(17));

	// Removes.
	SEOUL_UNITTESTING_ASSERT(vector.RemoveFirstInstance(UInt64(25)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(4, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(23, vector[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(25, vector[1]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(25, vector[2]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(17, vector[3]);

	SEOUL_UNITTESTING_ASSERT(!vector.RemoveFirstInstance(UInt64(16)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(4, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(23, vector[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(25, vector[1]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(25, vector[2]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(17, vector[3]);

	SEOUL_UNITTESTING_ASSERT(vector.RemoveFirstInstance(UInt64(25)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(23, vector[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(25, vector[1]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(17, vector[2]);

	SEOUL_UNITTESTING_ASSERT(vector.RemoveFirstInstance(UInt64(17)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(23, vector[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(25, vector[1]);

	SEOUL_UNITTESTING_ASSERT(vector.RemoveFirstInstance(UInt64(23)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(25, vector[0]);

	SEOUL_UNITTESTING_ASSERT(vector.RemoveFirstInstance(UInt64(25)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT(vector.IsEmpty());
}

void VectorTest::TestRemoveFirstInstanceComplex()
{
	Vector<ContainerTestComplex, MemoryBudgets::TBDContainer> vector;
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Begin(), vector.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT(vector.IsEmpty());

	// Nop
	SEOUL_UNITTESTING_ASSERT(!vector.RemoveFirstInstance(ContainerTestComplex(25)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Begin(), vector.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT(vector.IsEmpty());

	// Populate the vector.
	vector.PushBack(ContainerTestComplex(25));
	vector.PushBack(ContainerTestComplex(23));
	vector.PushBack(ContainerTestComplex(25));
	vector.PushBack(ContainerTestComplex(25));
	vector.PushBack(ContainerTestComplex(17));

	// Removes.
	SEOUL_UNITTESTING_ASSERT(vector.RemoveFirstInstance(ContainerTestComplex(25)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(4, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(23), vector[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(25), vector[1]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(25), vector[2]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(17), vector[3]);

	SEOUL_UNITTESTING_ASSERT(!vector.RemoveFirstInstance(ContainerTestComplex(16)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(4, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(23), vector[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(25), vector[1]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(25), vector[2]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(17), vector[3]);

	SEOUL_UNITTESTING_ASSERT(vector.RemoveFirstInstance(ContainerTestComplex(25)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(23), vector[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(25), vector[1]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(17), vector[2]);

	SEOUL_UNITTESTING_ASSERT(vector.RemoveFirstInstance(ContainerTestComplex(17)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(23), vector[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(25), vector[1]);

	SEOUL_UNITTESTING_ASSERT(vector.RemoveFirstInstance(ContainerTestComplex(23)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(25), vector[0]);

	SEOUL_UNITTESTING_ASSERT(vector.RemoveFirstInstance(ContainerTestComplex(25)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT(vector.IsEmpty());
}

void VectorTest::TestRemoveFirstInstanceComplexCoerce()
{
	Vector<ContainerTestComplex, MemoryBudgets::TBDContainer> vector;
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Begin(), vector.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT(vector.IsEmpty());

	// Nop
	SEOUL_UNITTESTING_ASSERT(!vector.RemoveFirstInstance(25));
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Begin(), vector.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT(vector.IsEmpty());

	// Populate the vector.
	vector.PushBack(ContainerTestComplex(25));
	vector.PushBack(ContainerTestComplex(23));
	vector.PushBack(ContainerTestComplex(25));
	vector.PushBack(ContainerTestComplex(25));
	vector.PushBack(ContainerTestComplex(17));

	// Removes.
	SEOUL_UNITTESTING_ASSERT(vector.RemoveFirstInstance(25));
	SEOUL_UNITTESTING_ASSERT_EQUAL(4, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(23), vector[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(25), vector[1]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(25), vector[2]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(17), vector[3]);

	SEOUL_UNITTESTING_ASSERT(!vector.RemoveFirstInstance(16));
	SEOUL_UNITTESTING_ASSERT_EQUAL(4, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(23), vector[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(25), vector[1]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(25), vector[2]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(17), vector[3]);

	SEOUL_UNITTESTING_ASSERT(vector.RemoveFirstInstance(25));
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(23), vector[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(25), vector[1]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(17), vector[2]);

	SEOUL_UNITTESTING_ASSERT(vector.RemoveFirstInstance(17));
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(23), vector[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(25), vector[1]);

	SEOUL_UNITTESTING_ASSERT(vector.RemoveFirstInstance(23));
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(25), vector[0]);

	SEOUL_UNITTESTING_ASSERT(vector.RemoveFirstInstance(25));
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT(vector.IsEmpty());
}

void VectorTest::TestRemoveFirstInstanceSimple()
{
	Vector<ContainerTestSimple, MemoryBudgets::TBDContainer> vector;
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Begin(), vector.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT(vector.IsEmpty());

	// Nop
	SEOUL_UNITTESTING_ASSERT(!vector.RemoveFirstInstance(ContainerTestSimple::Create(25)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Begin(), vector.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT(vector.IsEmpty());

	// Populate the vector.
	vector.PushBack(ContainerTestSimple::Create(25));
	vector.PushBack(ContainerTestSimple::Create(23));
	vector.PushBack(ContainerTestSimple::Create(25));
	vector.PushBack(ContainerTestSimple::Create(25));
	vector.PushBack(ContainerTestSimple::Create(17));

	// Removes.
	SEOUL_UNITTESTING_ASSERT(vector.RemoveFirstInstance(ContainerTestSimple::Create(25)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(4, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(23), vector[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(25), vector[1]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(25), vector[2]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(17), vector[3]);

	SEOUL_UNITTESTING_ASSERT(!vector.RemoveFirstInstance(ContainerTestSimple::Create(16)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(4, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(23), vector[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(25), vector[1]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(25), vector[2]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(17), vector[3]);

	SEOUL_UNITTESTING_ASSERT(vector.RemoveFirstInstance(ContainerTestSimple::Create(25)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(23), vector[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(25), vector[1]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(17), vector[2]);

	SEOUL_UNITTESTING_ASSERT(vector.RemoveFirstInstance(ContainerTestSimple::Create(17)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(23), vector[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(25), vector[1]);

	SEOUL_UNITTESTING_ASSERT(vector.RemoveFirstInstance(ContainerTestSimple::Create(23)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(25), vector[0]);

	SEOUL_UNITTESTING_ASSERT(vector.RemoveFirstInstance(ContainerTestSimple::Create(25)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT(vector.IsEmpty());
}

void VectorTest::TestRemoveCountBuiltin()
{
	Vector<Int, MemoryBudgets::TBDContainer> vector;
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Begin(), vector.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT(vector.IsEmpty());

	// Populate the vector.
	vector.PushBack(25);
	vector.PushBack(23);
	vector.PushBack(25);
	vector.PushBack(25);
	vector.PushBack(17);

	// Remove and test counts.
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, vector.Remove(25));
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, vector.Remove(17));
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, vector.Remove(23));
	SEOUL_UNITTESTING_ASSERT(vector.IsEmpty());
}

void VectorTest::TestRemoveCountComplex()
{
	Vector<ContainerTestComplex, MemoryBudgets::TBDContainer> vector;
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Begin(), vector.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT(vector.IsEmpty());

	// Populate the vector.
	vector.PushBack(ContainerTestComplex(25));
	vector.PushBack(ContainerTestComplex(23));
	vector.PushBack(ContainerTestComplex(25));
	vector.PushBack(ContainerTestComplex(25));
	vector.PushBack(ContainerTestComplex(17));

	// Remove and test counts.
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, vector.Remove(25));
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, vector.Remove(17));
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, vector.Remove(23));
	SEOUL_UNITTESTING_ASSERT(vector.IsEmpty());
}

void VectorTest::TestRemoveCountSimple()
{
	Vector<ContainerTestSimple, MemoryBudgets::TBDContainer> vector;
	SEOUL_UNITTESTING_ASSERT_EQUAL(vector.Begin(), vector.End());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vector.GetSize());
	SEOUL_UNITTESTING_ASSERT(vector.IsEmpty());

	// Populate the vector.
	vector.PushBack(ContainerTestSimple::Create(25));
	vector.PushBack(ContainerTestSimple::Create(23));
	vector.PushBack(ContainerTestSimple::Create(25));
	vector.PushBack(ContainerTestSimple::Create(25));
	vector.PushBack(ContainerTestSimple::Create(17));

	// Remove and test counts.
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, vector.Remove(ContainerTestSimple::Create(25)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, vector.Remove(ContainerTestSimple::Create(17)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, vector.Remove(ContainerTestSimple::Create(23)));
	SEOUL_UNITTESTING_ASSERT(vector.IsEmpty());
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
