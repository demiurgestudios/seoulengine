/**
 * \file ScriptEngineProcess.h
 * \brief Binder instance for exposing Seoul::Process into
 * a script VM.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCRIPT_ENGINE_PROCESS_H
#define SCRIPT_ENGINE_PROCESS_H

#include "Delegate.h"
#include "Logger.h"
#include "Prereqs.h"
namespace Seoul { class Process; }
namespace Seoul { namespace Script { class FunctionInterface; } }

namespace Seoul
{

/** Binder, wraps a Camera instance and exposes functionality to a script VM. */
class ScriptEngineProcess SEOUL_SEALED
{
public:
	SEOUL_DELEGATE_TARGET(ScriptEngineProcess);

	ScriptEngineProcess();
	void Construct(Script::FunctionInterface* pInterface);
	~ScriptEngineProcess();

	// Check if the process is still running - if this function returns false, then
	// the process was either not started or has completed executed.
	Bool CheckRunning();

	// Gets the return value from this process's previous execution.
	Int GetReturnValue() const;

	// Tell the process to exit immediately. SIGKILL processing.
	Bool Kill(Int iRequestedExitCode);

	// Update the logger channel used for standard output and error.
	// Default is the default logger channel.
	void SetStdErrChannel(LoggerChannel eChannel);
	void SetStdOutChannel(LoggerChannel eChannel);

	// Begin execution of the process.
	Bool Start();

	// If the process was started, wait until the process has exited. Otherwise, this
	// method is a nop.
	Int WaitUntilProcessIsNotRunning();

private:
	// Hooks for standard out and error.
	void HandleStandardError(Byte const*);
	void HandleStandardOutput(Byte const*);

	ScopedPtr<Process> m_pProcess;
	LoggerChannel m_eStdErrChannel;
	LoggerChannel m_eStdOutChannel;

	SEOUL_DISABLE_COPY(ScriptEngineProcess);
}; // class ScriptEngineProcess

} // namespace Seoul

#endif // include guard
