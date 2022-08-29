/**
 * \file EditorSceneStateLoadJob.h
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef EDITOR_SCENE_STATE_LOAD_JOB_H
#define EDITOR_SCENE_STATE_LOAD_JOB_H

#include "EditorSceneSettings.h"
#include "JobsJob.h"
#include "ScenePrefab.h"
namespace Seoul { struct ScenePrefabTemplate; }
namespace Seoul { namespace EditorScene { class State; } }

#if SEOUL_WITH_SCENE

namespace Seoul::EditorScene
{

class StateLoadJob SEOUL_SEALED : public Jobs::Job
{
public:
	StateLoadJob(const Settings& settings);
	~StateLoadJob();

	void AcquireNewStateDestroyOldState(ScopedPtr<State>& rp)
	{
		m_pState.Swap(rp);
		m_pState.Reset();
	}

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(StateLoadJob);

	virtual void InternalExecuteJob(Jobs::State& reNextState, ThreadId& rNextThreadId) SEOUL_OVERRIDE;

	Bool InternalCreateStateObjects();

	Settings const m_Settings;
	ScopedPtr<State> m_pState;
	Scene::PrefabContentHandle m_hRootScenePrefab;

	SEOUL_DISABLE_COPY(StateLoadJob);
}; // class StateLoadJob

} // namespace Seoul::EditorScene

#endif // /#if SEOUL_WITH_SCENE

#endif // include guard
