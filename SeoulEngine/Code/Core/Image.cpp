/**
 * \file Image.cpp
 * \brief Utility functions for loading image data of various formats
 * (e.g. PNG, TGA, etc.) into RGBA bytes.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Image.h"

// Necessary to include these here, so that stb_image.h does not include them again inside the
// Seoul namespace.
#include <assert.h>
#ifdef _MSC_VER
#include <emmintrin.h>
#include <intrin.h>
#endif
#include <math.h>
#include <memory.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace Seoul
{

#	define STB_IMAGE_IMPLEMENTATION
#	define STBI_ASSERT(x) SEOUL_ASSERT(x)
#	define STBI_MALLOC(sz) Seoul::MemoryManager::Allocate((size_t)(sz), MemoryBudgets::Io)
#	define STBI_REALLOC(p,sz) Seoul::MemoryManager::Reallocate((p), (size_t)(sz), MemoryBudgets::Io)
#	define STBI_FREE(p) Seoul::MemoryManager::Deallocate((p))
#	ifndef _MSC_VER
#		define STBI_NO_SIMD 1
#	endif
#	include <stb_image.h>

static UInt8* StbLoadImageFromMemory(
	void const* pBuffer,
	UInt32 uSizeInBytes,
	Int32* piWidth,
	Int32* piHeight,
	UInt32* puOutputSizeInBytes,
	Bool* pbOriginalHasAlpha)
{
	Int32 iWidth = 0;
	Int32 iHeight = 0;
	Int32 iOriginalComponents = 0;
	UInt8* pReturn = (UInt8*)stbi_load_from_memory(
		(stbi_uc const*)pBuffer,
		(Int)uSizeInBytes,
		&iWidth,
		&iHeight,
		&iOriginalComponents,
		STBI_rgb_alpha);
	if (pReturn)
	{
		if (piWidth)
		{
			*piWidth = iWidth;
		}

		if (piHeight)
		{
			*piHeight = iHeight;
		}

		if (puOutputSizeInBytes)
		{
			*puOutputSizeInBytes = (4u * (UInt32)iWidth * (UInt32)iHeight);
		}

		if (pbOriginalHasAlpha)
		{
			// 1 = grayscale.
			// 3 = rgb.
			*pbOriginalHasAlpha = (1 != iOriginalComponents && 3 != iOriginalComponents);
		}
	}

	return pReturn;
}

/** PNG header for dispatching to LoadPngFromMemory() vs. StbLoadImageFromMemory(). */
static const UInt8 kaPngSignature[8] = { 137u, 80u, 78u, 71u, 13u, 10u, 26u, 10u };

UInt8* LoadImageFromMemory(
	void const* pBuffer,
	UInt32 uSizeInBytes,
	Int32* piWidth /*= nullptr*/,
	Int32* piHeight /*= nullptr*/,
	UInt32* puOutputSizeInBytes /*= nullptr*/,
	Bool* pbOriginalHasAlpha /*= nullptr*/)
{
	// If PNG data, dispatch to specialized LoadPngFromMemory().
	if (sizeof(kaPngSignature) <= uSizeInBytes &&
		0 == memcmp(kaPngSignature, pBuffer, sizeof(kaPngSignature)))
	{
		return LoadPngFromMemory(pBuffer, uSizeInBytes, piWidth, piHeight, puOutputSizeInBytes, pbOriginalHasAlpha);
	}
	else
	{
		return StbLoadImageFromMemory(pBuffer, uSizeInBytes, piWidth, piHeight, puOutputSizeInBytes, pbOriginalHasAlpha);
	}
}

void FreeImage(UInt8*& rpImageBuffer)
{
	UInt8* pImageBuffer = rpImageBuffer;
	rpImageBuffer = nullptr;

	if (nullptr != pImageBuffer)
	{
		// NOTE: For correctness, we should be calling
		// stbi_image_free() here. We rely on internal
		// knowledge that stbi_image_free() just calls STBI_FREE,
		// which we redefine to MemoryManager::Deallocate() above.
		//
		// This is then necessary to allow the same free call
		// to be used for data returned by SPNG (.png files)
		// and STB (all other image formats).
		MemoryManager::Deallocate(pImageBuffer);
	}
}

} // namespace Seoul
