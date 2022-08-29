/**
 * \file HashSetTest.cpp
 * \brief Unit test header file for HashSet class
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "HashSetTest.h"
#include "HashFunctions.h"
#include "Logger.h"
#include "ReflectionDefine.h"
#include "SeoulAssert.h"
#include "SeoulString.h"
#include "UnitTesting.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

template <typename KEY, typename TRAITS>
static inline String UnitTestingToString(const HashSetIterator<KEY, TRAITS>& i)
{
	return String::Printf("%p", std::addressof(*i));
}

SEOUL_BEGIN_TYPE( HashSetTest )
	SEOUL_ATTRIBUTE( UnitTest )
	SEOUL_METHOD( TestInstantiation )
	SEOUL_METHOD( TestInstantiationFromVector )
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
	SEOUL_METHOD( TestEquality )
	SEOUL_METHOD( TestContains )
	SEOUL_METHOD( TestDisjoint )
SEOUL_END_TYPE();

/**
 * Make sure that we can create an empty hash set and that it has the
 * expected number of empty rows.
 */
void HashSetTest::TestInstantiation()
{
	String sTest;
	HashSet<Int32> hset;

	SEOUL_UNITTESTING_ASSERT(hset.IsEmpty());
	SEOUL_UNITTESTING_ASSERT(!hset.Erase(1));
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, hset.GetSize());
	SEOUL_UNITTESTING_ASSERT(!hset.HasKey(1));
	SEOUL_UNITTESTING_ASSERT_EQUAL(nullptr, hset.Find(1));
}

/*
 * Make sure we can construct a hash set from an iterable type
 */
void HashSetTest::TestInstantiationFromVector()
{
	Vector<Int> testVec;
	testVec.PushBack(3);
	testVec.PushBack(6);
	testVec.PushBack(12);

	HashSet<Int> hset(testVec.Begin(), testVec.End(), testVec.GetSize());

	SEOUL_UNITTESTING_ASSERT_EQUAL(3u, hset.GetSize());
	SEOUL_UNITTESTING_ASSERT(hset.HasKey(3));
	SEOUL_UNITTESTING_ASSERT(hset.HasKey(6));
	SEOUL_UNITTESTING_ASSERT(hset.HasKey(12));

	// Add a duplicate element and make sure the set
	// created is the correct size.  It should ignore the
	// duplicate and not complain.
	testVec.PushBack(3);

	HashSet<Int> hsetB(testVec.Begin(), testVec.End(), testVec.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(3u, hset.GetSize());
	SEOUL_UNITTESTING_ASSERT(hsetB.HasKey(3));
	SEOUL_UNITTESTING_ASSERT(hsetB.HasKey(6));
	SEOUL_UNITTESTING_ASSERT(hsetB.HasKey(12));
}

void VerifyAndInsertElements(HashSet<Seoul::String>& hset)
{
	{
		auto result = hset.Insert("one");
		SEOUL_UNITTESTING_ASSERT(result.Second);
		SEOUL_UNITTESTING_ASSERT(result.First != hset.End());
	}
	SEOUL_UNITTESTING_ASSERT(!hset.Insert("one").Second);

	{
		auto result = hset.Insert("two");
		SEOUL_UNITTESTING_ASSERT(result.Second);
		SEOUL_UNITTESTING_ASSERT(result.First != hset.End());
	}
	SEOUL_UNITTESTING_ASSERT(!hset.Insert("two").Second);

	{
		auto result = hset.Insert("three");
		SEOUL_UNITTESTING_ASSERT(result.Second);
		SEOUL_UNITTESTING_ASSERT(result.First != hset.End());
	}
	SEOUL_UNITTESTING_ASSERT(!hset.Insert("three").Second);

	{
		auto result = hset.Insert("tremendous");
		SEOUL_UNITTESTING_ASSERT(result.Second);
		SEOUL_UNITTESTING_ASSERT(result.First != hset.End());
	}
	SEOUL_UNITTESTING_ASSERT(!hset.Insert("tremendous").Second);

	{
		auto result = hset.Insert("terrific");
		SEOUL_UNITTESTING_ASSERT(result.Second);
		SEOUL_UNITTESTING_ASSERT(result.First != hset.End());
	}
	SEOUL_UNITTESTING_ASSERT(!hset.Insert("terrific").Second);

	{
		auto result = hset.Insert("toofreakinawesome");
		SEOUL_UNITTESTING_ASSERT(result.Second);
		SEOUL_UNITTESTING_ASSERT(result.First != hset.End());
	}
	SEOUL_UNITTESTING_ASSERT(!hset.Insert("toofreakinawesome").Second);

	{
		auto result = hset.Insert("four");
		SEOUL_UNITTESTING_ASSERT(result.Second);
		SEOUL_UNITTESTING_ASSERT(result.First != hset.End());
	}
	SEOUL_UNITTESTING_ASSERT(!hset.Insert("four").Second);

	{
		auto result = hset.Insert("five");
		SEOUL_UNITTESTING_ASSERT(result.Second);
		SEOUL_UNITTESTING_ASSERT(result.First != hset.End());
	}
	SEOUL_UNITTESTING_ASSERT(!hset.Insert("five").Second);

	{
		auto result = hset.Insert("six");
		SEOUL_UNITTESTING_ASSERT(result.Second);
		SEOUL_UNITTESTING_ASSERT(result.First != hset.End());
	}
	SEOUL_UNITTESTING_ASSERT(!hset.Insert("six").Second);

	{
		auto result = hset.Insert("seven");
		SEOUL_UNITTESTING_ASSERT(result.Second);
		SEOUL_UNITTESTING_ASSERT(result.First != hset.End());
	}
	SEOUL_UNITTESTING_ASSERT(!hset.Insert("seven").Second);

	// shouldn't be empty
	SEOUL_UNITTESTING_ASSERT(!hset.IsEmpty());

	// confirm that 10 entries stored
	SEOUL_UNITTESTING_ASSERT_EQUAL(10u, hset.GetSize());
}

/**
 * Confirm that clearing the table actually removes all the entries.
 */
void HashSetTest::TestClear()
{
	HashSet<Seoul::String> hset;
	SEOUL_UNITTESTING_ASSERT(hset.IsEmpty());

	VerifyAndInsertElements(hset);

	hset.Clear();

	// should be empty!
	SEOUL_UNITTESTING_ASSERT(hset.IsEmpty());
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

void HashSetTest::TestClusteringPrevention()
{
	HashSet<ClusterTestType> hset;
	SEOUL_UNITTESTING_ASSERT(hset.IsEmpty());

	SEOUL_UNITTESTING_ASSERT(hset.Insert(10).Second);
	SEOUL_UNITTESTING_ASSERT(hset.Insert(9).Second);
	SEOUL_UNITTESTING_ASSERT(hset.Insert(8).Second);
	SEOUL_UNITTESTING_ASSERT(hset.Insert(7).Second);
	SEOUL_UNITTESTING_ASSERT(hset.Insert(6).Second);
	SEOUL_UNITTESTING_ASSERT(hset.Insert(5).Second);
	SEOUL_UNITTESTING_ASSERT(hset.Insert(4).Second);
	SEOUL_UNITTESTING_ASSERT(hset.Insert(3).Second);
	SEOUL_UNITTESTING_ASSERT(hset.Insert(2).Second);
	SEOUL_UNITTESTING_ASSERT(hset.Insert(1).Second);
	SEOUL_UNITTESTING_ASSERT(hset.Insert(0).Second);

	SEOUL_UNITTESTING_ASSERT_EQUAL(16u, hset.GetCapacity());

	// Scope the HashSet<>::Iterator instance.
	{
		auto i = hset.Begin();
		SEOUL_UNITTESTING_ASSERT(*i == 10); ++i;
		SEOUL_UNITTESTING_ASSERT(*i == 1); ++i;
		SEOUL_UNITTESTING_ASSERT(*i == 2); ++i;
		SEOUL_UNITTESTING_ASSERT(*i == 3); ++i;
		SEOUL_UNITTESTING_ASSERT(*i == 4); ++i;
		SEOUL_UNITTESTING_ASSERT(*i == 5); ++i;
		SEOUL_UNITTESTING_ASSERT(*i == 6); ++i;
		SEOUL_UNITTESTING_ASSERT(*i == 7); ++i;
		SEOUL_UNITTESTING_ASSERT(*i == 8); ++i;
		SEOUL_UNITTESTING_ASSERT(*i == 9); ++i;
		SEOUL_UNITTESTING_ASSERT(*i == 0); ++i;
		SEOUL_UNITTESTING_ASSERT(hset.End() == i);
	}

	SEOUL_UNITTESTING_ASSERT(hset.Erase(10));

	SEOUL_UNITTESTING_ASSERT_EQUAL(16u, hset.GetCapacity());
	// Scope the HashSet<>::Iterator instance.
	{
		auto i = hset.Begin();
		SEOUL_UNITTESTING_ASSERT(*i == 0); ++i;
		SEOUL_UNITTESTING_ASSERT(*i == 1); ++i;
		SEOUL_UNITTESTING_ASSERT(*i == 2); ++i;
		SEOUL_UNITTESTING_ASSERT(*i == 3); ++i;
		SEOUL_UNITTESTING_ASSERT(*i == 4); ++i;
		SEOUL_UNITTESTING_ASSERT(*i == 5); ++i;
		SEOUL_UNITTESTING_ASSERT(*i == 6); ++i;
		SEOUL_UNITTESTING_ASSERT(*i == 7); ++i;
		SEOUL_UNITTESTING_ASSERT(*i == 8); ++i;
		SEOUL_UNITTESTING_ASSERT(*i == 9); ++i;
		SEOUL_UNITTESTING_ASSERT(hset.End() == i);
	}

	SEOUL_UNITTESTING_ASSERT(!hset.Insert(0).Second);
	SEOUL_UNITTESTING_ASSERT(hset.HasKey(0));
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, hset.Find(0)->m_I);
}

void HashSetTest::TestAssignment()
{
	using namespace Reflection;

	HashSet<HString> tester;
	SEOUL_UNITTESTING_ASSERT(tester.IsEmpty());

	SEOUL_UNITTESTING_ASSERT(tester.Insert(HString("a")).Second);
	SEOUL_UNITTESTING_ASSERT(tester.Insert(HString("b")).Second);
	SEOUL_UNITTESTING_ASSERT(tester.Insert(HString("c")).Second);
	SEOUL_UNITTESTING_ASSERT(tester.Insert(HString("d")).Second);
	SEOUL_UNITTESTING_ASSERT(tester.Insert(HString("e")).Second);
	SEOUL_UNITTESTING_ASSERT(tester.Insert(HString("f")).Second);

	// Copy constructor
	{
		HashSet<HString> tester2(tester);
		SEOUL_UNITTESTING_ASSERT(tester2.GetSize() == 6);
		SEOUL_UNITTESTING_ASSERT(tester2.GetSize() == tester.GetSize());
		SEOUL_UNITTESTING_ASSERT(tester2.GetCapacity() == tester.GetCapacity());

		SEOUL_UNITTESTING_ASSERT(tester2.HasKey(HString("a")));
		SEOUL_UNITTESTING_ASSERT(tester2.HasKey(HString("b")));
		SEOUL_UNITTESTING_ASSERT(tester2.HasKey(HString("c")));
		SEOUL_UNITTESTING_ASSERT(tester2.HasKey(HString("d")));
		SEOUL_UNITTESTING_ASSERT(tester2.HasKey(HString("e")));
		SEOUL_UNITTESTING_ASSERT(tester2.HasKey(HString("f")));

		SEOUL_UNITTESTING_ASSERT_EQUAL(HString("a"), *tester2.Find(HString("a")));
		SEOUL_UNITTESTING_ASSERT_EQUAL(HString("b"), *tester2.Find(HString("b")));
		SEOUL_UNITTESTING_ASSERT_EQUAL(HString("c"), *tester2.Find(HString("c")));
		SEOUL_UNITTESTING_ASSERT_EQUAL(HString("d"), *tester2.Find(HString("d")));
		SEOUL_UNITTESTING_ASSERT_EQUAL(HString("e"), *tester2.Find(HString("e")));
		SEOUL_UNITTESTING_ASSERT_EQUAL(HString("f"), *tester2.Find(HString("f")));
	}

	// Assignment operator with Any
	{
		HashSet<HString> tester2 = tester;
		SEOUL_UNITTESTING_ASSERT(tester2.GetSize() == 6);
		SEOUL_UNITTESTING_ASSERT(tester2.GetSize() == tester.GetSize());
		SEOUL_UNITTESTING_ASSERT(tester2.GetCapacity() == tester.GetCapacity());

		SEOUL_UNITTESTING_ASSERT(tester2.HasKey(HString("a")));
		SEOUL_UNITTESTING_ASSERT(tester2.HasKey(HString("b")));
		SEOUL_UNITTESTING_ASSERT(tester2.HasKey(HString("c")));
		SEOUL_UNITTESTING_ASSERT(tester2.HasKey(HString("d")));
		SEOUL_UNITTESTING_ASSERT(tester2.HasKey(HString("e")));
		SEOUL_UNITTESTING_ASSERT(tester2.HasKey(HString("f")));

		SEOUL_UNITTESTING_ASSERT_EQUAL(HString("a"), *tester2.Find(HString("a")));
		SEOUL_UNITTESTING_ASSERT_EQUAL(HString("b"), *tester2.Find(HString("b")));
		SEOUL_UNITTESTING_ASSERT_EQUAL(HString("c"), *tester2.Find(HString("c")));
		SEOUL_UNITTESTING_ASSERT_EQUAL(HString("d"), *tester2.Find(HString("d")));
		SEOUL_UNITTESTING_ASSERT_EQUAL(HString("e"), *tester2.Find(HString("e")));
		SEOUL_UNITTESTING_ASSERT_EQUAL(HString("f"), *tester2.Find(HString("f")));
	}

	// Self-assignment
	{
		tester = tester;
		SEOUL_UNITTESTING_ASSERT(tester.HasKey(HString("a")));
		SEOUL_UNITTESTING_ASSERT(tester.HasKey(HString("b")));
		SEOUL_UNITTESTING_ASSERT(tester.HasKey(HString("c")));
		SEOUL_UNITTESTING_ASSERT(tester.HasKey(HString("d")));
		SEOUL_UNITTESTING_ASSERT(tester.HasKey(HString("e")));
		SEOUL_UNITTESTING_ASSERT(tester.HasKey(HString("f")));

		SEOUL_UNITTESTING_ASSERT_EQUAL(HString("a"), *tester.Find(HString("a")));
		SEOUL_UNITTESTING_ASSERT_EQUAL(HString("b"), *tester.Find(HString("b")));
		SEOUL_UNITTESTING_ASSERT_EQUAL(HString("c"), *tester.Find(HString("c")));
		SEOUL_UNITTESTING_ASSERT_EQUAL(HString("d"), *tester.Find(HString("d")));
		SEOUL_UNITTESTING_ASSERT_EQUAL(HString("e"), *tester.Find(HString("e")));
		SEOUL_UNITTESTING_ASSERT_EQUAL(HString("f"), *tester.Find(HString("f")));
	}
}

void HashSetTest::TestInsert()
{
	HashSet<Seoul::String> hset;
	SEOUL_UNITTESTING_ASSERT(hset.IsEmpty());

	{
		auto result = hset.Insert("value");
		SEOUL_UNITTESTING_ASSERT(*result.First == "value");
		SEOUL_UNITTESTING_ASSERT(result.Second);
	}

	SEOUL_UNITTESTING_ASSERT(!hset.Insert("value").Second);
	SEOUL_UNITTESTING_ASSERT(hset.HasKey("value"));
	SEOUL_UNITTESTING_ASSERT_EQUAL("value", *hset.Find("value"));

	SEOUL_UNITTESTING_ASSERT(!hset.Insert("value").Second);
	SEOUL_UNITTESTING_ASSERT(hset.HasKey("value"));
	SEOUL_UNITTESTING_ASSERT_EQUAL("value", *hset.Find("value"));

	{
		auto result = hset.Insert("value");
		SEOUL_UNITTESTING_ASSERT(!result.Second);
		SEOUL_UNITTESTING_ASSERT(*result.First == "value");
	}

	SEOUL_UNITTESTING_ASSERT(hset.Erase("value"));
	SEOUL_UNITTESTING_ASSERT(hset.IsEmpty());
}

/**
 * Confirm that swapping the table with a second table results in the state of
 * each being swapped.
 */
void HashSetTest::TestSwap()
{
	HashSet<Seoul::String> hset;
	SEOUL_UNITTESTING_ASSERT(hset.IsEmpty());

	VerifyAndInsertElements(hset);

	HashSet<String> htab2;
	SEOUL_UNITTESTING_ASSERT(htab2.IsEmpty());

	hset.Swap(htab2);

	// should be empty!
	SEOUL_UNITTESTING_ASSERT(hset.IsEmpty());

	// shouldn't be empty
	SEOUL_UNITTESTING_ASSERT(!htab2.IsEmpty());

	// confirm that 10 entries stored
	SEOUL_UNITTESTING_ASSERT_EQUAL(10u, htab2.GetSize());

	// confirm entries
	SEOUL_UNITTESTING_ASSERT(htab2.HasKey("one"));
	SEOUL_UNITTESTING_ASSERT(htab2.HasKey("two"));
	SEOUL_UNITTESTING_ASSERT(htab2.HasKey("three"));
	SEOUL_UNITTESTING_ASSERT(htab2.HasKey("tremendous"));
	SEOUL_UNITTESTING_ASSERT(htab2.HasKey("terrific"));
	SEOUL_UNITTESTING_ASSERT(htab2.HasKey("toofreakinawesome"));
	SEOUL_UNITTESTING_ASSERT(htab2.HasKey("four"));
	SEOUL_UNITTESTING_ASSERT(htab2.HasKey("five"));
	SEOUL_UNITTESTING_ASSERT(htab2.HasKey("six"));
	SEOUL_UNITTESTING_ASSERT(htab2.HasKey("seven"));

	SEOUL_UNITTESTING_ASSERT_EQUAL("one", *htab2.Find("one"));
	SEOUL_UNITTESTING_ASSERT_EQUAL("two", *htab2.Find("two"));
	SEOUL_UNITTESTING_ASSERT_EQUAL("three", *htab2.Find("three"));
	SEOUL_UNITTESTING_ASSERT_EQUAL("tremendous", *htab2.Find("tremendous"));
	SEOUL_UNITTESTING_ASSERT_EQUAL("terrific", *htab2.Find("terrific"));
	SEOUL_UNITTESTING_ASSERT_EQUAL("toofreakinawesome", *htab2.Find("toofreakinawesome"));
	SEOUL_UNITTESTING_ASSERT_EQUAL("four", *htab2.Find("four"));
	SEOUL_UNITTESTING_ASSERT_EQUAL("five", *htab2.Find("five"));
	SEOUL_UNITTESTING_ASSERT_EQUAL("six", *htab2.Find("six"));
	SEOUL_UNITTESTING_ASSERT_EQUAL("seven", *htab2.Find("seven"));
}

/**
 * Confirm that ints function properly as keys.
 */
void HashSetTest::TestIntKeys()
{
	HashSet<Int32> hset;
	SEOUL_UNITTESTING_ASSERT(hset.IsEmpty());

	SEOUL_UNITTESTING_ASSERT(hset.Insert(1).Second);
	SEOUL_UNITTESTING_ASSERT(hset.HasKey(1));
	SEOUL_UNITTESTING_ASSERT(!hset.HasKey(2));
	SEOUL_UNITTESTING_ASSERT(!hset.Insert(1).Second);
	SEOUL_UNITTESTING_ASSERT(hset.Insert(2).Second);
	SEOUL_UNITTESTING_ASSERT(hset.HasKey(2));
	SEOUL_UNITTESTING_ASSERT(!hset.HasKey(3));
	SEOUL_UNITTESTING_ASSERT(!hset.Insert(2).Second);
	SEOUL_UNITTESTING_ASSERT(hset.Insert(3).Second);
	SEOUL_UNITTESTING_ASSERT(hset.HasKey(3));
	SEOUL_UNITTESTING_ASSERT(!hset.HasKey(4));
	SEOUL_UNITTESTING_ASSERT(!hset.Insert(3).Second);

	// shouldn't be empty
	SEOUL_UNITTESTING_ASSERT(!hset.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(3u, hset.GetSize());
}

/**
 * Confirm that Hashables function properly as keys.
 */
void HashSetTest::TestHashableKeys()
{
	HashSet<Int*> hset;
	SEOUL_UNITTESTING_ASSERT(hset.IsEmpty());

	Int i1;
	SEOUL_UNITTESTING_ASSERT(hset.Insert(&i1).Second);
	SEOUL_UNITTESTING_ASSERT(!hset.Insert(&i1).Second);

	Int i2;
	SEOUL_UNITTESTING_ASSERT(hset.Insert(&i2).Second);
	SEOUL_UNITTESTING_ASSERT(!hset.Insert(&i2).Second);

	Int i3;
	SEOUL_UNITTESTING_ASSERT(hset.Insert(&i3).Second);
	SEOUL_UNITTESTING_ASSERT(!hset.Insert(&i3).Second);

	// shouldn't be empty
	SEOUL_UNITTESTING_ASSERT(!hset.IsEmpty());
	SEOUL_UNITTESTING_ASSERT(hset.GetSize() == 3);
}

/**
 * Targeted testing of a key-value key with an explicit nullptr key.
 */
void HashSetTest::TestNullKey()
{
	HashSet<Int32> hset;
	SEOUL_UNITTESTING_ASSERT(hset.IsEmpty());

	// Multiple passes to test integrity after filling the table and removing all entries.
	for (Int iPass = 0; iPass < 8; ++iPass)
	{
		{
			SEOUL_UNITTESTING_ASSERT(!hset.Erase(1));
			SEOUL_UNITTESTING_ASSERT(hset.Insert(1).Second);
			SEOUL_UNITTESTING_ASSERT(hset.HasKey(1));
			SEOUL_UNITTESTING_ASSERT(!hset.HasKey(0));
			SEOUL_UNITTESTING_ASSERT(!hset.HasKey(2));
			SEOUL_UNITTESTING_ASSERT(!hset.HasKey(3));
			SEOUL_UNITTESTING_ASSERT(!hset.Insert(1).Second);
			SEOUL_UNITTESTING_ASSERT_EQUAL(1u, hset.GetSize());
			SEOUL_UNITTESTING_ASSERT(!hset.Erase(2));
			SEOUL_UNITTESTING_ASSERT(hset.Insert(2).Second);
			SEOUL_UNITTESTING_ASSERT(hset.HasKey(2));
			SEOUL_UNITTESTING_ASSERT(!hset.HasKey(0));
			SEOUL_UNITTESTING_ASSERT(!hset.HasKey(3));
			SEOUL_UNITTESTING_ASSERT(!hset.Insert(2).Second);
			SEOUL_UNITTESTING_ASSERT_EQUAL(2u, hset.GetSize());
			SEOUL_UNITTESTING_ASSERT(!hset.Erase(0));
			SEOUL_UNITTESTING_ASSERT(hset.Insert(0).Second);
			SEOUL_UNITTESTING_ASSERT(hset.HasKey(0));
			SEOUL_UNITTESTING_ASSERT(!hset.HasKey(3));
			SEOUL_UNITTESTING_ASSERT_EQUAL(3u, hset.GetSize());
			SEOUL_UNITTESTING_ASSERT(!hset.Insert(0).Second);
			SEOUL_UNITTESTING_ASSERT_EQUAL(3u, hset.GetSize());
			SEOUL_UNITTESTING_ASSERT(hset.HasKey(0));
			SEOUL_UNITTESTING_ASSERT(!hset.Erase(3));
			SEOUL_UNITTESTING_ASSERT(hset.Insert(3).Second);
			SEOUL_UNITTESTING_ASSERT(hset.HasKey(3));
			SEOUL_UNITTESTING_ASSERT(!hset.Insert(3).Second);
			SEOUL_UNITTESTING_ASSERT_EQUAL(4u, hset.GetSize());

			// shouldn't be empty
			SEOUL_UNITTESTING_ASSERT(!hset.IsEmpty());
			SEOUL_UNITTESTING_ASSERT_EQUAL(4u, hset.GetSize());

			// confirm that only 4 entries stored
			SEOUL_UNITTESTING_ASSERT_EQUAL(4u, hset.GetSize());

			// verify iteration behaves as expected with a nullptr key present.
			{
				Bool abSeen[4] = { false };
				Int iIterationCount = 0;
				auto const iBegin = hset.Begin();
				auto const iEnd = hset.End();
				for (auto i = iBegin; iEnd != i; ++i)
				{
					SEOUL_UNITTESTING_ASSERT_LESS_THAN(*i, 4);
					SEOUL_UNITTESTING_ASSERT(!abSeen[*i]);
					abSeen[*i] = true;
					++iIterationCount;
				}
				SEOUL_UNITTESTING_ASSERT_EQUAL(4u, iIterationCount);
			}

			// erase and reinsert to verify integrity.
			SEOUL_UNITTESTING_ASSERT(hset.Erase(2));
			SEOUL_UNITTESTING_ASSERT(!hset.HasKey(2));
			SEOUL_UNITTESTING_ASSERT_EQUAL(3u, hset.GetSize());
			SEOUL_UNITTESTING_ASSERT(!hset.IsEmpty());
			SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(hset.Begin(), hset.End());
			SEOUL_UNITTESTING_ASSERT(hset.Erase(0));
			SEOUL_UNITTESTING_ASSERT(!hset.HasKey(0));
			SEOUL_UNITTESTING_ASSERT_EQUAL(2u, hset.GetSize());
			SEOUL_UNITTESTING_ASSERT(!hset.IsEmpty());
			SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(hset.Begin(), hset.End());
			SEOUL_UNITTESTING_ASSERT(hset.Erase(3));
			SEOUL_UNITTESTING_ASSERT(!hset.HasKey(3));
			SEOUL_UNITTESTING_ASSERT_EQUAL(1u, hset.GetSize());
			SEOUL_UNITTESTING_ASSERT(!hset.IsEmpty());
			SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(hset.Begin(), hset.End());
			// Attempt a reinsert of nullptr now.
			SEOUL_UNITTESTING_ASSERT(hset.Insert(0).Second);
			SEOUL_UNITTESTING_ASSERT(hset.HasKey(0));
			SEOUL_UNITTESTING_ASSERT_EQUAL(2u, hset.GetSize());
			SEOUL_UNITTESTING_ASSERT(!hset.IsEmpty());
			SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(hset.Begin(), hset.End());
			SEOUL_UNITTESTING_ASSERT(hset.Erase(1));
			SEOUL_UNITTESTING_ASSERT(!hset.HasKey(1));
			SEOUL_UNITTESTING_ASSERT_EQUAL(1u, hset.GetSize());
			SEOUL_UNITTESTING_ASSERT(!hset.IsEmpty());
			SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(hset.Begin(), hset.End());
			// Sanity check that we can get nullptr when it's the last element.
			SEOUL_UNITTESTING_ASSERT(hset.HasKey(0));
			SEOUL_UNITTESTING_ASSERT(hset.Erase(0));
			SEOUL_UNITTESTING_ASSERT(!hset.HasKey(0));
			SEOUL_UNITTESTING_ASSERT_EQUAL(0u, hset.GetSize());
			SEOUL_UNITTESTING_ASSERT(hset.IsEmpty());
			SEOUL_UNITTESTING_ASSERT_EQUAL(hset.Begin(), hset.End());
		}

		// Insert a big bucket of elements, then clear to stress test.
		for (Int32 iPadding = 0; iPadding < (iPass + 1) * 4; ++iPadding)
		{
			SEOUL_UNITTESTING_ASSERT(hset.Insert(iPadding).Second);
			SEOUL_UNITTESTING_ASSERT(!hset.IsEmpty());
		}

		SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)((iPass + 1) * 4), hset.GetSize());

		{
			HashSet<Int32> hsetTemp(hset);
			hset = hsetTemp;
		}

		// Every other pass, either clear or erase manually.
		if (0 == (iPass % 2))
		{
			hset.Clear();
		}
		else
		{
			for (Int32 iPadding = ((iPass + 1) * 4) - 1; iPadding >= 0; --iPadding)
			{
				SEOUL_UNITTESTING_ASSERT(hset.Erase(iPadding));
			}
		}

		SEOUL_UNITTESTING_ASSERT(hset.IsEmpty());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0u, hset.GetSize());
	}
}

// Regression for a bug in HashSet when Find() was called
// on a hashset with a null key argument but no null key
// was present in the set.
void HashSetTest::TestFindNull()
{
	HashSet<Int32> hset;
	SEOUL_UNITTESTING_ASSERT(hset.IsEmpty());
	SEOUL_UNITTESTING_ASSERT(hset.Insert(1).Second);
	SEOUL_UNITTESTING_ASSERT_EQUAL(nullptr, hset.Find(HashSet<Int32>::Traits::GetNullKey()));
	SEOUL_UNITTESTING_ASSERT(hset.Insert(HashSet<Int32>::Traits::GetNullKey()).Second);
	SEOUL_UNITTESTING_ASSERT_EQUAL(HashSet<Int32>::Traits::GetNullKey(), *hset.Find(HashSet<Int32>::Traits::GetNullKey()));
}

/**
 * Confirm that Seoul::Strings function properly as keys.
 */
void HashSetTest::TestSeoulStringKeys()
{
	ScopedPtr< HashSet<Seoul::String> > hset(SEOUL_NEW(MemoryBudgets::TBD) HashSet<Seoul::String>());
	SEOUL_UNITTESTING_ASSERT(hset->IsEmpty());

	Seoul::String sIn("one");
	SEOUL_UNITTESTING_ASSERT(hset->Insert(sIn).Second);
	SEOUL_UNITTESTING_ASSERT(!hset->Insert(sIn).Second);

	sIn = "two";
	SEOUL_UNITTESTING_ASSERT(hset->Insert(sIn).Second);
	SEOUL_UNITTESTING_ASSERT(!hset->Insert(sIn).Second);

	sIn = "three";
	SEOUL_UNITTESTING_ASSERT(hset->Insert(sIn).Second);
	SEOUL_UNITTESTING_ASSERT(!hset->Insert(sIn).Second);

	// shouldn't be empty
	SEOUL_UNITTESTING_ASSERT(!hset->IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(3u, hset->GetSize());
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
void HashSetTest::TestPointerKeys()
{
	ScopedPtr< HashSet<FancyKey*> > hset(SEOUL_NEW(MemoryBudgets::TBD) HashSet<FancyKey*>());

	// use a pointer to something as a key
	ScopedPtr<FancyKey> test(SEOUL_NEW(MemoryBudgets::TBD) FancyKey);
	SEOUL_UNITTESTING_ASSERT(hset->Insert(test.Get()).Second);

	// shouldn't be empty
	SEOUL_UNITTESTING_ASSERT(!hset->IsEmpty());
}

/**
 * Confirm that entry erasure works
 */
void HashSetTest::TestErase()
{
	// make a hashtable of ints->strings
	ScopedPtr< HashSet<String> > hset(SEOUL_NEW(MemoryBudgets::TBD) HashSet<String>());

	// populate the table
	SEOUL_UNITTESTING_ASSERT(hset->Insert("one").Second);
	SEOUL_UNITTESTING_ASSERT(hset->Insert("two").Second);
	SEOUL_UNITTESTING_ASSERT(hset->Insert("three").Second);
	SEOUL_UNITTESTING_ASSERT(hset->Insert("tremendous").Second);
	SEOUL_UNITTESTING_ASSERT(hset->Insert("terrific").Second);
	SEOUL_UNITTESTING_ASSERT(hset->Insert("toofreakinawesome").Second);
	SEOUL_UNITTESTING_ASSERT(hset->Insert("four").Second);
	SEOUL_UNITTESTING_ASSERT(hset->Insert("five").Second);
	SEOUL_UNITTESTING_ASSERT(hset->Insert("six").Second);
	SEOUL_UNITTESTING_ASSERT(hset->Insert("seven").Second);

	// make sure it counted
	SEOUL_UNITTESTING_ASSERT_EQUAL(10u, hset->GetSize());

	// remove an entry
	SEOUL_UNITTESTING_ASSERT(hset->Erase("one"));

	// make sure removal worked
	SEOUL_UNITTESTING_ASSERT_EQUAL(9u, hset->GetSize());
	SEOUL_UNITTESTING_ASSERT(hset->HasKey("two"));
	SEOUL_UNITTESTING_ASSERT(hset->HasKey("three"));
	SEOUL_UNITTESTING_ASSERT(hset->HasKey("tremendous"));
	SEOUL_UNITTESTING_ASSERT(hset->HasKey("terrific"));
	SEOUL_UNITTESTING_ASSERT(hset->HasKey("toofreakinawesome"));
	SEOUL_UNITTESTING_ASSERT(hset->HasKey("four"));
	SEOUL_UNITTESTING_ASSERT(hset->HasKey("five"));
	SEOUL_UNITTESTING_ASSERT(hset->HasKey("six"));
	SEOUL_UNITTESTING_ASSERT(hset->HasKey("seven"));

	SEOUL_UNITTESTING_ASSERT(hset->Erase("two"));
	SEOUL_UNITTESTING_ASSERT(hset->Erase("seven"));
	SEOUL_UNITTESTING_ASSERT_EQUAL(7u, hset->GetSize());
	SEOUL_UNITTESTING_ASSERT(hset->HasKey("three"));
	SEOUL_UNITTESTING_ASSERT(hset->HasKey("tremendous"));
	SEOUL_UNITTESTING_ASSERT(hset->HasKey("terrific"));
	SEOUL_UNITTESTING_ASSERT(hset->HasKey("toofreakinawesome"));
	SEOUL_UNITTESTING_ASSERT(hset->HasKey("four"));
	SEOUL_UNITTESTING_ASSERT(hset->HasKey("five"));
	SEOUL_UNITTESTING_ASSERT(hset->HasKey("six"));

	SEOUL_UNITTESTING_ASSERT(hset->Erase("three"));
	SEOUL_UNITTESTING_ASSERT(hset->Insert("one").Second);
	SEOUL_UNITTESTING_ASSERT(hset->Erase("six"));
	SEOUL_UNITTESTING_ASSERT_EQUAL(6u, hset->GetSize());
	SEOUL_UNITTESTING_ASSERT(hset->HasKey("one"));
	SEOUL_UNITTESTING_ASSERT(hset->HasKey("tremendous"));
	SEOUL_UNITTESTING_ASSERT(hset->HasKey("terrific"));
	SEOUL_UNITTESTING_ASSERT(hset->HasKey("toofreakinawesome"));
	SEOUL_UNITTESTING_ASSERT(hset->HasKey("four"));
	SEOUL_UNITTESTING_ASSERT(hset->HasKey("five"));

	SEOUL_UNITTESTING_ASSERT(hset->Erase("five"));
	SEOUL_UNITTESTING_ASSERT(hset->Insert("two").Second);
	SEOUL_UNITTESTING_ASSERT(hset->Insert("three").Second);
	SEOUL_UNITTESTING_ASSERT(hset->Erase("tremendous"));
	SEOUL_UNITTESTING_ASSERT_EQUAL(6u, hset->GetSize());
	SEOUL_UNITTESTING_ASSERT(hset->HasKey("one"));
	SEOUL_UNITTESTING_ASSERT(hset->HasKey("two"));
	SEOUL_UNITTESTING_ASSERT(hset->HasKey("three"));
	SEOUL_UNITTESTING_ASSERT(hset->HasKey("terrific"));
	SEOUL_UNITTESTING_ASSERT(hset->HasKey("toofreakinawesome"));
	SEOUL_UNITTESTING_ASSERT(hset->HasKey("four"));

	SEOUL_UNITTESTING_ASSERT(hset->Erase("one"));
	SEOUL_UNITTESTING_ASSERT(hset->Erase("two"));
	SEOUL_UNITTESTING_ASSERT(hset->Erase("three"));
	SEOUL_UNITTESTING_ASSERT_EQUAL(3u, hset->GetSize());
	SEOUL_UNITTESTING_ASSERT(hset->HasKey("terrific"));
	SEOUL_UNITTESTING_ASSERT(hset->HasKey("toofreakinawesome"));
	SEOUL_UNITTESTING_ASSERT(hset->HasKey("four"));

	SEOUL_UNITTESTING_ASSERT(hset->Erase("terrific"));
	SEOUL_UNITTESTING_ASSERT(hset->Erase("toofreakinawesome"));
	SEOUL_UNITTESTING_ASSERT(hset->Erase("four"));

	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, hset->GetSize());
	SEOUL_UNITTESTING_ASSERT(hset->IsEmpty());
}

void HashSetTest::TestIterators()
{
	// make a hashtable of ints->strings
	typedef HashSet<String> SetType;
	ScopedPtr< SetType > hset(SEOUL_NEW(MemoryBudgets::TBD) SetType());

	// populate the table
	SEOUL_UNITTESTING_ASSERT(hset->Insert("one").Second);
	SEOUL_UNITTESTING_ASSERT(hset->Insert("two").Second);
	SEOUL_UNITTESTING_ASSERT(hset->Insert("three").Second);
	SEOUL_UNITTESTING_ASSERT(hset->Insert("tremendous").Second);
	SEOUL_UNITTESTING_ASSERT(hset->Insert("terrific").Second);
	SEOUL_UNITTESTING_ASSERT(hset->Insert("toofreakinawesome").Second);
	SEOUL_UNITTESTING_ASSERT(hset->Insert("four").Second);
	SEOUL_UNITTESTING_ASSERT(hset->Insert("five").Second);
	SEOUL_UNITTESTING_ASSERT(hset->Insert("six").Second);
	SEOUL_UNITTESTING_ASSERT(hset->Insert("seven").Second);

	// make sure it counted
	SEOUL_UNITTESTING_ASSERT_EQUAL(10u, hset->GetSize());

	// iterator over it - constant
	{
		UInt32 nCount = 0;
		HashSet<String> tester;
		for (auto i = const_cast<SetType const*>(hset.Get())->Begin(); const_cast<SetType const*>(hset.Get())->End() != i; ++i)
		{
			++nCount;
			SEOUL_UNITTESTING_ASSERT(tester.Insert(*i).Second);
		}

		SEOUL_UNITTESTING_ASSERT_EQUAL(10u, nCount);

		SEOUL_UNITTESTING_ASSERT(tester.HasKey("one"));
		SEOUL_UNITTESTING_ASSERT(tester.HasKey("two"));
		SEOUL_UNITTESTING_ASSERT(tester.HasKey("three"));
		SEOUL_UNITTESTING_ASSERT(tester.HasKey("tremendous"));
		SEOUL_UNITTESTING_ASSERT(tester.HasKey("terrific"));
		SEOUL_UNITTESTING_ASSERT(tester.HasKey("toofreakinawesome"));
		SEOUL_UNITTESTING_ASSERT(tester.HasKey("four"));
		SEOUL_UNITTESTING_ASSERT(tester.HasKey("five"));
		SEOUL_UNITTESTING_ASSERT(tester.HasKey("six"));
		SEOUL_UNITTESTING_ASSERT(tester.HasKey("seven"));
	}

	// iterator over it - constant (post increment)
	{
		UInt32 nCount = 0;
		HashSet<String> tester;
		for (auto i = const_cast<SetType const*>(hset.Get())->Begin(); const_cast<SetType const*>(hset.Get())->End() != i; i++)
		{
			++nCount;
			SEOUL_UNITTESTING_ASSERT(tester.Insert(*i).Second);
		}

		SEOUL_UNITTESTING_ASSERT_EQUAL(10u, nCount);

		SEOUL_UNITTESTING_ASSERT(tester.HasKey("one"));
		SEOUL_UNITTESTING_ASSERT(tester.HasKey("two"));
		SEOUL_UNITTESTING_ASSERT(tester.HasKey("three"));
		SEOUL_UNITTESTING_ASSERT(tester.HasKey("tremendous"));
		SEOUL_UNITTESTING_ASSERT(tester.HasKey("terrific"));
		SEOUL_UNITTESTING_ASSERT(tester.HasKey("toofreakinawesome"));
		SEOUL_UNITTESTING_ASSERT(tester.HasKey("four"));
		SEOUL_UNITTESTING_ASSERT(tester.HasKey("five"));
		SEOUL_UNITTESTING_ASSERT(tester.HasKey("six"));
		SEOUL_UNITTESTING_ASSERT(tester.HasKey("seven"));
	}

	// iterator over it - read-write
	{
		UInt32 nCount = 0;
		HashSet<String> tester;
		for (auto i = hset->Begin(); hset->End() != i; ++i)
		{
			++nCount;
			String sValue(i->CStr());
			SEOUL_UNITTESTING_ASSERT(tester.Insert(sValue).Second);
		}

		SEOUL_UNITTESTING_ASSERT_EQUAL(10u, nCount);

		SEOUL_UNITTESTING_ASSERT(tester.HasKey("one"));
		SEOUL_UNITTESTING_ASSERT(tester.HasKey("two"));
		SEOUL_UNITTESTING_ASSERT(tester.HasKey("three"));
		SEOUL_UNITTESTING_ASSERT(tester.HasKey("tremendous"));
		SEOUL_UNITTESTING_ASSERT(tester.HasKey("terrific"));
		SEOUL_UNITTESTING_ASSERT(tester.HasKey("toofreakinawesome"));
		SEOUL_UNITTESTING_ASSERT(tester.HasKey("four"));
		SEOUL_UNITTESTING_ASSERT(tester.HasKey("five"));
		SEOUL_UNITTESTING_ASSERT(tester.HasKey("six"));
		SEOUL_UNITTESTING_ASSERT(tester.HasKey("seven"));
	}

	// iterator over it - read-write (post increment)
	{
		UInt32 nCount = 0;
		HashSet<String> tester;
		for (auto i = hset->Begin(); hset->End() != i; i++)
		{
			++nCount;
			String sValue(i->CStr());
			SEOUL_UNITTESTING_ASSERT(tester.Insert(sValue).Second);
		}

		SEOUL_UNITTESTING_ASSERT_EQUAL(10u, nCount);

		SEOUL_UNITTESTING_ASSERT(tester.HasKey("one"));
		SEOUL_UNITTESTING_ASSERT(tester.HasKey("two"));
		SEOUL_UNITTESTING_ASSERT(tester.HasKey("three"));
		SEOUL_UNITTESTING_ASSERT(tester.HasKey("tremendous"));
		SEOUL_UNITTESTING_ASSERT(tester.HasKey("terrific"));
		SEOUL_UNITTESTING_ASSERT(tester.HasKey("toofreakinawesome"));
		SEOUL_UNITTESTING_ASSERT(tester.HasKey("four"));
		SEOUL_UNITTESTING_ASSERT(tester.HasKey("five"));
		SEOUL_UNITTESTING_ASSERT(tester.HasKey("six"));
		SEOUL_UNITTESTING_ASSERT(tester.HasKey("seven"));
	}

	// iterator - verify reference return value.
	{
		for (auto const& s : *hset)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(hset->Find(s), &s);
		}
	}
	// const iterator - verify reference return value.
	{
		for (auto const& s : *const_cast<SetType const*>(hset.Get()))
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(hset->Find(s), &s);
		}
	}

	// iterator - pre increment
	{
		SetType set;
		SEOUL_UNITTESTING_ASSERT(set.Insert("one").Second);
		SEOUL_UNITTESTING_ASSERT(set.Insert("two").Second);

		auto i = set.Begin();
		SEOUL_UNITTESTING_ASSERT_EQUAL("one", *i);

		auto j = ++i;

		SEOUL_UNITTESTING_ASSERT_EQUAL("two", *i);
		SEOUL_UNITTESTING_ASSERT_EQUAL("two", *j);

		SEOUL_UNITTESTING_ASSERT_EQUAL(set.End(), ++i);
	}

	// iterator - post increment
	{
		SetType set;
		SEOUL_UNITTESTING_ASSERT(set.Insert("one").Second);
		SEOUL_UNITTESTING_ASSERT(set.Insert("two").Second);

		auto i = set.Begin();
		SEOUL_UNITTESTING_ASSERT_EQUAL("one", *i);

		auto j = i++;

		SEOUL_UNITTESTING_ASSERT_EQUAL("two", *i);
		SEOUL_UNITTESTING_ASSERT_EQUAL("one", *j);

		SEOUL_UNITTESTING_ASSERT_EQUAL("two", *(i++));
		SEOUL_UNITTESTING_ASSERT_EQUAL(set.End(), i);
	}
}

void HashSetTest::TestRangedFor()
{
	HashSet<Int> testSet;
	SEOUL_UNITTESTING_ASSERT(testSet.Insert(3).Second);
	SEOUL_UNITTESTING_ASSERT(testSet.Insert(7).Second);
	SEOUL_UNITTESTING_ASSERT(testSet.Insert(2).Second);

	{
		auto i = testSet.Begin();
		for (auto v : testSet)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(*i, v);
			++i;
		}
	}

	SEOUL_UNITTESTING_ASSERT(testSet.Insert(35).Second);

	{
		auto i = testSet.Begin();
		for (auto v : testSet)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(*i, v);
			++i;
		}
	}

	SEOUL_UNITTESTING_ASSERT(testSet.Insert(77).Second);

	{
		auto i = testSet.Begin();
		for (auto v : testSet)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(*i, v);
			++i;
		}
	}
}

void HashSetTest::TestUtilities()
{
	HashSet<Int> testSet;

	Vector<Int> v;
	GetHashSetKeys(testSet, v);
	SEOUL_UNITTESTING_ASSERT(v.IsEmpty());

	SEOUL_UNITTESTING_ASSERT(testSet.Insert(3).Second);
	SEOUL_UNITTESTING_ASSERT(testSet.Insert(7).Second);
	SEOUL_UNITTESTING_ASSERT(testSet.Insert(2).Second);

	GetHashSetKeys(testSet, v);

	SEOUL_UNITTESTING_ASSERT_EQUAL(7, v[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, v[1]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, v[2]);

	// Make sure op clears the output vector properly.
	testSet.Clear();
	GetHashSetKeys(testSet, v);
	SEOUL_UNITTESTING_ASSERT(v.IsEmpty());
}

void HashSetTest::TestEquality()
{
	HashSet<Int> hsetLeft;
	hsetLeft.Insert(3);
	hsetLeft.Insert(4);
	hsetLeft.Insert(5);

	HashSet<Int> hsetRight;
	hsetRight.Insert(3);
	hsetRight.Insert(4);
	hsetRight.Insert(5);

	SEOUL_UNITTESTING_ASSERT(hsetLeft == hsetRight);

	hsetRight.Erase(3);

	SEOUL_UNITTESTING_ASSERT(hsetLeft != hsetRight);
}

void HashSetTest::TestContains()
{
	HashSet<Int> hsetLeft;
	hsetLeft.Insert(3);
	hsetLeft.Insert(4);
	hsetLeft.Insert(5);

	HashSet<Int> hsetRight;
	hsetRight.Insert(3);
	hsetRight.Insert(4);

	SEOUL_UNITTESTING_ASSERT(hsetLeft.Contains(hsetRight));

	hsetRight.Insert(9);

	SEOUL_UNITTESTING_ASSERT(!hsetLeft.Contains(hsetRight));

	hsetRight.Clear();
	hsetRight.Insert(9);

	SEOUL_UNITTESTING_ASSERT(!hsetLeft.Contains(hsetRight));
}

void HashSetTest::TestDisjoint()
{
	HashSet<Int> hsetLeft;
	hsetLeft.Insert(3);
	hsetLeft.Insert(4);
	hsetLeft.Insert(5);

	HashSet<Int> hsetRight;
	hsetRight.Insert(6);
	hsetRight.Insert(7);
	hsetRight.Insert(8);

	SEOUL_UNITTESTING_ASSERT(hsetLeft.Disjoint(hsetRight));

	hsetRight.Insert(3);

	SEOUL_UNITTESTING_ASSERT(!hsetLeft.Disjoint(hsetRight));
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
