/**
 * \file FixedArrayTest.cpp
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
#include "FixedArray.h"
#include "FixedArrayTest.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(FixedArrayTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestAssign)
	SEOUL_METHOD(TestCopyConstructor)
	SEOUL_METHOD(TestDefaultConstructor)
	SEOUL_METHOD(TestMethods)
	SEOUL_METHOD(TestIterators)
	SEOUL_METHOD(TestValueConstructor)
	SEOUL_METHOD(TestRangedFor)
SEOUL_END_TYPE()

void FixedArrayTest::TestAssign()
{
	// Assign with builtin
	{
		FixedArray<UInt64, 15> fixedArray;
		fixedArray.Fill((UInt64)27);

		// Test for initialized builtin.
		for (UInt32 i = 0; i < fixedArray.GetSize(); ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(27, fixedArray[i]);
		}

		SEOUL_UNITTESTING_ASSERT_EQUAL(15, fixedArray.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(15 * sizeof(UInt64), fixedArray.GetSizeInBytes());
		SEOUL_UNITTESTING_ASSERT(!fixedArray.IsEmpty());

		// Copy
		{
			FixedArray<UInt64, 15> fixedArray2;
			fixedArray2 = fixedArray;

			// Test for initialized builtin.
			for (UInt32 i = 0; i < fixedArray2.GetSize(); ++i)
			{
				SEOUL_UNITTESTING_ASSERT_EQUAL(27, fixedArray2[i]);
			}

			SEOUL_UNITTESTING_ASSERT_EQUAL(15, fixedArray2.GetSize());
			SEOUL_UNITTESTING_ASSERT_EQUAL(15 * sizeof(UInt64), fixedArray2.GetSizeInBytes());
			SEOUL_UNITTESTING_ASSERT(!fixedArray2.IsEmpty());
		}

		// Self copy
		{
			fixedArray = fixedArray;

			// Test for initialized builtin.
			for (UInt32 i = 0; i < fixedArray.GetSize(); ++i)
			{
				SEOUL_UNITTESTING_ASSERT_EQUAL(27, fixedArray[i]);
			}

			SEOUL_UNITTESTING_ASSERT_EQUAL(15, fixedArray.GetSize());
			SEOUL_UNITTESTING_ASSERT_EQUAL(15 * sizeof(UInt64), fixedArray.GetSizeInBytes());
			SEOUL_UNITTESTING_ASSERT(!fixedArray.IsEmpty());
		}
	}

	// Assign with Simple
	{
		ContainerTestSimple simple;
		simple.m_B = 33;
		simple.m_iA = 125209;
		FixedArray<ContainerTestSimple, 27> fixedArray;
		fixedArray.Fill(simple);

		// Test for initialized Simple.
		for (UInt32 i = 0; i < fixedArray.GetSize(); ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(125209, fixedArray[i].m_iA);
			SEOUL_UNITTESTING_ASSERT_EQUAL(33, fixedArray[i].m_B);
		}

		SEOUL_UNITTESTING_ASSERT_EQUAL(27, fixedArray.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(27 * sizeof(ContainerTestSimple), fixedArray.GetSizeInBytes());
		SEOUL_UNITTESTING_ASSERT(!fixedArray.IsEmpty());

		// Copy
		{
			FixedArray<ContainerTestSimple, 27> fixedArray2;
			fixedArray2 = fixedArray;

			// Test for initialized Simple.
			for (UInt32 i = 0; i < fixedArray2.GetSize(); ++i)
			{
				SEOUL_UNITTESTING_ASSERT_EQUAL(125209, fixedArray2[i].m_iA);
				SEOUL_UNITTESTING_ASSERT_EQUAL(33, fixedArray2[i].m_B);
			}

			SEOUL_UNITTESTING_ASSERT_EQUAL(27, fixedArray2.GetSize());
			SEOUL_UNITTESTING_ASSERT_EQUAL(27 * sizeof(ContainerTestSimple), fixedArray2.GetSizeInBytes());
			SEOUL_UNITTESTING_ASSERT(!fixedArray2.IsEmpty());
		}

		// Self copy
		{
			fixedArray = fixedArray;

			// Test for initialized Simple.
			for (UInt32 i = 0; i < fixedArray.GetSize(); ++i)
			{
				SEOUL_UNITTESTING_ASSERT_EQUAL(125209, fixedArray[i].m_iA);
				SEOUL_UNITTESTING_ASSERT_EQUAL(33, fixedArray[i].m_B);
			}

			SEOUL_UNITTESTING_ASSERT_EQUAL(27, fixedArray.GetSize());
			SEOUL_UNITTESTING_ASSERT_EQUAL(27 * sizeof(ContainerTestSimple), fixedArray.GetSizeInBytes());
			SEOUL_UNITTESTING_ASSERT(!fixedArray.IsEmpty());
		}
	}

	// Assign with complex
	{
		{
			FixedArray<ContainerTestComplex, 13> fixedArray;
			fixedArray.Fill(ContainerTestComplex(2342));

			// Check count.
			SEOUL_UNITTESTING_ASSERT_EQUAL(13, ContainerTestComplex::s_iCount);

			// Test for default constructed complex.
			for (UInt32 i = 0; i < fixedArray.GetSize(); ++i)
			{
				SEOUL_UNITTESTING_ASSERT_EQUAL(33, fixedArray[i].m_iFixedValue);
				SEOUL_UNITTESTING_ASSERT_EQUAL(2342, fixedArray[i].m_iVariableValue);
			}

			SEOUL_UNITTESTING_ASSERT_EQUAL(13, fixedArray.GetSize());
			SEOUL_UNITTESTING_ASSERT_EQUAL(13 * sizeof(ContainerTestComplex), fixedArray.GetSizeInBytes());
			SEOUL_UNITTESTING_ASSERT(!fixedArray.IsEmpty());

			// Copy
			{
				FixedArray<ContainerTestComplex, 13> fixedArray2;
				fixedArray2 = fixedArray;

				// Check count.
				SEOUL_UNITTESTING_ASSERT_EQUAL(26, ContainerTestComplex::s_iCount);

				// Test for default constructed complex.
				for (UInt32 i = 0; i < fixedArray2.GetSize(); ++i)
				{
					SEOUL_UNITTESTING_ASSERT_EQUAL(33, fixedArray2[i].m_iFixedValue);
					SEOUL_UNITTESTING_ASSERT_EQUAL(2342, fixedArray2[i].m_iVariableValue);
				}

				SEOUL_UNITTESTING_ASSERT_EQUAL(13, fixedArray2.GetSize());
				SEOUL_UNITTESTING_ASSERT_EQUAL(13 * sizeof(ContainerTestComplex), fixedArray2.GetSizeInBytes());
				SEOUL_UNITTESTING_ASSERT(!fixedArray2.IsEmpty());
			}

			// Check count.
			SEOUL_UNITTESTING_ASSERT_EQUAL(13, ContainerTestComplex::s_iCount);

			// Self copy
			{
				fixedArray = fixedArray;

				// Check count.
				SEOUL_UNITTESTING_ASSERT_EQUAL(13, ContainerTestComplex::s_iCount);

				// Test for default constructed complex.
				for (UInt32 i = 0; i < fixedArray.GetSize(); ++i)
				{
					SEOUL_UNITTESTING_ASSERT_EQUAL(33, fixedArray[i].m_iFixedValue);
					SEOUL_UNITTESTING_ASSERT_EQUAL(2342, fixedArray[i].m_iVariableValue);
				}

				SEOUL_UNITTESTING_ASSERT_EQUAL(13, fixedArray.GetSize());
				SEOUL_UNITTESTING_ASSERT_EQUAL(13 * sizeof(ContainerTestComplex), fixedArray.GetSizeInBytes());
				SEOUL_UNITTESTING_ASSERT(!fixedArray.IsEmpty());
			}

			// Check count.
			SEOUL_UNITTESTING_ASSERT_EQUAL(13, ContainerTestComplex::s_iCount);
		}

		// Check count.
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
	}
}

void FixedArrayTest::TestCopyConstructor()
{
	// Copy construction with builtin.
	{
		UInt16 aValues[] = { 23, 154, 23409, 109, 4098, 123, 230 };

		FixedArray<UInt16, 7> fixedArray(aValues);

		// Test for properly constructed builtin.
		for (UInt32 i = 0; i < fixedArray.GetSize(); ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(aValues[i], fixedArray[i]);
		}

		SEOUL_UNITTESTING_ASSERT_EQUAL(7, fixedArray.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(7 * sizeof(UInt16), fixedArray.GetSizeInBytes());
		SEOUL_UNITTESTING_ASSERT(!fixedArray.IsEmpty());

		// Now copy
		{
			FixedArray<UInt16, 7> fixedArray2(fixedArray);

			// Test for properly constructed builtin.
			for (UInt32 i = 0; i < fixedArray.GetSize(); ++i)
			{
				SEOUL_UNITTESTING_ASSERT_EQUAL(aValues[i], fixedArray2[i]);
			}

			SEOUL_UNITTESTING_ASSERT_EQUAL(7, fixedArray2.GetSize());
			SEOUL_UNITTESTING_ASSERT_EQUAL(7 * sizeof(UInt16), fixedArray2.GetSizeInBytes());
			SEOUL_UNITTESTING_ASSERT(!fixedArray2.IsEmpty());
		}
	}

	// Copy construction with Simple.
	{
		ContainerTestSimple aValues[] =
		{
			{ 23, 33},
			{ 154, 33},
			{ 23409, 33},
			{ 109, 33},
			{ 4098, 33},
			{ 123, 33},
			{ 230, 33},
		};

		FixedArray<ContainerTestSimple, 7> fixedArray(aValues);

		// Test for properly constructed Simple.
		for (UInt32 i = 0; i < fixedArray.GetSize(); ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(aValues[i], fixedArray[i]);
		}

		SEOUL_UNITTESTING_ASSERT_EQUAL(7, fixedArray.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(7 * sizeof(ContainerTestSimple), fixedArray.GetSizeInBytes());
		SEOUL_UNITTESTING_ASSERT(!fixedArray.IsEmpty());

		// Now copy
		{
			FixedArray<ContainerTestSimple, 7> fixedArray2(fixedArray);

			// Test for properly constructed Simple.
			for (UInt32 i = 0; i < fixedArray.GetSize(); ++i)
			{
				SEOUL_UNITTESTING_ASSERT_EQUAL(aValues[i], fixedArray2[i]);
			}

			SEOUL_UNITTESTING_ASSERT_EQUAL(7, fixedArray2.GetSize());
			SEOUL_UNITTESTING_ASSERT_EQUAL(7 * sizeof(ContainerTestSimple), fixedArray2.GetSizeInBytes());
			SEOUL_UNITTESTING_ASSERT(!fixedArray2.IsEmpty());
		}
	}

	// Copy construction with complex.
	{
		ContainerTestComplex aValues[] =
		{
			ContainerTestComplex(23),
			ContainerTestComplex(154),
			ContainerTestComplex(23409),
			ContainerTestComplex(109),
			ContainerTestComplex(4098),
			ContainerTestComplex(123),
			ContainerTestComplex(230),
		};

		SEOUL_UNITTESTING_ASSERT_EQUAL(7, ContainerTestComplex::s_iCount);

		FixedArray<ContainerTestComplex, 7> fixedArray(aValues);

		SEOUL_UNITTESTING_ASSERT_EQUAL(14, ContainerTestComplex::s_iCount);

		// Test for properly constructed complex.
		for (UInt32 i = 0; i < fixedArray.GetSize(); ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(aValues[i], fixedArray[i]);
		}

		SEOUL_UNITTESTING_ASSERT_EQUAL(7, fixedArray.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(7 * sizeof(ContainerTestComplex), fixedArray.GetSizeInBytes());
		SEOUL_UNITTESTING_ASSERT(!fixedArray.IsEmpty());

		// Now copy
		{
			FixedArray<ContainerTestComplex, 7> fixedArray2(fixedArray);

			SEOUL_UNITTESTING_ASSERT_EQUAL(21, ContainerTestComplex::s_iCount);

			// Test for properly constructed complex.
			for (UInt32 i = 0; i < fixedArray.GetSize(); ++i)
			{
				SEOUL_UNITTESTING_ASSERT_EQUAL(aValues[i], fixedArray2[i]);
			}

			SEOUL_UNITTESTING_ASSERT_EQUAL(7, fixedArray2.GetSize());
			SEOUL_UNITTESTING_ASSERT_EQUAL(7 * sizeof(ContainerTestComplex), fixedArray2.GetSizeInBytes());
			SEOUL_UNITTESTING_ASSERT(!fixedArray2.IsEmpty());
		}
	}
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
}

void FixedArrayTest::TestDefaultConstructor()
{
	// Default initialization with builtin
	{
		FixedArray<Int, 15> fixedArray;

		// Test for zero initialized builtin.
		for (UInt32 i = 0; i < fixedArray.GetSize(); ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, fixedArray[i]);
		}

		SEOUL_UNITTESTING_ASSERT_EQUAL(15, fixedArray.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(15 * sizeof(Int), fixedArray.GetSizeInBytes());
		SEOUL_UNITTESTING_ASSERT(!fixedArray.IsEmpty());
	}

	// Default initialization with Simple
	{
		FixedArray<ContainerTestSimple, 27> fixedArray;

		// Test for zero initialized Simple.
		for (UInt32 i = 0; i < fixedArray.GetSize(); ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, fixedArray[i].m_iA);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, fixedArray[i].m_B);
		}

		SEOUL_UNITTESTING_ASSERT_EQUAL(27, fixedArray.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(27 * sizeof(ContainerTestSimple), fixedArray.GetSizeInBytes());
		SEOUL_UNITTESTING_ASSERT(!fixedArray.IsEmpty());
	}

	// Default initialization with complex
	{
		{
			FixedArray<ContainerTestComplex, 13> fixedArray;

			// Check count.
			SEOUL_UNITTESTING_ASSERT_EQUAL(13, ContainerTestComplex::s_iCount);

			// Test for default constructed complex.
			for (UInt32 i = 0; i < fixedArray.GetSize(); ++i)
			{
				SEOUL_UNITTESTING_ASSERT_EQUAL(33, fixedArray[i].m_iFixedValue);
				SEOUL_UNITTESTING_ASSERT_EQUAL(433, fixedArray[i].m_iVariableValue);
			}

			SEOUL_UNITTESTING_ASSERT_EQUAL(13, fixedArray.GetSize());
			SEOUL_UNITTESTING_ASSERT_EQUAL(13 * sizeof(ContainerTestComplex), fixedArray.GetSizeInBytes());
			SEOUL_UNITTESTING_ASSERT(!fixedArray.IsEmpty());
		}

		// Check count.
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
	}
}

void FixedArrayTest::TestMethods()
{
	{
		FixedArray<ContainerTestComplex, 357> fixedArray;
		FixedArray<ContainerTestComplex, 357> const& fixedArrayConst = fixedArray;

		SEOUL_UNITTESTING_ASSERT_EQUAL(357, ContainerTestComplex::s_iCount);

		SEOUL_UNITTESTING_ASSERT_EQUAL(357, fixedArray.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(357, fixedArrayConst.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(357 * sizeof(ContainerTestComplex), fixedArray.GetSizeInBytes());
		SEOUL_UNITTESTING_ASSERT_EQUAL(357 * sizeof(ContainerTestComplex), fixedArrayConst.GetSizeInBytes());

		for (UInt32 i = 0u; i < fixedArray.GetSize(); ++i)
		{
			fixedArray.At(i) = ContainerTestComplex(i);
		}

		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(356), fixedArray.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(356), fixedArrayConst.Back());
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(0), *fixedArray.Begin());
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(0), *fixedArrayConst.Begin());
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(356), *(fixedArray.End() - 1));
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(356), *(fixedArrayConst.End() - 1));
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(0), fixedArray.Front());
		SEOUL_UNITTESTING_ASSERT_EQUAL(ContainerTestComplex(0), fixedArrayConst.Front());

		for (UInt32 i = 0u; i < fixedArray.GetSize(); ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(
				ContainerTestComplex(i),
				fixedArray.At(i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(
				ContainerTestComplex(i),
				fixedArrayConst.At(i));

			SEOUL_UNITTESTING_ASSERT_EQUAL(
				ContainerTestComplex(i),
				*(fixedArray.Begin() + i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(
				ContainerTestComplex(i),
				*(fixedArrayConst.Begin() + i));

			SEOUL_UNITTESTING_ASSERT_EQUAL(
				ContainerTestComplex(i),
				*(fixedArray.Data() + i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(
				ContainerTestComplex(i),
				*(fixedArrayConst.Data() + i));

			SEOUL_UNITTESTING_ASSERT_EQUAL(
				ContainerTestComplex(i),
				*(fixedArray.End() - 357 + i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(
				ContainerTestComplex(i),
				*(fixedArrayConst.End() - 357 + i));

			SEOUL_UNITTESTING_ASSERT_EQUAL(
				ContainerTestComplex(i),
				*(fixedArray.Get(i)));
			SEOUL_UNITTESTING_ASSERT_EQUAL(
				ContainerTestComplex(i),
				*(fixedArrayConst.Get(i)));

			SEOUL_UNITTESTING_ASSERT_EQUAL(
				ContainerTestComplex(i),
				fixedArray[i]);
			SEOUL_UNITTESTING_ASSERT_EQUAL(
				ContainerTestComplex(i),
				fixedArrayConst[i]);
		}

		// Swap
		SEOUL_UNITTESTING_ASSERT_EQUAL(357, ContainerTestComplex::s_iCount);
		FixedArray<ContainerTestComplex, 357> fixedArray2;
		SEOUL_UNITTESTING_ASSERT_EQUAL(714, ContainerTestComplex::s_iCount);
		for (UInt32 i = 0u; i < fixedArray2.GetSize(); ++i)
		{
			fixedArray2.At(i) = ContainerTestComplex(357 - i);
		}

		fixedArray2.Swap(fixedArray);

		// Check
		for (UInt32 i = 0u; i < fixedArray2.GetSize(); ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(
				ContainerTestComplex(i),
				fixedArray2[i]);
			SEOUL_UNITTESTING_ASSERT_EQUAL(
				ContainerTestComplex(357 - i),
				fixedArray[i]);
		}
	}
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
}

void FixedArrayTest::TestIterators()
{
	FixedArray<Int, 6> fixedArray;

	for (Int i = 0; i < 6; i++)
	{
		fixedArray[i] = (i + 10);
	}

	// Test value reads through iterator
	FixedArray<Int, 6>::Iterator iter = fixedArray.Begin();
	Int i = 0;
	while(iter != fixedArray.End())
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(i + 10, *iter);
		++iter;
		++i;
	}

	// Test value writes through iterator
	iter = fixedArray.Begin();
	i = 0;
	while(iter != fixedArray.End())
	{
		*iter = 3 * i;
		SEOUL_UNITTESTING_ASSERT_EQUAL(3 * i, fixedArray[i]);
		++iter;
	}
}

void FixedArrayTest::TestValueConstructor()
{
	// Value initialization with builtin
	{
		FixedArray<UInt64, 15> fixedArray((UInt64)27);

		// Test for initialized builtin.
		for (UInt32 i = 0; i < fixedArray.GetSize(); ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(27, fixedArray[i]);
		}

		SEOUL_UNITTESTING_ASSERT_EQUAL(15, fixedArray.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(15 * sizeof(UInt64), fixedArray.GetSizeInBytes());
		SEOUL_UNITTESTING_ASSERT(!fixedArray.IsEmpty());
	}

	// Value initialization with Simple
	{
		ContainerTestSimple simple;
		simple.m_B = 33;
		simple.m_iA = 125209;
		FixedArray<ContainerTestSimple, 27> fixedArray(simple);

		// Test for initialized Simple.
		for (UInt32 i = 0; i < fixedArray.GetSize(); ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(125209, fixedArray[i].m_iA);
			SEOUL_UNITTESTING_ASSERT_EQUAL(33, fixedArray[i].m_B);
		}

		SEOUL_UNITTESTING_ASSERT_EQUAL(27, fixedArray.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(27 * sizeof(ContainerTestSimple), fixedArray.GetSizeInBytes());
		SEOUL_UNITTESTING_ASSERT(!fixedArray.IsEmpty());
	}

	// Value initialization with complex
	{
		{
			FixedArray<ContainerTestComplex, 13> fixedArray(ContainerTestComplex(2342));

			// Check count.
			SEOUL_UNITTESTING_ASSERT_EQUAL(13, ContainerTestComplex::s_iCount);

			// Test for default constructed complex.
			for (UInt32 i = 0; i < fixedArray.GetSize(); ++i)
			{
				SEOUL_UNITTESTING_ASSERT_EQUAL(33, fixedArray[i].m_iFixedValue);
				SEOUL_UNITTESTING_ASSERT_EQUAL(2342, fixedArray[i].m_iVariableValue);
			}

			SEOUL_UNITTESTING_ASSERT_EQUAL(13, fixedArray.GetSize());
			SEOUL_UNITTESTING_ASSERT_EQUAL(13 * sizeof(ContainerTestComplex), fixedArray.GetSizeInBytes());
			SEOUL_UNITTESTING_ASSERT(!fixedArray.IsEmpty());
		}

		// Check count.
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, ContainerTestComplex::s_iCount);
	}
}

/** Tests for ranged-based for loops (for a : b). */
void FixedArrayTest::TestRangedFor()
{
	FixedArray<Int, 3> aFixed;
	aFixed[0] = 3;
	aFixed[1] = 7;
	aFixed[2] = 2;

	Int i = 0;
	for (auto v : aFixed)
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(aFixed[i], v);
		++i;
	}

	aFixed[1] = 35;
	i = 0;
	for (auto v : aFixed)
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(aFixed[i], v);
		++i;
	}

	aFixed[2] = 77;
	i = 0;
	for (auto v : aFixed)
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(aFixed[i], v);
		++i;
	}
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
