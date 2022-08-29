/**
 * \file SeoulProcess.cpp
 * \brief Class that represents an external application or process.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "SeoulProcess.h"
#include "SeoulProcessInternal.h"

namespace Seoul
{

/** @return The platform process id of the current process. */
Bool Process::GetThisProcessId(Int32& riProcessId)
{
	return ProcessDetail::GetThisProcessId(riProcessId);
}

Process::Process(
	const String& sProcessFilename,
	const ProcessArguments& vArguments,
	const OutputDelegate& standardOutput /* = OutputDelegate() */,
	const OutputDelegate& standardError /* = OutputDelegate() */,
	const InputDelegate& standardInput /* = InputDelegate() */)
	: Process(String(), sProcessFilename, vArguments, standardOutput, standardError, standardInput)
{
}

Process::Process(
	const String& sStartingDirectory,
	const String& sProcessFilename,
	const ProcessArguments& vArguments,
	const OutputDelegate& standardOutput /* = OutputDelegate() */,
	const OutputDelegate& standardError /* = OutputDelegate() */,
	const InputDelegate& standardInput /* = InputDelegate() */)
	: m_hHandle()
	, m_sStartingDirectory(sStartingDirectory)
	, m_sProcessFilename(sProcessFilename)
	, m_vArguments(vArguments)
	, m_StandardOutput(standardOutput)
	, m_StandardError(standardError)
	, m_StandardInput(standardInput)
	, m_iReturnValue(-1)
	, m_eState(kNotStarted)
{
}

Process::~Process()
{
	// Force the process to exit.
	(void)Kill(0);

	// Wait until completion.
	(void)WaitUntilProcessIsNotRunning();
	SEOUL_ASSERT(!CheckRunning());

	ProcessDetail::DestroyProcess(m_hHandle);
	SEOUL_ASSERT(!m_hHandle.IsValid());
}

/**
 * Attempt to start the process running.
 *
 * @return True if the process was started successfully, false otherwise. If this
 * method returns false, GetState() will return kError and the process will never
 * have been run.
 */
Bool Process::Start()
{
	// Only attempt to start if this Process's handle is
	// not already valid and if this Process' state is
	// kNotStarted.
	if (!m_hHandle.IsValid() && kNotStarted == m_eState)
	{
		m_eState = kRunning;

		if (!ProcessDetail::Start(
			m_sStartingDirectory,
			m_sProcessFilename,
			m_vArguments,
			m_StandardOutput,
			m_StandardError,
			m_StandardInput,
			m_hHandle))
		{
			m_eState = kErrorFailedToStart;
			return false;
		}
		else
		{
			return true;
		}
	}

	return false;
}

/**
 * Whether the process is currently executing. The process must have
 * been started and must not be finished to be "running".
 */
Bool Process::CheckRunning()
{
	if (kRunning == m_eState)
	{
		if (ProcessDetail::DoneRunning(m_iReturnValue, m_hHandle))
		{
			m_eState = kDoneRunning;
			return false;
		}
		else
		{
			return true;
		}
	}

	return false;
}

/**
 * Tell the process to exit immediately. SIGKILL processing.
 */
Bool Process::Kill(Int iRequestedExitCode)
{
	if (CheckRunning())
	{
		// Platform specific process kill.
		return ProcessDetail::KillProcess(m_eState, m_hHandle, iRequestedExitCode);
	}

	SEOUL_ASSERT(!CheckRunning());
	return true;
}

/**
 * If the process was started, wait until the process has exited. Otherwise, this
 * method is a nop.
 *
 * If iTimeoutInMilliseconds is < 0, then this method will wait forever for the
 * process to exit. Otherwise, it will wait until the timeout, and if that timeout
 * is reached, it will kill the process.
 *
 * @return the return value - this value will be -1 if the process was never started,
 * or if the process was killed due to a timeout.
 */
Int32 Process::WaitUntilProcessIsNotRunning(Int32 iTimeoutInMilliseconds /*= -1*/)
{
	if (CheckRunning())
	{
		// Wait for the process to complete.
		m_iReturnValue = ProcessDetail::WaitForProcess(m_eState, m_hHandle, iTimeoutInMilliseconds);
	}

	SEOUL_ASSERT(!CheckRunning());
	return m_iReturnValue;
}

} // namespace Seoul
