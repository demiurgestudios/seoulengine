/**
 * \file GameAnalytics.cpp
 * \brief Constants and utilities for analytics integration in Game.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "AnalyticsManager.h"
#include "CrashManager.h"
#include "DataStore.h"
#include "FacebookManager.h"
#include "FileManager.h"
#include "GameAnalytics.h"
#include "GameAuthManager.h"
#include "GameClient.h"
#include "GamePaths.h"
#include "HTTPManager.h"
#include "Path.h"
#include "ReflectionDefine.h"
#include "SeoulTime.h"
#include "ToString.h"
#include "TrackingManager.h"

namespace Seoul
{

namespace Analytics
{

static const String ksEventDiskWriteError("DiskWriteError");
static const String ksEventInstall("$ae_first_open");
static const String ksEventLaunch("Launch");
static const String ksEventPatcherOpen("PatcherOpen");
static const String ksEventPatcherClose("PatcherClose");
static const HString kEventPropertyPatcherDisplayTime("patcher_display_secs");
static const HString kEventPropertyPatcherUptime("patcher_uptime_secs");
static const HString kProfilePropertyCreated("$created");
static const HString kProfilePropertySandboxed("p_in_sandbox");
static const HString kProfilePropertyTransactions("$transactions");
static const HString kProfilePropertyAmount("$amount");
static const HString kProfilePropertyTime("$time");

static const String ksEventPropertyPatcherAuthLoginRequest("patcher_auth_login_");
static const String ksEventPropertyPatcherStateDisplayCountPrefix("patcher_display_count_");
static const String ksEventPropertyPatcherStateDisplayTimePrefix("patcher_display_secs_");
static const HString ksEventPropertyPatcherStateFileReloadCount("patcher_file_reload_count");

void OnDiskWriteError()
{
	AnalyticsEvent const evt(ksEventDiskWriteError);
	(void)AnalyticsManager::Get()->TrackEvent(evt);
}

void OnInstall()
{
	AnalyticsEvent const evt(ksEventInstall);
	(void)AnalyticsManager::Get()->TrackEvent(evt);

	// Al requested that all users appear in the "customers" page on Mixpanel,
	// so we send a $0.00 transaction append in sync with install events also.
	{
		AnalyticsProfileUpdate update(AnalyticsProfileUpdateOp::kAppend);

		AnalyticsProfileUpdate::Updates& rUpdates = update.GetUpdates();
		DataNode table = rUpdates.GetRootNode();

		// Set the "$transactions" table, which has two sub members - "$time" and "$amount".
		rUpdates.SetTableToTable(table, kProfilePropertyTransactions);
		rUpdates.GetValueFromTable(table, kProfilePropertyTransactions, table);

		auto now = Game::Client::Get()->GetCurrentServerTime();
		rUpdates.SetStringToTable(table, kProfilePropertyAmount, "0.00");
		rUpdates.SetStringToTable(table, kProfilePropertyTime, now.ToISO8601DateTimeUTCString());

		AnalyticsManager::Get()->UpdateProfile(update);
	}
}

void OnAppLaunch()
{
	AnalyticsEvent const evt(ksEventLaunch);
	(void)AnalyticsManager::Get()->TrackEvent(evt);
}

static void AppendRequestStats(
	const String& sPrefix,
	const HTTP::Stats& stats,
	DataStore& rt,
	DataNode root)
{
	rt.SetUInt32ValueToTable(root, HString(sPrefix + "resend_count"), stats.m_uResends);

	rt.SetFloat32ValueToTable(root, HString(sPrefix + "delay_secs"), (Float)stats.m_fApiDelaySecs);
	rt.SetFloat32ValueToTable(root, HString(sPrefix + "lookup_secs"), (Float)stats.m_fLookupSecs);
	rt.SetFloat32ValueToTable(root, HString(sPrefix + "connect_secs"), (Float)stats.m_fConnectSecs);
	rt.SetFloat32ValueToTable(root, HString(sPrefix + "appconnect_secs"), (Float)stats.m_fAppConnectSecs);
	rt.SetFloat32ValueToTable(root, HString(sPrefix + "pretransfer_secs"), (Float)stats.m_fPreTransferSecs);
	rt.SetFloat32ValueToTable(root, HString(sPrefix + "redirecttime_secs"), (Float)stats.m_fRedirectSecs);
	rt.SetFloat32ValueToTable(root, HString(sPrefix + "starttransfer_secs"), (Float)stats.m_fStartTransferSecs);
	rt.SetFloat32ValueToTable(root, HString(sPrefix + "totalrequest_secs"), (Float)stats.m_fTotalRequestSecs);
	rt.SetFloat32ValueToTable(root, HString(sPrefix + "overall_secs"), (Float)stats.m_fOverallSecs);

	rt.SetFloat32ValueToTable(root, HString(sPrefix + "bps_down"), (Float)stats.m_fAverageDownloadSpeedBytesPerSec);
	rt.SetFloat32ValueToTable(root, HString(sPrefix + "bps_up"), (Float)stats.m_fAverageUploadSpeedBytesPerSec);
	rt.SetUInt32ValueToTable(root, HString(sPrefix + "network_fails"), stats.m_uNetworkFailures);
	rt.SetUInt32ValueToTable(root, HString(sPrefix + "http_fails"), stats.m_uHttpFailures);
	rt.SetStringToTable(root, HString(sPrefix + "request_id"), stats.m_sRequestTraceId);
}

static void AppendDownloaderData(
	const String& sPrefix,
	const DownloadablePackageFileSystemStats& stats,
	DataStore& rt,
	DataNode root)
{
	for (auto const& pair : stats.m_tEvents)
	{
		rt.SetUInt32ValueToTable(root, HString(sPrefix + String(pair.First)), pair.Second);
	}
	for (auto const& pair : stats.m_tTimes)
	{
		rt.SetFloat32ValueToTable(root, HString(sPrefix + String(pair.First)),
			(Float)SeoulTime::ConvertTicksToSeconds(pair.Second));
	}
}

static void AppendSubStats(
	const String& sPrefix,
	const Game::PatcherDisplayStats::ApplySubStats& t,
	DataStore& rt,
	DataNode root)
{
	for (auto const& pair : t)
	{
		rt.SetUInt32ValueToTable(root, HString(sPrefix + String(pair.First) + "_count"), pair.Second.m_uCount);
		rt.SetFloat32ValueToTable(root, HString(sPrefix + String(pair.First) + "_secs"), pair.Second.m_fTimeSecs);
	}
}

static void AppendStats(
	const Game::PatcherDisplayStats& stats,
	DataStore& rt,
	DataNode root)
{
	// Report time for each patcher state to help understand where the time goes
	auto const& a = stats.m_aPerState;
	for (UInt32 i = 0u; i < a.GetSize(); ++i)
	{
		auto const& e = a[i];
		auto const sName = EnumToString<Game::PatcherState>(i);

		HString const countPropertyName(ksEventPropertyPatcherStateDisplayCountPrefix + sName);
		rt.SetUInt32ValueToTable(root, countPropertyName, e.m_uCount);
		HString const timePropertyName(ksEventPropertyPatcherStateDisplayTimePrefix + sName);
		rt.SetFloat32ValueToTable(root, timePropertyName, e.m_fTimeSecs);
	}

	// Report file reload count.
	rt.SetUInt32ValueToTable(root, ksEventPropertyPatcherStateFileReloadCount, stats.m_uReloadedFiles);

	// Report request info.
	AppendRequestStats(ksEventPropertyPatcherAuthLoginRequest, stats.m_AuthLoginRequest, rt, root);

	// Also max stats.
	{
		auto const sanitize = [](const String& s)
		{
			return "max_stat_" + TrimWhiteSpace(s).ToLowerASCII() + "_";
		};

		String sURL;
		HTTP::Stats maxStats;
		HTTP::Manager::Get()->GetMaxRequestStats(sURL, maxStats);
		auto uPos = sURL.FindLast("/");
		if (String::NPos != uPos) { sURL = sURL.Substring(uPos + 1u); }
		sURL = sanitize(sURL);
		AppendRequestStats(sURL, maxStats, rt, root);
	}

	// Patch data.
	AppendSubStats("apply_stat_", stats.m_tApplySubStats, rt, root);

	// Downloader data.
	AppendDownloaderData("additional_sar_stat_", stats.m_AdditionalStats, rt, root);
	AppendDownloaderData("config_sar_stat_", stats.m_ConfigStats, rt, root);
	AppendDownloaderData("content_sar_stat_", stats.m_ContentStats, rt, root);
}

void OnPatcherClose(
	const TimeInterval& patcherUptime,
	Float32 fPatcherDisplayTimeInSeconds,
	const Game::PatcherDisplayStats& stats)
{
	AnalyticsEvent evt(ksEventPatcherClose);

	auto& rt = evt.GetProperties();
	auto const root = rt.GetRootNode();
	rt.SetInt32ValueToTable(root, kEventPropertyPatcherDisplayTime, (Int32)Round(fPatcherDisplayTimeInSeconds));
	rt.SetInt64ValueToTable(root, kEventPropertyPatcherUptime, patcherUptime.GetSeconds());
	AppendStats(stats, rt, root);

	(void)AnalyticsManager::Get()->TrackEvent(evt);
}

void OnPatcherOpen()
{
	AnalyticsEvent const evt(ksEventPatcherOpen);
	(void)AnalyticsManager::Get()->TrackEvent(evt);
}

void UpdateCreatedAt(const WorldTime& createdAt)
{
	// Set once.
	AnalyticsProfileUpdate update(AnalyticsProfileUpdateOp::kSetOnce);
	auto& ds = update.GetUpdates();
	ds.MakeTable();
	auto const root = ds.GetRootNode();
	ds.SetStringToTable(root, kProfilePropertyCreated, createdAt.ToISO8601DateTimeUTCString());
	AnalyticsManager::Get()->UpdateProfile(update);
}

void UpdateSandboxed(Bool bSandboxed)
{
	if (AnalyticsManager::Get()->ShouldSetInSandboxProfileProperty())
	{
		// Set once.
		AnalyticsProfileUpdate update(AnalyticsProfileUpdateOp::kSet);
		auto& ds = update.GetUpdates();
		ds.MakeTable();
		auto const root = ds.GetRootNode();
		ds.SetBooleanValueToTable(root, kProfilePropertySandboxed, bSandboxed);
		AnalyticsManager::Get()->UpdateProfile(update);
	}
	else
	{
		// This property should not exist for this app
		AnalyticsProfileUpdate update(AnalyticsProfileUpdateOp::kUnset);
		auto& ds = update.GetUpdates();
		ds.MakeArray();
		auto const root = ds.GetRootNode();
		ds.SetStringToArray(root, 0u, kProfilePropertySandboxed);
		AnalyticsManager::Get()->UpdateProfile(update);
	}
}

/** Update the AnalyticsManager user ID. */
void SetAnalyticsUserID(const String& sUserID)
{
	// Note.
	auto const bSandboxed = AnalyticsManager::Get()->GetAnalyticsSandboxed();

	// Commit sandbox the first time.
	UpdateSandboxed(bSandboxed);

	// Update user ID values.
	AnalyticsManager::Get()->SetAnalyticsUserID(sUserID);
	FacebookManager::Get()->SetUserID(sUserID);
	TrackingManager::Get()->SetTrackingUserID(String(), sUserID);
}

/** Update A/B testing groups that the analytics manager will track. */
void SetAnalyticsABTests(const ABTests& tABTests)
{
	AnalyticsManager::Get()->SetABTests(tABTests);
}

/** Update sandboxing state to the analytics manager. */
void SetAnalyticsSandboxed(Bool bSandboxed)
{
	// On change, report  as a people property update as well.
	if (AnalyticsManager::Get()->GetAnalyticsSandboxed() != bSandboxed)
	{
		AnalyticsManager::Get()->SetAnalyticsSandboxed(bSandboxed);
	}
}

} // namespace Analytics

} //namespace Seoul
