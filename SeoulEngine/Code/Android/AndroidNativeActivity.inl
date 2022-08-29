/**
 * \file AndroidNativeActivity.inl
 * \brief Native integration of equivalent functionality
 * in AndroidNativeActivity.java in the AndroidJava project.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "AndroidGlobals.h"
#include "AndroidMainThreadQueue.h"
#include "AndroidPrereqs.h"
#include "AndroidTrackingManager.h"
#include "BuildChangelistPublic.h"
#include "EngineVirtuals.h"
#include "PlatformFlavor.h"
#include "SeoulUtil.h"
namespace Seoul { namespace Path { void AndroidSetCacheDir(const String& s); } }

namespace Seoul
{

/** If true, native code can continue with startup. */
Atomic32Value<Bool> g_bCanPerformNativeStartup(false);

/** Tracking counter for system vertical syncs. */
void AndroidNativeOnVsync();

/** Reporting of window inset changes. */
void AndroidNativeOnWindowInsets(Int iTop, Int iBottom);

/**
 * Structure used to enqueue message box callbacks.
 */
struct AndroidNativeActivityMessageBoxCallbackEntry
{
	AndroidNativeActivityMessageBoxCallbackEntry()
		: m_Callback()
		, m_eButtonPressed(kMessageBoxButtonOK)
	{
	}

	MessageBoxCallback m_Callback;
	EMessageBoxButton m_eButtonPressed;
}; // struct AndroidNativeActivityMessageBoxCallbackEntry

/**
 * Structured used to enqueue tracking info callbacks.
 */
struct AndroidNativeActivityTrackingInfoCallbackEntry
{
	AndroidNativeActivityTrackingInfoCallbackEntry()
		: m_Callback()
		, m_TrackingInfo()
	{
	}

	AndroidEngine::TrackingInfoCallback m_Callback;
	AndroidTrackingInfo m_TrackingInfo;
}; // struct AndroidNativeActivityTrackingInfoCallbackEntry

template <UInt32 SIZE>
inline static void SetStringFromJava(
	JNIEnv* pJniEnvironment,
	jstring inputString,
	Seoul::FixedArray<Seoul::Byte, SIZE>& rOutputString)
{
	using namespace Seoul;

	SEOUL_STATIC_ASSERT(SIZE > 0);

	if (inputString == nullptr)
	{
		rOutputString[0] = 0;
		return;
	}

	Byte const* sInputString = pJniEnvironment->GetStringUTFChars(inputString, nullptr);
	UInt32 const zLength = StrLen(sInputString);
	UInt32 const zCopySize = Min(zLength, rOutputString.GetSize() - 1u);
	memcpy(rOutputString.Get(0u), sInputString, zCopySize);
	rOutputString[zCopySize] = '\0';
	pJniEnvironment->ReleaseStringUTFChars(inputString, sInputString);
}

// Bindings used by JNI hooks, see below.
static void HandleAndroidNativeActivityMessageBoxCallbackEntry(AndroidNativeActivityMessageBoxCallbackEntry* pEntry)
{
	if (pEntry->m_Callback.IsValid())
	{
		(pEntry->m_Callback)(pEntry->m_eButtonPressed);
	}
	SafeDelete(pEntry);
}

static void HandleAndroidNativeActivityTrackingInfoCallbackEntry(AndroidNativeActivityTrackingInfoCallbackEntry* pEntry)
{
	if (pEntry->m_Callback.IsValid())
	{
		(pEntry->m_Callback)(pEntry->m_TrackingInfo);
	}
	SafeDelete(pEntry);
}

static void HandleSetExternalTrackingUserID(String sExternalTrackingUserID)
{
	if (AndroidTrackingManager::Get())
	{
		AndroidTrackingManager::Get()->SetExternalTrackingUserID(sExternalTrackingUserID);
	}
}

static void HandleSetAttributionData(String sCampaign, String sMediaSource)
{
	if (AndroidEngine::Get())
	{
		AndroidEngine::Get()->SetAttributionData(sCampaign, sMediaSource);
	}
}

static void DeepLinkCampaignDelegate(String sCampaign)
{
	if (AndroidTrackingManager::Get())
	{
		AndroidTrackingManager::Get()->DeepLinkCampaignDelegate(sCampaign);
	}
}

static void HandleApplyText(String sText)
{
	if (AndroidEngine::Get())
	{
		AndroidEngine::Get()->JavaToNativeTextEditableApplyText(sText);
	}
}

static void HandleStopEditing()
{
	if (AndroidEngine::Get())
	{
		AndroidEngine::Get()->JavaToNativeTextEditableStopEditing();
	}
}

static void HandleOpenURL(String sURL)
{
	if (!sURL.IsEmpty())
	{
		g_pEngineVirtuals->OnOpenURL(sURL);
	}
}

static void HandleOnSignInFinished(Bool bSignedIn, void *pVoidDelegate)
{
	// TODO: Google Plus sign in hookup - send a broadcast event, possibly,
	// pass to options screen.
}

#if SEOUL_WITH_REMOTE_NOTIFICATIONS
static void HandleSetRemoteNotificationToken(String sToken)
{
	auto pEngine = Engine::Get();
	if (pEngine.IsValid())
	{
		pEngine->SetRemoteNotificationToken(sToken);
	}
}
#endif // #if SEOUL_WITH_REMOTE_NOTIFICATIONS

// /Bindings used by JNI hooks, see below.

} // namespace Seoul

// JNI hooks for callbacks from Java into C++ code.
extern "C"
{

#define SEOUL_ACTIVITY_JNI_FUNC(name) \
	Java_com_demiurgestudios_seoul_AndroidNativeActivity_##name

JNIEXPORT void JNICALL SEOUL_ACTIVITY_JNI_FUNC(NativeMessageBoxCallbackInvoke)(
	JNIEnv* pJniEnvironment,
	jclass,
	jlong pUserData,
	jint iButtonPressed)
{
	using namespace Seoul;

	MessageBoxCallback* pCallback = reinterpret_cast<MessageBoxCallback*>(pUserData);

	AndroidNativeActivityMessageBoxCallbackEntry* pEntry = SEOUL_NEW(MemoryBudgets::TBD) AndroidNativeActivityMessageBoxCallbackEntry;
	pEntry->m_Callback = *pCallback;
	pEntry->m_eButtonPressed = (EMessageBoxButton)iButtonPressed;
	SafeDelete(pCallback);

	RunOnMainThread(&HandleAndroidNativeActivityMessageBoxCallbackEntry, pEntry);
}

JNIEXPORT void JNICALL SEOUL_ACTIVITY_JNI_FUNC(NativeSetExternalTrackingUserID)(
	JNIEnv* pJniEnvironment,
	jclass,
	jstring sJniExternalTrackingUserID)
{
	using namespace Seoul;

	String sExternalTrackingUserID;
	SetStringFromJava(pJniEnvironment, sJniExternalTrackingUserID, sExternalTrackingUserID);

	RunOnMainThread(&HandleSetExternalTrackingUserID, sExternalTrackingUserID);
}

JNIEXPORT void JNICALL SEOUL_ACTIVITY_JNI_FUNC(NativeTrackingInfoCallbackInvoke)(
	JNIEnv* pJniEnvironment,
	jclass,
	jlong pUserData,
	jstring sAdvertisingId,
	jboolean bLimitTracking)
{
	using namespace Seoul;

	AndroidEngine::TrackingInfoCallback* pCallback = reinterpret_cast<AndroidEngine::TrackingInfoCallback*>(pUserData);

	AndroidNativeActivityTrackingInfoCallbackEntry* pEntry = SEOUL_NEW(MemoryBudgets::TBD) AndroidNativeActivityTrackingInfoCallbackEntry;
	pEntry->m_Callback = *pCallback;
	SetStringFromJava(pJniEnvironment, sAdvertisingId, pEntry->m_TrackingInfo.m_sAdvertisingId);
	pEntry->m_TrackingInfo.m_bLimitTracking = (Bool)(0 != bLimitTracking);
	SafeDelete(pCallback);

	RunOnMainThread(&HandleAndroidNativeActivityTrackingInfoCallbackEntry, pEntry);
}

JNIEXPORT void JNICALL SEOUL_ACTIVITY_JNI_FUNC(NativeSetAttributionData)(
	JNIEnv* pJniEnvironment,
	jclass,
	jstring sJniCampaign,
	jstring sJniMediaSource)
{
	using namespace Seoul;

	String sCampaign;
	SetStringFromJava(pJniEnvironment, sJniCampaign, sCampaign);

	String sMediaSource;
	SetStringFromJava(pJniEnvironment, sJniMediaSource, sMediaSource);

	RunOnMainThread(&HandleSetAttributionData, sCampaign, sMediaSource);
}

JNIEXPORT void JNICALL SEOUL_ACTIVITY_JNI_FUNC(NativeDeepLinkCampaignDelegate)(
	JNIEnv* pJniEnvironment,
	jclass,
	jstring sJniCampaign)
{
	using namespace Seoul;

	String sCampaign;
	SetStringFromJava(pJniEnvironment, sJniCampaign, sCampaign);

	RunOnMainThread(&DeepLinkCampaignDelegate, sCampaign);
}

JNIEXPORT void JNICALL SEOUL_ACTIVITY_JNI_FUNC(NativeSetCanPerformNativeStartup)(
	JNIEnv*,
	jclass,
	jboolean bCanPerformNativeStartup)
{
	using namespace Seoul;

	g_bCanPerformNativeStartup = bCanPerformNativeStartup;
}

JNIEXPORT void JNICALL SEOUL_ACTIVITY_JNI_FUNC(NativeOnVsync)()
{
	using namespace Seoul;

	// Call the OGLES2 handler.
	AndroidNativeOnVsync();
}

JNIEXPORT void JNICALL SEOUL_ACTIVITY_JNI_FUNC(NativeOnWindowInsets)(
	JNIEnv* pJniEnvironment,
	jclass,
	jint iTop,
	jint iBottom)
{
	using namespace Seoul;

	// Call the OGLES2 handler.
	AndroidNativeOnWindowInsets(iTop, iBottom);
}

JNIEXPORT jboolean JNICALL Java_com_demiurgestudios_seoul_AndroidNativeCrashManager_NativeCrashManagerIsEnabled(JNIEnv*, jclass)
{
#if SEOUL_WITH_NATIVE_CRASH_REPORTING
	// Disabled if a local build.
	return (0 != Seoul::g_kBuildChangelistFixed);
#else
	return false;
#endif
}

JNIEXPORT void JNICALL SEOUL_ACTIVITY_JNI_FUNC(NativeSetCacheDirectory)(
	JNIEnv* pJniEnvironment,
	jclass,
	jstring cacheDir)
{
	using namespace Seoul;

	String s;
	SetStringFromJava(pJniEnvironment, cacheDir, s);

	Path::AndroidSetCacheDir(s);
}

JNIEXPORT void JNICALL SEOUL_ACTIVITY_JNI_FUNC(NativeSetInternalStorageDirectory)(
	JNIEnv* pJniEnvironment,
	jclass,
	jstring internalStorageDirectoryString)
{
	Seoul::SetStringFromJava(pJniEnvironment, internalStorageDirectoryString, Seoul::g_InternalStorageDirectoryString);
}

JNIEXPORT void JNICALL SEOUL_ACTIVITY_JNI_FUNC(NativeSetSourceDir)(
	JNIEnv* pJniEnvironment,
	jclass,
	jstring publicSourceDirString)
{
	Seoul::SetStringFromJava(pJniEnvironment, publicSourceDirString, Seoul::g_SourceDir);
}

JNIEXPORT void JNICALL SEOUL_ACTIVITY_JNI_FUNC(NativeSetSubPlatform)(
	JNIEnv* pJniEnvironment,
	jclass,
	jint iBuildType)
{
	using namespace Seoul;

	g_ePlatformFlavor = (PlatformFlavor)iBuildType;
}

JNIEXPORT void JNICALL SEOUL_ACTIVITY_JNI_FUNC(NativeSetTouchSlop)(
	JNIEnv* pJniEnvironment,
	jclass,
	jint iTouchSlop)
{
	Seoul::g_iTouchSlop = iTouchSlop;
}

JNIEXPORT void JNICALL SEOUL_ACTIVITY_JNI_FUNC(NativeSetCommandline)(
	JNIEnv* pJniEnvironment,
	jclass,
	jstring commandline)
{
	using namespace Seoul;

	SetStringFromJava(pJniEnvironment, commandline, g_CommandlineArguments);
}

JNIEXPORT jboolean JNICALL SEOUL_ACTIVITY_JNI_FUNC(NativeShouldEnableDebugLog)(
	JNIEnv* pJniEnvironment,
	jclass)
{
#if !SEOUL_SHIP
	return true;
#else
	return false;
#endif
}

JNIEXPORT void JNICALL SEOUL_ACTIVITY_JNI_FUNC(NativeHandleOpenURL)(
	JNIEnv* pJniEnvironment,
	jclass,
	jstring url)
{
	using namespace Seoul;

	String sURL;
	SetStringFromJava(pJniEnvironment, url, sURL);

	RunOnMainThread(&HandleOpenURL, sURL);
}

JNIEXPORT void JNICALL SEOUL_ACTIVITY_JNI_FUNC(NativeOnBackPressed)(
	JNIEnv* pJniEnvironment,
	jclass)
{
	using namespace Seoul;

	// Back button handling is now handled by the regular keyboard handling
	// in InternalStaticHandleInputEvent()
}

JNIEXPORT void JNICALL SEOUL_ACTIVITY_JNI_FUNC(NativeOnSignInFinished)(
	JNIEnv* pJniEnvironment,
	jclass,
	jboolean bSignedIn,
	jlong pUserData)
{
	using namespace Seoul;

	RunOnMainThread<Bool, void*>(&HandleOnSignInFinished, bSignedIn, (void *)pUserData);
}

#if SEOUL_WITH_REMOTE_NOTIFICATIONS
JNIEXPORT void JNICALL SEOUL_ACTIVITY_JNI_FUNC(NativeOnRegisteredForRemoteNotifications)(
	JNIEnv* pJniEnvironment,
	jclass,
	jstring sToken)
{
	using namespace Seoul;

	String sSeoulToken;
	SetStringFromJava(pJniEnvironment, sToken, sSeoulToken);

	RunOnMainThread(&HandleSetRemoteNotificationToken, sSeoulToken);
}
#endif // /SEOUL_WITH_REMOTE_NOTIFICATIONS

JNIEXPORT void JNICALL SEOUL_ACTIVITY_JNI_FUNC(NativeStopEditing)(
	JNIEnv* pJniEnvironment,
	jclass)
{
	using namespace Seoul;

	// New style: replace
	RunOnMainThread(&HandleStopEditing);
}

JNIEXPORT void JNICALL SEOUL_ACTIVITY_JNI_FUNC(NativeApplyText)(
	JNIEnv* pJniEnvironment,
	jclass,
	jstring sJniText)
{
	using namespace Seoul;

	String sText;
	SetStringFromJava(pJniEnvironment, sJniText, sText);

	RunOnMainThread(&HandleApplyText, sText);
}

#undef SEOUL_ACTIVITY_JNI_FUNC

} // extern "C"
