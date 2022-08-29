/**
 * \file PCEngine.h
 * \brief Specialization of Engine for the PC platform. Handles
 * setup operations that are specific to PC.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef PC_ENGINE_H
#define PC_ENGINE_H

#include "Atomic32.h"
#include "Delegate.h"
#include "D3DCommonDeviceSettings.h"
#include "Engine.h"
#include "GenericAnalyticsManager.h"
#include "Mutex.h"
#include "Platform.h"
#include "Point2DInt.h"
#include "SaveLoadManagerSettings.h"
#include "Vector.h"
namespace Seoul { class PCScopedCom; }

namespace Seoul
{

// Forward declarations
class D3DCommonDevice;
class DirectInput;

typedef Delegate<void(const String&)> ReceiveIPCMessageDelegate;

/**
 * Settings used to configure an PCEngine subclass of engine at construction.
 */
struct PCEngineSettings
{
	PCEngineSettings()
		: m_RenderDeviceSettings()
		, m_SaveLoadManagerSettings()
		, m_AnalyticsSettings()
		, m_sBaseDirectoryPath()
		, m_bAllowMultipleProcesses(false)
		, m_bWarmStartCOM(false)
	{
	}

	/** D3D device settings. */
	D3DCommonDeviceSettings m_RenderDeviceSettings;

	/** Settings for the SaveLoadManager. */
	SaveLoadManagerSettings m_SaveLoadManagerSettings;

	/** Settings for Analytics, including API key and device information. */
	GenericAnalyticsManagerSettings m_AnalyticsSettings;

	/*
	 * (Optional) Override the base path used by GamePaths. If not specified,
	 * will be derived automatically from the executable location.
	 */
	String m_sBaseDirectoryPath;

	/**
	 * (Optional) When true, multiple copies of this process can
	 * run simultaneously.
	 */
	Bool m_bAllowMultipleProcesses;

	/**
	 * (Optional) When true, PCEngine will initialize COM for the main thread
	 * during construction, with the expectation of later COM operations from
	 * the main thread (e.g. querying recent documents or file dialogs).
	 *
	 * This value is optional. COM dependent operations will still function when
	 * this value is fast, but there may be additional overhead to initialize
	 * COM when those operations occur.
	 */
	Bool m_bWarmStartCOM;
};

/**
 * Settings saved to a user accessible location and used to control
 * settings such as full screen mode, resolution, etc.
 */
struct PCEngineUserSettings
{
	D3DCommonUserGraphicsSettings m_GraphicsSettings;
};

/**
 * Specialization of Engine for the PC platform. Performs PC-specific
 * setup and ticking, and also owns manager singletons that have PC-specific
 * implementations (i.e. D3DCommonObject).
 */
class PCEngine SEOUL_ABSTRACT: public Engine
{
public:
	/**
	 * @return The global singleton instance. Will be nullptr if that instance has not
	 * yet been created.
	 */
	static CheckedPtr<PCEngine> Get()
	{
		if (Engine::Get() && (Engine::Get()->GetType() == EngineType::kPCDefault || Engine::Get()->GetType() == EngineType::kSteam))
		{
			return (PCEngine*)Engine::Get().Get();
		}
		else
		{
			return CheckedPtr<PCEngine>();
		}
	}

	PCEngine(const PCEngineSettings& settings);
	virtual ~PCEngine();

	virtual void Initialize() SEOUL_OVERRIDE;
	virtual void Shutdown() SEOUL_OVERRIDE;

	// Manual refresh of Uptime.
	virtual void RefreshUptime() SEOUL_OVERRIDE SEOUL_SEALED;

	virtual Bool QueryProcessMemoryUsage(size_t& rzWorkingSetBytes, size_t& rzPrivateBytes) const SEOUL_OVERRIDE;

	/**
	 * @return true always, file dialogs supported on PC.
	 */
	virtual Bool SupportsPlatformFileDialogs()
	{
		return true;
	}

	// File dialog implementation.
	virtual Bool DisplayFileDialogSingleSelection(
		String& rsOutput,
		FileDialogOp eOp,
		const FileFilters& vFilters = FileFilters(),
		const String& sWorkingDirectory = String()) const SEOUL_OVERRIDE;

	// Recent documents implementation.
	virtual Bool GetRecentDocuments(
		GameDirectory eGameDirectory,
		FileType eFileType,
		RecentDocuments& rvRecentDocuments) const SEOUL_OVERRIDE;

	// Recent documents.
	virtual String GetRecentDocumentPath() const SEOUL_OVERRIDE;

	/** @return True if the current platform has default/native back button handling. */
	virtual Bool HasNativeBackButtonHandling() const SEOUL_OVERRIDE
	{
		return true;
	}

	// Get and set the system clipboard contents.
	virtual Bool SupportsClipboard() const SEOUL_OVERRIDE { return true; }
	virtual Bool ReadFromClipboard(String& rsOutput) SEOUL_OVERRIDE;
	virtual Bool WriteToClipboard(const String& sInput) SEOUL_OVERRIDE;

	/**
	 * Tells the platform to trigger native back button handling:
	 * - Android - this exist the Activity, switching to the previously active activity.
	 */
	virtual Bool PostNativeQuitMessage() SEOUL_OVERRIDE;

	/**
	 * @return Whether the current application has focus or not. An application
	 * loses focus when the user clicks on another application, other than
	 * the current Seoul engine app.
	 */
	virtual Bool HasFocus() const SEOUL_OVERRIDE
	{
		return m_bHasFocus;
	}

	// Installs the current executable as a URL protocol handler for the given
	// URL protocol for the current user
	void InstallURLHandler(const String& sProtocol, const String& sDescription);

	// Returns true and assigns the value to rsData on success, false otherwise.
	Bool GetRegistryValue(HKEY hKey, const String& sSubKey, String& rsData) const;

	// Writes a value to the Windows registry
	Bool WriteRegistryValue(HKEY hKey, const String& sSubKey, const String& sValueName, const String& sData);

	virtual Bool Tick() SEOUL_OVERRIDE;
	virtual Bool RenderThreadPumpMessageQueue() SEOUL_OVERRIDE;

	/**
	 * @return The Win32 HINSTANCE handle to the current application.
	 */
	HINSTANCE GetApplicationInstance() const
	{
		return m_Settings.m_RenderDeviceSettings.m_hInstance;
	}

	/** @return The status of m_bQuit tracking. */
	Bool WantsQuit() const { return m_bQuit; }

	void InternalUpdateCursor();

	/** Tests if we're currently in a modal Windows message loop (PC-only) */
	virtual Bool IsInModalWindowsLoop() const
	{
		return m_bInModalWindowsLoop;
	}

	virtual SaveApi* CreateSaveApi() SEOUL_OVERRIDE;

	virtual String GetSystemLanguage() const SEOUL_OVERRIDE;

	virtual Bool UpdatePlatformUUID(const String& sPlatformUUID) SEOUL_OVERRIDE;

	// IPC hook for registering for custom URL events (e.g. myprotocol://).
	void SetIPCMessageCallback(ReceiveIPCMessageDelegate messageDelegate);
	void SetIPCPipeName(const String& sPipeName);

	void OnModalWindowsLoopEntered();
	void OnModalWindowsLoopExited();

protected:
	// Get the key used for storing the application's UUID.
	String GetPlatformUUIDRegistrySubkey() const;
	// Gets the string to use for application-specific registry settings, i.e. "Software\\[Company]\\[AppName]\\"
	String GetRegistrySubkeyAppRoot() const;

private:
	virtual Bool InternalOpenURL(const String& sURL) SEOUL_OVERRIDE;

	virtual void InternalStartTextEditing(ITextEditable* pTextEditable, const String& /*sText*/, const String& /*sDescription*/, const StringConstraints& /*constraints*/, Bool bAllowNonLatinKeyboard) SEOUL_OVERRIDE;

	// Handle Win32 messages
	static LRESULT CALLBACK MessageProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	// Handle Win32 timer messages
	static void CALLBACK WindowsTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);

	Bool InternalRenderThreadPumpMessageQueue();

	void InternalCheckExistingGameProcesses();
	void InternalInitializeAppNameAndVersionString();
	void InternalInitializePreferredRenderBackend();

	void InternalPCPreInitialize(String& rsLogName);

	void InternalInitializeDirectInput();
	void InternalShutdownDirectInput();

	void InternalInitializeIPC();
	void InternalTickIPCPipe();

	virtual void SetGDPRAccepted(Bool bAccepted) SEOUL_OVERRIDE;
	virtual Bool GetGDPRAccepted() const SEOUL_OVERRIDE;

	// Helper functions for doing synchronous I/O on our asynchronous pipe
	// handle
	Bool ReadPipeSynchronous(char *pBuffer, DWORD uNumberOfBytesToRead, DWORD *puNumberOfBytesRead);
	Bool WritePipeSynchronous(const char *pBuffer, DWORD uNumberOfBytesToWrite, DWORD *puNumberOfBytesWritten);

protected:
	void InternalShutdownIPC();

	virtual AnalyticsManager* InternalCreateAnalyticsManager() SEOUL_OVERRIDE;
	virtual CookManager* InternalCreateCookManager() SEOUL_OVERRIDE;
	virtual PlatformSignInManager* InternalCreatePlatformSignInManager() SEOUL_OVERRIDE;
	virtual Sound::Manager* InternalCreateSoundManager() SEOUL_OVERRIDE;

	virtual void InternalInitPlatformUUID();
	void InternalSetIfFirstRun();

	PCEngineSettings m_Settings;
	PCEngineUserSettings m_UserSettings;
	Mutex m_UserSettingsMutex;

	void InternalLoadUserSettings();
	void InternalSaveUserSettings();

	String m_sAppName;
	String m_sCompanyName;

private:
	static void InternalCallTextEditableApplyChar(UniChar c);
	Bool InternalGetProcessAbsolutePath(HANDLE hProcess, WString& rs) const;

	Point2DInt m_RenderThreadLastMousePosition;
	ScopedPtr<PCScopedCom> m_pWarmStartCom;
	ScopedPtr<D3DCommonDevice> m_pD3DCommonDevice;

	Bool m_bHasFocus;

	/** Are we in a modal Windows loop? */
	Atomic32Value<Bool> m_bInModalWindowsLoop;

	/** ID of the timer used for ticking the engine during modal loops */
	UINT_PTR m_uModalTimerID;

	ReceiveIPCMessageDelegate m_ReceiveIPCMessageDelegate;

	Int64 m_iAdditionalUptimeInMilliseconds;
	DWORD m_uReportedUptimeInMilliseconds;

	// For IPC using named pipes
	String m_sPipeName;
	OVERLAPPED m_Overlapped;
	HANDLE m_hPipeEvent;
	HANDLE m_hPipeReadWriteEvent;
	HANDLE m_hPipe;
	char m_aPipeBuffer[4096];
	HMODULE m_hPsapi;
	void* m_pGetModuleFileNameExW;
	void* m_pGetProcessMemoryInfo;
	Atomic32Value<Bool> m_bQuit;
	Atomic32Value<Bool> m_bActive;
	Bool m_bLastActive;
	Bool m_bMuteAudioWhenInactive;
	Bool m_bEnableControllerSupport;

	SEOUL_DISABLE_COPY(PCEngine);
}; // class PCEngine

} // namespace Seoul

#endif // include guard
