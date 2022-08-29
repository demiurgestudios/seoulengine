/**
 * \file Algorithms.h
 * \brief SeoulEngine rough equivalent to the standard <algorithm>
 * header. Includes templates utilities to apply to generic
 * containers and types.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ALGORITHMS_H
#define ALGORITHMS_H

#include "AlgorithmsInternal.h"
#include "Allocator.h"
#include "SeoulAssert.h"
#include "SeoulMath.h"
#include "SeoulTypes.h"
#include <algorithm>

namespace Seoul
{

/**
 * Equivalent to std::copy().
 *
 * Implemented manually due to MSVC iterator checks. std::uninitialized_fill()
 * cannot be applied to unchecked iterators in debug without disabling the
 * global check functionality, which can cause interoperability problems
 * with some of our libraries.
 */
template <typename ITER_IN, typename ITER_OUT>
inline ITER_OUT Copy(ITER_IN begin, ITER_IN end, ITER_OUT out)
{
	using namespace AlgorithmsInternal;

	// Big mess that just resolves to "can we use a memmove
	// for this or not". Conditions are:
	// - simple value.
	// - both iterators are pointers.
	// - in and out value types are the same.
	typedef typename RemoveConstVolatile<typename IteratorTraits<ITER_IN>::ValueType>::Type InValueType;
	typedef typename RemoveConstVolatile<typename IteratorTraits<ITER_OUT>::ValueType>::Type OutValueType;
	typedef CopyMoveUtil< ITER_IN, ITER_OUT, OutValueType,
		CanMemCpy<InValueType>::Value &&
		IsPointer<ITER_IN>::Value &&
		IsPointer<ITER_OUT>::Value &&
		AreSame<InValueType, OutValueType>::Value > Copier;

	// Range validation.
	SEOUL_ALGO_RANGE_CHECK(begin, end);

	// Sanity - overlapping regions are only allowed if !(out >= begin && out < end)
	// (we allow for the exception of (begin == end), since this is
	// a trivial nop).
	SEOUL_ASSERT((begin == end) || (out < begin) || (out >= end));

	return Copier::Copy(begin, end, out);
}

/**
 * Equivalent to std::copy_backward().
 *
 * Implemented manually due to MSVC iterator checks. Some std algorithms
 * cannot be applied to unchecked iterators in debug without disabling the
 * global check functionality, which can cause interoperability problems
 * with some of our libraries.
 */
template <typename ITER_IN, typename ITER_OUT>
inline ITER_OUT CopyBackward(ITER_IN begin, ITER_IN end, ITER_OUT outEnd)
{
	using namespace AlgorithmsInternal;

	// Big mess that just resolves to "can we use a memmove
	// for this or not". Conditions are:
	// - simple value.
	// - both iterators are pointers.
	// - in and out value types are the same.
	typedef typename RemoveConstVolatile<typename IteratorTraits<ITER_IN>::ValueType>::Type InValueType;
	typedef typename RemoveConstVolatile<typename IteratorTraits<ITER_OUT>::ValueType>::Type OutValueType;
	typedef CopyMoveUtil< ITER_IN, ITER_OUT, OutValueType,
		CanMemCpy<InValueType>::Value &&
		IsPointer<ITER_IN>::Value &&
		IsPointer<ITER_OUT>::Value &&
		AreSame<InValueType, OutValueType>::Value > Copier;

	// Range validation.
	SEOUL_ALGO_RANGE_CHECK(begin, end);

	// Sanity - overlapping regions are only allowed if !(outEnd >= begin && outEnd < end)
	// (we allow for the exception of (begin == end), since this is
	// a trivial nop).
	SEOUL_ASSERT((begin == end) || (outEnd < begin) || (outEnd >= end));

	return Copier::CopyBackward(begin, end, outEnd);
}

/**
 * SeoulEngine specific algorithm. Applies ~T() to complex
 * types in a range.
 *
 * If T is simple or a builtin type, this function is a nop.
 */
template <typename ITER>
inline void DestroyRange(ITER begin, ITER end)
{
	using namespace AlgorithmsInternal;
	typedef typename IteratorTraits<ITER>::ValueType ValueType;
	typedef DestroyUtil<ITER, ValueType, CanMemCpy<ValueType>::Value> Destroyer;

	// Range validation.
	SEOUL_ALGO_RANGE_CHECK(begin, end);

	Destroyer::DestroyRange(begin, end);
}

/**
 * Equivalent to std::fill().
 *
 * Implemented manually due to MSVC iterator checks. Some std algorithms
 * cannot be applied to unchecked iterators in debug without disabling the
 * global check functionality, which can cause interoperability problems
 * with some of our libraries.
 */
template <typename ITER, typename T>
inline void Fill(ITER begin, ITER end, const T& val)
{
	// Range validation.
	SEOUL_ALGO_RANGE_CHECK(begin, end);

	for (; begin != end; ++begin)
	{
		*begin = val;
	}
}

/**
 * Equivalent to std::find().
 */
template <typename ITER, typename T>
inline ITER Find(ITER begin, ITER end, const T& val)
{
	// Range validation.
	SEOUL_ALGO_RANGE_CHECK(begin, end);

	return std::find(begin, end, val);
}

/**
 * SeoulEngine specific find() variation,
 * allows for both a value and a predicate
 * (which is expected to be a binary operation
 * that returns true if val and *current
 * are equal).
 */
template <typename ITER, typename T, typename F>
inline ITER Find(ITER begin, ITER end, const T& val, F pred)
{
	// Range validation.
	SEOUL_ALGO_RANGE_CHECK(begin, end);

	for (; begin != end; ++begin)
	{
		if (pred(*begin, val))
		{
			break;
		}
	}

	return begin;
}

/** Equivalent to FindFromBack(), except returns true/false on find or failure. */
template <typename ITER, typename T>
inline Bool Contains(ITER begin, ITER end, const T& val)
{
	// Range validation.
	SEOUL_ALGO_RANGE_CHECK(begin, end);

	return (end != Find(begin, end, val));
}

/**
 * Find the specific value in the sequence and return an iterator
 * pointing to it. Searches from (end - 1) to begin.
 *
 * @param[in] begin start of the sequence to search
 * @param[in] end   end of the sequence to search
 * @param[in] val   value to search for.
 *
 * @return Valid iterator (>= begin && < end) if val was found,
 * or end if val was not found.
 */
template <typename ITER, typename T>
inline ITER FindFromBack(ITER begin, ITER end, const T& val)
{
	// Range validation.
	SEOUL_ALGO_RANGE_CHECK(begin, end);

	ITER origEnd = end;
	while (begin != end)
	{
		if (*(--end) == val)
		{
			return end;
		}
	}

	return origEnd;
}

/**
 * Find the specific value in the sequence and return an iterator
 * pointing to it. Searches from (end - 1) to begin.
 *
 * @param[in] begin start of the sequence to search
 * @param[in] end   end of the sequence to search
 * @param[in] val   value to search for.
 * @param[in] pred  functor to apply to cur and val to determine equality.
 *
 * @return Valid iterator (>= begin && < end) if val was found,
 * or end if val was not found.
 */
template <typename ITER, typename T, typename F>
inline ITER FindFromBack(ITER begin, ITER end, const T& val, F pred)
{
	// Range validation.
	SEOUL_ALGO_RANGE_CHECK(begin, end);

	ITER origEnd = end;
	while (begin != end)
	{
		if (pred(*(--end), val))
		{
			return end;
		}
	}

	return origEnd;
}

/** Equivalent to FindFromBack(), except returns true/false on find or failure. */
template <typename ITER, typename T>
inline Bool ContainsFromBack(ITER begin, ITER end, const T& val)
{
	// Range validation.
	SEOUL_ALGO_RANGE_CHECK(begin, end);

	return (end != FindFromBack(begin, end, val));
}

/**
 * Equivalent to std::find_if().
 */
template <typename ITER, typename F>
inline ITER FindIf(ITER begin, ITER end, F pred)
{
	// Range validation.
	SEOUL_ALGO_RANGE_CHECK(begin, end);

	return std::find_if(begin, end, pred);
}

/**
 * Equivalent to std::lower_bound().
 */
template <typename ITER, typename T>
inline ITER LowerBound(ITER begin, ITER end, const T& val)
{
	// Range validation.
	SEOUL_ALGO_RANGE_CHECK(begin, end);

	return std::lower_bound(begin, end, val);
}

/**
 * Equivalent to std::lower_bound().
 */
template <typename ITER, typename T, typename PRED>
inline ITER LowerBound(ITER begin, ITER end, const T& val, PRED pred)
{
	// Range validation.
	SEOUL_ALGO_RANGE_CHECK(begin, end);

	return std::lower_bound(begin, end, val, pred);
}

/**
 * Equivalent to std::upper_bound().
 */
template <typename ITER, typename T>
inline ITER UpperBound(ITER begin, ITER end, const T& val)
{
	// Range validation.
	SEOUL_ALGO_RANGE_CHECK(begin, end);

	return std::upper_bound(begin, end, val);
}

/**
 * Equivalent to std::lower_bound().
 */
template <typename ITER, typename T, typename PRED>
inline ITER UpperBound(ITER begin, ITER end, const T& val, PRED pred)
{
	// Range validation.
	SEOUL_ALGO_RANGE_CHECK(begin, end);

	return std::upper_bound(begin, end, val, pred);
}

/**
 * Equivalent to std::sort().
 */
template <typename ITER>
inline void QuickSort(ITER begin, ITER end)
{
	// Range validation.
	SEOUL_ALGO_RANGE_CHECK(begin, end);

	std::sort(begin, end);
}

/**
 * Equivalent to std::sort().
 */
template <typename ITER, typename COMP>
inline void QuickSort(ITER begin, ITER end, COMP comp)
{
	// Range validation.
	SEOUL_ALGO_RANGE_CHECK(begin, end);

	std::sort(begin, end, comp);
}

/**
 * Equivalent to std::random_shuffle().
 *
 * NOTE: Implemented inline (vs. calling into std::random_shuffle()) to
 * account for deprecation (in C++14) and eventual removal (in C++17)
 * of std::random_shuffle().
 */
template <typename ITER, typename RANDOM_GEN>
inline void RandomShuffle(ITER begin, ITER end, RANDOM_GEN&& gen)
{
	// Range validation.
	SEOUL_ALGO_RANGE_CHECK(begin, end);

	// Convenience typedef.
	typedef typename std::iterator_traits<ITER>::difference_type DifferenceType;

	// Edge case.
	if (begin == end)
	{
		return;
	}

	DifferenceType iOffsetFrom = 1;
	for (auto from = begin; ++from != end; ++iOffsetFrom)
	{
		// Generate the swap to index.
		DifferenceType const iOffsetTo = (DifferenceType)gen((DifferenceType)(iOffsetFrom + 1));
		// Sanity check that the generator function is returning good values.
		SEOUL_ASSERT(0 <= iOffsetTo && iOffsetTo <= iOffsetFrom);
		// No self swap.
		if (iOffsetTo == iOffsetFrom)
		{
			continue;
		}
		// Swap.
		std::swap(*from, *(begin + iOffsetTo));
	}
}

/**
 * Equivalent to std::random_shuffle().
 *
 * NOTE: Implemented inline (vs. calling into std::random_shuffle()) to
 * account for deprecation (in C++14) and eventual removal (in C++17)
 * of std::random_shuffle().
 */
template <typename ITER>
inline void RandomShuffle(ITER begin, ITER end)
{
	RandomShuffle(begin, end, &GlobalRandom::RandomShuffleGenerator);
}

/**
 * Equivalent to std::reverse().
 */
template <typename ITER>
inline void Reverse(ITER begin, ITER end)
{
	// Range validation.
	SEOUL_ALGO_RANGE_CHECK(begin, end);

	std::reverse(begin, end);
}

/** Single value equivalent to std::rotate(). */
template <typename T>
inline void Rotate(T& rA, T& rB, T& rC)
{
	T t = rC;
	rC = rA;
	rA = rB;
	rB = t;
}

/**
 * Equivalent to std::sort().
 */
template <typename ITER>
inline void Sort(ITER begin, ITER end)
{
	// Range validation.
	SEOUL_ALGO_RANGE_CHECK(begin, end);

	std::sort(begin, end);
}

/**
 * Equivalent to std::sort().
 */
template <typename ITER, typename COMP>
inline void Sort(ITER begin, ITER end, COMP comp)
{
	// Range validation.
	SEOUL_ALGO_RANGE_CHECK(begin, end);

	std::sort(begin, end, comp);
}

/**
 * Equivalent to std::stable_sort().
 */
template <typename ITER>
inline void StableSort(ITER begin, ITER end)
{
	// Range validation.
	SEOUL_ALGO_RANGE_CHECK(begin, end);

	std::stable_sort(begin, end);
}

/**
 * Equivalent to std::stable_sort().
 */
template <typename ITER, typename COMP>
inline void StableSort(ITER begin, ITER end, COMP comp)
{
	// Range validation.
	SEOUL_ALGO_RANGE_CHECK(begin, end);

	std::stable_sort(begin, end, comp);
}

/**
 * Equivalent to std::swap().
 */
template <typename T>
inline void Swap(T& a, T& b)
{
	std::swap(a, b);
}

/**
 * Equivalent to std::swap_ranges().
 *
 * Implemented manually due to MSVC iterator checks. Some std algorithms
 * cannot be applied to unchecked iterators in debug without disabling the
 * global check functionality, which can cause interoperability problems
 * with some of our libraries.
 */
template <typename ITER_IN, typename ITER_OUT>
inline ITER_OUT SwapRanges(ITER_IN begin, ITER_OUT end, ITER_OUT out)
{
	// Range validation.
	SEOUL_ALGO_RANGE_CHECK(begin, end);

	for (; begin != end; ++begin, ++out)
	{
		Swap(*begin, *out);
	}

	return out;
}

/**
 * Equivalent to std::uninitialized_copy().
 *
 * Implemented manually due to MSVC iterator checks. Some std algorithms
 * cannot be applied to unchecked iterators in debug without disabling the
 * global check functionality, which can cause interoperability problems
 * with some of our libraries.
 */
template <typename ITER_IN, typename ITER_OUT>
ITER_OUT UninitializedCopy(ITER_IN begin, ITER_IN end, ITER_OUT out)
{
	using namespace AlgorithmsInternal;

	// Big mess that just resolves to "can we use a memmove
	// for this or not". Conditions are:
	// - simple value.
	// - both iterators are pointers.
	// - in and out value types are the same.
	typedef typename RemoveConstVolatile<typename IteratorTraits<ITER_IN>::ValueType>::Type InValueType;
	typedef typename RemoveConstVolatile<typename IteratorTraits<ITER_OUT>::ValueType>::Type OutValueType;
	typedef CopyMoveUtil< ITER_IN, ITER_OUT, OutValueType,
		CanMemCpy<InValueType>::Value &&
		IsPointer<ITER_IN>::Value &&
		IsPointer<ITER_OUT>::Value &&
		AreSame<InValueType, OutValueType>::Value > Copier;

	// Range validation.
	SEOUL_ALGO_RANGE_CHECK(begin, end);

	// Sanity - overlapping regions are only allowed if !(out >= begin && out < end)
	// (we allow for the exception of (begin == end), since this is
	// a trivial nop).
	SEOUL_ASSERT((begin == end) || (out < begin) || (out >= end));

	return Copier::UninitializedCopy(begin, end, out);
}

/**
 * Equivalent to std::uninitialized_copy_backward().
 *
 * Implemented manually due to MSVC iterator checks. Some std algorithms
 * cannot be applied to unchecked iterators in debug without disabling the
 * global check functionality, which can cause interoperability problems
 * with some of our libraries.
 */
template <typename ITER_IN, typename ITER_OUT>
ITER_OUT UninitializedCopyBackward(ITER_IN begin, ITER_IN end, ITER_OUT outEnd)
{
	using namespace AlgorithmsInternal;

	// Big mess that just resolves to "can we use a memmove
	// for this or not". Conditions are:
	// - simple value.
	// - both iterators are pointers.
	// - in and out value types are the same.
	typedef typename RemoveConstVolatile<typename IteratorTraits<ITER_IN>::ValueType>::Type InValueType;
	typedef typename RemoveConstVolatile<typename IteratorTraits<ITER_OUT>::ValueType>::Type OutValueType;
	typedef CopyMoveUtil< ITER_IN, ITER_OUT, OutValueType,
		CanMemCpy<InValueType>::Value &&
		IsPointer<ITER_IN>::Value &&
		IsPointer<ITER_OUT>::Value &&
		AreSame<InValueType, OutValueType>::Value > Copier;

	// Range validation.
	SEOUL_ALGO_RANGE_CHECK(begin, end);

	// Sanity - overlapping regions are only allowed if !(outEnd >= begin && outEnd < end)
	// (we allow for the exception of (begin == end), since this is
	// a trivial nop).
	SEOUL_ASSERT((begin == end) || (outEnd < begin) || (outEnd >= end));

	return Copier::UninitializedCopyBackward(begin, end, outEnd);
}

/**
 * Equivalent to std::uninitialized_fill().
 *
 * Implemented manually due to MSVC iterator checks. Some std algorithms
 * cannot be applied to unchecked iterators in debug without disabling the
 * global check functionality, which can cause interoperability problems
 * with some of our libraries.
 */
template <typename ITER, typename T>
inline void UninitializedFill(ITER begin, ITER end, const T& val)
{
	// Range validation.
	SEOUL_ALGO_RANGE_CHECK(begin, end);

	for (; begin != end; ++begin)
	{
		::new ((void*)begin) T(val);
	}
}

/**
* Equivalent to std::uninitialized_move().
*
* Implemented manually due to MSVC iterator checks. Some std algorithms
* cannot be applied to unchecked iterators in debug without disabling the
* global check functionality, which can cause interoperability problems
* with some of our libraries.
*/
template <typename ITER_IN, typename ITER_OUT>
ITER_OUT UninitializedMove(ITER_IN begin, ITER_IN end, ITER_OUT out)
{
	using namespace AlgorithmsInternal;

	// Big mess that just resolves to "can we use a memmove
	// for this or not". Conditions are:
	// - simple value.
	// - both iterators are pointers.
	// - in and out value types are the same.
	typedef typename RemoveConstVolatile<typename IteratorTraits<ITER_IN>::ValueType>::Type InValueType;
	typedef typename RemoveConstVolatile<typename IteratorTraits<ITER_OUT>::ValueType>::Type OutValueType;
	typedef CopyMoveUtil< ITER_IN, ITER_OUT, OutValueType,
		CanMemCpy<InValueType>::Value &&
		IsPointer<ITER_IN>::Value &&
		IsPointer<ITER_OUT>::Value &&
		AreSame<InValueType, OutValueType>::Value > Mover;

	// Range validation.
	SEOUL_ALGO_RANGE_CHECK(begin, end);

	// Sanity - overlapping regions are only allowed if !(out >= begin && out < end)
	// (we allow for the exception of (begin == end), since this is
	// a trivial nop).
	SEOUL_ASSERT((begin == end) || (out < begin) || (out >= end));

	return Mover::UninitializedMove(begin, end, out);
}

/**
 * SeoulEngine specific - zero initializes the range iff T is
 * simple - this is useful to conditionally fill memory that
 * would otherwise be undefined (e.g. a member array of a generic
 * type T).
 */
template <typename ITER>
inline void ZeroFillSimple(ITER begin, ITER end)
{
	using namespace AlgorithmsInternal;
	typedef typename IteratorTraits<ITER>::ValueType ValueType;
	typedef ZeroFillSimpleUtil<ITER, ValueType, CanZeroInit<ValueType>::Value> Filler;

	// Range validation.
	SEOUL_ALGO_RANGE_CHECK(begin, end);

	Filler::ZeroFill(begin, end);
}

} // namespace Seoul

#endif // include guard
