/**
 * \file HashTableTest.cpp
 * \brief Unit test header file for HashTable class
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "HashTableTest.h"
#include "HashFunctions.h"
#include "Logger.h"
#include "ReflectionDefine.h"
#include "SeoulAssert.h"
#include "SeoulString.h"
#include "UnitTesting.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

template <typename KEY, typename VALUE, typename TRAITS>
static inline String UnitTestingToString(const HashTableIterator<KEY, VALUE, TRAITS>& i)
{
	return String::Printf("(%p, %p)", std::addressof(i->First), std::addressof(i->Second));
}

SEOUL_BEGIN_TYPE( HashTableTest )
	SEOUL_ATTRIBUTE( UnitTest )
	SEOUL_METHOD( TestInstantiation )
	SEOUL_METHOD( TestClear )
	SEOUL_METHOD( TestClusteringPrevention )
	SEOUL_METHOD( TestAssignment )
	SEOUL_METHOD( TestInsert )
	SEOUL_METHOD( TestSwap )
	SEOUL_METHOD( TestIntKeys )
	SEOUL_METHOD( TestHashableKeys )
	SEOUL_METHOD( TestNullKey )
	SEOUL_METHOD( TestFindNull )
	SEOUL_METHOD( TestSeoulStringKeys )
	SEOUL_METHOD( TestPointerKeys )
	SEOUL_METHOD( TestErase )
	SEOUL_METHOD( TestIterators )
	SEOUL_METHOD( TestRangedFor )
	SEOUL_METHOD( TestUtilities )
SEOUL_END_TYPE();

/**
 * Make sure that we can create an empty hashtable and that it has the
 * expected number of empty rows.
 */
void HashTableTest::TestInstantiation()
{
	String sTest;
	HashTable<Int32,Seoul::String> htab;

	SEOUL_UNITTESTING_ASSERT(htab.IsEmpty());
	SEOUL_UNITTESTING_ASSERT(!htab.Erase(1));
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, htab.GetSize());
	SEOUL_UNITTESTING_ASSERT(!htab.GetValue(1, sTest));
	SEOUL_UNITTESTING_ASSERT(!htab.HasValue(1));
}

struct HashValueTester
{
	HashValueTester()
		: m_iDummyValue(-13)
	{
		s_iHashValueTesterCount++;
	}

	explicit HashValueTester(Int iDummyValue)
		: m_iDummyValue(iDummyValue)
	{
		s_iHashValueTesterCount++;
	}

	HashValueTester(const HashValueTester& b)
		: m_iDummyValue(b.m_iDummyValue)
	{
		s_iHashValueTesterCount++;
	}

	HashValueTester& operator=(const HashValueTester& b)
	{
		m_iDummyValue = b.m_iDummyValue;
		return *this;
	}

	~HashValueTester()
	{
		m_iDummyValue = -25;
		s_iHashValueTesterCount--;
	}

	Bool operator==(const HashValueTester& b) const
	{
		return (m_iDummyValue == b.m_iDummyValue);
	}

	Bool operator!=(const HashValueTester& b) const
	{
		return !(*this == b);
	}

	Int m_iDummyValue;
	static Int s_iHashValueTesterCount;
};

Int HashValueTester::s_iHashValueTesterCount = 0;

SEOUL_BEGIN_TYPE(HashValueTester)
	SEOUL_PROPERTY_N("DummyValue", m_iDummyValue)
SEOUL_END_TYPE()

/**
 * Confirm that clearing the table actually removes all the entries.
 */
void HashTableTest::TestClear()
{
	HashTable<Seoul::String, HashValueTester> htab;
	SEOUL_UNITTESTING_ASSERT(htab.IsEmpty());

	SEOUL_UNITTESTING_ASSERT(htab.Insert("", HashValueTester()).Second);
	SEOUL_UNITTESTING_ASSERT(htab.Overwrite("", HashValueTester()).Second);
	{
		auto res(htab.Insert("", HashValueTester()));
		SEOUL_UNITTESTING_ASSERT(!res.Second);
		SEOUL_UNITTESTING_ASSERT_EQUAL(String(""), res.First->First);
		SEOUL_UNITTESTING_ASSERT_EQUAL(HashValueTester(), res.First->Second);
	}

	SEOUL_UNITTESTING_ASSERT(htab.Insert("one", HashValueTester()).Second);
	SEOUL_UNITTESTING_ASSERT(htab.Overwrite("one", HashValueTester()).Second);
	{
		auto res(htab.Insert("one", HashValueTester()));
		SEOUL_UNITTESTING_ASSERT(!res.Second);
		SEOUL_UNITTESTING_ASSERT_EQUAL(String("one"), res.First->First);
		SEOUL_UNITTESTING_ASSERT_EQUAL(HashValueTester(), res.First->Second);
	}

	SEOUL_UNITTESTING_ASSERT(htab.Insert("two",	HashValueTester()).Second);
	SEOUL_UNITTESTING_ASSERT(htab.Overwrite("two", HashValueTester()).Second);
	{
		auto res(htab.Insert("two", HashValueTester()));
		SEOUL_UNITTESTING_ASSERT(!res.Second);
		SEOUL_UNITTESTING_ASSERT_EQUAL(String("two"), res.First->First);
		SEOUL_UNITTESTING_ASSERT_EQUAL(HashValueTester(), res.First->Second);
	}

	SEOUL_UNITTESTING_ASSERT(htab.Insert("three", HashValueTester()).Second);
	SEOUL_UNITTESTING_ASSERT(htab.Overwrite("three", HashValueTester()).Second);
	{
		auto res(htab.Insert("three", HashValueTester()));
		SEOUL_UNITTESTING_ASSERT(!res.Second);
		SEOUL_UNITTESTING_ASSERT_EQUAL(String("three"), res.First->First);
		SEOUL_UNITTESTING_ASSERT_EQUAL(HashValueTester(), res.First->Second);
	}

	SEOUL_UNITTESTING_ASSERT(htab.Insert("tremendous", HashValueTester()).Second);
	SEOUL_UNITTESTING_ASSERT(htab.Overwrite("tremendous", HashValueTester()).Second);
	{
		auto res(htab.Insert("tremendous", HashValueTester()));
		SEOUL_UNITTESTING_ASSERT(!res.Second);
		SEOUL_UNITTESTING_ASSERT_EQUAL(String("tremendous"), res.First->First);
		SEOUL_UNITTESTING_ASSERT_EQUAL(HashValueTester(), res.First->Second);
	}

	SEOUL_UNITTESTING_ASSERT(htab.Insert("terrific", HashValueTester()).Second);
	SEOUL_UNITTESTING_ASSERT(htab.Overwrite("terrific", HashValueTester()).Second);
	{
		auto res(htab.Insert("terrific", HashValueTester()));
		SEOUL_UNITTESTING_ASSERT(!res.Second);
		SEOUL_UNITTESTING_ASSERT_EQUAL(String("terrific"), res.First->First);
		SEOUL_UNITTESTING_ASSERT_EQUAL(HashValueTester(), res.First->Second);
	}

	SEOUL_UNITTESTING_ASSERT(htab.Insert("toofreakinawesome", HashValueTester()).Second);
	SEOUL_UNITTESTING_ASSERT(htab.Overwrite("toofreakinawesome", HashValueTester()).Second);
	{
		auto res(htab.Insert("toofreakinawesome", HashValueTester()));
		SEOUL_UNITTESTING_ASSERT(!res.Second);
		SEOUL_UNITTESTING_ASSERT_EQUAL(String("toofreakinawesome"), res.First->First);
		SEOUL_UNITTESTING_ASSERT_EQUAL(HashValueTester(), res.First->Second);
	}

	SEOUL_UNITTESTING_ASSERT(htab.Insert("four", HashValueTester()).Second);
	SEOUL_UNITTESTING_ASSERT(htab.Overwrite("four", HashValueTester()).Second);
	{
		auto res(htab.Insert("four", HashValueTester()));
		SEOUL_UNITTESTING_ASSERT(!res.Second);
		SEOUL_UNITTESTING_ASSERT_EQUAL(String("four"), res.First->First);
		SEOUL_UNITTESTING_ASSERT_EQUAL(HashValueTester(), res.First->Second);
	}

	SEOUL_UNITTESTING_ASSERT(htab.Insert("five", HashValueTester()).Second);
	SEOUL_UNITTESTING_ASSERT(htab.Overwrite("five", HashValueTester()).Second);
	{
		auto res(htab.Insert("five", HashValueTester()));
		SEOUL_UNITTESTING_ASSERT(!res.Second);
		SEOUL_UNITTESTING_ASSERT_EQUAL(String("five"), res.First->First);
		SEOUL_UNITTESTING_ASSERT_EQUAL(HashValueTester(), res.First->Second);
	}

	SEOUL_UNITTESTING_ASSERT(htab.Insert("six", HashValueTester()).Second);
	SEOUL_UNITTESTING_ASSERT(htab.Overwrite("six", HashValueTester()).Second);
	{
		auto res(htab.Insert("six", HashValueTester()));
		SEOUL_UNITTESTING_ASSERT(!res.Second);
		SEOUL_UNITTESTING_ASSERT_EQUAL(String("six"), res.First->First);
		SEOUL_UNITTESTING_ASSERT_EQUAL(HashValueTester(), res.First->Second);
	}

	SEOUL_UNITTESTING_ASSERT(htab.Insert("seven", HashValueTester()).Second);
	SEOUL_UNITTESTING_ASSERT(htab.Overwrite("seven", HashValueTester()).Second);
	{
		auto res(htab.Insert("seven", HashValueTester()));
		SEOUL_UNITTESTING_ASSERT(!res.Second);
		SEOUL_UNITTESTING_ASSERT_EQUAL(String("seven"), res.First->First);
		SEOUL_UNITTESTING_ASSERT_EQUAL(HashValueTester(), res.First->Second);
	}

	// shouldn't be empty
	SEOUL_UNITTESTING_ASSERT(!htab.IsEmpty());

	// confirm that 11 entries stored
	SEOUL_UNITTESTING_ASSERT_EQUAL(11u, htab.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(11, HashValueTester::s_iHashValueTesterCount);

	htab.Clear();

	// should be empty!
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, HashValueTester::s_iHashValueTesterCount);
	SEOUL_UNITTESTING_ASSERT(htab.IsEmpty());
}

struct ClusterTestType
{
	ClusterTestType()
		: m_I(0)
	{
	}

	ClusterTestType(UInt32 i)
		: m_I(i)
	{
	}

	Bool operator==(const ClusterTestType& b) const
	{
		return (m_I == b.m_I);
	}

	Bool operator!=(const ClusterTestType& b) const
	{
		return !(*this == b);
	}

	UInt32 m_I;
};

template <>
struct DefaultHashTableKeyTraits<ClusterTestType>
{
	inline static Float GetLoadFactor()
	{
		return 0.75f;
	}

	inline static ClusterTestType GetNullKey()
	{
		return ClusterTestType(UIntMax);
	}

	static const Bool kCheckHashBeforeEquals = false;
};

inline UInt32 GetHash(ClusterTestType t)
{
	return t.m_I % 10;
}

void HashTableTest::TestClusteringPrevention()
{
	HashTable<ClusterTestType, Int> htab;
	SEOUL_UNITTESTING_ASSERT(htab.IsEmpty());

	SEOUL_UNITTESTING_ASSERT(htab.Insert(10, 10).First->Second == 10);
	SEOUL_UNITTESTING_ASSERT(htab.Insert(9, 9).First->Second == 9);
	SEOUL_UNITTESTING_ASSERT(htab.Insert(8, 8).First->Second == 8);
	SEOUL_UNITTESTING_ASSERT(htab.Insert(7, 7).First->Second == 7);
	SEOUL_UNITTESTING_ASSERT(htab.Insert(6, 6).First->Second == 6);
	SEOUL_UNITTESTING_ASSERT(htab.Insert(5, 5).First->Second == 5);
	SEOUL_UNITTESTING_ASSERT(htab.Insert(4, 4).First->Second == 4);
	SEOUL_UNITTESTING_ASSERT(htab.Insert(3, 3).First->Second == 3);
	SEOUL_UNITTESTING_ASSERT(htab.Insert(2, 2).First->Second == 2);
	SEOUL_UNITTESTING_ASSERT(htab.Insert(1, 1).First->Second == 1);
	SEOUL_UNITTESTING_ASSERT(htab.Insert(0, 0).First->Second == 0);

	SEOUL_UNITTESTING_ASSERT_EQUAL(16u, htab.GetCapacity());
	// Scope the HashTable<>::Iterator instance.
	{
		auto i = htab.Begin();
		SEOUL_UNITTESTING_ASSERT(i->First == 10 && i->Second == 10); ++i;
		SEOUL_UNITTESTING_ASSERT(i->First == 1 && i->Second == 1); ++i;
		SEOUL_UNITTESTING_ASSERT(i->First == 2 && i->Second == 2); ++i;
		SEOUL_UNITTESTING_ASSERT(i->First == 3 && i->Second == 3); ++i;
		SEOUL_UNITTESTING_ASSERT(i->First == 4 && i->Second == 4); ++i;
		SEOUL_UNITTESTING_ASSERT(i->First == 5 && i->Second == 5); ++i;
		SEOUL_UNITTESTING_ASSERT(i->First == 6 && i->Second == 6); ++i;
		SEOUL_UNITTESTING_ASSERT(i->First == 7 && i->Second == 7); ++i;
		SEOUL_UNITTESTING_ASSERT(i->First == 8 && i->Second == 8); ++i;
		SEOUL_UNITTESTING_ASSERT(i->First == 9 && i->Second == 9); ++i;
		SEOUL_UNITTESTING_ASSERT(i->First == 0 && i->Second == 0); ++i;
		SEOUL_UNITTESTING_ASSERT(htab.End() == i);
	}

	SEOUL_UNITTESTING_ASSERT(htab.Erase(10));

	SEOUL_UNITTESTING_ASSERT_EQUAL(16u, htab.GetCapacity());
	// Scope the HashTable<>::Iterator instance.
	{
		auto i = htab.Begin();
		SEOUL_UNITTESTING_ASSERT(i->First == 0 && i->Second == 0); ++i;
		SEOUL_UNITTESTING_ASSERT(i->First == 1 && i->Second == 1); ++i;
		SEOUL_UNITTESTING_ASSERT(i->First == 2 && i->Second == 2); ++i;
		SEOUL_UNITTESTING_ASSERT(i->First == 3 && i->Second == 3); ++i;
		SEOUL_UNITTESTING_ASSERT(i->First == 4 && i->Second == 4); ++i;
		SEOUL_UNITTESTING_ASSERT(i->First == 5 && i->Second == 5); ++i;
		SEOUL_UNITTESTING_ASSERT(i->First == 6 && i->Second == 6); ++i;
		SEOUL_UNITTESTING_ASSERT(i->First == 7 && i->Second == 7); ++i;
		SEOUL_UNITTESTING_ASSERT(i->First == 8 && i->Second == 8); ++i;
		SEOUL_UNITTESTING_ASSERT(i->First == 9 && i->Second == 9); ++i;
		SEOUL_UNITTESTING_ASSERT(htab.End() == i);
	}

	SEOUL_UNITTESTING_ASSERT(!htab.Insert(0, 0).Second);
	SEOUL_UNITTESTING_ASSERT(htab.HasValue(0));
}

void HashTableTest::TestAssignment()
{
	using namespace Reflection;

	// Test with Any value.
	{
		HashTable<HString, Any> tester;
		SEOUL_UNITTESTING_ASSERT(tester.IsEmpty());

		SEOUL_UNITTESTING_ASSERT(tester.Insert(HString("a"), 1).Second);
		SEOUL_UNITTESTING_ASSERT(tester.Insert(HString("b"), 2).Second);
		SEOUL_UNITTESTING_ASSERT(tester.Insert(HString("c"), 3).Second);
		SEOUL_UNITTESTING_ASSERT(tester.Insert(HString("d"), 4).Second);
		SEOUL_UNITTESTING_ASSERT(tester.Insert(HString("e"), 5).Second);
		SEOUL_UNITTESTING_ASSERT(tester.Insert(HString("f"), 6).Second);

		// Copy constructor with Any
		{
			HashTable<HString, Any> tester2(tester);
			SEOUL_UNITTESTING_ASSERT(tester2.GetSize() == 6);
			SEOUL_UNITTESTING_ASSERT(tester2.GetSize() == tester.GetSize());
			SEOUL_UNITTESTING_ASSERT(tester2.GetCapacity() == tester.GetCapacity());

			Any any;
			SEOUL_UNITTESTING_ASSERT(tester2.GetValue(HString("a"), any));
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, any.Cast<Int>());
			SEOUL_UNITTESTING_ASSERT(tester2.GetValue(HString("b"), any));
			SEOUL_UNITTESTING_ASSERT_EQUAL(2, any.Cast<Int>());
			SEOUL_UNITTESTING_ASSERT(tester2.GetValue(HString("c"), any));
			SEOUL_UNITTESTING_ASSERT_EQUAL(3, any.Cast<Int>());
			SEOUL_UNITTESTING_ASSERT(tester2.GetValue(HString("d"), any));
			SEOUL_UNITTESTING_ASSERT_EQUAL(4, any.Cast<Int>());
			SEOUL_UNITTESTING_ASSERT(tester2.GetValue(HString("e"), any));
			SEOUL_UNITTESTING_ASSERT_EQUAL(5, any.Cast<Int>());
			SEOUL_UNITTESTING_ASSERT(tester2.GetValue(HString("f"), any));
			SEOUL_UNITTESTING_ASSERT_EQUAL(6, any.Cast<Int>());
		}

		// Assignment operator with Any
		{
			HashTable<HString, Any> tester2 = tester;
			SEOUL_UNITTESTING_ASSERT(tester2.GetSize() == 6);
			SEOUL_UNITTESTING_ASSERT(tester2.GetSize() == tester.GetSize());
			SEOUL_UNITTESTING_ASSERT(tester2.GetCapacity() == tester.GetCapacity());

			Any any;
			SEOUL_UNITTESTING_ASSERT(tester2.GetValue(HString("a"), any));
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, any.Cast<Int>());
			SEOUL_UNITTESTING_ASSERT(tester2.GetValue(HString("b"), any));
			SEOUL_UNITTESTING_ASSERT_EQUAL(2, any.Cast<Int>());
			SEOUL_UNITTESTING_ASSERT(tester2.GetValue(HString("c"), any));
			SEOUL_UNITTESTING_ASSERT_EQUAL(3, any.Cast<Int>());
			SEOUL_UNITTESTING_ASSERT(tester2.GetValue(HString("d"), any));
			SEOUL_UNITTESTING_ASSERT_EQUAL(4, any.Cast<Int>());
			SEOUL_UNITTESTING_ASSERT(tester2.GetValue(HString("e"), any));
			SEOUL_UNITTESTING_ASSERT_EQUAL(5, any.Cast<Int>());
			SEOUL_UNITTESTING_ASSERT(tester2.GetValue(HString("f"), any));
			SEOUL_UNITTESTING_ASSERT_EQUAL(6, any.Cast<Int>());
		}

		// Self assignment
		{
			tester = tester;
			SEOUL_UNITTESTING_ASSERT(tester.GetSize() == 6);
			SEOUL_UNITTESTING_ASSERT(tester.GetSize() == tester.GetSize());
			SEOUL_UNITTESTING_ASSERT(tester.GetCapacity() == tester.GetCapacity());

			Any any;
			SEOUL_UNITTESTING_ASSERT(tester.GetValue(HString("a"), any));
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, any.Cast<Int>());
			SEOUL_UNITTESTING_ASSERT(tester.GetValue(HString("b"), any));
			SEOUL_UNITTESTING_ASSERT_EQUAL(2, any.Cast<Int>());
			SEOUL_UNITTESTING_ASSERT(tester.GetValue(HString("c"), any));
			SEOUL_UNITTESTING_ASSERT_EQUAL(3, any.Cast<Int>());
			SEOUL_UNITTESTING_ASSERT(tester.GetValue(HString("d"), any));
			SEOUL_UNITTESTING_ASSERT_EQUAL(4, any.Cast<Int>());
			SEOUL_UNITTESTING_ASSERT(tester.GetValue(HString("e"), any));
			SEOUL_UNITTESTING_ASSERT_EQUAL(5, any.Cast<Int>());
			SEOUL_UNITTESTING_ASSERT(tester.GetValue(HString("f"), any));
			SEOUL_UNITTESTING_ASSERT_EQUAL(6, any.Cast<Int>());
		}
	}

	// Test with WeakAny value.
	{
		HashTable<HString, WeakAny> tester;
		SEOUL_UNITTESTING_ASSERT(tester.IsEmpty());

		SEOUL_UNITTESTING_ASSERT(tester.Insert(HString("a"), 1).Second);
		SEOUL_UNITTESTING_ASSERT(tester.Insert(HString("b"), 2).Second);
		SEOUL_UNITTESTING_ASSERT(tester.Insert(HString("c"), 3).Second);
		SEOUL_UNITTESTING_ASSERT(tester.Insert(HString("d"), 4).Second);
		SEOUL_UNITTESTING_ASSERT(tester.Insert(HString("e"), 5).Second);
		SEOUL_UNITTESTING_ASSERT(tester.Insert(HString("f"), 6).Second);

		// Copy constructor with WeakAny
		{
			HashTable<HString, WeakAny> tester2(tester);
			SEOUL_UNITTESTING_ASSERT(tester2.GetSize() == 6);
			SEOUL_UNITTESTING_ASSERT(tester2.GetSize() == tester.GetSize());
			SEOUL_UNITTESTING_ASSERT(tester2.GetCapacity() == tester.GetCapacity());

			WeakAny any;
			SEOUL_UNITTESTING_ASSERT(tester2.GetValue(HString("a"), any));
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, any.Cast<Int>());
			SEOUL_UNITTESTING_ASSERT(tester2.GetValue(HString("b"), any));
			SEOUL_UNITTESTING_ASSERT_EQUAL(2, any.Cast<Int>());
			SEOUL_UNITTESTING_ASSERT(tester2.GetValue(HString("c"), any));
			SEOUL_UNITTESTING_ASSERT_EQUAL(3, any.Cast<Int>());
			SEOUL_UNITTESTING_ASSERT(tester2.GetValue(HString("d"), any));
			SEOUL_UNITTESTING_ASSERT_EQUAL(4, any.Cast<Int>());
			SEOUL_UNITTESTING_ASSERT(tester2.GetValue(HString("e"), any));
			SEOUL_UNITTESTING_ASSERT_EQUAL(5, any.Cast<Int>());
			SEOUL_UNITTESTING_ASSERT(tester2.GetValue(HString("f"), any));
			SEOUL_UNITTESTING_ASSERT_EQUAL(6, any.Cast<Int>());
		}

		// Assignment operator with WeakAny
		{
			HashTable<HString, WeakAny> tester2 = tester;
			SEOUL_UNITTESTING_ASSERT(tester2.GetSize() == 6);
			SEOUL_UNITTESTING_ASSERT(tester2.GetSize() == tester.GetSize());
			SEOUL_UNITTESTING_ASSERT(tester2.GetCapacity() == tester.GetCapacity());

			WeakAny any;
			SEOUL_UNITTESTING_ASSERT(tester2.GetValue(HString("a"), any));
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, any.Cast<Int>());
			SEOUL_UNITTESTING_ASSERT(tester2.GetValue(HString("b"), any));
			SEOUL_UNITTESTING_ASSERT_EQUAL(2, any.Cast<Int>());
			SEOUL_UNITTESTING_ASSERT(tester2.GetValue(HString("c"), any));
			SEOUL_UNITTESTING_ASSERT_EQUAL(3, any.Cast<Int>());
			SEOUL_UNITTESTING_ASSERT(tester2.GetValue(HString("d"), any));
			SEOUL_UNITTESTING_ASSERT_EQUAL(4, any.Cast<Int>());
			SEOUL_UNITTESTING_ASSERT(tester2.GetValue(HString("e"), any));
			SEOUL_UNITTESTING_ASSERT_EQUAL(5, any.Cast<Int>());
			SEOUL_UNITTESTING_ASSERT(tester2.GetValue(HString("f"), any));
			SEOUL_UNITTESTING_ASSERT_EQUAL(6, any.Cast<Int>());
		}

		// Self assignment
		{
			tester = tester;
			SEOUL_UNITTESTING_ASSERT(tester.GetSize() == 6);
			SEOUL_UNITTESTING_ASSERT(tester.GetSize() == tester.GetSize());
			SEOUL_UNITTESTING_ASSERT(tester.GetCapacity() == tester.GetCapacity());

			WeakAny any;
			SEOUL_UNITTESTING_ASSERT(tester.GetValue(HString("a"), any));
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, any.Cast<Int>());
			SEOUL_UNITTESTING_ASSERT(tester.GetValue(HString("b"), any));
			SEOUL_UNITTESTING_ASSERT_EQUAL(2, any.Cast<Int>());
			SEOUL_UNITTESTING_ASSERT(tester.GetValue(HString("c"), any));
			SEOUL_UNITTESTING_ASSERT_EQUAL(3, any.Cast<Int>());
			SEOUL_UNITTESTING_ASSERT(tester.GetValue(HString("d"), any));
			SEOUL_UNITTESTING_ASSERT_EQUAL(4, any.Cast<Int>());
			SEOUL_UNITTESTING_ASSERT(tester.GetValue(HString("e"), any));
			SEOUL_UNITTESTING_ASSERT_EQUAL(5, any.Cast<Int>());
			SEOUL_UNITTESTING_ASSERT(tester.GetValue(HString("f"), any));
			SEOUL_UNITTESTING_ASSERT_EQUAL(6, any.Cast<Int>());
		}
	}
}

void HashTableTest::TestInsert()
{
	HashTable<Seoul::String, Int> htab;
	SEOUL_UNITTESTING_ASSERT(htab.IsEmpty());

	Int iValue = -1;
	SEOUL_UNITTESTING_ASSERT(htab.Insert("value", 1).First->Second == 1);
	SEOUL_UNITTESTING_ASSERT(!htab.Insert("value", 2).Second);
	SEOUL_UNITTESTING_ASSERT(htab.HasValue("value"));
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, *htab.Find("value"));
	SEOUL_UNITTESTING_ASSERT(htab.GetValue("value", iValue));
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, iValue);

	SEOUL_UNITTESTING_ASSERT(htab.Overwrite("value", 2).Second);
	SEOUL_UNITTESTING_ASSERT(!htab.Insert("value", 1).Second);
	SEOUL_UNITTESTING_ASSERT(htab.HasValue("value"));
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, *htab.Find("value"));
	SEOUL_UNITTESTING_ASSERT(htab.GetValue("value", iValue));
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, iValue);

	SEOUL_UNITTESTING_ASSERT(htab.Erase("value"));
	SEOUL_UNITTESTING_ASSERT(htab.IsEmpty());
}

/**
 * Confirm that swapping the table with a second table results in the state of
 * each being swapped.
 */
void HashTableTest::TestSwap()
{
	HashTable<Seoul::String, HashValueTester> htab;
	SEOUL_UNITTESTING_ASSERT(htab.IsEmpty());

	SEOUL_UNITTESTING_ASSERT(htab.Insert("", HashValueTester()).Second);
	SEOUL_UNITTESTING_ASSERT(htab.Overwrite("", HashValueTester()).Second);
	{
		auto res(htab.Insert("", HashValueTester()));
		SEOUL_UNITTESTING_ASSERT(!res.Second);
		SEOUL_UNITTESTING_ASSERT_EQUAL(String(""), res.First->First);
		SEOUL_UNITTESTING_ASSERT_EQUAL(HashValueTester(), res.First->Second);
	}

	SEOUL_UNITTESTING_ASSERT(htab.Insert("one", HashValueTester()).Second);
	SEOUL_UNITTESTING_ASSERT(htab.Overwrite("one", HashValueTester()).Second);
	{
		auto res(htab.Insert("one", HashValueTester()));
		SEOUL_UNITTESTING_ASSERT(!res.Second);
		SEOUL_UNITTESTING_ASSERT_EQUAL(String("one"), res.First->First);
		SEOUL_UNITTESTING_ASSERT_EQUAL(HashValueTester(), res.First->Second);
	}

	SEOUL_UNITTESTING_ASSERT(htab.Insert("two",	HashValueTester()).Second);
	SEOUL_UNITTESTING_ASSERT(htab.Overwrite("two", HashValueTester()).Second);
	{
		auto res(htab.Insert("two", HashValueTester()));
		SEOUL_UNITTESTING_ASSERT(!res.Second);
		SEOUL_UNITTESTING_ASSERT_EQUAL(String("two"), res.First->First);
		SEOUL_UNITTESTING_ASSERT_EQUAL(HashValueTester(), res.First->Second);
	}

	SEOUL_UNITTESTING_ASSERT(htab.Insert("three", HashValueTester()).Second);
	SEOUL_UNITTESTING_ASSERT(htab.Overwrite("three", HashValueTester()).Second);
	{
		auto res(htab.Insert("three", HashValueTester()));
		SEOUL_UNITTESTING_ASSERT(!res.Second);
		SEOUL_UNITTESTING_ASSERT_EQUAL(String("three"), res.First->First);
		SEOUL_UNITTESTING_ASSERT_EQUAL(HashValueTester(), res.First->Second);
	}

	SEOUL_UNITTESTING_ASSERT(htab.Insert("tremendous", HashValueTester()).Second);
	SEOUL_UNITTESTING_ASSERT(htab.Overwrite("tremendous", HashValueTester()).Second);
	{
		auto res(htab.Insert("tremendous", HashValueTester()));
		SEOUL_UNITTESTING_ASSERT(!res.Second);
		SEOUL_UNITTESTING_ASSERT_EQUAL(String("tremendous"), res.First->First);
		SEOUL_UNITTESTING_ASSERT_EQUAL(HashValueTester(), res.First->Second);
	}

	SEOUL_UNITTESTING_ASSERT(htab.Insert("terrific", HashValueTester()).Second);
	SEOUL_UNITTESTING_ASSERT(htab.Overwrite("terrific", HashValueTester()).Second);
	{
		auto res(htab.Insert("terrific", HashValueTester()));
		SEOUL_UNITTESTING_ASSERT(!res.Second);
		SEOUL_UNITTESTING_ASSERT_EQUAL(String("terrific"), res.First->First);
		SEOUL_UNITTESTING_ASSERT_EQUAL(HashValueTester(), res.First->Second);
	}

	SEOUL_UNITTESTING_ASSERT(htab.Insert("toofreakinawesome", HashValueTester()).Second);
	SEOUL_UNITTESTING_ASSERT(htab.Overwrite("toofreakinawesome", HashValueTester()).Second);
	{
		auto res(htab.Insert("toofreakinawesome", HashValueTester()));
		SEOUL_UNITTESTING_ASSERT(!res.Second);
		SEOUL_UNITTESTING_ASSERT_EQUAL(String("toofreakinawesome"), res.First->First);
		SEOUL_UNITTESTING_ASSERT_EQUAL(HashValueTester(), res.First->Second);
	}

	SEOUL_UNITTESTING_ASSERT(htab.Insert("four", HashValueTester()).Second);
	SEOUL_UNITTESTING_ASSERT(htab.Overwrite("four", HashValueTester()).Second);
	{
		auto res(htab.Insert("four", HashValueTester()));
		SEOUL_UNITTESTING_ASSERT(!res.Second);
		SEOUL_UNITTESTING_ASSERT_EQUAL(String("four"), res.First->First);
		SEOUL_UNITTESTING_ASSERT_EQUAL(HashValueTester(), res.First->Second);
	}

	SEOUL_UNITTESTING_ASSERT(htab.Insert("five", HashValueTester()).Second);
	SEOUL_UNITTESTING_ASSERT(htab.Overwrite("five", HashValueTester()).Second);
	{
		auto res(htab.Insert("five", HashValueTester()));
		SEOUL_UNITTESTING_ASSERT(!res.Second);
		SEOUL_UNITTESTING_ASSERT_EQUAL(String("five"), res.First->First);
		SEOUL_UNITTESTING_ASSERT_EQUAL(HashValueTester(), res.First->Second);
	}

	SEOUL_UNITTESTING_ASSERT(htab.Insert("six", HashValueTester()).Second);
	SEOUL_UNITTESTING_ASSERT(htab.Overwrite("six", HashValueTester()).Second);
	{
		auto res(htab.Insert("six", HashValueTester()));
		SEOUL_UNITTESTING_ASSERT(!res.Second);
		SEOUL_UNITTESTING_ASSERT_EQUAL(String("six"), res.First->First);
		SEOUL_UNITTESTING_ASSERT_EQUAL(HashValueTester(), res.First->Second);
	}

	SEOUL_UNITTESTING_ASSERT(htab.Insert("seven", HashValueTester()).Second);
	SEOUL_UNITTESTING_ASSERT(htab.Overwrite("seven", HashValueTester()).Second);
	{
		auto res(htab.Insert("seven", HashValueTester()));
		SEOUL_UNITTESTING_ASSERT(!res.Second);
		SEOUL_UNITTESTING_ASSERT_EQUAL(String("seven"), res.First->First);
		SEOUL_UNITTESTING_ASSERT_EQUAL(HashValueTester(), res.First->Second);
	}

	// shouldn't be empty
	SEOUL_UNITTESTING_ASSERT(!htab.IsEmpty());

	// confirm that 10 entries stored
	SEOUL_UNITTESTING_ASSERT_EQUAL(11, HashValueTester::s_iHashValueTesterCount);
	SEOUL_UNITTESTING_ASSERT_EQUAL(11u, htab.GetSize());

	HashTable<String, HashValueTester> htab2;
	SEOUL_UNITTESTING_ASSERT(htab2.IsEmpty());

	htab.Swap(htab2);

	// should be empty!
	SEOUL_UNITTESTING_ASSERT(htab.IsEmpty());

	// shouldn't be empty
	SEOUL_UNITTESTING_ASSERT(!htab2.IsEmpty());

	// confirm that 11 entries stored
	SEOUL_UNITTESTING_ASSERT_EQUAL(11u, htab2.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(11, HashValueTester::s_iHashValueTesterCount);

	// confirm entry values
	HashValueTester value;
	SEOUL_UNITTESTING_ASSERT(htab2.GetValue("", value) && value.m_iDummyValue == -13);
	SEOUL_UNITTESTING_ASSERT(htab2.GetValue("one", value) && value.m_iDummyValue == -13);
	SEOUL_UNITTESTING_ASSERT(htab2.GetValue("two", value) && value.m_iDummyValue == -13);
	SEOUL_UNITTESTING_ASSERT(htab2.GetValue("three", value) && value.m_iDummyValue == -13);
	SEOUL_UNITTESTING_ASSERT(htab2.GetValue("tremendous", value) && value.m_iDummyValue == -13);
	SEOUL_UNITTESTING_ASSERT(htab2.GetValue("terrific", value) && value.m_iDummyValue == -13);
	SEOUL_UNITTESTING_ASSERT(htab2.GetValue("toofreakinawesome", value) && value.m_iDummyValue == -13);
	SEOUL_UNITTESTING_ASSERT(htab2.GetValue("four", value) && value.m_iDummyValue == -13);
	SEOUL_UNITTESTING_ASSERT(htab2.GetValue("five", value) && value.m_iDummyValue == -13);
	SEOUL_UNITTESTING_ASSERT(htab2.GetValue("six", value) && value.m_iDummyValue == -13);
	SEOUL_UNITTESTING_ASSERT(htab2.GetValue("seven", value) && value.m_iDummyValue == -13);
}

/**
 * Confirm that ints function properly as keys.
 */
void HashTableTest::TestIntKeys()
{
	HashTable<Int32, HashValueTester> htab;
	SEOUL_UNITTESTING_ASSERT(htab.IsEmpty());

	SEOUL_UNITTESTING_ASSERT(htab.Insert(1, HashValueTester()).Second);
	SEOUL_UNITTESTING_ASSERT(htab.HasValue(1));
	SEOUL_UNITTESTING_ASSERT(!htab.HasValue(2));
	SEOUL_UNITTESTING_ASSERT(!htab.Insert(1, HashValueTester()).Second);
	SEOUL_UNITTESTING_ASSERT(htab.Insert(2, HashValueTester()).Second);
	SEOUL_UNITTESTING_ASSERT(htab.HasValue(2));
	SEOUL_UNITTESTING_ASSERT(!htab.HasValue(3));
	SEOUL_UNITTESTING_ASSERT(!htab.Insert(2, HashValueTester()).Second);
	SEOUL_UNITTESTING_ASSERT(htab.Insert(3, HashValueTester()).Second);
	SEOUL_UNITTESTING_ASSERT(htab.HasValue(3));
	SEOUL_UNITTESTING_ASSERT(!htab.HasValue(4));
	SEOUL_UNITTESTING_ASSERT(!htab.Insert(3, HashValueTester()).Second);

	// shouldn't be empty
	SEOUL_UNITTESTING_ASSERT(!htab.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(3u, htab.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, HashValueTester::s_iHashValueTesterCount);
}

/**
 * Confirm that Hashables function properly as keys.
 */
void HashTableTest::TestHashableKeys()
{
	HashTable<Int*, HashValueTester> htab;
	SEOUL_UNITTESTING_ASSERT(htab.IsEmpty());

	Int i1;
	SEOUL_UNITTESTING_ASSERT(htab.Insert(&i1, HashValueTester()).Second);
	SEOUL_UNITTESTING_ASSERT(!htab.Insert(&i1, HashValueTester()).Second);

	Int i2;
	SEOUL_UNITTESTING_ASSERT(htab.Insert(&i2, HashValueTester()).Second);
	SEOUL_UNITTESTING_ASSERT(!htab.Insert(&i2, HashValueTester()).Second);

	Int i3;
	SEOUL_UNITTESTING_ASSERT(htab.Insert(&i3, HashValueTester()).Second);
	SEOUL_UNITTESTING_ASSERT(!htab.Insert(&i3, HashValueTester()).Second);

	// shouldn't be empty
	SEOUL_UNITTESTING_ASSERT(!htab.IsEmpty());
	SEOUL_UNITTESTING_ASSERT(htab.GetSize() == 3);
	SEOUL_UNITTESTING_ASSERT(HashValueTester::s_iHashValueTesterCount == 3);
}

/**
 * Targeted testing of a key-value key with an explicit nullptr key.
 */
void HashTableTest::TestNullKey()
{
	HashTable<Int32, HashValueTester> htab;
	SEOUL_UNITTESTING_ASSERT(htab.IsEmpty());

	// Multiple passes to test integrity after filling the table and removing all entries.
	for (Int iPass = 0; iPass < 8; ++iPass)
	{
		{
			HashValueTester tester;

			SEOUL_UNITTESTING_ASSERT(!htab.Erase(1));
			SEOUL_UNITTESTING_ASSERT(htab.Insert(1, HashValueTester()).Second);
			SEOUL_UNITTESTING_ASSERT(htab.HasValue(1));
			SEOUL_UNITTESTING_ASSERT(htab.GetValue(1, tester));
			SEOUL_UNITTESTING_ASSERT(!htab.GetValue(0, tester));
			SEOUL_UNITTESTING_ASSERT(!htab.HasValue(0));
			SEOUL_UNITTESTING_ASSERT(!htab.GetValue(2, tester));
			SEOUL_UNITTESTING_ASSERT(!htab.HasValue(2));
			SEOUL_UNITTESTING_ASSERT(!htab.GetValue(3, tester));
			SEOUL_UNITTESTING_ASSERT(!htab.HasValue(3));
			SEOUL_UNITTESTING_ASSERT(!htab.Insert(1, HashValueTester()).Second);
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, htab.GetSize());
			SEOUL_UNITTESTING_ASSERT(!htab.Erase(2));
			SEOUL_UNITTESTING_ASSERT(htab.Insert(2, HashValueTester()).Second);
			SEOUL_UNITTESTING_ASSERT(htab.HasValue(2));
			SEOUL_UNITTESTING_ASSERT(htab.GetValue(2, tester));
			SEOUL_UNITTESTING_ASSERT(!htab.GetValue(0, tester));
			SEOUL_UNITTESTING_ASSERT(!htab.HasValue(0));
			SEOUL_UNITTESTING_ASSERT(!htab.GetValue(3, tester));
			SEOUL_UNITTESTING_ASSERT(!htab.HasValue(3));
			SEOUL_UNITTESTING_ASSERT(!htab.Insert(2, HashValueTester()).Second);
			SEOUL_UNITTESTING_ASSERT_EQUAL(2, htab.GetSize());
			SEOUL_UNITTESTING_ASSERT(!htab.Erase(0));
			SEOUL_UNITTESTING_ASSERT(htab.Insert(0, HashValueTester(213)).Second);
			SEOUL_UNITTESTING_ASSERT(htab.HasValue(0));
			SEOUL_UNITTESTING_ASSERT(htab.GetValue(0, tester));
			SEOUL_UNITTESTING_ASSERT_EQUAL(213, tester.m_iDummyValue);
			SEOUL_UNITTESTING_ASSERT(!htab.GetValue(3, tester));
			SEOUL_UNITTESTING_ASSERT(!htab.HasValue(3));
			// nullptr key overwrite testing.
			SEOUL_UNITTESTING_ASSERT(htab.Insert(0, HashValueTester(237), true).Second);
			SEOUL_UNITTESTING_ASSERT_EQUAL(3, htab.GetSize());
			SEOUL_UNITTESTING_ASSERT(htab.HasValue(0));
			SEOUL_UNITTESTING_ASSERT(htab.GetValue(0, tester));
			SEOUL_UNITTESTING_ASSERT_EQUAL(237, tester.m_iDummyValue);
			SEOUL_UNITTESTING_ASSERT(!htab.Insert(0, HashValueTester()).Second);
			SEOUL_UNITTESTING_ASSERT_EQUAL(3, htab.GetSize());
			SEOUL_UNITTESTING_ASSERT(htab.HasValue(0));
			SEOUL_UNITTESTING_ASSERT(htab.GetValue(0, tester));
			SEOUL_UNITTESTING_ASSERT_EQUAL(237, tester.m_iDummyValue);
			SEOUL_UNITTESTING_ASSERT(!htab.Erase(3));
			SEOUL_UNITTESTING_ASSERT(htab.Insert(3, HashValueTester()).Second);
			SEOUL_UNITTESTING_ASSERT(htab.HasValue(3));
			SEOUL_UNITTESTING_ASSERT(htab.GetValue(3, tester));
			SEOUL_UNITTESTING_ASSERT(!htab.Insert(3, HashValueTester()).Second);
			SEOUL_UNITTESTING_ASSERT_EQUAL(4, htab.GetSize());

			// shouldn't be empty
			SEOUL_UNITTESTING_ASSERT(!htab.IsEmpty());
			SEOUL_UNITTESTING_ASSERT_EQUAL(4u, htab.GetSize());
			// +1 for HashValueTester since we have a local member.
			SEOUL_UNITTESTING_ASSERT_EQUAL(5, HashValueTester::s_iHashValueTesterCount);

			// confirm that only 4 entries stored
			SEOUL_UNITTESTING_ASSERT_EQUAL(4, htab.GetSize());

			// verify iteration behaves as expected with a nullptr key present.
			{
				Bool abSeen[4] = { false };
				Int iIterationCount = 0;
				auto const iBegin = htab.Begin();
				auto const iEnd = htab.End();
				for (auto i = iBegin; iEnd != i; ++i)
				{
					SEOUL_UNITTESTING_ASSERT_LESS_THAN(i->First, 4);
					if (i->First == 0)
					{
						SEOUL_UNITTESTING_ASSERT_EQUAL(237, i->Second.m_iDummyValue);
					}
					else
					{
						SEOUL_UNITTESTING_ASSERT_EQUAL(-13, i->Second.m_iDummyValue);
					}
					SEOUL_UNITTESTING_ASSERT(!abSeen[i->First]);
					abSeen[i->First] = true;
					++iIterationCount;
				}
				SEOUL_UNITTESTING_ASSERT_EQUAL(4, iIterationCount);
			}

			// erase and reinsert to verify integrity.
			SEOUL_UNITTESTING_ASSERT(htab.Erase(2));
			SEOUL_UNITTESTING_ASSERT(!htab.GetValue(2, tester));
			SEOUL_UNITTESTING_ASSERT(!htab.HasValue(2));
			SEOUL_UNITTESTING_ASSERT_EQUAL(3u, htab.GetSize());
			SEOUL_UNITTESTING_ASSERT_EQUAL(4u, HashValueTester::s_iHashValueTesterCount);
			SEOUL_UNITTESTING_ASSERT(!htab.IsEmpty());
			SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(htab.Begin(), htab.End());
			SEOUL_UNITTESTING_ASSERT(htab.Erase(0));
			SEOUL_UNITTESTING_ASSERT(!htab.GetValue(0, tester));
			SEOUL_UNITTESTING_ASSERT(!htab.HasValue(0));
			SEOUL_UNITTESTING_ASSERT_EQUAL(2u, htab.GetSize());
			SEOUL_UNITTESTING_ASSERT_EQUAL(3u, HashValueTester::s_iHashValueTesterCount);
			SEOUL_UNITTESTING_ASSERT(!htab.IsEmpty());
			SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(htab.Begin(), htab.End());
			SEOUL_UNITTESTING_ASSERT(htab.Erase(3));
			SEOUL_UNITTESTING_ASSERT(!htab.GetValue(3, tester));
			SEOUL_UNITTESTING_ASSERT(!htab.HasValue(3));
			SEOUL_UNITTESTING_ASSERT_EQUAL(1u, htab.GetSize());
			SEOUL_UNITTESTING_ASSERT_EQUAL(2u, HashValueTester::s_iHashValueTesterCount);
			SEOUL_UNITTESTING_ASSERT(!htab.IsEmpty());
			SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(htab.Begin(), htab.End());
			// Attempt a reinsert of nullptr now.
			SEOUL_UNITTESTING_ASSERT(htab.Insert(0, HashValueTester(819)).Second);
			SEOUL_UNITTESTING_ASSERT(htab.HasValue(0));
			SEOUL_UNITTESTING_ASSERT(htab.GetValue(0, tester));
			SEOUL_UNITTESTING_ASSERT_EQUAL(819u, tester.m_iDummyValue);
			SEOUL_UNITTESTING_ASSERT_EQUAL(2u, htab.GetSize());
			SEOUL_UNITTESTING_ASSERT_EQUAL(3u, HashValueTester::s_iHashValueTesterCount);
			SEOUL_UNITTESTING_ASSERT(!htab.IsEmpty());
			SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(htab.Begin(), htab.End());
			SEOUL_UNITTESTING_ASSERT(htab.Erase(1));
			SEOUL_UNITTESTING_ASSERT(!htab.GetValue(1, tester));
			SEOUL_UNITTESTING_ASSERT(!htab.HasValue(1));
			SEOUL_UNITTESTING_ASSERT_EQUAL(1u, htab.GetSize());
			SEOUL_UNITTESTING_ASSERT_EQUAL(2u, HashValueTester::s_iHashValueTesterCount);
			SEOUL_UNITTESTING_ASSERT(!htab.IsEmpty());
			SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(htab.Begin(), htab.End());
			// Sanity check that we can get nullptr when it's the last element.
			SEOUL_UNITTESTING_ASSERT(htab.HasValue(0));
			SEOUL_UNITTESTING_ASSERT(htab.GetValue(0, tester));
			SEOUL_UNITTESTING_ASSERT(htab.Erase(0));
			SEOUL_UNITTESTING_ASSERT(!htab.GetValue(0, tester));
			SEOUL_UNITTESTING_ASSERT(!htab.HasValue(0));
			SEOUL_UNITTESTING_ASSERT_EQUAL(0u, htab.GetSize());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1u, HashValueTester::s_iHashValueTesterCount);
			SEOUL_UNITTESTING_ASSERT(htab.IsEmpty());
			SEOUL_UNITTESTING_ASSERT_EQUAL(htab.Begin(), htab.End());
		}

		// Final count check.
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, HashValueTester::s_iHashValueTesterCount);

		// Add a big bucket of elements, then clear to stress test.
		for (Int32 iPadding = 0; iPadding < (iPass + 1) * 4; ++iPadding)
		{
			SEOUL_UNITTESTING_ASSERT(htab.Insert(iPadding, HashValueTester()).Second);
			SEOUL_UNITTESTING_ASSERT(!htab.IsEmpty());
		}

		SEOUL_UNITTESTING_ASSERT_EQUAL((UInt32)((iPass + 1) * 4), htab.GetSize());

		{
			HashTable<Int32, HashValueTester> htabTemp(htab);
			htab = htabTemp;
		}

		// Every other pass, either clear or erase manually.
		if (0 == (iPass % 2))
		{
			htab.Clear();
		}
		else
		{
			for (Int32 iPadding = ((iPass + 1) * 4) - 1; iPadding >= 0; --iPadding)
			{
				SEOUL_UNITTESTING_ASSERT(htab.Erase(iPadding));
			}
		}

		SEOUL_UNITTESTING_ASSERT(htab.IsEmpty());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, htab.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, HashValueTester::s_iHashValueTesterCount);
	}

	// Final count check.
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, HashValueTester::s_iHashValueTesterCount);
}

// Mirror of the equivalent test in HashSetTest.
void HashTableTest::TestFindNull()
{
	HashTable<Int32, Int32> ht;
	SEOUL_UNITTESTING_ASSERT(ht.IsEmpty());
	SEOUL_UNITTESTING_ASSERT(ht.Insert(1, 1).Second);
	SEOUL_UNITTESTING_ASSERT_EQUAL(nullptr, ht.Find(HashSet<Int32>::Traits::GetNullKey()));
	SEOUL_UNITTESTING_ASSERT(ht.Insert(HashSet<Int32>::Traits::GetNullKey(), 1).Second);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, *ht.Find(HashSet<Int32>::Traits::GetNullKey()));
}

/**
 * Confirm that Seoul::Strings function properly as keys.
 */
void HashTableTest::TestSeoulStringKeys()
{
	ScopedPtr< HashTable<Seoul::String, HashValueTester> > ht(SEOUL_NEW(MemoryBudgets::TBD) HashTable<Seoul::String, HashValueTester>());
	SEOUL_UNITTESTING_ASSERT(ht->IsEmpty());

	Seoul::String sIn("one");
	SEOUL_UNITTESTING_ASSERT(ht->Insert(sIn, HashValueTester()).Second);
	SEOUL_UNITTESTING_ASSERT(!ht->Insert(sIn, HashValueTester()).Second);

	sIn = "two";
	SEOUL_UNITTESTING_ASSERT(ht->Insert(sIn, HashValueTester()).Second);
	SEOUL_UNITTESTING_ASSERT(!ht->Insert(sIn, HashValueTester()).Second);

	sIn = "three";
	SEOUL_UNITTESTING_ASSERT(ht->Insert(sIn, HashValueTester()).Second);
	SEOUL_UNITTESTING_ASSERT(!ht->Insert(sIn, HashValueTester()).Second);

	// shouldn't be empty
	SEOUL_UNITTESTING_ASSERT(!ht->IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(3u, ht->GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, HashValueTester::s_iHashValueTesterCount);

	// confirm that we can extract expected string
	HashValueTester out;
	sIn = "two";

	SEOUL_UNITTESTING_ASSERT(ht->GetValue(sIn,out));
	SEOUL_UNITTESTING_ASSERT_EQUAL(-13, out.m_iDummyValue);
}

class FancyKey
{
public:
	FancyKey()
	{
		foo = 1;
		bar = "deseo";
	}
	Int32 foo;
	Seoul::String bar;
};

/**
 * Confirm that pointers function properly as keys.
 */
void HashTableTest::TestPointerKeys()
{
	ScopedPtr< HashTable<FancyKey*, HashValueTester> > ht(SEOUL_NEW(MemoryBudgets::TBD) HashTable<FancyKey*, HashValueTester>());

	// use a pointer to something as a key
	ScopedPtr<FancyKey> test(SEOUL_NEW(MemoryBudgets::TBD) FancyKey);
	SEOUL_UNITTESTING_ASSERT(ht->Insert(test.Get(), HashValueTester()).Second);

	// shouldn't be empty
	SEOUL_UNITTESTING_ASSERT(!ht->IsEmpty());

	// confirm that we can extract expected string
	HashValueTester out;
	SEOUL_UNITTESTING_ASSERT(ht->GetValue(test.Get(), out));
	SEOUL_UNITTESTING_ASSERT_EQUAL(-13, out.m_iDummyValue);
}

/**
 * Confirm that entry erasure works
 */
void HashTableTest::TestErase()
{
	// make a hashtable of ints->strings
	ScopedPtr< HashTable<String, HashValueTester> > ht(SEOUL_NEW(MemoryBudgets::TBD) HashTable<String, HashValueTester>());

	// populate the table
	SEOUL_UNITTESTING_ASSERT(ht->Insert("one", HashValueTester()).Second);
	SEOUL_UNITTESTING_ASSERT(ht->Insert("two", HashValueTester()).Second);
	SEOUL_UNITTESTING_ASSERT(ht->Insert("three", HashValueTester()).Second);
	SEOUL_UNITTESTING_ASSERT(ht->Insert("tremendous", HashValueTester()).Second);
	SEOUL_UNITTESTING_ASSERT(ht->Insert("terrific", HashValueTester()).Second);
	SEOUL_UNITTESTING_ASSERT(ht->Insert("toofreakinawesome", HashValueTester()).Second);
	SEOUL_UNITTESTING_ASSERT(ht->Insert("four", HashValueTester()).Second);
	SEOUL_UNITTESTING_ASSERT(ht->Insert("five", HashValueTester()).Second);
	SEOUL_UNITTESTING_ASSERT(ht->Insert("six", HashValueTester()).Second);
	SEOUL_UNITTESTING_ASSERT(ht->Insert("seven", HashValueTester()).Second);

	// make sure it counted
	SEOUL_UNITTESTING_ASSERT_EQUAL(10u, ht->GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(10, HashValueTester::s_iHashValueTesterCount);

	// remove an entry
	SEOUL_UNITTESTING_ASSERT(ht->Erase("one"));

	// make sure removal worked
	SEOUL_UNITTESTING_ASSERT_EQUAL(9u, ht->GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(9, HashValueTester::s_iHashValueTesterCount);
	SEOUL_UNITTESTING_ASSERT(ht->HasValue("two"));
	SEOUL_UNITTESTING_ASSERT(ht->HasValue("three"));
	SEOUL_UNITTESTING_ASSERT(ht->HasValue("tremendous"));
	SEOUL_UNITTESTING_ASSERT(ht->HasValue("terrific"));
	SEOUL_UNITTESTING_ASSERT(ht->HasValue("toofreakinawesome"));
	SEOUL_UNITTESTING_ASSERT(ht->HasValue("four"));
	SEOUL_UNITTESTING_ASSERT(ht->HasValue("five"));
	SEOUL_UNITTESTING_ASSERT(ht->HasValue("six"));
	SEOUL_UNITTESTING_ASSERT(ht->HasValue("seven"));

	SEOUL_UNITTESTING_ASSERT(ht->Erase("two"));
	SEOUL_UNITTESTING_ASSERT(ht->Erase("seven"));
	SEOUL_UNITTESTING_ASSERT_EQUAL(7u, ht->GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(7, HashValueTester::s_iHashValueTesterCount);
	SEOUL_UNITTESTING_ASSERT(ht->HasValue("three"));
	SEOUL_UNITTESTING_ASSERT(ht->HasValue("tremendous"));
	SEOUL_UNITTESTING_ASSERT(ht->HasValue("terrific"));
	SEOUL_UNITTESTING_ASSERT(ht->HasValue("toofreakinawesome"));
	SEOUL_UNITTESTING_ASSERT(ht->HasValue("four"));
	SEOUL_UNITTESTING_ASSERT(ht->HasValue("five"));
	SEOUL_UNITTESTING_ASSERT(ht->HasValue("six"));

	SEOUL_UNITTESTING_ASSERT(ht->Erase("three"));
	SEOUL_UNITTESTING_ASSERT(ht->Insert("one", HashValueTester()).Second);
	SEOUL_UNITTESTING_ASSERT(ht->Erase("six"));
	SEOUL_UNITTESTING_ASSERT_EQUAL(6u, ht->GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(6, HashValueTester::s_iHashValueTesterCount);
	SEOUL_UNITTESTING_ASSERT(ht->HasValue("one"));
	SEOUL_UNITTESTING_ASSERT(ht->HasValue("tremendous"));
	SEOUL_UNITTESTING_ASSERT(ht->HasValue("terrific"));
	SEOUL_UNITTESTING_ASSERT(ht->HasValue("toofreakinawesome"));
	SEOUL_UNITTESTING_ASSERT(ht->HasValue("four"));
	SEOUL_UNITTESTING_ASSERT(ht->HasValue("five"));

	SEOUL_UNITTESTING_ASSERT(ht->Erase("five"));
	SEOUL_UNITTESTING_ASSERT(ht->Insert("two", HashValueTester()).Second);
	SEOUL_UNITTESTING_ASSERT(ht->Insert("three", HashValueTester()).Second);
	SEOUL_UNITTESTING_ASSERT(ht->Erase("tremendous"));
	SEOUL_UNITTESTING_ASSERT_EQUAL(6u, ht->GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(6, HashValueTester::s_iHashValueTesterCount);
	SEOUL_UNITTESTING_ASSERT(ht->HasValue("one"));
	SEOUL_UNITTESTING_ASSERT(ht->HasValue("two"));
	SEOUL_UNITTESTING_ASSERT(ht->HasValue("three"));
	SEOUL_UNITTESTING_ASSERT(ht->HasValue("terrific"));
	SEOUL_UNITTESTING_ASSERT(ht->HasValue("toofreakinawesome"));
	SEOUL_UNITTESTING_ASSERT(ht->HasValue("four"));

	SEOUL_UNITTESTING_ASSERT(ht->Erase("one"));
	SEOUL_UNITTESTING_ASSERT(ht->Erase("two"));
	SEOUL_UNITTESTING_ASSERT(ht->Erase("three"));
	SEOUL_UNITTESTING_ASSERT_EQUAL(3u, ht->GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, HashValueTester::s_iHashValueTesterCount);
	SEOUL_UNITTESTING_ASSERT(ht->HasValue("terrific"));
	SEOUL_UNITTESTING_ASSERT(ht->HasValue("toofreakinawesome"));
	SEOUL_UNITTESTING_ASSERT(ht->HasValue("four"));

	SEOUL_UNITTESTING_ASSERT(ht->Erase("terrific"));
	SEOUL_UNITTESTING_ASSERT(ht->Erase("toofreakinawesome"));
	SEOUL_UNITTESTING_ASSERT(ht->Erase("four"));

	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, ht->GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, HashValueTester::s_iHashValueTesterCount);
	SEOUL_UNITTESTING_ASSERT(ht->IsEmpty());
}

void HashTableTest::TestIterators()
{
	// make a hashtable of ints->strings
	typedef HashTable<String, HashValueTester> TableType;
	ScopedPtr< TableType > ht(SEOUL_NEW(MemoryBudgets::TBD) TableType());

	// populate the table
	SEOUL_UNITTESTING_ASSERT(ht->Insert("one", HashValueTester()).Second);
	SEOUL_UNITTESTING_ASSERT(ht->Insert("two", HashValueTester()).Second);
	SEOUL_UNITTESTING_ASSERT(ht->Insert("three", HashValueTester()).Second);
	SEOUL_UNITTESTING_ASSERT(ht->Insert("tremendous", HashValueTester()).Second);
	SEOUL_UNITTESTING_ASSERT(ht->Insert("terrific", HashValueTester()).Second);
	SEOUL_UNITTESTING_ASSERT(ht->Insert("toofreakinawesome", HashValueTester()).Second);
	SEOUL_UNITTESTING_ASSERT(ht->Insert("four", HashValueTester()).Second);
	SEOUL_UNITTESTING_ASSERT(ht->Insert("five", HashValueTester()).Second);
	SEOUL_UNITTESTING_ASSERT(ht->Insert("six", HashValueTester()).Second);
	SEOUL_UNITTESTING_ASSERT(ht->Insert("seven", HashValueTester()).Second);

	// make sure it counted
	SEOUL_UNITTESTING_ASSERT_EQUAL(10u, ht->GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(10, HashValueTester::s_iHashValueTesterCount);

	// iterator over it - constant
	{
		UInt32 nCount = 0;
		HashTable<String, Bool> tester;
		for (TableType::ConstIterator i = const_cast<TableType const*>(ht.Get())->Begin(); const_cast<TableType const*>(ht.Get())->End() != i; ++i)
		{
			++nCount;
			SEOUL_UNITTESTING_ASSERT(tester.Insert(i->First, true).Second);
			SEOUL_UNITTESTING_ASSERT(i->Second.m_iDummyValue == -13);
		}

		SEOUL_UNITTESTING_ASSERT_EQUAL(10u, nCount);

		SEOUL_UNITTESTING_ASSERT(tester.HasValue("one"));
		SEOUL_UNITTESTING_ASSERT(tester.HasValue("two"));
		SEOUL_UNITTESTING_ASSERT(tester.HasValue("three"));
		SEOUL_UNITTESTING_ASSERT(tester.HasValue("tremendous"));
		SEOUL_UNITTESTING_ASSERT(tester.HasValue("terrific"));
		SEOUL_UNITTESTING_ASSERT(tester.HasValue("toofreakinawesome"));
		SEOUL_UNITTESTING_ASSERT(tester.HasValue("four"));
		SEOUL_UNITTESTING_ASSERT(tester.HasValue("five"));
		SEOUL_UNITTESTING_ASSERT(tester.HasValue("six"));
		SEOUL_UNITTESTING_ASSERT(tester.HasValue("seven"));
	}

	// iterator over it - constant (post increment)
	{
		UInt32 nCount = 0;
		HashTable<String, Bool> tester;
		for (TableType::ConstIterator i = const_cast<TableType const*>(ht.Get())->Begin(); const_cast<TableType const*>(ht.Get())->End() != i; i++)
		{
			++nCount;
			SEOUL_UNITTESTING_ASSERT(tester.Insert(i->First, true).Second);
			SEOUL_UNITTESTING_ASSERT_EQUAL(-13, i->Second.m_iDummyValue);
		}

		SEOUL_UNITTESTING_ASSERT_EQUAL(10u, nCount);

		SEOUL_UNITTESTING_ASSERT(tester.HasValue("one"));
		SEOUL_UNITTESTING_ASSERT(tester.HasValue("two"));
		SEOUL_UNITTESTING_ASSERT(tester.HasValue("three"));
		SEOUL_UNITTESTING_ASSERT(tester.HasValue("tremendous"));
		SEOUL_UNITTESTING_ASSERT(tester.HasValue("terrific"));
		SEOUL_UNITTESTING_ASSERT(tester.HasValue("toofreakinawesome"));
		SEOUL_UNITTESTING_ASSERT(tester.HasValue("four"));
		SEOUL_UNITTESTING_ASSERT(tester.HasValue("five"));
		SEOUL_UNITTESTING_ASSERT(tester.HasValue("six"));
		SEOUL_UNITTESTING_ASSERT(tester.HasValue("seven"));
	}

	// iterator over it - read-write
	{
		UInt32 nCount = 0;
		HashTable<String, Bool> tester;
		for (auto i = ht->Begin(); ht->End() != i; ++i)
		{
			++nCount;
			SEOUL_UNITTESTING_ASSERT(tester.Insert(i->First, true).Second);
			SEOUL_UNITTESTING_ASSERT(i->Second.m_iDummyValue == -13);
			SEOUL_UNITTESTING_ASSERT(i->Second.m_iDummyValue = 3);
		}

		SEOUL_UNITTESTING_ASSERT_EQUAL(10u, nCount);

		SEOUL_UNITTESTING_ASSERT(tester.HasValue("one"));
		SEOUL_UNITTESTING_ASSERT(tester.HasValue("two"));
		SEOUL_UNITTESTING_ASSERT(tester.HasValue("three"));
		SEOUL_UNITTESTING_ASSERT(tester.HasValue("tremendous"));
		SEOUL_UNITTESTING_ASSERT(tester.HasValue("terrific"));
		SEOUL_UNITTESTING_ASSERT(tester.HasValue("toofreakinawesome"));
		SEOUL_UNITTESTING_ASSERT(tester.HasValue("four"));
		SEOUL_UNITTESTING_ASSERT(tester.HasValue("five"));
		SEOUL_UNITTESTING_ASSERT(tester.HasValue("six"));
		SEOUL_UNITTESTING_ASSERT(tester.HasValue("seven"));
	}

	// iterator over it - read-write (post increment)
	{
		UInt32 nCount = 0;
		HashTable<String, Bool> tester;
		for (auto i = ht->Begin(); ht->End() != i; i++)
		{
			++nCount;
			SEOUL_UNITTESTING_ASSERT(tester.Insert(i->First, true).Second);
			SEOUL_UNITTESTING_ASSERT(i->Second.m_iDummyValue == 3);
			SEOUL_UNITTESTING_ASSERT(i->Second.m_iDummyValue = -13);
		}

		SEOUL_UNITTESTING_ASSERT_EQUAL(10u, nCount);

		SEOUL_UNITTESTING_ASSERT(tester.HasValue("one"));
		SEOUL_UNITTESTING_ASSERT(tester.HasValue("two"));
		SEOUL_UNITTESTING_ASSERT(tester.HasValue("three"));
		SEOUL_UNITTESTING_ASSERT(tester.HasValue("tremendous"));
		SEOUL_UNITTESTING_ASSERT(tester.HasValue("terrific"));
		SEOUL_UNITTESTING_ASSERT(tester.HasValue("toofreakinawesome"));
		SEOUL_UNITTESTING_ASSERT(tester.HasValue("four"));
		SEOUL_UNITTESTING_ASSERT(tester.HasValue("five"));
		SEOUL_UNITTESTING_ASSERT(tester.HasValue("six"));
		SEOUL_UNITTESTING_ASSERT(tester.HasValue("seven"));
	}

	// iterator - verify reference return value.
	{
		auto const iBegin = ht->Begin();
		auto const iEnd = ht->End();
		for (auto i = iBegin; iEnd != i; ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(ht->Find(i->First), &(i->Second));
		}
	}
	// const iterator - verify reference return value.
	{
		auto const iBegin = const_cast<TableType const*>(ht.Get())->Begin();
		auto const iEnd = const_cast<TableType const*>(ht.Get())->End();
		for (auto i = iBegin; iEnd != i; ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(ht->Find(i->First), &(i->Second));
		}
	}

	// iterator - pre increment
	{
		TableType t;
		SEOUL_UNITTESTING_ASSERT(t.Insert("one", HashValueTester(1)).Second);
		SEOUL_UNITTESTING_ASSERT(t.Insert("two", HashValueTester(2)).Second);

		auto i = t.Begin();
		SEOUL_UNITTESTING_ASSERT_EQUAL("one", i->First);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, i->Second.m_iDummyValue);

		auto j = ++i;

		SEOUL_UNITTESTING_ASSERT_EQUAL("two", i->First);
		SEOUL_UNITTESTING_ASSERT_EQUAL(2, i->Second.m_iDummyValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL("two", j->First);
		SEOUL_UNITTESTING_ASSERT_EQUAL(2, j->Second.m_iDummyValue);

		SEOUL_UNITTESTING_ASSERT_EQUAL(t.End(), ++i);
	}

	// iterator - post increment
	{
		TableType t;
		SEOUL_UNITTESTING_ASSERT(t.Insert("one", HashValueTester(1)).Second);
		SEOUL_UNITTESTING_ASSERT(t.Insert("two", HashValueTester(2)).Second);

		auto i = t.Begin();
		SEOUL_UNITTESTING_ASSERT_EQUAL("one", i->First);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, i->Second.m_iDummyValue);

		auto j = i++;

		SEOUL_UNITTESTING_ASSERT_EQUAL("two", i->First);
		SEOUL_UNITTESTING_ASSERT_EQUAL(2, i->Second.m_iDummyValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL("one", j->First);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, j->Second.m_iDummyValue);

		SEOUL_UNITTESTING_ASSERT_EQUAL("two", (i++)->First);
		SEOUL_UNITTESTING_ASSERT_EQUAL(t.End(), i);
	}
}

void HashTableTest::TestRangedFor()
{
	HashTable<Int, Int> testTable;
	SEOUL_UNITTESTING_ASSERT(testTable.Insert(3, 1).Second);
	SEOUL_UNITTESTING_ASSERT(testTable.Insert(7, 2).Second);
	SEOUL_UNITTESTING_ASSERT(testTable.Insert(2, 3).Second);

	{
		auto i = testTable.Begin();
		for (auto v : testTable)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(i->First, v.First);
			SEOUL_UNITTESTING_ASSERT_EQUAL(i->Second, v.Second);
			++i;
		}
	}

	SEOUL_UNITTESTING_ASSERT(testTable.Insert(35, 59).Second);

	{
		auto i = testTable.Begin();
		for (auto v : testTable)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(i->First, v.First);
			SEOUL_UNITTESTING_ASSERT_EQUAL(i->Second, v.Second);
			++i;
		}
	}

	SEOUL_UNITTESTING_ASSERT(testTable.Insert(77, 101).Second);

	{
		auto i = testTable.Begin();
		for (auto v : testTable)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(i->First, v.First);
			SEOUL_UNITTESTING_ASSERT_EQUAL(i->Second, v.Second);
			++i;
		}
	}
}

void HashTableTest::TestUtilities()
{
	// GetHashTableKeys()
	{
		HashTable<Int, Bool> testTable;

		Vector<Int> v;
		GetHashTableKeys(testTable, v);
		SEOUL_UNITTESTING_ASSERT(v.IsEmpty());

		SEOUL_UNITTESTING_ASSERT(testTable.Insert(3, true).Second);
		SEOUL_UNITTESTING_ASSERT(testTable.Insert(7, false).Second);
		SEOUL_UNITTESTING_ASSERT(testTable.Insert(2, true).Second);

		GetHashTableKeys(testTable, v);

		SEOUL_UNITTESTING_ASSERT_EQUAL(7, v[0]);
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v[1]);
		SEOUL_UNITTESTING_ASSERT_EQUAL(2, v[2]);

		// Make sure op clears the output vector properly.
		testTable.Clear();
		GetHashTableKeys(testTable, v);
		SEOUL_UNITTESTING_ASSERT(v.IsEmpty());
	}

	// GetHashTableEntries()
	{
		HashTable<Int, Bool> testTable;

		Vector< Pair<Int, Bool> > v;
		GetHashTableEntries(testTable, v);
		SEOUL_UNITTESTING_ASSERT(v.IsEmpty());

		SEOUL_UNITTESTING_ASSERT(testTable.Insert(3, true).Second);
		SEOUL_UNITTESTING_ASSERT(testTable.Insert(7, false).Second);
		SEOUL_UNITTESTING_ASSERT(testTable.Insert(2, true).Second);

		GetHashTableEntries(testTable, v);

		SEOUL_UNITTESTING_ASSERT_EQUAL(7, v[0].First);
		SEOUL_UNITTESTING_ASSERT_EQUAL(false, v[0].Second);
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v[1].First);
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, v[1].Second);
		SEOUL_UNITTESTING_ASSERT_EQUAL(2, v[2].First);
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, v[2].Second);

		// Make sure op clears the output vector properly.
		testTable.Clear();
		GetHashTableEntries(testTable, v);
		SEOUL_UNITTESTING_ASSERT(v.IsEmpty());
	}
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
