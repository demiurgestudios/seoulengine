/**
 * \file VectorInternal.h
 * \brief Private header used by Vector.h. Not to be included outside that context.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#ifndef VECTOR_H
#error VectorInternal.h should only be included by VectorInternal.h
#endif

namespace Seoul::VectorDetail
{

template <typename T, int MEMORY_BUDGETS>
class Buffer
{
public:
	typedef T            ValueType;
	typedef UInt32       SizeType;
	typedef ptrdiff_t    DifferenceType;
	typedef T*           Pointer;
	typedef T const*     ConstPointer;
	typedef T&           Reference;
	typedef T const&     ConstReference;
	typedef T*           Iterator;
	typedef T const*     ConstIterator;

	Buffer();
	template <typename ITER>
	Buffer(ITER begin, ITER end);
	Buffer(SizeType n, ConstReference val);
	~Buffer();

	void Assign(SizeType n, ConstReference val);
	template <typename ITER>
	void Assign(ITER begin, ITER end);

	Reference      At(SizeType n)                         { SEOUL_ASSERT(n < GetSize()); return Begin()[n]; }
	ConstReference At(SizeType n) const                   { SEOUL_ASSERT(n < GetSize()); return Begin()[n]; }

	Reference      Back()                                 { SEOUL_ASSERT(!IsEmpty()); return *(End() - 1); }
	ConstReference Back() const                           { SEOUL_ASSERT(!IsEmpty()); return *(End() - 1); }

	Iterator       Begin()                                { return m_pBegin; }
	ConstIterator  Begin() const                          { return m_pBegin; }

	void           Clear();

	Pointer        Data()                                 { return m_pBegin; }
	ConstPointer   Data() const                           { return m_pBegin; }

	Iterator       End()                                  { return m_pEnd; }
	ConstIterator  End() const                            { return m_pEnd; }

	Iterator       Erase(Iterator begin, Iterator end);

	void           Fill(ConstReference val)               { Seoul::Fill(Begin(), End(), val); }

	Reference      Front()                                { SEOUL_ASSERT(!IsEmpty()); return *Begin(); }
	ConstReference Front() const                          { SEOUL_ASSERT(!IsEmpty()); return *Begin(); }

	Pointer        Get(SizeType n)                        { SEOUL_ASSERT(n < GetSize()); return Begin() + n; }
	ConstPointer   Get(SizeType n) const                  { SEOUL_ASSERT(n < GetSize()); return Begin() + n; }

	SizeType       GetCapacity() const                    { return (SizeType)(m_pEndCapacity - m_pBegin); }
	SizeType       GetCapacityInBytes() const             { return sizeof(T) * GetCapacity(); }

	SizeType       GetSize() const                        { return (SizeType)(m_pEnd - m_pBegin); }
	SizeType       GetSizeInBytes() const                 { return sizeof(T) * GetSize(); }

	Iterator       Insert(Iterator pos, ConstReference val);
	void           Insert(Iterator pos, SizeType n, ConstReference val);
	Iterator       Insert(Iterator pos, ConstIterator begin, ConstIterator end);
	Iterator       Insert(Iterator pos, ValueType&& rval);

	Bool           IsEmpty() const                        { return (m_pEnd == m_pBegin); }

	Reference      operator[](SizeType n)                 { SEOUL_ASSERT(n < GetSize()); return At(n); }
	ConstReference operator[](SizeType n) const           { SEOUL_ASSERT(n < GetSize()); return At(n); }

	void           Reserve(SizeType n);

	void           Resize(SizeType n);
	void           Resize(SizeType n, ConstReference val);

	void           ShrinkToFit();

	void           Swap(Buffer& b);

private:
	T* m_pBegin;
	T* m_pEnd;
	T* m_pEndCapacity;

	SizeType ComputeNewCapacity(SizeType newSize) const;
	void InternalInsert(SizeType offset, SizeType n);

	/* Reserve to at least size n. Applies exponential growth
	 * factor to amortize the cost of incremental growth over time
	 * (for pushback or gradual resize patterns). */
	void ReserveWithAmortizedGrowth(SizeType n)
	{
		Reserve(ComputeNewCapacity(n));
	}

	SEOUL_DISABLE_COPY(Buffer);
}; // class Buffer

// Utilities for buffer management.
template <typename T, int MEMORY_BUDGETS, Bool CAN_MEM_CPY> struct BufferUtil;
template <typename T, int MEMORY_BUDGETS> struct BufferUtil<T, MEMORY_BUDGETS, true>
{
	// Simple, we can just Reallocate.
	static inline void Reallocate(T*& rpBegin, T*& rpEnd, T*& rpEndCapacity, size_t zNewCapacityOfTypeT)
	{
		// Sanity check.
		SEOUL_ASSERT(rpEnd >= rpBegin);

		// Cache the existing number of elements.
		size_t const size = (size_t)(rpEnd - rpBegin);

		// Sanity check.
		SEOUL_ASSERT(zNewCapacityOfTypeT >= size);

		// No new data if 0 == zNewCapacityOfTypeT.
		if (zNewCapacityOfTypeT > 0)
		{
			// Reallocate the buffer.
			rpBegin = Allocator<T>::Reallocate(rpBegin, zNewCapacityOfTypeT, (MemoryBudgets::Type)MEMORY_BUDGETS);

			// Now populate new m_pEnd and m_pEndCapacity.
			rpEnd = rpBegin + size;
			rpEndCapacity = rpBegin + zNewCapacityOfTypeT;
		}
		// Otherwise, deallocate the buffer and set all to nullptr.
		else
		{
			Allocator<T>::Deallocate(rpBegin);

			// Sanity check, Deallocate() should set the pointer to nullptr for us.
			SEOUL_ASSERT(nullptr == rpBegin);

			rpEnd = nullptr;
			rpEndCapacity = nullptr;
		}
	}

	// Simple, we can just use memmove.
	template <typename ITER_IN, typename ITER_OUT>
	static inline ITER_OUT UninitializedMoveBackward(ITER_IN begin, ITER_IN end, ITER_OUT out)
	{
		// Sanity checks.
		SEOUL_ALGO_RANGE_CHECK(begin, end);
		// As with std::move_backward, out cannot be in (begin, end].
		SEOUL_ASSERT(out <= begin || out > end);

		// Simple, can use memmove.
		ptrdiff_t const size = (ptrdiff_t)(end - begin);
		if (size > 0)
		{
			return Allocator<T>::MemMove(out - size, begin, size);
		}
		else
		{
			return out - size;
		}
	}
};
template <typename T, int MEMORY_BUDGETS> struct BufferUtil<T, MEMORY_BUDGETS, false>
{
	// Complex, we need to allocator a new buffer, copy values, then destroy
	// old values.
	static inline void Reallocate(T*& rpBegin, T*& rpEnd, T*& rpEndCapacity, size_t zNewCapacityOfTypeT)
	{
		// Special case optimization/handling - if rpBegin == rpEnd, we can
		// just use the simple handling, since there are no elements to copy/destruct.
		if (rpBegin == rpEnd)
		{
			typedef BufferUtil<T, MEMORY_BUDGETS, true> BufferUtilSimple;
			BufferUtilSimple::Reallocate(rpBegin, rpEnd, rpEndCapacity, zNewCapacityOfTypeT);
			return;
		}

		// Sanity check.
		SEOUL_ASSERT(rpEnd >= rpBegin);

		// Cache the existing number of elements.
		size_t const size = (size_t)(rpEnd - rpBegin);

		// Sanity check.
		SEOUL_ASSERT(zNewCapacityOfTypeT >= size);

		// No new buffer initially.
		T* pNewBuffer = nullptr;

		// No new data if 0 == zNewCapacityOfTypeT.
		if (zNewCapacityOfTypeT > 0)
		{
			// Allocate a new buffer.
			pNewBuffer = Allocator<T>::Allocate(zNewCapacityOfTypeT, (MemoryBudgets::Type)MEMORY_BUDGETS);

			// In place move construct the existing elements.
			(void)UninitializedMove(rpBegin, rpEnd, pNewBuffer);
		}

		// Destroy the moved elements.
		(void)DestroyRange(rpBegin, rpEnd);

		// Deallocate the old buffer.
		Allocator<T>::Deallocate(rpBegin);

		// Now populate with new values.
		rpBegin = pNewBuffer;
		rpEnd = rpBegin + size;
		rpEndCapacity = rpBegin + zNewCapacityOfTypeT;
	}

	// Complex uninitialized move backward - for each element, in place construct
	// the next element and then destroy the previous element.
	template <typename ITER_IN, typename ITER_OUT>
	static inline ITER_OUT UninitializedMoveBackward(ITER_IN begin, ITER_IN end, ITER_OUT out)
	{
		// Sanity checks.
		SEOUL_ALGO_RANGE_CHECK(begin, end);
		// As with std::move_backward, out cannot be in (begin, end].
		SEOUL_ASSERT(out <= begin || out > end);

		// Complex data, in-place construct element by element.
		while (begin != end)
		{
			// Copy construct out.
			::new((void*)(--out)) T(RvalRef(*(--end)));

			// Destroy end.
			end->~T();
		}

		return out;
	}
};

template <typename T, int MEMORY_BUDGETS, Bool CAN_ZERO_FILL> struct BufferZeroInitUtil;
template <typename T, int MEMORY_BUDGETS> struct BufferZeroInitUtil<T, MEMORY_BUDGETS, true>
{
	// Simple, we can just memset().
	template <typename ITER>
	static inline void UninitializedFillDefault(ITER begin, ITER end)
	{
		// Range validation.
		SEOUL_ALGO_RANGE_CHECK(begin, end);

		(void)Allocator<T>::ClearMemory(begin, (end - begin));
	}
};
template <typename T, int MEMORY_BUDGETS> struct BufferZeroInitUtil<T, MEMORY_BUDGETS, false>
{
	// Complex, need to perform in-place new in a loop.
	template <typename ITER>
	static inline void UninitializedFillDefault(ITER begin, ITER end)
	{
		// Range validation.
		SEOUL_ALGO_RANGE_CHECK(begin, end);

		for (; begin != end; ++begin)
		{
			::new ((void*)begin) T;
		}
	}
};

template <typename T, int MEMORY_BUDGETS>
inline Buffer<T, MEMORY_BUDGETS>::Buffer()
	: m_pBegin(nullptr)
	, m_pEnd(nullptr)
	, m_pEndCapacity(nullptr)
{
}

template <typename T, int MEMORY_BUDGETS>
template <typename ITER>
inline Buffer<T, MEMORY_BUDGETS>::Buffer(ITER begin, ITER end)
	: m_pBegin(nullptr)
	, m_pEnd(nullptr)
	, m_pEndCapacity(nullptr)
{
	SEOUL_ASSERT(end >= begin);

	// Compute the size of the buffer after assignment.
	SizeType n = (SizeType)(end - begin);

	// Make sure we have enough space for the new buffer.
	//
	// Do not amortize - std::vector<> behavior after review allows
	// construction to be an exact fit.
	Reserve(n);

	// Now in-place construct copies of the incoming
	// elements, and update end.
	UninitializedCopy(begin, end, m_pBegin);
	m_pEnd = m_pBegin + n;
}

template <typename T, int MEMORY_BUDGETS>
inline Buffer<T, MEMORY_BUDGETS>::Buffer(SizeType n, ConstReference val)
	: m_pBegin(nullptr)
	, m_pEnd(nullptr)
	, m_pEndCapacity(nullptr)
{
	// Make sure we have enough space for the new buffer.
	//
	// Do not amortize - std::vector<> behavior after review allows
	// construction to be an exact fit.
	Reserve(n);

	// Assign() spec states that we should destroy all existing elements,
	// then in-place fill the new elements.
	DestroyRange(m_pBegin, m_pEnd);
	m_pEnd = m_pBegin + n;
	UninitializedFill(m_pBegin, m_pEnd, val);
}

template <typename T, int MEMORY_BUDGETS>
inline Buffer<T, MEMORY_BUDGETS>::~Buffer()
{
	DestroyRange(m_pBegin, m_pEnd);

	m_pEnd = nullptr;
	m_pEndCapacity = nullptr;
	Allocator<T>::Deallocate(m_pBegin);
}

template <typename T, int MEMORY_BUDGETS>
inline void Buffer<T, MEMORY_BUDGETS>::Assign(SizeType n, ConstReference val)
{
	// Handle self assignment, local copy of val.
	auto const tmp(val);

	// Make sure we have enough space for the new buffer.
	ReserveWithAmortizedGrowth(n);

	// Assign() spec states that we should destroy all existing elements,
	// then in-place fill the new elements.
	DestroyRange(m_pBegin, m_pEnd);
	m_pEnd = m_pBegin + n;
	UninitializedFill(m_pBegin, m_pEnd, tmp);
}

template <typename T, int MEMORY_BUDGETS>
template <typename ITER>
inline void Buffer<T, MEMORY_BUDGETS>::Assign(ITER begin, ITER end)
{
	SEOUL_ASSERT(end >= begin);

	// Compute the size of the buffer after assignment.
	SizeType n = (SizeType)(end - begin);

	// Handle self assignment - it's important
	// that we check n > 0 here, because otherwise
	// we can incorrectly treat a case where a separate
	// vector is empty but happens to start at the memory
	// block immediately after "this" buffer.
	if (begin >= m_pBegin && end <= m_pEnd && n > 0)
	{
		// Two possibilities - entire range with
		// a self ssignment is a nop.
		if (begin == m_pBegin && end == m_pEnd)
		{
			// Nop
			return;
		}
		// Otherwise, we must erase the before
		// and after sections.
		else
		{
			//  Remove before and after portions.
			auto const beforeN = (begin - m_pBegin);
			auto const afterN = (m_pEnd - end);
			(void)Erase(m_pBegin, m_pBegin + beforeN);
			(void)Erase(m_pEnd - afterN, m_pEnd);
			return;
		}
	}

	// Assign() spec states that we should destroy all existing elements,
	// then copy construct the new elements.
	Clear();

	// Make sure we have enough space for the new buffer.
	ReserveWithAmortizedGrowth(n);

	// Now in-place construct copies of the incoming
	// elements, and update end.
	UninitializedCopy(begin, end, m_pBegin);
	m_pEnd = m_pBegin + n;
}

template <typename T, int MEMORY_BUDGETS>
inline void Buffer<T, MEMORY_BUDGETS>::Clear()
{
	DestroyRange(m_pBegin, m_pEnd);
	m_pEnd = m_pBegin;
}

template <typename T, int MEMORY_BUDGETS>
inline typename Buffer<T, MEMORY_BUDGETS>::Iterator Buffer<T, MEMORY_BUDGETS>::Erase(Iterator begin, Iterator end)
{
	SEOUL_ASSERT(begin >= m_pBegin && end <= m_pEnd);

	SizeType erased = (SizeType)(end - begin);

	// Early out if not actually erasing anything.
	if (0 == erased)
	{
		return begin;
	}

	// Copy elements at the end over the reigon we're erasing.
	if (end < m_pEnd)
	{
		Copy(end, m_pEnd, begin);
	}

	// Destroy erased elements.
	DestroyRange(m_pEnd - erased, m_pEnd);

	// Update m_pEnd.
	m_pEnd -= erased;

	// Done, always begin, since we don't reallocate here.
	return begin;
}

template <typename T, int MEMORY_BUDGETS>
inline void Buffer<T, MEMORY_BUDGETS>::Insert(Iterator pos, SizeType n, ConstReference val)
{
	SEOUL_ASSERT(pos >= m_pBegin && pos <= m_pEnd);

	// Early out if n is 0.
	if (0 == n)
	{
		return;
	}

	// Handle self assignment, local copy of val.
	auto const tmp(val);

	// Need to store offset to update pos after InternalInsert().
	SizeType offset = (SizeType)(pos - m_pBegin);

	// Common insert impl - reserves space - all we need to do is fill it.
	InternalInsert(offset, n);

	// Recompute pos.
	pos = (m_pBegin + offset);

	// Uninitialized fill the space.
	UninitializedFill(pos, pos + n, tmp);

	// Increase m_pEnd.
	m_pEnd += n;
}

template <typename T, int MEMORY_BUDGETS>
inline typename Buffer<T, MEMORY_BUDGETS>::Iterator Buffer<T, MEMORY_BUDGETS>::Insert(Iterator pos, ConstReference val)
{
	SEOUL_ASSERT(pos >= m_pBegin && pos <= m_pEnd);

	// Handle self assignment, local copy of val.
	auto const tmp(val);

	// Need to store offset to update pos after InternalInsert().
	SizeType offset = (SizeType)(pos - m_pBegin);

	// Common insert impl - reserves space - all we need to do is fill it.
	InternalInsert(offset, 1);

	// Recompute pos.
	pos = (m_pBegin + offset);

	// Uninitialized fill the space.
	UninitializedFill(pos, pos + 1, tmp);

	// Increase m_pEnd.
	++m_pEnd;

	// Return insertion point.
	return pos;
}

template <typename T, int MEMORY_BUDGETS>
inline typename Buffer<T, MEMORY_BUDGETS>::Iterator Buffer<T, MEMORY_BUDGETS>::Insert(Iterator pos, ConstIterator begin, ConstIterator end)
{
	SEOUL_ASSERT(pos >= m_pBegin && pos <= m_pEnd);
	SEOUL_ASSERT(end >= begin);

	SizeType n = (SizeType)(end - begin);

	// Early out if n is 0.
	if (0 == n)
	{
		return pos;
	}

	// Need to store offset to update pos after InternalInsert().
	SizeType offset = (SizeType)(pos - m_pBegin);

	// Common insert impl - reserves space - all we need to do is fill it.
	InternalInsert(offset, n);

	// Recompute pos.
	pos = (m_pBegin + offset);

	// Uninitialized copy forward the new elements.
	UninitializedCopy(begin, end, pos);

	// Increase m_pEnd.
	m_pEnd += n;

	// Return insertion point.
	return pos;
}

template <typename T, int MEMORY_BUDGETS>
inline typename Buffer<T, MEMORY_BUDGETS>::Iterator Buffer<T, MEMORY_BUDGETS>::Insert(Iterator pos, ValueType&& rval)
{
	SEOUL_ASSERT(pos >= m_pBegin && pos <= m_pEnd);

	// Need to store offset to update pos after InternalInsert().
	SizeType offset = (SizeType)(pos - m_pBegin);

	// Common insert impl - reserves space - all we need to do is fill it.
	InternalInsert(offset, 1);

	// Recompute pos.
	pos = (m_pBegin + offset);

	// Uninitialized copy forward the new elements.
	UninitializedMove(&rval, &rval + 1, pos);

	// Increase m_pEnd.
	++m_pEnd;

	// Return insertion point.
	return pos;
}

template <typename T, int MEMORY_BUDGETS>
inline void Buffer<T, MEMORY_BUDGETS>::Reserve(SizeType n)
{
	typedef BufferUtil<T, MEMORY_BUDGETS, CanMemCpy<T>::Value> BuffUtil;

	// TODO: Workaround for an apparent optimizer bug introduced in clang 11
	// and still active through clang 12. We want to apply this workaround, and then
	// recheck the need for it on newer compiler versions.
#	if defined(__clang_major__)
#		if (__clang_major__ <= 10)
			// Nothing to do if we're already at or above the desired capacity.
			if (m_pBegin + n <= m_pEndCapacity)
			{
				return;
			}
#		elif (__clang_major__ <= 12)
			// Nothing to do if we're already at or above the desired capacity.
			if (n <= m_pEndCapacity - m_pBegin)
			{
				return;
			}
#		else
#			error "Check if this workaround is still needed in this version of clang."
#		endif
#	else
	// Nothing to do if we're already at or above the desired capacity.
	if (m_pBegin + n <= m_pEndCapacity)
	{
		return;
	}
#	endif

	// Grow handles increasing the size of the buffer and updating our pointers.
	BuffUtil::Reallocate(m_pBegin, m_pEnd, m_pEndCapacity, (size_t)n);
}

template <typename T, int MEMORY_BUDGETS>
inline void Buffer<T, MEMORY_BUDGETS>::Resize(SizeType n)
{
	typedef BufferZeroInitUtil<T, MEMORY_BUDGETS, CanZeroInit<T>::Value> BufferZeroInitUtil;

	// Smaller or bigger, make sure we have enough space.
	ReserveWithAmortizedGrowth(n);

	// Fill if m_pBegin + n > m_pEnd.
	if (m_pBegin + n > m_pEnd)
	{
		BufferZeroInitUtil::UninitializedFillDefault(m_pEnd, m_pBegin + n);
	}
	// Otherwise, destroy if m_pBegin + n < m_pEnd
	else if (m_pBegin + n < m_pEnd)
	{
		DestroyRange(m_pBegin + n, m_pEnd);
	}

	// Update the size.
	m_pEnd = m_pBegin + n;
}

template <typename T, int MEMORY_BUDGETS>
inline void Buffer<T, MEMORY_BUDGETS>::Resize(SizeType n, ConstReference val)
{
	// Smaller or bigger, make sure we have enough space.
	ReserveWithAmortizedGrowth(n);

	// Fill if m_pBegin + n > m_pEnd.
	if (m_pBegin + n > m_pEnd)
	{
		Seoul::UninitializedFill(m_pEnd, m_pBegin + n, val);
	}
	// Otherwise, destroy if m_pBegin + n < m_pEnd
	else if (m_pBegin + n < m_pEnd)
	{
		DestroyRange(m_pBegin + n, m_pEnd);
	}

	// Update the size.
	m_pEnd = m_pBegin + n;
}

template <typename T, int MEMORY_BUDGETS>
inline void Buffer<T, MEMORY_BUDGETS>::ShrinkToFit()
{
	typedef BufferUtil<T, MEMORY_BUDGETS, CanMemCpy<T>::Value> BuffUtil;

	// Nothing to do if size already equals capacity.
	if (m_pEnd == m_pEndCapacity)
	{
		return;
	}

	// Otherwise, reallocate to size.
	BuffUtil::Reallocate(m_pBegin, m_pEnd, m_pEndCapacity, (m_pEnd - m_pBegin));
}

template <typename T, int MEMORY_BUDGETS>
inline void Buffer<T, MEMORY_BUDGETS>::Swap(Buffer& b)
{
	Seoul::Swap(m_pBegin, b.m_pBegin);
	Seoul::Swap(m_pEnd, b.m_pEnd);
	Seoul::Swap(m_pEndCapacity, b.m_pEndCapacity);
}

template <typename T, int MEMORY_BUDGETS>
inline void Buffer<T, MEMORY_BUDGETS>::InternalInsert(SizeType offset, SizeType n)
{
	typedef BufferUtil<T, MEMORY_BUDGETS, CanMemCpy<T>::Value> BuffUtil;

	// Sanity check - caller should enforce this.
	SEOUL_ASSERT(n > 0);

	SizeType const size = (SizeType)(m_pEnd - m_pBegin);
	SizeType const newSize = (size + n);

	// Make sure we have enough space.
	// Oversize to improve perf. of PushBack()
	// and similar usage patterns (50% beyond target size).
	//
	// Also, since we're trying to match the STL vector whenever
	// it makes sense to (which is almost all the time), we need
	// to grow expontentially to achieve "amortized constant time"
	// of PushBack() and other growth functions that require
	// a size increase.
	ReserveWithAmortizedGrowth(newSize);

	// Unless offset == size, we need to uninitialized move
	// (in place construct a copy and then destroy the original,
	// one-by-one from the back) the range, starting at the offset,
	// to the end of the buffer, to the new end of the buffer.
	//
	// If offset == size, either the buffer is empty, or the insertion
	// point is at the end of the buffer.
	if (size != offset)
	{
		// Uninitialized move from back.
		BuffUtil::UninitializedMoveBackward(m_pBegin + offset, m_pEnd, m_pEnd + n);
	}
}

template <typename T, int MEMORY_BUDGETS>
inline typename Buffer<T, MEMORY_BUDGETS>::SizeType Buffer<T, MEMORY_BUDGETS>::ComputeNewCapacity(SizeType newSize) const
{
	// Compute expontential growth for Insert() and other operations.
	// Expontential growth achieves amortized constant time of PushBack()
	// and other operations, to match the STL vector<> class as much as
	// possible.
	SizeType const oldCapacity = GetCapacity();

	// Early out if already big enough.
	if (newSize <= oldCapacity)
	{
		return oldCapacity;
	}

	// Attempt a 50% increase.
	auto const newCapacity = (oldCapacity + (oldCapacity / 2));
	// Not big enough, just use size.
	if (newCapacity < newSize)
	{
		return newSize;
	}
	// Done.
	return newCapacity;
}

} // namespace Seoul::VectorDetail
