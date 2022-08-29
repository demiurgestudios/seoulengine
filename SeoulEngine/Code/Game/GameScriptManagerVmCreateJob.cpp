/**
 * \file GameScriptManagerVmCreateJob.cpp
 * \brief Jobs::Manager Job used by Game::ScriptManager to create its Vm instance.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Logger.h"
#include "ReflectionDefine.h"
#include "GamePersistenceManager.h"
#include "GameScriptManagerSettings.h"
#include "GameScriptManagerVmCreateJob.h"
#include "ScriptFunctionInvoker.h"
#include "ScriptUIMovieClipInstance.h"
#include "ScriptVm.h"
#include "SeoulProfiler.h"
#include "UIManager.h"

namespace Seoul::Game
{

#if SEOUL_LOGGING_ENABLED
/**
 * Hook for print() output from Lua.
 */
void ScriptManagerLuaLog(Byte const* sTextLine)
{
	SEOUL_LOG_SCRIPT("%s", sTextLine);
}
#endif // /#if SEOUL_LOGGING_ENABLED

ScriptManagerVmCreateJob::ScriptManagerVmCreateJob(
	const ScriptManagerSettings& settings,
	Bool bReloadUI)
	: m_pVm()
	, m_Settings(settings)
	, m_bReloadUI(bReloadUI)
	, m_bHasProgress(false)
{
}

ScriptManagerVmCreateJob::~ScriptManagerVmCreateJob()
{
	WaitUntilJobIsNotRunning();

	// Terminate if we still own the VM.
	if (m_pVm.IsValid())
	{
		Script::FunctionInvoker invoker(*m_pVm, kFunctionSeoulDispose);
		if (invoker.IsValid())
		{
			(void)invoker.TryInvoke();
		}
	}
}

/** @return The total initialization progress of the Vm. */
void ScriptManagerVmCreateJob::GetProgress(Atomic32Type& rTotalSteps, Atomic32Type& rProgress) const
{
	if (m_bHasProgress)
	{
		m_pVm->InitGetProgress(rTotalSteps, rProgress);
	}
	else
	{
		rTotalSteps = 0;
		rProgress = 0;
	}
}

/** Trigger interruption of vm initialization. */
void ScriptManagerVmCreateJob::RaiseInterrupt()
{
	if (m_bHasProgress)
	{
		m_pVm->RaiseInterrupt();
	}
}

void ScriptManagerVmCreateJob::InternalExecuteJob(Jobs::State& reNextState, ThreadId& rNextThreadId)
{
	Script::VmSettings settings;
	settings.SetStandardBasePaths();
	settings.m_ErrorHandler = m_Settings.m_ScriptErrorHandler;
#if SEOUL_LOGGING_ENABLED
	settings.m_StandardOutput = SEOUL_BIND_DELEGATE(ScriptManagerLuaLog);
#endif // /#if SEOUL_LOGGING_ENABLED
	settings.m_sVmName = "GameScript";
	settings.m_pPreCollectionHook = ScriptUIMovieClipInstance::ResolveLuaJITPreCollectionHook();
#if SEOUL_ENABLE_DEBUGGER_CLIENT
	settings.m_bEnableDebuggerHooks = true;
#endif // /SEOUL_ENABLE_DEBUGGER_CLIENT
#if SEOUL_ENABLE_MEMORY_TOOLING
	// Enable memory profiling if leak detection is enabled.
	settings.m_bEnableMemoryProfiling = MemoryManager::GetVerboseMemoryLeakDetectionEnabled();
#endif // /SEOUL_ENABLE_MEMORY_TOOLING

	// There is increased pressure in developer builds in the script VM,
	// so we use more aggressive GC settings in that build as well.
#if !SEOUL_SHIP
	settings.m_fTargetIncrementalGcTimeInMilliseconds = 2.0f;
	settings.m_uMinGcStepSize = 256u;
#endif // /#if !SEOUL_SHIP

	m_pVm.Reset(SEOUL_NEW(MemoryBudgets::Scripting) Script::Vm(settings));

	// Must happen after construction and before RunScript.
	SeoulMemoryBarrier();
	m_bHasProgress = true;
	SeoulMemoryBarrier();

	SEOUL_PROF("TotalScriptInit");

	// TODO: I hate this locking, but the alternative is a lot of overhead in hot loading. In practice,
	// in a normal game, there should be no persistence contention, as there will only be one
	// VM alive at any given time.

	// Now run the main script - for the duration of this script, make access to Game::Persistence exclusive.
	Bool bSuccess = false;
	{
		SEOUL_PROF("ScriptInit");

#if SEOUL_WITH_GAME_PERSISTENCE
		PersistenceLock lock;
#endif // /#if SEOUL_WITH_GAME_PERSISTENCE
		bSuccess = m_pVm->RunScript(m_Settings.m_sMainScriptFileName);
	}

#if SEOUL_PROF_ENABLED
	{
		static const HString kScriptInit("ScriptInit");

		auto const iTime = SEOUL_PROF_TICKS(kScriptInit);
		auto const fMs = SeoulTime::ConvertTicksToMilliseconds(iTime);

		PlatformPrint::PrintStringFormatted(
			PlatformPrint::Type::kInfo,
			"Performance: Script Init: %.2f ms",
			fMs);
		SEOUL_PROF_LOG_CURRENT(kScriptInit);
	}
#endif // /SEOUL_PROF_ENABLED

	reNextState = (bSuccess ? Jobs::State::kComplete : Jobs::State::kError);
}

} // namespace Seoul::Game
