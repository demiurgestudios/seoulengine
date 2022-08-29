/**
 * \file ContentEntry.h
 * \brief Content::Entry<> is the generic placeholder used by Content::Handle<>
 * to allow an indirect reference to a content type T. If a Content::Handle<>
 * is indirect, it internally stores a pointer to a Content::Entry<>, which
 * then stores the actual SharedPtr<> to the content. This allows
 * the concrete content to be swapped in/out.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef CONTENT_ENTRY_H
#define CONTENT_ENTRY_H

#include "Atomic32.h"
#include "FilePath.h"
#include "MemoryBarrier.h"
#include "SharedPtr.h"

namespace Seoul::Content
{

class EntryBase SEOUL_ABSTRACT
{
public:
	/**
	 * @return True if this Content::EntryBase is actively being loaded,
	 * false otherwise.
	 */
	Bool IsLoading() const
	{
		return (m_nLoaderCount > 0) || m_bPendingFirstLoad;
	}

	/**
	 * Should be called by any content loader that is actively loading
	 * this Content::EntryBase right before it begins loading.
	 */
	void IncrementLoaderCount()
	{
		++m_nLoaderCount;

		// Gate to wait for the first ever load,
		// prevents race between threads (an entry
		// is inserted into the Content::Store and
		// then the store's mutex is unlocked for
		// the actual load handling - if we didn't
		// apply this gate, a secondary thread
		// could early out before this entry's
		// load was even started).
		SeoulMemoryBarrier();
		m_bPendingFirstLoad = false;
	}

	/**
	 * Should be called by any content loader that is actively loading
	 * this Content::EntryBase once it has completed loading.
	 */
	void DecrementLoaderCount()
	{
		--m_nLoaderCount;
		++m_nTotalLoadsCount;
	}

	/**
	 * Mark the load as cancelled.
	 */
	void CancelLoad()
	{
		m_bLoadCancelled = true;
	}

	/**
	 * @return FilePath of the content - may not fully qualifed the content
	 * reference depending on the concrete key type (i.e. ContentKey has
	 * a sub specifier) but is sufficient for many disk operations on the
	 * content.
	 */
	virtual FilePath GetFilePath() const = 0;

	/**
	 * @return The total number of times this Content::EntryBase has been loaded - can
	 * be used to determine if the content has changed or for stat tracking.
	 */
	Atomic32Type GetTotalLoadsCount() const
	{
		return m_nTotalLoadsCount;
	}

	/**
	 * Called on synchronous loads that bypass the loader instance.
	 */
	void OnSynchronousLoad()
	{
		++m_nTotalLoadsCount;

		// Gate to wait for the first ever load,
		// prevents race between threads (an entry
		// is inserted into the Content::Store and
		// then the store's mutex is unlocked for
		// the actual load handling - if we didn't
		// apply this gate, a secondary thread
		// could early out before this entry's
		// load was even started).
		SeoulMemoryBarrier();
		m_bPendingFirstLoad = false;
	}

	/**
	 * @return True if the load was cancelled.
	 */
	Bool WasLoadCancelled() const
	{
		return m_bLoadCancelled;
	}

	/**
	 * Reset the flag that marks the load
	 * as cancelled.
	 */
	void ResetCancelledLoadFlag()
	{
		m_bLoadCancelled = false;
	}

protected:
	EntryBase()
		: m_nGetCount(0)
		, m_nLoaderCount(0)
		, m_nTotalLoadsCount(0)
		, m_bLoadCancelled(false)
		, m_bPendingFirstLoad(true)
	{
	}

	virtual ~EntryBase()
	{
	}

	Atomic32 m_nGetCount;
	Atomic32 m_nLoaderCount;
	Atomic32 m_nTotalLoadsCount;
	Atomic32Value<Bool> m_bLoadCancelled;
	Atomic32Value<Bool> m_bPendingFirstLoad;
	SEOUL_REFERENCE_COUNTED(EntryBase);

private:
	SEOUL_DISABLE_COPY(EntryBase);
}; // class EntryBase

template <typename T, typename KEY_TYPE>
class Entry SEOUL_SEALED : public EntryBase
{
public:
	Entry(const KEY_TYPE& key, const SharedPtr<T>& p)
		: EntryBase()
		, m_pLRUPrev(nullptr)
		, m_pLRUNext(nullptr)
		, m_pObject(p)
		, m_Key(key)
	{
	}

	~Entry()
	{
		// Sanity checks.
		SEOUL_ASSERT(nullptr == m_pLRUPrev);
		SEOUL_ASSERT(nullptr == m_pLRUNext);
	}

	/**
	 * Replace the concrete SharedPtr<T> in this Content::Entry
	 * in a thread-safe manner.
	 */
	void AtomicReplace(const SharedPtr<T>& p)
	{
		// Use Atomic replace to swap in the new value.
		SharedPtr<T> pTemp(m_pObject.AtomicReplace(p.GetPtr()));

		// Wait until m_nGetCount is 0 - once this reaches zero,
		// we know that any threads which were in the process of acquiring
		// the old pointer have done so successfully (or have now acquired
		// the new pointer), which means the atomic counts of the SharedPtr<>s
		// are valid, preventing premature deletion of any of the objects
		// involved in this operation.
		while (0 != m_nGetCount) ;
	}

	// Override of EntryBase::GetFilePath().
	virtual FilePath GetFilePath() const SEOUL_OVERRIDE
	{
		return ContentKeyToFilePath(m_Key);
	}

	/**
	 * @return The unique identifier key associated with this ContentEntry.
	 */
	const KEY_TYPE& GetKey() const
	{
		return m_Key;
	}

	/**
	 * @return The SharedPtr<T> to content referred to
	 * by this ContentEntry.
	 */
	SharedPtr<T> GetPtr()
	{
		// Increment get count - this is used by AtomicReplace to avoid
		// returning (and destroying the "old" pointer) before
		// we have acquired the pointer and incremented its reference count.
		++m_nGetCount;

		// Store the object and increment its reference count.
		SharedPtr<T> pReturn(m_pObject);

		// Decrement the get count.
		--m_nGetCount;

		return pReturn;
	}

	/**
	 * @return The next entry after this Content::Entry in the LRU list.
	 *
	 * Next will produce an entry that is the same age or older than
	 * this entry.
	 */
	Entry* LRUGetNext() const
	{
		return m_pLRUNext;
	}

	/**
	 * @return The previous entry before this Content::Entry in the LRU list.
	 *
	 * Prev will produce an entry that is the same age or newer than
	 * this entry.
	 */
	Entry* LRUGetPrev() const
	{
		return m_pLRUPrev;
	}

	/**
	 * Insert or reinsert this entry into the LRU list defined
	 * by ppLRUHead and ppLRUTail.
	 *
	 * \pre It is the responsibility of the caller to ensure thread safety
	 * of this operation (ppLRUHead and ppLRUTail must be protected by a Mutex).
	 */
	void LRUInsert(Entry** ppLRUHead, Entry** ppLRUTail)
	{
		// Remove this node from its current owning list, if defined.
		LRURemove(ppLRUHead, ppLRUTail);

		// Our next is the head.
		m_pLRUNext = *ppLRUHead;

		// If we have a next, set it prev.
		if (m_pLRUNext)
		{
			m_pLRUNext->m_pLRUPrev = this;
		}

		// We're now the head.
		*ppLRUHead = this;

		// We're also the tail if it's currently null.
		if (nullptr == *ppLRUTail)
		{
			*ppLRUTail = this;
		}
	}

	/**
	 * Remove this entry from ppLRUHead and ppLRUTail
	 *
	 * It is the responsibility of the caller to call LRURemove()
	 * on this entry for its current list, before Inserting() it into
	 * a new list.
	 *
	 * \pre It is the responsibility of the caller to ensure thread safety
	 * of this operation (ppLRUHead and ppLRUTail must be protected by a Mutex).
	 */
	void LRURemove(Entry** ppLRUHead, Entry** ppLRUTail)
	{
		// If we have a next pointer, update its previous pointer.
		if (m_pLRUNext)
		{
			m_pLRUNext->m_pLRUPrev = m_pLRUPrev;
		}

		// If we have a previous pointer, update its next pointer.
		if (m_pLRUPrev)
		{
			m_pLRUPrev->m_pLRUNext = m_pLRUNext;
		}

		// If we're the head, set it to our next.
		if (this == *ppLRUHead)
		{
			*ppLRUHead = m_pLRUNext;
		}

		// If we're the tail, set it to our prev.
		if (this == *ppLRUTail)
		{
			*ppLRUTail = m_pLRUPrev;
		}

		// Clear our list pointers.
		m_pLRUPrev = nullptr;
		m_pLRUNext = nullptr;
	}

private:
	Entry* m_pLRUPrev;
	Entry* m_pLRUNext;
	SharedPtr<T> m_pObject;
	KEY_TYPE m_Key;

	// Body of the SEOUL_REFERENCE_COUNTED_SUBCLASS() macro, explicit here
	// because Content::Entry<> is itself templated.
	friend void Seoul::SeoulGlobalIncrementReferenceCount<Entry>(const Entry* p);
	friend Int32 Seoul::SeoulGlobalGetReferenceCount<Entry>(const Entry* p);
	friend void Seoul::SeoulGlobalDecrementReferenceCount<Entry>(const Entry* p);

	SEOUL_DISABLE_COPY(Entry);
}; // class Content::Entry

} // namespace Seoul::Content

#endif // include guard
