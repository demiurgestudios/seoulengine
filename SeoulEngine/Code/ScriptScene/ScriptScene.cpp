/**
 * \file ScriptScene.cpp
 * \brief A Scene container (tree of ScenePrefabs) with
 * a Script VM.
 *
 * Creates a scriptable 3D scene.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ContentLoadManager.h"
#include "EventsManager.h"
#include "Renderer.h"
#include "SceneRenderer.h"
#include "SceneTicker.h"
#include "ScriptFunctionInvoker.h"
#include "ScriptScene.h"
#include "ScriptSceneState.h"
#include "ScriptSceneStateLoadJob.h"
#include "ScriptVm.h"

#if SEOUL_WITH_SCENE

namespace Seoul
{

static const HString kFunctionSeoulDispose("SeoulDispose");

static inline Scene::RendererConfig ToSceneRendererConfig(const ScriptSceneSettings& settings)
{
	Scene::RendererConfig ret;
	ret.m_FxEffectFilePath = settings.m_FxEffectFilePath;
	ret.m_MeshEffectFilePath = settings.m_MeshEffectFilePath;
	return ret;
}

ScriptScene::ScriptScene(const ScriptSceneSettings& settings)
	: m_Settings(settings)
	, m_pScriptSceneStateLoadJob()
	, m_pSceneRenderer(SEOUL_NEW(MemoryBudgets::Rendering) Scene::Renderer(ToSceneRendererConfig(settings)))
	, m_pSceneTicker(SEOUL_NEW(MemoryBudgets::Scene) Scene::Ticker)
	, m_pState()
#if SEOUL_HOT_LOADING
	, m_bPendingHotLoad(false)
#endif // /#if SEOUL_HOT_LOADING
{
	// Allocate and start the initial load job.
	m_pScriptSceneStateLoadJob.Reset(SEOUL_NEW(MemoryBudgets::TBD) ScriptSceneStateLoadJob(settings));
	m_pScriptSceneStateLoadJob->StartJob();

#if SEOUL_HOT_LOADING
	Events::Manager::Get()->RegisterCallback(
		Content::FileLoadCompleteEventId,
		SEOUL_BIND_DELEGATE(&ScriptScene::OnFileLoadComplete, this));
#endif // /#if SEOUL_HOT_LOADING
}

ScriptScene::~ScriptScene()
{
#if SEOUL_HOT_LOADING
	Events::Manager::Get()->UnregisterCallback(
		Content::FileLoadCompleteEventId,
		SEOUL_BIND_DELEGATE(&ScriptScene::OnFileLoadComplete, this));
#endif // /#if SEOUL_HOT_LOADING

	if (m_pScriptSceneStateLoadJob.IsValid())
	{
		m_pScriptSceneStateLoadJob->WaitUntilJobIsNotRunning();
		{
			ScopedPtr<ScriptSceneState> p;
			m_pScriptSceneStateLoadJob->AcquireNewStateDestroyOldState(p);
		}
		m_pScriptSceneStateLoadJob.Reset();
	}

	if (m_pState.IsValid())
	{
		// Handle hot loading unregistration.
#if SEOUL_HOT_LOADING
		m_pState->GetVm()->UnregisterFromHotLoading();
#endif // /#if SEOUL_HOT_LOADING

		// Dispose global resources prior to reset.
		{
			Script::FunctionInvoker invoker(*m_pState->GetVm(), kFunctionSeoulDispose);
			if (invoker.IsValid())
			{
				(void)invoker.TryInvoke();
			}
		}
	}
}

/** @return True if the root scene is still loading, false otherwise. */
Bool ScriptScene::IsLoading() const
{
	return m_pScriptSceneStateLoadJob.IsValid() && m_pScriptSceneStateLoadJob->IsJobRunning();
}

/** Render the current scene state. */
void ScriptScene::Render(
	RenderPass& rPass,
	RenderCommandStreamBuilder& rBuilder)
{
	// Nothing to do if we don't have a state.
	if (!InternalCheckState())
	{
		return;
	}

	m_pSceneRenderer->Render(
		m_pState->GetCameras(),
		m_pState->GetObjects(),
		rPass,
		rBuilder);
}

/** Echo an event to the script VM. */
void ScriptScene::SendEvent(
	const Reflection::MethodArguments& aArguments,
	Int iArgumentCount)
{
	// Nothing to do if we don't have a state.
	if (!m_pState.IsValid())
	{
		return;
	}

	m_pState->CallScriptSendEvent(aArguments, iArgumentCount);
}

/** Run per-frame simulation and updates. */
void ScriptScene::Tick(Float fDeltaTimeInSeconds)
{
	// Nothing to do if we don't have a state.
	if (!InternalCheckState())
	{
		return;
	}

	// Incremental garbage collection.
	if (m_pState->GetVm().IsValid())
	{
		m_pState->GetVm()->StepGarbageCollector();
	}

	// Process the append queue.
	{
		m_pState->ProcessAddPrefabQueue();
	}

	// Process tickers.
	{
		m_pState->ProcessTickers(fDeltaTimeInSeconds);
	}

	// Step physics.
	{
		m_pState->StepPhysics(fDeltaTimeInSeconds);
	}

	// Tick native scene.
	{
		m_pSceneTicker->Tick(*m_pState, m_pState->GetObjects(), fDeltaTimeInSeconds);
	}

	// Pass update to script.
	{
		m_pState->CallScriptTick(fDeltaTimeInSeconds);
	}
}

#if SEOUL_HOT_LOADING
void ScriptScene::OnFileLoadComplete(FilePath filePath)
{
	if (filePath == m_Settings.m_RootScenePrefabFilePath)
	{
		if (!m_pScriptSceneStateLoadJob.IsValid())
		{
			if (m_Settings.m_CustomHotLoadHandler)
			{
				m_Settings.m_CustomHotLoadHandler();
			}
			else
			{
				m_bPendingHotLoad = true;
			}
		}
	}
	// If we have a valid state, check if filePath matches
	// a dynamically added prefab.
	else if (m_pState.IsValid() && !m_pScriptSceneStateLoadJob.IsValid())
	{
		if (m_pState->GetPrefabAddCache().HasValue(filePath))
		{
			if (m_Settings.m_CustomHotLoadHandler)
			{
				m_Settings.m_CustomHotLoadHandler();
			}
			else
			{
				m_bPendingHotLoad = true;
			}
		}
	}
}
#endif // /#if SEOUL_HOT_LOADING

/**
 * Checks the current state pimpl and potentially hot loads
 * or refreshes it.
 *
 * A true return valid means the state is ready to access,
 * false implies m_pState.IsValid() is false and no operations
 * against m_pState are possible.
 */
Bool ScriptScene::InternalCheckState()
{
	if (m_pScriptSceneStateLoadJob.IsValid())
	{
		if (m_pScriptSceneStateLoadJob->IsJobRunning())
		{
			return m_pState.IsValid();
		}

		if (m_pState.IsValid())
		{
			// Handle hot loading unregistration.
#if SEOUL_HOT_LOADING
			m_pState->GetVm()->UnregisterFromHotLoading();
#endif // /#if SEOUL_HOT_LOADING

			// Dispose global resources prior to reset.
			{
				Script::FunctionInvoker invoker(*m_pState->GetVm(), kFunctionSeoulDispose);
				if (invoker.IsValid())
				{
					(void)invoker.TryInvoke();
				}
			}
		}

		m_pScriptSceneStateLoadJob->AcquireNewStateDestroyOldState(m_pState);

		// Handle hot loading registration.
#if SEOUL_HOT_LOADING
		if (m_pState.IsValid())
		{
			m_pState->GetVm()->RegisterForHotLoading();
		}
#endif // /#if SEOUL_HOT_LOADING

		m_pScriptSceneStateLoadJob.Reset();
	}

#if SEOUL_HOT_LOADING
	// Set a pending hot load if the Vm is out of date.
	if (m_pState.IsValid() && m_pState->GetVm()->IsOutOfDate())
	{
		if (m_Settings.m_CustomHotLoadHandler)
		{
			m_Settings.m_CustomHotLoadHandler();
		}
		else
		{
			m_bPendingHotLoad = true;
		}
	}

	if (m_bPendingHotLoad)
	{
		// If we have an existing Vm, dispose before hot loading a new Vm.
		if (m_pState.IsValid())
		{
			Script::FunctionInvoker invoker(*m_pState->GetVm(), kFunctionSeoulDispose);
			if (invoker.IsValid())
			{
				(void)invoker.TryInvoke();
			}
		}

		m_pScriptSceneStateLoadJob.Reset(SEOUL_NEW(MemoryBudgets::TBD) ScriptSceneStateLoadJob(m_Settings));
		m_pScriptSceneStateLoadJob->StartJob();
		m_bPendingHotLoad = false;
	}
#endif // /#if SEOUL_HOT_LOADING

	return m_pState.IsValid();
}

} // namespace Seoul

#endif // /#if SEOUL_WITH_SCENE
