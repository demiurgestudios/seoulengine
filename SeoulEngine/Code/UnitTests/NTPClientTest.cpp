/**
 * \file NTPClientTest.cpp
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Logger.h"
#include "ReflectionDefine.h"
#include "NTPClient.h"
#include "NTPClientTest.h"
#include "ScopedAction.h"
#include "SeoulSocket.h"
#include "UnitTesting.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(NTPClientTest)
	SEOUL_ATTRIBUTE(UnitTest)
	// Disabled on mobile - TODO: Better way? We want to disable this test when we can't trust
	// the client time, which is true for DeviceFarm builds, not necessarily all mobile devices
	// in general.
#if !SEOUL_PLATFORM_ANDROID && !SEOUL_PLATFORM_IOS
	SEOUL_METHOD(TestBasic)
#endif // /(!SEOUL_PLATFORM_ANDROID && !SEOUL_PLATFORM_IOS)
SEOUL_END_TYPE()

void NTPClientTest::TestBasic()
{
	// We need to manually initialize socket support in the unit test.
	auto const action(MakeScopedAction(&Socket::StaticInitialize, &Socket::StaticShutdown));
	NTPClientSettings settings;
	settings.m_sHostname = "pool.ntp.org";
	NTPClient client(settings);

	auto const startTime = SeoulTime::GetGameTimeInTicks();
	while (SeoulTime::ConvertTicksToSeconds(SeoulTime::GetGameTimeInTicks() - startTime) < 10.0)
	{
		WorldTime ntpTime;
		if (client.SyncQueryTime(ntpTime))
		{
			auto const now = WorldTime::GetUTCTime();

			// Give some wiggle, but in general, we assume our test machine clock is
			// in the ballpack of the NTP server.
			auto const fDelta = (now - ntpTime).GetSecondsAsDouble();
			SEOUL_UNITTESTING_ASSERT_LESS_EQUAL(fDelta, 15.0);
			return;
		}
	}

	// Don't treat a communication failure with the NTP service as a unit test failure,
	// since we can't control its availability.
	SEOUL_LOG("NTPClientTest timed out waiting for NTP service to response, skipping test.");
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
