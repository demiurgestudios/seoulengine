/**
 * \file AppAndroid.cpp
 * \brief Singleton class unique to Android - hosts entry points
 * from the Java host code into SeoulEngine and to App C++ code.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "AnalyticsManager.h"
#include "AndroidCommerceManager.h"
#include "AndroidCrashManager.h"
#include "AndroidEngine.h"
#include "AndroidFacebookManager.h"
#include "AndroidFileSystem.h"
#include "AndroidZipFileSystem.h"
#include "AndroidGlobals.h"
#include "AndroidMainThreadQueue.h"
#include "AndroidPrereqs.h"
#include "AndroidTrackingManager.h"
#include "AppAndroid.h"
#include "AppAndroidJni.h"
#include "Atomic32.h"
#include "Atomic64.h"
#include "AtomicRingBuffer.h"
#include "BuildChangelist.h"
#include "BuildChangelistPublic.h"
#include "BuildDistro.h"
#include "BuildVersion.h"
#include "DataStoreParser.h"
#include "DiskFileSystem.h"
#include "DownloadablePackageFileSystem.h"
#include "EngineCommandLineArgs.h"
#include "EngineVirtuals.h"
#include "FileManager.h"
#include "GameAutomation.h"
#include "GameClient.h"
#include "GameClientSettings.h"
#include "GameConfigManager.h"
#include "GameMain.h"
#include "GamePaths.h"
#include "GenericInput.h"
#include "HTTPManager.h"
#include "JobsFunction.h"
#include "JobsManager.h"
#include "LocManager.h"
#include "Logger.h"
#include "MoriartyFileSystem.h"
#include "OGLES2RenderDevice.h"
#include "PackageFileSystem.h"
#include "PatchablePackageFileSystem.h"
#include "Path.h"
#include "PlatformData.h"
#include "ReflectionCommandLineArgs.h"
#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionDataStoreTableUtil.h"
#include "ReflectionDefine.h"
#include "Renderer.h"
#include "SeoulString.h"
#include "SettingsManager.h"
#include "StringUtil.h"
#include "Thread.h"
#include <android_native_app_glue.h>

extern "C"
{

// Make sure this dependency (which must be exported from the .so to be called from Android Java internals)
// is not elided by the linker.
JNIEXPORT void ANativeActivity_onCreate(ANativeActivity* activity, void* savedState, size_t savedStateSize);
volatile void* ANativeActivity_onCreate_anchor = (void*)ANativeActivity_onCreate;

} // extern "C"

namespace Seoul
{

Byte const* ksGoogleOauthClientId = "";

#if SEOUL_WITH_APPS_FLYER
Byte const* ksAppsFlyerID = "";
#endif // /SEOUL_WITH_APPS_FLYER
#if SEOUL_WITH_HELPSHIFT
Byte const* ksHelpShiftUserIDSuffix = "";
Byte const* ksHelpShiftKey = "";
Byte const* ksHelpShiftDomain = "";
Byte const* ksHelpShiftID = "";
#endif

/** Volatile global boolean to track if engine initialization has completed or not. */
static Atomic32Value<Bool> s_bEngineReady(false);

/** Mutex to protect android_main() from existing twice in two different threads. */
static Mutex s_MainMutex;

/** Lock to make certain access to Engine startup/shutdown exclusive. */
static Mutex s_EngineMutex;

/** If true, AppAndroid should be ticked. */
static Atomic32Value<Bool> s_bShouldTickAppAndroid(false);

/** Tracking for whether the app is currently in the background or not. */
static Atomic32Value<Bool> s_bInBackground(false);

/** Tracking for whether the app is currently active or not. */
static Atomic32Value<Bool> s_bIsActive(true);

/**
 * Bits for various state that affect focus status. NOTE: Lifecycle
 * behavior on Android is a mess and despite being (officially) documented
 * as a state machine (background -> pause -> resume -> foreground), in
 * practice, stages can be skipped or occur multiple times.
 *
 * So, our implementation has some additional complexities to respond with
 * reason under these conditions.
 */
enum
{
	/** If true, app is considered active but does not have focus. */
	kLevelUnfocused = (1 << 0),

	/** If true, app is paused, and is considered inactive (but not necessarily invisible). */
	kLevelPaused    = (1 << 1),

	/** If true, app is fully suspended/inactive and is not visible. */
	kLevelSuspended = (1 << 2),

	/** If true, text entry is active. We use this to suppress unfocus handling until the keyboard is dismissed. */
	kLevelKeyboard  = (1 << 3),
};
static Atomic32Value<Int> s_iFocusLevel(0);
static Atomic64Value<Int64> s_iLastKeyboardStateChange(-1);

/** Convenience for checking whether we should be in the background or not. */
static Bool FocusLevelNeedsBackground()
{
	// Always background on paused.
	if (kLevelPaused == (kLevelPaused & s_iFocusLevel)) { return true; }
	// Always background on suspend.
	if (kLevelSuspended == (kLevelSuspended & s_iFocusLevel)) { return true; }

	// Background if unfocused without keyboard.
	if (kLevelUnfocused == (kLevelUnfocused & s_iFocusLevel) &&
		0 == (kLevelKeyboard & s_iFocusLevel))
	{
		return true;
	}

	// Done.
	return false;
}

/**
 * Helper function called when we pause/resume. Update s_iFocusLevel first,
 * then call this function to actually apply the changes.
 */
static void ApplyFocusLevel()
{
	SEOUL_LOG("ApplyFocusLevel(): "
		"s_bInBackground = %s, "
		"kLevelUnfocused = %s, "
		"kLevelPaused = %s, "
		"kLevelSuspended = %s, "
		"kLevelKeyboard = %s",
		s_bInBackground ? "true" : "false",
		(kLevelUnfocused == (kLevelUnfocused & s_iFocusLevel)) ? "true" : "false",
		(kLevelPaused == (kLevelPaused & s_iFocusLevel)) ? "true" : "false",
		(kLevelSuspended == (kLevelSuspended & s_iFocusLevel)) ? "true" : "false",
		(kLevelKeyboard == (kLevelKeyboard & s_iFocusLevel)) ? "true" : "false");

	if (s_bInBackground)
	{
		// If we resume with focus, resume immediately.  On some devices, when
		// resuming a locked device, the activity is resumed but the lock
		// screen is displayed on top, so we don't have focus.  In that case,
		// we don't want to resume the app until after we gain focus.  Other
		// system dialogs (such as the volume overlay) may also have this
		// effect.
		if (!FocusLevelNeedsBackground())
		{
			if (Game::Main::Get())
			{
				Game::Main::Get()->OnLeaveBackground();
			}
			s_bInBackground = false;
		}
	}
	else
	{
		// When focus is lost, either the activity is about to be paused, or
		// another window has gained focus on top of us, but we might still be
		// visible underneath that window (e.g. an AlertDialog)
		if (FocusLevelNeedsBackground())
		{
			if (Game::Main::Get())
			{
				Game::Main::Get()->OnEnterBackground();
			}

			// Don't re-enter the background if we're already paused.
			s_bInBackground = true;
		}
	}
}

/** If true, native code can continue with startup. */
extern Atomic32Value<Bool> g_bCanPerformNativeStartup;

/** Cache the PatchablePackageFileSystem, passed to game app for handling downloadable config updates. */
static CheckedPtr<PatchablePackageFileSystem> s_pPatchableConfigPackageFileSystem;

/** Cache the PatchablePackageFileSystem, passed to game app for handling downloadable content updates. */
static CheckedPtr<PatchablePackageFileSystem> s_pPatchableContentPackageFileSystem;

/** Cache the DownloadablePackageFileSystem, used at startup to download updated game content, if necessary. */
extern CheckedPtr<DownloadablePackageFileSystem> g_pDownloadableContentPackageFileSystem;

/** Global instance of the native activity wrapper. */
static android_app* s_pAndroidApp = nullptr;

/**
 * Utility, returns the wait period for message polling,
 * based on s_bInBackground.
 *
 * Rough behavior here based on
 * NVIDIA GameWorks samples.
 */
static Int GetAppWaitInMilliseconds()
{
	if(Game::Main::Get())
	{
		return (Game::Main::Get()->IsInBackground() ? 250 : 0);
	}
	return (s_bInBackground ? 250 : 0);
}

/**
 * Global hook, called by FileManager as early as possible during initialization,
 * to give us a chance to hook up our file systems before any file requests are made.
 */
void OnInitializeFileSystems()
{
	// FileManager checks FileSystems in LIFO order, so we want the DiskFileSystem to
	// be absolutely last - check packages first - in ship builds.
#if SEOUL_SHIP
	FileManager::Get()->RegisterFileSystem<DiskFileSystem>();
#endif

	// Fallback path for Android_Content.sar
	String const sFallback(Path::Combine(GamePaths::Get()->GetBaseDir(), "Data/Android_Content.sar")); // read-only builtin

	// Workaround for a bug/bad behavior in AAssetManager, which is the Android
	// facility that implements AndroidFileSystem. Very large files (e.g. Android_Content.sar)
	// can fail to load on 32-bit OS due to exhaustion of the virtual address space. This
	// occurs because AAssetManager implements file reading by mapping the entire file using
	// mmap. See also:
	// - https://github.com/google/ExoPlayer/issues/5153
	// - https://android.googlesource.com/platform/frameworks/base/+/master/libs/androidfw/AssetManager.cpp#911
	//
	// NOTE: We only do this in non-Ship builds or when the build changelist is 0 (a local build)
	// because it is potentially risky to implement access of the .apk using our own .zip reader, for
	// several reasons:
	// - we do not control/create the APK distributed by Google Play since App Bundles (AAB files) were implemented
	// - technically, the OS is allowed to (e.g.) extract the APK and then implement AAssetManager transparently
	//   to read a directory instead of a .zip archive.
#if !SEOUL_SHIP || defined(SEOUL_PROFILING_BUILD) || 0 == BUILD_CHANGELIST
	Bool bDone = false;
	{
		// Public
		String const sSource(g_SourceDir.Data());
		if (DiskSyncFile::FileExists(sSource))
		{
			// Must use AndroidZipFileSystem, since even AndroidFileSystem::Exists() is affected
			// by the OOM issue.
			AndroidZipFileSystem tester(sSource);
			if (tester.Exists(sFallback))
			{
				FileManager::Get()->RegisterFileSystem<AndroidZipFileSystem>(
					sSource);
				bDone = true;
			}
		}
	}

	if (!bDone)
#endif
	{
		FileManager::Get()->RegisterFileSystem<AndroidFileSystem>(
			s_pAndroidApp->activity->assetManager);
	}

	// Android_ClientSettings.sar
	FileManager::Get()->RegisterFileSystem<PackageFileSystem>(
		Path::Combine(GamePaths::Get()->GetBaseDir(), "Data/Android_ClientSettings.sar"));

	// Android_Config.sar
	s_pPatchableConfigPackageFileSystem = FileManager::Get()->RegisterFileSystem<PatchablePackageFileSystem>(
		Path::Combine(GamePaths::Get()->GetBaseDir(), "Data/Android_Config.sar"), // read-only builtin
		Path::Combine(GamePaths::Get()->GetSaveDir(), "Data/Android_ConfigUpdate.sar")); // updateable path

	// Android_Content.sar - if the fallback package exists, just use it, don't instantiate the downloadable
	// system.
	String const sServerBaseURL(Game::ClientSettings::GetServerBaseURL());
	if (sServerBaseURL.IsEmpty() || FileManager::Get()->Exists(sFallback) || DiskSyncFile::FileExists(sFallback))
	{
		FileManager::Get()->RegisterFileSystem<PackageFileSystem>(sFallback);
	}
	else
	{
		DownloadablePackageFileSystemSettings settings;
		settings.m_sAbsolutePackageFilename = Path::Combine(GamePaths::Get()->GetSaveDir(), "Data/Android_Content.sar");
		settings.m_sInitialURL = String::Printf("%s/v1/auth/additional_clientgamedata", sServerBaseURL.CStr());
		g_pDownloadableContentPackageFileSystem = FileManager::Get()->RegisterFileSystem<DownloadablePackageFileSystem>(settings);
	}

	// Android_BaseContent.sar
	FileManager::Get()->RegisterFileSystem<PackageFileSystem>(
		Path::Combine(GamePaths::Get()->GetBaseDir(), "Data/Android_BaseContent.sar"));

	// In non-ship builds, also include debug script files.
#if !SEOUL_SHIP
	FileManager::Get()->RegisterFileSystem<PackageFileSystem>(
		Path::Combine(GamePaths::Get()->GetBaseDir(), "Data/Android_ScriptsDebug.sar"));
#endif // /#if !SEOUL_SHIP

	// Android_ContentUpdate.sar
	s_pPatchableContentPackageFileSystem = FileManager::Get()->RegisterFileSystem<PatchablePackageFileSystem>(
		Path::Combine(GamePaths::Get()->GetBaseDir(), "Data/Android_ContentUpdate.sar"), // read-only builtin
		Path::Combine(GamePaths::Get()->GetSaveDir(), "Data/Android_ContentUpdate.sar")); // updateable path

	// Otherwise, we want to prioritize the disk file system and the remote
	// file system, so developers read from local files instead of packages if
	// available.
#if !SEOUL_SHIP
	if (!EngineCommandLineArgs::GetPreferUsePackageFiles())
	{
		FileManager::Get()->RegisterFileSystem<DiskFileSystem>();
	}
#endif

#if SEOUL_WITH_MORIARTY
	if (!EngineCommandLineArgs::GetMoriartyServer().IsEmpty())
	{
		FileManager::Get()->RegisterFileSystem<MoriartyFileSystem>();
	}
#endif
}

Bool AndroidIsTrackingEnabled()
{
	// Only enable in prod.
	return (Game::ServerType::kProd == Game::ClientSettings::GetServerType());
}

Bool AndroidIsOnProd()
{
	return (Game::ServerType::kProd == Game::ClientSettings::GetServerType());
}

void AndroidSendCustomCrash(const CustomCrashErrorState& errorState)
{
	// Pass the custom crash data through to CrashManager.
	if (CrashManager::Get())
	{
		CrashManager::Get()->SendCustomCrash(errorState);
	}
}

extern "C"
{

static void AtExitJNI()
{
	// TODO: It appears, starting with Android 5.x,
	// that the libmedia.so dynamic library loads
	// a global static instance of AudioTrackClientProxy
	// that owns an instance named JNIAudioPortCallback
	// which interacts with Java via JNI calls,
	// on our game's main thread.
	//
	// This teardown happens in the cpp destructors
	// invoked by exit(0). As a result, we must
	// teardown JNI support in an atexit() handler,
	// or the app will crash when the code
	// in libmedia.so tries to tear down its
	// dependencies.
	Thread::ShutdownJavaNativeThreading();
}

} // extern "C"

static void SetupJNI(android_app* pAndroidApp)
{
	Thread::InitializeJavaNativeThreading(pAndroidApp->activity->vm);
	atexit(AtExitJNI);
}

AppAndroidEngine::AppAndroidEngine(
	const String& sAutomationScript,
	ANativeWindow* pMainWindow,
	const String& sBaseDirectoryPath)
	: m_pEngine()
{
	using namespace Seoul;

	// Hook up a callback that will be invoked when the FileSystem is starting up,
	// so we can configure the game's packages before any file requests are made.
	g_pInitializeFileSystemsCallback = OnInitializeFileSystems;

	// Initialize SeoulTime
	SeoulTime::MarkGameStartTick();

	// Mark that we're now in the main function.
	Seoul::BeginMainFunction();

	// Setup some game specific paths before initializing Engine and Core.
	GamePaths::SetUserConfigJsonFileName("game_config.json");

	// Set the main thread to the current thread.
	SetMainThreadId(Thread::GetThisThreadId());

	// Initialize CrashManager
	{
		AndroidCrashManagerSettings settings;
		settings.m_sCrashReportDirectory = sBaseDirectoryPath;
		m_pCrashManager.Reset(SEOUL_NEW(MemoryBudgets::Game) AndroidCrashManager(settings));
	}

	// Initialize Engine.
	{
		AndroidEngineSettings settings;
		settings.m_IsTrackingEnabled = SEOUL_BIND_DELEGATE(&AndroidIsTrackingEnabled);
		settings.m_pMainWindow = pMainWindow;
		settings.m_SaveLoadManagerSettings = Game::Main::GetSaveLoadManagerSettings(Game::MainSettings::kOff);
		settings.m_AnalyticsSettings.m_GetApiKeyDelegate = SEOUL_BIND_DELEGATE(&Game::ClientSettings::GetAnalyticsApiKey);
		settings.m_AnalyticsSettings.m_eType = GenericAnalyticsManagerType::kMixpanel;
		settings.m_AnalyticsSettings.m_CustomCurrentTimeDelegate = SEOUL_BIND_DELEGATE(&Game::Client::StaticGetCurrentServerTime);
		settings.m_TrackingSettings.m_pNativeActivity = s_pAndroidApp->activity;
#if SEOUL_WITH_APPS_FLYER
		settings.m_TrackingSettings.m_sAppsFlyerID = ksAppsFlyerID;
		settings.m_TrackingSettings.m_GetIsOnProd = SEOUL_BIND_DELEGATE(&AndroidIsOnProd);
#endif // /SEOUL_WITH_APPS_FLYER
#if SEOUL_WITH_HELPSHIFT
		settings.m_TrackingSettings.m_sHelpShiftUserIDSuffix = ksHelpShiftUserIDSuffix;
		settings.m_TrackingSettings.m_sHelpShiftKey = ksHelpShiftKey;
		settings.m_TrackingSettings.m_sHelpShiftDomain = ksHelpShiftDomain;
		settings.m_TrackingSettings.m_sHelpShiftID = ksHelpShiftID;
#endif
		settings.m_CoreSettings.m_GamePathsSettings.m_sBaseDirectoryPath = sBaseDirectoryPath;

#if SEOUL_WITH_GOOGLEPLAYGAMES
		settings.m_SignInManagerSettings.m_sOauthClientId = ksGoogleOauthClientId;
#endif // /#if SEOUL_WITH_GOOGLEPLAYGAMES

		// TODO: May not want this in some profiling cases?
#if defined(SEOUL_PROFILING_BUILD)
		if (sAutomationScript.Find("Performance") != String::NPos)
		{
			settings.m_bPreferHeadless = true;
		}
#endif // /#if defined(SEOUL_PROFILING_BUILD)

		// Settings for securing the persistent unique device identifier.
		//
		// This is the encryption key, random 32 bytes.
		UInt8 const aKey[32] =
		{
			0x2d, 0x5a, 0x41, 0x20, 0x1c, 0xf3, 0x59, 0x28,
			0x44, 0xe1, 0xb5, 0xd7, 0x00, 0x21, 0xc1, 0x59,
			0x9d, 0x8e, 0x36, 0x09, 0x35, 0x76, 0xb3, 0x1b,
			0xed, 0x60, 0x04, 0x91, 0x59, 0x23, 0x73, 0x63,
		};
		settings.m_vUUIDEncryptionKey.Resize((UInt32)sizeof(aKey));
		memcpy(settings.m_vUUIDEncryptionKey.Get(0), aKey, sizeof(aKey));

		// Sanitize the paths - if a base directory was specified but
		// it doesn't have a trailing slash, add one.
		if (!settings.m_CoreSettings.m_GamePathsSettings.m_sBaseDirectoryPath.IsEmpty() &&
			!settings.m_CoreSettings.m_GamePathsSettings.m_sBaseDirectoryPath.EndsWith("/"))
		{
			settings.m_CoreSettings.m_GamePathsSettings.m_sBaseDirectoryPath.Append('/');
		}

		settings.m_sExecutableName = Path::Combine(settings.m_CoreSettings.m_GamePathsSettings.m_sBaseDirectoryPath, "libAppAndroid.so");
		settings.m_pNativeActivity = s_pAndroidApp->activity;
		settings.m_ePlatformFlavor = g_ePlatformFlavor;

		m_pEngine.Reset(SEOUL_NEW(MemoryBudgets::Game) AndroidEngine(settings));
		m_pEngine->Initialize();
	}
}

AppAndroidEngine::~AppAndroidEngine()
{
	// Shutdown engine.
	m_pEngine->Shutdown();
	m_pEngine.Reset();

	// Shutdown crash manager.
	m_pCrashManager.Reset();

	Seoul::EndMainFunction();

	// Clear file IO hooks
	g_pDownloadableContentPackageFileSystem.Reset();
	s_pPatchableContentPackageFileSystem.Reset();
	s_pPatchableConfigPackageFileSystem.Reset();
	g_pInitializeFileSystemsCallback = nullptr;
}

AppAndroid::AppAndroid(const String& sAutomationScript)
	: m_pGameMain()
{
	auto const sServerBaseURL(Game::ClientSettings::GetServerBaseURL());
#if SEOUL_WITH_GAME_PERSISTENCE
	GamePersistenceSettings persistenceSettings;
	persistenceSettings.m_FilePath = FilePath::CreateSaveFilePath(Game::ClientSettings::GetSaveGameFilename());
	if (!sServerBaseURL.IsEmpty())
	{
		persistenceSettings.m_sCloudLoadURL = sServerBaseURL + "/v1/saving/load";
		persistenceSettings.m_sCloudResetURL = sServerBaseURL + "/v1/saving/reset";
		persistenceSettings.m_sCloudSaveURL = sServerBaseURL + "/v1/saving/save";
	}
	persistenceSettings.m_iVersion = AppPersistenceMigrations::kiPlayerDataVersion;
	persistenceSettings.m_pPersistenceManagerType = &TypeOf<AppPersistenceManager>();
	persistenceSettings.m_tMigrations = AppPersistenceMigrations::GetMigrations();
	Game::MainSettings settings(TypeOf<AppConfigManager>(), persistenceSettings);
#else
	Game::MainSettings settings(TypeOf<Game::NullConfigManager>());
#endif
	settings.m_sServerBaseURL = sServerBaseURL;
	settings.m_pConfigUpdatePackageFileSystem = s_pPatchableConfigPackageFileSystem;
	settings.m_pContentUpdatePackageFileSystem = s_pPatchableContentPackageFileSystem;

#if !SEOUL_SHIP || defined(SEOUL_PROFILING_BUILD)
	// Possibility of delay enabling automation scripting.
	if (!sAutomationScript.IsEmpty())
	{
		settings.m_sAutomationScriptMain = sAutomationScript;
		settings.m_eAutomatedTesting = Game::MainSettings::kAutomatedTesting;

		// Also disable OpenURL() to prevent loss of focus.
		Engine::Get()->SetSuppressOpenURL(true);

		// Configure booleans for automated testing.
		g_bRunningAutomatedTests = true;
		g_bHeadless = true;
		g_bShowMessageBoxesOnFailedAssertions = false;
		g_bEnableMessageBoxes = false;
	}
#endif // /#if !SEOUL_SHIP

	// Only hookup to CrashManager if custom crashes are supported.
	if (CrashManager::Get()->CanSendCustomCrashes())
	{
		settings.m_ScriptErrorHandler = SEOUL_BIND_DELEGATE(&AndroidSendCustomCrash);
	}
	// In non-ship builds, fall back to default handling.
#if !SEOUL_SHIP
	else
	{
		settings.m_ScriptErrorHandler = SEOUL_BIND_DELEGATE(&CrashManager::DefaultErrorHandler);
	}
#endif // /#if !SEOUL_SHIP

	m_pGameMain.Reset(SEOUL_NEW(MemoryBudgets::Game) Game::Main(settings));
}

AppAndroid::~AppAndroid()
{
	m_pGameMain.Reset();
}

// Common point for cleaning up app and engine.
static void DoAppShutdown();

static void OnTickRequestExit()
{
	// Cache for us below.
	auto pAndroidApp = s_pAndroidApp;

	// We kill the native layer first, because in situations of low available
	// memory, the OS will terminate the application immediately upon
	// the call to finish below, not allowing native shutdown to execute.
	DoAppShutdown(); SEOUL_TEARDOWN_TRACE();

	// Kill the activity.
	if (nullptr != pAndroidApp)
	{
		ScopedJavaEnvironment scope;
		JNIEnv *pEnvironment = scope.GetJNIEnv();

		// Tell the Java layer to exit.
		Java::Invoke<void>(
			pEnvironment,
			pAndroidApp->activity->clazz,
			"finish",
			"()V"); SEOUL_TEARDOWN_TRACE();
	}
}

void AppAndroid::Tick()
{
	if (!m_pGameMain->Tick())
	{
		OnTickRequestExit();
	}
}

void AppAndroid::Initialize()
{
}

void AppAndroid::Shutdown()
{
}

static void CreateAppAndroidEngine(
	const String& sAutomationScript,
	ANativeWindow* pMainWindow,
	const String& sBaseDirectoryPath)
{
	if (!AppAndroidEngine::Get().IsValid())
	{
		Lock lock(s_EngineMutex);
		SEOUL_NEW(MemoryBudgets::TBD) AppAndroidEngine(
			sAutomationScript,
			pMainWindow,
			sBaseDirectoryPath);
		s_bEngineReady = true;
	}
}

static void DestroyAppAndroidEngine()
{
	if (AppAndroidEngine::Get().IsValid())
	{
		Lock lock(s_EngineMutex);
		s_bEngineReady = false;
		SEOUL_DELETE AppAndroidEngine::Get();
	}
}

static void CreateAppAndroid(const String& sAutomationScript)
{
	if (!AppAndroid::Get().IsValid())
	{
		SEOUL_NEW(MemoryBudgets::TBD) AppAndroid(sAutomationScript);
		AppAndroid::Get()->Initialize();
	}
}

static void DestroyAppAndroid()
{
	if (AppAndroid::Get().IsValid())
	{
		AppAndroid::Get()->Shutdown();
		SEOUL_DELETE AppAndroid::Get();
	}
}

static void QueueKeyboardEvent(UInt32 uVirtualKeyCode, Bool bPressed)
{
	if (AppAndroid::Get().IsValid() && InputManager::Get())
	{
		InputManager::Get()->QueueKeyboardEvent(uVirtualKeyCode, bPressed);
	}
}

static void QueueTouchButtonEvent(InputButton eTouchButton, Bool bPressed)
{
	if (AppAndroid::Get().IsValid() && InputManager::Get())
	{
		InputManager::Get()->QueueTouchButtonEvent(eTouchButton, bPressed);
	}
}

static void QueueTouchMoveEvent(InputButton eTouch, const Point2DInt& location)
{
	if (AppAndroid::Get().IsValid() && InputManager::Get())
	{
		// Need to account for the hardware scalar when injecting input.
		Point2DInt rescaledLocation = location;
		OGLES2RenderDeviceHardwareScalarState const state = (OGLES2RenderDevice::Get() ? OGLES2RenderDevice::Get()->GetHardwareScalarState() : OGLES2RenderDeviceHardwareScalarState());
		if (state.IsScaling())
		{
			Float const fScalingFactor = state.GetScalingFactor();
			rescaledLocation.X = (Int32)Round(fScalingFactor * rescaledLocation.X);
			rescaledLocation.Y = (Int32)Round(fScalingFactor * rescaledLocation.Y);
		}

		InputManager::Get()->QueueTouchMoveEvent(eTouch, rescaledLocation);
	}
}

static InputButton TranslateAndroidKeyCodeToSeoulButton(int32_t iKeyCode)
{
	switch (iKeyCode)
	{
		// Keyboard buttons
	case AKEYCODE_BACK: return InputButton::KeyBrowserBack;
	case AKEYCODE_0: return InputButton::Key0;
	case AKEYCODE_1: return InputButton::Key1;
	case AKEYCODE_2: return InputButton::Key2;
	case AKEYCODE_3: return InputButton::Key3;
	case AKEYCODE_4: return InputButton::Key4;
	case AKEYCODE_5: return InputButton::Key5;
	case AKEYCODE_6: return InputButton::Key6;
	case AKEYCODE_7: return InputButton::Key7;
	case AKEYCODE_8: return InputButton::Key8;
	case AKEYCODE_9: return InputButton::Key9;
	case AKEYCODE_STAR: return InputButton::KeyNumpadTimes;
	case AKEYCODE_VOLUME_UP: return InputButton::ButtonUnknown;  // Use default handling for volume up/down buttons, so don't convert to KeyVolumeUp/KeyVolumeDown
	case AKEYCODE_VOLUME_DOWN: return InputButton::ButtonUnknown;
	case AKEYCODE_A: return InputButton::KeyA;
	case AKEYCODE_B: return InputButton::KeyB;
	case AKEYCODE_C: return InputButton::KeyC;
	case AKEYCODE_D: return InputButton::KeyD;
	case AKEYCODE_E: return InputButton::KeyE;
	case AKEYCODE_F: return InputButton::KeyF;
	case AKEYCODE_G: return InputButton::KeyG;
	case AKEYCODE_H: return InputButton::KeyH;
	case AKEYCODE_I: return InputButton::KeyI;
	case AKEYCODE_J: return InputButton::KeyJ;
	case AKEYCODE_K: return InputButton::KeyK;
	case AKEYCODE_L: return InputButton::KeyL;
	case AKEYCODE_M: return InputButton::KeyM;
	case AKEYCODE_N: return InputButton::KeyN;
	case AKEYCODE_O: return InputButton::KeyO;
	case AKEYCODE_P: return InputButton::KeyP;
	case AKEYCODE_Q: return InputButton::KeyQ;
	case AKEYCODE_R: return InputButton::KeyR;
	case AKEYCODE_S: return InputButton::KeyS;
	case AKEYCODE_T: return InputButton::KeyT;
	case AKEYCODE_U: return InputButton::KeyU;
	case AKEYCODE_V: return InputButton::KeyV;
	case AKEYCODE_W: return InputButton::KeyW;
	case AKEYCODE_X: return InputButton::KeyX;
	case AKEYCODE_Y: return InputButton::KeyY;
	case AKEYCODE_Z: return InputButton::KeyZ;
	case AKEYCODE_COMMA: return InputButton::KeyComma;
	case AKEYCODE_PERIOD: return InputButton::KeyPeriod;
	case AKEYCODE_ALT_LEFT: return InputButton::KeyLeftAlt;
	case AKEYCODE_ALT_RIGHT: return InputButton::KeyRightAlt;
	case AKEYCODE_SHIFT_LEFT: return InputButton::KeyLeftShift;
	case AKEYCODE_SHIFT_RIGHT: return InputButton::KeyRightShift;
	case AKEYCODE_TAB: return InputButton::KeyTab;
	case AKEYCODE_SPACE: return InputButton::KeySpace;
	case AKEYCODE_ENTER: return InputButton::KeyEnter;
	case AKEYCODE_DEL: return InputButton::KeyBackspace;
	case AKEYCODE_GRAVE: return InputButton::KeyGrave;
	case AKEYCODE_MINUS: return InputButton::KeyMinus;
	case AKEYCODE_EQUALS: return InputButton::KeyEquals;
	case AKEYCODE_LEFT_BRACKET: return InputButton::KeyLeftBracket;
	case AKEYCODE_RIGHT_BRACKET: return InputButton::KeyRightBracket;
	case AKEYCODE_BACKSLASH: return InputButton::KeyBackslash;
	case AKEYCODE_SEMICOLON: return InputButton::KeySemicolon;
	case AKEYCODE_APOSTROPHE: return InputButton::KeyApostrophe;
	case AKEYCODE_SLASH: return InputButton::KeySlash;
	case AKEYCODE_PLUS: return InputButton::KeyNumpadPlus;

		// Gamepad buttons
	case AKEYCODE_BUTTON_A: return InputButton::XboxA;
	case AKEYCODE_BUTTON_X: return InputButton::XboxX;
	case AKEYCODE_BUTTON_Y: return InputButton::XboxY;
	case AKEYCODE_BUTTON_B: return InputButton::XboxB;
	case AKEYCODE_BUTTON_START: return InputButton::XboxStart;
	case AKEYCODE_BUTTON_L1: return InputButton::XboxLeftBumper;
	case AKEYCODE_BUTTON_R1: return InputButton::XboxRightBumper;
	case AKEYCODE_DPAD_UP: return InputButton::XboxDpadUp;
	case AKEYCODE_DPAD_RIGHT: return InputButton::XboxDpadRight;
	case AKEYCODE_DPAD_DOWN: return InputButton::XboxDpadDown;
	case AKEYCODE_DPAD_LEFT: return InputButton::XboxDpadLeft;
	case AKEYCODE_BUTTON_THUMBL: return InputButton::XboxLeftThumbstickButton;
	case AKEYCODE_BUTTON_THUMBR: return InputButton::XboxRightThumbstickButton;

		// Unhandled buttons
	case AKEYCODE_SOFT_LEFT: return InputButton::ButtonUnknown;
	case AKEYCODE_SOFT_RIGHT: return InputButton::ButtonUnknown;
	case AKEYCODE_HOME: return InputButton::ButtonUnknown;
	case AKEYCODE_CALL: return InputButton::ButtonUnknown;
	case AKEYCODE_ENDCALL: return InputButton::ButtonUnknown;
	case AKEYCODE_POUND: return InputButton::ButtonUnknown;
	case AKEYCODE_POWER: return InputButton::ButtonUnknown;
	case AKEYCODE_CAMERA: return InputButton::ButtonUnknown;
	case AKEYCODE_CLEAR: return InputButton::ButtonUnknown;
	case AKEYCODE_SYM: return InputButton::ButtonUnknown;
	case AKEYCODE_EXPLORER: return InputButton::ButtonUnknown;
	case AKEYCODE_ENVELOPE: return InputButton::ButtonUnknown;
	case AKEYCODE_AT: return InputButton::ButtonUnknown;
	case AKEYCODE_NUM: return InputButton::ButtonUnknown;
	case AKEYCODE_HEADSETHOOK: return InputButton::ButtonUnknown;
	case AKEYCODE_FOCUS: return InputButton::ButtonUnknown;   // *Camera* focus
	case AKEYCODE_MENU: return InputButton::ButtonUnknown;
	case AKEYCODE_NOTIFICATION: return InputButton::ButtonUnknown;
	case AKEYCODE_SEARCH: return InputButton::ButtonUnknown;
	case AKEYCODE_MEDIA_PLAY_PAUSE: return InputButton::ButtonUnknown;
	case AKEYCODE_MEDIA_STOP: return InputButton::ButtonUnknown;
	case AKEYCODE_MEDIA_NEXT: return InputButton::ButtonUnknown;
	case AKEYCODE_MEDIA_PREVIOUS: return InputButton::ButtonUnknown;
	case AKEYCODE_MEDIA_REWIND: return InputButton::ButtonUnknown;
	case AKEYCODE_MEDIA_FAST_FORWARD: return InputButton::ButtonUnknown;
	case AKEYCODE_MUTE: return InputButton::ButtonUnknown;
	case AKEYCODE_PAGE_UP: return InputButton::ButtonUnknown;
	case AKEYCODE_PAGE_DOWN: return InputButton::ButtonUnknown;
	case AKEYCODE_PICTSYMBOLS: return InputButton::ButtonUnknown;
	case AKEYCODE_SWITCH_CHARSET: return InputButton::ButtonUnknown;

	case AKEYCODE_UNKNOWN:
	default:
		return InputButton::ButtonUnknown;
	}
}

static void HandleLocalNotification(Bool bWasInForeground, String sUserData)
{
	// Note: notifications may start an activity, which means the Engine may
	// not be ready yet.

	// Parse the user data into a DataStore
	DataStore userData;
	if (!DataStoreParser::FromString(sUserData, userData))
	{
		DataStore emptyDataStore;
		userData.Swap(emptyDataStore);
	}

	g_pEngineVirtuals->OnReceivedOSNotification(
		false,  // bRemoteNotification
		false,  // bWasRunning TODO: Set this properly
		bWasInForeground,
		userData,
		sUserData);
}

static void StartSession()
{
	if (!s_bIsActive)
	{
		s_bIsActive = true;

		if (Game::Main::Get())
		{
			Game::Main::Get()->OnSessionStart();
		}
	}

	// Update focus level and apply.
	s_iFocusLevel = ((Int)s_iFocusLevel) & ~kLevelSuspended;
	ApplyFocusLevel();
}

static void EndSession(WorldTime timeStamp)
{
	// Update focus level and apply.
	s_iFocusLevel = ((Int)s_iFocusLevel) | kLevelSuspended;
	ApplyFocusLevel();

	if (s_bIsActive)
	{
		if (Game::Main::Get())
		{
			Game::Main::Get()->OnSessionEnd(timeStamp);
		}

		s_bIsActive = false;
	}
}

void RenderThreadRequestRedraw(Bool bFinishGl)
{
	SEOUL_ASSERT(IsRenderThread());

	// Can't redraw with no render device.
	if (!OGLES2RenderDevice::Get().IsValid())
	{
		return;
	}

	// Start the redraw process.
	auto& r = GetOGLES2RenderDevice();
	if (!r.RedrawBegin())
	{
		return;
	}

	// First, attempt to do the redraw by
	// resubmitting the last command stream - if this fails,
	// just clear to black.
	if (!Renderer::Get() || !Renderer::Get()->RenderThread_ResubmitLast())
	{
		r.RedrawBlack();
	}

	// Done with the redraw.
	r.RedrawEnd(bFinishGl);
}

void RenderThreadUpdateWindow(ANativeWindow* pMainWindow)
{
	SEOUL_ASSERT(IsRenderThread());

	if (OGLES2RenderDevice::Get().IsValid())
	{
		GetOGLES2RenderDevice().UpdateWindow(pMainWindow);
	}
}

void RenderThreadUpdateWindowAndRequestRedraw(ANativeWindow* pMainWindow)
{
	SEOUL_ASSERT(IsRenderThread());

	if (OGLES2RenderDevice::Get().IsValid())
	{
		GetOGLES2RenderDevice().UpdateWindow(pMainWindow);
		RenderThreadRequestRedraw(true);
	}
}

#if SEOUL_LOGGING_ENABLED
static String AppCommandToString(Int32 iCommand)
{
#	define SEOUL_CASE(n) case n: return String(#n)
	switch (iCommand)
	{
	SEOUL_CASE(APP_CMD_INPUT_CHANGED);
	SEOUL_CASE(APP_CMD_INIT_WINDOW);
	SEOUL_CASE(APP_CMD_TERM_WINDOW);
	SEOUL_CASE(APP_CMD_WINDOW_RESIZED);
	SEOUL_CASE(APP_CMD_WINDOW_REDRAW_NEEDED);
	SEOUL_CASE(APP_CMD_CONTENT_RECT_CHANGED);
	SEOUL_CASE(APP_CMD_GAINED_FOCUS);
	SEOUL_CASE(APP_CMD_LOST_FOCUS);
	SEOUL_CASE(APP_CMD_CONFIG_CHANGED);
	SEOUL_CASE(APP_CMD_LOW_MEMORY);
	SEOUL_CASE(APP_CMD_START);
	SEOUL_CASE(APP_CMD_RESUME);
	SEOUL_CASE(APP_CMD_SAVE_STATE);
	SEOUL_CASE(APP_CMD_PAUSE);
	SEOUL_CASE(APP_CMD_STOP);
	SEOUL_CASE(APP_CMD_DESTROY);
	default:
		return String::Printf("APP_CMD_<%d>", iCommand);
	};
#	undef SEOUL_CASE
}
#endif // /#if SEOUL_LOGGING_ENABLED

#if SEOUL_UNIT_TESTS
Int AppAndroidRunUnitTests(
	android_app* pAndroidApp,
	const String& sBaseDirectoryPath,
	Byte const* sOptionalTestNameArgument);
static void RunUnitTests(android_app* pAndroidApp)
{
	// Do the unit test run.
	auto const iResult = AppAndroidRunUnitTests(
		pAndroidApp,
		g_InternalStorageDirectoryString.Data(),
		"");
	if (0 != iResult)
	{
		SEOUL_WARN("Unit tests exited with code: %d", iResult);
	}
}
#endif // /#if SEOUL_UNIT_TESTS

/** Common point for cleaning up app and engine. */
static void DoAppShutdown()
{
	// Check if already terminated.
	if (!Engine::Get())
	{
		s_bShouldTickAppAndroid = false; // Sanity.
		return;
	}

	SEOUL_LOG("%s: Starting app shutdown", __FUNCTION__);

	// Clear out the main thread job queue on shutdown
	RunMainThreadJobs();

	s_bShouldTickAppAndroid = false;
	SEOUL_LOG("%s: Destroying game tier", __FUNCTION__);
	DestroyAppAndroid();
	SEOUL_LOG("%s: Destroying engine tier", __FUNCTION__);
	DestroyAppAndroidEngine();
	SEOUL_LOG("%s: Flushing callbacks", __FUNCTION__);

	// Clear the AndroidApp cached value.
	s_pAndroidApp = nullptr;

	// Reset thread ids
	ResetAllFixedThreadIds();

	// TODO: Seeing inconsistent behavior at shutdown
	// on some devices on our test service. Some will
	// just terminate the app on call to finish(),
	// not giving enough time for our Exit() handling to engage.
	//
	// Not sure that's important/that I care given the nature of app
	// lifecycle on Android. So, I'm logging the exit signal
	// here to avoid the spurious failures. May need to re-evaluate
	// if we discover an exit hang/bug post this point.
	PlatformPrint::PrintString(
		PlatformPrint::Type::kInfo,
		"SEOUL-ENGINE-EXIT");
}

} // namespace Seoul

extern "C"
{

static void InternalStaticHandleApplicationCommand(
	android_app* pAndroidApp,
	int32_t iCommand)
{
	using namespace Seoul;

	SEOUL_LOG("%s", AppCommandToString(iCommand).CStr());

	// See also:
	// https://developer.nvidia.com/sites/default/files/akamai/mobile/docs/android_lifecycle_app_note.pdf
	// http://developer.android.com/reference/android/app/NativeActivity.html
	// http://developer.android.com/training/basics/activity-lifecycle/index.html
	// http://android-developers.blogspot.com/2011/11/making-android-games-that-play-nice.html
	switch (iCommand)
	{
	case APP_CMD_INIT_WINDOW: // fall-through
	case APP_CMD_WINDOW_RESIZED: // fall-through
	case APP_CMD_CONTENT_RECT_CHANGED:
		s_bShouldTickAppAndroid = true;

		if (OGLES2RenderDevice::Get().IsValid())
		{
			Jobs::AwaitFunction(
				GetRenderThreadId(),
				&RenderThreadUpdateWindowAndRequestRedraw,
				pAndroidApp->window);
		}
		break;

	case APP_CMD_TERM_WINDOW:
		if (OGLES2RenderDevice::Get().IsValid())
		{
			Jobs::AwaitFunction(
				GetRenderThreadId(),
				&RenderThreadUpdateWindow,
				(ANativeWindow*)nullptr);
		}
		break;

	case APP_CMD_LOST_FOCUS:
		s_iFocusLevel = ((Int)s_iFocusLevel) | kLevelUnfocused;
		ApplyFocusLevel();
		break;
	case APP_CMD_GAINED_FOCUS:
		s_iFocusLevel = ((Int)s_iFocusLevel) & ~kLevelUnfocused;
		ApplyFocusLevel();
		break;

	case APP_CMD_PAUSE:
		s_iFocusLevel = ((Int)s_iFocusLevel) | kLevelPaused;
		ApplyFocusLevel();
		break;
	case APP_CMD_RESUME:
		s_iFocusLevel = ((Int)s_iFocusLevel) & ~kLevelPaused;
		ApplyFocusLevel();
		break;

	case APP_CMD_STOP:
		// Nop
		break;

	case APP_CMD_START:
		// Nop
		break;

	case APP_CMD_DESTROY:
		s_bShouldTickAppAndroid = false;
		break;

	case APP_CMD_WINDOW_REDRAW_NEEDED:
		if (OGLES2RenderDevice::Get().IsValid())
		{
			Jobs::AwaitFunction(
				GetRenderThreadId(),
				&RenderThreadRequestRedraw,
				true);
		}
		break;

	case APP_CMD_CONFIG_CHANGED:
		// Nop
		break;

	default:
		break;
	};
}

static int32_t InternalStaticHandleInputEvent(
	android_app* pAndroidApp,
	AInputEvent* pInputEvent)
{
	using namespace Seoul;

	switch (AInputEvent_getType(pInputEvent))
	{
		// Handle motion events, and also implement touch slop (since
		// the events we receive via AMotionEvent_get* are raw and do not
		// factor in touch slop internally). The idea is, once a touch event
		// begins (we receive an AMOTION_EVENT_ACTION_DOWN), we do not report
		// move events until the point has moved at least g_iTouchSlop from the initial
		// touch point. Once we reach that point, we continuously report motion therafter.
	case AINPUT_EVENT_TYPE_MOTION:
		{
			// Gather information about the event.
			auto const iSupported = (Int)InputButton::TouchButtonLast - (Int)InputButton::TouchButtonFirst + 1;
			auto const iAction = AMotionEvent_getAction(pInputEvent);
			auto const uFlags = (UInt32)iAction & AMOTION_EVENT_ACTION_MASK;
			auto const iIndex = ((iAction & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
			auto const iId = AMotionEvent_getPointerId(pInputEvent, iIndex);

			// TODO: Verify that pointer ID will
			// always be contiguous and positive, starting at
			// 0.

			// Early out if the pointer is an invalid index.
			if (iId < 0 || iId >= iSupported)
			{
				return 1;
			}

			// Touch identifier.
			auto const eTouch = (InputButton)(iId + (Int)InputButton::TouchButtonFirst);

			// Position of the changed touch.
			Float const fX = AMotionEvent_getX(pInputEvent, iIndex);
			Float const fY = AMotionEvent_getY(pInputEvent, iIndex);
			switch (uFlags)
			{
			case AMOTION_EVENT_ACTION_DOWN: // fall-through
			case AMOTION_EVENT_ACTION_POINTER_DOWN:
				pAndroidApp->m_afLastTouchX[iId] = fX;
				pAndroidApp->m_afLastTouchY[iId] = fY;
				pAndroidApp->m_abInInitialTouchSlop[iId] = true;
				QueueTouchMoveEvent(eTouch, Point2DInt((Int)fX, (Int)fY));
				QueueTouchButtonEvent(eTouch, true);
				break;
			case AMOTION_EVENT_ACTION_UP: // fall-through
			case AMOTION_EVENT_ACTION_POINTER_UP:
				if (pAndroidApp->m_abInInitialTouchSlop[iId])
				{
					QueueTouchMoveEvent(eTouch, Point2DInt(
						(Int)pAndroidApp->m_afLastTouchX[iId],
						(Int)pAndroidApp->m_afLastTouchY[iId]));
					pAndroidApp->m_abInInitialTouchSlop[iId] = false;
				}
				else
				{
					QueueTouchMoveEvent(eTouch, Point2DInt((Int)fX, (Int)fY));
				}
				QueueTouchButtonEvent(eTouch, false);
				break;
			case AMOTION_EVENT_ACTION_MOVE:
				{
					if (pAndroidApp->m_abInInitialTouchSlop[iId])
					{
						Float const fDiff = Vector2D(
							fX - pAndroidApp->m_afLastTouchX[iId],
							fY - pAndroidApp->m_afLastTouchY[iId]).Length();
						pAndroidApp->m_abInInitialTouchSlop[iId] = ((Int)fDiff <= g_iTouchSlop);
					}

					if (!pAndroidApp->m_abInInitialTouchSlop[iId])
					{
						QueueTouchMoveEvent(eTouch, Point2DInt((Int)fX, (Int)fY));
					}
				}
				break;
			default:
				// Ignore the event.
				break;
			};
		}
		return 1;

	case AINPUT_EVENT_TYPE_KEY:
		{
			// Handle keyboard input.  Note that this also handles other button
			// input, like the Back button, and by intercepting that here, we
			// prevent the normal onBackPressed() handler from getting called
			// etc.
			int32_t iAction = AKeyEvent_getAction(pInputEvent);
			Bool const bPressed = (iAction == AKEY_EVENT_ACTION_DOWN);
			int32_t iKeyCode = AKeyEvent_getKeyCode(pInputEvent);

			InputButton eButton = TranslateAndroidKeyCodeToSeoulButton(iKeyCode);
			if (eButton != InputButton::ButtonUnknown)
			{
				UInt uVKCode = InputManager::GetVKCodeForInputButton(eButton);
				QueueKeyboardEvent(uVKCode, bPressed);

				return 1;
			}
			else
			{
				SEOUL_LOG("Unknown button pressed: %d\n", iKeyCode);
			}
		}
		break;

	default:
		break;
	};

	return 0;
}

// Convenience utility for forcing app exit.
static void Exit(android_app* pAndroidApp, Seoul::Bool bAutomationExit)
{
	using namespace Seoul;

	SEOUL_LOG("In final Exit()");

	// Explicitly report exit - used by test harnesses.
	if (bAutomationExit)
	{
		PlatformPrint::PrintString(
			PlatformPrint::Type::kInfo,
			"SEOUL-ENGINE-EXIT");

		// Give some time for the automation harness to notice.
		Thread::Sleep(1000);
	}

	// Attach a Java environment to the current thread.
	{
		ScopedJavaEnvironment scope;
		JNIEnv *pEnvironment = scope.GetJNIEnv();

		// Tell the Java layer to exit.
		Java::Invoke<void>(
			pEnvironment,
			pAndroidApp->activity->clazz,
			"Exit",
			"()V");
	}

	// See: https://groups.google.com/forum/#!topic/android-ndk/PgZhN5h1x8o
	//
	// Unfortunately, a shared library loaded via the Java System.loadLibrary() is not guaranteed
	// to be unloaded when the app goes into the destroyed state. We force the issue here by calling the
	// exit() function, to ensure global variables are reinitialized if the library is needed again.
	//
	// Given how the App is structured, there is no practical difference for the user. The vast
	// majority of startup time is spent in global singleton initialization, which happens
	// whether we call exit(0) here or not.
	exit(0);
}

void android_main(android_app* pAndroidApp)
{
	using namespace Seoul;

	// Lock s_MainMutex for the body of this function - on
	// some devices (e.g. GT-P3100), android_main is becomming
	// re-entrant from a secondary thread before the first
	// thread has completed. Likely this is due to rapid onDestroy()/onCreate()
	// cycling.
	//
	// Note that a possible alternative fix here is to instead
	// immediately exit from the new android_main() if a previous
	// android_main() is still active.
	Lock lock(s_MainMutex);

	// Allow JNI invocation on this thread.
	SetupJNI(pAndroidApp);

	// Load command-line arguments, may have been received via an intent from
	// Java.
	{
		DataStore dataStore;
		if (DataStoreParser::FromString(g_CommandlineArguments.Data(), StrLen(g_CommandlineArguments.Data()), dataStore))
		{
			// Array of arguments.
			UInt32 uArrayCount = 0u;
			(void)dataStore.GetArrayCount(dataStore.GetRootNode(), uArrayCount);

			// Assemble.
			Vector<String> vs;
			vs.Reserve(uArrayCount);
			for (UInt32 i = 0u; i < uArrayCount; ++i)
			{
				String sArgument;
				DataNode value;
				if (dataStore.GetValueFromArray(dataStore.GetRootNode(), i, value) &&
					dataStore.AsString(value, sArgument))
				{
					vs.PushBack(sArgument);
				}
			}

			// Process.
			(void)Reflection::CommandLineArgs::Parse(vs.Begin(), vs.End());
		}
	}

	// Query for script to run.
	String sAutomationScript;
	Bool bRunUnitTests = false;
	(void)bRunUnitTests;

#if !SEOUL_SHIP || defined(SEOUL_PROFILING_BUILD)
	// Must do this carefully - no core or engine startup yet, so much
	// of our backbone (FilePath, GamePaths) do not exist yet.
	// Give Java a bit to finish starting up.
	while (!g_bCanPerformNativeStartup) { Thread::Sleep(1); }
	if (DiskSyncFile::FileExists(g_SourceDir.Data()))
	{
		AndroidZipFileSystem fileSystem(g_SourceDir.Data());

		void* p = nullptr;
		UInt32 u = 0u;
		if (fileSystem.ReadAll(
			Path::Combine("Data", "7f1f95f02b694b1487c3020d324fc93c5ec035be291e4860a5fe97f22387e49a"),
			p,
			u,
			0u,
			MemoryBudgets::TBD))
		{
			sAutomationScript = TrimWhiteSpace(String((Byte const*)p, u));
		}
		MemoryManager::Deallocate(p); p = nullptr;

		bRunUnitTests = (sAutomationScript == "run_unit_tests");
	}
#endif // /#if !SEOUL_SHIP || defined(SEOUL_PROFILING_BUILD)

	// In developer builds, check for and run as a unit test build.
	// Exit immediately after execution.
#if SEOUL_UNIT_TESTS
	if (bRunUnitTests)
	{
		// Give Java a bit to finish starting up.
		while (!g_bCanPerformNativeStartup) { Thread::Sleep(1); }

		// Execute.
		RunUnitTests(pAndroidApp);

		// Exit.
		Exit(pAndroidApp, true);
		return;
	}
#endif // /#if SEOUL_UNIT_TESTS

	s_bInBackground = false;
	s_iFocusLevel = 0;
	s_iLastKeyboardStateChange = -1;

	s_pAndroidApp = pAndroidApp;
	pAndroidApp->onAppCmd = &InternalStaticHandleApplicationCommand;
	pAndroidApp->onInputEvent = &InternalStaticHandleInputEvent;

	// Give Java a bit to finish starting up. Process
	// events so we properly handle system events
	// while we're waiting.
	{
		android_poll_source* pSource = nullptr;
		Int iEvent = -1;
		Int iPollEvent = -1;
		while (!g_bCanPerformNativeStartup)
		{
			while ((iPollEvent = ALooper_pollAll(GetAppWaitInMilliseconds(), nullptr, &iEvent, (void**)&pSource)) >= 0)
			{
				if (nullptr != pSource)
				{
					pSource->process(pAndroidApp, pSource);
				}

				// Exit requested while waiting for startup, cleanup.
				if (pAndroidApp->destroyRequested)
				{
					// Reset the ticking flag.
					s_bShouldTickAppAndroid = false;

					// Clear the AndroidApp cached value.
					s_pAndroidApp = nullptr;

					// Trigger an exit.
					Exit(pAndroidApp, false);
					return;
				}
			}
		}
	}

	// Clear the native startup flag.
	g_bCanPerformNativeStartup = false;

	// Initialize engine singletons.
	CreateAppAndroidEngine(
		sAutomationScript,
		pAndroidApp->window,
		g_InternalStorageDirectoryString.Data());
	CreateAppAndroid(sAutomationScript);

	// Flag to track whether we've started ticking AppAndroid or not.
	Bool bStartedTickingAppAndroid = false;

	// Game loop
	while (true)
	{
		// Mark the start of one loop - used in the mini tick/render
		// loop we do if AppAndroid hasn't been initialized yet.
		Int64 const iStartTimeInTicks = SeoulTime::GetGameTimeInTicks();

		android_poll_source* pSource = nullptr;
		Int iEvent = -1;
		Int iPollEvent = -1;

		while ((iPollEvent = ALooper_pollAll(GetAppWaitInMilliseconds(), nullptr, &iEvent, (void**)&pSource)) >= 0)
		{
			if (bStartedTickingAppAndroid)
			{
				// Handle the main thread job queue now
				RunMainThreadJobs();
			}

			if (nullptr != pSource)
			{
				pSource->process(pAndroidApp, pSource);
			}

			if (pAndroidApp->destroyRequested)
			{
				// Cleanup app.
				DoAppShutdown();

				// Trigger an exit.
				Exit(pAndroidApp, g_bRunningAutomatedTests);
				return;
			}
		}

		// If a CreateAppAndroid action is pending, perform it now.
		if (s_bShouldTickAppAndroid && !bStartedTickingAppAndroid)
		{
			bStartedTickingAppAndroid = true;
		}

		if (bStartedTickingAppAndroid)
		{
			// Handle the main thread job queue now
			RunMainThreadJobs();

			// Update keyboard state now and apply focus level.
			//
			// NOTE: We intentionally want this to be tied to engine tick, to
			// give command processing a chance to recognize a return from background
			// to foreground before we update keyboard status, to avoid a single frame
			// of "enterbackground/leavebackground" when the keyboard is dismissed.
			if (Engine::Get())
			{
				if (Engine::Get()->IsEditingText() != (kLevelKeyboard == (kLevelKeyboard & s_iFocusLevel)))
				{
					// Track time if not tracking.
					if (s_iLastKeyboardStateChange < 0)
					{
						s_iLastKeyboardStateChange = SeoulTime::GetGameTimeInTicks();
					}

					// Two possibilities:
					// - if we're editing text, apply that change immediately.
					// - if we've stopped editing text, wait to apply that change. This
					//   suppresses spurious background/foreground events in the interval
					//   while the keyboard is dismissing.
					if (Engine::Get()->IsEditingText() ||
						SeoulTime::ConvertTicksToMilliseconds(SeoulTime::GetGameTimeInTicks() - s_iLastKeyboardStateChange) >= 500.0)
					{
						// Reset.
						s_iLastKeyboardStateChange = -1;

						if (Engine::Get()->IsEditingText())
						{
							s_iFocusLevel = ((Int)s_iFocusLevel) | kLevelKeyboard;
						}
						else
						{
							s_iFocusLevel = ((Int)s_iFocusLevel) & ~kLevelKeyboard;
						}

						ApplyFocusLevel();
					}
				}
			}

			// Tick if we're not in the background.
			if (!s_bInBackground)
			{
				if (AppAndroid::Get())
				{
					AppAndroid::Get()->Tick();
				}
			}
		}
		// Loop to make sure main thread jobs are running and we're refreshing the
		// viewport while waiting for full startup.
		else
		{
			// If we're not in the background, clear the screen.
			//
			// TODO: Insert a full screen quad draw of some builtin
			// image here, to mimic iOS behavior.
			if (!s_bInBackground)
			{
				if (OGLES2RenderDevice::Get().IsValid())
				{
					Jobs::AwaitFunction(
						GetRenderThreadId(),
						&RenderThreadRequestRedraw,
						false);
				}

				// Tick HTTP so that analytics events for the downloader can be sent.
				if (HTTP::Manager::Get().IsValid())
				{
					HTTP::Manager::Get()->Tick();
				}

				// Cap the frame time - this is a mini version of what
				// happens in AppAndroid::Get()->Tick. We only do this
				// while waiting for conditions to be met to initialize
				// AppAndroid.
				Double fRemainingFrameTimeInMs = 0.0;
				do
				{
					// Yield some thread time and sleep to avoid consuming too
					// much battery (and let the file IO threads get as much
					// time as possible).
					if (Jobs::Manager::Get())
					{
						Jobs::Manager::Get()->YieldThreadTime();
					}

					auto const maxFPS = (RenderDevice::Get() ? RenderDevice::Get()->GetDisplayRefreshRate() : RefreshRate(60.0, 1.0));
					if (!maxFPS.IsZero())
					{
						auto const fMaxFPS = maxFPS.ToHz();
						fRemainingFrameTimeInMs = (((1.0 / fMaxFPS) * 1000.0) - SeoulTime::ConvertTicksToMilliseconds(SeoulTime::GetGameTimeInTicks() - iStartTimeInTicks));
					}
					else
					{
						fRemainingFrameTimeInMs = 0.0;
					}
				} while (fRemainingFrameTimeInMs > 0.0) ;
			}
		}
	}
}

JNIEXPORT void JNICALL SEOUL_ACTIVITY_JNI_FUNC(NativeHandleLocalNotification)(
	JNIEnv* pJniEnvironment,
	jclass,
	jboolean bWasInForeground,
	jstring sJavaUserData)
{
	using namespace Seoul;

	String sUserData;
	SetStringFromJava(pJniEnvironment, sJavaUserData, sUserData);

	RunOnMainThread(
		&HandleLocalNotification,
		(Bool)bWasInForeground,
		sUserData);
}

JNIEXPORT void JNICALL SEOUL_ACTIVITY_JNI_FUNC(NativeOnSessionStart)(
	JNIEnv*,
	jclass)
{
	using namespace Seoul;

	SEOUL_LOG("NativeOnSessionStart()");
	RunOnMainThread(&StartSession);
}

JNIEXPORT void JNICALL SEOUL_ACTIVITY_JNI_FUNC(NativeOnSessionEnd)(
	JNIEnv*,
	jclass)
{
	using namespace Seoul;

	SEOUL_LOG("NativeOnSessionEnd()");
	RunOnMainThread(&EndSession, Game::Client::StaticGetCurrentServerTime());
}

} // extern "C"
