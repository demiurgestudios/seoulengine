/**
 * \file FMODSoundAnchors.cpp
 *
 * \brief Implements anchor classes for sound projects and events, used to
 * track whether an event or project needs to remain loaded or not.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FMODSoundAnchors.h"
#include "FMODSoundContentLoader.h"
#include "FMODSoundManager.h"
#include "ReflectionDefine.h"

#if SEOUL_WITH_FMOD
#include "fmod_studio.h"
#include "fmod_studio.hpp"

namespace Seoul
{

SEOUL_BEGIN_ENUM(FMODSound::ProjectAnchor::State)
	SEOUL_ENUM_N("Loading", FMODSound::ProjectAnchor::kLoading)
	SEOUL_ENUM_N("Loaded", FMODSound::ProjectAnchor::kLoaded)
	SEOUL_ENUM_N("Error", FMODSound::ProjectAnchor::kError)
SEOUL_END_ENUM()

Bool SoundProjectAnchorReloadProjectSoundEvents(void* pUserDataFilePath, const Content::Handle<FMODSound::EventAnchor>& hSoundEventAnchor)
{
	FilePath const filePath = *((FilePath*)pUserDataFilePath);
	ContentKey const key = hSoundEventAnchor.GetKey();

	if (key.GetFilePath() == filePath)
	{
		Content::Traits<FMODSound::EventAnchor>::Load(key, hSoundEventAnchor);
	}

	// Always return false - we never "handle" this event.
	return false;
}

Bool Content::Traits<FMODSound::ProjectAnchor>::FileChange(FilePath filePath, const Content::Handle<FMODSound::ProjectAnchor>& hSoundProjectAnchor)
{
	if (FileType::kSoundProject == filePath.GetType())
	{
		CheckedPtr<FMODSound::Manager> pSoundManager = FMODSound::Manager::Get();
		if (pSoundManager->SoundProjectChange(filePath, hSoundProjectAnchor))
		{
			// Load the project.
			Load(filePath, hSoundProjectAnchor);

			// Trigger a reload of SoundEventAnchors associated with the project, so
			// they don't attempt to start while the project is still loading.
			pSoundManager->m_SoundEvents.Apply(SEOUL_BIND_DELEGATE(&SoundProjectAnchorReloadProjectSoundEvents, (void*)&filePath));

			return true;
		}
	}

	return false;
}

SharedPtr<FMODSound::ProjectAnchor> Content::Traits<FMODSound::ProjectAnchor>::GetPlaceholder(FilePath filePath)
{
	return SharedPtr<FMODSound::ProjectAnchor>(SEOUL_NEW(MemoryBudgets::Audio) FMODSound::ProjectAnchor);
}

void Content::Traits<FMODSound::ProjectAnchor>::Load(FilePath filePath, const Content::Handle<FMODSound::ProjectAnchor>& hSoundProjectAnchor)
{
	Content::LoadManager::Get()->Queue(
		SharedPtr<Content::LoaderBase>(SEOUL_NEW(MemoryBudgets::Content) FMODSound::ProjectContentLoader(
			filePath,
			hSoundProjectAnchor)));
}

Bool Content::Traits<FMODSound::ProjectAnchor>::PrepareDelete(const KeyType& key, Content::Entry<FMODSound::ProjectAnchor, KeyType>& entry)
{
	return FMODSound::Manager::Get()->PrepareSoundProjectAnchorDelete(key, entry);
}

Bool Content::Traits<FMODSound::EventAnchor>::FileChange(const ContentKey& key, const Content::Handle<FMODSound::EventAnchor>& pEntry)
{
	return false;
}

SharedPtr<FMODSound::EventAnchor> Content::Traits<FMODSound::EventAnchor>::GetPlaceholder(const KeyType& key)
{
	return SharedPtr<FMODSound::EventAnchor>();
}

void Content::Traits<FMODSound::EventAnchor>::Load(const KeyType& key, const Content::Handle<FMODSound::EventAnchor>& hSoundEventAnchor)
{
	Content::LoadManager::Get()->Queue(
		SharedPtr<Content::LoaderBase>(SEOUL_NEW(MemoryBudgets::Content) FMODSound::EventContentLoader(
			key,
			hSoundEventAnchor,
			FMODSound::Manager::Get()->m_SoundProjects.GetContent(key.GetFilePath()))));
}

Bool Content::Traits<FMODSound::EventAnchor>::PrepareDelete(const KeyType& key, Content::Entry<FMODSound::EventAnchor, KeyType>& entry)
{
	return FMODSound::Manager::Get()->PrepareSoundEventAnchorDelete(key, entry);
}

CheckedPtr<::FMOD::Studio::EventDescription> FMODSound::EventAnchor::ResolveDescription() const
{
	SEOUL_ASSERT(IsMainThread());

	auto pSoundManager(FMODSound::Manager::Get());
	auto pFMODStudioSystem = (pSoundManager.IsValid() ? pSoundManager->m_pFMODStudioSystem : nullptr);

	// Resolve the sound event.
	::FMOD::Studio::EventDescription* pFMODEventDescription = nullptr;
	FMOD_RESULT eResult = pFMODStudioSystem->getEvent(m_sKey.CStr(), &pFMODEventDescription);

	// Return result on success.
	if (FMOD_OK == eResult) { return pFMODEventDescription; }
	else { return nullptr; }
}

} // namespace Seoul

#endif // /#if SEOUL_WITH_FMOD
