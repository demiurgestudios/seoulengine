/**
 * \file EncryptXXTEA.h
 * \brief Implementation of XXTEA encryption.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ENCRYPT_XXTEA_H
#define ENCRYPT_XXTEA_H

#include "Prereqs.h"

namespace Seoul::EncryptXXTEA
{

/** Fixed length of the simple encrypt (XXTEA) key in UInt32 (fixed length of 4, or 16 bytes). */
static const UInt32 kuKeyLengthInUInt32 = 4u;

/** The XXTEA max constant (magic number) computation. */
inline static UInt32 GetMax(
	UInt32 uE,
	UInt32 uP,
	UInt32 uY,
	UInt32 uZ,
	UInt32 uSum,
	UInt32 const aKey[kuKeyLengthInUInt32])
{
	return (((uZ >> 5u) ^ (uY << 2)) + ((uY >> 3) ^ (uZ << 4))) ^ ((uSum ^ uY) + (aKey[(uP & 3) ^ uE] ^ uZ));
}

/** The XXTEA delta constant (magic number). */
static const UInt32 kuDelta = 0x9e3779b9;

/**
 * Decrypt with the xxtea algorithm: http://www.movable-type.co.uk/scripts/xxtea.pdf
 *
 * Pre: Data must be UInt32 aligned and a multiple of 2u * sizeof(UInt32).
 */
inline static void DecryptInPlace(
	UInt32* pData,
	UInt32 uSizeInNumberOfUInt32,
	UInt32 const aKey[kuKeyLengthInUInt32])
{
	// uSizeInUInt32 must be >= 2 to encrypt.
	if (uSizeInNumberOfUInt32 < 2u)
	{
		return;
	}

	UInt32 uIterations = (6u + (52u / uSizeInNumberOfUInt32));
	UInt32 const uN = (uSizeInNumberOfUInt32 - 1u);
	UInt32 uSum = (uIterations * kuDelta);
	UInt32 uY = pData[0u];
	UInt32 uZ = 0u;

	do
	{
		UInt32 const uE = ((uSum >> 2u) & 3u);

		UInt32 uP = uN;
		for (; uP > 0u; --uP)
		{
			uZ = pData[uP - 1u];
			uY = (pData[uP] -= GetMax(uE, uP, uY, uZ, uSum, aKey));
		}

		uZ = pData[uN];
		uY = (pData[0u] -= GetMax(uE, uP, uY, uZ, uSum, aKey));
		uSum -= kuDelta;
	} while (0u != --uIterations);
}

/**
 * Encrypt with the xxtea algorithm: http://www.movable-type.co.uk/scripts/xxtea.pdf
 *
 * Pre: Data must be UInt32 aligned and a multiple of 2u * sizeof(UInt32).
 * Pre: uSizeInUInt32 must be >= 2
 */
inline static void EncryptInPlace(
	UInt32* pData,
	UInt32 uSizeInNumberOfUInt32,
	UInt32 const aKey[kuKeyLengthInUInt32])
{
	// uSizeInUInt32 must be >= 2 to encrypt.
	if (uSizeInNumberOfUInt32 < 2u)
	{
		return;
	}

	UInt32 uIterations = (6u + (52u / uSizeInNumberOfUInt32));
	UInt32 const uN = (uSizeInNumberOfUInt32 - 1u);
	UInt32 uSum = 0u;
	UInt32 uY = 0u;
	UInt32 uZ = pData[uN];

	do
	{
		uSum += kuDelta;
		UInt32 const uE = ((uSum >> 2u) & 3u);

		UInt32 uP = 0u;
		for (; uP < uN; ++uP)
		{
			uY = pData[uP + 1u];
			uZ = (pData[uP] += GetMax(uE, uP, uY, uZ, uSum, aKey));
		}

		uY = pData[0u];
		uZ = (pData[uN] += GetMax(uE, uP, uY, uZ, uSum, aKey));
	} while (0u != --uIterations);
}

} // namespace Seoul::EncryptXXTEA

#endif // include guard
