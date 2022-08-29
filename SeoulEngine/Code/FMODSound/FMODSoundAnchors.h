/**
 * \file FMODSoundAnchors.h
 * \brief Implements anchor classes for sound projects and events, used to
 * track whether an event or project needs to remain loaded or not.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FMOD_SOUND_ANCHORS_H
#define	FMOD_SOUND_ANCHORS_H

#include "ContentHandle.h"
#include "ContentKey.h"
#include "ContentTraits.h"
#include "HashSet.h"
#include "SharedPtr.h"

#if SEOUL_WITH_FMOD

namespace FMOD { namespace Studio { class EventDescription; } }

namespace Seoul
{

// Forward declarations
namespace FMODSound { class EventAnchor; }
namespace FMODSound { class ProjectAnchor; }

namespace Content
{

/**
 * Specialization of Content::Traits<> for FMODSound::ProjectAnchor, allows FMODSound::ProjectAnchor
 * to be managed as loadable content by the content system.
 */
template <>
struct Traits<FMODSound::ProjectAnchor>
{
	typedef FilePath KeyType;
	static const Bool CanSyncLoad = false;

	static Bool FileChange(FilePath filePath, const Handle<FMODSound::ProjectAnchor>& hSoundProjectAnchor);
	static SharedPtr<FMODSound::ProjectAnchor> GetPlaceholder(FilePath filePath);
	static void Load(FilePath filePath, const Handle<FMODSound::ProjectAnchor>& hSoundProjectAnchor);
	static Bool PrepareDelete(const KeyType& key, Entry<FMODSound::ProjectAnchor, KeyType>& entry);
	static void SyncLoad(FilePath filePath, const Handle<FMODSound::ProjectAnchor>& pEntry) {}
	static UInt32 GetMemoryUsage(const SharedPtr<FMODSound::ProjectAnchor>& p) { return 0u; }
}; // struct Content::Traits<FMODSoundProjectAnchor>

/**
 * Specialization of Content::Traits<> for FMODSound::EventAnchor, allows FMODSound::EventAnchor
 * to be managed as loadable content by the content system.
 */
template <>
struct Traits<FMODSound::EventAnchor>
{
	typedef ContentKey KeyType;
	static const Bool CanSyncLoad = false;

	static Bool FileChange(const ContentKey& key, const Handle<FMODSound::EventAnchor>& hSoundEventAnchor);
	static SharedPtr<FMODSound::EventAnchor> GetPlaceholder(const KeyType& key);
	static void Load(const KeyType& key, const Handle<FMODSound::EventAnchor>& pEntry);
	static Bool PrepareDelete(const KeyType& key, Entry<FMODSound::EventAnchor, KeyType>& entry);
	static void SyncLoad(const KeyType& key, const Handle<FMODSound::EventAnchor>& pEntry) {}
	static UInt32 GetMemoryUsage(const SharedPtr<FMODSound::EventAnchor>& p) { return 0u; }
}; // struct Content::Traits<FMODSoundEventAnchor>

} // namespace Content

namespace FMODSound
{

/**
 * Encapsulates a reference to a sound project and provides
 * queries to describe the state of the project.
 */
class ProjectAnchor SEOUL_SEALED
{
public:
	typedef Vector<FilePath, MemoryBudgets::Audio> BankFiles;
	typedef HashSet<FilePath, MemoryBudgets::Audio> BankSet;
	typedef HashTable<HString, BankSet, MemoryBudgets::Audio> EventDependencies;

	enum State
	{
		/** Sound project is in the process of being loaded. */
		kLoading,

		/** Sound project was successful loaded. */
		kLoaded,

		/** Sound project load was attempted but failed. */
		kError
	};

	ProjectAnchor()
		: m_vBankFiles()
		, m_eState(kLoading)
	{
	}

	State GetState() const
	{
		return m_eState;
	}

	void SetState(State eState)
	{
		m_eState = eState;
	}

	const BankFiles& GetBankFiles() const
	{
		return m_vBankFiles;
	}

	const EventDependencies& GetEventDependencies() const
	{
		return m_tEvents;
	}

	void SetBankFiles(const BankFiles& vBankFiles)
	{
		m_vBankFiles = vBankFiles;
	}

	void SetEventDependencies(const EventDependencies& tEvents)
	{
		m_tEvents = tEvents;
	}

	void Reset()
	{
		m_eState = kLoading;
		m_vBankFiles.Clear();
	}

private:
	BankFiles m_vBankFiles;
	EventDependencies m_tEvents;
	Atomic32Value<State> m_eState;

	SEOUL_REFERENCE_COUNTED(ProjectAnchor);
	SEOUL_DISABLE_COPY(ProjectAnchor);
}; // class FMODSound::ProjectAnchor

/**
 * Reference counted associate of a SoundEvent with its underlying data.
 */
class EventAnchor SEOUL_SEALED
{
public:
	EventAnchor(
		const Content::Handle<ProjectAnchor>& hSoundProjectAnchor,
		const String& sKey)
		: m_hSoundProjectAnchor(hSoundProjectAnchor)
		, m_sKey(sKey)
	{
	}

	Bool IsProjectLoading() const { return m_hSoundProjectAnchor.IsLoading(); }
	CheckedPtr<::FMOD::Studio::EventDescription> ResolveDescription() const;

private:
	Content::Handle<ProjectAnchor> m_hSoundProjectAnchor;
	String const m_sKey;

	SEOUL_REFERENCE_COUNTED(EventAnchor);
	SEOUL_DISABLE_COPY(EventAnchor);
}; // class FMODSound::EventAnchor

} // namespace FMODSound

} // namespace Seoul

#endif // /#if SEOUL_WITH_FMOD

#endif // include guard
