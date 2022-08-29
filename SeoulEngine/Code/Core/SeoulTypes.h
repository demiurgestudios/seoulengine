/**
 * \file SeoulTypes.h
 * \brief Low-level header file, in most cases, include Prereqs.h
 * instead.
 *
 * The primary purpose of this file is to define aliases for basic
 * builtin types into the Seoul namespace.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef	SEOUL_TYPES_H
#define	SEOUL_TYPES_H

#include "StandardPlatformIncludes.h"
#include "StaticAssert.h"
#include <stddef.h>

///////////////////////////////////////////////////////////////////////////////

namespace Seoul
{

// Aliases for standard <inttypes.h>.
typedef int8_t   Int8;
typedef uint8_t  UInt8;
typedef int16_t  Int16;
typedef uint16_t UInt16;
typedef int32_t  Int32;
typedef uint32_t UInt32;
typedef int64_t  Int64;
typedef uint64_t UInt64;
// Hard size float types - check for odd platforms.
typedef float    Float32;
typedef double   Float64;
SEOUL_STATIC_ASSERT(sizeof(Float32) == 4);
SEOUL_STATIC_ASSERT(sizeof(Float64) == 8);

#if SEOUL_PLATFORM_WINDOWS

// If true, wchar_t is 16-bits and will
// be typed to the WChar16 type.
#define SEOUL_WCHAR_T_IS_2_BYTES 1

typedef wchar_t                WChar16;
typedef long                   Atomic32Type;
typedef long long              Atomic64Type;
typedef unsigned int           PerThreadStorageIndexType;
typedef UInt32                 ThreadIdType;

#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX

#if defined(__MINGW32__)

#define SEOUL_WCHAR_T_IS_2_BYTES 1
typedef wchar_t		WChar16;
SEOUL_STATIC_ASSERT_MESSAGE(sizeof(wchar_t) == 2, "SeoulEngine is coded expecting wide characters to be 16-bits when compiling with MinGW.");

#else

// wchar_t is 32-bits on Android and iOS.
#define SEOUL_WCHAR_T_IS_2_BYTES 0
typedef uint16_t	WChar16;
SEOUL_STATIC_ASSERT_MESSAGE(sizeof(wchar_t) == 4, "SeoulEngine is coded expecting wide characters to be 32-bits when compiling for Android and iOS.");

#endif

typedef uint32_t Atomic32Type;
typedef uint64_t Atomic64Type;
#if SEOUL_PLATFORM_IOS
typedef long     PerThreadStorageIndexType;
#else // !(SEOUL_PLATFORM_IOS)
typedef int32_t  PerThreadStorageIndexType;
#endif // /!(SEOUL_PLATFORM_IOS)
typedef uint64_t ThreadIdType;
#else
#error "Define for the current platform."
#endif
///////////////////////////////////////////////////////////////////////////////

typedef char    Byte;
typedef Int8    SByte;
typedef UInt8   UByte;

typedef bool    Bool;

typedef Int16   Short;
typedef UInt16  UShort;
typedef Int32   Int;
typedef UInt32  UInt;
typedef Int64   LongInt;
typedef UInt64  ULongInt;

typedef Float32 Float;
typedef Float64 Double;

/**
 * Byte is a character for must purposes -- it's safe to copy strings of Bytes,
 * concatenate them, take the length of them, iterate over them looking for
 * ASCII characters, and so on.  The only things that are _not_ safe with UTF-8
 * strings is to assume that the number of bytes is equal to the number of
 * characters that will ultimately be displayed (or have been input), or to
 * go byte by byte when iterating over characters for font rendering or input
 * processing.  For determining what actual code points the string consists of
 * (including non-ASCII code points, which are represented as arbitrary-length
 * sequences of non-ASCII characters), you must use a string iterator, which
 * will give you UniChars.
 */
typedef Int32   UniChar;

///////////////////////////////////////////////////////////////////////////////
/**
 * Used by MemoryManager to track memory usage and budgeted
 * usage in various memory categories.
 */
class MemoryBudgets
{
public:
	enum Type
	{
		Analytics,
		Animation,
		Animation2D,
		Animation3D,
		Audio,
		Commerce,
		Compression,
		Config,
		Content,
		Cooking,
		Coroutines,
		Curves,
		DataStore,
		DataStoreData,
		// WARNING, WARNING: By design, the Debug memory type is ignored during memory leak detection.
		// This allows this type to be used for certain objects that are intentionally left allocated
		// until memory leak detection is complete (i.e. MapFile). Do not use this group unless you
		// want an object to have this characteristic.
		Debug,
		Developer,
		DevUI,
		Editor,
		Encryption,
		Falcon,
		Fx,
		Game,
		HString,
		Input,
		Io,
		Jobs,
		Navigation,
		Network,
		None,
		OperatorNew,
		OperatorNewArray,
		Particles,
		Performance,
		Persistence,
		Physics,
		Profiler,
		Reflection,
		RenderCommandStream,
		Rendering,
		Saving,
		Scene,
		SceneComponent,
		SceneObject,
		Scripting,
		SpatialSorting,
		StateMachine,
		Strings,
		TBD,
		TBDContainer,
		Threading,
		UIData,
		UIDebug,
		UIRawMemory,
		UIRendering,
		UIRuntime,
		Unknown,
		Video,

		// TODO: Unintended consequence of SEOUL_SPEC_TEMPLATE_TYPE(). Defining SEOUL_SPEC_TEMPLATE_TYPE()
		// for missing templated container specializations (e.g. Vector<>) is easiest by just copying the
		// value from a link failure, which unfortunately defines memory budgets by integer value
		// (e.g. Vector<int, 21>). As such, maintaining this enum in lexographical order breaks every
		// instance of SEOUL_SPEC_TEMPLATE_TYPE() that was defined with a raw integer.
		Event,
		FalconFont,

		FirstType = Analytics,
		LastType = FalconFont,

		TYPE_COUNT
	};

	static const char* ToString(Type eType);

private:
	// Not constructable - static class.
	MemoryBudgets();
	SEOUL_DISABLE_COPY(MemoryBudgets);
}; // class MemoryBudgets

///////////////////////////////////////////////////////////////////////////////

// Declare container types here, since the default value
// can only be specified once, and we don't always want
// to include the corresponding header when forward declaring
// a container.
template <typename T, int MEMORY_BUDGETS = MemoryBudgets::TBDContainer>
class AtomicRingBuffer;

template <typename T, int MEMORY_BUDGETS = MemoryBudgets::TBDContainer>
class List;

template <typename T, int MEMORY_BUDGETS = MemoryBudgets::TBDContainer>
class Queue;

template <typename T, int MEMORY_BUDGETS = MemoryBudgets::TBDContainer>
class Vector;

} // namespace Seoul

///////////////////////////////////////////////////////////////////////////////

#endif // include guard
