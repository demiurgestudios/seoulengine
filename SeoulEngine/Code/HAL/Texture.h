/**
 * \file Texture.h
 * \brief Platform-independent representation of a graphics
 * texture resource.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef TEXTURE_H
#define TEXTURE_H

#include "BaseGraphicsObject.h"
#include "Color.h"
#include "ContentHandle.h"
#include "ContentTraits.h"
#include "Geometry.h"
#include "List.h"
#include "PixelFormat.h"
#include "Prereqs.h"
#include "SeoulHString.h"
#include "SeoulString.h"
#include "SeoulTypes.h"
#include "UnsafeHandle.h"
#include "Vector.h"
namespace Seoul { class BaseTexture; }

namespace Seoul
{

namespace Content
{

/**
 * Specialization of Content::Traits<> for BaseTexture, allows BaseTexture to be managed
 * as loadable content in the content system.
 */
template <>
struct Traits<BaseTexture>
{
	typedef FilePath KeyType;
	static const Bool CanSyncLoad = false;

	static SharedPtr<BaseTexture> GetPlaceholder(FilePath filePath);
	static Bool FileChange(FilePath filePath, const Handle<BaseTexture>& hEntry);
	static void Load(FilePath filePath, const Handle<BaseTexture>& hEntry);
	static Bool PrepareDelete(FilePath filePath, Entry<BaseTexture, KeyType>& entry);
	static void SyncLoad(FilePath filePath, const Handle<BaseTexture>& hEntry) {}
	static UInt32 GetMemoryUsage(const SharedPtr<BaseTexture>& p) { return 0u; }
}; // Content::Traits<BaseTexture>

} // namespace Content

class BaseTexture SEOUL_ABSTRACT : public BaseGraphicsObject
{
public:
	/**
	 * Given ruWidth and ruHeight of mip 0 of a texture, adjust these values to
	 * equal the width and height of mip uLevel of the texture.
	 */
	inline static void AdjustWidthAndHeightForTextureLevel(UInt32 uLevel, Int32& riWidth, Int32& riHeight)
	{
		for (UInt i = 0u; i < uLevel; ++i)
		{
			riHeight >>= 1;
			if (riHeight < 1)
			{
				riHeight = 1;
			}

			riWidth >>= 1;
			if (riWidth < 1)
			{
				riWidth = 1;
			}
		}
	}

	virtual ~BaseTexture()
	{
		// It is the responsibility of the subclass to un-reset itself
		// on destruction if the graphics object was ever created.
		SEOUL_ASSERT(kCreated == GetState() || kDestroyed == GetState());
	}

	/**
	 * Width of the texture, in pixels.
	 */
	Int32 GetWidth() const
	{
		return m_iWidth;
	}

	/**
	 * Height of the texture, in pixels.
	 */
	Int32 GetHeight() const
	{
		return m_iHeight;
	}

	/**
	 * @return Format of the texture.
	 */
	PixelFormat GetFormat() const
	{
		return m_eFormat;
	}

	/**
	 * Returns a platform independent wrapper around a platform
	 * specific handle which represents the GPU object that stores
	 * this Texture object's texture data.
	 */
	virtual UnsafeHandle GetTextureHandle() const = 0;

	/**
	 * On some platforms, a secondary texture is used internally to
	 * store additional data, such as the alpha channel.
	 */
	virtual UnsafeHandle GetSecondaryTextureHandle() const
	{
		return UnsafeHandle();
	}

	/**
	 * @return True if this Texture needs its SecondaryTexture
	 * to render correctly, false otherwise.
	 *
	 * Typically, GetSecondaryTextureHandle() will always return
	 * a valid value if GetTextureHandle() is valid, but for some textures,
	 * that secondary texture may be a "nop texture" (i.e. a solid
	 * white pixel).
	 */
	virtual Bool NeedsSecondaryTexture() const
	{
		return false;
	}

	/**
	 * @return True if memory usage info for this texture is available,
	 * false otherwise. If this method returns true, rzMemoryUsageInBytes
	 * will contain the amount of memory occupied by this texture.
	 *
	 * The memory usage does not include the size of header structures
	 * or the Texture object itself, only the texture data used by the GPU.
	 */
	virtual Bool GetMemoryUsageInBytes(UInt32& rzMemoryUsageInBytes) const
	{
		return false;
	}

	/**
	 * @return The texture coordinates scale factor associated with this texture - should
	 * be applied to any texture coordinates used to sample this texture.
	 */
	const Vector2D& GetTexcoordsScale() const
	{
		return m_vTexcoordsScale;
	}

	/**
	 * Update the texture coordinates scale factor associated with this texture.
	 */
	void SetTexcoordsScale(const Vector2D& vTexcoordsScale)
	{
		m_vTexcoordsScale = vTexcoordsScale;
	}

	/**
	 * @return Offset and scaling factors of the occlusion rectangle of this texture.
	 *
	 * These values can be used to shrink a quad that is being used to draw this texture so that
	 * it only draws the occlusion portion of the texture. To apply these values:
	 * - transform coordinates into a [0, 1] space.
	 * - apply (coord * scale + offset)
	 * - apply the inverse of the transform used to place the the coordinates into a [0, 1] space.
	 *
	 * This value should be applied in addition to GetTexcoordsScale() when adjusting texture coordinates.
	 */
	const Vector4D& GetOcclusionRegionScaleAndOffset() const
	{
		return m_vOcclusionRegionScaleAndOffset;
	}

	/**
	 * @return Offset and scaling factors of the visible rectangle of this texture.
	 *
	 * These values can be used to shrink a quad that is being used to draw this texture so that
	 * it only draws the visible portion of the texture. To apply these values:
	 * - transform coordinates into a [0, 1] space.
	 * - apply (coord * scale + offset)
	 * - apply the inverse of the transform used to place the the coordinates into a [0, 1] space.
	 *
	 * This value should be applied in addition to GetTexcoordsScale() when adjusting texture coordinates.
	 */
	const Vector4D& GetVisibleRegionScaleAndOffset() const
	{
		return m_vVisibleRegionScaleAndOffset;
	}

	/** Update the texture's OcclusionRegionScaleAndOffset, see GetOcclusionRegionScaleAndOffset(). */
	void SetOcclusionRegionScaleAndOffset(const Vector4D& vOcclusionRegionScaleAndOffset)
	{
		m_vOcclusionRegionScaleAndOffset = vOcclusionRegionScaleAndOffset;
	}

	/** Update the texture's VisibleRegionScaleAndOffset, see GetVisibleRegionScaleAndOffset(). */
	void SetVisibleRegionScaleAndOffset(const Vector4D& vVisibleRegionScaleAndOffset)
	{
		m_vVisibleRegionScaleAndOffset = vVisibleRegionScaleAndOffset;
	}

	/** Convenience for setting the occlusion area to the entire texture area. */
	void SetIsFullOccluder()
	{
		m_vOcclusionRegionScaleAndOffset = Vector4D(1.0f, 1.0f, 0.0f, 0.0f);
	}

protected:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(BaseTexture);

	BaseTexture()
		: m_vOcclusionRegionScaleAndOffset(0.0f, 0.0f, 0.0f, 0.0f)
		, m_vVisibleRegionScaleAndOffset(1.0f, 1.0f, 0.0f, 0.0f)
		, m_vTexcoordsScale(1.0f, 1.0f)
		, m_iWidth(0)
		, m_iHeight(0)
		, m_eFormat((PixelFormat)0)
	{
	}

	Vector4D m_vOcclusionRegionScaleAndOffset;
	Vector4D m_vVisibleRegionScaleAndOffset;
	Vector2D m_vTexcoordsScale;
	UInt32 m_iWidth;
	UInt32 m_iHeight;
	PixelFormat m_eFormat;
};
typedef Content::Handle<BaseTexture> TextureContentHandle;

/**
 * A single level of texture data in a TextureData object.
 */
class TextureLevelData SEOUL_SEALED
{
public:
	TextureLevelData(
		void const* pAllData,
		UInt32 uAllSizeInBytes,
		void const* pTextureData,
		void const* pTextureDataSecondary);
	~TextureLevelData();

	// Overall data pointers and sizes.
	void const* GetAllData() const { return m_pAllData; }
	UInt32 GetAllSizeInBytes() const { return m_uAllSizeInBytes; }

	// Individual data access. Primary and (optional) secondary
	// data.
	void const* GetTextureData(Bool bSecondary) const
	{
		return (bSecondary ? m_pTextureDataSecondary : m_pTextureData);
	}
	void const* GetTextureData() const { return m_pTextureData; }
	void const* GetTextureDataSecondary() const { return m_pTextureDataSecondary; }

private:
	SEOUL_DISABLE_COPY(TextureLevelData);
	SEOUL_REFERENCE_COUNTED(TextureLevelData);

	void const* const m_pAllData;
	UInt32 const m_uAllSizeInBytes;
	void const* const m_pTextureData;
	void const* const m_pTextureDataSecondary;
}; // class TextureLevelData

/**
 * Utility structure - contains reference counted texture levels.
 * Used to store raw texture data to shared from content loading
 * to submission to graphics API. Immutable (can only create
 * new versions as combinations of existing versions).
 */
class TextureData SEOUL_SEALED
{
public:
	// Call to append a level, must be smaller than all previous levels.
	static TextureData PushBackLevel(const TextureData& base, const SharedPtr<TextureLevelData>& pLevel);
	// Call to prepend a level, must be larger than all previous levels.
	static TextureData PushFrontLevel(const TextureData& base, const SharedPtr<TextureLevelData>& pLevel);

	// Convenience for flat buffers of data. Also may
	// adjust the format as needed based on the current
	// environment (e.g. on platforms that do not support
	// BGRA, data will be converted
	// to RGBA).
	static TextureData CreateFromInMemoryBuffer(
		void const* pData,
		UInt32 uDataSizeInBytes,
		PixelFormat& reFormat);

	TextureData();
	~TextureData();

	// Access to data details.

	/** Access to an individual level/slice of texture data. */
	const SharedPtr<TextureLevelData>& GetLevel(UInt32 i) const
	{
		return m_vLevels[i];
	}

	/** The number of levels in this data. */
	UInt32 GetSize() const { return m_vLevels.GetSize(); }

	/** True if this data has at least one texture level/slice. */
	Bool HasLevels() const { return !m_vLevels.IsEmpty(); }

	/** True if this data has secondary texture data at every level. */
	Bool HasSecondary() const { return m_bHasSecondary; }

private:
	typedef Vector< SharedPtr<TextureLevelData>, MemoryBudgets::Content> Levels;
	Levels m_vLevels;
	Bool m_bHasSecondary;
}; // class TextureData

} // namespace Seoul

#endif // include guard
