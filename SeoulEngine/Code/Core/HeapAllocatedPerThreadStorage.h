/**
 * \file HeapAllocatedPreThreadStorage.h
 *
 * \brief Higher-level wrapper around PerThreadStorage. Provides
 * management of per-thread heap allocated objects, where each
 * thread will have its own instance of the object. Object lifespan
 * correlates to the lifespan of the HeapAllocatedPerThreadStorage object.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef HEAP_ALLOCATED_PER_THREAD_STORAGE_H
#define HEAP_ALLOCATED_PER_THREAD_STORAGE_H

#include "Atomic32.h"
#include "FixedArray.h"
#include "PerThreadStorage.h"
#include "Prereqs.h"
#include "Vector.h"

namespace Seoul
{

/**
 * Higher-level wrapper around PerThreadStorage. Provides
 * management of per-thread heap allocated objects, where each
 * thread will have its own instance of the object. Object lifespan
 * correlates to the lifespan of the HeapAllocatedPerThreadStorage object.
 */
template <typename T, UInt32 CAPACITY, MemoryBudgets::Type MEM_TYPE = MemoryBudgets::Threading>
class HeapAllocatedPerThreadStorage SEOUL_SEALED
{
public:
	static const UInt32 StaticCapacity = CAPACITY;

	typedef FixedArray<T*, CAPACITY> Objects;

	HeapAllocatedPerThreadStorage()
		: m_Objects((T*)nullptr)
		, m_Storage()
		, m_InternalCount(0)
		, m_Count(0)
	{
	}

	~HeapAllocatedPerThreadStorage()
	{
		for (Int i = (Int)m_InternalCount - 1; i >= 0; --i)
		{
			SafeDelete(m_Objects[i]);
		}
	}

	/**
	 * @return The number of unique per thread objects in this HeapAllocatedPerThreadStorage.
	 */
	Atomic32Type GetCount() const
	{
		return m_Count;
	}

	/**
	 * Get a read-write reference to the object stored in this
	 * HeapAllocatedPerThreadStorage() object. If the object has not
	 * yet been instantiated for the current thread, it will be instatied
	 * within this thread.
	 */
	T& Get()
	{
		void* pRawObject = m_Storage.GetPerThreadStorage();
		if (!pRawObject)
		{
			// Increment the internal count - we leave the external count untouched
			// until the object is completely setup, so that client code does not
			// attempt to access an incomplete object.
			Atomic32Type const index = (++m_InternalCount) - 1;

			T* pObject = SEOUL_NEW(MEM_TYPE) T { index };
			pRawObject = pObject;

			m_Objects[index] = pObject;
			m_Storage.SetPerThreadStorage(pRawObject);

			// Object is completely constructed, increment the public count - we
			// spin wait until we're the next slot to allocate - this prevents
			// empty slots, as 2 threads could allocate at the same time,
			// and if the thread with slot 4 finishes before slot 3, the count,
			// would be incremented by the slot 4 thread, even though slot 3
			// isn't assigned yet.
			while (m_Count != index) ;

			(++m_Count);
		}

		SEOUL_ASSERT(pRawObject);
		return *reinterpret_cast<T*>(pRawObject);
	}

	/**
	 * @return The FixedArray<> of all heap-allocated Objects allocated
	 * across all threads.
	 */
	const Objects& GetAllObjects() const
	{
		return m_Objects;
	}

	/** @return CAPACITY used as the maxs size of \a this HeapAllocatedPerThreadStorage. */
	UInt32 GetCapacity() const
	{
		return CAPACITY;
	}

	/**
	 * Equivalent to Get(), but will return nullptr if a previous
	 * call has not been made to Get() on the current thread (the
	 * current thread does not have storage yet allocated).
	 */
	T* TryGet() const
	{
		void* pRawObject = m_Storage.GetPerThreadStorage();
		if (nullptr == pRawObject)
		{
			return nullptr;
		}

		return reinterpret_cast<T*>(pRawObject);
	}

private:
	Objects m_Objects;
	PerThreadStorage m_Storage;
	Atomic32 m_InternalCount;
	Atomic32 m_Count;

	SEOUL_DISABLE_COPY(HeapAllocatedPerThreadStorage);
}; // class HeapAllocatedPerThreadStorage

} // namespace Seoul

#endif // include guard
