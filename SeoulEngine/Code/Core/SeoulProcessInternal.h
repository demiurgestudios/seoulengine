/**
 * \file SeoulProcessInternal.h
 * \brief Internal header file used by SeoulProcess.cpp. Do not include
 * in other header files.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#ifndef SEOUL_PROCESS_H
#error "Internal header file, must only be included by SeoulProcess.cpp"
#endif

#include "Atomic32.h"
#include "Mutex.h"
#include "Platform.h"
#include "ScopedPtr.h"
#include "StringUtil.h"
#include "Thread.h"

namespace Seoul::ProcessDetail
{

#if SEOUL_PLATFORM_WINDOWS

// Globals for managing our process pool
// Job object to ensure child processes are
// terminated in-sync with the parent process.
static Mutex s_JobObjectMutex;
static HANDLE s_hJobObject = INVALID_HANDLE_VALUE;
static Atomic32 s_ProcessCount(0);

// Known Win32 script extensions.
static const String ksBatFile(".bat");
static const String ksCmdFile(".cmd");

// Command-line processing.
static const UniChar kBackslash = '\\';
static const UniChar kDoubleQuote = '"';
static const UniChar kTab = '\t';
static const UniChar kSpace = ' ';

/**
 * Used to quote the argv[0] parameter (executable path) if it contains white space, otherwise,
 * the value is left unchanged.
 */
inline String QuoteIfContainsSpaceOrTab(const String& sExecutablePath)
{
	for (auto i = sExecutablePath.Begin(); i != sExecutablePath.End(); ++i)
	{
		if (*i == kTab || *i == kSpace)
		{
			return "\"" + sExecutablePath + "\"";
		}
	}

	return sExecutablePath;
}

/**
 * Applies processing to the argument to fulfill command-line formatting
 * rules on Windows, see: http://msdn.microsoft.com/en-us/library/windows/desktop/17w5ykft%28v=vs.85%29.aspx
 */
inline String FormatArgumentForCommandLine(const String& sArgument)
{
	String sResult;

	Bool bRequiresQuotes = false;
	Int nPreceedingBackslashes = 0;
	for (auto i = sArgument.Begin(); i != sArgument.End(); ++i)
	{
		// If the current element is a double quote, we need to handle it specially.
		if (*i == kDoubleQuote)
		{
			// Little tricky at first glance - this implements the following formatting rules:
			// - Backslashes are interpreted literally, unless they immediately precede a double quotation mark.
			// - If an even number of backslashes is followed by a double quotation mark, one backslash is placed
			//   in the argv array for every pair of backslashes, and the double quotation mark is interpreted as
			//   a string delimiter.
			// - If an odd number of backslashes is followed by a double quotation mark, one backslash is
			//   placed in the argv array for every pair of backslashes, and the double quotation mark is
			//   "escaped" by the remaining backslash, causing a literal double quotation mark (") to be placed
			//   in argv.
			//
			// In our case, we're going the other way - we're generating a command-line that, when converted by
			// the above rules into an argv array, we want the exact input argument sArgument to be restored. So,
			// reading between the lines a bit, the only case we need to handle specially is when a (") is present.
			// Further, since we want to get the exact same result, we always want to double the number of
			// backslashes and then add one more for quotes present in the input. For example:
			//
			// sArgument = "Hello World" -> \"Hello World\" (zero backslashes * 2 + 1)
			// sArgument = \"Hello World"\ -> \\\"Hello World\"\ (1 backslash * 2 + 1, zero backslashes * 2 + 1)
			sResult.Append(kBackslash, nPreceedingBackslashes + 1);
			sResult.Append(kDoubleQuote);

			nPreceedingBackslashes = 0;
		}
		// Otherwise, just append the character and potentially count backslashes in preparation
		// for a quote.
		else
		{
			// If the character is a backslash, increment the backslash count.
			if (*i == kBackslash)
			{
				++nPreceedingBackslashes;
			}
			// Otherwise...
			else
			{
				// ...reset the backslash count.
				nPreceedingBackslashes = 0;

				// If this character is a tab or a space (the 2 argument delimiters according to docs), then we will need
				// to quote this entire argument for it to be interpreted correctly as a single command-line argument
				// instead of multiple.
				if (*i == kTab || *i == kSpace)
				{
					bRequiresQuotes = true;
				}
			}

			// Append this character.
			sResult.Append(*i);
		}
	}

	// If we need to quote the entire argument, do so now.
	if (bRequiresQuotes)
	{
		// Leading double quote.
		sResult = "\"" + sResult;

		// See the comment above for the bulk of why this happens - for the very last quote, we *want* it to be interpreted
		// as a string delimiter (instead of passed through literally), so we need to double the number of preceeding
		// backslashes before adding the quote (we don't add 1 in this case, because that would case the quote to be escaped
		// and passed through literally, instead of acting as a string delimiter).
		sResult.Append(kBackslash, nPreceedingBackslashes);
		sResult.Append(kDoubleQuote);
	}

	return sResult;
}

/** @return true if the given handle has a valid value (non "NULL"). */
inline Bool IsValidHandle(HANDLE hHandle)
{
	return (INVALID_HANDLE_VALUE != hHandle && 0 != hHandle);
}

/**
 * If rhHandle is a value other than INVALID_HANDLE_VALUE,
 * attempt to close the handle. Otherwise, this function is a nop.
 *
 * rhHandle will always be equal to INVALID_HANDLE_VALUE when
 * this function returns.
 */
inline void SafeCloseHandle(HANDLE& rhHandle)
{
	HANDLE hHandle = rhHandle;
	rhHandle = INVALID_HANDLE_VALUE;

	if (IsValidHandle(hHandle))
	{
		SEOUL_VERIFY(FALSE != ::CloseHandle(hHandle));
	}
}

/** Lazy construct and handle association with the global job object. */
static void SeoulInsideLockAttachProcessToGlobalJobObject(HANDLE hProcess)
{
	// Sanity check, we shouldn't be called if either of these are true.
	SEOUL_ASSERT(hProcess != INVALID_HANDLE_VALUE && hProcess != 0);

	// If we're the first, create the job object.
	if (1 == (++s_ProcessCount))
	{
		// Instantiate the job object.
		s_hJobObject = ::CreateJobObjectW(nullptr, L"SeoulProcessGlobalJobObject");
		SEOUL_ASSERT(INVALID_HANDLE_VALUE != s_hJobObject && 0 != s_hJobObject);

		// Configure it to terminate all child processes.
		JOBOBJECT_EXTENDED_LIMIT_INFORMATION limitInformation;
		memset(&limitInformation, 0, sizeof(limitInformation));

		limitInformation.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE | JOB_OBJECT_LIMIT_BREAKAWAY_OK;
		SEOUL_VERIFY(FALSE != ::SetInformationJobObject(
			s_hJobObject,
			JobObjectExtendedLimitInformation,
			&limitInformation,
			sizeof(limitInformation)));
	}

	// Associate.
	SEOUL_VERIFY(FALSE != ::AssignProcessToJobObject(s_hJobObject, hProcess));
}

/** Lazy destruct and handle dis-association from the global job object. */
static void SeoulInsideLockSafeDetachProcessFromGlobalJobObject(HANDLE hProcess)
{
	// Nop if no process.
	if (INVALID_HANDLE_VALUE == hProcess || 0 == hProcess)
	{
		return;
	}

	// If we're the last process, kill the Job Object.
	if (0 == (--s_ProcessCount))
	{
		SafeCloseHandle(s_hJobObject);
	}
}

/** Special handling for various built-in Win32 script types. */
static inline Bool IsScript(const String& sProcessFilename)
{
	String const sExtension(Path::GetExtension(sProcessFilename).ToLowerASCII());
	if (sExtension == ksBatFile ||
		sExtension == ksCmdFile)
	{
		return true;
	}

	return false;
}

/** Get the path to cmd.exe. */
static inline String GetCmdPath()
{
	WCHAR aBuffer[MAX_PATH] = { 0 };
	DWORD const uResult = ::GetEnvironmentVariableW(
		L"ComSpec",
		aBuffer,
		SEOUL_ARRAY_COUNT(aBuffer));

	// Success if uResult != 0 and uResult < SEOUL_ARRAY_COUNT(aBuffer).
	// A value greater than the buffer size means the value is too big,
	// and a value 0 usually means the variable is not found.
	if (uResult > 0u && uResult < SEOUL_ARRAY_COUNT(aBuffer))
	{
		// Make sure we nullptr terminate the buffer in all cases.
		aBuffer[uResult] = (WCHAR)'\0';
		return WCharTToUTF8(aBuffer);
	}
	// Otherwise, punt, and just return "cmd.exe".
	else
	{
		return String("cmd.exe");
	}
}

class AsyncProcessInput SEOUL_SEALED
{
public:
	SEOUL_DELEGATE_TARGET(AsyncProcessInput);

	AsyncProcessInput(
		const Process::InputDelegate& input,
		HANDLE& rhInputWrite,
		HANDLE& rhProcessInput)
		: m_Input(input)
		, m_rhInputWrite(rhInputWrite)
		, m_rhProcessInput(rhProcessInput)
		, m_pWorker()
	{
		// Sanity check, must not be created with an invalid delegate.
		SEOUL_ASSERT(m_Input.IsValid());

		// Start the worker.
		m_pWorker.Reset(SEOUL_NEW(MemoryBudgets::Threading) Thread(SEOUL_BIND_DELEGATE(&AsyncProcessInput::Write, this)));
		m_pWorker->Start("ProcessWriter");
	}

	~AsyncProcessInput()
	{
	}

private:
	SEOUL_DISABLE_COPY(AsyncProcessInput);

	Int Write(const Thread& thread)
	{
		static const UInt32 kBufferSize = 1024;

		UInt32 uBytesAvailable = 0u;

		Byte aBuffer[kBufferSize];

		while (
			m_Input(aBuffer, sizeof(aBuffer), uBytesAvailable) &&
			uBytesAvailable > 0u)
		{
			while (uBytesAvailable > 0u)
			{
				DWORD uBytesWritten = 0u;
				if (FALSE == ::WriteFile(m_rhInputWrite, aBuffer, uBytesAvailable, &uBytesWritten, nullptr))
				{
					goto done;
				}

				uBytesWritten = (DWORD)Min((UInt32)uBytesWritten, uBytesAvailable);
				uBytesAvailable -= uBytesWritten;
				if (uBytesAvailable > 0u && uBytesWritten > 0u)
				{
					memmove(aBuffer, aBuffer + uBytesWritten, uBytesAvailable);
				}
			}
		}

	done:
		// Cleanup the write handles.
		SafeCloseHandle(m_rhProcessInput);
		SafeCloseHandle(m_rhInputWrite);

		return 0;
	}

	Process::InputDelegate const m_Input;
	HANDLE& m_rhInputWrite;
	HANDLE& m_rhProcessInput;
	ScopedPtr<Thread> m_pWorker;
}; // class AsyncProcessInput

class AsyncProcessOutput SEOUL_SEALED
{
public:
	SEOUL_DELEGATE_TARGET(AsyncProcessOutput);

	AsyncProcessOutput(
		const Process::OutputDelegate& output,
		HANDLE& rhOutputRead)
		: m_Output(output)
		, m_rhOutputRead(rhOutputRead)
		, m_pWorker()
	{
		// Sanity check, must not be created with an invalid delegate.
		SEOUL_ASSERT(m_Output.IsValid());

		// Start the worker.
		m_pWorker.Reset(SEOUL_NEW(MemoryBudgets::Threading) Thread(SEOUL_BIND_DELEGATE(&AsyncProcessOutput::Read, this)));
		m_pWorker->Start("ProcessReader");
	}

	~AsyncProcessOutput()
	{
	}

private:
	SEOUL_DISABLE_COPY(AsyncProcessOutput);

	Int Read(const Thread& thread)
	{
		static const UInt32 kBufferSize = 1024;

		Byte aBuffer[kBufferSize];
		String sBuffer;

		while (true)
		{
			DWORD uBytesRead = 0u;
			if (FALSE == ::ReadFile(m_rhOutputRead, aBuffer, sizeof(aBuffer), &uBytesRead, nullptr))
			{
				break;
			}

			// Append and potentially dispatch.
			if (uBytesRead > 0u)
			{
				sBuffer.Append(aBuffer, (UInt32)uBytesRead);

				// Check for a newline.
				auto uNewlinePosition = sBuffer.Find('\n');
				while (String::NPos != uNewlinePosition)
				{
					String const sLine(sBuffer.Substring(0u, uNewlinePosition + 1u));
					sBuffer = sBuffer.Substring(uNewlinePosition + 1u);
					m_Output(sLine.CStr());

					uNewlinePosition = sBuffer.Find('\n');
				}
			}
		}

		// Dispatch any remaining output.
		if (!sBuffer.IsEmpty())
		{
			m_Output(sBuffer.CStr());
		}

		// Cleanup the read handle.
		SafeCloseHandle(m_rhOutputRead);

		return 0;
	}

	Process::OutputDelegate const m_Output;
	HANDLE& m_rhOutputRead;
	ScopedPtr<Thread> m_pWorker;
}; // class AsyncProcessOutput

struct ProcessData SEOUL_SEALED
{
	ProcessData(
		const String& sStartingDirectory,
		const String& sProcessFilename,
		const Process::ProcessArguments& vArguments,
		const Process::OutputDelegate& standardOutput,
		const Process::OutputDelegate& standardError,
		const Process::InputDelegate& standardInput)
		: m_sApplicationName()
		, m_sStartingDirectory(sStartingDirectory.WStr())
		, m_sCommandLine()
		, m_StandardOutput(standardOutput)
		, m_StandardError(standardError)
		, m_StandardInput(standardInput)
		, m_hStdOutputRead(INVALID_HANDLE_VALUE)
		, m_hStdErrorRead(INVALID_HANDLE_VALUE)
		, m_hStdInputWrite(INVALID_HANDLE_VALUE)
		, m_pStdOutputStream()
		, m_pStdErrorStream()
		, m_pStdInputStream()
	{
		memset(&m_ProcessInformation, 0, sizeof(m_ProcessInformation));
		m_ProcessInformation.hProcess = INVALID_HANDLE_VALUE;
		m_ProcessInformation.hThread = INVALID_HANDLE_VALUE;
		memset(&m_StartupInfo, 0, sizeof(m_StartupInfo));
		m_StartupInfo.cb = sizeof(m_StartupInfo);
		m_StartupInfo.hStdError = INVALID_HANDLE_VALUE;
		m_StartupInfo.hStdInput = INVALID_HANDLE_VALUE;
		m_StartupInfo.hStdOutput = INVALID_HANDLE_VALUE;

		Bool const bScript = IsScript(sProcessFilename);
		String const sApplicationName = (bScript
			? GetCmdPath()
			: sProcessFilename);
		m_sApplicationName = sApplicationName.WStr();

		// TODO: To be confirmed - CommandLineToArgvW() appears
		// to parse argv[0] (the application path) differently than any other argument -
		// it does not follow the backslash preceeding a double quote behavior, instead, it seems to
		// always treat the second quote as a terminator in all cases, even if it is preceeded by a backslash,
		// and it never interprets backslashes specially in argv[0] (they are always passed through literally).
		//
		// In practice, assuming this is always true should be fine, as there isn't a valid executable path that
		// would fall into conditions in which special handling of backslashes is required for the path to be
		// interpreted correctly.
		String sCommandLine = QuoteIfContainsSpaceOrTab(sApplicationName);

		// Append the process if a script.
		if (bScript)
		{
			sCommandLine.Append(' ');
			sCommandLine.Append("/C");
			sCommandLine.Append(' ');
			sCommandLine.Append(FormatArgumentForCommandLine(sProcessFilename));
		}

		UInt32 const nArguments = vArguments.GetSize();
		for (UInt32 i = 0u; i < nArguments; ++i)
		{
			// Format the argument and separate it from the previous argument
			// with a space.
			sCommandLine.Append(' ');
			sCommandLine.Append(FormatArgumentForCommandLine(vArguments[i]));
		}

		sCommandLine.WStr().Swap(m_sCommandLine);
	}

	~ProcessData()
	{
		// Synchronize this body - note that while technically
		// this exclusion should only be necessary inside
		// SeoulInsideLockSafeDetachProcessFromGlobalJobObject,
		// failing to synchronize the entire process creation/destruction
		// causes spurious "access denied" failures in multi-threading
		// scenarios when calling ::AssignProcessToJob.
		Lock lock(s_JobObjectMutex);

		// Detach from the JobObject.
		SeoulInsideLockSafeDetachProcessFromGlobalJobObject(m_ProcessInformation.hProcess);

		// Close the process and thread handles.
		SafeCloseHandle(m_ProcessInformation.hProcess);
		SafeCloseHandle(m_ProcessInformation.hThread);

		// Close standard input stream, it will cleanup its handles.
		m_pStdInputStream.Reset();
		// Sanity check - should have been cleaned up by the thread,
		// or never created.
		SEOUL_ASSERT(INVALID_HANDLE_VALUE == m_StartupInfo.hStdInput);
		SEOUL_ASSERT(INVALID_HANDLE_VALUE == m_hStdInputWrite);

		// Close standard error handles and stream if they are defined.
		// IMPORTANT: Order matters here - we must close the
		// handle into the process first, to unblock reader/writer
		// threads.
		SafeCloseHandle(m_StartupInfo.hStdError);
		SeoulMemoryBarrier();
		m_pStdErrorStream.Reset();
		// Sanity check - should have been cleaned up by the thread,
		// or never created.
		SEOUL_ASSERT(INVALID_HANDLE_VALUE == m_hStdErrorRead);

		// Close standard output handles and stream if they are defined.
		// IMPORTANT: Order matters here - we must close the
		// handle into the process first, to unblock reader/writer
		// threads.
		SafeCloseHandle(m_StartupInfo.hStdOutput);
		SeoulMemoryBarrier();
		m_pStdOutputStream.Reset();
		// Sanity check - should have been cleaned up by the thread,
		// or never created.
		SEOUL_ASSERT(INVALID_HANDLE_VALUE == m_hStdOutputRead);
	}

	/**
	 * Kick off async workers to process any defined
	 * input or output streams.
	 */
	void StartStreams()
	{
		// Early out if already valid.
		if (m_pStdOutputStream.IsValid() ||
			m_pStdErrorStream.IsValid() ||
			m_pStdInputStream.IsValid())
		{
			return;
		}

		// Create necessary streams.
		if (m_StandardOutput.IsValid())
		{
			m_pStdOutputStream.Reset(SEOUL_NEW(MemoryBudgets::Threading) AsyncProcessOutput(
				m_StandardOutput,
				m_hStdOutputRead));
		}
		if (m_StandardError.IsValid())
		{
			m_pStdErrorStream.Reset(SEOUL_NEW(MemoryBudgets::Threading) AsyncProcessOutput(
				m_StandardError,
				m_hStdErrorRead));
		}
		if (m_StandardInput.IsValid())
		{
			m_pStdInputStream.Reset(SEOUL_NEW(MemoryBudgets::Threading) AsyncProcessInput(
				m_StandardInput,
				m_hStdInputWrite,
				m_StartupInfo.hStdInput));
		}
	}

	WString m_sApplicationName;
	WString const m_sStartingDirectory;
	ScopedArray<wchar_t> m_sCommandLine;
	Process::OutputDelegate const m_StandardOutput;
	Process::OutputDelegate const m_StandardError;
	Process::InputDelegate const m_StandardInput;
	PROCESS_INFORMATION m_ProcessInformation;
	STARTUPINFOW m_StartupInfo;

	HANDLE m_hStdOutputRead;
	HANDLE m_hStdErrorRead;
	HANDLE m_hStdInputWrite;

	ScopedPtr<AsyncProcessOutput> m_pStdOutputStream;
	ScopedPtr<AsyncProcessOutput> m_pStdErrorStream;
	ScopedPtr<AsyncProcessInput> m_pStdInputStream;
}; // struct ProcessData

/**
 * Create a pipe suitable for redirecting standard output,
 * standard error, or standard input for a process - in and out pipes will
 * be assigned to rhRead and rhWrite if this function is
 * successful.
 *
 * @return True if the pipe was created successfully, false otherwise.
 */
inline BOOL CreateStdPipe(HANDLE& rhRead, HANDLE& rhWrite, Bool bInput = false)
{
	BOOL bResult = TRUE;

	SECURITY_ATTRIBUTES securityAttributes;
	memset(&securityAttributes, 0, sizeof(securityAttributes));

	securityAttributes.nLength = sizeof(securityAttributes);
	securityAttributes.bInheritHandle = TRUE;

	// Create the pipe.
	bResult = (FALSE != bResult && FALSE != ::CreatePipe(
		&rhRead,
		&rhWrite,
		&securityAttributes,
		0u));

	if (bInput)
	{
		// Prevent the write handle from inheriting from standard input.
		bResult = (FALSE != bResult && FALSE != ::SetHandleInformation(
			rhWrite,
			HANDLE_FLAG_INHERIT,
			FALSE));
	}
	else
	{
		// Prevent the read handle from inheriting from standard output.
		bResult = (FALSE != bResult && FALSE != ::SetHandleInformation(
			rhRead,
			HANDLE_FLAG_INHERIT,
			FALSE));
	}

	// If anything failed, close the handles before returning.
	if (FALSE == bResult)
	{
		SafeCloseHandle(rhRead);
		SafeCloseHandle(rhWrite);
	}

	return bResult;
}

inline Bool Start(
	const String& sStartingDirectory,
	const String& sProcessFilename,
	const Process::ProcessArguments& vArguments,
	const Process::OutputDelegate& standardOutput,
	const Process::OutputDelegate& standardError,
	const Process::InputDelegate& standardInput,
	UnsafeHandle& rhHandle)
{
	// Synchronize this body - note that while technically
	// this exclusion should only be necessary inside
	// SeoulInsideLockAttachProcessToGlobalJobObject,
	// failing to synchronize the entire process creation/destruction
	// causes spurious "access denied" failures in multi-threading
	// scenarios when calling ::AssignProcessToJob.
	Lock lock(s_JobObjectMutex);

	ProcessData* pData = SEOUL_NEW(MemoryBudgets::TBD) ProcessData(
		sStartingDirectory,
		sProcessFilename,
		vArguments,
		standardOutput,
		standardError,
		standardInput);

	Bool bResult = true;

	// If there's a standard output delegate, bind it.
	if (pData->m_StandardOutput)
	{
		bResult = (bResult && FALSE != CreateStdPipe(pData->m_hStdOutputRead, pData->m_StartupInfo.hStdOutput));
	}

	// If there's a standard error delegate, bind it.
	if (pData->m_StandardError)
	{
		bResult = (bResult && FALSE != CreateStdPipe(pData->m_hStdErrorRead, pData->m_StartupInfo.hStdError));
	}

	// If there's a standard input delegate, bind it.
	if (pData->m_StandardInput)
	{
		bResult = (bResult && FALSE != CreateStdPipe(pData->m_StartupInfo.hStdInput, pData->m_hStdInputWrite, true));
	}

	// If everything was successful and we have standard error or output handles, enable them.
	if (bResult &&
		(pData->m_StandardError.IsValid() ||
		pData->m_StandardOutput.IsValid() ||
		pData->m_StandardInput.IsValid()))
	{
		pData->m_StartupInfo.dwFlags |= STARTF_USESTDHANDLES;
	}

	// TODO: Allow some options of this call (No window?) to be specified
	// by the Seoul::Process interface.

	// Attempt to create the process.
	bResult = (bResult && FALSE != ::CreateProcessW(
		pData->m_sApplicationName,
		pData->m_sCommandLine.Get(),
		nullptr,
		nullptr,
		TRUE,
		// CREATE_BREAKAWAY_FROM_JOB creates the child in a different process
		// from the parent, which may be hooked into a debugger
		// Job: http://stackoverflow.com/questions/89588/assignprocesstojobobject-fails-with-access-denied-error-when-running-under-the
		CREATE_NO_WINDOW | CREATE_BREAKAWAY_FROM_JOB,
		nullptr,
		(0u == pData->m_sStartingDirectory.GetLengthInChars()) ? (LPCWSTR)nullptr : (LPCWSTR)pData->m_sStartingDirectory,
		&pData->m_StartupInfo,
		&pData->m_ProcessInformation));

	// If anything failed, destroy the data and return false.
	if (FALSE == bResult ||
		!IsValidHandle(pData->m_ProcessInformation.hProcess) ||
		!IsValidHandle(pData->m_ProcessInformation.hThread))
	{
		// Kick off reader/writer streams - need to do this for proper cleanup.
		pData->StartStreams();

		SafeDelete(pData);
		rhHandle.Reset();
		return false;
	}
	else
	{
		// Assign to the JobObject on success.
		SeoulInsideLockAttachProcessToGlobalJobObject(pData->m_ProcessInformation.hProcess);

		// Kick off reader/writer streams.
		pData->StartStreams();

		rhHandle = UnsafeHandle(pData);
		return true;
	}
}

inline void DestroyProcess(UnsafeHandle& rhHandle)
{
	ProcessData* pData = StaticCast<ProcessData*>(rhHandle);
	rhHandle.Reset();

	SafeDelete(pData);
}

inline Bool DoneRunning(Atomic32Value<Int>& riReturnValue, UnsafeHandle hHandle)
{
	ProcessData* pData = StaticCast<ProcessData*>(hHandle);

	auto const uResult = ::WaitForSingleObject(pData->m_ProcessInformation.hProcess, 0);

	if (WAIT_OBJECT_0 == uResult)
	{
		DWORD uExitCode = 0;
		if (FALSE != ::GetExitCodeProcess(pData->m_ProcessInformation.hProcess, &uExitCode))
		{
			riReturnValue = (Int)uExitCode;
		}
		else
		{
			riReturnValue = -1;
		}

		return true;
	}
	else
	{
		return false;
	}
}

inline Bool KillProcess(Atomic32Value<Process::State>& reState, UnsafeHandle hHandle, Int iRequestedExitCode)
{
	ProcessData* pData = StaticCast<ProcessData*>(hHandle);
	if (FALSE != ::TerminateProcess(pData->m_ProcessInformation.hProcess, (UINT)iRequestedExitCode))
	{
		reState = Process::kKilled;
		return true;
	}

	return false;
}

inline Int WaitForProcess(Atomic32Value<Process::State>& reState, UnsafeHandle hHandle, Int32 iTimeoutInMilliseconds)
{
	ProcessData* pData = StaticCast<ProcessData*>(hHandle);

	// Setup timeout and then wait on the process to exit.
	auto const uTimeout = (iTimeoutInMilliseconds < 0 ? INFINITE : (DWORD)iTimeoutInMilliseconds);
	auto const uResult = ::WaitForSingleObject(pData->m_ProcessInformation.hProcess, uTimeout);

	// Process wait timed out or otherwise failed - kill it and
	// set an appropriate error code.
	if (WAIT_OBJECT_0 != uResult)
	{
		(void)KillProcess(reState, hHandle, 1);
		SeoulMemoryBarrier();
		reState = (WAIT_TIMEOUT == uResult ? Process::kErrorTimeout : Process::kErrorUnknown);
	}
	// Otherwise, normal termination. Track exit code.
	else
	{
		reState = Process::kDoneRunning;

		DWORD uExitCode = 0;
		if (FALSE != ::GetExitCodeProcess(pData->m_ProcessInformation.hProcess, &uExitCode))
		{
			return (Int)uExitCode;
		}
	}

	// Fallback error code in unexpected conditions.
	return -1;
}

/** Windows process id. */
inline static Bool GetThisProcessId(Int32& riProcessId)
{
	riProcessId = (Int32)::GetCurrentProcessId();
	return true;
}

#else // /#if SEOUL_PLATFORM_WINDOWS

inline Bool Start(
	const String& /*sStartingDirectory*/,
	const String& /*sProcessFilename*/,
	const Process::ProcessArguments& /*vArguments*/,
	const Process::OutputDelegate& /*standardOutput*/,
	const Process::OutputDelegate& /*standardError*/,
	const Process::InputDelegate& /*standardInput*/,
	UnsafeHandle& rhHandle)
{
	// TODO:

	rhHandle.Reset();
	return false;
}

inline void DestroyProcess(UnsafeHandle& /*rhHandle*/)
{
	// TODO:

	// Nop
}

inline Bool DoneRunning(Atomic32Value<Int>& riReturnValue, UnsafeHandle /*hHandle*/)
{
	// TODO:

	riReturnValue = -1;
	return true;
}

inline Bool KillProcess(Atomic32Value<Process::State>& reState, UnsafeHandle /*hHandle*/, Int /*iRequestedExitCode*/)
{
	// TODO:

	reState = Process::kKilled;
	return true;
}

inline Int WaitForProcess(Atomic32Value<Process::State>& reState, UnsafeHandle /*hHandle*/, Int32 /*iTimeoutInMilliseconds*/)
{
	// TODO:

	reState = Process::kErrorUnknown;
	return -1;
}

inline static Bool GetThisProcessId(Int32& riProcessId)
{
	// TODO:
	return false;
}
#endif

} // namespace Seoul::ProcessDetail
