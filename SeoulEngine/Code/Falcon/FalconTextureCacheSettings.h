/**
 * \file FalconTextureCacheSettings.h
 * \brief Configuration of the Falcon Texture Cache system.
 *
 * These settings are global for the Falcon Renderer system.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FALCON_TEXTURE_CACHE_SETTINGS_H
#define FALCON_TEXTURE_CACHE_SETTINGS_H

#include "Prereqs.h"

namespace Seoul::Falcon
{

struct TextureCacheSettings SEOUL_SEALED
{
	TextureCacheSettings()
		: m_iTexturePackerWidth(2048)
		, m_iTexturePackerHeight(1024)
		, m_iTexturePackerSubImageMaxDimensionSquare(128 * 128)
		, m_uTexturePackerPurgeThresholdInFrames(10)
		, m_uTextureMemorySoftPurgeThresholdInFrames(10)
		, m_iTextureMemorySoftPurgeThresholdInBytes(32 * 1024 * 1024)
		, m_iTextureMemoryHardPurgeThresholdInBytes(80 * 1024 * 1024)
	{
	}

	// Width of the global packer texture in pixels.
	Int32 m_iTexturePackerWidth;

	// Height of the global packer texture in pixels.
	Int32 m_iTexturePackerHeight;

	// Maximum dim * dim of a sub image that
	// will be allowed in the texture packer.
	Int32 m_iTexturePackerSubImageMaxDimensionSquare;

	// The number of frames that an image must not
	// be rendered before it can be purged from the
	// global texture packer.
	Int32 m_uTexturePackerPurgeThresholdInFrames;

	// When the soft threshold of total memory is reached, any textures
	// that have not been used for render for this many frames will
	// be purged.
	Int32 m_uTextureMemorySoftPurgeThresholdInFrames;

	// When this total texture memory usage size is reached,
	// textures that have not been used for render for
	// m_iTextureMemorySoftPurgeThresholdInFrames frames will be
	// purged. This value should be lower than
	// m_iTextureMemoryHardPurgeThresholdInBytes.
	Int32 m_iTextureMemorySoftPurgeThresholdInBytes;

	// When this total texture memory usage size is reached,
	// textures that have not been used for render for
	// 1 frame will be purged. This value should be greater than
	// m_iTextureMemorySoftPurgeThresholdInBytes.
	Int32 m_iTextureMemoryHardPurgeThresholdInBytes;
}; // struct TextureCacheSettings

} // namespace Seoul::Falcon

#endif // include guard
