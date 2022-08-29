/**
 * \file D3DCommonEffect.cpp
 * \brief Shared utility for parsing a D3D9/D3D11 universal Effect
 * file.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "D3DCommonEffect.h"
#include "StreamBuffer.h"

namespace Seoul
{

static const UInt32 kuPCEffectSignature = 0x4850A36F;
static const Int32 kiPCEffectVersion = 1;

Bool GetEffectData(
	Bool bD3D11,
	Byte const* pIn,
	UInt32 uIn,
	Byte const*& rpOut,
	UInt32& ruOut)
{
	if (uIn < 24u)
	{
		return false;
	}

	UInt32 uSignature = 0u;
	memcpy(&uSignature, pIn + 0, sizeof(uSignature));
#if SEOUL_BIG_ENDIAN
	EndianSwap32(uSignature);
#endif // /#if SEOUL_BIG_ENDIAN

	if (uSignature != kuPCEffectSignature)
	{
		return false;
	}

	Int32 iVersion = 0;
	memcpy(&iVersion, pIn + 4, sizeof(iVersion));
#if SEOUL_BIG_ENDIAN
	EndianSwap32(iVersion);
#endif // /#if SEOUL_BIG_ENDIAN

	if (iVersion != kiPCEffectVersion)
	{
		return false;
	}

	// Get offsets.
	UInt32 uOffset = 0u;
	UInt32 uSize = 0u;
	if (bD3D11)
	{
		memcpy(&uOffset, pIn + 8, sizeof(uOffset));
		memcpy(&uSize, pIn + 12, sizeof(uSize));
	}
	else
	{
		memcpy(&uOffset, pIn + 16, sizeof(uOffset));
		memcpy(&uSize, pIn + 20, sizeof(uSize));
	}

	// Failure if offset or size are invalid.
	if (uOffset < 24u || uOffset + uSize > uIn)
	{
		return false;
	}

	// Otherwise, done.
	ruOut = uSize;
	rpOut = pIn + uOffset;
	return true;
}

} // namespace Seoul
