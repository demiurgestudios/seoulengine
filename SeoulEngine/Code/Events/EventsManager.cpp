/**
 * \file EventsManager.cpp
 * \brief EventsManager implements global event-based messaging.
 * Senders can register events and receivers can register callbacks
 * to establish a one-to-many signaling relationship.
 *
 * EventsManager is not thread-safe and all interactions
 * can only come from the main thread.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Core.h"
#include "EventsManager.h"
#include "Logger.h"

namespace Seoul::Events
{

void Manager::RegisterCallbackImpl(
	HString id,
	ArgumentSignature pSignature,
	const EventDelegate& delegate,
	Bool bBoolReturn)
{
	SEOUL_ASSERT(IsMainThread());

	// Acquire the event.
	auto& evt = GetOrCreateEvent(id);
	// If no signature on the event, we're the first callback
	// ever registered, so configure the signature now.
	if (nullptr == evt.m_pSignature)
	{
		evt.m_Name = id;
		evt.m_pSignature = pSignature;
	}
	// Verify signature consistency.
	SEOUL_ASSERT(pSignature == evt.m_pSignature);
	if (pSignature != evt.m_pSignature)
	{
		return;
	}

	// TODO: Remove this limitation.
	//
	// Callback registration disallowed if event is being triggered.
	SEOUL_ASSERT(0 == evt.m_InTriggerCount);
	if (0 != evt.m_InTriggerCount)
	{
		return;
	}

	// Find a slot for the callback.
	for (auto& callback : evt.m_vCallbacks)
	{
		// Find a free slot, fill it.
		if (!callback.m_Delegate)
		{
			callback.m_Delegate = delegate;
			callback.m_bBoolReturn = bBoolReturn;
			return;
		}
	}

	// Insert a new callback.
	evt.m_vCallbacks.PushBack({ delegate, bBoolReturn });
}

void Manager::UnregisterCallbackImpl(HString id, const EventDelegate& delegate)
{
	SEOUL_ASSERT(IsMainThread());

	// Early out if no callbacks.
	auto pEvent = GetEvent(id);
	if (!pEvent)
	{
		return;
	}

	// Search for the corresponding delegate.
	for (auto& callback : pEvent->m_vCallbacks)
	{
		if (callback.m_Delegate == delegate)
		{
			callback = Callback{};
			return;
		}
	}
}

Manager::Manager()
{
	SEOUL_ASSERT(IsMainThread());
}

Manager::~Manager()
{
	SEOUL_ASSERT(IsMainThread());

	SafeDeleteTable(m_tEvents);
}

} // namespace Seoul::Events
