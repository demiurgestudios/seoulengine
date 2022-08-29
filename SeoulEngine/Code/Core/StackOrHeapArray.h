/**
 * \file StackOrHeapArray.h
 * \brief Utility structure, uses a fixed size array
 * of zStackArraySize unless the desired size is greater,
 * in which case it allocates a heap area for the buffer.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef STACK_OR_HEAP_ARRAY_H
#define STACK_OR_HEAP_ARRAY_H

#include "Algorithms.h"
#include "Allocator.h"

namespace Seoul
{

// Utility, handles a mix of heap or "stack" allocated array memory
// based on configured thresholds. Serves an alternative to alloca()
// or compiler specific variable C array support.
template <typename T, UInt32 zStackArraySize, int MEMORY_BUDGETS>
class StackOrHeapArray SEOUL_SEALED
{
public:
	typedef T                   ValueType;
	typedef UInt32              SizeType;
	typedef ptrdiff_t           DifferenceType;
	typedef ValueType*          Pointer;
	typedef const ValueType*    ConstPointer;
	typedef ValueType&          Reference;
	typedef const ValueType&    ConstReference;
	typedef Pointer             Iterator;
	typedef Pointer             iterator; // ranged-based for loop
	typedef ConstPointer        ConstIterator;
	typedef ConstPointer        const_iterator; // ranged-based for loop

	StackOrHeapArray(SizeType zArraySize)
		: m_zArraySize(zArraySize)
		, m_pHeapArray(nullptr)
	{
		// If the desired array is bigger than the stack size,
		// allocate an array on the heap.
		if (m_zArraySize > zStackArraySize)
		{
			m_pHeapArray = Allocator<T>::Allocate(m_zArraySize, (MemoryBudgets::Type)MEMORY_BUDGETS);
			UninitializedFill(Begin(), End(), T());
		}
		// Otherwise, just fill the array area.
		else
		{
			ZeroFillSimple(Begin(), End());
		}
	}

	~StackOrHeapArray()
	{
		// Need to cleanup the heap array if it was allocated.
		if (m_zArraySize > zStackArraySize)
		{
			DestroyRange(Begin(), End());
			Allocator<T>::Deallocate(m_pHeapArray);
		}
	}

	Reference      At(SizeType n)                  { SEOUL_ASSERT(n < GetSize()); return Data()[n]; }
	ConstReference At(SizeType n) const            { SEOUL_ASSERT(n < GetSize()); return Data()[n]; }

	Reference      Back()                          { SEOUL_ASSERT(!IsEmpty()); return Data()[GetSize() - 1]; }
	ConstReference Back() const                    { SEOUL_ASSERT(!IsEmpty()); return Data()[GetSize() - 1]; }

	Iterator       Begin()                         { return Data(); }
	ConstIterator  Begin() const                   { return Data(); }
	iterator       begin() /* range-based for loop */       { return Begin(); }
	const_iterator begin() const /* range-based for loop */ { return Begin(); }

	Pointer        Data()                          { return (IsUsingStack() ? m_aStackArray : m_pHeapArray); }
	ConstPointer   Data() const                    { return (IsUsingStack() ? m_aStackArray : m_pHeapArray); }

	Iterator       End()                           { return Data() + GetSize(); }
	ConstIterator  End() const                     { return Data() + GetSize(); }
	iterator       end() /* range-based for loop */       { return End(); }
	const_iterator end() const /* range-based for loop */ { return End(); }

	void           Fill(ConstReference val)        { Seoul::Fill(Begin(), End(), val); }

	Reference      Front()                         { SEOUL_ASSERT(!IsEmpty()); return Data()[0]; }
	ConstReference Front() const                   { SEOUL_ASSERT(!IsEmpty()); return Data()[0]; }

	Pointer        Get(SizeType n)                 { SEOUL_ASSERT(n < GetSize()); return Data() + n; }
	ConstPointer   Get(SizeType n) const           { SEOUL_ASSERT(n < GetSize()); return Data() + n; }

	SizeType       GetSize() const                 { return (SizeType)m_zArraySize; }
	SizeType       GetSizeInBytes() const          { return sizeof(T) * GetSize(); }

	Bool           IsEmpty() const                 { return (0 == GetSize()); }

	Bool           IsUsingStack() const            { return (nullptr == m_pHeapArray); }

	Reference      operator[](SizeType n)          { SEOUL_ASSERT(n < GetSize()); return Data()[n]; }
	ConstReference operator[](SizeType n) const    { SEOUL_ASSERT(n < GetSize()); return Data()[n]; }

	void           Swap(StackOrHeapArray& b)       { SwapRanges(Begin(), End(), b.Begin()); }

private:
	UInt32 const m_zArraySize;
	T m_aStackArray[zStackArraySize];
	T* m_pHeapArray;

	SEOUL_DISABLE_COPY(StackOrHeapArray);
}; // class StackOrHeapArray

/**
 * Equivalent to std::swap(). Override specifically for StackOrHeapArray<>.
 */
template <typename T, UInt32 zStackArraySize, int MEMORY_BUDGETS>
inline void Swap(StackOrHeapArray<T, zStackArraySize, MEMORY_BUDGETS>& a, StackOrHeapArray<T, zStackArraySize, MEMORY_BUDGETS>& b)
{
	a.Swap(b);
}

} // namespace Seoul

#endif // include guard
