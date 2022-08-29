/**
 * \file Atomic64.h
 * \brief Provides a thread-safe 64-bit integer value. Can be used
 * for thread-safe reference counts, flags, etc.
 *
 * \warning The sign of the fundamental type used by Atomic64
 * varies per platform. For guaranteed platform independent behavior,
 * assume Atomic64 can only store values in the range [0, 2^63 - 1].
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ATOMIC_64_H
#define ATOMIC_64_H

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

namespace Atomic64Common
{
	/*
	 * Attempt to set this Atomic64Type to newValue,
	 * if this Atomic64Type is atomicly equal to expectedCurrentValue
	 * at the time of the attempted set. If not equal, no set occurs.
	 *
	 * @return The original value of this Atomic64Type before the set
	 * attempt - will still be equal to this value after the set attempt if
	 * the current value is not equal to expectedCurrentValue, otherwise
	 * this Atomic64Type will be equal to newValue after the set.
	 */
	static inline Atomic64Type CompareAndSet(Atomic64Type volatile* pValue, Atomic64Type newValue, Atomic64Type expectedCurrentValue)
	{
#if SEOUL_PLATFORM_WINDOWS
		return ::_InterlockedCompareExchange64(
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
 * Provides thread-safe 64-bit integer.
 *
 * \warning The sign of Atomic64 varies per-platform. For safe, platform
 * independent usage, assume Atomic64 can store the positive range of
 * a signed 64-bit integer [0, 2^63 - 1].
 *
 * \sa boost::atomic_count
 */
class Atomic64 SEOUL_SEALED
{
public:
	Atomic64()
	{
		Set(0);
	}

	Atomic64(const Atomic64& b)
	{
		Set(b.m_Value);
	}

	explicit Atomic64(Atomic64Type b)
	{
		Set(b);
	}

	Atomic64& operator=(const Atomic64& value)
	{
		Set(value.m_Value);
		return *this;
	}

	/**
	 * Or value to this Atomic64,
	 * using intrinsics that are guaranteed to be atomic
	 * (the value will be updated correctly, even when
	 * called from multiple threads).
	 */
	Atomic64Type operator|=(Atomic64Type value)
	{
#if SEOUL_PLATFORM_WINDOWS
		Atomic64Type const ret = ::_InterlockedOr64(&m_Value, value);
		return (ret | value);
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
		// Returned value is the result of the operation.
		return __atomic_or_fetch(&m_Value, value, __ATOMIC_SEQ_CST);
#else
#		error Implement for this platform.
#endif
	}

	/**
	 * And value to this Atomic64,
	 * using intrinsics that are guaranteed to be atomic
	 * (the value will be updated correctly, even when
	 * called from multiple threads).
	 */
	Atomic64Type operator&=(Atomic64Type value)
	{
#if SEOUL_PLATFORM_WINDOWS
		Atomic64Type const ret = ::_InterlockedAnd64(&m_Value, value);
		return (ret & value);
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
		// Returned value is the result of the operation.
		return __atomic_and_fetch(&m_Value, value, __ATOMIC_SEQ_CST);
#else
#		error Implement for this platform.
#endif
	}

	/**
	 * Add value to this Atomic64,
	 * using intrinsics that are guaranteed to be atomic
	 * (the value will be updated correctly, even when
	 * called from multiple threads).
	 */
	Atomic64Type operator+=(Atomic64Type value)
	{
#if SEOUL_PLATFORM_WINDOWS
		Atomic64Type const ret = ::_InterlockedExchangeAdd64(&m_Value, value);
		return (ret + value);
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
		// Returned value is the result of the operation.
		return __atomic_add_fetch(&m_Value, value, __ATOMIC_SEQ_CST);
#else
#		error Implement for this platform.
#endif
	}

	/**
	 * Subtract value from this Atomic64,
	 * using intrinsics that are guaranteed to be atomic
	 * (the value will be updated correctly, even when
	 * called from multiple threads).
	 */
	Atomic64Type operator-=(Atomic64Type value)
	{
#if SEOUL_PLATFORM_WINDOWS
		Atomic64Type const ret = ::_InterlockedExchangeAdd64(&m_Value, -value);
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
	Atomic64Type operator++()
	{
#if SEOUL_PLATFORM_WINDOWS
		// InterlockedIncrement returns the post increment value.
		return ::_InterlockedIncrement64(&m_Value);
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
	Atomic64Type operator++(int)
	{
#if SEOUL_PLATFORM_WINDOWS
		// InterlockedIncrement returns the post increment value.
		return ::_InterlockedIncrement64(&m_Value) - 1;
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
	Atomic64Type operator--()
	{
#if SEOUL_PLATFORM_WINDOWS
		// InterlockedIncrement returns the post decrement value.
		return ::_InterlockedDecrement64(&m_Value);
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
	Atomic64Type operator--(int)
	{
#if SEOUL_PLATFORM_WINDOWS
		// InterlockedIncrement returns the post decrement value.
		return ::_InterlockedDecrement64(&m_Value) + 1;
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
		// Returned value is the result of the operation, so we need to return result + 1.
		return __atomic_sub_fetch(&m_Value, 1, __ATOMIC_SEQ_CST) + 1;
#else
#		error Implement for this platform.
#endif
	}

	/** @return this Atomic64 as its underlying builtin type. */
	operator Atomic64Type() const
	{
#if SEOUL_PLATFORM_WINDOWS
		return static_cast<Atomic64Type const volatile&>(m_Value);
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
		return __atomic_load_n((Atomic64Type*)&m_Value, __ATOMIC_SEQ_CST);
#else
#		error Implement for this platform.
#endif
	}

	/**
	 * Attempt to set this Atomic64Type to newValue,
	 * if this Atomic64Type is atomicly equal to expectedCurrentValue
	 * at the time of the attempted set. If not equal, no set occurs.
	 *
	 * @return The original value of this Atomic64Type before the set
	 * attempt - will still be equal to this value after the set attempt if
	 * the current value is not equal to expectedCurrentValue, otherwise
	 * this Atomic64Type will be equal to newValue after the set.
	 */
	Atomic64Type CompareAndSet(Atomic64Type newValue, Atomic64Type expectedCurrentValue)
	{
		return Atomic64Common::CompareAndSet(&m_Value, newValue, expectedCurrentValue);
	}

	/** Set the value of this Atomic64 using the raw Atomic64Type. */
	void Set(Atomic64Type value)
	{
#if SEOUL_PLATFORM_WINDOWS
		::_InterlockedExchange64(&m_Value, value);
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
		__atomic_store_n(&m_Value, value, __ATOMIC_SEQ_CST);
#else
#		error Implement for this platform.
#endif
	}

	/**
	 * Reset the Atomic64 value back to 0.
	 */
	void Reset()
	{
		Set(0);
	}

private:
	volatile Atomic64Type m_Value;
}; // class Atomic64

namespace Atomic64ValueUtil
{

template <typename T>
static inline Atomic64Type From(T v)
{
	union
	{
		T in;
		Atomic64Type out;
	};
	out = 0;
	in = v;
	return out;
}

// Variation to normalize some Float32 values.
static inline Atomic64Type From(Float32 f)
{
	if (IsNaN(f)) { return From(0xFFFFFFFE); }// Canonical 64-bit floating NaN.
	else
	{
		f = (0.0f == f ? 0.0f : f); // -0.0f vs. 0.0f

		union
		{
			Float32 in;
			Atomic64Type out;
		};
		out = 0;
		in = f;
		return out;
	}
}

// Variation to normalize some Float64 values.
static inline Atomic64Type From(Float64 f)
{
	if (IsNaN(f)) { return From(0xFFFFFFFFFFFFFFFE); }// Canonical 64-bit floating NaN.
	else
	{
		f = (0.0 == f ? 0.0 : f); // -0.0 vs. 0.0

		union
		{
			Float64 in;
			Atomic64Type out;
		};
		in = f;
		return out;
	}
}

template <typename T>
static inline T To(Atomic64Type v)
{
	union
	{
		Atomic64Type in;
		T out;
	};
	out = (T)0;
	in = v;
	return out;
}

} // namespace

/**
 * Generic, simplified version of Atomic64.
 *
 * Allows any primitive type of size <= 8 to be store atomically. Limited
 * utility functions, makes no assumption about the underyling value other
 * than its ability to be loaded and stored and its abiltiy to be cast to/from
 * an Atomic64Type.
 */
template <typename T>
class Atomic64Value SEOUL_SEALED
{
public:
	// Size must be <= 8.
	SEOUL_STATIC_ASSERT(sizeof(T) <= 8);

	Atomic64Value()
	{
		Set(Atomic64ValueUtil::To<T>(0));
	}

	explicit Atomic64Value(T b)
	{
		Set(b);
	}

	Atomic64Value& operator=(const Atomic64Value& value)
	{
		Set(Atomic64ValueUtil::To<T>(value.m_Value));
		return *this;
	}

	Atomic64Value& operator=(T value)
	{
		Set(value);
		return *this;
	}

	/** @return this Atomic64Value as its underlying T type. */
	operator T() const
	{
#if SEOUL_PLATFORM_WINDOWS
		return Atomic64ValueUtil::To<T>(static_cast<Atomic64Type const volatile&>(m_Value));
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
		return Atomic64ValueUtil::To<T>(__atomic_load_n(&m_Value, __ATOMIC_SEQ_CST));
#else
#		error Implement for this platform.
#endif
	}

	/**
	 * Attempt to set this Atomic64Value to newValue,
	 * if this Atomic64Value is atomicly equal to expectedCurrentValue
	 * at the time of the attempted set. If not equal, no set occurs.
	 *
	 * @return The original value of this Atomic64Value before the set
	 * attempt - will still be equal to this value after the set attempt if
	 * the current value is not equal to expectedCurrentValue, otherwise
	 * this Atomic64Value will be equal to newValue after the set.
	 */
	T CompareAndSet(T newValue, T expectedCurrentValue)
	{
		return Atomic64ValueUtil::To<T>(Atomic64Common::CompareAndSet(&m_Value, Atomic64ValueUtil::From(newValue), Atomic64ValueUtil::From(expectedCurrentValue)));
	}

	/** Set the value of this Atomic64Value using the raw value T. */
	void Set(T value)
	{
#if SEOUL_PLATFORM_WINDOWS
		::_InterlockedExchange64(&m_Value, Atomic64ValueUtil::From(value));
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
		__atomic_store_n(&m_Value, Atomic64ValueUtil::From(value), __ATOMIC_SEQ_CST);
#else
#		error Implement for this platform.
#endif
	}

private:
	volatile Atomic64Type m_Value;
}; // class Atomic64Value

} // namespace Seoul

#endif // include guard
