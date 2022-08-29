/**
 * \file AnimationStateMachineDefinition.cpp
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

#include "AnimationStateMachineDefinition.h"
#include "AnimationStateMachineInstance.h"
#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionDefine.h"

#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)

namespace Seoul
{

SEOUL_SPEC_TEMPLATE_TYPE(HashTable<HString, Animation::StateMachineState, 1, DefaultHashTableKeyTraits<HString>>)
SEOUL_SPEC_TEMPLATE_TYPE(Vector<Animation::StateMachineTransition, 1>)
SEOUL_BEGIN_TYPE(Animation::StateMachineTransition)
	SEOUL_PROPERTY_N("Conditions", m_vConditions)
	SEOUL_PROPERTY_N("NegativeConditions", m_vNegativeConditions)
	SEOUL_PROPERTY_N("Triggers", m_Triggers)
	SEOUL_PROPERTY_N("Time", m_fTimeInSeconds)
	SEOUL_PROPERTY_N("Target", m_Target)
	SEOUL_PROPERTY_N("OverrideDefaultState", m_sOverrideDefaultState)
	SEOUL_PROPERTY_N("SlotBlendMode", m_eSlotBlendMode)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(Animation::StateMachineState)
	SEOUL_PROPERTY_N("Child", m_pChild)
	SEOUL_PROPERTY_N("Transitions", m_vTransitions)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(Animation::StateMachineDefinition, TypeFlags::kDisableCopy)
	SEOUL_TYPE_ALIAS("AnimStateMachine")
	SEOUL_PARENT(Animation::NodeDefinition)
	SEOUL_PROPERTY_N("DefaultState", m_DefaultState)
	SEOUL_PROPERTY_N("States", m_tStates)
SEOUL_END_TYPE()

namespace Animation
{

StateMachineDefinition::StateMachineDefinition()
	: m_tStates()
	, m_DefaultState()
{
}

StateMachineDefinition::~StateMachineDefinition()
{
}

NodeInstance* StateMachineDefinition::CreateInstance(NetworkInstance& r, const NodeCreateData& creationData) const
{
	return SEOUL_NEW(MemoryBudgets::Animation) StateMachineInstance(r, SharedPtr<StateMachineDefinition const>(this), creationData);
}

} // namespace Animation

} // namespace Seoul

#endif // /#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)
