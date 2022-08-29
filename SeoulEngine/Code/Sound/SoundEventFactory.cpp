/**
 * \file SoundEventFactory.cpp
 * \brief Utility class which handles loading and playback of sound events
 * by HString identifier. It is not necessary to use this class to use Sound::Events,
 * it is provided as a convenience when you want more flexibility regarding
 * Sound::Event lifespan.
 *
 * "Tracked" sound events are sound events for which the handle persists,
 * and the particular instance of the sound event can be manipulated after the event
 * has started. You want to use a tracked sound event for looping events,
 * events with keys, parameters, or an event that you want to stop at a specific
 * time.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Logger.h"
#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionDefine.h"
#include "ReflectionDeserialize.h"
#include "SoundEventFactory.h"
#include "SoundManager.h"

namespace Seoul
{

namespace Sound
{

/** Utility, handles queueing and starting of sound events in the factory. */
FactoryEntry::State FactoryEntry::Poll()
{
	// If already started, check whether we're finished playing or not.
	if (m_bStarted)
	{
		// If the sound event is playing, just return kPlaying.
		if (m_pSoundEvent->IsPlaying())
		{
			return kPlaying;
		}
		// If the sound event is not currently playing, check various other
		// state to decide whether we should restart it or not.
		else
		{
			// If the sound event is considered a looping event (kStopOnDestruction
			// is set), check if it's loading, and if so, reset m_bStarted, to
			// support hot loading.
			if (m_pSoundEvent->StopOnDestruction())
			{
				// Actively loading, reset m_bStarted and return
				// kWaitingToStart.
				if (m_pSoundEvent->IsLoading())
				{
					m_bStarted = false;
					return kWaitingToStart;
				}
			}

			// Otherwise, just return "finished playing".
			return kFinishedPlaying;
		}
	}
	else
	{
		// Otherwise, try to start, if the event is not actively loading.
		if (!m_pSoundEvent->IsLoading())
		{
			// Compute an offset if m_iStartTimeInTicks >= 0. If it is not,
			// then this is the first attempt and we want no offset.
			Int32 iOffset = 0;
			if (m_iStartTimeInTicks >= 0)
			{
				iOffset = (Int32)SeoulTime::ConvertTicksToMilliseconds(
					SeoulTime::GetGameTimeInTicks() - m_iStartTimeInTicks);
			}

			m_bStarted = m_pSoundEvent->Start(
				m_vPosition,
				m_vVelocity,
				m_bStopOnDestruction,
				iOffset);
		}

		// Now return the correct code based on the state of m_bStarted.
		return (m_bStarted ? kPlaying : kWaitingToStart);
	}
}

EventFactory::EventFactory()
	: m_iNextTrackedEventId(0)
{
}

EventFactory::~EventFactory()
{
	ResetSoundDuckers();
	SafeDeleteVector(m_vUnnamedSoundEvents);
	SafeDeleteTable(m_tTrackedSoundEvents);
	SafeDeleteTable(m_tCachedSoundEvents);
	m_tSoundEvents.Clear();
}

/**
 * Utility used to add additional sound events to the factory beyond
 * the initial configuration.
 */
void EventFactory::AppendSoundEvent(
	HString soundEventId,
	ContentKey key)
{
	// Default.
	if (!key.GetFilePath().IsValid())
	{
		key.SetFilePath(Manager::Get()->GetDefaultProjectFilePath());
	}

	// If there is already an entry with this key in the table, delete it first
	{
		Event* pSoundEvent = nullptr;
		if (m_tCachedSoundEvents.GetValue(soundEventId, pSoundEvent))
		{
			m_tCachedSoundEvents.Erase(soundEventId);
			m_tSoundEvents.Erase(soundEventId);
			SafeDelete(pSoundEvent);
		}
	}

	// Always insert the key into the m_tSoundEvents table - should
	// be impossible for this to fail.
	auto const e = m_tSoundEvents.Insert(soundEventId, key);
	if (!e.Second)
	{
		if (e.First->Second != key)
		{
			SEOUL_WARN("Attempt to define sound event with id '%s' as both '%s' and '%s', only the first definition is valid.",
				soundEventId.CStr(),
				e.First->Second.ToString().CStr(),
				key.ToString().CStr());
		}
	}
	else
	{
		// Also cache a sound event instance for preloading.
		auto pSoundManager(Manager::Get());
		auto pSoundEvent = pSoundManager->NewSoundEvent();
		pSoundManager->AssociateSoundEvent(key, *pSoundEvent);

		// It should be impossible for this to fail, since the soundEventId was
		// already successfully inserted into a shadowed table.
		SEOUL_VERIFY(m_tCachedSoundEvents.Insert(soundEventId, pSoundEvent).Second);
	}
}

/**
 * Setup the set of sound events and optional duckers managed by this SoundEventFactory.
 *
 * \warning Calling this method will immediately stop any existing sound events
 * in this Sound::EventFactory and reset any duckers.
 */
Bool EventFactory::Configure(
	const ContentKey& key,
	const DataStore& dataStore,
	const DataNode& soundEventsTable,
	const DataNode& soundDuckersArray,
	Bool bAppend/* = false*/,
	HString sMovieTypeName /*=HString()*/)
{
	return
		ConfigureSoundEvents(key, dataStore, soundEventsTable, bAppend, sMovieTypeName) &&
		ConfigureSoundDuckers(key, dataStore, soundDuckersArray, sMovieTypeName);
}

/**
 * Setup the set of sound events that can be instanced by a soundEventId identifier.
 *
 * \warning Calling this method will immediately stop any existing sound events
 * in this SoundEventFactory.
 */
Bool EventFactory::ConfigureSoundEvents(
	const ContentKey& keyParent,
	const DataStore& dataStore,
	const DataNode& tableNode,
	Bool bAppend /*=false*/,
	HString sMovieTypeName /*=HString()*/)
{
	SEOUL_ASSERT(IsMainThread());

	SafeDeleteVector(m_vUnnamedSoundEvents);
	SafeDeleteTable(m_tTrackedSoundEvents);
	if (!bAppend)
	{
		SafeDeleteTable(m_tCachedSoundEvents);
		m_tSoundEvents.Clear();
	}

	Bool bHitAFailure = false;
	auto const iBegin = dataStore.TableBegin(tableNode);
	auto const iEnd = dataStore.TableEnd(tableNode);
	for (auto i = iBegin; iEnd != i; ++i)
	{
		// Populate the key object for the sound event.
		ContentKey key;
		if (!key.SetFromDataStore(dataStore, i->Second))
		{
			// Support usage of the default project if the value (i->Second)
			// is either just a string (the event name by itself) or an
			// array where the first element is a string (but not a FilePath).
			DataNode dataNode;
			Byte const* sData = nullptr;
			UInt32 uData = 0u;
			if (dataStore.AsString(i->Second, sData, uData) ||
				(i->Second.IsArray() && 
					dataStore.GetValueFromArray(i->Second, 0u, dataNode) &&
					dataStore.AsString(dataNode, sData, uData)))
			{
				key.SetData(HString(sData, uData, true));
				key.SetFilePath(Manager::Get()->GetDefaultProjectFilePath());
			}
			else
			{
				// Unknown format, error.
				SEOUL_WARN("Malformed file path in %s for SFX %s in movie %s",
					keyParent.GetFilePath().CStr(), i->First.CStr(), sMovieTypeName.CStr());
				bHitAFailure = true;
				continue;
			}
		}

		// Also retrieve whether preloading is enabled or not - default to enabled.
		Bool bPreload = true;
		{
			UInt32 uArrayCount = 0u;
			DataNode preloadNode;
			if (dataStore.GetArrayCount(i->Second, uArrayCount) &&
				uArrayCount > 0u &&
				dataStore.GetValueFromArray(i->Second, uArrayCount - 1u, preloadNode) &&
				preloadNode.IsBoolean())
			{
				(void)dataStore.AsBoolean(preloadNode, bPreload);
			}
		}

		Event* pSoundEvent = nullptr;
		if (bAppend)
		{
			// If there is already an entry with this key in the table, delete it first
			if (m_tCachedSoundEvents.GetValue(i->First, pSoundEvent))
			{
				m_tCachedSoundEvents.Erase(i->First);
				m_tSoundEvents.Erase(i->First);
				SafeDelete(pSoundEvent);
			}
		}

		// Always insert the key into the m_tSoundEvents table - should
		// be impossible for this to fail.
		SEOUL_VERIFY(m_tSoundEvents.Insert(i->First, key).Second);

		// Unless preloading is disabled, also cache the sound event object.
		if (bPreload)
		{
			CheckedPtr<Manager> pSoundManager(Manager::Get());
			pSoundEvent = pSoundManager->NewSoundEvent();
			pSoundManager->AssociateSoundEvent(key, *pSoundEvent);

			// It should be impossible for this to fail, since the key is already a key in
			// a table and must be unique.
			SEOUL_VERIFY(m_tCachedSoundEvents.Insert(i->First, pSoundEvent).Second);
		}
	}

	return !bHitAFailure;
}

/**
 * Configure a new set of sound duckers associated with this
 * SoundEventFactory.
 */
Bool EventFactory::ConfigureSoundDuckers(
	const ContentKey& key,
	const DataStore& dataStore,
	const DataNode& arrayNode,
	HString sMovieTypeName /*=HString()*/)
{
	SEOUL_ASSERT(IsMainThread());

	// Make sure any active duckers restore their associated categories before being destroyed.
	ResetSoundDuckers();

	// Remove any existing entries from the set of sound duckers.
	m_vSoundDuckers.Clear();

	// Array means no duckers were defined, which is allowed.
	if (!arrayNode.IsNull())
	{
		// Deserialize the duckers vector.
		Bool bSuccess = Reflection::DeserializeObject(
			key,
			dataStore,
			arrayNode,
			Reflection::WeakAny(&m_vSoundDuckers));
		if (!bSuccess)
		{
			SEOUL_WARN("Failed to deserialize SoundDuckers in %s for movie %s",
				key.GetFilePath().CStr(), sMovieTypeName.CStr());
		}
		return bSuccess;
	}

	return true;
}


/**
 * Trigger a one-off sound event - must be a finite sound event that does not loop,
 * as you will have no control over the event once this method returns.
 *
 * @return True if the event was successfully started, false otherwise.
 */
Bool EventFactory::StartSoundEvent(
	HString soundEventId,
	const Vector3D& vPosition,
	const Vector3D& vVelocity /* = Vector3D::Zero() */,
	Bool bStopOnDestruction /*= false*/)
{
	SEOUL_ASSERT(IsMainThread());

	Event* pSoundEvent = nullptr;
	if (!m_tCachedSoundEvents.GetValue(soundEventId, pSoundEvent))
	{
		// Lazy cache the sound event if it is available.
		ContentKey key;
		if (m_tSoundEvents.GetValue(soundEventId, key))
		{
			CheckedPtr<Manager> pSoundManager(Manager::Get());
			pSoundEvent = pSoundManager->NewSoundEvent();
			pSoundManager->AssociateSoundEvent(key, *pSoundEvent);

			// It should be impossible for this to fail, since the key is already a key in
			// a table and must be unique.
			SEOUL_VERIFY(m_tCachedSoundEvents.Insert(soundEventId, pSoundEvent).Second);
		}
	}

	// Start the event if we retrieved one, one way or another.
	if (nullptr != pSoundEvent)
	{
		// Generate an entry for the sound event.
		FactoryEntry* pEntry = SEOUL_NEW(MemoryBudgets::Audio) FactoryEntry(soundEventId, *pSoundEvent);
		pEntry->m_vPosition = vPosition;
		pEntry->m_vVelocity = vVelocity;
		pEntry->m_bStopOnDestruction = bStopOnDestruction;

		// Poll once to try to start the event.
		(void)pEntry->Poll();

		// Now set the desired start time - we do this after the first poll,
		// since we only want to apply a start time adjustment if the first poll
		// failed to start the event.
		pEntry->m_iStartTimeInTicks = SeoulTime::GetGameTimeInTicks();

		// Insert to track it.
		m_vUnnamedSoundEvents.PushBack(pEntry);

		return true;
	}

	return false;
}

/**
 * Trigger a tracked sound event - can be looping or a one-off sound event.
 *
 * @return True if the event was successfully started, false otherwise.
 */
Bool EventFactory::StartTrackedSoundEvent(
	HString soundEventId,
	Int32& riId,
	const Vector3D& vPosition,
	const Vector3D& vVelocity /* = Vector3D::Zero() */,
	Bool bStopOnDestruction /* = false*/)
{
	SEOUL_ASSERT(IsMainThread());

	FactoryEntry* pSoundEventEntry = nullptr;

	// Start the entry.
	{
		Event* pTemplateSoundEvent = nullptr;
		if (!m_tCachedSoundEvents.GetValue(soundEventId, pTemplateSoundEvent))
		{
			// Lazy cache the sound event if it is available.
			ContentKey key;
			if (!m_tSoundEvents.GetValue(soundEventId, key))
			{
				return false;
			}

			CheckedPtr<Manager> pSoundManager(Manager::Get());
			pTemplateSoundEvent = pSoundManager->NewSoundEvent();
			pSoundManager->AssociateSoundEvent(key, *pTemplateSoundEvent);

			// It should be impossible for this to fail, since the key is already a key in
			// a table and must be unique.
			SEOUL_VERIFY(m_tCachedSoundEvents.Insert(soundEventId, pTemplateSoundEvent).Second);
		}

		// Generate an entry for the sound event.
		riId = m_iNextTrackedEventId++;

		// Handle overflow.
		if (m_iNextTrackedEventId < 0)
		{
			m_iNextTrackedEventId = 0;
		}

		pSoundEventEntry = SEOUL_NEW(MemoryBudgets::Audio) FactoryEntry(soundEventId, *pTemplateSoundEvent);
		SEOUL_VERIFY(m_tTrackedSoundEvents.Insert(riId, pSoundEventEntry).Second);
	}

	// Configure settings.
	pSoundEventEntry->m_vPosition = vPosition;
	pSoundEventEntry->m_vVelocity = vVelocity;
	pSoundEventEntry->m_bStopOnDestruction = bStopOnDestruction;

	// Poll once to try to start the event.
	(void)pSoundEventEntry->Poll();

	// Now set the desired start time - we do this after the first poll,
	// since we only want to apply a start time adjustment if the first poll
	// failed to start the event.
	pSoundEventEntry->m_iStartTimeInTicks = SeoulTime::GetGameTimeInTicks();

	return true;
}

/**
 * Stop the already playing tracked sound event iId.
 *
 * @param[in] bStopImmediately If true, the event will not play its tail
 * and will instead stop as quickly as possible.
 *
 * @return True if the sound event was stopped, false otherwise.
 */
Bool EventFactory::StopTrackedSoundEvent(Int32 iId, Bool bStopImmediately /*= false*/)
{
	FactoryEntry* pSoundFactoryEntry = nullptr;
	if (!m_tTrackedSoundEvents.GetValue(iId, pSoundFactoryEntry))
	{
		return false;
	}

	pSoundFactoryEntry->m_pSoundEvent->Stop(bStopImmediately);

	// Purge from table, and delete.
	m_tTrackedSoundEvents.Erase(iId);
	SafeDelete(pSoundFactoryEntry);

	return true;
}

/**
 * Set a parameter of an already playing tracked sound event.
 *
 * @return True if the sound event parameter was changed, false otherwise.
 */
Bool EventFactory::SetTrackedSoundEventParameter(Int32 iId, HString parameterName, Float fValue)
{
	FactoryEntry* pSoundFactoryEntry = nullptr;
	if (!m_tTrackedSoundEvents.GetValue(iId, pSoundFactoryEntry))
	{
		return false;
	}

	return pSoundFactoryEntry->m_pSoundEvent->SetParameter(parameterName, fValue);
}

/**
 * Trigger an already playing tracked sound event sustain point cue.
 *
 * @return True if the cue was triggered, false otherwise.
 */
Bool EventFactory::TriggerTrackedSoundEventCue(Int32 iId)
{
	FactoryEntry* pSoundFactoryEntry = nullptr;
	if (!m_tTrackedSoundEvents.GetValue(iId, pSoundFactoryEntry))
	{
		return false;
	}

	return pSoundFactoryEntry->m_pSoundEvent->TriggerCue();
}

/**
 * Attempt to update the 3D attributes of an already playing tracked sound event.
 *
 * @return True if the attributes were updated, false otherwise.
 */
Bool EventFactory::SetTrackedSoundEvent3DAttributes(
	Int32 iId, 
	const Vector3D& vPosition,
	const Vector3D& vVelocity)
{
	FactoryEntry* pSoundFactoryEntry = nullptr;
	if (!m_tTrackedSoundEvents.GetValue(iId, pSoundFactoryEntry))
	{
		return false;
	}

	pSoundFactoryEntry->m_vPosition = vPosition;
	pSoundFactoryEntry->m_vVelocity = vVelocity;
	pSoundFactoryEntry->m_pSoundEvent->Set3DAttributes(vPosition, vVelocity);
	return true;
}

/**
 * @return True if the sound events in this factory are still being loaded.
 */
Bool EventFactory::IsLoading() const
{
	for (auto i = m_tCachedSoundEvents.Begin(); m_tCachedSoundEvents.End() != i; ++i)
	{
		if (i->Second->IsLoading())
		{
			return true;
		}
	}

	return false;
}

/**
 * Perform per-frame update of sound events.
 *
 * \pre Must be called from the main thread.
 */
void EventFactory::Poll()
{
	SEOUL_ASSERT(IsMainThread());

	// Update any sound duckers.
	PollSoundDuckers();

	// Walk the list of tracked sound events, verify that they are all playing - play
	// can sometimes fail initially, if the sound is not loaded.
	for (auto e : m_tTrackedSoundEvents)
	{
		(void)e.Second->Poll();
	}

	// Walk the list of unnamed, pending play sound events.
	Int32 iCount = (Int32)m_vUnnamedSoundEvents.GetSize();
	for (Int32 i = 0; i < iCount; ++i)
	{
		FactoryEntry* pEntry = m_vUnnamedSoundEvents[i];
		FactoryEntry::State const eState = pEntry->Poll();

		// We can remove the entry if it has finished or can't be started.
		if (FactoryEntry::kCantStart == eState ||
			FactoryEntry::kFinishedPlaying == eState)
		{
			// "Swap trick" to remove the entry.
			Swap(m_vUnnamedSoundEvents[i], m_vUnnamedSoundEvents[iCount - 1]);
			--i;
			--iCount;

			// Now delete it.
			SafeDelete(pEntry);
		}
	}

	// Trim the list to the final size.
	m_vUnnamedSoundEvents.Resize((UInt32)iCount);
}

/**
 * If any sound duckers are active, apply their unduck change
 * and reset them to the inactive state.
 */
void EventFactory::ResetSoundDuckers()
{
	UInt32 const nDuckers = m_vSoundDuckers.GetSize();
	for (UInt32 i = 0u; i < nDuckers; ++i)
	{
		Ducker& rDucker = m_vSoundDuckers[i];
		if (rDucker.m_bActive)
		{
			UInt32 const nCategories = rDucker.m_vCategories.GetSize();
			for (UInt32 j = 0u; j < nCategories; ++j)
			{
				const Ducker::Category& category = rDucker.m_vCategories[j];
				Manager::Get()->SetCategoryVolume(
					category.m_CategoryName,
					category.m_fUnduckedVolume,
					category.m_uUnduckTimeMS / 1000.0f);
			}
			rDucker.m_bActive = false;
		}
	}
}

/**
 * Test for changes to sound duckers and apply them.
 */
void EventFactory::PollSoundDuckers()
{
	// Iterate over the entire list of duckers.
	UInt32 const nDuckers = m_vSoundDuckers.GetSize();
	for (UInt32 i = 0u; i < nDuckers; ++i)
	{
		Ducker& rDucker = m_vSoundDuckers[i];

		// For each event associated with the ducker, check if the event
		// is playing or not.
		Bool bActive = false;
		UInt32 const nEvents = rDucker.m_vEvents.GetSize();
		for (UInt32 j = 0u; j < nEvents; ++j)
		{
			auto const name = rDucker.m_vEvents[j];

			// Check unnamed events.
			for (auto const& e : m_vUnnamedSoundEvents)
			{
				if (e->m_SoundEventId == name && e->m_pSoundEvent->IsPlaying())
				{
					// Mark and done.
					bActive = true;
					break;
				}
			}

			// Check tracked events.
			if (!bActive)
			{
				for (auto const& e : m_tTrackedSoundEvents)
				{
					if (e.Second->m_SoundEventId == name && e.Second->m_pSoundEvent->IsPlaying())
					{
						// Mark and done.
						bActive = true;
						break;
					}
				}
			}
		}

		// If the ducker active state is different from what it should be, apply the change.
		if (bActive != rDucker.m_bActive)
		{
			// Enumerate the list of categories in the ducker.
			UInt32 const nCategories = rDucker.m_vCategories.GetSize();
			for (UInt32 j = 0u; j < nCategories; ++j)
			{
				Ducker::Category category = rDucker.m_vCategories[j];

				// The volume of the category and delta time is the ducked value if the
				// ducker is active, or the unducked value if it is not.
				Float const fVolume = (bActive ? category.m_fDuckedVolume : category.m_fUnduckedVolume);
				Float const fSeconds = (bActive ? category.m_uDuckTimeMS : category.m_uUnduckTimeMS) / 1000.0f;

				// Apply the volume change to the category over time.
				Manager::Get()->SetCategoryVolume(
					category.m_CategoryName,
					fVolume,
					fSeconds);
			}

			// The ducker is now in its correct state.
			rDucker.m_bActive = bActive;
		}
	}
}

} // namespace Sound

SEOUL_SPEC_TEMPLATE_TYPE(Vector<Sound::EventFactory::Ducker, 4>)
SEOUL_SPEC_TEMPLATE_TYPE(Vector<Sound::EventFactory::Ducker::Category, 4>)
SEOUL_BEGIN_TYPE(Sound::EventFactory::Ducker)
	SEOUL_PROPERTY_N("Events", m_vEvents)
	SEOUL_PROPERTY_N("Categories", m_vCategories)
	SEOUL_PROPERTY_N("Active", m_bActive)
		SEOUL_ATTRIBUTE(DoNotSerialize)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(Sound::EventFactory::Ducker::Category)
	SEOUL_PROPERTY_N("Name", m_CategoryName)
	SEOUL_PROPERTY_N("DuckedVolume", m_fDuckedVolume)
	SEOUL_PROPERTY_N("UnduckedVolume", m_fUnduckedVolume)
	SEOUL_PROPERTY_N("DuckTimeMS", m_uDuckTimeMS)
	SEOUL_PROPERTY_N("UnduckTimeMS", m_uUnduckTimeMS)
SEOUL_END_TYPE()

} // namespace Seoul
