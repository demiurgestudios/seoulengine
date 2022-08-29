/**
 * \file AnimationStateMachineDefinition.h
 * \brief Defines a state machine node in an animation graph. This is read-only
 * data at runtime. To evaluate a state machine node, you must instantiate
 * an Animation::StateMachineInstance, which will normally occur as part
 * of creating an Animation::NetworkInstance.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ANIMATION_STATE_MACHINE_DEFINITION_H
#define ANIMATION_STATE_MACHINE_DEFINITION_H

#include "AnimationNodeDefinition.h"
#include "AnimationSlotBlendMode.h"
#include "HashSet.h"
#include "HashTable.h"
#include "Vector.h"

#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)

namespace Seoul::Animation
{

/** Defines an edge on the graph formed by the state machine states. */
struct StateMachineTransition SEOUL_SEALED
{
	typedef Vector<HString, MemoryBudgets::Animation> Conditions;
	typedef Vector<HString, MemoryBudgets::Animation> NegativeConditions;
	typedef HashSet<HString, MemoryBudgets::Animation> Triggers;

	StateMachineTransition()
		: m_vConditions()
		, m_vNegativeConditions()
		, m_Triggers()
		, m_fTimeInSeconds(0.0f)
		, m_Target()
		, m_sOverrideDefaultState()
		, m_eSlotBlendMode(Animation::SlotBlendMode::kNone)
	{
	}

	Conditions m_vConditions;
	NegativeConditions m_vNegativeConditions;
	Triggers m_Triggers;
	Float32 m_fTimeInSeconds;
	HString m_Target;
	HString m_sOverrideDefaultState;
	Animation::SlotBlendMode m_eSlotBlendMode;
}; // struct StateMachineTransition

/** Defines a single discrete state in the state machine. */
struct StateMachineState SEOUL_SEALED
{
	typedef Vector<StateMachineTransition, MemoryBudgets::Animation> Transitions;

	SharedPtr<NodeDefinition> m_pChild;
	Transitions m_vTransitions;
}; // struct StateMachineState

/**
 * A state machine node in an animation network. Allows controlled transitions
 * between various animation clips.
 */
class StateMachineDefinition SEOUL_SEALED : public NodeDefinition
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(StateMachineDefinition);

	typedef HashTable<HString, StateMachineState, MemoryBudgets::Animation> States;

	StateMachineDefinition();
	~StateMachineDefinition();

	virtual NodeInstance* CreateInstance(NetworkInstance& r, const NodeCreateData& creationData) const SEOUL_OVERRIDE;
	virtual NodeType GetType() const SEOUL_OVERRIDE { return NodeType::kStateMachine; }

	HString GetDefaultState() const { return m_DefaultState; }
	const States& GetStates() const { return m_tStates; }

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(StateMachineDefinition);
	SEOUL_REFLECTION_FRIENDSHIP(StateMachineDefinition);

	States m_tStates;
	HString m_DefaultState;

	SEOUL_DISABLE_COPY(StateMachineDefinition);
}; // class StateMachineDefinition

} // namespace Seoul::Animation

#endif // /#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)

#endif // include guard
