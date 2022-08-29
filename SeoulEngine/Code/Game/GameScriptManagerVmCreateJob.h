/**
 * \file GameScriptManagerVmCreateJob.h
 * \brief Jobs::Manager Job used by ScriptUI to create its Vm instance.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef GAME_SCRIPT_MANAGER_VM_CREATE_JOB_H
#define GAME_SCRIPT_MANAGER_VM_CREATE_JOB_H

#include "JobsJob.h"
#include "SharedPtr.h"
namespace Seoul { namespace Game { struct ScriptManagerSettings; } }
namespace Seoul { namespace Script { class Vm; } }

namespace Seoul::Game
{

static const HString kFunctionSeoulDispose("SeoulDispose");

class ScriptManagerVmCreateJob SEOUL_SEALED : public Jobs::Job
{
public:
	ScriptManagerVmCreateJob(
		const ScriptManagerSettings& settings,
		Bool bReloadUI);
	~ScriptManagerVmCreateJob();

	/** @return The total initialization progress of the Vm. */
	void GetProgress(Atomic32Type& rTotalSteps, Atomic32Type& rProgress) const;

	/** @return The settings used to create the VM. */
	const ScriptManagerSettings& GetSettings() const { return m_Settings; }

	/** Trigger interruption of vm initialization. */
	void RaiseInterrupt();

	// Acquire the VM from this Job. Only safe to call when the Job is not running.
	SharedPtr<Script::Vm> TakeOwnershipOfVm()
	{
		SEOUL_ASSERT(!IsJobRunning());
		auto pVm(m_pVm);
		m_pVm.Reset();
		return pVm;
	}

	/** This Job includes a UI reload. */
	Bool IsReloadUI() const { return m_bReloadUI; }

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(ScriptManagerVmCreateJob);

	virtual void InternalExecuteJob(Jobs::State& reNextState, ThreadId& rNextThreadId) SEOUL_OVERRIDE;

	SharedPtr<Script::Vm> m_pVm;
	ScriptManagerSettings const m_Settings;
	Bool const m_bReloadUI;
	Atomic32Value<Bool> m_bHasProgress;

	SEOUL_DISABLE_COPY(ScriptManagerVmCreateJob);
}; // class Game::ScriptManagerVmCreateJob

} // namespace Seoul::Game

#endif // include guard
