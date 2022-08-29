/**
 * \file SeoulWildcardTest.cpp
 * \brief Unit tests for the Wildcard class.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Logger.h"
#include "ReflectionDefine.h"
#include "SeoulWildcard.h"
#include "SeoulWildcardTest.h"
#include "UnitTesting.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(SeoulWildcardTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestAsterisk)
	SEOUL_METHOD(TestMixed)
	SEOUL_METHOD(TestQuestionMark)
SEOUL_END_TYPE()

void SeoulWildcardTest::TestAsterisk()
{
	Wildcard test("Authored/*");

	// Try with both separators.
	SEOUL_UNITTESTING_ASSERT(test.IsExactMatch("Authored/Seoul"));
	SEOUL_UNITTESTING_ASSERT(test.IsExactMatch("Authored\\Seoul"));
	
	SEOUL_UNITTESTING_ASSERT(!test.IsExactMatch("Run/Authored/Seoul"));
	SEOUL_UNITTESTING_ASSERT(!test.IsExactMatch("Run\\Authored\\Seoul"));
	
	SEOUL_UNITTESTING_ASSERT(test.IsMatch("Run/Authored/Seoul"));
	SEOUL_UNITTESTING_ASSERT(test.IsMatch("Run\\Authored\\Seoul"));
}

void SeoulWildcardTest::TestMixed()
{
	Wildcard test("Authored/?ee?/Hello*");

	// Try with both separators.
	SEOUL_UNITTESTING_ASSERT(test.IsExactMatch("Authored/beef/Hello"));
	SEOUL_UNITTESTING_ASSERT(test.IsExactMatch("Authored\\beef\\Hello"));

	SEOUL_UNITTESTING_ASSERT(!test.IsExactMatch("Authored/eef/Hello"));
	SEOUL_UNITTESTING_ASSERT(!test.IsExactMatch("Authored\\eef\\Hello"));

	SEOUL_UNITTESTING_ASSERT(!test.IsMatch("Authored/eef/Hello"));
	SEOUL_UNITTESTING_ASSERT(!test.IsMatch("Authored\\eef\\Hello"));

	SEOUL_UNITTESTING_ASSERT(!test.IsExactMatch("Authored/beefy/Hello"));
	SEOUL_UNITTESTING_ASSERT(!test.IsExactMatch("Authored\\beefy\\Hello"));

	SEOUL_UNITTESTING_ASSERT(!test.IsMatch("Authored/beefy/Hello"));
	SEOUL_UNITTESTING_ASSERT(!test.IsMatch("Authored\\beefy\\Hello"));

	SEOUL_UNITTESTING_ASSERT(test.IsExactMatch("Authored/beef/Hello/Goodbye"));
	SEOUL_UNITTESTING_ASSERT(test.IsExactMatch("Authored\\beef\\Hello\\Goodbye"));

	SEOUL_UNITTESTING_ASSERT(!test.IsExactMatch("Shop/Authored/beef/Hello/Goodbye"));
	SEOUL_UNITTESTING_ASSERT(!test.IsExactMatch("Shop\\Authored\\beef\\Hello\\Goodbye"));

	SEOUL_UNITTESTING_ASSERT(test.IsMatch("Shop/Authored/beef/Hello/Goodbye"));
	SEOUL_UNITTESTING_ASSERT(test.IsMatch("Shop\\Authored\\beef\\Hello\\Goodbye"));
}

void SeoulWildcardTest::TestQuestionMark()
{
	Wildcard test("Authored/?eoul");

	// Try with both separators.
	SEOUL_UNITTESTING_ASSERT(test.IsExactMatch("Authored/Seoul"));
	SEOUL_UNITTESTING_ASSERT(test.IsExactMatch("Authored\\Seoul"));
	
	SEOUL_UNITTESTING_ASSERT(!test.IsExactMatch("Run/Authored/Seoul"));
	SEOUL_UNITTESTING_ASSERT(!test.IsExactMatch("Run\\Authored\\Seoul"));

	SEOUL_UNITTESTING_ASSERT(test.IsMatch("Run/Authored/Seoul"));
	SEOUL_UNITTESTING_ASSERT(test.IsMatch("Run\\Authored\\Seoul"));

	SEOUL_UNITTESTING_ASSERT(!test.IsExactMatch("Authored/Sseoul"));
	SEOUL_UNITTESTING_ASSERT(!test.IsExactMatch("Authored\\Sseoul"));

	SEOUL_UNITTESTING_ASSERT(!test.IsMatch("Authored/Sseoul"));
	SEOUL_UNITTESTING_ASSERT(!test.IsMatch("Authored\\Sseoul"));
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
