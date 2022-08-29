/**
 * \file StateMachine.h
 * \brief Generic class to define and manage transitions between discrete states.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include "CheckedPtr.h"
#include "HashSet.h"
#include "HashTable.h"
#include "DataStore.h"
#include "SeoulHString.h"
#include "ScopedPtr.h"

namespace Seoul
{

// Forward declarations
template <typename T> class StateMachine;

template <typename T>
struct StateTraits
{
	/**
	 * Specializations of StateTraits<> must implement this function
	 * so that it returns a non-null subclass of type T, which corresponds
	 * to the state type identified by stateIdentifier.
	 *
	 * This function can return nullptr if stateIdentifier is not a known
	 * state.
	 */
	inline static T* NewState(StateMachine<T>& rOwner, HString stateIdentifier)
	{
		return nullptr;
	}
};

// Common constants and helper functions for StateMachine<>
namespace StateMachineCommon
{
	static const HString kConditionsTableEntry("Conditions");
	static const HString kDefaultStateTableEntry("DefaultState");
	static const HString kEnableTransitionsTableEntry("EnableTransitions");
	static const HString kNegativeConditionsTableEntry("NegativeConditions");
	static const HString kTransitionTag("Tag");
	static const HString kTransitionTarget("Target");
	static const HString kTransitionsTableEntry("Transitions");
	static const HString kTriggersTableEntry("Triggers");
}

/**
 * Defines a finite state machine - the generic argument T is the class
 * used to instantiate states, and must fulfill the following generic contract:
 * - define a method EnterState(T* pPreviousState), which will be invoked
 *   when a state is about to be transitioned to, with pPreviousState defined
 *   as the state being transitioned from. pPreviousState will be null if there
 *   is no previous state.
 * - define methods ExitState(T* pNextState), which will be invoked
 *   when a state is about to be transitioned out, with pNextState defined
 *   as the state being transitioned to. pNextState will be null if there is no
 *   next state.
 * - NOTE: in both EnterState and ExitState cases, pPreviousState and pNextState
 *   will never be equal to this (self transitions generate a new instance
 *   of a state with the same configuration as the existing state).
 * - specialize StateTraits<> - see the documentation on StateTraits<> for
 *   the specifics of that specialization.
 * - define a method TransitionComplete(), which will be invoked after both EnterState()
 *   and ExitState() have completed, and after any previous state has been destroyed.
 *   This is useful to run code that should be deferred until evaluation has fully entered
 *   the new state.
 * - NOTE: TransitionComplete() will not be invoked unless EnterState() is invoked.
 *
 * State transitions obey the following rules:
 * - if any conditions are specified, all conditions must be true.
 * - if any triggers are specified, any single trigger can activate the transition.
 * - if both conditions and triggers are specified, all conditions must be true
 *   when the trigger is fired to activate the transition.
 * - if neither a trigger or condition is specified this transition will always occur
 *	 (unless a trigger before it in the list also meets its requirements)
 * - calling EvaluateConditions() will not activate a transition that requires a trigger.
 * - calling Trigger() will only activate a transition that specifies that trigger, unless
 *   the Trigger identifier is the empty string. In the latter case, Trigger() behaves
 *   similarly to EvaluateConditions(), except that Trigger() will not evaluate a default
 *   transition from the empty state.
 */
template <typename T>
class StateMachine SEOUL_SEALED
{
public:
	typedef HashTable<HString, Bool, MemoryBudgets::StateMachine> Conditions;

	StateMachine(HString name = HString())
		: m_Name(name)
	{
	}

	~StateMachine()
	{
		// Goto the empty state.
		(void)ActivateState(HString());
	}

	/**
	 * @return The currently active state - can be nullptr.
	 */
	CheckedPtr<T> GetActiveState() const
	{
		return m_pActiveState.Get();
	}

	/**
	 * @return The HString identifier of the currently active state, or the empty string
	 * if no state is currently active.
	 */
	HString GetActiveStateIdentifier() const
	{
		return m_ActiveStateIdentifier;
	 }

	/**
	 * @return The name used to identify this state machine - provided for debugging and
	 * categorization purposes, this value is not used internally by StateMachine<>
	 */
	HString GetName() const
	{
		return m_Name;
	}

	/**
	 * @return A read-only reference to the configuration used
	 * to determine the behavior of this StateMachine.
	 *
	 * When specified in an JSON file, a StateMachine has the following format:
	 *
	 * DefaultState=MyDefaultStateName
	 * Transitions=({Conditions=(MyCondition), Triggers=(MyTrigger), Target=MyTargetStateName})
	 *
	 * [MyDefaultStateName]
	 * Transitions=({Conditions=(MyCondition), Triggers=(MyTrigger), Target=MyTargetStateName})
	 *
	 * [MyTargetStateName]
	 * ...
	 *
	 * which includes:
	 * - <sections> - each section defines a state, the name of the section corresponds to the name of the state.
	 * - DefaultState - entry in the default section, name of the state that will be transitioned to
	 *   on a call to EvaluateConditions() when the state machine does not have a current state. Optional.
	 * - EnableTransitions - an optional table that enables/disables a transition by tag. Useful to disable inherited global transitions.
	 * - Transitions - an array of transtions - under a section name, defines the transitions *from* the
	 *   state defined by the section. In the default section, defines "global" transitions, which will
	 *   be evaluated if no other transitions are fulfilled from the current state.
	 *   - Each element of the Transitions array is a table with the following elements:
	 *     - Conditions - an array of condition names - all conditions must be true for the transition to be activated.
	 *     - NegativeConditions - an array of condition names - all conditions must be false for the transition to be activated.
	 *     - Tag - an option identifier used to refer to a transition in an EnableTransitions entry.
	 *     - Triggers - an array of trigger names - any single trigger will activate the transition
	 *     - Target - the name of the state that will be activated if the transition is activated.
	 */
	const DataStore& GetStateMachineConfiguration() const
	{
		return m_StateMachineConfiguration;
	}

	/**
	 * @return A read-write reference to the configuration used
	 * to determine the behavior of this StateMachine.
	 */
	DataStore& GetStateMachineConfiguration()
	{
		return m_StateMachineConfiguration;
	}

	/**
	 * @return A read-only reference to the current state of condition variables
	 * in this StateMachine.
	 */
	const Conditions& GetConditions() const
	{
		return m_tConditions;
	}

	/**
	 * @return The vaule of condition conditionName.
	 *
	 * This function returns false for conditions that have not been set yet.
	 */
	Bool GetCondition(HString conditionName) const
	{
		Bool bReturn = false;
		(void)m_tConditions.GetValue(conditionName, bReturn);
		return bReturn;
	}

	/**
	 * @return The identifier of initial state of the state machine.
	 */
	HString GetDefaultStateIdentifier() const
	{
		DataNode node;
		Byte const* s = nullptr;
		UInt32 u = 0u;
		(void)m_StateMachineConfiguration.GetValueFromTable(m_StateMachineConfiguration.GetRootNode(), StateMachineCommon::kDefaultStateTableEntry, node);
	 	(void)m_StateMachineConfiguration.AsString(node, s, u);
	 	return HString(s, u);
	}

	/**
	 * Set the state of a condition variable.
	 *
	 * It is necessary to either call EvaluateConditions() to evaluate condition-only
	 * transitions, or to Trigger() the state machine, for this condition to have an effect
	 * on StateMachine<> state.
	 */
	void SetCondition(HString conditionName, Bool bValue)
	{
		m_tConditions.Overwrite(conditionName, bValue);
	}

	/**
	 * Replace all conditions in this StateMachine with tConditions.
	 */
	void SetConditions(const Conditions& tConditions)
	{
		m_tConditions = tConditions;
	}

	/**
	 * Evaluate all currently set conditions against the currently set state.
	 * If the currently set state has a transition which is fulfilled by the currently
	 * set conditions, set rActivatedTransition and rTargetStateIdentifier to the appropriate
	 * values and return true. Otherwise, leave these values unmodified and return false.
	 *
	 * In some circumstances, rActivatedTransition will be set to an invalid
	 * DataNode if no transition was activated (changed from "no state" to the
	 * default state of the state machine).
	 *
	 * @return True if a transition can be activated with current conditions, false otherwise.
	 */
	Bool CheckConditions(
		HString& rTargetStateIdentifier,
		DataNode& rActivatedTransition,
		UInt32& ruActivatedTransitionIndex) const
	{
		// With no active state, check for a default state transition, if one is specified.
		if (!m_pActiveState.IsValid())
		{
			DataNode defaultState;
			Byte const* s = nullptr;
			UInt32 u = 0u;
			if (m_StateMachineConfiguration.GetValueFromTable(m_StateMachineConfiguration.GetRootNode(), StateMachineCommon::kDefaultStateTableEntry, defaultState) &&
				m_StateMachineConfiguration.AsString(defaultState, s, u) &&
				u > 0u)
			{
				rTargetStateIdentifier = HString(s, u);
				rActivatedTransition = DataNode();
				ruActivatedTransitionIndex = 0u;
				return true;
			}

			return false;
		}

		// Otherwise, check an empty name "trigger" - this will evaluate transitions as conditions only.
		return CheckTrigger(HString(), rTargetStateIdentifier, rActivatedTransition, ruActivatedTransitionIndex);
	}

	/** Populate rTriggerSet with all trigger names that can possible trigger given the current state. */
	typedef HashSet<HString, MemoryBudgets::StateMachine> ViableTriggerNames;
	void GetViableTriggerNames(ViableTriggerNames& rNames) const
	{
		// Need a configuration to transition.
		DataNode stateConfiguration;
		if (!m_StateMachineConfiguration.GetValueFromTable(m_StateMachineConfiguration.GetRootNode(), m_ActiveStateIdentifier, stateConfiguration))
		{
			return;
		}

		// No transitions array, can't transition on a trigger.
		DataNode transitions;
		if (!m_StateMachineConfiguration.GetValueFromTable(stateConfiguration, StateMachineCommon::kTransitionsTableEntry, transitions))
		{
			return;
		}
	
		UInt32 nNumberOfTransitions = 0u;
		(void)m_StateMachineConfiguration.GetArrayCount(transitions, nNumberOfTransitions);
		for (UInt32 i = 0u; i < nNumberOfTransitions; ++i)
		{
			DataNode transition;
			(void)m_StateMachineConfiguration.GetValueFromArray(transitions, i, transition);

			DataNode triggers;
			(void)m_StateMachineConfiguration.GetValueFromTable(transition, StateMachineCommon::kTriggersTableEntry, triggers);

			UInt32 uTriggers = 0u;
			(void)m_StateMachineConfiguration.GetArrayCount(triggers, uTriggers);
			for (UInt32 j = 0u; j < uTriggers; ++j)
			{
				DataNode value;
				(void)m_StateMachineConfiguration.GetValueFromArray(triggers, j, value);

				HString triggerName;
				(void)m_StateMachineConfiguration.AsString(value, triggerName);

				HString unusedStateIdentifier;
				if (!triggerName.IsEmpty() && CheckTransition(stateConfiguration, triggerName, transition, unusedStateIdentifier))
				{
					(void)rNames.Insert(triggerName);
				}
			}
		}	
	}

	/**
	 * Force an immediate transition to state stateIdentifier - ignores any
	 * transition conditions or triggers that would otherwise prevent the transition.
	 *
	 * @return True if the target state was activated, false otherwise.
	 */
	Bool GotoState(HString stateIdentifier)
	{
		return ActivateState(stateIdentifier);
	}

	/**
	 * Check if the trigger defined by triggerName will activate
	 * a state transition in this state machine. If so, populate 
	 * rTargetStateIdentifier with the target state name and 
	 * rActivatedTransition with the transition data, and return true.
	 *
	 * @return True if the trigger can activate a transition, false otherwise.
	 * If this method returns false, out values are left unmodified.
	 */
	Bool CheckTrigger(
		HString triggerName,
		HString& rTargetStateIdentifier,
		DataNode& rActivatedTransition,
		UInt32& ruActivatedTransitionIndex) const
	{
		// No transition possible if no currently active state.
		if (!m_pActiveState.IsValid())
		{
			return false;
		}

		// Need a configuration to transition.
		DataNode stateConfiguration;
		if (!m_StateMachineConfiguration.GetValueFromTable(m_StateMachineConfiguration.GetRootNode(), m_ActiveStateIdentifier, stateConfiguration))
		{
			return false;
		}

		// No local transitions array, can't transition on a local trigger.
		DataNode transitions;

		// Check global transitions first
		if (m_StateMachineConfiguration.GetValueFromTable(
			m_StateMachineConfiguration.GetRootNode(),
			StateMachineCommon::kTransitionsTableEntry,
			transitions))
		{
			if (CheckTransitions(
				stateConfiguration,
				triggerName,
				transitions,
				rTargetStateIdentifier,
				rActivatedTransition,
				ruActivatedTransitionIndex))
			{
				return true;
			}
		}

		// If no global, check local transitions
		if (m_StateMachineConfiguration.GetValueFromTable(stateConfiguration, StateMachineCommon::kTransitionsTableEntry, transitions))
		{
			if (CheckTransitions(
				stateConfiguration,
				triggerName,
				transitions,
				rTargetStateIdentifier,
				rActivatedTransition,
				ruActivatedTransitionIndex))
			{
				return true;
			}
		}

		return false;
	}

private:
	/**
	 * Basic activation check of a transition - if a candidate,
	 * the target state identifier is set to rTargetStateIdentifier and true
	 * is returned.
	 */
	Bool CheckCanActivateTransition(const DataNode& transition, HString& rTargetStateIdentifier) const
	{
		DataNode transitionTarget;
		(void)m_StateMachineConfiguration.GetValueFromTable(transition, StateMachineCommon::kTransitionTarget, transitionTarget);

		Byte const* s = nullptr;
		UInt32 u = 0u;
		HString targetName;
		if (m_StateMachineConfiguration.AsString(transitionTarget, s, u) &&
			HString::Get(targetName, s, u))
		{
			if (targetName != m_ActiveStateIdentifier)
			{
				rTargetStateIdentifier = targetName;
				return true;
			}
		}

		return false;
	}

	/**
	 * Evaluate all transitions. The first transition that is fulfilled by the current
	 * conditions and triggerName will be set to rTargetStateIdentifier and
	 * rActivatedTransition.
	 *
	 * @return True if a target transition is available, false otherwise.
	 */
	Bool CheckTransitions(
		const DataNode& stateConfiguration,
		HString triggerName,
		const DataNode& transitions,
		HString& rTargetStateIdentifier,
		DataNode& rActivatedTransition,
		UInt32& ruActivatedTransitionIndex) const
	{
		UInt32 nNumberOfTransitions = 0u;
		(void)m_StateMachineConfiguration.GetArrayCount(transitions, nNumberOfTransitions);

		for (UInt32 i = 0u; i < nNumberOfTransitions; ++i)
		{
			DataNode transition;
			(void)m_StateMachineConfiguration.GetValueFromArray(transitions, i, transition);

			if (CheckTransition(stateConfiguration, triggerName, transition, rTargetStateIdentifier))
			{
				rActivatedTransition = transition;
				ruActivatedTransitionIndex = i;
				return true;
			}
		}

		return false;
	}

	/**
	 * Evaluate transition - if all its conditions are currently fulfilled
	 * and it responds to triggerName (or triggerName is the empty string), the
	 * target state of the transition will be set to rTargetStateIdentifier and
	 * true will be returned.
	 *
	 * @return True if a target transition is available, false otherwise.
	 */
	Bool CheckTransition(
		const DataNode& stateConfiguration,
		HString triggerName,
		const DataNode& transition,
		HString& rTargetStateIdentifier) const
	{
		// Get the transition tag (may not be defined)
		DataNode tagNode;
		HString tag;
		(void)m_StateMachineConfiguration.GetValueFromTable(transition, StateMachineCommon::kTransitionTag, tagNode);
		{
			Byte const* s = nullptr;
			UInt32 u = 0u;
			if (m_StateMachineConfiguration.AsString(tagNode, s, u))
			{
				tag = HString(s, u);
			}
		}

		// Get the table used to enable/disable specific transitions for the current state configuration.
		DataNode enableTransitionsTable;
		(void)m_StateMachineConfiguration.GetValueFromTable(stateConfiguration, StateMachineCommon::kEnableTransitionsTableEntry, enableTransitionsTable);

		// Get whether the transition is enabled/disabled.
		DataNode enabled;
		(void)m_StateMachineConfiguration.GetValueFromTable(enableTransitionsTable, tag, enabled);

		// Skip the transition if it's explicitly disabled - in other cases (explicitly enabled or not specified),
		// evaluate the transition.
		if (enabled.IsBoolean() && !m_StateMachineConfiguration.AssumeBoolean(enabled))
		{
			return false;
		}

		DataNode conditions;
		(void)m_StateMachineConfiguration.GetValueFromTable(transition, StateMachineCommon::kConditionsTableEntry, conditions);

		DataNode negativeConditions;
		(void)m_StateMachineConfiguration.GetValueFromTable(transition, StateMachineCommon::kNegativeConditionsTableEntry, negativeConditions);

		if (AreConditionsTrue(conditions) &&
			AreConditionsFalse(negativeConditions))
		{
			DataNode triggers;
			(void)m_StateMachineConfiguration.GetValueFromTable(transition, StateMachineCommon::kTriggersTableEntry, triggers);

			// If the trigger name is not empty, then verify that the list of triggers
			// contains the specified trigger.
			if (!triggerName.IsEmpty())
			{
				if (!m_StateMachineConfiguration.ArrayContains(triggers, triggerName))
				{
					return false;
				}
			}
			// Otherwise, this is a conditions only activation check - activation can occur
			// if there are no triggers specified.
			else
			{
				UInt32 nConditionsArrayCount = 0u;
				(void)m_StateMachineConfiguration.GetArrayCount(conditions, nConditionsArrayCount);
				UInt32 nNegativeConditionsArrayCount = 0u;
				(void)m_StateMachineConfiguration.GetArrayCount(negativeConditions, nNegativeConditionsArrayCount);
				UInt32 nTriggersArrayCount = 0u;
				(void)m_StateMachineConfiguration.GetArrayCount(triggers, nTriggersArrayCount);

				// Can't activate if at least one trigger is specified - can only activate when a trigger is fired.
				if (triggers.IsArray() && nTriggersArrayCount > 0u)
				{
					return false;
				}
			}

			// Final check, return true if a candidate transition.
			if (CheckCanActivateTransition(transition, rTargetStateIdentifier))
			{
				return true;
			}
		}

		return false;
	}

	/**
	 * Activates a state transition - changes the currently activated state to the new state
	 * targetName.
	 *
	 * @return True if the state transition was successfully applied, false otherwise. If this
	 * method returns true, the state described by targetName is now the active state, otherwise
	 * the existing state will remain the active state.
	 */
	Bool ActivateState(HString targetName)
	{
		// Special case, go back to the "null" state.
		if (targetName.IsEmpty())
		{
			ScopedPtr<T> pState;
			pState.Swap(m_pActiveState);
			Seoul::Swap(m_ActiveStateIdentifier, targetName);

			if (pState.IsValid())
			{
				pState->ExitState(nullptr);
			}

			pState.Reset();
			return true;
		}

		ScopedPtr<T> pState(StateTraits<T>::NewState(*this, targetName));
		if (!pState.IsValid())
		{
			return false;
		}

		m_pActiveState.Swap(pState);
		Seoul::Swap(m_ActiveStateIdentifier, targetName);
		if (!m_pActiveState->EnterState(pState.Get()))
		{
			m_pActiveState.Swap(pState);
			Seoul::Swap(m_ActiveStateIdentifier, targetName);
			return false;
		}
		else
		{
			// Invoke ExitState if the outgoing state is defined.
			if (pState.IsValid())
			{
				pState->ExitState(m_pActiveState.Get());
			}

			// Destroy state.
			pState.Reset();

			// Invoke TransitionComplete if the incoming state is defined.
			if (m_pActiveState.IsValid())
			{
				m_pActiveState->TransitionComplete();
			}

			return true;
		}
	}

	/**
	 * @return True if all conditions listed in conditionsArray are set to true, false otherwise.
	 *
	 * This method returns true if conditionsArray is empty or not a valid array.
	 */
	Bool AreConditionsTrue(const DataNode& conditionsArray) const
	{
		UInt32 nConditions = 0u;
		m_StateMachineConfiguration.GetArrayCount(conditionsArray, nConditions);

		for (UInt32 i = 0u; i < nConditions; ++i)
		{
			DataNode condition;
			HString conditionName;
			m_StateMachineConfiguration.GetValueFromArray(conditionsArray, i, condition);

			Byte const* s = nullptr;
			UInt32 u = 0u;
			if (m_StateMachineConfiguration.AsString(condition, s, u))
			{
				HString conditionName;
				if (!HString::Get(conditionName, s, u))
				{
					 return false;
				}

				Bool bConditionValue = false;
				if (!m_tConditions.GetValue(conditionName, bConditionValue) ||
					!bConditionValue)
				{
					return false;
				}
			}
		}

		return true;
	}

	/**
	 * @return True if all conditions listed in conditionsArray are either unset, or
	 * set to false.
	 *
	 * This method returns true if conditionsArray is empty or not a valid array.
	 */
	Bool AreConditionsFalse(const DataNode& conditionsArray) const
	{
		UInt32 nConditions = 0u;
		m_StateMachineConfiguration.GetArrayCount(conditionsArray, nConditions);

		for (UInt32 i = 0u; i < nConditions; ++i)
		{
			DataNode condition;
			m_StateMachineConfiguration.GetValueFromArray(conditionsArray, i, condition);

			Byte const* s = nullptr;
			UInt32 u = 0u;
			HString conditionName;
			if (m_StateMachineConfiguration.AsString(condition, s, u) &&
				HString::Get(conditionName, s, u))
			{
				Bool bConditionValue = false;
				if (m_tConditions.GetValue(conditionName, bConditionValue) &&
					bConditionValue)
				{
					return false;
				}
			}
		}

		return true;
	}

	Conditions m_tConditions;

	struct ConditionEntry
	{
		ConditionEntry()
			: m_ConditionName()
			, m_bValue(false)
		{
		}

		ConditionEntry(HString name, Bool bValue)
			: m_ConditionName(name)
			, m_bValue(bValue)
		{
		}

		HString m_ConditionName;
		Bool m_bValue;
	};

	DataStore m_StateMachineConfiguration;
	ScopedPtr<T> m_pActiveState;
	HString m_ActiveStateIdentifier;
	HString m_Name;

	SEOUL_DISABLE_COPY(StateMachine);
}; // class StateMachine

} // namespace Seoul

#endif // include guard
