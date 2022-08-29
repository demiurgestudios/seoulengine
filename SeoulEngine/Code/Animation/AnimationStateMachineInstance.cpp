/**
 * \file AnimationStateMachineInstance.cpp
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

#include "AnimationStateMachineDefinition.h"
#include "AnimationStateMachineInstance.h"
#include "AnimationNetworkInstance.h"
#include "Logger.h"

#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)

namespace Seoul::Animation
{

StateMachineInstance::StateMachineInstance(NetworkInstance& r, const SharedPtr<StateMachineDefinition const>& pStateMachine, const NodeCreateData& creationData)
	: m_r(r)
	, m_pStateMachine(pStateMachine)
	, m_vPendingTriggers()
	, m_pOld()
	, m_pNew()
	, m_OldId()
	, m_NewId()
	, m_fInTransitionTime(0.0f)
	, m_fTransitionTargetTime(0.0f)
	, m_uTransitionCount(0u)
	, m_eSlotBlendMode(Animation::SlotBlendMode::kNone)
{
	// Start in the default state.
	if (creationData.m_sOverrideDefaultState.IsEmpty() || !m_pStateMachine->GetStates().HasValue(creationData.m_sOverrideDefaultState))
	{
		InternalGotoState(m_pStateMachine->GetDefaultState(), creationData);
	}
	else
	{
		InternalGotoState(creationData.m_sOverrideDefaultState, creationData);
	}
}

StateMachineInstance::~StateMachineInstance()
{
}

Float StateMachineInstance::GetCurrentMaxTime() const
{
	if (m_pNew.IsValid())
	{
		return m_pNew->GetCurrentMaxTime();
	}
	else
	{
		return 0.0f;
	}
}

Bool StateMachineInstance::GetTimeToEvent(HString sEventName, Float& fTimeToEvent) const
{
	if (m_pNew.IsValid())
	{
		return m_pNew->GetTimeToEvent(sEventName, fTimeToEvent);
	}
	else
	{
		return false;
	}
}

/** @return True if all playing clips are finished (one-offs that have completed). */
void StateMachineInstance::AllDonePlaying(Bool& rbDone, Bool& rbLooping) const
{
	rbDone = true;
	rbLooping = false;
	if (m_pNew.IsValid())
	{
		Bool a = false, b = true;
		m_pNew->AllDonePlaying(a, b);
		rbDone = rbDone && a;
		rbLooping = rbLooping || b;
	}

	if (m_pOld.IsValid())
	{
		Bool a = false, b = true;
		m_pOld->AllDonePlaying(a, b);
		rbDone = rbDone && a;
		rbLooping = rbLooping || b;
	}
}

/** @return True if both old and new states are valid. */
Bool StateMachineInstance::IsInStateTransition() const
{
	return m_pNew.IsValid() && m_pOld.IsValid();
}

/** Apply a trigger to the state machine. Queues up a possible state machine transition. */
void StateMachineInstance::TriggerTransition(HString name)
{
	m_vPendingTriggers.PushBack(name);

	if (m_pNew.IsValid())
	{
		m_pNew->TriggerTransition(name);
	}
}

/**
 * Per-frame update work on the state machine. May apply state machine transitions,
 * and also updates any children.
 */
Bool StateMachineInstance::Tick(Float fDeltaTimeInSeconds, Float fAlpha, Bool bBlendDiscreteState)
{
	// Check transitions first.
	InternalCheckTransitions();

	// Update states
	Float32 fMix = 1.0f;
	if (InTransition())
	{
		// Must be enforced by InTransition().
		SEOUL_ASSERT(m_fTransitionTargetTime > 0.0f);

		// Advance and get the current transition alpha.
		m_fInTransitionTime += fDeltaTimeInSeconds;
		fMix = GetTransitionAlpha();
	}

	// If either not in a transition or no longer in a transition, make sure m_OldId
	// is cleared.
	if (!InTransition())
	{
		m_pOld.Reset();
		m_OldId = HString();
	}

	// Tick the active states.
	Bool bReturn = true;
	if (m_pOld.IsValid())
	{
		auto const bBlend = bBlendDiscreteState && (SlotBlendMode::kNone == m_eSlotBlendMode || SlotBlendMode::kOnlySource == m_eSlotBlendMode);
		bReturn = m_pOld->Tick(fDeltaTimeInSeconds, (1.0f - fMix) * fAlpha, bBlend) || bReturn;
	}

	if (m_pNew.IsValid())
	{
		auto const bBlend = bBlendDiscreteState && (!m_pOld.IsValid() || (SlotBlendMode::kNone == m_eSlotBlendMode || SlotBlendMode::kOnlyTarget == m_eSlotBlendMode));
		bReturn = m_pNew->Tick(fDeltaTimeInSeconds, fMix * fAlpha, bBlend) || bReturn;
	}

	return bReturn;
}

/**
 * Utility function, meant for developer utilities. Returns a list of the trigger names that
 * will cause a transition if fired in the current state machine context.
 */
void StateMachineInstance::GetViableTriggers(ViableTriggers& r) const
{
	// No triggers if no new/current state.
	auto p(m_pStateMachine->GetStates().Find(m_NewId));
	if (nullptr == p)
	{
		return;
	}

	// Enumerate transitions of the current state and check for viability.
	for (auto i = p->m_vTransitions.Begin(); p->m_vTransitions.End() != i; ++i)
	{
		// Empty trigger set, no triggers.
		if (i->m_Triggers.IsEmpty())
		{
			continue;
		}

		// Any conditions fail, can't trigger.
		if (!InternalAreTrue(i->m_vConditions))
		{
			continue;
		}

		// Any negative conditions are true, can't trigger.
		if (!InternalAreFalse(i->m_vNegativeConditions))
		{
			continue;
		}

		// Viable - all all triggers of this transition to the viable set.
		{
			auto const iBegin = i->m_Triggers.Begin();
			auto const iEnd = i->m_Triggers.End();
			for (auto iTrigger = iBegin; iEnd != iTrigger; ++iTrigger)
			{
				(void)r.Insert(*iTrigger);
			}
		}
	}
}

/** Utility, checks if all listed conditions are the true value. */
Bool StateMachineInstance::InternalAreTrue(const Conditions& vConditions) const
{
	for (auto i = vConditions.Begin(); vConditions.End() != i; ++i)
	{
		if (!m_r.GetCondition(*i))
		{
			return false;
		}
	}

	return true;
}

/** Utility, checks if all listed conditions are the false value. */
Bool StateMachineInstance::InternalAreFalse(const Conditions& vConditions) const
{
	for (auto i = vConditions.Begin(); vConditions.End() != i; ++i)
	{
		if (m_r.GetCondition(*i))
		{
			return false;
		}
	}

	return true;
}

void StateMachineInstance::InternalCheckTransitions()
{
	// If no new/current state, try a Goto() default.
	if (m_NewId.IsEmpty())
	{
		SEOUL_LOG_ANIMATION("Network %s no new state, goto default state", m_r.GetNetworkHandle().GetKey().CStr());
		InternalGotoState(m_pStateMachine->GetDefaultState(), NodeCreateData());
	}
	// Otherwise, evaluate transitions.
	else
	{
		auto p = m_pStateMachine->GetStates().Find(m_NewId);
		SEOUL_ASSERT(nullptr != p);

		// Prior to evaluation, insert an empty trigger to evaluate
		// just conditions, as the final check.
		m_vPendingTriggers.PushBack(HString());

		// Enumerate all triggers, then evaluate all against each transition of the current state.
		for (auto const &trigger : m_vPendingTriggers)
		{
			bool bHandledTrigger = false;
			for (auto const &transition : p->m_vTransitions)
			{
				// Transition evaluates if:
				// - it has no triggers, and this is a condition-only evaluation, or it contains the trigger *and*
				// - it has no conditions or all those conditions are true *and*
				// - it has no negative conditions or all those conditions are false.
				if (((trigger.IsEmpty() && transition.m_Triggers.IsEmpty()) || transition.m_Triggers.HasKey(trigger)) &&
					InternalAreTrue(transition.m_vConditions) &&
					InternalAreFalse(transition.m_vNegativeConditions) &&
					InternalGotoState(transition.m_Target, NodeCreateData(transition.m_sOverrideDefaultState)))
				{
					m_fTransitionTargetTime = transition.m_fTimeInSeconds;
					m_eSlotBlendMode = transition.m_eSlotBlendMode;

					// Otherwise, acquire the new state for the remaining processing, then break out of the inner loop.
					p = m_pStateMachine->GetStates().Find(m_NewId);
					SEOUL_ASSERT(nullptr != p);
					bHandledTrigger = true;
					break;
				}
			}

			if (bHandledTrigger)
			{
				SEOUL_LOG_ANIMATION("Network %s State %s handled trigger %s", m_r.GetNetworkHandle().GetKey().CStr(), m_OldId.CStr(), trigger.CStr());
			}
			else if (!trigger.IsEmpty())
			{
				SEOUL_LOG_ANIMATION("Networkd %s State %s dropped trigger %s", m_r.GetNetworkHandle().GetKey().CStr(), m_NewId.CStr(), trigger.CStr());
			}
		}

		// Done with pending triggers.
		m_vPendingTriggers.Clear();
	}
}

/**
 * Internal utility, actually performs the state changing and necessary cleanup
 * to move to a new state.
 */
Bool StateMachineInstance::InternalGotoState(HString name, const NodeCreateData& creationData)
{
	SEOUL_LOG_ANIMATION("Network %s StateMachineInstance::InternalGotoState %s from %s", m_r.GetNetworkHandle().GetKey().CStr(), name.CStr(), m_NewId.CStr());
	// Fail if we can't find the specified state.
	auto p = m_pStateMachine->GetStates().Find(name);
	if (nullptr == p)
	{
		return false;
	}

	// If we're in a transition, then we keep either m_pOld
	// or m_pNew, depending on the current transition alpha.
	if (InTransition())
	{
		Float const fMix = GetTransitionAlpha();

		// Mix is >= 0.5, keep new. Swap it in for old.
		if (fMix >= 0.5f)
		{
			m_pOld.Swap(m_pNew);
			m_OldId = m_NewId;
		}
	}
	// Otherwise, always keep m_pNew, so swap it to m_pOld,
	// as m_pNew is just about to be replaced with a new
	// instance.
	else
	{
		m_pOld.Swap(m_pNew);
		m_OldId = m_NewId;
	}

	// Create new.
	m_pNew.Reset(p->m_pChild->CreateInstance(m_r, creationData));
	m_NewId = name;

	// TODO: Not the best if we just short-circuited a transition.
	// I think the "correct" solution would be to keep as many as 'n' states,
	// and blend between all by varying alphas.

	// Reset the transition time.
	m_fInTransitionTime = 0.0f;

	// Increment the transition count.
	++m_uTransitionCount;

	return true;
}

} // namespace Seoul::Animation

#endif // /#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)
