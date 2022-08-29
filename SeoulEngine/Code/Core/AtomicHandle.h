/**
 * \file AtomicHandle.h
 * \brief Thread-safe equivalent to Seoul::Handle. Likely and eventually,
 * will be fully merged with Handle.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ATOMIC_HANDLE_H
#define ATOMIC_HANDLE_H

#include "Atomic32.h"
#include "AtomicPointer.h"
#include "FixedArray.h"
#include "Prereqs.h"
#include "SeoulMath.h"
namespace Seoul { template <typename T> class AtomicHandleTable; }

namespace Seoul
{

namespace AtomicHandleTableCommon
{

/** Maximum number of objects that can be registered at the same time. */
static const UInt32 kGlobalArraySize = (1 << 12); // 4096u, must be a power of 2 less than 2^16.

// Sanity check that kGlobalArraySize is power of 2.
SEOUL_STATIC_ASSERT((0 == (kGlobalArraySize & (kGlobalArraySize - 1))));
// Sanity check that kGlobalArraySize is less than 2^16.
SEOUL_STATIC_ASSERT(kGlobalArraySize < (1 << 16));

/**
 * Entry in the handle table, defaults to a nullptr pointer
 * a 0th generation id.
 */
struct Entry SEOUL_SEALED
{
	Entry()
		: m_p(nullptr)
		, m_uGenerationID(0)
	{
	}

	void* m_p;
	UInt16 m_uGenerationID;
}; // struct Entry

typedef FixedArray<Entry, kGlobalArraySize> GlobalPool;
typedef FixedArray<Entry*, kGlobalArraySize> GlobalPoolIndirect;

/**
 * All data used by the global handle table.
 */
struct Data SEOUL_SEALED
{
	Data();

	GlobalPool m_Pool;
	GlobalPoolIndirect m_PoolIndirect;
	Atomic32 m_AllocatedCount;
}; // struct Data

} // namespace AtomicHandleTableCommon

/**
 * Handle used to indirectly reference T instances.
 */
template <typename T>
class AtomicHandle SEOUL_SEALED
{
public:
	AtomicHandle()
	{
		Reset();
	}

	AtomicHandle(const AtomicHandle& b)
		: m_AtomicValue(b.m_AtomicValue)
	{
	}

	AtomicHandle& operator=(const AtomicHandle& b)
	{
		m_AtomicValue = b.m_AtomicValue;
		return *this;
	}

	/** Equality of handles. */
	Bool operator==(const AtomicHandle& b) const
	{
		return (m_AtomicValue == b.m_AtomicValue);
	}

	/** Equality of handles. */
	Bool operator!=(const AtomicHandle& b) const
	{
		return !(*this == b);
	}

	/**
	 * Attempt to update this AtomicHandle to the value
	 * of h atomically.
	 *
	 * @return True if the set succeeded, false otherwise.
	 */
	Bool AtomicSet(AtomicHandle h)
	{
		Atomic32Type const existingValue = m_AtomicValue;
		Atomic32Type const newValue = h.m_AtomicValue;
		return (existingValue == Atomic32Common::CompareAndSet(&m_AtomicValue, newValue, existingValue));
	}

	/**
	 * @return True if this AtomicHandle potentially references
	 * an object in the global table, false otherwise.
	 */
	Bool IsInternalValid() const
	{
		return (UInt16Max != m_uIndex);
	}

	/**
	 * Reset this AtomicHandle to its default state. The default
	 * state returns false from calls to IsValid().
	 */
	void Reset()
	{
		m_uIndex = UInt16Max;
		m_uGenerationId = UInt16Max;
	}

	/**
	 * @return h as a void* sized value of data.
	 */
	static void* ToVoidStar(AtomicHandle h)
	{
		union
		{
			Atomic32Type hIn;
			void* pOut;
		};

		pOut = nullptr;
		hIn = h.m_AtomicValue;
		return pOut;
	}

	/**
	 * @return A AtomicHandle from p, where p
	 * was previously defined by a call to ToVoidStar().
	 */
	static AtomicHandle ToHandle(void* p)
	{
		union
		{
			void* pIn;
			Atomic32Type hOut;
		};

		pIn = p;
		AtomicHandle hReturn;
		hReturn.m_AtomicValue = hOut;
		return hReturn;
	}

	/** Read-only access to the internal handle value. */
	Atomic32Type GetAtomicValue() const { return m_AtomicValue; }

private:
	friend class AtomicHandleTable<T>;

	union
	{
		struct
		{
			UInt16 m_uIndex;
			UInt16 m_uGenerationId;
		};
		Atomic32Type m_AtomicValue;
	};
}; // class AtomicHandle

/**
 * Static class, contains global methods for interacting with a
 * global T handle table.
 *
 * AtomicHandleTable is thread-safe, with caveats (see Free())
 *
 * \warning While AtomicHandleTable is thread safe, pointers returned
 * by AtomicHandleTable::Get() are not locked. As a result, there is
 * no guarantee that the object pointed at by a handle remains defined while the
 * pointer is in use (a thread A can acquire a pointer and a thread B can destroy
 * the object referenced by that pointer while thread A is still using the pointer).
 */
template <typename T>
class AtomicHandleTable SEOUL_SEALED
{
public:
	// Sanity check
	SEOUL_STATIC_ASSERT(sizeof(AtomicHandle<T>) == sizeof(Atomic32Type));

	/**
	 * @return A handle referencing p stored in the global handle table.
	 */
	static AtomicHandle<T> Allocate(T* p)
	{
		using namespace AtomicHandleTableCommon;

		// Cache the total array size as a local Int variable.
		static const Int kCapacity = (Int)kGlobalArraySize;

		// Globals
		Data& rData = s_Data;
		GlobalPool& rDirect = rData.m_Pool;
		GlobalPoolIndirect& rIndirect = rData.m_PoolIndirect;

		// Sanity check
		SEOUL_ASSERT(rData.m_AllocatedCount < kCapacity);

		// Hash the pointer to give us a reasonable starting position in the global table.
		UInt32 const uHash = GetHash(p);

		// Initialize and walk the table until we hit a nullptr entry.
		Int iIndex = (Int)uHash;
		while (true)
		{
			// Truncate the index to the size of the table.
			iIndex &= (kCapacity - 1);

			// Sanity check.
			SEOUL_ASSERT(iIndex >= 0 && iIndex < (Int)rIndirect.GetSize());

			// Cache the entry.
			Entry* pData = rDirect.Get(iIndex);

			// If we successfully swap in pData, it means the indirect
			// reference at slot iIndex was nullptr, so we've found an empty
			// slot.
			if (nullptr == AtomicPointerCommon::AtomicSetIfEqual(
				(void**)rIndirect.Get(iIndex),
				pData,
				nullptr))
			{
				// Assign the pointer - the generation ID is up to date from
				// the last call to free.
				pData->m_p = p;

				// Update count
				++rData.m_AllocatedCount;

				// Return the handle.
				AtomicHandle<T> ret;
				ret.m_uGenerationId = pData->m_uGenerationID;
				ret.m_uIndex = (UInt16)iIndex;
				return ret;
			}

			// Move onto the next slot.
			++iIndex;
		}
	}

	/**
	 * Release the slot associate with rh.
	 *
	 * rh will return false from IsValid() after this method returns.
	 * rh does not need to be valid.
	 *
	 * \warning While the global handle table is thread-safe, multiple calls
	 * to Free() for the same value of rh from multiple threads will result
	 * in undefined behavior. In other words, the design of AtomicHandleTable
	 * assumes that an object is only destroyed by one thread and the call to Free()
	 * is in sync with the destruction of the object.
	 */
	static void Free(AtomicHandle<T>& rh)
	{
		using namespace AtomicHandleTableCommon;

		// Reset the input argument and cache the value locally.
		AtomicHandle<T> h = rh;
		rh.Reset();

		// This check is both a sanity check (out of range handles) and
		// a check for rh.IsValid() == false (IsValid() returns false if
		// m_uIndex is equal to UInt16Max).
		if (h.m_uIndex >= kGlobalArraySize)
		{
			return;
		}

		// Cache the indirect pool.
		GlobalPoolIndirect& rIndirect = s_Data.m_PoolIndirect;

		// Get entry pointed to by the indirect pool at the specified index.
		Entry* pData = rIndirect[h.m_uIndex];

		// Sanity check that the Handle being freed was not already freed
		// by another context.
		SEOUL_ASSERT(nullptr != pData && pData->m_uGenerationID == h.m_uGenerationId);

		// Reset the contents of the handle table entry, advancing the generation ID.
		pData->m_p = nullptr;
		pData->m_uGenerationID = (UInt32)((UInt32)(pData->m_uGenerationID + 1) & (UInt32)(UInt16Max));

		// Clear the indirect entry.
		SEOUL_VERIFY(pData == AtomicPointerCommon::AtomicSetIfEqual((void**)rIndirect.Get(h.m_uIndex), nullptr, pData));

		// Update count
		SEOUL_ASSERT(s_Data.m_AllocatedCount > 0);
		--s_Data.m_AllocatedCount;
	}

	/**
	 * @return The pointer associated with h - will return nullptr if h is invalid,
	 * if the generation id in the slot pointed to be h does not match h,
	 * or if the handle table entry was defined with a nullptr object.
	 */
	static void* Get(AtomicHandle<T> h)
	{
		using namespace AtomicHandleTableCommon;

		// This check is both a sanity check (out of range handles) and
		// a check for rh.IsValid() == false (IsValid() returns false if
		// m_uIndex is equal to UInt16Max).
		if (h.m_uIndex >= kGlobalArraySize)
		{
			return nullptr;
		}

		// Locally cache the entry referenced by the index.
		Entry entry = s_Data.m_Pool[h.m_uIndex];

		// If the generation ID is invalid, return nullptr.
		if (entry.m_uGenerationID != h.m_uGenerationId)
		{
			return nullptr;
		}

		// Return a pointer to the object.
		return entry.m_p;
	}

	/**
	 * @return The current number of allocated handles.
	 */
	static Atomic32Type GetAllocatedCount()
	{
		return s_Data.m_AllocatedCount;
	}

private:
	static AtomicHandleTableCommon::Data s_Data;

	// Static class
	AtomicHandleTable();
	AtomicHandleTable(const AtomicHandleTable&);
	AtomicHandleTable& operator=(const AtomicHandleTable&);
}; // class AtomicHandleTable
template <typename T>
AtomicHandleTableCommon::Data AtomicHandleTable<T>::s_Data;

} // namespace Seoul

#endif  // include guard
