/**
 * \file MapFileDbgHelp.cpp
 * \brief Implementation of IMapFile using the DbgHelp library to load debug
 * information from PDB files.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "MapFileDbgHelp.h"

#if SEOUL_ENABLE_STACK_TRACES && SEOUL_PLATFORM_WINDOWS

#include "Platform.h"
#include "SeoulMath.h"
#include "ThreadId.h"

#include <dbghelp.h>

// Printf format string for printing out pointers and pointer-sized integers
// with leading 0's.  Note that %p is implementation-defined and may not
// necessarily have leading 0's.
#ifdef _WIN64
#define SEOUL_POINTER_FORMAT "0x%016llx"
#else
#define SEOUL_POINTER_FORMAT "0x%08x"
#endif

namespace Seoul
{

/**
 * Initializes the symbol store
 */
MapFileDbgHelp::MapFileDbgHelp()
{
	SEOUL_ASSERT(IsMainThread());

	// Load symbols lazily and load line number information
	SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);

	// Enumerate all modules (DLLs) in the current process and try to find their
	// corresponding PDB files.
	SEOUL_VERIFY(SymInitialize(GetCurrentProcess(), nullptr, TRUE));
}

/**
 * Deinitializes the symbol store
 */
MapFileDbgHelp::~MapFileDbgHelp()
{
	SEOUL_ASSERT(IsMainThread());

	SEOUL_VERIFY(SymCleanup(GetCurrentProcess()));
}

/**
 * Attempts to populate the output buffer with the function name for the given function address.
 */
Bool MapFileDbgHelp::QueryFunctionName(size_t zAddress, Byte *sFunctionName, size_t zMaxNameLen)
{
	// A SYMBOL_INFO struct has a flexible array member at the end for the name,
	// so zMaxNameLen must be at least big enough for the base SYMBOL_INFO structure
	// and a null terminator.
	if (zMaxNameLen <= sizeof(SYMBOL_INFO))
	{
		return false;
	}
	// Setup.
	SYMBOL_INFO *pSymbolInfo = (SYMBOL_INFO *)sFunctionName;
	pSymbolInfo->SizeOfStruct = sizeof(SYMBOL_INFO);
	pSymbolInfo->MaxNameLen = (ULONG)(zMaxNameLen - sizeof(SYMBOL_INFO) - 1);

	// Try to look up symbol and line information
	DWORD64 nUnusedFunctionDisplacement;
	if (FALSE == SymFromAddr(GetCurrentProcess(), (DWORD64)zAddress, &nUnusedFunctionDisplacement, pSymbolInfo))
	{
		return false;
	}

	// SymFromAddr doesn't null-terminate the result if the symbol name is
	// too long
	pSymbolInfo->Name[zMaxNameLen-1] = 0;

	// Populate - truncate and move into place.
	auto sInputName = pSymbolInfo->Name;
	auto const zNameSize = strlen(sInputName) + 1;
	auto const zSize = Min(zMaxNameLen - 1, zNameSize - 1);
	memmove(sFunctionName, sInputName, zSize);
	sFunctionName[zSize] = '\0';
	return true;
}

/**
 * Attempts to populate the file and line info for the function at the given address.
 */
Bool MapFileDbgHelp::QueryLineInfo(size_t zAddress, Byte *sFileName, size_t zMaxNameLen, UInt32* puLineNumber)
{
	// Lookup.
	IMAGEHLP_LINE64 lineInfo;
	lineInfo.SizeOfStruct = sizeof(lineInfo);
	DWORD nUnusedLineDisplacement;
	if (FALSE == ::SymGetLineFromAddr64(GetCurrentProcess(), (DWORD64)zAddress, &nUnusedLineDisplacement, &lineInfo))
	{
		return false;
	}

	// Populate.
	if (zMaxNameLen > 0)
	{
		auto sInputFileName = lineInfo.FileName;
		auto const zFileNameSize = strlen(sInputFileName) + 1;
		auto const zSize = Min(zMaxNameLen - 1, zFileNameSize - 1);
		memcpy(sFileName, sInputFileName, zSize);
		sFileName[zSize] = '\0';
	}
	if (nullptr != puLineNumber)
	{
		*puLineNumber = lineInfo.LineNumber;
	}

	return true;
}

/**
 * Resolves the given address to a function name and other useful data
 */
void MapFileDbgHelp::ResolveFunctionAddress(size_t zAddress, Byte *sFunctionName, size_t zMaxNameLen)
{
	// A SYMBOL_INFO struct has a flexible array member at the end for the name,
	// so reserve space for the name.
	Byte aSymbolBytes[sizeof(SYMBOL_INFO) + IMapFile::kMaxFunctionNameLength];
	SYMBOL_INFO *pSymbolInfo = (SYMBOL_INFO *)aSymbolBytes;
	pSymbolInfo->SizeOfStruct = sizeof(SYMBOL_INFO);
	pSymbolInfo->MaxNameLen = IMapFile::kMaxFunctionNameLength;

	IMAGEHLP_LINE64 lineInfo;
	lineInfo.SizeOfStruct = sizeof(lineInfo);

	DWORD64 nFunctionDisplacement;
	DWORD nLineDisplacement;

	// Try to look up symbol and line information
	if (SymFromAddr(GetCurrentProcess(), (DWORD64)zAddress, &nFunctionDisplacement, pSymbolInfo))
	{
		// SymFromAddr doesn't null-terminate the result if the symbol name is
		// too long
		pSymbolInfo->Name[IMapFile::kMaxFunctionNameLength] = 0;

		if (SymGetLineFromAddr64(GetCurrentProcess(), (DWORD64)zAddress, &nLineDisplacement, &lineInfo))
		{
			// Got a symbol name and a line number, excellent!
			SNPRINTF(sFunctionName, zMaxNameLen,
					 SEOUL_POINTER_FORMAT " %s+0x%x [%s:%d+0x%x]",
					 zAddress,
					 pSymbolInfo->Name,
					 (UInt32)nFunctionDisplacement,
					 lineInfo.FileName,
					 lineInfo.LineNumber,
					 nLineDisplacement);
		}
		else
		{
			// Got a symbol name but no line number
			SNPRINTF(sFunctionName, zMaxNameLen,
					 SEOUL_POINTER_FORMAT " %s+0x%x",
					 zAddress,
					 pSymbolInfo->Name,
					 (UInt32)nFunctionDisplacement);
		}
	}
	else
	{
		// Couldn't get a symbol name.  Boo!  Just try to get the module name
		// that it came from.
		ResolveAddressToModule(zAddress, sFunctionName, zMaxNameLen);
	}
}

/**
 * Helper structure for ResolveAddressToModule
 */
struct ResolveAddressToModuleHelper
{
	DWORD64 m_uAddress;
	Byte *m_sFunctionName;
	size_t m_zMaxNameLen;
	Bool m_bFoundModule;
};

/**
 * Tries to resolve the given symbol to a module name and offset
 */
void MapFileDbgHelp::ResolveAddressToModule(size_t zAddress, Byte *sFunctionName, size_t zMaxNameLen)
{
	ResolveAddressToModuleHelper data;
	data.m_uAddress = zAddress;
	data.m_sFunctionName = sFunctionName;
	data.m_zMaxNameLen = zMaxNameLen;
	data.m_bFoundModule = false;

	if (!EnumerateLoadedModules64(GetCurrentProcess(), &MapFileDbgHelp::EnumLoadedModulesProc, &data) ||
		!data.m_bFoundModule)
	{
		// If we couldn't find the module, be sad.
		SNPRINTF(sFunctionName, zMaxNameLen, SEOUL_POINTER_FORMAT " [???]", zAddress);
	}
}

/**
 * Callback for EnumerateLoadedModules64
 */
BOOL CALLBACK MapFileDbgHelp::EnumLoadedModulesProc(PCSTR sModuleName, DWORD64 uModuleBase, ULONG uModuleSize, PVOID pUserContext)
{
	ResolveAddressToModuleHelper *pData = (ResolveAddressToModuleHelper *)pUserContext;

	// If the address lies within this module, print out the module name
	if (pData->m_uAddress >= uModuleBase &&
		pData->m_uAddress <  uModuleBase + uModuleSize)
	{
		SNPRINTF(pData->m_sFunctionName, pData->m_zMaxNameLen,
				 SEOUL_POINTER_FORMAT " %s+0x%llx",
				 (size_t)pData->m_uAddress,
				 sModuleName,
				 pData->m_uAddress - uModuleBase);
		pData->m_bFoundModule = true;
		return FALSE;  // Stop enumeration
	}

	return TRUE;  // Continue enumeration
}

} // namespace Seoul

#endif  // SEOUL_ENABLE_STACK_TRACES && !SEOUL_SHIP
