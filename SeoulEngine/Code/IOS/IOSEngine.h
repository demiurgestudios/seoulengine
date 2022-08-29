/**
 * \file IOSEngine.h
 * \brief Specialization of Engine for iOS. Implements platform
 * specific methods and exposes a few system functions that
 * are iOS specific and contained.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef IOS_ENGINE_H
#define IOS_ENGINE_H

#include "CoreSettings.h"
#include "Engine.h"
#include "GenericAnalyticsManager.h"
#include "IOSTrackingManager.h"
#include "ITextEditable.h"
#include "Mutex.h"
#include "Platform.h"
#include "SaveLoadManagerSettings.h"
#include "SeoulTime.h"

// Forward declarations
#ifdef __OBJC__
@class CAEAGLLayer;
#else
typedef void CAEAGLLayer;
#endif

namespace Seoul
{

// Forward declarations
class OGLES2RenderDevice;
class IOSLeaderboardManager;
class IOSProfiles;
class IOSSession;

/**
 * Identifiers for known iOS devices. Used to set and check minimum hardware requirements.
 */
enum clas IOSHardwareId
{
	kUnknown,

	kIPad1G,
	kIPad2G,
	kIPad3G,
	kIPad4G,
	kIPadMini,
	kIPadAir,
	kIPadMini2,
	kIPadAir2,
	kIPadMini3,
	kIPadMini4,
	kIPadPro9point7inch,
	kIPadPro12point9inch,
	kIPad2017,
	kIPadPro2G,
	kIPadPro10point5inch,
	kIPad6G,
	kIPad7G,
	kIPadPro11inch3G,
	kIPadPro12point9inch3G,
	kIPadMini5,
	kIPadAir3,
	MIN_IPAD = kIPad1G,
	MAX_IPAD = kIPadAir3,

	kIPhone1G,
	kIPhone3G,
	kIPhone3GS,
	kIPhone4,
	kIPhone4S,
	kIPhone5,
	kIPhone5s,
	kIPhone6,
	kIPhone6Plus,
	kIPhone6s,
	kIPhone6sPlus,
	kIPhone7,
	kIPhone7Plus,
	kIPhoneSE,
	kIPhone8,
	kIPhone8Plus,
	kIPhoneX,
	kIPhoneXS,
	kIPhoneXSMax,
	kIPhoneXR,
	kIPhone11,
	kIPhone11Pro,
	kIPhone11ProMax,
	MIN_IPHONE = kIPhone1G,
	MAX_IPHONE = kIPhone11ProMax,

	kIPod1G,
	kIPod2G,
	kIPod3G,
	kIPod4G,
	kIPod5G,
	kIPod6G,
	kIPod7G,
	MIN_IPOD = kIPod1G,
	MAX_IPOD = kIPod7G,

	kAppleWatch38mm,
	kAppleWatch42mm,
	kAppleWatch38mmSeries1,
	kAppleWatch42mmSeries1,
	kAppleWatch38mmSeries2,
	kAppleWatch42mmSeries2,
	kAppleWatch38mmSeries3,
	kAppleWatch42mmSeries3,
	kAppleWatch40mmSeries4,
	kAppleWatch44mmSeries4,
	kAppleWatch40mmSeries5,
	kAppleWatch44mmSeries5,
	MIN_WATCH = kAppleWatch38mm,
	MAX_WATCH = kAppleWatch44mmSeries5,
};

struct IOSEngineSettings
{
	IOSEngineSettings()
		: m_pLayer(nullptr)
		, m_SaveLoadManagerSettings()
		, m_AnalyticsSettings()
		, m_CoreSettings()
		, m_eMinimumIPad(IOSHardwareId::MIN_IPAD)
		, m_eMinimumIPhone(IOSHardwareId::MIN_IPHONE)
		, m_eMinimumIPod(IOSHardwareId::MIN_IPOD)
		, m_IsTrackingEnabled()
		, m_bEnterpriseBuild(false)
		, m_bSaveDeviceIdToAppleCloud(false)
	{
	}

	CAEAGLLayer* m_pLayer;

	/** Settings for the SaveLoadManager. */
	SaveLoadManagerSettings m_SaveLoadManagerSettings;

	/** Settings for Analytics, including API key and device information. */
	GenericAnalyticsManagerSettings m_AnalyticsSettings;

	/** Settings for Tracking, including keys and configuration. */
	IOSTrackingManagerSettings m_TrackingSettings;

	/** Settings used to configure Core. */
	CoreSettings m_CoreSettings;

	/* Minimum required hardware version if running on an iPad. */
	IOSHardwareId m_eMinimumIPad;

	/* Minimum required hardware version if running on an iPhone. */
	IOSHardwareId m_eMinimumIPhone;

	/* Minimum required hardware version if running on an iPod. */
	IOSHardwareId m_eMinimumIPod;

	/** Returns true if tracking should be enabled, false if disabled, and the NullTrackingManager should be used. */
	typedef Delegate<Bool()> IsTrackingEnabledDelegate;
	IsTrackingEnabledDelegate m_IsTrackingEnabled;

	/**
	 * True if this is an enterprise signed build, which disables some features.
	 * The app code should check this with the bundle id.
	 */
	Bool m_bEnterpriseBuild;

	/**
	 * When true, enables synchronizing of the generated device id to Apple's cloud
	 * storage. This all but guarantees that two different devices will report the same
	 * device ID if the user switches between the two. This is considered deprecated
	 * behavior but can be enabled for existing applications.
	 */
	Bool m_bSaveDeviceIdToAppleCloud;
};

struct IOSEditTextSettings SEOUL_SEALED
{
	IOSEditTextSettings()
		: m_sText()
		, m_sDescription()
		, m_Constraints()
		, m_bAllowNonLatinKeyboard()
	{
	}

	String m_sText;
	String m_sDescription;
	StringConstraints m_Constraints;
	Bool m_bAllowNonLatinKeyboard;
}; // struct IOSEditTextSettings

class IOSEngine SEOUL_SEALED : public Engine
{
public:
	/**
	 * @return The global singleton instance. Will be nullptr if that instance has not
	 * yet been created.
	 */
	static CheckedPtr<IOSEngine> Get()
	{
		if (Engine::Get() && Engine::Get()->GetType() == EngineType::kIOS)
		{
			return (IOSEngine*)Engine::Get().Get();
		}
		else
		{
			return CheckedPtr<IOSEngine>();
		}
	}

	IOSEngine(const IOSEngineSettings& settings);
	virtual ~IOSEngine();

	const IOSEngineSettings& GetSettings() const { return m_Settings; }

	virtual EngineType GetType() const SEOUL_OVERRIDE { return EngineType::kIOS; }

	// Manual refresh of Uptime.
	virtual void RefreshUptime() SEOUL_OVERRIDE SEOUL_SEALED;

	// Return the hardware name associated with the current device.
	static String GetHardwareName();

	// Return the hardware id associated with the current device.
	static IOSHardwareId GetHardwareId();

	virtual Bool QueryBatteryLevel(Float& rfBatteryLevel) SEOUL_OVERRIDE;

	virtual Bool QueryProcessMemoryUsage(size_t& rzWorkingSetBytes, size_t& rzPrivateBytes) const SEOUL_OVERRIDE;

	virtual void ShowAppStoreToRateThisApp() SEOUL_OVERRIDE;

	virtual Bool DoesSupportInAppRateMe() const SEOUL_OVERRIDE;

	virtual Bool MeetsMinimumHardwareRequirements() const SEOUL_OVERRIDE;

	virtual void Initialize() SEOUL_OVERRIDE;
	virtual void Shutdown() SEOUL_OVERRIDE;

	/**
	 * @return Whether the current application has focus or not. An application
	 * loses focus when the user clicks on another application, other than
	 * the current Seoul engine app.
	 */
	virtual Bool HasFocus() const SEOUL_OVERRIDE
	{
		return m_bHasFocus;
	}

	/**
	 * Update whether the game currently has focus or not.
	 */
	void SetHasFocus(Bool bHasFocus)
	{
		m_bHasFocus = bHasFocus;
	}

	virtual Bool Tick() SEOUL_OVERRIDE;

	virtual SaveApi* CreateSaveApi() SEOUL_OVERRIDE;

	virtual String GetSystemLanguage() const SEOUL_OVERRIDE;

	virtual Bool UpdatePlatformUUID(const String& sPlatformUUID) SEOUL_OVERRIDE;

	virtual void ScheduleLocalNotification(
		Int iNotificationID,
		const WorldTime& fireDate,
		Bool bIsWallClockTime,
		const String& sLocalizedMessage,
		Bool bHasActionButton,
		const String& sLocalizedActionButtonText,
		const String& sLaunchImageFilePath,
		const String& sSoundFilePath,
		Int nIconBadgeNumber,
		const DataStore& userInfo) SEOUL_OVERRIDE;

	virtual void CancelAllLocalNotifications() SEOUL_OVERRIDE;

	virtual void CancelLocalNotification(Int iNotificationID) SEOUL_OVERRIDE;

	// GDPR Compliance
    virtual void SetGDPRAccepted(Bool bAccepted) SEOUL_OVERRIDE;
    virtual Bool GetGDPRAccepted() const SEOUL_OVERRIDE;

	// Initially false
	virtual Bool CanRequestRemoteNotificationsWithoutPrompt() SEOUL_OVERRIDE;
	// Mark this device as having shown the "request remote notifications" dialog in UserDefaults
	virtual void SetCanRequestRemoteNotificationsWithoutPrompt(Bool) SEOUL_OVERRIDE;

#if SEOUL_WITH_REMOTE_NOTIFICATIONS
	virtual void RegisterForRemoteNotifications() SEOUL_OVERRIDE;

	// Called from the app delegate after registering for remote notifications
	void OnRegisteredForRemoteNotifications(Bool bSuccess, const String& sDeviceToken);

	// Called from the app delegate after getting (or not getting) permission from the user to display remote notifications
	void OnRegisteredUserNotificationSettings(Bool bAllowNotifications);

	virtual Bool SupportsRemoteNotifications() const SEOUL_OVERRIDE { return true; }

	virtual Bool HasEnabledRemoteNotifications() const SEOUL_OVERRIDE
	{
		return m_bEnabledRemoteNotifications;
	}

	virtual RemoteNotificationType GetRemoteNotificationType() const SEOUL_OVERRIDE
	{
		return RemoteNotificationType::kIOS;
	}

	virtual Bool IsRemoteNotificationDevelopmentEnvironment() const SEOUL_OVERRIDE;
#endif // /SEOUL_WITH_REMOTE_NOTIFICATIONS

	/** @return The most recently set edit text configuration/settings. */
	const IOSEditTextSettings& GetEditTextSettings() const
	{
		return m_EditTextSettings;
	}

	/** Async update of the media source and campaign data. */
	void SetAttributionData(const String& sCampaign, const String& sMediaSource);

protected:
	virtual Bool InternalOpenURL(const String& sURL) SEOUL_OVERRIDE;

	// Shows/hides the virtual keyboard
	virtual void InternalStartTextEditing(ITextEditable* pTextEditable, const String& sText, const String& sDescription, const StringConstraints& constraints, Bool allowNonLatinKeyboard) SEOUL_OVERRIDE;
	virtual void InternalStopTextEditing() SEOUL_OVERRIDE;

private:
	virtual AnalyticsManager* InternalCreateAnalyticsManager() SEOUL_OVERRIDE;
	virtual CommerceManager* InternalCreateCommerceManager() SEOUL_OVERRIDE;
	virtual FacebookManager* InternalCreateFacebookManager() SEOUL_OVERRIDE;
	virtual PlatformSignInManager* InternalCreatePlatformSignInManager() SEOUL_OVERRIDE;
	virtual Sound::Manager* InternalCreateSoundManager() SEOUL_OVERRIDE;
	virtual TrackingManager* InternalCreateTrackingManager() SEOUL_OVERRIDE;

	void InternalPopulatePlatformData();
	void InternalRegisterUserNotificationSettings();

	void InternalEnableBatteryMonitoring();
	void InternalDisableBatteryMonitoring();

	void InternalGenerateOrRestoreUniqueUserID();
	Bool InternalGetScreenPPI(Vector2D& rv) const;

	void RenderThreadInitializeOGLES2RenderDevice();
	void RenderThreadShutdownOGLES2RenderDevice();

	void InternalInitializeIOSInput();
	void InternalShutdownIOSInput();

	void InternalIOSPostShutdown();

	void InternalSetIsFirstRun();

	void InternalCancelLocalNotification(Int iNotificationID);

	Mutex m_Mutex;
	ScopedPtr<OGLES2RenderDevice> m_pOGLES2RenderDevice;
	WorldTime m_LastBatteryLevelCheckWorldTime;
	Float m_fBatteryLevel;
	IOSEngineSettings m_Settings;
	IOSEditTextSettings m_EditTextSettings;
	Bool m_bHasFocus;
	Bool m_bMeetsMinimumHardwareRequirements;
	Bool m_bIsSandboxEnvironment;
#if SEOUL_WITH_REMOTE_NOTIFICATIONS
	Bool m_bEnabledRemoteNotifications;
#endif // /SEOUL_WITH_REMOTE_NOTIFICATIONS

	SEOUL_DISABLE_COPY(IOSEngine);
}; // class IOSEngine

} // namespace Seoul

#endif // include guard
