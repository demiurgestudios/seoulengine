/**
 * \file Atomic32.h
 * \brief Provides a thread-safe 32-bit integer value. Can be used
 * for thread-safe reference counts, flags, etc.
 *
 * \warning The sign of the fundamental type used by Atomic32
 * varies per platform. For guaranteed platform independent behavior,
 * assume Atomic32 can only store values in the range [0, 2^31 - 1].
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ATOMIC_32_H
#define ATOMIC_32_H

#include "Prereqs.h"
#include "SeoulMath.h"

	// Avoid dependency on the Windows.h header.
#if SEOUL_PLATFORM_WINDOWS
#	include <intrin.h>
	extern "C"
	{
		long _InterlockedCompareExchange(long volatile*, long, long);
#		pragma intrinsic(_InterlockedCompareExchange)

		long _InterlockedExchange(long volatile*, long);
#		pragma intrinsic(_InterlockedExchange)

		long _InterlockedExchangeAdd(long volatile*, long);
#		pragma intrinsic(_InterlockedExchangeAdd)

		long _InterlockedIncrement(long volatile*);
#		pragma intrinsic(_InterlockedIncrement)

		long _InterlockedDecrement(long volatile*);
#		pragma intrinsic(_InterlockedDecrement)
	}
#endif // /#if SEOUL_PLATFORM_WINDOWS

namespace Seoul
{

namespace Atomic32Common
{
	/*
	 * Attempt to set this Atomic32Type to newValue,
	 * if this Atomic32Type is atomicly equal to expectedCurrentValue
	 * at the time of the attempted set. If not equal, no set occurs.
	 *
	 * @return The original value of this Atomic32Type before the set
	 * attempt - will still be equal to this value after the set attempt if
	 * the current value is not equal to expectedCurrentValue, otherwise
	 * this Atomic32Type will be equal to newValue after the set.
	 */
	static inline Atomic32Type CompareAndSet(Atomic32Type volatile* pValue, Atomic32Type newValue, Atomic32Type expectedCurrentValue)
	{
#if SEOUL_PLATFORM_WINDOWS
		return ::_InterlockedCompareExchange(
			pValue,
			newValue,
			expectedCurrentValue);
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
		(void)__atomic_compare_exchange_n(
			pValue,
			&expectedCurrentValue,
			newValue,
			false,
			__ATOMIC_SEQ_CST,
			__ATOMIC_SEQ_CST);
		return expectedCurrentValue;
#else
#		error Implement for this platform.
#endif
	}
}

/**
 * Provides thread-safe 32-bit integer.
 *
 * \warning The sign of Atomic32 varies per-platform. For safe, platform
 * independent usage, assume Atomic32 can store the positive range of
 * a signed 32-bit integer [0, 2^31 - 1].
 *
 * \sa boost::atomic_count
 */
class Atomic32 SEOUL_SEALED
{
public:
	Atomic32()
	{
		Set(0);
	}

	Atomic32(const Atomic32& b)
	{
		Set(b.m_Value);
	}

	explicit Atomic32(Atomic32Type b)
	{
		Set(b);
	}

	Atomic32& operator=(const Atomic32& value)
	{
		Set(value.m_Value);
		return *this;
	}

	/**
	 * Or value to this Atomic32,
	 * using intrinsics that are guaranteed to be atomic
	 * (the value will be updated correctly, even when
	 * called from multiple threads).
	 */
	Atomic32Type operator|=(Atomic32Type value)
	{
#if SEOUL_PLATFORM_WINDOWS
		Atomic32Type const ret = ::_InterlockedOr(&m_Value, value);
		return (ret | value);
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
		// Returned value is the result of the operation.
		return __atomic_or_fetch(&m_Value, value, __ATOMIC_SEQ_CST);
#else
#		error Implement for this platform.
#endif
	}

	/**
	 * And value to this Atomic32,
	 * using intrinsics that are guaranteed to be atomic
	 * (the value will be updated correctly, even when
	 * called from multiple threads).
	 */
	Atomic32Type operator&=(Atomic32Type value)
	{
#if SEOUL_PLATFORM_WINDOWS
		Atomic32Type const ret = ::_InterlockedAnd(&m_Value, value);
		return (ret & value);
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
		// Returned value is the result of the operation.
		return __atomic_and_fetch(&m_Value, value, __ATOMIC_SEQ_CST);
#else
#		error Implement for this platform.
#endif
	}

	/**
	 * Add value to this Atomic32,
	 * using intrinsics that are guaranteed to be atomic
	 * (the value will be updated correctly, even when
	 * called from multiple threads).
	 */
	Atomic32Type operator+=(Atomic32Type value)
	{
#if SEOUL_PLATFORM_WINDOWS
		Atomic32Type const ret = ::_InterlockedExchangeAdd(&m_Value, value);
		return (ret + value);
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
		// Returned value is the result of the operation.
		return __atomic_add_fetch(&m_Value, value, __ATOMIC_SEQ_CST);
#else
#		error Implement for this platform.
#endif
	}

	/**
	 * Subtract value from this Atomic32,
	 * using intrinsics that are guaranteed to be atomic
	 * (the value will be updated correctly, even when
	 * called from multiple threads).
	 */
	Atomic32Type operator-=(Atomic32Type value)
	{
#if SEOUL_PLATFORM_WINDOWS
		Atomic32Type const ret = ::_InterlockedExchangeAdd(&m_Value, -value);
		return (ret - value);
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
		// Returned value is the result of the operation.
		return __atomic_sub_fetch(&m_Value, value, __ATOMIC_SEQ_CST);
#else
#		error Implement for this platform.
#endif
	}

	/**
	 * Preincrement is guaranteed atomic (the count
	 * will be updated correctly, even with operations
	 * called from multiple threads).
	 */
	Atomic32Type operator++()
	{
#if SEOUL_PLATFORM_WINDOWS
		// InterlockedIncrement returns the post increment value.
		return ::_InterlockedIncrement(&m_Value);
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
		// Returned value is the result of the operation.
		return __atomic_add_fetch(&m_Value, 1, __ATOMIC_SEQ_CST);
#else
#		error Implement for this platform.
#endif
	}

	/**
	 * Postincrement is guaranteed atomic (the count
	 * will be updated correctly, even with operations
	 * called from multiple threads).
	 */
	Atomic32Type operator++(int)
	{
#if SEOUL_PLATFORM_WINDOWS
		// InterlockedIncrement returns the post increment value.
		return ::_InterlockedIncrement(&m_Value) - 1;
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
		// Returned value is the result of the operation, so we need to return result - 1.
		return __atomic_add_fetch(&m_Value, 1, __ATOMIC_SEQ_CST) - 1;
#else
#		error Implement for this platform.
#endif
	}

	/**
	 * Predecrement is guaranteed atomic (the count
	 * will be updated correctly, even with operations
	 * called from multiple threads).
	 */
	Atomic32Type operator--()
	{
#if SEOUL_PLATFORM_WINDOWS
		// InterlockedIncrement returns the post decrement value.
		return ::_InterlockedDecrement(&m_Value);
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
		// Returned value is the result of the operation.
		return __atomic_sub_fetch(&m_Value, 1, __ATOMIC_SEQ_CST);
#else
#		error Implement for this platform.
#endif
	}

	/**
	 * Postdecrement is guaranteed atomic (the count
	 * will be updated correctly, even with operations
	 * called from multiple threads).
	 */
	Atomic32Type operator--(int)
	{
#if SEOUL_PLATFORM_WINDOWS
		// InterlockedIncrement returns the post decrement value.
		return ::_InterlockedDecrement(&m_Value) + 1;
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
		// Returned value is the result of the operation, so we need to return result + 1.
		return __atomic_sub_fetch(&m_Value, 1, __ATOMIC_SEQ_CST) + 1;
#else
#		error Implement for this platform.
#endif
	}

	/** @return this Atomic32 as its underlying builtin type. */
	operator Atomic32Type() const
	{
#if SEOUL_PLATFORM_WINDOWS
		return static_cast<Atomic32Type const volatile&>(m_Value);
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
		return __atomic_load_n((Atomic32Type*)&m_Value, __ATOMIC_SEQ_CST);
#else
#		error Implement for this platform.
#endif
	}

	/**
	 * Attempt to set this Atomic32Type to newValue,
	 * if this Atomic32Type is atomicly equal to expectedCurrentValue
	 * at the time of the attempted set. If not equal, no set occurs.
	 *
	 * @return The original value of this Atomic32Type before the set
	 * attempt - will still be equal to this value after the set attempt if
	 * the current value is not equal to expectedCurrentValue, otherwise
	 * this Atomic32Type will be equal to newValue after the set.
	 */
	Atomic32Type CompareAndSet(Atomic32Type newValue, Atomic32Type expectedCurrentValue)
	{
		return Atomic32Common::CompareAndSet(&m_Value, newValue, expectedCurrentValue);
	}

	/** Set the value of this Atomic32 using the raw Atomic32Type. */
	void Set(Atomic32Type value)
	{
#if SEOUL_PLATFORM_WINDOWS
		::_InterlockedExchange(&m_Value, value);
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
		__atomic_store_n(&m_Value, value, __ATOMIC_SEQ_CST);
#else
#		error Implement for this platform.
#endif
	}

	/**
	 * Reset the Atomic32 value back to 0.
	 */
	void Reset()
	{
		Set(0);
	}

private:
	volatile Atomic32Type m_Value;
}; // class Atomic32

namespace Atomic32ValueUtil
{

template <typename T>
static inline Atomic32Type From(T v)
{
	union
	{
		T in;
		Atomic32Type out;
	};
	out = 0;
	in = v;
	return out;
}

// Variation to normalize some Float values.
static inline Atomic32Type From(Float f)
{
	if (IsNaN(f)) { return From(0xFFFFFFFE); }// Canonical 32-bit floating NaN.
	else
	{
		f = (0.0f == f ? 0.0f : f); // -0.0f vs. 0.0f

		union
		{
			Float in;
			Atomic32Type out;
		};
		in = f;
		return out;
	}
}

template <typename T>
static inline T To(Atomic32Type v)
{
	union
	{
		Atomic32Type in;
		T out;
	};
	out = (T)0;
	in = v;
	return out;
}

} // namespace

/**
 * Generic, simplified version of Atomic32.
 *
 * Allows any primitive type of size <= 4 to be store atomically. Limited
 * utility functions, makes no assumption about the underyling value other
 * than its ability to be loaded and stored and its abiltiy to be cast to/from
 * an Atomic32Type.
 */
template <typename T>
class Atomic32Value SEOUL_SEALED
{
public:
	// Size must be <= 4.
	SEOUL_STATIC_ASSERT(sizeof(T) <= 4);

	Atomic32Value()
	{
		Set(Atomic32ValueUtil::To<T>(0));
	}

	explicit Atomic32Value(T b)
	{
		Set(b);
	}

	Atomic32Value& operator=(const Atomic32Value& value)
	{
		Set(Atomic32ValueUtil::To<T>(value.m_Value));
		return *this;
	}

	Atomic32Value& operator=(T value)
	{
		Set(value);
		return *this;
	}

	/** @return this Atomic32Value as its underlying T type. */
	operator T() const
	{
#if SEOUL_PLATFORM_WINDOWS
		return Atomic32ValueUtil::To<T>(static_cast<Atomic32Type const volatile&>(m_Value));
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
		return Atomic32ValueUtil::To<T>(__atomic_load_n(&m_Value, __ATOMIC_SEQ_CST));
#else
#		error Implement for this platform.
#endif
	}

	/**
	 * Attempt to set this Atomic32Value to newValue,
	 * if this Atomic32Value is atomicly equal to expectedCurrentValue
	 * at the time of the attempted set. If not equal, no set occurs.
	 *
	 * @return The original value of this Atomic32Value before the set
	 * attempt - will still be equal to this value after the set attempt if
	 * the current value is not equal to expectedCurrentValue, otherwise
	 * this Atomic32Value will be equal to newValue after the set.
	 */
	T CompareAndSet(T newValue, T expectedCurrentValue)
	{
		return Atomic32ValueUtil::To<T>(Atomic32Common::CompareAndSet(&m_Value, Atomic32ValueUtil::From(newValue), Atomic32ValueUtil::From(expectedCurrentValue)));
	}

	/** Set the value of this Atomic32Value using the raw value T. */
	void Set(T value)
	{
#if SEOUL_PLATFORM_WINDOWS
		::_InterlockedExchange(&m_Value, Atomic32ValueUtil::From(value));
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
		__atomic_store_n(&m_Value, Atomic32ValueUtil::From(value), __ATOMIC_SEQ_CST);
#else
#		error Implement for this platform.
#endif
	}

private:
	volatile Atomic32Type m_Value;
}; // class Atomic32Value

} // namespace Seoul

#endif // include guard
