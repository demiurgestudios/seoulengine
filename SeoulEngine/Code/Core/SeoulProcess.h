/**
 * \file SeoulProcess.h
 * \brief Class that represents an external application or process.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SEOUL_PROCESS_H
#define SEOUL_PROCESS_H

#include "Delegate.h"
#include "FilePath.h"
#include "SeoulString.h"
#include "UnsafeHandle.h"
#include "Vector.h"

namespace Seoul
{

/**
 * Represents an operation system process. Processes
 * can be used for concurrent programming in cases where
 * you want to dispatch work to a utility application.
 */
class Process SEOUL_SEALED
{
public:
	/**
	 * Delegate that can be registered with a Process instance
	 * to provide standard input. Should return false (or zero output)
	 * after input has been closed.
	 */
	typedef Delegate<Bool(Byte*, UInt32, UInt32&)> InputDelegate;

	/**
	 * Delegate that can be registered with a Process instance
	 * to receive standard output or standard error streams from the process.
	 */
	typedef Delegate<void(Byte const*)> OutputDelegate;

	/** Container of arguments that should be passed to the process when it's started. */
	typedef Vector<String, MemoryBudgets::TBD> ProcessArguments;

	/**
	 * States of execution of this Process.
	 */
	enum State
	{
		/** Process is not running and has not been started. */
		kNotStarted,

		/** Process is running. */
		kRunning,

		/** Process was started and completed successfully. */
		kDoneRunning,

		/** Process was explicitly killed by user action. */
		kKilled,

		/** Starting the process failed - possibly due to an invalid command-line or invalid arguments. */
		kErrorFailedToStart,

		/** Process was killed due to reaching a timeout. */
		kErrorTimeout,

		/** An unexpected error occurred when terminating the process. */
		kErrorUnknown,
	};

	/** @return The platform process id of the current process. Not supported on all platforms. */
	static Bool GetThisProcessId(Int32& riProcessId);

	/**
	 * Construct this Process with an executable FilePath and a Vector<> of
	 * arguments to be passed to the executable.
	 *
	 * Arguments in vArguments should not be delimited - for example, do not
	 * surround an argument that contains spaces with double quotes. The Process class is designed
	 * to add additional characters as needed as required on the current platform so that each
	 * argument in vArguments will arrive verbatim as a command-line argument into the target
	 * process.
	 *
	 * \example
	 *   If vArguments[1] == "Hello World", the target process will receive "Hello World"
	 *   as a command-line argument, *not* Hello World unquoted.
	 */
	Process(
		const String& sProcessFilename,
		const ProcessArguments& vArguments,
		const OutputDelegate& standardOutput = OutputDelegate(),
		const OutputDelegate& standardError = OutputDelegate(),
		const InputDelegate& standardInput = InputDelegate());
	Process(
		const String& sStartingDirectory,
		const String& sProcessFilename,
		const ProcessArguments& vArguments,
		const OutputDelegate& standardOutput = OutputDelegate(),
		const OutputDelegate& standardError = OutputDelegate(),
		const InputDelegate& standardInput = InputDelegate());
	~Process();

	// Begin execution of the process.
	Bool Start();

	// Check if the process is still running - if this function returns false, then
	// the process was either not started or has completed executed.
	Bool CheckRunning();

	/**
	 * Gets the return value from this process's previous execution.
	 *
	 * Will return -1 if the process has not been started or has not yet
	 * finished running.
	 */
	Int GetReturnValue() const
	{
		return m_iReturnValue;
	}

	/**
	 * Returns the current execution state of this Process.
	 *
	 * \warning If this function returns kRunning, then the process can
	 * either be in the running state or may have completed running. Processes
	 * do not advance from the kRunning state to the kDoneRunning state until
	 * either CheckRunning() or WaitUntilProcessIsNotRunning() is called.
	 */
	State GetState() const
	{
		return m_eState;
	}

	// Tell the process to exit immediately. SIGKILL processing.
	Bool Kill(Int iRequestedExitCode);

	// If the process was started, wait until the process has exited. Otherwise, this
	// method is a nop.
	//
	// If iTimeoutInMilliseconds is < 0, then this method will wait forever for the
	// process to exit. Otherwise, it will wait until the timeout, and if that timeout
	// is reached, it will kill the process.
	//
	// Returns the return value - this value will be -1 if the process was never started,
	// or if the process was killed due to a timeout.
	Int32 WaitUntilProcessIsNotRunning(Int32 iTimeoutInMilliseconds = -1);

	/**
	 * @return The standard output delegate registered with this Process.
	 */
	const OutputDelegate& GetStandardOutput() const
	{
		return m_StandardOutput;
	}

	/**
	 * @return The standard error delegate registered with this Process.
	 */
	const OutputDelegate& GetStandardError() const
	{
		return m_StandardError;
	}

	/**
	 * @return The standard input delegate registered with this Process.
	 */
	const InputDelegate& GetStandardInput() const
	{
		return m_StandardInput;
	}

private:
	UnsafeHandle m_hHandle;
	String const m_sStartingDirectory;
	String const m_sProcessFilename;
	ProcessArguments const m_vArguments;
	OutputDelegate const m_StandardOutput;
	OutputDelegate const m_StandardError;
	InputDelegate const m_StandardInput;

	Atomic32Value<Int> m_iReturnValue;
	Atomic32Value<State> m_eState;

	SEOUL_DISABLE_COPY(Process);
}; // class Process

} // namespace Seoul

#endif // include guard
