/**
 * \file MapFileLinux.cpp
 * \brief Implementation of IMapFile using Linux system API functionality
 * to resolve debug symbols.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "MapFileLinux.h"

#if SEOUL_ENABLE_STACK_TRACES && (SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_LINUX)

#include "Platform.h"
#include "ScopedAction.h"
#include "SeoulString.h"
#include "ThreadId.h"

#include <cxxabi.h>
#include <execinfo.h>
#include <stdlib.h>

namespace Seoul
{

MapFileLinux::MapFileLinux()
{
	SEOUL_ASSERT(IsMainThread());
}

MapFileLinux::~MapFileLinux()
{
}

/** Start of a mangled symbol. */
static const String ksMangledSymbolStart("_Z");

/** Checks for valid symbol characters. */
static inline Bool IsValidChar(UniChar ch)
{
	if (ch >= 'a' && ch <= 'z') { return true; }
	if (ch >= 'A' && ch <= 'Z') { return true; }
	if (ch >= '0' && ch <= '9') { return true; }
	if ('_' == ch) { return true; }

	return false;
}

/** Utility to demangle the symbol name returned by backtrace_symbols(). */
static void Demangle(String& rs)
{
	UInt32 u = 0u;
	while (u < rs.GetSize())
	{
		// Start.
		auto const uStart = rs.Find(ksMangledSymbolStart, u);
		if (String::NPos == uStart)
		{
			return;
		}

		// End.
		auto uEnd = uStart + 2u;
		while (uEnd < rs.GetSize() && IsValidChar(rs[uEnd]))
		{
			++uEnd;
		}

		// Demangle.
		String sMangled(rs.Substring(uStart, (uEnd - uStart)));

		Int iResult = -1;
		auto sDemangled = __cxxabiv1::__cxa_demangle(sMangled.CStr(), nullptr, nullptr, &iResult);
		auto const deferred(MakeDeferredAction([&]() { free(sDemangled); }));
		if (0 == iResult)
		{
			rs = rs.Substring(0, uStart) + sDemangled + rs.Substring(uEnd);
			u = (uStart + StrLen(sDemangled));
		}
		else
		{
			u = uEnd; // Skip _Z.
		}
	}
}

/**
 * Resolves the given address to a function name and other useful data
 */
void MapFileLinux::ResolveFunctionAddress(
	size_t zAddress,
	Byte* sFunctionName,
	size_t zMaxNameLen)
{
	SEOUL_STATIC_ASSERT(sizeof(size_t) == sizeof(void*));

	void** ppAddress = (void**)(&zAddress);
	Byte** ppSymbols = backtrace_symbols(ppAddress, 1);

	if (nullptr != ppSymbols && nullptr != *ppSymbols)
	{
		String s(*ppSymbols);
		Demangle(s);
		SNPRINTF(sFunctionName, zMaxNameLen, "%s", s.CStr());
	}
	else
	{
		SNPRINTF(sFunctionName, zMaxNameLen, "0x%08zx", zAddress);
	}

	free(ppSymbols);
}

} // namespace Seoul

#endif // /SEOUL_ENABLE_STACK_TRACES && (SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_LINUX)
