/**
 * \file ScriptEngineProcess.cpp
 * \brief Binder instance for exposing Seoul::Process into
 * a script VM.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "GamePaths.h"
#include "Path.h"
#include "ReflectionDefine.h"
#include "ScriptEngineProcess.h"
#include "ScriptFunctionInterface.h"
#include "SeoulProcess.h"

namespace Seoul
{

SEOUL_BEGIN_TYPE(ScriptEngineProcess, TypeFlags::kDisableCopy)
	SEOUL_METHOD(Construct)
	SEOUL_METHOD(CheckRunning)
	SEOUL_METHOD(GetReturnValue)
	SEOUL_METHOD(Kill)
	SEOUL_METHOD(SetStdErrChannel)
	SEOUL_METHOD(SetStdOutChannel)
	SEOUL_METHOD(Start)
	SEOUL_METHOD(WaitUntilProcessIsNotRunning)
SEOUL_END_TYPE()

ScriptEngineProcess::ScriptEngineProcess()
	: m_pProcess()
	, m_eStdErrChannel(LoggerChannel::Default)
	, m_eStdOutChannel(LoggerChannel::Default)
{
}

/**
 * To be called immediately after ScriptEngineProcess().
 *
 * If all script infrastructure is working as expected,
 * this will be called automatically as part of native
 * user data construction.
 */
void ScriptEngineProcess::Construct(Script::FunctionInterface* pInterface)
{
	String sFilename;
	if (!pInterface->GetString(1u, sFilename))
	{
		pInterface->RaiseError(1, "expected string process filename.");
		return;
	}

	// Resolve the filename relative to the game's base directory.
	if (!Path::IsRooted(sFilename))
	{
		if (!Path::CombineAndSimplify(
			GamePaths::Get()->GetBaseDir(),
			sFilename,
			sFilename))
		{
			pInterface->RaiseError(1, "failed resolving relative filename.");
			return;
		}
	}

	Int const iArguments = (pInterface->GetArgumentCount() - 2);
	Process::ProcessArguments vsArguments((UInt32)Max(iArguments, 0));
	for (Int i = 0; i < iArguments; ++i)
	{
		if (!pInterface->GetString(i + 2, vsArguments[i]))
		{
			pInterface->RaiseError(i + 2, "expected string process argument.");
			return;
		}
	}

	m_pProcess.Reset(SEOUL_NEW(MemoryBudgets::Scripting) Process(
		sFilename,
		vsArguments,
		SEOUL_BIND_DELEGATE(&ScriptEngineProcess::HandleStandardOutput, this),
		SEOUL_BIND_DELEGATE(&ScriptEngineProcess::HandleStandardError, this)));
}

ScriptEngineProcess::~ScriptEngineProcess()
{
	m_pProcess.Reset();
}

/**
 * Check if the process is still running - if this function returns false, then
 * the process was either not started or has completed executed.
 */
Bool ScriptEngineProcess::CheckRunning()
{
	return m_pProcess->CheckRunning();
}

/** Gets the return value from this process's previous execution. */
Int ScriptEngineProcess::GetReturnValue() const
{
	if (m_pProcess.IsValid())
	{
		return m_pProcess->GetReturnValue();
	}

	return -1;
}

/** Tell the process to exit immediately. SIGKILL processing. */
Bool ScriptEngineProcess::Kill(Int iRequestedExitCode)
{
	if (m_pProcess.IsValid())
	{
		return m_pProcess->Kill(iRequestedExitCode);
	}

	return false;
}

void ScriptEngineProcess::SetStdErrChannel(LoggerChannel eChannel)
{
	m_eStdErrChannel = eChannel;
}

void ScriptEngineProcess::SetStdOutChannel(LoggerChannel eChannel)
{
	m_eStdOutChannel = eChannel;
}

/** Begin execution of the process. */
Bool ScriptEngineProcess::Start()
{
	if (m_pProcess.IsValid())
	{
		return m_pProcess->Start();
	}

	return false;
}

/**
 * If the process was started, wait until the process has exited. Otherwise, this
 * method is a nop.
 */
Int ScriptEngineProcess::WaitUntilProcessIsNotRunning()
{
	if (m_pProcess.IsValid())
	{
		return m_pProcess->WaitUntilProcessIsNotRunning();
	}

	return -1;
}

void ScriptEngineProcess::HandleStandardError(Byte const* s)
{
#if SEOUL_LOGGING_ENABLED
	LogMessage(m_eStdErrChannel, "%s", s);
#endif // /SEOUL_LOGGING_ENABLED
}

void ScriptEngineProcess::HandleStandardOutput(Byte const* s)
{
#if SEOUL_LOGGING_ENABLED
	LogMessage(m_eStdOutChannel, "%s", s);
#endif // /SEOUL_LOGGING_ENABLED
}

} // namespace Seoul
