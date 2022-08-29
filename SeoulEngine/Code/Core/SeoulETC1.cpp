/**
 * \file SeoulETC1.cpp
 * \brief Implementation of ETC texture decompression in software.
 * \sa https://www.khronos.org/registry/OpenGL/extensions/OES/OES_compressed_ETC1_RGB8_texture.txt
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "DDS.h"
#include "FixedArray.h"
#include "SeoulETC1.h"
#include "SeoulMath.h"
#include "TextureFooter.h"

namespace Seoul
{

// Internal ETC1 decompression.
namespace
{

// Simple utility macros, extract bit portions of given values.
#define SEOUL_GET_LO(val, size, pos)  (((val) >> (((pos) - 0)  - (size) + 1)) & ((1 << (size)) - 1))
#define SEOUL_GET_HI(val, size, pos)  (((val) >> (((pos) - 32) - (size) + 1)) & ((1 << (size)) - 1))

/** From iTable 3.17.2: https://www.khronos.org/registry/OpenGL/extensions/OES/OES_compressed_ETC1_RGB8_texture.txt */
static const Int32 kaiModifierTable[16][4] =
{
	{ -8, -2,  2, 8 },
	{ -8, -2,  2, 8 },
	{ -17, -5, 5, 17 },
	{ -17, -5, 5, 17 },
	{ -29, -9, 9, 29 },
	{ -29, -9, 9, 29 },
	{ -42, -13, 13, 42 },
	{ -42, -13, 13, 42 },
	{ -60, -18, 18, 60 },
	{ -60, -18, 18, 60 },
	{ -80, -24, 24, 80 },
	{ -80, -24, 24, 80 },
	{-106, -33, 33, 106 },
	{ -106, -33, 33, 106 },
	{ -183, -47, 47, 183 },
	{ -183, -47, 47, 183 }
};

/** Maps a compute modifier index to a modifier iTable value. */
static const Int32 kaiModifierRemap[] = { 2, 3, 1, 0 };

/** An ETC1 block (64-bits, color end points in various configurations and selector bits). */
struct Block SEOUL_SEALED
{
	UInt32 m_uEndPoints;
	UInt32 m_uSelectors;

	/** ETC1 data is big endian. */
	void ConditionalEndianSwap()
	{
#if SEOUL_LITTLE_ENDIAN
		m_uEndPoints = Seoul::EndianSwap32(m_uEndPoints);
		m_uSelectors = Seoul::EndianSwap32(m_uSelectors);
#endif // /SEOUL_LITTLE_ENDIAN
	}
};
SEOUL_STATIC_ASSERT(sizeof(Block) == 8u);

/** Pixel used for processing. */
struct RGBAu8 SEOUL_SEALED
{
	inline static RGBAu8 Create(UInt8 uR, UInt8 uG, UInt8 uB, UInt8 uA = 255u)
	{
		RGBAu8 ret;
		ret.m_uR = uR;
		ret.m_uG = uG;
		ret.m_uB = uB;
		ret.m_uA = uA;
		return ret;
	}

#if SEOUL_LITTLE_ENDIAN
	UInt8 m_uR;
	UInt8 m_uG;
	UInt8 m_uB;
	UInt8 m_uA;
#else
	UInt8 m_uA;
	UInt8 m_uB;
	UInt8 m_uG;
	UInt8 m_uR;
#endif
};
SEOUL_STATIC_ASSERT(sizeof(RGBAu8) == 4u);

/** Utility used by DecompressBlockHalf, computes and applies a color. */
template <Bool B_MERGE_ALPHA>
inline static void ApplyColor(
	Int32 iTable,
	UInt8 aBaseColors[3],
	UInt32 uPixelIndicesMSB,
	UInt32 uPixelIndicesLSB,
	Int32 iShift,
	RGBAu8& r)
{
	// Compute and then remap the selector index.
	Int32 iIndex = ((uPixelIndicesMSB >> iShift) & 0x1) << 1;
	iIndex |= ((uPixelIndicesLSB >> iShift) & 0x1);
	iIndex = kaiModifierRemap[iIndex];

	// Get the modifier value.
	Int32 const iModifier = kaiModifierTable[iTable][iIndex];

	// If B_MERGE_ALPHA is true, than the green channel is
	// actually the alpha channel of data already in the buffer.
	if (B_MERGE_ALPHA)
	{
		r.m_uA = (UInt8)Clamp((Int32)aBaseColors[1] + iModifier, 0, 255);
	}
	else
	{
		// Apply the modifer and clamp to get the final value.
		r = RGBAu8::Create(
			(UInt8)Clamp((Int32)aBaseColors[0] + iModifier, 0, 255),
			(UInt8)Clamp((Int32)aBaseColors[1] + iModifier, 0, 255),
			(UInt8)Clamp((Int32)aBaseColors[2] + iModifier, 0, 255));
	}
}

/**
 * Decompress half of an ETC1 block. Shared, with
 * templated configuration, between *Half0 and *Half1.
 *
 * \sa https://www.khronos.org/registry/OpenGL/extensions/OES/OES_compressed_ETC1_RGB8_texture.txt
 *
 * c) bit layout in bits 31 through 0 (in both cases)
 *
 * 31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16
 * -----------------------------------------------
 * |       most significant pixel index bits       |
 * | p| o| n| m| l| k| j| i| h| g| f| e| d| c| b| a|
 * -----------------------------------------------
 *
 * 15 14 13 12 11 10  9  8  7  6  5  4  3   2   1  0
 * --------------------------------------------------
 * |         least significant pixel index bits       |
 * | p| o| n| m| l| k| j| i| h| g| f| e| d| c | b | a |
 * --------------------------------------------------
 */
template <Bool B_MERGE_ALPHA, Int32 TABLE_BIT, Int32 START_OFFSET>
inline static void DecompressBlockHalf(
	UInt8 aBaseColors[3],
	const Block& block,
	RGBAu8* pData,
	Int32 iWidth,
	Int32 iStartX,
	Int32 iStartY)
{
	// Get the iTable selector.
	Int32 const iTable = (SEOUL_GET_HI(block.m_uEndPoints, 3, TABLE_BIT) << 1);

	// Get the pixel selector bits.
	UInt32 const uPixelIndicesMSB = SEOUL_GET_LO(block.m_uSelectors, 16, 31);
	UInt32 const uPixelIndicesLSB = SEOUL_GET_LO(block.m_uSelectors, 16, 15);

	Int32 const iFlipBit = (SEOUL_GET_HI(block.m_uEndPoints, 1, 32));

	// Flip bit is 0, blocks are split left-to-right.
	if (0 == iFlipBit)
	{
		Int32 iShift = (START_OFFSET * 4); // iShift starts as 0 or 8.
		for (Int32 iX = iStartX + START_OFFSET; iX < iStartX + 2 + START_OFFSET; ++iX)
		{
			// Iterate over the block portion and derive the color.
			for (Int32 iY = iStartY; iY < iStartY + 4; ++iY)
			{
				ApplyColor<B_MERGE_ALPHA>(iTable, aBaseColors, uPixelIndicesMSB, uPixelIndicesLSB, iShift, pData[iY * iWidth + iX]);
				++iShift;
			}
		}
	}
	// Flip bit is 1, blocks are split top-to-bottom.
	else
	{
		Int32 iShift = START_OFFSET; // Offset is 0 or 2.
		for (Int32 iX = iStartX; iX < iStartX + 4; ++iX)
		{
			// Iterate over the block portion and derive the color.
			for (Int32 iY = iStartY + START_OFFSET; iY < iStartY + 2 + START_OFFSET; ++iY)
			{
				ApplyColor<B_MERGE_ALPHA>(iTable, aBaseColors, uPixelIndicesMSB, uPixelIndicesLSB, iShift, pData[iY * iWidth + iX]);
				++iShift;
			}

			iShift += 2;
		}
	}
}

/**
 * Decompress the half 0 of an ETC1 block. Depending on mode,
 * this is either the left or top half of the block.
 */
template <Bool B_MERGE_ALPHA>
inline static void DecompressBlockHalf0(
	UInt8 aBaseColors[3],
	const Block& block,
	RGBAu8* pData,
	Int32 iWidth,
	Int32 iStartX,
	Int32 iStartY)
{
	DecompressBlockHalf<B_MERGE_ALPHA, 39, 0>(aBaseColors, block, pData, iWidth, iStartX, iStartY);
}

/**
 * Decompress the half 1 of an ETC1 block. Depending on mode,
 * this is either the right or bottom half of the block.
 */
template <Bool B_MERGE_ALPHA>
inline static void DecompressBlockHalf1(
	UInt8 aBaseColors[3],
	const Block& block,
	RGBAu8* pData,
	Int32 iWidth,
	Int32 iStartX,
	Int32 iStartY)
{
	DecompressBlockHalf<B_MERGE_ALPHA, 36, 2>(aBaseColors, block, pData, iWidth, iStartX, iStartY);
}

/**
 * Decompress a single ETC1 block where the diff bit is set to 0.
 *
 * \sa https://www.khronos.org/registry/OpenGL/extensions/OES/OES_compressed_ETC1_RGB8_texture.txt
 *
 * a) bit layout in bits 63 through 32 if diffbit = 0
 *
 * 63 62 61 60 59 58 57 56 55 54 53 52 51 50 49 48
 * -----------------------------------------------
 * | base col1 | base col2 | base col1 | base col2 |
 * | R1 (4bits)| R2 (4bits)| G1 (4bits)| G2 (4bits)|
 * -----------------------------------------------
 *
 * 47 46 45 44 43 42 41 40 39 38 37 36 35 34  33  32
 * ---------------------------------------------------
 * | base col1 | base col2 | iTable | iTable |diff|flip|
 * | B1 (4bits)| B2 (4bits)| cw 1   | cw 2   |bit |bit |
 * ---------------------------------------------------
 */
template <Bool B_MERGE_ALPHA>
inline static void DecompressBlockDiff0(
	const Block& block,
	RGBAu8* pData,
	Int32 iWidth,
	Int32 iStartX,
	Int32 iStartY)
{
	// Left/top half.
	{
		// Get the color endpoints.
		UInt8 aBaseColors[3];
		aBaseColors[0] = SEOUL_GET_HI(block.m_uEndPoints, 4, 63);
		aBaseColors[1] = SEOUL_GET_HI(block.m_uEndPoints, 4, 55);
		aBaseColors[2] = SEOUL_GET_HI(block.m_uEndPoints, 4, 47);

		aBaseColors[0] |= (aBaseColors[0] << 4);
		aBaseColors[1] |= (aBaseColors[1] << 4);
		aBaseColors[2] |= (aBaseColors[2] << 4);

		// Decompress the left/top half.
		DecompressBlockHalf0<B_MERGE_ALPHA>(aBaseColors, block, pData, iWidth, iStartX, iStartY);
	}

	// Right/bottom half.
	{
		// Get the color endpoints.
		UInt8 aBaseColors[3];
		aBaseColors[0] = SEOUL_GET_HI(block.m_uEndPoints, 4, 59);
		aBaseColors[1] = SEOUL_GET_HI(block.m_uEndPoints, 4, 51);
		aBaseColors[2] = SEOUL_GET_HI(block.m_uEndPoints, 4, 43);

		aBaseColors[0] |= (aBaseColors[0] << 4);
		aBaseColors[1] |= (aBaseColors[1] << 4);
		aBaseColors[2] |= (aBaseColors[2] << 4);

		// Decompress the right/bottom half.
		DecompressBlockHalf1<B_MERGE_ALPHA>(aBaseColors, block, pData, iWidth, iStartX, iStartY);
	}
}

/**
 * Decompress a single ETC1 block where the diff bit is set to 1.
 *
 * \sa https://www.khronos.org/registry/OpenGL/extensions/OES/OES_compressed_ETC1_RGB8_texture.txt
 *
 * b) bit layout in bits 63 through 32 if diffbit = 1
 *
 * 63 62 61 60 59 58 57 56 55 54 53 52 51 50 49 48
 * -----------------------------------------------
 * | base col1    | dcol 2 | base col1    | dcol 2 |
 * | R1' (5 bits) | dR2    | G1' (5 bits) | dG2    |
 * -----------------------------------------------
 *
 * 47 46 45 44 43 42 41 40 39 38 37 36 35 34  33  32
 * ---------------------------------------------------
 * | base col 1   | dcol 2 | iTable  | iTable  |diff|flip|
 * | B1' (5 bits) | dB2    | cw 1   | cw 2   |bit |bit |
 * ---------------------------------------------------
 */
template <Bool B_MERGE_ALPHA>
inline static void DecompressBlockDiff1(
	const Block& block,
	RGBAu8* pData,
	Int32 iWidth,
	Int32 iStartX,
	Int32 iStartY)
{
	UInt8 aColors555_0[3];

	// Left/top half.
	{
		// Get the 555 color endpoints.
		aColors555_0[0] = SEOUL_GET_HI(block.m_uEndPoints, 5, 63);
		aColors555_0[1] = SEOUL_GET_HI(block.m_uEndPoints, 5, 55);
		aColors555_0[2] = SEOUL_GET_HI(block.m_uEndPoints, 5, 47);

		// Now expand to 888.
		UInt8 aBaseColors[3];
		aBaseColors[0] = (aColors555_0[0] << 3) | (aColors555_0[0] >> 2);
		aBaseColors[1] = (aColors555_0[1] << 3) | (aColors555_0[1] >> 2);
		aBaseColors[2] = (aColors555_0[2] << 3) | (aColors555_0[2] >> 2);

		// Decompress the left/top half.
		DecompressBlockHalf0<B_MERGE_ALPHA>(aBaseColors, block, pData, iWidth, iStartX, iStartY);
	}

	// Right/bottom half.
	{
		// Get the 333 signed delta endpoints.
		Int8 aiDiffColors[3];
		aiDiffColors[0] = SEOUL_GET_HI(block.m_uEndPoints, 3, 58);
		aiDiffColors[1] = SEOUL_GET_HI(block.m_uEndPoints, 3, 50);
		aiDiffColors[2] = SEOUL_GET_HI(block.m_uEndPoints, 3, 42);

		// Now expand to 888.
		aiDiffColors[0] = (aiDiffColors[0] << 5);
		aiDiffColors[1] = (aiDiffColors[1] << 5);
		aiDiffColors[2] = (aiDiffColors[2] << 5);
		aiDiffColors[0] = (aiDiffColors[0] >> 5);
		aiDiffColors[1] = (aiDiffColors[1] >> 5);
		aiDiffColors[2] = (aiDiffColors[2] >> 5);

		// Apply the deltas to compute the 555 of the second color.
		UInt8 aColors555_1[3];
		aColors555_1[0] = (aColors555_0[0] + aiDiffColors[0]);
		aColors555_1[1] = (aColors555_0[1] + aiDiffColors[1]);
		aColors555_1[2] = (aColors555_0[2] + aiDiffColors[2]);

		// Now expand to 888.
		UInt8 aBaseColors[3];
		aBaseColors[0] = (aColors555_1[0] << 3) | (aColors555_1[0] >> 2);
		aBaseColors[1] = (aColors555_1[1] << 3) | (aColors555_1[1] >> 2);
		aBaseColors[2] = (aColors555_1[2] << 3) | (aColors555_1[2] >> 2);

		// Decompress the right/bottom half.
		DecompressBlockHalf1<B_MERGE_ALPHA>(aBaseColors, block, pData, iWidth, iStartX, iStartY);
	}
}

/**
 * Decompress a single ETC1 block.
 *
 * \sa https://www.khronos.org/registry/OpenGL/extensions/OES/OES_compressed_ETC1_RGB8_texture.txt
 */
template <Bool B_MERGE_ALPHA>
static void DecompressBlock(
	const Block& block,
	RGBAu8* pData,
	Int32 iWidth,
	Int32 iStartX,
	Int32 iStartY)
{
	Int32 const iDiffBit = (SEOUL_GET_HI(block.m_uEndPoints, 1, 33));
	if (0 == iDiffBit)
	{
		DecompressBlockDiff0<B_MERGE_ALPHA>(block, pData, iWidth, iStartX, iStartY);
	}
	else
	{
		DecompressBlockDiff1<B_MERGE_ALPHA>(block, pData, iWidth, iStartX, iStartY);
	}
}

/** Decompresses an ETC1 block in place. */
template <Bool B_MERGE_ALPHA>
static void DoDecompress(
	void const* pInput,
	UInt32 uWidth,
	UInt32 uHeight,
	RGBAu8* pOutput)
{
	Block* p = (Block*)pInput;
	UInt32 const uBlockWidth = (uWidth / 4u);
	for (UInt32 uY = 0u; uY < uHeight; uY += 4u)
	{
		for (UInt32 uX = 0u; uX < uWidth; uX += 4u)
		{
			UInt32 const uBlockX = (uX / 4u);
			UInt32 const uBlockY = (uY / 4u);
			auto block = p[uBlockY * uBlockWidth + uBlockX];
			block.ConditionalEndianSwap();

			DecompressBlock<B_MERGE_ALPHA>(
				block,
				pOutput,
				uWidth,
				uX,
				uY);
		}
	}
}

/**
 * Utility, either allocates a buffer and decompresses as RGB,
 * or merges, treating the G channel of the input texture as
 * the alpha channel of the output texture.
 */
static Bool InternalETC1Decompress(
	void const* pIn,
	UInt32 uInputDataSizeInBytes,
	void*& rpOut,
	UInt32& ruOutputDataSizeInBytes,
	MemoryBudgets::Type eType,
	UInt32 uAlignmentOfOutputBuffer,
	Bool bMerge,
	UInt32& ruConsumedInputInBytes)
{
	DdsHeader header;
	memset(&header, 0, sizeof(header));
	DdsHeaderDX10 headerDX10;
	memset(&headerDX10, 0, sizeof(headerDX10));

	// Size must be at least as big as the side of the DdsHeader or
	// we have invalid data.
	if (uInputDataSizeInBytes < sizeof(header))
	{
		return false;
	}

	// Populate the DDS header.
	memcpy(&header, pIn, sizeof(header));

	// Sanity check header values.
	if (header.m_MagicNumber != kDdsMagicValue ||
		header.m_Size != (sizeof(header) - sizeof(header.m_MagicNumber)))
	{
		return false;
	}

	// Check for a DX10 format DDS
	Bool bDX10 = false;
	if (header.m_PixelFormat == kDdsPixelFormatDX10)
	{
		if (uInputDataSizeInBytes < (sizeof(header) + sizeof(headerDX10)))
		{
			return false;
		}

		bDX10 = true;
		memcpy(&headerDX10, ((Byte*)pIn) + sizeof(header), sizeof(headerDX10));
	}

	UInt32 const uHeaderSize = (bDX10 ? (sizeof(header) + sizeof(headerDX10)) : sizeof(header));

	// Cube maps and volume textures not supported.
	if (header.m_CubemapFlags != 0 ||
		0u != (header.m_HeaderFlags & kDdsHeaderFlagsVolume) ||
		(bDX10 && headerDX10.m_ResourceDimension == (UInt32)D3d11ResourceDimension::kTexture3D))
	{
		return false;
	}

	// Convert the format to a known Open GL format.
	PixelFormat const ePixelFormat = DDS::ToPixelFormat(header, headerDX10);

	// If the data is not ETC1, fail.
	if (PixelFormat::kETC1_RGB8 != ePixelFormat)
	{
		return false;
	}

	// Process and populate.
	// Populate the texture data.
	UInt32 const uWidth = header.m_Width;
	UInt32 const uHeight = header.m_Height;
	UInt32 const uMips = (0u == header.m_MipMapCount ? 1u : header.m_MipMapCount);

	UInt32 uTotalOutputSize = sizeof(DdsHeader);

	// Iterate, compute output size, and check.
	{
		ruConsumedInputInBytes = uHeaderSize;
		UInt32 uMipWidth = uWidth;
		UInt32 uMipHeight = uHeight;
		for (UInt32 i = 0u; i < uMips; ++i)
		{
			// Data size in bytes of the current blob.
			UInt32 const uMipSize = GetDataSizeForPixelFormat(uMipWidth, uMipHeight, ePixelFormat);

			// Output size - output is always RGBA8888.
			uTotalOutputSize += GetDataSizeForPixelFormat(uMipWidth, uMipHeight, PixelFormat::kA8B8G8R8);

			// Compute next level.
			uMipWidth = Max(uMipWidth >> 1, 1u);
			uMipHeight = Max(uMipHeight >> 1, 1u);
			ruConsumedInputInBytes += uMipSize;

			// Sanity check that we haven't run out of data.
			if (ruConsumedInputInBytes > uInputDataSizeInBytes)
			{
				return false;
			}
		}
	}

	// Now we perform actual processing - if we're not merging, we need to instantiate
	// and populate the output buffer. Otherwise, we need to check it (make sure
	// the alpha data has the exact same size and characteristics as the RGB data).
	if (bMerge)
	{
		if (uTotalOutputSize != ruOutputDataSizeInBytes)
		{
			return false;
		}
	}
	// Allocate.
	else
	{
		// Allocate the block, leaving enough room for the footer.
		rpOut = MemoryManager::AllocateAligned(
			(uTotalOutputSize + sizeof(TextureFooter)),
			eType,
			Max(uAlignmentOfOutputBuffer, 4u));
		ruOutputDataSizeInBytes = uTotalOutputSize;

		// Create and then copy through a new DDS container header.
		DdsHeader newHeader;
		memset(&newHeader, 0, sizeof(newHeader));
		newHeader.m_MagicNumber = kDdsMagicValue;
		newHeader.m_Size = (sizeof(newHeader) - sizeof(newHeader.m_MagicNumber));
		newHeader.m_HeaderFlags = kDdsHeaderFlagsTexture | kDdsHeaderFlagsLinearSize;
		newHeader.m_Height = uHeight;
		newHeader.m_Width = uWidth;
		newHeader.m_PitchOrLinearSize = (uWidth * 4u);
		newHeader.m_Depth = 1;
		newHeader.m_MipMapCount = uMips;
		newHeader.m_PixelFormat = kDdsPixelFormatA8B8G8R8;
		newHeader.m_SurfaceFlags = kDdsSurfaceFlagsTexture;
		memcpy(rpOut, &newHeader, sizeof(newHeader));
	}

	// Iterate, compute output size, and check.
	{
		UInt32 uInOffset = uHeaderSize;
		UInt32 uOutOffset = sizeof(DdsHeader);
		UInt32 uMipWidth = uWidth;
		UInt32 uMipHeight = uHeight;
		for (UInt32 i = 0u; i < uMips; ++i)
		{
			// Data size in bytes of the current blob.
			UInt32 const uInSize = GetDataSizeForPixelFormat(uMipWidth, uMipHeight, ePixelFormat);

			// Output size - output is always RGBA8888.
			UInt32 const uOutSize = GetDataSizeForPixelFormat(uMipWidth, uMipHeight, PixelFormat::kA8B8G8R8);

			// Merge or perform in place.
			if (bMerge)
			{
				DoDecompress<true>((Byte*)pIn + uInOffset, uMipWidth, uMipHeight, (RGBAu8*)((Byte*)rpOut + uOutOffset));
			}
			else
			{
				DoDecompress<false>((Byte*)pIn + uInOffset, uMipWidth, uMipHeight, (RGBAu8*)((Byte*)rpOut + uOutOffset));
			}

			// Compute next level.
			uMipWidth = Max(uMipWidth >> 1, 1u);
			uMipHeight = Max(uMipHeight >> 1, 1u);
			uInOffset += uInSize;
			uOutOffset += uOutSize;
		}
	}

	return true;
}

} // namespace anonymous

/**
 * Decompress ETC1 texture data. Input is expected to be
 * a SeoulEngine DDS container (which means, it may actually
 * be 2 DDS containers, one after another, where the second
 * DDS container is the alpha data for the decompressed data).
 *
 * On success, rpOut will be allocated and populated with
 * a new SeoulEngine DDS container in a 32-bit RGBA format.
 */
Bool ETC1Decompress(
	void const* pIn,
	UInt32 uInputDataSizeInBytes,
	void*& rpOut,
	UInt32& ruOutputDataSizeInBytes,
	MemoryBudgets::Type eType /*= MemoryBudgets::Rendering*/,
	UInt32 uAlignmentOfOutputBuffer /*= 4u*/)
{
	// Adjust size for the texture footer.
	if (uInputDataSizeInBytes < sizeof(TextureFooter))
	{
		return false;
	}
	uInputDataSizeInBytes -= sizeof(TextureFooter);

	void* p = nullptr;
	UInt32 u = 0u;

	UInt32 uConsumed = 0u;
	if (!InternalETC1Decompress(
		pIn,
		uInputDataSizeInBytes,
		p,
		u,
		eType,
		uAlignmentOfOutputBuffer,
		false,
		uConsumed) || (uConsumed > uInputDataSizeInBytes))
	{
		MemoryManager::Deallocate(p);
		return false;
	}

	// If consumed is < uInputDataSizeInBytes, it means the first image
	// is followed by an alpha image, so we need to merge it.
	if (uConsumed < uInputDataSizeInBytes)
	{
		void* pNext = (Byte*)pIn + uConsumed;
		UInt32 uNext = (uInputDataSizeInBytes - uConsumed);

		// At this point, we must consume all input, so uNextConsumed must == uNext when
		// we're done.
		UInt32 uNextConsumed = 0u;
		if (!InternalETC1Decompress(
			pNext,
			uNext,
			p,
			u,
			eType,
			uAlignmentOfOutputBuffer,
			true,
			uNextConsumed) || uNextConsumed != uNext)
		{
			MemoryManager::Deallocate(p);
			return false;
		}
	}

	// Copy through the footer.
	memcpy((Byte*)p + u, (Byte*)pIn + uInputDataSizeInBytes, sizeof(TextureFooter));

	// Done, success.
	rpOut = p;
	ruOutputDataSizeInBytes = (u + sizeof(TextureFooter));
	return true;
}

} // namespace Seoul
