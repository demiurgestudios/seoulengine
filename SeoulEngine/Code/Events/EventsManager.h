/**
 * \file EventsManager.h
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

#pragma once
#ifndef EVENTS_MANAGER_H
#define EVENTS_MANAGER_H

#include "Atomic32.h"
#include "Delegate.h"
#include "HashTable.h"
#include "Prereqs.h"
#include "ScopedIncrement.h"
#include "SeoulHString.h"
#include "Singleton.h"
#include "ThreadId.h"
#include "Vector.h"

namespace Seoul::Events
{

/**
 * Events::Manager class
 *
 * The Events::Manager class is a singleton object which manages global events.
 * Multiple callbacks can be registered against an HString identifier (so long
 * as they have identical function signature). Triggering an event will dispatch
 * to all registered callbacks.
 *
 * \warning Events::Manager can only be used from the main thread.
 */
class Manager SEOUL_SEALED : public Singleton<Manager>
{
private:
	template <typename... ARGS>
	static void ArgumentSignatureHook()
	{
	}
	typedef void (*ArgumentSignature)();

	// Base Delegate type, will be reinterpret casted to the appropriate type on Trigger.
	typedef Delegate<Bool()> EventDelegate;

	// Concrete private function called by the templated RegisterCallback functions.
	void RegisterCallbackImpl(
		HString id,
		ArgumentSignature pSignature,
		const EventDelegate& delegate,
		Bool bBoolReturn);
	// Concrete private function called by the tempalted UnregisterCallback functions.
	void UnregisterCallbackImpl(
		HString id,
		const EventDelegate& delegate);

	/** Stored information about a callback. */
	struct Callback SEOUL_SEALED
	{
		EventDelegate m_Delegate;
		Bool m_bBoolReturn = false;
	};

	/** Stored information about an event. */
	struct Event SEOUL_SEALED
	{
		typedef Vector<Callback, MemoryBudgets::Event> Callbacks;

		/**
		 * Array of callbacks. Can contain "holes" (invalid delegates) when callbacks are unregistered.
		 */
		Callbacks m_vCallbacks;

		/** Unique name of the event. */
		HString m_Name;

		/** Signature of the event. */
		ArgumentSignature m_pSignature = nullptr;

		/** Used to prevent certain mutations of the event while triggering is active. */
		Atomic32 m_InTriggerCount;

		/** True if the event is currently enabled, false otherwise. */
		Bool m_bEnabled = true;
	};

	typedef HashTable<HString, CheckedPtr<Event>, MemoryBudgets::Event> Events;

	/** Overall table of events. */
	Events m_tEvents;

	/** @return a pointer to the corresponding event - nullptr if not currently registered. */
	CheckedPtr<Event const> GetEvent(HString id) const
	{
		return const_cast<Manager*>(this)->GetEvent(id);
	}

	/** @return a pointer to the corresponding event - nullptr if not currently registered. */
	CheckedPtr<Event> GetEvent(HString id)
	{
		SEOUL_ASSERT(IsMainThread());

		auto pp = m_tEvents.Find(id);
		if (pp)
		{
			return *pp;
		}
		else
		{
			return nullptr;
		}
	}

	/** @return a reference to the event - created if no existing entry. */
	Event& GetOrCreateEvent(HString id)
	{
		SEOUL_ASSERT(IsMainThread());

		auto p = GetEvent(id);
		if (!p)
		{
			p = SEOUL_NEW(MemoryBudgets::Event) Event;
			SEOUL_VERIFY(m_tEvents.Insert(id, p).Second);
		}

		return *p;
	}

public:
	Manager();
	~Manager();

	/**
	 * @return true if an event will dispatch to callbacks, false otherwise.
	 *
	 * \note An event that has not yet been implicitly registered (due to
	 * no registered callbacks) will return true/is considered enabled
	 * by default.
	 */
	Bool IsEventEnabled(HString id) const
	{
		SEOUL_ASSERT(IsMainThread());

		auto pEvent = GetEvent(id);
		if (!pEvent)
		{
			return true;
		}

		return pEvent->m_bEnabled;
	}

	/**
	 * Configure whether an event is enabled or not - when disabled,
	 * TriggerEvent() will not dispatch to any callbacks.
	 */
	void SetEventEnabled(HString id, Bool bEnabled)
	{
		SEOUL_ASSERT(IsMainThread());

		auto& evt = GetOrCreateEvent(id);
		evt.m_bEnabled = bEnabled;
	}

	/**
	 * Move the last entry in the associated event callbacks to the front of the array.
	 */
	void MoveLastCallbackToFirst(HString id)
	{
		SEOUL_ASSERT(IsMainThread());

		auto& callbacks = GetOrCreateEvent(id).m_vCallbacks;
		if (callbacks.IsEmpty())
		{
			return;
		}

		auto const callback = callbacks.Back();
		callbacks.PopBack();
		callbacks.Insert(callbacks.Begin(), callback);
	}

	/**
	 * Registers a callback delegate for a given event.
	 *
	 * @param[in] id       Unique identifier of the event.
	 * @param[in] delegate Delegate to invoke.
	 *
	 * \warning The signature of callback must match any previously registered
	 * callbacks for the \a id event.
	 */
	template <typename... ARGS>
	void RegisterCallback(HString id, const Delegate<void(ARGS...)>& delegate)
	{
		SEOUL_ASSERT(IsMainThread());

		auto const& eventDelegate = *reinterpret_cast<EventDelegate const*>(&delegate);
		RegisterCallbackImpl(
			id,
			&ArgumentSignatureHook<ARGS...>,
			eventDelegate,
			false /* bBoolReturn */);
	}

	/**
	 * Registers a callback delegate for a given event.
	 *
	 * @param[in] id       Unique identifier of the event.
	 * @param[in] delegate Delegate to invoke.
	 *
	 * \warning The signature of callback must match any previously registered
	 * callbacks for the \a id event.
	 */
	template <typename... ARGS>
	void RegisterCallback(HString id, const Delegate<Bool(ARGS...)>& delegate)
	{
		SEOUL_ASSERT(IsMainThread());

		auto const& eventDelegate = *reinterpret_cast<EventDelegate const*>(&delegate);
		RegisterCallbackImpl(
			id,
			&ArgumentSignatureHook<ARGS...>,
			eventDelegate,
			true /* bBoolReturn */);
	}

	/**
	 * Triggers an event with a set of arguments.
	 *
	 * @param[in] id   Event ID of the event being triggered.
	 * @param[in] args Arguments to pass to the invoked callbacks.
	 *
	 * \warning The signature of args must exactly match all previously
	 * registered callbacks for this event.
	 */
	template <typename... ARGS>
	Bool TriggerEvent(HString id, ARGS... args) const
	{
		SEOUL_ASSERT(IsMainThread());

		// Return false if no event (no callbacks).
		auto pEvent = GetEvent(id);
		if (!pEvent)
		{
			return false;
		}

		// Return false if the event is not enabled.
		if (!pEvent->m_bEnabled)
		{
			return false;
		}

		// In triggering of this event for the remainder of this method.
		ScopedIncrement<Atomic32> scoped(const_cast<Atomic32&>(pEvent->m_InTriggerCount));

		// Fail in non-ship on signature mismatch (otherwise, just return false in
		// shipping builds).
		SEOUL_ASSERT(&ArgumentSignatureHook<ARGS...> == pEvent->m_pSignature);
		if (&ArgumentSignatureHook<ARGS...> != pEvent->m_pSignature)
		{
			return false;
		}

		auto const& callbacks = pEvent->m_vCallbacks;
		for (auto const& eventCallback : callbacks)
		{
			// May be invalid if unregistered.
			if (!eventCallback.m_Delegate)
			{
				continue;
			}

			// Invoke based on return type.
			if (eventCallback.m_bBoolReturn)
			{
				auto const& callback = *reinterpret_cast<Delegate<Bool(ARGS...)> const*>(&eventCallback.m_Delegate);
				if (callback(args...))
				{
					return true;
				}
			}
			else
			{
				auto const& callback = *reinterpret_cast<Delegate<void(ARGS...)> const*>(&eventCallback.m_Delegate);
				callback(args...);
			}
		}

		return false;
	}

	/**
	 * Unregisters a callback delegate for a given event.
	 *
	 * @param[in] id       Unique identifier of the event.
	 * @param[in] delegate Delegate to unregister.
	 */
	template <typename R, typename... ARGS>
	void UnregisterCallback(HString id, const Delegate<R(ARGS...)>& delegate)
	{
		SEOUL_ASSERT(IsMainThread());

		auto const& eventDelegate = *reinterpret_cast<EventDelegate const*>(&delegate);
		UnregisterCallbackImpl(id, eventDelegate);
	}
}; // class Events::Manager

} // namespace Seoul::Events

#endif // include guard
