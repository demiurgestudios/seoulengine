/**
 * \file FMODSoundContentLoader.cpp
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

#include "Compress.h"
#include "CookManager.h"
#include "FileManager.h"
#include "FMODSoundContentLoader.h"
#include "FMODSoundEvent.h"
#include "FMODSoundManager.h"
#include "FMODSoundUtil.h"
#include "GamePaths.h"
#include "Logger.h"
#include "Path.h"
#include "ReflectionDefine.h"
#include "ReflectionUtil.h"
#include "ScopedAction.h"
#include "SoundUtil.h"

#if SEOUL_WITH_FMOD
#include "fmod_studio.h"
#include "fmod_studio.hpp"

namespace Seoul
{

namespace FMODSound
{

/** Prefix required on events as of FMOD Studio. */
static const String ksEventDomain("event:/");

/** Utility to resolve the FMOD event name from a SeoulEngine key. */
static inline String ToEventKey(const ContentKey& key)
{
	auto const sReturn(ksEventDomain + String(key.GetData()));
	return sReturn;
}

/**
 * NOTE: Atomic32 just to enforce volatile, not meant to be accessed
 * outside of the main thread (logic depends on no race from different
 * threads around this gating).
 */
static Atomic32 s_EventLoadersActive;

/**
 * Utility that encapsulates shared behavior
 * of loading FMOD sound banks that have yet
 * to be loaded. Also includes fetching those
 * banks with a network request.
 */
class BankFileLoader SEOUL_SEALED
{
public:
	typedef Vector<FilePath, MemoryBudgets::Audio> BankFiles;
	typedef HashSet<FilePath, MemoryBudgets::Audio> BankSet;
	typedef Vector<::FMOD::Studio::Bank*, MemoryBudgets::Audio> Loading;

	/** Convenience, set to list. */
	static BankFiles Convert(const BankSet& set)
	{
		BankFiles vFiles;
		for (auto const& e : set)
		{
			vFiles.PushBack(e);
		}
		return vFiles;
	}

	BankFileLoader(const BankFiles& vFiles, Bool bProject)
		: m_AsyncMarker(0)
		, m_vFiles(vFiles)
		, m_vLoading(vFiles.GetSize(), nullptr)
		, m_bProject(bProject)
		, m_bStartedLoading(false)
	{
	}

	BankFileLoader(const BankSet& set, Bool bProject)
		: m_AsyncMarker(0)
		, m_vFiles(Convert(set))
		, m_vLoading(set.GetSize(), nullptr)
		, m_bProject(bProject)
		, m_bStartedLoading(false)
	{
	}

	~BankFileLoader()
	{
		if (m_bStartedLoading && !m_bProject)
		{
			--s_EventLoadersActive;
		}
	}

	/**
	 * Must call this function on a thread other than main to ensure
	 * banks have been network fetched.
	 */
	Bool NetworkFetch()
	{
		SEOUL_ASSERT(!IsMainThread());

		// Prefetch so banks near each other can batch.
		NetworkPrefetch();

		// Now require the fetch.
		auto p(FileManager::Get());
		for (auto const& e : m_vFiles)
		{
			if (p->IsServicedByNetwork(e))
			{
				if (!p->NetworkFetch(e))
				{
					SEOUL_WARN("%s: network fetch failed.", e.CStr());
					return false;
				}
			}
		}

		// Done.
		return true;
	}

	/**
	 * Can be called from the main thread or a worker thread. Queues
	 * banks for fetching. Must call NeedsNetworkFetch() to check status.
	 */
	void NetworkPrefetch()
	{
		auto p(FileManager::Get());
		for (auto const& e : m_vFiles)
		{
			(void)p->NetworkPrefetch(e);
		}
	}

	/**
	 * @return true if any of the bank dependencies
	 * of this loader still need to be fetched
	 * from the network. false otherwise.
	 */
	Bool NeedsNetworkFetch() const
	{
		auto p(FileManager::Get());
		for (auto const& e : m_vFiles)
		{
			if (p->IsServicedByNetwork(e))
			{
				return true;
			}
		}

		return false;
	}

	enum class LoadState
	{
		Loading,
		Loaded,
		Error,
	};

	/** Perform per-frame loading maintenance. *Must* be called from the main thread. */
	LoadState Load()
	{
		SEOUL_ASSERT(IsMainThread());

		// Finally load the banks.
		CheckedPtr<Manager> pSoundManager = Manager::Get();
		CheckedPtr<::FMOD::Studio::System> pFMODStudioSystem = pSoundManager->m_pFMODStudioSystem;

		// Early out if no system.
		if (!pFMODStudioSystem.IsValid())
		{
			return LoadState::Error;
		}

		// Tracking - important - this increment
		// *must* only occur from the main thread,
		// since we do not utilize any other form
		// of synchronization (effectively, this
		// counter takes the place of a read-write
		// mutex to exclude event loads from project
		// loading).
		if (!m_bStartedLoading)
		{
			// If we're not a project load, increment tracking.
			if (!m_bProject)
			{
				++s_EventLoadersActive;

				// Now started.
				m_bStartedLoading = true;
			}
			// If we're a project, we can't start load operations
			// until any in flight event load operations have
			// completed.
			else if (0 != s_EventLoadersActive)
			{
				UpdateAsyncMarker(); // Tracking.

				// Waiting for completion.
				return LoadState::Loading;
			}
			// TODO: This is only ok because we use one
			// project per application, which is not otherwise
			// explicitly enforced.
			else
			{
				// Unload to flush state.
				FMOD_VERIFY(pFMODStudioSystem->unloadAll());

				// Success or failure, dirty pitch catch so it will be re-applied.
				pSoundManager->DirtyMasterPitchCache();

				UpdateAsyncMarker(); // Tracking.

				// Now started.
				m_bStartedLoading = true;

				// Waiting for completion.
				return LoadState::Loading;
			}
		}

		// TODO: Not entirely sure why/when this is necessary, but somewhat supported by
		// the documentation on Studio::System::update()
		//
		// "When Studio has been initialized in asynchronous mode, calling update flips a command
		// buffer for commands to be executed on the async thread and then immediately returns.
		// It is very fast since it is not executing any commands itself. When Studio has been
		// initialized with FMOD_STUDIO_INIT_SYNCHRONOUS_UPDATE, update will execute the queued commands before returning.
		//
		// If you do not call Studio::System::update then previous commands will not be executed.
		// While most of the API hides this behaviour with use of shadowed variables, it can cause
		// unexpected results if waiting in a loop for Studio::EventDescription::getSampleLoadingState
		// or Studio::Bank::getLoadingState without calling update first."
		//
		// In short, it appears that in some cases, getSampledLoadingState() can prematurely return
		// "loaded" if we do not give FMOD's async processing thread time to process commands. Using
		// async markers here is roughly (or exactly?) equivalent to calling flushCommands() without
		// blocking our main thread to do so.
		//
		// Don't continue until a full async pass has completed on FMOD's async thread.
		auto const preMarker = pSoundManager->GetAsyncPreMarker();
		auto const postMarker = pSoundManager->GetAsyncPostMarker();

		// Must wait until a new full async pass has both started and completed
		// (current pre-marker is > than our recorded pre-marker and current
		// post-marker is >= current pre-marker).
		if (m_AsyncMarker >= preMarker || postMarker < preMarker)
		{
			return LoadState::Loading;
		}

		// Now perform actual bank loading management.
		return InternalLoad();
	}

	/** Track where in the FMOD async thread's tick timeline we are, for ensuring valid state queries. */
	void UpdateAsyncMarker()
	{
		m_AsyncMarker = Manager::Get()->GetAsyncPreMarker();
	}

private:
	SEOUL_DISABLE_COPY(BankFileLoader);

	LoadState InternalLoad()
	{
		SEOUL_ASSERT(IsMainThread());

		// Cache FMODSound::Manager and perform further processing for the load.
		CheckedPtr<Manager> pSoundManager = Manager::Get();

		// Get the FMOD event system from the sound manager.
		CheckedPtr<::FMOD::Studio::System> pFMODStudioSystem = pSoundManager->m_pFMODStudioSystem;

		// Check and wait for banks to complete loading.
		Bool bError = false;
		UInt32 const uCount = m_vFiles.GetSize();
		m_vLoading.Resize(uCount);

		// Now enumerate, start loading banks that need to start loading.
		for (UInt32 i = 0u; i < uCount; ++i)
		{
			auto const& bankFilePath = m_vFiles[i];
			auto const sFilename(bankFilePath.GetRelativeFilename());
			if (nullptr == m_vLoading[i] || !m_vLoading[i]->isValid())
			{
				auto const eResult = pFMODStudioSystem->loadBankFile(
					sFilename.CStr(),
					FMOD_STUDIO_LOAD_BANK_NONBLOCKING,
					&m_vLoading[i]);

				// No bank, this is an error.
				if (FMOD_OK != eResult || nullptr == m_vLoading[i])
				{
					bError = true;
				}
			}
		}

		// Finally, check loading state.
		if (!bError)
		{
			UInt32 uLoaded = 0u;
			for (UInt32 i = 0u; i < uCount; ++i)
			{
				// Check bank loading state.
				FMOD_STUDIO_LOADING_STATE eState = FMOD_STUDIO_LOADING_STATE_ERROR;
				auto const eResult = m_vLoading[i]->getLoadingState(&eState);

				// Succss.
				if (FMOD_OK == eResult)
				{
					// Error is an error, loaded is loaded, all other states are considered
					// pending.
					if (FMOD_STUDIO_LOADING_STATE_LOADED == eState) { ++uLoaded; }
					else if (FMOD_STUDIO_LOADING_STATE_ERROR == eState)
					{
						bError = true;
						break;
					}
				}
				// Possible success if "already loaded", which happens frequently
				// with our usage of single-asset-per-bank bank generation.
				else
				{
					if (FMOD_ERR_EVENT_ALREADY_LOADED == eResult)
					{
						++uLoaded;
					}
					// Everythig else is an error.
					else
					{
						bError = true;
						break;
					}
				}
			}

			// Not done yet, try again.
			if (!bError)
			{
				if (uCount != uLoaded)
				{
					UpdateAsyncMarker();
					return LoadState::Loading;
				}
			}
		}

		// On error, return error, otherwise complete and release.
		if (bError)
		{
			return LoadState::Error;
		}
		else
		{
			return LoadState::Loaded;
		}
	}

	Atomic32Type m_AsyncMarker;
	BankFiles const m_vFiles;
	Loading m_vLoading;
	Bool const m_bProject;
	Bool m_bStartedLoading;
}; // class FMODSound::BankFileLoader

} // namespace FMOD

SEOUL_BEGIN_ENUM(FMODSound::BankFileLoader::LoadState)
	SEOUL_ENUM_N("Loading", FMODSound::BankFileLoader::LoadState::Loading)
	SEOUL_ENUM_N("Loaded", FMODSound::BankFileLoader::LoadState::Loaded)
	SEOUL_ENUM_N("Error", FMODSound::BankFileLoader::LoadState::Error)
SEOUL_END_ENUM()

namespace FMODSound
{

/** Start on the main thread if all deps are already locally serviced. */
static Bool ShouldEventLoadStartOnMainThread(const ContentKey& key, const Content::Handle<ProjectAnchor>& hSoundProjectAnchor)
{
	auto pSoundProject(hSoundProjectAnchor.GetPtr());
	if (hSoundProjectAnchor.IsLoading() || pSoundProject->GetState() != ProjectAnchor::kLoaded)
	{
		// Need to wait for sound project.
		return false;
	}

	auto pvDeps(pSoundProject->GetEventDependencies().Find(key.GetData()));
	if (nullptr == pvDeps)
	{
		// No dependencies, this is an error.
		return false;
	}

	// Check.
	auto p(FileManager::Get());
	for (auto const& dep : (*pvDeps))
	{
		if (p->IsServicedByNetwork(dep))
		{
			return false;
		}
	}

	// Done, can jump straight to main thread.
	return true;
}

EventContentLoader::EventContentLoader(
	const ContentKey& key,
	const Content::Handle<EventAnchor>& hSoundEventAnchor,
	const Content::Handle<ProjectAnchor>& hSoundProjectAnchor)
	: Content::LoaderBase(key.GetFilePath(), ShouldEventLoadStartOnMainThread(key, hSoundProjectAnchor) ? Content::LoadState::kLoadingOnMainThread : Content::LoadState::kLoadingOnWorkerThread)
	, m_Key(key)
	, m_sKey(ToEventKey(key))
	, m_hEntry(hSoundEventAnchor)
	, m_hSoundProjectAnchor(hSoundProjectAnchor)
{
	Content::LoadManager::Get()->BeginSensitiveContent();
	m_hEntry.GetContentEntry()->IncrementLoaderCount();
}

EventContentLoader::~EventContentLoader()
{
	WaitUntilContentIsNotLoading();

	InternalReleaseEntry();
	Content::LoadManager::Get()->EndSensitiveContent();
}

Content::LoadState EventContentLoader::InternalExecuteContentLoadOp()
{
	// Default quantum by default - certain cases may switch scheduling quantums.
	SetJobQuantum(Min(GetJobQuantum(), Jobs::Quantum::kDefault));

	// The load starts on a worker thread if the project was still
	// loading or there are network assets to fetch.
	if (GetContentLoadState() == Content::LoadState::kLoadingOnWorkerThread)
	{
		// Check if we're still waiting for the sound project to load.
		SharedPtr<ProjectAnchor> pAnchor = m_hSoundProjectAnchor.GetPtr();
		if (m_hSoundProjectAnchor.IsLoading() ||
			ProjectAnchor::kLoading == pAnchor->GetState())
		{
			// Swith to the appropriate quantum.
			SetJobQuantum(Jobs::Quantum::kWaitingForDependency);
			return Content::LoadState::kLoadingOnWorkerThread;
		}
		// If we're not waiting but we're not loaded, an error has occured.
		else if (ProjectAnchor::kLoaded != pAnchor->GetState())
		{
			SEOUL_WARN("%s(%d): Sound project in unexpected state: %s",
				m_Key.ToString().CStr(),
				__LINE__,
				EnumToString<ProjectAnchor::State>(pAnchor->GetState()));
			return Content::LoadState::kError;
		}

		// Must use m_Key.GetData() here, event names in this table
		// do not include the event:/ prefix.
		auto pEventBankDeps = pAnchor->GetEventDependencies().Find(m_Key.GetData());

		// Require, error if not found.
		if (nullptr == pEventBankDeps)
		{
			SEOUL_WARN("%s(%d): no event bank dependencies, this indicates an invalid event key or a cooker bug.", m_Key.ToString().CStr(), __LINE__);
			return Content::LoadState::kError;
		}

		// Make sure we don't progress loading any further while file systems are initializing
		// and our bank dependencies are affected.
		if (FileManager::Get()->IsAnyFileSystemStillInitializing())
		{
			for (auto const& e : *pEventBankDeps)
			{
				if (!FileManager::Get()->Exists(e))
				{
					SetJobQuantum(Jobs::Quantum::kWaitingForDependency);
					return GetContentLoadState();
				}
			}
		}

		// Create the loader and perform a fetch of network assets.
		m_pLoader.Reset(SEOUL_NEW(MemoryBudgets::Audio) BankFileLoader(*pEventBankDeps, false));
		{
			// Swith to the appropriate quantum.
			Jobs::ScopedQuantum scope(*this, Jobs::Quantum::kWaitingForDependency);
			if (!m_pLoader->NetworkFetch())
			{
				SEOUL_WARN("%s(%d): failed network fetch of bank dependency.", m_Key.ToString().CStr(), __LINE__);
				return Content::LoadState::kError;
			}
		}

		// Done, switch to main thread for further processing.
		return Content::LoadState::kLoadingOnMainThread;
	}
	// Otherwise - it starts on the main thread. For robustness,
	// we still handle cases (e.g. project still loading).
	else if (GetContentLoadState() == Content::LoadState::kLoadingOnMainThread)
	{
		// Sanity check
		SEOUL_ASSERT(IsMainThread());

		// No loader yet, check for dependency state.
		if (!m_pLoader.IsValid())
		{
			// Check if we're still waiting for the sound project to load.
			SharedPtr<ProjectAnchor> pAnchor = m_hSoundProjectAnchor.GetPtr();
			if (m_hSoundProjectAnchor.IsLoading() ||
				ProjectAnchor::kLoading == pAnchor->GetState())
			{
				// Swith to the appropriate quantum.
				SetJobQuantum(Jobs::Quantum::kWaitingForDependency);

				return Content::LoadState::kLoadingOnMainThread;
			}
			// If we're not waiting but we're not loaded, an error has occured.
			else if (ProjectAnchor::kLoaded != pAnchor->GetState())
			{
				SEOUL_WARN("%s(%d): Sound project in unexpected state: %s",
					m_Key.ToString().CStr(),
					__LINE__,
					EnumToString<ProjectAnchor::State>(pAnchor->GetState()));
				return Content::LoadState::kError;
			}

			// Must use m_Key.GetData() here, event names in this table
			// do not include the event:/ prefix.
			auto pEventBankDeps = pAnchor->GetEventDependencies().Find(m_Key.GetData());

			// Require, error if not found.
			if (nullptr == pEventBankDeps)
			{
				SEOUL_WARN("%s(%d): no event bank dependencies, this indicates an invalid event key or a cooker bug.", m_Key.ToString().CStr(), __LINE__);
				return Content::LoadState::kError;
			}

			// Make sure we don't progress loading any further while file systems are initializing
			// and our bank dependencies are affected.
			if (FileManager::Get()->IsAnyFileSystemStillInitializing())
			{
				for (auto const& e : *pEventBankDeps)
				{
					if (!FileManager::Get()->Exists(e))
					{
						SetJobQuantum(Jobs::Quantum::kWaitingForDependency);
						return GetContentLoadState();
					}
				}
			}

			// Create the loader.
			m_pLoader.Reset(SEOUL_NEW(MemoryBudgets::Audio) BankFileLoader(*pEventBankDeps, false));
		}

		// Check if we still need to fetch assets from the network.
		if (m_pLoader->NeedsNetworkFetch())
		{
			// Swith to the appropriate quantum.
			SetJobQuantum(Jobs::Quantum::kWaitingForDependency);

			// Prefetch and wait.
			m_pLoader->NetworkPrefetch();
			return Content::LoadState::kLoadingOnMainThread;
		}

		// If we're the only reference to the content, "cancel" the load.
		if (m_hEntry.IsUnique())
		{
			m_hEntry.GetContentEntry()->CancelLoad();

			// Release the entry before returning.
			InternalReleaseEntry();
			return Content::LoadState::kLoaded;
		}

		// Perform load of bank dependencies - handling based on result.
		{
			auto const eLoadState = m_pLoader->Load();
			if (BankFileLoader::LoadState::Loading == eLoadState)
			{
				SetJobQuantum(Jobs::Quantum::kWaitingForDependency);
				return Content::LoadState::kLoadingOnMainThread;
			}
			else if (BankFileLoader::LoadState::Error == eLoadState)
			{
				SEOUL_WARN("%s(%d): Sound project in unexpected state: %s",
					m_Key.ToString().CStr(),
					__LINE__,
					EnumToString<BankFileLoader::LoadState>(eLoadState));
				return Content::LoadState::kError;
			}
			// Otherwise, fall through - banks are loaded.
		}

		// Cache FMODSound::Manager and perform further processing for the load.
		CheckedPtr<Manager> pSoundManager = Manager::Get();

		// Get the FMOD event system from the sound manager.
		CheckedPtr<::FMOD::Studio::System> pFMODStudioSystem = pSoundManager->m_pFMODStudioSystem;

		// Acquire description of the sound event.
		::FMOD::Studio::EventDescription* pFMODEventDescription = nullptr;
		{
			// Get the sound event - if this fails, we're done with an error.
			FMOD_RESULT eResult = pFMODStudioSystem->getEvent(m_sKey.CStr(), &pFMODEventDescription);

			// Failure, fail loading the sound event.
			if (FMOD_OK != eResult || nullptr == pFMODEventDescription)
			{
				SEOUL_WARN("%s(%d): Sound event getEvent() failed with error: %s",
					m_Key.ToString().CStr(),
					__LINE__,
					FMOD_ErrorString(eResult));
				return Content::LoadState::kError;
			}
		}

		// Get the state of the sound event.
		FMOD_STUDIO_LOADING_STATE eState = FMOD_STUDIO_LOADING_STATE_ERROR;
		FMOD_RESULT const eStateResult = pFMODEventDescription->getSampleLoadingState(&eState);

		// If we failed querying for state, fail the load.
		if (FMOD_OK != eStateResult)
		{
			SEOUL_WARN("%s(%d): Sound event getState() failed: (%s, %d)",
				m_Key.ToString().CStr(),
				__LINE__,
				FMOD_ErrorString(eStateResult),
				eState);
			return Content::LoadState::kError;
		}
		// If loading state is in error, fail to load.
		else if (FMOD_STUDIO_LOADING_STATE_ERROR == eState)
		{
			SEOUL_WARN("%s(%d): Sound event getState() failed: (%s, %d)",
				m_Key.ToString().CStr(),
				__LINE__,
				FMOD_ErrorString(eStateResult),
				eState);
			return Content::LoadState::kError;
		}
		else if (FMOD_STUDIO_LOADING_STATE_UNLOADED == eState)
		{
			// Start the load.
			auto const eLoadResult = pFMODEventDescription->loadSampleData();
			if (FMOD_OK != eLoadResult)
			{
				SEOUL_WARN("%s(%d): Sound event loadSampleData failed with error: %s",
					m_Key.ToString().CStr(),
					__LINE__,
					FMOD_ErrorString(eLoadResult));
				return Content::LoadState::kError;
			}

			// Swith to the appropriate quantum.
			SetJobQuantum(Jobs::Quantum::kWaitingForDependency);

			// Update marker before returning.
			m_pLoader->UpdateAsyncMarker();

			// If the state is loading, return for now, giving back the
			// main thread's time. We're effectively waiting for the
			// loading system to load the sound event's audio data.
			return Content::LoadState::kLoadingOnMainThread;
		}
		else if (FMOD_STUDIO_LOADING_STATE_LOADING == eState || FMOD_STUDIO_LOADING_STATE_UNLOADING == eState)
		{
			// Swith to the appropriate quantum.
			SetJobQuantum(Jobs::Quantum::kWaitingForDependency);

			// Update marker before returning.
			m_pLoader->UpdateAsyncMarker();

			// If the state is loading, return for now, giving back the
			// main thread's time. We're effectively waiting for the
			// FMOD async loading system to load the sound event's
			// audio data.
			return Content::LoadState::kLoadingOnMainThread;
		}
		// Loaded, we're set.
		else if (FMOD_STUDIO_LOADING_STATE_LOADED == eState)
		{
			SharedPtr< Content::Entry<EventAnchor, ContentKey> > pAnchor(m_hEntry.GetContentEntry());
			if (!pAnchor.IsValid())
			{
				SEOUL_WARN("%s(%d): Sound event anchor resolve failed, anchor is null.",
					m_Key.ToString().CStr(),
					__LINE__);
				return Content::LoadState::kError;
			}

			pAnchor->AtomicReplace(SharedPtr<EventAnchor>(
				SEOUL_NEW(MemoryBudgets::Audio) EventAnchor(m_hSoundProjectAnchor, m_sKey)));

			// Release the entry before returning.
			InternalReleaseEntry();
			return Content::LoadState::kLoaded;
		}
		// Any other state is unexpected.
		else
		{
			return Content::LoadState::kError;
		}
	}

	// Unexpected context.
	return Content::LoadState::kError;
}

/**
 * Release the loader's reference on its content entry - doing this as
 * soon as loading completes allows anything waiting for the load to react
 * as soon as possible.
 */
void EventContentLoader::InternalReleaseEntry()
{
	if (m_hEntry.IsInternalPtrValid())
	{
		// NOTE: We need to release our reference before decrementing the loader count.
		// This is safe, because a Content::Entry's Content::Store always maintains 1 reference,
		// and does not release it until the content is done loading.
		Content::Handle<EventAnchor>::EntryType* pEntry = m_hEntry.GetContentEntry().GetPtr();
		m_hEntry.Reset();
		pEntry->DecrementLoaderCount();
	}
}

ProjectContentLoader::ProjectContentLoader(
	FilePath filePath,
	const Content::Handle<ProjectAnchor>& pSoundProjectAnchor)
	: Content::LoaderBase(filePath)
	, m_hEntry(pSoundProjectAnchor)
	, m_vBankFiles()
	, m_pFileData(nullptr)
	, m_uFileSize(0u)
{
	// Contributes to sensitive content load.
	Content::LoadManager::Get()->BeginSensitiveContent();

	m_hEntry.GetContentEntry()->IncrementLoaderCount();
}

ProjectContentLoader::~ProjectContentLoader()
{
	// Block until this Content::Loaded is in a non-loading state.
	WaitUntilContentIsNotLoading();

	InternalReleaseEntry();

	// Cleanup file data, if we still have it.
	if (m_pFileData)
	{
		MemoryManager::Deallocate(m_pFileData);
		m_pFileData = nullptr;
	}
	m_uFileSize = 0u;

	// No longer contributes to sensitive content load.
	Content::LoadManager::Get()->EndSensitiveContent();
}

Content::LoadState ProjectContentLoader::InternalExecuteContentLoadOp()
{
	// Default quantum by default - certain cases may switch scheduling quantums.
	SetJobQuantum(Min(GetJobQuantum(), Jobs::Quantum::kDefault));

	// Handle reloading - which can overlap ops. Always kLoading until the last
	// op completes.
	SharedPtr<ProjectAnchor> pAnchor = m_hEntry.GetPtr();
	pAnchor->SetState(ProjectAnchor::kLoading);

	if (Content::LoadState::kLoadingOnFileIOThread == GetContentLoadState())
	{
		// Conditionally cook if the cooked file is not up to date with the source file.
		CookManager::Get()->CookIfOutOfDate(GetFilePath());

		if (!FileManager::Get()->ReadAll(
			GetFilePath(),
			m_pFileData,
			m_uFileSize,
			0u,
			MemoryBudgets::Content))
		{
			SEOUL_WARN("%s(%d): Loading sound project failed, ReadAll() failed.",
				GetFilePath().CStr(),
				__LINE__);
			goto error;
		}

		return Content::LoadState::kLoadingOnWorkerThread;
	}
	else if (Content::LoadState::kLoadingOnWorkerThread == GetContentLoadState())
	{
		void* pC = nullptr;
		UInt32 uC = 0u;
		if (!ZSTDDecompress(m_pFileData, m_uFileSize, pC, uC, MemoryBudgets::Content))
		{
			SEOUL_WARN("%s(%d): Loading sound project failed, ZSTDDecompress() failed.",
				GetFilePath().CStr(),
				__LINE__);
			goto error;
		}
		MemoryManager::Deallocate(m_pFileData);
		m_pFileData = pC;
		m_uFileSize = uC;

		if (!InternalGetBanksAndEvents(m_vBankFiles, m_tEvents))
		{
			SEOUL_WARN("%s(%d): Loading sound project failed, failed decoding FSB files from project file.",
				GetFilePath().CStr(),
				__LINE__);
			goto error;
		}

		// Make sure we don't progress loading any further while file systems are initializing
		// and our bank dependencies are affected.
		{
			Bool bRestoreQuantum = false;
			while (FileManager::Get()->IsAnyFileSystemStillInitializing())
			{
				Bool bDone = true;
				for (auto const& e : m_vBankFiles)
				{
					if (!FileManager::Get()->Exists(e))
					{
						bDone = false;
						break;
					}
				}

				if (!bDone)
				{
					bRestoreQuantum = true;
					SetJobQuantum(Jobs::Quantum::kWaitingForDependency);
					Jobs::Manager::Get()->YieldThreadTime();
				}
				else
				{
					break;
				}
			}
			if (bRestoreQuantum)
			{
				// Default quantum by default - certain cases may switch scheduling quantums.
				SetJobQuantum(Min(GetJobQuantum(), Jobs::Quantum::kDefault));
			}
		}

		// Create the loader and perform a fetch of network assets.
		m_pLoader.Reset(SEOUL_NEW(MemoryBudgets::Audio) BankFileLoader(m_vBankFiles, true));
		{
			// Swith to the appropriate quantum.
			Jobs::ScopedQuantum scope(*this, Jobs::Quantum::kWaitingForDependency);
			if (!m_pLoader->NetworkFetch())
			{
				SEOUL_WARN("%s(%d): Loading sound project failed, NetworkFetch failed.",
					GetFilePath().CStr(),
					__LINE__);
				goto error;
			}
		}

		// Finish on the main thread.
		return Content::LoadState::kLoadingOnMainThread;
	}
	else if (Content::LoadState::kLoadingOnMainThread == GetContentLoadState())
	{
		// Perform load of bank dependencies - handling based on result.
		{
			auto const eLoadState = m_pLoader->Load();
			if (BankFileLoader::LoadState::Loading == eLoadState)
			{
				SetJobQuantum(Jobs::Quantum::kWaitingForDependency);
				return Content::LoadState::kLoadingOnMainThread;
			}
			else if (BankFileLoader::LoadState::Error == eLoadState)
			{
				SEOUL_WARN("%s(%d): Loading sound project failed, load state is in error.",
					GetFilePath().CStr(),
					__LINE__);
				goto error;
			}
			// Otherwise, fall through - banks are loaded.
		}

		// Done.
		pAnchor->SetBankFiles(m_vBankFiles);
		pAnchor->SetEventDependencies(m_tEvents);
		pAnchor->SetState(ProjectAnchor::kLoaded);

		InternalReleaseEntry();
		return Content::LoadState::kLoaded;
	}

	// If we get here, some sort of error occured. Clean up
	// resources and return as such.
error:
	if (m_pFileData)
	{
		MemoryManager::Deallocate(m_pFileData);
		m_pFileData = nullptr;
	}
	m_uFileSize = 0u;

	pAnchor->SetState(ProjectAnchor::kError);
	return Content::LoadState::kError;
}

/**
 * Release the loader's reference on its content entry - doing this as
 * soon as loading completes allows anything waiting for the load to react
 * as soon as possible.
 */
void ProjectContentLoader::InternalReleaseEntry()
{
	if (m_hEntry.IsInternalPtrValid())
	{
		// NOTE: We need to release our reference before decrementing the loader count.
		// This is safe, because a Content::Entry's Content::Store always maintains 1 reference,
		// and does not release it until the content is done loading.
		Content::Handle<ProjectAnchor>::EntryType* pEntry = m_hEntry.GetContentEntry().GetPtr();
		m_hEntry.Reset();
		pEntry->DecrementLoaderCount();
	}
}

Bool ProjectContentLoader::InternalGetBanksAndEvents(BankFiles& rvBankFiles, EventDependencies& rtEvents)
{
	// Scope acquire the file data for reading. Relinquish on completion.
	auto pFileData = (Byte*)m_pFileData;
	UInt32 uFileSize = m_uFileSize;

	StreamBuffer buffer;
	auto const scoped(MakeScopedAction(
	[&]()
	{
		buffer.TakeOwnership(pFileData, uFileSize);
		m_pFileData = pFileData = nullptr;
		m_uFileSize = uFileSize = 0u;
	},
	[&]()
	{
		buffer.RelinquishBuffer(pFileData, uFileSize);
		m_pFileData = pFileData;
		m_uFileSize = uFileSize;
	}));

	// Cache the directory of the project file for further processing.
	String const sProjectFileDirectory = Path::GetDirectoryName(GetFilePath().GetAbsoluteFilename());

	// Perform the read.
	return SoundUtil::ReadBanksAndEvents(sProjectFileDirectory, buffer, rvBankFiles, rtEvents);
}

} // namespace FMODSound

} // namespace Seoul

#endif // /#if SEOUL_WITH_FMOD
