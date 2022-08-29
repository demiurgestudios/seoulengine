/**
 * \file AnimationNetworkInstance.cpp
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

#include "AnimationEventInterface.h"
#include "AnimationIData.h"
#include "AnimationIState.h"
#include "AnimationNetworkDefinition.h"
#include "AnimationNetworkInstance.h"
#include "AnimationNodeDefinition.h"
#include "AnimationNodeInstance.h"

#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)

namespace Seoul::Animation
{

NetworkInstance::NetworkInstance(
	const AnimationNetworkContentHandle& hNetwork,
	IData* pDataInterface,
	const SharedPtr<EventInterface>& pEventInterface)
	: m_pEventInterface(pEventInterface)
	, m_hNetwork(hNetwork)
	, m_pNetwork()
	, m_pDataInterface(pDataInterface)
	, m_tConditions()
	, m_tParameters()
	, m_pState()
	, m_pRoot()
	, m_vTriggers()
	, m_fTimestepOffset(0.0f)
#if SEOUL_HOT_LOADING
	, m_LoadDataCount(0)
	, m_LoadNetworkCount(0)
#endif
{
}

NetworkInstance::~NetworkInstance()
{
	InternalDestroy();
}

/**
 * Call to manually prepare this network instance. Normally called as part of
 * Tick(). May be necessary if you want to leave a network instance in its
 * default state (t-pose) without applying any animations from its network.
 */
Bool NetworkInstance::CheckState()
{
	// TODO: Smooth out this hot load, so we carry over any state that we can.
#if SEOUL_HOT_LOADING
	// A change in load count indicates a new to (re)load the data.
	if (m_LoadDataCount != m_pDataInterface->GetTotalLoadsCount() ||
		m_LoadNetworkCount != m_hNetwork.GetTotalLoadsCount())
	{
		InternalDestroy();
		m_LoadDataCount = m_pDataInterface->GetTotalLoadsCount();
		m_LoadNetworkCount = m_hNetwork.GetTotalLoadsCount();
	}
#endif // /#if SEOUL_HOT_LOADING

	// If we haven't acquired a network yet, try to do so now.
	if (!m_pNetwork.IsValid())
	{
		if (!m_hNetwork.IsLoading())
		{
			// Acquire the network.
			m_pNetwork = m_hNetwork.GetPtr();
			if (m_pNetwork.IsValid())
			{
				// Apply default parameters and conditions.
				{
					auto const iBegin = m_pNetwork->GetConditions().Begin();
					auto const iEnd = m_pNetwork->GetConditions().End();
					for (auto i = iBegin; iEnd != i; ++i)
					{
						// Insert so we don't overwrite an existing value.
						(void)m_tConditions.Insert(i->First, i->Second);
					}
				}
				{
					auto const iBegin = m_pNetwork->GetParameters().Begin();
					auto const iEnd = m_pNetwork->GetParameters().End();
					for (auto i = iBegin; iEnd != i; ++i)
					{
						// Insert so we don't overwrite an existing value.
						(void)m_tParameters.Insert(i->First, i->Second.m_fDefault);
					}
				}
			}
		}
	}

	// If we have a network, check if we need data.
	if (m_pNetwork.IsValid())
	{
		if (!m_pDataInterface->HasInstance())
		{
			if (!m_pDataInterface->IsLoading())
			{
				m_pDataInterface->AcquireInstance();
			}
		}
	}

	// If we have a network and data, check if we need to initialize state.
	if (m_pNetwork.IsValid() && m_pDataInterface->HasInstance())
	{
		if (!m_pState.IsValid())
		{
			m_pRoot.Reset();
			m_pState.Reset(CreateState());
		}
		if (!m_pRoot.IsValid())
		{
			m_pRoot.Reset(m_pNetwork->GetRoot()->CreateInstance(*this, NodeCreateData()));
		}
	}

	return (m_pState.IsValid() && m_pRoot.IsValid());
}

/**
 * Make a full copy of this network, with current conditions and parameters.
 */
NetworkInstance* NetworkInstance::Clone() const
{
	auto p = CreateClone();
	p->m_tConditions = m_tConditions;
	p->m_tParameters = m_tParameters;
	return p;
}

/**
 * @return The current animation clip duration. This is a combination of
 * the network state as follows:
 * - anim clip returns its duration.
 * - blend nodes return the max of its children.
 * - state machine nodes return their new node duration.
 *
 * Nodes that cannot return a measured value return 0.0.
 */
Float NetworkInstance::GetCurrentMaxTime() const
{
	if (m_pRoot.IsValid())
	{
		return m_pRoot->GetCurrentMaxTime();
	}

	return 0.0f;
}

Bool NetworkInstance::GetTimeToEvent(HString sEventName, Float& fTimeToEvent) const
{
	if (m_pRoot.IsValid())
	{
		return m_pRoot->GetTimeToEvent(sEventName, fTimeToEvent);
	}

	return false;
}

/**
 * If all active play clips are finished (one-offs that have reached their end).
 */
void NetworkInstance::AllDonePlaying(Bool& rbDone, Bool& rbLooping) const
{
	if (m_pRoot.IsValid())
	{
		m_pRoot->AllDonePlaying(rbDone, rbLooping);
	}
	else
	{
		rbDone = true;
		rbLooping = false;
	}
}

/**
 * Any state machine in this network is actively tweening between states.
 */
Bool NetworkInstance::IsInStateTransition() const
{
	if (m_pRoot.IsValid())
	{
		return m_pRoot->IsInStateTransition();
	}

	return false;
}

/**
 * Animation network data is asynchronously loaded. A network is not fully initialized
 * until this method returns true.
 */
Bool NetworkInstance::IsReady() const
{
	return (m_pDataInterface->HasInstance() && m_pState.IsValid());
}

/**
 * Per-frame advancement of the network. Performs lazy loading, applies triggers, and
 * updates the network (which applies any animations to its state).
 */
void NetworkInstance::Tick(Float fDeltaTimeInSeconds)
{
	// Setup and make sure we have state.
	if (!CheckState())
	{
		return;
	}

	// Apply triggers to root.
	{
		for (auto i = m_vTriggers.Begin(); m_vTriggers.End() != i; ++i)
		{
			m_pRoot->TriggerTransition(*i);
		}
		m_vTriggers.Clear();
	}

	if (m_fTimestepOffset > 0.0f)
	{
		fDeltaTimeInSeconds += m_fTimestepOffset * GetCurrentMaxTime();
		m_fTimestepOffset = 0.0f;
	}

	// Tick root.
	Bool bTick = false;
	{
		bTick = m_pRoot->Tick(fDeltaTimeInSeconds, 1.0f, true);
	}

	// On changes, tick the state.
	if (bTick)
	{
		// Tick stateful data.
		m_pState->Tick(fDeltaTimeInSeconds);
	}

	// Tick the event interface, if defined.
	if (m_pEventInterface.IsValid())
	{
		m_pEventInterface->Tick(fDeltaTimeInSeconds);
	}
}

void NetworkInstance::InternalDestroy()
{
	m_pRoot.Reset();
	m_pState.Reset();
	m_pDataInterface->ReleaseInstance();
	m_pNetwork.Reset();
}

} // namespace Seoul::Animation

#endif // /#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)
