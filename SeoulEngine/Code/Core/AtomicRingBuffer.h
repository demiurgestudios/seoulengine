/**
 * \file AtomicRingBuffer.h
 * \brief Implements a ring buffer (or circular buffer) container that is
 * thread safe, when used in a multiple consumer, multiple producer
 * environment.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ATOMIC_RING_BUFFER_H
#define ATOMIC_RING_BUFFER_H

#include "Atomic32.h"
#include "AtomicPointer.h"
#include "MemoryBarrier.h"
#include "Mutex.h"
#include "Prereqs.h"
#include "SeoulTypeTraits.h"

namespace Seoul
{

/**
 * AtomicRingBuffer is a thread safe ring buffer with a fixed capacity.
 * It supports threaded multiple consumers and producers.
 *
 * AtomicRingBuffer can only be specialized on pointer types.
 *
 * nullptr is used as a special value for "no type" - as a result, it is not
 * possible to set nullptr to an AtomicRingBuffer.
 */
template <typename T, int MEMORY_BUDGETS>
class AtomicRingBuffer SEOUL_SEALED
{
public:
	typedef T PointerType;
	typedef UInt32 SizeType;
	typedef typename RemovePointer<T>::Type ValueType;

	SEOUL_STATIC_ASSERT_MESSAGE(IsPointer<PointerType>::Value, "AtomicRingBuffer<> can only be specialized on pointer types.");

	AtomicRingBuffer()
		: m_pBuffer(nullptr)
		, m_PushIndex(0)
		, m_PopIndex(0)
		, m_Count(0)
		, m_Capacity(0)
		, m_Mutex()
	{
	}

	~AtomicRingBuffer()
	{
		MemoryManager::Deallocate(m_pBuffer);
	}

	/**
	 * @return The number of elements in this AtomicRingBuffer.
	 */
	Atomic32Type GetCount() const
	{
		return m_Count;
	}

	/**
	 * @return True if there are no valid objects in this AtomicRingBuffer, false otherwise.
	 */
	Bool IsEmpty() const
	{
		return (0 == m_Count);
	}

	/**
	 * @return The next element of this AtomicRingBuffer, or nullptr
	 * if the buffer is empty. The element is left in the ring buffer.
	 *
	 * In general, Peek() is only a useful method in a multiple
	 * producer, single consumer usage scenario. Peek() is unsafe if there
	 * are multiple consumers, as the AtomicRingBuffer cannot ensure
	 * that the Peek()ed element is not Pop()ed by a second consumer
	 * after it has been Peek()ed by the first consumer.
	 */
	PointerType Peek() const
	{
		// Early out.
		if (0 == m_Capacity)
		{
			return nullptr;
		}

		// Lock access - this is a specialized lock
		// which also handles resizing the ring buffer
		// on a pending push.
		LockImpl lock(*this, false);

		// Cache the current capacity - must be performed after
		// the lock when we depend on the value.
		SizeType const kSize = (SizeType)m_Capacity;

		// Early out.
		if (0 == kSize)
		{
			return nullptr;
		}

		// Get the next pop index and the associated value.
		SizeType const popIndex = ((SizeType)(m_PopIndex)) & (kSize - 1);
		T value = m_pBuffer[popIndex];

		return value;
	}

	/**
	 * Pops the next element of this AtomicRingBuffer.
	 *
	 * @return nullptr if the next element of this AtomicRingBuffer
	 * was nullptr, or a valid value if the next element was non-null.
	 */
	PointerType Pop()
	{
		// Early out.
		if (0 == m_Capacity)
		{
			return nullptr;
		}

		// Lock access - this is a specialized lock
		// which also handles resizing the ring buffer
		// on a pending push.
		LockImpl lock(*this, false);

		// Cache the current capacity - must be performed after
		// the lock when we depend on the value.
		SizeType const kSize = (SizeType)m_Capacity;

		// Early out.
		if (0 == kSize)
		{
			return nullptr;
		}

		// Get the pop raw value and index.
		SizeType const popValue = (SizeType)m_PopIndex;
		SizeType const popIndex = (popValue & (kSize - 1));

		// Check - if successful, reduce the count, increment
		// the pop index and return the value.
		PointerType value = m_pBuffer[popIndex];
		if (nullptr != value)
		{
			// Conditional success - another consumer may have already
			// processed this value.
			if (value == AtomicPointerCommon::AtomicSetIfEqual(
				(void**)(m_pBuffer + popIndex),
				(void*)nullptr,
				(void*)value))
			{
				// Count acts as a gate, so set it last.
				++m_PopIndex;
				SeoulMemoryBarrier();
				--m_Count;

				return value;
			}
		}

		// Otherwise, return nullptr for no value.
		return nullptr;
	}

	/**
	 * Push value onto this AtomicRingBuffer.
	 *
	 * \pre value must be non-null.
	 */
	void Push(PointerType value)
	{
		SEOUL_ASSERT(value != nullptr);

		// Lock access - this is a specialized lock
		// which also handles resizing the ring buffer
		// on a pending push.
		LockImpl lock(*this, true);

		// Cache the current capacity - must be performed after
		// the lock when we depend on the value.
		SizeType const kSize = (SizeType)m_Capacity;

		// Once we get into this section, the push must alays succeed, since
		// the lock operation reserves enough space for this push before it
		// completes. The AtomicSetIfEqual here is for barrier and
		// verification purposes.
		SizeType const pushValue = (++m_PushIndex) - 1;
		SizeType const pushIndex = (pushValue & (kSize - 1));

		SEOUL_VERIFY(nullptr == AtomicPointerCommon::AtomicSetIfEqual(
			(void**)(m_pBuffer + pushIndex),
			(void*)value,
			(void*)nullptr));
	}

private:
	PointerType* m_pBuffer;
	Atomic32 m_PushIndex;
	Atomic32 m_PopIndex;
	Atomic32 m_Count;
	Atomic32 m_Capacity;
	Mutex m_Mutex;

	/**
	 * Called from LockImpl() - handles exclusivity operations on Peek(), Pop(), and Push()
	 * operations.
	 */
	void DoLock(Bool bPush)
	{
		// Synchronize.
		m_Mutex.Lock();

		// If this is a lock for a push, increment m_Count and check
		// for out-of-space.
		if (bPush)
		{
			Atomic32Type const nCount = ++m_Count;

			// Lock and resize if it appears to be necessary.
			if (nCount > m_Capacity)
			{
				// Must wait before performing the buffer resize.
				SeoulMemoryBarrier();

				// Cache the existing capacity and compute the new one (always power of 2,
				// big enough to contain the pending count).
				SizeType const oldCapacity = (SizeType)m_Capacity;
				SizeType const newCapacity = (SizeType)GetNextPowerOf2((SizeType)nCount);

				// Sanity check - newCapacity must be larger than oldCapacity.
				SEOUL_ASSERT(newCapacity > oldCapacity);

				// Refresh capacity and the buffer.
				m_Capacity.Set((Atomic32Type)newCapacity);
				m_pBuffer = (PointerType*)MemoryManager::ReallocateAligned(
					m_pBuffer,
					sizeof(PointerType) * newCapacity,
					SEOUL_ALIGN_OF(PointerType),
					(MemoryBudgets::Type)MEMORY_BUDGETS);

				// Update the buffer layout to be linear - this moves the pop index such
				// that it is now always 0, and places the push index at the end of the
				// existing contiguous block of data.
				SizeType const popIndex = ((SizeType)m_PopIndex & (oldCapacity - 1));

				// Sanity checks - oldCapacity + popIndex must be <= newCapacity, since
				// we're already increasing capacity to the next largest power of 2.
				SEOUL_ASSERT(oldCapacity + popIndex <= newCapacity);

				// Copy the first half of the data, before the pop index, to the end,
				// and then shift the entire block up, so the data is now a standard linear
				// buffer from 0 to m_Count.
				memcpy(m_pBuffer + oldCapacity, m_pBuffer, popIndex * sizeof(PointerType));
				memmove(m_pBuffer, m_pBuffer + popIndex, oldCapacity * sizeof(PointerType));

				// Zero initialize the new area.
				memset(m_pBuffer + oldCapacity, 0, (newCapacity - oldCapacity) * sizeof(PointerType));

				// Refresh push and pop indices. Pop is always 0, push is at
				// the end of the existing count of values (minus 1, since count
				// has been increased once already for the current thread, which has
				// not actually inserted its data yet).
				m_PopIndex.Reset();
				m_PushIndex.Set(m_Count - 1);

				// Sanity checks - push index should be within the new capacity and
				// must reference a nullptr element.
				SEOUL_ASSERT((SizeType)m_PushIndex < newCapacity && nullptr == m_pBuffer[m_PushIndex]);
			}
		}
	}

	/**
	 * Called from ~LockImpl() - handles exclusivity operations on Peek(), Pop(), and Push()
	 * operations.
	 */
	void DoUnlock()
	{
		// Synchronize.
		m_Mutex.Unlock();
	}

	/**
	 * Special Lock implementation that handles buffer growth
	 * for push operations in addition to synchronization.
	 */
	class LockImpl SEOUL_SEALED
	{
	public:
		LockImpl(const AtomicRingBuffer& r, Bool bPush)
			: m_r(const_cast<AtomicRingBuffer&>(r))
		{
			m_r.DoLock(bPush);
		}

		~LockImpl()
		{
			m_r.DoUnlock();
		}

	private:
		AtomicRingBuffer& m_r;

		SEOUL_DISABLE_COPY(LockImpl);
	}; // class LockImpl

	SEOUL_DISABLE_COPY(AtomicRingBuffer);
}; // class AtomicRingBuffer

} // namespace Seoul

#endif // include guard
