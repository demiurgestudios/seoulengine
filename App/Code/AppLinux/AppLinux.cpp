/**
 * \file AppLinux.cpp
 * \brief Defines the entry point for the Linux game
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "AppLinuxAutomatedTests.h"
#include "AppLinuxCommandLineArgs.h"
#include "AppLinuxUnitTests.h"
#include "Engine.h"
#include "EngineCommandLineArgs.h"
#include "LocManager.h"
#include "ReflectionCommandLineArgs.h"
#include "ReflectionType.h"
#include "SeoulUtil.h"
#include <setjmp.h>
#include <signal.h>
#include <unistd.h>

namespace Seoul
{

void NopShowMessageBox(
	const String& sMessage,
	const String& sTitle,
	MessageBoxCallback onCompleteCallback,
	EMessageBoxButton eDefaultButton,
	const String& sButtonLabel1,
	const String& sButtonLabel2,
	const String& sButtonLabel3)
{
}

/**
 * Linux-specific core function table
 */
static const CoreVirtuals s_LinuxCoreVirtuals =
{
	&NopShowMessageBox,
	&LocManager::CoreLocalize,
	&Engine::CoreGetPlatformUUID,
	&Engine::CoreGetUptime,
};

/**
 * Linux-specific core function table pointer
 */
const CoreVirtuals* g_pCoreVirtuals = &s_LinuxCoreVirtuals;

extern String g_sLinuxMyExecutableAbsolutePath;

/** Main entry point for the application. */
int RealMain(int iArgC, char** ppArgV)
{
	// Parse command-line arguments.
	if (!Reflection::CommandLineArgs::Parse(ppArgV + 1, ppArgV + iArgC))
	{
		return 1;
	}

	// Before anything else, resolve and assign our absolute path for overall
	// engine path resolution.
	{
		Byte aPath[PATH_MAX];
		memset(aPath, 0, sizeof(aPath));

		ssize_t const zSize = readlink("/proc/self/exe", aPath, PATH_MAX);
		if (zSize == PATH_MAX)
		{
			fprintf(stderr, "Failed getting path to self with readlink, path is too long, greater than '%d' characters.\n", (int)PATH_MAX);
			return 1;
		}
		else if (zSize > 0)
		{
			g_sLinuxMyExecutableAbsolutePath.Assign(aPath);
		}
		else
		{
			fprintf(stderr, "Failed getting path to self with readlink.\n");
			return 1;
		}
	}

	// Enable as early as possible.
#if SEOUL_ENABLE_MEMORY_TOOLING
	if (Seoul::AppLinuxCommandLineArgs::GetVerboseMemoryTooling() ||
		Seoul::AppLinuxCommandLineArgs::GetRunUnitTests().IsSet())
	{
		Seoul::MemoryManager::SetVerboseMemoryLeakDetectionEnabled(true);
	}
#endif // /SEOUL_ENABLE_MEMORY_TOOLING

	// State.
	auto const bRunAutomatedTests = !Seoul::AppLinuxCommandLineArgs::GetRunAutomatedTest().IsEmpty();
	(void)bRunAutomatedTests;
	auto const bEnableDownloadableContent = Seoul::AppLinuxCommandLineArgs::GetDownloadablePackageFileSystemsEnabled();
	(void)bEnableDownloadableContent;
	auto const bRunUnitTests = Seoul::AppLinuxCommandLineArgs::GetRunUnitTests().IsSet();
	(void)bRunUnitTests;
	auto const bPersistent = Seoul::AppLinuxCommandLineArgs::GetPersistentTest();
	(void)bPersistent;

	// If unit testing is enabled, check if we're running to
	// execute unit tests or automated tests.
#if (SEOUL_AUTO_TESTS || SEOUL_UNIT_TESTS)
	{
		// One and one only.
		if (bRunAutomatedTests + bRunUnitTests > 1)
		{
			fprintf(stderr, "-run_unit_tests and -run_automated_tests are mutually exclusive.\n");
			return 1;
		}

#if SEOUL_AUTO_TESTS
		// Run automated tests.
		if (bRunAutomatedTests)
		{
			return Seoul::AppLinuxRunAutomatedTests(
				Seoul::AppLinuxCommandLineArgs::GetRunAutomatedTest(),
				bEnableDownloadableContent,
				bPersistent);
		}
#endif // /#if SEOUL_AUTO_TESTS

#if SEOUL_UNIT_TESTS
		// Run unit tests
		if (bRunUnitTests)
		{
			return Seoul::AppLinuxRunUnitTests(Seoul::AppLinuxCommandLineArgs::GetRunUnitTests());
		}
#endif // /#if SEOUL_UNIT_TESTS
	}
#endif // /#if (SEOUL_AUTO_TESTS || SEOUL_UNIT_TESTS)

	return 0;
}

static bool s_bHasError = false;
static sigjmp_buf s_HandlerJump;
#if SEOUL_ENABLE_STACK_TRACES
static size_t s_aCallStack[32] = { 0 };
static UInt32 s_uCallStackSize = 0u;
#endif // /#if SEOUL_ENABLE_STACK_TRACES

static void SignalHandler(int)
{
	s_bHasError = false;

#if SEOUL_ENABLE_STACK_TRACES
	s_uCallStackSize = Core::GetCurrentCallStack(0u, SEOUL_ARRAY_COUNT(s_aCallStack), s_aCallStack);
#endif // /#if SEOUL_ENABLE_STACK_TRACES

	siglongjmp(s_HandlerJump, 0);
}

static void TrapSignal(int sig)
{
	SEOUL_VERIFY(SIG_ERR != signal(sig, SignalHandler));
}

} // namespace Seoul

int main(int argc, char** argv)
{
	using namespace Seoul;

	TrapSignal(SIGABRT);
	TrapSignal(SIGBUS);
	TrapSignal(SIGFPE);
	TrapSignal(SIGILL);
	TrapSignal(SIGPIPE);
	TrapSignal(SIGSEGV);

	memset(&s_HandlerJump, 0, sizeof(s_HandlerJump));
	if (0 == sigsetjmp(s_HandlerJump, 1))
	{
		return RealMain(argc, argv);
	}
	else
	{
#if SEOUL_ENABLE_STACK_TRACES
		Byte aStackTrace[4096] = { 0 };
		Core::PrintStackTraceToBuffer(aStackTrace, sizeof(aStackTrace), s_aCallStack, s_uCallStackSize);
		aStackTrace[SEOUL_ARRAY_COUNT(aStackTrace) - 1] = '\0';

		fprintf(stderr, "Unhandled fatal signal at main.\n");
		fprintf(stderr, "%s\n", aStackTrace);
#endif // /#if SEOUL_ENABLE_STACK_TRACES

		return 1;
	}
}
