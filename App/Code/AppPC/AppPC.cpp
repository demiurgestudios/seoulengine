/**
 * \file AppPC.cpp
 * \brief Defines the entry point for the PC game
 *
 * Copyright (c) Demiurge Studios, Inc.
 *
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Platform.h"
#include "AppPCCommandLineArgs.h"
#include "AppPCResource.h"
#include "AppPCAutomatedTests.h"
#include "AppPCRunScript.h"
#include "AppPCUnitTests.h"
#include "CheckedPtr.h"
#include "CrashManager.h"
#include "DiskFileSystem.h"
#include "EngineCommandLineArgs.h"
#include "FileManager.h"
#include "GameClient.h"
#include "GameClientSettings.h"
#include "GameConfigManager.h"
#include "GameMain.h"
#include "GamePaths.h"
#include "Logger.h"
#include "MoriartyFileSystem.h"
#include "PatchablePackageFileSystem.h"
#include "PCEngineDefault.h"
#include "Platform.h"
#include "Prereqs.h"
#include "ReflectionCommandLineArgs.h"
#include "ReflectionDataStoreTableUtil.h"
#include "ReflectionDefine.h"
#include "RenderDevice.h"
#include "ScopedAction.h"
#include "SettingsManager.h"
#include "Thread.h"
#if SEOUL_WITH_STEAM
#include "SteamEngine.h"
#endif

#include <ShellAPI.h>

namespace Seoul
{

D3DDeviceEntry GetD3D11DeviceWindowEntry();

/** Cache the PatchablePackageFileSystem, passed to game app for handling downloadable config updates. */
static CheckedPtr<PatchablePackageFileSystem> s_pPatchableConfigPackageFileSystem;

/** Cache the PatchablePackageFileSystem, passed to game app for handling downloadable content updates. */
static CheckedPtr<PatchablePackageFileSystem> s_pPatchableContentPackageFileSystem;

/**
 * Global hook, called by FileManager as early as possible during initialization,
 * to give us a chance to hook up our file systems before any file requests are made.
 */
static void OnInitializeFileSystems()
{
	// Difference behavior in developer builds
	// if using package file systems.
	auto const bUsePackages = (SEOUL_SHIP ||
		(EngineCommandLineArgs::GetPreferUsePackageFiles() ||
			!DiskSyncFile::FileExists(Path::Combine(GamePaths::Get()->GetBaseDir(), "Data/Config/application.json"))));

	// FileManager checks FileSystems in LIFO order, so we want the DiskFileSystem to
	// be absolutely last - check packages first - when packages are enabled.
	if (bUsePackages)
	{
		FileManager::Get()->RegisterFileSystem<DiskFileSystem>();
	}

	// PC_ClientSettings.sar - loaded into memory for performance and to
	// avoid lock contention for developers when syncing Perforce on Windows.
	FileManager::Get()->RegisterFileSystem<PackageFileSystem>(
		Path::Combine(GamePaths::Get()->GetBaseDir(), "Data/PC_ClientSettings.sar"),
		true);

	// If in a non-ship build, we disable package files systems (other than
	// client settings) unless explicitly enabled or the Data/Config/application.json
	// files is missing (we use this as an indicator of an external packaging
	// of the developer build).
	if (bUsePackages)
	{
		// PC_Config.sar
		s_pPatchableConfigPackageFileSystem = FileManager::Get()->RegisterFileSystem<PatchablePackageFileSystem>(
			Path::Combine(GamePaths::Get()->GetBaseDir(), "Data/PC_Config.sar"), // read-only builtin
			Path::Combine(GamePaths::Get()->GetSaveDir(), "Data/PC_ConfigUpdate.sar")); // updateable path

		// PC_Content.sar
		FileManager::Get()->RegisterFileSystem<PackageFileSystem>(
			Path::Combine(GamePaths::Get()->GetBaseDir(), "Data/PC_Content.sar"));

		// PC_BaseContent.sar
		FileManager::Get()->RegisterFileSystem<PackageFileSystem>(
			Path::Combine(GamePaths::Get()->GetBaseDir(), "Data/PC_BaseContent.sar"));

		// In non-ship builds, also include debug script files.
#if !SEOUL_SHIP
		FileManager::Get()->RegisterFileSystem<PackageFileSystem>(
			Path::Combine(GamePaths::Get()->GetBaseDir(), "Data/PC_ScriptsDebug.sar"));
#endif // /#if !SEOUL_SHIP

		// PC_ContentUpdate.sar
		s_pPatchableContentPackageFileSystem = FileManager::Get()->RegisterFileSystem<PatchablePackageFileSystem>(
			Path::Combine(GamePaths::Get()->GetBaseDir(), "Data/PC_ContentUpdate.sar"), // read-only builtin
			Path::Combine(GamePaths::Get()->GetSaveDir(), "Data/PC_ContentUpdate.sar")); // updateable path
	}
	else
	{
		FileManager::Get()->RegisterFileSystem<DiskFileSystem>();
	}

#if SEOUL_WITH_MORIARTY
	if (!EngineCommandLineArgs::GetMoriartyServer().IsEmpty())
	{
		FileManager::Get()->RegisterFileSystem<MoriartyFileSystem>();
	}
#endif
}

/**
 * Windows main wrapper
 *
 * Initialize the game and enter the render loop
 *
 * @param[in] hInstance Current instance
 * @param[in] hPrevInstance Previous instance
 * @param[in] lpCmdLine Input command line
 * @param[in] nShowCmd Specifies how the window should appear (maximized, minimized, etc)
 *
 * @return    Returns an integer exit code
 */

int RealWinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPWSTR lpCmdLine,
	int nShowCmd)
{
#if SEOUL_WITH_STEAM
	if (SteamEngine::RestartAppIfNecessary(<steam-app-id>))
	{
		return 1;
	}
#endif // /#if SEOUL_WITH_STEAM

	// Hook up a callback that will be invoked when the FileSystem is starting up,
	// so we can configure the game's packages before any file requests are made.
	g_pInitializeFileSystemsCallback = OnInitializeFileSystems;

	// Initialize SeoulTime
	SeoulTime::MarkGameStartTick();

	// Mark that we're now in the main function.
	auto const inMain(MakeScopedAction(&BeginMainFunction, &EndMainFunction));

	// Setup some game specific paths before initializing Engine and Core.
	GamePaths::SetUserConfigJsonFileName("game_config.json");

	// Set the main thread to the current thread.
	SetMainThreadId(Thread::GetThisThreadId());

	PCEngineSettings settings;
	settings.m_RenderDeviceSettings.m_hInstance = hInstance;

	// Ordered list of devices we support. Highest priority first.
	settings.m_RenderDeviceSettings.m_vEntries.PushBack(GetD3D11DeviceWindowEntry());

	// Cursor and icon settings.
#if SEOUL_SHIP && !defined(SEOUL_PROFILING_BUILD)
	settings.m_RenderDeviceSettings.m_aMouseCursors[(UInt32)MouseCursor::kArrow] = (HCURSOR)::LoadImage(hInstance, MAKEINTRESOURCE(IDC_SHIP_CURSOR_ARROW), IMAGE_CURSOR, 0, 0, 0);
#else // !(SEOUL_SHIP && !defined(SEOUL_PROFILING_BUILD))
	settings.m_RenderDeviceSettings.m_aMouseCursors[(UInt32)MouseCursor::kArrow] = (HCURSOR)::LoadImage(hInstance, MAKEINTRESOURCE(IDC_DEV_CURSOR_ARROW), IMAGE_CURSOR, 0, 0, 0);
	settings.m_RenderDeviceSettings.m_aMouseCursors[(UInt32)MouseCursor::kArrowLeftBottomRightTop] = (HCURSOR)::LoadImage(hInstance, MAKEINTRESOURCE(IDC_DEV_CURSOR_ARROW_LBRT), IMAGE_CURSOR, 0, 0, 0);
	settings.m_RenderDeviceSettings.m_aMouseCursors[(UInt32)MouseCursor::kArrowLeftRight] = (HCURSOR)::LoadImage(hInstance, MAKEINTRESOURCE(IDC_DEV_CURSOR_ARROW_LR), IMAGE_CURSOR, 0, 0, 0);
	settings.m_RenderDeviceSettings.m_aMouseCursors[(UInt32)MouseCursor::kArrowLeftTopRightBottom] = (HCURSOR)::LoadImage(hInstance, MAKEINTRESOURCE(IDC_DEV_CURSOR_ARROW_LTRB), IMAGE_CURSOR, 0, 0, 0);
	settings.m_RenderDeviceSettings.m_aMouseCursors[(UInt32)MouseCursor::kArrowUpDown] = (HCURSOR)::LoadImage(hInstance, MAKEINTRESOURCE(IDC_DEV_CURSOR_ARROW_UD), IMAGE_CURSOR, 0, 0, 0);
	settings.m_RenderDeviceSettings.m_aMouseCursors[(UInt32)MouseCursor::kIbeam] = (HCURSOR)::LoadImage(hInstance, MAKEINTRESOURCE(IDC_DEV_CURSOR_IBEAM), IMAGE_CURSOR, 0, 0, 0);
	settings.m_RenderDeviceSettings.m_aMouseCursors[(UInt32)MouseCursor::kMove] = (HCURSOR)::LoadImage(hInstance, MAKEINTRESOURCE(IDC_DEV_CURSOR_MOVE), IMAGE_CURSOR, 0, 0, 0);
#endif // /!(SEOUL_SHIP && !defined(SEOUL_PROFILING_BUILD))
	settings.m_RenderDeviceSettings.m_iApplicationIcon = IDI_PCLAUNCH;

	// Graphics minimum requirements
	settings.m_RenderDeviceSettings.m_uMinimumPixelShaderVersion = 2u;
	settings.m_RenderDeviceSettings.m_uMinimumVertexShaderVersion = 2u;

	// General behavior settings.
	settings.m_SaveLoadManagerSettings = Game::Main::GetSaveLoadManagerSettings(Game::MainSettings::kOff);
	settings.m_AnalyticsSettings.m_GetApiKeyDelegate = SEOUL_BIND_DELEGATE(&Game::ClientSettings::GetAnalyticsApiKey);
	settings.m_AnalyticsSettings.m_eType = GenericAnalyticsManagerType::kMixpanel;
	settings.m_AnalyticsSettings.m_CustomCurrentTimeDelegate = SEOUL_BIND_DELEGATE(&Game::Client::StaticGetCurrentServerTime);

	// Startup, run, and shutdown.
	{
		NullCrashManager crashManager;
#if SEOUL_WITH_STEAM
		SteamEngine engine(settings);
#else
		PCEngineDefault engine(settings);
#endif
		engine.Initialize();

		// Multiple copy handling may trigger a quit
		// during initialize, so just skip everything else.
		if (!engine.WantsQuit())
		{
			{
				auto const sServerBaseURL(Game::ClientSettings::GetServerBaseURL());
#if SEOUL_WITH_GAME_PERSISTENCE
				GamePersistenceSettings persistenceSettings;
				persistenceSettings.m_FilePath = FilePath::CreateSaveFilePath(GameClientSettings::GetSaveGameFilename());
				if (!sServerBaseURL.IsEmpty())
				{
					persistenceSettings.m_sCloudLoadURL = sServerBaseURL + "/v1/saving/load";
					persistenceSettings.m_sCloudResetURL = sServerBaseURL + "/v1/saving/reset";
					persistenceSettings.m_sCloudSaveURL = sServerBaseURL + "/v1/saving/save";
				}
				persistenceSettings.m_iVersion = AppPersistenceMigrations::kiPlayerDataVersion;
				persistenceSettings.m_pPersistenceManagerType = &TypeOf<AppPersistenceManager>();
				persistenceSettings.m_tMigrations = AppPersistenceMigrations::GetMigrations();
				GameMainSettings settings(TypeOf<AppConfigManager>(), persistenceSettings);
#else
				Game::MainSettings settings(TypeOf<Game::NullConfigManager>());
#endif // /#if SEOUL_WITH_GAME_PERSISTENCE

				settings.m_sServerBaseURL = sServerBaseURL;
				settings.m_pConfigUpdatePackageFileSystem = s_pPatchableConfigPackageFileSystem;
				settings.m_pContentUpdatePackageFileSystem = s_pPatchableContentPackageFileSystem;

#if !SEOUL_SHIP
				// Conditional support, when enabled, activates automated testing in the normal PC build (can be
				// used for local reproduction of bugs).
				if (!EngineCommandLineArgs::GetAutomationScript().IsEmpty())
				{
					settings.m_sAutomationScriptMain = EngineCommandLineArgs::GetAutomationScript();
					settings.m_eAutomatedTesting = Game::MainSettings::kAutomatedTesting;

					// Also disable OpenURL() to prevent loss of focus.
					Engine::Get()->SetSuppressOpenURL(true);
				}
#endif // /#if !SEOUL_SHIP

				// Only hookup to CrashManager if custom crashes are supported.
				if (CrashManager::Get()->CanSendCustomCrashes())
				{
					settings.m_ScriptErrorHandler = SEOUL_BIND_DELEGATE(&PCSendCustomCrash);
				}
				// In non-ship builds, fall back to default handling.
#if !SEOUL_SHIP
				else
				{
					settings.m_ScriptErrorHandler = SEOUL_BIND_DELEGATE(&CrashManager::DefaultErrorHandler);
				}
#endif // /#if !SEOUL_SHIP

				{
					Game::Main main(settings);
					main.Run();
				}
			}
		}

		engine.Shutdown();
	}

	return 0;
}

static inline void GetOptionalArgument(wchar_t const* sArg, wchar_t aOptionalArgument[MAX_PATH])
{
	// Find the (optional) test name argument.
	wchar_t const* sEqualPosition = wcsstr(sArg, L"=");
	if (nullptr != sEqualPosition)
	{
		wchar_t const* sOptionalArgument = sEqualPosition + 1;
		while (*sOptionalArgument && iswspace(*sOptionalArgument))
		{
			++sOptionalArgument;
		}

		(void)wcscpy_s(aOptionalArgument, MAX_PATH, sOptionalArgument);
	}
}

#if SEOUL_WITH_STEAM
/**
 * Seoul Win32 exception filter: tells Steam to create a minidump and upload it
 * to Steam's servers, if the game was launched from the Steam client.
 *
 * @param[in] uExceptionCode Exception code, e.g. EXCEPTION_ACCESS_VIOLATION
 * @param[in] pExceptionInfo Pointer to the machine-dependent exception record
 *            and context record structures
 *
 * @return Code indicating how execution is to continue after the exception
 */
int ExceptionFilter(DWORD uExceptionCode, LPEXCEPTION_POINTERS pExceptionInfo)
{
	SteamEngine::WriteMiniDump(uExceptionCode, pExceptionInfo);

	// Execute the __except handler
	return EXCEPTION_EXECUTE_HANDLER;
}
#endif

} // namespace Seoul

/**
 * Windows program entry point
 */
INT WINAPI wWinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPWSTR lpCmdLine,
	int nShowCmd)
{
	// Set abort behavior - fully enabled in non-ship builds,
	// fully disabled in ship builds.
#if SEOUL_SHIP
	_set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#else
	_set_abort_behavior(0xFFFFFFFF, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif

	// Enable run-time memory check for debug builds.
#if SEOUL_DEBUG
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

	// Parse command-line arguments.
	{
		using namespace Seoul;

		// Get args.
		Int iArgC = 0;
		auto ppArgV = (wchar_t const**)::CommandLineToArgvW(::GetCommandLineW(), &iArgC);

		// Process.
		(void)Reflection::CommandLineArgs::Parse(ppArgV + 1, ppArgV + iArgC);

		// CommandLineToArgvW allocates memory for us, so we have to free it
		::LocalFree(ppArgV);
		ppArgV = nullptr;
	}

	// Enable as early as possible.
#if SEOUL_ENABLE_MEMORY_TOOLING
	if (Seoul::AppPCCommandLineArgs::GetVerboseMemoryTooling() ||
		Seoul::AppPCCommandLineArgs::GetRunUnitTests().IsSet())
	{
		Seoul::MemoryManager::SetVerboseMemoryLeakDetectionEnabled(true);
	}
#endif // /SEOUL_ENABLE_MEMORY_TOOLING

	// State.
	auto const bRunScript = !Seoul::AppPCCommandLineArgs::GetRunScript().IsEmpty();
	auto const bRunAutomatedTests = !Seoul::AppPCCommandLineArgs::GetRunAutomatedTest().IsEmpty();
	auto const bEnableDownloadableContent = Seoul::AppPCCommandLineArgs::GetDownloadablePackageFileSystemsEnabled();
	auto const bRunUnitTests = Seoul::AppPCCommandLineArgs::GetRunUnitTests().IsSet();
	auto const bPersistent = Seoul::AppPCCommandLineArgs::GetPersistentTest();
	auto const bGenerateScriptBindings = !Seoul::AppPCCommandLineArgs::GetGenerateScriptBindings().IsEmpty();

	// If unit testing is enabled, check if we're running to
	// execute unit tests or automated tests.
#if (SEOUL_AUTO_TESTS || SEOUL_UNIT_TESTS)
	{
		// One and one only.
		if (bRunAutomatedTests + bRunScript + bRunUnitTests > 1)
		{
			fprintf(stderr, "-run_script, -run_unit_tests and -run_automated_tests are mutually exclusive.\n");
			return 1;
		}

#if !SEOUL_SHIP
		// Run a script.
		if (bGenerateScriptBindings)
		{
			return Seoul::AppPCRunScript(true, Seoul::AppPCCommandLineArgs::GetGenerateScriptBindings());
		}
		else if (bRunScript)
		{
			return Seoul::AppPCRunScript(false, Seoul::AppPCCommandLineArgs::GetRunScript());
		}
#endif

#if SEOUL_AUTO_TESTS
		// Run automated tests.
		if (bRunAutomatedTests)
		{
			return Seoul::AppPCRunAutomatedTests(
				Seoul::AppPCCommandLineArgs::GetRunAutomatedTest(),
				bEnableDownloadableContent,
				bPersistent);
		}
#endif // /#if SEOUL_AUTO_TESTS

#if SEOUL_UNIT_TESTS
		// Run unit tests
		if (bRunUnitTests)
		{
			return Seoul::AppPCRunUnitTests(Seoul::AppPCCommandLineArgs::GetRunUnitTests());
		}
#endif // /#if SEOUL_UNIT_TESTS
	}
#endif // /#if (SEOUL_AUTO_TESTS || SEOUL_UNIT_TESTS)

	// Set up Steam crash handling if no debugger is present (don't want to
	// report crashes for developers)
	if (IsDebuggerPresent())
	{
		return Seoul::RealWinMain(hInstance, hPrevInstance, lpCmdLine, nShowCmd);
	}
	else
	{
#	if SEOUL_WITH_STEAM
		__try
		{
#	endif
			return Seoul::RealWinMain(hInstance, hPrevInstance, lpCmdLine, nShowCmd);
#	if SEOUL_WITH_STEAM
		}
		__except (Seoul::ExceptionFilter(GetExceptionCode(), GetExceptionInformation()))
		{
			return 255;
		}
#	endif
	}
}
