/**
 * \file ImagePNG.cpp
 * \brief Implementation of LoadPngFromMemory(), used by
 * LoadImageFromMemory() for .png files.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Image.h"
#include "ScopedAction.h"

#include <spng.h>

namespace Seoul
{

// Memory allocation hooks for PNG to hook into Seoul Engine
// memory allocation.
static void* SpngMalloc(size_t size) { return MemoryManager::Allocate(size, MemoryBudgets::Rendering); }
static void* SpngRealloc(void* ptr, size_t size) { return MemoryManager::Reallocate(ptr, size, MemoryBudgets::Rendering); }
static void* SpngCalloc(size_t count, size_t size)
{
	auto const zTotalSize = count * size;
	void* p = MemoryManager::Allocate(zTotalSize, MemoryBudgets::Rendering);
	memset(p, 0, zTotalSize);
	return p;
}
static void SpngFree(void* p) { MemoryManager::Deallocate(p); }

// Fixed structure for hooking memory allocation hooks.
static const spng_alloc s_Alloc =
{
	SpngMalloc,
	SpngRealloc,
	SpngCalloc,
	SpngFree,
};

UInt8* LoadPngFromMemory(
	void const* pBuffer,
	UInt32 uSizeInBytes,
	Int32* piWidth /*= nullptr*/,
	Int32* piHeight /*= nullptr*/,
	UInt32* puOutputSizeInBytes /*= nullptr*/,
	Bool* pbOriginalHasAlpha /*= nullptr*/)
{
	// SPNG processing context hooked into Seoul Engine
	// memory allocation hooks.
	auto pCtxt = spng_ctx_new2((spng_alloc*)&s_Alloc, 0);
	auto const deferred(MakeDeferredAction([&]()
	{
		spng_ctx_free(pCtxt);
		pCtxt = nullptr;
	}));

	// Set the PNG buffer.
	if (0 != spng_set_png_buffer(pCtxt, pBuffer, (size_t)uSizeInBytes))
	{
		return nullptr;
	}

	// Read the header, needed for various queries.
	spng_ihdr hdr;
	memset(&hdr, 0, sizeof(hdr));
	if (0 != spng_get_ihdr(pCtxt, &hdr))
	{
		return nullptr;
	}

	// Get the output size in bytes - always RGBA8.
	size_t outSize = 0;
	if (0 != spng_decoded_image_size(pCtxt, SPNG_FMT_RGBA8, &outSize))
	{
		return nullptr;
	}

	// Parse the PNG.
	auto p = MemoryManager::Allocate(outSize, MemoryBudgets::Rendering);
	if (0 != spng_decode_image(pCtxt, p, outSize, SPNG_FMT_RGBA8, 0))
	{
		MemoryManager::Deallocate(p);
		return nullptr;
	}

	// On success, apply any out parameters.
	if (piWidth)
	{
		*piWidth = hdr.width;
	}
	if (piHeight)
	{
		*piHeight = hdr.height;
	}
	if (puOutputSizeInBytes)
	{
		*puOutputSizeInBytes = (UInt32)outSize;
	}
	if (pbOriginalHasAlpha)
	{
		// PNG contains an alpha channel unless it is explicitly
		// single channel grayscale or triplet (RGB) truecolor.
		*pbOriginalHasAlpha = (
			hdr.color_type != SPNG_COLOR_TYPE_GRAYSCALE &&
			hdr.color_type != SPNG_COLOR_TYPE_TRUECOLOR);
	}

	// Finished.
	return (UInt8*)p;
}

} // namespace Seoul
