/**
 * \file GamePaths.cpp
 * \brief Global singleton reponsible for (on some platforms) discover
 * and tracking of various file paths, used for reading and writing
 * classes of files in a standard game application.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#define SEOUL_ENABLE_CTLMGR
#include "Platform.h"

#if SEOUL_PLATFORM_WINDOWS
#include <shlobj.h>
#elif SEOUL_PLATFORM_LINUX
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#endif

#include "Core.h"
#include "Directory.h"
#include "DiskFileSystem.h"
#include "FromString.h"
#include "GamePaths.h"
#include "GamePathsSettings.h"
#include "Logger.h"
#include "Path.h"
#include "ScopedAction.h"
#include "StringUtil.h"

namespace Seoul
{

#if SEOUL_PLATFORM_IOS
String IOSGetBaseDirectory();
String IOSGetUserDirectory();
#elif SEOUL_PLATFORM_LINUX
String g_sLinuxMyExecutableAbsolutePath;
#endif

/**
 * @return The tools path for the current execution environment.
 */
static Byte const* InternalStaticGetBaseToolsPathForCurrentEnvironment()
{
#if SEOUL_PLATFORM_WINDOWS
#if SEOUL_DEBUG
	// To support debug builds of tools, tools within the SeoulTools
	// folder just resolve to the process folder. Otherwise, we fall back
	// to the stock Developer folder.
	auto const sProcessPath(Path::GetProcessDirectory());
	if (sProcessPath.EndsWith(R"(SeoulTools\Binaries\PC\Debug\x64)"))
	{
		return "..\\SeoulTools\\Binaries\\PC\\Debug\\x64\\";
	}
	else
#endif
	{
		return "..\\SeoulTools\\Binaries\\PC\\Developer\\x64\\";
	}
#else
	return "SeoulTools\\Binaries\\";
#endif
}

// Per platform content dir relative names.
static Byte const* s_kaContentDirNames[(UInt32)Platform::SEOUL_PLATFORM_COUNT] =
{
	"Data\\ContentPC\\", // PC
	"Data/ContentIOS/", // IOS
	"Data/ContentAndroid/", // Android
	"Data/ContentAndroid/", // Linux
};

// Per platform content dir relative names.
static Byte const* s_kaGeneratedContentDirNames[(UInt32)Platform::SEOUL_PLATFORM_COUNT] =
{
	"GeneratedPC", // PC
	"GeneratedIOS", // IOS
	"GeneratedAndroid", // Android
	"GeneratedAndroid", // Linux
};

// Initialize static member variables
const char* GamePaths::s_sDefaultPath = DEFAULT_PATH;
const char* GamePaths::s_sBinaryDirName = "Binaries";
const char* GamePaths::s_sConfigDirName = "Data\\Config\\";
#if SEOUL_PLATFORM_WINDOWS
const char* GamePaths::s_sContentDirName = s_kaContentDirNames[(UInt32)Platform::kPC];
const char* GamePaths::s_sSaveDirName = SOEUL_APP_SAVE_COMPANY_DIR "\\" SEOUL_APP_SAVE_DIR "\\";
const char* GamePaths::s_sUserConfigJsonFileName = "unknown_game_config.json";
#elif SEOUL_PLATFORM_IOS
const char* GamePaths::s_sContentDirName = s_kaContentDirNames[(UInt32)Platform::kIOS];
const char* GamePaths::s_sSaveDirName = SOEUL_APP_SAVE_COMPANY_DIR "/" SEOUL_APP_SAVE_DIR "/";
const char* GamePaths::s_sUserConfigJsonFileName = "unknown_game_config.json";
#elif SEOUL_PLATFORM_ANDROID
const char* GamePaths::s_sContentDirName = s_kaContentDirNames[(UInt32)Platform::kAndroid];
const char* GamePaths::s_sSaveDirName = SOEUL_APP_SAVE_COMPANY_DIR "/" SEOUL_APP_SAVE_DIR "/";
const char* GamePaths::s_sUserConfigJsonFileName = "unknown_game_config.json";
#elif SEOUL_PLATFORM_LINUX
// Not a typo - Linux currently shares Android data, since we only use the
// Linux platform for automated testing.
const char* GamePaths::s_sContentDirName = s_kaContentDirNames[(UInt32)Platform::kLinux];
const char* GamePaths::s_sSaveDirName = SOEUL_APP_SAVE_COMPANY_DIR "/" SEOUL_APP_SAVE_DIR "/";
const char* GamePaths::s_sUserConfigJsonFileName = "unknown_game_config.json";
#else
#error "Define for this platform."
#endif

const char* GamePaths::s_sToolsDirBinName = InternalStaticGetBaseToolsPathForCurrentEnvironment();

const char* GamePaths::s_sSourceDirName = "Source\\";
const char* GamePaths::s_sLogDirName = "Data\\Log\\";
const char* GamePaths::s_sVideosDirName = "Data\\Videos\\";
const char* GamePaths::s_sStaticObjectDirName = "StaticObjects";

/** Canonical handling of base GamePaths. */
static String MakeCanonical(const String& s)
{
	auto sReturn(Path::GetExactPathName(s));
	if (!sReturn.EndsWith(Path::DirectorySeparatorChar()))
	{
		sReturn += Path::DirectorySeparatorChar();
	}
	return sReturn;
}

/**
 * Default constructor
 *
 * Sets all of the member paths to the default
 */
GamePaths::GamePaths()
	: m_bInitialized(false)
	, m_sExeDir(s_sDefaultPath)
	, m_sBaseDir(s_sDefaultPath)
	, m_sConfigDir(s_sDefaultPath)
	, m_sContentDir(s_sDefaultPath)
	, m_sSaveDir(s_sDefaultPath)
	, m_sSourceDir(s_sDefaultPath)
	, m_sToolsBinDir(s_sDefaultPath)
	, m_sUserDir(s_sDefaultPath)
	, m_sLogDir(s_sDefaultPath)
	, m_sVideosDir(s_sDefaultPath)
{
}

/**
 * Destructor
 */
GamePaths::~GamePaths()
{
}

/**
 * Must be called pre-Initialize() to have an effect - set the user config INI filename for the current game.
 */
void GamePaths::SetUserConfigJsonFileName(const char* sUserConfigJsonFileName)
{
	// Set to an HString in case sRelativeSaveDirPath is heap allocated. HString's string will be valid forever.
	HString userConfigJsonFileName(sUserConfigJsonFileName);
	s_sUserConfigJsonFileName = userConfigJsonFileName.CStr();

	// In non-ship builds, isolate by adding a _dev suffix.
#if !SEOUL_SHIP
	s_sUserConfigJsonFileName = HString(Path::GetFileNameWithoutExtension(s_sUserConfigJsonFileName) + "_dev.json").CStr();
#endif
}

/**
 * Must be called pre-Initialize() to have an effect - set the relative directory to the user's
 * save folder for the current game.
 */
void GamePaths::SetRelativeSaveDirPath(const char* sRelativeSaveDirPath)
{
	// Set to an HString in case sRelativeSaveDirPath is heap allocated. HString's string will be valid forever.
	HString relativeSaveDirPath(sRelativeSaveDirPath);
	s_sSaveDirName = relativeSaveDirPath.CStr();
}

/**
 * Static initialization function
 *
 * Instantiates GamePaths::Get() and calls InitializeInteral()
 *
 */
void GamePaths::Initialize(const GamePathsSettings& inSettings)
{
	GamePathsSettings settings(inSettings);
	if (settings.m_sBaseDirectoryPath.IsEmpty())
	{
		InternalGetBasePath(settings.m_sBaseDirectoryPath);
	}

	SEOUL_NEW(MemoryBudgets::TBD) GamePaths();
	GamePaths::Get()->InitializeInternal(settings);
}

/**
 * Static shutdown function
 *
 * Frees the global GamePaths::Get() pointer and calls shutdown internal
 */
void GamePaths::ShutDown()
{
	GamePaths::Get()->ShutDownInternal();
	SEOUL_DELETE GamePaths::Get();
}

/**
 * Acquire a default base path for the current platform - some
 * applications can override this.
 *
 * Typically the "base path" is the directory that contains the current
 * process.
 */
void GamePaths::InternalGetBasePath(String& rsProcessDir)
{
	// Base path - typically the directory containing the current
	// process, but can be alternatives on some platforms (e.g. Android).
#if SEOUL_PLATFORM_ANDROID
	rsProcessDir = DEFAULT_PATH;
#elif SEOUL_PLATFORM_WINDOWS
	rsProcessDir = Path::GetProcessDirectory();
#elif SEOUL_PLATFORM_IOS
	rsProcessDir = IOSGetBaseDirectory();
#elif SEOUL_PLATFORM_LINUX
	rsProcessDir = Path::GetDirectoryName(g_sLinuxMyExecutableAbsolutePath);
#else
#error "Define your platform's base path"
#endif
}

/**
 * Initialization function which sets the game paths
 *
 * Fills in the member data paths based on the exe launch directory and the statically-defined offsets
 *
 * @param[in] sPath exe path, used to determine the folder that the exe was launched from.
 */
void GamePaths::InitializeInternal(const GamePathsSettings& settings)
{
	const String& sPath = settings.m_sBaseDirectoryPath;

	if (!sPath.IsEmpty())
	{
		m_sExeDir = MakeCanonical(sPath);
	}

	// Set the base directory (the path up to the binaries directory)
	// Handle the case where there is more than one instance of "Binaries" in the path.
	UInt loc = m_sExeDir.FindLast(s_sBinaryDirName);
	if (loc != String::NPos)
	{
		SetBaseDir(m_sExeDir.Substring(0, loc));
	}
	else //try to find lowercase binaries
	{
		loc = m_sExeDir.ToLowerASCII().Find("binaries");
		if (loc != String::NPos)
		{
			SetBaseDir( m_sExeDir.Substring(0, loc) );
		}
		else
		{
			// if we can't find a binaries directory, use the exe directory one as the base
			SetBaseDir(m_sExeDir);
		}
	}

	// set the config directory (base dir & config dir)
	SetConfigDir(GetBaseDir() + s_sConfigDirName);

	// set the content directory (base dir & content dir)
	for (auto i = (Int32)Platform::SEOUL_PLATFORM_FIRST; i <= (Int32)Platform::SEOUL_PLATFORM_LAST; ++i)
	{
		SetContentDirForPlatform(GetBaseDir() + s_kaContentDirNames[i], (Platform)i);
	}
	SetContentDir(GetBaseDir() + s_sContentDirName);

	// set the source directory
	SetSourceDir(GetBaseDir() + s_sSourceDirName);

	// set the tools directory
	String sAbsolutePathToToolsBinDir;
	if (Path::CombineAndSimplify(GetBaseDir(), s_sToolsDirBinName, sAbsolutePathToToolsBinDir))
	{
		SetToolsBinDir(sAbsolutePathToToolsBinDir);
	}

#if SEOUL_PLATFORM_WINDOWS
	wchar_t szPath[MAX_PATH];
	SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA|CSIDL_FLAG_CREATE, nullptr, 0, szPath);
	SetUserDir(WCharTToUTF8(szPath) + "\\");
#elif SEOUL_PLATFORM_IOS
	String sUserDir = IOSGetUserDirectory();
	SetUserDir(sUserDir);
#elif SEOUL_PLATFORM_ANDROID
	String sUserDir = Path::Combine(GetBaseDir(), "UserData/");
	SetUserDir(sUserDir);
#elif SEOUL_PLATFORM_LINUX
	Byte const* sHomeDirectory = getenv("HOME");
	if (nullptr == sHomeDirectory)
	{
		auto p = getpwuid(getuid());
		if (nullptr != p)
		{
			sHomeDirectory = p->pw_dir;
		}
	}

	if (nullptr != sHomeDirectory)
	{
		SetUserDir(sHomeDirectory);
	}
#else
#error "Define for this platform."
#endif

	// Set the platform specific save content directory (user dir &
	// platform specific save content dir)
	SEOUL_VERIFY(Path::CombineAndSimplify(
		String(),
		GetUserDir() + s_sSaveDirName,
		m_sSaveDir));

	// set the log and videos directories
#if SEOUL_PLATFORM_IOS
	SetLogDir(GetUserDir() + s_sLogDirName);
	SetVideosDir(GetUserDir() + s_sVideosDirName);
#else
	SetLogDir(GetBaseDir() + s_sLogDirName);
	SetVideosDir(GetBaseDir() + s_sVideosDirName);
#endif

	// initialize global json file paths.
	m_ApplicationJsonFilePath = FilePath::CreateConfigFilePath("application.json");
	m_AudioJsonFilePath = FilePath::CreateConfigFilePath("audio.json");
	m_GuiJsonFilePath = FilePath::CreateConfigFilePath("gui.json");
	m_InputJsonFilePath = FilePath::CreateConfigFilePath("input.json");
	m_LogJsonFilePath = FilePath::CreateConfigFilePath("log.json");
	m_OnlineJsonFilePath = FilePath::CreateConfigFilePath("online.json");
	m_TrialJsonFilePath = FilePath::CreateConfigFilePath("trial.json");
	m_UserConfigJsonFilePath = FilePath::CreateSaveFilePath(s_sUserConfigJsonFileName);

	m_bInitialized = true;
}

/**
 * Shutdown function
 *
 */
void GamePaths::ShutDownInternal()
{
	m_bInitialized = false;
}

/** Utility, since some platforms shared content. */
Byte const* GamePaths::GetGeneratedContentDirName(Platform ePlatform)
{
	return s_kaGeneratedContentDirNames[(UInt32)ePlatform];
}

/**
 * Get function for the base directory
 * @return const reference to the base directory path
 */
const String& GamePaths::GetBaseDir() const
{
	return m_sBaseDir;
}

/**
 * Get function for the config directory
 * @return const reference to the config directory path
 */
const String& GamePaths::GetConfigDir() const
{
	return m_sConfigDir;
}

/**
 * Get function for the content directory
 * @return const reference to the content directory path
 */
const String& GamePaths::GetContentDir() const
{
	return m_sContentDir;
}

/**
 * Access method for the content directory for an explicit platform.
 */
const String& GamePaths::GetContentDirForPlatform(Platform ePlatform) const
{
	return m_asContentDirs[(UInt32)ePlatform];
}

/**
 * Get function for the platform specific save content directory
 * @return const reference to the save content directory path
 */
const String& GamePaths::GetSaveDir() const
{
	return m_sSaveDir;
}

/**
 * Get function for the source directory
 */
const String& GamePaths::GetSourceDir() const
{
	return m_sSourceDir;
}

/**
 * Get function for the tools binaries directory
 */
const String& GamePaths::GetToolsBinDir() const
{
	return m_sToolsBinDir;
}

/**
 * Get function for the user directory
 */
const String& GamePaths::GetUserDir() const
{
	return m_sUserDir;
}

/**
 * Get function for the log directory
 * @return const reference to the log directory path
 */
const String& GamePaths::GetLogDir() const
{
	return m_sLogDir;
}

/**
 * Get function for the videos directory
 * @return const reference to the videos directory path
 */
const String& GamePaths::GetVideosDir() const
{
	return m_sVideosDir;
}

/**
 * Set function for the base directory
 * @param[in] sNewBaseDir : The new base directory
 */
void GamePaths::SetBaseDir(const String& sNewBaseDir)
{
	m_sBaseDir = MakeCanonical(sNewBaseDir);
}

/**
 * Set function for the content directory
 * @param[in] sNewContentDir : The new content directory
 */
void GamePaths::SetContentDir(const String& sNewContentDir)
{
	m_sContentDir = MakeCanonical(sNewContentDir);
	m_asContentDirs[(UInt32)keCurrentPlatform] = MakeCanonical(sNewContentDir);
}

/**
 * Set function for the content directory for a particular platform.
 * @param[in] sNewContentDir : The new content directory
 * @param[in] ePlatform : Platform to update.
 */
void GamePaths::SetContentDirForPlatform(const String& sNewContentDir, Platform ePlatform)
{
	if (keCurrentPlatform == ePlatform)
	{
		SetContentDir(sNewContentDir);
	}
	else
	{
		m_asContentDirs[(UInt32)ePlatform] = MakeCanonical(sNewContentDir);
	}
}

/**
 * Set function for the save content directory
 * @param[in] NewContentDir : The new save content directory
 */
void GamePaths::SetSaveDir(const String& sNewSaveDir)
{
	m_sSaveDir = MakeCanonical(sNewSaveDir);
	m_UserConfigJsonFilePath = FilePath::CreateSaveFilePath(m_sSaveDir + s_sUserConfigJsonFileName);
}

/**
 * Set function for the source directory
 */
void GamePaths::SetSourceDir(const String& sNewSourceDir)
{
	m_sSourceDir = MakeCanonical(sNewSourceDir);
}

/**
 * Set function for the tools binaries directory
 */
void GamePaths::SetToolsBinDir(const String& sNewToolsBinDir)
{
	m_sToolsBinDir = MakeCanonical(sNewToolsBinDir);
}

/**
 * Set function for the user directory
 */
void GamePaths::SetUserDir(const String& sNewUserDir)
{
	m_sUserDir = MakeCanonical(sNewUserDir);
}

/**
 * Set function for the config directory
 * @param[in] NewConfigDir : The new config directory
 */
void GamePaths::SetConfigDir(const String& sNewConfigDir)
{
	m_sConfigDir = MakeCanonical(sNewConfigDir);
}

/**
 * Set function for the log directory
 * @param[in] NewLogDir : The new log directory
 */
void GamePaths::SetLogDir(const String& sNewLogDir)
{
	m_sLogDir = MakeCanonical(sNewLogDir);
}

/**
 * Set function for the videos directory
 */
void GamePaths::SetVideosDir(const String& sNewVideosDir)
{
	m_sVideosDir = MakeCanonical(sNewVideosDir);
}

} // namespace Seoul
