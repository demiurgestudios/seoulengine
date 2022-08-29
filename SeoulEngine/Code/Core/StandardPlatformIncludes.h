/**
 * \file StandardPlatformIncludes.h
 * \brief Low-level header file - in general, include Prereqs.h instead.
 *
 * This file's primary responsibility is to detect and normalize
 * platform configuration details into SeoulEngine constants.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef	STANDARD_PLATFORM_INCLUDES_H
#define STANDARD_PLATFORM_INCLUDES_H

#include "BuildConfig.h"
#include "BuildDistro.h"

#include <string.h>

///////////////////////////////////////////////////////////////////////////////

// inttypes.h needs this to defined PRId64 and PRIu64
#define __STDC_FORMAT_MACROS

// stdint.h needs this to use certain constants in C++
#define __STDC_LIMIT_MACROS

#include <cstdarg>
#include <inttypes.h>
#include <stdint.h>

// Bit width detection.
#if UINTPTR_MAX == 0xFFFFFFFF
#	define SEOUL_PLATFORM_32         1
#	define SEOUL_PLATFORM_64         0
#elif UINTPTR_MAX == 0xFFFFFFFFFFFFFFFF
#	define SEOUL_PLATFORM_32         0
#	define SEOUL_PLATFORM_64         1
#else
#	error "Unexpected bit width."
#endif

// Create our own platform defines, which are unique for each platform

#if (defined(_WIN32) && !defined(__MINGW32__))

#define SEOUL_PLATFORM_WINDOWS    1
#define SEOUL_PLATFORM_IOS        0
#define SEOUL_PLATFORM_ANDROID    0
#define SEOUL_PLATFORM_LINUX      0

#elif defined(__APPLE__)

#define SEOUL_PLATFORM_WINDOWS    0
#define SEOUL_PLATFORM_ANDROID    0
#define SEOUL_PLATFORM_LINUX      0

#	include "TargetConditionals.h"
#	if TARGET_IPHONE_SIMULATOR || defined(TARGET_OS_IPHONE)  // TARGET_IPHONE_SIMULATOR is always defined in XCode, will be 0 or 1.  See TargetConditionals.h
#	define SEOUL_PLATFORM_IOS            1
#	else
#	error "Define your platform's constants"
#	endif

#elif defined(__ANDROID__)

#define SEOUL_PLATFORM_WINDOWS    0
#define SEOUL_PLATFORM_IOS        0
#define SEOUL_PLATFORM_ANDROID    1
#define SEOUL_PLATFORM_LINUX      0

#elif (defined(__linux__) || defined(__MINGW32__))

#define SEOUL_PLATFORM_WINDOWS    0
#define SEOUL_PLATFORM_IOS        0
#define SEOUL_PLATFORM_ANDROID    0
#define SEOUL_PLATFORM_LINUX      1

#else

#error "Define your platform's constants"

#endif

// Detect if AddressSanitizer (http://clang.llvm.org/docs/AddressSanitizer.html) is enabled.
// Used to disable certain engine chunks.
#if defined(__has_feature)
#	if __has_feature(address_sanitizer)
#		define SEOUL_ADDRESS_SANITIZER 1
#		if SEOUL_PLATFORM_IOS
#			define SEOUL_DISABLE_ADDRESS_SANITIZER __attribute__((no_sanitize_address))
#		else
#			define SEOUL_DISABLE_ADDRESS_SANITIZER __attribute__((no_sanitize("address")))
#		endif
#	else
#		define SEOUL_ADDRESS_SANITIZER 0
#		define SEOUL_DISABLE_ADDRESS_SANITIZER
#	endif
#else
#	define SEOUL_ADDRESS_SANITIZER 0
#	define SEOUL_DISABLE_ADDRESS_SANITIZER
#endif

// Define to 1 to enable the overhead of memory tooling - this
// includes additional size per allocation and possible thread
// contention and allocation count overhead (if memory leak
// tracking and detection is enabled at runtime).
#define SEOUL_ENABLE_MEMORY_TOOLING (!SEOUL_SHIP && SEOUL_PLATFORM_WINDOWS && !SEOUL_ADDRESS_SANITIZER && !SEOUL_EDITOR_AND_TOOLS && !SEOUL_BUILD_UE4)

// Define to 1 to enable script debugging support.
#define SEOUL_ENABLE_DEBUGGER_CLIENT (!SEOUL_SHIP)

// Flag controlling whether or not to run sanity checks on config data.
//
// These checks are application specific (currently, this flag
// is not referenced in engine code).
#define SEOUL_WITH_CONFIG_VERIFICATION !SEOUL_SHIP

// TODO: Rename these SEOUL_WITH_* macros. I'd like
// to firm up a convention that SEOUL_WITH_* exclusively refers
// to global build configuration macros defining whether certain
// optional features of SeoulEngine are included in the current
// application (e.g. physics or Facebook integration).

// Flag controlling native crash reporting state.
// Only enabled in Ship builds and only in "distro" (branch)
// builds.
#if (SEOUL_SHIP && !defined(SEOUL_PROFILING_BUILD))
#define SEOUL_WITH_NATIVE_CRASH_REPORTING 1
#else
#define SEOUL_WITH_NATIVE_CRASH_REPORTING 0
#endif

// Flag controlling if the Moriarty client is enabled
#define SEOUL_WITH_MORIARTY (!SEOUL_SHIP && !SEOUL_BUILD_UE4)

// Needed for STRTO set of macros
#if SEOUL_PLATFORM_WINDOWS
#include <stdlib.h>
#endif

// Macros for testing endianness at compile time
#if SEOUL_PLATFORM_WINDOWS || SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID
#define SEOUL_BIG_ENDIAN    0
#define SEOUL_LITTLE_ENDIAN 1
#elif SEOUL_PLATFORM_LINUX
// TODO: This can be either or under a Linux environment.
#define SEOUL_BIG_ENDIAN    0
#define SEOUL_LITTLE_ENDIAN 1
#else
#error "Define for this platform"
#endif

// Whether hot loading is available on this platform.
#if !SEOUL_SHIP
#define SEOUL_HOT_LOADING 1
#else
#define SEOUL_HOT_LOADING 0
#endif

// Macros for indicating to the linker that
// a something should be kept, even if it
// appears to not be referenced.
#if SEOUL_PLATFORM_WINDOWS
#define SEOUL_ALWAYS_USED
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
#define SEOUL_ALWAYS_USED __attribute__((used))
#else
#error "Define for this platform."
#endif

// Macros for indicating to the compiler that
// a static variable or function, which is unused
// in a module, is expected to be unused.
#if SEOUL_PLATFORM_WINDOWS
#define SEOUL_UNUSED
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
#define SEOUL_UNUSED __attribute__((unused))
#else
#error "Define for this platform."
#endif

// Macro for hinting to the compiler that a branch is likely to evaluated to true.
#if SEOUL_PLATFORM_WINDOWS
#define SEOUL_LIKELY(x) (x)
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
#define SEOUL_LIKELY(x) __builtin_expect(!!(x), 1)
#else
#error "Define for this platform."
#endif

// Macro for hinting to the compiler that a branch is not likely to evaluated to true.
#if SEOUL_PLATFORM_WINDOWS
#define SEOUL_UNLIKELY(x) (x)
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
#define SEOUL_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#error "Define for this platform."
#endif

// Macros for defining packed data structures
//
// NOTE: !defined(__clang__) here is to support
// include-what-you-use in our tools pipeline. It defines _MSVC
// and clang supports most VisualStudio quirks, but this is one
// that it does not.
#if (SEOUL_PLATFORM_WINDOWS && !defined(__clang__))
#define SEOUL_DEFINE_PACKED_STRUCT(definition) \
	__pragma(pack(push, 1)) \
	definition \
	__pragma(pack(pop))
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX || defined(__clang__)
#define SEOUL_DEFINE_PACKED_STRUCT(definition) \
	definition __attribute__((packed))
#else
#error "Define for this platform"
#endif

// Attributes to tag printf-style vararg functions for better compiler
// detection of errors
#if SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
#define SEOUL_PRINTF_FORMAT_ATTRIBUTE(stringIndex, firstToCheck) __attribute__((format(printf, stringIndex, firstToCheck)))
#elif SEOUL_PLATFORM_WINDOWS
#define SEOUL_PRINTF_FORMAT_ATTRIBUTE(stringIndex, firstToCheck)
#else
#error "Define for this platform"
#endif

/**
 * Preprocessor macros for declaring specific alignment of
 * a type.
 */
#if SEOUL_PLATFORM_WINDOWS
#define SEOUL_DECLARE_ALIGNMENT_TYPE(type, a) __declspec(align(a)) type
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
#define SEOUL_DECLARE_ALIGNMENT_TYPE(type, a) type __attribute__((aligned(a)))
#else
#error "Define for this platform."
#endif

/**
 * Preprocessor macros for hinting to the
 * compiler that a function should not be
 * inlined.
 */
#if SEOUL_PLATFORM_WINDOWS
#define SEOUL_NO_INLINE __declspec(noinline)
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
#define SEOUL_NO_INLINE __attribute__ ((noinline))
#else
#error "Define for this platform."
#endif

/**
 * Preprocessor macros for declaring a function as
 * a "no return" function, which means what it sounds like.
 * These functions are typically abort code paths or
 * the base of a coroutine.
 */
#if SEOUL_PLATFORM_WINDOWS
#define SEOUL_NO_RETURN __declspec(noreturn)
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
#define SEOUL_NO_RETURN __attribute__((noreturn))
#else
#error "Define for this platform."
#endif

/**
 * Preprocessor macro which indicates to the static analyzer on iOS
 * that the tagged function should be treated as if it never returns; however,
 * the compiler will still assume that it might return.
 */
#if SEOUL_PLATFORM_IOS
#define SEOUL_ANALYZER_NORETURN __attribute__((analyzer_noreturn))
#else
#define SEOUL_ANALYZER_NORETURN
#endif

#if SEOUL_PLATFORM_IOS && defined(__OBJC__)
// Indicates to the static analyzer that the tagged function returns an
// autoreleased Cocoa object (i.e. an object with a retain count of 0 but which is
// gauranteed to be valid until the next time the autorelease pool is drained)
#define SEOUL_RETURNS_AUTORELEASED __attribute__((ns_returns_autoreleased))

// Indicates to the static analyzer that the tagged function parameter (which
// must be an Objective-C object) is sent a -release message in the function.
#define SEOUL_NS_CONSUMED __attribute__((ns_consumed))

#else
#define SEOUL_RETURNS_AUTORELEASED
#define SEOUL_NS_CONSUMED
#endif

///////////////////////////////////////////////////////////////////////////////

// Standard headers

#if SEOUL_PLATFORM_WINDOWS

// Make sure we don't include useless stuff from <windows.h> that slows down compiling
// TODO: Lines commented out are functionality that we need in general. It may be useful
// to disable certain functionality in specific code modules, so these lines can be
// used as a reference.
#define NOGDICAPMASKS
//#define NOVIRTUALKEYCODES
// #define NOWINMESSAGES
// #define NOWINSTYLES
// #define NOSYSMETRICS
// #define NOMENUS
// #define NOICONS
#define NOKEYSTATES
// #define NOSYSCOMMANDS
#define NORASTEROPS
// #define NOSHOWWINDOW
#define OEMRESOURCE
#define NOATOM
// #define NOCLIPBOARD
#define NOCOLOR
#ifndef SEOUL_ENABLE_CTLMGR
#define NOCTLMGR
#endif
#define NODRAWTEXT
// #define NOGDI
#define NOKERNEL
// #define NOUSER
//#define NONLS
// #define NOMB
#define NOMEMMGR
#define NOMETAFILE
#define NOMINMAX
// #define NOMSG
#define NOOPENFILE
#define NOSCROLL
#define NOSERVICE
#define NOSOUND
// #define NOTEXTMETRIC
#define NOWH
// #define NOWINOFFSETS
#define NOCOMM
#define NOKANJI
#define NOHELP
#define NOPROFILER
#define NODEFERWINDOWPOS
#define NOMCX
#define WIN32_LEAN_AND_MEAN

// Target Windows Vista and later
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#elif _WIN32_WINNT < 0x0600
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#define SEOUL_THROW0() noexcept
#define SEOUL_THROWS(ignored)

#elif SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX || SEOUL_PLATFORM_IOS

#define SEOUL_THROW0() noexcept
#define SEOUL_THROWS(ignored)

#else

#error "Define for this platform."

#endif

///////////////////////////////////////////////////////////////////////////////

// Standard paths

#if SEOUL_PLATFORM_WINDOWS
#define DEFAULT_PATH		".\\"
#elif SEOUL_PLATFORM_IOS
#define DEFAULT_PATH		"./"
#elif SEOUL_PLATFORM_ANDROID
#define DEFAULT_PATH        "/"
#elif SEOUL_PLATFORM_LINUX
#define DEFAULT_PATH        "/"
#else
#error "Define your platform's default path"
#endif

///////////////////////////////////////////////////////////////////////////////

// Functions

#if SEOUL_PLATFORM_WINDOWS

#define FPRINTF(file, format, ...) fprintf_s((file), (format), ##__VA_ARGS__)
#define SSCANF(str, format, ...) sscanf_s((str), (format), ##__VA_ARGS__)
#define STRCMP_CASE_INSENSITIVE(strA, strB) _stricmp((strA), (strB))
#define STRNCMP_CASE_INSENSITIVE(strA, strB, n) _strnicmp((strA), (strB), (n))
#define STRNCPY(dest, src, size) strncpy_s((dest), (size), (src), _TRUNCATE)
#define STRNCAT(dest, src, size) strncat_s((dest), (size), (src), _TRUNCATE)
#define SNPRINTF(buffer, size, format, ...) _snprintf_s((buffer), (size), _TRUNCATE, (format), ##__VA_ARGS__)
#define STRTOD(str, end_ptr) strtod((str), (end_ptr))
#define STRTOINT64(str, end_ptr, base) _strtoi64((str), (end_ptr), (base))
#define STRTOUINT64(str, end_ptr, base) _strtoui64((str), (end_ptr), (base))
#define VA_COPY(dest_list, src_list) ((dest_list) = (src_list))
#define VSCPRINTF(format, args) _vscprintf((format), (args))
#define VSCWPRINTF(format, args) _vscwprintf((format), (args))
#define VSNPRINTF(buffer, size, format, args) _vsnprintf_s((buffer), (size), _TRUNCATE, (format), (args))
#define VSNWPRINTF(buffer, size, format, args) _vsnwprintf_s((buffer), (size), _TRUNCATE, (format), (args))
#define WSTRCMP_CASE_INSENSITIVE(strA, strB) _wcsicmp((strA), (strB))
#define WSTRNCPY(dest, src, size) wcsncpy_s((dest), (size), (src), _TRUNCATE)
#define WSTRLEN(str) wcslen((str))

#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX

// Implement of vscprintf for wide characters on platforms
// that don't support it.
int vscwprintf(wchar_t const* sFormat, va_list argList);
namespace Seoul { char* StrNCat(char* sDest, const char* sSrc, size_t zSize); }
namespace Seoul { char* StrNCopy(char* sDest, const char* sSrc, size_t zSize); }

#define FPRINTF(file, format, ...) fprintf((file), (format), ##__VA_ARGS__)
#define SSCANF(str, format, ...) sscanf((str), (format), ##__VA_ARGS__)
#define STRCMP_CASE_INSENSITIVE(strA, strB) strcasecmp((strA), (strB))
#define STRNCMP_CASE_INSENSITIVE(strA, strB, n) strncasecmp((strA), (strB), (n))
#define STRNCPY(dest, src, size) ::Seoul::StrNCopy((dest), (src), (size))
#define STRNCAT(dest, src, size) ::Seoul::StrNCat((dest), (src), (size))
#define SNPRINTF(buffer, size, format, ...) snprintf((buffer), (size), (format), ##__VA_ARGS__)
#define STRTOD(str, end_ptr) strtod((str), (end_ptr))
#define STRTOINT64(str, end_ptr, base) strtoimax((str), (end_ptr), (base))
#define STRTOUINT64(str, end_ptr, base) strtoumax((str), (end_ptr), (base))
#define VA_COPY(dest_list, src_list) va_copy(dest_list, src_list)
#define VSCPRINTF(format, args) vsnprintf(nullptr, 0, (format), (args))
#define VSCWPRINTF(format, args) vscwprintf((format), (args))
#define VSNPRINTF(buffer, size, format, args) vsnprintf((buffer), (size), (format), (args))
#define VSNWPRINTF(buffer, size, format, args) vswprintf((buffer), (size), (format), (args))
#define WSTRLEN(str) wcslen((str))
#else

#error "Define for this platform."

#endif

// Standard constants

#if SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
#include <errno.h>

namespace Seoul
{
	inline bool IsEBusy(int iErrorCode)
	{
		return (EBUSY == iErrorCode);
	}

	inline bool IsETimedOut(int iErrorCode)
	{
		return (ETIMEDOUT == iErrorCode);
	}
}
#endif

///////////////////////////////////////////////////////////////////////////////

#if SEOUL_PLATFORM_WINDOWS
#	define SEOUL_ALIGN_OF __alignof
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
#	define SEOUL_ALIGN_OF __alignof__
#else
#	error Define SEOUL_ALIGN_OF for this platform.
#endif

// Handle lack of inttypes.h header in older versions of MSVC.
#if defined(_MSC_VER) && _MSC_VER <= 1600
#	define PRId64 "I64d"
#	define PRIu64 "I64u"
#	define PRIx64 "I64x"
#elif defined(_MSC_VER) && _MSC_VER >= 1800
// windows inttypes.h actually does not need this macro defined right now
// for PRId64 and PRIu64, but maybe someday... futureproofing
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#endif

// Convenience EOL macro that can be easily concatenated with string literals.
#if SEOUL_PLATFORM_WINDOWS
#	define SEOUL_EOL "\r\n"
#else
#	define SEOUL_EOL "\n"
#endif

// Convenience macro for disabling a class's copy constructor and assignment operator.
#define SEOUL_DISABLE_COPY(name) \
	name(const name&) = delete; \
	name& operator=(const name&) = delete

// Include BuildFeatures.h at the end so it is always included elsewhere
// but has access to the platform macros.
#include "BuildFeatures.h"

// Flag controlling if we support encrypted save games on this platform
#define SEOUL_ENABLE_ENCRYPTED_SAVE_GAMES (SEOUL_WITH_OPENSSL || SEOUL_PLATFORM_IOS)

#endif // def STANDARD_SEOUL_PLATFORM_INCLUDES_H
