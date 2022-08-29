/**
 * \file AnimationStateMachineInstance.h
 * \brief Runtime instantiation of a state machine animation network
 * node. Used for runtime playback of a defined state in an animation
 * graph.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ANIMATION_STATE_MACHINE_INSTANCE_H
#define ANIMATION_STATE_MACHINE_INSTANCE_H

#include "AnimationNodeInstance.h"
#include "AnimationSlotBlendMode.h"
#include "HashSet.h"
#include "Vector.h"
namespace Seoul { namespace Animation { class StateMachineDefinition; } }

#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)

namespace Seoul::Animation
{

/** Defines a runtime state machine in an animation network. */
class StateMachineInstance SEOUL_SEALED : public NodeInstance
{
public:
	typedef Vector<HString, MemoryBudgets::Animation> PendingTriggers;
	typedef HashSet<HString, MemoryBudgets::Animation> ViableTriggers;

	StateMachineInstance(NetworkInstance& r, const SharedPtr<StateMachineDefinition const>& pStateMachine, const NodeCreateData& creationData);
	~StateMachineInstance();

	virtual Float GetCurrentMaxTime() const SEOUL_OVERRIDE;

	// returns true if the animation event was found after the current animation time, and sets the time to event
	// returns false if the animation event was not found
	virtual Bool GetTimeToEvent(HString sEventName, Float& fTimeToEvent) const SEOUL_OVERRIDE;

	virtual NodeType GetType() const SEOUL_OVERRIDE { return NodeType::kStateMachine; }
	virtual void AllDonePlaying(Bool& rbDone, Bool& rbLooping) const SEOUL_OVERRIDE;
	virtual Bool IsInStateTransition() const SEOUL_OVERRIDE;
	virtual void TriggerTransition(HString name) SEOUL_OVERRIDE;
	virtual Bool Tick(Float fDeltaTimeInSeconds, Float fAlpha, Bool bBlendDiscreteState) SEOUL_OVERRIDE;

	// Read-only access to various bits of state machine state.
	const SharedPtr<NodeInstance>& GetOld() const { return m_pOld; }
	const SharedPtr<NodeInstance>& GetNew() const { return m_pNew; }
	HString GetOldId() const { return m_OldId; }
	HString GetNewId() const { return m_NewId; }
	const SharedPtr<StateMachineDefinition const>& GetStateMachine() const { return m_pStateMachine; }
	UInt32 GetTransitionCount() const { return m_uTransitionCount; }

	/**
	 * @return The mix value between old and new states. Return value is on [0, 1].
	 */
	Float GetTransitionAlpha() const
	{
		// Clamp is NaN safe, so a divide by zero here will be clamped
		// to 0.0f.
		return Clamp(m_fInTransitionTime / m_fTransitionTargetTime, 0.0f, 1.0f);
	}

	// Utility function, meant for developer utilities. Returns a list of the trigger names that
	// will cause a transition if fired in the current state machine context.
	void GetViableTriggers(ViableTriggers& r) const;

	/** @return True if the state machine is currently transitioning, false otherwise. */
	Bool InTransition() const
	{
		return (m_pOld.IsValid() && m_pNew.IsValid() && m_fInTransitionTime < m_fTransitionTargetTime);
	}

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(StateMachineInstance);

	NetworkInstance& m_r;
	SharedPtr<StateMachineDefinition const> m_pStateMachine;
	PendingTriggers m_vPendingTriggers;
	SharedPtr<NodeInstance> m_pOld;
	SharedPtr<NodeInstance> m_pNew;
	HString m_OldId;
	HString m_NewId;
	Float32 m_fInTransitionTime;
	Float32 m_fTransitionTargetTime;
	UInt32 m_uTransitionCount;
	Animation::SlotBlendMode m_eSlotBlendMode;

	typedef Vector<HString, MemoryBudgets::Animation> Conditions;
	Bool InternalAreTrue(const Conditions& vConditions) const;
	Bool InternalAreFalse(const Conditions& vConditions) const;
	void InternalCheckTransitions();
	Bool InternalGotoState(HString name, const NodeCreateData& creationData);

	SEOUL_DISABLE_COPY(StateMachineInstance);
}; // class StateMachineInstance

} // namespace Seoul::Animation

#endif // /#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)

#endif // include guard
