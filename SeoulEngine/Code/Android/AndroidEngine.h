/**
 * \file AndroidEngine.h
 * \brief Specialization of Engine for the Android platform.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ANDROID_ENGINE_H
#define ANDROID_ENGINE_H

#include "AndroidPlatformSignInManager.h"
#include "AndroidTrackingManager.h"
#include "CoreSettings.h"
#include "Engine.h"
#include "GenericAnalyticsManager.h"
#include "ITextEditable.h"
#include "Mutex.h"
#include "Platform.h"
#include "PlatformFlavor.h"
#include "RenderDevice.h"
#include "SaveLoadManagerSettings.h"
#include "SeoulTime.h"
#include "SeoulUtil.h"
#include "Vector2D.h"
namespace Seoul { struct AndroidTrackingInfo; }

// Forward declarations
struct ANativeActivity;
struct ANativeWindow;

namespace Seoul
{

// Forward declarations
class OGLES2RenderDevice;

struct AndroidEngineSettings
{
	AndroidEngineSettings()
		: m_pMainWindow(nullptr)
		, m_SaveLoadManagerSettings()
		, m_AnalyticsSettings()
		, m_TrackingSettings()
		, m_CoreSettings()
		, m_sExecutableName()
		, m_vUUIDEncryptionKey()
		, m_pNativeActivity(nullptr)
		, m_ePlatformFlavor(PlatformFlavor::kUnknown)
		, m_IsTrackingEnabled()
#if !SEOUL_SHIP || defined(SEOUL_PROFILING_BUILD)
		, m_bPreferHeadless(false)
#endif // /#if !SEOUL_SHIP
	{
	}

	ANativeWindow* m_pMainWindow;
	SaveLoadManagerSettings m_SaveLoadManagerSettings;
	GenericAnalyticsManagerSettings m_AnalyticsSettings;
	AndroidTrackingManagerSettings m_TrackingSettings;
	CoreSettings m_CoreSettings;
	String m_sExecutableName;
	Vector<UInt8> m_vUUIDEncryptionKey;
	CheckedPtr<ANativeActivity> m_pNativeActivity;
	PlatformFlavor m_ePlatformFlavor;
	typedef Delegate<Bool()> IsTrackingEnabledDelegate;
	IsTrackingEnabledDelegate m_IsTrackingEnabled;
#if SEOUL_WITH_GOOGLEPLAYGAMES
	AndroidPlatformSignInManagerSettings m_SignInManagerSettings;
#endif // /#if SEOUL_WITH_GOOGLEPLAYGAMES

	// Developer only flag, used to disable systems that
	// will cause a switch into the foreground (e.g. commerce
	// or platform authentication). Used for performance testing.
#if !SEOUL_SHIP || defined(SEOUL_PROFILING_BUILD)
	Bool m_bPreferHeadless;
#endif
};

/** Utility, encapsulates advertiser/user tracking info on Android. */
struct AndroidTrackingInfo SEOUL_SEALED
{
	AndroidTrackingInfo()
		: m_sCampaign()
		, m_sMediaSource()
		, m_sAdvertisingId()
		, m_bLimitTracking(true)
	{
	}

	String m_sCampaign;
	String m_sMediaSource;
	String m_sAdvertisingId;
	Bool m_bLimitTracking;
}; // struct AndroidTrackingInfo

class AndroidEngine SEOUL_SEALED : public Engine
{
public:
	AndroidEngine(const AndroidEngineSettings& settings);
	virtual ~AndroidEngine();

	virtual EngineType GetType() const SEOUL_OVERRIDE { return EngineType::kAndroid; }

	// Issue an async query to retrieve advertiser/user tracking info - callback is
	// guaranteed to be invoked at some point.
	typedef Delegate<void(const AndroidTrackingInfo& trackingInfo)> TrackingInfoCallback;

	/** @return True if the current platform has default/native back button handling. */
	virtual Bool HasNativeBackButtonHandling() const SEOUL_OVERRIDE
	{
		return true;
	}

	// Manual refresh of Uptime.
	virtual void RefreshUptime() SEOUL_OVERRIDE SEOUL_SEALED;

	/**
	 * Tells the platform to trigger native back button handling:
	 * - Android - this exist the Activity, switching to the previously active activity.
	 */
	virtual Bool PostNativeQuitMessage() SEOUL_OVERRIDE;

	virtual Bool QueryBatteryLevel(Float& rfLevel) SEOUL_OVERRIDE;

	virtual Bool QueryNetworkConnectionType(NetworkConnectionType& reType) SEOUL_OVERRIDE;

	virtual Bool QueryProcessMemoryUsage(size_t& rzWorkingSetBytes, size_t& rzPrivateBytes) const SEOUL_OVERRIDE;

	virtual void ShowAppStoreToRateThisApp() SEOUL_OVERRIDE;

	static CheckedPtr<AndroidEngine> Get()
	{
		if (Engine::Get() && Engine::Get()->GetType() == EngineType::kAndroid)
		{
			return (AndroidEngine*)Engine::Get().Get();
		}
		else
		{
			return CheckedPtr<AndroidEngine>();
		}
	}

	virtual void Initialize() SEOUL_OVERRIDE;
	virtual void Shutdown() SEOUL_OVERRIDE;

	/**
	 * Gets the native activity pointer
	 */
	CheckedPtr<ANativeActivity> GetActivity()
	{
		return m_Settings.m_pNativeActivity;
	}

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
	 * Update whether the game currently has focus or not - typically, this should
	 * only be called by the global specialization of pp::Instance for the game.
	 */
	void SetHasFocus(Bool bHasFocus)
	{
		m_bHasFocus = bHasFocus;
	}

	virtual Bool Tick() SEOUL_OVERRIDE;

	virtual SaveApi* CreateSaveApi() SEOUL_OVERRIDE;

	virtual String GetSystemLanguage() const SEOUL_OVERRIDE;

	virtual Bool UpdatePlatformUUID(const String& sPlatformUUID) SEOUL_OVERRIDE;

	void ShowMessageBox(
		const String& sMessage,
		const String& sTitle,
		MessageBoxCallback onCompleteCallback,
		EMessageBoxButton eDefaultButton,
		const String& sButtonLabel1,
		const String& sButtonLabel2,
		const String& sButtonLabel3);

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

	virtual void CancelLocalNotification(Int iNotificationID) SEOUL_OVERRIDE;

	virtual void SetGDPRAccepted(Bool bAccepted) SEOUL_OVERRIDE;
	virtual Bool GetGDPRAccepted() const SEOUL_OVERRIDE;

#if SEOUL_WITH_REMOTE_NOTIFICATIONS
	virtual RemoteNotificationType GetRemoteNotificationType() const SEOUL_OVERRIDE;

	virtual void RegisterForRemoteNotifications() SEOUL_OVERRIDE;

	virtual Bool SupportsRemoteNotifications() const SEOUL_OVERRIDE { return true; }

	virtual Bool HasEnabledRemoteNotifications() const SEOUL_OVERRIDE { return true; }
#endif // /SEOUL_WITH_REMOTE_NOTIFICATIONS

	// Internal text editing hook - called by the Android
	// backend to deliver text to the current ITextEditable target.
	void JavaToNativeTextEditableApplyText(const String& sText);

	// Internal text editing hook - called by the Android
	// backend to notify the current ITextEditable target that editing
	// has ended.
	void JavaToNativeTextEditableStopEditing();

	/** @return Settings used to configure AndroidEngine. */
	const AndroidEngineSettings& GetSettings() const
	{
		return m_Settings;
	}

	/** Async update of the media source and campaign data. */
	void SetAttributionData(const String& sCampaign, const String& sMediaSource);

private:
	virtual Bool InternalOpenURL(const String& sURL) SEOUL_OVERRIDE;
	void InternalAsyncGetTrackingInfo();
	static void InternalOnReceiveTrackingInfo(const AndroidTrackingInfo& trackingInfo);
	void InternalCheckVirtualKeyboardState();
	virtual void InternalStartTextEditing(ITextEditable* pTextEditable, const String& sText, const String& sDescription, const StringConstraints& constraints, Bool bAllowNonLatinKeyboard) SEOUL_OVERRIDE;
	virtual void InternalStopTextEditing() SEOUL_OVERRIDE;

	void RenderThreadInitializeOGLES2RenderDevice();
	void RenderThreadShutdownOGLES2RenderDevice();

	void InternalInitializeAndroidInput();
	void InternalShutdownAndroidInput();

	void InternalAndroidPostShutdown();

	virtual CookManager* InternalCreateCookManager() SEOUL_OVERRIDE;
	virtual AnalyticsManager* InternalCreateAnalyticsManager() SEOUL_OVERRIDE;
	virtual AchievementManager* InternalCreateAchievementManager() SEOUL_OVERRIDE;
	virtual CommerceManager* InternalCreateCommerceManager() SEOUL_OVERRIDE;
	virtual FacebookManager* InternalCreateFacebookManager() SEOUL_OVERRIDE;
	virtual PlatformSignInManager* InternalCreatePlatformSignInManager() SEOUL_OVERRIDE;
	virtual Sound::Manager* InternalCreateSoundManager() SEOUL_OVERRIDE;
	virtual TrackingManager* InternalCreateTrackingManager() SEOUL_OVERRIDE;

	String InternalGetUUIDFilePath() const;
	void InternalRestoreSavedUUID();
	Bool InternalReadUUID(const String& sAbsoluteFilename, String& rsPlatformUUID) const;
	Bool InternalWriteUUID(const String& sAbsoluteFilename, const String& sPlatformUUID) const;

	struct VirtualKeyboardState
	{
		VirtualKeyboardState()
			: m_Mutex()
			, m_LastChangeWorldTime()
			, m_sText()
			, m_sDescription()
			, m_Constraints()
			, m_bWantsVirtualKeyboard(false)
			, m_bHasVirtualKeyboard(false)
		{
		}

		Mutex m_Mutex;
		WorldTime m_LastChangeWorldTime;
		String m_sText;
		String m_sDescription;
		StringConstraints m_Constraints;
		Bool m_bWantsVirtualKeyboard;
		Bool m_bHasVirtualKeyboard;
	}; // struct VirtualKeyboardState

	VirtualKeyboardState m_VirtualKeyboardState;
	WorldTime m_LastBatteryLevelCheckWorldTime;
	Float m_fBatteryLevel;
	WorldTime m_LastNetworkConnectionTypeWorldTime;
	NetworkConnectionType m_eNetworkConnectionType;
	AndroidEngineSettings m_Settings;
	ScopedPtr<OGLES2RenderDevice> m_pOGLES2RenderDevice;
	RefreshRate m_RefreshRate;
	Bool m_bHasFocus;

	SEOUL_DISABLE_COPY(AndroidEngine);
}; // class AndroidEngine

} // namespace Seoul

#endif // include guard
