/**
 * \file ScriptUIManager.h
 * \brief Binder instance for exposing the global UI::Manager singleton
 * into script.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCRIPT_UI_MANAGER_H
#define SCRIPT_UI_MANAGER_H

#include "Prereqs.h"
namespace Seoul { namespace Script { class FunctionInterface; } }

namespace Seoul
{

/** Binder, instanced into a script VM to expose UI::Manager to the script environment. */
class ScriptUIManager SEOUL_SEALED
{
public:
	ScriptUIManager();
	~ScriptUIManager();

	// Send a UI broadcast to all UI::Movie instances on the stack.
	void BroadcastEvent(Script::FunctionInterface* pInterface);

	// Send a UI broadcast to a specific UI::Movie on the stack.
	void BroadcastEventTo(Script::FunctionInterface* pInterface);

	// takes in a state machine and target movieclip name, returns movie clip instance id
	void GetRootMovieClip(Script::FunctionInterface* pInterface) const;

	// Get the current state of a UI condition variable.
	Bool GetCondition(HString conditionName) const;

	Float GetPerspectiveFactorAdjustment() const;

	// Send a UI broadcast to all UI::Movie instances on the stack.
	// If the event is not received, it will be queued and retried
	// until it is received.
	void PersistentBroadcastEvent(Script::FunctionInterface* pInterface);

	// Send a UI broadcast to a specific UI::Movie on the stack.
	// If the event is not received, it will be queued and retried
	// until it is received.
	void PersistentBroadcastEventTo(Script::FunctionInterface* pInterface);

	// Update the state of a UI condition variable.
	void SetCondition(HString conditionName, Bool bValue);

	void SetPerspectiveFactorAdjustment(Float f);

	void SetStage3DSettings(HString name);

	// Fire a UI trigger.
	void TriggerTransition(HString triggerName);
	void DebugLogEntireUIState() const;

	Float GetViewportAspectRatio() const;

	// Returns a 2D position projected into "projected world space"
	void ComputeWorldSpaceDepthProjection(Script::FunctionInterface* pInterface) const;

	// Returns a 2D position unprojected through "projected world space"
	void ComputeInverseWorldSpaceDepthProjection(Script::FunctionInterface* pInterface) const;

	// Returns the current state id for the given state machine.
	HString GetStateMachineCurrentStateId(HString sStateMachineName) const;

	// Management of the input whitelist, which is used to apply exclusivity
	// to a limited subset of movie clips.
	void AddToInputWhitelist(Script::FunctionInterface* pInterface);
	void ClearInputWhitelist();
	void RemoveFromInputWhitelist(Script::FunctionInterface* pInterface);
	void SetInputActionsEnabled(bool bEnabled);
	Bool HasPendingTransitions() const;

	// Developer only - immediately jump to a state.
#if SEOUL_ENABLE_CHEATS
	void GotoState(HString stateMachineName, HString stateName);
#endif // /#if SEOUL_ENABLE_CHEATS

#if !SEOUL_SHIP
	Bool ValidateUiFiles(const String& sExcludeWildcard) const;
#endif // /#if !SEOUL_SHIP

	void TriggerRestart(Bool bForceImmediate);
	String GetPlainTextString(const String& input) const;
#if SEOUL_HOT_LOADING
	void ShelveDataForHotLoad(Script::FunctionInterface* pInterface);
	void UnshelveDataFromHotLoad(Script::FunctionInterface* pInterface);
#endif

private:
	SEOUL_DISABLE_COPY(ScriptUIManager);
}; // class ScriptUIManager

} // namespace Seoul

#endif // include guard
