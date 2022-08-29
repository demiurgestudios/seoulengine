/**
 * \file DelegateMemberBindHandle.h
 *
 * \brief Utility used by Delegate<> when binding to a member function. Equivalent
 * to Handle<>, except thread-safe and specialized for its use case within Delegate<>
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef DELEGATE_MEMBER_BIND_HANDLE_H
#define DELEGATE_MEMBER_BIND_HANDLE_H

#include "Atomic32.h"
#include "AtomicPointer.h"
#include "FixedArray.h"
#include "HashFunctions.h"
#include "MemoryBarrier.h"
#include "Prereqs.h"
#include "SeoulMath.h"

namespace Seoul
{

/** Maximum number of objects that can be registered at the same time. */
static const UInt32 kDelegateMemberBindHandleTableGlobalArraySize = (1 << 12); // 4096u, must be a power of 2 less than 2^16.

// Sanity check that kDelegateMemberBindHandleTableGlobalArraySize is power of 2.
SEOUL_STATIC_ASSERT((0 == (kDelegateMemberBindHandleTableGlobalArraySize & (kDelegateMemberBindHandleTableGlobalArraySize - 1))));
// Sanity check that kDelegateMemberBindHandleTableGlobalArraySize is less than 2^16.
SEOUL_STATIC_ASSERT(kDelegateMemberBindHandleTableGlobalArraySize < (1 << 16));

/**
 * Handle used to indirectly reference objects that can be bound for member function
 * binds in Delegate<>
 */
struct DelegateMemberBindHandle SEOUL_SEALED
{
	union
	{
		struct
		{
			UInt16 m_uIndex;
			UInt16 m_uGenerationId;
		};
		Atomic32Type m_AtomicValue;
	};

	/**
	 * Attempt to update this DelegateMemberBindHandle to the value
	 * of h atomically.
	 *
	 * @return True if the set succeeded, false otherwise.
	 */
	Bool AtomicSet(DelegateMemberBindHandle h)
	{
		Atomic32Type const existingValue = m_AtomicValue;
		Atomic32Type const newValue = h.m_AtomicValue;
		return (existingValue == Atomic32Common::CompareAndSet(&m_AtomicValue, newValue, existingValue));
	}

	/**
	 * @return True if this DelegateMemberBindHandle potentially references
	 * an object in the global table, false otherwise.
	 */
	Bool IsValid() const
	{
		return (UInt16Max != m_uIndex);
	}

	/**
	 * Reset this DelegateMemberBindHandle to its default state. The default
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
	static void* ToVoidStar(DelegateMemberBindHandle h)
	{
		union
		{
			DelegateMemberBindHandle hIn;
			void* pOut;
		};

		// On platforms with sizeof(void*) > sizeof(DelegateMemberBindHandle),
		// we want the output void* value to be consistent.
		pOut = nullptr;
		hIn = h;
		return pOut;
	}

	/**
	 * @return A DelegateMemberBindHandle from p, where p
	 * was previously defined by a call to ToVoidStar().
	 */
	static DelegateMemberBindHandle ToHandle(void* p)
	{
		union
		{
			void* pIn;
			DelegateMemberBindHandle hOut;
		};

		pIn = p;
		return hOut;
	}
}; // struct DelegateMemberBindHandle

// Sanity check
SEOUL_STATIC_ASSERT(sizeof(DelegateMemberBindHandle) == sizeof(Atomic32Type));

/**
 * Static class, contains global methods for interacting with the
 * global Delegate member function bind handle table.
 *
 * DelegateMemberBindHandleTable is thread-safe, with caveats (see Free())
 *
 * \warning While DelegateMemberBindHandleTable is thread safe, pointers returned
 * by DelegateMemberBindHandleTable::Get() are not locked. As a result, there is
 * no guarantee that the object pointed at by a handle remains defined while the
 * pointer is in use (a thread A can acquire a pointer and a thread B can destroy
 * the object referenced by that pointer while thread A is still using the pointer).
 */
class DelegateMemberBindHandleTable SEOUL_SEALED
{
public:
	/**
	 * Entry in the handle table, defaults to a nullptr pointer
	 * a 0th generation id.
	 */
	struct Entry
	{
		Entry()
			: m_p(nullptr)
			, m_uGenerationID(0)
		{
		}

		void* m_p;
		UInt16 m_uGenerationID;
	};

	typedef FixedArray<Entry, kDelegateMemberBindHandleTableGlobalArraySize> GlobalPool;
	typedef FixedArray<Entry*, kDelegateMemberBindHandleTableGlobalArraySize> GlobalPoolIndirect;

	/**
	 * All data used by the global handle table.
	 */
	struct Data
	{
		Data();

		GlobalPool m_Pool;
		GlobalPoolIndirect m_PoolIndirect;
		Atomic32 m_AllocatedCount;
	};

	/**
	 * @return A handle referencing p stored in the global handle table.
	 */
	static DelegateMemberBindHandle Allocate(void* p)
	{
		// Cache the total array size as a local Int variable.
		static const Int kCapacity = (Int)kDelegateMemberBindHandleTableGlobalArraySize;

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
				DelegateMemberBindHandle ret;
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
	 * to Free() for the same value of rh fomr multiple threads will result
	 * in undefined behavior. In other words, the design of DelegateMemberBindHandleTable
	 * assumes that an object is only destroyed by one thread and the call to Free()
	 * is in sync with the destruction of the object.
	 */
	static void Free(DelegateMemberBindHandle& rh)
	{
		// Reset the input argument and cache the value locally.
		DelegateMemberBindHandle h = rh;
		rh.Reset();

		// This check is both a sanity check (out of range handles) and
		// a check for rh.IsValid() == false (IsValid() returns false if
		// m_uIndex is equal to UInt16Max).
		if (h.m_uIndex >= kDelegateMemberBindHandleTableGlobalArraySize)
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
		pData->m_uGenerationID = (pData->m_uGenerationID + 1) & (UInt16Max);

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
	static void* Get(DelegateMemberBindHandle h)
	{
		// This check is both a sanity check (out of range handles) and
		// a check for rh.IsValid() == false (IsValid() returns false if
		// m_uIndex is equal to UInt16Max).
		if (h.m_uIndex >= kDelegateMemberBindHandleTableGlobalArraySize)
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
	static Data s_Data;

	// Static class
	DelegateMemberBindHandleTable();
	DelegateMemberBindHandleTable(const DelegateMemberBindHandleTable&);
	DelegateMemberBindHandleTable& operator=(const DelegateMemberBindHandleTable&);
}; // class DelegateMemberBindHandleTable

/**
 * An anchor is a member aggregate that must be included in any objects
 * that will be bind target of a SEOUL_BIND_DELEGATE(&Foo::Bar, this) style
 * bind
 *
 * Use SEOUL_DELEGATE_TARGET(class_name) to include an anchor and necessary
 * hooks for default handling
 *
 * If a class is part of a hierarchy, SEOUL_DELEGATE_TARGET() should be used
 * once and only once at the most root point of the hierarchy that will require
 * delegate bind semantics.
 */
class DelegateMemberBindHandleAnchor SEOUL_SEALED
{
public:
	DelegateMemberBindHandleAnchor()
		: m_hHandle()
	{
		// Set the handle to the invalid state - lazy initialization.
		m_hHandle.Reset();
	}

	~DelegateMemberBindHandleAnchor()
	{
		// Free the handle - DelegateMemberBindHandleTable::Free() handles an invalid handle.
		DelegateMemberBindHandleTable::Free(m_hHandle);
	}

	// Copy construct and assignment operator deliberately do not
	// copy the handle, a new instance has a new handle.
	DelegateMemberBindHandleAnchor(const DelegateMemberBindHandleAnchor&)
		: m_hHandle()
	{
		// Set the handle to the invalid state - lazy initialization.
		m_hHandle.Reset();
	}

	DelegateMemberBindHandleAnchor& operator=(const DelegateMemberBindHandleAnchor&)
	{
		// Nop
		return *this;
	}

	/**
	 * @return The handle associate with this and p
	 *
	 * \pre p must be a pointer to the object that contains this
	 * instance of DelegateMemberBindHandleAnchor.
	 */
	DelegateMemberBindHandle GetHandle(void* p)
	{
		// Lazy initialization - acquire a handle if we don't have one yet.
		if (!m_hHandle.IsValid())
		{
			// Allocate the handle.
			DelegateMemberBindHandle hNew = DelegateMemberBindHandleTable::Allocate(p);

			// Atomically update the member handle - if this fails, another thread
			// has already done this, so free the handle we just allocated.
			if (!m_hHandle.AtomicSet(hNew))
			{
				DelegateMemberBindHandleTable::Free(hNew);
			}

			// Make sure the initialization code happens in full before attempt to
			// read the handle.
			SeoulMemoryBarrier();
		}

		// Sanity check that a class hierarchy has not mistakenly included SEOUL_DELEGATE_TARGET()
		// twice - unfortunately, there is no way to detect this at compile time. If that occurs,
		// it's possible for the handle target at m_hHandle to be defined to a non-nullptr pointer which
		// is not equal to p (it points at a class higher in the hierarchy of the object pointed at
		// by p).
		SEOUL_ASSERT(DelegateMemberBindHandleTable::Get(m_hHandle) == p);

		return m_hHandle;
	}

private:
	DelegateMemberBindHandle m_hHandle;
}; // class DelegateMemberBindHandleAnchor

namespace DelegateMemberBindHandleAnchorGlobal
{
	/**
	 * Default implementation of the global function used to get a handle to p, can be specialized
	 * for classes that implement custom handling for this operation.
	 */
	template <typename T>
	DelegateMemberBindHandle GetHandle(T* p)
	{
		SEOUL_ASSERT(nullptr != p);
		return p->m_DelegateMemberBindHandleAnchor.GetHandle((void*)((typename T::DelegateMemberBindHandleAnchorType*)p));
	}

	/**
	 * Default implementation of the global function used to convert a handle to a pointer of type p, can be specialized
	 * for classes that implement custom handling for this operation.
	 */
	template <typename T>
	T* GetPointer(DelegateMemberBindHandle h)
	{
		return (T*)((typename T::DelegateMemberBindHandleAnchorType*)DelegateMemberBindHandleTable::Get(h));
	}

	/**
	 * Utility that must evaluate to True if a type is a viable delegate target.
	 */
	template <typename T>
	struct IsDelegateTarget
	{
	private:
		typedef Byte True[1];
		typedef Byte False[2];

		template <typename U>
		static True& TestForDelegateMemberBindHandleAnchorType(typename U::DelegateMemberBindHandleAnchorType*);

		template <typename U>
		static False& TestForDelegateMemberBindHandleAnchorType(...);

	public:
		static const Bool Value = (sizeof(TestForDelegateMemberBindHandleAnchorType<T>(nullptr)) == sizeof(True));
	}; // struct IsDelegateTarget
} // namespace DelegateMemberBindHandleAnchorGlobal

// Include this in any classes/structs that need SEOUL_BIND_DELEGATE(&Foo::Bar, this) style bind semantics.
//
// Must be in the public: section of the class.
// Must be included ONCE in a class hierarchy. For example, if delegate binding will occur against Foo and Bar, and Bar
// inherits from Foo, this macro should be included in Foo but not Bar.
#define SEOUL_DELEGATE_TARGET(class_name) \
	typedef class_name DelegateMemberBindHandleAnchorType; \
	Seoul::DelegateMemberBindHandleAnchor m_DelegateMemberBindHandleAnchor

} // namespace Seoul

#endif  // include guard
