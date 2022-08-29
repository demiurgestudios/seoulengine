/**
 * \file AppAndroidUnitTests.cpp
 * \brief Defines the main function for a build run that will execute
 * unit tests.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "AndroidZipFileSystem.h"
#include "AndroidGlobals.h"
#include "DiskFileSystem.h"
#include "FileManager.h"
#include "GamePaths.h"
#include "Logger.h"
#include "PackageFileSystem.h"
#include "ReflectionUnitTestRunner.h"
#include "ScopedAction.h"
#include "ScopedPtr.h"
#include "SeoulUtil.h"
#include "StringUtil.h"
#include "Thread.h"
#include "UnitTests.h"
#include "UnitTesting.h"

#include <android_native_app_glue.h>

namespace Seoul
{

#if SEOUL_UNIT_TESTS
/** Caching. */
static android_app* s_pAndroidApp = nullptr;

/** For unit testing, hookup a callback that registers the Android file system. */
extern InitializeFileSystemsCallback g_pUnitTestsFileSystemsCallback;
extern String g_sUnitTestsBaseDirectoryPath;

static void UnitTestFileSystemHook()
{
	// Register a disk file system.
	FileManager::Get()->RegisterFileSystem<DiskFileSystem>();

	// Register the android file system.
	FileManager::Get()->RegisterFileSystem<AndroidZipFileSystem>(
		g_SourceDir.Data());

	// Give access to the config archive.
	FileManager::Get()->RegisterFileSystem<PackageFileSystem>(
		Path::Combine(GamePaths::Get()->GetBaseDir(), "Data/Android_Config.sar"));
}

Int AppAndroidRunUnitTests(
	android_app* pAndroidApp,
	const String& sBaseDirectoryPath,
	Byte const* sOptionalTestNameArgument)
{
	// Cache.
	s_pAndroidApp = pAndroidApp;

	// Setup base directory path.
	g_sUnitTestsBaseDirectoryPath = sBaseDirectoryPath;

	// Setup file system hook.
	g_pUnitTestsFileSystemsCallback = UnitTestFileSystemHook;

	// Initialize SeoulTime
	SeoulTime::MarkGameStartTick();

	// Mark that we're now in the main function.
	auto const inMain(MakeScopedAction(&BeginMainFunction, &EndMainFunction));

	GamePaths::SetUserConfigJsonFileName("game_config.json");
	GamePaths::SetRelativeSaveDirPath(SOEUL_APP_SAVE_COMPANY_DIR "\\UnitTests\\");

	// Set the main thread to the current thread.
	SetMainThreadId(Thread::GetThisThreadId());

	// Configure booleans for unit testing.
	g_bRunningUnitTests = true;
	g_bShowMessageBoxesOnFailedAssertions = false;
	g_bEnableMessageBoxes = false;

	// Disable timestamping in the logger.
	Logger::GetSingleton().SetOutputTimestamps(false);
	Logger::GetSingleton().EnableChannelName(LoggerChannel::UnitTest, false);

#if SEOUL_ENABLE_MEMORY_TOOLING
	// Output memory leak info to stdout instead of a file.
	MemoryManager::SetMemoryLeaksFilename(String());
#endif // /SEOUL_ENABLE_MEMORY_TOOLING

	// Cleanup temp files prior to unit testing.
	DeleteAllTempFiles();

	String const sOptionalTestName(sOptionalTestNameArgument);
	UnitTesting::RunBenchmarks(sOptionalTestName);
	return (UnitTesting::RunUnitTests(sOptionalTestName) ? 0 : 1);
}
#endif // /#if SEOUL_UNIT_TESTS

} // namespace Seoul
