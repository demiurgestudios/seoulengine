/**
 * \file AtomicPointer.h
 * \brief Implements a wrapper around generic weak pointers
 * that supports atomic operations.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ATOMIC_POINTER_H
#define ATOMIC_POINTER_H

#include "Prereqs.h"

	// Avoid dependency on the Windows.h header.
#if SEOUL_PLATFORM_WINDOWS
#	include <intrin.h>
	extern "C"
	{
		long _InterlockedCompareExchange(long volatile*, long, long);
#		pragma intrinsic(_InterlockedCompareExchange)

#		if SEOUL_PLATFORM_64
			void* _InterlockedCompareExchangePointer(void* volatile*, void*, void*);
#			pragma intrinsic(_InterlockedCompareExchangePointer)
#		endif
	}
#endif // /#if SEOUL_PLATFORM_WINDOWS

namespace Seoul
{

namespace AtomicPointerCommon
{
	/** Guaranteed atomic get of the pointer from ppTarget. */
	static inline void* AtomicGet(void** ppTarget)
	{
#if SEOUL_PLATFORM_WINDOWS
		return static_cast<void* const volatile&>(*ppTarget);
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
		return __atomic_load_n(ppTarget, __ATOMIC_SEQ_CST);
#else
#		error Implement for this platform.
#endif
	}

	/** Guaranteed atomic set of the pointer at ppTarget to the new value pNewValue. */
	static inline void AtomicSet(void** ppTarget, void* pNewValue)
	{
#if SEOUL_PLATFORM_WINDOWS
#		if SEOUL_PLATFORM_64
			(void)::_InterlockedExchange64(
				(__int64*)(ppTarget),
				(__int64)(pNewValue));
#		else
			(void)::_InterlockedExchange(
				(long*)(ppTarget),
				(long)(pNewValue));
#		endif
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
		__atomic_store_n(ppTarget, pNewValue, __ATOMIC_SEQ_CST);
#else
#		error Implement for this platform.
#endif
	}

	/**
	 * Set the value at ppTarget to pNewValue, if its existing is atomicly equal
	 * to pExpectedValue at the moment of the set attempt. Returns the
	 * existing value of ppTarget the moment of the set attempt.
	 */
	static inline void* AtomicSetIfEqual(void** ppTarget, void* pNewValue, void* pExpectedValue)
	{
#if SEOUL_PLATFORM_WINDOWS
#		if SEOUL_PLATFORM_64
			return ::_InterlockedCompareExchangePointer(
				ppTarget,
				pNewValue,
				pExpectedValue);
#		else
			return(
				(void*)(long*)_InterlockedCompareExchange(
					(long volatile *)ppTarget,
					(long)(long*)pNewValue,
					(long)(long*)pExpectedValue));
#		endif
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
		(void)__atomic_compare_exchange_n(
			ppTarget,
			&pExpectedValue,
			pNewValue,
			false,
			__ATOMIC_SEQ_CST,
			__ATOMIC_SEQ_CST);
		return pExpectedValue;
#else
#error Implement for this platform.
#endif
	}
}

template <typename T>
class AtomicPointer SEOUL_SEALED
{
public:
	AtomicPointer()
		: m_P(nullptr)
	{
	}

	/**
	 * Copy constructor, constructs this AtomicPointer to p using an atomic set operation.
	 */
	AtomicPointer(const AtomicPointer& b)
		: m_P(nullptr)
	{
		*this = b;
	}

	/**
	 * Constructor, constructs this AtomicPointer to p using an atomic set operation.
	 */
	explicit AtomicPointer(T* p)
		: m_P(nullptr)
	{
		Set(p);
	}

	/**
	 * Assignment operator, uses an atomic set operation.
	 */
	AtomicPointer& operator=(const AtomicPointer& b)
	{
		Set(b.m_P);
		return *this;
	}

	/**
	 * Implicit cast of this AtomicPointer to/from its underlying pointer type - no operations
	 * through this operator are guaranteed atomic.
	 */
	operator T*() const
	{
		return (T*)AtomicPointerCommon::AtomicGet((void**)&m_P);
	}

	/**
	 * Attempt to set this AtomicPointer to newValue, only if
	 * the current value of this AtomicPointer is atomicly equal to expectedCurrentValue
	 * at the moment of the set attempt.
	 *
	 * @return The current value of this AtomicPointer prior to the set attempt.
	 */
	T* CompareAndSet(T* newValue, T* expectedCurrentValue)
	{
		return (T*)AtomicPointerCommon::AtomicSetIfEqual(
			(void**)&m_P,
			(void*)newValue,
			(void*)expectedCurrentValue);
	}

	/**
	 * Set this AtomicPointer to p using an atomic set operation.
	 */
	void Set(T* p)
	{
		AtomicPointerCommon::AtomicSet(
			(void**)&m_P,
			p);
	}

	/**
	 * Set this AtomicPointer to nullptr using an atomic set operation.
	 */
	void Reset()
	{
		Set(nullptr);
	}

private:
	T* m_P;
}; // class AtomicPointer

} // namespace Seoul

#endif // include guard
