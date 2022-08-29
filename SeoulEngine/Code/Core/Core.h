/**
 * \file Core.h
 * \brief Primary header file for the core singleton. Contains
 * shared functionality that needs runtime initialization
 * (e.g. map file support for stack traces).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef CORE_H
#define CORE_H

#include "StandardPlatformIncludes.h"
#include "SeoulAssert.h"
#include "MemoryManager.h"
#include "SeoulTypes.h"

#include <stdio.h>

/**
 * Number of elements in a static array
 */
#define SEOUL_ARRAY_COUNT(array) (sizeof(array) / sizeof((array)[0]))

// If defined to 1, call stack captures are supported with
// a call to Core::GetCurrentCallStack(). The call stack
// addresses can be resolved to human readable names if
// a map file is present and loaded via Core::GetMapFile().
#if (SEOUL_PLATFORM_WINDOWS && (!SEOUL_SHIP || defined(SEOUL_PROFILING_BUILD))) || (!SEOUL_SHIP && !SEOUL_PLATFORM_ANDROID)
#	define SEOUL_ENABLE_STACK_TRACES 1
#else
#	define SEOUL_ENABLE_STACK_TRACES 0
#endif

namespace Seoul
{

// Forward declarations
struct CoreSettings;
class String;

#if SEOUL_ENABLE_STACK_TRACES
/**
 * Encompasses a map file, which can be used to
 * resolve function addresses into human readable names.
 */
class IMapFile
{
public:
	/**
	 * Maximum function name length - the bigger this value,
	 * the larger the map file will be in memory, so be careful.
	 */
	static const UInt32 kMaxFunctionNameLength = 96u;

	virtual ~IMapFile() {}

	// Start loading the map file asynchronously, if possible
	virtual void StartLoad() {}

	// Wait until the map file has finished loading
	virtual void WaitUntilLoaded() {}

	// Attempts to populate the output buffer with the function name for the given function address.
	virtual Bool QueryFunctionName(size_t zAddress, Byte *sFunctionName, size_t zMaxNameLen) { return false; }
	// Attempts to populate the file and line info for the function at the given address.
	virtual Bool QueryLineInfo(size_t zAddress, Byte *sFileName, size_t zMaxNameLen, UInt32* puLineNumber) { return false; }

	// Convert the function at address zAddress to a human readable name.
	virtual void ResolveFunctionAddress(size_t zAddress, Byte *sFunctionName, size_t zMaxNameLen) = 0;
}; // class IMapFile
#endif // /SEOUL_ENABLE_STACK_TRACES

class Core
{
public:
	static void Initialize(const CoreSettings& settings);
	static void ShutDown();

#if SEOUL_ENABLE_STACK_TRACES
	// Get the current stack information - uses a
	// raw pointer and size argument because, in many cases
	// where a stack is desired, we don't want to dynamically
	// allocate memory.
	static UInt32 GetCurrentCallStack(
		UInt32 nNumberOfEntriesToSkip,
		UInt32 nMaxNumberOfAddressesToGet,
		size_t* pOutBuffer);

	// Gets the current stack trace and writes it out as a string into sBuffer,
	// which is of size zSize.
	static void GetStackTraceString(Byte *sBuffer, size_t zSize);

	// Prints the given stack trace into the given buffer
	static void PrintStackTraceToBuffer(
		Byte *sBuffer,
		size_t zSize,
		Byte const* sPerLinePrefix,
		const size_t *aCallStack,
		UInt32 nFrames);

	static void PrintStackTraceToBuffer(
		Byte *sBuffer,
		size_t zSize,
		const size_t *aCallStack,
		UInt32 nFrames)
	{
		PrintStackTraceToBuffer(sBuffer, zSize, "", aCallStack, nFrames);
	}

	// Get the currently set map file - can be nullptr.
	static IMapFile* GetMapFile();

	// Set the currently set map file.
	//
	// NOTE: Core takes ownership of the MapFile - if non-nullptr, the MapFile
	// will be destroyed just before exit, to allow it to be used
	// in verbose memory leak detection (if enabled). It is stil safe
	// to explicitly call Core::SetMapFile(nullptr) and to destroy the
	// MapFile on your own, but this will disable stack traces in
	// verbose memory leak detection.
	static void SetMapFile(IMapFile* pMapFile);

#endif // /#if SEOUL_ENABLE_STACK_TRACES
};

namespace PlatformPrint
{

enum class Type
{
	kInfo,
	kError,
	kFailure,
	kWarning,
};

// Print a string to this platform's standard out. Always,
// use with care and for special exceptions.
void PrintString(Type eType, const String& sMessage);
void PrintString(Type eType, const Byte *sMessage);
void PrintStringFormatted(Type eType, const Byte *sFormat, ...) SEOUL_PRINTF_FORMAT_ATTRIBUTE(2, 3);
void PrintStringFormattedV(Type eType, const Byte *sFormat, va_list args) SEOUL_PRINTF_FORMAT_ATTRIBUTE(2, 0);

// Print a string that will als be split on newlines (and a final newline appended, if needed).
void PrintStringMultiline(Type eType, const Byte *sPrefix, const String& sIn);
void PrintStringMultiline(Type eType, const Byte *sPrefix, const Byte *sMessage);
void PrintStringFormattedMultiline(Type eType, const Byte *sPrefix, const Byte* sFormat, ...) SEOUL_PRINTF_FORMAT_ATTRIBUTE(3, 4);
void PrintStringFormattedVMultiline(Type eType, const Byte *sPrefix, const Byte* sFormat, va_list args) SEOUL_PRINTF_FORMAT_ATTRIBUTE(3, 0);

// Print a string to the debugger console (not named OutputDebugString because
// the Windows headers #define that name to either OutputDebugStringA or
// OutputDebugStringW, and while we could #undef it, it's easier just to avoid
// that problem). Only enabled in Ship.
void PrintDebugString(Type eType, const String& sMessage);
void PrintDebugString(Type eType, const Byte *sMessage);
void PrintDebugStringFormatted(Type eType, const Byte *sFormat, ...) SEOUL_PRINTF_FORMAT_ATTRIBUTE(2, 3);
void PrintDebugStringFormattedV(Type eType, const Byte *sFormat, va_list args) SEOUL_PRINTF_FORMAT_ATTRIBUTE(2, 0);

} // namespace PlatformPrint

/** Platform enumeration constants used for tagging cooked data etc. */
enum class Platform
{
	kPC,
	kIOS,
	kAndroid,
	kLinux,

	SEOUL_PLATFORM_COUNT,
	SEOUL_PLATFORM_FIRST = kPC,
	SEOUL_PLATFORM_LAST = kLinux,
};

// Names of the platforms, e.g. "PC"
extern const Byte* kaPlatformNames[(UInt32)Platform::SEOUL_PLATFORM_COUNT];
// Macros names used for effect compiling, e.g. "SEOUL_PLATFORM_WINDOWS"
extern const Byte* kaPlatformMacroNames[(UInt32)Platform::SEOUL_PLATFORM_COUNT];

const Platform keCurrentPlatform =
#if SEOUL_PLATFORM_WINDOWS
		Platform::kPC;
#elif SEOUL_PLATFORM_IOS
Platform::kIOS;
#elif SEOUL_PLATFORM_ANDROID
Platform::kAndroid;
#elif SEOUL_PLATFORM_LINUX
Platform::kLinux;
#else
#error "Define for this platform."
#endif

// Gets the name of the current platform, e.g. "PC"
const Byte* GetCurrentPlatformName();

// Test if the current OS environment is 64-bit - whether or not
// the current executable is 64-bits.
//
// NOTE: If this function is not implemented for the current platform
// or cannot otherwise issue a query, it will always default to returning
// false.
Bool IsOperatingSystem64Bits();

extern Bool g_bRunningAutomatedTests;
extern Bool g_bRunningUnitTests;
extern Bool g_bHeadless;

} // namespace Seoul

#endif // include guard
