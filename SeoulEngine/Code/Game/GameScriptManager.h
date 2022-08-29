/**
 * \file GameScriptManager.h
 * \brief Global Singletons that owns the Lua script VM used
 * by game logic and UI screens.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef GAME_SCRIPT_MANAGER_H
#define GAME_SCRIPT_MANAGER_H

#include "DataStore.h"
#include "GameScriptManagerSettings.h"
#include "HashTable.h"
#include "ReflectionAny.h"
#include "ReflectionType.h"
#include "ScriptVm.h"
#include "Singleton.h"
#include "EngineVirtuals.h"
namespace Seoul { namespace Game { class ScriptManagerVmCreateJob; } }
namespace Seoul { namespace Script { class Vm; } }
namespace Seoul { namespace Falcon { class Instance; } }
namespace Seoul { namespace Falcon { class MovieClipInstance; } }

namespace Seoul::Game
{

class ScriptManager SEOUL_SEALED : public Singleton<ScriptManager>
{
public:
	SEOUL_DELEGATE_TARGET(ScriptManager);

	typedef Delegate<UI::Movie*(HString typeName)> CustomUIMovieInstantiator;
	ScriptManager(const ScriptManagerSettings& settings, const SharedPtr<Script::Vm>& pVm);
	~ScriptManager();

	const SharedPtr<Script::Vm>& GetVm() const { return m_pVm; }
	void LoadNewVm(Bool bReloadUI = false);
	void PreShutdown();
	void Tick();
	void OnScriptInitializeComplete();

private:
	SharedPtr<Script::Vm> m_pVm;
	SharedPtr<ScriptManagerVmCreateJob> m_pVmCreateJob;
	ScriptManagerSettings const m_Settings;
	friend class ScriptManagerProxy;
	DataStore m_DataStore;
	DataStore m_MetatablesDataStore;
	EngineVirtuals const* m_pPreviousEngineVirtuals;
	EngineVirtuals m_CurrentEngineVirtuals;

	void FinalizeVmCreateJob();
	UI::Movie* InstantiateScriptingMovie(HString typeName);

	static Bool CanHandlePurchasedItems();
	static void OnItemInfoRefreshed(CommerceManager::ERefreshResult eResult);
	static void OnSessionStart(const WorldTime& timestamp);
	static void ScriptOnSessionStart(const WorldTime& timestamp);
	static void OnItemPurchased(
		HString itemID,
		CommerceManager::EPurchaseResult eResult,
		PurchaseReceiptData const* pReceiptData);

	SEOUL_DISABLE_COPY(ScriptManager);
}; // class Game::ScriptManager

} // namespace Seoul::Game

#endif // include guard
