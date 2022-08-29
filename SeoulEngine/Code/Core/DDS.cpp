/**
 * \file DDS.cpp
 * \brief Structures and helper functions for reading DDS (Direct Draw Surface) image
 * files, used as the runtime texture format for OGLES2.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Algorithms.h"
#include "DDS.h"

namespace Seoul::DDS
{

struct PixelFormatEntry
{
	DdsPixelFormat m_eDdsPixelFormat;
	PixelFormat m_ePixelFormat;
};

struct DxgiFormatEntry
{
	DxgiFormat m_eDxgiFormat;
	PixelFormat m_ePixelFormat;
};

// Format conversions currently supported.
static const PixelFormatEntry ksaPixelFormatEntries[] =
{
	{ kDdsPixelFormatDXT1, PixelFormat::kDXT1 },
	{ kDdsPixelFormatDXT3, PixelFormat::kDXT3 },
	{ kDdsPixelFormatDXT5, PixelFormat::kDXT5 },
	{ kDdsPixelFormatPVRTC_RGBA_2BPPV1, PixelFormat::kPVRTC_RGBA_2BPPV1 },
	{ kDdsPixelFormatPVRTC_RGBA_4BPPV1, PixelFormat::kPVRTC_RGBA_4BPPV1 },
	{ kDdsPixelFormatETC1_RGB8, PixelFormat::kETC1_RGB8 },
	{ kDdsPixelFormatA8R8G8B8, PixelFormat::kA8R8G8B8 },
	{ kDdsPixelFormatA8B8G8R8, PixelFormat::kA8B8G8R8 },
	{ kDdsPixelFormatA1R5G5B5, PixelFormat::kA1R5G5B5 },
	{ kDdsPixelFormatA4R4G4B4, PixelFormat::kA4R4G4B4 },
	{ kDdsPixelFormatR8G8B8, PixelFormat::kR8G8B8 },
	{ kDdsPixelFormatR5G6B5, PixelFormat::kR5G6B5 },
};

// Format conversion from DXGI formats currently supported.
static const DxgiFormatEntry ksaDxgiFormatEntries[] =
{
	{ DxgiFormat::kB8G8R8A8_UNORM, PixelFormat::kA8R8G8B8 },
	{ DxgiFormat::kR8G8B8A8_UNORM, PixelFormat::kA8B8G8R8 },
};

/** Shared utility, populates a DdsHeader and possibly DdsHeaderDX10. Can fail if data is invalid. */
static Bool GetHeaders(void const* pData, UInt32 uSizeInBytes, DdsHeader& rHeader, DdsHeaderDX10& rHeaderDX10, Bool& rbDX10)
{
	memset(&rHeader, 0, sizeof(rHeader));
	memset(&rHeaderDX10, 0, sizeof(rHeaderDX10));

	// Size must be at least as big as the side of the DdsHeader or
	// we have invalid data.
	if (uSizeInBytes < sizeof(rHeader))
	{
		return false;
	}

	// Populate the DDS header.
	memcpy(&rHeader, pData, sizeof(rHeader));

	// Sanity check header values.
	if (rHeader.m_MagicNumber != kDdsMagicValue ||
		rHeader.m_Size != (sizeof(rHeader) - sizeof(rHeader.m_MagicNumber)))
	{
		return false;
	}

	// Check for a DX10 format DDS
	rbDX10 = false;
	if (rHeader.m_PixelFormat == kDdsPixelFormatDX10)
	{
		if (uSizeInBytes < (sizeof(rHeader) + sizeof(rHeaderDX10)))
		{
			return false;
		}

		rbDX10 = true;
		memcpy(&rHeaderDX10, ((Byte*)pData) + sizeof(rHeader), sizeof(rHeaderDX10));
	}

	return true;
}

/** Internal function for updating an existing pixel format. */
static Bool ApplyPixelFormat(DdsHeader& header, DdsHeaderDX10& headerDX10, PixelFormat eFormat)
{
	if (header.m_PixelFormat == kDdsPixelFormatDX10)
	{
		for (size_t i = 0u; i < SEOUL_ARRAY_COUNT(ksaDxgiFormatEntries); ++i)
		{
			if (ksaDxgiFormatEntries[i].m_ePixelFormat == eFormat)
			{
				headerDX10.m_DxgiFormat = (UInt32)ksaDxgiFormatEntries[i].m_eDxgiFormat;
				return true;
			}
		}
	}
	// Otherwise, compare the pixel format.
	else
	{
		for (size_t i = 0u; i < SEOUL_ARRAY_COUNT(ksaPixelFormatEntries); ++i)
		{
			if (ksaPixelFormatEntries[i].m_ePixelFormat == eFormat)
			{
				header.m_PixelFormat = ksaPixelFormatEntries[i].m_eDdsPixelFormat;
				return true;
			}
		}
	}

	return false;
}

/**
 * Update the header of pData to contain a new pixel format.
 */
static Bool UpdatePixelFormat(void* pData, UInt32 uSizeInBytes, PixelFormat eFormat)
{
	DdsHeader header;
	DdsHeaderDX10 headerDX10;
	Bool bDX10 = false;
	if (!GetHeaders(pData, uSizeInBytes, header, headerDX10, bDX10))
	{
		return false;
	}

	// Now perform the update.
	if (!ApplyPixelFormat(header, headerDX10, eFormat))
	{
		return false;
	}

	// On success, copy back the updated header.
	if (bDX10)
	{
		memcpy((Byte*)pData + sizeof(header), &headerDX10, sizeof(headerDX10));
	}
	else
	{
		memcpy(pData, &header, sizeof(header));
	}

	return true;
}

/**
 * Extract all relevant data from the DDS file. On success,
 * rpOut (and optionally rpOutSecondary) point(s) *inside* pData and *must not* be deallocated
 * separately.
 */
Bool Decode(
	void const* pData,
	UInt32 uSizeInBytes,
	UInt32& ruWidth,
	UInt32& ruHeight,
	PixelFormat& reFormat,
	void const*& rpOut,
	void const*& rpOutSecondary)
{
	DdsHeader header;
	DdsHeaderDX10 headerDX10;
	Bool bDX10 = false;
	if (!GetHeaders(pData, uSizeInBytes, header, headerDX10, bDX10))
	{
		return false;
	}

	// Check - SeoulEngine expects only a single mip level per DDS container.
	if (header.m_MipMapCount > 1)
	{
		return false;
	}

	UInt32 const uHeaderSize = (UInt32)(bDX10 ? (sizeof(header) + sizeof(headerDX10)) : sizeof(header));
	ruWidth = header.m_Width;
	ruHeight = header.m_Height;
	reFormat = ToPixelFormat(header, headerDX10);
	rpOut = (void*)(((Byte const*)pData) + uHeaderSize);

	// Optional secondary texture.
	UInt32 const uDataSize = GetDataSizeForPixelFormat(ruWidth, ruHeight, reFormat);
	if ((uHeaderSize + uDataSize) * 2u == uSizeInBytes)
	{
		// TODO: We're assuming the secondary texture has the same dimensions
		// and format as the primary. This is not currently enforced by the cooker.
		//
		// Skip the first texture data and the second texture's header.
		rpOutSecondary = ((Byte const*)rpOut) + uDataSize + uHeaderSize;
	}

	return true;
}

/**
 * Read the PixelFormat from the DDS header data contained in the given stream.
 * Can fail if the stream is invalid.
 */
Bool ReadPixelFormat(void const* pData, UInt32 uSizeInBytes, PixelFormat& reFormat)
{
	DdsHeader header;
	DdsHeaderDX10 headerDX10;
	Bool bDX10 = false;
	if (!GetHeaders(pData, uSizeInBytes, header, headerDX10, bDX10))
	{
		return false;
	}

	reFormat = ToPixelFormat(header, headerDX10);
	return true;
}

/**
 * Specialized utility - given a DDS in an uncompressed BGRA8888 or RGBA8888 format,
 * swaps the RB channels and updates the data's header.
 */
Bool SwapChannelsRB(void* pData, UInt32 uSizeInBytes)
{
	DdsHeader header;
	DdsHeaderDX10 headerDX10;
	Bool bDX10 = false;
	if (!GetHeaders(pData, uSizeInBytes, header, headerDX10, bDX10))
	{
		return false;
	}

	// Get and check.
	auto ePixelFormat = ToPixelFormat(header, headerDX10);

	// Only valid for BGRA8888 and RGBA8888
	if (PixelFormat::kA8B8G8R8 != ePixelFormat && PixelFormat::kA8R8G8B8 != ePixelFormat)
	{
		return false;
	}

	// Swap channels.
	UInt32 const uHeaderSize = (bDX10 ? (sizeof(header) + sizeof(headerDX10)) : sizeof(header));
	UInt32 const uWidth = header.m_Width;
	UInt32 const uHeight = header.m_Height;
	UInt32 const uMips = (0u == header.m_MipMapCount ? 1u : header.m_MipMapCount);
	UInt32 uOffset = 0u;
	UInt32 uMipWidth = uWidth;
	UInt32 uMipHeight = uHeight;
	for (UInt32 i = 0u; i < uMips; ++i)
	{
		UInt32 const uMipSize = GetDataSizeForPixelFormat(uMipWidth, uMipHeight, ePixelFormat);
		UInt32 const uTotalOffset = (uOffset + uHeaderSize);
		if (uTotalOffset + uMipSize > uSizeInBytes)
		{
			return false;
		}

		UInt8* const pBegin = (UInt8*)pData + uTotalOffset;
		UInt8* const pEnd = (pBegin + uMipSize);
		for (auto p = pBegin; p < pEnd; p += 4u)
		{
			Swap(p[0], p[2]);
		}

		uMipWidth = Max(uMipWidth >> 1, 1u);
		uMipHeight = Max(uMipHeight >> 1, 1u);
		uOffset += uMipSize;
	}

	// Set the new pixel format.
	if (!UpdatePixelFormat(pData, uSizeInBytes, (ePixelFormat == PixelFormat::kA8B8G8R8 ? PixelFormat::kA8R8G8B8 : PixelFormat::kA8B8G8R8)))
	{
		return false;
	}

	// Done, success.
	return true;
}

// Convert a SeoulEngine PixelFormat to a DdsPixelFormat.
DdsPixelFormat ToDdsPixelFormat(PixelFormat eFormat)
{
	for (auto const& e : ksaPixelFormatEntries)
	{
		if (e.m_ePixelFormat == eFormat)
		{
			return e.m_eDdsPixelFormat;
		}
	}

	DdsPixelFormat ret;
	memset(&ret, 0, sizeof(ret));
	return ret;
}

/**
 * @return A SeoulEngine PixelFormat::Enum value corresponding to the pixel format
 * defined in pixelFormat from a DDS file.
 */
PixelFormat ToPixelFormat(const DdsHeader& header, const DdsHeaderDX10& headerDX10)
{
	PixelFormat eReturn = PixelFormat::kInvalid;

	// Handle DX10 format DDS by looking at the DxgiFormat member.
	if (header.m_PixelFormat == kDdsPixelFormatDX10)
	{
		for (size_t i = 0u; i < SEOUL_ARRAY_COUNT(ksaDxgiFormatEntries); ++i)
		{
			if (ksaDxgiFormatEntries[i].m_eDxgiFormat == (DxgiFormat)headerDX10.m_DxgiFormat)
			{
				eReturn = ksaDxgiFormatEntries[i].m_ePixelFormat;
				break;
			}
		}
	}
	// Otherwise, compare the pixel format.
	else
	{
		for (size_t i = 0u; i < SEOUL_ARRAY_COUNT(ksaPixelFormatEntries); ++i)
		{
			if (ksaPixelFormatEntries[i].m_eDdsPixelFormat == header.m_PixelFormat)
			{
				eReturn = ksaPixelFormatEntries[i].m_ePixelFormat;
				break;
			}
		}
	}

	return eReturn;
}

// Given a DDS fourcc, return a SeoulEngine PixelFormat.
PixelFormat ToPixelFormat(UInt32 uFourCC)
{
	for (auto const& e : ksaPixelFormatEntries)
	{
		if (e.m_eDdsPixelFormat.m_FourCC == uFourCC)
		{
			return e.m_ePixelFormat;
		}
	}

	return PixelFormat::kInvalid;
}

} // namespace Seoul::DDS
