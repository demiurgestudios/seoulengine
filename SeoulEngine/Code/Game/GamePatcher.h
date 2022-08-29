/**
 * \file GamePatcher.h
 * \brief Screens and logic involved in the patching process.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef GAME_PATCHER_H
#define GAME_PATCHER_H

#include "ContentReload.h"
#include "GamePatcherState.h"
#include "HTTPCommon.h"
#include "Singleton.h"
#include "UIMovie.h"
namespace Seoul { namespace Game { class ConfigManagerLoadJob; } }
namespace Seoul { namespace Game { class PersistenceManagerLoadJob; } }
namespace Seoul { namespace Game { class PatcherApplyJob; } }
namespace Seoul { namespace Game { class ScriptManager; } }
namespace Seoul { namespace Game { class ScriptManagerVmCreateJob; } }

namespace Seoul::Game
{

/**
 * UI::Movie with no associated SWF file. Clears all game state
 * when loaded, intended to acts as an empty state in which it is safe
 * to clear game state.
 */
class Patcher SEOUL_SEALED : public UI::Movie, public Singleton<Patcher>
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(Patcher);

	Patcher();
	~Patcher();

	// Return the total operation progress - range is [0, 1].
	Float GetProgress() const;

	/** @return The current state of the patcher sequence. */
	PatcherState GetState() const
	{
		return m_eState;
	}

	/** @return The current state of the patcher's stats tracking. */
	const PatcherDisplayStats& GetStats() const
	{
		return m_Stats;
	}

#if SEOUL_HOT_LOADING
	/**
	 * The Game::Patcher does not hot reload.
	 */
	virtual Bool IsPartOfHotReload() const SEOUL_OVERRIDE
	{
		return false;
	}
#endif // /#if SEOUL_HOT_LOADING

private:
	SEOUL_DISABLE_COPY(Patcher);
	SEOUL_REFLECTION_FRIENDSHIP(Patcher);

	// Broadcast event handler.
	void OnPatcherStatusFirstRender();
	// /Broadcast event handler.

	static HTTP::CallbackResult OnPrecacheUrl(HTTP::Result, HTTP::Response*);
	virtual void OnTick(RenderPass& rPass, Float fDeltaTimeInSeconds) SEOUL_OVERRIDE;

	// Configuration.
	typedef Vector<String, MemoryBudgets::Game> PrecacheUrls;
	PrecacheUrls m_vPrecacheUrls;

	void SetState(PatcherState eState);

	// State.
	PatcherDisplayStats m_Stats;
	TimeInterval const m_StartUptime;
	TimeInterval m_LastStateChangeUptime;
	Float m_fElapsedDisplayTimeInSeconds;
	SharedPtr<PatcherApplyJob> m_pApplyJob;
	SharedPtr<ConfigManagerLoadJob> m_pGameConfigManagerLoadJob;
#if SEOUL_WITH_GAME_PERSISTENCE
	SharedPtr<PersistenceManagerLoadJob> m_pGamePersistenceManagerLoadJob;
#endif
	Atomic32 m_CachedUrls;
	SharedPtr<ScriptManagerVmCreateJob> m_pVmCreateJob;
	Content::Reload m_ContentPending;
	Float m_fApplyProgress;
	Float m_fLoadProgress;
	Float m_fScriptProgress;
	PatcherState m_eState;
	Bool m_bPatcherStatusLoading;
	Bool m_bSentDiskWriteFailureAnalytics;

	static Bool StayOnLoadingScreen;
}; // class Game::Patcher

} // namespace Seoul::Game

#endif // include guard
