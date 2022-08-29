/**
 * \file AlgorithmsTest.cpp
 * \brief Unit test header file for global utilities
 * defined in Algorithms.h
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Algorithms.h"
#include "AlgorithmsTest.h"
#include "ContainerTestUtil.h"
#include "FakeRandom.h"
#include "ReflectionDefine.h"
#include "UnitTesting.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(AlgorithmsTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestContains)
	SEOUL_METHOD(TestContainsFromBack)
	SEOUL_METHOD(TestCopy)
	SEOUL_METHOD(TestCopyBackward)
	SEOUL_METHOD(TestDestroyRange)
	SEOUL_METHOD(TestFill)
	SEOUL_METHOD(TestFind)
	SEOUL_METHOD(TestFindIf)
	SEOUL_METHOD(TestFindFromBack)
	SEOUL_METHOD(TestLowerBound)
	SEOUL_METHOD(TestRandomShuffle)
	SEOUL_METHOD(TestReverse)
	SEOUL_METHOD(TestRotate)
	SEOUL_METHOD(TestSort)
	SEOUL_METHOD(TestSwap)
	SEOUL_METHOD(TestSwapRanges)
	SEOUL_METHOD(TestUninitializedCopy)
	SEOUL_METHOD(TestUninitializedCopyBackward)
	SEOUL_METHOD(TestUninitializedFill)
	SEOUL_METHOD(TestUninitializedMove)
	SEOUL_METHOD(TestUpperBound)
	SEOUL_METHOD(TestZeroFillSimple)
SEOUL_END_TYPE()

void AlgorithmsTest::TestContains()
{
	Int aValues[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };

	// We deliberately exclude the endpoints in the begin/end.
	Int const* pBegin = &aValues[1];
	Int const* pEnd = &aValues[SEOUL_ARRAY_COUNT(aValues) - 1];
	SEOUL_UNITTESTING_ASSERT(Contains(pBegin, pEnd, 1));
	SEOUL_UNITTESTING_ASSERT(Contains(pBegin, pEnd, 5));
	SEOUL_UNITTESTING_ASSERT(Contains(pBegin, pEnd, 8));

	// Endpoints, don't find.
	SEOUL_UNITTESTING_ASSERT(!Contains(pBegin, pEnd, 0));
	SEOUL_UNITTESTING_ASSERT(!Contains(pBegin, pEnd, 9));
}

void AlgorithmsTest::TestContainsFromBack()
{
	Int aValues[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };

	// We deliberately exclude the endpoints in the begin/end.
	Int const* pBegin = &aValues[1];
	Int const* pEnd = &aValues[SEOUL_ARRAY_COUNT(aValues) - 1];
	SEOUL_UNITTESTING_ASSERT(ContainsFromBack(pBegin, pEnd, 1));
	SEOUL_UNITTESTING_ASSERT(ContainsFromBack(pBegin, pEnd, 5));
	SEOUL_UNITTESTING_ASSERT(ContainsFromBack(pBegin, pEnd, 8));

	// Endpoints, don't find.
	SEOUL_UNITTESTING_ASSERT(!ContainsFromBack(pBegin, pEnd, 0));
	SEOUL_UNITTESTING_ASSERT(!ContainsFromBack(pBegin, pEnd, 9));
}

void AlgorithmsTest::TestCopy()
{
	// Builtin
	{
		Int const aExpectedValues[] = { 1, 2, 3, 3, };
		Int aValues[] = { 0, 1, 2, 3, };
		size_t const zCount = SEOUL_ARRAY_COUNT(aValues);

		// Make sure copy handles overlapping ranges -
		// this is allowed, as long as out != begin.
		Int const* pOut = Copy(&aValues[1], &aValues[zCount], &aValues[0]);
		SEOUL_UNITTESTING_ASSERT_EQUAL(&aValues[zCount - 1], pOut);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(aValues, aExpectedValues, sizeof(aExpectedValues)));
	}

	// Complex
	{
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
			ContainerTestComplex const aExpectedValues[] = { ContainerTestComplex(1), ContainerTestComplex(2), ContainerTestComplex(3), ContainerTestComplex(3), };
			ContainerTestComplex aValues[] = { ContainerTestComplex(0), ContainerTestComplex(1), ContainerTestComplex(2), ContainerTestComplex(3), };
			size_t const zCount = SEOUL_ARRAY_COUNT(aValues);
			SEOUL_UNITTESTING_ASSERT_EQUAL(zCount * 2, ContainerTestComplex::s_iCount);

			// Make sure copy handles overlapping ranges -
			// this is allowed, as long as out != begin.
			ContainerTestComplex const* pOut = Copy(&aValues[1], &aValues[zCount], &aValues[0]);
			SEOUL_UNITTESTING_ASSERT_EQUAL(zCount * 2, ContainerTestComplex::s_iCount);
			SEOUL_UNITTESTING_ASSERT_EQUAL(&aValues[zCount - 1], pOut);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(aValues, aExpectedValues, sizeof(aExpectedValues)));
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
	}

	// Simple
	{
		ContainerTestSimple const aExpectedValues[] = { ContainerTestSimple::Create(1), ContainerTestSimple::Create(2), ContainerTestSimple::Create(3), ContainerTestSimple::Create(3), };
		ContainerTestSimple aValues[] = { ContainerTestSimple::Create(0), ContainerTestSimple::Create(1), ContainerTestSimple::Create(2), ContainerTestSimple::Create(3), };
		size_t const zCount = SEOUL_ARRAY_COUNT(aValues);

		// Make sure copy handles overlapping ranges -
		// this is allowed, as long as out != begin.
		ContainerTestSimple const* pOut = Copy(&aValues[1], &aValues[zCount], &aValues[0]);
		SEOUL_UNITTESTING_ASSERT_EQUAL(&aValues[zCount - 1], pOut);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(aValues, aExpectedValues, sizeof(aExpectedValues)));
	}
}

void AlgorithmsTest::TestCopyBackward()
{
	// Builtin
	{
		Int const aExpectedValues[] = { 1, 1, 2, 3, };
		Int aValues[] = { 1, 2, 3, 0, };
		size_t const zCount = SEOUL_ARRAY_COUNT(aValues);

		// Make sure copy backward handles overlapping ranges -
		// this is allowed, as long as out != end.
		Int const* pOut = CopyBackward(&aValues[0], &aValues[zCount - 1], &aValues[zCount]);
		SEOUL_UNITTESTING_ASSERT_EQUAL(&aValues[1], pOut);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(aValues, aExpectedValues, sizeof(aExpectedValues)));
	}

	// Complex
	{
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
			ContainerTestComplex const aExpectedValues[] = { ContainerTestComplex(1), ContainerTestComplex(1), ContainerTestComplex(2), ContainerTestComplex(3), };
			ContainerTestComplex aValues[] = { ContainerTestComplex(1), ContainerTestComplex(2), ContainerTestComplex(3), ContainerTestComplex(0), };
			size_t const zCount = SEOUL_ARRAY_COUNT(aValues);
			SEOUL_UNITTESTING_ASSERT_EQUAL(zCount * 2, ContainerTestComplex::s_iCount);

			// Make sure copy backward handles overlapping ranges -
			// this is allowed, as long as out != end.
			ContainerTestComplex const* pOut = CopyBackward(&aValues[0], &aValues[zCount - 1], &aValues[zCount]);
			SEOUL_UNITTESTING_ASSERT_EQUAL(zCount * 2, ContainerTestComplex::s_iCount);
			SEOUL_UNITTESTING_ASSERT_EQUAL(&aValues[1], pOut);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(aValues, aExpectedValues, sizeof(aExpectedValues)));
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
	}

	// Simple
	{
		ContainerTestSimple const aExpectedValues[] = { ContainerTestSimple::Create(1), ContainerTestSimple::Create(1), ContainerTestSimple::Create(2), ContainerTestSimple::Create(3), };
		ContainerTestSimple aValues[] = { ContainerTestSimple::Create(1), ContainerTestSimple::Create(2), ContainerTestSimple::Create(3), ContainerTestSimple::Create(0), };
		size_t const zCount = SEOUL_ARRAY_COUNT(aValues);

		// Make sure copy backward handles overlapping ranges -
		// this is allowed, as long as out != end.
		ContainerTestSimple const* pOut = CopyBackward(&aValues[0], &aValues[zCount - 1], &aValues[zCount]);
		SEOUL_UNITTESTING_ASSERT_EQUAL(&aValues[1], pOut);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(aValues, aExpectedValues, sizeof(aExpectedValues)));
	}
}

// We're lying - this structure gets the CanMemCpy<> specialization,
// but actually has a destructor. We use this
// in the TestDestroyRange test to make sure the destructor
// is not actually invoked.
struct AlgorithmsTestTestDestroyRangeNotReallySimple
{
	Int m_iA;
	Byte m_B;

	~AlgorithmsTestTestDestroyRangeNotReallySimple()
	{
		SEOUL_UNITTESTING_ASSERT_MESSAGE(false, "Destructor invocation on Simple in DestroyRange test.");
	}

	static AlgorithmsTestTestDestroyRangeNotReallySimple Create(Int iA)
	{
		AlgorithmsTestTestDestroyRangeNotReallySimple ret;
		ret.m_iA = iA;
		ret.m_B = 33;
		return ret;
	}
};
template <> struct CanMemCpy<AlgorithmsTestTestDestroyRangeNotReallySimple> { static const Bool Value = true; };

SEOUL_BEGIN_TYPE(AlgorithmsTestTestDestroyRangeNotReallySimple)
	SEOUL_PROPERTY_N("A", m_iA)
	SEOUL_PROPERTY_N("B", m_B)
SEOUL_END_TYPE()

void AlgorithmsTest::TestDestroyRange()
{
	// Complex
	{
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
			ContainerTestComplex aExpectedValues[] = { ContainerTestComplex(-1), ContainerTestComplex(1), ContainerTestComplex(1), ContainerTestComplex(2), ContainerTestComplex(3), ContainerTestComplex(-1) };
			size_t const zCount = SEOUL_ARRAY_COUNT(aExpectedValues);

			// Set the endpoints specially.
			aExpectedValues[0].m_iFixedValue = -1;
			aExpectedValues[zCount - 1].m_iFixedValue = -1;

			SEOUL_UNITTESTING_ASSERT_EQUAL(zCount, ContainerTestComplex::s_iCount);
			size_t const zSizeInBytes = zCount * sizeof(ContainerTestComplex);
			ContainerTestComplex* pValues = (ContainerTestComplex*)MemoryManager::AllocateAligned(
				zSizeInBytes,
				MemoryBudgets::Developer,
				SEOUL_ALIGN_OF(ContainerTestComplex));
			SEOUL_UNITTESTING_ASSERT_EQUAL(zCount, ContainerTestComplex::s_iCount);

			// Clear the memory, we use this for fence posting.
			// In the case of Complex, this also checks if UninitializedCopy
			// is incorrectly invoking the assignment operator.
			memset(pValues, 0xFF, zSizeInBytes);

			// Do the copy to the middle.
			ContainerTestComplex const* pBegin = &aExpectedValues[1];
			ContainerTestComplex const* pEnd = &aExpectedValues[zCount - 1];
			ContainerTestComplex const* pOut = UninitializedCopy(pBegin, pEnd, pValues + 1);
			SEOUL_UNITTESTING_ASSERT_EQUAL(zCount + (zCount - 2), ContainerTestComplex::s_iCount);
			SEOUL_UNITTESTING_ASSERT_EQUAL(pValues + zCount - 1, pOut);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(pValues, aExpectedValues, sizeof(aExpectedValues)));

			// Destroy range test, make sure ~ is invoked on the sub range, but no more.
			DestroyRange(pValues + 1, pValues + zCount - 1);
			SEOUL_UNITTESTING_ASSERT_EQUAL(zCount, ContainerTestComplex::s_iCount);

			MemoryManager::Deallocate(pValues);
			pValues = nullptr;

			// Restore the endpoints prior to destruction.
			aExpectedValues[0].m_iFixedValue = 33;
			aExpectedValues[zCount - 1].m_iFixedValue = 33;
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
	}

	// Simple
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);

		size_t zCount = 6u;
		size_t const zSizeInBytes = zCount * sizeof(AlgorithmsTestTestDestroyRangeNotReallySimple);
		AlgorithmsTestTestDestroyRangeNotReallySimple* pExpectedValues =
			(AlgorithmsTestTestDestroyRangeNotReallySimple*)MemoryManager::AllocateAligned(
			zSizeInBytes,
			MemoryBudgets::Developer,
			SEOUL_ALIGN_OF(AlgorithmsTestTestDestroyRangeNotReallySimple));
		memset(pExpectedValues, 0xFF, zSizeInBytes);
		pExpectedValues[1].m_iA = 1;
		pExpectedValues[1].m_B = 33;
		pExpectedValues[2].m_iA = 1;
		pExpectedValues[2].m_B = 33;
		pExpectedValues[3].m_iA = 2;
		pExpectedValues[3].m_B = 33;
		pExpectedValues[4].m_iA = 3;
		pExpectedValues[4].m_B = 33;

		AlgorithmsTestTestDestroyRangeNotReallySimple* pValues = (AlgorithmsTestTestDestroyRangeNotReallySimple*)MemoryManager::AllocateAligned(
			zSizeInBytes,
			MemoryBudgets::Developer,
			SEOUL_ALIGN_OF(AlgorithmsTestTestDestroyRangeNotReallySimple));

		// Clear the memory, we use this for fence posting.
		// In the case of Complex, this also checks if UninitializedCopy
		// is incorrectly invoking the assignment operator.
		memset(pValues, 0xFF, zSizeInBytes);

		// Do the copy to the middle.
		AlgorithmsTestTestDestroyRangeNotReallySimple const* pBegin = &pExpectedValues[1];
		AlgorithmsTestTestDestroyRangeNotReallySimple const* pEnd = &pExpectedValues[zCount - 1];
		AlgorithmsTestTestDestroyRangeNotReallySimple const* pOut = UninitializedCopy(pBegin, pEnd, pValues + 1);
		SEOUL_UNITTESTING_ASSERT_EQUAL(pValues + zCount - 1, pOut);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(pValues, pExpectedValues, zSizeInBytes));

		// Destroy range test - should not actually invoke the destructor.
		DestroyRange(pValues + 1, pValues + zCount - 1);

		MemoryManager::Deallocate(pValues);
		pValues = nullptr;

		// Cleanup expected values.
		MemoryManager::Deallocate(pExpectedValues);
		pExpectedValues = nullptr;
	}
}

void AlgorithmsTest::TestFill()
{
	// Builtin
	{
		Int aValues[25] = { Int(0) };
		size_t const zCount = SEOUL_ARRAY_COUNT(aValues);

		// Make sure fill only fills the specified range.
		Int* pBegin = &aValues[1];
		Int* pEnd = &aValues[zCount - 1];
		Fill(pBegin, pEnd, Int(25));

		for (size_t i = 0; i < zCount; ++i)
		{
			if (0 == i || zCount - 1 == i)
			{
				SEOUL_UNITTESTING_ASSERT_EQUAL(Int(0), aValues[i]);
			}
			else
			{
				SEOUL_UNITTESTING_ASSERT_EQUAL(Int(25), aValues[i]);
			}
		}
	}

	// Complex
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
		{
			ContainerTestComplex aValues[25];
			aValues[0] = ContainerTestComplex(0);
			aValues[24] = ContainerTestComplex(0);
			SEOUL_UNITTESTING_ASSERT_EQUAL(25, ContainerTestComplex::s_iCount);
			size_t const zCount = SEOUL_ARRAY_COUNT(aValues);

			// Make sure fill only fills the specified range.
			ContainerTestComplex* pBegin = &aValues[1];
			ContainerTestComplex* pEnd = &aValues[zCount - 1];
			Fill(pBegin, pEnd, ContainerTestComplex(25));
			SEOUL_UNITTESTING_ASSERT_EQUAL(25, ContainerTestComplex::s_iCount);

			for (size_t i = 0; i < zCount; ++i)
			{
				if (0 == i || zCount - 1 == i)
				{
					SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(0), aValues[i]);
				}
				else
				{
					SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(25), aValues[i]);
				}
			}
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
	}

	// Simple
	{
		ContainerTestSimple aValues[25];
		aValues[0] = ContainerTestSimple::Create(0);
		aValues[24] = ContainerTestSimple::Create(0);
		size_t const zCount = SEOUL_ARRAY_COUNT(aValues);

		// Make sure fill only fills the specified range.
		ContainerTestSimple* pBegin = &aValues[1];
		ContainerTestSimple* pEnd = &aValues[zCount - 1];
		Fill(pBegin, pEnd, ContainerTestSimple::Create(25));

		for (size_t i = 0; i < zCount; ++i)
		{
			if (0 == i || zCount - 1 == i)
			{
				SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(0), aValues[i]);
			}
			else
			{
				SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(25), aValues[i]);
			}
		}
	}
}

static Bool AlgorithmsTestTestFindDoNotFind6(Int a, Int b) { if (a == 6 || b == 6) { return false; } else { return (a == b); } }
void AlgorithmsTest::TestFind()
{
	// Find, no predicate.
	{
		Int aValues[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };

		// We deliberately exclude the endpoints in the begin/end.
		Int const* pBegin = &aValues[1];
		Int const* pEnd = &aValues[SEOUL_ARRAY_COUNT(aValues) - 1];
		SEOUL_UNITTESTING_ASSERT_EQUAL(pBegin + 0, Find(pBegin, pEnd, 1));
		SEOUL_UNITTESTING_ASSERT_EQUAL(pBegin + 4, Find(pBegin, pEnd, 5));
		SEOUL_UNITTESTING_ASSERT_EQUAL(pBegin + 7, Find(pBegin, pEnd, 8));

		// Endpoints, don't find.
		SEOUL_UNITTESTING_ASSERT_EQUAL(pEnd, Find(pBegin, pEnd, 0));
		SEOUL_UNITTESTING_ASSERT_EQUAL(pEnd, Find(pBegin, pEnd, 9));
	}

	// Find, predicate.
	{
		Int aValues[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };

		// We deliberately exclude the endpoints in the begin/end.
		Int const* pBegin = &aValues[1];
		Int const* pEnd = &aValues[SEOUL_ARRAY_COUNT(aValues) - 1];
		SEOUL_UNITTESTING_ASSERT_EQUAL(pBegin + 0, Find(pBegin, pEnd, 1, AlgorithmsTestTestFindDoNotFind6));
		SEOUL_UNITTESTING_ASSERT_EQUAL(pBegin + 4, Find(pBegin, pEnd, 5, AlgorithmsTestTestFindDoNotFind6));
		SEOUL_UNITTESTING_ASSERT_EQUAL(pBegin + 7, Find(pBegin, pEnd, 8, AlgorithmsTestTestFindDoNotFind6));

		// Predicate should fail to find this.
		SEOUL_UNITTESTING_ASSERT_EQUAL(pEnd, Find(pBegin, pEnd, 6, AlgorithmsTestTestFindDoNotFind6));

		// Endpoints, don't find.
		SEOUL_UNITTESTING_ASSERT_EQUAL(pEnd, Find(pBegin, pEnd, 0, AlgorithmsTestTestFindDoNotFind6));
		SEOUL_UNITTESTING_ASSERT_EQUAL(pEnd, Find(pBegin, pEnd, 9, AlgorithmsTestTestFindDoNotFind6));
	}
}

template <int VALUE> static Bool AlgorithmsTestTestFindIf(Int a) { return (a == VALUE); }
static Bool AlgorithmsTestTestFindIfDoNotFind1(Int a) { if (a == 1) { return false; } else { return true; } }
static Bool AlgorithmsTestTestFindIfFail(Int a) { return false; }
void AlgorithmsTest::TestFindIf()
{
	Int aValues[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };

	// We deliberately exclude the endpoints in the begin/end.
	Int const* pBegin = &aValues[1];
	Int const* pEnd = &aValues[SEOUL_ARRAY_COUNT(aValues) - 1];
	SEOUL_UNITTESTING_ASSERT_EQUAL(pBegin + 0, FindIf(pBegin, pEnd, AlgorithmsTestTestFindIf<1>));
	SEOUL_UNITTESTING_ASSERT_EQUAL(pBegin + 4, FindIf(pBegin, pEnd, AlgorithmsTestTestFindIf<5>));
	SEOUL_UNITTESTING_ASSERT_EQUAL(pBegin + 7, FindIf(pBegin, pEnd, AlgorithmsTestTestFindIf<8>));

	// Predicate should fail to find this.
	SEOUL_UNITTESTING_ASSERT_EQUAL(pEnd, FindIf(pBegin, pEnd, AlgorithmsTestTestFindIfFail));

	// Should find 2 (skip 1).
	SEOUL_UNITTESTING_ASSERT_EQUAL(pBegin + 1, FindIf(pBegin, pEnd, AlgorithmsTestTestFindIfDoNotFind1));

	// Endpoints, don't find.
	SEOUL_UNITTESTING_ASSERT_EQUAL(pEnd, FindIf(pBegin, pEnd, AlgorithmsTestTestFindIf<0>));
	SEOUL_UNITTESTING_ASSERT_EQUAL(pEnd, FindIf(pBegin, pEnd, AlgorithmsTestTestFindIf<9>));
}

void AlgorithmsTest::TestFindFromBack()
{
	// FindFromBack, no predicate.
	{
		Int aValues[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 8, 9 };

		// We deliberately exclude the endpoints in the begin/end.
		Int const* pBegin = &aValues[1];
		Int const* pEnd = &aValues[SEOUL_ARRAY_COUNT(aValues) - 1];
		SEOUL_UNITTESTING_ASSERT_EQUAL(pBegin + 0, FindFromBack(pBegin, pEnd, 1));
		SEOUL_UNITTESTING_ASSERT_EQUAL(pBegin + 4, FindFromBack(pBegin, pEnd, 5));
		SEOUL_UNITTESTING_ASSERT_EQUAL(pBegin + 8, FindFromBack(pBegin, pEnd, 8));

		// Endpoints, don't find.
		SEOUL_UNITTESTING_ASSERT_EQUAL(pEnd, FindFromBack(pBegin, pEnd, 0));
		SEOUL_UNITTESTING_ASSERT_EQUAL(pEnd, FindFromBack(pBegin, pEnd, 9));
	}

	// FindFromBack, predicate.
	{
		Int aValues[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 8, 9 };

		// We deliberately exclude the endpoints in the begin/end.
		Int const* pBegin = &aValues[1];
		Int const* pEnd = &aValues[SEOUL_ARRAY_COUNT(aValues) - 1];
		SEOUL_UNITTESTING_ASSERT_EQUAL(pBegin + 0, FindFromBack(pBegin, pEnd, 1, AlgorithmsTestTestFindDoNotFind6));
		SEOUL_UNITTESTING_ASSERT_EQUAL(pBegin + 4, FindFromBack(pBegin, pEnd, 5, AlgorithmsTestTestFindDoNotFind6));
		SEOUL_UNITTESTING_ASSERT_EQUAL(pBegin + 8, FindFromBack(pBegin, pEnd, 8, AlgorithmsTestTestFindDoNotFind6));

		// Predicate should fail to find this.
		SEOUL_UNITTESTING_ASSERT_EQUAL(pEnd, FindFromBack(pBegin, pEnd, 6, AlgorithmsTestTestFindDoNotFind6));

		// Endpoints, don't find.
		SEOUL_UNITTESTING_ASSERT_EQUAL(pEnd, FindFromBack(pBegin, pEnd, 0, AlgorithmsTestTestFindDoNotFind6));
		SEOUL_UNITTESTING_ASSERT_EQUAL(pEnd, FindFromBack(pBegin, pEnd, 9, AlgorithmsTestTestFindDoNotFind6));
	}
}

template <typename T>
struct TestLowerBoundPred SEOUL_SEALED
{
	Bool operator()(const T& a, const T& b) const
	{
		// Reverse sort.
		return (b < a);
	}
};
void AlgorithmsTest::TestLowerBound()
{
	// No predicate.
	{
		// Builtin
		{
			Int const aOrigValues[] = { 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 };
			size_t const zCount = SEOUL_ARRAY_COUNT(aOrigValues);

			FixedArray<Int, zCount> aValues(aOrigValues);
			SEOUL_UNITTESTING_ASSERT_EQUAL(9, *LowerBound(aValues.Begin(), aValues.End(), 4));
			SEOUL_UNITTESTING_ASSERT_EQUAL(aValues.End(), LowerBound(aValues.Begin(), aValues.End(), 20));
			SEOUL_UNITTESTING_ASSERT_EQUAL(aValues.Begin(), LowerBound(aValues.Begin(), aValues.End(), -1));
		}

		// Complex
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
			{
				ContainerTestComplex const aOrigValues[] = { ContainerTestComplex(9), ContainerTestComplex(8), ContainerTestComplex(7), ContainerTestComplex(6), ContainerTestComplex(5), ContainerTestComplex(4), ContainerTestComplex(3), ContainerTestComplex(2), ContainerTestComplex(1), ContainerTestComplex(0) };
				size_t const zCount = SEOUL_ARRAY_COUNT(aOrigValues);

				FixedArray<ContainerTestComplex, zCount> aValues(aOrigValues);
				SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(9), *LowerBound(aValues.Begin(), aValues.End(), ContainerTestComplex(4)));
				SEOUL_UNITTESTING_ASSERT_EQUAL(aValues.End(), LowerBound(aValues.Begin(), aValues.End(), ContainerTestComplex(20)));
				SEOUL_UNITTESTING_ASSERT_EQUAL(aValues.Begin(), LowerBound(aValues.Begin(), aValues.End(), ContainerTestComplex(-1)));
			}
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
		}

		// Simple
		{
			ContainerTestSimple const aOrigValues[] = { ContainerTestSimple::Create(9), ContainerTestSimple::Create(8), ContainerTestSimple::Create(7), ContainerTestSimple::Create(6), ContainerTestSimple::Create(5), ContainerTestSimple::Create(4), ContainerTestSimple::Create(3), ContainerTestSimple::Create(2), ContainerTestSimple::Create(1), ContainerTestSimple::Create(0) };
			size_t const zCount = SEOUL_ARRAY_COUNT(aOrigValues);

			FixedArray<ContainerTestSimple, zCount> aValues(aOrigValues);
			SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(9), *LowerBound(aValues.Begin(), aValues.End(), ContainerTestSimple::Create(4)));
			SEOUL_UNITTESTING_ASSERT_EQUAL(aValues.End(), LowerBound(aValues.Begin(), aValues.End(), ContainerTestSimple::Create(20)));
			SEOUL_UNITTESTING_ASSERT_EQUAL(aValues.Begin(), LowerBound(aValues.Begin(), aValues.End(), ContainerTestSimple::Create(-1)));
		}
	}

	// Predicate.
	{
		// Builtin
		{
			Int const aOrigValues[] = { 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 };
			size_t const zCount = SEOUL_ARRAY_COUNT(aOrigValues);

			FixedArray<Int, zCount> aValues(aOrigValues);
			SEOUL_UNITTESTING_ASSERT_EQUAL(4, *LowerBound(aValues.Begin(), aValues.End(), 4, TestLowerBoundPred<Int>()));
			SEOUL_UNITTESTING_ASSERT_EQUAL(aValues.Begin(), LowerBound(aValues.Begin(), aValues.End(), 20, TestLowerBoundPred<Int>()));
			SEOUL_UNITTESTING_ASSERT_EQUAL(aValues.End(), LowerBound(aValues.Begin(), aValues.End(), -1, TestLowerBoundPred<Int>()));
		}

		// Complex
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
			{
				ContainerTestComplex const aOrigValues[] = { ContainerTestComplex(9), ContainerTestComplex(8), ContainerTestComplex(7), ContainerTestComplex(6), ContainerTestComplex(5), ContainerTestComplex(4), ContainerTestComplex(3), ContainerTestComplex(2), ContainerTestComplex(1), ContainerTestComplex(0) };
				size_t const zCount = SEOUL_ARRAY_COUNT(aOrigValues);

				FixedArray<ContainerTestComplex, zCount> aValues(aOrigValues);
				SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(4), *LowerBound(aValues.Begin(), aValues.End(), ContainerTestComplex(4), TestLowerBoundPred<ContainerTestComplex>()));
				SEOUL_UNITTESTING_ASSERT_EQUAL(aValues.Begin(), LowerBound(aValues.Begin(), aValues.End(), ContainerTestComplex(20), TestLowerBoundPred<ContainerTestComplex>()));
				SEOUL_UNITTESTING_ASSERT_EQUAL(aValues.End(), LowerBound(aValues.Begin(), aValues.End(), ContainerTestComplex(-1), TestLowerBoundPred<ContainerTestComplex>()));
			}
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
		}

		// Simple
		{
			ContainerTestSimple const aOrigValues[] = { ContainerTestSimple::Create(9), ContainerTestSimple::Create(8), ContainerTestSimple::Create(7), ContainerTestSimple::Create(6), ContainerTestSimple::Create(5), ContainerTestSimple::Create(4), ContainerTestSimple::Create(3), ContainerTestSimple::Create(2), ContainerTestSimple::Create(1), ContainerTestSimple::Create(0) };
			size_t const zCount = SEOUL_ARRAY_COUNT(aOrigValues);

			FixedArray<ContainerTestSimple, zCount> aValues(aOrigValues);
			SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(4), *LowerBound(aValues.Begin(), aValues.End(), ContainerTestSimple::Create(4), TestLowerBoundPred<ContainerTestSimple>()));
			SEOUL_UNITTESTING_ASSERT_EQUAL(aValues.Begin(), LowerBound(aValues.Begin(), aValues.End(), ContainerTestSimple::Create(20), TestLowerBoundPred<ContainerTestSimple>()));
			SEOUL_UNITTESTING_ASSERT_EQUAL(aValues.End(), LowerBound(aValues.Begin(), aValues.End(), ContainerTestSimple::Create(-1), TestLowerBoundPred<ContainerTestSimple>()));
		}
	}
}

struct AlgorithmsTestTestRandomShuffleGen SEOUL_SEALED
{
	AlgorithmsTestTestRandomShuffleGen()
	{
	}

	FakeRandom random;
	ptrdiff_t operator()(ptrdiff_t n)
	{
		return (ptrdiff_t)(random.NextFloat32() * n);
	}
};
void AlgorithmsTest::TestRandomShuffle()
{
	// No generator.
	{
		// Builtin
		{
			Int const aOrigValues[] = { 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 };
			size_t const zCount = SEOUL_ARRAY_COUNT(aOrigValues);

			FixedArray<Int, zCount> aValues(aOrigValues);
			RandomShuffle(aValues.Begin(), aValues.End());
			SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(0, memcmp(aOrigValues, aValues.Begin(), aValues.GetSizeInBytes()));
		}

		// Complex
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
			{
				ContainerTestComplex const aOrigValues[] = { ContainerTestComplex(9), ContainerTestComplex(8), ContainerTestComplex(7), ContainerTestComplex(6), ContainerTestComplex(5), ContainerTestComplex(4), ContainerTestComplex(3), ContainerTestComplex(2), ContainerTestComplex(1), ContainerTestComplex(0) };
				size_t const zCount = SEOUL_ARRAY_COUNT(aOrigValues);

				FixedArray<ContainerTestComplex, zCount> aValues(aOrigValues);
				RandomShuffle(aValues.Begin(), aValues.End());
				SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(0, memcmp(aOrigValues, aValues.Begin(), aValues.GetSizeInBytes()));
			}
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
		}

		// Simple
		{
			ContainerTestSimple const aOrigValues[] = { ContainerTestSimple::Create(9), ContainerTestSimple::Create(8), ContainerTestSimple::Create(7), ContainerTestSimple::Create(6), ContainerTestSimple::Create(5), ContainerTestSimple::Create(4), ContainerTestSimple::Create(3), ContainerTestSimple::Create(2), ContainerTestSimple::Create(1), ContainerTestSimple::Create(0) };
			size_t const zCount = SEOUL_ARRAY_COUNT(aOrigValues);

			FixedArray<ContainerTestSimple, zCount> aValues(aOrigValues);
			RandomShuffle(aValues.Begin(), aValues.End());
			SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(0, memcmp(aOrigValues, aValues.Begin(), aValues.GetSizeInBytes()));
		}
	}

	// Generator
	{
		// Builtin
		{
			Int const aOrigValues[] = { 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 };
			size_t const zCount = SEOUL_ARRAY_COUNT(aOrigValues);
			AlgorithmsTestTestRandomShuffleGen generator;

			FixedArray<Int, zCount> aValues(aOrigValues);
			RandomShuffle(aValues.Begin(), aValues.End(), generator);
			SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(0, memcmp(aOrigValues, aValues.Begin(), aValues.GetSizeInBytes()));
		}

		// Complex
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
			{
				ContainerTestComplex const aOrigValues[] = { ContainerTestComplex(9), ContainerTestComplex(8), ContainerTestComplex(7), ContainerTestComplex(6), ContainerTestComplex(5), ContainerTestComplex(4), ContainerTestComplex(3), ContainerTestComplex(2), ContainerTestComplex(1), ContainerTestComplex(0) };
				size_t const zCount = SEOUL_ARRAY_COUNT(aOrigValues);
				AlgorithmsTestTestRandomShuffleGen generator;

				FixedArray<ContainerTestComplex, zCount> aValues(aOrigValues);
				RandomShuffle(aValues.Begin(), aValues.End(), generator);
				SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(0, memcmp(aOrigValues, aValues.Begin(), aValues.GetSizeInBytes()));
			}
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
		}

		// Simple
		{
			ContainerTestSimple const aOrigValues[] = { ContainerTestSimple::Create(9), ContainerTestSimple::Create(8), ContainerTestSimple::Create(7), ContainerTestSimple::Create(6), ContainerTestSimple::Create(5), ContainerTestSimple::Create(4), ContainerTestSimple::Create(3), ContainerTestSimple::Create(2), ContainerTestSimple::Create(1), ContainerTestSimple::Create(0) };
			size_t const zCount = SEOUL_ARRAY_COUNT(aOrigValues);
			AlgorithmsTestTestRandomShuffleGen generator;

			FixedArray<ContainerTestSimple, zCount> aValues(aOrigValues);
			RandomShuffle(aValues.Begin(), aValues.End(), generator);
			SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(0, memcmp(aOrigValues, aValues.Begin(), aValues.GetSizeInBytes()));
		}
	}
}

void AlgorithmsTest::TestReverse()
{
	// Builtin
	{
		Int const aExpectedValues[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
		Int const aOrigValues[] = { 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 };
		size_t const zCount = SEOUL_ARRAY_COUNT(aOrigValues);

		FixedArray<Int, zCount> aValues(aOrigValues);
		Reverse(aValues.Begin(), aValues.End());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(aExpectedValues, aValues.Begin(), aValues.GetSizeInBytes()));
	}

	// Complex
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
		{
			ContainerTestComplex const aExpectedValues[] = { ContainerTestComplex(0), ContainerTestComplex(1), ContainerTestComplex(2), ContainerTestComplex(3), ContainerTestComplex(4), ContainerTestComplex(5), ContainerTestComplex(6), ContainerTestComplex(7), ContainerTestComplex(8), ContainerTestComplex(9) };
			ContainerTestComplex const aOrigValues[] = { ContainerTestComplex(9), ContainerTestComplex(8), ContainerTestComplex(7), ContainerTestComplex(6), ContainerTestComplex(5), ContainerTestComplex(4), ContainerTestComplex(3), ContainerTestComplex(2), ContainerTestComplex(1), ContainerTestComplex(0) };
			size_t const zCount = SEOUL_ARRAY_COUNT(aOrigValues);

			FixedArray<ContainerTestComplex, zCount> aValues(aOrigValues);
			Reverse(aValues.Begin(), aValues.End());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(aExpectedValues, aValues.Begin(), aValues.GetSizeInBytes()));
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
	}

	// Simple
	{
		ContainerTestSimple const aExpectedValues[] = { ContainerTestSimple::Create(0), ContainerTestSimple::Create(1), ContainerTestSimple::Create(2), ContainerTestSimple::Create(3), ContainerTestSimple::Create(4), ContainerTestSimple::Create(5), ContainerTestSimple::Create(6), ContainerTestSimple::Create(7), ContainerTestSimple::Create(8), ContainerTestSimple::Create(9) };
		ContainerTestSimple const aOrigValues[] = { ContainerTestSimple::Create(9), ContainerTestSimple::Create(8), ContainerTestSimple::Create(7), ContainerTestSimple::Create(6), ContainerTestSimple::Create(5), ContainerTestSimple::Create(4), ContainerTestSimple::Create(3), ContainerTestSimple::Create(2), ContainerTestSimple::Create(1), ContainerTestSimple::Create(0) };
		size_t const zCount = SEOUL_ARRAY_COUNT(aOrigValues);

		FixedArray<ContainerTestSimple, zCount> aValues(aOrigValues);
		Reverse(aValues.Begin(), aValues.End());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(aExpectedValues, aValues.Begin(), aValues.GetSizeInBytes()));
	}
}

static Bool AlgorithmsTestTestSortBuiltin(Int a, Int b)
{
	a = (a == 9 ? -1 : a);
	b = (b == 9 ? -1 : b);
	return (a < b);
}
static Bool AlgorithmsTestTestSortBuiltinStablePredicate(Int a, Int b)
{
	a = (a == 2 ? 6 : a);
	b = (b == 2 ? 6 : b);
	return (a < b);
}
static Bool AlgorithmsTestTestSortComplex(ContainerTestComplex a, ContainerTestComplex b)
{
	a = (a == ContainerTestComplex(9) ? ContainerTestComplex(-1) : a);
	b = (b == ContainerTestComplex(9) ? ContainerTestComplex(-1) : b);
	return (a < b);
}
static Bool AlgorithmsTestTestSortComplexStablePredicate(ContainerTestComplex a, ContainerTestComplex b)
{
	a = (a == ContainerTestComplex(2) ? ContainerTestComplex(6) : a);
	b = (b == ContainerTestComplex(2) ? ContainerTestComplex(6) : b);
	return (a < b);
}
static Bool AlgorithmsTestTestSortSimple(ContainerTestSimple a, ContainerTestSimple b)
{
	a = (a == ContainerTestSimple::Create(9) ? ContainerTestSimple::Create(-1) : a);
	b = (b == ContainerTestSimple::Create(9) ? ContainerTestSimple::Create(-1) : b);
	return (a < b);
}
static Bool AlgorithmsTestTestSortSimpleStablePredicate(ContainerTestSimple a, ContainerTestSimple b)
{
	a = (a == ContainerTestSimple::Create(2) ? ContainerTestSimple::Create(6) : a);
	b = (b == ContainerTestSimple::Create(2) ? ContainerTestSimple::Create(6) : b);
	return (a < b);
}
void AlgorithmsTest::TestSort()
{
	// Builtin
	{
		Int const aExpectedValues[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
		Int const aExpectedPredicateValues[] = { 9, 0, 1, 2, 3, 4, 5, 6, 7, 8 };
		Int const aExpectedStablePredicateValues[] = { 0, 1, 3, 4, 5, 2, 6, 7, 8, 9 };
		Int const aOrigValues[] = { 8, 5, 1, 4, 3, 2, 6, 0, 7, 9 };
		size_t const zCount = SEOUL_ARRAY_COUNT(aOrigValues);

		// QuickSort, no predicate.
		{
			FixedArray<Int, zCount> aValues(aOrigValues);
			QuickSort(aValues.Begin(), aValues.End());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(aExpectedValues, aValues.Begin(), aValues.GetSizeInBytes()));
		}

		// QuickSort, predicate.
		{
			FixedArray<Int, zCount> aValues(aOrigValues);
			QuickSort(aValues.Begin(), aValues.End(), AlgorithmsTestTestSortBuiltin);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(aExpectedPredicateValues, aValues.Begin(), aValues.GetSizeInBytes()));
		}

		// Sort, no predicate.
		{
			FixedArray<Int, zCount> aValues(aOrigValues);
			Sort(aValues.Begin(), aValues.End());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(aExpectedValues, aValues.Begin(), aValues.GetSizeInBytes()));
		}

		// Sort, predicate.
		{
			FixedArray<Int, zCount> aValues(aOrigValues);
			QuickSort(aValues.Begin(), aValues.End(), AlgorithmsTestTestSortBuiltin);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(aExpectedPredicateValues, aValues.Begin(), aValues.GetSizeInBytes()));
		}

		// StableSort, no predicate.
		{
			FixedArray<Int, zCount> aValues(aOrigValues);
			StableSort(aValues.Begin(), aValues.End());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(aExpectedValues, aValues.Begin(), aValues.GetSizeInBytes()));
		}

		// StableSort, predicate.
		{
			FixedArray<Int, zCount> aValues(aOrigValues);
			QuickSort(aValues.Begin(), aValues.End(), AlgorithmsTestTestSortBuiltinStablePredicate);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(aExpectedStablePredicateValues, aValues.Begin(), aValues.GetSizeInBytes()));
		}
	}

	// Complex
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
		{
			ContainerTestComplex const aExpectedValues[] = { ContainerTestComplex(0), ContainerTestComplex(1), ContainerTestComplex(2), ContainerTestComplex(3), ContainerTestComplex(4), ContainerTestComplex(5), ContainerTestComplex(6), ContainerTestComplex(7), ContainerTestComplex(8), ContainerTestComplex(9) };
			ContainerTestComplex const aExpectedPredicateValues[] = { ContainerTestComplex(9), ContainerTestComplex(0), ContainerTestComplex(1), ContainerTestComplex(2), ContainerTestComplex(3), ContainerTestComplex(4), ContainerTestComplex(5), ContainerTestComplex(6), ContainerTestComplex(7), ContainerTestComplex(8) };
			ContainerTestComplex const aExpectedStablePredicateValues[] = { ContainerTestComplex(0), ContainerTestComplex(1), ContainerTestComplex(3), ContainerTestComplex(4), ContainerTestComplex(5), ContainerTestComplex(2), ContainerTestComplex(6), ContainerTestComplex(7), ContainerTestComplex(8), ContainerTestComplex(9) };
			ContainerTestComplex const aOrigValues[] = { ContainerTestComplex(8), ContainerTestComplex(5), ContainerTestComplex(1), ContainerTestComplex(4), ContainerTestComplex(3), ContainerTestComplex(2), ContainerTestComplex(6), ContainerTestComplex(0), ContainerTestComplex(7), ContainerTestComplex(9) };
			size_t const zCount = SEOUL_ARRAY_COUNT(aOrigValues);

			// QuickSort, no predicate.
			{
				FixedArray<ContainerTestComplex, zCount> aValues(aOrigValues);
				QuickSort(aValues.Begin(), aValues.End());
				SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(aExpectedValues, aValues.Begin(), aValues.GetSizeInBytes()));
			}

			// QuickSort, predicate.
			{
				FixedArray<ContainerTestComplex, zCount> aValues(aOrigValues);
				QuickSort(aValues.Begin(), aValues.End(), AlgorithmsTestTestSortComplex);
				SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(aExpectedPredicateValues, aValues.Begin(), aValues.GetSizeInBytes()));
			}

			// Sort, no predicate.
			{
				FixedArray<ContainerTestComplex, zCount> aValues(aOrigValues);
				Sort(aValues.Begin(), aValues.End());
				SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(aExpectedValues, aValues.Begin(), aValues.GetSizeInBytes()));
			}

			// Sort, predicate.
			{
				FixedArray<ContainerTestComplex, zCount> aValues(aOrigValues);
				QuickSort(aValues.Begin(), aValues.End(), AlgorithmsTestTestSortComplex);
				SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(aExpectedPredicateValues, aValues.Begin(), aValues.GetSizeInBytes()));
			}

			// StableSort, no predicate.
			{
				FixedArray<ContainerTestComplex, zCount> aValues(aOrigValues);
				StableSort(aValues.Begin(), aValues.End());
				SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(aExpectedValues, aValues.Begin(), aValues.GetSizeInBytes()));
			}

			// StableSort, predicate.
			{
				FixedArray<ContainerTestComplex, zCount> aValues(aOrigValues);
				StableSort(aValues.Begin(), aValues.End(), AlgorithmsTestTestSortComplexStablePredicate);
				SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(aExpectedStablePredicateValues, aValues.Begin(), aValues.GetSizeInBytes()));
			}
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
	}

	// Simple
	{
		ContainerTestSimple const aExpectedValues[] = { ContainerTestSimple::Create(0), ContainerTestSimple::Create(1), ContainerTestSimple::Create(2), ContainerTestSimple::Create(3), ContainerTestSimple::Create(4), ContainerTestSimple::Create(5), ContainerTestSimple::Create(6), ContainerTestSimple::Create(7), ContainerTestSimple::Create(8), ContainerTestSimple::Create(9) };
		ContainerTestSimple const aExpectedPredicateValues[] = { ContainerTestSimple::Create(9), ContainerTestSimple::Create(0), ContainerTestSimple::Create(1), ContainerTestSimple::Create(2), ContainerTestSimple::Create(3), ContainerTestSimple::Create(4), ContainerTestSimple::Create(5), ContainerTestSimple::Create(6), ContainerTestSimple::Create(7), ContainerTestSimple::Create(8) };
		ContainerTestSimple const aExpectedStablePredicateValues[] = { ContainerTestSimple::Create(0), ContainerTestSimple::Create(1), ContainerTestSimple::Create(3), ContainerTestSimple::Create(4), ContainerTestSimple::Create(5), ContainerTestSimple::Create(2), ContainerTestSimple::Create(6), ContainerTestSimple::Create(7), ContainerTestSimple::Create(8), ContainerTestSimple::Create(9) };
		ContainerTestSimple const aOrigValues[] = { ContainerTestSimple::Create(8), ContainerTestSimple::Create(5), ContainerTestSimple::Create(1), ContainerTestSimple::Create(4), ContainerTestSimple::Create(3), ContainerTestSimple::Create(2), ContainerTestSimple::Create(6), ContainerTestSimple::Create(0), ContainerTestSimple::Create(7), ContainerTestSimple::Create(9) };
		size_t const zCount = SEOUL_ARRAY_COUNT(aOrigValues);

		// QuickSort, no predicate.
		{
			FixedArray<ContainerTestSimple, zCount> aValues(aOrigValues);
			QuickSort(aValues.Begin(), aValues.End());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(aExpectedValues, aValues.Begin(), aValues.GetSizeInBytes()));
		}

		// QuickSort, predicate.
		{
			FixedArray<ContainerTestSimple, zCount> aValues(aOrigValues);
			QuickSort(aValues.Begin(), aValues.End(), AlgorithmsTestTestSortSimple);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(aExpectedPredicateValues, aValues.Begin(), aValues.GetSizeInBytes()));
		}

		// Sort, no predicate.
		{
			FixedArray<ContainerTestSimple, zCount> aValues(aOrigValues);
			Sort(aValues.Begin(), aValues.End());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(aExpectedValues, aValues.Begin(), aValues.GetSizeInBytes()));
		}

		// Sort, predicate.
		{
			FixedArray<ContainerTestSimple, zCount> aValues(aOrigValues);
			QuickSort(aValues.Begin(), aValues.End(), AlgorithmsTestTestSortSimple);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(aExpectedPredicateValues, aValues.Begin(), aValues.GetSizeInBytes()));
		}

		// StableSort, no predicate.
		{
			FixedArray<ContainerTestSimple, zCount> aValues(aOrigValues);
			StableSort(aValues.Begin(), aValues.End());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(aExpectedValues, aValues.Begin(), aValues.GetSizeInBytes()));
		}

		// StableSort, predicate.
		{
			FixedArray<ContainerTestSimple, zCount> aValues(aOrigValues);
			QuickSort(aValues.Begin(), aValues.End(), AlgorithmsTestTestSortSimpleStablePredicate);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(aExpectedStablePredicateValues, aValues.Begin(), aValues.GetSizeInBytes()));
		}
	}
}

void AlgorithmsTest::TestRotate()
{
	// Builtin
	{
		Int a = 5, b = -23, c = -107;
		Rotate(a, b, c);
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, c);
		SEOUL_UNITTESTING_ASSERT_EQUAL(-107, b);
		SEOUL_UNITTESTING_ASSERT_EQUAL(-23, a);
	}

	// Complex
	{
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
			ContainerTestComplex a = ContainerTestComplex(5), b = ContainerTestComplex(-23), c = ContainerTestComplex(-107);
			SEOUL_UNITTESTING_ASSERT_EQUAL(3, ContainerTestComplex::s_iCount);
			Rotate(a, b, c);
			SEOUL_UNITTESTING_ASSERT_EQUAL(3, ContainerTestComplex::s_iCount);
			SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(5), c);
			SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(-107), b);
			SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(-23), a);
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
	}

	// Simple
	{
		ContainerTestSimple a = ContainerTestSimple::Create(5), b = ContainerTestSimple::Create(-23), c = ContainerTestSimple::Create(-107);
		Rotate(a, b, c);
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(5), c);
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(-107), b);
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(-23), a);
	}
}

void AlgorithmsTest::TestSwap()
{
	// Builtin
	{
		Int a = 5, b = -23;
		Swap(a, b);
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, b);
		SEOUL_UNITTESTING_ASSERT_EQUAL(-23, a);
	}

	// Complex
	{
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
			ContainerTestComplex a = ContainerTestComplex(5), b = ContainerTestComplex(-23);
			SEOUL_UNITTESTING_ASSERT_EQUAL(2, ContainerTestComplex::s_iCount);
			Swap(a, b);
			SEOUL_UNITTESTING_ASSERT_EQUAL(2, ContainerTestComplex::s_iCount);
			SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(5), b);
			SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(-23), a);
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
	}

	// Simple
	{
		ContainerTestSimple a = ContainerTestSimple::Create(5), b = ContainerTestSimple::Create(-23);
		Swap(a, b);
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(5), b);
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(-23), a);
	}
}

void AlgorithmsTest::TestSwapRanges()
{
	// Builtin
	{
		Int const aOrigValuesA[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
		Int const aOrigValuesB[] = { 8, 5, 1, 4, 3, 2, 6, 0, 7, 9 };
		Int aValuesA[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
		Int aValuesB[] = { 8, 5, 1, 4, 3, 2, 6, 0, 7, 9 };

		Int* const p = SwapRanges(&aValuesA[0], &aValuesA[SEOUL_ARRAY_COUNT(aValuesA)], &aValuesB[0]);

		SEOUL_UNITTESTING_ASSERT_EQUAL(&aValuesB[SEOUL_ARRAY_COUNT(aValuesB)], p);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(aOrigValuesA, aValuesB, sizeof(aValuesB)));
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(aOrigValuesB, aValuesA, sizeof(aValuesA)));
	}

	// Complex
	{
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);

			ContainerTestComplex const aOrigValuesA[] = { ContainerTestComplex(0), ContainerTestComplex(1), ContainerTestComplex(2), ContainerTestComplex(3), ContainerTestComplex(4), ContainerTestComplex(5), ContainerTestComplex(6), ContainerTestComplex(7), ContainerTestComplex(8), ContainerTestComplex(9) };
			ContainerTestComplex const aOrigValuesB[] = { ContainerTestComplex(8), ContainerTestComplex(5), ContainerTestComplex(1), ContainerTestComplex(4), ContainerTestComplex(3), ContainerTestComplex(2), ContainerTestComplex(6), ContainerTestComplex(0), ContainerTestComplex(7), ContainerTestComplex(9) };
			ContainerTestComplex aValuesA[] = { ContainerTestComplex(0), ContainerTestComplex(1), ContainerTestComplex(2), ContainerTestComplex(3), ContainerTestComplex(4), ContainerTestComplex(5), ContainerTestComplex(6), ContainerTestComplex(7), ContainerTestComplex(8), ContainerTestComplex(9) };
			ContainerTestComplex aValuesB[] = { ContainerTestComplex(8), ContainerTestComplex(5), ContainerTestComplex(1), ContainerTestComplex(4), ContainerTestComplex(3), ContainerTestComplex(2), ContainerTestComplex(6), ContainerTestComplex(0), ContainerTestComplex(7), ContainerTestComplex(9) };

			SEOUL_UNITTESTING_ASSERT_EQUAL(SEOUL_ARRAY_COUNT(aOrigValuesA) * 4, ContainerTestComplex::s_iCount);
			ContainerTestComplex* const p = SwapRanges(&aValuesA[0], &aValuesA[SEOUL_ARRAY_COUNT(aValuesA)], &aValuesB[0]);
			SEOUL_UNITTESTING_ASSERT_EQUAL(SEOUL_ARRAY_COUNT(aOrigValuesA) * 4, ContainerTestComplex::s_iCount);

			SEOUL_UNITTESTING_ASSERT_EQUAL(&aValuesB[SEOUL_ARRAY_COUNT(aValuesB)], p);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(aOrigValuesA, aValuesB, sizeof(aValuesB)));
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(aOrigValuesB, aValuesA, sizeof(aValuesA)));
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
	}

	// Simple
	{
		ContainerTestSimple const aOrigValuesA[] = { ContainerTestSimple::Create(0), ContainerTestSimple::Create(1), ContainerTestSimple::Create(2), ContainerTestSimple::Create(3), ContainerTestSimple::Create(4), ContainerTestSimple::Create(5), ContainerTestSimple::Create(6), ContainerTestSimple::Create(7), ContainerTestSimple::Create(8), ContainerTestSimple::Create(9) };
		ContainerTestSimple const aOrigValuesB[] = { ContainerTestSimple::Create(8), ContainerTestSimple::Create(5), ContainerTestSimple::Create(1), ContainerTestSimple::Create(4), ContainerTestSimple::Create(3), ContainerTestSimple::Create(2), ContainerTestSimple::Create(6), ContainerTestSimple::Create(0), ContainerTestSimple::Create(7), ContainerTestSimple::Create(9) };
		ContainerTestSimple aValuesA[] = { ContainerTestSimple::Create(0), ContainerTestSimple::Create(1), ContainerTestSimple::Create(2), ContainerTestSimple::Create(3), ContainerTestSimple::Create(4), ContainerTestSimple::Create(5), ContainerTestSimple::Create(6), ContainerTestSimple::Create(7), ContainerTestSimple::Create(8), ContainerTestSimple::Create(9) };
		ContainerTestSimple aValuesB[] = { ContainerTestSimple::Create(8), ContainerTestSimple::Create(5), ContainerTestSimple::Create(1), ContainerTestSimple::Create(4), ContainerTestSimple::Create(3), ContainerTestSimple::Create(2), ContainerTestSimple::Create(6), ContainerTestSimple::Create(0), ContainerTestSimple::Create(7), ContainerTestSimple::Create(9) };

		ContainerTestSimple* const p = SwapRanges(&aValuesA[0], &aValuesA[SEOUL_ARRAY_COUNT(aValuesA)], &aValuesB[0]);

		SEOUL_UNITTESTING_ASSERT_EQUAL(&aValuesB[SEOUL_ARRAY_COUNT(aValuesB)], p);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(aOrigValuesA, aValuesB, sizeof(aValuesB)));
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(aOrigValuesB, aValuesA, sizeof(aValuesA)));
	}
}

void AlgorithmsTest::TestUninitializedCopy()
{
	// Builtin
	{
		Int const aExpectedValues[] = { -1, 1, 1, 2, 3, -1 };
		size_t const zCount = SEOUL_ARRAY_COUNT(aExpectedValues);
		size_t const zSizeInBytes = (zCount + 2) * sizeof(Int);
		Int* pValues = (Int*)MemoryManager::AllocateAligned(
			zSizeInBytes,
			MemoryBudgets::Developer,
			SEOUL_ALIGN_OF(Int));

		// Clear the memory, we use this for fence posting.
		memset(pValues, 0xFF, zSizeInBytes);

		// Do the copy to the middle.
		Int const* pBegin = &aExpectedValues[1];
		Int const* pEnd = &aExpectedValues[zCount - 1];
		Int const* pOut = UninitializedCopy(pBegin, pEnd, pValues + 1);
		SEOUL_UNITTESTING_ASSERT_EQUAL(pValues + zCount - 1, pOut);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(pValues, aExpectedValues, sizeof(aExpectedValues)));

		MemoryManager::Deallocate(pValues);
		pValues = nullptr;
	}

	// Complex
	{
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
			ContainerTestComplex aExpectedValues[] = { ContainerTestComplex(-1), ContainerTestComplex(1), ContainerTestComplex(1), ContainerTestComplex(2), ContainerTestComplex(3), ContainerTestComplex(-1) };
			size_t const zCount = SEOUL_ARRAY_COUNT(aExpectedValues);

			// Set the endpoints specially.
			aExpectedValues[0].m_iFixedValue = -1;
			aExpectedValues[zCount - 1].m_iFixedValue = -1;

			SEOUL_UNITTESTING_ASSERT_EQUAL(zCount, ContainerTestComplex::s_iCount);
			size_t const zSizeInBytes = zCount * sizeof(ContainerTestComplex);
			ContainerTestComplex* pValues = (ContainerTestComplex*)MemoryManager::AllocateAligned(
				zSizeInBytes,
				MemoryBudgets::Developer,
				SEOUL_ALIGN_OF(ContainerTestComplex));
			SEOUL_UNITTESTING_ASSERT_EQUAL(zCount, ContainerTestComplex::s_iCount);

			// Clear the memory, we use this for fence posting.
			// In the case of Complex, this also checks if UninitializedCopy
			// is incorrectly invoking the assignment operator.
			memset(pValues, 0xFF, zSizeInBytes);

			// Do the copy to the middle.
			ContainerTestComplex const* pBegin = &aExpectedValues[1];
			ContainerTestComplex const* pEnd = &aExpectedValues[zCount - 1];
			ContainerTestComplex const* pOut = UninitializedCopy(pBegin, pEnd, pValues + 1);
			SEOUL_UNITTESTING_ASSERT_EQUAL(zCount + (zCount - 2), ContainerTestComplex::s_iCount);
			SEOUL_UNITTESTING_ASSERT_EQUAL(pValues + zCount - 1, pOut);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(pValues, aExpectedValues, sizeof(aExpectedValues)));

			DestroyRange(pValues + 1, pValues + zCount - 1);
			SEOUL_UNITTESTING_ASSERT_EQUAL(zCount, ContainerTestComplex::s_iCount);

			MemoryManager::Deallocate(pValues);
			pValues = nullptr;

			// Restore the endpoints prior to destruction.
			aExpectedValues[0].m_iFixedValue = 33;
			aExpectedValues[zCount - 1].m_iFixedValue = 33;
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
	}

	// Simple
	{
		ContainerTestSimple aExpectedValues[] = { ContainerTestSimple::Create(-1), ContainerTestSimple::Create(1), ContainerTestSimple::Create(1), ContainerTestSimple::Create(2), ContainerTestSimple::Create(3), ContainerTestSimple::Create(-1) };
		size_t const zCount = SEOUL_ARRAY_COUNT(aExpectedValues);

		// Set the endpoints specially.
		memset(&aExpectedValues[0], 0xFF, sizeof(ContainerTestSimple));
		memset(&aExpectedValues[zCount - 1], 0xFF, sizeof(ContainerTestSimple));

		size_t const zSizeInBytes = zCount * sizeof(ContainerTestSimple);
		ContainerTestSimple* pValues = (ContainerTestSimple*)MemoryManager::AllocateAligned(
			zSizeInBytes,
			MemoryBudgets::Developer,
			SEOUL_ALIGN_OF(ContainerTestSimple));

		// Clear the memory, we use this for fence posting.
		memset(pValues, 0xFF, zSizeInBytes);

		// Do the copy to the middle.
		ContainerTestSimple const* pBegin = &aExpectedValues[1];
		ContainerTestSimple const* pEnd = &aExpectedValues[zCount - 1];
		ContainerTestSimple const* pOut = UninitializedCopy(pBegin, pEnd, pValues + 1);
		SEOUL_UNITTESTING_ASSERT_EQUAL(pValues + zCount - 1, pOut);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(pValues, aExpectedValues, zSizeInBytes));

		MemoryManager::Deallocate(pValues);
		pValues = nullptr;

		// Restore the endpoints prior to destruction.
		aExpectedValues[0].m_B = 33;
		aExpectedValues[zCount - 1].m_B = 33;
	}
}

void AlgorithmsTest::TestUninitializedCopyBackward()
{
	// Builtin
	{
		Int const aExpectedValues[] = { -1, 1, 1, 2, 3, -1 };
		size_t const zCount = SEOUL_ARRAY_COUNT(aExpectedValues);
		size_t const zSizeInBytes = (zCount + 2) * sizeof(Int);
		Int* pValues = (Int*)MemoryManager::AllocateAligned(
			zSizeInBytes,
			MemoryBudgets::Developer,
			SEOUL_ALIGN_OF(Int));

		// Clear the memory, we use this for fence posting.
		memset(pValues, 0xFF, zSizeInBytes);

		// Do the copy to the middle.
		Int const* pBegin = &aExpectedValues[1];
		Int const* pEnd = &aExpectedValues[zCount - 1];
		Int const* pOut = UninitializedCopyBackward(pBegin, pEnd, pValues + zCount - 1);
		SEOUL_UNITTESTING_ASSERT_EQUAL(pValues + 1, pOut);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(pValues, aExpectedValues, sizeof(aExpectedValues)));

		MemoryManager::Deallocate(pValues);
		pValues = nullptr;
	}

	// Complex
	{
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
			ContainerTestComplex aExpectedValues[] = { ContainerTestComplex(-1), ContainerTestComplex(1), ContainerTestComplex(1), ContainerTestComplex(2), ContainerTestComplex(3), ContainerTestComplex(-1) };
			size_t const zCount = SEOUL_ARRAY_COUNT(aExpectedValues);

			// Set the endpoints specially.
			aExpectedValues[0].m_iFixedValue = -1;
			aExpectedValues[zCount - 1].m_iFixedValue = -1;

			SEOUL_UNITTESTING_ASSERT_EQUAL(zCount, ContainerTestComplex::s_iCount);
			size_t const zSizeInBytes = zCount * sizeof(ContainerTestComplex);
			ContainerTestComplex* pValues = (ContainerTestComplex*)MemoryManager::AllocateAligned(
				zSizeInBytes,
				MemoryBudgets::Developer,
				SEOUL_ALIGN_OF(ContainerTestComplex));
			SEOUL_UNITTESTING_ASSERT_EQUAL(zCount, ContainerTestComplex::s_iCount);

			// Clear the memory, we use this for fence posting.
			// In the case of Complex, this also checks if UninitializedCopy
			// is incorrectly invoking the assignment operator.
			memset(pValues, 0xFF, zSizeInBytes);

			// Do the copy to the middle.
			ContainerTestComplex const* pBegin = &aExpectedValues[1];
			ContainerTestComplex const* pEnd = &aExpectedValues[zCount - 1];
			ContainerTestComplex const* pOut = UninitializedCopyBackward(pBegin, pEnd, pValues + zCount - 1);
			SEOUL_UNITTESTING_ASSERT_EQUAL(zCount + (zCount - 2), ContainerTestComplex::s_iCount);
			SEOUL_UNITTESTING_ASSERT_EQUAL(pValues + 1, pOut);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(pValues, aExpectedValues, sizeof(aExpectedValues)));

			DestroyRange(pValues + 1, pValues + zCount - 1);
			SEOUL_UNITTESTING_ASSERT_EQUAL(zCount, ContainerTestComplex::s_iCount);

			MemoryManager::Deallocate(pValues);
			pValues = nullptr;

			// Restore the endpoints prior to destruction.
			aExpectedValues[0].m_iFixedValue = 33;
			aExpectedValues[zCount - 1].m_iFixedValue = 33;
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
	}

	// Simple
	{
		ContainerTestSimple aExpectedValues[] = { ContainerTestSimple::Create(-1), ContainerTestSimple::Create(1), ContainerTestSimple::Create(1), ContainerTestSimple::Create(2), ContainerTestSimple::Create(3), ContainerTestSimple::Create(-1) };
		size_t const zCount = SEOUL_ARRAY_COUNT(aExpectedValues);

		// Set the endpoints specially.
		memset(&aExpectedValues[0], 0xFF, sizeof(ContainerTestSimple));
		memset(&aExpectedValues[zCount - 1], 0xFF, sizeof(ContainerTestSimple));

		size_t const zSizeInBytes = zCount * sizeof(ContainerTestComplex);
		ContainerTestSimple* pValues = (ContainerTestSimple*)MemoryManager::AllocateAligned(
			zSizeInBytes,
			MemoryBudgets::Developer,
			SEOUL_ALIGN_OF(ContainerTestComplex));

		// Clear the memory, we use this for fence posting.
		memset(pValues, 0xFF, zSizeInBytes);

		// Do the copy to the middle.
		ContainerTestSimple const* pBegin = &aExpectedValues[1];
		ContainerTestSimple const* pEnd = &aExpectedValues[zCount - 1];
		ContainerTestSimple const* pOut = UninitializedCopyBackward(pBegin, pEnd, pValues + zCount - 1);
		SEOUL_UNITTESTING_ASSERT_EQUAL(pValues + 1, pOut);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(pValues, aExpectedValues, sizeof(aExpectedValues)));

		MemoryManager::Deallocate(pValues);
		pValues = nullptr;

		// Restore the endpoints prior to destruction.
		aExpectedValues[0].m_B = 33;
		aExpectedValues[zCount - 1].m_B = 33;
	}
}

void AlgorithmsTest::TestUninitializedFill()
{
	// Builtin
	{
		Int* pValues = (Int*)MemoryManager::AllocateAligned(25 * sizeof(Int), MemoryBudgets::Developer, SEOUL_ALIGN_OF(Int));
		size_t const zCount = 25;

		// Fill for fence posting.
		memset(pValues, 0, zCount * sizeof(Int));

		// Make sure fill only fills the specified range.
		Int* pBegin = &pValues[1];
		Int* pEnd = &pValues[zCount - 1];
		UninitializedFill(pBegin, pEnd, Int(25));

		for (size_t i = 0; i < zCount; ++i)
		{
			if (0 == i || zCount - 1 == i)
			{
				SEOUL_UNITTESTING_ASSERT_EQUAL(Int(0), pValues[i]);
			}
			else
			{
				SEOUL_UNITTESTING_ASSERT_EQUAL(Int(25), pValues[i]);
			}
		}

		MemoryManager::Deallocate(pValues);
		pValues = nullptr;
	}

	// Complex
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
		{
			ContainerTestComplex* pValues = (ContainerTestComplex*)MemoryManager::AllocateAligned(25 * sizeof(ContainerTestComplex), MemoryBudgets::Developer, SEOUL_ALIGN_OF(ContainerTestComplex));
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
			size_t const zCount = 25;

			// Fill for fence posting.
			memset(pValues, 0, zCount * sizeof(ContainerTestComplex));
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);

			// Make sure fill only fills the specified range.
			ContainerTestComplex* pBegin = &pValues[1];
			ContainerTestComplex* pEnd = &pValues[zCount - 1];
			UninitializedFill(pBegin, pEnd, ContainerTestComplex(25));
			SEOUL_UNITTESTING_ASSERT_EQUAL(23, ContainerTestComplex::s_iCount);

			for (size_t i = 0; i < zCount; ++i)
			{
				if (0 == i || zCount - 1 == i)
				{
					// Raw check, constructor not invoked.
					SEOUL_UNITTESTING_ASSERT_EQUAL(0, pValues[i].m_iFixedValue);
					SEOUL_UNITTESTING_ASSERT_EQUAL(0, pValues[i].m_iVariableValue);
				}
				else
				{
					SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(25), pValues[i]);
				}
			}

			// Destroy the proper sub range.
			DestroyRange(pBegin, pEnd);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);

			MemoryManager::Deallocate(pValues);
			pValues = nullptr;
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
	}

	// Simple
	{
		ContainerTestSimple* pValues = (ContainerTestSimple*)MemoryManager::AllocateAligned(25 * sizeof(ContainerTestSimple), MemoryBudgets::Developer, SEOUL_ALIGN_OF(ContainerTestSimple));
		size_t const zCount = 25;

		// Fill for fence posting.
		memset(pValues, 0, zCount * sizeof(ContainerTestSimple));

		// Make sure fill only fills the specified range.
		ContainerTestSimple* pBegin = &pValues[1];
		ContainerTestSimple* pEnd = &pValues[zCount - 1];
		UninitializedFill(pBegin, pEnd, ContainerTestSimple::Create(25));

		for (size_t i = 0; i < zCount; ++i)
		{
			if (0 == i || zCount - 1 == i)
			{
				// Raw check, constructor not invoked.
				SEOUL_UNITTESTING_ASSERT_EQUAL(0, pValues[i].m_iA);
				SEOUL_UNITTESTING_ASSERT_EQUAL(0, pValues[i].m_B);
			}
			else
			{
				SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(25), pValues[i]);
			}
		}

		MemoryManager::Deallocate(pValues);
		pValues = nullptr;
	}
}

void AlgorithmsTest::TestUninitializedMove()
{
	// Builtin
	{
		Int aExpectedValues[] = { -1, 1, 1, 2, 3, -1 };
		size_t const zCount = SEOUL_ARRAY_COUNT(aExpectedValues);
		size_t const zSizeInBytes = (zCount + 2) * sizeof(Int);
		Int* pValues = (Int*)MemoryManager::AllocateAligned(
			zSizeInBytes,
			MemoryBudgets::Developer,
			SEOUL_ALIGN_OF(Int));

		// Clear the memory, we use this for fence posting.
		memset(pValues, 0xFF, zSizeInBytes);

		// Do the move to the middle.
		Int* pBegin = &aExpectedValues[1];
		Int* pEnd = &aExpectedValues[zCount - 1];
		Int const* pOut = UninitializedMove(pBegin, pEnd, pValues + 1);
		SEOUL_UNITTESTING_ASSERT_EQUAL(pValues + zCount - 1, pOut);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(pValues, aExpectedValues, sizeof(aExpectedValues)));

		MemoryManager::Deallocate(pValues);
		pValues = nullptr;
	}

	// Complex
	{
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
			ContainerTestComplex aExpectedValues[] = { ContainerTestComplex(-1), ContainerTestComplex(1), ContainerTestComplex(1), ContainerTestComplex(2), ContainerTestComplex(3), ContainerTestComplex(-1) };
			ContainerTestComplex aTestValues[] = { ContainerTestComplex(-1), ContainerTestComplex(1), ContainerTestComplex(1), ContainerTestComplex(2), ContainerTestComplex(3), ContainerTestComplex(-1) };
			size_t const zCount = SEOUL_ARRAY_COUNT(aExpectedValues);

			// Set the endpoints specially.
			aExpectedValues[0].m_iFixedValue = -1;
			aExpectedValues[zCount - 1].m_iFixedValue = -1;
			aTestValues[0].m_iFixedValue = -1;
			aTestValues[zCount - 1].m_iFixedValue = -1;

			SEOUL_UNITTESTING_ASSERT_EQUAL(2 * zCount, ContainerTestComplex::s_iCount);
			size_t const zSizeInBytes = zCount * sizeof(ContainerTestComplex);
			ContainerTestComplex* pValues = (ContainerTestComplex*)MemoryManager::AllocateAligned(
				zSizeInBytes,
				MemoryBudgets::Developer,
				SEOUL_ALIGN_OF(ContainerTestComplex));
			SEOUL_UNITTESTING_ASSERT_EQUAL(2 * zCount, ContainerTestComplex::s_iCount);

			// Clear the memory, we use this for fence posting.
			// In the case of Complex, this also checks if UninitializedCopy
			// is incorrectly invoking the assignment operator.
			memset(pValues, 0xFF, zSizeInBytes);

			// Do the copy to the middle.
			ContainerTestComplex* pBegin = &aTestValues[1];
			ContainerTestComplex* pEnd = &aTestValues[zCount - 1];
			ContainerTestComplex const* pOut = UninitializedMove(pBegin, pEnd, pValues + 1);
			SEOUL_UNITTESTING_ASSERT_EQUAL(2 * zCount + (zCount - 2), ContainerTestComplex::s_iCount);
			SEOUL_UNITTESTING_ASSERT_EQUAL(pValues + zCount - 1, pOut);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(pValues, aExpectedValues, sizeof(aExpectedValues)));
			SEOUL_UNITTESTING_ASSERT_EQUAL(aTestValues[0], aExpectedValues[0]);
			SEOUL_UNITTESTING_ASSERT_EQUAL(aTestValues[1], ContainerTestComplex(5235));
			SEOUL_UNITTESTING_ASSERT_EQUAL(aTestValues[2], ContainerTestComplex(5235));
			SEOUL_UNITTESTING_ASSERT_EQUAL(aTestValues[3], ContainerTestComplex(5235));
			SEOUL_UNITTESTING_ASSERT_EQUAL(aTestValues[4], ContainerTestComplex(5235));
			SEOUL_UNITTESTING_ASSERT_EQUAL(aTestValues[5], aExpectedValues[5]);

			DestroyRange(pValues + 1, pValues + zCount - 1);
			SEOUL_UNITTESTING_ASSERT_EQUAL(2 * zCount, ContainerTestComplex::s_iCount);

			MemoryManager::Deallocate(pValues);
			pValues = nullptr;

			// Restore the endpoints prior to destruction.
			aTestValues[0].m_iFixedValue = 33;
			aTestValues[zCount - 1].m_iFixedValue = 33;
			aExpectedValues[0].m_iFixedValue = 33;
			aExpectedValues[zCount - 1].m_iFixedValue = 33;
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
	}

	// Simple
	{
		ContainerTestSimple aExpectedValues[] = { ContainerTestSimple::Create(-1), ContainerTestSimple::Create(1), ContainerTestSimple::Create(1), ContainerTestSimple::Create(2), ContainerTestSimple::Create(3), ContainerTestSimple::Create(-1) };
		size_t const zCount = SEOUL_ARRAY_COUNT(aExpectedValues);

		// Set the endpoints specially.
		memset(&aExpectedValues[0], 0xFF, sizeof(ContainerTestSimple));
		memset(&aExpectedValues[zCount - 1], 0xFF, sizeof(ContainerTestSimple));

		size_t const zSizeInBytes = zCount * sizeof(ContainerTestSimple);
		ContainerTestSimple* pValues = (ContainerTestSimple*)MemoryManager::AllocateAligned(
			zSizeInBytes,
			MemoryBudgets::Developer,
			SEOUL_ALIGN_OF(ContainerTestSimple));

		// Clear the memory, we use this for fence posting.
		memset(pValues, 0xFF, zSizeInBytes);

		// Do the copy to the middle.
		ContainerTestSimple* pBegin = &aExpectedValues[1];
		ContainerTestSimple* pEnd = &aExpectedValues[zCount - 1];
		ContainerTestSimple const* pOut = UninitializedMove(pBegin, pEnd, pValues + 1);
		SEOUL_UNITTESTING_ASSERT_EQUAL(pValues + zCount - 1, pOut);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(pValues, aExpectedValues, zSizeInBytes));

		MemoryManager::Deallocate(pValues);
		pValues = nullptr;

		// Restore the endpoints prior to destruction.
		aExpectedValues[0].m_B = 33;
		aExpectedValues[zCount - 1].m_B = 33;
	}
}

template <typename T>
struct TestUpperBoundPred SEOUL_SEALED
{
	Bool operator()(const T& a, const T& b) const
	{
		// Reverse sort.
		return (b < a);
	}
};
void AlgorithmsTest::TestUpperBound()
{
	// No predicate.
	{
		// Builtin
		{
			Int const aOrigValues[] = { 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 };
			size_t const zCount = SEOUL_ARRAY_COUNT(aOrigValues);

			FixedArray<Int, zCount> aValues(aOrigValues);
			SEOUL_UNITTESTING_ASSERT_EQUAL(aValues.End(), UpperBound(aValues.Begin(), aValues.End(), 4));
			SEOUL_UNITTESTING_ASSERT_EQUAL(aValues.End(), UpperBound(aValues.Begin(), aValues.End(), 20));
			SEOUL_UNITTESTING_ASSERT_EQUAL(aValues.Begin(), UpperBound(aValues.Begin(), aValues.End(), -1));
		}

		// Complex
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
			{
				ContainerTestComplex const aOrigValues[] = { ContainerTestComplex(9), ContainerTestComplex(8), ContainerTestComplex(7), ContainerTestComplex(6), ContainerTestComplex(5), ContainerTestComplex(4), ContainerTestComplex(3), ContainerTestComplex(2), ContainerTestComplex(1), ContainerTestComplex(0) };
				size_t const zCount = SEOUL_ARRAY_COUNT(aOrigValues);

				FixedArray<ContainerTestComplex, zCount> aValues(aOrigValues);
				SEOUL_UNITTESTING_ASSERT_EQUAL(aValues.End(), UpperBound(aValues.Begin(), aValues.End(), ContainerTestComplex(4)));
				SEOUL_UNITTESTING_ASSERT_EQUAL(aValues.End(), UpperBound(aValues.Begin(), aValues.End(), ContainerTestComplex(20)));
				SEOUL_UNITTESTING_ASSERT_EQUAL(aValues.Begin(), UpperBound(aValues.Begin(), aValues.End(), ContainerTestComplex(-1)));
			}
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
		}

		// Simple
		{
			ContainerTestSimple const aOrigValues[] = { ContainerTestSimple::Create(9), ContainerTestSimple::Create(8), ContainerTestSimple::Create(7), ContainerTestSimple::Create(6), ContainerTestSimple::Create(5), ContainerTestSimple::Create(4), ContainerTestSimple::Create(3), ContainerTestSimple::Create(2), ContainerTestSimple::Create(1), ContainerTestSimple::Create(0) };
			size_t const zCount = SEOUL_ARRAY_COUNT(aOrigValues);

			FixedArray<ContainerTestSimple, zCount> aValues(aOrigValues);
			SEOUL_UNITTESTING_ASSERT_EQUAL(aValues.End(), UpperBound(aValues.Begin(), aValues.End(), ContainerTestSimple::Create(4)));
			SEOUL_UNITTESTING_ASSERT_EQUAL(aValues.End(), UpperBound(aValues.Begin(), aValues.End(), ContainerTestSimple::Create(20)));
			SEOUL_UNITTESTING_ASSERT_EQUAL(aValues.Begin(), UpperBound(aValues.Begin(), aValues.End(), ContainerTestSimple::Create(-1)));
		}
	}

	// Predicate.
	{
		// Builtin
		{
			Int const aOrigValues[] = { 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 };
			size_t const zCount = SEOUL_ARRAY_COUNT(aOrigValues);

			FixedArray<Int, zCount> aValues(aOrigValues);
			SEOUL_UNITTESTING_ASSERT_EQUAL(3, *UpperBound(aValues.Begin(), aValues.End(), 4, TestUpperBoundPred<Int>()));
			SEOUL_UNITTESTING_ASSERT_EQUAL(aValues.Begin(), UpperBound(aValues.Begin(), aValues.End(), 20, TestUpperBoundPred<Int>()));
			SEOUL_UNITTESTING_ASSERT_EQUAL(aValues.End(), UpperBound(aValues.Begin(), aValues.End(), -1, TestUpperBoundPred<Int>()));
		}

		// Complex
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
			{
				ContainerTestComplex const aOrigValues[] = { ContainerTestComplex(9), ContainerTestComplex(8), ContainerTestComplex(7), ContainerTestComplex(6), ContainerTestComplex(5), ContainerTestComplex(4), ContainerTestComplex(3), ContainerTestComplex(2), ContainerTestComplex(1), ContainerTestComplex(0) };
				size_t const zCount = SEOUL_ARRAY_COUNT(aOrigValues);

				FixedArray<ContainerTestComplex, zCount> aValues(aOrigValues);
				SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(3), *UpperBound(aValues.Begin(), aValues.End(), ContainerTestComplex(4), TestUpperBoundPred<ContainerTestComplex>()));
				SEOUL_UNITTESTING_ASSERT_EQUAL(aValues.Begin(), UpperBound(aValues.Begin(), aValues.End(), ContainerTestComplex(20), TestUpperBoundPred<ContainerTestComplex>()));
				SEOUL_UNITTESTING_ASSERT_EQUAL(aValues.End(), UpperBound(aValues.Begin(), aValues.End(), ContainerTestComplex(-1), TestUpperBoundPred<ContainerTestComplex>()));
			}
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
		}

		// Simple
		{
			ContainerTestSimple const aOrigValues[] = { ContainerTestSimple::Create(9), ContainerTestSimple::Create(8), ContainerTestSimple::Create(7), ContainerTestSimple::Create(6), ContainerTestSimple::Create(5), ContainerTestSimple::Create(4), ContainerTestSimple::Create(3), ContainerTestSimple::Create(2), ContainerTestSimple::Create(1), ContainerTestSimple::Create(0) };
			size_t const zCount = SEOUL_ARRAY_COUNT(aOrigValues);

			FixedArray<ContainerTestSimple, zCount> aValues(aOrigValues);
			SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(3), *UpperBound(aValues.Begin(), aValues.End(), ContainerTestSimple::Create(4), TestUpperBoundPred<ContainerTestSimple>()));
			SEOUL_UNITTESTING_ASSERT_EQUAL(aValues.Begin(), UpperBound(aValues.Begin(), aValues.End(), ContainerTestSimple::Create(20), TestUpperBoundPred<ContainerTestSimple>()));
			SEOUL_UNITTESTING_ASSERT_EQUAL(aValues.End(), UpperBound(aValues.Begin(), aValues.End(), ContainerTestSimple::Create(-1), TestUpperBoundPred<ContainerTestSimple>()));
		}
	}
}

void AlgorithmsTest::TestZeroFillSimple()
{
	// Builtin
	{
		Int aValues[25];
		for (Int i = 0; i < 25; ++i)
		{
			aValues[i] = Int(-1);
		}
		size_t const zCount = SEOUL_ARRAY_COUNT(aValues);

		// Make sure zerofill only fills the specified range.
		Int* pBegin = &aValues[1];
		Int* pEnd = &aValues[zCount - 1];
		ZeroFillSimple(pBegin, pEnd);

		for (size_t i = 0; i < zCount; ++i)
		{
			if (0 == i || zCount - 1 == i)
			{
				SEOUL_UNITTESTING_ASSERT_EQUAL(Int(-1), aValues[i]);
			}
			else
			{
				SEOUL_UNITTESTING_ASSERT_EQUAL(Int(0), aValues[i]);
			}
		}
	}

	// Complex
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
		{
			ContainerTestComplex aValues[25];
			for (Int i = 0; i < 25; ++i)
			{
				aValues[i] = ContainerTestComplex(-1);
			}
			SEOUL_UNITTESTING_ASSERT_EQUAL(25, ContainerTestComplex::s_iCount);
			size_t const zCount = SEOUL_ARRAY_COUNT(aValues);

			// Zerofill should not do anything, since the value is a complex type.
			ContainerTestComplex* pBegin = &aValues[1];
			ContainerTestComplex* pEnd = &aValues[zCount - 1];
			ZeroFillSimple(pBegin, pEnd);
			SEOUL_UNITTESTING_ASSERT_EQUAL(25, ContainerTestComplex::s_iCount);

			for (size_t i = 0; i < zCount; ++i)
			{
				SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(-1), aValues[i]);
			}
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
	}

	// Simple
	{
		ContainerTestSimple aValues[25];
		for (Int i = 0; i < 25; ++i)
		{
			aValues[i] = ContainerTestSimple::Create(-1);
		}
		size_t const zCount = SEOUL_ARRAY_COUNT(aValues);

		// Make sure zerofill only fills the specified range.
		ContainerTestSimple* pBegin = &aValues[1];
		ContainerTestSimple* pEnd = &aValues[zCount - 1];
		ZeroFillSimple(pBegin, pEnd);

		for (size_t i = 0; i < zCount; ++i)
		{
			if (0 == i || zCount - 1 == i)
			{
				SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestSimple::Create(-1), aValues[i]);
			}
			else
			{
				SEOUL_UNITTESTING_ASSERT_EQUAL(0, aValues[i].m_B);
				SEOUL_UNITTESTING_ASSERT_EQUAL(0, aValues[i].m_iA);
			}
		}
	}
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
