/**
 * \file PixelFormat.h
 * \brief Enum defining valid pixel formats for color targets and textures.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef PIXEL_FORMAT_H
#define PIXEL_FORMAT_H

#include "Prereqs.h"

namespace Seoul
{

class HString;

enum class PixelFormat : Int32
{
	kInvalid = 0,
	kR8G8B8,
	kA8R8G8B8,
	kA8R8G8B8sRGB,
	kX8R8G8B8,
	kR5G6B5,
	kX1R5G5B5,
	kA1R5G5B5,
	kA4R4G4B4,
	kR3G3B2,
	kA8,
	kA8L8,
	kP8,
	kX4R4G4B4,
	kA2B10G10R10,
	kA8B8G8R8,
	kX8B8G8R8,
	kG16R16,
	kA2R10G10B10,
	kA2B10G10R10F,
	kA16B16G16R16,
	kA16B16G16R16_SIGNED,
	kA16B16G16R16_SIGNED_EXP1,
	kA16B16G16R16_UNSIGNED_EXP1,
	kA16B16G16R16_SIGNED_EXP2,
	kA16B16G16R16_UNSIGNED_EXP2,
	kA16B16G16R16_SIGNED_EXP3,
	kA16B16G16R16_UNSIGNED_EXP3,
	kR16F,
	kD16I,
	kG16R16F,
	kA16B16G16R16F,
	kR32F,
	kR32F_AS_R16F_EXPAND,
	kG32R32F,
	kG32R32F_AS_G16R16,
	kA32B32G32R32F,
	kA32B32G32R32,
	kDXT1,
	kDXT2,
	kDXT3,
	kDXT4,
	kDXT5,
	kPVRTC_RGB_4BPPV1,
	kPVRTC_RGB_2BPPV1,
	kPVRTC_RGBA_4BPPV1,
	kPVRTC_RGBA_2BPPV1,
	kETC1_RGB8,

	PF_COUNT
};

PixelFormat HStringToPixelFormat(HString hstring);

// Populate ruBytesPerPixel with the total bytes of a single pixel of format eFormat.
// ruBytesPerPixel is left unchanged if this function returns false - this function returns
// false for pixel formats that do not have a valid bytes per pixel (compressed formats).
Bool PixelFormatBytesPerPixel(PixelFormat eFormat, UInt& ruBytesPerPixel);

// Populate ruPitch with the contiguous pitch in bytes of eFormat with uWidth.
// ruPitch is left unchanged if this function returns false - this function returns
// false for pixel formats that do not have a valid pitch (compressed formats).
Bool GetPitchForPixelFormat(Int32 iWidth, PixelFormat eFormat, UInt& ruPitch);

// Returns the total size in bytes of contiguously packed image data of uWidth, uHeight,
// and format eFormat.
UInt GetDataSizeForPixelFormat(Int32 iWidth, Int32 iHeight, PixelFormat eFormat);

/**
 * @return True if a format has an alpha channel, false otherwise.
 */
inline Bool PixelFormatHasAlpha(PixelFormat eFormat)
{
	switch (eFormat)
	{
	case PixelFormat::kA8R8G8B8:
	case PixelFormat::kA8R8G8B8sRGB:
	case PixelFormat::kA1R5G5B5:
	case PixelFormat::kA4R4G4B4:
	case PixelFormat::kA8:
	case PixelFormat::kA8L8:
	case PixelFormat::kA2B10G10R10:
	case PixelFormat::kA8B8G8R8:
	case PixelFormat::kA2R10G10B10:
	case PixelFormat::kA2B10G10R10F:
	case PixelFormat::kA16B16G16R16:
	case PixelFormat::kA16B16G16R16_SIGNED:
	case PixelFormat::kA16B16G16R16_SIGNED_EXP1:
	case PixelFormat::kA16B16G16R16_UNSIGNED_EXP1:
	case PixelFormat::kA16B16G16R16_SIGNED_EXP2:
	case PixelFormat::kA16B16G16R16_UNSIGNED_EXP2:
	case PixelFormat::kA16B16G16R16_SIGNED_EXP3:
	case PixelFormat::kA16B16G16R16_UNSIGNED_EXP3:
	case PixelFormat::kA16B16G16R16F:
	case PixelFormat::kA32B32G32R32F:
	case PixelFormat::kA32B32G32R32:
	case PixelFormat::kDXT1:
	case PixelFormat::kDXT4:
	case PixelFormat::kDXT5:
	case PixelFormat::kPVRTC_RGBA_4BPPV1:
	case PixelFormat::kPVRTC_RGBA_2BPPV1:
		return true;
	default:
		return false;
	};
}

inline Bool PixelFormatIsRGB(PixelFormat eFormat)
{
	switch (eFormat)
	{
	case PixelFormat::kA2B10G10R10:
	case PixelFormat::kA8B8G8R8:
	case PixelFormat::kX8B8G8R8:
	case PixelFormat::kA2B10G10R10F:
	case PixelFormat::kA16B16G16R16:
	case PixelFormat::kA16B16G16R16_SIGNED:
	case PixelFormat::kA16B16G16R16_SIGNED_EXP1:
	case PixelFormat::kA16B16G16R16_UNSIGNED_EXP1:
	case PixelFormat::kA16B16G16R16_SIGNED_EXP2:
	case PixelFormat::kA16B16G16R16_UNSIGNED_EXP2:
	case PixelFormat::kA16B16G16R16_SIGNED_EXP3:
	case PixelFormat::kA16B16G16R16_UNSIGNED_EXP3:
	case PixelFormat::kA16B16G16R16F:
	case PixelFormat::kA32B32G32R32F:
	case PixelFormat::kA32B32G32R32:
		return true;
	default:
		return false;
	};
}

/**
 * @return True if eFormat is a compressed pixel format, false otherwise.
 */
inline Bool IsCompressedPixelFormat(PixelFormat eFormat)
{
	switch (eFormat)
	{
	case PixelFormat::kDXT1: // fall-through
	case PixelFormat::kDXT2: // fall-through
	case PixelFormat::kDXT3: // fall-through
	case PixelFormat::kDXT4: // fall-through
	case PixelFormat::kDXT5: // fall-through
	case PixelFormat::kPVRTC_RGB_2BPPV1: // fall-through
	case PixelFormat::kPVRTC_RGB_4BPPV1: // fall-through
	case PixelFormat::kPVRTC_RGBA_2BPPV1: // fall-through
	case PixelFormat::kPVRTC_RGBA_4BPPV1: // fall-through
	case PixelFormat::kETC1_RGB8:
		return true;
	default:
		return false;
	};
}

} // namespace Seoul

#endif // include guard
