/**
 * \file GameScriptManager.cpp
 * \brief Global Singletons that owns the Lua script VM used
 * by game logic and UI screens.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "AnalyticsManager.h"
#include "GameClient.h"
#include "EventsManager.h"
#include "GamePersistenceManager.h"
#include "GameScriptManager.h"
#include "GameScriptManagerVmCreateJob.h"
#include "ReflectionUtil.h"
#include "ReflectionDefine.h"
#include "ScriptFunctionInvoker.h"
#include "ScriptManager.h"
#include "ScriptUIInstance.h"
#include "ScriptUIMovie.h"
#include "ScriptVm.h"
#include "TrackingManager.h"
#include "UIManager.h"
#include "VmStats.h"

namespace Seoul::Game
{

static const HString kGlobalCanHandlePurchasedItems("HANDLER_GlobalCanHandlePurchasedItems");
static const HString kGlobalOnItemPurchased("HANDLER_GlobalOnItemPurchased");
static const HString kGlobalOnItemInfoRefreshed("HANDLER_GlobalOnItemInfoRefreshed");
static const HString kGlobalOnSessionStart("HANDLER_GlobalOnSessionStart");
static const HString ksGetValue("GetValue");
static const HString ksSetValue("SetValue");
static const HString kFunctionSeoulMainThreadInit("SeoulMainThreadInit");
static const HString kFunctionScriptInitializeComplete("ScriptInitializeComplete");

Bool ScriptManager::CanHandlePurchasedItems()
{
	// Check original hook first - if it was overriden
	// prior to our override, then it will always return
	// false. Otherwise it is a valid hook.
	if (ScriptManager::Get()->m_pPreviousEngineVirtuals->CanHandlePurchasedItems())
	{
		return true;
	}

	auto pVM(ScriptManager::Get()->GetVm());
	if (!pVM.IsValid())
	{
		return false;
	}
	Script::FunctionInvoker invoker(*pVM, kGlobalCanHandlePurchasedItems);
	if (!invoker.IsValid())
	{
		return false;
	}
	if (!invoker.TryInvoke())
	{
		return false;
	}
	Bool ScriptEnvironmentActive = false;
	invoker.GetBoolean(0, ScriptEnvironmentActive);
	return ScriptEnvironmentActive;
}

void ScriptManager::OnItemInfoRefreshed(CommerceManager::ERefreshResult eResult)
{
	// "super" call.
	ScriptManager::Get()->m_pPreviousEngineVirtuals->OnItemInfoRefreshed(eResult);

	auto pVM(ScriptManager::Get()->GetVm());
	if (pVM.IsValid())
	{
		Script::FunctionInvoker invoker(*pVM, kGlobalOnItemInfoRefreshed);
		if (invoker.IsValid())
		{
			invoker.PushEnumAsNumber(eResult);
			(void)invoker.TryInvoke();
		}
	}
}

void ScriptManager::OnSessionStart(const WorldTime& timestamp)
{
	// "super" call.
	ScriptManager::Get()->m_pPreviousEngineVirtuals->OnSessionStart(timestamp);

	// "this" call.
	ScriptOnSessionStart(timestamp);
}

void ScriptManager::ScriptOnSessionStart(const WorldTime& timestamp)
{
	auto pVM(ScriptManager::Get()->GetVm());
	if (pVM.IsValid())
	{
		Script::FunctionInvoker invoker(*pVM, kGlobalOnSessionStart);
		if (invoker.IsValid())
		{
			(void)invoker.TryInvoke();
		}
		else
		{
			// OnSessionStart is allowed to not exist.
		}
	}
	else
	{
		SEOUL_WARN("Vm was not valid to invoke OnSessionStart");
	}
}

void ScriptManager::OnItemPurchased(
	HString itemID,
	CommerceManager::EPurchaseResult eResult,
	PurchaseReceiptData const* pReceiptData)
{
	// "super" call.
	ScriptManager::Get()->m_pPreviousEngineVirtuals->OnItemPurchased(itemID, eResult, pReceiptData);

	auto pVM(ScriptManager::Get()->GetVm());
	if (pVM.IsValid())
	{
		Script::FunctionInvoker invoker(*pVM, kGlobalOnItemPurchased);
		if (invoker.IsValid())
		{
			invoker.PushString(itemID);
			invoker.PushEnumAsNumber(eResult);
			if (nullptr == pReceiptData)
			{
				invoker.PushNil();
			}
			else
			{
				invoker.PushAsTable(*pReceiptData);
			}
			(void)invoker.TryInvoke();
		}
	}
}

ScriptManager::ScriptManager(const ScriptManagerSettings& settings, const SharedPtr<Script::Vm>& pVm)
	: m_pVm(pVm)
	, m_pVmCreateJob()
	, m_Settings(settings)
	, m_DataStore()
	, m_MetatablesDataStore()
	, m_pPreviousEngineVirtuals()
	, m_CurrentEngineVirtuals()
{
	SEOUL_ASSERT(IsMainThread());

	// Sanity check.
	SEOUL_ASSERT(m_pVm.IsValid());

	// Register the first VM for hot loading.
#if SEOUL_HOT_LOADING
	m_pVm->RegisterForHotLoading();
#endif // /#if SEOUL_HOT_LOADING

	// Run main thread init of the new VM.
	{
		Script::FunctionInvoker invoker(*m_pVm, kFunctionSeoulMainThreadInit);
		if (invoker.IsValid())
		{
			(void)invoker.TryInvoke();
		}
	}

	if (m_Settings.m_InstantiatorOverride.IsValid())
	{
		// Register instantiatorOverride as the custom instantiator.
		UI::Manager::Get()->SetCustomUIMovieInstantiator(m_Settings.m_InstantiatorOverride);
	}
	else
	{
		// Register Game::ScriptManager as the custom instantiator.
		UI::Manager::Get()->SetCustomUIMovieInstantiator(SEOUL_BIND_DELEGATE(&ScriptManager::InstantiateScriptingMovie, this));
	}

	m_pPreviousEngineVirtuals = g_pEngineVirtuals;
	m_CurrentEngineVirtuals = *g_pEngineVirtuals;
	m_CurrentEngineVirtuals.CanHandlePurchasedItems = &CanHandlePurchasedItems;
	m_CurrentEngineVirtuals.OnItemPurchased = &OnItemPurchased;
	m_CurrentEngineVirtuals.OnItemInfoRefreshed = &OnItemInfoRefreshed;
	m_CurrentEngineVirtuals.OnSessionStart = &OnSessionStart;
	g_pEngineVirtuals = &m_CurrentEngineVirtuals;

	ScriptOnSessionStart(Client::StaticGetCurrentServerTime());
}

ScriptManager::~ScriptManager()
{
	SEOUL_ASSERT(IsMainThread());

	// Restore previous engine virtuals.
	g_pEngineVirtuals = m_pPreviousEngineVirtuals;
	m_pPreviousEngineVirtuals = nullptr;

	// We are no longer the custom instantiator.
	UI::Manager::Get()->SetCustomUIMovieInstantiator(UI::Manager::CustomUIMovieInstantiator());

	// Unregister our current VM from hot loading.
#if SEOUL_HOT_LOADING
	m_pVm->UnregisterFromHotLoading();
#endif // /#if SEOUL_HOT_LOADING

	// Dispose global resources prior to reset.
	{
		Script::FunctionInvoker invoker(*m_pVm, kFunctionSeoulDispose);
		if (invoker.IsValid())
		{
			(void)invoker.TryInvoke();
		}
	}

	// End hot loading interval if requested.
	if (m_pVmCreateJob.IsValid())
	{
		// Make sure the create job terminates before
		// we continue.
		m_pVmCreateJob->WaitUntilJobIsNotRunning();
		m_pVmCreateJob.Reset();
		Script::Manager::Get()->EndAppScriptHotLoad();
	}

	// Sanity check - if we're not the exclusive owner at this point, there is some lingering
	// game state that has a dangling reference to the script VM.
	SEOUL_ASSERT(m_pVm.IsUnique());

	// Finalize any remaining UI instance objects.
	ScriptUIInstance::FreeRoots(true);

	m_pVm.Reset();

	// Finalize any remaining UI instance objects.
	ScriptUIInstance::FreeRoots(true);
}

void ScriptManager::LoadNewVm(Bool bReloadUI /*=false*/)
{
	SEOUL_ASSERT(IsMainThread());

	// Can't have two jobs running simultaneously.
	if (m_pVmCreateJob.IsValid())
	{
		return;
	}

	// Dispose are existing Vm before hot loading a new one. This will release debugger resources.
	Script::FunctionInvoker invoker(*m_pVm, kFunctionSeoulDispose);
	if (invoker.IsValid())
	{
		(void)invoker.TryInvoke();
	}

	// Now in a hot loading interval.
	Script::Manager::Get()->BeginAppScriptHotLoad();

	// Instantiate and start the create job.
	m_pVmCreateJob.Reset(SEOUL_NEW(MemoryBudgets::Scripting) ScriptManagerVmCreateJob(m_Settings, bReloadUI));
	m_pVmCreateJob->StartJob();
}

void ScriptManager::PreShutdown()
{
	if (!m_pVm.IsValid())
	{
		return;
	}

	// Dispose before final GC.
	{
		Script::FunctionInvoker invoker(*m_pVm, kFunctionSeoulDispose);
		if (invoker.IsValid())
		{
			(void)invoker.TryInvoke();
		}
	}

	// Run a full GC cycle.
	m_pVm->GcFull();

	// Finalize any remaining UI instance objects.
	ScriptUIInstance::FreeRoots(true);
}

void ScriptManager::Tick()
{
	SEOUL_ASSERT(IsMainThread());

	// Incremental garbage collection.
	m_pVm->StepGarbageCollector();

	// Finalize any pending VM create job.
	FinalizeVmCreateJob();

#if SEOUL_HOT_LOADING
	if (m_pVm->IsOutOfDate())
	{
		// Avoid a loading loop.
		m_pVm->ClearOutOfDate();

		// Start a new VM load.
		LoadNewVm(true);
	}
#endif // /#if SEOUL_HOT_LOADING

	// Gradual release of UI instance nodes.
	ScriptUIInstance::FreeRoots(false);
}

void ScriptManager::FinalizeVmCreateJob()
{
	// Early out if no Job or still running.
	if (!m_pVmCreateJob.IsValid() || m_pVmCreateJob->IsJobRunning())
	{
		return;
	}

	// Job did not complete successfully, can't do anything further.
	if (Jobs::State::kComplete != m_pVmCreateJob->GetJobState())
	{
		m_pVmCreateJob.Reset();

		// End hot loading interval.
		Script::Manager::Get()->EndAppScriptHotLoad();

		return;
	}

	// Acquire the Job's VM.
	auto pVm(m_pVmCreateJob->TakeOwnershipOfVm());

#if SEOUL_HOT_LOADING
	// Now register the new Vm for hot loading.
	pVm->RegisterForHotLoading();

	// If the Job was a UI reload trigger, start that process now.
	if (m_pVmCreateJob->IsReloadUI())
	{
		// Tell the existing VM we're hot loading.
		m_pVm->TryInvokeGlobalOnHotload();
		pVm->TryInvokeGlobalRestoreDynamicGameStateData();
		pVm->TryInvokeGlobalPostHotload();
	}

	// Unregister the old VM from hot loading.
	m_pVm->UnregisterFromHotLoading();

	// Now trigger the UI hot loading if specified.
	if (m_pVmCreateJob->IsReloadUI())
	{
		UI::Manager::Get()->HotReload();
	}
#endif // /#if SEOUL_HOT_LOADING

	// Dispose global resources of the old VM prior to resetting.
	{
		Script::FunctionInvoker invoker(*m_pVm, kFunctionSeoulDispose);
		if (invoker.IsValid())
		{
			(void)invoker.TryInvoke();
		}
	}

	// Finalize any remaining UI instance objects.
	ScriptUIInstance::FreeRoots(true);

	// Now swap out and release.
	m_pVmCreateJob.Reset();
	m_pVm.Swap(pVm);
	pVm.Reset();

	// Finalize any remaining UI instance objects.
	ScriptUIInstance::FreeRoots(true);

	// Run main thread init of the new VM.
	{
		Script::FunctionInvoker invoker(*m_pVm, kFunctionSeoulMainThreadInit);
		if (invoker.IsValid())
		{
			(void)invoker.TryInvoke();
		}
	}

	// End hot loading interval.
	Script::Manager::Get()->EndAppScriptHotLoad();
}

UI::Movie* ScriptManager::InstantiateScriptingMovie(HString typeName)
{
	return SEOUL_NEW(MemoryBudgets::Scripting) ScriptUIMovie(m_pVm, typeName);
}

void ScriptManager::OnScriptInitializeComplete()
{
	auto pVM(ScriptManager::Get()->GetVm());
	if (pVM.IsValid())
	{
		Script::FunctionInvoker invoker(*pVM, kFunctionScriptInitializeComplete);
		if (invoker.IsValid())
		{
			(void)invoker.TryInvoke();
		}
	}
}

} // namespace Seoul::Game
