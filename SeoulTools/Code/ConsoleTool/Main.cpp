/**
 * \file Main.cpp
 *
 * \brief ConsoleToolApp is a utility for wrapping Windows system proesses
 * (e.g. the main App game executable) so they can be run like command-line
 * proesses.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Atomic32.h"
#include "CommandLineArgWrapper.h"
#include "Directory.h"
#include "Mutex.h"
#include "Path.h"
#include "ReflectionCommandLineArgs.h"
#include "ReflectionDefine.h"
#include "ReflectionScriptStub.h"
#include "SeoulProcess.h"
#include "SeoulString.h"
#include "SeoulUtil.h"
#include "Vector.h"

namespace Seoul
{

/**
 * Root level command-line arguments - handled by reflection, can be
 * configured via the literal command-line, environment variables, or
 * a configuration file.
 */
class ConsoleToolCommandLineArgs SEOUL_SEALED
{
public:
	static CommandLineArgWrapper<String> Command;
	static Double TimeoutSecs;
	static Bool TestRunner;

private:
	ConsoleToolCommandLineArgs() = delete; // Static class.
	SEOUL_DISABLE_COPY(ConsoleToolCommandLineArgs);
};

CommandLineArgWrapper<String> ConsoleToolCommandLineArgs::Command{};
Double ConsoleToolCommandLineArgs::TimeoutSecs{};
Bool ConsoleToolCommandLineArgs::TestRunner{};

SEOUL_BEGIN_TYPE(ConsoleToolCommandLineArgs, TypeFlags::kDisableNew | TypeFlags::kDisableCopy)
	SEOUL_CMDLINE_PROPERTY(Command, 0, "command", true, false /* prefix */, true /* terminator */)
	SEOUL_CMDLINE_PROPERTY(TimeoutSecs, "timeout_secs")
		SEOUL_ATTRIBUTE(Description, "Timeout before running command will be terminated.")
	SEOUL_CMDLINE_PROPERTY(TestRunner, "test_runner")
		SEOUL_ATTRIBUTE(Description, "Some output from command is recognized and adds to error count.")
SEOUL_END_TYPE()

/** When test_runner mode is enabled, these are string matches that are considered errors. */
static Byte const* kasFailureMessageSubstrings[] =
{
	": FAIL",
	"Assertion: ",
	"Crash: ",
	"LocalizationWarning: ",
	"Warning: ",
	"Unhandled Win32 Exception",
	"Unhandled x64 Exception",
};

// Use default core virtuals
const CoreVirtuals* g_pCoreVirtuals = &g_DefaultCoreVirtuals;

namespace
{

/** Util for capturing and redirecting output and errors. */
class Util SEOUL_SEALED
{
public:
	SEOUL_DELEGATE_TARGET(Util);

	Util()
		: m_bTestRunner(ConsoleToolCommandLineArgs::TestRunner)
		, m_ErrorStream(stderr)
		, m_OutputStream(stdout)
	{
	}

	~Util()
	{
	}

	/** Hook for stderr. */
	void Error(Byte const* s)
	{
		DoOutput(m_ErrorStream, s);
	}

	/** Hook for stdout. */
	void Output(Byte const* s)
	{
		DoOutput(m_OutputStream, s);
	}

	/** Bind stderr. */
	Process::OutputDelegate StdErr()
	{
		return SEOUL_BIND_DELEGATE(&Util::Error, this);
	}

	/** Bind stdout. */
	Process::OutputDelegate StdOut()
	{
		return SEOUL_BIND_DELEGATE(&Util::Output, this);
	}

	/**
	 * Called on command completion when a test runner has completed. Allows
	 * the utility to log additional information and update the exit code
	 * as needed.
	 */
	void OnExit(Int32& riExitCode)
	{
		// No checking if not a test runner.
		if (!m_bTestRunner)
		{
			return;
		}

		// Append errors gathered by monitoring output.
		riExitCode += (Int32)m_AdditionalErrors;

		// If the result has an error exit code and we
		// have possible explanations, log those.
		Lock lock(m_Mutex);
		if (riExitCode > 0 && !m_vPossibleFailureExplanations.IsEmpty())
		{
			// Header string.
			fprintf(stderr, "---- BEGIN SUMMARY -------------\n");

			// Report all failure reasons.
			for (auto const& sReason : m_vPossibleFailureExplanations)
			{
				fputs(sReason.CStr(), stderr);
			}

			// Footer string.
			fprintf(stderr, "---- END SUMMARY ---------------\n");
			fflush(stderr);
		}
	}

private:
	SEOUL_DISABLE_COPY(Util);

	Bool const m_bTestRunner;
	FILE* const m_ErrorStream;
	FILE* const m_OutputStream;
	Atomic32 m_AdditionalErrors;
	Mutex m_Mutex;
	Vector<String, MemoryBudgets::Strings> m_vPossibleFailureExplanations;

	/** Shared functionality, applied potentially to output or error lines. */
	void Check(Byte const* sOutput)
	{
		// No checking if not a test runner.
		if (!m_bTestRunner)
		{
			return;
		}

		// If a test runner, check for the memory leaks tag.
		if (nullptr != strstr(sOutput, "---- Memory Leaks ----"))
		{
			// Increment the additional errors count so that, even if
			// test succeeded otherwise, memory leaks are considered
			// an error.
			++m_AdditionalErrors;

			// Add "memory leaks" as a possible failure reason.
			{
				Lock lock(m_Mutex);
				m_vPossibleFailureExplanations.PushBack(
					"- Memory leaks detected (see log for details).");
			}
		}

		// If a test runner, check for the "Connection abandoned." tag.
		if (nullptr != strstr(sOutput, "Connection abandoned."))
		{
			// Output from plink if a host association has
			// not been created, which will then result in
			// a silent failure (plink returns a success
			// code in this case).

			// Increment the additional errors count so that, even if
			// test succeeded otherwise, the abandoned connection is considered
			// an error.
			++m_AdditionalErrors;
			{
				Lock lock(m_Mutex);
				m_vPossibleFailureExplanations.PushBack(
					"- Apparent connection abandoned. Check that your ssh connection "
					"has been configured and authorized manually.");
			}
		}

		// Enumerate failure reason substrings and add the current
		// data if it contains a substring.
		for (auto const& s : kasFailureMessageSubstrings)
		{
			if (nullptr != strstr(sOutput, s))
			{
				Lock lock(m_Mutex);
				m_vPossibleFailureExplanations.PushBack(String::Printf("- %s", sOutput));
				break;
			}
		}
	}

	/** Common implementation for output handlers. */
	void DoOutput(FILE* pStream, Byte const* s)
	{
		Check(s);

		// Exclusion.
		Lock lock(m_Mutex);

		// Manual version of fputs to avoid implicit conversion of '\r\n'
		// to '\r\r\n'.
		while (*s)
		{
			if ('\r' != *s)
			{
				fputc(*s, pStream);
			}
			++s;
		}

		// Commit output.
		fflush(pStream);
	}
}; // class Util

} // namespace anonymous

} // namespace Seoul

int main(int iArgC, char** ppArgV)
{
	using namespace Seoul;
	using namespace Seoul::Reflection;

	// Parse command-line args.
	if (!CommandLineArgs::Parse(ppArgV + 1, ppArgV + iArgC))
	{
		return 1;
	}

	// Consume remaining args to the sub process itself.
	Process::ProcessArguments vArguments;
	// +2 to skip the arg itself (the command) as well as account for the skipped executable (+1 above).
	for (Int i = ConsoleToolCommandLineArgs::Command.GetCommandLineArgOffset() + 2; i < iArgC; ++i)
	{
		vArguments.PushBack(ppArgV[i]);
	}

	// Execute the process.
	Util util;
	Int32 iReturn = -1;
	{
		Process process(
			ConsoleToolCommandLineArgs::Command,
			vArguments,
			util.StdOut(),
			util.StdErr());
		if (!process.Start())
		{
			util.Error(String::Printf(R"(Failed starting process: "%s")", ConsoleToolCommandLineArgs::Command.Get().CStr()).CStr());
			return 1;
		}

		auto const iTimeoutInMilliseconds = (ConsoleToolCommandLineArgs::TimeoutSecs > 0.0
			? (Int32)(ConsoleToolCommandLineArgs::TimeoutSecs * (Double)WorldTime::kSecondsToMilliseconds)
			: -1);
		iReturn = process.WaitUntilProcessIsNotRunning(iTimeoutInMilliseconds);
	}

	// Done.
	if (iReturn < 0)
	{
		util.Error(String::Printf(R"(Warning: killed process "%s", reached timeout of %.2f seconds)",
			ConsoleToolCommandLineArgs::Command.Get().CStr(),
			ConsoleToolCommandLineArgs::TimeoutSecs).CStr());
		return 1;
	}
	else
	{
		util.OnExit(iReturn);
		return iReturn;
	}
}
