/**
 * \file ContentLoaderBase.cpp
 * \brief Abstract base class that should be specialized for all
 * content that needs to be loaded off disk.
 *
 * Creating a subclass of Content::LoaderBase
 * - override Content::LoaderBase::InternalExecuteContentLoadOp().
 * - this method will automatically be called whenever
 *   Content::LoaderBase::StartContentLoad() is called (by Content::LoadManager
 *   or otherwise).
 * - it will be called again and again until it returns kLoaded or
 *   kError.
 * - the first time InternalExecuteContentLoadOp() is called, it will be executing on the File IO
 *   thread (see Threading.h). After this, it will be executing
 *   on whatever thread was specified in the return value from
 *   the last call to InternalExecuteContentLoadOp().
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ContentLoaderBase.h"
#include "ContentLoadManager.h"
#include "EventsManager.h"
#include "FileManager.h"
#include "JobsFunction.h"
#include "JobsManager.h"
#include "Logger.h"

namespace Seoul::Content
{

Atomic32 LoaderBase::s_ActiveLoaderCount(0);

/** Utility, used on content load completion to dispatch and release the loader. */
static void LoadManagerSendFileLoadCompleteEvent(LoaderBase* pLoader, FilePath filePath)
{
	// TODO: This is only necessary to work around
	// an unfortunate design problem with Content::Handle<>::IsLoading().
	// Basically, we have a number of use cases where things check IsLoading()
	// and *also* listen for the FileLoadComplete callback. That logic is
	// dependent on IsLoading() being true until after FileLoadComplete()
	// has been sent, but the only way that is enforced is by not decrementing
	// the loader count until the destructor of the loader, and keeping a reference
	// to the loader (to prevent the destructor) until after the FileLoadComplete()
	// callback is dispatched, which happens on the main thread and therefore
	// has to wait for the main thread tick loop before a load can be considered
	// complete.
	//
	// Even more unfortunate, we need to wait to release the loader until this moment on the main thread,
	// but we need to release it *before* actually calling the event dispatch, so that IsLoading()
	// is false inside the call.

	// Release the reference to the loader made by ContentLoadManager.
	SeoulGlobalDecrementReferenceCount(pLoader);

	// If Content::LoadManager still exists, dispatch the file load complete
	// callback for the loaded file.
	if (LoadManager::Get())
	{
		if (filePath.IsValid())
		{
			Events::Manager::Get()->TriggerEvent(FileLoadCompleteEventId, filePath);
		}
	}
}

/** @return a ThreadId corresponding to the start content load state. */
inline static ThreadId FromStartContentLoadStateToThreadId(LoadState eStartContentLoadState)
{
	switch (eStartContentLoadState)
	{
	case LoadState::kLoadingOnWorkerThread: return ThreadId();
	case LoadState::kLoadingOnFileIOThread: return GetFileIOThreadId();
	case LoadState::kLoadingOnMainThread: return GetMainThreadId();
	case LoadState::kLoadingOnRenderThread: return GetRenderThreadId();

		// All other cases, start on the file IO thread. This is our default
		// thread for content jobs.
	default:
		return GetFileIOThreadId();
	};
}

/**
 * If Content::LoaderBase is a Job, then the initial call to
 * InternalExecuteContentLoadOp will *always* execute on the file IO
 * thread.
 */
LoaderBase::LoaderBase(
	FilePath filePath,
	LoadState eStartContentLoadState /*= Content::LoadState::kLoadingOnFileIOThread*/)
	: Job(FromStartContentLoadStateToThreadId(eStartContentLoadState))
	, m_FilePath(filePath)
	, m_eStartContentLoadState(eStartContentLoadState)
	, m_eContentLoadState(LoadState::kNotLoaded)
	, m_bWasWaiting(false)
{
	// Active loader.
	++s_ActiveLoaderCount;
}

LoaderBase::~LoaderBase()
{
	// Make sure that if this Content::LoaderBase is being executed as a
	// concurrent Job, it is complete before destruction.
	WaitUntilJobIsNotRunning();

	// No longer active.
	--s_ActiveLoaderCount;
}

/**
 * Initiate the content load. Actual loading will then occur
 * somewhat or fully asynchronously.
 *
 * \warning This method can only be called once per instantiated
 * Content::Loader specialization. Calling this method a second time
 * is an error.
 *
 * \warning It is an error to call this method from threads other than
 * the main thread.
 */
void LoaderBase::StartContentLoad()
{
	SEOUL_ASSERT(LoadState::kNotLoaded == m_eContentLoadState);

	// Start off in whatever thread was specified by the
	// content loader specialization.
	m_eContentLoadState = m_eStartContentLoadState;

	// Start the job, this will handle actual content loading tasks.
	StartJob(true);
}

/**
 * Blocks the calling thread until this Content::Loader is
 * in a non-loading state.
 */
void LoaderBase::WaitUntilContentIsNotLoading()
{
	// Identical to WaitUntilJobIsNotRunning().
	WaitUntilJobIsNotRunning();
}

/**
 * Specialization of Job::InternalExecuteJob(), just calls
 * InternalExecuteContentLoadOp() and converts state and thread hint
 * as needed. Sealed to prevent specialization by child classes.
 */
void LoaderBase::InternalExecuteJob(Jobs::State& reNextState, ThreadId& rNextThreadId)
{
	// General purpose handling - if file systems are still initializing and the
	// file of this op is not found, delay running the op. IsInitializing() on a FileSystem
	// is expected to return false under shutdown conditions so this checks does not lookup forever.
	if (FileManager::Get()->IsAnyFileSystemStillInitializing())
	{
		if (!FileManager::Get()->Exists(GetFilePath()))
		{
			SetJobQuantum(Jobs::Quantum::kWaitingForDependency);
			m_bWasWaiting = true;
			return;
		}
	}

	// Restore job level.
	if (m_bWasWaiting)
	{
		m_bWasWaiting = false;
		// Default quantum by default - certain cases may switch scheduling quantums.
		SetJobQuantum(Min(GetJobQuantum(), Jobs::Quantum::kDefault));
	}

	// Execute the loading op.
	m_eContentLoadState = InternalExecuteContentLoadOp();

	// Based on the result.
	switch (m_eContentLoadState)
	{
	case LoadState::kLoadingOnMainThread:
		reNextState = Jobs::State::kScheduledForOrRunning;
		rNextThreadId = GetMainThreadId();
		break;
	case LoadState::kLoadingOnWorkerThread:
		reNextState = Jobs::State::kScheduledForOrRunning;
		rNextThreadId = ThreadId();
		break;
	case LoadState::kLoadingOnFileIOThread:
		reNextState = Jobs::State::kScheduledForOrRunning;
		rNextThreadId = GetFileIOThreadId();
		break;
	case LoadState::kLoadingOnRenderThread:
		reNextState = Jobs::State::kScheduledForOrRunning;
		rNextThreadId = GetRenderThreadId();
		break;
	case LoadState::kLoaded:
		reNextState = Jobs::State::kComplete;

		// Queue up a Job to send off the load finished event.
		Jobs::AsyncFunction(
			GetMainThreadId(),
			&LoadManagerSendFileLoadCompleteEvent,
			this,
			m_FilePath);
		break;
	case LoadState::kError:
		reNextState = Jobs::State::kError;

		// Loads will cancel during shutdown, filter this warning in that
		// case.
		if (LoadManager::Get()->GetLoadContext() != LoadContext::kShutdown)
		{
			SEOUL_WARN(
				"Failed to load content: %s",
				GetContentKey().CStr());
		}

		// Release the reference to the loader made by Content::LoadManager,
		// immediately, since we have no results to dispatch.
		SeoulGlobalDecrementReferenceCount(this);
		break;
	default:
		reNextState = Jobs::State::kError;

		// Release the reference to the loader made by Content::LoadManager,
		// immediately, since we have no results to dispatch.
		SeoulGlobalDecrementReferenceCount(this);
		SEOUL_FAIL("Out of sync enum in InternalExecuteJob().");
		break;
	};
}

} // namespace Seoul::Content
