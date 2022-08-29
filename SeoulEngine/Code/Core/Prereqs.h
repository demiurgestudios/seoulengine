/**
 * \file Prereqs.h
 * \brief Prerequisites header, simple root to include commonly
 * used functions, macros, and constants.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef PREREQS_H
#define PREREQS_H

#include "StandardPlatformIncludes.h"
#include "Core.h"
#include "SeoulAssert.h"
#include "SeoulTypeTraits.h"
#include "SeoulTypes.h"
#include "StaticAssert.h"

// NOTE: !defined(__clang__) here is to support
// include-what-you-use in our tools pipeline. It defines _MSVC
// and clang support most VisualStudio quirks, but this is one
// that it does not.
#if defined(_MSC_VER) && !defined(__clang__)
/**
 * SEOUL_ABSTRACT is a #define wrapper around the MSVC proprietary
 * extension keyword 'abstract'. 'abstract' has the following meaning:
 * - "A member can only be defined in a derived type."
 * - "A type cannot be instantiated (can only act as a base type)."
 *
 * SEOUL_ABSTRACT should be used to tag either of these two cases. Most
 * importantly, SEOUL_ABSTRACT can be used to tag classes as abstract
 * that would otherwise not be, preventing their instantiation without
 * a specialization.
 *
 * Using SEOUL_ABSTRACT allows the MSVC compiler to verify that your
 * intent matches your execution. It does not add new or proprietary semantics
 * or create a new type or class of object.
 */
#define SEOUL_ABSTRACT abstract

/**
 * SEOUL_OVERRIDE is a #define wrapper around the MSVC proprietary
 * extension keyword 'override'. 'override' has the following meaning:
 * - "override indicates that a member of a managed type must override a
 *   base class or a base interface member. If there is no member to override,
 *   the compiler will generate an error."
 *
 * SEOUL_OVERRIDE should be used to tag any function that overrides a virtual
 * base function. Most importantly, SEOUL_OVERRIDE will verify that a function
 * override is declared properly and will prevent errors caused by the
 * overriding function declaration not matching the base function declaration.
 *
 * Using SEOUL_OVERRIDE allows the MSVC compiler to verify that
 * your intent matches your execution. It does not add new or proprietary
 * semantics or create a new type or class of object.
 */
#define SEOUL_OVERRIDE override

/**
 * SEOUL_SEALED is a #define wrapper around the MSVC proprietary
 * extension keyword 'sealed'. 'sealed' has the following meaning:
 *  - "A virtual member cannot be overridden."
 *  - "A type cannot be used as a base type."
 *
 * Most importantly, SEOUL_SEALED allows you to prevent inheritance of
 * functions or classes. This can be used to create shallow class
 * hierarchies or to ensure that classes which are not designed to be
 * polymorphic are not treated as such.
 *
 * Using SEOUL_SEALED allows the MSVC compiler to verify that your
 * intent matches your execution. It does not add new or proprietary semantics
 */
#define SEOUL_SEALED sealed

/**
 * SEOUL_EXPORT declares that a function should be exported from
 * the current dynamic library.
 */
#define SEOUL_EXPORT __declspec(dllexport)

/**
 * SEOUL_STD_CALL declares a function calling convention. It forces
 * a function to use the standard call calling convention. This is usually
 * important for callbacks or other situations where an external library
 * expects a very specific function calling convention.
 */
#define SEOUL_STD_CALL __stdcall
#else

// Subset are supported in all C++11 compilers.
#if __cplusplus >= 201103L
#	define SEOUL_ABSTRACT
#	define SEOUL_OVERRIDE override
#	define SEOUL_SEALED
#	define SEOUL_EXPORT
#	define SEOUL_STD_CALL
#else
#	define SEOUL_ABSTRACT
#	define SEOUL_OVERRIDE
#	define SEOUL_SEALED
#	define SEOUL_EXPORT
#	define SEOUL_STD_CALL
#endif

#endif

namespace Seoul
{

/**
 * Used by HashTable as the "null" key value,
 * used as a marker for undefined values and should never
 * be passed in as a valid key.
 */
template <typename T>
struct DefaultHashTableKeyTraits
{
	inline static Float GetLoadFactor()
	{
		return 0.75f;
	}

	inline static T GetNullKey()
	{
		return (T)0;
	}

	static const Bool kCheckHashBeforeEquals = false;
};

/** Utility used by Reflection and other utilities that need to convert types to a DataNode. */
template <typename T, Bool B_IS_ENUM>
struct DataNodeHandler
{
	static const Bool Value = false;
};

/**
 * Deletes a heap allocated object and sets the pointer to nullptr
 */
template <typename T>
inline void SafeDelete(T*& rpPointer)
{
	T* p = rpPointer;
	rpPointer = nullptr;

	SEOUL_DELETE p;
}

/**
 * Deletes a heap allocated array and sets the pointer to nullptr
 */
template <typename T>
inline void SafeDeleteArray(T*& rpPointer)
{
	T* p = rpPointer;
	rpPointer = nullptr;

	SEOUL_DELETE [] p;
}

/**
 * Calls AddRef() on a valid pointer and returns the reference
 * count returned by the AddRef() function.
 *
 * Returns 0u and does nothing if pPointer is nullptr.
 */
template <typename T>
UInt SafeAcquire(T* pPointer)
{
	if (pPointer)
	{
		return pPointer->AddRef();
	}

	return 0u;
}

/**
 * Calls release on a valid pointer pPointer (does nothing if the
 * pointer is nullptr), and sets ap to nullptr.
 */
template <typename T>
UInt SafeRelease(T*& rpPointer)
{
	T* p = rpPointer;
	rpPointer = nullptr;

	if (p)
	{
		return p->Release();
	}

	return 0u;
}

/**
 * @return True if v is a power of 2, false otherwise.
 */
template <typename T>
inline Bool IsPowerOfTwo(T v)
{
	return (0 == (v & (v - 1)));
}

/**
 * @return The value zValue rounded up so that it is a multiple
 * of the alignment zAlignment.
 */
inline size_t RoundUpToAlignment(size_t zValue, size_t zAlignment)
{
	if (zAlignment > 0u)
	{
		size_t const zModulo = (zValue % zAlignment);
		size_t const zReturn = (0u == zModulo)
			? zValue
			: (zValue + zAlignment - zModulo);

		SEOUL_ASSERT(0u == zReturn % zAlignment);

		return zReturn;
	}
	else
	{
		return zValue;
	}
}

/**
 * @return The pointer pValue rounded up so that it is a multiple
 * of the alignment zAlignment.
 */
inline void* RoundUpToAlignment(void* pValue, size_t zAlignment)
{
	return (void*)RoundUpToAlignment((size_t)pValue, zAlignment);
}

/**
 * @return The pointer pValue rounded up so that it is a multiple
 * of the alignment zAlignment.
 */
inline Byte* RoundUpToAlignment(Byte* pValue, size_t zAlignment)
{
	return (Byte*)RoundUpToAlignment((size_t)pValue, zAlignment);
}

/**
 * @return The pointer pValue rounded up so that it is a multiple
 * of the alignment zAlignment.
 */
inline Byte const* RoundUpToAlignment(Byte const* pValue, size_t zAlignment)
{
	return (Byte const*)RoundUpToAlignment((size_t)pValue, zAlignment);
}

/**
 * @return The pointer pValue, rounded up to alignment zAlignment, relative
 * to pBase, assuming pBase fulfills zAlignment.
 */
inline Byte const* RoundUpToRelativeAlignment(Byte const* pValue, Byte const* pBaseValue, size_t zAlignment)
{
	SEOUL_ASSERT(0u == ((size_t)pBaseValue % zAlignment));
	SEOUL_ASSERT(pBaseValue <= pValue);

	return pBaseValue + RoundUpToAlignment(pValue - pBaseValue, zAlignment);
}

/**
 * @return A UInt32 that is next highest power of 2u of u.
 */
inline UInt32 GetNextPowerOf2(UInt32 u)
{
	// Handle the edge case of u == 0u.
	if (0u == u)
	{
		return 1u;
	}

	// Subtract first to handle values that are already power of 2.
	u--;
	u |= (u >> 1u);
	u |= (u >> 2u);
	u |= (u >> 4u);
	u |= (u >> 8u);
	u |= (u >> 16u);
	u++;

	return u;
}

/**
 * Return an unsigned integer that is the power of 2
 * which is closest and less than or equal to x.
 */
inline UInt32 GetPreviousPowerOf2(UInt32 u)
{
	return GetNextPowerOf2(u + 1u) / 2u;
}

/**
 * Swaps the endianness of a 16-bit value
 *
 * Reverses the order of the bytes of a 16-bit value
 *
 * @param[in] nValue Value to switch endianness of
 *
 * @return nValue with its endianness swapped
 */
inline UInt16 EndianSwap16(UInt16 nValue)
{
	return (nValue >> 8) | (nValue << 8);
}

/**
 * Swaps the endianness of a 16-bit value
 *
 * Reverses the order of the bytes of a 16-bit value
 *
 * @param[in] nValue Value to switch endianness of
 *
 * @return nValue with its endianness swapped
 */
inline Int16 EndianSwap16(Int16 nValue)
{
	return (Int16)EndianSwap16((UInt16)nValue);
}

/**
 * Swaps the endianness of a 32-bit value
 *
 * Reverses the order of the bytes of a 32-bit value
 *
 * @param[in] nValue Value to switch endianness of
 *
 * @return nValue with its endianness swapped
 */
inline UInt32 EndianSwap32(UInt32 nValue)
{
	return ((nValue << 24) | ((nValue << 8) & 0x00FF0000) | ((nValue >> 8) & 0x0000FF00) | (nValue >> 24));
}

/**
 * Swaps the endianness of a 32-bit value
 *
 * Reverses the order of the bytes of a 32-bit value
 *
 * @param[in] nValue Value to switch endianness of
 *
 * @return nValue with its endianness swapped
 */
inline Int32 EndianSwap32(Int32 nValue)
{
	return (Int32)EndianSwap32((UInt32)nValue);
}

/**
 * Swaps the endianness of a 64-bit value
 *
 * Reverses the order of the bytes of a 64-bit value
 *
 * @param[in] nValue Value to switch endianness of
 *
 * @return nValue with its endianness swapped
 */
inline UInt64 EndianSwap64(UInt64 nValue)
{
	return ((nValue << 56) |
			((nValue << 40) & 0x00FF000000000000ull) |
			((nValue << 24) & 0x0000FF0000000000ull) |
			((nValue <<  8) & 0x000000FF00000000ull) |
			((nValue >>  8) & 0x00000000FF000000ull) |
			((nValue >> 24) & 0x0000000000FF0000ull) |
			((nValue >> 40) & 0x000000000000FF00ull) |
			(nValue >> 56));
}

/**
 * Swaps the endianness of a 64-bit value
 *
 * Reverses the order of the bytes of a 64-bit value
 *
 * @param[in] nValue Value to switch endianness of
 *
 * @return nValue with its endianness swapped
 */
inline Int64 EndianSwap64(Int64 nValue)
{
	return (Int64)EndianSwap64((UInt64)nValue);
}

/**
 * Tests if the current system is running on a little-endian
 *        architecture
 *
 * @return True if the system is little-endian, or false if the system is
 *         big-endian
 */
inline Bool IsSystemLittleEndian()
{
	UByte bytes[2] = {0x01, 0x00};

	return *(UShort *)bytes == 0x0001;
}

/**
 * Tests if the current system is running on a big-endian
 *        architecture
 *
 * @return True if the system is big-endian, or false if the system is
 *         little-endian
 */
inline Bool IsSystemBigEndian()
{
	return !IsSystemLittleEndian();
}

/**
 * Rounds an integer up to the next power of 2
 *
 * Rounds an integer up to the next power of 2.  For example, the output is 16
 * for any input in between 9 and 16 inclusive.  The output is 0 for an input of
 * 0.
 *
 * @param[in] nValue Value to round up to the next power of 2
 *
 * @return The smallest power of 2 greater than or equal to nValue
 */
inline UInt32 RoundUpToPowerOf2(UInt32 nValue)
{
	// This is to make sure that exact powers of 2 don't go up to the next
	// power of 2
	nValue--;

	// This saturates all of the bits below the highest bit set, i.e. if the
	// input is 0001xxxx, the output is 00011111
	nValue |= nValue >> 1;
	nValue |= nValue >> 2;
	nValue |= nValue >> 4;
	nValue |= nValue >> 8;
	nValue |= nValue >> 16;

	// Now go from 00011111 to 00100000
	nValue++;

	return nValue;
}

/**
 * @return The length of s in bytes, excluding the null terminator.
 *
 * This is a Seoul wrapper around strlen to cast results to a UInt32.
 */
inline UInt32 StrLen(Byte const* s)
{
	return (UInt32)strlen(s);
}

extern Bool g_bInMain;

/**
 * Returns true if the app is currently in its main function, false otherwise. Can be
 * used to determine when we are after/before static initialization/destruction.
 */
inline Bool IsInMainFunction()
{
	return g_bInMain;
}

/**
 * Set if the app is currently in its main function - use correctly. Call immediately upon
 * entering main.
 */
inline void BeginMainFunction()
{
	g_bInMain = true;
}

/**
 * Set if the app is currently in its main function - use correctly. Call immediately upon
 * leaving main.
 */
inline void EndMainFunction()
{
	g_bInMain = false;
}

// Convenience for pre-declarations.
template <typename T, UInt32 SIZE> class FixedArray;
namespace Reflection { class Any; }

// Array used for method invocation - max of
// 15 arguments, this is an arbitrary
// but reasonable limit.
namespace Reflection { typedef FixedArray<Any, 15u> MethodArguments; }

} // namespace Seoul

// Whether or not we are allowing cheats in this build
#if (!SEOUL_SHIP || defined(SEOUL_PROFILING_BUILD))
#	define SEOUL_ENABLE_CHEATS 1
#else
#	define SEOUL_ENABLE_CHEATS 0
#endif

// Combines two values - useful for combining a macro name to another constant that will then
// be evaluated as a macro.
#define SEOUL_CONCAT_IMPLB(a) a
#define SEOUL_CONCAT_IMPL(a,b) SEOUL_CONCAT_IMPLB(a##b)
#define SEOUL_CONCAT(a,b) SEOUL_CONCAT_IMPL(a,b)

// Simple macro that removes parentheses from its argument - used to unpack
// macros of the form SEOUL_FOO((a, b), (c, d), (d, e)).
#define SEOUL_REMOVE_PARENTHESES_HELPER(...) __VA_ARGS__
#define SEOUL_REMOVE_PARENTHESES_I(macro, arg) macro arg
#define SEOUL_REMOVE_PARENTHESES(arg) SEOUL_REMOVE_PARENTHESES_I(SEOUL_REMOVE_PARENTHESES_HELPER, arg)

// Whether or not log output is enabled in the current build.
#define SEOUL_LOGGING_ENABLED !SEOUL_SHIP

// Whether or not benchmarking tests should be compiled into the current build.
#define SEOUL_BENCHMARK_TESTS (!SEOUL_SHIP)

// Whether or not unit tests should be compiled into the current build.
#define SEOUL_UNIT_TESTS (!SEOUL_SHIP)

// Whether developer UI should consider this a mobile platform or not (expect
// touch input and possible other adjustments).
#define SEOUL_DEVUI_MOBILE (SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_IOS)

// Whether or not automated testing should be compiled into the current build.
#if (!SEOUL_SHIP || defined(SEOUL_PROFILING_BUILD))
#define SEOUL_AUTO_TESTS 1
#else
#define SEOUL_AUTO_TESTS 0
#endif

#if !SEOUL_BUILD_FOR_DISTRIBUTION // Always enabled unless in a distribution branch.
#	define SEOUL_ENABLE_DEV_UI 1
#elif (!SEOUL_SHIP || defined(SEOUL_PROFILING_BUILD))
#	define SEOUL_ENABLE_DEV_UI 1
#else
#	define SEOUL_ENABLE_DEV_UI 0
#endif

// These macros are used to generate unique symbol names for SEOUL_STATIC_ASSERT,
// using the __COUNTER__ macro.  We need 2 macros since if we used only 1, we
// would get the name foo__COUNTER__ instead of foo0, foo1, etc.  See section
// 16.3.1 of the C++ standard.
#define SEOUL_MACRO_JOIN2(x, y) x ## y
#define SEOUL_MACRO_JOIN(x, y)  SEOUL_MACRO_JOIN2(x, y)

#endif // include guard
