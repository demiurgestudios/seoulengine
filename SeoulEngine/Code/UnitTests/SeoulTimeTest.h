/**
 * \file SeoulTimeTest.h
 * \brief SeoulTime unit test fixture.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SEOUL_TIME_TEST_H
#define SEOUL_TIME_TEST_H

#include "Prereqs.h"
#include "SeoulTime.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

/**
 * Text fixture class for Memory Manager.
 */
class SeoulTimeTest SEOUL_SEALED
{
public:
	SeoulTimeTest();
	~SeoulTimeTest();

	void TestTime();
	void TestWorldTime();
	void TestTimeInterval();
	void TestISO8601Parsing();
	void TestISO8601ToString();
	void TestGetDayStartTime();
};

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif  // include guard
