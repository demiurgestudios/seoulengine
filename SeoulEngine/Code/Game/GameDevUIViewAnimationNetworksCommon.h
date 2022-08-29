/**
 * \file GameDevUIViewAnimationNetworksCommon.h
 * \brief Shared functionality between 2D and 3D network visualization.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef GAME_DEV_UI_VIEW_ANIMATION_NETWORKS_COMMON_H
#define GAME_DEV_UI_VIEW_ANIMATION_NETWORKS_COMMON_H

#include "Prereqs.h"

#if (SEOUL_ENABLE_DEV_UI && (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D) && !SEOUL_SHIP)

namespace Seoul
{

struct AnimationNetworkSorter SEOUL_SEALED
{
	Bool operator()(const SharedPtr<Animation::NetworkInstance>& pA, const SharedPtr<Animation::NetworkInstance>& pB) const
	{
		return (strcmp(
			pA->GetNetworkHandle().GetKey().GetRelativeFilenameWithoutExtension().CStr(),
			pB->GetNetworkHandle().GetKey().GetRelativeFilenameWithoutExtension().CStr()) < 0);
	}
}; // struct AnimationNetworkSorter

struct LexographicalSorter SEOUL_SEALED
{
	Bool operator()(HString a, HString b) const
	{
		return (strcmp(a.CStr(), b.CStr()) < 0);
	}
}; // struct LexographicalSorter

struct ConditionEntry SEOUL_SEALED
{
	ConditionEntry(HString name = HString(), Bool bValue = false)
		: m_Name(name)
		, m_bValue(bValue)
	{
	}

	Bool operator<(const ConditionEntry& b) const
	{
		return (strcmp(m_Name.CStr(), b.m_Name.CStr()) < 0);
	}

	HString m_Name;
	Bool m_bValue;
}; // struct ConditionEntry
typedef Vector<ConditionEntry, MemoryBudgets::Animation> Conditions;

struct ParameterEntry SEOUL_SEALED
{
	ParameterEntry(HString name = HString(), Float32 fValue = 0.0f)
		: m_Name(name)
		, m_fValue(fValue)
	{
	}

	Bool operator<(const ParameterEntry& b) const
	{
		return (strcmp(m_Name.CStr(), b.m_Name.CStr()) < 0);
	}

	HString m_Name;
	Float32 m_fValue;
}; // struct ParameterEntry
typedef Vector<ParameterEntry, MemoryBudgets::Animation> Parameters;
typedef HashSet<HString, MemoryBudgets::Animation> ViableTriggers;

static Bool HStringVectorGetter(void* pData, Int iIndex, Byte const** psOut)
{
	auto const&v = *((Vector<HString, MemoryBudgets::DevUI>*)pData);

	if (iIndex >= 0 && (UInt32)iIndex < v.GetSize())
	{
		*psOut = v[iIndex].CStr();
		return true;
	}

	return false;
}

static void GatherTriggers(const SharedPtr<Animation::NodeInstance>& p, ViableTriggers& r)
{
	using namespace Animation;

	switch (p->GetType())
	{
	case NodeType::kBlend:
		GatherTriggers(((BlendInstance*)p.GetPtr())->GetChildA(), r);
		GatherTriggers(((BlendInstance*)p.GetPtr())->GetChildB(), r);
		return;

	case NodeType::kPlayClip:
		// Nop
		return;

	case NodeType::kStateMachine:
	{
		SharedPtr<StateMachineInstance> ps((StateMachineInstance*)p.GetPtr());
		ps->GetViableTriggers(r);
		if (ps->GetNew().IsValid())
		{
			GatherTriggers(ps->GetNew(), r);
		}
	}
	return;

	default:
		SEOUL_FAIL("Programmer error, out-of-sync enum.");
		return;
	};
}

} // namespace Seoul

#endif // /#if (SEOUL_ENABLE_DEV_UI && (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D) && !SEOUL_SHIP)

#endif // include guard
