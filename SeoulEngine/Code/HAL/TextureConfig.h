/**
 * \file TextureConfig.h
 * \brief Additional config used (on some platforms) to set up
 * a texture object. For file textures, this is stored globally
 * in TextureManager.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef TEXTURE_CONFIG_H
#define TEXTURE_CONFIG_H

#include "Prereqs.h"

namespace Seoul
{

/** Defines texture config overrides, applied to a texture on GPU create. */
struct TextureConfig SEOUL_SEALED
{
	TextureConfig()
		: m_bWrapAddressU(false)
		, m_bWrapAddressV(false)
		, m_bMipped(false)
	{
	}

	/** By default, textures use clamped sampling. This enables wrapped sampling for the texture. */
	Bool m_bWrapAddressU;
	Bool m_bWrapAddressV;

	/**
	 * When true, a full mip chain is loaded into the texture instance progressively. e.g. If
	 * sif0 is requested with m_bMipped enabled, sif4...sif1 will also be loaded into the
	 * texture instance associated with sif0 as mips.
	 */
	Bool m_bMipped;
}; // struct TextureConfig

} // namespace Seoul

#endif // include guard
