/**
 * \file ContentLoaderBase.h
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

#pragma once
#ifndef CONTENT_LOADER_BASE_H
#define CONTENT_LOADER_BASE_H

#include "Atomic32.h"
#include "FilePath.h"
#include "JobsJob.h"
#include "Prereqs.h"

namespace Seoul::Content
{

enum class LoadState
{
	/** Initial state, loader is has not entered any active load state. */
	kNotLoaded,

	/** Content loader is running on a worker thread. */
	kLoadingOnWorkerThread,

	/** Content loader is running on the main thread. */
	kLoadingOnMainThread,

	/** Content loader is running on the file IO thread. */
	kLoadingOnFileIOThread,

	/** Content loader is running on the render thread. */
	kLoadingOnRenderThread,

	/** Content loader is complete and was successful. */
	kLoaded,

	/** Content loader is complete and failed. */
	kError
};

/**
 * Content::LoaderBase is an abstract base class that should
 * be specialized for all content that needs to be loaded off disk.
 * Specialized classes will then be used by Content::LoadManager (or in
 * special use cases, used directly) to load content into associated
 * C++ objects.
 */
class LoaderBase SEOUL_ABSTRACT : public Jobs::Job
{
public:
	/** @return The total number of active loader instances across the engine. */
	static Atomic32Type GetActiveLoaderCount()
	{
		return s_ActiveLoaderCount;
	}

	// Initiate the content load. Actual loading will then occur
	// somewhat or fully asynchronously.
	//
	// IMPORTANT: It is only valid to call this method once per
	// Content::LoaderBase instantiation.
	void StartContentLoad();

	/**
	 * Get the current state of this ContentLoader. Until
	 * StartContentLoad() is called, the state will be kNotLoaded.
	 */
	LoadState GetContentLoadState() const
	{
		return m_eContentLoadState;
	}

	/**
	 * @return the FilePath associated with the content being loaded
	 * by this Content::LoaderBase
	 */
	FilePath GetFilePath() const
	{
		return m_FilePath;
	}

	/** Provides more detailed path data for some loader types, for debugging. */
	virtual String GetContentKey() const
	{
		return GetFilePath().GetRelativeFilenameInSource();
	}

	/**
	 * @return True if content is considered loaded, false otherwise.
	 *
	 * This method *only* returns true if GetContentLoadState()
	 * also returns kLoaded.
	 */
	Bool IsContentLoaded() const
	{
		return (LoadState::kLoaded == m_eContentLoadState);
	}

	/**
	 * @return True if this Content is currently in the process of
	 * being loaded, false otherwise.
	 */
	Bool IsContentLoading() const
	{
		return IsJobRunning();
	}

	// Blocks until this Content::Loader is in a non-loading state.
	void WaitUntilContentIsNotLoading();

	// Methods in this block must be implemented by subclasses
	// to fully define a content loader.
protected:
	virtual ~LoaderBase();
	SEOUL_REFERENCE_COUNTED_SUBCLASS(LoaderBase);

	virtual LoadState InternalExecuteContentLoadOp() = 0;

	LoaderBase(
		FilePath filePath,
		LoadState eStartContentLoadState = LoadState::kLoadingOnFileIOThread);

private:
	virtual void InternalExecuteJob(Jobs::State& reNextState, ThreadId& rNextThreadId) SEOUL_OVERRIDE SEOUL_SEALED;

	static Atomic32 s_ActiveLoaderCount;

	FilePath const m_FilePath;
	LoadState const m_eStartContentLoadState;
	Atomic32Value<LoadState> m_eContentLoadState;
	Atomic32Value<Bool> m_bWasWaiting;
}; // class Content::LoaderBase

} // namespace Seoul::Content

#endif // include guard
