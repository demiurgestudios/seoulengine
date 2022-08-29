/**
 * \file FixedArray.h
 * \brief SeoulEngine equivalent to std::array<>. Identical to std::array<> API, except:
 * - CamelCase function and members names.
 * - additional StaticSize member static constant.
 * - arrays of simple values are zero initialized.
 * - additional FixedArray(ConstReference val), FixedArray(T const aValues[SIZE]) constructors.
 * - FixedArray<T, 0> is not allowed (compile time error).
 * - empty() is called IsEmpty().
 * - size() is called GetSize().
 * - SizeType is always a UInt32.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef	FIXED_ARRAY_H
#define FIXED_ARRAY_H

#include "Algorithms.h"
#include "Allocator.h"

namespace Seoul
{

template <typename T, UInt32 SIZE>
class FixedArray
{
public:
	SEOUL_STATIC_ASSERT_MESSAGE(SIZE > 0, "FixedArray<T, 0> is not allowed.");

	static const UInt32 StaticSize = (UInt32)SIZE;

	typedef T            ValueType;
	typedef UInt32       SizeType;
	typedef ptrdiff_t    DifferenceType;
	typedef T*           Pointer;
	typedef T const*     ConstPointer;
	typedef T&           Reference;
	typedef T const&     ConstReference;
	typedef T*           iterator;
	typedef T*           Iterator;
	typedef T const*     const_iterator;
	typedef T const*     ConstIterator;

	               FixedArray()                    { ZeroFillSimple(Begin(), End()); }
			       FixedArray(const FixedArray& b) { Copy(b.Begin(), b.End(), Begin()); }
	explicit       FixedArray(ConstReference val)  { Seoul::Fill(Begin(), End(), val); }
				   FixedArray(T const b[SIZE])     { Copy(b, b + SIZE, Begin()); }

	Reference      At(SizeType n)                  { SEOUL_ASSERT(n < GetSize()); return m_a[n]; }
	ConstReference At(SizeType n) const            { SEOUL_ASSERT(n < GetSize()); return m_a[n]; }

	Reference      Back()                          { SEOUL_ASSERT(!IsEmpty()); return m_a[SIZE - 1]; }
	ConstReference Back() const                    { SEOUL_ASSERT(!IsEmpty()); return m_a[SIZE - 1]; }

	iterator       begin() /* range-based for loop */       { return Begin(); }
	const_iterator begin() const /* range-based for loop */ { return Begin(); }
	Iterator       Begin()                                  { return &m_a[0]; }
	ConstIterator  Begin() const                            { return &m_a[0]; }

	Pointer        Data()                          { return m_a; }
	ConstPointer   Data() const                    { return m_a; }

	iterator       end() /* range-based for loop */       { return End(); }
	const_iterator end() const /* range-based for loop */ { return End(); }
	Iterator       End()                                  { return m_a + SIZE; }
	ConstIterator  End() const                            { return m_a + SIZE; }

	void           Fill(ConstReference val)        { Seoul::Fill(Begin(), End(), val); }

	Reference      Front()                         { SEOUL_ASSERT(!IsEmpty()); return m_a[0]; }
	ConstReference Front() const                   { SEOUL_ASSERT(!IsEmpty()); return m_a[0]; }

	Pointer        Get(SizeType n)                 { SEOUL_ASSERT(n < GetSize()); return Data() + n; }
	ConstPointer   Get(SizeType n) const           { SEOUL_ASSERT(n < GetSize()); return Data() + n; }

	SizeType       GetSize() const                 { return (SizeType)SIZE; }
	SizeType       GetSizeInBytes() const          { return sizeof(T) * GetSize(); }

	Bool           IsEmpty() const                 { return (0 == SIZE); }

	FixedArray&    operator=(const FixedArray& b);

	Reference      operator[](SizeType n)          { SEOUL_ASSERT(n < GetSize()); return m_a[n]; }
	ConstReference operator[](SizeType n) const    { SEOUL_ASSERT(n < GetSize()); return m_a[n]; }

	void           Swap(FixedArray& b)             { SwapRanges(Begin(), End(), b.Begin()); }

private:
	T m_a[SIZE];
}; // class FixedArray

template <typename T, UInt32 SIZE>
inline FixedArray<T, SIZE>& FixedArray<T, SIZE>::operator=(const FixedArray<T, SIZE>& b)
{
	// Handle self assignment.
	if (this != &b)
	{
		Copy(b.Begin(), b.End(), Begin());
	}

	return *this;
}

/** @return True if FixedArrayA == FixedArrayB, false otherwise. */
template <typename TA, typename TB, UInt32 SIZE>
inline Bool operator==(const FixedArray<TA, SIZE>& vA, const FixedArray<TB, SIZE>& vB)
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

/** @return True if FixedArrayA == FixedArrayB, false otherwise. */
template <typename TA, typename TB, UInt32 SIZE>
inline Bool operator!=(const FixedArray<TA, SIZE>& vA, const FixedArray<TB, SIZE>& vB)
{
	return !(vA == vB);
}

/**
 * Equivalent to std::swap(). Override specifically for FixedArray<>.
 */
template <typename T, UInt32 SIZE>
inline void Swap(FixedArray<T, SIZE>& a, FixedArray<T, SIZE>& b)
{
	a.Swap(b);
}

} // namespace Seoul

#endif // include guard
