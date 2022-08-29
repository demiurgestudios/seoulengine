/**
 * \file SoundEventFactory.h
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

#pragma once
#ifndef SOUND_EVENT_FACTORY_H
#define SOUND_EVENT_FACTORY_H

#include "HashTable.h"
#include "ContentKey.h"
#include "DataStore.h"
#include "FixedArray.h"
#include "ReflectionDeclare.h"
#include "SeoulHString.h"
#include "SoundEvent.h"
#include "Vector3D.h"

namespace Seoul::Sound
{

/**
 * Utility structure that contains and tracks tracked sound event data
 * within a SoundEventFactory.
 */
class FactoryEntry SEOUL_SEALED
{
public:
	FactoryEntry(HString soundEventId, const Event& templateSoundEvent)
		: m_SoundEventId(soundEventId)
		, m_pSoundEvent(templateSoundEvent.Clone())
		, m_iStartTimeInTicks(-1)
		, m_vPosition(Vector3D::Zero())
		, m_vVelocity(Vector3D::Zero())
		, m_bStopOnDestruction(false)
		, m_bStarted(false)
	{
	}

	HString const m_SoundEventId;
	ScopedPtr<Event> const m_pSoundEvent;
	Int64 m_iStartTimeInTicks;
	Vector3D m_vPosition;
	Vector3D m_vVelocity;
	Bool m_bStopOnDestruction;
	Bool m_bStarted;

	enum State
	{
		/** Sound event is waiting to start playing (likely, waiting to load). */
		kWaitingToStart,

		/** Sound event is actively playing. */
		kPlaying,

		/** Could not start the sound event in the time alloted. */
		kCantStart,

		/** Sound event started and finished playing. */
		kFinishedPlaying,
	};

	// Performs per-frame work on a tracked sound event - verifies that the event is
	// playing if it failed to start initially.
	State Poll();

private:
	SEOUL_DISABLE_COPY(FactoryEntry);
}; // class Sound::FactoryEntry
typedef HashTable<Int32, FactoryEntry*, MemoryBudgets::Audio> TrackedSoundEvents;

class EventFactory SEOUL_SEALED
{
public:
	EventFactory();
	~EventFactory();

	// Utility used to add additional sound events to the factory beyond
	// the initial configuration.
	void AppendSoundEvent(HString soundEventId, ContentKey key);

	// Setup this Sound::EventFactory with settings in a DataStore table,
	// contain in dataStore. WARNING: Calling this method will immediately
	// stop and release any Sound::Event already contained in this
	// SoundEventFactory.
	Bool Configure(
		const ContentKey& key,
		const DataStore& dataStore,
		const DataNode& soundEventsTableNode,
		const DataNode& soundDuckersArrayNode,
		Bool bAppend = false,
		HString sMovieTypeName = HString());

	// Kick off a one-off sound event - must be a non-looping, finite sound event.
	Bool StartSoundEvent(HString soundEventId, const Vector3D& vPositiony = Vector3D::Zero(), const Vector3D& vVelocity = Vector3D::Zero(), Bool bStopOnDestruction = false);

	// Kick off a tracked sound event - this will create a new entry for the sound event instance
	// (if one does not already exist) which can be queried later and manipulated
	// (position, keys, parameters, and stopped).
	Bool StartTrackedSoundEvent(HString soundEventId, Int32& riId, const Vector3D& vPositiony = Vector3D::Zero(), const Vector3D& vVelocity = Vector3D::Zero(), Bool bStopOnDestruction = false);

	// Mutators for a playing tracked sound event.
	Bool StopTrackedSoundEvent(Int32 iId, Bool bStopImmediately = false);
	Bool SetTrackedSoundEventParameter(Int32 iId, HString parameterName, Float fValue);
	Bool TriggerTrackedSoundEventCue(Int32 iId);
	Bool SetTrackedSoundEvent3DAttributes(Int32 iId, const Vector3D& vPositiony = Vector3D::Zero(), const Vector3D& vVelocity = Vector3D::Zero());

	// Return true if the sound events in this factory are still being loaded.
	Bool IsLoading() const;

	// Perform perf frame maintenance of sound events.
	void Poll();

	/**
	 * Structure used to configure sound ducking settings.
	 */
	struct Ducker SEOUL_SEALED
	{
		Ducker()
			: m_vEvents()
			, m_vCategories()
			, m_bActive(false)
		{
		}

		// List of events to trigger ducking when playing - this is
		// name of a Sound::Event in the Sound::Factory
		// configuration.
		Vector<HString, MemoryBudgets::Audio> m_vEvents;

		// Define a category entry to apply ducking to.
		struct Category SEOUL_SEALED
		{
			Category()
				: m_CategoryName()
				, m_fDuckedVolume(1.0f)
				, m_fUnduckedVolume(1.0f)
				, m_uDuckTimeMS(0u)
				, m_uUnduckTimeMS(0u)
			{
			}

			/** Name of the category, for example, "music". */
			HString m_CategoryName;

			/** Target volume when ducking is being applied. */
			Float m_fDuckedVolume;

			/** Volume to restore the category to when ducking is complete. */
			Float m_fUnduckedVolume;

			/** Time in milliseconds over which volume changes from current to target when ducking. */
			UInt32 m_uDuckTimeMS;

			/** Time in milliseconds over which volume changes from current to target when ducking is complete. */
			UInt32 m_uUnduckTimeMS;
		};

		Vector<Category, MemoryBudgets::Audio> m_vCategories;

		/** Unserialized value used to track whether ducking is active or not for an entry. */
		Bool m_bActive;
	};

	Bool ConfigureSoundEvents(
		const ContentKey& key,
		const DataStore& dataStore,
		const DataNode& tableNode,
		Bool bAppend,
		HString sMovieTypeName = HString());

private:
	Bool ConfigureSoundDuckers(
		const ContentKey& key,
		const DataStore& dataStore,
		const DataNode& arrayNode,
		HString sMovieTypeName = HString());
	void ResetSoundDuckers();
	void PollSoundDuckers();

	Int32 m_iNextTrackedEventId;

	typedef HashTable<HString, ContentKey, MemoryBudgets::Audio> Events;
	Events m_tSoundEvents;
	typedef HashTable<HString, Event*, MemoryBudgets::Audio> CachedSoundEvents;
	CachedSoundEvents m_tCachedSoundEvents;

	TrackedSoundEvents m_tTrackedSoundEvents;

	typedef Vector<FactoryEntry*, MemoryBudgets::Audio> UnnamedSoundEvents;
	UnnamedSoundEvents m_vUnnamedSoundEvents;

	typedef Vector< Ducker, MemoryBudgets::Audio> Duckers;
	Duckers m_vSoundDuckers;

	SEOUL_DISABLE_COPY(EventFactory);
}; // class Sound::EventFactory

} // namespace Seoul::Sound

#endif // include guard
