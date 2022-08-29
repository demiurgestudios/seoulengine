
/**
 * \file OGLES2Texture.h
 * \brief Specialization of the base Texture class for various OGLES2 specific
 * texture types - particularly, volatile (created by code) textures
 * and persistent (created from files on disk) textures.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef OGLES2_TEXTURE_H
#define OGLES2_TEXTURE_H

#include "DDS.h"
#include "FilePath.h"
#include "OGLES2Util.h"
#include "Texture.h"
#include "TextureConfig.h"
#include "UnsafeHandle.h"

namespace Seoul
{

/**
 * OGLES2Texture encapsulates a texture object
 * in the OGLES2 API.
 */
class OGLES2Texture SEOUL_SEALED : public BaseTexture
{
public:
	virtual Bool GetMemoryUsageInBytes(UInt32& rzMemoryUsageInBytes) const SEOUL_OVERRIDE
	{
		rzMemoryUsageInBytes = m_zGraphicsMemoryUsageInBytes;
		return true;
	}

	/**
	 * Called by an Effect parameter to get the raw texture
	 * data, almost always used to set the data to the GPU.
	 */
	virtual UnsafeHandle GetTextureHandle() const SEOUL_OVERRIDE
	{
		return UnsafeHandle(m_Texture);
	}

	/**
	 * @return The secondary texture associated with this OGLES2Texture,
	 * or an invalid UnsafeHandle if no secondary texture is used.
	 */
	virtual UnsafeHandle GetSecondaryTextureHandle() const SEOUL_OVERRIDE
	{
		// If the current texture doesn't use a secondary texture explicitly,
		// use the global one-pixel white texture (255, 255, 255, 255).
		return (0u != m_TextureSecondary
			? UnsafeHandle(m_TextureSecondary)
			: UnsafeHandle(GetOGLES2RenderDevice().GetOnePixelWhiteTexture()));
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
	virtual Bool NeedsSecondaryTexture() const SEOUL_OVERRIDE
	{
		return (0u != m_TextureSecondary);
	}

	virtual Bool OnCreate() SEOUL_OVERRIDE;

private:
	OGLES2Texture(
		const TextureConfig& config,
		const TextureData& data,
		UInt uWidth,
		UInt uHeight,
		PixelFormat eFormat,
		UInt32 zGraphicsMemoryUsageInBytes,
		Bool bDynamic,
		Bool bCreateImmediate);
	virtual ~OGLES2Texture();
	SEOUL_REFERENCE_COUNTED_SUBCLASS(OGLES2Texture);

	friend class OGLES2RenderDevice;
	friend class OGLES2RenderCommandStreamBuilder;

	Bool InternalCreateTexture();
	GLuint InternalCreate(Bool bSecondary) const;

	TextureConfig const m_Config;
	GLuint m_Texture;
	UInt32 const m_zGraphicsMemoryUsageInBytes;
	TextureData m_Data;
	Bool m_bDynamic;
	GLuint m_TextureSecondary;

	void InternalFreeData();
}; // class OGLES2Texture

} // namespace Seoul

#endif // include guard
