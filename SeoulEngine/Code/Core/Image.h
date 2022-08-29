/**
 * \file Image.h
 * \brief Utility functions for loading image data of various formats
 * (e.g. PNG, TGA, etc.) into RGBA bytes.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef IMAGE_H
#define IMAGE_H

#include "Prereqs.h"

namespace Seoul
{

/*
 * Function to load an image from an in-memory buffer.
 * @param[in] pBuffer Image data.
 * @param[in] uSizeInBytes Length of the input data in bytes.
 * @param[out] piWidth (optional) Width of the loaded image in pixels on success.
 * @param[out] piHeight (optional) Height of the loaded image in pixels on success.
 *
 * @return nullptr on failure, or the data in RGBA 8-bit format on success.
 */
UInt8* LoadImageFromMemory(
	void const* pBuffer,
	UInt32 uInputSizeInBytes,
	Int32* piWidth = nullptr,
	Int32* piHeight = nullptr,
	UInt32* puOutputSizeInBytes = nullptr,
	Bool* pbOriginalHasAlpha = nullptr);

/**
 * Specialized variant for PNG data of LoadImageFromMemory.
 *
 * NOTE: Used when LoadImageFromMemory() detects a PNG - calling this
 * function directly is useful for correctness (input must be a PNG).
 */
UInt8* LoadPngFromMemory(
	void const* pBuffer,
	UInt32 uInputSizeInBytes,
	Int32* piWidth = nullptr,
	Int32* piHeight = nullptr,
	UInt32* puOutputSizeInBytes = nullptr,
	Bool* pbOriginalHasAlpha = nullptr);

/*
 * Releases the allocated image buffer that was created when loading
 */
void FreeImage(UInt8*& rpImageBuffer);

} // namespace Seoul

#endif // include guard
