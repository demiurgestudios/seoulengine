/**
 * \file FMODSoundManager.h
 * \brief Singleton manager of sound effects and music in Seoul engine,
 * implemented with the FMOD Ex sound system.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FMOD_SOUND_MANAGER_H
#define	FMOD_SOUND_MANAGER_H

#include "Atomic32.h"
#include "FMODSoundEvent.h"
#include "SoundManager.h"
#include "UnsafeHandle.h"

#if SEOUL_WITH_FMOD

namespace FMOD { class DSP; }
namespace FMOD { namespace Studio { class Bank; } }
namespace FMOD { namespace Studio { class System; } }
namespace Seoul { namespace FMODSound { class ManagerFileManager; } }

namespace Seoul::FMODSound
{

/**
 * The FMODSound::Manager handles loading/unloading of sound projects
 * and attached sound banks. It provides a method to get "sound event" instances,
 * which can be thought of as sound effects, except that sound events are more
 * complex (they can contain multiple wave files, effect processing, and
 * can react to runtime variables to respond to gameplay events).
 */
class Manager SEOUL_SEALED : public Sound::Manager
{
public:
	/** Internal FMODSound::Manager configuration flags. */
	enum Flags
	{
		kNone = 0u,
		kEnableNetInterface = (1 << 0u),
		kHeadless = (1 << 1u),
		kNonRealTime = (1 << 2u),
	};

	/**
	 * @return The global singleton instance. Will be nullptr if that instance has not
	 * yet been created.
	 */
	static CheckedPtr<Manager> Get()
	{
		if (Sound::Manager::Get() && Sound::Manager::Get()->GetType() == Sound::ManagerType::kFMOD)
		{
			return (Manager*)Sound::Manager::Get().Get();
		}
		else
		{
			return CheckedPtr<Manager>();
		}
	}

	Manager(UInt32 uFlags = kNone);
	~Manager();

	virtual Sound::ManagerType GetType() const SEOUL_OVERRIDE { return Sound::ManagerType::kFMOD; }

	virtual Bool IsInitialized() const SEOUL_OVERRIDE;

	virtual void SetListenerCamera(const SharedPtr<Camera>& pCamera) SEOUL_OVERRIDE;

	virtual void AssociateSoundEvent(const ContentKey& soundEventKey, Sound::Event& rInEvent) SEOUL_OVERRIDE;
	virtual Sound::Event* NewSoundEvent() const SEOUL_OVERRIDE;
	virtual void Tick(Float fDeltaTime) SEOUL_OVERRIDE;

	virtual Bool SetCategoryPaused(HString categoryName, Bool bPaused = true) SEOUL_OVERRIDE;

	virtual Bool SetMasterMute(Bool bMuted = true) SEOUL_OVERRIDE;
	virtual Bool SetMasterPaused(Bool bPaused = true) SEOUL_OVERRIDE;
	virtual Bool SetMasterVolume(Float fVolume) SEOUL_OVERRIDE;

	virtual Bool SetCategoryMute(HString hsCategoryName, Bool bMute = true, Bool bAllowPending = false, Bool bSuppressLogging = false) SEOUL_OVERRIDE;
	virtual Bool SetCategoryVolume(HString hsCategoryName, Float fVolume, Float fFadeTimeInSeconds = 0.0f, Bool bAllowPending = false, Bool bSuppressLogging = false) SEOUL_OVERRIDE;
	virtual Float GetCategoryVolume(HString hsCategoryName) const SEOUL_OVERRIDE;

	virtual Bool GetMasterAttributes(UInt32& ruSamplingRate, UInt32& ruChannels) const SEOUL_OVERRIDE;

	virtual void SetMaster3DAttributes(
		Float fDopplerShiftScale = 1.0f,
		Float fUnitsPerMeter = 100.0f,
		Float f3DRolloffScale = 1.0f) SEOUL_OVERRIDE;

	virtual UInt32 GetTotalMemoryUsageInBytes() const SEOUL_OVERRIDE;

	virtual void OnEnterBackground() SEOUL_OVERRIDE;
	virtual void OnLeaveBackground() SEOUL_OVERRIDE;

	virtual void OnInterruptStart() SEOUL_OVERRIDE;
	virtual void OnInterruptEnd() SEOUL_OVERRIDE;

	// Return a platform dependent pointer to the system audio driver,
	// wrapped in a UnsafeHandle to prevent incorrect casting.
	virtual Bool GetPlatformDependentAudioDriverHandle(UnsafeHandle& rhAudioDriver) SEOUL_OVERRIDE;

	// Entry hook for the sound capture DSP callback.
	void OnSoundCapture(
		Float const* pfIn,
		UInt32 uSamples,
		UInt32 uChannels,
		UInt64 uClock,
		UInt32 uClockOffset,
		UInt32 uClockLength);

	// Utility, intended to check if any sounds are still playing
	// to track when activity has "settled". Returns true
	// only for one off events (not looping). Must be called
	// on the main thread.
	virtual Bool IsCategoryPlaying(HString categoryName, Bool bIncludeLoopingSounds = false) const SEOUL_OVERRIDE;

	// Tracking of FMOD's asynchronous processing thread.
	Atomic32Type GetAsyncPreMarker() const { return m_AsyncPreMarker; }
	Atomic32Type GetAsyncPostMarker() const { return m_AsyncPostMarker; }
	void OnPreAsyncUpdate() { ++m_AsyncPreMarker; }
	void OnPostAsyncUpdate() { ++m_AsyncPostMarker; }

	// Reset the master pitch cache so it needs
	// to be re-cached. Designed to be called
	// from threads other than the main thread.
	void DirtyMasterPitchCache()
	{
		m_fMasterPitch = -1.0f;
	}

private:
	virtual UInt64 GetClockTimeDSP() const SEOUL_OVERRIDE;

	Bool InternalCategoryMute(HString hsCategoryName, Bool bMute, Bool bAllowPending, Bool bSuppressLogging);

	void InternalShutdown();

	void InternalCreateSoundCaptureDSP();
	void InternalDestroySoundCaptureDSP();

	void InternalSuspendMixer();
	void InternalResumeMixer();

	friend struct Content::Traits<ProjectAnchor>;
	friend struct Content::Traits<EventAnchor>;
	Content::Store<ProjectAnchor> m_SoundProjects;
	Content::Store<EventAnchor> m_SoundEvents;
	Bool PrepareSoundProjectAnchorDelete(FilePath, Content::Entry<ProjectAnchor, FilePath>& entry);
	Bool PrepareSoundEventAnchorDelete(const ContentKey& key, Content::Entry<EventAnchor, ContentKey>& entry);
	Bool SoundProjectChange(FilePath filePath, const Content::Handle<ProjectAnchor>& hProjectAnchor);

	friend class BankFileLoader;
	friend class Event;
	friend class EventAnchor;
	friend class EventContentLoader;
	friend class ProjectContentLoader;
	ScopedPtr<ManagerFileManager> m_pFiles;
	CheckedPtr<::FMOD::Studio::System> m_pFMODStudioSystem;
	CheckedPtr<::FMOD::DSP> m_pFMODAudioCaptureDSP;
	SharedPtr<Camera> m_pCamera;
	Atomic32Value<Float> m_fMasterPitch;
	UInt32 m_uFlags;
	Bool m_bShuttingDown;
	Atomic32Value<Bool> m_bInBackground;
	Atomic32Value<Bool> m_bInterrupted;
	Atomic32Value<Bool> m_bMusicMuted;
	Atomic32 m_AsyncPreMarker;
	Atomic32 m_AsyncPostMarker;

	SEOUL_DISABLE_COPY(Manager);
}; // class FMODSound::Manager

} // namespace Seoul::FMODSound

#endif // /#if SEOUL_WITH_FMOD

#endif // include guard
