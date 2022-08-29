/**
 * \file ContentLoadManager.h
 * \brief Content::LoadManager handles queued loading of content data from permanent
 * storage.
 *
 * While Content::LoadManager is a single choke point for content loads,
 * content management is otherwise handled by disparate managers (i.e. TextureManager).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef CONTENT_LOAD_MANAGER_H
#define CONTENT_LOAD_MANAGER_H

#include "AtomicRingBuffer.h"
#include "ContentLoaderBase.h"
#include "ContentHandle.h"
#include "ContentReload.h"
#include "HashTable.h"
#include "List.h"
#include "Mutex.h"
#include "ScopedPtr.h"
#include "Singleton.h"
namespace Seoul { namespace Content { struct ChangeEvent; } }
namespace Seoul { namespace Content { class ChangeNotifier; } }

namespace Seoul::Content
{

/**
 * Global application context used by Content::LoadManager,
 * determines termination and loggin behaviors.
 */
enum class LoadContext
{
	kStartup,
	kRun,
	kShutdown,
};

enum class LoadManagerHotLoadMode
{
	kNoAction, // Nothing to do yet.
	kAccept, // One time accept of a hot load.
	kReject, // One time reject of a hot load.
	kPermanentAccept, // Indefinitely accept hot loads until mode is changed.
};

/**
 * The Events::Manager id that will be triggered when a file changes on disk.
 *
 * Signature: Bool(ContentChangeEvent*)
 */
static const HString FileChangeEventId("ContentFileChangeEvent");

/**
 * The Events::Manager id that will be triggered to check if a file exists - callbacks
 * should return true if the file defined by the FilePath argument is currently loaded.
 *
 * Signature: Bool(FilePath)
 */
static const HString FileIsLoadedEventId("FileIsLoadedEventId");

/**
 * The Events::Manager id that will be triggered when a piece of content is finished
 * loading successfully - callbacks should never return true, to allow all interested callbacks
 * to receive the event.
 *
 * Signature: Bool(FilePath)
 */
static const HString FileLoadCompleteEventId("FileLoadCompleteEventId");

/**
 * The Events::Manager id that will be triggered while the Content::LoadManager is waiting
 * for content load task completion on the main thread. This allows systems that must be
 * tricked for a load task to complete to get an opportunity to do so.
 */
static const HString MainThreadTickWhileWaiting("ContentMainThreadTickWhileWaiting");

/**
 * The Events::Manager id that will be triggered during Content::Manager::Poll() - this
 * gives other components of the content system a chance to perform per-poll maintenance.
 *
 * Signature: Bool(void)
 *
 * This callback can be called more than once per frame, but will always be called on the main thread.
 */
static const HString PollEventId("ContentPollEvent");

/**
 * The Events::Manager id that will be triggered when
 * a reload of files is requested. Handlers are expected to check
 * the input set and trigger a reload of any currently loaded
 * content contained in the input set (or trigger a reload of
 * all content if the input set is empty). Content reloaded should
 * be added to the output set. pContentReload is guaranteed
 * to be non-null.
 *
 * Signature: void(ContentReload* pContentReload);
 */
static const HString ReloadEventId("ReloadEvent");

/**
 * The Events::Manager id that will be triggered when
 * an unload of files is requested. Content is in most cases
 * unloaded as soon as possible. In some cases and for some types
 * of Content, an explicit Unload() call will immediately and
 * as aggressively as possible unload the content from memory.
 */
static const HString UnloadEventId("UnloadEvent");

/**
 * Content::LoadManager handles queued loading of content data
 * from persistent storage. It also handles file change events
 * to facilitate hot loading.
 */
class LoadManager SEOUL_SEALED : public Singleton<LoadManager>
{
public:
	typedef Vector<FilePath, MemoryBudgets::Cooking> DepVector;
	typedef HashTable<FilePath, DepVector, MemoryBudgets::Content> DepTable;

	LoadManager();
	~LoadManager();

	/** Retrieve current content context. */
	LoadContext GetLoadContext() const
	{
		return m_eContext;
	}

	/** Update the current content context. */
	void SetLoadContext(LoadContext eContext)
	{
		m_eContext = eContext;
	}

	// Return true if load operations are active, false otherwise.
	Bool HasActiveLoads() const;

	// Return true if content is loading that is marked as
	// "sensitive" - some operations (e.g. patching or hot loading)
	// should wait for these loads to complete, since switching files
	// when these operations are in-flight can generate an error.
	Bool IsSensitiveContentLoading() const
	{
		return (0 != m_SensitiveContent);
	}

	// Return true if any thread is waiting on all loads to complete.
	Bool IsWaitingForLoadsToFinish() const
	{
		return (0 != m_AllLoadWait);
	}

	// Specify a list of files to reload or an empty set to reload all files.
	// Files reloaded will be added to the output set, to allow monitoring of
	// load progress.
	void Reload(Reload& rReload);

	// An explicit and aggressive unload of all currently loaded content.
	void UnloadAll();

	/**
	 * Reference and start a new content loader.
	 */
	void Queue(const SharedPtr<LoaderBase>& pLoader)
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
		// has to wait for the main thread tick loop to finish the load.

		// Grab a reference to the loader.
		LoaderBase* p = pLoader.GetPtr();
		SeoulGlobalIncrementReferenceCount(p);

		// Kick off the loader.
		pLoader->StartContentLoad();
	}

	// Not thread safe - should be called once per frame on the main
	// thread during the PrePose() phase.
	void PrePose();

	// Wait until all active content load ops are complete.
	void WaitUntilAllLoadsAreFinished();

	// Wait until a specific load has completed.
	template <typename T>
	void WaitUntilLoadIsFinished(const Handle<T>& hContent)
	{
		// Early out case.
		if (!hContent.IsLoading()) { return; }

		// Handle looping with proper thread relinquish.
		InteranlWaitUntilLoadIsFinished(
			&LoadManager::InternalIsLoading<T>,
			(void*)&hContent,
			ContentKeyToFilePath(hContent.GetKey()));
	}

	/** Returns true if specified content will be available after a loading request */
	Bool IsContentAvailable(FilePath filePath) const;

#if SEOUL_HOT_LOADING
	// Read-only copy of the current set of ContentChanges. Must only be
	// accessed from the main thread.
	typedef HashTable<FilePath, SharedPtr<ChangeEvent>, MemoryBudgets::Content> Changes;
	const Changes& GetContentChanges() const
	{
		SEOUL_ASSERT(IsMainThread());

		return m_tContentChanges;
	}

	// Call to associate the set of files vDependencies with filePath.
	//
	// This method assumes that vDependencies is the complete set of dependencies.
	// It clears any previous dependency association with filePath.
	void SetDependencies(FilePath filePath, const DepVector& vDependencies);
#endif // /#if SEOUL_HOT_LOADING

	/**
	 * Start a scope in which hot loading is suppressed. Code that reacts
	 * to hot loads is expected to not run when IsHotLoadingSuppressed() returns
	 * true.
	 */
	void BeginHotLoadSuppress()
	{
		++m_HotLoadSuppress;
	}

	/**
	 * End a scope in which hot loading is suppressed. Code that reacts
	 * to hot loads is expected to not run when IsHotLoadingSuppressed() returns
	 * true.
	 */
	void EndHotLoadSuppress()
	{
		--m_HotLoadSuppress;
	}

	// Tracking of sensitive content load.
	void BeginSensitiveContent() { ++m_SensitiveContent; }
	void EndSensitiveContent() { --m_SensitiveContent; }

	/** @return True if hot loading has been suppressed, false otherwise. */
	Bool IsHotLoadingSuppressed() const
	{
		return (0 != m_HotLoadSuppress);
	}

	/** Setup the hot load mode - typically a single action, but can be permanent accept. */
	void SetHoadLoadMode(LoadManagerHotLoadMode eHotLoadMode)
	{
		m_eHotLoadMode = eHotLoadMode;
	}

	// Allow hot loading to be momentarily suppressed for a specific file. Intended use case is saving.
	void TempSuppressSpecificHotLoad(FilePath filePath);

	/**
	 * Conditional checking - when enabled, any blocking on a pending load
	 * on the main or render threads generates a warning.
	 */
	void SetEnableBlockingLoadCheck(Bool bEnable)
	{
#if SEOUL_LOGGING_ENABLED
		m_bBlockingLoadCheck = bEnable;
#endif // /#if SEOUL_LOGGING_ENABLED
	}

private:
	void InteranlWaitUntilLoadIsFinished(
		Bool (*isLoading)(void*),
		void* pContent,
		const FilePath& filePath);

	template <typename T>
	static Bool InternalIsLoading(void* p)
	{
		return reinterpret_cast<Handle<T>*>(p)->IsLoading();
	}

	Atomic32Value<LoadContext> m_eContext{ LoadContext::kStartup };
	Atomic32 m_HotLoadSuppress;
	Atomic32 m_AllLoadWait;
	Atomic32 m_SensitiveContent;
	LoadManagerHotLoadMode m_eHotLoadMode;
#if SEOUL_LOGGING_ENABLED
	Atomic32Value<Bool> m_bBlockingLoadCheck;
#endif // /#if SEOUL_LOGGING_ENABLED

#if SEOUL_HOT_LOADING
	void InsideLockClearDependencies(FilePath filePath);
	void InternalPollContentChanges();
	void InternalInsertEventAndDependents(ChangeEvent* pEvent);
	void InternalDispatchContentChanges();
	Mutex m_DependencyMutex;
	DepTable m_tDependencyTable;
	DepTable m_tDependentsTable;
	ScopedPtr<ChangeNotifier> m_pContentChangeNotifier;
	Changes m_tContentChanges;
	typedef HashTable<FilePath, Int64, MemoryBudgets::Content> SuppressSpecific;
	SuppressSpecific m_tHotLoadingSuppressSpecific;
#endif

	SEOUL_DISABLE_COPY(LoadManager);
}; // class Content::LoadManager

} // namespace Seoul::Content

#endif // include guard
