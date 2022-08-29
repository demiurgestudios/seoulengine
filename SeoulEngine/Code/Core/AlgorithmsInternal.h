/**
 * \file AlgorithmsInternal.h
 * \brief Utility of the Algorithms header. Don't include elsewhere.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#ifndef ALGORITHMS_H
#error AlgorithmsInternal.h should only be included by Algorithms.h
#endif

#include "Allocator.h"
#include "Prereqs.h"

namespace Seoul::AlgorithmsInternal
{

//
// CopyMoveUtil
//
template <typename ITER_IN, typename ITER_OUT, typename OUT_TYPE, Bool CAN_MEM_CPY> struct CopyMoveUtil;
template <typename ITER_IN, typename ITER_OUT, typename OUT_TYPE> struct CopyMoveUtil<ITER_IN, ITER_OUT, OUT_TYPE, true>
{
	static inline ITER_OUT Copy(ITER_IN begin, ITER_IN end, ITER_OUT out)
	{
		// Simple, can use memmove. std::copy()
		// allows the copy regions to overlap, as long as begin != out,
		// so we can't use memcpy() here.
		ptrdiff_t const size = (ptrdiff_t)(end - begin);
		if (size > 0)
		{
			return Allocator<OUT_TYPE>::MemMove(out, begin, size) + size;
		}
		else
		{
			return out + size;
		}
	}

	static inline ITER_OUT CopyBackward(ITER_IN begin, ITER_IN end, ITER_OUT outEnd)
	{
		// Simple, can use memmove. std::copy_backward()
		// allows the copy regions to overlap, as long as end != out,
		// so we can't use memcpy() here.
		ptrdiff_t const size = (ptrdiff_t)(end - begin);
		if (size > 0)
		{
			return Allocator<OUT_TYPE>::MemMove(outEnd - size, begin, size);
		}
		else
		{
			return outEnd - size;
		}
	}

	static inline ITER_OUT UninitializedCopy(ITER_IN begin, ITER_IN end, ITER_OUT out)
	{
		// Simple, can use memcpy. By definition, copy to
		// an uninitialized region cannot overlap a source region,
		// so we can use memcpy() safely here.
		ptrdiff_t const size = (ptrdiff_t)(end - begin);
		if (size > 0)
		{
			return Allocator<OUT_TYPE>::MemCpy(out, begin, size) + size;
		}
		else
		{
			return out + size;
		}
	}

	static inline ITER_OUT UninitializedCopyBackward(ITER_IN begin, ITER_IN end, ITER_OUT outEnd)
	{
		// Simple, can use memcpy. By definition, copy to
		// an uninitialized region cannot overlap a source region,
		// so we can use memcpy() safely here.
		ptrdiff_t const size = (ptrdiff_t)(end - begin);
		if (size > 0)
		{
			return Allocator<OUT_TYPE>::MemCpy(outEnd - size, begin, size);
		}
		else
		{
			return outEnd - size;
		}
	}

	static inline ITER_OUT UninitializedMove(ITER_IN begin, ITER_IN end, ITER_OUT out)
	{
		// For POD types, same as copy.
		return UninitializedCopy(begin, end, out);
	}
};
template <typename ITER_IN, typename ITER_OUT, typename OUT_TYPE> struct CopyMoveUtil<ITER_IN, ITER_OUT, OUT_TYPE, false>
{
	static inline ITER_OUT Copy(ITER_IN begin, ITER_IN end, ITER_OUT out)
	{
		// Complex data, copy element by element.
		for (; begin != end; ++out, ++begin)
		{
			*out = *begin;
		}

		return out;
	}

	static inline ITER_OUT CopyBackward(ITER_IN begin, ITER_IN end, ITER_OUT outEnd)
	{
		// Complex data, copy element by element.
		while (begin != end)
		{
			*(--outEnd) = *(--end);
		}

		return outEnd;
	}

	static inline ITER_OUT UninitializedCopy(ITER_IN begin, ITER_IN end, ITER_OUT out)
	{
		// Complex data, in-place construct element by element.
		for (; begin != end; ++out, ++begin)
		{
			::new((void*)out) OUT_TYPE(*begin);
		}

		return out;
	}

	static inline ITER_OUT UninitializedCopyBackward(ITER_IN begin, ITER_IN end, ITER_OUT outEnd)
	{
		// Complex data, in-place construct element by element.
		while (begin != end)
		{
			::new((void*)(--outEnd)) OUT_TYPE(*(--end));
		}

		return outEnd;
	}

	static inline ITER_OUT UninitializedMove(ITER_IN begin, ITER_IN end, ITER_OUT out)
	{
		// Complex data, in-place construct element by element, use move on input
		for (; begin != end; ++out, ++begin)
		{
			::new((void*)out) OUT_TYPE(RvalRef(*begin));
		}

		return out;
	}
};

//
// DestroyUtil
//
template <typename ITER, typename T, Bool CAN_MEM_CPY> struct DestroyUtil;
template <typename ITER, typename T> struct DestroyUtil<ITER, T, true>
{
	static inline void DestroyRange(ITER begin, ITER end)
	{
		// Nop
	}
};
template <typename ITER, typename T> struct DestroyUtil<ITER, T, false>
{
	static inline void DestroyRange(ITER begin, ITER end)
	{
		while (begin != end)
		{
			(--end)->~T();
		}
	}
};

//
// ZeroFillSimpleUtil
//
template <typename ITER, typename T, Bool CAN_ZERO_FILL> struct ZeroFillSimpleUtil;
template <typename ITER, typename T> struct ZeroFillSimpleUtil<ITER, T, true>
{
	static inline void ZeroFill(ITER begin, ITER end)
	{
		(void)Allocator<T>::ClearMemory(begin, (end - begin));
	}
};
template <typename ITER, typename T> struct ZeroFillSimpleUtil<ITER, T, false>
{
	static inline void ZeroFill(ITER begin, ITER end)
	{
		// Nop
	}
};

#if !SEOUL_ASSERTIONS_DISABLED
// Range check - make sure iterators are reasonable.
template <typename ITER>
inline void RangeCheck(ITER begin, ITER end)
{
}

template <typename T>
inline void RangeCheck(T const* begin, T const* end)
{
	SEOUL_ASSERT(begin <= end);
}

#define SEOUL_ALGO_RANGE_CHECK(begin, end) ::Seoul::AlgorithmsInternal::RangeCheck(begin, end)
#else
#define SEOUL_ALGO_RANGE_CHECK(begin, end) ((void)0)
#endif

} // namespace Seoul::AlgorithmsInternal
