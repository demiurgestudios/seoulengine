/**
 * \file SeoulAssert.cpp
 * \brief SeoulEngine rough equivalent to the standard <cassert>
 * or <assert.h> headers, specifically, the assert() functionality.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "BuildChangelistPublic.h"
#include "BuildVersion.h"
#include "Core.h"
#include "Logger.h"
#include "Platform.h"
#include "SeoulAssert.h"
#include "SeoulUtil.h"
#include "StringUtil.h"

#include <stdlib.h>

namespace Seoul
{

/**
 * Whether or not to show message boxes on failed assertions.
 *
 * This is generally set to true.  If running on an unattended system (say, an
 * autobuilder), this should be set to false so that message boxes do not pop
 * up while running unit tests.
 */
Bool g_bShowMessageBoxesOnFailedAssertions = true;

/**
 * Called upon a failed assertion.  Halts the program and emits a useful
 * error message.
 *
 * @param[in] sExpression The assertion that failed
 * @param[in] sMessage    The message string to display
 * @param[in] sFile       The name of the source file containing the failed assertion
 * @param[in] sFunction   The name of the function containing the failed assertion
 * @param[in] nLine       The line number of the failed assertion
 * @param[in] bLog        Whether or not to print the failed assertion to the log file
 *
 * @return This function does not return, unless we're running on Windows and
 *         the user elects to ignore the assertion failure.
 *
 * This function is only called by the SEOUL_ASSERT() and SEOUL_ASSERT_NO_LOG() macros
 * and should not be called directly.  It prints the information about the
 * failed assertion to standard out and aborts the program.  If bLog is
 * true, the message is also logged to the default log file.  If the target
 * platform is Windows, a message box is also displayed containing the message;
 * the user is also allowed to choose between attaching a debugger or exiting.
 */
void AssertionFailed(const char *sExpression, const char *sMessage, const char *sFile, const char *sFunction, int nLine, bool bLog)
{
	// Don't fail recursively -- if we're calling ourselves recursively, that's
	// badness 10000.  Just let the debugger handle it or hope for the best if
	// no debugger.
	static Bool s_bIsFailing = false;
	if (s_bIsFailing)
	{
		return;
	}
	s_bIsFailing = true;

	// Use static buffer here - we don't want to use the String class, which
	// uses dynamic memory, because we have no idea how we got here.  If we ran
	// out of memory, we want to make absolute sure that the assertion handler
	// runs.
	static Byte sBuffer[6144];

	SNPRINTF(sBuffer, SEOUL_ARRAY_COUNT(sBuffer), "Assertion Failed: %s\n\nFile: %s\nLine: %d\nFunction: %s\nSeoul Engine %s.v%s.%s\n\nExpression: %s\n", sMessage, sFile, nLine, sFunction, SEOUL_BUILD_CONFIG_STR, BUILD_VERSION_STR, g_ksBuildChangelistStrFixed, sExpression);

#if SEOUL_ENABLE_STACK_TRACES
	// Capture the callstack, if available
	STRNCAT(sBuffer, "\nStack trace:\n", SEOUL_ARRAY_COUNT(sBuffer));
	Core::GetStackTraceString(sBuffer + strlen(sBuffer), SEOUL_ARRAY_COUNT(sBuffer) - strlen(sBuffer));
#else
	STRNCAT(sBuffer, "\n<Stack trace unavailable>\n", SEOUL_ARRAY_COUNT(sBuffer));
#endif

	if (bLog)
	{
		SEOUL_LOG_ASSERTION("%s", sBuffer);
	}
	else
	{
		// If not logging, at least give it to the debugger
		PlatformPrint::PrintDebugString(PlatformPrint::Type::kFailure, sBuffer);
	}

#if SEOUL_PLATFORM_WINDOWS
	FPRINTF(stderr, "%s", sBuffer);

	// Don't show message box for unit tests
	if (g_bShowMessageBoxesOnFailedAssertions)
	{
		// Display a message box allowing the user to break into the debugger
		// or exit, if a debugger isn't already attached.
		UINT nMessageBoxType = MB_ICONERROR | MB_SETFOREGROUND | MB_TOPMOST;
		if (IsDebuggerPresent())
		{
			nMessageBoxType |= MB_OK;
		}
		else
		{
			strncat_s(sBuffer, SEOUL_ARRAY_COUNT(sBuffer), "\nDo you want to attach a debugger?  Press \"Yes\" to attach a debugger, or press \"No\" to exit.", _TRUNCATE);
			nMessageBoxType |= MB_YESNO;
		}

		int nButtonPressed = MessageBoxA(nullptr, sBuffer, "Assertion Failed", nMessageBoxType);

		// Check IsDebuggerPresent() again in case the debugger was detached
		// during the message box
		if (nButtonPressed == IDYES || IsDebuggerPresent())
		{
			// The SEOUL_ASSERT() macro takes care of calling DebugBreak() if we
			// return here, so that the debugger stops at the line of code that
			// failed the assert.  This is considerably more useful than if the
			// debugger were to stop here, which is what would happen if we
			// called DebugBreak() right now.
			return;
		}
		// else nButtonBressed == IDNO
	}

	if (g_bRunningUnitTests)
	{
		// For unit tests, we don't want to kill the whole test suite, but we
		// also don't want to throw an exception since we're not using the C++
		// exception mechanism in Seoul.  So, we'll trigger an intentional
		// access violation which will be caught by the UnitTest protector chain.
		*(UInt *)0 = 1;
	}
	else
	{
		// Not running unit tests, just end the program.  Do not run atexit()
		// handlers or destructors for global objects.  Do not flush buffers
		// for open file streams.  Do not pass go.  Do not collect $200.
		_set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
		abort();
	}
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
	SEOUL_DEBUG_BREAK();
#else
#error "Define assertion fail handling for this platform."
#endif
}

} // namespace Seoul
