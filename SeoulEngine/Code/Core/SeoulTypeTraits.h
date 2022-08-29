/**
 * \file SeoulTypeTraits.h
 * \brief Similar in purpose to the <type_traits> C99 header, which
 * is not available on all platforms. Provides basic template<> utilities
 * for manipulating types, checking types and signatures, etc. at runtime.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SEOUL_TYPE_TRAITS_H
#define SEOUL_TYPE_TRAITS_H

#include "SeoulTypes.h"
#include "StandardPlatformIncludes.h"

namespace Seoul
{

/**
 * Utility template class, evaluates to true if A and B are the same type.
 */
template <typename T, typename U> struct AreSame { static const Bool Value = false; };
template <typename T> struct AreSame<T, T> { static const Bool Value = true; };

/**
 * Utility template class, evaluates to true if T is an abstract class.
 */
template <typename T>
struct IsAbstract
{
#if defined(__GNUC__) || defined(_MSC_VER)
	static const Bool Value = __is_abstract(T);
#else
#	error "Define for this platform."
#endif
};

/**
 * Utility template class, evaluates to true if PARENT is a base class of T.
 */
template <typename PARENT, typename T>
struct IsBaseOf
{
#if defined(__GNUC__) || defined(_MSC_VER)
	static const Bool Value = __is_base_of(PARENT, T);
#else
#	error "Define for this platform."
#endif
};

/**
 * Utility template class, evalutes to true if a type is const qualified.
 */
template <typename T> struct IsConst { static const Bool Value = false; };
template <typename T> struct IsConst<T const> { static const Bool Value = true; };
template <typename T> struct IsConst<T const volatile> { static const Bool Value = true; };

/**
 * Utility template class, evalutes to true if a type can be converted from FROM to type TO, false otherwise.
 */
template <typename FROM, typename TO>
struct IsConvertible
{
#if defined(__GNUC__) || defined(_MSC_VER)
	static const Bool Value = __is_convertible_to(FROM, TO);
#else
#	error "Define for this platform."
#endif
};

/**
 * Utility template class, evaluates to true if a type is an enum, false otherwise.
 */
template <typename T>
struct IsEnum
{
#if defined(__GNUC__) || defined(_MSC_VER)
	static const Bool Value = __is_enum(T);
#else
#	error "Define for this platform."
#endif
};

/**
 * Utility template class, evaluates to true if a type is a floating point type, false otherwise.
 */
template <typename T> struct IsFloatingPoint { static const Bool Value = false; };
template <> struct IsFloatingPoint<Float32> { static const Bool Value = true; };
template <> struct IsFloatingPoint<Float64> { static const Bool Value = true; };
template <typename T> struct IsFloatingPoint<T const> { static const Bool Value = IsFloatingPoint<T>::Value; };
template <typename T> struct IsFloatingPoint<T volatile> { static const Bool Value = IsFloatingPoint<T>::Value; };
template <typename T> struct IsFloatingPoint<T const volatile> { static const Bool Value = IsFloatingPoint<T>::Value; };

/**
 * Utility template class, evaluates to true if a type is an integer type, false otherwise.
 */
template <typename T> struct IsIntegral { static const Bool Value = false; };
template <> struct IsIntegral<Bool> { static const Bool Value = true; };
template <> struct IsIntegral<char> { static const Bool Value = true; };
template <> struct IsIntegral<wchar_t> { static const Bool Value = true; };
template <> struct IsIntegral<Int8> { static const Bool Value = true; };
template <> struct IsIntegral<UInt8> { static const Bool Value = true; };
template <> struct IsIntegral<Int16> { static const Bool Value = true; };
template <> struct IsIntegral<UInt16> { static const Bool Value = true; };
template <> struct IsIntegral<Int32> { static const Bool Value = true; };
template <> struct IsIntegral<UInt32> { static const Bool Value = true; };
template <> struct IsIntegral<Int64> { static const Bool Value = true; };
template <> struct IsIntegral<UInt64> { static const Bool Value = true; };
template <typename T> struct IsIntegral<T const> { static const Bool Value = IsIntegral<T>::Value; };
template <typename T> struct IsIntegral<T volatile> { static const Bool Value = IsIntegral<T>::Value; };
template <typename T> struct IsIntegral<T const volatile> { static const Bool Value = IsIntegral<T>::Value; };

/** Determine if a destructor is trivially destructible. */
#if defined(__GNUC__)
	template <typename T> struct IsTriviallyDestructible { static const Bool Value = __has_trivial_destructor(T); };
#elif defined(_MSC_VER)
	template <typename T> struct IsTriviallyDestructible { static const Bool Value = __is_trivially_destructible(T); };
#else
#	error "Define for this platform."
#endif

/**
 * Utility template class, evaluates to true if it is safe to memcpy() (or equivalent)
 * a value of this type, false otherwise.
 *
 * Can be explicitly overriden to allow this handling even when a value has an explicit copy constructor.
 */
#if defined(__GNUC__)
	template <typename T> struct CanMemCpy { static const Bool Value = __is_pod(T); };
#elif defined(_MSC_VER)
	template <typename T> struct CanMemCpy { static const Bool Value = (!__is_class(T) || __is_pod(T)); };
#else
#	error "Define for this platform."
#endif

/**
 * Utility template class, evaluates to true if it is safe to memset(, 0, ) (or equivalent)
 * a value of this type, false otherwise.
 *
 * Can be explicitly overriden to allow this handling even when a value has an explicit default constructor.
 */
#if defined(__GNUC__)
	template <typename T> struct CanZeroInit { static const Bool Value = __is_pod(T); };
#elif defined(_MSC_VER)
	template <typename T> struct CanZeroInit { static const Bool Value = (!__is_class(T) || __is_pod(T)); };
#else
#	error "Define for this platform."
#endif

/**
 * Utility template class, evaluates to true if a type is a pointer, false otherwise.
 */
template <typename T> struct IsPointer { static const Bool Value = false; };
template <typename T> struct IsPointer<T*> { static const Bool Value = true; };
template <typename T> struct IsPointer<T* const> { static const Bool Value = true; };
template <typename T> struct IsPointer<T* volatile> { static const Bool Value = true; };
template <typename T> struct IsPointer<T* const volatile> { static const Bool Value = true; };

/**
 * Utility template class, evaluates to true if a type is a reference, false otherwise.
 */
template <typename T> struct IsReference { static const Bool Value = false; };
template <typename T> struct IsReference<T&> { static const Bool Value = true; };

/**
 * Utility template class, evalutes to true if a type is the void type.
 */
template <typename T> struct IsVoid { static const Bool Value = false; };
template <> struct IsVoid<void> { static const Bool Value = true; };

/**
 * Utility template class, evalutes to true if a type is volatile qualified.
 */
template <typename T> struct IsVolatile { static const Bool Value = false; };
template <typename T> struct IsVolatile<T volatile> { static const Bool Value = true; };
template <typename T> struct IsVolatile<T const volatile> { static const Bool Value = true; };

/** Limited set of iterator traits, roughly equivalent to std::iterator_traits<>. */
template <typename T> struct IteratorTraits { typedef typename T::ValueType ValueType; };
template <typename T> struct IteratorTraits<T*> { typedef T ValueType; };

/**
 * Utility template class, removes all pointer, reference, const, and volatile qualifiers from a type.
 * Essentially, gets the "base" type of a type, without qualifiers, excluding array qualifiers.
 */
template <typename T> struct RemoveAllCvReferencePointer { typedef T Type; };
template <typename T> struct RemoveAllCvReferencePointer<T&> { typedef typename RemoveAllCvReferencePointer<T>::Type Type; };
template <typename T> struct RemoveAllCvReferencePointer<T*> { typedef typename RemoveAllCvReferencePointer<T>::Type Type; };
template <typename T> struct RemoveAllCvReferencePointer<T const> { typedef typename RemoveAllCvReferencePointer<T>::Type Type; };
template <typename T> struct RemoveAllCvReferencePointer<T volatile> { typedef typename RemoveAllCvReferencePointer<T>::Type Type; };
template <typename T> struct RemoveAllCvReferencePointer<T const volatile> { typedef typename RemoveAllCvReferencePointer<T>::Type Type; };

/**
 * Utility template class, removes const qualifier on the type T.
 */
template <typename T> struct RemoveConst { typedef T Type; };
template <typename T> struct RemoveConst<T const> { typedef T Type; };
template <typename T> struct RemoveConst<T const volatile> { typedef T volatile Type; };

/**
 * Utility template class, removes const and volatile qualifiers on the type T.
 */
template <typename T> struct RemoveConstVolatile { typedef T Type; };
template <typename T> struct RemoveConstVolatile<T const> { typedef T Type; };
template <typename T> struct RemoveConstVolatile<T const volatile> { typedef T Type; };
template <typename T> struct RemoveConstVolatile<T volatile> { typedef T Type; };

/**
 * Utility template class, removes pointer qualifier on the type T.
 */
template <typename T> struct RemovePointer { typedef T Type; };
template <typename T> struct RemovePointer<T*> { typedef T Type; };
template <typename T> struct RemovePointer<T* const> { typedef T Type; };
template <typename T> struct RemovePointer<T* volatile> { typedef T Type; };
template <typename T> struct RemovePointer<T* const volatile> { typedef T Type; };


/**
 * Utility template class, removes reference qualifier on the type T.
 */
template <typename T> struct RemoveReference { typedef T Type; };
template <typename T> struct RemoveReference<T&> { typedef T Type; };

/**
 * Utility template class, removes volatile qualifier on the type T.
 */
template <typename T> struct RemoveVolatile { typedef T Type; };
template <typename T> struct RemoveVolatile<T volatile> { typedef T Type; };
template <typename T> struct RemoveVolatile<T const volatile> { typedef T const Type; };

/**
 * Utility template class, removes outer const, volatile, and ref from a type T.
 */
template <typename T> struct RemoveConstVolatileReference { typedef T Type; };
template <typename T> struct RemoveConstVolatileReference<T&> { typedef T Type; };
template <typename T> struct RemoveConstVolatileReference<T const> { typedef T Type; };
template <typename T> struct RemoveConstVolatileReference<T const volatile> { typedef T Type; };
template <typename T> struct RemoveConstVolatileReference<T volatile> { typedef T Type; };

/**
 * SeoulEngine equivalent to std::move().
 */
template <typename T>
constexpr typename RemoveReference<T>::Type&& RvalRef(T&& arg)
{
	return (static_cast<typename RemoveReference<T>::Type&&>(arg));
}

} // namespace Seoul

#endif // include guard
