/**
 * \file AppLinuxUnitTests.cpp
 * \brief Defines the main function for a build run that will execute
 * unit tests.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "AppLinuxUnitTests.h"
#include "GamePaths.h"
#include "Logger.h"
#include "MapFileLinux.h"
#include "ReflectionUnitTestRunner.h"
#include "ScopedAction.h"
#include "ScopedPtr.h"
#include "SeoulUtil.h"
#include "StringUtil.h"
#include "Thread.h"
#include "UnitTests.h"
#include "UnitTesting.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS
Int AppLinuxRunUnitTests(const String& sOptionalTestName)
{
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

#if SEOUL_ENABLE_STACK_TRACES
	// We don't initialize Core in general for unit tests (although some tests may
	// create and tear down Core and/or Engine), so manually setup the dbghelp based
	// MapFile to provide better memory leak and/or assertion messaging.
	{
		MapFileLinux* pMapFile = SEOUL_NEW(MemoryBudgets::Debug) MapFileLinux;
		Core::SetMapFile(pMapFile);
		pMapFile->StartLoad();
	}
#endif // /SEOUL_ENABLE_STACK_TRACES

#if SEOUL_ENABLE_MEMORY_TOOLING
	// Output memory leak info to stdout instead of a file.
	MemoryManager::SetMemoryLeaksFilename(String());
#endif // /SEOUL_ENABLE_MEMORY_TOOLING

	// Cleanup temp files before and after unit testing.
	auto const scoped(MakeScopedAction(&DeleteAllTempFiles, &DeleteAllTempFiles));

	UnitTesting::RunBenchmarks(sOptionalTestName);
	return (UnitTesting::RunUnitTests(sOptionalTestName) ? 0 : 1);
}
#endif // /#if SEOUL_UNIT_TESTS

} // namespace Seoul
