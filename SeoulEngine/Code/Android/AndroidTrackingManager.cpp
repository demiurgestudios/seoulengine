/**
 * \file AndroidTrackingManager.cpp
 * \brief Specialization of TrackingManager for Android. Currently,
 * binds the AppsFlyer SDK.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "AnalyticsManager.h"
#include "AndroidEngine.h"
#include "AndroidPrereqs.h"
#include "AndroidTrackingManager.h"
#include "Engine.h"
#include "PlatformData.h"
#include "ToString.h"

namespace Seoul
{

AndroidTrackingManager::AndroidTrackingManager(const AndroidTrackingManagerSettings& settings)
	: m_Settings(settings)
	, m_bIsOnProd(settings.m_GetIsOnProd())
	, m_sExternalTrackingUserID()
	, m_bHasUserID(false)
{
}

AndroidTrackingManager::~AndroidTrackingManager()
{
	if (!m_bHasUserID)
	{
		return;
	}

	// Shutdown HelpShift if enabled.
#if SEOUL_WITH_HELPSHIFT
	if (!m_Settings.m_sHelpShiftKey.IsEmpty())
	{
		// Attach a Java environment to the current thread.
		ScopedJavaEnvironment scope;
		JNIEnv* pEnvironment = scope.GetJNIEnv();

		Java::Invoke<void>(
			pEnvironment,
			m_Settings.m_pNativeActivity->clazz,
			"HelpShiftShutdown",
			"()V");
	}
#endif

	// Shutdown AppsFlyer if enabled.
#if SEOUL_WITH_APPS_FLYER
	{
		// Attach a Java environment to the current thread.
		ScopedJavaEnvironment scope;
		JNIEnv* pEnvironment = scope.GetJNIEnv();

		Java::Invoke<void>(
			pEnvironment,
			m_Settings.m_pNativeActivity->clazz,
			"AppsFlyerShutdown",
			"()V");
	}
#endif // /SEOUL_WITH_APPS_FLYER
}

Bool AndroidTrackingManager::OpenThirdPartyURL(const String& sURL) const
{
	if (!m_bHasUserID)
	{
		return false;
	}

#if SEOUL_WITH_HELPSHIFT
	if (!m_Settings.m_sHelpShiftKey.IsEmpty())
	{
		if (sURL.StartsWith("helpshift://"))
		{
			// Gather properties.
			UserData vsCustomIssueFields;
			UserData vsMetadataFields;
			GetUserData(vsCustomIssueFields, vsMetadataFields);

			// Attach a Java environment to the current thread.
			ScopedJavaEnvironment scope;
			JNIEnv* pEnvironment = scope.GetJNIEnv();

			return Java::Invoke<Bool>(
				pEnvironment,
				m_Settings.m_pNativeActivity->clazz,
				"HelpShiftOpenUrl",
				"([Ljava/lang/String;[Ljava/lang/String;Ljava/lang/String;)Z",
				vsCustomIssueFields,
				vsMetadataFields,
				sURL);
		}
	}
#endif // /#if SEOUL_WITH_HELPSHIFT

	return false;
}

Bool AndroidTrackingManager::ShowHelp() const
{
	if (!m_bHasUserID)
	{
		return false;
	}

#if SEOUL_AUTO_TESTS
	if (g_bRunningAutomatedTests)
	{
		return false;
	}
#endif // /SEOUL_AUTO_TESTS

#if SEOUL_WITH_HELPSHIFT
	if (!m_Settings.m_sHelpShiftKey.IsEmpty())
	{
		// Gather properties.
		UserData vsCustomIssueFields;
		UserData vsMetadataFields;
		GetUserData(vsCustomIssueFields, vsMetadataFields);

		// Attach a Java environment to the current thread.
		ScopedJavaEnvironment scope;
		JNIEnv* pEnvironment = scope.GetJNIEnv();

		return Java::Invoke<Bool>(
			pEnvironment,
			m_Settings.m_pNativeActivity->clazz,
			"HelpShiftShowHelp",
			"([Ljava/lang/String;[Ljava/lang/String;)Z",
			vsCustomIssueFields,
			vsMetadataFields);
	}
#endif // /#if SEOUL_WITH_HELPSHIFT

	return false;
}

void AndroidTrackingManager::SetTrackingUserID(const String& sUserName, const String& sUserID)
{
	// SetTrackingUserID is a bit unique - we don't perform further
	// processin if we already have a user ID, or if the user ID
	// is invalid.
	if (sUserID.IsEmpty() || m_bHasUserID)
	{
		return;
	}

	// We've now hit the point where we have a user ID.
	m_bHasUserID = true;

	// Initialize AppsFlyer if enabled - deferred initialization
	// until we have a unique user ID.
#if SEOUL_WITH_APPS_FLYER
	{
		PlatformData data;
		Engine::Get()->GetPlatformData(data);

		Bool const bIsUpdate = (!data.m_bFirstRunAfterInstallation);
		Bool const bEnableDebugLogging = !SEOUL_SHIP;

		// Attach a Java environment to the current thread.
		ScopedJavaEnvironment scope;
		JNIEnv* pEnvironment = scope.GetJNIEnv();

		SEOUL_LOG_TRACKING("AppsFlyerInitialize(%s, %s, %s, %s)",
			sUserID.CStr(),
			m_Settings.m_sAppsFlyerID.CStr(),
			bIsUpdate ? "update" : "not-update",
			bEnableDebugLogging ? "debug" : "no-debug");

		String sChannel;
		if (Seoul::IsSamsungPlatformFlavor(data.m_eDevicePlatformFlavor))
		{
			sChannel = "Samsung";
		}
		
		Java::Invoke<void>(
			pEnvironment,
			m_Settings.m_pNativeActivity->clazz,
			"AppsFlyerInitialize",
			"(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;ZZZ)V",
			sUserID,
			m_Settings.m_sAppsFlyerID,
			m_Settings.m_sDeepLinkCampaignScheme,
			sChannel,
			bIsUpdate,
			bEnableDebugLogging,
			m_bIsOnProd);
	}
#endif // /SEOUL_WITH_APPS_FLYER

	// Initialize HelpShift if enabled - deferred initialization
	// until we have a unique user ID.
#if SEOUL_WITH_HELPSHIFT
	if (!m_Settings.m_sHelpShiftKey.IsEmpty())
	{
		// Attach a Java environment to the current thread.
		ScopedJavaEnvironment scope;
		JNIEnv* pEnvironment = scope.GetJNIEnv();

		Java::Invoke<void>(
			pEnvironment,
			m_Settings.m_pNativeActivity->clazz,
			"HelpShiftInitialize",
			"(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V",
			sUserName,
			sUserID + m_Settings.m_sHelpShiftUserIDSuffix,
			m_Settings.m_sHelpShiftKey,
			m_Settings.m_sHelpShiftDomain,
			m_Settings.m_sHelpShiftID);
	}
#endif // /#if SEOUL_WITH_HELPSHIFT
}

void AndroidTrackingManager::TrackEvent(const String& sEventName)
{
	if (!m_bHasUserID)
	{
		return;
	}

	// Report via AppsFlyer if enabled.
#if SEOUL_WITH_APPS_FLYER
	{
		// Attach a Java environment to the current thread.
		ScopedJavaEnvironment scope;
		JNIEnv* pEnvironment = scope.GetJNIEnv();

		SEOUL_LOG_TRACKING("AppsFlyerTrackEvent(%s)", sEventName.CStr());

		Java::Invoke<void>(
			pEnvironment,
			m_Settings.m_pNativeActivity->clazz,
			"AppsFlyerTrackEvent",
			"(Ljava/lang/String;Z)V",
			sEventName,
			m_bIsOnProd);
	}
#endif // /SEOUL_WITH_APPS_FLYER
}

void AndroidTrackingManager::OnSessionChange(const AnalyticsSessionChangeEvent& evt)
{
	if (!m_bHasUserID)
	{
		return;
	}

	// Report via AppsFlyer if enabled.
#if SEOUL_WITH_APPS_FLYER
	{
		// Attach a Java environment to the current thread.
		ScopedJavaEnvironment scope;
		JNIEnv* pEnvironment = scope.GetJNIEnv();

		SEOUL_LOG_TRACKING("AppsFlyerSessionChange(%s, %s, %s, %" PRIu64 ")",
			evt.m_bSessionStart ? "start" : "end",
			evt.m_SessionUUID.ToString().CStr(),
			evt.m_TimeStamp.ToISO8601DateTimeUTCString().CStr(),
			evt.m_Duration.GetMicroseconds());

		Java::Invoke<void>(
			pEnvironment,
			m_Settings.m_pNativeActivity->clazz,
			"AppsFlyerSessionChange",
			"(ZLjava/lang/String;JJZ)V",
			evt.m_bSessionStart,
			evt.m_SessionUUID.ToString(),
			evt.m_TimeStamp.GetMicroseconds(),
			evt.m_Duration.GetMicroseconds(),
			m_bIsOnProd);
	}
#endif // /SEOUL_WITH_APPS_FLYER
}

void AndroidTrackingManager::GetUserData(UserData& rvsCustomIssueFields, UserData& rvsMetadataFields) const
{
	Lock lock(m_UserDataMutex);

	UserData vsCustomIssueFields;
	for (auto i = m_UserCustomData.TableBegin(m_UserCustomData.GetRootNode()), iEnd = m_UserCustomData.TableEnd(m_UserCustomData.GetRootNode()); iEnd != i; ++i)
	{
		// TODO: Warn? Needs to be an array of 2 strings, type and then value as string.
		if (!i->Second.IsArray()) { continue; }
		UInt32 uCount = 0u;
		(void)m_UserCustomData.GetArrayCount(i->Second, uCount);
		if (2u != uCount) { continue; }

		DataNode type;
		(void)m_UserCustomData.GetValueFromArray(i->Second, 0u, type);
		String sType;
		if (!m_UserCustomData.AsString(type, sType)) { continue; }

		DataNode value;
		(void)m_UserCustomData.GetValueFromArray(i->Second, 1u, value);
		String sValue;
		if (!m_UserCustomData.AsString(value, sValue)) { continue; }

		// Append - always counts of 3.
		vsCustomIssueFields.PushBack(String(i->First));
		vsCustomIssueFields.PushBack(sType);
		vsCustomIssueFields.PushBack(sValue);
	}

	UserData vsMetadataFields;
	for (auto i = m_UserMetaData.TableBegin(m_UserMetaData.GetRootNode()), iEnd = m_UserMetaData.TableEnd(m_UserMetaData.GetRootNode()); iEnd != i; ++i)
	{
		// TODO: Warn? Expect values as strings.
		String sValue;
		if (!m_UserMetaData.AsString(i->Second, sValue)) { continue; }

		// Append - always counts of 2.
		vsMetadataFields.PushBack(String(i->First));
		vsMetadataFields.PushBack(sValue);
	}

	// Done, swap and return.
	rvsCustomIssueFields.Swap(vsCustomIssueFields);
	rvsMetadataFields.Swap(vsMetadataFields);
}

} // namespace Seoul
