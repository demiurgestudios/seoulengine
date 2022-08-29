/**
 * \file PCEngine.cpp
 * \brief Specialization of Engine for the PC platform. Handles
 * setup operations that are specific to PC.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#define SEOUL_ENABLE_CTLMGR 1 // For CoCreateInstance, etc.

#include "BuildChangelistPublic.h"
#include "BuildVersion.h"
#include "ContentChangeNotifierLocal.h"
#include "CookManagerMoriarty.h"
#include "CoreSettings.h"
#include "D3DCommonDevice.h"
#include "EncryptAES.h"
#include "Engine.h"
#include "EngineCommandLineArgs.h"
#include "FileManager.h"
#include "FMODSoundManager.h"
#include "EventsManager.h"
#include "GamePaths.h"
#include "JobsFunction.h"
#include "GenericAnalyticsManager.h"
#include "GenericSaveApi.h"
#include "InputManager.h"
#include "ITextEditable.h"
#include "JobsManager.h"
#include "LocManager.h"
#include "MoriartyClient.h"
#include "PackageFileSystem.h"
#include "Path.h"
#include "PCCookManager.h"
#include "PCEngine.h"
#include "PCInput.h"
#include "PCScopedCom.h"
#include "PCXInput.h"
#include "PlatformData.h"
#include "PlatformSignInManager.h"
#include "ReflectionDataStoreTableUtil.h"
#include "ReflectionDefine.h"
#include "ReflectionSerialize.h"
#include "ScopedAction.h"
#include "SettingsManager.h"
#include "SeoulUtil.h"
#include "SoundManager.h"
#include "StackOrHeapArray.h"
#include "StringUtil.h"

#include <shellapi.h>
#include <ShlObj.h>
#include <ShObjIdl.h>
#include <Tlhelp32.h>

// For GetAdapterAddresses (winsock2 must be before IPHlpApi)
#include <winsock2.h>
#include <IPHlpApi.h>

// Definition of _PROCESS_MEMORY_COUNTERS and getter.
extern "C"
{

typedef struct _PROCESS_MEMORY_COUNTERS_EX {
	DWORD  cb;
	DWORD  PageFaultCount;
	SIZE_T PeakWorkingSetSize;
	SIZE_T WorkingSetSize;
	SIZE_T QuotaPeakPagedPoolUsage;
	SIZE_T QuotaPagedPoolUsage;
	SIZE_T QuotaPeakNonPagedPoolUsage;
	SIZE_T QuotaNonPagedPoolUsage;
	SIZE_T PagefileUsage;
	SIZE_T PeakPagefileUsage;
	SIZE_T PrivateUsage;
} PROCESS_MEMORY_COUNTERS_EX;
typedef PROCESS_MEMORY_COUNTERS_EX *PPROCESS_MEMORY_COUNTERS_EX;

typedef BOOL (WINAPI *GetProcessMemoryInfo)(HANDLE, PPROCESS_MEMORY_COUNTERS_EX, DWORD cb);
typedef DWORD (WINAPI *GetModuleFileNameExW)(HANDLE, HMODULE, LPWSTR, DWORD);

} // extern "C"

namespace Seoul
{

/** Constants used to extract configuration values from application.json. */
static const HString ksApplication("Application");
static const HString ksApplicationName("ApplicationName");
static const HString ksCompanyName("CompanyName");
static const HString ksLocalizedApplicationToken("LocalizedApplicationToken");
static const HString ksMuteAudioWhenInactive("MuteAudioWhenInactive");
static const HString ksEnablePCControllerSupport("EnablePCControllerSupport");
static const HString ksPCRenderBackend("PCRenderBackend");

// GDPR Version for PC. Note that each platform maintains its own version.
static const Int s_iGDPRVersion = 1;

SEOUL_BEGIN_TYPE(PCEngineUserSettings)
	SEOUL_PROPERTY_N("GraphicsSettings", m_GraphicsSettings)
		SEOUL_ATTRIBUTE(NotRequired)
SEOUL_END_TYPE()

/**
 * Utility used to dispatch opening a URL (using ShellExecuteW)
 * on the render thread.
 */
class OpenURLJob SEOUL_SEALED : public Jobs::Job
{
public:
	OpenURLJob(const String& sURL)
		: Job(GetRenderThreadId())
		, m_sURL(sURL)
		, m_bResult(false)
	{
	}

	~OpenURLJob()
	{
		WaitUntilJobIsNotRunning();
	}

	Bool GetResult() const
	{
		return m_bResult;
	}

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(OpenURLJob);

	virtual void InternalExecuteJob(Jobs::State& reNextState, ThreadId& rNextThreadId) SEOUL_OVERRIDE
	{
		m_bResult = Engine::Get()->OpenURL(m_sURL);
		reNextState = Jobs::State::kComplete;
	}

	String m_sURL;
	Bool m_bResult;

	SEOUL_DISABLE_COPY(OpenURLJob);
}; // class OpenURLJob

/**
 * Alternative implementation of TerminateProcess(), invokes a thread in
 * our target process which calls ExitProcess(). From:
 * http://www.drdobbs.com/a-safer-alternative-to-terminateprocess/184416547
 */
static BOOL SafeTerminateProcess(HANDLE hProcess, UINT uExitCode, DWORD uWaitTimeInMilliseconds = INFINITE)
{
	DWORD dwTID, dwCode, dwErr = 0;
	HANDLE hProcessDup = INVALID_HANDLE_VALUE;
	HANDLE hRT = nullptr;
	HINSTANCE hKernel = GetModuleHandle("Kernel32");
	BOOL bSuccess = FALSE;

	BOOL bDup = DuplicateHandle(
		GetCurrentProcess(),
		hProcess,
		GetCurrentProcess(),
		&hProcessDup,
		PROCESS_ALL_ACCESS,
		FALSE,
		0);

	// Detect the special case where the process is
	// already dead...
	if (FALSE != ::GetExitCodeProcess((bDup) ? hProcessDup : hProcess, &dwCode) &&
		(dwCode == STILL_ACTIVE))
	{
		FARPROC pfnExitProc = ::GetProcAddress(hKernel, "ExitProcess");

		hRT = ::CreateRemoteThread((bDup) ? hProcessDup : hProcess,
			nullptr,
			0,
			(LPTHREAD_START_ROUTINE)pfnExitProc,
			(PVOID)(size_t)uExitCode, 0, &dwTID);

		if (hRT == nullptr)
		{
			dwErr = GetLastError();
		}
	}
	else
	{
		dwErr = ERROR_PROCESS_ABORTED;
	}

	if (nullptr != hRT)
	{
		// Must wait process to terminate to
		// guarantee that it has exited...
		if (WAIT_OBJECT_0 == ::WaitForSingleObject((bDup) ? hProcessDup : hProcess, uWaitTimeInMilliseconds))
		{
			bSuccess = TRUE;
		}

		SEOUL_VERIFY(FALSE != ::CloseHandle(hRT));
	}

	if (FALSE != bDup)
	{
		SEOUL_VERIFY(FALSE != ::CloseHandle(hProcessDup));
	}

	if (FALSE == bSuccess)
	{
		::SetLastError(dwErr);
	}

	return bSuccess;
}

namespace
{

static Atomic32Value<Bool> s_bPumpShutdown(false);
static Atomic32Value<Bool> s_bPumpRunning(false);
static SharedPtr<Jobs::Job> s_pPumpJob;

void Pump()
{
	// Done, terminate.
	if (s_bPumpShutdown)
	{
		s_pPumpJob.Reset();
		SeoulMemoryBarrier();
		s_bPumpRunning = false;
		return;
	}

	// Only pump if not in the middle of a scene call.
	if (!D3DCommonDevice::Get()->IsInScene())
	{
		Engine::Get()->RenderThreadPumpMessageQueue();
	}

	// Reschedule another pump.
	s_pPumpJob = Jobs::MakeFunction(GetRenderThreadId(), &Pump);
	s_pPumpJob->SetJobQuantum(Jobs::Quantum::kDisplayRefreshPeriodic);
	s_pPumpJob->StartJob();
}

} // namespace anonymous

PCEngine::PCEngine(const PCEngineSettings& settings)
	: Engine()
	, m_Settings(settings)
	, m_sAppName()
	, m_sCompanyName()
	, m_pWarmStartCom(settings.m_bWarmStartCOM ? SEOUL_NEW(MemoryBudgets::TBD) PCScopedCom : nullptr)
	, m_pD3DCommonDevice()
	, m_bHasFocus(false)
	, m_bInModalWindowsLoop(false)
	, m_uModalTimerID(0)
	, m_ReceiveIPCMessageDelegate()
	, m_iAdditionalUptimeInMilliseconds(0)
	, m_uReportedUptimeInMilliseconds(::GetTickCount())
	, m_sPipeName()
	, m_hPipeEvent(nullptr)
	, m_hPipeReadWriteEvent(nullptr)
	, m_hPipe(nullptr)
	, m_hPsapi(nullptr)
	, m_pGetModuleFileNameExW(nullptr)
	, m_pGetProcessMemoryInfo(nullptr)
	, m_bQuit(false)
	, m_bActive(true)
	, m_bLastActive(true)
	, m_bMuteAudioWhenInactive(false)
	, m_bEnableControllerSupport(false)
{
	memset(&m_Overlapped, 0, sizeof(m_Overlapped));
	memset(m_aPipeBuffer, 0, sizeof(m_aPipeBuffer));

	m_hPsapi = ::LoadLibraryW(L"Psapi.dll");
	if (nullptr != m_hPsapi)
	{
		m_pGetModuleFileNameExW = (void*)::GetProcAddress(m_hPsapi, "GetModuleFileNameExW");
		m_pGetProcessMemoryInfo = (void*)::GetProcAddress(m_hPsapi, "GetProcessMemoryInfo");
	}

	if (nullptr == m_Settings.m_RenderDeviceSettings.m_pWndProc)
	{
		m_Settings.m_RenderDeviceSettings.m_pWndProc = &PCEngine::MessageProcedure;
	}

	m_iStartUptimeInMilliseconds = (Int64)m_uReportedUptimeInMilliseconds;
	m_iUptimeInMilliseconds = m_iStartUptimeInMilliseconds;
}

PCEngine::~PCEngine()
{
	m_pGetProcessMemoryInfo = nullptr;
	m_pGetModuleFileNameExW = nullptr;
	if (nullptr != m_hPsapi)
	{
		SEOUL_VERIFY(FALSE != ::FreeLibrary(m_hPsapi));
		m_hPsapi = nullptr;
	}
}

/**
 * Performs PC specific Engine initialization.
 */
void PCEngine::Initialize()
{
	// Get basic values needed by Engine::InternalPreRenderDeviceInitialization()
	CoreSettings settings;
	settings.m_GamePathsSettings.m_sBaseDirectoryPath = m_Settings.m_sBaseDirectoryPath;
	InternalPCPreInitialize(settings.m_sLogName);

	// Perform Engine initialization prior to creating the render device.
	InternalPreRenderDeviceInitialization(
		settings,
		m_Settings.m_SaveLoadManagerSettings);

	// Prior to settings load, initialize the preferred render backend.
	InternalInitializePreferredRenderBackend();

	{
		Lock lock(m_UserSettingsMutex);

		// Load the initial user settings
		InternalLoadUserSettings();

		// Commit the user settings - this makes sure
		// that the user settings exist as soon as possible (before any potential
		// failures) and have been brought up to date.
		InternalSaveUserSettings();
	}

	// If multiple processes are not enabled, check that now.
	if (!m_Settings.m_bAllowMultipleProcesses)
	{
		InternalCheckExistingGameProcesses();
	}

	InternalInitializeIPC();

	// The app name and version string is needed by D3DCommonDevice, but
	// can't be initialized until after basic Engine initialization
	// (it depends on GamePaths::Get()).
	InternalInitializeAppNameAndVersionString();

	// Initialize the first run flag.
	InternalSetIfFirstRun();

	// Instantiate the D3DCommonDevice.
	struct Internal
	{
		static void InitializeD3DCommonDevice(PCEngine* pPC)
		{
			pPC->m_pD3DCommonDevice.Reset(D3DCommonDevice::CreateD3DDevice(pPC->m_Settings.m_RenderDeviceSettings));

			// Enable drag file support in non-ship builds.
#if !SEOUL_SHIP
			if (pPC->m_pD3DCommonDevice.IsValid() && pPC->m_pD3DCommonDevice->PCEngineFriend_GetMainWindow())
			{
				::DragAcceptFiles(pPC->m_pD3DCommonDevice->PCEngineFriend_GetMainWindow(), TRUE);
			}
#endif // /!SEOUL_SHIP
		}
	};

	m_Settings.m_RenderDeviceSettings.m_UserSettings = m_UserSettings.m_GraphicsSettings;
	Jobs::AwaitFunction(GetRenderThreadId(), &Internal::InitializeD3DCommonDevice, this);

	// Perform post render device Engine setup.
	Engine::InternalPostRenderDeviceInitialization();

	// We can now safely instantiate this platform's input capture,
	// as well as objects which can depend on rendering and input.
	InternalInitializeDirectInput();

	InternalInitPlatformUUID();

	// Perform final initialization tasks.
	Engine::InternalPostInitialization();

	// Absolute last step, kick off our render thread pump.
	s_bPumpRunning = true;
	s_pPumpJob = Jobs::MakeFunction(GetRenderThreadId(), &Pump);
	s_pPumpJob->SetJobQuantum(Jobs::Quantum::kDisplayRefreshPeriodic);
	s_pPumpJob->StartJob();
}

/**
 * Performs PC specific Engine shutdown.
 *
 * All tasks must maintain an LIFO order with respect
 * to equivalent tasks in PCEngine::Initialize().
 */
void PCEngine::Shutdown()
{
	// Shutdown the message pump.
	s_bPumpShutdown = true;
	while (s_bPumpRunning)
	{
		Jobs::Manager::Get()->YieldThreadTime();
	}
	// Sanity check, job should have cleaned itself up.
	SEOUL_ASSERT(!s_pPumpJob.IsValid());

	// Perform basic first step shutdown tasks in engine
	Engine::InternalPreShutdown();

	// Shutdown objects that were initialized after render device setup.
	InternalShutdownDirectInput();

	// Shutdown Engine's components that were created after the render
	// device
	Engine::InternalPreRenderDeviceShutdown();

	// Destroy the render device
	struct Internal
	{
		static void DestroyD3DCommonDevice(PCEngine* pPC)
		{
			{
				Lock lock(pPC->m_UserSettingsMutex);
				pPC->m_pD3DCommonDevice->MergeUserGraphicsSettings(pPC->m_UserSettings.m_GraphicsSettings);
			}
			pPC->m_pD3DCommonDevice.Reset();
		}
	};

	Jobs::AwaitFunction(GetRenderThreadId(), &Internal::DestroyD3DCommonDevice, this);

	InternalShutdownIPC();

	{
		Lock lock(m_UserSettingsMutex);

		// Commit the user settings.
		InternalSaveUserSettings();
	}

	// Perform final shutdown tasks.
	Engine::InternalPostRenderDeviceShutdown();
}

static Bool SetDialogFileFilters(IFileDialog& r, const Engine::FileFilters& vFilters)
{
	if (vFilters.IsEmpty())
	{
		return true;
	}

	// Buffers for conversion.
	auto const uSize = vFilters.GetSize();
	Vector< Pair<WString, WString> > vTemp(uSize);
	Vector<COMDLG_FILTERSPEC> vSpecs(uSize);
	for (UInt32 i = 0u; i < uSize; ++i)
	{
		// Storage.
		vTemp[i].First = vFilters[i].m_sFriendlyName.WStr();
		vTemp[i].Second = vFilters[i].m_sPattern.WStr();

		// Assignment.
		vSpecs[i].pszName = vTemp[i].First;
		vSpecs[i].pszSpec = vTemp[i].Second;
	}

	// Done.
	return (SUCCEEDED(r.SetFileTypes((UINT)uSize, vSpecs.Data())));
}

static Bool SetDialogWorkingDirectory(IFileDialog& r, const String& sWorkingDirectory)
{
	if (sWorkingDirectory.IsEmpty())
	{
		return true;
	}

	IShellItem* pWorkingDirectory = nullptr;
	auto const deferred(MakeDeferredAction([&]() { SafeRelease(pWorkingDirectory); }));
	if (FAILED(::SHCreateItemFromParsingName(sWorkingDirectory.WStr(), nullptr, IID_PPV_ARGS(&pWorkingDirectory))))
	{
		return false;
	}

	return (SUCCEEDED(r.SetFolder(pWorkingDirectory)));
}

/** File dialog implementation. */
Bool PCEngine::DisplayFileDialogSingleSelection(
	String& rsOutput,
	FileDialogOp eOp,
	const FileFilters& vFilters /*= FileFilters()*/,
	const String& sWorkingDirectory /*= String()*/) const
{
	// Handle com initialization.
	PCScopedCom com;

	// Create the dialog.
	IFileDialog* pDialog = nullptr;
	auto const deferredDialog(MakeDeferredAction([&]() { SafeRelease(pDialog); }));
	if (FAILED(::CoCreateInstance(
		(FileDialogOp::kOpen == eOp) ? CLSID_FileOpenDialog : CLSID_FileSaveDialog,
		nullptr,
		CLSCTX_INPROC_SERVER,
		(FileDialogOp::kOpen == eOp) ? IID_IFileOpenDialog : IID_IFileSaveDialog,
		(LPVOID*)&pDialog)))
	{
		return false;
	}

	// Commit the file filters.
	if (!SetDialogFileFilters(*pDialog, vFilters))
	{
		return false;
	}

	// Commit the working directory.
	if (!SetDialogWorkingDirectory(*pDialog, sWorkingDirectory))
	{
		return false;
	}

	// Display.
	if (FAILED(pDialog->Show(nullptr)))
	{
		return false;
	}

	// Acquire the result.
	IShellItem* pShellItem = nullptr;
	auto const deferredItem(MakeDeferredAction([&]() { SafeRelease(pShellItem); }));
	if (FAILED(pDialog->GetResult(&pShellItem)))
	{
		return false;
	}

	// Get the file name
	LPWSTR pFileSysPath = nullptr;
	auto const deferredFileSysPath(MakeDeferredAction([&]() { ::CoTaskMemFree(pFileSysPath); pFileSysPath = nullptr; }));
	if (FAILED(pShellItem->GetDisplayName(SIGDN_FILESYSPATH, &pFileSysPath)) || nullptr == pFileSysPath)
	{
		return false;
	}

	// Done.
	rsOutput = Path::GetExactPathName(WCharTToUTF8(pFileSysPath));
	return true;
}

/** Implementation of GetRecentDocuments() for PC. */
Bool PCEngine::GetRecentDocuments(
	GameDirectory eGameDirectory,
	FileType eFileType,
	RecentDocuments& rvRecentDocuments) const
{
	WCHAR aPath[MAX_PATH] = { 0 };

	// Lookup the path to recent documents.
	if (FAILED(::SHGetFolderPathW(
		nullptr,
		CSIDL_RECENT,
		nullptr,
		SHGFP_TYPE_CURRENT,
		&aPath[0])))
	{
		return false;
	}

	Vector<String> vsShortcuts;

	// Iterate over all .lnk (shortcut) files.
	if (!FileManager::Get()->GetDirectoryListing(
		WCharTToUTF8(aPath),
		vsShortcuts,
		false,
		false,
		".lnk"))
	{
		return false;
	}

	// Handle com initialization.
	PCScopedCom com;

	// Resolve the shortcuts.
	IShellLink* pShellLink = nullptr;
	if (FAILED(::CoCreateInstance(
		CLSID_ShellLink,
		nullptr,
		CLSCTX_INPROC_SERVER,
		IID_IShellLink,
		(LPVOID*)&pShellLink)))
	{
		return false;
	}

	// Iterate over all shortcuts and append any appropriate documents.
	RecentDocuments vRecentDocuments;
	for (auto const& sShortcut : vsShortcuts)
	{
		// Initiate the resolve...
		IPersistFile* pLinkFile = nullptr;
		if (FAILED(pShellLink->QueryInterface(IID_IPersistFile, (LPVOID*)&pLinkFile)))
		{
			continue;
		}

		// ...finish the resolve.
		if (FAILED(pLinkFile->Load(sShortcut.WStr(), STGM_READ)))
		{
			SafeRelease(pLinkFile);
			continue;
		}

		// Now query the link for the absolute path to the actual file.
		CHAR aLinkPath[MAX_PATH] = { 0 };
		if (FAILED(pShellLink->GetPath(aLinkPath, SEOUL_ARRAY_COUNT(aLinkPath), nullptr, 0u)))
		{
			SafeRelease(pLinkFile);
			continue;
		}

		// Convert the path and check it against the given file type.
		String const sPath(aLinkPath);
		FilePath const filePath(FilePath::CreateFilePath(eGameDirectory, sPath));
		if (filePath.IsValid() &&
			filePath.GetDirectory() == eGameDirectory &&
			filePath.GetType() == eFileType &&
			FileManager::Get()->ExistsInSource(filePath))
		{
			vRecentDocuments.PushBack(filePath);
		}

		// Free up the link file.
		SafeRelease(pLinkFile);
	}

	// Free up the shell link.
	SafeRelease(pShellLink);

	// Done - swap and return.
	rvRecentDocuments.Swap(vRecentDocuments);
	return true;
}

// Recent documents.
String PCEngine::GetRecentDocumentPath() const
{
	WCHAR aPath[MAX_PATH] = { 0 };

	// Lookup the path to recent documents.
	if (FAILED(SHGetFolderPathW(
		nullptr,
		CSIDL_RECENT,
		nullptr,
		SHGFP_TYPE_CURRENT,
		&aPath[0])))
	{
		return String();
	}

	return WCharTToUTF8(aPath);
}

/** Get the system clipboard contents. */
Bool PCEngine::ReadFromClipboard(String& rsOutput)
{
	// Open the clipboard - this can fail if another window has focus, so handle
	// that gracefully. Note that, despite the MSDN documentation, we want
	// to use nullptr here, not the window handle, as it can cause hangs if we
	// interact with the window handle from a thread other than the render thread.
	if (FALSE == ::OpenClipboard(nullptr))
	{
		return false;
	}

	// First try to get the data as Unicode text - if that fails,
	// request plain (ASCII) text.
	Bool bUnicodeData = true;
	HGLOBAL hClipboardData = ::GetClipboardData(CF_UNICODETEXT);
	if (nullptr == hClipboardData)
	{
		hClipboardData = ::GetClipboardData(CF_TEXT);
		if (nullptr == hClipboardData)
		{
			// If we failed getting data of either type, close the clipboard
			// and fail to read from the clipboard.
			SEOUL_VERIFY(FALSE != ::CloseClipboard());
			return false;
		}

		bUnicodeData = false;
	}

	// Lock the clipboard data for reading.
	void* pData = ::GlobalLock(hClipboardData);
	SEOUL_ASSERT(nullptr != pData);

	// Copy the data as either wide characters or ascii text.
	if (bUnicodeData)
	{
		// Wide-char data.
		rsOutput = WCharTToUTF8((wchar_t const*)pData);
	}
	else
	{
		// ASCII data.
		rsOutput.Assign((Byte const*)pData);
	}

	// Release the clipboard data and the clipboard and return success.

	// See: https://msdn.microsoft.com/en-us/library/windows/desktop/aa366595%28v=vs.85%29.aspx
	// Despite returning BOOL, this function's return codes are essentially
	// an existing ref count after the unlock.
	//
	// NOTE: Not safe to verify 0u == ::GlobalUnlock here. Not entirely sure why -
	// my assumption is that when dealing with the clipboard, external applications
	// can lock the clipboard.
	(void)::GlobalUnlock(hClipboardData);
	SEOUL_VERIFY(FALSE != ::CloseClipboard());
	return true;
}

/** Set the system clipboard contents. */
Bool PCEngine::WriteToClipboard(const String& sInput)
{
	// Open the clipboard - this can fail if another window has focus, so handle
	// that gracefully. Note that, despite the MSDN documentation, we want
	// to use nullptr here, not the window handle, as it can cause hangs if we
	// interact with the window handle from a thread other than the render thread.
	if (FALSE == ::OpenClipboard(nullptr))
	{
		return false;
	}

	// Releases any data currently associated with the clipboard.
	if (FALSE == ::EmptyClipboard())
	{
		SEOUL_VERIFY(FALSE != ::CloseClipboard());
		return false;
	}

	// Convert the string to wide characters.
	WString const wideInput(sInput.WStr());

	// Allocate enough space for the wchar version of the input string.
	HGLOBAL const hInput = ::GlobalAlloc(GMEM_MOVEABLE, (wideInput.GetLengthInChars() + 1u) * sizeof(wchar_t));
	if (nullptr == hInput)
	{
		SEOUL_VERIFY(FALSE != ::CloseClipboard());
		return false;
	}

	// Lock the memory, than copy the string into it.
	{
		wchar_t* pData = (wchar_t*)::GlobalLock(hInput);
		if (nullptr == pData)
		{
			SEOUL_VERIFY(nullptr == ::GlobalFree(hInput));
			SEOUL_VERIFY(FALSE != ::CloseClipboard());
			return false;
		}

		memcpy(
			(void*)pData,
			(void const*)((wchar_t const*)wideInput),
			(wideInput.GetLengthInChars() * sizeof(wchar_t)));
		pData[wideInput.GetLengthInChars()] = (wchar_t)'\0';

		// See: https://msdn.microsoft.com/en-us/library/windows/desktop/aa366595%28v=vs.85%29.aspx
		// Despite returning BOOL, this function's return codes are essentially
		// an existing ref count after the unlock.
		SEOUL_VERIFY(0 == ::GlobalUnlock(hInput));
	}

	// Commit the new clipboard data.
	if (nullptr == ::SetClipboardData(CF_UNICODETEXT, hInput))
	{
		SEOUL_VERIFY(nullptr == ::GlobalFree(hInput));
		SEOUL_VERIFY(FALSE != ::CloseClipboard());
		return false;
	}

	// Done with the clipboard.
	SEOUL_VERIFY(FALSE != ::CloseClipboard());
	return true;
}

/**
 * Tells the platform to trigger native back button handling:
 * - Android - this exist the Activity, switching to the previously active activity.
 */
Bool PCEngine::PostNativeQuitMessage()
{
	struct Internal
	{
		static void DoPostQuit()
		{
			::PostQuitMessage(0);
		}
	};

	{
		// Asynchronously call the action on the render thread.
		Jobs::AsyncFunction(GetRenderThreadId(), &Internal::DoPostQuit);
		return true;
	}
}

/**
 * Implementation of Engine::OpenURL() for PC - uses ShellExecute()
 * top open the URL with the default browser for the current system.
 *
 * @return True if the URL was opened successfully, false otherwise.
 */
Bool PCEngine::InternalOpenURL(const String& sURL)
{
	// ShellExecuteW can hang in this use case on Windows XP if it is not run on the render thread.
	if (!IsRenderThread())
	{
		SharedPtr<OpenURLJob> pJob(SEOUL_NEW(MemoryBudgets::Jobs) OpenURLJob(sURL));
		pJob->StartJob();
		pJob->WaitUntilJobIsNotRunning();
		return pJob->GetResult();
	}

	// TODO: Always true?
	Bool const bOpen = (!sURL.StartsWith("file://"));

	// See http://msdn.microsoft.com/en-us/library/windows/desktop/bb762153%28v=vs.85%29.aspx for
	// what's going on here with the return value.
	Int iResult = (Int)(size_t)::ShellExecuteW(
		nullptr,
		(bOpen ? L"open" : nullptr),
		sURL.WStr(),
		nullptr,
		nullptr,
		SW_SHOWNORMAL);

	return (iResult > 32);
}

void PCEngine::RefreshUptime()
{
	DWORD const uUptimeInMilliseconds = ::GetTickCount();

	Lock lock(*m_pUptimeMutex);

	// Need to handle the case where the uptime has wrapped around. GetTickCount64() is not available
	// until Windows 7.
	if (uUptimeInMilliseconds < m_uReportedUptimeInMilliseconds)
	{
		m_iAdditionalUptimeInMilliseconds += (Int64)m_uReportedUptimeInMilliseconds;
	}

	m_uReportedUptimeInMilliseconds = uUptimeInMilliseconds;
	m_iUptimeInMilliseconds = ((Int64)m_uReportedUptimeInMilliseconds + m_iAdditionalUptimeInMilliseconds);
}

Bool PCEngine::QueryProcessMemoryUsage(size_t& rzWorkingSetBytes, size_t& rzPrivateBytes) const
{
	if (nullptr == m_pGetProcessMemoryInfo)
	{
		return false;
	}

	auto pFunc = (GetProcessMemoryInfo)m_pGetProcessMemoryInfo;

	PROCESS_MEMORY_COUNTERS_EX counters;
	memset(&counters, 0, sizeof(counters));
	counters.cb = sizeof(counters);

	if (FALSE == pFunc(::GetCurrentProcess(), &counters, sizeof(counters)))
	{
		return false;
	}

	rzWorkingSetBytes = (size_t)counters.WorkingSetSize;
	rzPrivateBytes = (size_t)counters.PrivateUsage;
	return true;
}

/**
 * Perform tick loop operations - for PCEngine, this is where
 * the Win32 message pump is processed, where we determine if
 * the game has focus or not, and where various developer only
 * bindings are processed.
 */
Bool PCEngine::Tick()
{
	Bool const bActive = m_bActive;
	SeoulMemoryBarrier();

	// Update the active flag.
	if (m_bLastActive != bActive)
	{
		// Special handling on PC, mute audio when not the active application.
		if (m_bMuteAudioWhenInactive)
		{
			Sound::Manager::Get()->SetMasterMute(!bActive);
		}

		// Update the active flag.
		m_bLastActive = bActive;
	}

	// Perform base engine begin tick operations.
	Engine::InternalBeginTick();

	// Perform base engine end tick operations.
	Engine::InternalEndTick();

	// Various PCEngine binding handling.
	static const String kExitGameBinding("UI.ExitGame");
	static const String kToggleFullscreenBinding("UI.ToggleFullscreen");
	static const String kExitFullscreenBinding("UI.ExitFullscreen");
#if !SEOUL_SHIP
	static const String kToggleAspectRatioBinding("UI.ToggleAspectRatio");
#endif

	// Quit game binding - ALT+F4 by default.
	if (InputManager::Get()->WasBindingPressed(kExitGameBinding))
	{
		(void)PostNativeQuitMessage();
	}
	// Toggle fullscreen mode - ALT+ENTER by default.
	else if (InputManager::Get()->WasBindingPressed(kToggleFullscreenBinding))
	{
		struct Internal
		{
			static void DoToggleFullscreenMode()
			{
				D3DCommonDevice::Get()->ToggleFullscreenMode();
			}
		};

		// Asynchronously call the action on the render thread.
		Jobs::AsyncFunction(GetRenderThreadId(), &Internal::DoToggleFullscreenMode);
	}
	// Exit fullscreen mode if we're in fullscreen - ESCAPE by default.
	else if (InputManager::Get()->WasBindingPressed(kExitFullscreenBinding))
	{
		struct Internal
		{
			static void DoExitFullscreenMode()
			{
				if (!D3DCommonDevice::Get()->IsWindowed())
				{
					D3DCommonDevice::Get()->ToggleFullscreenMode();
				}
			}
		};

		// Asynchronously call the action on the render thread.
		Jobs::AsyncFunction(GetRenderThreadId(), &Internal::DoExitFullscreenMode);
	}
	// /Various PCEngine binding handling.

	InternalTickIPCPipe();

	return !m_bQuit;
}

// This exists to fix a bug where the cursor only changes if you move it.
// For example, if you enter a loading screen, you have to move the mouse before it will
// turn into the loading cursor. So, this function is called on tick to change the cursor
// if inside the client area.
void PCEngine::InternalUpdateCursor()
{
	SEOUL_ASSERT(IsRenderThread());

	// Early out if we can't get the current position for some reason.
	POINT cursorPoint;
	if (FALSE == ::GetCursorPos(&cursorPoint))
	{
		return;
	}

	// Early out if we failed acquiring the window handle.
	HWND hwnd = m_pD3DCommonDevice->PCEngineFriend_GetMainWindow();
	if (nullptr == hwnd)
	{
		return;
	}

	// Convert to relative coords
	SEOUL_VERIFY(FALSE != ::ScreenToClient(hwnd, &cursorPoint));

	RECT clientRect;
	memset(&clientRect, 0, sizeof(clientRect));
	SEOUL_VERIFY(FALSE != ::GetClientRect(hwnd, &clientRect));

	// Check if inside client area.
	if (cursorPoint.x >= clientRect.left &&
		cursorPoint.x < clientRect.right &&
		cursorPoint.y >= clientRect.top &&
		cursorPoint.y < clientRect.bottom)
	{
		// Commit the mouse cursor if one is defined.
		auto const hcursor = m_Settings.m_RenderDeviceSettings.m_aMouseCursors[(UInt32)GetMouseCursor()];
		if (hcursor)
		{
			// Apply the mouse cursor.
			(void)::SetCursor(hcursor);
		}
	}

	// Handle mouse outside window client (can happen beyond clientRect checks above due
	// to usage of window region functionality). If the reported cursors position is not
	// equalt to the current mouse position of the input manager, trigger an explicit mouse
	// move event.
	{
		Point2DInt const newPosition(cursorPoint.x, cursorPoint.y);
		if (newPosition != m_RenderThreadLastMousePosition)
		{
			if (auto pMouseDevice = InputManager::Get()->FindFirstMouseDevice())
			{
				m_RenderThreadLastMousePosition = newPosition;
				pMouseDevice->QueueMouseMoveEvent(newPosition);
			}
		}
	}
}

/*
 * Updates the named pipe server; sends off messages to registered
 * delegates when messages are received.
 */
void PCEngine::InternalTickIPCPipe()
{
	if (HasOverlappedIoCompleted(&m_Overlapped))
	{
		DWORD dwRead;
		if (ReadPipeSynchronous(m_aPipeBuffer, sizeof(m_aPipeBuffer) - 1, &dwRead))
		{
			m_aPipeBuffer[dwRead] = 0;

			SEOUL_LOG("Message:\"%s\"", m_aPipeBuffer);
			String sMessageContents = String(m_aPipeBuffer);

			// Send our PID back to the other instance so that it can call
			// AllowSetForegroundWindow() on us so we can steal the foreground
			char sTempBuffer[32];
			SNPRINTF(sTempBuffer, sizeof(sTempBuffer), "%u", GetCurrentProcessId());
			if (WritePipeSynchronous(sTempBuffer, (DWORD)strlen(sTempBuffer), nullptr))
			{
				// Read a dummy byte to wait for the other instance to close
				// the pipe, so that we can be sure it's finished calling
				// AllowSetForegroundWindow() by now.  This will fail right
				// away with ERROR_BROKEN_PIPE when the other instance closes
				// its end of the pipe.
				(void)ReadPipeSynchronous(sTempBuffer, 1, nullptr);
			}


			// Send the message to the receiver delegate
			if (m_ReceiveIPCMessageDelegate.IsValid())
			{
				m_ReceiveIPCMessageDelegate(sMessageContents);
			}
		}
		else
		{
			SEOUL_LOG("ReadFile on named pipe error: %d", GetLastError());
		}

		DisconnectNamedPipe(m_hPipe);
		ConnectNamedPipe(m_hPipe, &m_Overlapped);
	}
}

/**
 * Helper function for doing a synchronous ReadFile() on our asynchronous pipe
 * handle
 */
Bool PCEngine::ReadPipeSynchronous(char *pBuffer, DWORD uNumberOfBytesToRead, DWORD *puNumberOfBytesRead)
{
	OVERLAPPED overlapped;
	memset(&overlapped, 0, sizeof(overlapped));
	overlapped.hEvent = m_hPipeReadWriteEvent;

	if (ReadFile(m_hPipe, pBuffer, uNumberOfBytesToRead, puNumberOfBytesRead, &overlapped))
	{
		// Operation completed synchronously anyways
		return true;
	}

	if (GetLastError() != ERROR_IO_PENDING)
	{
		return false;
	}

	// Wait for the operation to complete
	DWORD uBytesTransferred;
	DWORD *puBytesTransferred = (puNumberOfBytesRead != nullptr) ? puNumberOfBytesRead : &uBytesTransferred;
	return (GetOverlappedResult(m_hPipe, &overlapped, puBytesTransferred, TRUE) != 0);
}

/**
 * Helper function for doing a synchronous WriteFile() on our asynchronous pipe
 * handle
 */
Bool PCEngine::WritePipeSynchronous(const char *pBuffer, DWORD uNumberOfBytesToWrite, DWORD *puNumberOfBytesWritten)
{
	OVERLAPPED overlapped;
	memset(&overlapped, 0, sizeof(overlapped));
	overlapped.hEvent = m_hPipeReadWriteEvent;

	if (WriteFile(m_hPipe, pBuffer, uNumberOfBytesToWrite, puNumberOfBytesWritten, &overlapped))
	{
		// Operation completed synchronously anyways
		return true;
	}

	if (GetLastError() != ERROR_IO_PENDING)
	{
		return false;
	}

	// Wait for the operation to complete
	DWORD uBytesTransferred;
	DWORD *puBytesTransferred = (puNumberOfBytesWritten != nullptr) ? puNumberOfBytesWritten : &uBytesTransferred;
	return (GetOverlappedResult(m_hPipe, &overlapped, puBytesTransferred, TRUE) != 0);
}

/**
 * @return True if any button in aButtonState is down, false otherwise.
 */
inline Bool AnyButtonsPressed(const FixedArray<Bool, 5u>& aButtonState)
{
	for (UInt32 i = 0u; i < aButtonState.GetSize(); ++i)
	{
		if (aButtonState[i])
		{
			return true;
		}
	}

	return false;
}

/**
 * @return The Seoul::InputButton that corresponds to the button state in
 * a 5 element state array at index uIndex.
 */
inline InputButton FromArrayIndex(UInt32 uIndex)
{
	switch (uIndex)
	{
	case 0u: return InputButton::MouseLeftButton;
	case 1u: return InputButton::MouseRightButton;
	case 2u: return InputButton::MouseMiddleButton;
	case 3u: return InputButton::MouseButton4;
	case 4u: return InputButton::MouseButton5;
	default:
		return InputButton::MouseLeftButton;
	};
}

/**
 * Utility, maps a WM_ mouse button identifier to an index, to allow
 * state to be tracked in a 5 element state array.
 */
inline UInt32 ToArrayIndex(UInt wmButtonCode, WPARAM wParam)
{
	UInt32 const xParam = GET_XBUTTON_WPARAM(wParam);

	switch (wmButtonCode)
	{
	case WM_LBUTTONDOWN: // fall-through
	case WM_LBUTTONUP:
		return 0u;

	case WM_RBUTTONDOWN: // fall-through
	case WM_RBUTTONUP:
		return 1u;

	case WM_MBUTTONDOWN: // fall-through
	case WM_MBUTTONUP:
		return 2u;

	case WM_XBUTTONDOWN: // fall-through
	case WM_XBUTTONUP:
		if (XBUTTON1 == xParam)
		{
			return 3u;
		}
		else
		{
			return 4u;
		}

	default:
		return 0u;
	};
}

/**
 * @return The Seoul::InputButton that corresponds to a windows button code
 * and modifier in wmButtonCode and wParam.
 */
inline InputButton ToInputButton(UInt wmButtonCode, WPARAM wParam)
{
	UInt32 const xParam = GET_XBUTTON_WPARAM(wParam);

	switch (wmButtonCode)
	{
	case WM_LBUTTONDOWN: // fall-through
	case WM_LBUTTONUP:
		return InputButton::MouseLeftButton;

	case WM_RBUTTONDOWN: // fall-through
	case WM_RBUTTONUP:
		return InputButton::MouseRightButton;

	case WM_MBUTTONDOWN: // fall-through
	case WM_MBUTTONUP:
		return InputButton::MouseMiddleButton;

	case WM_XBUTTONDOWN: // fall-through
	case WM_XBUTTONUP:
		if (XBUTTON1 == xParam)
		{
			return InputButton::MouseButton4;
		}
		else
		{
			return InputButton::MouseButton5;
		}

	default:
		return InputButton::MouseLeftButton;
	};
}

/**
 * Release all buttons in raMouseButtonState and if
 * necessary, call ::ReleaseCapture().
 */
inline void ReleaseMouseButtons(FixedArray<Bool, 5>& raMouseButtonState)
{
	Bool const bWasAnyButtonPressed = AnyButtonsPressed(raMouseButtonState);
	raMouseButtonState.Fill(false);
	if (bWasAnyButtonPressed)
	{
		SEOUL_VERIFY(FALSE != ::ReleaseCapture());
	}
}

void PCEngine::InternalStartTextEditing(ITextEditable* pTextEditable, const String& /*sText*/, const String& /*sDescription*/, const StringConstraints& /*constraints*/, Bool bAllowNonLatinKeyboard)
{
	pTextEditable->TextEditableEnableCursor();
}

// Enable drag file support in non-ship builds.
#if !SEOUL_SHIP
namespace
{

/**
 * Main thread hook for dispatch a filename dragged onto
 * the current rendering window to the Events::Manager.
 */
void DispatchDropFile(const String& sFilename)
{
	SEOUL_ASSERT(IsMainThread());

	// Environment confirmation.
	if (Events::Manager::Get())
	{
		// Dispatch.
		Events::Manager::Get()->TriggerEvent<const String&>(EngineDropFileEventId, sFilename);
	}
}

} // namespace anonymous
#endif // /!SEOUL_SHIP

/**
 * The Seoul message procedure.  Returns 0 if everything is normal.
 *
 * @param[in] hwnd   Window receiving the message
 * @param[in] msg    Window message identifier
 * @param[in] wParam First message-dependent parameter
 * @param[in] lParam Second message-dependent parameter
*/
LRESULT CALLBACK PCEngine::MessageProcedure(HWND hwnd, UInt msg, WPARAM wParam, LPARAM lParam)
{
	SEOUL_ASSERT(IsRenderThread());

	// Local array to track mouse button events, to allow us to do some immediate handling
	// based on press or not.
	static FixedArray<Bool, 5> s_aMouseButtonState(false);

	CheckedPtr<PCEngine> pPCEngine = PCEngine::Get();

	// Get the first mouse device from InputManager.
	auto pMouseDevice = InputManager::Get()->FindFirstMouseDevice();

	// Handling.
	if (pPCEngine && pPCEngine->IsInitialized())
	{
		RECT clientRect = { 0, 0, 0, 0 };
		Bool bKeyPressed = false;
		switch (msg)
		{
			// Enable drag file support in non-ship builds.
#if !SEOUL_SHIP
		case WM_DROPFILES:
		{
			// Query the number of files in the event.
			auto const hDrop = (HDROP)wParam;
			auto const uCount = (UInt32)::DragQueryFileW(hDrop, (UINT)-1 /* special value for querying count */, nullptr, 0);

			// Iterate - buffer used for caching the filename.
			Vector<WCHAR> vBuffer;
			for (UInt32 i = 0u; i < uCount; ++i)
			{
				// NOTE: Bit of a mismatched API - size returned is the characters
				// excluding the null terminator, but the size we pass *into* DragQueryFileW
				// is the size *including* the null terminator.
				auto const uLength = ::DragQueryFileW(hDrop, i, nullptr, 0u) + 1; /* return the length */
				vBuffer.Resize(Max(vBuffer.GetSize(), uLength)); // +1 for null terminator.
				(void)::DragQueryFileW(hDrop, i, vBuffer.Data(), uLength);

				// Dispatch on the main thread - asynchronous.
				Jobs::AsyncFunction(
					GetMainThreadId(),
					&DispatchDropFile,
					WCharTToUTF8(vBuffer.Data()));
			}

			// Done - complete the drop event.
			::DragFinish(hDrop);
			return 0;
		}
#endif // /!SEOUL_SHIP

		case WM_DWMSENDICONICLIVEPREVIEWBITMAP:
		{
			pPCEngine->m_pD3DCommonDevice->PCEngineFriend_OnLivePreviewBitmap();
			return 0;
		}

		case WM_DWMSENDICONICTHUMBNAIL:
		{
			pPCEngine->m_pD3DCommonDevice->PCEngineFriend_OnLiveThumbnail(HIWORD(lParam), LOWORD(lParam));
			return 0;
		}

		case WM_ACTIVATE:
			if (wParam == WA_ACTIVE || wParam == WA_CLICKACTIVE)
			{
				pPCEngine->m_pD3DCommonDevice->PCEngineFriend_SetActive(true);
				InputManager::Get()->TriggerRescan();
				return 0;
			}
			else if (wParam == WA_INACTIVE)
			{
				// If events are being ignored, just return 0 without reporting the inactive.
				if (pPCEngine->m_pD3DCommonDevice->PCEngineFriend_ShouldIgnoreActivateEvents())
				{
					return 0;
				}

				// Release all mouse buttons and indicate that input focus has been lost.
				ReleaseMouseButtons(s_aMouseButtonState);
				InputManager::Get()->OnLostFocus();
				pPCEngine->m_pD3DCommonDevice->PCEngineFriend_SetActive(false);
				return 0;
			}
			break;
		case WM_ACTIVATEAPP:
			if (wParam == TRUE)
			{
				pPCEngine->m_pD3DCommonDevice->PCEngineFriend_SetActive(true);
				InputManager::Get()->TriggerRescan();
				return 0;
			}
			else if (wParam == FALSE)
			{
				// If events are being ignored, just return 0 without reporting the inactive.
				if (pPCEngine->m_pD3DCommonDevice->PCEngineFriend_ShouldIgnoreActivateEvents())
				{
					return 0;
				}

				// Release all mouse buttons and indicate that input focus has been lost.
				ReleaseMouseButtons(s_aMouseButtonState);
				InputManager::Get()->OnLostFocus();
				pPCEngine->m_pD3DCommonDevice->PCEngineFriend_SetActive(false);
				return 0;
			}
			break;

		case WM_SIZE:
			(void)::GetClientRect(pPCEngine->m_pD3DCommonDevice->PCEngineFriend_GetMainWindow(), &clientRect);

			if (wParam == SIZE_MINIMIZED || (clientRect.top == 0 && clientRect.bottom == 0))
			{
				pPCEngine->m_pD3DCommonDevice->PCEngineFriend_Minimized(true);
			}
			else
			{
				if (wParam == SIZE_RESTORED)
				{
					if (pPCEngine->m_pD3DCommonDevice->IsMinimized())
					{
						pPCEngine->m_pD3DCommonDevice->PCEngineFriend_Minimized(false);
					}
					// Ignore size restored events when leaving fullscreen, as they can be erroneous.
					else if (!pPCEngine->m_pD3DCommonDevice->PCEngineFriend_IsLeavingFullscren())
					{
						pPCEngine->m_pD3DCommonDevice->PCEngineFriend_CaptureAndResizeClientViewport();
					}
				}
				else if (wParam == SIZE_MAXIMIZED)
				{
					pPCEngine->m_pD3DCommonDevice->PCEngineFriend_Minimized(false);

					if (pPCEngine->m_pD3DCommonDevice->IsWindowed())
					{
						// If we set to enter fullscreen on maximize, do so.
						if (pPCEngine->m_pD3DCommonDevice->PCEngineFriend_GetGraphicsParameters().m_bFullscreenOnMaximize)
						{
							pPCEngine->m_pD3DCommonDevice->ToggleFullscreenMode();
						}
						// Otherwise, just tell the RenderDevice that it needs
						// to resize the viewport.
						else
						{
							pPCEngine->m_pD3DCommonDevice->PCEngineFriend_CaptureAndResizeClientViewport();
						}
					}
				}
			}

			return 0;

		case WM_SETCURSOR:
			// WM_SETCURSOR is sent to notify us that we should change the cursor.
			// Set the cursor based on the return value of this delegate

			// Only do this if the cursor is inside the client area.
			if (LOWORD(lParam) == HTCLIENT)
			{
				auto const hcursor = pPCEngine->m_Settings.m_RenderDeviceSettings.m_aMouseCursors[(UInt32)pPCEngine->GetMouseCursor()];
				if (nullptr != hcursor)
				{
					SetCursor(hcursor);
					return 0;
				}
			}
			break;

		case WM_ENTERMENULOOP:
			pPCEngine->OnModalWindowsLoopEntered();
			return 0;

		case WM_EXITMENULOOP:
			pPCEngine->OnModalWindowsLoopExited();
			return 0;

		case WM_ENTERSIZEMOVE:
			pPCEngine->OnModalWindowsLoopEntered();
			return 0;

		case WM_EXITSIZEMOVE:
			pPCEngine->OnModalWindowsLoopExited();
			pPCEngine->m_pD3DCommonDevice->PCEngineFriend_CaptureAndResizeClientViewport();
			return 0;

		case WM_CLOSE:
			// WM_CLOSE is sent when the user presses the 'X' button in the caption bar menu.
			pPCEngine->m_pD3DCommonDevice->PCEngineFriend_DestroyWindow();
			return 0;

		case WM_DESTROY:
			// WM_DESTROY is sent when the window is being destroyed.
			PostQuitMessage(0);
			return 0;

		case WM_SYSKEYDOWN: // fall-through
							// Although MSDN says "It also occurs when no window currently has the
							// keyboard focus; in this case, the WM_SYSKEYDOWN message is sent to
							// the active window. The window that receives the message can distinguish
							// between these two contexts by checking the context code in the lParam parameter.",
							// it fails to note that the context code will be 0 when the F10 key is pressed.
			if (!(lParam & 0x20000000) && (VK_F10 != wParam))
			{
				// This message is just telling us that no window has the current focus. Don't do anything
				return 0;
			}
			// fall through
		case WM_KEYDOWN:
			bKeyPressed = true;
			// fall through
		case WM_SYSKEYUP: // fall through
		case WM_KEYUP:
			for (UInt i = 0; i < InputManager::Get()->GetNumDevices(); i++)
			{
				InputDevice* pDevice = InputManager::Get()->GetDevice(i);
				if (pDevice->GetDeviceType() == InputDevice::Keyboard)
				{
					pDevice->QueueKeyEvent((UInt)wParam, bKeyPressed);

					// If it is one of the special keys, we need to figure out whether the left or right one is being pressed
					switch (wParam)
					{
					case VK_SHIFT:
						pDevice->QueueKeyEvent(VK_LSHIFT + ((lParam & 0x1000000) >> 24), bKeyPressed);
						break;
					case VK_CONTROL:
						pDevice->QueueKeyEvent(VK_LCONTROL + ((lParam & 0x1000000) >> 24), bKeyPressed);
						break;
					case VK_MENU:
						pDevice->QueueKeyEvent(VK_LMENU + ((lParam & 0x1000000) >> 24), bKeyPressed);
						break;
					}
				}
			}

			// Capture key events to prevent warning sounds when certain key
			// combinations (i.e. ALT+ENTER to toggle full screen).
			return 0;

		case WM_UNICHAR:
			// Indicate that we support Unicode char messages
			if (wParam == UNICODE_NOCHAR)
			{
				return TRUE;
			}
			// Fall-through
		case WM_CHAR: // Fall-through
		case WM_SYSCHAR:
			if (pPCEngine->m_pTextEditable)
			{
				// Translation for simplification, since in all other cases,
				// interpretation of newlines requires cr+lf, but keyboard
				// entry will just translate ENTER into cr.
				UniChar ch = (UniChar)wParam;
				if ('\r' == ch)
				{
					ch = '\n';
				}

				Jobs::AsyncFunction(
					GetMainThreadId(),
					&InternalCallTextEditableApplyChar,
					ch);
			}

			return 0;

		case WM_DISPLAYCHANGE:
			return 0;

		case WM_DEVICECHANGE:
			// Next poll, do a scan.
			InputManager::Get()->TriggerRescan();
			break;

			// Begin code from DXUT - various handling typical for game applications:
		case WM_GETMINMAXINFO:
			// Limit the minimum window size.
			((MINMAXINFO*)lParam)->ptMinTrackSize.x = kMinimumResolutionWidth;
			((MINMAXINFO*)lParam)->ptMinTrackSize.y = kMinimumResolutionHeight;
			// fall-through - let the default handler get this message as well.
			break;

		case WM_ERASEBKGND:
			return 1;

		case WM_NCHITTEST:
			// Prevent selection of the menu when running in full screen.
			if (!pPCEngine->m_pD3DCommonDevice->IsWindowed())
			{
				return HTCLIENT;
			}
			break;

		case WM_SYSCOMMAND:
			switch (wParam & 0xFFF0)
			{
				// Disallow move, resize, maximize, and displaying the menu in full screen, as well
				// as monitor power and screen save triggers.
			case SC_KEYMENU: // fall-through
			case SC_MAXIMIZE: // fall-through
			case SC_MONITORPOWER: // fall-through
			case SC_MOVE: // fall-through
			case SC_SCREENSAVE: // fall-through
			case SC_SIZE:
				if (!pPCEngine->m_pD3DCommonDevice->IsWindowed())
				{
					return 0;
				}
				break;
			};
			break;
			// /End code from DXUT - various handling typical for game applications:
		case WM_SETFOCUS:
			// Next poll, do a scan.
			InputManager::Get()->TriggerRescan();
			return 0;

		case WM_KILLFOCUS:
			// If events are being ignored, just return 0 without reporting the inactive.
			if (pPCEngine->m_pD3DCommonDevice->PCEngineFriend_ShouldIgnoreActivateEvents())
			{
				return 0;
			}

			// Release all mouse buttons and indicate that input focus has been lost.
			ReleaseMouseButtons(s_aMouseButtonState);
			InputManager::Get()->OnLostFocus();
			return 0;

			// Mouse handling
		case WM_CAPTURECHANGED:
			// On a capture changed event, we've lost capture - if any buttons are
			// marked as down, queue a mouse up event.
			for (UInt32 i = 0u; i < s_aMouseButtonState.GetSize(); ++i)
			{
				if (s_aMouseButtonState[i])
				{
					s_aMouseButtonState[i] = false;
					if (nullptr != pMouseDevice)
					{
						pMouseDevice->QueueMouseButtonEvent(FromArrayIndex(i), false);
					}
				}
			}

			return 0;

		case WM_MOUSEMOVE:
			// Pass the mouse move event to the mouse device.
			if (nullptr != pMouseDevice)
			{
				Point2DInt const newPosition(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
				if (newPosition != pPCEngine->m_RenderThreadLastMousePosition)
				{
					pPCEngine->m_RenderThreadLastMousePosition = newPosition;
					pMouseDevice->QueueMouseMoveEvent(newPosition);
				}
			}
			return 0;

		case WM_MOUSEWHEEL:
			// Pass the mouse wheel event to the mouse device.
			if (nullptr != pMouseDevice)
			{
				// Basing this off:
				// https://docs.microsoft.com/en-us/dotnet/api/system.windows.forms.mouseeventargs.delta?view=netframework-4.8#remarks
				//
				// "Currently, a value of 120 is the standard for one detent.
				// If higher resolution mice are introduced, the definition of
				// WHEEL_DELTA might become smaller. Most applications should
				// check for a positive or negative value rather than an aggregate total."
				//
				// Negative becomes -127 (the standard min so values fit into an int8)
				// and positive becomes 127.
				auto iDelta = (Int)GET_WHEEL_DELTA_WPARAM(wParam);
				iDelta = (iDelta < 0 ? MouseDevice::kMinWheelDelta : (iDelta > 0 ? MouseDevice::kMaxWheelDelta : 0));
				pMouseDevice->QueueMouseWheelEvent(iDelta);
			}
			return 0;

		case WM_LBUTTONDOWN: // fall-through
		case WM_RBUTTONDOWN: // fall-through
		case WM_MBUTTONDOWN: // fall-through
		case WM_XBUTTONDOWN:
			// Handle the down event (potentially set the input capture),
			// then pass the input event to the mouse device.
		{
			if (!AnyButtonsPressed(s_aMouseButtonState))
			{
				(void)::SetCapture(pPCEngine->m_pD3DCommonDevice->PCEngineFriend_GetMainWindow());
			}
			s_aMouseButtonState[ToArrayIndex(msg, wParam)] = true;

			if (nullptr != pMouseDevice)
			{
				pMouseDevice->QueueMouseButtonEvent(ToInputButton(msg, wParam), true);
			}
		}
		return 0;

		case WM_LBUTTONUP: // fall-through
		case WM_RBUTTONUP: // fall-through
		case WM_MBUTTONUP: // fall-through
		case WM_XBUTTONUP:
			// Handle the up event (potentially release the input capture),
			// then pass the input event to the mouse device.
		{
			Bool const bWasAnyButtonPressed = AnyButtonsPressed(s_aMouseButtonState);
			s_aMouseButtonState[ToArrayIndex(msg, wParam)] = false;
			if (bWasAnyButtonPressed && !AnyButtonsPressed(s_aMouseButtonState))
			{
				SEOUL_VERIFY(FALSE != ::ReleaseCapture());
			}

			if (nullptr != pMouseDevice)
			{
				pMouseDevice->QueueMouseButtonEvent(ToInputButton(msg, wParam), false);
			}
		}
		return 0;
		// /Mouse handling

		// Capture various events that we don't use, but we don't want
		// the DefProc to handle (i.e. WM_DEADCHAR, etc. trigger a system
		// warning beep as the default handling). We let these functions
		// fall through if in the system draw loop.
		case WM_DEADCHAR: // fall-through
		case WM_SYSDEADCHAR: // fall-through
		case WM_MOUSEACTIVATE: // fall-through
		case WM_MOUSEHOVER: // fall-through
		case WM_MOUSELEAVE: // fall-through
		case WM_LBUTTONDBLCLK: // fall-through
		case WM_RBUTTONDBLCLK: // fall-through
		case WM_MBUTTONDBLCLK: // fall-through
		case WM_XBUTTONDBLCLK:
			if (!pPCEngine->IsInModalWindowsLoop())
			{
				return 0;
			}
		};
	}

	return ::DefWindowProc(hwnd, msg, wParam, lParam);
}

/**
 * Handle per-frame update tasks that must run on the render thread.
 */
Bool PCEngine::RenderThreadPumpMessageQueue()
{
	SEOUL_ASSERT(IsRenderThread());

	Bool bReturn = InternalRenderThreadPumpMessageQueue();

	m_bHasFocus = (GetForegroundWindow() == m_pD3DCommonDevice->PCEngineFriend_GetMainWindow());

	InternalUpdateCursor();

	m_bQuit = (m_bQuit || !bReturn);

	return !m_bQuit;
}

Bool PCEngine::InternalRenderThreadPumpMessageQueue()
{
	SEOUL_ASSERT(IsRenderThread());

	Bool bReturn = true;

	MSG msg;
	memset(&msg, 0, sizeof(msg));
	while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
	{
		// Break out of the main loop when we get a WM_QUIT message
		if (WM_QUIT == msg.message)
		{
			bReturn = false;
			m_bQuit = true;
		}

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// Update the active flag.
	m_bActive = (m_pD3DCommonDevice->IsActive() && !m_pD3DCommonDevice->IsMinimized());

	return bReturn && !m_bQuit;
}

/**
 * We allow two copies of the current process if one is
 * a distro and the other is not.
 */
static inline Bool GetDistro(const WString& sFileName, Bool& rbDistro)
{
	auto const uInfoSize = GetFileVersionInfoSizeW(sFileName, nullptr);
	if (0u == uInfoSize)
	{
		return false;
	}

	StackOrHeapArray<Byte, 128u, MemoryBudgets::TBD> aData(uInfoSize);
	if (FALSE == ::GetFileVersionInfoW(sFileName, 0u, uInfoSize, aData.Data()))
	{
		return false;
	}

	PVOID pDistro = nullptr;
	UINT uDistro = 0u;
	if (FALSE == ::VerQueryValueW(aData.Data(), L"\\StringFileInfo\\040904E4\\DistroBuild", &pDistro, &uDistro))
	{
		return false;
	}

	String const sDistro((Byte const*)pDistro, (UInt32)(Max(uDistro, 1u) - 1u));
	if (sDistro == "1")
	{
		rbDistro = true;
		return true;
	}
	else if (sDistro == "0")
	{
		rbDistro = false;
		return true;
	}
	else
	{
		return false;
	}
}

/**
 * If running, prompts and attempts to kill existing instances of the game.
 */
void PCEngine::InternalCheckExistingGameProcesses()
{
	// Maximum amount of time that we will wait for a process to terminate, in milliseconds. Currently 5 seconds.
	static const DWORD kuWaitTimeInMilliseconds = 5000u;

	// Cache the current process id, and create a snapshot of currently active processes.
	DWORD const uThisProcessId = ::GetCurrentProcessId();
	HANDLE hSnapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPALL, 0);

	PROCESSENTRY32W entry;
	memset(&entry, 0, sizeof(entry));
	entry.dwSize = sizeof(entry);

	// First walk the process list to see if an existing game process is alive.
	Bool bHasCollision = false;
	WString myPath;
	Bool bDistro = false;
	if (!InternalGetProcessAbsolutePath(nullptr, myPath))
	{
		// Failure to get my path, assume collision.
		bHasCollision = true;
	}
	else
	{
		if (!GetDistro(myPath, bDistro))
		{
			// Assume collision if we couldn't read the depot flag.
			bHasCollision = true;
		}
		else
		{
			for (BOOL b = ::Process32FirstW(hSnapshot, &entry); FALSE != b; b = ::Process32NextW(hSnapshot, &entry))
			{
				// Don't consider the current process.
				if (uThisProcessId == entry.th32ProcessID)
				{
					continue;
				}

				// Check that the process's base filename is contained
				// within the path of the current process (indicating
				// that they are the same process - e.g. AppPC.exe)
				if (nullptr == wcsstr(myPath, entry.szExeFile))
				{
					continue;
				}

				// If another instance of the current process, based on name, report that we have a process to kill
				// and break.

				// Match version info - we allow certain combinations.
				auto hProcess = ::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, entry.th32ProcessID);
				if (INVALID_HANDLE_VALUE == hProcess || nullptr == hProcess)
				{
					// Failed to open the process, skip.
					continue;
				}

				// Get the path - if this fails, assume a conflict and exit immediately.
				WString otherPath;
				auto const bOk = InternalGetProcessAbsolutePath(hProcess, otherPath);
				SEOUL_VERIFY(FALSE != ::CloseHandle(hProcess));
				hProcess = INVALID_HANDLE_VALUE;
				if (!bOk)
				{
					bHasCollision = true;
					break;
				}

				// Check if both processes are distro or both processes are not distro.
				// If one or the other, we're ok.
				Bool bOtherDistro = false;
				if (!GetDistro(otherPath, bOtherDistro))
				{
					bHasCollision = true;
					break;
				}

				// Both are distro or both are not distro, collision.
				if ((bDistro && bOtherDistro) ||
					(!bDistro && !bOtherDistro))
				{
					bHasCollision = true;
					break;
				}
			}
		}
	}

	// If we have a process to kill.
	if (bHasCollision)
	{
		static const HString ksErrorGameAlreadyRunningMessage("game_already_running_message");
		static const HString ksErrorGameAlreadyRunningTitle("game_already_running_title");

		String sMessage = LocManager::Get()->Localize(ksErrorGameAlreadyRunningMessage);
		String sTitle = LocManager::Get()->Localize(ksErrorGameAlreadyRunningTitle);

		// Ask the user if (s)he wants to kill the existing game instance.
		Int iResult = ::MessageBoxW(
			nullptr,
			sMessage.WStr(),
			sTitle.WStr(),
			MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON1 | MB_SETFOREGROUND);

		// If the chose yes, go for it.
		if (IDYES == iResult)
		{
			memset(&entry, 0, sizeof(entry));
			entry.dwSize = sizeof(entry);

			for (BOOL b = ::Process32FirstW(hSnapshot, &entry); FALSE != b; b = ::Process32NextW(hSnapshot, &entry))
			{
				if (uThisProcessId != entry.th32ProcessID)
				{
					// For each game process that is not the current one, kill it with SafeTerminateProcess.
					if (nullptr != wcsstr(myPath, entry.szExeFile))
					{
						HANDLE hProcess = ::OpenProcess(PROCESS_TERMINATE, FALSE, entry.th32ProcessID);
						if (nullptr != hProcess && INVALID_HANDLE_VALUE != hProcess)
						{
							(void)SafeTerminateProcess(hProcess, 1, kuWaitTimeInMilliseconds);
							SEOUL_VERIFY(FALSE != ::CloseHandle(hProcess));
						}
					}
				}
			}
		}
		// Otherwise, we need to exit.
		else
		{
			m_bQuit = true;
		}
	}

	// When all done, close the snaphost of processes.
	SEOUL_VERIFY(FALSE != ::CloseHandle(hSnapshot));
}

/**
 * Parse the application JSON and build version
 * to create strings for the application name and window name.
 */
void PCEngine::InternalInitializeAppNameAndVersionString()
{
	m_sAppName = "Seoul";
	m_sCompanyName = "Unknown Company";
	Bool const bSetLocalizedAppNameAndVersion = m_Settings.m_RenderDeviceSettings.m_sLocalizedAppNameAndVersion.IsEmpty();

	// Override the AppName if it is set in the .json file
	SEOUL_ASSERT(GamePaths::Get() && GamePaths::Get()->IsInitialized());
	SharedPtr<DataStore> pDataStore(SettingsManager::Get()->WaitForSettings(
		GamePaths::Get()->GetApplicationJsonFilePath()));
	if (pDataStore.IsValid())
	{
		DataStoreTableUtil const applicationSection(*pDataStore, ksApplication);
		(void)applicationSection.GetValue(ksApplicationName, m_sAppName);
		(void)applicationSection.GetValue(ksCompanyName, m_sCompanyName);
		(void)applicationSection.GetValue(ksMuteAudioWhenInactive, m_bMuteAudioWhenInactive);
		(void)applicationSection.GetValue(ksEnablePCControllerSupport, m_bEnableControllerSupport);

		// Only compute the localized string+version if the settings value passed in is empty.
		if (bSetLocalizedAppNameAndVersion)
		{
			HString locToken;
			if (applicationSection.GetValue(ksLocalizedApplicationToken, locToken))
			{
				m_Settings.m_RenderDeviceSettings.m_sLocalizedAppNameAndVersion = LocManager::Get()->Localize(locToken);
			}
			else
			{
				m_Settings.m_RenderDeviceSettings.m_sLocalizedAppNameAndVersion = m_sAppName;
			}
		}
	}

	// Only compute the localized string+version if the settings value passed in is empty.
	if (bSetLocalizedAppNameAndVersion)
	{
		m_Settings.m_RenderDeviceSettings.m_sLocalizedAppNameAndVersion.Append(':');
		m_Settings.m_RenderDeviceSettings.m_sLocalizedAppNameAndVersion.Append(' ');
		m_Settings.m_RenderDeviceSettings.m_sLocalizedAppNameAndVersion.Append(SEOUL_BUILD_CONFIG_STR);
		m_Settings.m_RenderDeviceSettings.m_sLocalizedAppNameAndVersion.Append(" v");
		m_Settings.m_RenderDeviceSettings.m_sLocalizedAppNameAndVersion.Append(BUILD_VERSION_STR);
		m_Settings.m_RenderDeviceSettings.m_sLocalizedAppNameAndVersion.Append(".");
		m_Settings.m_RenderDeviceSettings.m_sLocalizedAppNameAndVersion.Append(g_sBuildChangelistStr);
	}
}

/** Called to load the preferred render backend from application INI. */
void PCEngine::InternalInitializePreferredRenderBackend()
{
	SharedPtr<DataStore> pDataStore(SettingsManager::Get()->WaitForSettings(
		GamePaths::Get()->GetApplicationJsonFilePath()));
	if (pDataStore.IsValid())
	{
		DataStoreTableUtil const applicationSection(*pDataStore, ksApplication);
		(void)applicationSection.GetValue(ksPCRenderBackend, m_Settings.m_RenderDeviceSettings.m_sPreferredBackend);
	}
}

/**
 * Installs the current executable as a URL protocol handler for the given
 * URL protocol for the current user
 */
void PCEngine::InstallURLHandler(const String& sProtocol, const String& sDescription)
{
	String sOpenCommand = String::Printf("\"%s\" -Message=\"%%1\"", GetExecutableName().CStr());
	String sKeyPath = "Software\\Classes\\" + sProtocol;
	if (!WriteRegistryValue(HKEY_CURRENT_USER, sKeyPath, "", "URL:" + sDescription) ||
		!WriteRegistryValue(HKEY_CURRENT_USER, sKeyPath, "URL Protocol", "") ||
		!WriteRegistryValue(HKEY_CURRENT_USER, sKeyPath + "\\DefaultIcon", "", GetExecutableName() + ",1") ||
		!WriteRegistryValue(HKEY_CURRENT_USER, sKeyPath + "\\shell\\open\\command", "", sOpenCommand))
	{
		SEOUL_WARN("Failed to install URL handler for %s protocol", sProtocol.CStr());
	}
}

/**
 * @return True if a key is in the Windows registry, false otherwise.
 *
 * @param[in] hKey A handle to an open registry key, or one of the predefined
 *            root hive keys such as HKEY_CURRENT_USER.
 * @param[in] sSubKey Subkey of the given key to test.
 * @parma[out] rsData The output value stored in the registry key. Unchanged when this method returns false.
 */
Bool PCEngine::GetRegistryValue(HKEY hKey, const String& sSubKey, String& rsData) const
{
	Bool bReturn = false;

	HKEY hOpenKey = nullptr;
	if (ERROR_SUCCESS == ::RegOpenKeyW(hKey, sSubKey.WStr(), &hOpenKey))
	{
		LONG nValueSize = 0;
		bReturn = (ERROR_SUCCESS == ::RegQueryValueW(hOpenKey, nullptr, nullptr, &nValueSize) && nValueSize > 0);
		if (bReturn)
		{
			Vector<WCHAR> vData((UInt32)nValueSize);
			bReturn = (ERROR_SUCCESS == ::RegQueryValueW(hOpenKey, nullptr, vData.Get(0u), &nValueSize) && (UInt32)nValueSize == vData.GetSize());
			if (bReturn)
			{
				vData[vData.GetSize() - 1] = (WCHAR)'\0';
				rsData = WCharTToUTF8(vData.Get(0u));
			}
		}

		SEOUL_VERIFY(ERROR_SUCCESS == ::RegCloseKey(hOpenKey));
		hOpenKey = nullptr;
	}

	return bReturn;
}

/**
 * Writes a value to the Windows registry.  The value is stored as a string
 * (REG_SZ).
 *
 * @param[in] hKey A handle to an open registry key, or one of the predefined
 *            root hive keys such as HKEY_CURRENT_USER.
 * @param[in] sSubKey Subkey of the given key to write to
 * @param[in] sValueName Name of the registry value within the given key to
 *            write; pass the empty string to write the default value
 * @param[in] sData Data to write to the given value
 *
 * @return True if the write succeeded, or false otherwise
 */
Bool PCEngine::WriteRegistryValue(HKEY hKey, const String& sSubKey, const String& sValueName, const String& sData)
{
	// First create or open the subkey
	HKEY hSubKey;
	if (RegCreateKeyW(hKey, sSubKey.WStr(), &hSubKey) != ERROR_SUCCESS)
	{
		return false;
	}

	// Set the value in the key
	WString wsData = sData.WStr();
	bool bSuccess = (RegSetValueExW(hSubKey,
		sValueName.WStr(),
		0,
		REG_SZ,
		(const BYTE *)(const wchar_t *)wsData,
		(wsData.GetLengthInChars() + 1) * sizeof(wchar_t))
		== ERROR_SUCCESS);
	RegCloseKey(hKey);

	return bSuccess;
}

String PCEngine::GetRegistrySubkeyAppRoot() const
{
	return String::Printf("Software\\%s\\%s\\", m_sCompanyName.CStr(), m_sAppName.CStr());
}

// Allow subclasses to override where the platform UUID is stored.
String PCEngine::GetPlatformUUIDRegistrySubkey() const
{
	String const sSubKey(String::Printf("%sUUID", GetRegistrySubkeyAppRoot().CStr()));
	return sSubKey;
}

void PCEngine::SetIPCPipeName(const String& sPipeName)
{
	m_sPipeName = sPipeName;
}

void PCEngine::InternalInitializeIPC()
{
	// Make an event for when the pipe receives connection or data
	m_hPipeEvent = CreateEvent(nullptr, TRUE, TRUE, nullptr);
	m_hPipeReadWriteEvent = CreateEvent(nullptr, TRUE, TRUE, nullptr);

	SEOUL_ASSERT(m_hPipeEvent != nullptr);
	SEOUL_ASSERT(m_hPipeReadWriteEvent != nullptr);

	memset(&m_Overlapped, 0, sizeof(m_Overlapped));

	// Make a pipe in overlapped mode
	m_hPipe = CreateNamedPipe(m_sPipeName.CStr(), PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
								   PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, 1, 4096, 4096, 5000, nullptr);

    if (m_hPipe == INVALID_HANDLE_VALUE)
    {
		DWORD error = GetLastError();
		SEOUL_LOG("Pipe creation failed. Error: %u", error);

		// We use the pipe to determine if another instance of the game is
		// already running (we should probably use a regular Mutex instead,
		// but since we already have the pipe...).  So if the pipe failed to
		// be created because all of its instances are busy, just exit.
		if (error == ERROR_PIPE_BUSY)
		{
#if !SEOUL_SHIP
			return; // This allows devs to launch multiple copies on the same machine for testing
#else
			Engine::InternalPostRenderDeviceShutdown();
			exit(0);
#endif // !SEOUL_SHIP
		}
		else
		{
			return;
		}
	}

	if(!ConnectNamedPipe(m_hPipe, &m_Overlapped) && GetLastError() != ERROR_IO_PENDING)
	{
		SEOUL_LOG("ConnectNamedPipe failed, %u", GetLastError());
	}
}

void PCEngine::InternalShutdownIPC()
{
	if (m_hPipeReadWriteEvent != nullptr)
	{
		SEOUL_VERIFY(CloseHandle(m_hPipeReadWriteEvent));
	}

	if (m_hPipeEvent != nullptr)
	{
		SEOUL_VERIFY(CloseHandle(m_hPipeEvent));
	}

	if (m_hPipe != nullptr && m_hPipe != INVALID_HANDLE_VALUE)
	{
		SEOUL_VERIFY(CloseHandle(m_hPipe));
	}

	m_hPipeReadWriteEvent = nullptr;
	m_hPipeEvent = nullptr;
	m_hPipe = nullptr;
}

void PCEngine::SetIPCMessageCallback(ReceiveIPCMessageDelegate messageDelegate)
{
	m_ReceiveIPCMessageDelegate = messageDelegate;
}

/**
 * Perform PC pre-initialize tasks - parse command-line
 * arguments and setup names for the log and window.
 */
void PCEngine::InternalPCPreInitialize(String& rsLogName)
{
	// Argument 0 is the executable path, but we use GetModuleFileName in
	// case somebody decided to CreateProcess() us with something else in
	// argv[0].
	{
		Vector<WCHAR, MemoryBudgets::Io> v(MAX_PATH);

		// Size == result size means we didn't succeed, so need to increase the buffer size.
		auto uResult = ::GetModuleFileNameW(nullptr, v.Data(), v.GetSize());
		while (v.GetSize() <= uResult)
		{
			v.Resize(v.GetSize() * 2u);
			uResult = ::GetModuleFileNameW(nullptr, v.Data(), v.GetSize());
		}

		// 0 means failure, so yell about that.
		if (0u == uResult)
		{
			SEOUL_WARN("GetModuleFileNameW: error 0x%08x\n", GetLastError());
		}
		else
		{
			// Otherwise, commit the executable name.
			SetExecutableName(WCharTToUTF8(v.Data()));
		}
	}

	rsLogName.Clear(); // TODO:
}

/**
 * Create Win32 input handling using DirectInput.
 */
void PCEngine::InternalInitializeDirectInput()
{
	PCInputDeviceEnumerator inputDeviceEnumerator;
	InputManager::Get()->EnumerateInputDevices(&inputDeviceEnumerator);

	// Based on the application JSON setting for EnablePCControllerSupport
	if (m_bEnableControllerSupport)
	{
		PCXInputDeviceEnumerator xb360Enumerator;
		InputManager::Get()->EnumerateInputDevices(&xb360Enumerator);
	}

	// Set the dead-zones for the controllers that were just created
	InputManager::Get()->UpdateDeadZonesForCurrentControllers();
}

void PCEngine::InternalShutdownDirectInput()
{
}

void PCEngine::SetGDPRAccepted(Bool bAccepted)
{
	HKEY const hKey = HKEY_CURRENT_USER;
	String sSubKey(String::Printf("%sGDPR_Compliance_Version", GetRegistrySubkeyAppRoot().CStr()));
	String const sValueName;
	String const sData = String(String::Printf("%i", bAccepted ? s_iGDPRVersion : 0));

	SEOUL_VERIFY(WriteRegistryValue(hKey, sSubKey, sValueName, sData));
}

Bool PCEngine::GetGDPRAccepted() const
{
	HKEY const hKey = HKEY_CURRENT_USER;
	String sSubKey(String::Printf("%sGDPR_Compliance_Version", GetRegistrySubkeyAppRoot().CStr()));

	String sGDPRAcceptedStatus;
	if (!GetRegistryValue(hKey, sSubKey, sGDPRAcceptedStatus) || sGDPRAcceptedStatus.IsEmpty())
	{
		// Migration from old key.
		sSubKey = String::Printf("%sGDPRAccepted", GetRegistrySubkeyAppRoot().CStr());
		if (!GetRegistryValue(hKey, sSubKey, sGDPRAcceptedStatus) || sGDPRAcceptedStatus.IsEmpty())
		{
			return false;
		}
	}

	Int version = 0;
	if (!FromString(sGDPRAcceptedStatus, version))
	{
		return false;
	}

	return version >= s_iGDPRVersion;
}

AnalyticsManager* PCEngine::InternalCreateAnalyticsManager()
{
	return CreateGenericAnalyticsManager(m_Settings.m_AnalyticsSettings);
}

/**
 * @return A CookManager subclass to be used for cooking content files:
 *   - In developer builds, if packages are not being used to service file
 *     requests and the -no_cooking command line option is not present:
 *     - CookManagerMoriarty, if connected to a Moriarty server
 *     - PCCookManager if not connected
 *   - NullCookManager otherwise
 */
CookManager* PCEngine::InternalCreateCookManager()
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

		return SEOUL_NEW(MemoryBudgets::Cooking) PCCookManager;
	}
#endif

	return SEOUL_NEW(MemoryBudgets::Cooking) NullCookManager;
}

PlatformSignInManager* PCEngine::InternalCreatePlatformSignInManager()
{
#if SEOUL_ENABLE_CHEATS
	return SEOUL_NEW(MemoryBudgets::TBD) DeveloperPlatformSignInManager;
#else
	return Engine::InternalCreatePlatformSignInManager();
#endif
}

Sound::Manager* PCEngine::InternalCreateSoundManager()
{
	// No sounds in tools/editor builds that happen to use PCEngine.
#if SEOUL_EDITOR_AND_TOOLS
	return SEOUL_NEW(MemoryBudgets::Audio) Sound::NullManager;
#else
#	if SEOUL_WITH_FMOD
		return SEOUL_NEW(MemoryBudgets::Audio) FMODSound::Manager;
#	else
		return SEOUL_NEW(MemoryBudgets::Audio) Sound::NullManager;
#	endif
#endif
}

SaveApi* PCEngine::CreateSaveApi()
{
	return SEOUL_NEW(MemoryBudgets::Saving) GenericSaveApi();
}

String PCEngine::GetSystemLanguage() const
{
	// Get the language from the system's default locale
	Byte sLanguageCode[16] = { 0 };
	int nChars = GetLocaleInfo(LOCALE_SYSTEM_DEFAULT, LOCALE_SISO639LANGNAME, sLanguageCode, sizeof(sLanguageCode));

	if (nChars == 0)
	{
		SEOUL_WARN("GetLocalInfo failed: error %d", GetLastError());
		return "English";
	}

	if (strcmp(sLanguageCode, "en") == 0)
	{
		return "English";
	}
	else if (strcmp(sLanguageCode, "fr") == 0)
	{
		return "French";
	}
	else if (strcmp(sLanguageCode, "de") == 0)
	{
		return "German";
	}
	else if (strcmp(sLanguageCode, "it") == 0)
	{
		return "Italian";
	}
	else if (strcmp(sLanguageCode, "ja") == 0)
	{
		return "Japanese";
	}
	else if (strcmp(sLanguageCode, "ko") == 0)
	{
		return "Korean";
	}
	else if (strcmp(sLanguageCode, "es") == 0)
	{
		return "Spanish";
	}
	else if (strcmp(sLanguageCode, "pt") == 0)
	{
		return "Portuguese";
	}
	else if (strcmp(sLanguageCode, "ru") == 0)
	{
		return "Russian";
	}
	else
	{
		// Use default language when requested language is not supported
		return "English";
	}
}

/**
 * Update the platform's UUID. In a platform dependent way,
 * attempts to commit the updated ID to permanent storage,
 * so future runs will return the same UUID.
 */
Bool PCEngine::UpdatePlatformUUID(const String& sPlatformUUID)
{
	// Don't allow an empty UUID
	if (sPlatformUUID.IsEmpty())
	{
		return false;
	}

	// Early out if the ID is already equal.
	if (sPlatformUUID == GetPlatformUUID())
	{
		return true;
	}

	// Otherwise, commit and update the UUID.
	HKEY const hKey = HKEY_CURRENT_USER;
	String const sSubKey(GetPlatformUUIDRegistrySubkey());

	String const sValueName;
	String sData;

	if (WriteRegistryValue(hKey, sSubKey, sValueName, sPlatformUUID))
	{
		// Commit the value to the local cache if we successfully committed
		// it to the registry.
		Lock lock(*m_pPlatformDataMutex);
		m_pPlatformData->m_sPlatformUUID = sPlatformUUID;
		return true;
	}

	// Failed to update the UUID.
	return false;
}

/**
 * Initializes our unique user IDs
 */
void PCEngine::InternalInitPlatformUUID()
{
	// Try to retrieve an already cached UUID for the pad number. If successful, we're done.
	HKEY const hKey = HKEY_CURRENT_USER;

	// Try to load from the registry first.
	String sPlatformUUID;
	{
		String const sSubKey(GetPlatformUUIDRegistrySubkey());
		if (GetRegistryValue(hKey, sSubKey, sPlatformUUID) && !sPlatformUUID.IsEmpty())
		{
			Lock lock(*m_pPlatformDataMutex);
			m_pPlatformData->m_sPlatformUUID = sPlatformUUID;
			return;
		}
	}

	// Migration handling older projects used a few variations for the key, try to load from them as well.
	{
		// SteamEngine first.
		{
			// This used to be based off a concept named LocalUser that since is always 0, so now a
			// hardcoded 0 to maintain backwards compatibility. LocalUser should have been named ControllerPadNumber,
			// and just corresponded to a controller index that we have not used in recent projects.
			String const sSubKey(String::Printf("%sSteamUUID%u", GetRegistrySubkeyAppRoot().CStr(), 0u));
			if (GetRegistryValue(hKey, sSubKey, sPlatformUUID) && !sPlatformUUID.IsEmpty())
			{
				Lock lock(*m_pPlatformDataMutex);
				m_pPlatformData->m_sPlatformUUID = sPlatformUUID;
				return;
			}
		}

		// Deprecated non-Steam storage next.
		{
			// This used to be based off a concept named LocalUser that since is always 0, so now a
			// hardcoded 0 to maintain backwards compatibility. LocalUser should have been named ControllerPadNumber,
			// and just corresponded to a controller index that we have not used in recent projects.
			String const sSubKey(String::Printf("%sUUID%u", GetRegistrySubkeyAppRoot().CStr(), 0u));
			if (GetRegistryValue(hKey, sSubKey, sPlatformUUID) && !sPlatformUUID.IsEmpty())
			{
				Lock lock(*m_pPlatformDataMutex);
				m_pPlatformData->m_sPlatformUUID = sPlatformUUID;
				return;
			}
		}
	}

	// Otherwise, generate a new entry and save it to the registry.
	sPlatformUUID = UUID::GenerateV4().ToString();

	// Commit the new ID to the registry and the m_pPlatformData->m_sPlatformUUID value.
	if (!UpdatePlatformUUID(sPlatformUUID))
	{
		// In the event of a failure, directly commit the value
		// to our local cache, we always want this step to succeed.
		Lock lock(*m_pPlatformDataMutex);
		m_pPlatformData->m_sPlatformUUID = sPlatformUUID;
	}
}

/** Called when we enter a modal Windows loop */
void PCEngine::OnModalWindowsLoopEntered()
{
	SEOUL_ASSERT(IsRenderThread());

	m_bInModalWindowsLoop = true;

	// We set a timer on our window to give us WM_TIMER messages every so often
	// so that we can keep rendering during a modal message loop
	// (such as when the user drags or resizes the window).

	SEOUL_ASSERT(m_uModalTimerID == 0);

	// Only keep rendering if the device is available and the window is still alive.
	if (m_pD3DCommonDevice.IsValid() && m_pD3DCommonDevice->PCEngineFriend_GetMainWindow())
	{
		m_uModalTimerID = SetTimer(m_pD3DCommonDevice->PCEngineFriend_GetMainWindow(), 1, 1, &PCEngine::WindowsTimerProc);
		SEOUL_ASSERT(m_uModalTimerID != 0);
	}
}

/** Called when we exit a modal Windows loop */
void PCEngine::OnModalWindowsLoopExited()
{
	SEOUL_ASSERT(IsRenderThread());

	// Kill the timer that was ticking us during the modal loop, if we have one
	if (m_uModalTimerID != 0)
	{
		SEOUL_VERIFY(KillTimer(m_pD3DCommonDevice->PCEngineFriend_GetMainWindow(), m_uModalTimerID));
		m_uModalTimerID = 0;
	}

	m_bInModalWindowsLoop = false;
}

/** Called by DefWindowProc in response to a WM_TIMER message */
void CALLBACK PCEngine::WindowsTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	SEOUL_ASSERT(IsRenderThread());

	CheckedPtr<PCEngine> pPCEngine = PCEngine::Get();

	// Give time to the Jobs::Manager to process render thread jobs.
	while (Jobs::Manager::Get()->YieldThreadTime());
}

/**
 * Utility, checks (and sets) a registry entry record if this is the first run
 * of the game on the current machine. Sets the results to m_Settings.m_AnalyticsSettings.m_bFirstRunAfterInstallation
 */
void PCEngine::InternalSetIfFirstRun()
{
	HKEY const hKey = HKEY_CURRENT_USER;
	String const sSubKey(GetRegistrySubkeyAppRoot() + "RunOnce");
	String const sValueName;
	String sData;

	Bool const bHasValue = GetRegistryValue(hKey, sSubKey, sData);
	if (!bHasValue)
	{
		if (!WriteRegistryValue(hKey, sSubKey, sValueName, String(g_sBuildChangelistStr)))
		{
			SEOUL_WARN("Failed setting first run registry key %s%s to %s, the game will think it's the first install every time it is run.",
				sSubKey.CStr(),
				sValueName.CStr(),
				sData.CStr());
		}

		// Consider this a first run if the specified registry value was not
		// present of if the changelist has been updated.
		Lock lock(*m_pPlatformDataMutex);
		m_pPlatformData->m_bFirstRunAfterInstallation = true;
	}
}

void PCEngine::InternalCallTextEditableApplyChar(UniChar c)
{
	SEOUL_ASSERT(IsMainThread());

	if (PCEngine::Get())
	{
		if (nullptr != PCEngine::Get()->m_pTextEditable)
		{
			PCEngine::Get()->m_pTextEditable->TextEditableApplyChar(c);
		}
	}
}

/** Convenience, get the absolute path to a process. */
Bool PCEngine::InternalGetProcessAbsolutePath(HANDLE hProcess, WString& rs) const
{
	Vector<wchar_t> sBuffer(MAX_PATH);

	// Special case, "self".
	if (nullptr == hProcess)
	{
		while (true)
		{
			auto const uResult = ::GetModuleFileNameW(nullptr, sBuffer.Data(), sBuffer.GetSize());
			if (0u == uResult)
			{
				// Failure.
				return false;
			}
			else if (sBuffer.GetSize() == uResult)
			{
				// Need more space.
				sBuffer.Resize(sBuffer.GetSize() * 2u);
			}
			// Otherwise, success.
			else
			{
				break;
			}
		}
	}
	// Other.
	else
	{
		// Can't retrieve, no function.
		if (nullptr == m_pGetModuleFileNameExW)
		{
			return false;
		}

		auto pGet = (GetModuleFileNameExW)m_pGetModuleFileNameExW;
		while (true)
		{
			auto const uResult = pGet(hProcess, nullptr, sBuffer.Data(), sBuffer.GetSize());
			if (0u == uResult)
			{
				// Failure.
				return false;
			}
			else if (sBuffer.GetSize() == uResult)
			{
				// Need more space.
				sBuffer.Resize(sBuffer.GetSize() * 2u);
			}
			// Otherwise, success.
			else
			{
				break;
			}
		}
	}

	rs = WString(WCharTToUTF8(sBuffer.Data()));
	return true;
}

/**
 * Load user settings from disk - if this fails for any reason, the user
 * settings structure will be reset to its default values.
 */
void PCEngine::InternalLoadUserSettings()
{
	FilePath const userConfigJsonFilePath = GamePaths::Get()->GetUserConfigJsonFilePath();

	// Load the user settings.
	if (!FileManager::Get()->Exists(userConfigJsonFilePath) ||
		!SettingsManager::Get()->DeserializeObject(userConfigJsonFilePath, &m_UserSettings))
	{
		// Reset to defaults.
		m_UserSettings = PCEngineUserSettings();
	}

	// Give the D3DDeviceCommn a chance to configure initial settings that are invalid or have never
	// configured.
	D3DCommonDevice::CheckAndConfigureSettings(m_UserSettings.m_GraphicsSettings);
}

/**
 * Commit user settings to disk.
 */
void PCEngine::InternalSaveUserSettings()
{
	FilePath const userConfigJsonFilePath = GamePaths::Get()->GetUserConfigJsonFilePath();

	ScopedPtr<SyncFile> pFile;
	DataStore dataStore;
	dataStore.MakeArray();
	String s;

	if (!Seoul::Reflection::SerializeObjectToArray(
		userConfigJsonFilePath,
		dataStore,
		dataStore.GetRootNode(),
		0u,
		&m_UserSettings))
	{
		goto error;
	}

	dataStore.ReplaceRootWithArrayElement(dataStore.GetRootNode(), 0u);

	// Output.
	dataStore.ToString(dataStore.GetRootNode(), s, true, 0, true);
	if (s.IsEmpty())
	{
		goto error;
	}

	if (!FileManager::Get()->WriteAll(userConfigJsonFilePath, s.CStr(), s.GetSize()))
	{
		goto error;
	}

	// Flush.
	pFile.Reset();

	// On success, cleanup any stale .ini file that might still exist. The .ini file is now deprecated.
	{
		auto const sIni(Path::ReplaceExtension(userConfigJsonFilePath.GetAbsoluteFilename(), ".ini"));
		(void)FileManager::Get()->Delete(sIni);
	}

	return;

error:
	SEOUL_WARN("Failed saving user settings to: %s\n", userConfigJsonFilePath.CStr());
}

// Shows a platform-specific message box
void PCShowMessageBox(
	const String& sMessage,
	const String& sTitle,
	MessageBoxCallback onCompleteCallback,
	EMessageBoxButton eDefaultButton,
	const String& sButtonLabel1,
	const String& sButtonLabel2,
	const String& sButtonLabel3)
{
	HWND hwndOwner = nullptr;

	// TODO: Support customizable button labels on Windows.  See
	// http://blogs.msdn.com/b/oldnewthing/archive/2005/04/29/412577.aspx for
	// an example of how that might be done.
	UINT uFlags = MB_ICONWARNING | MB_SETFOREGROUND;
	if (!sButtonLabel3.IsEmpty())
	{
		uFlags |= MB_YESNOCANCEL;
		uFlags |= (eDefaultButton == kMessageBoxButton1 ? MB_DEFBUTTON1 :
				   eDefaultButton == kMessageBoxButton2 ? MB_DEFBUTTON2 :
				                                          MB_DEFBUTTON3);
	}
	else if (!sButtonLabel2.IsEmpty())
	{
		uFlags |= MB_YESNO;
		uFlags |= (eDefaultButton == kMessageBoxButtonYes ? MB_DEFBUTTON1 : MB_DEFBUTTON2);
	}
	else
	{
		uFlags |= MB_OK;
	}

	int iResult = MessageBoxW(hwndOwner, sMessage.WStr(), sTitle.WStr(), uFlags);
	if (onCompleteCallback)
	{
		EMessageBoxButton eButtonPressed;
		if (!sButtonLabel3.IsEmpty())
		{
			eButtonPressed =
				(iResult == IDYES ? kMessageBoxButton1 :
				 iResult == IDNO  ? kMessageBoxButton2 :
				                    kMessageBoxButton3);
		}
		else if (!sButtonLabel2.IsEmpty())
		{
			eButtonPressed = (iResult == IDYES ? kMessageBoxButtonYes : kMessageBoxButtonNo);
		}
		else
		{
			eButtonPressed = kMessageBoxButtonOK;
		}

		onCompleteCallback(eButtonPressed);
	}
}

/**
 * PC-specific core function table
 */
static const CoreVirtuals s_PCCoreVirtuals =
{
	&PCShowMessageBox,
	&LocManager::CoreLocalize,
	&Engine::CoreGetPlatformUUID,
	&Engine::CoreGetUptime,
};

/**
 * PC-specific core function table pointer
 */
const CoreVirtuals *g_pCoreVirtuals = &s_PCCoreVirtuals;

} // namespace Seoul
