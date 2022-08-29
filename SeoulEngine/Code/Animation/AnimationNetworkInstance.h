/**
 * \file AnimationNetworkInstance.h
 * \brief Instantiation of a network graph at runtime. You need
 * a network instance to play back an animation network. The structure
 * of the network is defined by an Animation::NetworkDefinition instance.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ANIMATION_NETWORK_INSTANCE_H
#define ANIMATION_NETWORK_INSTANCE_H

#include "AnimationNetworkDefinition.h"
#include "Delegate.h"
#include "HashTable.h"
#include "Prereqs.h"
#include "SeoulTime.h"
#include "SharedPtr.h"
#include "Vector.h"
namespace Seoul { namespace Animation { class BlendDefinition; } }
namespace Seoul { namespace Animation { struct ClipSettings; } }
namespace Seoul { namespace Animation { class IData; } }
namespace Seoul { namespace Animation { class IState; } }
namespace Seoul { namespace Animation { class EventInterface; } }
namespace Seoul { namespace Animation { class Manager; } }
namespace Seoul { namespace Animation { class NodeInstance; } }
namespace Seoul { namespace Animation { class PlayClipDefinition; } }

#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)

namespace Seoul::Animation
{

class NetworkInstance SEOUL_ABSTRACT
{
public:
	typedef HashTable<HString, Bool, MemoryBudgets::Animation> Conditions;
	typedef HashTable<HString, Float32, MemoryBudgets::Animation> Parameters;
	typedef Vector<HString, MemoryBudgets::Animation> Triggers;

	virtual ~NetworkInstance();

	/**
	 * Accumulate time offset. This will be applied during the next Update()
	 * call. One use case is to slightly offset animations to avoid perfect
	 * synchronization between clones of the same network and definition.
	 */
	void AddTimestepOffset(Float fTimestepOffset)
	{
		m_fTimestepOffset += fTimestepOffset;
	}

	// Call to manually prepare this network instance. Normally called as part of
	// Update(). May be necessary if you want to leave a network instance in its
	// default state (t-pose) without applying any animations from its network.
	Bool CheckState();

	// Create a new instance of this NetworkInstance, initialized to its current
	// state.
	NetworkInstance* Clone() const;

	// Condition variable access. Conditions control any state machines
	// in this network.
	const Conditions& GetConditions() const { return  m_tConditions; }
	Bool GetCondition(HString name) const
	{
		Bool b = false;
		(void)m_tConditions.GetValue(name, b);
		return b;
	}

	// Return the current animation clip duration/last time key.
	Float GetCurrentMaxTime() const;

	// returns true if the animation event was found after the current animation time, and sets the time to event
	// returns false if the animation event was not found
	// Note: This does not account for animation blends or state machine transitions,
	// since these may change dynamically and are impossible to predict correctly.
	Bool GetTimeToEvent(HString sEventName, Float& fTimeToEvent) const;

	// DataDefinition references of this network. Safe to access at any time.
	const IData& GetDataInterface() const { return *m_pDataInterface; }
	const AnimationNetworkContentHandle& GetNetworkHandle() const { return m_hNetwork; }

	// Return the event interface originally associated with this network.
	const SharedPtr<EventInterface>& GetEventInterface() const { return m_pEventInterface; }

	// Instance data of this network. It is only safe to access these members when IsReady()
	// is true.
	const SharedPtr<NetworkDefinition const>& GetNetwork() const {return m_pNetwork; }

	// Parameters of this network. Parameters are used by blend nodes to define
	// mix amount.
	const Parameters& GetParameters() const { return  m_tParameters; }
	Float32 GetParameter(HString name) const
	{
		Float32 f = 0.0f;
		(void)m_tParameters.GetValue(name, f);
		return f;
	}

	// Root node of this network. It is only safe to access this member when IsReady()
	// is true.
	const SharedPtr<NodeInstance>& GetRoot() const { SEOUL_ASSERT(IsReady()); return m_pRoot; }

	// Animation data state. Defines the current pose of an animation entity. It is only safe to
	// access this member when IsReady() is true.
	const IState& GetStateInterface() const { SEOUL_ASSERT(IsReady()); return *m_pState; }
	IState& GetStateInterface() { SEOUL_ASSERT(IsReady()); return *m_pState; }

	// If all active play clips are finished (one-offs that have reached their end).
	void AllDonePlaying(Bool& rbDone, Bool& rbLooping) const;

	// Any state machine in this network is actively tweening between states.
	Bool IsInStateTransition() const;

	// Animation network data is asynchronously loaded. A network is not fully initialized
	// until this method returns true.
	Bool IsReady() const;

	/**
	 * Update a condition variable. These control state machines in the network.
	 */
	void SetCondition(HString name, Bool bValue)
	{
		(void)m_tConditions.Overwrite(name, bValue);
	}

	/**
	 * Update a parameter. These control blend nodes in the network.
	 */
	void SetParameter(HString name, Float32 fValue)
	{
		(void)m_tParameters.Overwrite(name, fValue);
	}

	/**
	 * Enqueue a state machine transition. These affect all state machines in the
	 * network.
	 */
	void TriggerTransition(HString name)
	{
		m_vTriggers.PushBack(name);
	}

	// Per-frame advancement of a network state.
	void Tick(Float fDeltaTimeInSeconds);

#if SEOUL_HOT_LOADING
	Atomic32Type GetLoadDataCount() const
	{
		return m_LoadDataCount;
	}

	Atomic32Type GetLoadNetworkCount() const
	{
		return m_LoadNetworkCount;
	}
#endif

	virtual NodeInstance* CreatePlayClipInstance(
		const SharedPtr<PlayClipDefinition const>& pDef,
		const ClipSettings& settings) = 0;

protected:
	SEOUL_REFERENCE_COUNTED(NetworkInstance);

	NetworkInstance(
		const AnimationNetworkContentHandle& hNetwork,
		IData* pDataInterface,
		const SharedPtr<EventInterface>& pEventInterface);

	virtual IState* CreateState() = 0;
	virtual NetworkInstance* CreateClone() const = 0;

private:
	SharedPtr<EventInterface> const m_pEventInterface;
	AnimationNetworkContentHandle const m_hNetwork;
	SharedPtr<NetworkDefinition const> m_pNetwork;
	ScopedPtr<IData> m_pDataInterface;
	Conditions m_tConditions;
	Parameters m_tParameters;
	ScopedPtr<IState> m_pState;
	SharedPtr<NodeInstance> m_pRoot;
	Triggers m_vTriggers;
	Float m_fTimestepOffset;

	void InternalDestroy();

#if SEOUL_HOT_LOADING
	Atomic32Type m_LoadDataCount;
	Atomic32Type m_LoadNetworkCount;
#endif

	SEOUL_DISABLE_COPY(NetworkInstance);
}; // class NetworkInstance

} // namespace Seoul::Animation

#endif // /#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)

#endif // include guard
