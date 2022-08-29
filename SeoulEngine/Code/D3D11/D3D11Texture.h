/**
 * \file D3D11Texture.h
 * \brief D3D11 implementation of texture classes.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef D3D11_TEXTURE_H
#define D3D11_TEXTURE_H

#include "DDS.h"
#include "FilePath.h"
#include "Texture.h"
#include "UnsafeHandle.h"
struct ID3D11Resource;
struct ID3D11ShaderResourceView;

namespace Seoul
{

/**
 * D3D11Texture encapsulates a texture created
 * by code. Unlike a persistent texture, it cannot be reloaded
 * from disk, but it can be locked for writing using Lock() and Unock().
 */
class D3D11Texture SEOUL_SEALED : public BaseTexture
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
		return UnsafeHandle((ID3D11ShaderResourceView*)m_pView.Get());
	}

	/**
	 * On some platforms, a secondary texture is used internally to
	 * store additional data, such as the alpha channel.
	 */
	virtual UnsafeHandle GetSecondaryTextureHandle() const SEOUL_OVERRIDE
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
	virtual Bool NeedsSecondaryTexture() const SEOUL_OVERRIDE
	{
		return false;
	}

	virtual Bool OnCreate() SEOUL_OVERRIDE;

private:
	D3D11Texture(
		const TextureData& data,
		UInt uWidth,
		UInt uHeight,
		PixelFormat eFormat,
		UInt32 zGraphicsMemoryUsageInBytes,
		Bool bDynamic,
		Bool bCreateImmediate);
	virtual ~D3D11Texture();

	SEOUL_REFERENCE_COUNTED_SUBCLASS(D3D11Texture);

	friend class D3D11Device;
	friend class D3D11RenderCommandStreamBuilder;

	Bool InternalCreateTexture();
	CheckedPtr<ID3D11Resource> m_pTexture;
	CheckedPtr<ID3D11ShaderResourceView> m_pView;
	UInt32 m_zGraphicsMemoryUsageInBytes;
	TextureData m_Data;
	Bool m_bDynamic;

	void InternalFreeData();
}; // class D3D11Texture

} // namespace Seoul

#endif // include guard
