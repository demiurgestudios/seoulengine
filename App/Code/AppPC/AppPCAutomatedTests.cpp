/**
 * \file AppPCAutomatedTests.cpp
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
#include "AppPCCommandLineArgs.h"
#include "AppPCAutomatedTests.h"
#include "CrashManager.h"
#include "DataStoreParser.h"
#include "DiskFileSystem.h"
#include "D3DCommonDevice.h"
#include "D3DCommonDeviceSettings.h"
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
#include <ShellAPI.h>

namespace Seoul
{

void PCSendCustomCrash(const CustomCrashErrorState& errorState)
{
	// Pass the custom crash data through to CrashManager.
	if (CrashManager::Get())
	{
		CrashManager::Get()->SendCustomCrash(errorState);
	}
}

#if SEOUL_AUTO_TESTS

/**
 * FileSystem used to capture file access and generate/update
 * a training database, used to exclude files (currently textures)
 * from overflow that are needed early in program flow.
 *
 * Overflow files are those moved into an additional download and
 * downloaded by the game on-the-fly.
 */
class AppPCTrainerFileSystem SEOUL_SEALED : public IFileSystem
{
public:
	struct Entry SEOUL_SEALED
	{
		FilePath m_FilePath{};
		UInt32 m_uDeterioration{};
		WorldTime m_Time{};
	};
	typedef HashTable<FilePath, UInt32, MemoryBudgets::Developer> Lookup;
	typedef Vector<Entry, MemoryBudgets::Developer> Order;

	AppPCTrainerFileSystem(FilePath filePath)
		: m_FilePath(filePath)
		, m_Now(WorldTime::GetUTCTime())
	{
	}

	~AppPCTrainerFileSystem()
	{
	}

	virtual Bool Copy(FilePath from, FilePath to, Bool bAllowOverwrite = false) SEOUL_OVERRIDE { return false; }
	virtual Bool Copy(const String& sAbsoluteFrom, const String& sAbsoluteTo, Bool bAllowOverwrite = false) SEOUL_OVERRIDE { return false; }
	virtual Bool CreateDirPath(FilePath dirPath) SEOUL_OVERRIDE { return false; }
	virtual Bool CreateDirPath(const String& sAbsoluteDirPath) SEOUL_OVERRIDE { return false; }
	virtual Bool DeleteDirectory(FilePath dirPath, Bool bRecursive) SEOUL_OVERRIDE { return false; }
	virtual Bool DeleteDirectory(const String& sAbsoluteDirPath, Bool bRecursive) SEOUL_OVERRIDE { return false; }
	virtual Bool GetFileSize(FilePath filePath, UInt64& rzFileSize) const SEOUL_OVERRIDE { return false; }
	virtual Bool GetFileSize(const String& sAbsoluteFilename, UInt64& rzFileSize) const SEOUL_OVERRIDE { return false; }
	virtual Bool GetModifiedTime(FilePath filePath, UInt64& ruModifiedTime) const SEOUL_OVERRIDE { return false; }
	virtual Bool GetModifiedTime(const String& sAbsoluteFilename, UInt64& ruModifiedTime) const SEOUL_OVERRIDE { return false; }
	virtual Bool SetModifiedTime(FilePath filePath, UInt64 uModifiedTime) SEOUL_OVERRIDE { return false; }
	virtual Bool SetModifiedTime(const String& sAbsoluteFilename, UInt64 uModifiedTime) SEOUL_OVERRIDE { return false; }
	virtual Bool Delete(FilePath filePath) SEOUL_OVERRIDE { return false; }
	virtual Bool Delete(const String& sAbsoluteFilename) SEOUL_OVERRIDE { return false; }
	virtual Bool Exists(FilePath filePath) const SEOUL_OVERRIDE { return false; }
	virtual Bool Exists(const String& sAbsoluteFilename) const SEOUL_OVERRIDE { return false; }
	virtual Bool IsDirectory(FilePath filePath) const SEOUL_OVERRIDE { return false; }
	virtual Bool IsDirectory(const String& sAbsoluteFilename) const SEOUL_OVERRIDE { return false; }
	virtual Bool Open(
		FilePath filePath,
		File::Mode eMode,
		ScopedPtr<SyncFile>& rpFile) SEOUL_OVERRIDE
	{
		// Only care about textures and audio for the time being.
		if (!IsTextureFileType(filePath.GetType()) &&
			filePath.GetType() != FileType::kSoundBank &&
			filePath.GetType() != FileType::kSoundProject)
		{
			return false;
		}

		// Reduce textures to the base type.
		if (IsTextureFileType(filePath.GetType()))
		{
			filePath.SetType(FileType::kTexture0);
		}

		// Track the file open.
		Lock lock(m_Mutex);

		// Insert if new.
		UInt32 uIndex = 0u;
		if (!m_tLookup.GetValue(filePath, uIndex))
		{
			uIndex = m_vOrder.GetSize();
			SEOUL_VERIFY(m_tLookup.Insert(filePath, uIndex).Second);

			Entry entry;
			entry.m_FilePath = filePath;
			entry.m_uDeterioration = 0u;
			entry.m_Time = m_Now;
			m_vOrder.PushBack(entry);
		}

		// Always false - we're not a real file system, so we can't
		// handle any file operations.
		return false;
	}

	virtual Bool Open(const String& sAbsoluteFilename, File::Mode eMode, ScopedPtr<SyncFile>& rpFile) SEOUL_OVERRIDE { return false; }
	virtual Bool GetDirectoryListing(FilePath filePath, Vector<String>& rvResults, Bool bIncludeDirectoriesInResults = true, Bool bRecursive = true, const String& sFileExtension = String()) const SEOUL_OVERRIDE { return false; }
	virtual Bool GetDirectoryListing(const String& sAbsoluteDirectoryPath, Vector<String>& rvResults, Bool bIncludeDirectoriesInResults = true, Bool bRecursive = true, const String& sFileExtension = String()) const SEOUL_OVERRIDE { return false; }
	virtual Bool Rename(FilePath from, FilePath to) SEOUL_OVERRIDE { return false; }
	virtual Bool Rename(const String& sAbsoluteFrom, const String& sAbsoluteTo) SEOUL_OVERRIDE { return false; }
	virtual Bool SetReadOnlyBit(FilePath filePath, Bool bReadOnlyBit) SEOUL_OVERRIDE { return false; }
	virtual Bool SetReadOnlyBit(const String& sAbsoluteFilename, Bool bReadOnlyBit) SEOUL_OVERRIDE { return false; }

	/** Commit current state of training data to the specified database. */
	void CommitTrainerData()
	{
		// Minimum age before an entry is discarded.
		static const TimeInterval kMinAge = TimeInterval::FromDaysInt64(1);
		// Minimum deterioration before an entry is discarded.
		// An entry deteriorates every time it is maintained but
		// not scene.
		static const UInt32 kuMinDeterioration = 30u;

		Lookup t;
		Order o;
		{
			Lock lock(m_Mutex);
			t = m_tLookup;
			o = m_vOrder;
		}

		// Resolve to the output path.
		auto const sFileName(m_FilePath.GetAbsoluteFilenameInSource());

		// Existing - read in and merge if we have an existing database.
		{
			if (FileManager::Get()->Exists(sFileName))
			{
				String sBody;
				if (!FileManager::Get()->ReadAll(sFileName, sBody))
				{
					SEOUL_WARN("[Tracking]: failed reading existing log '%s'", sFileName.CStr());
					return;
				}

				AppPCTrainerFileSystem::Order vExisting;
				if (!Reflection::DeserializeFromString(sBody, &vExisting))
				{
					SEOUL_WARN("[Tracking]: failed deserializing existing log '%s'", sFileName.CStr());
					return;
				}

				auto const u = vExisting.GetSize();
				for (UInt32 i = 0u; i < u; ++i)
				{
					auto entry = vExisting[i];
					if (!t.HasValue(entry.m_FilePath))
					{
						// Discard if beyond the minimum age and
						// minimum deterioration. Otherwise,
						// keep, but increase deterioration.
						if ((m_Now - entry.m_Time) <= kMinAge ||
							entry.m_uDeterioration < kuMinDeterioration)
						{
							// Increase deterioration.
							++entry.m_uDeterioration;

							auto const uIndex = o.GetSize();
							o.PushBack(entry);
							SEOUL_VERIFY(t.Insert(entry.m_FilePath, uIndex).Second);
						}
					}
				}
			}
		}

		// New - write out the results.
		if (!Reflection::SaveObject(&o, sFileName))
		{
			SEOUL_WARN("[Tracking]: failed serialize of access log '%s'", sFileName.CStr());
		}
	}

private:
	SEOUL_DISABLE_COPY(AppPCTrainerFileSystem);

	FilePath const m_FilePath;
	WorldTime const m_Now;
	Lookup m_tLookup;
	Order m_vOrder;
	Mutex m_Mutex;
}; // class AppPCTrainerFileSystem

SEOUL_SPEC_TEMPLATE_TYPE(Vector<AppPCTrainerFileSystem::Entry, 15>)
SEOUL_BEGIN_TYPE(AppPCTrainerFileSystem::Entry)
	SEOUL_PROPERTY_N("Path", m_FilePath)
	SEOUL_PROPERTY_N("Deterioration", m_uDeterioration)
	SEOUL_PROPERTY_N("Time", m_Time)
SEOUL_END_TYPE()

Int AutomatedTestsExceptionFilter(
	DWORD uExceptionCode,
	LPEXCEPTION_POINTERS pExceptionInfo);

D3DDeviceEntry GetD3D11DeviceHeadlessEntry();
static RenderDevice* CreateD3DHeadlessDevice(Int32 iWidth, Int32 iHeight)
{
	D3DCommonDeviceSettings settings;
	settings.m_iPreferredViewportWidth = iWidth;
	settings.m_iPreferredViewportHeight = iHeight;

	auto const entry = GetD3D11DeviceHeadlessEntry();
	return entry.m_pCreateD3DDevice(settings);
}

#if SEOUL_WITH_FMOD
Sound::Manager* CreateFMODHeadlessSoundManager();
#endif

static CheckedPtr<PatchablePackageFileSystem> s_pPatchableConfigPackageFileSystem;
static CheckedPtr<PatchablePackageFileSystem> s_pPatchableContentPackageFileSystem;

/** Conditionally enabled downloadable filesystem for content. */
extern CheckedPtr<DownloadablePackageFileSystem> g_pDownloadableContentPackageFileSystem;
static Bool s_bEnableDownloadableContent = false;
static Bool s_bEnableUnrestrictedDiskAccess = false;
static CheckedPtr<AppPCTrainerFileSystem> s_pTrainerFileSystem;

static void OnInitializeFileSystems()
{
	// Free disk access.
	if (s_bEnableUnrestrictedDiskAccess)
	{
		FileManager::Get()->RegisterFileSystem<DiskFileSystem>();
	}

	// PC_ClientSettings.sar
	FileManager::Get()->RegisterFileSystem<PackageFileSystem>(
		Path::Combine(GamePaths::Get()->GetBaseDir(), "Data/PC_ClientSettings.sar"));

	// PC_Config.sar
	s_pPatchableConfigPackageFileSystem = FileManager::Get()->RegisterFileSystem<PatchablePackageFileSystem>(
		Path::Combine(GamePaths::Get()->GetBaseDir(), "Data/PC_Config.sar"), // read-only builtin
		Path::Combine(GamePaths::Get()->GetSaveDir(), "Data/PC_ConfigUpdate.sar")); // updateable path

	// If true, this automated test should use a downloadable content package.
	String const sServerBaseURL(Game::ClientSettings::GetServerBaseURL());
	if (s_bEnableDownloadableContent && !sServerBaseURL.IsEmpty())
	{
		// Configure downloader with default settings.
		DownloadablePackageFileSystemSettings settings;
		settings.m_sAbsolutePackageFilename = Path::Combine(GamePaths::Get()->GetSaveDir(), "Data/PC_Content.sar");
		settings.m_sInitialURL = String::Printf("%s/v1/auth/additional_clientgamedata", sServerBaseURL.CStr());
		g_pDownloadableContentPackageFileSystem = FileManager::Get()->RegisterFileSystem<DownloadablePackageFileSystem>(settings);
	}
	else
	{
		// PC_Content.sar
		FileManager::Get()->RegisterFileSystem<PackageFileSystem>(
			Path::Combine(GamePaths::Get()->GetBaseDir(), "Data/PC_Content.sar"));
	}

	// PC_BaseContent.sar
	FileManager::Get()->RegisterFileSystem<PackageFileSystem>(
		Path::Combine(GamePaths::Get()->GetBaseDir(), "Data/PC_BaseContent.sar"));

	// PC_ScriptsDebug.sar - normally needed by all automated testing.
	FileManager::Get()->RegisterFileSystem<PackageFileSystem>(
		Path::Combine(GamePaths::Get()->GetBaseDir(), "Data/PC_ScriptsDebug.sar"));

	// PC_ContentUpdate.sar
	s_pPatchableContentPackageFileSystem = FileManager::Get()->RegisterFileSystem<PatchablePackageFileSystem>(
		Path::Combine(GamePaths::Get()->GetBaseDir(), "Data/PC_ContentUpdate.sar"), // read-only builtin
		Path::Combine(GamePaths::Get()->GetSaveDir(), "Data/PC_ContentUpdate.sar")); // updateable path

	// Read-only, restricted file system to the content://Authored/Scripts/DevOnly folder.
	if (!s_bEnableUnrestrictedDiskAccess)
	{
		FilePath restrictedDirectoryPath = FilePath::CreateContentFilePath("Authored/Scripts/DevOnly");
		FileManager::Get()->RegisterFileSystem<RestrictedDiskFileSystem>(restrictedDirectoryPath, true);
	}

	// Trainer file system if defined. Trainer is used
	// to learn files needed during early game execution
	// (determined by the automation script we're run with).
	{
		FilePath filePath;
		if (!AppPCCommandLineArgs::GetTrainerFile().IsEmpty() &&
			DataStoreParser::StringAsFilePath(AppPCCommandLineArgs::GetTrainerFile(), filePath))
		{
			s_pTrainerFileSystem = FileManager::Get()->RegisterFileSystem<AppPCTrainerFileSystem>(filePath);
		}
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

static Int AppPCRunAutomatedTestsImplLevel2(NullPlatformEngine& engine, const String& sAutomationScriptFileName)
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
				if (Game::Automation::Get())
				{
					iAdditionalWarningCount = (Int)Game::Automation::Get()->GetAdditionalWarningCount();
				}
			}
		}
	}

	// Total up warning count to determine whether we should commit
	// training data.
#if SEOUL_LOGGING_ENABLED
	auto const iWarningCount = iAdditionalWarningCount + Logger::GetSingleton().GetWarningCount();
#else // !SEOUL_LOGGING_ENABLED
	auto const iWarningCount = iAdditionalWarningCount;
#endif // /!SEOUL_LOGGING_ENABLED

	// Update training if successful.
	if (s_pTrainerFileSystem.IsValid() && 0 == iWarningCount)
	{
		s_pTrainerFileSystem->CommitTrainerData();
	}

	return iAdditionalWarningCount;
}

static Int AppPCRunAutomatedTestsImplLevel1(NullPlatformEngine& engine, const String& sAutomationScriptFileName)
{
	__try
	{
		return AppPCRunAutomatedTestsImplLevel2(engine, sAutomationScriptFileName);
	}
	__except (AutomatedTestsExceptionFilter(GetExceptionCode(), GetExceptionInformation()))
	{
		return 1;
	}
}

static Int AppPCRunAutomatedTestsImplLevel0(const String& sAutomationScriptFileName, Bool bPersistent)
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

		// Check arguments now - unrestricted access to the disk file system.
		if (AppPCCommandLineArgs::GetFreeDiskAccess())
		{
			s_bEnableUnrestrictedDiskAccess = true;
		}

		settings.m_bDefaultGDPRAccepted = false;
		if (AppPCCommandLineArgs::GetAcceptGDPR())
		{
			settings.m_bDefaultGDPRAccepted = true;
		}

		// Use a D3D headless backend instead of the null device.
		if (AppPCCommandLineArgs::GetD3D11Headless())
		{
			auto const entry = GetD3D11DeviceHeadlessEntry();
			D3DCommonDeviceSettings d3dsettings;
			if (entry.m_pIsSupported(d3dsettings))
			{
				settings.m_CreateRenderDevice = CreateD3DHeadlessDevice;
			}
		}

		// Use an FMOD headless backend instead of the null device.
		if (AppPCCommandLineArgs::GetFMODHeadless())
		{
#if SEOUL_WITH_FMOD
			settings.m_CreateSoundManager = CreateFMODHeadlessSoundManager;
#else
			SEOUL_WARN("-fmod_headless passed but this build does not include FMOD.");
			iInnerResult = 1;
#endif
		}

		if (0 == iInnerResult)
		{
			NullPlatformEngine engine(settings);
			engine.Initialize();
			iInnerResult = AppPCRunAutomatedTestsImplLevel1(engine, sAutomationScriptFileName);
			engine.Shutdown();
		}
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

Int AppPCRunAutomatedTests(const String& sAutomationScriptFileName, Bool bEnableDownloadableContent, Bool bPersistent)
{
	__try
	{
		s_bEnableDownloadableContent = bEnableDownloadableContent;
		return AppPCRunAutomatedTestsImplLevel0(sAutomationScriptFileName, bPersistent);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		fprintf(stderr, "Unhandled x64 Exception (likely null pointer dereference or heap corruption)\n");
		return 1;
	}
}

#endif // /#if SEOUL_AUTO_TESTS

} // namespace Seoul
