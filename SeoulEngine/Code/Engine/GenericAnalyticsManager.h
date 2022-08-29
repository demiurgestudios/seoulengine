/**
 * \file GenericAnalyticsManager.h
 * \brief Shared data between Generic (HTTP based)
 * analytics managers.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef GENERIC_ANALYTICS_MANAGER_H
#define GENERIC_ANALYTICS_MANAGER_H

#include "Delegate.h"
#include "SeoulString.h"
#include "SeoulTime.h"
namespace Seoul { struct PlatformData; }

namespace Seoul
{

/** Concrete generic analytics manager types. */
enum class GenericAnalyticsManagerType
{
	kNone,
	kMixpanel
};

/**
 * Settings used to configure the behavior of a GenericAnalyticsManager.
 */
struct GenericAnalyticsManagerSettings
{
	GenericAnalyticsManagerSettings()
		: m_fHeartbeatTimeInSeconds(60.0)
		, m_eType(GenericAnalyticsManagerType::kNone)
		, m_GetApiKeyDelegate()
		, m_ShouldSendAnalyticsDelegate()
		, m_CustomCurrentTimeDelegate()
	{
	}

	/** How often analytics are sent if not explicitly flushed. */
	Double m_fHeartbeatTimeInSeconds;

	/** Type of generic analytics manager to use. */
	GenericAnalyticsManagerType m_eType;

	/**
	 * API key - required to associate events with an account.
	 *
	 * Return an empty key to disable analytics. May be called multiple
	 * times, so should not be prohibitively costly to call and must
	 * always return the same value.
	 */
	typedef Delegate<String()> GetApiKeyDelegate;
	GetApiKeyDelegate m_GetApiKeyDelegate;

	/**
	 * Separate query for disabling analytics.
	 *
	 * Return true to enable analytics, false otherwise. Called once
	 * prior to creating the analytics manager. May be called multiple
	 * times, so should not be prohibitively costly to call and must
	 * always return the same value.
	 */
	typedef Delegate<Bool()> ShouldSendAnalyticsDelegate;
	ShouldSendAnalyticsDelegate m_ShouldSendAnalyticsDelegate;

	/** Optional - if defined, can override the base URL used by the analytics manager. Meant for unit testing. */
	typedef Delegate<String()> GetBaseURL;
	GetBaseURL m_GetBaseEventURL;
	GetBaseURL m_GetBaseProfileURL;

	/** Allows the timestamp time used by the analytics manager to be customized (e.g. if the current app has server time). */
	typedef Delegate<WorldTime()> CustomCurrentTimeDelegate;
	CustomCurrentTimeDelegate m_CustomCurrentTimeDelegate;

	/** When true, session start/stop is automatically tracked. */
	Bool m_bTrackSessions = true;

	/* TODO: Make a dictionary of Profile Properties and Event Properties to toggle on and off. */

	/* When true, set the event property "in_sandbox". When false, do not set the event property at all. */
	Bool m_bSetEventPropertyInSandbox = true;

	/** Track whether push notifications are enabled or not. */
	Bool m_bReportPushNotificationStatus = true;

	/** True to report the build major version with the app version property, otherwise, report only the changelist. */
	Bool m_bReportBuildVersionMajorWithAppVersion = true;

	/** True to report the "App Session" event. */
	Bool m_bReportAppSession = true;

	/** Prefix to prefix to all properties (except for some system built-ins - e.g. distinct_id in the Mixpanel backend. */
	String m_sPropertyPrefix = "s_";

	/** Filename to use for storing persistent analytics state. */
	String m_sSaveFilename = "player-analytics.dat";

	/** OS prefix - prefix to apply to the OS version information reported to analytics. */
	String m_sOsPrefix{};

	/** True to set the people property "p_in_sandbox". When false, do not set this people property. */
	Bool m_bShouldSetInSandboxProfileProperty = true;

	/** Allows override of the value reported as part of the OS version. Must be safe to call from any thread in any context. */
	typedef Delegate<String(const PlatformData& data)> OsVersionDelegate;
	OsVersionDelegate m_OsVersionDelegate;
}; // struct GenericAnalyticsManagerSettings

class AnalyticsManager;
AnalyticsManager* CreateGenericAnalyticsManager(const GenericAnalyticsManagerSettings& settings);

} // namespace Seoul

#endif // include guard
