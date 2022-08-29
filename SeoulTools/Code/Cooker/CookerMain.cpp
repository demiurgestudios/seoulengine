/**
 * \file CookerMain.cpp
 * \brief Root entry point of the SeoulEngine cooking executable.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "BuildChangelistPublic.h"
#include "BuildVersion.h"
#include "CachingDiskFileSystem.h"
#include "CommandLineArgWrapper.h"
#include "CookDatabase.h"
#include "Cooker.h"
#include "CookerUnitTests.h"
#include "CookerUnitTestsLink.h"
#include "CookTasks.h"
#include "CrashManager.h"
#include "DiskFileSystem.h"
#include "FileManager.h"
#include "GamePaths.h"
#include "JobsManager.h"
#include "Logger.h"
#include "NullPlatformEngine.h"
#include "Platform.h"
#include "ReflectionCommandLineArgs.h"
#include "ReflectionDefine.h"
#include "ReflectionEnum.h"
#include "ReflectionScriptStub.h"
#include "SccPerforceClient.h"
#include "ScopedAction.h"
#include "SeoulTime.h"
#include "SeoulUtil.h"
#include "StringUtil.h"
#include "Thread.h"

namespace Seoul
{

// Storage.
static Byte s_aBuildChangelistStr[16];

/**
 * Root level command-line arguments - handled by reflection, can be
 * configured via the literal command-line, environment variables, or
 * a configuration file.
 */
class CookerCommandLineArgs SEOUL_SEALED
{
public:
	static Platform Platform;
	static String PackageCookConfig;
	static Bool DebugOnly;
	static Bool Local;
	static Bool ForceGenCdict;
	static Bool Force;
	static Int Srccl;
	static String OutFile;
	static Int P4Changelist;
	static String P4ClientWorkspace;
	static String P4Port;
	static String P4User;
	static UInt32 CookerVersion;
	static UInt32 DataVersion;
	static String BaseDir;
	static CommandLineArgWrapper<String> RunUnitTests;
	static Bool Verbose;

private:
	CookerCommandLineArgs() = delete; // Static class.
	SEOUL_DISABLE_COPY(CookerCommandLineArgs);
};

Platform CookerCommandLineArgs::Platform = keCurrentPlatform;
String CookerCommandLineArgs::PackageCookConfig{};
Bool CookerCommandLineArgs::DebugOnly{};
Bool CookerCommandLineArgs::Local{};
Bool CookerCommandLineArgs::ForceGenCdict{};
Bool CookerCommandLineArgs::Force{};
Int CookerCommandLineArgs::Srccl = -1;
String CookerCommandLineArgs::OutFile{};
Int CookerCommandLineArgs::P4Changelist = -1;
String CookerCommandLineArgs::P4ClientWorkspace{};
String CookerCommandLineArgs::P4Port{};
String CookerCommandLineArgs::P4User{};
UInt32 CookerCommandLineArgs::CookerVersion{};
UInt32 CookerCommandLineArgs::DataVersion{};
String CookerCommandLineArgs::BaseDir{};
CommandLineArgWrapper<String> CookerCommandLineArgs::RunUnitTests{};
Bool CookerCommandLineArgs::Verbose{};

SEOUL_BEGIN_TYPE(CookerCommandLineArgs, TypeFlags::kDisableNew | TypeFlags::kDisableCopy)
	// Disable engine command-line arguments in the cooker.
	SEOUL_ATTRIBUTE(DisableCommandLineArgs, "EngineCommandLineArgs")
	SEOUL_CMDLINE_PROPERTY(Platform, "platform")
		SEOUL_ATTRIBUTE(Description, "cooking platform target")
	SEOUL_CMDLINE_PROPERTY(PackageCookConfig, "package_file", "file")
		SEOUL_ATTRIBUTE(Description, ".cfg of .sar packages to cook")
	SEOUL_CMDLINE_PROPERTY(DebugOnly, "debug_only")
		SEOUL_ATTRIBUTE(Description, "only generate debug scripts, not ship")
	SEOUL_CMDLINE_PROPERTY(Local, "local")
		SEOUL_ATTRIBUTE(Description, "for non-CI full cooks, disables time-consuming features")
	SEOUL_CMDLINE_PROPERTY(ForceGenCdict, "force_gen_cdict")
		SEOUL_ATTRIBUTE(Description, "force regeneration of compression dictionaries")
	SEOUL_CMDLINE_PROPERTY(Srccl, "srccl", "changelist")
		SEOUL_ATTRIBUTE(Description, "specify CL that will be backed into .sar files. Required if not -local.")
	SEOUL_CMDLINE_PROPERTY(OutFile, "out_file", "file")
		SEOUL_ATTRIBUTE(Description, "target of single file cook")
		SEOUL_ATTRIBUTE(Remarks,
			"if -out_file is not specified, cook is a full "
			"incremental cook of all assets.")
	SEOUL_CMDLINE_PROPERTY(P4Changelist, "p4_change", "changelist")
		SEOUL_ATTRIBUTE(Description, "change # for p4 add/edit (must exist)")
		SEOUL_ATTRIBUTE(Remarks,
			"if P4 options are given, you are responsible for managing and submitting "
			"the changelist. Cooker only adds/edits/deletes files.")
	SEOUL_CMDLINE_PROPERTY(P4ClientWorkspace, "p4_client", "workspace")
		SEOUL_ATTRIBUTE(Description, "workspace name for p4 ops")
	SEOUL_CMDLINE_PROPERTY(P4Port, "p4_port", "server:port")
		SEOUL_ATTRIBUTE(Description, "port for p4 ops (e.g. perforce:1683)")
	SEOUL_CMDLINE_PROPERTY(P4User, "p4_user", "username")
		SEOUL_ATTRIBUTE(Description, "username for p4 ops")
	SEOUL_CMDLINE_PROPERTY(CookerVersion, "cooker_version", "version")
		SEOUL_ATTRIBUTE(Description, "cooker version check for single cooks")
	SEOUL_CMDLINE_PROPERTY(DataVersion, "data_version", "version")
		SEOUL_ATTRIBUTE(Description, "data version check for single cooks")
	SEOUL_CMDLINE_PROPERTY(BaseDir, "base_dir", "directory")
		SEOUL_ATTRIBUTE(Description, "override application base directory")
	SEOUL_CMDLINE_PROPERTY(RunUnitTests, "run_unit_tests", "test-options")
		SEOUL_ATTRIBUTE(Description, "run cooker unit tests")
	SEOUL_CMDLINE_PROPERTY(Verbose, "verbose")
		SEOUL_ATTRIBUTE(Description, "log verbose output")
SEOUL_END_TYPE()

// Use default core virtuals
const CoreVirtuals* g_pCoreVirtuals = &g_DefaultCoreVirtuals;

/** Get the Cooker's base directory - the folder that contains the Cooker executable. */
static String GetCookerBaseDirectoryPath()
{
#if SEOUL_PLATFORM_WINDOWS
	WCHAR aBuffer[MAX_PATH];
	SEOUL_VERIFY(0u != ::GetModuleFileNameW(nullptr, aBuffer, SEOUL_ARRAY_COUNT(aBuffer)));

	// Resolve the exact path to the editor binaries directory.
	auto const sCookerPath = Path::GetExactPathName(Path::GetDirectoryName(WCharTToUTF8(aBuffer)));
#else
#	error Define for this platform.
#endif

	return sCookerPath;
}

/** Get the App's base directory - we use the app's base directory for GamePaths. */
static String GetBaseDirectoryPath()
{
	// First, check if an explicit override was provided.
	if (!CookerCommandLineArgs::BaseDir.IsEmpty())
	{
		return Path::GetExactPathName(CookerCommandLineArgs::BaseDir);
	}

	// Otherwise, derive based on cooker location.
	auto const sCookerPath = GetCookerBaseDirectoryPath();

	// Now resolve the App directory using assumed directory structure.
	auto const sPath = Path::GetExactPathName(Path::Combine(
		Path::GetDirectoryName(sCookerPath, 5),
		Path::Combine(SEOUL_APP_ROOT_NAME, "Binaries", "PC", "Developer", "x64")));

	return sPath;
}

static void OnInitializeFileSystems()
{
	FileManager::Get()->RegisterFileSystem<DiskFileSystem>();
	// Register caches for on-disk config and content unless
	// this is a single file cook.
	if (CookerCommandLineArgs::OutFile.IsEmpty())
	{
		FileManager::Get()->RegisterFileSystem<CachingDiskFileSystem>(CookerCommandLineArgs::Platform, GameDirectory::kConfig);
		FileManager::Get()->RegisterFileSystem<CachingDiskFileSystem>(CookerCommandLineArgs::Platform, GameDirectory::kContent);
		FileManager::Get()->RegisterFileSystem<SourceCachingDiskFileSystem>(CookerCommandLineArgs::Platform);
	}
}

static Bool ProcessSettings(Cooking::CookerSettings& settings)
{
	settings.m_ePlatform = CookerCommandLineArgs::Platform;
	settings.m_sPackageCookConfig = CookerCommandLineArgs::PackageCookConfig;

	if (!settings.m_sPackageCookConfig.IsEmpty())
	{
		if (!FileManager::Get()->Exists(settings.m_sPackageCookConfig))
		{
			SEOUL_LOG_COOKING("-package_file argument is invalid, does not exist: \"%s\"", settings.m_sPackageCookConfig.CStr());
			return false;
		}

		settings.m_sPackageCookConfig = Path::GetExactPathName(settings.m_sPackageCookConfig);
	}

	settings.m_bDebugOnly = CookerCommandLineArgs::DebugOnly;
	settings.m_bLocal = CookerCommandLineArgs::Local;
	settings.m_bForceGenCdict = CookerCommandLineArgs::ForceGenCdict;

	if (!CookerCommandLineArgs::OutFile.IsEmpty())
	{
		auto const filePath = FilePath::CreateContentFilePath(CookerCommandLineArgs::OutFile);
		if (!filePath.IsValid())
		{
			SEOUL_LOG_COOKING("-out_file argument is invalid: \"%s\"", CookerCommandLineArgs::OutFile.CStr());
			return false;
		}

		if (!FileManager::Get()->ExistsInSource(filePath))
		{
			SEOUL_LOG_COOKING("-out_file argument is invalid, source does not exist: \"%s\"", filePath.CStr());
			return false;
		}

		settings.m_SingleCookPath = filePath;
	}

	Bool bSomeSCC = false;
	if (CookerCommandLineArgs::P4Changelist >= 0)
	{
		bSomeSCC = true;
		settings.m_P4Parameters.m_iP4Changelist = CookerCommandLineArgs::P4Changelist;
	}

	settings.m_P4Parameters.m_sP4ClientWorkspace = CookerCommandLineArgs::P4ClientWorkspace;
	bSomeSCC = bSomeSCC || !settings.m_P4Parameters.m_sP4ClientWorkspace.IsEmpty();
	settings.m_P4Parameters.m_sP4Port = CookerCommandLineArgs::P4Port;
	bSomeSCC = bSomeSCC || !settings.m_P4Parameters.m_sP4Port.IsEmpty();
	settings.m_P4Parameters.m_sP4User = CookerCommandLineArgs::P4User;
	bSomeSCC = bSomeSCC || !settings.m_P4Parameters.m_sP4User.IsEmpty();

	if (bSomeSCC && !settings.m_P4Parameters.IsValid())
	{
		SEOUL_LOG_COOKING("Some P4 arguments are incorrect.");
		return false;
	}

	// Apply version checks now if specified.
	{
		auto const uVersion = CookerCommandLineArgs::CookerVersion;
		if (0u != uVersion)
		{
			if (CookDatabase::GetCookerVersion() != uVersion)
			{
				SEOUL_LOG_COOKING("Cooker version mismatch: expected '%u' got '%u'. Likely, this means the Cooker needs to be compiled or synced.",
					uVersion,
					CookDatabase::GetCookerVersion());
				return false;
			}
		}
	}
	if (settings.m_SingleCookPath.IsValid())
	{
		auto const uVersion = CookerCommandLineArgs::DataVersion;
		if (0u != uVersion)
		{
			if (CookDatabase::GetDataVersion(settings.m_SingleCookPath.GetType()) != uVersion)
			{
				SEOUL_LOG_COOKING("Data version mismatch: expected '%u' got '%u'. Likely, this means the Cooker needs to be compiled or synced.",
					uVersion,
					CookDatabase::GetDataVersion(settings.m_SingleCookPath.GetType()));
				return false;
			}
		}
	}

	return true;
}

class ScopedContentDirectory SEOUL_SEALED
{
public:
	ScopedContentDirectory(Platform ePlatform)
		: m_ePlatform(ePlatform)
		, m_sOriginal(GamePaths::Get()->GetContentDir())
		, m_sTarget(GamePaths::Get()->GetContentDirForPlatform(ePlatform))
	{
		// Set to our desired target.
		GamePaths::Get()->SetContentDir(m_sTarget);
	}

	~ScopedContentDirectory()
	{
		// Sanity check and warn.
		if (GamePaths::Get()->GetContentDir() != m_sTarget)
		{
			SEOUL_LOG_COOKING("Content dir was changed from %s to %s during cooking, "
				"this likely caused cooking errors.",
				m_sTarget.CStr(),
				GamePaths::Get()->GetContentDir().CStr());
		}

		// Restore the content directory.
		GamePaths::Get()->SetContentDir(m_sOriginal);
	}

private:
	Platform const m_ePlatform;
	String const m_sOriginal;
	String const m_sTarget;

	SEOUL_DISABLE_COPY(ScopedContentDirectory);
}; // class ScopedContentDirectory

static Int RunCooker()
{
#if SEOUL_ENABLE_MEMORY_TOOLING
	// Output memory leak info to stdout instead of a file.
	MemoryManager::SetMemoryLeaksFilename(String());
#endif // /SEOUL_ENABLE_MEMORY_TOOLING

	// Need to set the environment before doing anything.
	auto const ePlatform = CookerCommandLineArgs::Platform;

	// Override platform directory for the scope of this call.
	ScopedContentDirectory scope(ePlatform);

	Cooking::CookerSettings settings;
	if (!ProcessSettings(settings))
	{
		return 1;
	}

	// Prior to cook, if interacting with source control, sync the Source/Generated* folder
	// to head, since this cooker "owns" it and wants an up-to-date version. This resolves
	// a few weird bugs/edge cases, such as:
	// - CL100 deletes a .cs file.
	// - CL101 makes another change.
	// - CL102 - build triggers on CL100, deletes the .lua corresponding to the .cs delete in CL100.
	// - CL103 - build triggers on CL101 - because the .lua was deleted in CL102, it is restored,
	//           and would otherwise appear in the sources gathered by the cooker at startup if we
	//           did not perform a sync against generated here.
	//
	// IMPORTANT: Must be done prior to construction of the Cooker to have the intended effect (affect
	// the source files list).
	//
	// TODO: Move this and the epilogue source control operations into the Cooker class?
	if (!settings.m_bLocal &&
		!settings.m_SingleCookPath.IsValid() &&
		settings.m_P4Parameters.IsValid())
	{
		// Get the target platform's Source/Generated*/ folder.
		auto const sGenerated(Path::Combine(
			GamePaths::Get()->GetSourceDir(),
			GamePaths::GetGeneratedContentDirName(CookerCommandLineArgs::Platform)) + Path::DirectorySeparatorChar() + "...");

		// Sync the generated folder.
		Scc::PerforceClient client(settings.m_P4Parameters);
		if (!client.Sync(&sGenerated, &sGenerated + 1))
		{
			SEOUL_LOG_COOKING("Failed syncing '%s' to head, prep for generated source output.", sGenerated.CStr());
			return 1;
		}
	}

	Cooking::Cooker cooker(settings);

	// Single file cook.
	if (settings.m_SingleCookPath.IsValid())
	{
		if (!cooker.CookSingle())
		{
			return 1;
		}
	}
	// All cook.
	else
	{
		if (!cooker.CookAllOutOfDateContent())
		{
			return 1;
		}
	}

	return 0;
}

static Int Main(Int iArgC, Byte** ppArgV)
{
	// Parse command-line argument.
	if (!Reflection::CommandLineArgs::Parse(ppArgV + 1, ppArgV + iArgC))
	{
		return 1;
	}

	// Apply the SRCCL if specified.
	if (CookerCommandLineArgs::Srccl > 0)
	{
		// Apply - we're dirty cheaters and modify the constant. Check first, build issue
		// if these were set explicitly by the builder environment.
		g_iBuildChangelist = CookerCommandLineArgs::Srccl;
		SNPRINTF(s_aBuildChangelistStr, sizeof(s_aBuildChangelistStr), "CL%d", CookerCommandLineArgs::Srccl);
		g_sBuildChangelistStr = s_aBuildChangelistStr;
	}

	// Special case handling for unit tests, just defer to the
	// harness.
	if (CookerCommandLineArgs::RunUnitTests.IsSet())
	{
		return CookerRunUnitTests(CookerCommandLineArgs::RunUnitTests);
	}

	// Immediately verify and apply srccl, once we've determined
	// this is not a unit test run.
	if (!CookerCommandLineArgs::Local && CookerCommandLineArgs::Srccl <= 0)
	{
		// -srccl is required if -local is not specified.
		SEOUL_LOG_COOKING("-srccl is required unless -local was passed.");
		return 1;
	}

	// Disable verbose memory leak detection to
	// avoid significant overhead in small block
	// allocations.
#if SEOUL_ENABLE_MEMORY_TOOLING
	// Runtime control of verbose memory leak detection. Useful
	// in tools and other scenarios where we want a developer
	// build (with logging, assertions, etc. enabled) but don't
	// want the overhead of verbose memory leak tracking.
	MemoryManager::SetVerboseMemoryLeakDetectionEnabled(false);
#endif // /#if SEOUL_ENABLE_MEMORY_TOOLING

	// Initialize SeoulTime
	SeoulTime::MarkGameStartTick();

	// Mark that we're now in the main function.
	auto const inMain(MakeScopedAction(&BeginMainFunction, &EndMainFunction));

	// Setup the main thread ID.
	SetMainThreadId(Thread::GetThisThreadId());

	// Configure booleans for headless commandline application.
	g_bHeadless = true;
	g_bShowMessageBoxesOnFailedAssertions = false;
	g_bEnableMessageBoxes = false;

	// Disable timestamping in the logger.
	Logger::GetSingleton().SetOutputTimestamps(false);
	Logger::GetSingleton().EnableChannelName(LoggerChannel::Cooking, false);

	// Disable all log channels.
	Logger::GetSingleton().EnableAllChannels(false);
	// Enable cooking channel initially.
	Logger::GetSingleton().EnableChannel(LoggerChannel::Cooking, true);

	// File system hookage.
	g_pInitializeFileSystemsCallback = OnInitializeFileSystems;

	// Start timing.
	auto const iStart = SeoulTime::GetGameTimeInTicks();

	NullCrashManager crashManager;
	NullPlatformEngineSettings settings;
	settings.m_iViewportWidth = 1;
	settings.m_iViewportHeight = 1;
	settings.m_sBaseDirectoryPath = GetBaseDirectoryPath();

	Int iReturn = 1;

	NullPlatformEngine engine(settings);
	engine.Initialize();

	// Enable a few more log channels that we care about during cooking.
	Logger::GetSingleton().EnableChannel(LoggerChannel::Assertion, true);
	Logger::GetSingleton().EnableChannel(LoggerChannel::Warning, true);

	// Perform the cook.
	iReturn = RunCooker();

	// Cleanup.
	engine.Shutdown();

	// End timing.
	auto const iEnd = SeoulTime::GetGameTimeInTicks();

	// Report overall results.
	if (CookerCommandLineArgs::OutFile.IsEmpty())
	{
		SEOUL_LOG_COOKING("%s-Cooking: %s (%.2f s)",
			EnumToString<Platform>(CookerCommandLineArgs::Platform),
			(0 == iReturn ? "OK" : "FAIL"),
			SeoulTime::ConvertTicksToSeconds(iEnd - iStart));
	}
	else
	{
		SEOUL_LOG_COOKING("%s-Cooking (%s): %s (%.2f s)",
			EnumToString<Platform>(CookerCommandLineArgs::Platform),
			CookerCommandLineArgs::OutFile.CStr(),
			(0 == iReturn ? "OK" : "FAIL"),
			SeoulTime::ConvertTicksToSeconds(iEnd - iStart));
	}

	// Done.
	return iReturn;
}

} // namespace Seoul

int main(int argc, char** argv)
{
	return Seoul::Main(argc, argv);
}
