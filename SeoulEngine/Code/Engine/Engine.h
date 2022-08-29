/**
 * \file Engine.h
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

#pragma once
#ifndef ENGINE_H
#define ENGINE_H

#include "Atomic64.h"
#include "Delegate.h"
#include "FilePath.h"
#include "HashTable.h"
#include "MouseCursor.h"
#include "NetworkConnectionType.h"
#include "ScopedPtr.h"
#include "SeoulString.h"
#include "SeoulTime.h"
#include "Singleton.h"
#include "Ternary.h"
#include "Vector.h"

namespace Seoul
{

// Forward declarations
class AchievementManager;
class AnalyticsManager;
class AssetManager;
class CommerceManager;
namespace Content { class LoadManager; }
class CookManager;
struct CoreSettings;
class DataStore;
class EffectManager;
class FacebookManager;
namespace Jobs { class Manager; }
namespace HTTP { class Request; }
class ITextEditable;
class MaterialManager;
class Mutex;
struct PlatformData;
class PlatformSignInManager;
class RenderDevice;
class Renderer;
class SaveApi;
class SaveLoadManager;
struct SaveLoadManagerSettings;
class SeoulTime;
class SettingsManager;
class Signal;
namespace Sound { class Manager; }
struct StringConstraints;
class TextureManager;
class Thread;
class TrackingManager;
struct Vector2D;
class WorldTime;

/**
 * On supported platforms (currently PC), the Events::Manager event ID that can
 * be used to register for drop file events.
 *
 * Signature is void Func(const String& sFilename),
 * where sFilename will be the absolute path to a file that was dragged to the active window.
 *
 * This functionality is only available in developer builds.
 */
static const HString EngineDropFileEventId("DropFileEventId");

#if SEOUL_WITH_REMOTE_NOTIFICATIONS
// Enum of various service types associated with remote notifications.
enum class RemoteNotificationType
{
	/** Remote notifications using the Amazon Device Messaging (ADM) service. */
	kADM,

	/** Remote notifications using the Firebase Cloud Messaging (FCM) service. */
	kFCM,

	/** Remote notifications using Apple iOS push notifications. */
	kIOS,

	COUNT,
};
#endif // /SEOUL_WITH_REMOTE_NOTIFICATIONS

enum class EngineType
{
	kAndroid,
	kIOS,
	kNull,
	kPCDefault,
	kSteam,
};

/**
 * Engine is the abstract base class for the Engine singleton.
 * It must be specialized per platform.
 *
 * Once Engine has been constructed and Initialize()d, the following
 * will be available:
 * - Core::Initialize() will have been called.
 * - if stack traces are enabled, a MapFileAsync will be constructed
 *   and set as the map file to Core to provide stack trace name resolution.
 * - AnalyticsManager
 * - AssetManager
 * - Content::LoadManager
 * - CookManager
 * - EffectManager
 * - Events::Manager
 * - Input
 * - Jobs::Manager
 * - LocManager
 * - MaterialManager
 * - Renderer
 * - SettingsManager
 * - Sound::Manager
 * - TextureManager
 * - TrackingManager
 */
class Engine SEOUL_ABSTRACT : public Singleton<Engine>
{
public:
	SEOUL_DELEGATE_TARGET(Engine);

	// Hook for g_pCoreVirtuals, must be explicitly assigned in main process code.
	static String CoreGetPlatformUUID();
	// Hook for g_pCoreVirtuals, must be explicitly assigned in main process code.
	static TimeInterval CoreGetUptime();

	Engine();
	virtual ~Engine();

	virtual EngineType GetType() const = 0;

	// Populate rPlatformData with the current platform's runtime data.
	void GetPlatformData(PlatformData& rData) const;

	// TODO: Decide if this is the desired API or if it should be abstracted further.
	Bool IsSamsungPlatformFlavor() const;
	Bool IsAmazonPlatformFlavor() const;
	Bool IsGooglePlayPlatformFlavor() const;

	// Convenience, access to the m_sPlatformUUID member of this Engine's platform data.
	String GetPlatformUUID() const;

	// Get a hash of the player's device info for load shedding purposes, from 00 to 99. Changes daily.
	String GetLoadShedPlatformUuidHash() const;

	// TODO: Unfortunate wart, but Steam needs this for DLC processing.
	typedef Vector<UByte, MemoryBudgets::TBD> AuthTicket;
	virtual const AuthTicket& GetAuthenticationTicket() const
	{
		static const AuthTicket s_k;
		return s_k;
	}

	// TODO: Move this into PlatformData and also implement for other
	// platforms that have a similar concept (e.g. the first party app id
	// on iOS).

	// Title app identifier on platforms which support a unique app identifier.
	virtual UInt32 GetTitleAppID() const { return 0; }

	/**
	 * @return true if the current platform Engine supports native file/directory dialogues.
	 */
	virtual Bool SupportsPlatformFileDialogs()
	{
		return false;
	}

	/** Operation to perform with a file dialog. */
	enum class FileDialogOp
	{
		kOpen,
		kSave,
	};

	/** Entry for a file filter when opening a file dialog. */
	struct FileFilter SEOUL_SEALED
	{
		/** On supported platforms, the human readable name to identify the filter. */
		String m_sFriendlyName;

		/** The pattern used to match files (e.g. "*.png"). */
		String m_sPattern;
	};
	typedef Vector<FileFilter> FileFilters;

	/**
	 * @return true and populate rsOutput with the select output file, on supported platforms and without
	 * user cancellation. Otherwise, return false.
	 */
	virtual Bool DisplayFileDialogSingleSelection(
		String& rsOutput,
		FileDialogOp eOp,
		const FileFilters& vFilters = FileFilters(),
		const String& sWorkingDirectory = String()) const
	{
		return false;
	}

	// Return true and populate rsOutput with the select output file, on supported platforms and without
	// user cancellation. Otherwise, return false.
	Bool DisplayFileDialogSingleSelection(
		FilePath& rInputOutput,
		FileDialogOp eOp,
		FileType eFileType,
		GameDirectory eWorkingDirectory) const;

	/**
	 * @return true and populate rvRecentDocuments with a list of recently opened
	 * files of the given type as reported by the system. Will return false if
	 * recent document functionality is not available on the current platform.
	 */
	typedef Vector<FilePath> RecentDocuments;
	virtual Bool GetRecentDocuments(
		GameDirectory eGameDirectory,
		FileType eFileType,
		RecentDocuments& rvRecentDocuments) const
	{
		return false;
	}

	/**
	 * On supported platforms, returns a path
	 * to a directory to monitor for recent document
	 * changes. Will return String() if not supported.
	 */
	virtual String GetRecentDocumentPath() const
	{
		return String();
	}

	/**
	 * @return True if the current hardware meets minimum requirements for the current
	 * application, false otherwise.
	 */
	virtual Bool MeetsMinimumHardwareRequirements() const
	{
		// Default to always true - engine platform specializations can implement
		// this as needed.
		return true;
	}

	/** @return True if the current platform has default/native back button handling. */
	virtual Bool HasNativeBackButtonHandling() const
	{
		return false;
	}

	/** @return true if the current platform has external clipboard support, false otherwise. */
	virtual Bool SupportsClipboard() const
	{
		return false;
	}

	/**
	 * If the current platform supports a clipboard, get the system clipboard
	 * contents into rsOutput.
	 */
	virtual Bool ReadFromClipboard(String& rsOutput)
	{
		return false;
	}

	/**
	 * If the current platform supports a clipboard, set the system clipboard
	 * contents to sInput.
	 */
	virtual Bool WriteToClipboard(const String& sInput)
	{
		return false;
	}

	/**
	 * Asks for application quit. Not supported on all platforms.
	 */
	virtual Bool PostNativeQuitMessage()
	{
		return false;
	}

	/**
	 * @return True if the current platform supports battery level queries, false otherwise.
	 * If this method returns true, rfLevel will contain the battery level (on [0, 1]),
	 * otherwise it will be left unchanged.
	 */
	virtual Bool QueryBatteryLevel(Float& rfLevel)
	{
		// Default to always false.
		return false;
	}

	/**
	 * @return True if the current platform supports network connection type queries, false otherwise.
	 * If this method returns true, reType will contain the most recently queried
	 * connection type (backing query frequency is platform dependent).
	 */
	virtual Bool QueryNetworkConnectionType(NetworkConnectionType& reType)
	{
		// Default to always false.
		return false;
	}

	/**
	* Shows the iOS App Store to allow the user to rate this app. On Android opens AppStore URL.
	*/
	virtual void ShowAppStoreToRateThisApp()
	{
	}

	/**
	 * @return True if the current platform supports displaying "Rate Me" UI over the application,
	 * rather than requiring a redirect to the store page.
	 */
	virtual Bool DoesSupportInAppRateMe() const
	{
		return false;
	}

	// Platform dependent overall process memory query. Guaranteed to be thread-safe.
	//
	// On supported platforms, "Private Bytes" will be the total memory reserved by
	// the current process and "Working Set" will be the total memory reserved by
	// the current process that must be in physical memory (vs. paged out). On platforms
	// without a page file, these values will be the same and will correspond to
	// the "proportional set size" (PSS).
	//
	// WARNING: Likely a very expensive call - may take multiple milliseconds or whole seconds
	// to return. Prefer calling on a worker thread, periodically.
	virtual Bool QueryProcessMemoryUsage(size_t& rzWorkingSetBytes, size_t& rzPrivateBytes) const
	{
		// Default to always false, not an available query.
		return false;
	}

	// The majority of engine startup and shutdown code should be implemented
	// in these two methods.
	virtual void Initialize() = 0;
	virtual void Shutdown() = 0;

	// Open a URL in an external browser.
	Bool OpenURL(const String& sURL);

	/**
	 * Functionality used for various testing configurations - prevents OpenURL
	 * from triggering external applications to avoid the App losing focus.
	 */
	Bool GetSuppressOpenURL() const { return m_bSuppressOpenURL; }
	void SetSuppressOpenURL(Bool bSuppressOpenURL) { m_bSuppressOpenURL = bSuppressOpenURL; }

	// Engine::Tick() should be called at the beginning of each frame. If
	// this method returns false, then the user has requested a game exit
	// and the game should exit its loop and shutdown.
	virtual Bool Tick() = 0;

	// On some platforms, this method can be called to poll the window
	// message queue. This function should not be called during normal
	// program flow, but can be used in cases where the game is intentionally
	// not calling Engine::Tick().
	virtual Bool RenderThreadPumpMessageQueue()
	{
		return true;
	}

#if SEOUL_ENABLE_CHEATS
	/** Developer only - globally scales the apparent passage of time as reported by the engine. */
	Double GetDevOnlyGlobalTickScale() const { return m_fDevOnlyGlobalTickScale; }
	void SetDevOnlyGlobalTickScale(Double f) { m_fDevOnlyGlobalTickScale = Max(f, 0.0); }
	private: Double m_fDevOnlyGlobalTickScale = 1.0f; public:
#endif

	/** For developer only functionality, get raw tick seconds without any global developer scaling. */
	Float DevOnlyGetRawSecondsInTick() const
	{
		return (Float)m_fTickSeconds;
	}

	/**
	 * @return The elapsed time during the previous tick - or - the amount
	 * of time elapsed from the beginning of the previous frame tick to the
	 * beginning of the current frame tick.
	 */
	Float GetSecondsInTick() const
	{
#if SEOUL_ENABLE_CHEATS
		// Factor in the global developer time scaling factor now.
		return (Float)(m_fTickSeconds * m_fDevOnlyGlobalTickScale);
#else
		return (Float)m_fTickSeconds;
#endif
	}

	/**
	 * If a fixed time step is enabled, this is the unfixed value (it represents the real
	 * delta time, clamped for sanitizing but not for fixed time stepping.
	 */
	Float GetUnfixedSecondsInTick() const
	{
#if SEOUL_ENABLE_CHEATS
		// Factor in the global developer time scaling factor now.
		return (Float)(m_fUnfixedSeconds * m_fDevOnlyGlobalTickScale);
#else
		return (Float)m_fUnfixedSeconds;
#endif
	}

	/**
	 * @return The current GetSecondsInTick() scaling factor. Usually 1.0f.
	 *
	 * Individual subsystems (e.g. animation in the UI or audio) must apply
	 * this scale independently. It is only stored in Engine, not applied.
	 */
	Double GetSecondsInTickScale() const
	{
		return m_fTickSecondsScale;
	}

	/**
	 * Update the fixed seconds in tick value - if set to a value > 0.0f,
	 * this time will always be reported as the Engine delta time, independent
	 * of the actual delta time. Typically used to force a fixed time step
	 * or for debugging/developer purposes.
	 */
	void SetFixedSecondsInTick(Double fFixedSecondsInTick)
	{
		m_fFixedSecondsInTick = fFixedSecondsInTick;
	}

	/**
	 * Apply a scaling factor to the value returned by GetSecondsInTick().
	 *
	 * Individual subsystems (e.g. animation in the UI or audio) must apply
	 * this scale independently. It is only stored in Engine, not applied.
	 */
	void SetSecondsInTickScale(Double fScale)
	{
		m_fTickSecondsScale = fScale;
	}

	/**
	 * @return The total # of seconds that have elapsed since the first
	 * call to Engine::Tick().
	 */
	Double GetSecondsSinceStartup() const
	{
		return m_fTotalSeconds;
	}

	/**
	 * @return The total # of in game seconds that have elapsed since the first
	 * call to Engine::Tick(). This only counts time for when the game is running.
	 * It does not include sleep time or the game hanging.
	 */
	Double GetGameSecondsSinceStartup() const
	{
		return m_fTotalGameSeconds;
	}

	/**
	 * Calling this method begins a timer which, after a call to UnPauseTickTimer(),
	 * will cause the window between PauseTickTimer() and UnPauseTickTimer() to
	 * be factored out from GetSecondsInTick(). This allows program spikes
	 * to be masked out from the game elapsed time.
	 *
	 * A call to PauseTickTimer() must be followed by a call to
	 * UnPauseTickTimer() before calling Engine::Tick() or the results are
	 * undefined.
	 *
	 * \pre PauseTickTimer() cannot be called a second time before
	 * calling UnPauseTickTimer().
	 */
	void PauseTickTimer();

	/**
	 * @return True if the tick timer has been paused
	 * with a call to PauseTickTimer(), false otherwise.
	 */
	Bool IsTickTimerPaused() const
	{
		return (0 != m_PauseTimerActive);
	}

	/**
	 * Calling this method ends a timer which, after a call to PauseTickTimer(),
	 * will cause the window between PauseTickTimer() and UnPauseTickTimer() to
	 * be factored out from GetSecondsInTick(). This allows program spikes
	 * to be masked out from the game elapsed time.
	 *
	 * A call to UnPauseTickTimer() must be preceeded by a call to
	 * PauseTickTimer() before calling Engine::Tick() or the results are
	 * undefined.
	 *
	 * \pre UnPauseTickTimer() cannot be called a second time before
	 * calling PauseTickTimer().
	 */
	void UnPauseTickTimer();

	/**
	 * @return The total # of frames that have elapsed since the first
	 * call to Engine::Tick().
	 */
	UInt32 GetFrameCount() const
	{
		return m_uFrameCount;
	}

	// Marker equivalent to GetUptimeInMilliseconds() - set once at startup. Allows
	// relative queries from the start of the process.
	Int64 GetStartUptimeInMilliseconds() const;

	// Platform dependent measurement of uptime. Can be system uptime or app uptime
	// depending on platform or even device. Expected, only useful as a baseline
	// for measuring persistent delta time, unaffected by system clock changes or app sleep.
	Int64 GetUptimeInMilliseconds() const;

	/** Convenience function, start uptime as a TimeInterval. */
	TimeInterval GetStartUptime() const
	{
		Int64 const iStartUptimeInMilliseconds = GetStartUptimeInMilliseconds();
		return TimeInterval::FromMicroseconds(iStartUptimeInMilliseconds * (Int64)1000);
	}

	/** Convenience function, uptime as a TimeInterval. */
	TimeInterval GetUptime() const
	{
		Int64 const iUptimeInMilliseconds = GetUptimeInMilliseconds();
		return TimeInterval::FromMicroseconds(iUptimeInMilliseconds * (Int64)1000);
	}

	Int64 FrameStartTicks() const
	{
		return m_iFrameStartTicks;
	}

	/**
	 * Uptime will automatically be refreshed when the Engine is ticked. This function
	 * can be used to guarantee a fresh uptime.
	 */
	virtual void RefreshUptime() = 0;

	// Platform dependent indication of whether the game has focus or not.
	// Engine does not use this value internally, but it can be used
	// by subclasses or by the game code.
	virtual Bool HasFocus() const = 0;

	/**
	 * Get the name of the currently running executable
	 */
	const String& GetExecutableName()
	{
		return m_sExecutableName;
	}

	/**
	 * @return True if Engine::Initialize() was called, false otherwise.
	 *
	 * \warning Calling any methods of Engine besides Initialize() when
	 * this method returns false will result in undefined behavior.
	 */
	Bool IsInitialized() const
	{
		return m_bInitialized;
	}

	/** Tests if we're currently in a modal Windows message loop (PC-only) */
	virtual Bool IsInModalWindowsLoop() const { return false; }

	// Return the platform specific implementation of SaveApi
	virtual SaveApi* CreateSaveApi() = 0;

	virtual String GetSystemLanguage() const
	{
		return "English";
	}

	// Gets the two-letter ISO 639-1 language code for the current system
	// language, e.g. "en"
	String GetSystemLanguageCode() const;

	// Gets one of our local (internal) IPv4 addresses.  Note that we may have
	// more than one if we have multiple network interfaces.
	virtual String GetAnIPv4Address() const;

	// Attempt to overwrite/update the m_sPlatformUUID of the platform's
	// PlatformData. Return true if successfully overwritten, false otherwise.
	// On true, future calls to GetPlatformData() will have this updated
	// value.
	virtual Bool UpdatePlatformUUID(const String& sPlatformUUID) = 0;

	// Schedules a local notification to be delivered to us by the OS at a
	// later time.  Not supported on all platforms.
	//
	// iOS: Supports all specified arguments.
	// Android: Ignores the following arguments
	// - bIsWallClockTime - this argument is always assumed to be true.
	// - sLaunchImageFilePath
	// - sSoundFilePath
	// - nIconBadgeNumber
	// - userInfo - is used to generate a hash to uniquely identify event types, but is otherwise ignored.
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
		const DataStore& userInfo);

	// Cancels all currently scheduled local notifications.  Not supported on
	// all platforms.
	//
	// iOS: Available
	// Android: Not available - scheduled events can only be canceled by ID
	//          with CancelLocalNotification(Int)
	virtual void CancelAllLocalNotifications();

	// Cancels the local notification with the given ID.   Not supported on
	// all platforms.
	//
	// iOS: Available
	// Android: Available
	virtual void CancelLocalNotification(Int iNotificationID);

#if SEOUL_WITH_REMOTE_NOTIFICATIONS
	// Asynchronously registers this device to receive remote notifications.
	// Not supported on all platforms.  Poll Engine::GetRegistrationToken() for changes
	// to the device's remote notification token.
	virtual void RegisterForRemoteNotifications();

	virtual Bool SupportsRemoteNotifications() const { return false; }
	virtual Bool HasEnabledRemoteNotifications() const { return false; }

	virtual RemoteNotificationType GetRemoteNotificationType() const { return RemoteNotificationType::kIOS; }
	virtual Bool IsRemoteNotificationDevelopmentEnvironment() const { return false; }
	String GetRemoteNotificationToken() const;
	void SetRemoteNotificationToken(const String&);
	void OnDisplayRemoteNotificationToken(Bool bAllowNotifications);
	Ternary GetDisplayRemoteNotificationToken() const;
#endif // /SEOUL_WITH_REMOTE_NOTIFICATIONS

	// Shows the virtual keyboard on supported platforms
	void StartTextEditing(ITextEditable* pTextEditable, const String& sText, const String& sDescription, const StringConstraints& constraints, bool allowNonLatinKeyboard);

	// Hides the virtual keyboard on supported platforms
	void StopTextEditing(ITextEditable* pTextEditable);

	// Return true if text editing is currently active (on appropriate platforms,
	// this implies a virtual keyboard is active).
	Bool IsEditingText() const;

	ITextEditable* GetTextEditable() const
	{
		return m_pTextEditable;
	}

	// Get the current mouse cursor state. Conditional platform support
	// (not all platforms will display a mouse cursor).
	MouseCursor GetMouseCursor() const { return m_eActiveMouseCursor; }

	// Update the current mouse cursor state.
	virtual void SetMouseCursor(MouseCursor eMouseCursor) { m_eActiveMouseCursor = eMouseCursor; }

	// Update platform specific storage to reflect whether the GDPR check
	// has been accepted.
	virtual void SetGDPRAccepted(Bool bAccepted) = 0;

	// Check platform specific storage to see if the player has accepted the
	// GDPR check.
	virtual Bool GetGDPRAccepted() const = 0;

	// Can we request remote notifications without a user-facing OS prompt?
	virtual Bool CanRequestRemoteNotificationsWithoutPrompt()
	{
		return true;
	}

	// Mark this device as having prompted for remote notifications, so we know
	// it's safe to do at start-up without triggering a prompt.
	// Noop on platforms that never prompt for notification permissions.
	virtual void SetCanRequestRemoteNotificationsWithoutPrompt(Bool)
	{
		return;
	}
protected:
	virtual Bool InternalOpenURL(const String& sURL)
	{
		// Fail by default.
		return false;
	}

	// Hook to allow subclasses to set the executable's name
	void SetExecutableName(const String& sExecutableName);

	// Tick hooks - must be called at the beginning and end
	// of a subclass's Tick() method.
	void InternalBeginTick();
	void InternalEndTick();

	// Can be specialized in subclasses of Engine to implement
	// platform-specific cook managers. By default, returns a NullCookManager
	virtual CookManager* InternalCreateCookManager();

	// Can be specialized in subclasses of Engine to implement
	// platform-specific analytics managers. By default, returns a
	// NullAnalyticsManager
	virtual AnalyticsManager* InternalCreateAnalyticsManager();

	// Can be specialized in subclasses of Engine to implement
	// platform-specific achievement managers. By default, returns a
	// NullAchievementManager
	virtual AchievementManager* InternalCreateAchievementManager();

	// Can be specialized in subclasses of Engine to implement
	// platform-specific commerce managers. By default, returns a
	// NullCommerceManager
	virtual CommerceManager* InternalCreateCommerceManager();

	// Can be specialized in subclasses of Engine to implement
	// platform-specific Facebook managers.  By default, returns a
	// NullFacebookManager
	virtual FacebookManager* InternalCreateFacebookManager();

	// Specialized in Engine subclasses to enable sign in with
	// platform dependent auth services.
	virtual PlatformSignInManager* InternalCreatePlatformSignInManager();

	// Can be specialized in subclasses of Engine to implement
	// platform-specific sound managers. By default, returns
	// a Sound::NullManager.
	virtual Sound::Manager* InternalCreateSoundManager();

	// Can be specialized in subclasses of Engine to implement
	// platform-specific Tracking managers. By default, returns a
	// NullTrackingManager
	virtual TrackingManager* InternalCreateTrackingManager();

	// Initialization hooks - must be called by the Engine subclass
	// at appropriate points.
	virtual void InternalPreRenderDeviceInitialization(
		const CoreSettings& coreSettings,
		const SaveLoadManagerSettings& saveLoadManagerSettings);
	void InternalPostRenderDeviceInitialization();
	void InternalPostInitialization();

	// Shutdown hooks - must be called by the Engine subclass
	// at appropriate points.
	void InternalPreShutdown();
	void InternalPreRenderDeviceShutdown();
	void InternalPostRenderDeviceShutdown();

	// Platform-specific hooks to show/hide the virtual keyboard
	virtual void InternalStartTextEditing(ITextEditable* pTextEditable, const String& sText, const String& sDescription, const StringConstraints& constraints, Bool bAllowNonLatinKeyboard)
	{
	}

	virtual void InternalStopTextEditing()
	{
	}
	ITextEditable* m_pTextEditable;

private:
	String m_sExecutableName;

	ScopedPtr<SeoulTime> m_pPauseTimer;
	Int64 m_iPauseTimeInTicks;

	UInt32 m_uFrameCount;

	Int64 m_iFrameStartTicks;
	Double m_fUnfixedSeconds;
	Double m_fTickSeconds;
	Double m_fTickSecondsScale;
	Double m_fFixedSecondsInTick;
	Double m_fTotalSeconds;
	Double m_fTotalGameSeconds;

protected:
	ScopedPtr<Mutex> m_pPlatformDataMutex;
	ScopedPtr<PlatformData> m_pPlatformData;
	Atomic64Value<Int64> m_iStartUptimeInMilliseconds;
	Atomic64Value<Int64> m_iUptimeInMilliseconds;
	ScopedPtr<Mutex> m_pUptimeMutex;
	ScopedPtr<Signal> m_pUptimeSignal;
	ScopedPtr<Thread> m_pUptimeThread;
	Atomic32Value<Bool> m_bUptimeThreadRunning;

	Int UptimeWorker(const Thread& thread);

private:
	ScopedPtr<AnalyticsManager> m_pAnalyticsManager;
	ScopedPtr<AssetManager> m_pAssetManager;
	ScopedPtr<CommerceManager> m_pCommerceManager;
	ScopedPtr<SettingsManager> m_pSettingsManager;
	ScopedPtr<Jobs::Manager> m_pJobManager;
	ScopedPtr<Content::LoadManager> m_pContentLoadManager;
	ScopedPtr<CookManager> m_pCookManager;
	ScopedPtr<TextureManager> m_pTextureManager;
	ScopedPtr<MaterialManager> m_pMaterialManager;
	ScopedPtr<EffectManager> m_pEffectManager;
	ScopedPtr<Renderer> m_pRenderer;
	ScopedPtr<SaveLoadManager> m_pSaveLoadManager;
	ScopedPtr<Sound::Manager> m_pSoundManager;

	Atomic32Value<MouseCursor> m_eActiveMouseCursor;

	Bool m_bInitialized;
	Atomic32 m_PauseTimerActive;
	Atomic32Value<Bool> m_bSuppressOpenURL;
#if SEOUL_WITH_REMOTE_NOTIFICATIONS
	ScopedPtr<Mutex> m_pRemoteNotificationTokenMutex;
	String m_RemoteNotificationToken;
	Ternary m_DisplayRemoteNotificationToken;
#endif // /SEOUL_WITH_REMOTE_NOTIFICATIONS

	void InternalUpdateTimings();

	void InternalPreInitialize();
	void InternalPostShutdown();

	void InternalInitializeCore(const CoreSettings& settings);
	void InternalShutdownCore();

	void InternalInitializeInput();
	void InternalShutdownInput();

	void InternalInitializeLocManager();
	void InternalShutdownLocManager();

	SEOUL_DISABLE_COPY(Engine);
}; // class Engine

} // namespace Seoul

#endif // include guard
