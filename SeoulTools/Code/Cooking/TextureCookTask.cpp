/**
 * \file TextureCookTask.cpp
 * \brief Cooking tasks for cooking source .png files into runtime
 * SeoulEngine .sif0, .sif1, .sif2, .sif3, and .sif4 files.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Color.h"
#include "Compress.h"
#include "CookPriority.h"
#include "BaseCookTask.h"
#include "FileManager.h"
#include "ICookContext.h"
#include "Image.h"
#include "Logger.h"
#include "Prereqs.h"
#include "ReflectionDefine.h"
#include "ScopedAction.h"
#include "StackOrHeapArray.h"
#include "TextureEncodingType.h"
#include "TextureFooter.h"
#include "UnsafeBuffer.h"

#include <crnlib.h>

namespace Seoul
{

#	define STBIR_ASSERT(x) SEOUL_ASSERT(x)
#	define STBIR_MALLOC(sz,unused_context) Seoul::MemoryManager::Allocate((size_t)(sz), MemoryBudgets::Io)
#	define STBIR_FREE(p,unused_context) Seoul::MemoryManager::Deallocate((p))
#	define STB_IMAGE_RESIZE_IMPLEMENTATION
#	define STB_IMAGE_RESIZE_STATIC
#	include <stb_image_resize.h>

} // namespace Seoul

namespace Seoul
{

namespace Cooking
{

Bool EncodeTexture(
	UInt8 const* pInputRGBAImageData,
	UInt32 uInputWidth,
	UInt32 uInputHeight,
	TextureEncodingType::Enum eType,
	Byte** ppOutputData,
	UInt32* puOutputDataSizeInBytes);

namespace
{

/**
 * @return The "best" power of 2 for a given dimension - currently just
 * uses the next highest power of 2 if needed.
 */
static UInt32 GetBestPowerOfTwo(UInt32 uDimension)
{
	// Start with the previous power of 2.
	UInt32 uPowerOfTwo = GetPreviousPowerOf2(uDimension);

	// If the previous power of 2 is less than
	// the original dimension, use the next power of 2 instead.
	if (uPowerOfTwo < uDimension)
	{
		uPowerOfTwo *= 2u;
	}

	return uPowerOfTwo;
}

/**
* @return The maximum input image dimension supported for ePlatform, images
* with a dimension larger than this cannot be cooked.
*/
static inline Int32 GetMaxInputImageDimension(Platform ePlatform)
{
	// Hard limit of our image resizing.
	return 4096;
}

/**
 * @return The iWidth * height of a texture, below which it will not be compressed.
 */
static inline Bool NeedsCompression(
	Platform ePlatform,
	UInt32 uOutputMipLevel,
	UInt32 uWidth,
	UInt32 uHeight)
{
	if (uWidth < 4 || uHeight < 4)
	{
		// None of the formats we use support textures of less
		// than 4 pixels in either dimension, so we can't compress
		// this far regardless.
		return false;
	}

	// Compute mip zero width and height.
	UInt32 uMip0Width = (uWidth << uOutputMipLevel);
	UInt32 uMip0Height = (uHeight << uOutputMipLevel);

	// Texture must be compressed if their mip0 is > (128 x 128).
	return (uMip0Width * uMip0Height) > (128u * 128u);
}

namespace ImageAlphaType
{

/** Describes input image alpha data. */
enum Enum
{
	/** Used to indicate an unselected or invalid format. */
	kUnknown,

	/** Output file is a compressed texture with no alpha channel. */
	kRgbNoAlpha,

	/** Output file is a compressed texture with 1-bit alpha. */
	kRgbMaskAlpha,

	/** Output file is a compressed texture with an alpha channel. */
	kRgbFullAlpha,
};

} // namespace ImageAlphaType

class Image SEOUL_SEALED
{
public:
	Image()
	{
	}

	~Image()
	{
		Destroy();
	}

	/**
	 * Clone the contents of this Image into
	 * cloneImage.
	 */
	void CloneTo(Image& cloneImage) const
	{
		cloneImage.m_pData = (UInt8*)MemoryManager::Allocate(
			m_uHeight * m_uWidth * 4u,
			MemoryBudgets::Cooking);
		memcpy(cloneImage.m_pData, m_pData, m_uHeight * m_uWidth * 4u);
		cloneImage.m_uHeight = m_uHeight;
		cloneImage.m_uWidth = m_uWidth;
	}

	/** Single scanline (X) offset in pixels, given an occlusion entry offset. */
	static inline UInt32 ImgOffsetFromOccX(UInt32 uX)
	{
		return (uX * 32u);
	}

	/** Sub bit selection from within an occlusion entry, given an X position in pixels. */
	static inline UInt32 OccBitFromImgX(UInt32 uX)
	{
		return (1u << (uX % 32u));
	}

	/** Single scanline (X) offset in occlusion entries, given an X position in pixels. */
	static inline UInt32 OccOffsetFromImgX(UInt32 uX)
	{
		return (uX / 32u);
	}

	/** Single scanline (X) offset in occlusion entries from an X position in pixels, rounded up. */
	static inline UInt32 OccOffsetFromImgXCeil(UInt32 uX)
	{
		return OccOffsetFromImgX(uX) + ((uX % 32) != 0 ? 1 : 0);
	}

	/** Convert an image X, Y position in pixels to an offset within the occlusion entry array. */
	UInt32 OccIndexFromImg(UInt32 uOccWidth, UInt32 uX, UInt32 uY) const
	{
		return (uY * uOccWidth) + OccOffsetFromImgX(uX);
	}

	/** Adjust p to the given scanline. */
	static inline UInt32* Scanline(UInt32 uOccWidth, UInt32* p, UInt32 uY)
	{
		return p + (uY * uOccWidth);
	}

	/** Adjust p to the given scanline. */
	static inline UInt32 const* Scanline(UInt32 uOccWidth, UInt32 const* p, UInt32 uY)
	{
		return p + (uY * uOccWidth);
	}

	/**
	 * Encapsulates the sub region of the image that can
	 * occlude other renders (solid color, within
	 * kOcclusionThreshold.
	 */
	struct OcclusionRectangle SEOUL_SEALED
	{
		Int32 m_iX{};
		Int32 m_iY{};
		Int32 m_iWidth{};
		Int32 m_iHeight{};

		Bool operator==(const OcclusionRectangle& b) const
		{
			return
				(m_iX == b.m_iX) &&
				(m_iY == b.m_iY) &&
				(m_iWidth == b.m_iWidth) &&
				(m_iHeight == b.m_iHeight);
		}

		Bool operator!=(const OcclusionRectangle& b) const
		{
			return !(*this == b);
		}

		Int32 GetArea() const
		{
			return (m_iWidth * m_iHeight);
		}
	}; // struct OcclusionRectangle
	typedef Vector<OcclusionRectangle, MemoryBudgets::Cooking> OcclusionRectangles;

	/**
	 * Find a single rectangle (the largest possible)
	 * that encloses pixels that are fully opaque.
	 */
#if SEOUL_UNIT_TESTS
	void GetOcclusionRegion(Int32& riX, Int32& riY, Int32& riWidth, Int32& riHeight, OcclusionRectangles* pvOut = nullptr) const
#else
	void GetOcclusionRegion(Int32& riX, Int32& riY, Int32& riWidth, Int32& riHeight) const
#endif // /SEOUL_UNIT_TESTS
	{
		auto const uOccWidth = OccOffsetFromImgXCeil(m_uWidth);
		auto const uHeight = m_uHeight;
		auto const uOccSize = (uOccWidth * uHeight);
		Vector<UInt32, MemoryBudgets::Cooking> vuVisited(uOccSize); // Zero by default.
		Vector<UInt32, MemoryBudgets::Cooking> vuOccluding(uOccSize); // Zero by default.

		// Fill vuOccluding.
		for (UInt32 uY = 0u; uY < m_uHeight; ++uY)
		{
			for (UInt32 uX = 0u; uX < m_uWidth; ++uX)
			{
				// Above occlusion threshold, set the occlusion bit.
				auto const uImgIndex = uY * m_uWidth + uX;
				auto const uA = m_pData[uImgIndex * 4u + 3u];
				if (uA >= ku8bitColorOcclusionThreshold)
				{
					vuOccluding[OccIndexFromImg(uOccWidth, uX, uY)] |= OccBitFromImgX(uX);
				}
			}
		}

		OcclusionRectangle best;
		for (UInt32 uY = 0u; uY < m_uHeight; ++uY)
		{
			for (UInt32 uX = 0u; uX < m_uWidth; ++uX)
			{
				auto const uImgIndex = uY * m_uWidth + uX;
				auto const uOccIndex = OccIndexFromImg(uOccWidth, uX, uY);
				auto const uOccBit = OccBitFromImgX(uX);

				// Never start a new search from a pixel
				// we've already visited.
				if (vuVisited[uOccIndex] & uOccBit)
				{
					continue;
				}

				// Don't start a search from a pixel
				// that doesn't occlude.
				if (0u == (vuOccluding[uOccIndex] & uOccBit))
				{
					continue;
				}

				// New search.
				auto const cur = ComputeOcclusionRectangle(uX, uY, uOccWidth, vuVisited.Data(), vuOccluding.Data());
#if SEOUL_UNIT_TESTS
				if (pvOut) { pvOut->PushBack(cur); }
#endif // /SEOUL_UNIT_TESTS
				if (cur.GetArea() > best.GetArea())
				{
					best = cur;
				}
			}
		}

		riX = best.m_iX;
		riY = best.m_iY;
		riWidth = best.m_iWidth;
		riHeight = best.m_iHeight;
	}

	/**
	 * Find the single rectangle that encloses the visible
	 * pixels (alpha > 0) of this Image.
	 */
	void GetVisibleRegion(Int32& riX, Int32& riY, Int32& riWidth, Int32& riHeight) const
	{
		// Start with the inverse rectangle. Note the
		// slightly unexpected values. Right/bottom
		// are always coordinate+1, and left/top are always
		// just coordinate, so these adjustments account for that.
		Int32 iX0 = (Int32)m_uWidth - 1;
		Int32 iY0 = (Int32)m_uHeight - 1;
		Int32 iX1 = 1;
		Int32 iY1 = 1;

		// Fixed stride in bytes for RGBA and BGRA.
		Int32 const kiStride = 4u;
		Int32 const iPitch = (kiStride * (Int32)m_uWidth);

		for (Int32 iY = 0; iY < (Int32)m_uHeight; ++iY)
		{
			for (Int32 iX = 0; iX < (Int32)m_uWidth; ++iX)
			{
				Int32 const iIndex = (iY * iPitch) + (iX * kiStride);

				UInt8 const uA = m_pData[iIndex + 3];

				if (0 != uA)
				{
					iX0 = Min(iX0, iX);
					iY0 = Min(iY0, iY);
					iX1 = Max(iX1, iX + 1);
					iY1 = Max(iY1, iY + 1);
				}
			}
		}

		// Done, output final results.
		riX = iX0;
		riY = iY0;
		riWidth = Max((iX1 - iX0), 0);
		riHeight = Max((iY1 - iY0), 0);
	}

	Bool Load(FilePath filePath, ImageAlphaType::Enum& reType)
	{
		void* p = nullptr;
		UInt32 u = 0u;
		if (!FileManager::Get()->ReadAll(filePath.GetAbsoluteFilenameInSource(), p, u, 0u, MemoryBudgets::Cooking))
		{
			SEOUL_LOG_COOKING("%s: failed reading source texture data for cook.", filePath.GetAbsoluteFilenameInSource().CStr());
			return false;
		}

		Int32 iWidth = 0;
		Int32 iHeight = 0;
		Bool bOriginalHasAlpha = false;
		UInt8* pImage = LoadPngFromMemory(
			(UInt8 const*)p,
			(Int32)u,
			&iWidth,
			&iHeight,
			nullptr,
			&bOriginalHasAlpha);
		MemoryManager::Deallocate(p);
		p = nullptr;

		if (nullptr == pImage)
		{
			SEOUL_LOG_COOKING("%s: LoadPngFromMemory returned a null ptr, image data is likely corrupt or invalid.", filePath.CStr());
			return false;
		}

		Destroy();

		m_pData = pImage;
		m_uHeight = (UInt32)iHeight;
		m_uWidth = (UInt32)iWidth;

		// Easy case.
		if (!bOriginalHasAlpha)
		{
			reType = ImageAlphaType::kRgbNoAlpha;
		}

		return true;
	}

	void MakeEmpty(UInt32 uWidth, UInt32 uHeight)
	{
		Destroy();

		m_pData = (UInt8*)MemoryManager::Allocate(uWidth * uHeight * 4u, MemoryBudgets::Cooking);
		memset(m_pData, 0, uWidth * uHeight * 4u);
		m_uHeight = uHeight;
		m_uWidth = uWidth;
	}

	/**
	 * Copy the contents of subImage into this Image at
	 * (iX, iY).
	 *
	 * @return True if the sub image was pasted into this Image, false otherwise
	 * Currently, this method will return false if:
	 * - both this Image and subImage are not top-to-bottom
	 * - the format of this Image is not equal to the format of subImage
	 * - subImage does not completely fit within this Image at the specified
	 *   coordinates.
	 */
	Bool PasteSubImage(Int32 iX, Int32 iY, const Image& subImage)
	{
		// Cache values used during the copy.
		Int32 const iSourceStride = 4;
		Int32 const iStride = 4;
		Int32 const iSourcePitch = ((Int32)subImage.GetWidth() * iSourceStride);
		Int32 const iPitch = ((Int32)m_uWidth * iStride);

		// Start the destination offset at the upper left corner of the target coordinates.
		Int32 iOffset = (iY * iPitch) + (iX * iStride);

		// Copy by row.
		for (Int32 iSourceY = 0; iSourceY < (Int32)subImage.GetHeight(); ++iSourceY)
		{
			// Source offset is the beginning of each row of the source.
			Int32 const iSourceOffset = (iSourceY * iSourcePitch);

			// Copy the data.
			memcpy(m_pData + iOffset, subImage.GetData() + iSourceOffset, iSourcePitch);

			// Advance the destination offset by the destination pitch.
			iOffset += iPitch;
		}

		return true;
	}

	void Swap(Image& r)
	{
		Seoul::Swap(m_pData, r.m_pData);
		Seoul::Swap(m_uHeight, r.m_uHeight);
		Seoul::Swap(m_uWidth, r.m_uWidth);
	}

	void TakeOwnership(UInt8* pImage, UInt32 uWidth, UInt32 uHeight)
	{
		Destroy();

		m_pData = pImage;
		m_uHeight = uHeight;
		m_uWidth = uWidth;
	}

	/**
	 * Apply the alpha channel to the color channel of this image.
	 *
	 * After premultiply, the image will be:
	 *   (R, G, B, A) . (R * A, G * A, B * A, A)
	 */
	void PremultiplyAlpha()
	{
		// Fixed stride in bytes for RGBA.
		UInt32 const kuStride = 4u;
		auto const uSize = (kuStride * m_uWidth * m_uHeight);

		for (UInt32 i = 0u; i < uSize; i += 4u)
		{
			Int32 const iA = (Int32)m_pData[i + 3];
			m_pData[i + 0] = (UInt8)(((Int32)m_pData[i + 0] * iA + 127) / 255);
			m_pData[i + 1] = (UInt8)(((Int32)m_pData[i + 1] * iA + 127) / 255);
			m_pData[i + 2] = (UInt8)(((Int32)m_pData[i + 2] * iA + 127) / 255);
		}
	}

	UInt8 const* GetData() const { return m_pData; }
	UInt8* GetData() { return m_pData; }
	UInt32 GetHeight() const { return m_uHeight; }
	UInt32 GetSizeInBytes() const { return (m_uWidth * m_uHeight * 4u); }
	UInt32 GetWidth() const { return m_uWidth; }

private:
	void Destroy()
	{
		MemoryManager::Deallocate(m_pData);
		m_pData = nullptr;
		m_uWidth = 0u;
		m_uHeight = 0u;
	}

	// NOTE: FindLeft and FindRight are well suited to (e.g.) using _BitScanForward and BitScanReverse,
	// as well as skipping 32-bits at a time for "interior" occlusion chunks. This has proven to not
	// be faster in tests - based on generated assembly, the compiler has a much easier time optimizing
	// these simpler versions of FindLeft and FindRight in context.
	//
	// Also likely due to the nature of the occlusion rectangle algorithm implemented here, long horizontal
	// runs happen less, so there is less benefit from the extra complexity of skipping 32-bits at a time.

	/**
	 * Part of FindHorizontal, find the leftmost occlusion pixel
	 * starting from iStartX, iStartY.
	 */
	Int32 FindLeft(Int32 iStartX, Int32 iStartY, UInt32 uOccWidth, UInt32 const* puOccluding) const
	{
		auto const p = Scanline(uOccWidth, puOccluding, iStartY);
		Int32 iX = iStartX;
		for (; iX > 0; --iX)
		{
			auto const uIndex = OccOffsetFromImgX(iX - 1);
			auto const uBits = OccBitFromImgX(iX - 1);
			if (0u == (p[uIndex] & uBits))
			{
				return iX;
			}
		}

		return iX;
	}

	/**
	 * Part of FindHorizontal, find the leftmost occlusion pixel
	 * starting from iStartX, iStartY.
	 */
	Int32 FindRight(Int32 iStartX, Int32 iStartY, UInt32 uOccWidth, UInt32 const* puOccluding) const
	{
		auto const p = Scanline(uOccWidth, puOccluding, iStartY);
		Int32 iX = iStartX;
		for (; iX + 1 < (Int32)m_uWidth; ++iX)
		{
			auto const uIndex = OccOffsetFromImgX(iX + 1);
			auto const uBits = OccBitFromImgX(iX + 1);
			if (0u == (p[uIndex] & uBits))
			{
				return iX + 1;
			}
		}

		return iX + 1;
	}

	/**
	 * Part of ComputeOcclusionRectangle(), find the X and Width
	 * starting from iStartX.
	 */
	void FindHorizontal(Int32 iStartX, Int32 iStartY, UInt32 uOccWidth, UInt32 const* puOccluding, OcclusionRectangle& rect) const
	{
		rect.m_iX = FindLeft(iStartX, iStartY, uOccWidth, puOccluding);
		rect.m_iWidth = (FindRight(iStartX, iStartY, uOccWidth, puOccluding) - rect.m_iX);
	}

	/**
	 * Part of FindVertical, find the bottommost occlusion pixel
	 * starting from iStartY, with extents already defined
	 * in rect.
	 */
	void FindBottom(Int32 iStartY, UInt32 const* puOccluding, UInt32 uOccWidth, OcclusionRectangle& rect) const
	{
		auto const uStartOccX = OccOffsetFromImgX(rect.m_iX);
		auto const uEndOccX = OccOffsetFromImgXCeil(rect.m_iX + rect.m_iWidth);
		auto uStartMask = ~(OccBitFromImgX(rect.m_iX) - 1);
		auto uEndMask = (OccBitFromImgX(rect.m_iX + rect.m_iWidth) - 1);
		// Falls on boundary, set to full.
		if (0 == uEndMask)
		{
			uEndMask = UIntMax;
		}
		// Single entry, combine.
		if (uStartOccX + 1 == uEndOccX)
		{
			uStartMask &= uEndMask;
			uEndMask &= uStartMask;
		}

		Int32 iY = iStartY;
		for (; iY + 1 < (Int32)m_uHeight; ++iY)
		{
			auto const p = Scanline(uOccWidth, puOccluding, (iY + 1));
			UInt32 uOccX = uStartOccX;

			// Start is special.
			if (uOccX < uEndOccX)
			{
				if (uStartMask != (uStartMask & p[uOccX]))
				{
					goto done;
				}
			}
			++uOccX;
			for (; uOccX + 1u < uEndOccX; ++uOccX)
			{
				if (UIntMax != p[uOccX])
				{
					goto done;
				}
			}
			// End is special.
			if (uOccX < uEndOccX)
			{
				if (uEndMask != (uEndMask & p[uOccX]))
				{
					goto done;
				}
			}
		}

	done:
		rect.m_iHeight = (iY - rect.m_iY) + 1;
	}

	/**
	 * Part of FindVertical, find the topmost occlusion pixel
	 * starting from iStartY, with extents already defined
	 * in rect.
	 */
	void FindTop(Int32 iStartY, UInt32 const* puOccluding, UInt32 uOccWidth, OcclusionRectangle& rect) const
	{
		auto const uStartOccX = OccOffsetFromImgX(rect.m_iX);
		auto const uEndOccX = OccOffsetFromImgXCeil(rect.m_iX + rect.m_iWidth);
		auto uStartMask = ~(OccBitFromImgX(rect.m_iX) - 1);
		auto uEndMask = (OccBitFromImgX(rect.m_iX + rect.m_iWidth) - 1);
		// Falls on boundary, set to full.
		if (0 == uEndMask)
		{
			uEndMask = UIntMax;
		}
		// Single entry, combine.
		if (uStartOccX + 1 == uEndOccX)
		{
			uStartMask &= uEndMask;
			uEndMask &= uStartMask;
		}

		Int32 iY = iStartY;
		for (; iY > 0; --iY)
		{
			auto const p = Scanline(uOccWidth, puOccluding, (iY - 1));
			UInt32 uOccX = uStartOccX;

			// Start is special.
			if (uOccX < uEndOccX)
			{
				if (uStartMask != (uStartMask & p[uOccX]))
				{
					goto done;
				}
			}
			++uOccX;
			for (; uOccX + 1u < uEndOccX; ++uOccX)
			{
				if (UIntMax != p[uOccX])
				{
					goto done;
				}
			}
			// End is special.
			if (uOccX < uEndOccX)
			{
				if (uEndMask != (uEndMask & p[uOccX]))
				{
					goto done;
				}
			}
		}

	done:
		rect.m_iY = iY;
	}

	/**
	 * Part of ComputeOcclusionRectangle(), find the Y and Height
	 * starting from iStartY. FindHorizontal() must have been
	 * called on rect previously.
	 */
	void FindVertical(Int32 iStartY, UInt32 uOccWidth, UInt32 const* puOccluding, OcclusionRectangle& rect) const
	{
		FindTop(iStartY, puOccluding, uOccWidth, rect);
		FindBottom(iStartY, puOccluding, uOccWidth, rect);
	}

	/**
	 * Part of ComputeOcclusionRectangle(), apply the rectangle to the visited
	 * array.
	 */
	void TouchVisited(UInt32* puVisited, UInt32 uOccWidth, const OcclusionRectangle& rect) const
	{
		auto const uStartOccX = OccOffsetFromImgX(rect.m_iX);
		auto const uEndOccX = OccOffsetFromImgXCeil(rect.m_iX + rect.m_iWidth);
		auto uStartMask = ~(OccBitFromImgX(rect.m_iX) - 1);
		auto uEndMask = (OccBitFromImgX(rect.m_iX + rect.m_iWidth) - 1);
		// Falls on boundary, set to full.
		if (0 == uEndMask)
		{
			uEndMask = UIntMax;
		}
		// Single entry, combine.
		if (uStartOccX + 1 == uEndOccX)
		{
			uStartMask &= uEndMask;
			uEndMask &= uStartMask;
		}

		for (Int32 iY = rect.m_iY; iY < (rect.m_iY + rect.m_iHeight); ++iY)
		{
			auto p = Scanline(uOccWidth, puVisited, iY);
			UInt32 uOccX = uStartOccX;

			// Start is special.
			if (uOccX < uEndOccX)
			{
				p[uOccX] |= uStartMask;
			}
			++uOccX;
			for (; uOccX + 1u < uEndOccX; ++uOccX)
			{
				p[uOccX] |= UIntMax;
			}
			// End is special.
			if (uOccX < uEndOccX)
			{
				p[uOccX] |= uEndMask;
			}
		}
	}

	/** Part of GetOcclusionRegion, computes an opaque region starting at iX, iY. */
	OcclusionRectangle ComputeOcclusionRectangle(
		Int32 iStartX,
		Int32 iStartY,
		UInt32 uOccWidth,
		UInt32* puVisited,
		UInt32 const* puOccluding) const
	{
		OcclusionRectangle ret;
		FindHorizontal(iStartX, iStartY, uOccWidth, puOccluding, ret);
		FindVertical(iStartY, uOccWidth, puOccluding, ret);
		TouchVisited(puVisited, uOccWidth, ret);
		return ret;
	}

	UInt8* m_pData{};
	UInt32 m_uWidth{};
	UInt32 m_uHeight{};

	SEOUL_DISABLE_COPY(Image);
}; // class Image

struct CookParameters SEOUL_SEALED
{
	Bool m_bCompress{};
	Bool m_bPad{};
	Bool m_bResize{};
	Int32 m_iInnerWidth{};
	Int32 m_iInnerHeight{};
	Int32 m_iOuterWidth{};
	Int32 m_iOuterHeight{};
}; // struct CookParameters

void* CrnRealloc(void* p, size_t zSize, size_t* pzActualSize, Bool bMovable, void*)
{
	if (nullptr != pzActualSize) { *pzActualSize = zSize; }

	if (0u == zSize)
	{
		MemoryManager::Deallocate(p);
		return nullptr;
	}
	else
	{
		return MemoryManager::ReallocateAligned(p, zSize, CRNLIB_MIN_ALLOC_ALIGNMENT, MemoryBudgets::Cooking);
	}
}

size_t CrnMsize(void* p, void*)
{
	return MemoryManager::GetAllocationSizeInBytes(p);
}

} // namespace anonymous

class TextureCookTask SEOUL_SEALED : public BaseCookTask
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(TextureCookTask);

	TextureCookTask()
	{
		crn_set_memory_callbacks(CrnRealloc, CrnMsize, nullptr);
		crn_disable_console();
	}

	virtual Bool CanCook(FilePath filePath) const SEOUL_OVERRIDE
	{
		return IsTextureFileType(filePath.GetType());
	}

	virtual Bool CookAllOutOfDateContent(ICookContext& rContext) SEOUL_OVERRIDE
	{
		ContentFiles v;
		if (!DefaultOutOfDateCookMulti(rContext, FileType::FIRST_TEXTURE_TYPE, FileType::LAST_TEXTURE_TYPE, v, true))
		{
			return false;
		}

		return true;
	}

	virtual Int32 GetPriority() const SEOUL_OVERRIDE
	{
		return CookPriority::kTexture;
	}

private:
	SEOUL_DISABLE_COPY(TextureCookTask);

	/**
	* @return The ImageAlphaType of image, based on its contents.
	* @param[in] eInitialImageAlphaType The cook format based on the pixel format of the input image.
	*
	* \pre image must already been in RGBA format.
	*/
	static ImageAlphaType::Enum DeriveImageAlphaType(
		ImageAlphaType::Enum eInitialImageAlphaType,
		const Image& image)
	{
		// Number of bins in the histogram used to analyze the texture alpha.
		const Int32 kBinCount = 32;
		const Int32 kDenominator = (256 / kBinCount);

		// If the image is not RgbNoAlpha, scan the pixels to determine
		// if it can be RgbaMaskAlpha or RgbFullAlpha.
		if (ImageAlphaType::kRgbNoAlpha != eInitialImageAlphaType)
		{
			// Initialize the histogram to all 0s.
			FixedArray<Int32, kBinCount> aHistogram;

			// Walk the image alpha values and insert them into the histogram.
			UInt32 const uSize = image.GetSizeInBytes();
			for (UInt32 i = 3u; i < uSize; i += 4u)
			{
				aHistogram[(Int32)image.GetData()[i] / kDenominator]++;
			}

			for (Int32 i = 1; i < kBinCount - 1; ++i)
			{
				// If any pixels fell into an interior bin, consider the image full alpha.
				if (aHistogram[i] != 0)
				{
					return ImageAlphaType::kRgbFullAlpha;
				}
			}

			// If we get here, the image is either opaque or alpha masked.

			// If no pixels in the lowest bin, than we can consider the image opaque.
			if (aHistogram[0] == 0)
			{
				return ImageAlphaType::kRgbNoAlpha;
			}
			// Otherwise consider it alpha masked.
			else
			{
				return ImageAlphaType::kRgbMaskAlpha;
			}
		}

		return eInitialImageAlphaType;
	}

	/**
	 * Encase a texture in a DDS file container, and optionally apply compression.
	 * For compression, input image must be:
	 * - RGBA format.
	 * - power of two width and height.
	 * - a minimum of 4x4 pixels.
	 */
	void* EncodeImage(const Image& image, TextureEncodingType::Enum eType, UInt32& ru) const
	{
		Byte* pReturn = nullptr;
		if (!EncodeTexture(
			image.GetData(),
			image.GetWidth(),
			image.GetHeight(),
			eType,
			&pReturn,
			&ru))
		{
			return nullptr;
		}

		return pReturn;
	}

	/**
	* Handle cooking a texture file in sInputFilename to a platform specific
	* format, stored in sOutputFilename.
	*
	* @return True if the cook was successful, false otherwise.
	*/
	Bool DoPlatformDependentCook(
		CookParameters cookParameters,
		const Image& image,
		Platform ePlatform,
		ImageAlphaType::Enum eImageAlphaType,
		FilePath filePath,
		void*& rpData,
		UInt32& ruData) const
	{
		// Pick the encoding type.
		TextureEncodingType::Enum eEncodingType = TextureEncodingType::kA8R8G8B8;
		if (cookParameters.m_bCompress)
		{
			// ETC1 compression on Android and iOS.
			if (Platform::kAndroid == ePlatform || Platform::kIOS == ePlatform)
			{
				eEncodingType = TextureEncodingType::kETC1;
			}
			// DXT* on other platforms.
			else
			{
				switch (eImageAlphaType)
				{
				case ImageAlphaType::kRgbFullAlpha: eEncodingType = TextureEncodingType::kDXT5; break;
				case ImageAlphaType::kRgbMaskAlpha: eEncodingType = TextureEncodingType::kDXT5; break;
				case ImageAlphaType::kRgbNoAlpha: eEncodingType = TextureEncodingType::kDXT1; break;
				default: eEncodingType = TextureEncodingType::kDXT1; break;
				}
			}
		}
		// Otherwise, without compression, just use kA8R8G8B8
		else
		{
			eEncodingType = TextureEncodingType::kA8R8G8B8;
		}

		// TODO: Move this step into the native function.
		// Special handling of ETC1 compressed data
		void* pCompressedData = nullptr;
		UInt32 uCompressedData = 0u;
		if (TextureEncodingType::kETC1 == eEncodingType)
		{
			// Always generate an image for the opaque data.
			{
				// Clone the image into rgbImage.
				Image rgbImage;
				image.CloneTo(rgbImage);

				// Set the alpha channel to 255.
				for (UInt32 i = 3u; i < rgbImage.GetSizeInBytes(); i += 4u)
				{
					rgbImage.GetData()[i] = 255;
				}

				// Compress the data - the color channel of mip0 textures
				// using clustered compression. We always use unclustered
				// compression for alpha or at other mip levels.
				auto const eCompressionType = (filePath.GetType() == FileType::kTexture0
					? TextureEncodingType::kETC1Clustered
					: TextureEncodingType::kETC1);
				pCompressedData = EncodeImage(rgbImage, eCompressionType, uCompressedData);
			}

			// If compression of the RGB channels succeeded and
			// if we need a second alpha image, generate and append that.
			if (nullptr != pCompressedData && ImageAlphaType::kRgbNoAlpha != eImageAlphaType)
			{
				// Clone the original image.
				Image alphaImage;
				image.CloneTo(alphaImage);

				// Set the alpha channel to the RGB channels and set the alpha channel
				// to 255 for the compressor.
				for (UInt32 i = 0u; i < alphaImage.GetSizeInBytes(); i += 4u)
				{
					UInt8 a = alphaImage.GetData()[i + 3u];
					alphaImage.GetData()[i + 0] = a;
					alphaImage.GetData()[i + 1] = a;
					alphaImage.GetData()[i + 2] = a;
					alphaImage.GetData()[i + 3] = 255;
				}

				// Compress the data.
				UInt32 uCompressedAlphaData = 0u;
				void* pCompressedAlphaData = EncodeImage(alphaImage, eEncodingType, uCompressedAlphaData);

				// If compression succeeded, append the alpha data to the end
				// of the RGB data and use the combined buffer as the returned output.
				if (nullptr == pCompressedAlphaData)
				{
					pCompressedData = nullptr;
					uCompressedData = 0u;
				}
				else
				{
					pCompressedData = MemoryManager::Reallocate(
						pCompressedData,
						uCompressedData + uCompressedAlphaData,
						MemoryBudgets::Cooking);
					memcpy((UInt8*)pCompressedData + uCompressedData, pCompressedAlphaData, uCompressedAlphaData);
					uCompressedData += uCompressedAlphaData;
					MemoryManager::Deallocate(pCompressedAlphaData);
					pCompressedAlphaData = nullptr;
					uCompressedAlphaData = 0u;
				}
			}
		}
		else
		{
			pCompressedData = EncodeImage(image, eEncodingType, uCompressedData);
		}

		// If compression failed, the overall operation failed.
		if (nullptr == pCompressedData)
		{
			SEOUL_LOG_COOKING("%s: image compression failed.", filePath.CStr());
			return false;
		}

		rpData = pCompressedData;
		ruData = uCompressedData;
		return true;
	}

	/**
	 * Load the contents of an image file in sInputFilename into image.
	 *
	 * @return True on success, false otherwise.
	 *
	 * Independent of the input format of sInputFilename, if successful, image
	 * will contain data that is top-to-bottom and the RGBA pixel format.
	 */
	static Bool LoadImage(
		FilePath filePath,
		Image& rImage,
		ImageAlphaType::Enum& reImageAlphaType,
		Bool bSourceContainsPremultipliedAlpha)
	{
		reImageAlphaType = ImageAlphaType::kUnknown;
		if (!rImage.Load(filePath, reImageAlphaType))
		{
			return false;
		}

		// Derive the texture cook format from the initial format and
		// the contents of the image.
		reImageAlphaType = DeriveImageAlphaType(reImageAlphaType, rImage);

		// Premultiply alpha before performing further processing, if the image
		// was not RGB (no alpha channel)
		// If an RGBA target, pre-multiply the alpha.
		if (!bSourceContainsPremultipliedAlpha &&
			ImageAlphaType::kRgbNoAlpha != reImageAlphaType)
		{
			rImage.PremultiplyAlpha();
		}

		return true;
	}

	/**
	 * Resize the image rImage to the specified width and height, using the default
	 * resampler.
	 */
	Bool Resample(Image& rImage, UInt32 uTargetWidth, UInt32 uTargetHeight) const
	{
		// Iterate until we hit the target dimensions - next resize
		// more than 2x (increase or decrease).
		while (rImage.GetWidth() != uTargetWidth || rImage.GetHeight() != uTargetHeight)
		{
			// Initial resample targets are the overall targets.
			UInt32 uResampleWidth = uTargetWidth;
			UInt32 uResampleHeight = uTargetHeight;

			// If we're sampling up...
			if (uResampleWidth > rImage.GetWidth())
			{
				// If we're going to more than double the image
				// width in a single resample, clamp it to
				// 2x.
				if (rImage.GetWidth() * 2u < uResampleWidth)
				{
					uResampleWidth = rImage.GetWidth() * 2u;
				}
			}
			// Otherwise, we're sampling down.
			else
			{
				// If we're going to less than halve the image
				// width in a single resample, clamp it to
				// half.
				if (rImage.GetWidth() / 2u > uResampleWidth)
				{
					uResampleWidth = Max((UInt32)Ceil(rImage.GetWidth() / 2.0f), 1u);
				}
			}

			// If we're sampling up...
			if (uResampleHeight > rImage.GetHeight())
			{
				// If we're going to more than double the image
				// height in a single resample, clamp it to
				// 2x.
				if (rImage.GetHeight() * 2u < uResampleHeight)
				{
					uResampleHeight = rImage.GetHeight() * 2u;
				}
			}
			// Otherwise, we're sampling down.
			else
			{
				// If we're going to less than halve the image
				// height in a single resample, clamp it to
				// half.
				if (rImage.GetHeight() / 2u > uResampleHeight)
				{
					uResampleHeight = Max((UInt32)Ceil(rImage.GetHeight() / 2.0f), 1u);
				}
			}

			if (!ResizeImage(
				rImage,
				uResampleWidth,
				uResampleHeight))
			{
				return false;
			}
		}

		return true;
	}

	/**
	 * Resize the image rImage to the specified width and height, using the SeoulEngine
	 * native library.
	 *
	 * \pre rImage must be top-to-bottom and in RGBA format.
	 */
	Bool ResizeImage(
		Image& rImage,
		UInt32 uTargetWidth,
		UInt32 uTargetHeight) const
	{
		UInt32 const uOutputSize = uTargetWidth * uTargetHeight * 4u;
		UInt8* pOutput = (UInt8*)MemoryManager::Allocate(uOutputSize, MemoryBudgets::Cooking);

		// Settings here are all intentional, change with care - in particular,
		// our current pipeline leverages premultiplied alpha textures that are
		// not gamma correct (we blend gamma values in linear space). Consistent
		// wrong is better than inconsistent right.
		Bool const bResult = (0 != stbir__resize_arbitrary(
			nullptr,
			rImage.GetData(),
			rImage.GetWidth(),
			rImage.GetHeight(),
			(rImage.GetWidth() * 4u),
			pOutput,
			uTargetWidth,
			uTargetHeight,
			(uTargetWidth * 4u),
			0.0f, 0.0f, 1.0f, 1.0f,
			nullptr,
			4,
			-1,
			STBIR_FLAG_ALPHA_PREMULTIPLIED,
			STBIR_TYPE_UINT8,
			STBIR_FILTER_DEFAULT,
			STBIR_FILTER_DEFAULT,
			STBIR_EDGE_CLAMP,
			STBIR_EDGE_CLAMP,
			STBIR_COLORSPACE_LINEAR));

		if (bResult)
		{
			rImage.TakeOwnership((UInt8*)pOutput, uTargetWidth, uTargetHeight);
		}
		else
		{
			MemoryManager::Deallocate(pOutput);
			pOutput = nullptr;
		}

		return bResult;
	}

	/**
	 * Assign parameters used to determine the behavior of cooking to rbCompress,
	 * rbGenerateMips, rbResize, riTargetWidth and riTargetHeight.
	 *
	 * @return True if image is ready for cooking, false otherwise.
	 */
	Bool GetCookParameters(
		FilePath filePath,
		UInt32 uOrigImageWidth,
		UInt32 uOrigImageHeight,
		Platform ePlatform,
		ImageAlphaType::Enum eImageAlphaType,
		Int32 iOutputMipLevel,
		CookParameters& rCookParameters) const
	{
		// Validate that the texture is not 0 size.
		if (0u == uOrigImageWidth || 0u == uOrigImageHeight)
		{
			SEOUL_LOG_COOKING("%s: invalid image for texture cook, width or height is 0.", filePath.CStr());
			return false;
		}

		// Get the image size - this may be half the original image size if
		// the target platform reduces mip 0 to a quarter.
		Int32 iInnerWidth = (Int32)uOrigImageWidth;
		Int32 iInnerHeight = (Int32)uOrigImageHeight;

		// Account for mip argument - don't do this for atlasses, they were generated at the appropriate
		// mip level
		if (iOutputMipLevel >= 1)
		{
			// Reduce the inner size by half until we hit the target mip level.
			Int32 iMipLevel = iOutputMipLevel;
			while (iMipLevel > 0)
			{
				iInnerWidth = Max((Int32)Ceil(iInnerWidth / 2.0f), 1);
				iInnerHeight = Max((Int32)Ceil(iInnerHeight / 2.0f), 1);
				iMipLevel--;
			}
		}

		// Set the compression setting based on the mip adjusted size of the image (we don't compress any images at or below our threshold).
		rCookParameters.m_bCompress = NeedsCompression(
			ePlatform,
			iOutputMipLevel,
			iInnerWidth,
			iInnerHeight);

		// Setup the default cook parameters.
		rCookParameters.m_bPad = false;
		rCookParameters.m_bResize = false;
		rCookParameters.m_iInnerWidth = iInnerWidth;
		rCookParameters.m_iInnerHeight = iInnerHeight;
		rCookParameters.m_iOuterWidth = GetBestPowerOfTwo(iInnerWidth);
		rCookParameters.m_iOuterHeight = GetBestPowerOfTwo(iInnerHeight);

		// If not compressing and if we've rounded up either dimension, use the starting size.
		if (Platform::kAndroid == ePlatform || Platform::kIOS == ePlatform)
		{
			// Track whether the image size was increased from its starting size.
			Bool bEnlarged = (
				rCookParameters.m_iOuterWidth > iInnerWidth ||
				rCookParameters.m_iOuterHeight > iInnerHeight);

			if (!rCookParameters.m_bCompress && bEnlarged)
			{
				rCookParameters.m_iOuterWidth = iInnerWidth;
				rCookParameters.m_iOuterHeight = iInnerHeight;
			}
		}

		// Clamp the inner dimensions to the outer dimensions (this can happen if the best power of 2
		// was chosen to be slightly smaller than the original image).
		rCookParameters.m_iInnerWidth = Min(
			rCookParameters.m_iInnerWidth,
			rCookParameters.m_iOuterWidth);
		rCookParameters.m_iInnerHeight = Min(
			rCookParameters.m_iInnerHeight,
			rCookParameters.m_iOuterHeight);

		// Make sure, after clamping, that we maintain the aspect ratio of the original image.
		Float32 fInnerAspectRatio = (Float32)rCookParameters.m_iInnerWidth / (Float32)rCookParameters.m_iInnerHeight;
		Float32 fImageAspectRatio = (Float32)((Int32)uOrigImageWidth) / (Float32)((Int32)uOrigImageHeight);
		if (!Equals(fImageAspectRatio, fInnerAspectRatio, 0.01f))
		{
			if (uOrigImageWidth > uOrigImageHeight)
			{
				Int32 iNewInnerHeight = Max(
					(Int32)(rCookParameters.m_iInnerWidth / fImageAspectRatio),
					1);
				if (iNewInnerHeight > rCookParameters.m_iOuterHeight)
				{
					rCookParameters.m_iInnerWidth = Max((Int32)(
						rCookParameters.m_iInnerHeight * fImageAspectRatio),
						1);
				}
				else
				{
					rCookParameters.m_iInnerHeight = iNewInnerHeight;
				}
			}
			else
			{
				Int32 iNewInnerWidth = Max(
					(Int32)(rCookParameters.m_iInnerHeight * fImageAspectRatio),
					1);
				if (iNewInnerWidth > rCookParameters.m_iOuterWidth)
				{
					rCookParameters.m_iInnerHeight = Max(
						(Int32)(rCookParameters.m_iInnerWidth / fImageAspectRatio),
						1);
				}
				else
				{
					rCookParameters.m_iInnerWidth = iNewInnerWidth;
				}
			}
		}

		// Pad if the inner dimensions are not the same as the outer dimensions
		rCookParameters.m_bPad =
			(rCookParameters.m_iInnerWidth != rCookParameters.m_iOuterWidth) ||
			(rCookParameters.m_iInnerHeight != rCookParameters.m_iOuterHeight);

		// Resize if the dimensions have changed.
		rCookParameters.m_bResize =
			(rCookParameters.m_iInnerWidth != (Int32)uOrigImageWidth) ||
			(rCookParameters.m_iInnerHeight != (Int32)uOrigImageHeight);

		return true;
	}

	/**
	* Insert a texture footer structure at the end of the texture data.
	*/
	void AddTextureFooter(
		Platform ePlatform,
		Float32 fTexcoordsScaleU,
		Float32 fTexcoordsScaleV,
		Float32 fVisibleRegionScaleU,
		Float32 fVisibleRegionScaleV,
		Float32 fVisibleRegionOffsetU,
		Float32 fVisibleRegionOffsetV,
		Float32 fOcclusionRegionScaleU,
		Float32 fOcclusionRegionScaleV,
		Float32 fOcclusionRegionOffsetU,
		Float32 fOcclusionRegionOffsetV,
		void*& rp,
		UInt32& ru) const
	{
		if (nullptr == rp)
		{
			return;
		}

		// TODO: Big endian support.

		TextureFooter textureFooter;
		textureFooter.m_uSignature = kTextureFooterSignature;
		textureFooter.m_uVersion = kTextureFooterVersion;
		textureFooter.m_fTexcoordsScaleU = Clamp(fTexcoordsScaleU, 0.0f, 1.0f);
		textureFooter.m_fTexcoordsScaleV = Clamp(fTexcoordsScaleV, 0.0f, 1.0f);
		textureFooter.m_fVisibleRegionScaleU = Clamp(fVisibleRegionScaleU, 0.0f, 1.0f);
		textureFooter.m_fVisibleRegionScaleV = Clamp(fVisibleRegionScaleV, 0.0f, 1.0f);
		textureFooter.m_fVisibleRegionOffsetU = Clamp(fVisibleRegionOffsetU, 0.0f, 1.0f);
		textureFooter.m_fVisibleRegionOffsetV = Clamp(fVisibleRegionOffsetV, 0.0f, 1.0f);
		textureFooter.m_fOcclusionRegionScaleU = Clamp(fOcclusionRegionScaleU, 0.0f, 1.0f);
		textureFooter.m_fOcclusionRegionScaleV = Clamp(fOcclusionRegionScaleV, 0.0f, 1.0f);
		textureFooter.m_fOcclusionRegionOffsetU = Clamp(fOcclusionRegionOffsetU, 0.0f, 1.0f);
		textureFooter.m_fOcclusionRegionOffsetV = Clamp(fOcclusionRegionOffsetV, 0.0f, 1.0f);

		auto const uSize = ru;
		rp = MemoryManager::Reallocate(rp, uSize + sizeof(textureFooter), MemoryBudgets::Cooking);
		memcpy((Byte*)rp + uSize, &textureFooter, sizeof(textureFooter));
		ru += sizeof(textureFooter);
	}

	Bool SourceIsPremultiplied(FilePath filePath) const
	{
		auto const sRelative(filePath.GetRelativeFilenameWithoutExtension().ToString());

		Bool b = false;
		b = b || sRelative.StartsWithASCIICaseInsensitive("Generated"); // TODO: Don't hard code.
		b = b || sRelative.EndsWith("_nopre");
		return b;
	}

	virtual Bool InternalCook(ICookContext& rContext, FilePath filePath) SEOUL_OVERRIDE
	{
		return InternalCookMulti(rContext, &filePath, &filePath + 1);
	}

	virtual Bool InternalCookMulti(ICookContext& rContext, FilePath const* p, FilePath const* const pEnd) SEOUL_OVERRIDE
	{
		auto const bSourceContainsPremultipliedAlpha = SourceIsPremultiplied(*p);

		Image image;
		ImageAlphaType::Enum eImageAlphaType = ImageAlphaType::kUnknown;
		if (!LoadImage(
			*p,
			image,
			eImageAlphaType,
			bSourceContainsPremultipliedAlpha))
		{
			return false;
		}

		// Capture width/height.
		auto const uOrigImageWidth = image.GetWidth();
		auto const uOrigImageHeight = image.GetHeight();
		for (; pEnd != p; ++p)
		{
			if (!DoCook(rContext, *p, uOrigImageWidth, uOrigImageHeight, image, eImageAlphaType))
			{
				return false;
			}
		}

		return true;
	}

	Bool DoCook(
		ICookContext& rContext,
		FilePath filePath,
		UInt32 uOrigImageWidth,
		UInt32 uOrigImageHeight,
		Image& rResampledImage,
		ImageAlphaType::Enum eImageAlphaType) const
	{
		// Sanity check the input image size - if larger than the largest we support,
		// fail cooking.
		UInt32 const uMaxInputImageDimension = GetMaxInputImageDimension(rContext.GetPlatform());
		if (uOrigImageWidth > uMaxInputImageDimension ||
			uOrigImageHeight > uMaxInputImageDimension)
		{
			SEOUL_LOG_COOKING("%s: invalid image, width or height is greater than %u.", filePath.CStr(), uMaxInputImageDimension);
			return false;
		}

		// Get the properties used to perform the rest of cooking.
		Int32 const iOutputMipLevel = ((Int32)filePath.GetType() - (Int32)FileType::FIRST_TEXTURE_TYPE);
		CookParameters cookParameters;
		if (!GetCookParameters(
			filePath,
			uOrigImageWidth,
			uOrigImageHeight,
			rContext.GetPlatform(),
			eImageAlphaType,
			iOutputMipLevel,
			cookParameters))
		{
			return false;
		}

		// Before pre-multiplying the alpha, resample the image if necessary.
		if (cookParameters.m_bResize)
		{
			// Multi-op resampling depends on the order of Filepaths in each multi-op
			// to be ordered from largest mip level (.sif0) to smallest (currently .sif4).
			SEOUL_ASSERT((Int)rResampledImage.GetHeight() >= cookParameters.m_iInnerHeight);
			SEOUL_ASSERT((Int)rResampledImage.GetWidth() >= cookParameters.m_iInnerWidth);
			if ((Int)rResampledImage.GetHeight() < cookParameters.m_iInnerHeight ||
				(Int)rResampledImage.GetWidth() < cookParameters.m_iInnerWidth)
			{
				SEOUL_LOG_COOKING("%s: failed resizing image to (%d x %d), resample image is already too small at (%u x %u), "
					"this is a cook logic error and should never happen.",
					filePath.CStr(),
					cookParameters.m_iInnerWidth, cookParameters.m_iInnerHeight,
					rResampledImage.GetWidth(), rResampledImage.GetHeight());
				return false;
			}

			// If resizing fails, cooking fails.
			if (!Resample(
				rResampledImage,
				cookParameters.m_iInnerWidth,
				cookParameters.m_iInnerHeight))
			{
				SEOUL_LOG_COOKING("%s: failed resizing image to (%d x %d).", filePath.CStr(), cookParameters.m_iInnerWidth, cookParameters.m_iInnerHeight);
				return false;
			}
			// Otherwise, make sure to let the rest of the process know that the image
			// no longer needs to be resized.
			else
			{
				cookParameters.m_bResize = false;
			}
		}

		Int32 iOcclusionRectangleLeft = 0;
		Int32 iOcclusionRectangleTop = 0;
		Int32 iOcclusionRectangleWidth = 0;
		Int32 iOcclusionRectangleHeight = 0;
		Int32 iVisibleRectangleLeft = 0;
		Int32 iVisibleRectangleTop = 0;
		Int32 iVisibleRectangleWidth = 0;
		Int32 iVisibleRectangleHeight = 0;

		// Early out case - if image is opaque, occlusion rectangle
		// and visible rectangle are the entire image.
		if (eImageAlphaType == ImageAlphaType::kRgbNoAlpha)
		{
			iOcclusionRectangleLeft = iVisibleRectangleLeft = 0;
			iOcclusionRectangleTop = iVisibleRectangleTop = 0;
			iOcclusionRectangleWidth = iVisibleRectangleWidth = (Int32)rResampledImage.GetWidth();
			iOcclusionRectangleHeight = iVisibleRectangleHeight = (Int32)rResampledImage.GetHeight();
		}
		else
		{
			// Get the occlusion rectangle.
			rResampledImage.GetOcclusionRegion(
				iOcclusionRectangleLeft,
				iOcclusionRectangleTop,
				iOcclusionRectangleWidth,
				iOcclusionRectangleHeight);

			// Get the visible rectangle for further processing prior to padding.
			rResampledImage.GetVisibleRegion(
				iVisibleRectangleLeft,
				iVisibleRectangleTop,
				iVisibleRectangleWidth,
				iVisibleRectangleHeight);
		}

		// Potentially generating a new (padded image), or using
		// the current resampled image.
		CheckedPtr<Image> pImage(&rResampledImage);

		// Pad the image after pre-multiplying if necessary.
		Image buf;
		if (cookParameters.m_bPad)
		{
			buf.MakeEmpty(cookParameters.m_iOuterWidth, cookParameters.m_iOuterHeight);

			// If padding fails, return failure.
			if (!buf.PasteSubImage(0, 0, rResampledImage))
			{
				SEOUL_LOG_COOKING("%s: failed pasting image of (%u x %u) into (%u x %u) for padding.",
					filePath.CStr(),
					rResampledImage.GetWidth(),
					rResampledImage.GetHeight(),
					buf.GetWidth(),
					buf.GetHeight());
				return false;
			}
			// Otherwise, finish padding and make sure to let the rest of the process know that the image
			// no longer needs to be padded.
			else
			{
				// Pad the image to avoid sampling artifacts.
				Int32 const iPadIterations = Max(
					(Int32)buf.GetHeight() - (Int32)rResampledImage.GetHeight(),
					(Int32)buf.GetWidth() - (Int32)rResampledImage.GetWidth());

				// Initial pad x/y values are equal to the inner width and height.
				Int32 iPadX = (Int32)rResampledImage.GetWidth();
				Int32 iPadY = (Int32)rResampledImage.GetHeight();
				for (Int32 i = 0; i < iPadIterations; ++i)
				{
					// Extend the bottom if the padded image is smaller than the total image along the y axis.
					if (iPadY < (Int32)buf.GetHeight())
					{
						Int32 const iPadWidth = Min(iPadX, (Int32)buf.GetWidth());
						for (Int32 x = 0; x < iPadWidth; ++x)
						{
							Int32 const iFrom = ((iPadY - 1) * (Int32)buf.GetWidth() * 4) + (x * 4);
							Int32 const iTo = (iPadY * (Int32)buf.GetWidth() * 4) + (x * 4);

							// Copy through all 4 components of the image.
							for (Int32 iComponent = 0; iComponent < 4; ++iComponent)
							{
								buf.GetData()[iTo + iComponent] = buf.GetData()[iFrom + iComponent];
							}
						}
					}

					// Extend the right if the padded image is smaller than the total image along the x axis.
					if (iPadX < (Int32)buf.GetWidth())
					{
						Int32 const iPadHeight = Min(iPadY, (Int32)buf.GetHeight());
						for (Int32 y = 0; y < iPadHeight; ++y)
						{
							Int32 const iFrom = (y * (Int32)buf.GetWidth() * 4) + ((iPadX - 1) * 4);
							Int32 const iTo = (y * (Int32)buf.GetWidth() * 4) + (iPadX * 4);

							// Copy through all 4 components of the image.
							for (Int32 iComponent = 0; iComponent < 4; ++iComponent)
							{
								buf.GetData()[iTo + iComponent] = buf.GetData()[iFrom + iComponent];
							}
						}
					}

					// If the padded image is smaller than the total image along both x and y, add a pixel corner.
					if (iPadX < (Int32)buf.GetWidth() && iPadY < (Int32)buf.GetHeight())
					{
						Int32 const iFrom = ((iPadY - 1) * (Int32)buf.GetWidth() * 4) + ((iPadX - 1) * 4);
						Int32 const iTo = (iPadY * (Int32)buf.GetWidth() * 4) + (iPadX * 4);

						// Copy through all 4 components of the image.
						for (Int32 iComponent = 0; iComponent < 4; ++iComponent)
						{
							buf.GetData()[iTo + iComponent] = buf.GetData()[iFrom + iComponent];
						}
					}

					// Moving along, the inner image that we're copying from is now 1 pixel
					// larger on a side.
					++iPadX;
					++iPadY;
				}

				// Pointer now at buf.
				pImage = &buf;

				// No longer needs to be padded.
				cookParameters.m_bPad = false;
			}
		}

		// Perform the actual platform dependent conversion.
		void* pCookedData = nullptr;
		UInt32 uCookedData = 0u;
		auto const deferredU(MakeDeferredAction([&]()
			{
				MemoryManager::Deallocate(pCookedData);
				pCookedData = nullptr;
				uCookedData = 0u;
			}));

		if (!DoPlatformDependentCook(
			cookParameters,
			*pImage,
			rContext.GetPlatform(),
			eImageAlphaType,
			filePath,
			pCookedData,
			uCookedData))
		{
			SEOUL_LOG_COOKING("%s: encoding failed, compression error.", filePath.CStr());
			return false;
		}

		// Add a texture footer to the image, describing the subregion of the texture.
		AddTextureFooter(
			rContext.GetPlatform(),
			(Float32)cookParameters.m_iInnerWidth / (Float32)cookParameters.m_iOuterWidth,
			(Float32)cookParameters.m_iInnerHeight / (Float32)cookParameters.m_iOuterHeight,
			(Float32)iVisibleRectangleWidth / (Float32)cookParameters.m_iInnerWidth,
			(Float32)iVisibleRectangleHeight / (Float32)cookParameters.m_iInnerHeight,
			(Float32)iVisibleRectangleLeft / (Float32)cookParameters.m_iInnerWidth,
			(Float32)iVisibleRectangleTop / (Float32)cookParameters.m_iInnerHeight,
			(Float32)iOcclusionRectangleWidth / (Float32)cookParameters.m_iInnerWidth,
			(Float32)iOcclusionRectangleHeight / (Float32)cookParameters.m_iInnerHeight,
			(Float32)iOcclusionRectangleLeft / (Float32)cookParameters.m_iInnerWidth,
			(Float32)iOcclusionRectangleTop / (Float32)cookParameters.m_iInnerHeight,
			pCookedData,
			uCookedData);

		void* pC = nullptr;
		UInt32 uC = 0u;
		auto const deferredC(MakeDeferredAction([&]()
			{
				MemoryManager::Deallocate(pC);
				pC = nullptr;
				uC = 0u;
			}));

		if (!ZSTDCompress(pCookedData, uCookedData, pC, uC))
		{
			SEOUL_LOG_COOKING("%s: lossless disk compression with ZSTD failed.", filePath.CStr());
			return false;
		}

		return AtomicWriteFinalOutput(rContext, pC, uC, filePath);
	}
}; // class TextureCookTask

} // namespace Cooking

SEOUL_BEGIN_TYPE(Cooking::TextureCookTask, TypeFlags::kDisableCopy)
	SEOUL_PARENT(Cooking::BaseCookTask)
SEOUL_END_TYPE()


} // namespace Seoul

// Unit tests
#if SEOUL_UNIT_TESTS

#include "GamePaths.h"
#include "UnitTesting.h"
#include "UnitTestsFileManagerHelper.h"

namespace Seoul
{

namespace
{

struct OldImage
{
	/**
	 * Part of FindHorizontal, find the leftmost occlusion pixel
	 * starting from iStartX, iStartY.
	 */
	Int32 FindLeft(Int32 iStartX, Int32 iStartY, Bool* pbOccluding) const
	{
		Int32 iX = iStartX;
		for (; iX > 0; --iX)
		{
			if (!pbOccluding[iStartY * (Int32)m_uWidth + (iX - 1)])
			{
				return iX;
			}
		}

		return iX;
	}

	/**
	 * Part of FindHorizontal, find the leftmost occlusion pixel
	 * starting from iStartX, iStartY.
	 */
	Int32 FindRight(Int32 iStartX, Int32 iStartY, Bool* pbOccluding) const
	{
		Int32 iX = iStartX;
		for (; iX + 1 < (Int32)m_uWidth; ++iX)
		{
			if (!pbOccluding[iStartY * (Int32)m_uWidth + iX + 1])
			{
				return iX;
			}
		}

		return iX;
	}

	/**
	 * Part of ComputeOcclusionRectangle(), find the X and Width
	 * starting from iStartX.
	 */
	void FindHorizontal(Int32 iStartX, Int32 iStartY, Bool* pbOccluding, Cooking::Image::OcclusionRectangle& rect) const
	{
		rect.m_iX = FindLeft(iStartX, iStartY, pbOccluding);
		rect.m_iWidth = (FindRight(iStartX, iStartY, pbOccluding) - rect.m_iX) + 1;
	}

	/**
	 * Part of FindVertical, find the bottommost occlusion pixel
	 * starting from iStartY, with extents already defined
	 * in rect.
	 */
	void FindBottom(Int32 iStartY, Bool* pbOccluding, Cooking::Image::OcclusionRectangle& rect) const
	{
		Int32 iY = iStartY;
		for (; iY + 1 < (Int32)m_uHeight; ++iY)
		{
			for (Int32 iX = rect.m_iX; iX < (rect.m_iX + rect.m_iWidth); ++iX)
			{
				if (!pbOccluding[(iY + 1) * (Int32)m_uWidth + iX])
				{
					rect.m_iHeight = (iY - rect.m_iY) + 1;
					return;
				}
			}
		}

		rect.m_iHeight = (iY - rect.m_iY) + 1;
	}

	/**
	 * Part of FindVertical, find the topmost occlusion pixel
	 * starting from iStartY, with extents already defined
	 * in rect.
	 */
	void FindTop(Int32 iStartY, Bool* pbOccluding, Cooking::Image::OcclusionRectangle& rect) const
	{
		Int32 iY = iStartY;
		for (; iY > 0; --iY)
		{
			for (Int32 iX = rect.m_iX; iX < (rect.m_iX + rect.m_iWidth); ++iX)
			{
				if (!pbOccluding[(iY - 1) * (Int32)m_uWidth + iX])
				{
					rect.m_iY = iY;
					return;
				}
			}
		}

		rect.m_iY = iY;
	}

	/**
	 * Part of ComputeOcclusionRectangle(), find the Y and Height
	 * starting from iStartY. FindHorizontal() must have been
	 * called on rect previously.
	 */
	void FindVertical(Int32 iStartY, Bool* pbOccluding, Cooking::Image::OcclusionRectangle& rect) const
	{
		FindTop(iStartY, pbOccluding, rect);
		FindBottom(iStartY, pbOccluding, rect);
	}

	/**
	 * Part of ComputeOcclusionRectangle(), apply the rectangle to the visited
	 * array.
	 */
	void TouchVisited(Bool* pbVisited, const Cooking::Image::OcclusionRectangle& ret) const
	{
		for (Int32 iY = ret.m_iY; iY < (ret.m_iY + ret.m_iHeight); ++iY)
		{
			for (Int32 iX = ret.m_iX; iX < (ret.m_iX + ret.m_iWidth); ++iX)
			{
				pbVisited[iY * (Int32)m_uWidth + iX] = true;
			}
		}
	}

	/** Part of GetOcclusionRegion, computes an opaque region starting at iX, iY. */
	Cooking::Image::OcclusionRectangle ComputeOcclusionRectangle(
		Int32 iStartX,
		Int32 iStartY,
		Bool* pbVisited,
		Bool* pbOccluding) const
	{
		Cooking::Image::OcclusionRectangle ret;
		FindHorizontal(iStartX, iStartY, pbOccluding, ret);
		FindVertical(iStartY, pbOccluding, ret);
		TouchVisited(pbVisited, ret);
		return ret;
	}

	/**
	 * Find a single rectangle (the largest possible)
	 * that encloses pixels that are fully opaque.
	 */
	void GetOcclusionRegion(Int32& riX, Int32& riY, Int32& riWidth, Int32& riHeight, Cooking::Image::OcclusionRectangles* pvOut = nullptr) const
	{
		auto const uSize = (m_uWidth * m_uHeight);
		UnsafeBuffer<Bool, MemoryBudgets::Cooking> vbVisited;
		UnsafeBuffer<Bool, MemoryBudgets::Cooking> vbOccluding;

		// Fill visited (false by default).
		vbVisited.Assign(uSize, false);

		// Fill vbOccluding.
		vbOccluding.ResizeNoInitialize(uSize);
		for (UInt32 i = 0u; i < uSize; ++i)
		{
			auto const uA = m_pData[i * 4u + 3u];
			vbOccluding[i] = (uA >= ku8bitColorOcclusionThreshold);
		}

		Cooking::Image::OcclusionRectangle best;
		for (UInt32 uY = 0u; uY < m_uHeight; ++uY)
		{
			for (UInt32 uX = 0u; uX < m_uWidth; ++uX)
			{
				UInt32 const uIndex = uY * m_uWidth + uX;

				// Never start a new search from a pixel
				// we've already visited.
				if (vbVisited[uIndex])
				{
					continue;
				}

				// Don't start a search from a pixel
				// that doesn't occlude.
				if (!vbOccluding[uIndex])
				{
					continue;
				}

				// New search.
				auto const cur = ComputeOcclusionRectangle(uX, uY, vbVisited.Data(), vbOccluding.Data());
				if (pvOut) { pvOut->PushBack(cur); }
				if (cur.GetArea() > best.GetArea())
				{
					best = cur;
				}
			}
		}

		riX = best.m_iX;
		riY = best.m_iY;
		riWidth = best.m_iWidth;
		riHeight = best.m_iHeight;
	}

	UInt8* m_pData{};
	UInt32 m_uWidth{};
	UInt32 m_uHeight{};
};

} // namespace anonymous

class TextureCookTaskTest SEOUL_SEALED
{
public:
	void TestOcclusion()
	{
		UnitTestsFileManagerHelper helper;

		static Byte const* kasFiles[] =
		{
			"Test001.png",
			"Test002.png",
			"Test003.png",
			"Test004.png",
			"Test005.png",
			"Test006.png",
			"Test007.png",
			"Test008.png",
		};

		for (auto const& sFileName : kasFiles)
		{
			Cooking::Image img;
			Cooking::ImageAlphaType::Enum eAlphaType = Cooking::ImageAlphaType::kUnknown;
			SEOUL_UNITTESTING_ASSERT(img.Load(
				FilePath::CreateConfigFilePath(String::Printf(R"(UnitTests\Images\%s)", sFileName)),
				eAlphaType));
			Int32 const iWidth = img.GetWidth();
			Int32 const iHeight = img.GetHeight();
			if (Cooking::ImageAlphaType::kRgbNoAlpha != eAlphaType)
			{
				img.PremultiplyAlpha();
			}

			Cooking::Image::OcclusionRectangles vExpected;
			Int32 iExpectedX = -1;
			Int32 iExpectedY = -1;
			Int32 iExpectedWidth = -1;
			Int32 iExpectedHeight = -1;
			{
				OldImage old;
				old.m_pData = img.GetData();
				old.m_uHeight = (UInt32)iHeight;
				old.m_uWidth = (UInt32)iWidth;
				old.GetOcclusionRegion(iExpectedX, iExpectedY, iExpectedWidth, iExpectedHeight, &vExpected);
			}

			Cooking::Image::OcclusionRectangles vTest;
			Int32 iTestX = -1;
			Int32 iTestY = -1;
			Int32 iTestWidth = -1;
			Int32 iTestHeight = -1;
			img.GetOcclusionRegion(iTestX, iTestY, iTestWidth, iTestHeight, &vTest);

			SEOUL_LOG_COOKING("Testing %s: (%d, %d, %d, %d)",
				sFileName,
				vExpected[0].m_iX,
				vExpected[0].m_iY,
				vExpected[0].m_iWidth,
				vExpected[0].m_iHeight);
			SEOUL_UNITTESTING_ASSERT_EQUAL(vExpected.GetSize(), vTest.GetSize());
			for (auto i = vExpected.Begin(), iEnd = vExpected.End(), i2 = vTest.Begin(); iEnd != i; ++i, ++i2)
			{
				SEOUL_UNITTESTING_ASSERT_EQUAL(i->m_iX, i2->m_iX);
				SEOUL_UNITTESTING_ASSERT_EQUAL(i->m_iY, i2->m_iY);
				SEOUL_UNITTESTING_ASSERT_EQUAL(i->m_iWidth, i2->m_iWidth);
				SEOUL_UNITTESTING_ASSERT_EQUAL(i->m_iHeight, i2->m_iHeight);
			}
			SEOUL_UNITTESTING_ASSERT_EQUAL(iExpectedX, iTestX);
			SEOUL_UNITTESTING_ASSERT_EQUAL(iExpectedY, iTestY);
			SEOUL_UNITTESTING_ASSERT_EQUAL(iExpectedWidth, iTestWidth);
			SEOUL_UNITTESTING_ASSERT_EQUAL(iExpectedHeight, iTestHeight);
		}
	}
};
#endif // /SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(TextureCookTaskTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestOcclusion)
SEOUL_END_TYPE()

} // namespace Seoul
