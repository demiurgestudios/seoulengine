/**
 * \file UnsafeBuffer.h
 * \brief A variation of Seoul::Vector<> for usages where
 * default initialization is prohibitive.
 *
 * Unsafe buffer has similar semantics to Vector<>, but specifically
 * does not zero initialize its contents on initialization or clear.
 * An example use case are render buffers, which are typically large
 * enough such that the overhead of zero initialization is significant.
 *
 * Care must be taken when using this data structure. In particular:
 * - only POD or POD-like types (constructors and destructors are never
 *   invoked).
 * - you must initialize a member yourself prior to use. Its default
 *   value is always undefined.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef UNSAFE_BUFFER_H
#define UNSAFE_BUFFER_H

#include "Allocator.h"
#include "Prereqs.h"

namespace Seoul
{

template <typename T, int MEMORY_BUDGETS = MemoryBudgets::TBDContainer>
class UnsafeBuffer SEOUL_SEALED
{
public:
	typedef T            ValueType;
	typedef UInt32       SizeType;
	typedef ptrdiff_t    DifferenceType;
	typedef T*           Pointer;
	typedef T const*     ConstPointer;
	typedef T&           Reference;
	typedef T const&     ConstReference;
	typedef T*           iterator; /* range-based for loop */
	typedef T*           Iterator;
	typedef T const*     const_iterator; /* range-based for loop */
	typedef T const*     ConstIterator;

	// Safety checks - this class cannot be used with complex types (values
	// must be either POD types or "POD-like" types).
	SEOUL_STATIC_ASSERT(CanMemCpy<T>::Value);
	SEOUL_STATIC_ASSERT(CanZeroInit<T>::Value);

	UnsafeBuffer()
		: m_pBegin(nullptr)
		, m_uSize(0u)
		, m_uCapacity(0u)
	{
	}

	UnsafeBuffer(const UnsafeBuffer& b)
		: m_pBegin(nullptr)
		, m_uSize(0u)
		, m_uCapacity(0u)
	{
		Assign(b.Begin(), b.End());
	}

	template <int B_MEMORY_BUDGETS>
	UnsafeBuffer(const UnsafeBuffer<T, B_MEMORY_BUDGETS>& b)
		: m_pBegin(nullptr)
		, m_uSize(0u)
		, m_uCapacity(0u)
	{
		Assign(b.Begin(), b.End());
	}

	template <typename ITER>
	UnsafeBuffer(ITER begin, ITER end)
		: m_pBegin(nullptr)
		, m_uSize(0u)
		, m_uCapacity(0u)
	{
		Assign(begin, end);
	}

	~UnsafeBuffer()
	{
		Allocator<T>::Deallocate(m_pBegin);
	}

	void           Append(const UnsafeBuffer& b)                      { Append(b.Begin(), b.End()); }

	               template <int B_MEMORY_BUDGETS>
	void           Append(const UnsafeBuffer<T, B_MEMORY_BUDGETS>& b) { Append(b.Begin(), b.End()); }

	               template <typename ITER>
	void           Append(ITER begin, ITER end);

	void           Assign(SizeType n, ConstReference val = ValueType());
	               template <typename ITER>
	void           Assign(ITER begin, ITER end);

	Reference      At(SizeType n)               { SEOUL_ASSERT(n < GetSize()); return Begin()[n]; }
	ConstReference At(SizeType n) const         { SEOUL_ASSERT(n < GetSize()); return Begin()[n]; }

	Reference      Back()                       { SEOUL_ASSERT(!IsEmpty()); return *(End() - 1); }
	ConstReference Back() const                 { SEOUL_ASSERT(!IsEmpty()); return *(End() - 1); }

	iterator       begin() /* range-based for loop */       { return Begin(); }
	const_iterator begin() const /* range-based for loop */ { return Begin(); }
	Iterator       Begin()                      { return m_pBegin; }
	ConstIterator  Begin() const                { return m_pBegin; }

	void           Clear()                      { m_uSize = 0u; }

	Pointer        Data()                       { return m_pBegin; }
	ConstPointer   Data() const                 { return m_pBegin; }

	iterator       end() /* range-based for loop */       { return End(); }
	const_iterator end() const /* range-based for loop */ { return End(); }
	Iterator       End()                        { return m_pBegin + m_uSize; }
	ConstIterator  End() const                  { return m_pBegin + m_uSize; }

	void           Fill(ConstReference val)     { Assign(GetSize(), val); }

	Reference      Front()                      { SEOUL_ASSERT(!IsEmpty()); return *Begin(); }
	ConstReference Front() const                { SEOUL_ASSERT(!IsEmpty()); return *Begin(); }

	Pointer        Get(SizeType n)              { SEOUL_ASSERT(n < GetSize()); return Begin() + n; }
	ConstPointer   Get(SizeType n) const        { SEOUL_ASSERT(n < GetSize()); return Begin() + n; }

	SizeType       GetCapacity() const          { return m_uCapacity; }
	SizeType       GetCapacityInBytes() const   { return sizeof(T) * GetCapacity(); }

	SizeType       GetSize() const              { return m_uSize; }
	SizeType       GetSizeInBytes() const       { return sizeof(T) * GetSize(); }
	Bool           IsEmpty() const              { return (0u == m_uSize); }

	UnsafeBuffer&  operator=(const UnsafeBuffer& b)                      { Assign(b.Begin(), b.End()); return *this; }

	               template <int B_MEMORY_BUDGETS>
	UnsafeBuffer&  operator=(const UnsafeBuffer<T, B_MEMORY_BUDGETS>& b) { Assign(b.Begin(), b.End()); return *this; }

	Reference      operator[](SizeType n)         { SEOUL_ASSERT(n < GetSize()); return At(n); }
	ConstReference operator[](SizeType n) const   { SEOUL_ASSERT(n < GetSize()); return At(n); }

	void           PopBack()                      { SEOUL_ASSERT(!IsEmpty()); --m_uSize; }
	void           PushBack(ConstReference val);

	void           Reserve(SizeType n);

	// Uniquely named since this is the most dangerous method.
	// If a caller doesn't realize this will not initialize the new
	// area, the end result could be an assumption of value initialized
	// memory that is actually undefined.
	void           ResizeNoInitialize(SizeType n) { Reserve(n); m_uSize = n; }

	void           ShrinkToFit();

	void           Swap(UnsafeBuffer& b);

private:
	T* m_pBegin;
	SizeType m_uSize;
	SizeType m_uCapacity;
}; // class UnsafeBuffer

template <typename T, int MEMORY_BUDGETS>
template <typename ITER>
inline void UnsafeBuffer<T, MEMORY_BUDGETS>::Append(ITER begin, ITER end)
{
	auto const u = (SizeType)(end - begin);
	auto const uOffset = GetSize();
	
	ResizeNoInitialize(GetSize() + u);
	Allocator<T>::MemCpy(Begin() + uOffset, begin, u);
}

template <typename T, int MEMORY_BUDGETS>
inline void UnsafeBuffer<T, MEMORY_BUDGETS>::Assign(SizeType n, ConstReference val)
{
	Reserve(n);
	m_uSize = n;
	UninitializedFill(m_pBegin, m_pBegin + n, val);
}

template <typename T, int MEMORY_BUDGETS>
template <typename ITER>
inline void UnsafeBuffer<T, MEMORY_BUDGETS>::Assign(ITER begin, ITER end)
{
	auto const u = (SizeType)(end - begin);
	ResizeNoInitialize(u);
	Allocator<T>::MemCpy(m_pBegin, begin, u);
}

template <typename T, int MEMORY_BUDGETS>
inline void UnsafeBuffer<T, MEMORY_BUDGETS>::PushBack(ConstReference val)
{
	// Make sure we have enough space.
	auto const uNewSize = (GetSize() + 1u);
	if (uNewSize > m_uCapacity)
	{
		// Oversize to improve perf. (50% beyond target size).
		m_uCapacity = (uNewSize * 3u) / 2u;
		m_pBegin = Allocator<T>::Reallocate(m_pBegin, m_uCapacity, (MemoryBudgets::Type)MEMORY_BUDGETS);
	}

	// Insert the value and increase size.
	m_pBegin[m_uSize] = val;
	m_uSize = uNewSize;
}

template <typename T, int MEMORY_BUDGETS>
inline void UnsafeBuffer<T, MEMORY_BUDGETS>::Reserve(SizeType n)
{
	if (n <= m_uCapacity)
	{
		return;
	}

	m_pBegin = Allocator<T>::Reallocate(m_pBegin, n, (MemoryBudgets::Type)MEMORY_BUDGETS);
	m_uCapacity = n;
}

template <typename T, int MEMORY_BUDGETS>
inline void UnsafeBuffer<T, MEMORY_BUDGETS>::ShrinkToFit()
{
	// Nothing to do if size already equals capacity.
	if (m_uSize == m_uCapacity)
	{
		return;
	}

	// Otherwise, reallocate to size.
	if (0u == m_uSize)
	{
		Allocator<T>::Deallocate(m_pBegin);
	}
	else
	{
		m_pBegin = Allocator<T>::Reallocate(m_pBegin, m_uSize, (MemoryBudgets::Type)MEMORY_BUDGETS);
	}
	m_uCapacity = m_uSize;
}

template <typename T, int MEMORY_BUDGETS>
inline void UnsafeBuffer<T, MEMORY_BUDGETS>::Swap(UnsafeBuffer& b)
{
	Seoul::Swap(m_pBegin, b.m_pBegin);
	Seoul::Swap(m_uSize, b.m_uSize);
	Seoul::Swap(m_uCapacity, b.m_uCapacity);
}

/** @return True if vA == vB, false otherwise. */
template <typename TA, int MEMORY_BUDGETS_A, typename TB, int MEMORY_BUDGETS_B>
inline Bool operator==(const UnsafeBuffer<TA, MEMORY_BUDGETS_A>& vA, const UnsafeBuffer<TB, MEMORY_BUDGETS_B>& vB)
{
	if (vA.GetSize() != vB.GetSize())
	{
		return false;
	}

	auto const iBeginA = vA.Begin();
	auto const iEndA = vA.End();
	auto const iBeginB = vB.Begin();
	for (auto iA = iBeginA, iB = iBeginB; iEndA != iA; ++iA, ++iB)
	{
		if (*iA != *iB)
		{
			return false;
		}
	}

	return true;
}

/** @return True if vA == vB, false otherwise. */
template <typename TA, int MEMORY_BUDGETS_A, typename TB, int MEMORY_BUDGETS_B>
inline Bool operator!=(const UnsafeBuffer<TA, MEMORY_BUDGETS_A>& vA, const UnsafeBuffer<TB, MEMORY_BUDGETS_B>& vB)
{
	return !(vA == vB);
}

/**
 * Equivalent to std::swap(). Override specifically for UnsafeBuffer<>.
 */
template <typename T, int MEMORY_BUDGETS>
inline void Swap(UnsafeBuffer<T, MEMORY_BUDGETS>& a, UnsafeBuffer<T, MEMORY_BUDGETS>& b)
{
	a.Swap(b);
}

} // namespace Seoul

#endif // include guard
