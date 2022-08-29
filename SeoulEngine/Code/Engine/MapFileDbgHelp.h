/**
 * \file MapFileDbgHelp.h
 * \brief Implementation of IMapFile using the DbgHelp library to load debug
 * information from PDB files.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef MAP_FILE_DBG_HELP_H
#define MAP_FILE_DBG_HELP_H

#include "Core.h"
#include "Prereqs.h"

#if SEOUL_ENABLE_STACK_TRACES && SEOUL_PLATFORM_WINDOWS

#include "Platform.h"

namespace Seoul
{

/**
 * Class used to resolve function addresses using the DbgHelp library
 */
class MapFileDbgHelp SEOUL_SEALED : public IMapFile
{
public:
	MapFileDbgHelp();
	virtual ~MapFileDbgHelp();

	virtual Bool QueryFunctionName(
		size_t zAddress,
		Byte *sFunctionName,
		size_t zMaxNameLen) SEOUL_OVERRIDE;
	virtual Bool QueryLineInfo(
		size_t zAddress, 
		Byte *sFileName, 
		size_t zMaxNameLen,
		UInt32* puLineNumber) SEOUL_OVERRIDE;

	virtual void ResolveFunctionAddress(size_t zAddress, Byte *sFunctionName, size_t zMaxNameLen) SEOUL_OVERRIDE;

private:
	// Tries to resolve the given symbol to a module name and offset
	void ResolveAddressToModule(size_t zAddress, Byte *sFunctionName, size_t zMaxNameLen);

	// Callback for EnumerateLoadedModules64
	static BOOL CALLBACK EnumLoadedModulesProc(PCSTR sModuleName, DWORD64 uModuleBase, ULONG uModuleSize, PVOID pUserContext);
};

} // namespace Seoul

#endif  // SEOUL_ENABLE_STACK_TRACES && !SEOUL_SHIP

#endif // include guard
