/**
 * \file CoreTest.cpp
 * \brief Unit test implementations for core functionality
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Core.h"
#include "CoreTest.h"
#include "Logger.h"
#include "ReflectionDefine.h"
#include "UnitTesting.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(CoreTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestRoundUpToPowerOf2)
SEOUL_END_TYPE()

/**
 * Tests the RoundUpToPowerOf2 function
 */
void CoreTest::TestRoundUpToPowerOf2(void)
{
	UInt32 anInputs[] =  {0, 1, 2, 3, 4, 5, 0x80000000u, 0x80000001u};
	UInt32 anOutputs[] = {0, 1, 2, 4, 4, 8, 0x80000000u, 0};

	SEOUL_UNITTESTING_ASSERT_EQUAL(SEOUL_ARRAY_COUNT(anInputs), SEOUL_ARRAY_COUNT(anOutputs));

	for(UInt nIndex = 0; nIndex < SEOUL_ARRAY_COUNT(anInputs); nIndex++)
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(anOutputs[nIndex], RoundUpToPowerOf2(anInputs[nIndex]));
	}
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
