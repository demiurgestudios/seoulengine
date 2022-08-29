/**
 * \file AndroidTrackingManager.h
 * \brief Specialization of TrackingManager for Android. Currently,
 * binds the AppsFlyer SDK.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ANDROID_TRACKING_MANAGER_H
#define ANDROID_TRACKING_MANAGER_H

#include "TrackingManager.h"
#include "Logger.h"
#include "JobsFunction.h"

// Forward declarations
struct ANativeActivity;

namespace Seoul
{

/** Utility, collection of configuration settings for AndroidTrackingManager. */
struct AndroidTrackingManagerSettings
{
	static Bool DefaultIsOnProd() { return false; }
	static void DefaultDeepLinkCampaignDelegate(const String& sCampaign)
	{
		SEOUL_WARN("AndroidTrackingManager: Received deep link campaign %s but no handler has been assigned.", sCampaign.CStr());
	}

	AndroidTrackingManagerSettings()
		: m_pNativeActivity(nullptr)
		, m_GetIsOnProd(SEOUL_BIND_DELEGATE(&DefaultIsOnProd))
#if SEOUL_WITH_APPS_FLYER
		, m_sAppsFlyerID()
		, m_sDeepLinkCampaignScheme()
		, m_DeepLinkCampaignDelegate(SEOUL_BIND_DELEGATE(&DefaultDeepLinkCampaignDelegate))
#endif // SEOUL_WITH_APPS_FLYER
	{
	}

	CheckedPtr<ANativeActivity> m_pNativeActivity;
	Delegate<Bool()> m_GetIsOnProd;
#if SEOUL_WITH_APPS_FLYER
	String m_sAppsFlyerID;
	String m_sDeepLinkCampaignScheme;
	Delegate<void(const String&)> m_DeepLinkCampaignDelegate;
#endif // SEOUL_WITH_APPS_FLYER
#if SEOUL_WITH_HELPSHIFT
	String m_sHelpShiftUserIDSuffix;
	String m_sHelpShiftDomain;
	String m_sHelpShiftID;
	String m_sHelpShiftKey;
#endif // /#if SEOUL_WITH_HELPSHIFT
}; // struct AndroidTrackingManagerSettings

/** Android specific implementation of user acquisition and tracking functionality. */
class AndroidTrackingManager SEOUL_SEALED : public TrackingManager
{
public:
	AndroidTrackingManager(const AndroidTrackingManagerSettings& settings);
	~AndroidTrackingManager();

	static CheckedPtr<AndroidTrackingManager> Get()
	{
		if (TrackingManager::Get() && TrackingManager::Get()->GetType() == TrackingManagerType::kAndroid)
		{
			return (AndroidTrackingManager*)TrackingManager::Get().Get();
		}
		else
		{
			return CheckedPtr<AndroidTrackingManager>();
		}
	}

	virtual TrackingManagerType GetType() const SEOUL_OVERRIDE { return TrackingManagerType::kAndroid; }

	// TrackingManager overrides
	virtual String GetExternalTrackingUserID() const SEOUL_OVERRIDE
	{
		return m_sExternalTrackingUserID;
	}

	virtual Bool OpenThirdPartyURL(const String& sURL) const SEOUL_OVERRIDE;

	virtual Bool ShowHelp() const SEOUL_OVERRIDE;

	virtual void SetTrackingUserID(const String& sUserName, const String& sUserID) SEOUL_OVERRIDE;

	virtual void TrackEvent(const String& sEventName) SEOUL_OVERRIDE;
	// /TrackingManager overrides

	void SetExternalTrackingUserID(const String& sExternalTrackingUserID)
	{
		m_sExternalTrackingUserID = sExternalTrackingUserID;
	}

	void DeepLinkCampaignDelegate(const String& sCampaign)
	{
#if SEOUL_WITH_APPS_FLYER
		Jobs::AsyncFunction(GetMainThreadId(), m_Settings.m_DeepLinkCampaignDelegate, sCampaign);
#endif // SEOUL_WITH_APPS_FLYER
	}

private:
	AndroidTrackingManagerSettings const m_Settings;
	Bool const m_bIsOnProd;
	String m_sExternalTrackingUserID;
	Bool m_bHasUserID;

	virtual void OnSessionChange(const AnalyticsSessionChangeEvent& evt) SEOUL_OVERRIDE;

	typedef Vector<String, MemoryBudgets::Analytics> UserData;
	void GetUserData(UserData& rvsCustomIssueFields, UserData& rvsMetadataFields) const;

	SEOUL_DISABLE_COPY(AndroidTrackingManager);
}; // class AndroidTrackingManager

} // namespace Seoul

#endif // include guard
