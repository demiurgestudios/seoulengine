/**
 * \file EditorSceneStateLoadJob.cpp
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Camera.h"
#include "EditorSceneState.h"
#include "EditorSceneStateLoadJob.h"

#include "Logger.h"
#include "ReflectionDefine.h"
#include "SceneObject.h"
#include "ScenePrefabManager.h"
#include "SceneSetTransformComponent.h"

#if SEOUL_WITH_SCENE

namespace Seoul::EditorScene
{

static const HString kEditor("Editor");

StateLoadJob::StateLoadJob(const Settings& settings)
	: m_Settings(settings)
	, m_pState(SEOUL_NEW(MemoryBudgets::Scene) State)
	, m_hRootScenePrefab(Scene::PrefabManager::Get()->GetPrefab(m_Settings.m_RootScenePrefabFilePath))
{
}

StateLoadJob::~StateLoadJob()
{
	WaitUntilJobIsNotRunning();
}

void StateLoadJob::InternalExecuteJob(Jobs::State& reNextState, ThreadId& rNextThreadId)
{
	if (m_hRootScenePrefab.IsLoading())
	{
		return;
	}

	FilePath const rootScenePrefabFilePath(m_Settings.m_RootScenePrefabFilePath);
	SharedPtr<Scene::Prefab> pScenePrefab(m_hRootScenePrefab.GetPtr());
	if (!pScenePrefab.IsValid())
	{
		SEOUL_WARN("%s: load failed.", rootScenePrefabFilePath.CStr());
		goto error;
	}

	if (!InternalCreateStateObjects())
	{
		goto error;
	}

	// Grab editor only data.
	{
		auto const& data = pScenePrefab->GetTemplate().m_Data;

		DataNode editorOnly;
		if (data.GetValueFromTable(data.GetRootNode(), kEditor, editorOnly))
		{
			if (!Reflection::DeserializeObject(
				rootScenePrefabFilePath,
				data,
				editorOnly,
				&m_pState->GetEditState()))
			{
				goto error;
			}
		}
	}

	if (!m_pState->AppendScenePrefab(
		rootScenePrefabFilePath,
		pScenePrefab->GetTemplate(),
		Matrix4D::Identity(),
		String()))
	{
		goto error;
	}

	reNextState = Jobs::State::kComplete;
	return;

error:
	{
		m_pState.Reset();
	}
	reNextState = Jobs::State::kError;
}

Bool StateLoadJob::InternalCreateStateObjects()
{
	// Clear objects.
	m_pState->m_vObjects.Clear();

	return true;
}

} // namespace Seoul::EditorScene

#endif // /#if SEOUL_WITH_SCENE
