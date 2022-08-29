/**
 * \file ContentHandle.h
 * \brief Content::Handle is an indirect wrapper around content resources that
 * must be loaded from persistent media (there is an expected delay to load
 * the data). A Content::Handle can wrap about a value directly (in which case
 * it behaves nearly the same as a SharedPtr<>) or can wrap an indirect
 * value, in which cases it indirectly references a SharedPtr<> to the
 * value, which allows a persistent reference to the data to be maintained
 * (the Content::Handle<>) while the data is loaded and swapped into the SharedPtr<>
 * under the hood.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef CONTENT_HANDLE_H
#define CONTENT_HANDLE_H

#include "Algorithms.h"
#include "CheckedPtr.h"
#include "ContentEntry.h"
#include "ContentTraits.h"

namespace Seoul::Content
{

/**
 * Content::Handle<> can be initialized to either an indirect value, in which
 * case it indirectly references a SharedPtr<> to the desired value, or a direct
 * value, in which case it directly references the desired value.
 */
template <typename T>
class Handle SEOUL_SEALED
{
public:
	typedef typename Traits<T>::KeyType KeyType;
	typedef Entry<T, KeyType> EntryType;

	/**
	 * Mask used to indicate (using the lower 2 bits of the pointer) whether this
	 * is a direct value or an indirect value.
	 */
	static const size_t kFlagsMask = (1 << 0) | (1 << 1);
	static const size_t kIndirectFlag = (1 << 0);

	Handle()
		: m_pDirect(nullptr)
	{
	}

	/**
	 * Initialize this Content::Handle with direct value p.
	 */
	explicit Handle(T* p)
		: m_pDirect(p)
	{
		// Pointer must be 4-byte aligned, minimum.
		SEOUL_ASSERT(0u == ((size_t)m_pDirect % 4u));

		// Increment the reference count - this will check for IsValid()
		// and become a nop if this pointer is nullptr.
		IncrementReferenceCount();
	}

	/**
	 * Initialize this Content::Handle with indirect value p
	 */
	explicit Handle(EntryType* p)
		: m_pIndirect(p)
	{
		// Pointer must be 4-byte aligned, minimum.
		SEOUL_ASSERT(0u == ((size_t)m_pIndirect % 4u));

		// If we have a non-null value, mask the lower 2 bits.
		if (m_pIndirect)
		{
			m_uFlags = (m_uFlags & (~kFlagsMask)) | kIndirectFlag;
		}

		// Increment the reference count - this will check for IsValid()
		// and become a nop if this pointer is nullptr.
		IncrementReferenceCount();
	}

	Handle(const Handle& b)
		: m_pDirect(b.m_pDirect)
	{
		IncrementReferenceCount();
	}

	~Handle()
	{
		DecrementReferenceCount();
	}

	Handle& operator=(const Handle& b)
	{
		DecrementReferenceCount();
		m_pDirect = b.m_pDirect;
		IncrementReferenceCount();

		return *this;
	}

	Bool operator==(const Handle& b) const
	{
		return (m_pDirect == b.m_pDirect);
	}

	Bool operator!=(const Handle& b) const
	{
		return !(*this == b);
	}

	/**
	 * @return A SharedPtr< EntryType > with IsValid() == true
	 * if this Content::Handle<> is indirect, or a SharedPtr<> with
	 * IsValid() == false otherwise.
	 */
	SharedPtr< EntryType > GetContentEntry() const
	{
		if (IsIndirect())
		{
			return SharedPtr< EntryType >(InternalGetIndirectPtr());
		}

		return SharedPtr< EntryType >();
	}

	/**
	 * @return The SharedPtr<> associated with this Content::Handle<>, either
	 * a SharedPtr<> wrapping this Content::Handle<> direct value, or a SharedPtr<>
	 * wrpaper the dereferenced indirect value.
	 */
	SharedPtr<T> GetPtr() const
	{
		// IsIndirect() is only true for non-null internal pointers,
		// because the lower 2 bits are left as 0 for nullptr values (a nullptr
		// is always considered a "direct" value).
		if (IsIndirect())
		{
			return InternalGetIndirectPtr()->GetPtr();
		}
		else
		{
			return SharedPtr<T>(InternalGetDirectPtr());
		}
	}

	/** @return The raw flags value - only particularly useful for comparisons. */
	size_t GetRawValue() const { return m_uFlags; }

	/**
	 * @return The total number of loads of the associated content handle, or 0
	 * if no state is available.
	 */
	Atomic32Type GetTotalLoadsCount() const
	{
		if (IsIndirect())
		{
			return InternalGetIndirectPtr()->GetTotalLoadsCount();
		}

		return 0;
	}

	/**
	 * @return True if this Content::Handle<> indirectly references its value, false otherwise.
	 */
	Bool IsIndirect() const
	{
		return (kIndirectFlag == (m_uFlags & kFlagsMask));
	}

	/**
	 * @return True if this Content::Handle<> directly references its value, false otherwise.
	 */
	Bool IsDirect() const
	{
		return (0u == (m_uFlags & kFlagsMask));
	}

	/**
	 * @return The key associated with this Content::Handle<>, or the default key if
	 * this Content::Handle<> is null or IsDirect().
	 */
	KeyType GetKey() const
	{
		return (IsIndirect() ? InternalGetIndirectPtr()->GetKey() : KeyType());
	}

	/**
	 * @return The reference count to the data referenced by this ContentHandle.
	 */
	Int32 GetReferenceCount() const
	{
		return (IsIndirect()
			? (SeoulGlobalGetReferenceCount(InternalGetIndirectPtr().Get()))
			: (SeoulGlobalGetReferenceCount(InternalGetDirectPtr().Get())));
	}

	/**
	 * @return True if this Content::Handle<> IsIndirect() and the referenced Content::Entry<> is currently
	 * loading.
	 */
	Bool IsLoading() const
	{
		return (IsIndirect() ? InternalGetIndirectPtr()->IsLoading() : false);
	}

	/**
	 * @return True if GetPtr() will return a valid SharedPtr<>, false
	 * otherwise.
	 */
	Bool IsPtrValid() const
	{
		return GetPtr().IsValid();
	}

	/**
	 * @return True if:
	 * - IsDirect() is true and the value pointer is non-null
	 * - IsIndirect() is true and the indirect pointer (pointer to Content::Entry<>) is non-null.
	 */
	Bool IsInternalPtrValid() const
	{
		return (0u != m_uFlags);
	}

	/**
	 * @return True if:
	 * - IsDirect() is true and the reference count of the object is 1.
	 * - IsIndirect() is true and the reference count of the object is 2, since the Content::Store always keeps 1 reference to the object.
	 */
	Bool IsUnique() const
	{
		return (IsIndirect()
			? (2 == SeoulGlobalGetReferenceCount(InternalGetIndirectPtr().Get()))
			: (1 == SeoulGlobalGetReferenceCount(InternalGetDirectPtr().Get())));
	}

	/**
	 * Reset this Content::Handle<> so that IsInternalPtrValid() and IsPtrValid() will return false.
	 */
	void Reset()
	{
		DecrementReferenceCount();
		m_pDirect = nullptr;
	}

	/**
	 * Switch the contents of this Content::Handle with rContentHandle.
	 */
	void Swap(Handle& rContentHandle)
	{
		Seoul::Swap(m_uFlags, rContentHandle.m_uFlags);
	}

private:
	/**
	 * Increment the current indirect or direct reference of this Content::Handle<>,
	 * becomes a nop if IsValid() is false.
	 */
	void IncrementReferenceCount()
	{
		if (IsIndirect())
		{
			SeoulGlobalIncrementReferenceCount(InternalGetIndirectPtr().Get());
		}
		else if (IsInternalPtrValid())
		{
			SeoulGlobalIncrementReferenceCount(InternalGetDirectPtr().Get());
		}
	}

	/**
	 * Decrement the current indirect or direct reference of this Content::Handle<>,
	 * becomes a nop if IsValid() is false.
	 */
	void DecrementReferenceCount()
	{
		if (IsIndirect())
		{
			SeoulGlobalDecrementReferenceCount(InternalGetIndirectPtr().Get());
		}
		else if (IsInternalPtrValid())
		{
			SeoulGlobalDecrementReferenceCount(InternalGetDirectPtr().Get());
		}
	}

	/**
	 * @return A pointer to the indirect value wrapper EntryType contained in this Content::Handle<>.
	 *
	 * \pre this Content::Handle<> is an indirect pointer (IsIndirect() returns true).
	 */
	CheckedPtr< EntryType > InternalGetIndirectPtr() const
	{
		SEOUL_ASSERT(IsIndirect());

		// Convert the flags after masking to a pointer.
		struct Converter
		{
			union
			{
				size_t uIn;
				EntryType* pOut;
			};
		};

		// Sanity check
		SEOUL_STATIC_ASSERT(
			sizeof(Converter) == sizeof(size_t) &&
			sizeof(size_t) == sizeof(EntryType*));

		// Mask the lower 2 bits of the flags to remove the "is indirect" flag, then
		// return the resulting pointer.
		Converter converter;
		converter.uIn = (m_uFlags & (~kFlagsMask));
		return converter.pOut;
	}

	/**
	 * @return A pointer to the direct value T contained in this Content::Handle<>.
	 *
	 * \pre this Content::Handle<> is a direct pointer (IsDirect() returns true).
	 */
	CheckedPtr<T> InternalGetDirectPtr() const
	{
		SEOUL_ASSERT(IsDirect());
		return m_pDirect;
	}

	union
	{
		T* m_pDirect;
		EntryType* m_pIndirect;
		size_t m_uFlags;
	};
}; // class Content::Handle

} // namespace Seoul::Content

#endif // include guard
