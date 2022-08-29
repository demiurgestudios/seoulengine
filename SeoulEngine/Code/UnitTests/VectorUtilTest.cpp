/**
 * \file VectorUtilTest.cpp
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ReflectionDefine.h"
#include "UnitTesting.h"
#include "VectorUtil.h"
#include "VectorUtilTest.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(VectorUtilTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestFindValueRandomTiebreaker)
SEOUL_END_TYPE()

void VectorUtilTest::TestFindValueRandomTiebreaker()
{
	// Failure.
	{
		Vector<UInt> v;

		Vector<UInt> vRes;
		SEOUL_UNITTESTING_ASSERT(!FindValueRandomTiebreaker(v, VectorSelectionType::Lowest, vRes));
		SEOUL_UNITTESTING_ASSERT(!FindValueRandomTiebreaker(v, VectorSelectionType::LowestIncludeTies, vRes));
		SEOUL_UNITTESTING_ASSERT(!FindValueRandomTiebreaker(v, VectorSelectionType::Highest, vRes));
		SEOUL_UNITTESTING_ASSERT(!FindValueRandomTiebreaker(v, VectorSelectionType::HighestIncludeTies, vRes));
	}

	// Success.
	{
		auto const a = { 3u, 2u, 1u, 1u, 3u };
		Vector<UInt> v(a.begin(), a.end());

		Vector<UInt> vRes;
		SEOUL_UNITTESTING_ASSERT(FindValueRandomTiebreaker(v, VectorSelectionType::Lowest, vRes));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, vRes.GetSize());
		SEOUL_UNITTESTING_ASSERT(2 == vRes[0] || 3 == vRes[0]); // Tie break.

		SEOUL_UNITTESTING_ASSERT(FindValueRandomTiebreaker(v, VectorSelectionType::HighestIncludeTies, vRes));
		SEOUL_UNITTESTING_ASSERT_EQUAL(2, vRes.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, vRes[0]);
		SEOUL_UNITTESTING_ASSERT_EQUAL(4, vRes[1]);

		SEOUL_UNITTESTING_ASSERT(FindValueRandomTiebreaker(v, VectorSelectionType::Highest, vRes));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, vRes.GetSize());
		SEOUL_UNITTESTING_ASSERT(0 == vRes[0] || 4 == vRes[0]); // Tie break.

		SEOUL_UNITTESTING_ASSERT(FindValueRandomTiebreaker(v, VectorSelectionType::LowestIncludeTies, vRes));
		SEOUL_UNITTESTING_ASSERT_EQUAL(2, vRes.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(2, vRes[0]);
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, vRes[1]);
	}
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
