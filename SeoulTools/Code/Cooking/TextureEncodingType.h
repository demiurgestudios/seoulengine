/**
 * \file TextureEncodingType.h
 * \brief Utility enum of the TextureCook implementation.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef TEXTURE_ENCODING_TYPE
#define TEXTURE_ENCODING_TYPE

#include "DDS.h"
#include "PixelFormat.h"
#include "Prereqs.h"

namespace Seoul::Cooking
{

namespace TextureEncodingType
{

enum Enum
{
	kA8R8G8B8 = 0,
	kA8B8G8R8 = 1,
	kDXT1 = 2,
	kDXT5 = 3,
	kETC1 = 4,
	kETC1Clustered = 5,
};

static inline Bool IsCompressedType(Enum e)
{
	switch (e)
	{
	case kA8R8G8B8: return false;
	case kA8B8G8R8: return false;
	case kDXT1: return true;
	case kDXT5: return true;
	case kETC1: return true;
	case kETC1Clustered: return true;
	default:
		SEOUL_FAIL("Out-of-sync enum.");
		return false;
	};
}

static inline PixelFormat ToPixelFormat(Enum e)
{
	switch (e)
	{
	case kA8R8G8B8: return PixelFormat::kA8R8G8B8;
	case kA8B8G8R8: return PixelFormat::kA8B8G8R8;
	case kDXT1: return PixelFormat::kDXT1;
	case kDXT5: return PixelFormat::kDXT5;
	case kETC1: return PixelFormat::kETC1_RGB8;
	case kETC1Clustered: return PixelFormat::kETC1_RGB8;
	default:
		SEOUL_FAIL("Out-of-sync enum.");
		return PixelFormat::kInvalid;
	};
}

static inline DdsPixelFormat ToDdsPixelFormat(Enum e)
{
	switch (e)
	{
	case kA8R8G8B8: return kDdsPixelFormatA8R8G8B8;
	case kA8B8G8R8: return kDdsPixelFormatA8B8G8R8;
	case kDXT1: return kDdsPixelFormatDXT1;
	case kDXT5: return kDdsPixelFormatDXT5;
	case kETC1: return kDdsPixelFormatETC1_RGB8;
	case kETC1Clustered: return kDdsPixelFormatETC1_RGB8;
	default:
	{
		SEOUL_FAIL("Out-of-sync enum.");
		DdsPixelFormat ret;
		memset(&ret, 0, sizeof(ret));
		return ret;
	}
	};
}

} // namespace TextureEncodingType

} // namespace Seoul::Cooking

#endif // include guard
