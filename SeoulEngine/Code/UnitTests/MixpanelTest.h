/**
 * \file MixpanelTest.h
 * \brief Unit test for our Mixpanel client.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef MIXPANEL_TEST_H
#define MIXPANEL_TEST_H

#include "Prereqs.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class MixpanelTest SEOUL_SEALED
{
public:
	void TestBasic();
	void TestProfile();
	void TestScript();
	void TestShutdown();
	void TestSessionFilter();
	void TestSessions();
	void TestTrackingManagerEcho();
	void TestPruningByAgeEvents();
	void TestPruningByAgeProfile();
	void TestPruningBySizeEvents();
	void TestPruningBySizeProfile();
	void TestAnalyticsDisable();
}; // class MixpanelTest

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif  // include guard
