/**
 * \file AndroidEngine.cpp
 * \brief Specialization of Engine for the Android platform.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "AnalyticsManager.h"
#include "AndroidAchievementManager.h"
#include "AndroidCommerceManager.h"
#include "AndroidEngine.h"
#include "AndroidFacebookManager.h"
#include "AndroidInput.h"
#include "AndroidPrereqs.h"
#include "BuildChangelistPublic.h"
#include "CookManager.h"
#include "CookManagerMoriarty.h"
#include "DataStore.h"
#include "DataStoreParser.h"
#include "DiskFileSystem.h"
#include "EncryptAES.h"
#include "Engine.h"
#include "EngineCommandLineArgs.h"
#include "FileManager.h"
#include "FMODSoundManager.h"
#include "GamePaths.h"
#include "GenericAnalyticsManager.h"
#include "GenericSaveApi.h"
#include "HTTPManager.h"
#include "JobsFunction.h"
#include "LocManager.h"
#include "Logger.h"
#include "MoriartyClient.h"
#include "Mutex.h"
#include "OGLES2RenderDevice.h"
#include "Path.h"
#include "PlatformData.h"
#include "SeoulFileReaders.h"
#include "SeoulFileWriters.h"
#include "SeoulProfiler.h"
#include "SeoulUUID.h"

namespace Seoul
{

/** Size of the header block of an encrypted UUID file. */
static UInt32 const kEncryptedUUIDHeaderSizeInBytes = 10;

/** String at the head of an encrypted UUID file. */
static Byte const* kEncryptedUUIDHeaderString = "SEOUL_UDIF";

/** Total size of an encrypted UUID file. */
static UInt32 kEncryptedUUIDTotalFileSizeInBytes = 256;

/**
 * Return system uptime - monotonically increasing value,
 * not affected by deep sleep or changes to the system clock.
 */
static inline Int64 AndroidGetUptimeInMilliseconds(JNIEnv* pEnvironment, jobject clazz)
{
	Int64 const iElapsedRealtimeNanos = Java::Invoke<Int64>(
		pEnvironment,
		clazz,
		"GetElapsedRealtimeNanos",
		"()J");

	return (iElapsedRealtimeNanos / (Int64)1000000);
}

/**
 * Rudimentary check for root access. Not intended to be exhaustive,
 * just barely sufficient for analytics. Can return both false
 * positives and false negatives since it's just searching for the
 * existence of specific files, not actually checking access
 * or permissions.
 */
static inline Bool IsDeviceRooted()
{
	static Byte const* s_kaPathsToCheck[] =
	{
		"/data/local/bin/su",
		"/data/local/su",
		"/data/local/xbin/su",
		"/sbin/su",
		"/su/bin/su"
		"/system/bin/failsafe/su",
		"/system/bin/su",
		"/system/sd/xbin/su",
		"/system/xbin/su",
	};

	for (size_t i = 0u; i < SEOUL_ARRAY_COUNT(s_kaPathsToCheck); ++i)
	{
		if (DiskSyncFile::FileExists(s_kaPathsToCheck[i]))
		{
			return true;
		}
	}

	return false;
}

AndroidEngine::AndroidEngine(const AndroidEngineSettings& settings)
	: Engine()
	, m_VirtualKeyboardState()
	, m_LastBatteryLevelCheckWorldTime()
	, m_fBatteryLevel(0.0f)
	, m_LastNetworkConnectionTypeWorldTime()
	, m_eNetworkConnectionType(NetworkConnectionType::kUnknown)
	, m_Settings(settings)
	, m_pOGLES2RenderDevice()
	, m_bHasFocus(true)
{
}

AndroidEngine::~AndroidEngine()
{
}

void AndroidEngine::RefreshUptime()
{
	ScopedJavaEnvironment scope;
	JNIEnv* pEnvironment = scope.GetJNIEnv();

	// Get the new value.
	auto const iNewUptimeInMilliseconds = AndroidGetUptimeInMilliseconds(pEnvironment, m_Settings.m_pNativeActivity->clazz);

	// TODO: Max here is just for safety, since Android
	// devices tend to be unpredictable, and I'm paranoid that
	// a device will return unexpected results from
	// android.os.SystemClock.elapsedTimeNanos().
	Lock lock(*m_pUptimeMutex);
	m_iUptimeInMilliseconds = Max(
		iNewUptimeInMilliseconds,
		(Int64)m_iUptimeInMilliseconds);
}

String AndroidEngine::GetSystemLanguage() const
{
	static const String ksEnglish("English");

	struct LanguageEntry
	{
		String m_sIso3LanguageCode;
		String m_sLanguage;
	};

	// Known ISO 639-2 3-letter language codes to SeoulEngine languages.
	static const LanguageEntry kasLanguages[] =
	{
		{ "deu", "German" },
		{ "eng", "English" },
		{ "fra", "French" },
		{ "fre", "French" },
		{ "ger", "German" },
		{ "ita", "Italian" },
		{ "jpn", "Japanese" },
		{ "kor", "Korean" },
		{ "spa", "Spanish" },
		{ "por", "Portuguese" },
		{ "rus", "Russian" },
	};

	String sIso3LanguageCode;

	{
		// Attach a Java environment to the current thread.
		ScopedJavaEnvironment scope;
		JNIEnv* pEnvironment = scope.GetJNIEnv();

		// Get the language code from Java.
		sIso3LanguageCode = Java::Invoke<String>(
			pEnvironment,
			m_Settings.m_pNativeActivity->clazz,
			"GetLanguageIso3Code",
			"()Ljava/lang/String;");
	}

	// Convert the language code to a SeoulEngine language.
	for (size_t i = 0u; i < SEOUL_ARRAY_COUNT(kasLanguages); ++i)
	{
		if (kasLanguages[i].m_sIso3LanguageCode == sIso3LanguageCode)
		{
			return kasLanguages[i].m_sLanguage;
		}
	}

	// If not found, assume the default.
	return ksEnglish;
}

Bool AndroidEngine::UpdatePlatformUUID(const String& sPlatformUUID)
{
	// Don't allow an empty UUID
	if (sPlatformUUID.IsEmpty())
	{
		return false;
	}

	if (sPlatformUUID != GetPlatformUUID())
	{
		// Commit the new UUID to platform data.
		{
			Lock lock(*m_pPlatformDataMutex);
			m_pPlatformData->m_sPlatformUUID = sPlatformUUID;
		}

		// Commit the changes to disk.
		String const sUUIDFilePath(InternalGetUUIDFilePath());

		// Write the UUID.
		InternalWriteUUID(sUUIDFilePath, sPlatformUUID);
	}

	return true;
}

Bool AndroidEngine::PostNativeQuitMessage()
{
	// Attach a Java environment to the current thread.
	ScopedJavaEnvironment scope;
	JNIEnv* pEnvironment = scope.GetJNIEnv();

	Java::Invoke<void>(
		pEnvironment,
		m_Settings.m_pNativeActivity->clazz,
		"OnInvokeDefaultBackButtonHandling",
		"()V");

	return true;
}

Bool AndroidEngine::QueryBatteryLevel(Float& rfLevel)
{
	// Don't poll for battery level too frequently.
	static const Double kfBatteryLevelPollIntervalInSeconds = 15.0;

	WorldTime const currentWorldTime = WorldTime::GetUTCTime();
	if (m_LastBatteryLevelCheckWorldTime == WorldTime() ||
		(currentWorldTime - m_LastBatteryLevelCheckWorldTime).GetSeconds() > kfBatteryLevelPollIntervalInSeconds)
	{
		m_LastBatteryLevelCheckWorldTime = currentWorldTime;

		// Attach a Java environment to the current thread.
		ScopedJavaEnvironment scope;
		JNIEnv* pEnvironment = scope.GetJNIEnv();

		m_fBatteryLevel = Java::Invoke<Float>(
			pEnvironment,
			m_Settings.m_pNativeActivity->clazz,
			"QueryBatteryLevel",
			"()F");
	}

	// Negative battery level values indicate a failed query,
	// return false until it succeeds.
	if (m_fBatteryLevel < 0.0)
	{
		return false;
	}

	rfLevel = m_fBatteryLevel;
	return true;
}

Bool AndroidEngine::QueryNetworkConnectionType(NetworkConnectionType& reType)
{
	// Don't poll for network connection type too frequently.
	static const Double kfNetworkConnectionPollIntervalInSeconds = 15.0;

	WorldTime const currentWorldTime = WorldTime::GetUTCTime();
	if (m_LastNetworkConnectionTypeWorldTime == WorldTime() ||
		(currentWorldTime - m_LastNetworkConnectionTypeWorldTime).GetSeconds() > kfNetworkConnectionPollIntervalInSeconds)
	{
		m_LastNetworkConnectionTypeWorldTime = currentWorldTime;

		// Attach a Java environment to the current thread.
		ScopedJavaEnvironment scope;
		JNIEnv* pEnvironment = scope.GetJNIEnv();

		m_eNetworkConnectionType = (NetworkConnectionType)Java::Invoke<Int>(
			pEnvironment,
			m_Settings.m_pNativeActivity->clazz,
			"QueryNetworkConnectionType",
			"()I");
	}

	// Negative network connection type value indicates a failed query,
	// filter and return false.
	if ((Int)m_eNetworkConnectionType < 0)
	{
		m_eNetworkConnectionType = NetworkConnectionType::kUnknown;
		return false;
	}

	reType = m_eNetworkConnectionType;
	return true;
}

Bool AndroidEngine::QueryProcessMemoryUsage(size_t& rzWorkingSetBytes, size_t& rzPrivateBytes) const
{
	// Attach a Java environment to the current thread.
	ScopedJavaEnvironment scope;
	JNIEnv* pEnvironment = scope.GetJNIEnv();

	Int64 const iSize = Java::Invoke<Int64>(
		pEnvironment,
		m_Settings.m_pNativeActivity->clazz,
		"QueryProcessMemoryUsage",
		"()J");

	if (iSize < 0)
	{
		return false;
	}
	else
	{
		rzWorkingSetBytes = (size_t)iSize;
		rzPrivateBytes = (size_t)iSize;
		return true;
	}
}

/**
 * Shows the Google Play Store to allow the user to rate this app
 */
void AndroidEngine::ShowAppStoreToRateThisApp()
{
	static const HString ksPlayStoreNotFound("UI_RateMe_StoreNotFound");

	// Attach a Java environment to the current thread.
	ScopedJavaEnvironment scope;
	JNIEnv *pEnvironment = scope.GetJNIEnv();

	Java::Invoke<void>(
		pEnvironment,
		m_Settings.m_pNativeActivity->clazz,
		"ShowAppStoreToRateThisApp",
		"(Ljava/lang/String;)V",
		LocManager::Get()->Localize(ksPlayStoreNotFound));
}

void AndroidEngine::Initialize()
{
	SetExecutableName(m_Settings.m_sExecutableName);

	// Populate PlatformData that we can populate now, as well as some other values we cache.
	{
		ScopedJavaEnvironment scope;
		auto pEnv(scope.GetJNIEnv());
		auto pClass(m_Settings.m_pNativeActivity->clazz);

		// Fill in non-platform data members.
		auto const f = (Double)Java::Invoke<Float>(pEnv, pClass, "GetScreenRefreshRateInHz", "()F");
		m_RefreshRate.m_uNumerator = (UInt32)Floor(f * 1000.0);
		m_RefreshRate.m_uDenominator = 1000u;
		m_iStartUptimeInMilliseconds = AndroidGetUptimeInMilliseconds(pEnv, pClass);
		m_iUptimeInMilliseconds = m_iStartUptimeInMilliseconds;

		// Now fill in a  platform data structure.
		PlatformData data;
		data.m_iAppVersionCode = Java::Invoke<Int>(pEnv, pClass, "GetAppVersionCode", "()I");
		data.m_sAppVersionName = Java::Invoke<String>(pEnv, pClass, "GetAppVersionName", "()Ljava/lang/String;");
		data.m_sCountryCode = Java::Invoke<String>(pEnv, pClass, "GetCountryCode", "()Ljava/lang/String;");
		data.m_sDeviceManufacturer = Java::Invoke<String>(pEnv, pClass, "GetDeviceManufacturer", "()Ljava/lang/String;");
		data.m_sDeviceModel = Java::Invoke<String>(pEnv, pClass, "GetDeviceModel", "()Ljava/lang/String;");
		data.m_sDeviceId = Java::Invoke<String>(pEnv, pClass, "GetDeviceId", "()Ljava/lang/String;");
		data.m_sDeviceNetworkCountryCode = Java::Invoke<String>(pEnv, pClass, "GetDeviceNetworkCountryCode", "()Ljava/lang/String;");
		data.m_sDeviceNetworkOperatorName = Java::Invoke<String>(pEnv, pClass, "GetDeviceNetworkOperatorName", "()Ljava/lang/String;");
		data.m_sDevicePlatformName = "Android";
		data.m_eDevicePlatformFlavor = m_Settings.m_ePlatformFlavor;
		data.m_sDeviceSimCountryCode = Java::Invoke<String>(pEnv, pClass, "GetDeviceSimCountryCode", "()Ljava/lang/String;");
		data.m_sFacebookInstallAttribution = Java::Invoke<String>(pEnv, pClass, "GetFacebookInstallAttribution", "()Ljava/lang/String;");
		data.m_sLanguageCodeIso2 = Java::Invoke<String>(pEnv, pClass, "GetLanguageIso2Code", "()Ljava/lang/String;");
		data.m_sLanguageCodeIso3 = Java::Invoke<String>(pEnv, pClass, "GetLanguageIso3Code", "()Ljava/lang/String;");
		data.m_sOsName = Java::Invoke<String>(pEnv, pClass, "GetOsName", "()Ljava/lang/String;");
		data.m_sOsVersion = Java::Invoke<String>(pEnv, pClass, "GetOsVersion", "()Ljava/lang/String;");
		data.m_sPackageName = Java::Invoke<String>(pEnv, pClass, "GetPackageName", "()Ljava/lang/String;");
		// Populate initial UUID - may be overwriten/restored later.
		data.m_sPlatformUUID = UUID::GenerateV4().ToString();
		data.m_bRooted = IsDeviceRooted();
		data.m_vScreenPPI.X = Java::Invoke<Float>(pEnv, pClass, "GetScreenPPIX", "()F");
		data.m_vScreenPPI.Y = Java::Invoke<Float>(pEnv, pClass, "GetScreenPPIY", "()F");
		data.m_iTargetApiOrSdkVersion = Java::Invoke<Int>(pEnv, pClass, "GetBuildSDKVersion", "()I");
		data.m_iTimeZoneOffsetInSeconds = Java::Invoke<Int>(pEnv, pClass, "GetTimeZoneOffsetInSeconds", "()I");
		data.m_bImmersiveMode = Java::Invoke<Bool>(pEnv, pClass, "IsInImmersiveMode", "()Z");

		// All done, fill in the platform data.
		{
			Lock lock(*m_pPlatformDataMutex);
			*m_pPlatformData = data;
		}

		// Now issue a request for platform data that takes time.
		InternalAsyncGetTrackingInfo();
	}

	Engine::InternalPreRenderDeviceInitialization(
		m_Settings.m_CoreSettings,
		m_Settings.m_SaveLoadManagerSettings);

	InternalRestoreSavedUUID();

	Jobs::AwaitFunction(
		GetRenderThreadId(),
		SEOUL_BIND_DELEGATE(&AndroidEngine::RenderThreadInitializeOGLES2RenderDevice, this));

	Engine::InternalPostRenderDeviceInitialization();

	InternalInitializeAndroidInput();

	Engine::InternalPostInitialization();
}

void AndroidEngine::Shutdown()
{
	Engine::InternalPreShutdown(); SEOUL_TEARDOWN_TRACE();

	InternalShutdownAndroidInput(); SEOUL_TEARDOWN_TRACE();

	Engine::InternalPreRenderDeviceShutdown(); SEOUL_TEARDOWN_TRACE();

	Jobs::AwaitFunction(
		GetRenderThreadId(),
		SEOUL_BIND_DELEGATE(&AndroidEngine::RenderThreadShutdownOGLES2RenderDevice, this));
	SEOUL_TEARDOWN_TRACE();

	Engine::InternalPostRenderDeviceShutdown(); SEOUL_TEARDOWN_TRACE();

	InternalAndroidPostShutdown(); SEOUL_TEARDOWN_TRACE();
}

/**
 * Implementation of Engine::OpenURL for Android. Invokes
 * the native URL handler via a Java Intent.
 */
Bool AndroidEngine::InternalOpenURL(const String& sURL)
{
	// If no activity, cannot open URLs.
	if (!m_Settings.m_pNativeActivity.IsValid())
	{
		return false;
	}

	// Cache the native activity for the current application.
	ANativeActivity* pActivity = m_Settings.m_pNativeActivity.Get();

	// Attach a Java environment to the current thread.
	ScopedJavaEnvironment scope;
	JNIEnv* pEnvironment = scope.GetJNIEnv();

	return Java::Invoke<Bool>(
		pEnvironment,
		pActivity->clazz,
		"OpenURL",
		"(Ljava/lang/String;)Z",
		sURL);
}

Bool AndroidEngine::Tick()
{
	InternalCheckVirtualKeyboardState();

	Engine::InternalBeginTick();
	Engine::InternalEndTick();

	return true;
}

/**
 * Implementation to handle the global
 * ShowMessageBox() on Android.
 */
void AndroidEngine::ShowMessageBox(
	const String& sMessage,
	const String& sTitle,
	MessageBoxCallback onCompleteCallback,
	EMessageBoxButton eDefaultButton,
	const String& sButtonLabel1,
	const String& sButtonLabel2,
	const String& sButtonLabel3)
{
	// If no activity, cannot display a message box.
	if (!m_Settings.m_pNativeActivity.IsValid())
	{
		SEOUL_LOG("AndroidEngine::ShowMessageBox: No Activity, cannot show message box\n");
		return;
	}

	// Cache the native activity for the current application.
	ANativeActivity* pActivity = m_Settings.m_pNativeActivity.Get();

	// Attach a Java environment to the current thread.
	ScopedJavaEnvironment scope;
	JNIEnv* pEnvironment = scope.GetJNIEnv();

	MessageBoxCallback* pOnCompleteCallback = SEOUL_NEW(MemoryBudgets::TBD) MessageBoxCallback(
		onCompleteCallback);

	(void)eDefaultButton;  // Ignored on Android

	Java::Invoke<void>(
		pEnvironment,
		pActivity->clazz,
		"ShowMessageBox",
		"(Ljava/lang/String;Ljava/lang/String;JLjava/lang/String;Ljava/lang/String;Ljava/lang/String;)V",
		sMessage,
		sTitle,
		reinterpret_cast<jlong>(pOnCompleteCallback),
		sButtonLabel1,
		sButtonLabel2,
		sButtonLabel3);
}

/**
 * Schedules a local notification to be delivered to us by the OS at a later
 * time.  Not supported on all platforms.
 *
 * @param[in] iNotificationID
 *   ID of the notification.  If a pending local notification with the same ID
 *   is alreadys scheduled, that noficiation is canceled and replaced by the
 *   new one.
 *
 * @param[in] fireDate
 *   Date and time at which the local notification should fire
 *
 * @param[in] bIsWallClockTime
 *   If true, the fireDate is interpreted as a time relative to the current
 *   time in wall clock time and is adjusted appropriately if the user's time
 *   zone changes.  If false, the fireDate is interpeted as an absolute time in
 *   UTC and is not affected by time zone changes.
 *
 * @param[in] sLocalizedMessage
 *   Localized message to display when the notification fires
 *
 * @param[in] bHasActionButton
 *   If true, the notification will have an additional action button
 *
 * @param[in] sLocalizedActionButtonText
 *   If bHasActionButton is true, contains the localized text to display on
 *   the additional action button
 *
 * @param[in] sLaunchImageFilePath
 *   Ignored.
 *
 * @param[in] sSoundFilePath
 *   Ignored.
 *
 * @param[in] nIconBadgeNumber
 *   Ignored.
 *
 * @param[in] userInfo
 *   Used to uniquely identify the notification, but otherwise ignored.
 *
 * @return
 *   True if the notification was scheduled, or false if local notifications
 *   are not supported on this platform.
 */
void AndroidEngine::ScheduleLocalNotification(
	Int iNotificationID,
	const WorldTime& fireDate,
	Bool bIsWallClockTime,
	const String& sLocalizedMessage,
	Bool bHasActionButton,
	const String& sLocalizedActionButtonText,
	const String& sLaunchImageFilePath,
	const String& sSoundFilePath,
	Int nIconBadgeNumber,
	const DataStore& userInfo)
{
	SEOUL_PROF("AndroidEngine.ScheduleLocalNotification");

	SEOUL_LOG("Scheduling local notification for %s: id=%d message=%s\n", fireDate.ToLocalTimeString(true).CStr(), iNotificationID, sLocalizedMessage.CStr());

	// Attach a Java environment to the current thread.
	ScopedJavaEnvironment scope;
	JNIEnv* pEnvironment = scope.GetJNIEnv();

	// Serialize the user info to a String
	String sUserInfo;
	userInfo.ToString(userInfo.GetRootNode(), sUserInfo);

	// On Android 8.0+ we need a relative time for the JobScheduler.
	// Eventually we should move over to just one method of scheduling local notifications but
	// We still support pre-5.0, which is before the JobScheduler.
	Int64 relativeTime = fireDate.GetSeconds();
	if (!bIsWallClockTime)
	{
		WorldTime currentTime = WorldTime::GetUTCTime();
		relativeTime -= currentTime.GetSeconds();
	}

	Java::Invoke<void>(
		pEnvironment,
		m_Settings.m_pNativeActivity->clazz,
		"ScheduleLocalNotification",
		"(IJZJLjava/lang/String;ZLjava/lang/String;ZLjava/lang/String;)V",
		(jint)iNotificationID,
		(jlong)fireDate.GetSeconds(),
		bIsWallClockTime,
		(jlong)relativeTime,
		sLocalizedMessage,
		(jboolean)bHasActionButton,
		sLocalizedActionButtonText,
		(jboolean)(!sSoundFilePath.IsEmpty()),
		sUserInfo);
}

/**
 * Cancels the local notification with the given ID.  Not
 * supported on all platforms.
 */
void AndroidEngine::CancelLocalNotification(Int iNotificationID)
{
	SEOUL_LOG("Canceling local notification with ID: %d\n", iNotificationID);

	// Attach a Java environment to the current thread.
	ScopedJavaEnvironment scope;
	JNIEnv* pEnvironment = scope.GetJNIEnv();

	Java::Invoke<void>(
		pEnvironment,
		m_Settings.m_pNativeActivity->clazz,
		"CancelLocalNotification",
		"(I)V",
		(jint)iNotificationID);
}

void AndroidEngine::SetGDPRAccepted(Bool bAccepted)
{
	// Attach a Java environment to the current thread.
	ScopedJavaEnvironment scope;
	JNIEnv* pEnvironment = scope.GetJNIEnv();

	return Java::Invoke<void>(
		pEnvironment,
		m_Settings.m_pNativeActivity->clazz,
		"SetGDPRAccepted",
		"(Z)V",
		(jboolean)bAccepted);
}

Bool AndroidEngine::GetGDPRAccepted() const
{
	// Attach a Java environment to the current thread.
	ScopedJavaEnvironment scope;
	JNIEnv* pEnvironment = scope.GetJNIEnv();

	return Java::Invoke<Bool>(
		pEnvironment,
		m_Settings.m_pNativeActivity->clazz,
		"GetGDPRAccepted",
		"()Z");
}

#if SEOUL_WITH_REMOTE_NOTIFICATIONS
RemoteNotificationType AndroidEngine::GetRemoteNotificationType() const
{
	if (IsAmazonPlatformFlavor())
	{
		return RemoteNotificationType::kADM;
	}
	else
	{
		return RemoteNotificationType::kFCM;
	}
}

void AndroidEngine::RegisterForRemoteNotifications()
{
	// Attach a Java environment to the current thread.
	ScopedJavaEnvironment scope;
	JNIEnv* pEnvironment = scope.GetJNIEnv();

	Java::Invoke<void>(
		pEnvironment,
		m_Settings.m_pNativeActivity->clazz,
		"RegisterForRemoteNotifications",
		"()V");
}
#endif // /SEOUL_WITH_REMOTE_NOTIFICATIONS

/**
 * Internal text editing hook - called by the Android
 * backend to deliver text to the current ITextEditable target.
 */
void AndroidEngine::JavaToNativeTextEditableApplyText(const String& sText)
{
	if (nullptr != m_pTextEditable)
	{
		m_pTextEditable->TextEditableApplyText(sText);
	}
}

/**
 * Internal text editing hook - called by the Android
 * backend to notify the current ITextEditable target that editing
 * has ended.
 */
void AndroidEngine::JavaToNativeTextEditableStopEditing()
{
	if (nullptr != m_pTextEditable)
	{
		{
			Lock lock(m_VirtualKeyboardState.m_Mutex);
			m_VirtualKeyboardState.m_bHasVirtualKeyboard = false;
			m_VirtualKeyboardState.m_bWantsVirtualKeyboard = false;
			m_VirtualKeyboardState.m_LastChangeWorldTime = WorldTime::GetUTCTime();
		}

		m_pTextEditable->TextEditableStopEditing();
		m_pTextEditable = nullptr;
	}
}

/**
 * Internal function, issues a query to retrieve advertising
 * and tracking data.
 */
void AndroidEngine::InternalAsyncGetTrackingInfo()
{
	auto const callback(SEOUL_BIND_DELEGATE(&InternalOnReceiveTrackingInfo));
	TrackingInfoCallback* pCallback = SEOUL_NEW(MemoryBudgets::TBD) TrackingInfoCallback(callback);

	// Attach a Java environment to the current thread.
	ScopedJavaEnvironment scope;
	JNIEnv* pEnvironment = scope.GetJNIEnv();

	Java::Invoke<void>(
		pEnvironment,
		m_Settings.m_pNativeActivity->clazz,
		"AsyncGetTrackingInfo",
		"(J)V",
		reinterpret_cast<jlong>(pCallback));
}

/**
 * AndroidEngine handler for receiving tracking info.
 */
void AndroidEngine::InternalOnReceiveTrackingInfo(const AndroidTrackingInfo& trackingInfo)
{
	auto p(AndroidEngine::Get());
	if (p)
	{
		Lock lock(*p->m_pPlatformDataMutex);

		auto& r = *p->m_pPlatformData;
		r.m_sAdvertisingId = trackingInfo.m_sAdvertisingId;
		r.m_bEnableAdTracking = !trackingInfo.m_bLimitTracking;
		r.m_sUACampaign = trackingInfo.m_sCampaign;
		r.m_sUAMediaSource = trackingInfo.m_sMediaSource;
	}
}

/** Tick function, handles applying updates to the virtual keyboard state periodically. */
void AndroidEngine::InternalCheckVirtualKeyboardState()
{
	// Don't apply changes to the virtual keyboard too frequently.
	static const Double kfKeyboardChangeIntervalInSeconds = 0.5;

	// Don't allow state changes while we're applying.
	Lock lock(m_VirtualKeyboardState.m_Mutex);

	// Do work if desired state differs from current.
	if (m_VirtualKeyboardState.m_bWantsVirtualKeyboard != m_VirtualKeyboardState.m_bHasVirtualKeyboard)
	{
		// Periodic frequency, workaround the fact that we don't know exactly how the virtual keyboard
		// will behave and we don't have good (or in some cases, any) ways of querying its state.
		WorldTime const currentWorldTime = WorldTime::GetUTCTime();
		if (WorldTime() == m_VirtualKeyboardState.m_LastChangeWorldTime ||
			(currentWorldTime - m_VirtualKeyboardState.m_LastChangeWorldTime).GetSeconds() > kfKeyboardChangeIntervalInSeconds)
		{
			m_VirtualKeyboardState.m_LastChangeWorldTime = currentWorldTime;
			if (m_VirtualKeyboardState.m_bWantsVirtualKeyboard)
			{
				// Attach a Java environment to the current thread.
				ScopedJavaEnvironment scope;
				JNIEnv* pEnvironment = scope.GetJNIEnv();

				Java::Invoke<void>(
					pEnvironment,
					m_Settings.m_pNativeActivity->clazz,
					"ShowVirtualKeyboard",
					"(Ljava/lang/String;Ljava/lang/String;ILjava/lang/String;)V",
					m_VirtualKeyboardState.m_sText,
					m_VirtualKeyboardState.m_sDescription,
					m_VirtualKeyboardState.m_Constraints.m_iMaxCharacters,
					m_VirtualKeyboardState.m_Constraints.m_sRestrict);
			}
			else
			{
				// Attach a Java environment to the current thread.
				ScopedJavaEnvironment scope;
				JNIEnv* pEnvironment = scope.GetJNIEnv();

				Java::Invoke<void>(
					pEnvironment,
					m_Settings.m_pNativeActivity->clazz,
					"HideVirtualKeyboard",
					"()V");
			}
			m_VirtualKeyboardState.m_bHasVirtualKeyboard = m_VirtualKeyboardState.m_bWantsVirtualKeyboard;
		}
	}
}

void AndroidEngine::InternalStartTextEditing(ITextEditable* pTextEditable, const String& sText, const String& sDescription, const StringConstraints& constraints, Bool /*bAllowNonLatinKeyboard*/)
{
	Lock lock(m_VirtualKeyboardState.m_Mutex);
	m_VirtualKeyboardState.m_sText = sText;
	m_VirtualKeyboardState.m_sDescription = sDescription;
	m_VirtualKeyboardState.m_Constraints = constraints;
	m_VirtualKeyboardState.m_bWantsVirtualKeyboard = true;

#if !SEOUL_SHIP || defined(SEOUL_PROFILING_BUILD)
	if (m_Settings.m_bPreferHeadless)
	{
		m_VirtualKeyboardState.m_bWantsVirtualKeyboard = false;
	}
#endif // /#if !SEOUL_SHIP || defined(SEOUL_PROFILING_BUILD)
}

void AndroidEngine::InternalStopTextEditing()
{
	Lock lock(m_VirtualKeyboardState.m_Mutex);
	m_VirtualKeyboardState.m_sText = String();
	m_VirtualKeyboardState.m_sDescription = String();
	m_VirtualKeyboardState.m_Constraints = StringConstraints();
	m_VirtualKeyboardState.m_bWantsVirtualKeyboard = false;
}

void AndroidEngine::InternalAndroidPostShutdown()
{
}

void AndroidEngine::InternalInitializeAndroidInput()
{
	AndroidInputDeviceEnumerator androidEnumerator;
	InputManager::Get()->EnumerateInputDevices(&androidEnumerator);

	// Set the dead-zones for the controllers that were just created
	InputManager::Get()->UpdateDeadZonesForCurrentControllers();
}

void AndroidEngine::InternalShutdownAndroidInput()
{
}

void AndroidEngine::RenderThreadInitializeOGLES2RenderDevice()
{
	SEOUL_ASSERT(IsRenderThread());

	Bool bDesireBGRA = true;

	PlatformData data;
	GetPlatformData(data);

	// Workaround - some Samsung Galaxy devices report BGRA support, but do not actually support it.
	// https://codereview.qt-project.org/#/c/75290/3/src/quick/scenegraph/util/qsgtexture.cpp
	if (data.m_sDeviceManufacturer.CompareASCIICaseInsensitive("samsung") == 0)
	{
		if (data.m_sDeviceModel.CompareASCIICaseInsensitive("SM-T210") == 0 ||
			data.m_sDeviceModel.CompareASCIICaseInsensitive("SM-T211") == 0 ||
			data.m_sDeviceModel.CompareASCIICaseInsensitive("SM-T215") == 0)
		{
			bDesireBGRA = false;
		}
	}

	m_pOGLES2RenderDevice.Reset(SEOUL_NEW(MemoryBudgets::Game) OGLES2RenderDevice(m_Settings.m_pMainWindow, m_RefreshRate, bDesireBGRA));
}

void AndroidEngine::RenderThreadShutdownOGLES2RenderDevice()
{
	SEOUL_ASSERT(IsRenderThread());

	m_pOGLES2RenderDevice.Reset();
}

SaveApi* AndroidEngine::CreateSaveApi()
{
	return SEOUL_NEW(MemoryBudgets::Saving) GenericSaveApi;
}

CookManager* AndroidEngine::InternalCreateCookManager()
{
#if !SEOUL_SHIP
	if (!EngineCommandLineArgs::GetNoCooking())
	{
#if SEOUL_WITH_MORIARTY
		if (MoriartyClient::Get() && MoriartyClient::Get()->IsConnected())
		{
			return SEOUL_NEW(MemoryBudgets::Cooking) CookManagerMoriarty;
		}
#endif
	}
#endif

	return SEOUL_NEW(MemoryBudgets::Cooking) NullCookManager;
}

AnalyticsManager* AndroidEngine::InternalCreateAnalyticsManager()
{
	return CreateGenericAnalyticsManager(m_Settings.m_AnalyticsSettings);
}

AchievementManager *AndroidEngine::InternalCreateAchievementManager()
{
	return SEOUL_NEW(MemoryBudgets::TBD) AndroidAchievementManager();
}

CommerceManager* AndroidEngine::InternalCreateCommerceManager()
{
#if !SEOUL_SHIP || defined(SEOUL_PROFILING_BUILD)
	if (m_Settings.m_bPreferHeadless)
	{
		return Engine::InternalCreateCommerceManager();
	}
#endif // /#if !SEOUL_SHIP || defined(SEOUL_PROFILING_BUILD)

	PlatformData data;
	GetPlatformData(data);

	AndroidCommerceManagerSettings settings;
	settings.m_pNativeActivity = m_Settings.m_pNativeActivity;
	settings.m_eDevicePlatformFlavor = data.m_eDevicePlatformFlavor;
	return SEOUL_NEW(MemoryBudgets::Commerce) AndroidCommerceManager(settings);
}

FacebookManager* AndroidEngine::InternalCreateFacebookManager()
{
#if SEOUL_WITH_FACEBOOK
	return SEOUL_NEW(MemoryBudgets::TBD) AndroidFacebookManager();
#else
	return SEOUL_NEW(MemoryBudgets::TBD) NullFacebookManager();
#endif
}

PlatformSignInManager* AndroidEngine::InternalCreatePlatformSignInManager()
{
#if !SEOUL_SHIP || defined(SEOUL_PROFILING_BUILD)
	if (m_Settings.m_bPreferHeadless)
	{
		return Engine::InternalCreatePlatformSignInManager();
	}
#endif // /#if !SEOUL_SHIP || defined(SEOUL_PROFILING_BUILD)

#if SEOUL_WITH_GOOGLEPLAYGAMES
	// Only instantiate if available based on runtime check. Availability
	// of library and defined oauth2 tokens.
	if (AndroidPlatformSignInManager::IsAvailable() &&
		!m_Settings.m_SignInManagerSettings.m_sOauthClientId.IsEmpty())
	{
		return SEOUL_NEW(MemoryBudgets::TBD) AndroidPlatformSignInManager(m_Settings.m_SignInManagerSettings);
	}
#endif

	return Engine::InternalCreatePlatformSignInManager();
}

Sound::Manager* AndroidEngine::InternalCreateSoundManager()
{
#if SEOUL_WITH_FMOD
	return SEOUL_NEW(MemoryBudgets::Audio) FMODSound::Manager;
#else
	return SEOUL_NEW(MemoryBudgets::Audio) Sound::NullManager;
#endif
}

TrackingManager* AndroidEngine::InternalCreateTrackingManager()
{
	// Just use the base implementation if tracking is not enabled.
	if (!m_Settings.m_IsTrackingEnabled || !m_Settings.m_IsTrackingEnabled())
	{
		return Engine::InternalCreateTrackingManager();
	}

	return SEOUL_NEW(MemoryBudgets::Analytics) AndroidTrackingManager(m_Settings.m_TrackingSettings);
}

String AndroidEngine::InternalGetUUIDFilePath() const
{
	String sPackageName;
	{
		Lock lock(*m_pPlatformDataMutex);
		sPackageName = m_pPlatformData->m_sPackageName;
	}

	return Path::Combine(GamePaths::Get()->GetBaseDir(), sPackageName + ".save.bak");
}

void AndroidEngine::InternalRestoreSavedUUID()
{
	// Internal and external storage backup of UUID.
	auto const sUUIDFilePath(InternalGetUUIDFilePath());

	// Read the UUID.
	String sPlatformUUID;
	if (InternalReadUUID(sUUIDFilePath, sPlatformUUID))
	{
		Lock lock(*m_pPlatformDataMutex);
		m_pPlatformData->m_sPlatformUUID = sPlatformUUID;
		return;
	}

	// If we get here, consider this a first time install.
	{
		Lock lock(*m_pPlatformDataMutex);
		m_pPlatformData->m_bFirstRunAfterInstallation = true;
	}

	{
		String const sPlatformUUID(GetPlatformUUID());

		// Write the generated UUID to both the internal and external paths.
		InternalWriteUUID(sUUIDFilePath, sPlatformUUID);
	}
}

/**
 * Attempt to read a unique device identifier cached to a disk file.
 */
Bool AndroidEngine::InternalReadUUID(
	const String& sAbsoluteFilename,
	String& rsPlatformUUID) const
{
	// Open the file for read.
	ScopedPtr<SyncFile> pFile;
	if (!FileManager::Get()->OpenFile(sAbsoluteFilename, File::kRead, pFile))
	{
		return false;
	}

	// Fully populate a StreamBuffer with the contents of the file.
	StreamBuffer buffer;
	if (!buffer.Load(*pFile))
	{
		return false;
	}

	// Check for the encrypted file type - it will have
	// the encrypted file header if it is an encrypted file type.
	Byte aHeader[kEncryptedUUIDHeaderSizeInBytes];
	if (buffer.Read(aHeader, sizeof(aHeader)) &&
		0 == memcmp(aHeader, kEncryptedUUIDHeaderString, sizeof(aHeader)))
	{
		// We can't decrypt an encrypted file if no key was specified.
		if (m_Settings.m_vUUIDEncryptionKey.IsEmpty())
		{
			return false;
		}

		// Read the Nonce (number-once) for decryption.
		UInt8 aNonce[EncryptAES::kEncryptionNonceLength];
		if (!buffer.Read(aNonce, sizeof(aNonce)))
		{
			return false;
		}

		// Decrypt the data (this includes the SHA512 digest).
		EncryptAES::DecryptInPlace(
			buffer.GetBuffer() + buffer.GetOffset(),
			buffer.GetTotalDataSizeInBytes() - buffer.GetOffset(),
			m_Settings.m_vUUIDEncryptionKey.Get(0),
			m_Settings.m_vUUIDEncryptionKey.GetSize(),
			aNonce);

		// Read the digest that was stored with the file.
		UInt8 aActualDigest[EncryptAES::kSHA512DigestLength];
		memset(aActualDigest, 0, sizeof(aActualDigest));
		if (!buffer.Read(aActualDigest, sizeof(aActualDigest)))
		{
			return false;
		}

		// Generate the digest again, for comparison.
		UInt8 aExpectedDigest[EncryptAES::kSHA512DigestLength];
		memset(aExpectedDigest, 0, sizeof(aExpectedDigest));
		EncryptAES::SHA512Digest(
			(UInt8 const*)buffer.GetBuffer() + buffer.GetOffset(),
			buffer.GetTotalDataSizeInBytes() - buffer.GetOffset(),
			aExpectedDigest);

		// If the digest stored with the file matches the expected, the file
		// is considered valid.
		if (0 != memcmp(aExpectedDigest, aActualDigest, sizeof(aActualDigest)))
		{
			return false;
		}

		// Finally read the identifier and, as one last sanity check, make sure
		// we have a non-empty identifier when done.
		return buffer.Read(rsPlatformUUID) && !rsPlatformUUID.IsEmpty();
	}

	// File is unencrypted and we're not allowed to read it.
	return false;
}

/**
 * Attempt to store a unique device identifier to a disk file.
 */
Bool AndroidEngine::InternalWriteUUID(
	const String& sAbsoluteFilename,
	const String& sPlatformUUID) const
{
	// Early out if no encryption key.
	if (m_Settings.m_vUUIDEncryptionKey.IsEmpty())
	{
		return false;
	}

	// Generate the encryption number-once.
	UInt8 aNonce[EncryptAES::kEncryptionNonceLength];
	EncryptAES::InitializeNonceForEncrypt(aNonce);

	// Zero digest is written initially as a placeholder.
	UInt8 aDigest[EncryptAES::kSHA512DigestLength];
	memset(aDigest, 0, sizeof(aDigest));

	// Write the file contents and pad it to the desired length.
	StreamBuffer buffer;
	buffer.Write(kEncryptedUUIDHeaderString, kEncryptedUUIDHeaderSizeInBytes);
	buffer.Write(aNonce, sizeof(aNonce));
	buffer.Write(aDigest, sizeof(aDigest));
	buffer.Write(sPlatformUUID);
	buffer.PadTo(kEncryptedUUIDTotalFileSizeInBytes, true);

	// Seek to the head of our data and generate the SHA512 digest.
	buffer.SeekToOffset(kEncryptedUUIDHeaderSizeInBytes + sizeof(aNonce) + sizeof(aDigest));
	EncryptAES::SHA512Digest(
		(UInt8 const*)buffer.GetBuffer() + buffer.GetOffset(),
		buffer.GetTotalDataSizeInBytes() - buffer.GetOffset(),
		aDigest);

	// Now update the digest in the data we're about to encrypt.
	buffer.SeekToOffset(kEncryptedUUIDHeaderSizeInBytes + sizeof(aNonce));
	buffer.Write(aDigest, sizeof(aDigest));
	buffer.SeekToOffset(kEncryptedUUIDHeaderSizeInBytes + sizeof(aNonce));

	// Encrypt the data.
	EncryptAES::EncryptInPlace(
		buffer.GetBuffer() + buffer.GetOffset(),
		buffer.GetTotalDataSizeInBytes() - buffer.GetOffset(),
		m_Settings.m_vUUIDEncryptionKey.Get(0),
		m_Settings.m_vUUIDEncryptionKey.GetSize(),
		aNonce);

	// Create the directory structure for UUID file.
	(void)FileManager::Get()->CreateDirPath(Path::GetDirectoryName(sAbsoluteFilename));

	// Open the output file for write.
	ScopedPtr<SyncFile> pFile;
	if (!FileManager::Get()->OpenFile(sAbsoluteFilename, File::kWriteTruncate, pFile))
	{
		return false;
	}

	// Write all the data to the output file.
	buffer.SeekToOffset(0);
	return buffer.Save(*pFile);
}

/** Async update of the media source and campaign data. */
void AndroidEngine::SetAttributionData(const String& sCampaign, const String& sMediaSource)
{
	{
		// Commit the new data to platform data.
		Lock lock(*m_pPlatformDataMutex);
		m_pPlatformData->m_sUACampaign = sCampaign;
		m_pPlatformData->m_sUAMediaSource = sMediaSource;
	}
	AnalyticsManager::Get()->SetAttributionData(sCampaign, sMediaSource);
}

void AndroidShowMessageBox(
	const String& sMessage,
	const String& sTitle,
	MessageBoxCallback onCompleteCallback,
	EMessageBoxButton eDefaultButton,
	const String& sButtonLabel1,
	const String& sButtonLabel2,
	const String& sButtonLabel3)
{
	if (Engine::Get().IsValid())
	{
		AndroidEngine::Get()->ShowMessageBox(
			sMessage,
			sTitle,
			onCompleteCallback,
			eDefaultButton,
			sButtonLabel1,
			sButtonLabel2,
			sButtonLabel3);
	}
	else if (onCompleteCallback.IsValid())
	{
		onCompleteCallback(eDefaultButton);
	}
}

/**
 * Android-specific core function table
 */
static const CoreVirtuals s_AndroidCoreVirtuals =
{
	&AndroidShowMessageBox,
	&LocManager::CoreLocalize,
	&Engine::CoreGetPlatformUUID,
	&Engine::CoreGetUptime,
};

/**
 * Android-specific core function table pointer
 */
const CoreVirtuals* g_pCoreVirtuals = &s_AndroidCoreVirtuals;

} // namespace Seoul

// AndroidNativeActivity.inl is included here, instead of
// a standard .cpp implementation, becuase it is comprised
// of mostly JNI hooks which would otherwise be stripped
// by the linker.
#include "AndroidNativeActivity.inl"
