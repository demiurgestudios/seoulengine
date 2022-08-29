/**
 * \file MapFileLinux.h
 * \brief Implementation of IMapFile using Linux system API functionality
 * to resolve debug symbols.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef MAP_FILE_LINUX_H
#define MAP_FILE_LINUX_H

#include "Core.h"
#include "Prereqs.h"

namespace Seoul
{

#if SEOUL_ENABLE_STACK_TRACES && (SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_LINUX)

/**
* Class used to resolve function addresses using Linux system calls.
*/
class MapFileLinux SEOUL_SEALED : public IMapFile
{
public:
	MapFileLinux();
	virtual ~MapFileLinux();

	virtual void ResolveFunctionAddress(
		size_t zAddress,
		Byte* sFunctionName,
		size_t zMaxNameLen) SEOUL_OVERRIDE;
};

#endif  // /SEOUL_ENABLE_STACK_TRACES && (SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_LINUX)

} // namespace Seoul

#endif // include guard
