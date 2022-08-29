/**
 * \file FMODSoundManager.cpp
 * \brief Singleton manager of sound effects and music in Seoul engine,
 * implemented with the FMOD Ex sound system.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Camera.h"
#include "ContentLoadManager.h"
#include "Engine.h"
#include "FileManager.h"
#include "FMODSoundManager.h"
#include "FMODSoundUtil.h"
#include "GamePaths.h"
#include "JobsFunction.h"
#include "Logger.h"
#include "SoundUtil.h"
#include "StackOrHeapArray.h"
#include "UnsafeHandle.h"

#if SEOUL_WITH_FMOD
#include "fmod_studio.h"
#include "fmod_studio.hpp"

namespace Seoul
{

namespace FMODSound
{

// Defined per platform.
#if SEOUL_PLATFORM_IOS
Bool SoundManagerIsExternalAudioPlaying();
#else
Bool SoundManagerIsExternalAudioPlaying()
{
	// Always false currently - may want to look into methods on Android.
	return false;
}
#endif

#if SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_WINDOWS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
static void* GetExtraDriverData()
{
	return nullptr;
}

/**
 * Set rhAudioDriver to a pointer to the platform-dependent
 * audio system driver for the current platform.
 *
 * @return True if rhAudioDriver was set to a valid pointer, false otherwise.
 * If this method returns false, rhAudioDriver is left unchanged.
 */
Bool Manager::GetPlatformDependentAudioDriverHandle(UnsafeHandle& rhAudioDriver)
{
	return false;
}
#else
#error "Define for this platform."
#endif

/** Maximum # of simultaneous audio channels allowed. */
static const Int kMaxAudioChannels = 64;

/** How much to scale the FMOD doppler shift effect - 1.0 is the FMOD default. */
static const Float kDopplerShiftScale = 1.0f;

/** Scaling factor, so FMOD knows how many game units = 1.0 meter. */
static const Float kUnitsPerMeter = 100.0f;

/** 3D attenuation rolloff scale = 1.0 is the FMOD default. */
static const Float k3DRolloffScale = 1.0f;

/**
 * Handles all async requests from FMOD to read file data,
 * including mapping single files to multi-files and management
 * around file data that may be network serviced.
 */
class ManagerFileManager SEOUL_SEALED : public Singleton<ManagerFileManager>
{
public:
	SEOUL_DELEGATE_TARGET(ManagerFileManager);

	ManagerFileManager()
	{
	}

	~ManagerFileManager()
	{
	}

	/** Close the given file - all reads will fail immediately after a file has been closed. */
	FMOD_RESULT Close(void* pHandle)
	{
		// Null handling.
		if (nullptr == pHandle)
		{
			return FMOD_OK;
		}

		auto pFMODFile((FileFmod*)pHandle);
		SEOUL_ASSERT(nullptr != pFMODFile);

		// Cleanup.
		{
			SharedPtr<FileFmod> pFile(pFMODFile);

			// Mark the file as closed, then release FMOD's reference on it.
			pFile->Close();
		}

		// WARNING: Do not access FileFmod after this dereference call, it
		// may be destroyed.
		SeoulGlobalDecrementReferenceCount(pFMODFile);
		pFMODFile = nullptr;

		return FMOD_OK;
	}

	/** Open a new file for FMOD servicing. */
	FMOD_RESULT Open(FilePath filePath, UInt* puFileSize, void** ppHandle)
	{
		auto pFileSystem(FileManager::Get());

		// Files must be locally cached or we have a cooker bug.
		if (pFileSystem->IsServicedByNetwork(filePath))
		{
			// Unexpected unless the engine is shutting down - during shutdown,
			// network I/O is disabled and requests can be terminated in this
			// manner.
#if SEOUL_LOGGING_ENABLED
			if (Content::LoadManager::Get()->GetLoadContext() != Content::LoadContext::kShutdown)
			{
				SEOUL_WARN("%s: FMOD request for basic FMOD file type that is network serviced, programmer error and unexpected.", filePath.CStr());
			}
#endif

			return FMOD_ERR_FILE_NOTFOUND;
		}

		// Open the file.
		ScopedPtr<SyncFile> pFile;

		// Special handling for the MasterBank.strings.bank file, since it is obfuscated.
		if (SoundUtil::IsStringsBank(filePath))
		{
			// Read and (de)obfuscate - obfuscation is the inverse of itself.
			void* p = nullptr;
			UInt32 u = 0u;
			if (!SoundUtil::ReadAllAndObfuscate(filePath, p, u))
			{
				return FMOD_ERR_FILE_NOTFOUND;
			}

			// Assign.
			pFile.Reset(SEOUL_NEW(MemoryBudgets::Audio) FullyBufferedSyncFile(
				p, u, true, filePath.GetAbsoluteFilename()));
		}
		// Typical handling.
		else
		{
			if (!pFileSystem->OpenFile(filePath, File::kRead, pFile))
			{
				return FMOD_ERR_FILE_NOTFOUND;
			}
		}

		// Get the size.
		auto uSize = pFile->GetSize();
		// Check it.
		if (uSize > (UInt64)UIntMax)
		{
			return FMOD_ERR_FILE_BAD;
		}


		// Generate the file.
		SharedPtr<FileFmod> pFMODFile(SEOUL_NEW(MemoryBudgets::Audio) FileFmod(RvalRef(pFile)));

		// Output the size.
		*puFileSize = (UInt)uSize;

		// FMOD now has a reference.
		SeoulGlobalIncrementReferenceCount(pFMODFile.GetPtr());
		*ppHandle = (void*)pFMODFile.GetPtr();

		return FMOD_OK;
	}

	/** Perform a read targeting the given file. */
	FMOD_RESULT Read(
		void* pHandle,
		void* pBuffer,
		UInt32 uSizeBytes,
		UInt32* puBytesRead)
	{
		auto pFMODFile((FileFmod*)pHandle);
		SEOUL_ASSERT(nullptr != pFMODFile);

		return pFMODFile->Read(pBuffer, uSizeBytes, *puBytesRead);
	}

	/** Perform a seek targeting the given file. */
	FMOD_RESULT Seek(
		void* pHandle,
		UInt32 uPosition)
	{
		auto pFMODFile((FileFmod*)pHandle);
		SEOUL_ASSERT(nullptr != pFMODFile);

		return pFMODFile->Seek(uPosition);
	}

private:
	SEOUL_DISABLE_COPY(ManagerFileManager);

	/** Wrapper around data and behavior for handling file read requests from FMOD. */
	class FileFmod SEOUL_SEALED
	{
	public:
		FileFmod(ScopedPtr<SyncFile>&& rp)
			: m_Mutex()
			, m_pFile(RvalRef(rp))
		{
		}

		/** Close this file - any new read attempts will fail immediately after Close() has been called. */
		void Close()
		{
			Lock lock(m_Mutex);

			// Close.
			m_pFile.Reset();
		}

		/** @return true if Close() has been called on this FMODFile. */
		Bool IsClosed() const { return !m_pFile.IsValid(); }

		/** Attempt to perform a read. */
		FMOD_RESULT Read(
			void* pBuffer,
			UInt32 uSizeBytes,
			UInt32& ruBytesRead)
		{
			Lock lock(m_Mutex);

			// If closed, cancel the read immediately.
			if (!m_pFile.IsValid())
			{
				// Immediate cancellation.
				ruBytesRead = 0u;
				return FMOD_ERR_FILE_EOF;
			}

			// Perform the read.
			ruBytesRead = m_pFile->ReadRawData(pBuffer, uSizeBytes);
			return (uSizeBytes == ruBytesRead ? FMOD_OK : FMOD_ERR_FILE_EOF);
		}

		/** Update the next read position of the file. */
		FMOD_RESULT Seek(UInt32 uPosition)
		{
			Lock lock(m_Mutex);

			// Perform the seek;
			if (!m_pFile->Seek((Int64)uPosition, File::kSeekFromStart))
			{
				return FMOD_ERR_FILE_COULDNOTSEEK;
			}
			else
			{
				return FMOD_OK;
			}
		}

	private:
		Mutex m_Mutex;
		ScopedPtr<SyncFile> m_pFile;

		SEOUL_DISABLE_COPY(FileFmod);
		SEOUL_REFERENCE_COUNTED(FileFmod);
	}; // class FileFmod
}; // class FMODSound::ManagerFileManager

/** FMOD file open. */
static FMOD_RESULT F_CALLBACK InternalStaticFmodFileOpen(
	Byte const* sName,
	UInt* puFileSize,
	void** ppHandle,
	void* /*pUserData*/)
{
	// Construct a file path - we only support reads of files
	// that have supported extensions and paths.
	FilePath filePath = FilePath::CreateContentFilePath(sName);

	// Cast to the manager type.
	auto pMan(ManagerFileManager::Get());

	// Perform the open.
	return pMan->Open(filePath, puFileSize, ppHandle);
}

/** FMOD file close. */
static FMOD_RESULT F_CALLBACK InternalStaticFmodFileClose(
	void* pHandle,
	void* /*pUserData*/)
{
	// Cast to the manager type.
	auto pMan(ManagerFileManager::Get());

	// Perform the close.
	return pMan->Close(pHandle);
}

/** FMOD read implementation. */
static FMOD_RESULT F_CALLBACK InternalStaticFmodFileRead(
	void* pHandle,
	void* pBuffer,
	UInt32 uSizeBytes,
	UInt32* puBytesRead,
	void* /*pUserData*/)
{
	// Cast to the manager type.
	auto pMan(ManagerFileManager::Get());

	// Perform the read.
	return pMan->Read(pHandle, pBuffer, uSizeBytes, puBytesRead);
}

/* FMODseek implementation. */
static FMOD_RESULT F_CALLBACK InternalStaticFmodFileSeek(
	void* pHandle,
	UInt32 uPosition,
	void* /*pUserData*/)
{
	// Cast to the manager type.
	auto pMan(ManagerFileManager::Get());

	// Perform the read.
	return pMan->Seek(pHandle, uPosition);
}

static FMOD_RESULT StudioSystemCallback(
	FMOD_STUDIO_SYSTEM* pSystem,
	FMOD_STUDIO_SYSTEM_CALLBACK_TYPE eType,
	void *commanddata,
	void *userdata)
{
	// Track async updates - used by loader code to
	// guarantee that loaded sample state is accurate
	// (it appears that getSampleLoadedState() and
	// getLoadedState() can prematurely return
	// loaded if we don't give the FMOD async thread
	// enough time to finish processing).
	if (eType == FMOD_STUDIO_SYSTEM_CALLBACK_PREUPDATE)
	{
		Manager::Get()->OnPreAsyncUpdate();
	}
	else if (eType == FMOD_STUDIO_SYSTEM_CALLBACK_POSTUPDATE)
	{
		Manager::Get()->OnPostAsyncUpdate();
	}

	return FMOD_OK;
}

/**
 * Instantiate and initialize the FMOD::Studio::System - handles
 * platform specific details that vary per platform when initializing the
 * FMOD::Studio::System object.
 */
static FMOD_RESULT InternalStaticInitializeFMODStudioSystem(
	UInt32& ruFlags,
	::FMOD::Studio::System*& rpFMODStudioSystem)
{
	FMOD_VERIFY(::FMOD::Studio::System::create(&rpFMODStudioSystem));
	SEOUL_ASSERT(nullptr != rpFMODStudioSystem);

	FMOD_INITFLAGS eFMODInitFlags = FMOD_INIT_3D_RIGHTHANDED /* TODO: | FMOD_INIT_ASYNCREAD_FAST*/;
	FMOD_STUDIO_INITFLAGS eFMODStudioInitFlags = FMOD_STUDIO_INIT_NORMAL;

	// Attempt to enable profiling and the FMOD net interface in
	// non-ship builds.
#if !SEOUL_SHIP
	eFMODInitFlags |= FMOD_INIT_PROFILE_ENABLE;
	eFMODStudioInitFlags |= FMOD_STUDIO_INIT_LIVEUPDATE;
	ruFlags |= Manager::kEnableNetInterface;
#endif

	// Get the FMOD system object.
	::FMOD::System* pFMODSystem = nullptr;
	FMOD_VERIFY(rpFMODStudioSystem->getCoreSystem(&pFMODSystem));
	SEOUL_ASSERT(pFMODSystem);

	// Check that the FMOD DLL is the correct version.
	UInt32 uFMODDllVersion = 0u;
	FMOD_VERIFY(pFMODSystem->getVersion(&uFMODDllVersion));

	// A version mismatch is an unrecoverable error, return failure if this happens.
	if (uFMODDllVersion < FMOD_VERSION)
	{
		FMOD_VERIFY(rpFMODStudioSystem->release());
		rpFMODStudioSystem = nullptr;
		return FMOD_ERR_VERSION;
	}

	// Check for available audio device drivers.
	Int32 nDrivers = 0;
	FMOD_VERIFY(pFMODSystem->getNumDrivers(&nDrivers));

	// Disable sound output if the system reports that there is no output device,
	// or if headless mode has been requested.
	if (nDrivers < 1 ||
		Manager::kHeadless == (Manager::kHeadless & ruFlags))
	{
		if (Manager::kNonRealTime == (Manager::kNonRealTime & ruFlags))
		{
			FMOD_VERIFY(pFMODSystem->setOutput(FMOD_OUTPUTTYPE_NOSOUND_NRT));
		}
		else
		{
			FMOD_VERIFY(pFMODSystem->setOutput(FMOD_OUTPUTTYPE_NOSOUND));
		}
	}

	// Set up advanced settings.
	{
		// Configure the input parts of settings for getAdvancedSettings().
		FMOD_STUDIO_ADVANCEDSETTINGS settings;
		memset(&settings, 0, sizeof(settings));
		settings.cbsize = sizeof(settings);

		// Update the event queue size. Large to accomodate network file reads.
		settings.commandqueuesize = 128u * 1024u; /* 32u * 1024u is the default */

		// Commit updated advanced settings.
		FMOD_VERIFY(rpFMODStudioSystem->setAdvancedSettings(&settings));
	}

	// The first initialization attempt can fail in a way we can recover from, so
	// track the return value and react to it.
	FMOD_RESULT eResult = rpFMODStudioSystem->initialize(
		kMaxAudioChannels,			// max # of simultaneous audio channels
		eFMODStudioInitFlags,       // studio init flags
		eFMODInitFlags,				// init flags
		GetExtraDriverData());		// platform specific data

	// Handle a failure due to an attempt to enable the net interface
#if !SEOUL_SHIP
	if (FMOD_OK != eResult)
	{
		eFMODStudioInitFlags &= ~FMOD_STUDIO_INIT_LIVEUPDATE;
		eFMODInitFlags &= ~FMOD_INIT_PROFILE_ENABLE;
		ruFlags &= ~Manager::kEnableNetInterface;

		eResult = rpFMODStudioSystem->initialize(
				kMaxAudioChannels,			// max # of simultaneous audio channels
				eFMODStudioInitFlags,       // studio init flags
				eFMODInitFlags,				// init flags
				GetExtraDriverData());		// platform specific data
	}
#endif

	// Cleanup the event system pointer before returning if initialization failed.
	if (FMOD_OK != eResult)
	{
		(void)rpFMODStudioSystem->release();
		rpFMODStudioSystem = nullptr;
	}

	return eResult;
}

// Define FMOD allocator hooks.
void* F_CALLBACK MallocHook(unsigned int zSizeInBytes, FMOD_MEMORY_TYPE /*type*/, const char* /*sourcestr*/)
{
	return MemoryManager::Allocate((size_t)zSizeInBytes, MemoryBudgets::Audio);
}

void* F_CALLBACK ReallocHook(void* pPointerToReallocate, unsigned int zSizeInBytes, FMOD_MEMORY_TYPE /*type*/, const char* /*sourcestr*/)
{
	return MemoryManager::Reallocate(pPointerToReallocate, zSizeInBytes, MemoryBudgets::Audio);
}

void F_CALLBACK FreeHook(void* pPointerToFree, FMOD_MEMORY_TYPE /*type*/, const char* /*sourcestr*/)
{
	return MemoryManager::Deallocate(pPointerToFree);
}

// Audio capture DSP hook.
static FMOD_RESULT F_CALLBACK AudioCaptureDSPCallback(
	FMOD_DSP_STATE* pState,
	Float* pfIn,
	Float* pfOut,
	UInt32 uLength,
	Int32 iInChannels,
	Int32* piOutChannels)
{
	void* pSoundManager = nullptr;
	FMOD_VERIFY(pState->functions->getuserdata(pState, &pSoundManager));

	// Get clock values for the samples.
	unsigned long long uClock = 0u;
	UInt32 uClockOffset = 0u;
	UInt32 uClockLength = 0u;
	FMOD_VERIFY(pState->functions->getclock(pState, &uClock, &uClockOffset, &uClockLength));

	// Pass the data through to FMODSoundManager.
	auto& r = *((Manager*)pSoundManager);
	r.OnSoundCapture(
		pfIn,
		uLength,
		iInChannels,
		(UInt64)uClock,
		uClockOffset,
		uClockLength);

	// TODO: This is fine for our current use cases but will not be fine
	// if a use case wants audio capture+playback.
	return FMOD_ERR_DSP_SILENCE;
}

/**
 * Construct and initialize the sound manager. Sets up the FMOD
 * event system and FMOD Ex backend and configures the backend with
 * Seoul engine specific default settings.
 */
Manager::Manager(UInt32 uFlags /*= kNone*/)
	: m_pFiles(SEOUL_NEW(MemoryBudgets::Audio) ManagerFileManager)
	, m_pFMODStudioSystem(nullptr)
	, m_pCamera(nullptr)
	, m_fMasterPitch(1.0f)
	, m_uFlags(uFlags)
	, m_bShuttingDown(false)
	, m_bInBackground(false)
	, m_bInterrupted(false)
	, m_bMusicMuted(false)
	, m_AsyncPreMarker(0)
	, m_AsyncPostMarker(0)
{
	SEOUL_ASSERT(IsMainThread());

	// Hook up our memory allocation hooks.
	FMOD_VERIFY(::FMOD::Memory_Initialize(
		nullptr,
		0,
		MallocHook,
		ReallocHook,
		FreeHook));

	// Create an instance of the FMOD event system.
	::FMOD::Studio::System* pFMODStudioSystem = nullptr;
	FMOD_RESULT err = InternalStaticInitializeFMODStudioSystem(m_uFlags, pFMODStudioSystem);

	if (err != FMOD_OK)
	{
		SEOUL_WARN("FMOD::Studio::System::init() failed: %s (0x%08x).  Audio will be disabled.\n", FMOD_ErrorString(err), err);

		// Some sort of critical audio system failure on PC - allow game to continue with
		// disabled audio.
		return;
	}

	// Set the callback.
	FMOD_VERIFY(pFMODStudioSystem->setCallback(StudioSystemCallback, FMOD_STUDIO_SYSTEM_CALLBACK_PREUPDATE | FMOD_STUDIO_SYSTEM_CALLBACK_POSTUPDATE));

	// Get the FMOD system object.
	::FMOD::System* pFMODSystem = nullptr;
	FMOD_VERIFY(pFMODStudioSystem->getCoreSystem(&pFMODSystem));
	SEOUL_ASSERT(pFMODSystem);

	// Set up file handlers.
	FMOD_VERIFY(pFMODSystem->setFileSystem(
		InternalStaticFmodFileOpen,
		InternalStaticFmodFileClose,
		InternalStaticFmodFileRead,
		InternalStaticFmodFileSeek,
		nullptr,
		nullptr,
		-1));

	// Set up global 3D settings in FMOD.
	FMOD_VERIFY(pFMODSystem->set3DSettings(
		kDopplerShiftScale,
		kUnitsPerMeter,
		k3DRolloffScale));

	// Done, store the event system.
	m_pFMODStudioSystem = pFMODStudioSystem;

	// Create the sound capture DSP if we're a headless FMOD.
	if (Manager::kHeadless == (Manager::kHeadless & uFlags))
	{
		InternalCreateSoundCaptureDSP();
	}
}

/**
 * The FMODSound::Manager destructor cleans up FMOD and unloads any loaded
 * audio data.
 */
Manager::~Manager()
{
	SEOUL_ASSERT(IsMainThread());

	// Make sure we're not in the background on shutdown.
	OnLeaveBackground(); SEOUL_TEARDOWN_TRACE();

	// Unload any loaded audio data.
	InternalShutdown(); SEOUL_TEARDOWN_TRACE();

	// Release the FMOD sound system and reset this FMODSound::Manager's pointer.
	if (m_pFMODStudioSystem.IsValid())
	{
		FMOD_VERIFY(m_pFMODStudioSystem->release()); SEOUL_TEARDOWN_TRACE();
		m_pFMODStudioSystem.Reset(); SEOUL_TEARDOWN_TRACE();
	}
}

/** Tests if the audio system was successfully initialized */
Bool Manager::IsInitialized() const
{
	return m_pFMODStudioSystem.IsValid();
}

/**
 * Ticks the sound manager, performing per-frame update operations.
 *
 * @param fDeltaTime Time in seconds since the last frame.
 *
 * Any Event callbacks will happen within the scope of this function.
 */
void Manager::Tick(Float fDeltaTime)
{
	SEOUL_ASSERT(IsMainThread());

	// Control music channel mute state based on whether
	// an external audio source is playing. Implementation of
	// the query is platform dependent.
	InternalCategoryMute(Sound::CategoryNames::khsSoundCategoryMusic, m_bMusicMuted || SoundManagerIsExternalAudioPlaying(), false, true);

	// Let the base class do some work.
	Sound::Manager::Tick(fDeltaTime);

	CheckedPtr<::FMOD::Studio::System> pFMODStudioSystem = m_pFMODStudioSystem;

	// Nothing to do if no audio system
	if (pFMODStudioSystem == nullptr)
	{
		return;
	}

	// Change pitch shift on time scale change.
	if (Engine::Get())
	{
#if SEOUL_ENABLE_CHEATS
		auto fDesiredPitchShift = (Float)(Engine::Get()->GetSecondsInTickScale() * Engine::Get()->GetDevOnlyGlobalTickScale());
#else
		auto fDesiredPitchShift = (Float)Engine::Get()->GetSecondsInTickScale();
#endif

		// Sanitizing, snap to 1.0f if close.
		if (Equals(fDesiredPitchShift, 1.0f, 1e-3f))
		{
			fDesiredPitchShift = 1.0f;
		}

		// TODO: Ideally we'd just use getPitch() here to check
		// the current pitch on the specified channel group, but that
		// introduces a periodic (rare) crash on Android. The workaround
		// until I get some followup from FMOD support is to maintain
		// the expected pitch in m_fMasterPitch and then periodically
		// dirty the expected pitch on events that seem likely to invalidate
		// it.

		// If desired differs from actual (as cached),
		// update it.
		Float const fMasterPitch = m_fMasterPitch;
		Float fNewMasterPitch = fMasterPitch;
		if (fDesiredPitchShift != fMasterPitch)
		{
			// Get the SFX bus - return true if it was found, false otherwise.
			::FMOD::Studio::Bus* pBus = nullptr;
			if (FMOD_OK == m_pFMODStudioSystem->getBus(
				Sound::CategoryNames::khsSoundCategorySFXPitched.CStr(),
				&pBus))
			{
				// Get the channel group of the SFX bus.
				::FMOD::ChannelGroup* pChannelGroup = nullptr;
				if (FMOD_OK == pBus->getChannelGroup(&pChannelGroup))
				{
					// Update the pitch.
					if (FMOD_OK == pChannelGroup->setPitch(fDesiredPitchShift))
					{
						// Update on success.
						fNewMasterPitch = fDesiredPitchShift;
					}
				}
			}
		}

		// Recache on changes, if the cache didn't change out from under us.
		if (fNewMasterPitch != fMasterPitch)
		{
			m_fMasterPitch.CompareAndSet(fNewMasterPitch, fMasterPitch);
		}
	}

	// Update listener attributes if we have a camera.
	if (m_pCamera.IsValid())
	{
		// Calculate the listener reference position.
		const Vector3D vCameraPosition = m_pCamera->GetPosition();

		// Set the current listener attributes to FMOD.
		FMOD_3D_ATTRIBUTES attrs;
		memset(&attrs, 0, sizeof(attrs));
		attrs.forward = Vector3DToFMOD_VECTOR(m_pCamera->GetViewAxis());
		attrs.position = Vector3DToFMOD_VECTOR(vCameraPosition);
		attrs.up = Vector3DToFMOD_VECTOR(m_pCamera->GetUpAxis());
		attrs.velocity = Vector3DToFMOD_VECTOR(Vector3D::Zero());
		FMOD_VERIFY(pFMODStudioSystem->setListenerAttributes(0, &attrs));
	}

	// Update FMOD - this will call any Event callbacks currently registered
	// that have a callback waiting.
	auto const eResult = pFMODStudioSystem->update();
	(void)eResult;

	// TODO: Unfortunately, it looks like FMOD doesn't try
	// to register the listener socket for the new FMOD network
	// interface until later, so it is no longer caught during startup
	// if the socket can't be bound (e.g. if two copies of the game
	// are running). So we need to ignore that particular
	// error code here.
	SEOUL_ASSERT(FMOD_OK == eResult || FMOD_ERR_NET_SOCKET_ERROR == eResult);
}

/**
 * Sets whether sounds in category categoryName are paused or not.
 */
Bool Manager::SetCategoryPaused(
	HString categoryName,
	Bool bPaused /* = true */)
{
	SEOUL_ASSERT(IsMainThread());

	if (!m_pFMODStudioSystem.IsValid())
	{
		return false;
	}

	// Get the bus - return true if it was found, false otherwise.
	::FMOD::Studio::Bus* pBus = nullptr;
	if (FMOD_OK == m_pFMODStudioSystem->getBus(
		categoryName.CStr(),
		&pBus))
	{
		// Set the paused status of the category.
		return (FMOD_OK == pBus->setPaused(bPaused));
	}
	else
	{
		return false;
	}
}

/**
 * Mutes or unmutes the mater channel based on bMuted.
 */
Bool Manager::SetMasterMute(Bool bMuted /*= true*/)
{
	SEOUL_ASSERT(IsMainThread());

	if (!m_pFMODStudioSystem.IsValid())
	{
		return false;
	}

	// Get the bus - return true if it was found, false otherwise.
	::FMOD::Studio::Bus* pBus = nullptr;
	if (FMOD_OK == m_pFMODStudioSystem->getBus(
		Sound::CategoryNames::khsSoundCategoryMaster.CStr(),
		&pBus))
	{
		// Set the paused status of the category.
		return (FMOD_OK == pBus->setMute(bMuted));
	}
	else
	{
		return false;
	}
}

/**
 * Sets whether all sounds are paused or not.
 */
Bool Manager::SetMasterPaused(Bool bPaused)
{
	return SetCategoryPaused(Sound::CategoryNames::khsSoundCategoryMaster, bPaused);
}

/**
 * Sets the volume of the all sound and music.
 * @param[in] fVolume Volume level on [0, 1].
 */
Bool Manager::SetMasterVolume(Float fVolume)
{
	return SetCategoryVolume(Sound::CategoryNames::khsSoundCategoryMaster, fVolume);
}

/**
 * Mute audio for a category.
 *
 * @param hsCategoryName the category name
 * @param bMute true to mute, false to unmute
 * @param bAllowPending true to allow FMODSound::Manager to wait for the category to become available, false
 *                      to only apply immediately and fail (return false) if the category is not available.
 * @param bSuppressLogging if true, error cases (return false) will not output to the SEOUL_LOG.
 */
Bool Manager::SetCategoryMute(HString hsCategoryName, Bool bMute /*= true*/, Bool bAllowPending /*= false*/, Bool bSuppressLogging /*= false*/)
{
	// Special handling since we directly control mute of this channel internally.
	if (hsCategoryName == Sound::CategoryNames::khsSoundCategoryMusic)
	{
		m_bMusicMuted = bMute;
		return true;
	}

	return InternalCategoryMute(hsCategoryName, bMute, bAllowPending, bSuppressLogging);
}

Bool Manager::InternalCategoryMute(HString hsCategoryName, Bool bMute, Bool bAllowPending, Bool bSuppressLogging)
{
	SEOUL_ASSERT(IsMainThread());

	if (!m_pFMODStudioSystem.IsValid())
	{
		if (!bSuppressLogging)
		{
			SEOUL_LOG("InternalSetCategoryMute(%s): Event system is not initialized\n", hsCategoryName.CStr());
		}

		return false;
	}

	// Get the bus - it is not always present in every project.
	::FMOD::Studio::Bus* pBus = nullptr;
	auto eResult = m_pFMODStudioSystem->getBus(
		hsCategoryName.CStr(),
		&pBus);

	if (bAllowPending && eResult == FMOD_ERR_EVENT_NOTFOUND)
	{
		DeferCategoryMute(hsCategoryName, bMute);

		return true;
	}
	else if (eResult != FMOD_OK || pBus == nullptr)
	{
		if (!bSuppressLogging)
		{
			SEOUL_LOG("InternalSetCategoryMute(%s): Failed to find category: %s\n", hsCategoryName.CStr(), FMOD_ErrorString(eResult));
		}

		return false;
	}

	// Set the the mute state of the category.
	return (FMOD_OK == pBus->setMute(bMute));
}

/**
 * Sets the volume of the specified category
 *
 * @param[in] eCategory Category to modify
 * @param[in] fVolume   Desired volume level [0, 1]
 * @param[in] fFadeTimeInSeconds Time in seconds to apply the volume change. If
 *              0.0f, change occurs instantaneously.
 * @param[in] bAllowPending If true and the category doesn't exist, pends the
 *              volume change until a later time when the category does exist
 *              after an event in that category is loaded
 * @param[in] bSuppressLogging If true, SEOUL_LOG() statements associated with various
 *              error cases will be suppressed. Typically you want to do this
 *              if a return value of false is expected/acceptable.
 *
 * @return True if successful, false otherwise
 */
Bool Manager::SetCategoryVolume(HString hsCategoryName, Float fVolume, Float fFadeTimeInSeconds /* = 0.0f */, Bool bAllowPending /* = false */, Bool bSuppressLogging /* = false*/)
{
	SEOUL_ASSERT(IsMainThread());

	if (!m_pFMODStudioSystem.IsValid())
	{
		if (!bSuppressLogging)
		{
			SEOUL_LOG("SetCategoryVolume(%s): Event system is not initialized\n", hsCategoryName.CStr());
		}

		return false;
	}

	// Get the bus - it is not always present in every project.
	::FMOD::Studio::Bus* pBus = nullptr;
	auto eResult = m_pFMODStudioSystem->getBus(
		hsCategoryName.CStr(),
		&pBus);

	if (fFadeTimeInSeconds > 0.0f ||
		(bAllowPending && eResult == FMOD_ERR_EVENT_NOTFOUND))
	{
		// If a volume set with a duration, create it specially.
		if (fFadeTimeInSeconds > 0.0f && nullptr != pBus)
		{
			// Get the current volume as the start volume - if this fails, don't update the volume.
			Float fStartVolume = 1.0f;
			FMOD_RESULT const eGetVolumeResult = pBus->getVolume(&fStartVolume);
			if (eResult != FMOD_ERR_EVENT_NOTFOUND && FMOD_OK != eGetVolumeResult)
			{
				if (!bSuppressLogging)
				{
					SEOUL_LOG("SetCategoryVolume(%s): Failed getting start volume for fade over time volume set: %s\n", hsCategoryName.CStr(), FMOD_ErrorString(eGetVolumeResult));
				}

				return false;
			}

			// Set a deferred volume change with the specified start and end volumes, and
			// the desired duration.
			DeferCategoryVolume(hsCategoryName, fStartVolume, fVolume, fFadeTimeInSeconds);
		}
		else
		{
			// Set a deferred instantaneous volume change.
			DeferCategoryVolume(hsCategoryName, fVolume);
		}

		return true;
	}
	else if (eResult != FMOD_OK || pBus == nullptr)
	{
		if (!bSuppressLogging)
		{
			SEOUL_LOG("SetCategoryVolume(%s): Failed to find category: %s\n", hsCategoryName.CStr(), FMOD_ErrorString(eResult));
		}

		return false;
	}

	// Set the the volume of the category.
	FMOD_VERIFY(pBus->setVolume(fVolume));
	return true;
}

/**
 * Retrieves the volume of the specified category
 *
 * @param[in] eCategory		category to query
 * @return volume [0,1]
 */
Float Manager::GetCategoryVolume(HString hsCategoryName) const
{
	SEOUL_ASSERT(IsMainThread());

	// No volume if no audio system.
	if (!m_pFMODStudioSystem.IsValid())
	{
		return 0.0f;
	}

	// Get the bus - it is not always present in every project.
	::FMOD::Studio::Bus* pBus = nullptr;
	auto eResult = m_pFMODStudioSystem->getBus(
		hsCategoryName.CStr(),
		&pBus);

	// The the volume of the category.
	Float fCurrentVolume = 0.f;
	if (FMOD_OK == eResult && nullptr != pBus)
	{
		FMOD_VERIFY(pBus->getVolume(&fCurrentVolume));
	}
	return fCurrentVolume;
}

Bool Manager::GetMasterAttributes(UInt32& ruSamplingRate, UInt32& ruChannels) const
{
	SEOUL_ASSERT(IsMainThread());

	if (!m_pFMODStudioSystem.IsValid())
	{
		return false;
	}

	::FMOD::System* pFMODSystem = nullptr;
	FMOD_VERIFY(m_pFMODStudioSystem->getCoreSystem(&pFMODSystem));
	SEOUL_ASSERT(nullptr != pFMODSystem);

	Int iSamplingRate = 0;
	FMOD_SPEAKERMODE eSpeakerMode = FMOD_SPEAKERMODE_DEFAULT;
	Int iRawSpeakers = 0;
	if (FMOD_OK == pFMODSystem->getSoftwareFormat(
		&iSamplingRate,
		&eSpeakerMode,
		&iRawSpeakers))
	{
		ruSamplingRate = (UInt32)iSamplingRate;
		ruChannels = (UInt32)iRawSpeakers;
		return true;
	}

	return false;
}

/**
 * Reconfigure the global 3d attributes of the sound system.
 * Calling this method with no arguments resets the sound system
 * to its defaults.
 */
void Manager::SetMaster3DAttributes(
	Float fDopplerShiftScale /* = 1.0f */,
	Float fUnitsPerMeter /* = 100.0f */,
	Float f3DRolloffScale /* = 1.0f */)
{
	SEOUL_ASSERT(IsMainThread());

	CheckedPtr<::FMOD::Studio::System> pFMODStudioSystem = m_pFMODStudioSystem;
	if (!pFMODStudioSystem.IsValid())
	{
		return;
	}

	::FMOD::System* pFMODSystem = nullptr;
	FMOD_VERIFY(pFMODStudioSystem->getCoreSystem(&pFMODSystem));
	SEOUL_ASSERT(nullptr != pFMODSystem);

	// Set up global 3D settings in FMOD.
	FMOD_VERIFY(pFMODSystem->set3DSettings(
		fDopplerShiftScale,
		fUnitsPerMeter,
		f3DRolloffScale));
}

/** Update the Camera instance used for computing 3D spatial positioning each frame. */
void Manager::SetListenerCamera(const SharedPtr<Camera>& pCamera)
{
	m_pCamera = pCamera;
}

/**
 * Gets the sound event contained defined by soundEventKey.
 *
 * @return True if the Event was found, false otherwise. Note that if this
 * method fails, rEvent will be reset to its default state and rEvent.IsValid()
 * will return false.
 */
void Manager::AssociateSoundEvent(const ContentKey& key, Sound::Event& rInEvent)
{
	Event& rEvent = static_cast<Event&>(rInEvent);
	rEvent.InternalSetAnchor(m_SoundEvents.GetContent(key));
}

/** @return A derived subclass of Event appropriate for use with this Manager. */
Sound::Event* Manager::NewSoundEvent() const
{
	return SEOUL_NEW(MemoryBudgets::Audio) Event;
}

/** @return The current DSP clock tick time. */
UInt64 Manager::GetClockTimeDSP() const
{
	SEOUL_ASSERT(IsMainThread());

	CheckedPtr<::FMOD::Studio::System> pFMODStudioSystem = m_pFMODStudioSystem;
	if (!pFMODStudioSystem.IsValid())
	{
		return 0u;
	}

	::FMOD::System* pFMODSystem = nullptr;
	FMOD_VERIFY(pFMODStudioSystem->getCoreSystem(&pFMODSystem));
	SEOUL_ASSERT(nullptr != pFMODSystem);

	::FMOD::ChannelGroup* pFMODMasterGroup = nullptr;
	FMOD_VERIFY(pFMODSystem->getMasterChannelGroup(&pFMODMasterGroup));
	SEOUL_ASSERT(pFMODMasterGroup);

	unsigned long long uUnusedClock = 0u;
	unsigned long long uParentClock = 0u;
	FMOD_VERIFY(pFMODMasterGroup->getDSPClock(&uUnusedClock, &uParentClock));

	return (UInt64)uParentClock;
}

/**
 * Used to unload all music system data and all loaded sound projects.
 */
void Manager::InternalShutdown()
{
	SEOUL_ASSERT(IsMainThread());

	// Stop all sounds.
	if (m_pFMODStudioSystem.IsValid())
	{
		CheckedPtr<::FMOD::Studio::System> pStudioSystem = m_pFMODStudioSystem;

		Int iBankCount = 0;
		FMOD_VERIFY(pStudioSystem->getBankCount(&iBankCount));
		if (iBankCount > 0)
		{
			Vector<::FMOD::Studio::Bank*, MemoryBudgets::Audio> vBanks((UInt32)iBankCount);
			FMOD_VERIFY(pStudioSystem->getBankList(vBanks.Data(), (Int)vBanks.GetSize(), &iBankCount));
			for (Int i = 0; i < iBankCount; ++i)
			{
				auto pBank(vBanks[i]);

				Int32 iBusCount = 0;
				(void)pBank->getBusCount(&iBusCount); // Can fail if the bank metadata failed to load, which can happen if loading is cancelled.
				if (iBusCount > 0)
				{
					Vector<::FMOD::Studio::Bus*, MemoryBudgets::Audio> vBus((UInt32)iBusCount);
					FMOD_VERIFY(pBank->getBusList(vBus.Data(), (Int)vBus.GetSize(), &iBusCount));
					for (Int j = 0; j < iBusCount; ++j)
					{
						auto pBus(vBus[j]);
						FMOD_VERIFY(pBus->stopAllEvents(FMOD_STUDIO_STOP_IMMEDIATE));
					}
				}
			}
		}
	}
	SEOUL_TEARDOWN_TRACE();

	// Tell the termination functions to force the stop.
	m_bShuttingDown = true;

	// Clear events, then projects - if either of these fail, it
	// means something is keeping a reference, which will result in
	// trouble beyond this point.
	SEOUL_VERIFY(m_SoundEvents.Clear()); SEOUL_TEARDOWN_TRACE();
	SEOUL_VERIFY(m_SoundProjects.Clear()); SEOUL_TEARDOWN_TRACE();

	// Unload all sound projects.
	if (m_pFMODStudioSystem.IsValid())
	{
		FMOD_VERIFY(m_pFMODStudioSystem->unloadAll()); SEOUL_TEARDOWN_TRACE();
	}

	// Clear capture DSP, then destroy it.
	{
		Lock lock(m_SoundCapture);
		m_vSoundCapture.Clear();
	}
	InternalDestroySoundCaptureDSP(); SEOUL_TEARDOWN_TRACE();

	// Done, no longer in shutting down state.
	m_bShuttingDown = false;
}

/**
 * Expected to be called once at startup. Register
 * a DSP (but do not attach) to be used to
 * capture audio from the master bus.
 */
void Manager::InternalCreateSoundCaptureDSP()
{
	// Make sure any existing DSP is destroyed.
	InternalDestroySoundCaptureDSP();

	CheckedPtr<::FMOD::Studio::System> pFMODStudioSystem = m_pFMODStudioSystem;
	if (!pFMODStudioSystem.IsValid())
	{
		return;
	}

	::FMOD::System* pFMODSystem = nullptr;
	FMOD_VERIFY(pFMODStudioSystem->getCoreSystem(&pFMODSystem));
	SEOUL_ASSERT(pFMODSystem);

	// Instantiate the DSP.
	FMOD_DSP_DESCRIPTION desc;
	memset(&desc, 0, sizeof(desc));

	STRNCPY(desc.name, "SeoulEngineSoundCapture", sizeof(desc.name));
	desc.numinputbuffers = 1u;
	desc.numoutputbuffers = 0u;
	desc.numparameters = 0u;
	desc.read = AudioCaptureDSPCallback;
	desc.userdata = this;
	desc.version = 1u;

	::FMOD::DSP* pDSP = nullptr;
	FMOD_VERIFY(pFMODSystem->createDSP(&desc, &pDSP));
	SEOUL_ASSERT(nullptr != pDSP);

	m_pFMODAudioCaptureDSP = pDSP;

	// Add the DSP to the master group.
	::FMOD::ChannelGroup* pFMODMasterGroup = nullptr;
	FMOD_VERIFY(pFMODSystem->getMasterChannelGroup(&pFMODMasterGroup));
	SEOUL_ASSERT(pFMODMasterGroup);
	FMOD_VERIFY(pFMODMasterGroup->addDSP(0, m_pFMODAudioCaptureDSP));
}

/**
 * Destroy the master bus capture DSP.
 */
void Manager::InternalDestroySoundCaptureDSP()
{
	if (m_pFMODAudioCaptureDSP)
	{
		// Remove the DSP from the master group prior to destruction.
		{
			CheckedPtr<::FMOD::Studio::System> pFMODStudioSystem = m_pFMODStudioSystem;
			if (pFMODStudioSystem.IsValid())
			{
				::FMOD::System* pFMODSystem = nullptr;
				FMOD_VERIFY(pFMODStudioSystem->getCoreSystem(&pFMODSystem));
				SEOUL_ASSERT(pFMODSystem);

				::FMOD::ChannelGroup* pFMODMasterGroup = nullptr;
				FMOD_VERIFY(pFMODSystem->getMasterChannelGroup(&pFMODMasterGroup));
				SEOUL_ASSERT(pFMODMasterGroup);
				FMOD_VERIFY(pFMODMasterGroup->removeDSP(m_pFMODAudioCaptureDSP));
			}
		}

		auto pDSP = m_pFMODAudioCaptureDSP;
		m_pFMODAudioCaptureDSP.Reset();
		FMOD_VERIFY(pDSP->release());
	}
}

void Manager::InternalSuspendMixer()
{
	CheckedPtr<::FMOD::Studio::System> pFMODStudioSystem = m_pFMODStudioSystem;
	if (pFMODStudioSystem.IsValid())
	{
		::FMOD::System* pFMODSystem = nullptr;
		FMOD_VERIFY(pFMODStudioSystem->getCoreSystem(&pFMODSystem));
		if (nullptr != pFMODSystem)
		{
			FMOD_VERIFY(pFMODSystem->mixerSuspend());
		}
	}
}

void Manager::InternalResumeMixer()
{
	// Resume the mixer thread.
	CheckedPtr<::FMOD::Studio::System> pFMODStudioSystem = m_pFMODStudioSystem;
	if (pFMODStudioSystem.IsValid())
	{
		::FMOD::System* pFMODSystem = nullptr;
		FMOD_VERIFY(pFMODStudioSystem->getCoreSystem(&pFMODSystem));
		if (nullptr != pFMODSystem)
		{
			FMOD_VERIFY(pFMODSystem->mixerResume());
		}
	}
}

Bool Manager::PrepareSoundProjectAnchorDelete(FilePath filePath, Content::Entry<ProjectAnchor, FilePath>& entry)
{
	SEOUL_ASSERT(IsMainThread());

	SharedPtr<ProjectAnchor> pAnchor = entry.GetPtr();
	if (!pAnchor.IsValid())
	{
		return true;
	}

	// Can't free the sound until pending references are free - there will be 2
	// (the local managed ptr, and the one contained in the Content::Entry)
	auto const count = pAnchor.GetReferenceCount();
	if (count != 2)
	{
		return false;
	}

	// Can immediately free the anchor if loading failed (it's in the error state).
	if (pAnchor->GetState() == ProjectAnchor::kError)
	{
		return true;
	}

	CheckedPtr<::FMOD::Studio::System> pFMODStudioSystem = m_pFMODStudioSystem;
	if (!pFMODStudioSystem.IsValid())
	{
		// Return true immediately here - data is already free if no system.
		return true;
	}

	// TODO: This is only "ok" because we currently use
	// (by convention) only 1 project per game in FMOD Studio
	// (a "project" is our concept - FMOD Studio only has banks).
	//
	// While I don't see any reason that this will need to change,
	// it is not enforced, which may mean it will be violated at
	// some point.
	//
	// Now release all the bank files.
	FMOD_VERIFY(pFMODStudioSystem->unloadAll());
	return true;
}

Bool Manager::PrepareSoundEventAnchorDelete(const ContentKey& key, Content::Entry<EventAnchor, ContentKey>& entry)
{
	SEOUL_ASSERT(IsMainThread());

	SharedPtr<EventAnchor> pAnchor = entry.GetPtr();
	if (!pAnchor.IsValid())
	{
		return true;
	}

	// Can't free the sound until pending references are free - there will be 2
	// (the local managed ptr, and the one contained in the Content::Entry)
	auto const count = pAnchor.GetReferenceCount();
	if (count != 2)
	{
		return false;
	}

	CheckedPtr<::FMOD::Studio::System> pFMODStudioSystem = m_pFMODStudioSystem;
	if (!pFMODStudioSystem.IsValid())
	{
		// Return true immediately here - data is already free if no system.
		return true;
	}

	// Get the description - if null, assume the description will
	// be forever invalid and just return true immediately.
	auto pFMODEventDescription = pAnchor->ResolveDescription();
	if (nullptr == pFMODEventDescription)
	{
		return true;
	}

	// Skip stop checking if we're shutting down.
	if (!m_bShuttingDown)
	{
		// Check instance state - let them finish if still playing.
		Int32 iCount = 0;
		auto e = pFMODEventDescription->getInstanceCount(&iCount);
		if (FMOD_OK != e)
		{
			return false;
		}

		StackOrHeapArray<::FMOD::Studio::EventInstance*, 16u, MemoryBudgets::Audio> a((UInt32)iCount);
		e = pFMODEventDescription->getInstanceList(a.Data(), iCount, &iCount);
		if (FMOD_OK != e)
		{
			return false;
		}

		Bool bWaitingForStopping = false;
		for (Int32 i = 0; i < iCount; ++i)
		{
			FMOD_STUDIO_PLAYBACK_STATE eState = FMOD_STUDIO_PLAYBACK_STOPPING;
			e = a[i]->getPlaybackState(&eState);
			if (FMOD_OK != e)
			{
				return false;
			}

			switch (eState)
			{
				// Waiting for stop.
			case FMOD_STUDIO_PLAYBACK_STOPPING:
				bWaitingForStopping = true;
				break;

				// Stop, can continue on.
			case FMOD_STUDIO_PLAYBACK_STOPPED:
				break;

				// Any other state, explicitly trigger a stop of it.
			default:
				a[i]->stop(FMOD_STUDIO_STOP_ALLOWFADEOUT);
				bWaitingForStopping = true;
				break;
			};
		}

		// Don't force stop if we're waiting.
		if (bWaitingForStopping)
		{
			return false;
		}
	}

	// Release all instances.
	auto eResult = pFMODEventDescription->releaseAllInstances();
	if (FMOD_OK != eResult)
	{
		// Invalid handle indicates entire event was already released,
		// so just return success.
		if (FMOD_ERR_INVALID_HANDLE == eResult)
		{
			return true;
		}

		return false;
	}

	// Unload sample data before termination.
	eResult = pFMODEventDescription->unloadSampleData();
	if (FMOD_OK != eResult && FMOD_ERR_STUDIO_NOT_LOADED != eResult)
	{
		return false;
	}

	return true;
}

/**
 * Called when a sound project file has changed on disk - if successful,
 * the entry for the sound project will be in a state to be reloaded. If this
 * function returns false, then the project cannot be reloaded, but should
 * be left in a valid state.
 */
Bool Manager::SoundProjectChange(FilePath filePath, const Content::Handle<ProjectAnchor>& hProjectAnchor)
{
	SEOUL_ASSERT(IsMainThread());

	return true;
}

/**
 * Return the total memory being used by the FMOD System and event system.
 * This will always be less than or equal to the fixed memory pool size given
 * to FMOD at initialization.
 */
UInt32 Manager::GetTotalMemoryUsageInBytes() const
{
	SEOUL_ASSERT(IsMainThread());

	CheckedPtr<::FMOD::Studio::System> pFMODStudioSystem = m_pFMODStudioSystem;
	if (!pFMODStudioSystem.IsValid())
	{
		// No memory usage if no audio.
		return 0u;
	}

	::FMOD::System* pFMODSystem = nullptr;
	FMOD_VERIFY(pFMODStudioSystem->getCoreSystem(&pFMODSystem));
	SEOUL_ASSERT(pFMODSystem);

	Int iTotalMemoryUsageInBytes = 0;
	FMOD_VERIFY(::FMOD::Memory_GetStats(&iTotalMemoryUsageInBytes, nullptr, false));

	return (UInt32)Max(iTotalMemoryUsageInBytes, 0);
}

/** Utility used by IsCategoryPlaying(). */
static inline Bool IsCategoryPlaying(::FMOD::ChannelGroup* pFMODGroup, Bool bIncludeLoopingSounds)
{
	Int iChannels = 0;
	FMOD_VERIFY(pFMODGroup->getNumChannels(&iChannels));
	for (Int i = 0; i < iChannels; ++i)
	{
		::FMOD::Channel* pChannel = nullptr;

		// TODO: Should be FMOD_VERIFY, need to dig into
		// why this can fail.
		if (FMOD_OK != pFMODGroup->getChannel(i, &pChannel))
		{
			continue;
		}

		Bool bPlaying = false;

		// TODO: Should be FMOD_VERIFY, need to dig into
		// why this can fail.
		if (FMOD_OK != pChannel->isPlaying(&bPlaying))
		{
			continue;
		}

		if (!bPlaying)
		{
			continue;
		}

		::FMOD::Sound* pSound = nullptr;
		FMOD_VERIFY(pChannel->getCurrentSound(&pSound));

		Int iLoopCount = 0;
		FMOD_VERIFY(pSound->getLoopCount(&iLoopCount));

		// A bit unintuitive compared to (e.g.) the Fx
		// API, but a loop count of 0 is the only valid
		// value for a one-shot. -1 is an indefinite looping
		// sound, and 1 is a "loop once then stop" sound.
		if (bIncludeLoopingSounds || 0 == iLoopCount)
		{
			return true;
		}
	}

	// Check children.
	Int iGroups = 0;
	FMOD_VERIFY(pFMODGroup->getNumGroups(&iGroups));
	for (Int i = 0; i < iGroups; ++i)
	{
		::FMOD::ChannelGroup* pSubGroup = nullptr;
		FMOD_VERIFY(pFMODGroup->getGroup(i, &pSubGroup));

		if (IsCategoryPlaying(pSubGroup, bIncludeLoopingSounds))
		{
			return true;
		}
	}

	return false;
}

/**
 * Utility, intended to check if any sounds are still playing
 * to track when activity has "settled". Returns true
 * only for one off events (not looping). Must be called
 * on the main thread.
 */
Bool Manager::IsCategoryPlaying(HString categoryName, Bool bIncludeLoopingSounds /*= false*/) const
{
	if (!m_pFMODStudioSystem.IsValid())
	{
		return false;
	}

	// Get the bus - return true if it was found, false otherwise.
	::FMOD::Studio::Bus* pBus = nullptr;
	if (FMOD_OK != m_pFMODStudioSystem->getBus(
		categoryName.CStr(),
		&pBus))
	{
		return false;
	}

	::FMOD::ChannelGroup* pFMODGroup = nullptr;
	if (FMOD_OK != pBus->getChannelGroup(&pFMODGroup))
	{
		return false;
	}
	SEOUL_ASSERT(pFMODGroup);

	return FMODSound::IsCategoryPlaying(pFMODGroup, bIncludeLoopingSounds);
}

void Manager::OnEnterBackground()
{
	// In background already, early out.
	if (m_bInBackground)
	{
		return;
	}

	// Now in the background.
	m_bInBackground = true;

	// Log for testing and debug tracking.
	SEOUL_LOG("FMODSoundManager::OnEnterBackground()");

	// Pause all audio and mute the master volume.
	SetMasterPaused(true);
	SetMasterVolume(0.0f);

	// Make sure we commit the settings to FMOD
	Tick(0.0f);

	// Now pause the mixer thread, unless we're already interrupted.
	if (!m_bInterrupted)
	{
		InternalSuspendMixer();
	}
}

void Manager::OnLeaveBackground()
{
	// Not in the background, early out.
	if (!m_bInBackground)
	{
		return;
	}

	// No longer in the background.
	m_bInBackground = false;

	// Log for testing and debug tracking.
	SEOUL_LOG("FMODSoundManager::OnLeaveBackground()");

	// Resume the mixer thread if not interrupted.
	if (!m_bInterrupted)
	{
		InternalResumeMixer();
	}

	// Control music channel mute state based on whether
	// an external audio source is playing. Implementation of
	// the query is platform dependent.
	InternalCategoryMute(Sound::CategoryNames::khsSoundCategoryMusic, m_bMusicMuted || SoundManagerIsExternalAudioPlaying(), false, true);

	// Unpause and restore the volume of the master channel.
	SetMasterVolume(1.0f);
	SetMasterPaused(false);

	// Make sure we commit the settings to FMOD
	Tick(0.0f);
}

void Manager::OnInterruptStart()
{
	// Early out if already interrupted.
	if (m_bInterrupted)
	{
		return;
	}

	// Now interrupted.
	m_bInterrupted = true;

	// Log for testing and debug tracking.
	SEOUL_LOG("FMODSoundManager::OnInterruptStart()");

	// Suspend the mixer if not already in the background.
	if (!m_bInBackground)
	{
		InternalSuspendMixer();
	}
}

void Manager::OnInterruptEnd()
{
	// Not interrupted, nothing to do - also,
	// if in background, don't restore.
	if (!m_bInterrupted)
	{
		return;
	}

	// No longer interrupted.
	m_bInterrupted = false;

	// Log for testing and debug tracking.
	SEOUL_LOG("FMODSoundManager::OnInterruptEnd()");

	// If we're not also in the background,
	// resume the mixer.
	if (!m_bInBackground)
	{
		InternalResumeMixer();
	}
}

static void InvokeCallback(
	const SharedPtr<Sound::ICapture>& pCallback,
	const SharedPtr<Sound::SampleData>& pSampleData)
{
	pCallback->OnSamples(pSampleData);
}

// Entry hook for the master bus sound capture DSP callback.
void Manager::OnSoundCapture(
	Float const* pfIn,
	UInt32 uSamples,
	UInt32 uChannels,
	UInt64 uClock,
	UInt32 uClockOffset,
	UInt32 uClockLength)
{
	Lock lock(m_SoundCapture);

	// Enumerate any capture instances, advance,
	// and pass in data.
	for (auto& e : m_vSoundCapture)
	{
		// Cache and advance some values.
		auto const uFrame = e.m_uFrame;
		auto const uOffset = e.m_uOffsetInSamples;

		// Compute initial values to adjust by the clock parameters.
		auto uClockShift = Min(uClockOffset, uSamples);
		auto pfAdjustedIn = pfIn + uClockShift * uChannels;
		auto uAdjustedSamples = Min(uClockLength, (uSamples - uClockShift));

		// No samples, drop immediately.
		if (0u == uAdjustedSamples)
		{
			continue;
		}

		// Adjust the input for this sample based on the clock
		// parameters and the existing offset.
		auto const uExpectedTime = (e.m_uStartClockTime + e.m_uOffsetInSamples);

		// If uClock > uLastTime, we need padding.
		UInt32 uPadding = 0u;
		if (uClock > uExpectedTime)
		{
			uPadding = (UInt32)(uClock - uExpectedTime);
		}
		// If uClock < uLastTime, we need to further adjust
		// pfAdjustedIn and uAdjustedSamples
		else if (uClock < uExpectedTime)
		{
			uClockShift = Min(uAdjustedSamples, (UInt32)(uExpectedTime - uClock));
			pfAdjustedIn += (uClockShift * uChannels);
			uAdjustedSamples -= uClockShift;

			// No samples, drop immediately.
			if (0u == uAdjustedSamples)
			{
				continue;
			}
		}

		e.m_uFrame++;
		e.m_uOffsetInSamples += uAdjustedSamples;

		// Dispatch if registered
		if (e.m_p.IsValid())
		{
			SharedPtr<Sound::SampleData> pSample(SEOUL_NEW(MemoryBudgets::Audio) Sound::SampleData(
				uFrame,
				uOffset,
				uAdjustedSamples,
				uChannels,
				pfAdjustedIn,
				uPadding));
			Jobs::AsyncFunction(
				e.m_ThreadId,
				&InvokeCallback,
				e.m_p,
				pSample);
		}
	}
}

/**
 * Utility function called when an FMOD API call fails with FMOD_ERR_MEMORY
 */
void OutOfMemory()
{
	static const UInt32 kFMODMemorySizeInBytes = 0u;
	(void)kFMODMemorySizeInBytes;

	int nCurrentAllocated = -1, nMaxAllocated = -1;
	if (::FMOD::Memory_GetStats(&nCurrentAllocated, &nMaxAllocated, true) == FMOD_OK)
	{
		SEOUL_WARN("FMOD ran out of memory!  CurrentAllocated=%d MaxAllocated=%d kFMODMemorySizeInBytes=%u", nCurrentAllocated, nMaxAllocated, kFMODMemorySizeInBytes);
	}
	else
	{
		SEOUL_WARN("FMOD ran out of memory!  Unable to get current memory usage.  kFMODMemorySizeInBytes=%u", kFMODMemorySizeInBytes);
	}
}

} // namespace FMODSound

/** Linked hook for app roots. */
Sound::Manager* CreateFMODHeadlessSoundManager()
{
	return SEOUL_NEW(MemoryBudgets::Audio) FMODSound::Manager(FMODSound::Manager::kHeadless);
}

} // namespace Seoul

#endif // /#if SEOUL_WITH_FMOD
