/**
 * \file ContentStore.h
 * \brief Content::Store<> is a generic class that can be used to
 * manage content which is loaded from persistent media. It handles
 * unloading the content when all references to the content have been
 * released, as well as providing Getters() to acquire content by
 * key, and trigger loads when content has been requested for the
 * first time.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef CONTENT_STORE_H
#define CONTENT_STORE_H

#include "HashTable.h"
#include "ContentChangeNotifier.h"
#include "ContentHandle.h"
#include "ContentReload.h"
#include "ContentTraits.h"
#include "Delegate.h"
#include "JobsManager.h"
#include "Logger.h"
#include "SharedPtr.h"
#include "Mutex.h"
#include "ThreadId.h"

namespace Seoul::Content
{

/**
 * Base class of Content::Store<T>, likely you don't want to use this directly ever,
 * this class exists to provide common functionality across specializations of Content::Store<>.
 */
class BaseStore
{
public:
	SEOUL_DELEGATE_TARGET(BaseStore);

	virtual ~BaseStore();

	// Overriden by Content::Store<> to handle FileIsLoaded queries.
	virtual Bool IsFileLoaded(FilePath filePath) const = 0;

	// Overriden by Content::Store<> to handle FileChange events.
	virtual Bool FileChange(ChangeEvent* pFileChangeEvent) = 0;

	// Overriden by Content::Store<> to handle Reload events.
	virtual void Reload(Reload& rContentReload) = 0;

	// Overriden by Content::Store<> to handle Unload events.
	//
	// Returns the number of entries remaining after this
	// call to Unload.
	virtual UInt32 Unload() = 0;

	// Override by Content::Store<> to handle UnloadLRU events.
	virtual Bool UnloadLRU() = 0;

protected:
	BaseStore(Bool bUnloadAll);
	const Bool m_bUnloadAll;

private:
	Bool CallFileChange(ChangeEvent* pFileChangeEvent);
	Bool CallIsFileLoaded(FilePath filePath);
	void CallPoll();
	void CallReload(Content::Reload* pContentReload);
	void CallUnload(UInt32* puTotalRemaining);

	SEOUL_DISABLE_COPY(BaseStore);
}; // class BaseStore

// Shared common.
void StoreCompleteSyncLoad(const FilePath& filePath);

/**
 * Class that provides management of game content - handles reference count
 * tracking of content items and thread-safe retrieval of content by key.
 * Actual loader management (reading content from disk) is managed by ContentLoadManager.
 */
template <typename T>
class Store SEOUL_SEALED : public BaseStore
{
public:
	typedef typename Traits<T>::KeyType KeyType;
	typedef Entry<T, KeyType> Entry;

	Store(Bool bUnloadAll = true)
		: BaseStore(bUnloadAll)
		, m_uUnloadLRUThresholdInBytes(0u)
		, m_tContent()
		, m_Mutex()
		, m_pLRUHead(nullptr)
		, m_pLRUTail(nullptr)
		, m_bOkLRU(false)
	{
	}

	virtual ~Store()
	{
		auto const bClearSucceeded = Clear();
		(void)bClearSucceeded;

#if SEOUL_LOGGING_ENABLED
		// At this point, if the clear fails, we're leaking memory, or allowing
		// references to content beyond the lifespan of their containment system.
		if (!bClearSucceeded)
		{
			Lock lock(m_Mutex);
			for (auto const& pair : m_tContent)
			{
				SEOUL_WARN("Leaking content: '%s'", ContentKeyToFilePath(pair.First).CStr());
			}
		}
#endif //SEOUL_LOGGING_ENABLED

		// Sanity check.
		SEOUL_ASSERT(nullptr == m_pLRUHead);
		SEOUL_ASSERT(nullptr == m_pLRUTail);
	}

	/**
	 * Mutator, applys a function delegate delegate to all entires
	 * in this ContentStore.
	 */
	typedef Delegate<Bool(const Handle<T>& h)> ApplyDelegate;
	void Apply(const ApplyDelegate& delegate) const
	{
		Lock lock(m_Mutex);
		for (auto i = m_tContent.Begin(); m_tContent.End() != i; ++i)
		{
			// If the delegate returns true, this indicates "handled", so stop
			// walking the table.
			if (delegate(Handle<T>(i->Second)))
			{
				return;
			}
		}
	}

	/**
	 * Mutator, applys a function delegate delegate to a subset
	 * of entries in this ContentStore.
	 *
	 * Entries not in this Content::Store specified in vEntries are silently skipped.
	 * As a result, Apply() is implicitly applied to only currently loaded/loading entries.
	 */
	template <typename VECTOR_TYPE>
	void Apply(const ApplyDelegate& delegate, const VECTOR_TYPE& vEntries) const
	{
		Lock lock(m_Mutex);
		for (auto const& key : vEntries)
		{
			CheckedPtr<Entry> pEntry;
			if (!m_tContent.GetValue(key, pEntry))
			{
				continue;
			}

			// If the delegate returns true, this indicates "handled", so stop
			// walking the table.
			if (delegate(Handle<T>(pEntry)))
			{
				return;
			}
		}
	}

	/**
	 * Attempt to flush all content references from this Content::Store<>.
	 *
	 * @return Success on flush, false otherwise. If this method returns false:
	 * - a Content::Traits<T>::PrepareDelete() call returned false.
	 * - an element contained a reference count > 1.
	 */
	Bool Clear()
	{
		Lock lock(m_Mutex);
		UInt32 uLastRemaining = m_tContent.GetSize();
		while (true)
		{
			// Let the Jobs::Manager do some work, give loaders time to complete.
			Jobs::Manager::Get()->YieldThreadTime();
			auto const uRemaining = InsideLockUnload();

			// If we hit 0, we're done.
			if (0u == uRemaining) { return true; }

			// Otherwise, if we fail to remove any entries, but we
			// failed.
			if (uRemaining == uLastRemaining) { return false; }

			// Otherwise, update last and continue.
			uLastRemaining = uRemaining;
		}
	}

	/**
	 * @return A Content::Handle<T> of key - if key does not already
	 * have a Content::Handle<T>, this method will create a new entry and initiate
	 * a load operation of the content. As a result, the Content::Handle<T>
	 * returned by this method will always return true for Content::Handle<T>::IsInternalPtrValid().
	 *
	 * If bSyncLoad is true, when necessary and supported, the content will be synchronously
	 * loaded in the body of GetContent(). It is exceptionally rare that this is what you
	 * want, and most content types don't even support this operation.
	 *
	 * Currently, it is reserved for Settings and Scripts, which are commonly synchronously
	 * loaded as part of our Script VM startup, and on some platforms, performing these loads
	 * in other threads results in very slow load times, due to thread contention
	 * coupled with selective thread priority management by the platform's
	 * scheduler.
	 */
	Handle<T> GetContent(const KeyType& key, Bool bSyncLoad = false)
	{
		Lock lock(m_Mutex);

		// If an entry exists already, return it.
		CheckedPtr<Entry> p = nullptr;
		if (m_tContent.GetValue(key, p))
		{
			// Content loaders are allowed to cancel loads if they detect that they hold
			// the only reference to a piece of content mid-load. However, if GetContent()
			// is called after the cancel but before Content::Store<> can cleanup the entry,
			// the result is a piece of content that is never loaded.
			Handle<T> pReturn(p);
			if (p->WasLoadCancelled())
			{
				p->ResetCancelledLoadFlag();

				// Free the mutex around the call to Load.
				m_Mutex.Unlock();
				Traits<T>::Load(key, pReturn);
				m_Mutex.Lock();
			}

			// (Re)insert it back into our LRU list.
			p->LRUInsert(&m_pLRUHead, &m_pLRUTail);
			m_bOkLRU = false;
			return pReturn;
		}

		// Create a new entry, increment its reference count to claim ownership of it.
		p = SEOUL_NEW(MemoryBudgets::Content) Entry(key, Traits<T>::GetPlaceholder(key));
		SeoulGlobalIncrementReferenceCount(p.Get());

		// Insert  the entry = if this fails, it means the key is an invalid key.
		if (!m_tContent.Insert(key, p).Second)
		{
			SEOUL_ASSERT(Table::Traits::GetNullKey() == key);

			// Allow the nullptr key case to fall through, using the placeholder
			// content in this case.
			Handle<T> pReturn(Traits<T>::GetPlaceholder(key).GetPtr());
			SeoulGlobalDecrementReferenceCount(p.Get());
			return pReturn;
		}

		// Initiate load of the entry.
		Handle<T> hReturn(p);

		// Free the mutex around the call to Load.
		m_Mutex.Unlock();

		// Sync load case, if supported.
		if (bSyncLoad && Traits<T>::CanSyncLoad)
		{
			Traits<T>::SyncLoad(key, hReturn);
			{
				auto pEntry(hReturn.GetContentEntry());
				if (pEntry.IsValid())
				{
					pEntry->OnSynchronousLoad();
				}
			}
			StoreCompleteSyncLoad(ContentKeyToFilePath(key));
		}
		else
		{
			// Otherwise, schedule an async load.
			Traits<T>::Load(key, hReturn);
		}

		// Restore the mutex lock.
		m_Mutex.Lock();

		// (Re)insert it back into our LRU list before returning.
		p->LRUInsert(&m_pLRUHead, &m_pLRUTail);
		m_bOkLRU = false;
		return hReturn;
	}

	/**
	 * Replace the value that is associated with key. This is a thread-safe
	 * operation.
	 *
	 * @return a Content::Handle<T> to the replaced content entry.
	 */
	Handle<T> SetContent(const KeyType& key, const SharedPtr<T>& p)
	{
		Lock lock(m_Mutex);

		// If an entry does not exist, create one.
		CheckedPtr<Entry> pEntry = nullptr;
		if (!m_tContent.GetValue(key, pEntry))
		{
			// Create a new entry, increment its reference count to claim ownership of it.
			pEntry = SEOUL_NEW(MemoryBudgets::Content) Entry(key, p);
			SeoulGlobalIncrementReferenceCount(pEntry.Get());

			// Insert  the entry.
			if (!m_tContent.Insert(key, pEntry).Second)
			{
				SEOUL_ASSERT(Table::Traits::GetNullKey() == key);

				// Allow the nullptr key case to fall through, using the placeholder
				// content in this case.
				Handle<T> pReturn(p.GetPtr());
				SeoulGlobalDecrementReferenceCount(pEntry.Get());
				return pReturn;
			}
		}
		else
		{
			pEntry->AtomicReplace(p);
		}

		return Handle<T>(pEntry);
	}

	/**
	 * @return True if a file is loaded into this Content::Store, false otherwise.
	 */
	virtual Bool IsFileLoaded(FilePath filePath) const SEOUL_OVERRIDE
	{
		KeyType key = FilePathToContentKey<KeyType>(filePath);

		{
			Lock lock(m_Mutex);
			return (m_tContent.HasValue(key));
		}
	}

	/**
	 * Invoke with pFileChangeEvent to indicate that a file on disk
	 * has been modified - uses Content::Traits<T> of the types stored in this
	 * Content::Store to handle the event.
	 *
	 * @return True if the event was handled, false otherwise.
	 */
	virtual Bool FileChange(ChangeEvent* pFileChangeEvent) SEOUL_OVERRIDE
	{
		KeyType key = FilePathToContentKey<KeyType>(pFileChangeEvent->m_New);

		Bool bReturn = false;
		{
			CheckedPtr<Entry> p;
			{
				Lock lock(m_Mutex);

				// If the content doesn't exist in this Content::Store<>, we're done. Can't handle the change.
				if (!m_tContent.GetValue(key, p))
				{
					return false;
				}
			}

			// If the content is already being loaded, ignore the change. Don't want to pile up loads.
			// Handle the event, but do nothing in response to it.
			if (p->IsLoading())
			{
				return true;
			}

			// Let the type specific handling ultimately decide if we're going to handle this change event.
			bReturn = Traits<T>::FileChange(key, Handle<T>(p));
		}

		// Unload to commit any flush operations after the file change event. We need to do this in
		// case the Content::Handle<T>(p) passed to Content::Traits<T>::FileChange() was preventing
		// a flush inside that call.
		if (bReturn && m_bUnloadAll)
		{
			Unload();
		}

		return bReturn;
	}

	/**
	 * Invoked to reload a list of files. If rContentReload contains
	 * an empty input list, reloads all (candidate) loaded files, otherwise
	 * loads all or a subset of the files in the input list. The list of
	 * files for which a load is triggered is output to rContentReload
	 * as an BaseContentEntry pointer to allow the caller to track
	 * reload completion.
	 */
	virtual void Reload(Content::Reload& rContentReload) SEOUL_OVERRIDE
	{
		Vector< SharedPtr<Entry>, MemoryBudgets::Content> vToReload;

		// Gather entries to reload.
		{
			Lock lock(m_Mutex);

			auto const iBegin = m_tContent.Begin();
			auto const iEnd = m_tContent.End();
			for (auto i = iBegin; iEnd != i; ++i)
			{
				// Don't trigger reloads of files that have not been loaded
				// at all yet. These are either dynamic files (that cannot
				// be loaded but were manually set) or have already in progress
				// first-time loads).
				if (i->Second->GetTotalLoadsCount() > 0)
				{
					vToReload.PushBack(SharedPtr<Entry>(i->Second.Get()));
				}
			}
		}

		// Now issue the reloads and append to the output Vector<>.
		UInt32 const uToReload = vToReload.GetSize();
		for (UInt32 i = 0u; i < uToReload; ++i)
		{
			SharedPtr<Entry> p(vToReload[i]);
			Traits<T>::Load(p->GetKey(), Handle<T>(p.GetPtr()));
			rContentReload.m_vReloaded.PushBack(p);
		}
	}

	/**
	 * Attempts to unload all content, only unloads
	 * content with no existing references.
	 *
	 * @return The number of remaining entries.
	 */
	virtual UInt32 Unload() SEOUL_OVERRIDE
	{
		Lock lock(m_Mutex);
		return InsideLockUnload();
	}

	/**
	 * Similar to Unload, but unloads based on a memory threshold and a LRU list. If
	 * the threshold is set to 0u, Unload is disabled completely.
	 *
	 * \pre Must be called on the main thread.
	 *
	 * @return True if a file was unloaded, false otherwise.
	 */
	virtual Bool UnloadLRU() SEOUL_OVERRIDE
	{
		SEOUL_ASSERT(IsMainThread());

		// No unloading if the threshold is 0u.
		if (0u == m_uUnloadLRUThresholdInBytes)
		{
			return false;
		}

		// If m_bOkLRU is true, it means our memory threshold was ok
		// and nothing has changed since, so we don't need to check again.
		if (m_bOkLRU)
		{
			return false;
		}

		// Unload content to meet our threshold, as possible.
		{
			Lock lock(m_Mutex);

			// First iterate over the LRU front-to-back. If we hit
			// the memory threshold, mark that entry as the last entry
			// to unload, and then break.
			UInt32 uTotalMemoryUsageInBytes = 0u;
			Entry* pToUnloadEnd = nullptr;
			Entry* pPrev = nullptr;
			for (Entry* p = m_pLRUHead; nullptr != p; p = p->LRUGetNext())
			{
				SharedPtr<T> pInstance(p->GetPtr());
				if (!pInstance.IsValid())
				{
					continue;
				}

				// Add the memory contribution of this asset.
				uTotalMemoryUsageInBytes += Traits<T>::GetMemoryUsage(pInstance);

				// If memory is now above the threshold, mark this asset as the end of
				// our set to unload, and then break out of the loop
				if (uTotalMemoryUsageInBytes > m_uUnloadLRUThresholdInBytes)
				{
					pToUnloadEnd = pPrev;
					break;
				}

				// Set prev to p and advance.
				pPrev = p;
			}

			// Nothing to do if pToUnloadEnd is nullptr.
			if (nullptr == pToUnloadEnd)
			{
				m_bOkLRU = true;
				return false;
			}

			// Whether we unloaded anything or not.
			Bool bReturn = false;

			// Walk the LRU list back-to-front (oldest to newest), stopping
			// before the pToUnloadEnd node.
			for (Entry* t = m_pLRUTail; pToUnloadEnd != t; )
			{
				// Set p and advance t, in case we remove t.
				Entry* p = t;
				t = t->LRUGetPrev();

				// Skip content that has no loads - this is content set by SetContent(),
				// which, if unloaded, may not be recoverable.
				if (0 == p->GetTotalLoadsCount())
				{
					continue;
				}

				// Cache the key and whether the asset is currently loading.
				KeyType const key = p->GetKey();
				Bool const bIsLoading = p->IsLoading();

				// If we're the only reference to the entry, ask to delete it.
				if (1 == SeoulGlobalGetReferenceCount(p))
				{
					// Don't destroy if still loading.
					if (!bIsLoading &&
						// Let the type specific handling decide if we can unload the content right now.
						Traits<T>::PrepareDelete(key, *p))
					{
						// Remove the entry from our LRU list and destroy it.
						p->LRURemove(&m_pLRUHead, &m_pLRUTail);
						SeoulGlobalDecrementReferenceCount(p);

						// LRU has changed since last check.
						m_bOkLRU = false;

						// Erase the entry from the content table.
						SEOUL_VERIFY(m_tContent.Erase(key));

						// Successfully unloaded some content.
						bReturn = true;
					}
				}
				// Otherwise, move the entry to the head of the LRU, since it
				// is actively in use.
				else
				{
					p->LRUInsert(&m_pLRUHead, &m_pLRUTail);
					m_bOkLRU = false;
				}
			}

			// Done.
			return bReturn;
		}
	}

	/**
	 * @return The current memory threshold that triggers unloading
	 * when a Content::Store<> is configured to unload based on LRU.
	 */
	UInt32 GetUnloadLRUThresholdInBytes() const
	{
		return m_uUnloadLRUThresholdInBytes;
	}

	/**
	 * Update the threshold at which LRU unloading is allowed. Set to 0 or a very
	 * large value to disable unloading against the LRU.
	 */
	void SetUnloadLRUThresholdInBytes(UInt32 uUnloadLRUThresholdInBytes)
	{
		m_uUnloadLRUThresholdInBytes = uUnloadLRUThresholdInBytes;
	}

private:
	UInt32 m_uUnloadLRUThresholdInBytes;
	typedef HashTable<KeyType, CheckedPtr<Entry>, MemoryBudgets::Content> Table;
	Table m_tContent;
	typedef Vector<KeyType, MemoryBudgets::Content> ToErase;
	ToErase m_vToErase;
	Mutex m_Mutex;
	Entry* m_pLRUHead;
	Entry* m_pLRUTail;
	Bool m_bOkLRU;

	UInt32 InsideLockUnload()
	{
		// Setup our scratch.
		m_vToErase.Clear();

		// Iterate.
		{
			auto const iBegin = m_tContent.Begin();
			auto const iEnd = m_tContent.End();
			for (auto i = iBegin; iEnd != i; ++i)
			{
				// We need to check against IsLoading() first, and then never
				// release the entry if IsLoading() is true. This allows loaders
				// to release their reference first, and then decrement the IsLoading()
				// count, without fear of the object being destroyed between those 2 operations.
				Bool const bIsLoading = (i->Second->IsLoading());

				// If we're the only reference to the entry, ask to delete it.
				if (1 == SeoulGlobalGetReferenceCount(i->Second.Get()))
				{
					// Don't destroy if still loading.
					if (!bIsLoading &&
						// Let the type specific handling decide if we can unload the content right now.
						Traits<T>::PrepareDelete(i->First, *(i->Second)))
					{
						// Prepare for removal.
						CheckedPtr<Entry> p = i->Second;
						m_vToErase.PushBack(i->First);

						// Remove the entry from our LRU list and destroy it.
						p->LRURemove(&m_pLRUHead, &m_pLRUTail);
						SeoulGlobalDecrementReferenceCount(p.Get());

						// LRU has changed since last check.
						m_bOkLRU = false;
					}
				}
			}
		}

		// Erase entries tracked.
		for (auto const& e : m_vToErase)
		{
			m_tContent.Erase(e);
		}
		m_vToErase.Clear();

		// Output the remaining size.
		return m_tContent.GetSize();
	}

	SEOUL_DISABLE_COPY(Store);
}; // class Content::Store

} // namespace Seoul::Content

#endif // include guard
