/**
 * \file TextureCook.cpp
 * \brief Implementation of texture compression (DXT1, DXT5, and ETC formats) as
 *        well as encoding raw RGBA or BGRA data into an encoded DDS container.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Algorithms.h"
#include "DDS.h"
#include "FixedArray.h"
#include "TextureCookCrunch.h"
#include "TextureCookISPC.h"
#include "Prereqs.h"
#include "SeoulMath.h"
#include "TextureEncodingType.h"

namespace Seoul::Cooking
{

/** Quality level of ETC1 clustered compression. */
static const Int32 kiClusteredQuality = 200;

/** Compress a square RGBAu8 image into compressed block data of the specified type. */
static UInt8* CompressTextureData(
	UInt8 const* pIn,
	UInt32 uWidth,
	UInt32 uHeight,
	TextureEncodingType::Enum eType)
{
	auto const ePixelFormat = TextureEncodingType::ToPixelFormat(eType);
	UInt32 const uSizeInBytes = GetDataSizeForPixelFormat(uWidth, uHeight, ePixelFormat);
	UInt8* pReturn = (UInt8*)MemoryManager::Allocate(uSizeInBytes, MemoryBudgets::Cooking);

	ispc::Image image;
	memset(&image, 0, sizeof(image));
	image.m_iHeight = uHeight;
	image.m_pData = (UInt8*)pIn;
	image.m_iPitchInBytes = (uWidth * 4u);
	image.m_iWidth = uWidth;

	using namespace TextureEncodingType;
	switch (eType)
	{
		// DXT1 just compresses directly.
	case kDXT1:
		CompressorISPC::CompressBlocksDXT1(image, pReturn);
		break;

		// DXT5 just compresses directly.
	case kDXT5:
		CompressorISPC::CompressBlocksDXT5(image, pReturn);
		break;

		// ETC1 just compresses directly.
	case kETC1:
		CompressorISPC::CompressBlocksETC1(image, pReturn);
		break;

		// Invalid.
	default:
		SEOUL_FAIL("Out-of-sync enum.");
		break;
	};

	return pReturn;
}

/**
 * Encase a texture in a DDS file container, and optionally apply compression.
 * For compression, input image must be:
 * - RGBA format.
 * - power of two width and height.
 * - a minimum of 4x4 pixels.
 */
static UInt8* EncodeTexture(
	UInt8 const* pIn,
	UInt32 uWidth,
	UInt32 uHeight,
	TextureEncodingType::Enum eType,
	UInt32& ruReturnDataSizeInBytes)
{
	// Compute the data size in bytes.
	auto const ePixelFormat = TextureEncodingType::ToPixelFormat(eType);
	UInt32 const uDataSizeInBytes = GetDataSizeForPixelFormat(uWidth, uHeight, ePixelFormat);
	UInt32 uPitchInBytes = 0u;
	(void)GetPitchForPixelFormat(uWidth, ePixelFormat, uPitchInBytes);

	// Data, compressed or passed through as needed.
	UInt8* pData = nullptr;
	if (IsCompressedType(eType))
	{
		// If a clustered type, returned data will already be a crn blob,
		// so just return (no DDS header).
		if (eType == TextureEncodingType::kETC1Clustered)
		{
			UInt8* pReturn = nullptr;
			UInt32 uOutSize = 0u;
			if (!CompressorCrunch::CompressBlocksETC1(
				pIn,
				uWidth,
				uHeight,
				kiClusteredQuality,
				pReturn,
				uOutSize))
			{
				return nullptr;
			}
			else
			{
				ruReturnDataSizeInBytes = uOutSize;
				return pReturn;
			}
		}
		// Otherwise, compress and fall through.
		else
		{
			pData = CompressTextureData(pIn, uWidth, uHeight, eType);
		}
	}
	else if (TextureEncodingType::kA8R8G8B8 == eType)
	{
		// Swap RB and pass through.
		pData = (UInt8*)MemoryManager::Allocate(uDataSizeInBytes, MemoryBudgets::Cooking);
		memcpy(pData, pIn, uDataSizeInBytes);

		for (UInt32 i = 0u; i < uDataSizeInBytes; i += 4u)
		{
			Swap(pData[i + 0], pData[i + 2]);
		}
	}
	else
	{
		// Allocate and pass through.
		pData = (UInt8*)MemoryManager::Allocate(uDataSizeInBytes, MemoryBudgets::Cooking);
		memcpy(pData, pIn, uDataSizeInBytes);
	}

	// nullptr used to indicate failure, so return immediately.
	if (nullptr == pData)
	{
		return pData;
	}

	// Initialize a DDS container header.
	DdsHeader header;
	memset(&header, 0, sizeof(header));

	// Populate relevant fields for an DXT image.
	header.m_MagicNumber = kDdsMagicValue;
	header.m_Size = (sizeof(header) - sizeof(header.m_MagicNumber));
	header.m_HeaderFlags = kDdsHeaderFlagsTexture | kDdsHeaderFlagsLinearSize;
	header.m_Height = uHeight;
	header.m_Width = uWidth;
	header.m_PitchOrLinearSize = (TextureEncodingType::IsCompressedType(eType) ? uDataSizeInBytes : uPitchInBytes);
	header.m_Depth = 1;
	header.m_MipMapCount = 1;
	header.m_PixelFormat = TextureEncodingType::ToDdsPixelFormat(eType);
	header.m_SurfaceFlags = kDdsSurfaceFlagsTexture;

	// Resize the data buffer and insert the header at the beginning.
	pData = (UInt8*)MemoryManager::Reallocate(pData, uDataSizeInBytes + sizeof(header), MemoryBudgets::TBD);
	memmove((UInt8*)pData + sizeof(header), pData, uDataSizeInBytes);
	memcpy(pData, &header, sizeof(header));

	// Populate riReturnDataSizeInBytes with the total data size, including
	// both container data and header.
	ruReturnDataSizeInBytes = (UInt32)(uDataSizeInBytes + sizeof(header));

	// Return the fully constructed container data (DDS header + compressed data).
	return pData;
}

/**
 * Encase a texture in a DDS file container, and optionally apply compression.
 * For compression, input image must be:
 * - RGBA format.
 * - power of two width and height.
 * - a minimum of 4x4 pixels.
 */
Bool EncodeTexture(
	UInt8 const* pInputRGBAImageData,
	UInt32 uInputWidth,
	UInt32 uInputHeight,
	TextureEncodingType::Enum eType,
	Byte** ppOutputData,
	UInt32* puOutputDataSizeInBytes)
{
	// Attempt to encode the data.
	UInt32 uOutputDataSizeInBytes = 0;
	UInt8* pOutputData = EncodeTexture(
		pInputRGBAImageData,
		uInputWidth,
		uInputHeight,
		eType,
		uOutputDataSizeInBytes);

	// Done.
	*ppOutputData = (Byte*)pOutputData;
	*puOutputDataSizeInBytes = uOutputDataSizeInBytes;
	return true;
}

} // namespace Seoul::Cooking
