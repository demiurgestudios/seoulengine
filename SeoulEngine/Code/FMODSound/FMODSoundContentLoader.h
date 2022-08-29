/**
 * \file FMODSoundContentLoader.h
 * \brief Specialization of Content::LoaderBase for loading SoundEvent objects and project files.
 * Handles loading the project that contains the sound event (if necessary)
 * and then async loading the sound event itself.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FMOD_SOUND_CONTENT_LOADER_H
#define FMOD_SOUND_CONTENT_LOADER_H

#include "ContentLoaderBase.h"
#include "ContentKey.h"
#include "ContentHandle.h"
#include "FMODSoundAnchors.h"
#include "ScopedPtr.h"
#include "SharedPtr.h"

#if SEOUL_WITH_FMOD

namespace FMOD { namespace Studio { class Bank; } }
namespace FMOD { namespace Studio { class EventDescription; } }
namespace Seoul { namespace FMODSound { class BankFileLoader; } }

namespace Seoul::FMODSound
{

/**
 * Specialization of Content::LoaderBase designed for asynchronously
 * loading the data associated with sound events.
 */
class EventContentLoader SEOUL_SEALED : public Content::LoaderBase
{
public:
	EventContentLoader(
		const ContentKey& key,
		const Content::Handle<EventAnchor>& hSoundEventAnchor,
		const Content::Handle<ProjectAnchor>& hSoundProjectAnchor);
	virtual ~EventContentLoader();

	virtual String GetContentKey() const SEOUL_OVERRIDE
	{
		return m_Key.ToString();
	}

protected:
	virtual Content::LoadState InternalExecuteContentLoadOp() SEOUL_OVERRIDE;

private:
	SEOUL_DISABLE_COPY(EventContentLoader);

	void InternalReleaseEntry();

	ContentKey const m_Key;
	String const m_sKey;
	Content::Handle<EventAnchor> m_hEntry;
	Content::Handle<ProjectAnchor> m_hSoundProjectAnchor;
	ScopedPtr<BankFileLoader> m_pLoader;
}; // class FMODSound::EventContentLoader

/**
 * Specialization of Content::LoaderBase designed for asynchronously
 * loading FMOD project files.
 */
class ProjectContentLoader SEOUL_SEALED : public Content::LoaderBase
{
public:
	ProjectContentLoader(
		FilePath filePath,
		const Content::Handle<ProjectAnchor>& pSoundProjectAnchor);
	virtual ~ProjectContentLoader();

private:
	typedef Vector<FilePath, MemoryBudgets::Audio> BankFiles;
	typedef HashSet<FilePath, MemoryBudgets::Audio> BankSet;
	typedef HashTable<HString, BankSet, MemoryBudgets::Audio> EventDependencies;

	virtual Content::LoadState InternalExecuteContentLoadOp() SEOUL_OVERRIDE;

	Bool InternalGetBanksAndEvents(BankFiles& rvBankFiles, EventDependencies& rtEvents);
	void InternalReleaseEntry();

	Content::Handle<ProjectAnchor> m_hEntry;
	BankFiles m_vBankFiles;
	EventDependencies m_tEvents;
	ScopedPtr<BankFileLoader> m_pLoader;

	void* m_pFileData;
	UInt32 m_uFileSize;

	SEOUL_DISABLE_COPY(ProjectContentLoader);
}; // class FMODSound::ProjectContentLoader

} // namespace Seoul::FMODSound

#endif // /#if SEOUL_WITH_FMOD

#endif // include guard
