/**
 * \file SoundManager.cpp
 * \brief Singleton manager of sound effects and music in Seoul engine.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ContentLoadManager.h"
#include "EventsManager.h"
#include "SoundEvent.h"
#include "SoundManager.h"

namespace Seoul::Sound
{

SampleData::SampleData(
	UInt64 uFrame,
	UInt64 uOffsetInSamples,
	UInt32 uSizeInSamples,
	UInt32 uChannels,
	Float const* pfData,
	UInt32 uPaddingInSamples)
	: m_uFrame(uFrame)
	, m_uOffsetInSamples(uOffsetInSamples)
	, m_uSizeInSamples(uSizeInSamples + uPaddingInSamples)
	, m_uChannels(uChannels)
	, m_pfData(nullptr)
{
	auto const uPaddingInBytes =
		(uPaddingInSamples * uChannels * sizeof(Float));
	auto const uDataSizeInBytes =
		(uSizeInSamples * uChannels * sizeof(Float));
	auto const uSizeInBytes = uPaddingInBytes + uDataSizeInBytes;

	// Buffer for all the data.
	m_pfData = (Float*)MemoryManager::AllocateAligned(
		uSizeInBytes,
		MemoryBudgets::Audio,
		SEOUL_ALIGN_OF(Float));

	// Padding if specified.
	if (uPaddingInBytes > 0)
	{
		memset(m_pfData, 0, uPaddingInBytes);
	}

	// Data.
	memcpy(m_pfData + (uPaddingInSamples * uChannels), pfData, uDataSizeInBytes);
}

SampleData::~SampleData()
{
	MemoryManager::Deallocate(m_pfData);
}

Manager::~Manager()
{
	SEOUL_ASSERT(IsMainThread());

	// No longer Tick relevant.
	Events::Manager::Get()->UnregisterCallback(Content::MainThreadTickWhileWaiting, SEOUL_BIND_DELEGATE(&Manager::Tick, this));
}

/**
 * Ticks Sound::Manager, performing per-frame update operations.
 *
 * @param fDeltaTime Time in seconds since the last frame.
 */
void Manager::Tick(Float fDeltaTime)
{
	SEOUL_ASSERT(IsMainThread());

	// Process any pending category volume sets.
	InternalProcessPendingSetCategoryVolumes(fDeltaTime);

	// Process any pending category mutes.
	InternalProcessPendingSetCategoryMutes();

	// Enumerate and prune any sound capture instances.
	InternalProcessSoundCapture();
}

Manager::Manager()
	: m_DefaultProjectFilePath(FilePath::CreateContentFilePath("Authored/Sound/App.fspro"))
{
	SEOUL_ASSERT(IsMainThread());

	// Now Tick relevant.
	Events::Manager::Get()->RegisterCallback(Content::MainThreadTickWhileWaiting, SEOUL_BIND_DELEGATE(&Manager::Tick, this));
}

/**
 * Register a capture instance that will receive master
 * bus audio data. Will be automatically released
 * when all references are released.
 */
void Manager::RegisterSoundCapture(
	const SharedPtr<ICapture>& pCapture,
	const ThreadId& callbackThreadId /*= ThreadId()*/)
{
	SEOUL_ASSERT(IsMainThread());

	CaptureEntry entry;
	entry.m_p = pCapture;
	entry.m_ThreadId = callbackThreadId;
	entry.m_uStartClockTime = GetClockTimeDSP();

	{
		Lock lock(m_SoundCapture);
		m_vSoundCapture.PushBack(entry);
	}
}

/**
 * Enqueue a category mute operation until the time at which
 * in can be completed.
 */
void Manager::DeferCategoryMute(HString sCategoryName, Bool bMute)
{
	SEOUL_ASSERT(IsMainThread());

	// If the category doesn't exist yet, defer the mute change until
	// some time later.
	PendingSetCategoryMute entry(sCategoryName, bMute);

	// Check if an update for the category is already in the queue - if so, remove the existing
	// entry and replace with the new one.
	PendingSetCategoryMutesList::Iterator i = m_vPendingSetCategoryMutes.Find(entry.m_sCategoryName);
	if (m_vPendingSetCategoryMutes.End() != i)
	{
		*i = entry;
	}
	else
	{
		// Insert a new entry.
		m_vPendingSetCategoryMutes.PushBack(entry);
	}
}

/**
 * Enqueue a deferred instantaneous volume change.
 */
void Manager::DeferCategoryVolume(HString sCategoryName, Float fVolume)
{
	SEOUL_ASSERT(IsMainThread());

	InternalDeferCategoryVolume(PendingSetCategoryVolume(sCategoryName, fVolume));
}

/**
 * Enqueue a deferred volume change to a sound category with a fade time.
 */
void Manager::DeferCategoryVolume(HString sCategoryName, Float fStartVolume, Float fEndVolume, Float fSeconds)
{
	SEOUL_ASSERT(IsMainThread());

	InternalDeferCategoryVolume(PendingSetCategoryVolume(sCategoryName, fStartVolume, fEndVolume, fSeconds));
}

/**
 * Shared utility, enqueue a deferred volume change to a sound category.
 */
void Manager::InternalDeferCategoryVolume(const PendingSetCategoryVolume& pendingSetCategoryVolume)
{
	SEOUL_ASSERT(IsMainThread());

	// Check if an update for the category is already in the queue - if so, remove the existing
	// entry and replace with the new one.
	PendingSetCategoryVolumesList::Iterator i = m_vPendingSetCategoryVolumes.Find(pendingSetCategoryVolume.m_sCategoryName);
	if (m_vPendingSetCategoryVolumes.End() != i)
	{
		*i = pendingSetCategoryVolume;
	}
	else
	{
		// Insert a new entry.
		m_vPendingSetCategoryVolumes.PushBack(pendingSetCategoryVolume);
	}
}

/**
 * Attempts to process any pending SetCategoryMute calls which were deferred
 * due to the given categories not existing at the time.
 */
void Manager::InternalProcessPendingSetCategoryMutes()
{
	SEOUL_ASSERT(IsMainThread());

	// Walk the list of pending volume sets.
	Int32 nCount = (Int32)m_vPendingSetCategoryMutes.GetSize();
	for (Int32 i = 0; i < nCount; ++i)
	{
		// Cache the entry.
		PendingSetCategoryMute& rEntry = m_vPendingSetCategoryMutes[i];

		// If the Apply() returns true, it means that the update has been fully
		// applied Remove the entry from the queue using the "swap trick" - we don't care
		// about order in the vector, so we can erase the entry with minimal work.
		if (rEntry.Apply())
		{
			Swap(m_vPendingSetCategoryMutes[nCount - 1], rEntry);
			--nCount;
			--i;
		}
	}

	// Resize the vector if any entries were removed.
	SEOUL_ASSERT(nCount >= 0);
	m_vPendingSetCategoryMutes.Resize((UInt32)nCount);
}

/**
 * Attempts to process any pending SetCategoryVolume calls which were deferred
 * due to the given categories not existing at the time.
 */
void Manager::InternalProcessPendingSetCategoryVolumes(Float fDeltaTimeInSeconds)
{
	SEOUL_ASSERT(IsMainThread());

	// Walk the list of pending volume sets.
	Int32 nCount = (Int32)m_vPendingSetCategoryVolumes.GetSize();
	for (Int32 i = 0; i < nCount; ++i)
	{
		// Apply delta time to the entry.
		PendingSetCategoryVolume& rEntry = m_vPendingSetCategoryVolumes[i];
		rEntry.Tick(fDeltaTimeInSeconds);

		// If the Apply() returns true, it means that the update has been fully
		// applied (it was either an instantaneous update and has succeeded, or
		// an update over time and we've reached the target time). Remove the
		// entry from the queue using the "swap trick" - we don't care about order
		// in the vector, so we can erase the entry with minimal work.
		if (rEntry.Apply())
		{
			Swap(m_vPendingSetCategoryVolumes[nCount - 1], rEntry);
			--nCount;
			--i;
		}
	}

	// Resize the vector if any entries were removed.
	SEOUL_ASSERT(nCount >= 0);
	m_vPendingSetCategoryVolumes.Resize((UInt32)nCount);
}

/**
 * Enumerate and prune any orphaned sound capture
 * instances.
 */
void Manager::InternalProcessSoundCapture()
{
	// Attempt to lock - bail on failure, since
	// this is effectively just lazy GC.
	TryLock lock(m_SoundCapture);
	if (!lock.IsLocked())
	{
		return;
	}

	Int32 iCount = (Int32)m_vSoundCapture.GetSize();
	for (Int32 i = 0; i < iCount; ++i)
	{
		if (m_vSoundCapture[i].m_p.IsUnique())
		{
			Swap(m_vSoundCapture[i], m_vSoundCapture[iCount - 1]);
			--iCount;
			--i;
		}
	}

	m_vSoundCapture.Resize((UInt32)iCount);
}

void Manager::ApplySoundSettings(const Settings& soundSettings)
{
	// We use SetCategoryVolume instead of SetCategoryMute, because this plays better with code that may mute sounds for other reasons
	// For example, on iOS game music is muted and unmuted if there is another app playing music
	SetCategoryVolume(CategoryNames::khsSoundCategoryMusic, soundSettings.GetMusicEnabled() ? 1.0f : 0.0f, 0.0f, true, false);
	SetCategoryVolume(CategoryNames::khsSoundCategorySFX, soundSettings.GetSoundEffectsEnabled() ? 1.0f : 0.0f, 0.0f, true, false);
}

NullManager::NullManager()
{
}

NullManager::~NullManager()
{
}

void NullManager::AssociateSoundEvent(const ContentKey& soundEventKey, Event& rEvent)
{
	static_cast<NullEvent&>(rEvent).m_Key = soundEventKey;
}

Event* NullManager::NewSoundEvent() const
{
	return SEOUL_NEW(MemoryBudgets::Audio) NullEvent;
}

} // namespace Seoul::Sound
