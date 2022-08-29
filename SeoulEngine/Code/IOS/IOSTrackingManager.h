/**
 * \file IOSTrackingManager.h
 * \brief Specialization of TrackingManager for IOS. Currently, integrates
 * the AppsFlyer SDK.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef IOS_TRACKING_MANAGER_H
#define IOS_TRACKING_MANAGER_H

#include "TrackingManager.h"
#include "Logger.h"

#if SEOUL_WITH_APPS_FLYER
// Forward declarations
#ifdef __OBJC__
@class AppsFlyerTracker;
#else
typedef void AppsFlyerTracker;
#endif
#endif // /#if SEOUL_WITH_APPS_FLYER

namespace Seoul
{

/** Utility, collection of configuration settings for IOSTrackingManager. */
struct IOSTrackingManagerSettings
{
	static Bool DefaultIsOnProd() { return false; }
	static void DefaultDeepLinkCampaignDelegate(const String& sCampaignName)
	{
		SEOUL_WARN("IOSTrackingManager: Received Deep Link Campaign %s but no handler has been assigned.", sCampaignName.CStr());
	}

	IOSTrackingManagerSettings()
		: m_sAppleID()
		, m_GetIsOnProd(SEOUL_BIND_DELEGATE(&DefaultIsOnProd))
#if SEOUL_WITH_APPS_FLYER
		, m_sAppsFlyerID()
		, m_sDeepLinkCampaignScheme()
		, m_DeepLinkCampaignDelegate(SEOUL_BIND_DELEGATE(&DefaultDeepLinkCampaignDelegate))
#endif // #if SEOUL_WITH_APPS_FLYER
	{
	}

	String m_sAppleID;
	Delegate<Bool()> m_GetIsOnProd;
#if SEOUL_WITH_APPS_FLYER
	String m_sAppsFlyerID;
	String m_sDeepLinkCampaignScheme;
	Delegate<void(const String&)> m_DeepLinkCampaignDelegate;
#endif // #if SEOUL_WITH_APPS_FLYER
#if SEOUL_WITH_HELPSHIFT
	String m_sHelpShiftUserIDSuffix;
	String m_sHelpShiftDomain;
	String m_sHelpShiftID;
	String m_sHelpShiftKey;
#endif // /#if SEOUL_WITH_HELPSHIFT
}; // struct IOSTrackingManagerSettings

/** Android specific implementation of user acquisition and tracking functionality. */
class IOSTrackingManager SEOUL_SEALED : public TrackingManager
{
public:
	IOSTrackingManager(const IOSTrackingManagerSettings& settings);
	~IOSTrackingManager();

	static CheckedPtr<IOSTrackingManager> Get()
	{
		if (TrackingManager::Get() && TrackingManager::Get()->GetType() == TrackingManagerType::kIOS)
		{
			return (IOSTrackingManager*)TrackingManager::Get().Get();
		}
		else
		{
			return CheckedPtr<IOSTrackingManager>();
		}
	}

	virtual TrackingManagerType GetType() const SEOUL_OVERRIDE { return TrackingManagerType::kIOS; }

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

	// IOS specific hooks.
	void SetRootViewControler(void* pRootViewController)
	{
		m_pRootViewController = pRootViewController;
	}
	
#if SEOUL_WITH_APPS_FLYER
	Bool IsAppsFlyerInitialized() const;
	
	void DelayedAppsFlyerSetDelegate();
	
	String GetDeepLinkScheme() const { return m_Settings.m_sDeepLinkCampaignScheme; }
	
	Delegate<void(const String&)> GetDeepLinkDelegate() const { return m_Settings.m_DeepLinkCampaignDelegate; }
	
	// Called from the AppDelegate to handle opening a deep link URL,
	// if AppsFlyer is enabled
	Bool HandleOpenURL(
		void* pUrl,
		void* pSourceApplication,
		void* pInOptions,
		void* pAnnotation) const;
		
	// Called from the AppDelegate to handle opening a Universal Link
	Bool HandleContinueUserActivity(void* pUserActivity, void* pRestorationHandler) const;
#endif // SEOUL_WITH_APPS_FLYER

private:
	IOSTrackingManagerSettings const m_Settings;
	Bool const m_bIsOnProd;
	String m_sExternalTrackingUserID;
#if SEOUL_WITH_APPS_FLYER
	AppsFlyerTracker* m_pAppsFlyerTracker;
#endif // /#if SEOUL_WITH_APPS_FLYER
	void* m_pRootViewController = nullptr;
	String m_sUniqueUserId;
	Bool m_bHasUserID;

	virtual void OnSessionChange(const AnalyticsSessionChangeEvent& evt) SEOUL_OVERRIDE;

#if SEOUL_WITH_HELPSHIFT
	// Helper method to wrap the HelpshiftAPIConfig creation and issue HelpshiftSupport calls.
	Bool ShowHelpshiftFAQ(String sFAQ = String()) const;
#endif

	SEOUL_DISABLE_COPY(IOSTrackingManager);
}; // class IOSTrackingManager

} // namespace Seoul

#endif // include guard
