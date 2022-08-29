/**
 * \file SoundManager.h
 * \brief Singleton manager of sound effects and music in Seoul engine.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SOUND_MANAGER_H
#define	SOUND_MANAGER_H

#include "ContentStore.h"
#include "ContentLoadManager.h"
#include "Delegate.h"
#include "HashSet.h"
#include "HashTable.h"
#include "List.h"
#include "SharedPtr.h"
#include "Prereqs.h"
#include "Singleton.h"
#include "SoundEvent.h"
#include "UnsafeHandle.h"
#include "Vector3D.h"
namespace Seoul { class Camera; }

namespace Seoul::Sound
{

namespace CategoryNames
{

/** Standard category for the overall mix. */
static const HString khsSoundCategoryMaster("bus:/");

/** Standard category for the music sub mix. */
static const HString khsSoundCategoryMusic("bus:/music");

/** Standard categories for sound FX and sound FX that should be pitched shifted with scaled time stepping. */
static const HString khsSoundCategorySFX("bus:/SFX");
static const HString khsSoundCategorySFXPitched("bus:/SFX/pitched");

} // namespace Sound::CategoryNames

class SampleData SEOUL_SEALED
{
public:
	SampleData(
		UInt64 uFrame,
		UInt64 uOffsetInSamples,
		UInt32 uSizeInSamples,
		UInt32 uChannels,
		Float const* pfData,
		UInt32 uPaddingInSamples);
	~SampleData();

	UInt32 GetChannels() const { return m_uChannels; }
	UInt64 GetFrame() const { return m_uFrame; }
	UInt64 GetOffsetInSamples() const { return m_uOffsetInSamples; }
	UInt32 GetSizeInBytes() const { return (m_uSizeInSamples * m_uChannels * sizeof(Float)); }
	UInt32 GetSizeInSamples() const { return m_uSizeInSamples; }
	Float const* GetData() const { return m_pfData; }

private:
	UInt64 const m_uFrame;
	UInt64 const m_uOffsetInSamples;
	UInt32 const m_uSizeInSamples;
	UInt32 const m_uChannels;
	Float* m_pfData;

	SEOUL_REFERENCE_COUNTED(SampleData);
	SEOUL_DISABLE_COPY(SampleData);
}; // class Sound::SampleData

/**
 * Interface that can be implemented to capture
 * raw audio data from the master bus. Will
 * receive update events until released
 * (until the audio system has the
 * last reference to the IAudioCapture instance).
 *
 * NOTE: Update events may be delievered out of order but
 * will not contain holes. It is the callee's responsibility
 * to check uOffsetInBytes and uSizeInBytes of OnSamples()
 * and reorder chunks as needed.
 */
class ICapture SEOUL_ABSTRACT
{
public:
	virtual ~ICapture()
	{
	}

	virtual void OnSamples(const SharedPtr<SampleData>& pData) = 0;

protected:
	SEOUL_REFERENCE_COUNTED(ICapture);

	ICapture()
	{
	}

private:
	SEOUL_DISABLE_COPY(ICapture);
}; // class ICapture

enum class ManagerType
{
	kFMOD,
	kNull,
};

class Settings SEOUL_SEALED
{
public:
	Settings(): m_bMusicEnabled(true), m_bSoundEffectsEnabled(true) {}

	Bool GetMusicEnabled() const { return m_bMusicEnabled; }
	void SetMusicEnabled(Bool bMusicEnabled) { m_bMusicEnabled = bMusicEnabled; }

	Bool GetSoundEffectsEnabled() const { return m_bSoundEffectsEnabled; }
	void SetSoundEffectsEnabled(Bool bSoundEffectsEnabled) { m_bSoundEffectsEnabled = bSoundEffectsEnabled; }

private:
	Bool m_bMusicEnabled;
	Bool m_bSoundEffectsEnabled;

	SEOUL_DISABLE_COPY(Settings);
}; // class Sound::Settings

/**
 * Sound::Manager handles loading/unloading of sound projects
 * and attached sound banks. It provides a method to get Sound::Event instances,
 * which can be thought of as sound effects, except that Sound::Events are more
 * complex (they can contain multiple wave files, effect processing, and
 * can react to runtime variables to respond to gameplay events).
 */
class Manager SEOUL_ABSTRACT : public Singleton<Manager>
{
public:
	SEOUL_DELEGATE_TARGET(Manager);

	virtual ~Manager();

	virtual ManagerType GetType() const = 0;

	/**
	 * Return the default Sound project file for the application.
	 * Sound events that do not explicitly specify a project file
	 * are expected to default to this file.
	 */
	FilePath GetDefaultProjectFilePath() const
	{
		return m_DefaultProjectFilePath;
	}

	virtual Bool IsInitialized() const = 0;

	virtual void SetListenerCamera(const SharedPtr<Camera>& pCamera) = 0;

	virtual void AssociateSoundEvent(const ContentKey& soundEventKey, Event& rEvent) = 0;
	virtual Event* NewSoundEvent() const = 0;
	virtual void Tick(Float fDeltaTime);

	virtual Bool SetCategoryPaused(HString categoryName, Bool bPaused = true) = 0;

	virtual Bool SetMasterMute(Bool bMuted = true) = 0;
	virtual Bool SetMasterPaused(Bool bPaused = true) = 0;
	virtual Bool SetMasterVolume(Float fVolume) = 0;

	virtual Bool SetCategoryMute(HString hsCategoryName, Bool bMute = true, Bool bAllowPending = false, Bool bSuppressLogging = false) = 0;
	virtual Bool SetCategoryVolume(HString hsCategoryName, Float fVolume, Float fFadeTimeInSeconds = 0.0f, Bool bAllowPending = false, Bool bSuppressLogging = false) = 0;
	virtual Float GetCategoryVolume(HString hsCategoryName) const = 0;

	virtual Bool GetMasterAttributes(UInt32& ruSamplingRate, UInt32& ruChannels) const = 0;

	virtual void SetMaster3DAttributes(
		Float fDopplerShiftScale = 1.0f,
		Float fUnitsPerMeter = 100.0f,
		Float f3DRolloffScale = 1.0f) = 0;

	virtual UInt32 GetTotalMemoryUsageInBytes() const = 0;

	// Return a platform dependent pointer to the system audio driver,
	// wrapped in a UnsafeHandle to prevent incorrect casting.
	virtual Bool GetPlatformDependentAudioDriverHandle(UnsafeHandle& rhAudioDriver) = 0;

	// Utility, check if any sounds are playing in the given category.
	virtual Bool IsCategoryPlaying(HString categoryName, Bool bIncludeLoopingSounds = false) const = 0;

	// Enter and leave background.
	virtual void OnEnterBackground() = 0;
	virtual void OnLeaveBackground() = 0;

	// iOS specific hooks for audio interrupt start and end events.
	virtual void OnInterruptStart() = 0;
	virtual void OnInterruptEnd() = 0;

	// Sound capture hooking.
	void RegisterSoundCapture(
		const SharedPtr<ICapture>& pCapture,
		const ThreadId& callbackThreadId = ThreadId());

	void ApplySoundSettings(const Settings& soundSettings);

protected:
	Manager();

	void DeferCategoryMute(HString sCategoryName, Bool bMute);
	void DeferCategoryVolume(HString sCategoryName, Float fVolume);
	void DeferCategoryVolume(HString sCategoryName, Float fStartVolume, Float fEndVolume, Float fSeconds);

	virtual UInt64 GetClockTimeDSP() const { return 0u; }

	// List of sound capture instances.
	struct CaptureEntry SEOUL_SEALED
	{
		SharedPtr<ICapture> m_p{};
		ThreadId m_ThreadId{};
		UInt64 m_uOffsetInSamples{};
		UInt64 m_uFrame{};
		UInt64 m_uStartClockTime{};
	}; // struct Sound::Capture

	typedef Vector<CaptureEntry, MemoryBudgets::Audio> Capture;
	Capture m_vSoundCapture;
	Mutex m_SoundCapture;

private:
	FilePath const m_DefaultProjectFilePath;

	struct PendingSetCategoryMute SEOUL_SEALED
	{
		PendingSetCategoryMute()
			: m_sCategoryName()
			, m_bMute(false)
		{
		}

		PendingSetCategoryMute(HString sCategoryName, Bool bMute)
			: m_sCategoryName(sCategoryName)
			, m_bMute(bMute)
		{
		}

		Bool operator==(HString name) const
		{
			return (m_sCategoryName == name);
		}

		Bool operator!=(HString name) const
		{
			return !(*this == name);
		}

		/**
		 * Commit the current state of this category update.
		 *
		 * @return True if this updates mute changed was applied successfully, false otherwise.
		 */
		Bool Apply()
		{
			return (Manager::Get()->SetCategoryMute(m_sCategoryName, m_bMute, false, true));
		}

		HString m_sCategoryName;
		Bool m_bMute;
	}; // struct PendingSetCategoryMute

	struct PendingSetCategoryVolume SEOUL_SEALED
	{
		PendingSetCategoryVolume()
			: m_sCategoryName()
			, m_fStartVolume(0.0f)
			, m_fEndVolume(0.0f)
			, m_fTargetSeconds(0.0f)
			, m_fElapsedSeconds(0.0f)
		{
		}

		PendingSetCategoryVolume(HString sCategoryName, Float fVolume)
			: m_sCategoryName(sCategoryName)
			, m_fStartVolume(fVolume)
			, m_fEndVolume(fVolume)
			, m_fTargetSeconds(0.0f)
			, m_fElapsedSeconds(0.0f)
		{
		}

		PendingSetCategoryVolume(HString sCategoryName, Float fStartVolume, Float fEndVolume, Float fSeconds)
			: m_sCategoryName(sCategoryName)
			, m_fStartVolume(fStartVolume)
			, m_fEndVolume(fEndVolume)
			, m_fTargetSeconds(fSeconds)
			, m_fElapsedSeconds(0.0f)
		{
		}

		Bool operator==(HString name) const
		{
			return (m_sCategoryName == name);
		}

		Bool operator!=(HString name) const
		{
			return !(*this == name);
		}

		/**
		 * Commit the current state of this category update.
		 *
		 * @return True if this update has reached its target elapsed time, false otherwise.
		 */
		Bool Apply()
		{
			// Divide by zero here is ok, Clamp() is NaN safe.
			Float const fAlpha = Clamp(m_fElapsedSeconds / m_fTargetSeconds, 0.0f, 1.0f);
			Float const fVolume = Lerp(m_fStartVolume, m_fEndVolume, fAlpha);

			return (Manager::Get()->SetCategoryVolume(m_sCategoryName, fVolume, 0.0f, false, true) && (1.0f == fAlpha));
		}

		/**
		 * Tick the elapsed time of this category volume.
		 */
		void Tick(Float fDeltaTimeInSeconds)
		{
			m_fElapsedSeconds += fDeltaTimeInSeconds;
		}

		HString m_sCategoryName;
		Float m_fStartVolume;
		Float m_fEndVolume;
		Float m_fTargetSeconds;
		Float m_fElapsedSeconds;
	}; // struct PendingSetCategoryVolume

	void InternalDeferCategoryVolume(const PendingSetCategoryVolume& pendingSetCategoryVolume);
	void InternalProcessPendingSetCategoryMutes();
	void InternalProcessPendingSetCategoryVolumes(Float fDeltaTimeInSeconds);
	void InternalProcessSoundCapture();

	// List of pending volume changes to allow setting category volumes before
	// any events in those categories have been loaded or to allow volume
	// changes over time.
	typedef Vector<PendingSetCategoryVolume, MemoryBudgets::Audio> PendingSetCategoryVolumesList;
	PendingSetCategoryVolumesList m_vPendingSetCategoryVolumes;

	// List of pending mute changes to allow setting category mute states before
	// any events in those categories have been loaded.
	typedef Vector<PendingSetCategoryMute, MemoryBudgets::Audio> PendingSetCategoryMutesList;
	PendingSetCategoryMutesList m_vPendingSetCategoryMutes;

	SEOUL_DISABLE_COPY(Manager);
}; // class Sound::Manager

/**
 * Placeholder implementation of SoundManager. Can be used for headless applications
 * or as a placeholder for platforms that do not support audio.
 */
class NullManager SEOUL_SEALED : public Manager
{
public:
	NullManager();
	~NullManager();

	virtual ManagerType GetType() const SEOUL_OVERRIDE { return ManagerType::kNull; }

	virtual Bool IsInitialized() const SEOUL_OVERRIDE
	{
		return true;
	}

	virtual void SetListenerCamera(const SharedPtr<Camera>& pCamera) SEOUL_OVERRIDE
	{
		// Nop
	}

	virtual void AssociateSoundEvent(const ContentKey& soundEventKey, Event& rEvent) SEOUL_OVERRIDE;

	virtual Event* NewSoundEvent() const SEOUL_OVERRIDE;

	virtual Bool SetCategoryPaused(HString categoryName, Bool bPaused = true) SEOUL_OVERRIDE
	{
		return true;
	}

	virtual Bool SetMasterMute(Bool bMuted = true) SEOUL_OVERRIDE
	{
		return true;
	}

	virtual Bool SetMasterPaused(Bool bPaused = true) SEOUL_OVERRIDE
	{
		return true;
	}

	virtual Bool SetMasterVolume(Float fVolume) SEOUL_OVERRIDE
	{
		return true;
	}

	virtual Bool SetCategoryMute(HString hsCategoryName, Bool bMute = true, Bool bAllowPending = false, Bool bSuppressLogging = false) SEOUL_OVERRIDE
	{
		return true;
	}

	virtual Bool SetCategoryVolume(HString hsCategoryName, Float fVolume, Float fFadeTimeInSeconds = 0.0f, Bool bAllowPending = false, Bool bSuppressLogging = false) SEOUL_OVERRIDE
	{
		return true;
	}

	virtual Float GetCategoryVolume(HString hsCategoryName) const SEOUL_OVERRIDE
	{
		return 0.0f;
	}

	virtual Bool GetMasterAttributes(UInt32& ruSamplingRate, UInt32& ruChannels) const SEOUL_OVERRIDE
	{
		return false;
	}

	virtual void SetMaster3DAttributes(
		Float fDopplerShiftScale = 1.0f,
		Float fUnitsPerMeter = 100.0f,
		Float f3DRolloffScale = 1.0f) SEOUL_OVERRIDE
	{
		// Nop
	}

	virtual UInt32 GetTotalMemoryUsageInBytes() const SEOUL_OVERRIDE
	{
		return 0u;
	}

	// Utility, intended to check if any sounds are still playing
	// to track when activity has "settled". Returns true
	// only for one off events (not looping). Must be called
	// on the main thread.
	virtual Bool IsCategoryPlaying(HString categoryName, Bool bIncludeLoopingSounds = false) const SEOUL_OVERRIDE
	{
		return false;
	}

	// Enter and leave background.
	virtual void OnEnterBackground() SEOUL_OVERRIDE
	{
	}

	virtual void OnLeaveBackground() SEOUL_OVERRIDE
	{
	}

	// iOS specific hooks for audio interrupt start and end events.
	virtual void OnInterruptStart() SEOUL_OVERRIDE
	{
	}

	virtual void OnInterruptEnd() SEOUL_OVERRIDE
	{
	}

	virtual Bool GetPlatformDependentAudioDriverHandle(UnsafeHandle& rhAudioDriver) SEOUL_OVERRIDE
	{
		return false;
	}

private:
	SEOUL_DISABLE_COPY(NullManager);
}; // class NullManager

} // namespace Seoul::Sound

#endif // include guard
