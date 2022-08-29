/**
 * \file GamePatcherStatus.h
 * \brief Screen framework for displaying progress and status
 * of the patching process.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef GAME_PATCHER_STATUS_H
#define GAME_PATCHER_STATUS_H

#include "GamePatcherState.h"
#include "UIMovie.h"

namespace Seoul::Game
{

/**
 * Displays patching status/progress. Can be customized by an App to implement
 * display updates to particular events.
 *
 * Cannot depend on scripting. This screen exists prior to and during the creation
 * of the application's main script VM.
 */
class PatcherStatus : public UI::Movie
{
public:
#if SEOUL_ENABLE_CHEATS
	static Bool s_bDevOnlyDisableMinimumDisplay;
#endif // /#if SEOUL_ENABLE_CHEATS

	SEOUL_REFLECTION_POLYMORPHIC(PatcherStatus);

	PatcherStatus();
	virtual ~PatcherStatus();

protected:
	virtual void HandleProgress(Float fProgress) {}
	virtual void HandleServerDown(const String& sMessage) {}
	virtual void HandleSetText(HString locToken) {}
	virtual void HandleUpdateRequired(Bool bUpdateRequired) {}
	virtual void OnTick(RenderPass& rPass, Float fDeltaTimeInSeconds) SEOUL_OVERRIDE;

private:
	SEOUL_REFLECTION_FRIENDSHIP(PatcherStatus);

	virtual void OnConstructMovie(HString movieTypeName) SEOUL_OVERRIDE SEOUL_SEALED;
	virtual void OnLinkClicked(
		const String& sLinkInfo,
		const String& sLinkType,
		const SharedPtr<Falcon::MovieClipInstance>& pInstance) SEOUL_OVERRIDE;
	virtual void OnPose(RenderPass& rPass, UI::Renderer& rRenderer) SEOUL_OVERRIDE SEOUL_SEALED;

	// NOTE: This functionality is in large part copy and pasted
	// from the script definition of MovieScroller.cs and
	// utility functions in ScriptUIMovieClipInstance.cpp. This is
	// because patching exists without game code (and no script VM)
	// and must remain isolated.
	void AddFullScreenClipper();
	Bool m_bScrollOut;
	Bool GetScrollOut() const { return m_bScrollOut; }
	Bool PerformScrollOut(Float fDeltaTimeInSeconds);
	void SetScrollOut(Bool b) { m_bScrollOut = b; }
	// END NOTE:

	class StateTracking SEOUL_SEALED
	{
	public:
		StateTracking()
			: m_fDisplayProgress(0.0f)
			, m_fTargetProgress(0.0f)
			, m_iStateChangeTimeInTicks(-1)
			, m_eLastState(PatcherState::kGDPRCheck)
			, m_bIsConnected(false)
			, m_bRequiredVersionUpdate(false)
		{
		}

		/** @return The smoothed progress value we display to the user. */
		Float GetDisplayProgress() const
		{
			return m_fDisplayProgress;
		}

		/** @return the last committed patcher state. */
		PatcherState GetLastState() const
		{
			return m_eLastState;
		}

		// Apply a progress update - applies interpolation and smoothing.
		Float ApplyProgress(Float fDeltaTimeInSeconds, Float fProgress);

		// Attempt to update the current state - returns true on successful update, false otherwise.
		Bool UpdateState(
			PatcherState eState,
			Bool bIsConnected,
			Bool bRequiredVersionUpdate);

	private:
		Float m_fDisplayProgress;
		Float m_fTargetProgress;
		Int64 m_iStateChangeTimeInTicks;
		PatcherState m_eLastState;
		Bool m_bIsConnected;
		Bool m_bRequiredVersionUpdate;
	}; // class StateTracking

	Float m_fElapsedDisplayTimeInSeconds;
	Float m_fMinimumDisplayTimeInSeconds;
	StateTracking m_Tracking;
	Bool m_bFirstRender;
}; // class Game::PatcherStatus

} // namespace Seoul::Game

#endif // include guard
