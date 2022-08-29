/**
 * \file AppLinuxAutomatedTests.cpp
 * \brief Defines the main function for a build run that will execute
 * automated tests.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Platform.h"
#include "AppLinuxAutomatedTests.h"
#include "AppLinuxCommandLineArgs.h"
#include "CrashManager.h"
#include "DataStoreParser.h"
#include "DiskFileSystem.h"
#include "DownloadablePackageFileSystem.h"
#include "FileManager.h"
#include "GameAutomation.h"
#include "GameClientSettings.h"
#include "GameConfigManager.h"
#include "GameMain.h"
#include "GamePaths.h"
#include "HTTPManager.h"
#include "Logger.h"
#include "NullPlatformEngine.h"
#include "PackageFileSystem.h"
#include "PatchablePackageFileSystem.h"
#include "Path.h"
#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionDataStoreTableUtil.h"
#include "ReflectionDefine.h"
#include "ScopedAction.h"
#include "SettingsManager.h"
#include "StringUtil.h"
#include "Thread.h"

namespace Seoul
{

void LinuxSendCustomCrash(const CustomCrashErrorState& errorState)
{
	// Pass the custom crash data through to CrashManager.
	if (CrashManager::Get())
	{
		CrashManager::Get()->SendCustomCrash(errorState);
	}
}

#if SEOUL_AUTO_TESTS

static CheckedPtr<PatchablePackageFileSystem> s_pPatchableConfigPackageFileSystem;
static CheckedPtr<PatchablePackageFileSystem> s_pPatchableContentPackageFileSystem;

/** Conditionally enabled downloadable filesystem for content. */
extern CheckedPtr<DownloadablePackageFileSystem> g_pDownloadableContentPackageFileSystem;
static Bool s_bEnableDownloadableContent = false;

static void OnInitializeFileSystems()
{
	// Intentional - Android/Linux are data compatible, so we just use Android content on Linux.

	// Android_ClientSettings.sar
	FileManager::Get()->RegisterFileSystem<PackageFileSystem>(
		Path::Combine(GamePaths::Get()->GetBaseDir(), "Data/Android_ClientSettings.sar"));

	// Android_Config.sar
	s_pPatchableConfigPackageFileSystem = FileManager::Get()->RegisterFileSystem<PatchablePackageFileSystem>(
		Path::Combine(GamePaths::Get()->GetBaseDir(), "Data/Android_Config.sar"), // read-only builtin
		Path::Combine(GamePaths::Get()->GetSaveDir(), "Data/Android_ConfigUpdate.sar")); // updateable path

	// If true, this automated test should use a downloadable content package.
	String const sServerBaseURL(Game::ClientSettings::GetServerBaseURL());
	if (s_bEnableDownloadableContent && !sServerBaseURL.IsEmpty())
	{
		// Configure downloader with default settings.
		DownloadablePackageFileSystemSettings settings;
		settings.m_sAbsolutePackageFilename = Path::Combine(GamePaths::Get()->GetSaveDir(), "Data/Android_Content.sar");
		settings.m_sInitialURL = String::Printf("%s/v1/auth/additional_clientgamedata", sServerBaseURL.CStr());
		g_pDownloadableContentPackageFileSystem = FileManager::Get()->RegisterFileSystem<DownloadablePackageFileSystem>(settings);
	}
	else
	{
		// Android_Content.sar
		FileManager::Get()->RegisterFileSystem<PackageFileSystem>(
			Path::Combine(GamePaths::Get()->GetBaseDir(), "Data/Android_Content.sar"));
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

	// Read-only, restricted file system to the content://Authored/Scripts/DevOnly/AutomatedTests/ folder.
	{
		FilePath restrictedDirectoryPath = FilePath::CreateContentFilePath("Authored/Scripts/DevOnly/AutomatedTests");
		FileManager::Get()->RegisterFileSystem<RestrictedDiskFileSystem>(restrictedDirectoryPath, true);
	}
}

static void OnInitializeFileSystemsPersistent()
{
	OnInitializeFileSystems();

	// Read-write, restricted file system to the save:// folder.
	{
		FilePath restrictedDirectoryPath = FilePath::CreateSaveFilePath(String());
		FileManager::Get()->RegisterFileSystem<RestrictedDiskFileSystem>(restrictedDirectoryPath, false);
	}
}

static Int AppLinuxRunAutomatedTestsImplLevel2(NullPlatformEngine& engine, const String& sAutomationScriptFileName)
{
	// Override the UUID to prepend "test", so we can easily identify
	// users added to the server that were generated as part of automated testing.
	{
		String sUUID = engine.GetPlatformUUID();
		if (!sUUID.IsEmpty())
		{
			sUUID = "test" + sUUID;

			// Truncate to 40 characters, this is the server limit for a device ID.
			sUUID = sUUID.Substring(0u, 40u);

			engine.UpdatePlatformUUID(sUUID);
		}
	}

#if SEOUL_ENABLE_MEMORY_TOOLING
	// Output memory leak info to stdout instead of a file.
	MemoryManager::SetMemoryLeaksFilename(String());
#endif // /SEOUL_ENABLE_MEMORY_TOOLING

	// Convert the automation script filename.
	Int iAdditionalWarningCount = 0;
	{
		{
			auto const sServerBaseURL(Game::ClientSettings::GetServerBaseURL());
#if SEOUL_WITH_GAME_PERSISTENCE
			Game::PersistenceSettings persistenceSettings;
			persistenceSettings.m_FilePath = FilePath::CreateSaveFilePath("player-save-test.dat");
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
			settings.m_sAutomationScriptMain = sAutomationScriptFileName;
			settings.m_eAutomatedTesting = (engine.GetSettings().m_bPersistent
				? Game::MainSettings::kPersistentAutomatedTesting
				: Game::MainSettings::kAutomatedTesting);

			// Only hookup to CrashManager if custom crashes are supported.
			if (CrashManager::Get()->CanSendCustomCrashes())
			{
				settings.m_ScriptErrorHandler = SEOUL_BIND_DELEGATE(&LinuxSendCustomCrash);
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
				if (Game::Automation::Get())
				{
					iAdditionalWarningCount = (Int)Game::Automation::Get()->GetAdditionalWarningCount();
				}
			}
		}
	}

	return iAdditionalWarningCount;
}

static Int AppLinuxRunAutomatedTestsImplLevel1(NullPlatformEngine& engine, const String& sAutomationScriptFileName)
{
	return AppLinuxRunAutomatedTestsImplLevel2(engine, sAutomationScriptFileName);
}

static Int AppLinuxRunAutomatedTestsImplLevel0(const String& sAutomationScriptFileName, Bool bPersistent)
{
	g_pInitializeFileSystemsCallback = (bPersistent
		? OnInitializeFileSystemsPersistent
		: OnInitializeFileSystems);

	// Initialize SeoulTime
	SeoulTime::MarkGameStartTick();

	// Mark that we're now in the main function.
	auto const inMain(MakeScopedAction(&BeginMainFunction, &EndMainFunction));

	GamePaths::SetUserConfigJsonFileName("game_config.json");

	SetMainThreadId(Thread::GetThisThreadId());

	// Configure booleans for automated testing.
	g_bRunningAutomatedTests = true;
	g_bHeadless = true;
	g_bShowMessageBoxesOnFailedAssertions = false;
	g_bEnableMessageBoxes = false;

	// Enable all logger channels.
#if SEOUL_LOGGING_ENABLED
	Logger::GetSingleton().EnableAllChannels(true);
#endif // /#if SEOUL_LOGGING_ENABLED

	// Startup, run, and shutdown.
	Int iInnerResult = 0;
	{
		NullCrashManager crashManager;
		NullPlatformEngineSettings settings;
		settings.m_SaveLoadManagerSettings = Game::Main::GetSaveLoadManagerSettings(bPersistent
			? Game::MainSettings::kPersistentAutomatedTesting
			: Game::MainSettings::kAutomatedTesting);
		settings.m_bEnableGenericKeyboardInput = true;
		settings.m_bEnableGenericMouseInput = true;
		settings.m_bEnableSaveApi = true;
		settings.m_bPersistent = bPersistent;
		settings.m_iViewportWidth = 720;
		settings.m_iViewportHeight = 1280;
		NullPlatformEngine engine(settings);
		engine.Initialize();
		iInnerResult = AppLinuxRunAutomatedTestsImplLevel1(engine, sAutomationScriptFileName);
		engine.Shutdown();
	}

#if SEOUL_LOGGING_ENABLED
	// Return the number of warnings and serialization errors to
	// indicate problems.
	return iInnerResult + Logger::GetSingleton().GetWarningCount();
#else // !SEOUL_LOGGING_ENABLED
	// Otherwise, just assume no problems unless a crash occured.
	return iInnerResult;
#endif // /!SEOUL_LOGGING_ENABLED
}

Int AppLinuxRunAutomatedTests(const String& sAutomationScriptFileName, Bool bEnableDownloadableContent, Bool bPersistent)
{
	s_bEnableDownloadableContent = bEnableDownloadableContent;
	return AppLinuxRunAutomatedTestsImplLevel0(sAutomationScriptFileName, bPersistent);
}

#endif // /#if SEOUL_AUTO_TESTS

} // namespace Seoul
