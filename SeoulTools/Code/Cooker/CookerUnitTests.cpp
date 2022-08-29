/**
 * \file CookerUnitTests.cpp
 * \brief Defines the main function for a build run that will execute
 * unit tests.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Core.h"
#include "Logger.h"
#include "Prereqs.h"
#include "ReflectionUnitTestRunner.h"
#include "ScopedAction.h"
#include "SeoulTime.h"
#include "SeoulUtil.h"
#include "Thread.h"
#include "UnitTesting.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS
Int CookerRunUnitTests(const String& sOptionalTestName)
{
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

	// Cleanup temp files before and after unit testing.
	auto const scoped(MakeScopedAction(&DeleteAllTempFiles, &DeleteAllTempFiles));

	UnitTesting::RunBenchmarks(sOptionalTestName);
	return (UnitTesting::RunUnitTests(sOptionalTestName) ? 0 : 1);
}
#endif // /#if SEOUL_UNIT_TESTS

} // namespace Seoul
