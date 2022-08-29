/**
 * \file Engine.cpp
 * \brief Engine is the root singleton for Engine and Core project code.
 * All managers defined in the Engine project or Core are owned
 * by Engine. Once Engine has been constructed and Initialize()d,
 * Core and Engine singletons are available for use.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "AchievementManager.h"
#include "AnalyticsManager.h"
#include "ApplicationJson.h"
#include "AssetManager.h"
#include "BuildChangelistPublic.h"
#include "BuildVersion.h"
#include "CommerceManager.h"
#include "ContentLoadManager.h"
#include "CookManager.h"
#include "CookManagerMoriarty.h"
#include "CoreVirtuals.h"
#include "DeveloperCommerceManager.h"
#include "Directory.h"
#include "DownloadablePackageFileSystem.h"
#include "EffectManager.h"
#include "Engine.h"
#include "EngineCommandLineArgs.h"
#include "FacebookManager.h"
#include "FileManager.h"
#include "EventsManager.h"
#include "GamePaths.h"
#include "HTTPManager.h"
#include "JobsFunction.h"
#include "IFileSystem.h"
#include "InputManager.h"
#include "ITextEditable.h"
#include "JobsManager.h"
#include "LocManager.h"
#include "Logger.h"
#include "MapFileAsync.h"
#include "MapFileDbgHelp.h"
#include "MapFileLinux.h"
#include "MaterialManager.h"
#include "MoriartyClient.h"
#include "Mutex.h"
#include "Path.h"
#include "PlatformData.h"
#include "PlatformSignInManager.h"
#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionDataStoreTableUtil.h"
#include "ReflectionDefine.h"
#include "RenderDevice.h"
#include "Renderer.h"
#include "SaveLoadManager.h"
#include "SeoulTime.h"
#include "SettingsManager.h"
#include "SoundManager.h"
#include "TextureManager.h"
#include "TrackingManager.h"

namespace Seoul
{

SEOUL_LINK_ME(class, EngineCommandLineArgs);
#if SEOUL_ENABLE_CHEATS
SEOUL_LINK_ME(class, EngineCommands);
#endif // /#if SEOUL_ENABLE_CHEATS

// TODO: These values are reported directly analytics. That is not ideal.

// WARNING: Do not rename - analytics depends on these strings.
SEOUL_BEGIN_ENUM(NetworkConnectionType)
	SEOUL_ENUM_N("Unknown", NetworkConnectionType::kUnknown)
	SEOUL_ENUM_N("WiFi", NetworkConnectionType::kWiFi)
	SEOUL_ENUM_N("Mobile", NetworkConnectionType::kMobile)
	SEOUL_ENUM_N("Wired", NetworkConnectionType::kWired)
SEOUL_END_ENUM()

#if SEOUL_WITH_REMOTE_NOTIFICATIONS
SEOUL_BEGIN_ENUM(RemoteNotificationType)
	SEOUL_ENUM_N("ADM", RemoteNotificationType::kADM)
	SEOUL_ENUM_N("FCM", RemoteNotificationType::kFCM)
	SEOUL_ENUM_N("IOS", RemoteNotificationType::kIOS)
SEOUL_END_ENUM()
#endif // /SEOUL_WITH_REMOTE_NOTIFICATIONS

#if SEOUL_WITH_MORIARTY
/**
 * Handler to be registered with the MoriartyClient if the current platform
 * uses Moriarty. This handler will map any keyboard key events received
 * over RPC to all keyboard devices registered with InputManager.
 */
void MoriartyKeyboardKeyEventHandler(const MoriartyRPC::KeyEvent& keyEvent)
{
	for (UInt i = 0; i < InputManager::Get()->GetNumDevices(); i++)
	{
		InputDevice* pDevice = InputManager::Get()->GetDevice(i);
		if (pDevice->GetDeviceType() == InputDevice::Keyboard)
		{
			if (keyEvent.m_eKeyEventType == MoriartyRPC::kKeyAllReleased)
			{
				pDevice->OnLostFocus();
			}
			else
			{
				pDevice->QueueKeyEvent(keyEvent.m_uVirtualKeyCode, (MoriartyRPC::kKeyPressed == keyEvent.m_eKeyEventType));
			}
		}
	}
}

/**
 * Handler to be registered with the MoriartyClient if the current platform
 * uses Moriarty. This handler will map any keyboard key events received
 * over RPC to all keyboard devices registered with InputManager.
 */
void MoriartyKeyboardCharEventHandler(UniChar character)
{
	if (!IsMainThread())
	{
		Jobs::AsyncFunction(
			GetMainThreadId(),
			&MoriartyKeyboardCharEventHandler,
			character);
		return;
	}

	if (Engine::Get() && Engine::Get()->GetTextEditable())
	{
		Engine::Get()->GetTextEditable()->TextEditableApplyChar(character);
	}
}
#endif

////////// Map file options //////////
#if SEOUL_ENABLE_STACK_TRACES

/** Map file subclass we use */
#if SEOUL_PLATFORM_WINDOWS
typedef MapFileDbgHelp MapFileClass;
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_LINUX
typedef MapFileLinux MapFileClass;
#else
typedef MapFileAsync MapFileClass;
#endif

#endif // /SEOUL_ENABLE_STACK_TRACES

/**
 * Hook for g_pCoreVirtuals, must be explicitly assigned in main process code.
 */
String Engine::CoreGetPlatformUUID()
{
	if (Engine::Get())
	{
		return Engine::Get()->GetPlatformUUID();
	}
	else
	{
		return g_DefaultCoreVirtuals.GetPlatformUUID();
	}
}

/**
 * Hook for g_pCoreVirtuals, must be explicitly assigned in main process code.
 */
TimeInterval Engine::CoreGetUptime()
{
	if (Engine::Get())
	{
		return Engine::Get()->GetUptime();
	}
	else
	{
		return g_DefaultCoreVirtuals.GetUptime();
	}
}

Engine::Engine()
	: m_pTextEditable(nullptr)
	, m_pPauseTimer(SEOUL_NEW(MemoryBudgets::TBD) SeoulTime)
	, m_iPauseTimeInTicks(0)
	, m_uFrameCount(0u)
	, m_iFrameStartTicks(0)
	, m_fUnfixedSeconds(0.0)
	, m_fTickSeconds(0.0)
	, m_fTickSecondsScale(1.0)
	, m_fFixedSecondsInTick(0.0)
	, m_fTotalSeconds(0.0)
	, m_fTotalGameSeconds(0.0)
	, m_pPlatformDataMutex(SEOUL_NEW(MemoryBudgets::TBD) Mutex)
	, m_pPlatformData(SEOUL_NEW(MemoryBudgets::TBD) PlatformData)
	, m_iStartUptimeInMilliseconds(0)
	, m_iUptimeInMilliseconds(0)
	, m_pUptimeMutex(SEOUL_NEW(MemoryBudgets::TBD) Mutex)
	, m_pUptimeSignal(SEOUL_NEW(MemoryBudgets::TBD) Signal)
	, m_pUptimeThread()
	, m_bUptimeThreadRunning(true)
	, m_pAnalyticsManager()
	, m_pAssetManager()
	, m_pCommerceManager()
	, m_pSettingsManager()
	, m_pJobManager()
	, m_pContentLoadManager()
	, m_pCookManager()
	, m_pTextureManager()
	, m_pMaterialManager()
	, m_pEffectManager()
	, m_pRenderer()
	, m_pSoundManager()
	, m_eActiveMouseCursor(MouseCursor::kArrow)
	, m_bInitialized(false)
	, m_PauseTimerActive()
	, m_bSuppressOpenURL(false)
#if SEOUL_WITH_REMOTE_NOTIFICATIONS
	, m_pRemoteNotificationTokenMutex(SEOUL_NEW(MemoryBudgets::TBD) Mutex)
	, m_RemoteNotificationToken()
	, m_DisplayRemoteNotificationToken(Ternary::Unset)
#endif // #if SEOUL_WITH_REMOTE_NOTIFICATIONS
{
}

Engine::~Engine()
{
	// Engine::Shutdown() must have been called or it is a usage error.
	SEOUL_ASSERT(!m_bInitialized);
}

/**
 * Populate rPlatformData with the current platform's runtime data.
 */
void Engine::GetPlatformData(PlatformData& rData) const
{
	Lock lock(*m_pPlatformDataMutex);
	rData = *m_pPlatformData;
}

// TODO: Decide if this is the desired API of it should be abstracted further.
Bool Engine::IsSamsungPlatformFlavor() const
{
	PlatformData data;
	GetPlatformData(data);

	return (Seoul::IsSamsungPlatformFlavor(data.m_eDevicePlatformFlavor));
}

Bool Engine::IsAmazonPlatformFlavor() const
{
	PlatformData data;
	GetPlatformData(data);

	return (Seoul::IsAmazonPlatformFlavor(data.m_eDevicePlatformFlavor));
}

Bool Engine::IsGooglePlayPlatformFlavor() const
{
	PlatformData data;
	GetPlatformData(data);

	return (Seoul::IsGooglePlayPlatformFlavor(data.m_eDevicePlatformFlavor));
}

/** Convenience, access to the m_sPlatformUUID member of this Engine's platform data. */
String Engine::GetPlatformUUID() const
{
	String sReturn;
	{
		Lock lock(*m_pPlatformDataMutex);
		sReturn = m_pPlatformData->m_sPlatformUUID;
	}

	return sReturn;
}

String Engine::GetLoadShedPlatformUuidHash() const
{
	// Salt the platform UUID with the current date, so players get different load
	// shedding luck every day (rather than always shedding the same players first)
	Int dayNumber = (Int)WorldTime::GetUTCTime().GetSeconds() / WorldTime::kDaysToSeconds;
	auto sSaltedUuid = String::Printf("%d%s%d", dayNumber, Engine::Get()->GetPlatformUUID().CStr(), dayNumber);

	// Return hash as a padded string, from 00 to 99
	auto uHash = GetHash(sSaltedUuid) % 100;
	return String::Printf("%02d", uHash);
}

/**
 * @return true and populate rsOutput with the select output file, on supported platforms and without
 * user cancellation. Otherwise, return false.
 *
 * This is a convenient variation of DisplayFileDialogSingleSelection that takes a single FileType
 * and GameDirectory for selection purposes. If rInputOutput is defined, the working directory of
 * the selection will be the directory of rInputOutput, otherwise it will be eWorkingDirectory.
 */
Bool Engine::DisplayFileDialogSingleSelection(
	FilePath& rInputOutput,
	FileDialogOp eOp,
	FileType eFileType,
	GameDirectory eWorkingDirectory) const
{
	// Compute working directory.
	String sWorkingDirectory;
	if (rInputOutput.IsValid())
	{
		auto const sExistingDirectory = Path::GetDirectoryName(rInputOutput.GetAbsoluteFilenameInSource());
		if (Directory::DirectoryExists(sExistingDirectory))
		{
			sWorkingDirectory = sExistingDirectory;
			eWorkingDirectory = rInputOutput.GetDirectory();
		}
	}
	if (sWorkingDirectory.IsEmpty())
	{
		FilePath filePath;
		filePath.SetDirectory(eWorkingDirectory);
		sWorkingDirectory = filePath.GetAbsoluteFilenameInSource();
	}

	// Compute friendly name and search filter for the file type.
	String const sFriendlyName(FileType::kUnknown == eFileType
		? String()
		: EnumToString<FileType>(eFileType));
	String const sFilter(FileType::kUnknown == eFileType
		? String()
		: "*" + FileTypeToSourceExtension(eFileType));

	// Perform the operation.
	String sPath;
	if (DisplayFileDialogSingleSelection(
		sPath,
		eOp,
		{ { sFriendlyName, sFilter } },
		sWorkingDirectory))
	{
		rInputOutput = FilePath::CreateFilePath(eWorkingDirectory, sPath);

		// Allow the user to specify a filename without an extension if the save case.
		if (FileDialogOp::kSave == eOp &&
			FileType::kUnknown != eFileType &&
			rInputOutput.GetType() == FileType::kUnknown)
		{
			rInputOutput.SetType(eFileType);
		}

		return true;
	}

	return false;
}

/**
 * Utility function, by default, this is a nop and always returns false. On
 * platforms which support it, this method will attempt to open a URL in the platform's
 * default web browser.
 *
 * @return True if the URL was opened, false otherwise.
 */
Bool Engine::OpenURL(const String& sURL)
{
	if (m_bSuppressOpenURL)
	{
		return false;
	}

	SEOUL_LOG_ENGINE("OpenURL: %s", sURL.CStr());

#if SEOUL_WITH_HELPSHIFT
	// Special handling for helpshift:// URLs.
	if (sURL.StartsWith("helpshift://"))
	{
#if SEOUL_AUTO_TEST
		return false;
#else
		return TrackingManager::Get()->OpenThirdPartyURL(sURL);
#endif
	}
#endif

	// Pass to specialization implementation.
	return InternalOpenURL(sURL);
}

/** Body of the worker thread that updates (periodically) the uptime value. */
Int Engine::UptimeWorker(const Thread& thread)
{
	// Period that we refresh uptime.
	static const UInt32 kuUptimeRefreshMilliseconds = 100u;

	while (m_bUptimeThreadRunning)
	{
		RefreshUptime();

		// Go to sleep for ever if we're paused.
		if (0 != m_PauseTimerActive)
		{
			m_pUptimeSignal->Wait();
		}
		// Otherwise, wait for interval.
		else
		{
			m_pUptimeSignal->Wait(kuUptimeRefreshMilliseconds);
		}
	}

	return 0;
}

/**
 * When called, Engine updates per-frame tick values and timings.
 *
 * This method must be called once per frame to update per-frame timing
 * information (elapsed and absolute time).
 */
void Engine::InternalUpdateTimings()
{
	// Set a maximum delta tick time that we will report to the engine -
	// this prevents systems from freaking out due to an extreme delta t
	// (morpheme in particular is picky about the delta t being too large)
	static const Double kMaxTickTime = 0.1;

	// Compute changed values and update.
	Int64 const iNewTick = SeoulTime::GetGameTimeInTicks();
	const Double fElapsedTime = SeoulTime::ConvertTicksToSeconds(Max(
		iNewTick - m_iFrameStartTicks,
		(Int64)0));
	m_iFrameStartTicks = iNewTick;

	// Update the frame count.
	m_uFrameCount++;

	// By default, tick seconds is the elapsed time. Past frame
	// predicts next frame.
	m_fTickSeconds = fElapsedTime;

	// If the tick timer was paused, subtract it from the delta time to prevent
	// big timesteps. Don't subtract it from the total elapsed time, because
	// we still want that to accurately measure time since startup.
	if (m_iPauseTimeInTicks > 0)
	{
		m_fTickSeconds -= SeoulTime::ConvertTicksToSeconds(m_iPauseTimeInTicks);
		m_fTickSeconds = Max(m_fTickSeconds, 0.0);
		m_iPauseTimeInTicks = 0;
	}

	// Limit maximum frame time to something reasonable in case we hit
	// a hitch (leaderboard, saving, etc. doing bad things - or debug breaks).
	m_fTickSeconds = Min(m_fTickSeconds, kMaxTickTime);

	// Prior to fixing, record unmodified value.
	m_fUnfixedSeconds = m_fTickSeconds;

	// Also, override the value if m_fFixedSecondsInTick is > 0.0
	if (m_fFixedSecondsInTick > 0.0)
	{
		m_fTickSeconds = m_fFixedSecondsInTick;
	}

	// Unclamped/unmodified accumulation.
	m_fTotalSeconds += fElapsedTime;

	// Only use clamped seconds for game time. This means we don't found sleeping on phones
	// or long pauses or hangs.
	m_fTotalGameSeconds += m_fTickSeconds;
}

void Engine::PauseTickTimer()
{
	if (1 != ++m_PauseTimerActive)
	{
		return;
	}

	SEOUL_LOG_ENGINE("Pausing Tick Timer.");
	m_pPauseTimer->StartTimer();
}

void Engine::UnPauseTickTimer()
{
	if (0 != --m_PauseTimerActive)
	{
		return;
	}

	SEOUL_LOG_ENGINE("Unpausing Tick Timer.");
	m_pPauseTimer->StopTimer();

	Int64 const iElapsedTicks = m_pPauseTimer->GetElapsedTicks();
	m_iPauseTimeInTicks += iElapsedTicks;
	if (Renderer::Get().IsValid())
	{
		Renderer::Get()->AddPauseTicks(iElapsedTicks);
	}

	// Poke the uptime thread.
	m_pUptimeSignal->Activate();
}

/**
 * Platform dependent measurement of uptime, startup marker.
 *
 * The marker is set once ever and can be used for relative queries
 * to process start.
 */
Int64 Engine::GetStartUptimeInMilliseconds() const
{
	Lock lock(*m_pUptimeMutex);
	return (Int64)m_iStartUptimeInMilliseconds;
}

/**
 * Platform dependent measurement of uptime. Can be system uptime or app uptime
 * depending on platform or even device. Expected, only useful as a baseline
 * for measuring persistent delta time, unaffected by system clock changes or app sleep.
 */
Int64 Engine::GetUptimeInMilliseconds() const
{
	Lock lock(*m_pUptimeMutex);
	return (Int64)m_iUptimeInMilliseconds;
}

/**
 * Gets the two-letter ISO 639-1 language code for the current system language,
 * e.g. "en"
 */
String Engine::GetSystemLanguageCode() const
{
	return LocManager::GetLanguageCode(GetSystemLanguage());
}

/**
 * Gets one of our local (internal) IPv4 addresses.  Note that we may have more
 * than one if we have multiple network interfaces.
 */
String Engine::GetAnIPv4Address() const
{
	SEOUL_WARN("Must implement GetAnIPAddress in an engine subclass");
	return String();
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
 *   If non-empty, contains the path to the launch image to be used instead of
 *   the normal launch image (e.g. Default.png on iOS) when launching the game
 *   from the notification
 *
 * @param[in] sSoundFilePath
 *   If non-empty, contains the path to the sound file to play when the
 *   notification fires
 *
 * @param[in] nIconBadgeNumber
 *   If non-zero, contains the number to display as the application's icon
 *   badge
 *
 * @param[in] userInfo
 *   Arbitrary dictionary of data to pass back to the application when the
 *   notification fires
 *
 * @return
 *   True if the notification was scheduled, or false if local notifications
 *   are not supported on this platform.
 */
void Engine::ScheduleLocalNotification(
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
	SEOUL_LOG_NOTIFICATION("ScheduleLocalNotification not supported on this platform\n");
}

/**
 * Cancels all currently scheduled local notifications.  Not supported on all
 * platforms.
 */
void Engine::CancelAllLocalNotifications()
{
	SEOUL_LOG_NOTIFICATION("CancelAllLocalNotifications not supported on this platform\n");
}

/**
 * Cancels the local notification with the given ID.  Not supported on all
 * platforms.
 */
void Engine::CancelLocalNotification(Int iNotificationID)
{
	SEOUL_LOG_NOTIFICATION("CancelLocalNotifications not supported on this platform\n");
}

#if SEOUL_WITH_REMOTE_NOTIFICATIONS
/**
 * Asynchronously registers this device to receive remote notifications.  Not
 * supported on all platforms.  Poll Engine::GetRegistrationToken() for changes
 * to the device's remote notification token.
 */
void Engine::RegisterForRemoteNotifications()
{
	SEOUL_LOG("Remote notifications not supported on this platform\n");
}

void Engine::SetRemoteNotificationToken(const String& sToken)
{
	Lock lock(*m_pRemoteNotificationTokenMutex);
	m_RemoteNotificationToken = sToken;
}

String Engine::GetRemoteNotificationToken() const
{
	Lock lock(*m_pRemoteNotificationTokenMutex);
	return m_RemoteNotificationToken;
}

void Engine::OnDisplayRemoteNotificationToken(Bool bAllowNotifications)
{
	Lock lock(*m_pRemoteNotificationTokenMutex);
	m_DisplayRemoteNotificationToken = bAllowNotifications ? Ternary::TernaryTrue : Ternary::TernaryFalse;
}

Ternary Engine::GetDisplayRemoteNotificationToken() const
{
	Lock lock(*m_pRemoteNotificationTokenMutex);
	return m_DisplayRemoteNotificationToken;
}
#endif // /SEOUL_WITH_REMOTE_NOTIFICATIONS

/**
 * Initiates text editing support for a particular ITextEditable using
 * the (platform dependent) text editing support.
 *
 * @param[in] pTextEditable The ITextEditable instance to initiate text editing support on.
 * @param[in] sText The initial text.
 * @parma[in] sDescription A human-readable title/description - displayed on some platforms, ignored on others.
 * @param[in] constraints Editing constraints for the edit session.
 */
void Engine::StartTextEditing(ITextEditable* pTextEditable, const String& sText, const String& sDescription, const StringConstraints& constraints, bool allowNonLatinKeyboard)
{
	StopTextEditing(m_pTextEditable);

	m_pTextEditable = pTextEditable;
	InternalStartTextEditing(pTextEditable, sText, sDescription, constraints, allowNonLatinKeyboard);
}

/**
 * Stop the current text editing session.
 */
void Engine::StopTextEditing(ITextEditable* pTextEditable)
{
	if (nullptr != m_pTextEditable && pTextEditable == m_pTextEditable)
	{
		InternalStopTextEditing();
		m_pTextEditable = nullptr;
	}
}

/**
 * @return true if text editing is currently active (on appropriate platforms,
 * this implies a virtual keyboard is active).
 */
Bool Engine::IsEditingText() const
{
	return nullptr != m_pTextEditable;
}

/**
 * Sets the name of the currently running executable
 *
 * This should be called by a subclass.  The value of argv[0] from initial
 * program startup should typically be passed.
 */
void Engine::SetExecutableName(const String& sExecutableName)
{
	m_sExecutableName = sExecutableName;
}

/**
 * Protected member, must be called by the platform dependent
 * specialization of Engine::Tick() at the start of the tick.
 */
void Engine::InternalBeginTick()
{
	// Cap the frame rate and update per-frame timing values.
	InternalUpdateTimings();

	// Reset the global per-frame message box limit.
#if SEOUL_LOGGING_ENABLED
	Logger::GetSingleton().OnFrame();
#endif // /#if SEOUL_LOGGING_ENABLED

	InputManager::Get()->Tick(GetSecondsInTick());
}

/**
 * Protected member, must be called by the platform dependent
 * specialization of Engine::Tick() at the end of the tick.
 */
void Engine::InternalEndTick()
{
	auto const f(GetSecondsInTick());
	CookManager::Get()->Tick(f);
	CommerceManager::Get()->Tick();
	AchievementManager::Get()->Tick();
	HTTP::Manager::Get()->Tick();
}

/**
 * @return Either a CookManagerMoriarty or a NullCookManager, depending on if
 * Moriarty is enabled and if the -no_cooking command line option is
 * present.  Can be specialized in subclasses of Engine to implement
 * platform/game specific CookManagers.
 */
CookManager* Engine::InternalCreateCookManager()
{
#if SEOUL_WITH_MORIARTY
	if (!EngineCommandLineArgs::GetNoCooking())
	{
		return SEOUL_NEW(MemoryBudgets::Cooking) CookManagerMoriarty;
	}
#endif

	return SEOUL_NEW(MemoryBudgets::Cooking) NullCookManager();
}

AnalyticsManager* Engine::InternalCreateAnalyticsManager()
{
	return SEOUL_NEW(MemoryBudgets::Analytics) NullAnalyticsManager();
}

AchievementManager* Engine::InternalCreateAchievementManager()
{
	return SEOUL_NEW(MemoryBudgets::TBD) NullAchievementManager();
}

CommerceManager* Engine::InternalCreateCommerceManager()
{
#if !SEOUL_SHIP
	return SEOUL_NEW(MemoryBudgets::Commerce) DeveloperCommerceManager();
#else
	return SEOUL_NEW(MemoryBudgets::Commerce) NullCommerceManager();
#endif
}

/**
 * Default implementation, creates a NullFacebookManager
 */
FacebookManager* Engine::InternalCreateFacebookManager()
{
#if SEOUL_ENABLE_CHEATS && SEOUL_PLATFORM_WINDOWS
	static const HString ksUseDebugFacebookManager("UseDebugFacebookManager");
	Bool bUseDebugFacebookManager = false;
	if (GetApplicationJsonValue(ksUseDebugFacebookManager, bUseDebugFacebookManager) && bUseDebugFacebookManager)
	{
		return SEOUL_NEW(MemoryBudgets::TBD) DebugPCFacebookManager();
	}
	else
#endif
	{
		return SEOUL_NEW(MemoryBudgets::TBD) NullFacebookManager();
	}
}

/**
 * Default implementation, instantiate a NullPlatformSignInManager.
 */
PlatformSignInManager* Engine::InternalCreatePlatformSignInManager()
{
	return SEOUL_NEW(MemoryBudgets::TBD) NullPlatformSignInManager();
}

/**
 * Default implementation, creates a Sound::NullManager
 */
Sound::Manager* Engine::InternalCreateSoundManager()
{
	return SEOUL_NEW(MemoryBudgets::TBD) Sound::NullManager();
}

/**
 * Default implementation, creates a NullTrackingManager
 */
TrackingManager* Engine::InternalCreateTrackingManager()
{
	return SEOUL_NEW(MemoryBudgets::TBD) NullTrackingManager();
}

/**
 * Protected member, must be called by the platform dependent
 * specialization of Initialize(), prior to initializing the RenderDevice.
 * When this method returns, the following will be initialized:
 * - SettingsManager
 * - Core::Initialize()
 * - MoriartyClient (if Moriarty is enabled)
 * - Input
 * - Jobs::Manager
 * - An IMapFile subclass for handling stack trace resolution
 * - LocManager
 * - Content::LoadManager
 */
void Engine::InternalPreRenderDeviceInitialization(
	const CoreSettings& coreSettings,
	const SaveLoadManagerSettings& saveLoadManagerSettings)
{
	// SettingsManager is constructed first, in a "bootstrap" mode, so it
	// is accessible at certain (very) early initialization points (FileSystem
	// initialization). Once Content::LoadManager is available, it will be
	// switched to normal operation, dependent on Content::LoadManager and the
	// Job system.
	m_pSettingsManager.Reset(SEOUL_NEW(MemoryBudgets::Game) SettingsManager());

	InternalInitializeCore(coreSettings);
	InternalPreInitialize();
	SEOUL_NEW(MemoryBudgets::Game) Jobs::Manager;

#if SEOUL_ENABLE_STACK_TRACES
	if (nullptr == Core::GetMapFile())
	{
		MapFileClass *pMapFile = SEOUL_NEW(MemoryBudgets::Debug) MapFileClass;
		Core::SetMapFile(pMapFile);
		pMapFile->StartLoad();
	}
#endif // /SEOUL_ENABLE_STACK_TRACES

	m_pCookManager.Reset(InternalCreateCookManager());
	m_pContentLoadManager.Reset(SEOUL_NEW(MemoryBudgets::Game) Content::LoadManager());

	// Now switch SettingsManager to its normal operation mode.
	m_pSettingsManager->OnInitializeContentLoader();

	InternalInitializeInput();
	InternalInitializeLocManager();

	// Kick off our thread to periodically refresh uptime.
	RefreshUptime();
	m_pUptimeThread.Reset(SEOUL_NEW(MemoryBudgets::TBD) Thread(SEOUL_BIND_DELEGATE(&Engine::UptimeWorker, this)));
	m_pUptimeThread->Start("Uptime Thread");
	m_pUptimeThread->SetPriority(Thread::kLow);

	{
		HTTP::ManagerSettings settings;

		// Read in the SSL certificate data
		auto const rootCertPath(FilePath::CreateConfigFilePath("app_root_cert.pem"));
		(void)FileManager::Get()->ReadAll(rootCertPath, settings.m_sSSLCertificates);

		// Sub platform.
		{
			PlatformData data;
			GetPlatformData(data);

			if (PlatformFlavor::kUnknown != data.m_eDevicePlatformFlavor)
			{
				settings.m_sSubPlatform = EnumToString<PlatformFlavor>(data.m_eDevicePlatformFlavor);
			}
		}

		SEOUL_NEW(MemoryBudgets::Game) HTTP::Manager(settings);
	}

	// Enable network file IO.
	FileManager::Get()->EnableNetworkFileIO();

	// Create the save-load manager.
	m_pSaveLoadManager.Reset(SEOUL_NEW(MemoryBudgets::Saving) SaveLoadManager(saveLoadManagerSettings));

	// Create the analytics manager for other systems to use.
	m_pAnalyticsManager.Reset(InternalCreateAnalyticsManager());
}

/**
 * Protected member, must be called by the platform dependent
 * specialization of Initialize(), immediately after initializing the RenderDevice.
 * When this method returns, the following will be initialized:
 * - AssetManager
 * - EffectManager
 * - MaterialManager
 * - Renderer
 * - Sound::Manager
 * - TextureManager
 */
void Engine::InternalPostRenderDeviceInitialization()
{
	m_pTextureManager.Reset(SEOUL_NEW(MemoryBudgets::Game) TextureManager());
	m_pMaterialManager.Reset(SEOUL_NEW(MemoryBudgets::Game) MaterialManager);
	m_pAssetManager.Reset(SEOUL_NEW(MemoryBudgets::Game) AssetManager);
	m_pEffectManager.Reset(SEOUL_NEW(MemoryBudgets::Game) EffectManager());
	m_pRenderer.Reset(SEOUL_NEW(MemoryBudgets::Game) Renderer());
	m_pSoundManager.Reset(InternalCreateSoundManager());
}

/**
 * Protected member, must be called by the platform dependent
 * specialization of Initialize(), at the very end of Initialize().
 */
void Engine::InternalPostInitialization()
{
	m_pCommerceManager.Reset(InternalCreateCommerceManager());
	m_pCommerceManager->Initialize();

	(void)InternalCreateAchievementManager();
	(void)InternalCreateFacebookManager();
	(void)InternalCreatePlatformSignInManager();
	(void)InternalCreateTrackingManager();

#if SEOUL_HOT_LOADING
	LocManager::Get()->RegisterForHotLoading();
#endif // /#if SEOUL_HOT_LOADING

	m_bInitialized = true;
}

/**
 * Protected member, must be called by the platform dependent
 * specialization of Shutdown(), at the very beginning of Shutdown().
 */
void Engine::InternalPreShutdown()
{
	StopTextEditing(m_pTextEditable); SEOUL_TEARDOWN_TRACE();

	m_bInitialized = false;

#if SEOUL_HOT_LOADING
	LocManager::Get()->UnregisterFromHotLoading();
#endif // /#if SEOUL_HOT_LOADING

	// Disable network file IO before further processing, we don't want
	// calls to WaitUntilAllLoadsAreFinished() to content manager with
	// network file IO still active.
	FileManager::Get()->DisableNetworkFileIO(); SEOUL_TEARDOWN_TRACE();

	// Finish off any active content loads.
	m_pContentLoadManager->WaitUntilAllLoadsAreFinished(); SEOUL_TEARDOWN_TRACE();

	SEOUL_DELETE TrackingManager::Get(); SEOUL_TEARDOWN_TRACE();
	SEOUL_DELETE PlatformSignInManager::Get(); SEOUL_TEARDOWN_TRACE();
	SEOUL_DELETE FacebookManager::Get(); SEOUL_TEARDOWN_TRACE();
	SEOUL_DELETE AchievementManager::Get(); SEOUL_TEARDOWN_TRACE();

	m_pCommerceManager->Shutdown(); SEOUL_TEARDOWN_TRACE();
	m_pCommerceManager.Reset(); SEOUL_TEARDOWN_TRACE();
}

/**
 * Protected member, must be called by the platform dependent
 * specialization of Shutdown(), immediately before destroying the render device.
 *
 * All operations in this method must be LIFO with respect
 * to the operation order in Engine::InternalPostRenderDeviceInitialization().
 */
void Engine::InternalPreRenderDeviceShutdown()
{
	m_pSoundManager.Reset(); SEOUL_TEARDOWN_TRACE();

	m_pRenderer.Reset(); SEOUL_TEARDOWN_TRACE();
	m_pEffectManager.Reset(); SEOUL_TEARDOWN_TRACE();
	m_pAssetManager.Reset(); SEOUL_TEARDOWN_TRACE();
	m_pMaterialManager.Reset(); SEOUL_TEARDOWN_TRACE();
	m_pTextureManager.Reset(); SEOUL_TEARDOWN_TRACE();
}

/**
 * Protected member, must be called by the platform dependent
 * specialization of Shutdown(), immediately after destroying the render device.
 *
 * All operations in this method must be LIFO with respect
 * to the operation order in Engine::InternalPreRenderDeviceInitialization().
 */
void Engine::InternalPostRenderDeviceShutdown()
{
	m_pAnalyticsManager.Reset(); SEOUL_TEARDOWN_TRACE();
	m_pSaveLoadManager.Reset(); SEOUL_TEARDOWN_TRACE();

	SEOUL_DELETE HTTP::Manager::Get(); SEOUL_TEARDOWN_TRACE();

	// Kill the uptime refresh thread.
	m_bUptimeThreadRunning = false;
	m_pUptimeSignal->Activate();
	m_pUptimeThread.Reset(); SEOUL_TEARDOWN_TRACE();

	InternalShutdownLocManager(); SEOUL_TEARDOWN_TRACE();
	InternalShutdownInput(); SEOUL_TEARDOWN_TRACE();

	// Place SettingsManager back into bootstrap mode prior
	// to Content::LoadManager destruction.
	m_pSettingsManager->OnShutdownContentLoader(); SEOUL_TEARDOWN_TRACE();
	m_pContentLoadManager.Reset(); SEOUL_TEARDOWN_TRACE();
	m_pCookManager.Reset(); SEOUL_TEARDOWN_TRACE();

	// NOTE: We intentionally leave the MapFile
	// allocated here - see the note on
	// Core::SetMapFile().
	SEOUL_DELETE Jobs::Manager::Get(); SEOUL_TEARDOWN_TRACE();
	InternalPostShutdown(); SEOUL_TEARDOWN_TRACE();
	InternalShutdownCore(); SEOUL_TEARDOWN_TRACE();

	// Destroy SettingsManager after all other shutdown.
	m_pSettingsManager.Reset(); SEOUL_TEARDOWN_TRACE();
}

/**
 * Private member function, used by Engine to perform very basic
 * initialization tasks, before initializing any complex managers.
 */
void Engine::InternalPreInitialize()
{
	SEOUL_NEW(MemoryBudgets::Event) Events::Manager;

#if SEOUL_WITH_MORIARTY
	// Connect to a Moriarty server, if the command line requested it
	if (!EngineCommandLineArgs::GetMoriartyServer().IsEmpty())
	{
		SEOUL_LOG_ENGINE("Attempting to connect to Moriarty server: %s\n", EngineCommandLineArgs::GetMoriartyServer().CStr());
		MoriartyClient::Get()->Connect(EngineCommandLineArgs::GetMoriartyServer());

#if SEOUL_LOGGING_ENABLED
		// Reload the logger configuration, since the logger gets initialized
		// before Moriarty
		(void)Seoul::Logger::GetSingleton().LoadConfiguration();
#endif
	}
#endif
}

/**
 * Private member function, used by Engine to perform very basic
 * shutdown tasks, after all other shutdown has occured.
 */
void Engine::InternalPostShutdown()
{
	SEOUL_DELETE Events::Manager::Get();
}

void Engine::InternalInitializeCore(const CoreSettings& settings)
{
	Core::Initialize(settings);
}

void Engine::InternalShutdownCore()
{
	Core::ShutDown();
}

void Engine::InternalInitializeInput()
{
	SEOUL_NEW(MemoryBudgets::TBD) InputManager;
	InputManager::Get()->Initialize();

#if SEOUL_WITH_MORIARTY
	// Register the keyboard key handler if running with Moriarty.
	MoriartyClient::Get()->RegisterKeyboardKeyEventHandler(MoriartyKeyboardKeyEventHandler);
	// Register the keyboard char handler if running with Moriarty.
	MoriartyClient::Get()->RegisterKeyboardCharEventHandler(MoriartyKeyboardCharEventHandler);
#endif
}

void Engine::InternalShutdownInput()
{
#if SEOUL_WITH_MORIARTY
	// Unregister the keyboard char handler if running with Moriarty.
	MoriartyClient::Get()->RegisterKeyboardCharEventHandler(nullptr);
	// Unregister the keyboard key handler if running with Moriarty.
	MoriartyClient::Get()->RegisterKeyboardKeyEventHandler(nullptr);
#endif

	SEOUL_DELETE InputManager::Get();
}

void Engine::InternalInitializeLocManager()
{
	SEOUL_NEW(MemoryBudgets::Strings) LocManager();
}

void Engine::InternalShutdownLocManager()
{
	SEOUL_DELETE LocManager::Get();
}

} // namespace Seoul
