/**
 * \file ScriptSceneStateLoadJob.h
 * \brief Handles the job of asynchronously loading scene data
 * into a scriptable scene instance.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCRIPT_SCENE_STATE_LOAD_JOB_H
#define SCRIPT_SCENE_STATE_LOAD_JOB_H

#include "ScriptSceneSettings.h"
#include "JobsJob.h"
namespace Seoul { namespace Scene { struct PrefabTemplate; } }
namespace Seoul { class ScriptSceneState; }

#if SEOUL_WITH_SCENE

namespace Seoul
{

class ScriptSceneStateLoadJob SEOUL_SEALED : public Jobs::Job
{
public:
	ScriptSceneStateLoadJob(const ScriptSceneSettings& settings);
	~ScriptSceneStateLoadJob();

	void AcquireNewStateDestroyOldState(ScopedPtr<ScriptSceneState>& rp)
	{
		m_pState.Swap(rp);
		m_pState.Reset();
	}

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(ScriptSceneStateLoadJob);

	virtual void InternalExecuteJob(Jobs::State& reNextState, ThreadId& rNextThreadId) SEOUL_OVERRIDE;

	Bool InternalCreateStateObjects();
	Bool InternalLoadVm();

	ScriptSceneSettings const m_Settings;
	ScopedPtr<ScriptSceneState> m_pState;

	SEOUL_DISABLE_COPY(ScriptSceneStateLoadJob);
}; // class ScriptSceneStateLoadJob

} // namespace Seoul

#endif // /#if SEOUL_WITH_SCENE

#endif // include guard
