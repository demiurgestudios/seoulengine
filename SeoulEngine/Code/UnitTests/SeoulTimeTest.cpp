/**
 * \file SeoulTimeTest.cpp
 * \brief Contains the unit test implementations for SeoulTime.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Logger.h"
#include "ReflectionDefine.h"
#include "SeoulTimeTest.h"
#include "SeoulTime.h"
#include "Thread.h"
#include "UnitTesting.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(SeoulTimeTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestTime)
	SEOUL_METHOD(TestWorldTime)
	SEOUL_METHOD(TestTimeInterval)
	SEOUL_METHOD(TestISO8601Parsing)
	SEOUL_METHOD(TestISO8601ToString)
	SEOUL_METHOD(TestGetDayStartTime)
SEOUL_END_TYPE()

/**
 * Set up test framework. No initialization necessary for SeoulTime.
 */
SeoulTimeTest::SeoulTimeTest()
{
}

/**
 * Shut down test framework. Nothing to do for SeoulTime.
 */
SeoulTimeTest::~SeoulTimeTest()
{
}

void SeoulTimeTest::TestTime()
{
	auto const iTicks = SeoulTime::GetGameTimeInTicks();
	auto const fMicros = SeoulTime::GetGameTimeInMicroseconds();
	auto const fMillis = SeoulTime::GetGameTimeInMilliseconds();

	SEOUL_UNITTESTING_ASSERT_GREATER_EQUAL(fMicros, SeoulTime::ConvertTicksToMicroseconds(iTicks));
	SEOUL_UNITTESTING_ASSERT_GREATER_EQUAL(fMillis, SeoulTime::ConvertTicksToMilliseconds(iTicks));
	SEOUL_UNITTESTING_ASSERT_GREATER_EQUAL(fMillis * 1000.0, fMicros);

	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(SeoulTime::ConvertTicksToMilliseconds(iTicks) * 1000.0, SeoulTime::ConvertTicksToMicroseconds(iTicks), Epsilon);
}

void SeoulTimeTest::TestWorldTime()
{
	WorldTime time;

	SEOUL_UNITTESTING_ASSERT_EQUAL((Int64)0, time.GetMicroseconds());
	SEOUL_UNITTESTING_ASSERT(time == WorldTime());

	time.SetMicroseconds(2013u);
	SEOUL_UNITTESTING_ASSERT_EQUAL((Int64)2013, time.GetMicroseconds());

	time.AddSecondsDouble(0.0000015);
	SEOUL_UNITTESTING_ASSERT_EQUAL((Int64)2014, time.GetMicroseconds());

	time.AddSecondsDouble(0.0000009);
	SEOUL_UNITTESTING_ASSERT_EQUAL((Int64)2014, time.GetMicroseconds());

	time.AddSeconds(1);
	SEOUL_UNITTESTING_ASSERT_EQUAL((Int64)1002014, time.GetMicroseconds());

	time.AddMilliseconds(-1000);
	SEOUL_UNITTESTING_ASSERT_EQUAL((Int64)2014, time.GetMicroseconds());

	time.AddMicroseconds(42);
	SEOUL_UNITTESTING_ASSERT_EQUAL((Int64)2056, time.GetMicroseconds());

	time.Reset();
	SEOUL_UNITTESTING_ASSERT_EQUAL((Int64)0, time.GetMicroseconds());

	WorldTime time2;
	time2.SetMicroseconds(1u);

	SEOUL_UNITTESTING_ASSERT(time < time2);
	SEOUL_UNITTESTING_ASSERT(!(time == time2));
	SEOUL_UNITTESTING_ASSERT(time != time2);
	SEOUL_UNITTESTING_ASSERT(time <= time2);
	SEOUL_UNITTESTING_ASSERT(!(time > time2));
	SEOUL_UNITTESTING_ASSERT(!(time >= time2));

	time2.Reset();
	SEOUL_UNITTESTING_ASSERT(!(time < time2));
	SEOUL_UNITTESTING_ASSERT(time == time2);
	SEOUL_UNITTESTING_ASSERT(!(time != time2));
	SEOUL_UNITTESTING_ASSERT(time <= time2);
	SEOUL_UNITTESTING_ASSERT(!(time > time2));
	SEOUL_UNITTESTING_ASSERT(time >= time2);
}

void SeoulTimeTest::TestTimeInterval()
{
	TimeInterval i0;
	SEOUL_UNITTESTING_ASSERT_EQUAL(0ll, i0.GetMicroseconds());

	TimeInterval i1(42), i2(42, 0);
	SEOUL_UNITTESTING_ASSERT(i1 == i2);
	SEOUL_UNITTESTING_ASSERT_EQUAL(42000000ll, i1.GetMicroseconds());

	TimeInterval i3(-12, 1), i4(-11, -999999);
	SEOUL_UNITTESTING_ASSERT(i4 == i3);
	SEOUL_UNITTESTING_ASSERT_EQUAL(-11999999ll, i3.GetMicroseconds());

	TimeInterval i5(-10, -1999999), i6(-13, 1000001);
	SEOUL_UNITTESTING_ASSERT(i5 == i3);
	SEOUL_UNITTESTING_ASSERT(i6 == i3);

	TimeInterval i7 = TimeInterval::FromMicroseconds(-11999999);
	SEOUL_UNITTESTING_ASSERT(i7 == i3);

	SEOUL_UNITTESTING_ASSERT_EQUAL(-12ll, i3.GetSeconds());
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(-11.999999, i3.GetSecondsAsDouble(), 1e-12);

	TimeInterval i8(-12, 2);
	SEOUL_UNITTESTING_ASSERT(i3 < i8);
	SEOUL_UNITTESTING_ASSERT(i3 <= i8);
	SEOUL_UNITTESTING_ASSERT(!(i3 == i8));
	SEOUL_UNITTESTING_ASSERT(i3 != i8);
	SEOUL_UNITTESTING_ASSERT(!(i3 > i8));
	SEOUL_UNITTESTING_ASSERT(!(i3 >= i8));

	SEOUL_UNITTESTING_ASSERT(!(i8 < i3));
	SEOUL_UNITTESTING_ASSERT(!(i8 <= i3));
	SEOUL_UNITTESTING_ASSERT(!(i8 == i3));
	SEOUL_UNITTESTING_ASSERT(i8 != i3);
	SEOUL_UNITTESTING_ASSERT(i8 > i3);
	SEOUL_UNITTESTING_ASSERT(i8 >= i3);

	SEOUL_UNITTESTING_ASSERT(!(i3 < i7));
	SEOUL_UNITTESTING_ASSERT(i3 <= i7);
	SEOUL_UNITTESTING_ASSERT(i3 == i7);
	SEOUL_UNITTESTING_ASSERT(!(i3 != i7));
	SEOUL_UNITTESTING_ASSERT(!(i3 > i7));
	SEOUL_UNITTESTING_ASSERT(i3 >= i7);

	TimeInterval i9(11, 999999);
	SEOUL_UNITTESTING_ASSERT(i9 == -i3);
	SEOUL_UNITTESTING_ASSERT(i3 == -i9);
}

void SeoulTimeTest::TestISO8601Parsing()
{
	// Got the correct epoch time from http://www.epochconverter.com

	String sDateTime("2014-03-19T21:32:05+01:00");
	WorldTime time = WorldTime::ParseISO8601DateTime(sDateTime);
	SEOUL_UNITTESTING_ASSERT(time.GetMicroseconds() == 1395261125000000ll);

	// Same as above, but with +HHMM syntax instead of +HH:MM
	sDateTime = String("2014-03-19T21:32:05+0100");
	time = WorldTime::ParseISO8601DateTime(sDateTime);
	SEOUL_UNITTESTING_ASSERT(time.GetMicroseconds() == 1395261125000000ll);

	sDateTime = String("2014-01-01T06:14:05-01:20");
	time = WorldTime::ParseISO8601DateTime(sDateTime);
	SEOUL_UNITTESTING_ASSERT(time.GetMicroseconds() == 1388561645000000ll);

	// Same as above, but with +HHMM syntax instead of +HH:MM
	sDateTime = String("2014-01-01T06:14:05-0120");
	time = WorldTime::ParseISO8601DateTime(sDateTime);
	SEOUL_UNITTESTING_ASSERT(time.GetMicroseconds() == 1388561645000000ll);

	sDateTime = String("2013-04-28T12:56:15.004-03:50");
	time = WorldTime::ParseISO8601DateTime(sDateTime);
	SEOUL_UNITTESTING_ASSERT(time.GetMicroseconds() == 1367167575004000ll);

	sDateTime = String("2013-04-28T12:56:15.123456-03:50");
	time = WorldTime::ParseISO8601DateTime(sDateTime);
	SEOUL_UNITTESTING_ASSERT(time.GetMicroseconds() == 1367167575123456ll);

	sDateTime = String("2013-04-28T12:56:15.12345678-03:50");
	time = WorldTime::ParseISO8601DateTime(sDateTime);
	SEOUL_UNITTESTING_ASSERT(time.GetMicroseconds() == 1367167575123456ll);

	sDateTime = String("2013-04-28T12:56:15.123-03:50");
	time = WorldTime::ParseISO8601DateTime(sDateTime);
	SEOUL_UNITTESTING_ASSERT(time.GetMicroseconds() == 1367167575123000ll);

	// " " separator between date and time, instead of "T"
	sDateTime = String("2013-04-28 12:56:15.123-03:50");
	time = WorldTime::ParseISO8601DateTime(sDateTime);
	SEOUL_UNITTESTING_ASSERT(time.GetMicroseconds() == 1367167575123000ll);

	sDateTime = String("1970-01-01T00:00:00.00+00:00");
	time = WorldTime::ParseISO8601DateTime(sDateTime);
	SEOUL_UNITTESTING_ASSERT(time.GetMicroseconds() == 0ll);
	SEOUL_UNITTESTING_ASSERT(time.GetMicroseconds() == WorldTime().GetMicroseconds());

	sDateTime = String("1970-01-01T00:00:00.00Z");
	time = WorldTime::ParseISO8601DateTime(sDateTime);
	SEOUL_UNITTESTING_ASSERT(time.GetMicroseconds() == 0ll);
	SEOUL_UNITTESTING_ASSERT(time.GetMicroseconds() == WorldTime().GetMicroseconds());
}

void SeoulTimeTest::TestISO8601ToString()
{
	// Got the correct epoch time from http://www.epochconverter.com

	WorldTime time;
	String s;

	String sDateTime("2014-03-19T21:32:05+01:00");
	time = WorldTime::ParseISO8601DateTime(sDateTime);
	s = time.ToISO8601DateTimeUTCString();
	SEOUL_UNITTESTING_ASSERT_EQUAL("2014-03-19T20:32:05Z", s);

	sDateTime = String("2014-01-01T06:14:05-01:20");
	time = WorldTime::ParseISO8601DateTime(sDateTime);
	s = time.ToISO8601DateTimeUTCString();
	SEOUL_UNITTESTING_ASSERT_EQUAL("2014-01-01T07:34:05Z", s);

	sDateTime = String("2013-04-28T12:56:15.004-03:50");
	time = WorldTime::ParseISO8601DateTime(sDateTime);
	s = time.ToISO8601DateTimeUTCString();
	SEOUL_UNITTESTING_ASSERT_EQUAL("2013-04-28T16:46:15Z", s);

	sDateTime = String("2013-04-28T12:56:15.123456-03:50");
	time = WorldTime::ParseISO8601DateTime(sDateTime);
	s = time.ToISO8601DateTimeUTCString();
	SEOUL_UNITTESTING_ASSERT_EQUAL("2013-04-28T16:46:15Z", s);

	sDateTime = String("2013-04-28T12:56:15.12345678-03:50");
	time = WorldTime::ParseISO8601DateTime(sDateTime);
	s = time.ToISO8601DateTimeUTCString();
	SEOUL_UNITTESTING_ASSERT_EQUAL("2013-04-28T16:46:15Z", s);

	sDateTime = String("2013-04-28T12:56:15.123-03:50");
	time = WorldTime::ParseISO8601DateTime(sDateTime);
	s = time.ToISO8601DateTimeUTCString();
	SEOUL_UNITTESTING_ASSERT_EQUAL("2013-04-28T16:46:15Z", s);

	sDateTime = String("1970-01-01T00:00:00.00+00:00");
	time = WorldTime::ParseISO8601DateTime(sDateTime);
	s = time.ToISO8601DateTimeUTCString();
	SEOUL_UNITTESTING_ASSERT_EQUAL("1970-01-01T00:00:00Z", s);
}

void SeoulTimeTest::TestGetDayStartTime()
{
	// First is in, Second is expected out
	Vector<Pair<String, String>> testCases;

	Int64 hoursOffset = 5;
	testCases.PushBack(Pair<String, String>("2018-05-18T23:59:59+00:00", "2018-05-18T05:00:00+00:00"));
	testCases.PushBack(Pair<String, String>("2018-05-19T00:00:01+00:00", "2018-05-18T05:00:00+00:00"));
	testCases.PushBack(Pair<String, String>("2018-05-19T04:59:59+00:00", "2018-05-18T05:00:00+00:00"));
	testCases.PushBack(Pair<String, String>("2018-05-19T05:00:00+00:00", "2018-05-19T05:00:00+00:00"));

	for (auto const& tc : testCases)
	{
		auto input = WorldTime::ParseISO8601DateTime(tc.First);
		auto expected = WorldTime::ParseISO8601DateTime(tc.Second);

		auto actual = input.GetDayStartTime(hoursOffset);
		SEOUL_UNITTESTING_ASSERT_EQUAL(expected, actual);
	}
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
