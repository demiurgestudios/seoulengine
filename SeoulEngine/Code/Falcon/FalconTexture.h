/**
 * \file FalconTexture.h
 * \brief Abstraction of GPU image data used
 * by Falcon. Platform and concrete coordinate
 * version of BitmapDefinition.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FALCON_TEXTURE_H
#define FALCON_TEXTURE_H

#include "FilePath.h"
#include "Prereqs.h"

namespace Seoul::Falcon
{

struct TextureLoadDataEntry SEOUL_SEALED
{
	TextureLoadDataEntry()
		: m_fThreshold(FloatMax)
		, m_eType(FileType::LAST_TEXTURE_TYPE)
		, m_iDimensions(0)
	{
	}

	/**
	 * Max dimensions - when the viewspace dimension is below this value, the corresponding
	 * resolution described by m_eType can be used.
	 */
	Float m_fThreshold;

	/** The corresponding file type to use. */
	FileType m_eType;

	/** Raw Width * Height of the texture in pixels. */
	Int32 m_iDimensions;
};

struct TextureLoadingData SEOUL_SEALED
{
	typedef FixedArray<TextureLoadDataEntry, (UInt32)FileType::LAST_TEXTURE_TYPE - (UInt32)FileType::FIRST_TEXTURE_TYPE + 1u> Entries;

	TextureLoadingData()
		: m_aEntries()
		, m_bNeedsRefresh(false)
	{
	}

	Entries m_aEntries;
	Bool m_bNeedsRefresh;
}; // struct TextureLoadingData

struct TextureMetrics SEOUL_SEALED
{
	TextureMetrics()
		: m_iWidth(0)
		, m_iHeight(0)
		, m_vOcclusionScale(Vector2D::Zero())
		, m_vOcclusionOffset(Vector2D::Zero())
		, m_vVisibleScale(Vector2D::One())
		, m_vVisibleOffset(Vector2D::Zero())
		, m_vAtlasScale(Vector2D::One())
		, m_vAtlasOffset(Vector2D::Zero())
	{
	}

	Int32 m_iWidth;
	Int32 m_iHeight;
	Vector2D m_vOcclusionScale;
	Vector2D m_vOcclusionOffset;
	Vector2D m_vVisibleScale;
	Vector2D m_vVisibleOffset;
	Vector2D m_vAtlasScale;
	Vector2D m_vAtlasOffset;
}; // struct TextureMetrics

class Texture SEOUL_ABSTRACT
{
public:
	virtual ~Texture()
	{
	}

	Int32 GetMemoryUsageInBytes() const
	{
		return m_iMemoryUsageInBytes;
	}

	virtual Bool HasDimensions() const = 0;

	virtual Bool IsAtlas() const = 0;

	virtual Bool IsLoading() const = 0;

	Bool ResolveMemoryUsageInBytes(Int32& riMemoryUsageInBytes)
	{
		if (DoResolveMemoryUsageInBytes(riMemoryUsageInBytes))
		{
			m_iMemoryUsageInBytes = riMemoryUsageInBytes;
			return true;
		}

		return false;
	}

	virtual Bool ResolveLoadingData(
		const FilePath& filePath,
		TextureLoadingData& rData) const = 0;

	virtual Bool ResolveTextureMetrics(
		TextureMetrics& rMetrics) const = 0;

protected:
	SEOUL_REFERENCE_COUNTED(Texture);
	Int32 m_iMemoryUsageInBytes;

	Texture()
		: m_iMemoryUsageInBytes(0)
	{
	}

	virtual Bool DoResolveMemoryUsageInBytes(Int32& riMemoryUsageInBytes) const = 0;

private:
	SEOUL_DISABLE_COPY(Texture);
}; // class Texture

struct TextureReference SEOUL_SEALED
{
	TextureReference()
		: m_pTexture()
		, m_vOcclusionOffset(Vector2D::Zero())
		, m_vOcclusionScale(Vector2D::Zero())
		, m_vVisibleOffset(Vector2D::Zero())
		, m_vVisibleScale(Vector2D::One())
		, m_vAtlasOffset(Vector2D::Zero())
		, m_vAtlasScale(Vector2D::One())
		, m_vAtlasMin(Vector2D::Zero())
		, m_vAtlasMax(Vector2D::One())
		// Developer only field to track the mip level of a reference.
#if SEOUL_ENABLE_CHEATS
		, m_eTextureType(FileType::kTexture0)
#endif
	{
	}

	SharedPtr<Texture> m_pTexture;

	/**
	 * Offset and scale of the sub-region that is completely opaque pixels
	 * within the image.
	 */
	Vector2D m_vOcclusionOffset;
	Vector2D m_vOcclusionScale;

	/**
	 * Offset and scale of the sub-region that is not completely transparent pixels
	 * within the image. This must be adjusted by m_vAtlasOffset and m_vAtlasScale to
	 * get the portion of the overall texture that is visible, vs. the atlas sub-region.
	 */
	Vector2D m_vVisibleOffset;
	Vector2D m_vVisibleScale;

	/**
	 * Offset and scale of the sub-region that corresponds to the actual texture. This
	 * is distinctly different from m_vVisibleOffset and m_vVisibleScale, as the mesh
	 * must be adjusted to apply those values, whereas these values *must* be applied for
	 * the texcoords in a mesh to be correct (the mesh expects [0, 1] to map to the entire
	 * texture, whereas if atlasing is involed, the portion for the mesh is described by
	 * these values and they must be applied to correct the default [0, 1] texcoords).
	 */
	Vector2D m_vAtlasOffset;
	Vector2D m_vAtlasScale;

	/**
	 * To account for floating point error, this is the min/max of this texture referenece
	 * into the bigger atlas. Used to clamp texture coordinates after final rescaling.
	 */
	Vector2D m_vAtlasMin;
	Vector2D m_vAtlasMax;

#if SEOUL_ENABLE_CHEATS
	/** Developer only field, used for texture resolution visualization. */
	FileType m_eTextureType;
#endif // /#if SEOUL_ENABLE_CHEATS
}; // struct TextureReference

} // namespace Seoul::Falcon

#endif // include guard
