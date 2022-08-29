/**
 * \file SeoulCrc32.h
 * \brief Implementation of a 32-bit cyclic redundancy check.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SEOUL_CRC32_H
#define SEOUL_CRC32_H

#include "Prereqs.h"

namespace Seoul
{

extern UInt32 const kaCrc32Table[256];

/**
 * @return A 32-bit CRC value for the block of data described by pData and zSizeInBytes.
 */
inline static UInt32 GetCrc32(UInt32 uCrc32, UInt8 const* pData, size_t zSizeInBytes)
{
	UInt8 const* const pEnd = (pData + zSizeInBytes);

	while (pData < pEnd)
	{
		UInt8 const uValue = *pData;
		++pData;

		uCrc32 = (uCrc32 >> 8u) ^ kaCrc32Table[(uValue) ^ ((uCrc32) & 0x000000FF)];
	}

	return uCrc32;
}

/**
 * Syntactic sugar for starting a Crc32 computation.
 */
inline static UInt32 GetCrc32(UInt8 const* pData, size_t zSizeInBytes)
{
	return GetCrc32(0xFFFFFFFF, pData, zSizeInBytes);
}

} // namespace Seoul

#endif // include guard
