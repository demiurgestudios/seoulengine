/**
 * \file ScriptSceneStateLoadJob.cpp
 * \brief Handles the job of asynchronously loading scene data
 * into a scriptable scene instance.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Camera.h"
#include "Logger.h"
#include "ReflectionDefine.h"
#include "SceneObject.h"
#include "ScenePrefabManager.h"
#include "SceneSetTransformComponent.h"
#include "ScriptEngineCamera.h"
#include "ScriptSceneState.h"
#include "ScriptSceneStateBinder.h"
#include "ScriptSceneStateLoadJob.h"
#include "ScriptVm.h"

#if SEOUL_WITH_SCENE

namespace Seoul
{

#if SEOUL_LOGGING_ENABLED
/**
 * Hook for printf and stderr output from Lua.
 */
void ScriptSceneLuaLog(Byte const* sTextLine)
{
	SEOUL_LOG_SCRIPT("%s", sTextLine);
}
#endif // /#if SEOUL_LOGGING_ENABLED

ScriptSceneStateLoadJob::ScriptSceneStateLoadJob(
	const ScriptSceneSettings& settings)
	: m_Settings(settings)
	, m_pState(SEOUL_NEW(MemoryBudgets::Scene) ScriptSceneState)
{
	m_pState->m_hRootScenePrefab = Scene::PrefabManager::Get()->GetPrefab(m_Settings.m_RootScenePrefabFilePath);
}

ScriptSceneStateLoadJob::~ScriptSceneStateLoadJob()
{
	WaitUntilJobIsNotRunning();
}

void ScriptSceneStateLoadJob::InternalExecuteJob(Jobs::State& reNextState, ThreadId& rNextThreadId)
{
	if (m_pState->m_hRootScenePrefab.IsLoading())
	{
		return;
	}

	FilePath const rootScenePrefabFilePath(m_Settings.m_RootScenePrefabFilePath);
	SharedPtr<Scene::Prefab> pScenePrefab(m_pState->m_hRootScenePrefab.GetPtr());
	if (!pScenePrefab.IsValid())
	{
		SEOUL_WARN("%s: load failed.", rootScenePrefabFilePath.CStr());
		goto error;
	}

	if (!InternalCreateStateObjects())
	{
		goto error;
	}

	if (!InternalLoadVm())
	{
		goto error;
	}

	if (!m_pState->AppendScenePrefab(
		rootScenePrefabFilePath,
		pScenePrefab->GetTemplate(),
		Matrix4D::Identity(),
		String()))
	{
		goto error;
	}

	// Final, send out the script OnLoad event.
	m_pState->CallScriptOnLoad();

	reNextState = Jobs::State::kComplete;
	return;

error:
	{
		m_pState.Reset();
	}
	reNextState = Jobs::State::kError;
}

Bool ScriptSceneStateLoadJob::InternalCreateStateObjects()
{
	// Clear object and cameras.
	m_pState->m_vObjects.Clear();
	m_pState->m_vCameras.Clear();

	// ScriptVM.
	{
		Script::VmSettings settings;
		settings.m_vBasePaths.PushBack(Path::GetDirectoryName(
			m_Settings.m_sScriptMainRelativeFilename));
		settings.m_ErrorHandler = m_Settings.m_ScriptErrorHandler;
#if SEOUL_LOGGING_ENABLED
		settings.m_StandardOutput = SEOUL_BIND_DELEGATE(ScriptSceneLuaLog);
#endif // /#if SEOUL_LOGGING_ENABLED
		m_pState->m_pVm.Reset(SEOUL_NEW(MemoryBudgets::Scripting) Script::Vm(settings));
	}

	return true;
}

Bool ScriptSceneStateLoadJob::InternalLoadVm()
{
	// Before running main, bind the global native scene state user data.
	// This allows scripts to access the scene during initialization,
	// which is important for allowing asynchronous operations
	// during VM creation.
	if (!m_pState->BindSelfIntoScriptVm())
	{
		return false;
	}

	// Now if defined, execute the main script.
	if (!m_Settings.m_sScriptMainRelativeFilename.IsEmpty())
	{
		if (!m_pState->m_pVm->RunScript(m_Settings.m_sScriptMainRelativeFilename))
		{
			return false;
		}
	}

	return true;
}

} // namespace Seoul

#endif // /#if SEOUL_WITH_SCENE
