/**
 * \file NullGraphicsTexture.h
 * \brief Nop implementation of a Texture for
 * contexts without graphics hardware.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef NULL_GRAPHICS_TEXTURE_H
#define NULL_GRAPHICS_TEXTURE_H

#include "Texture.h"
#include "UnsafeHandle.h"

namespace Seoul
{

class NullGraphicsTexture SEOUL_SEALED : public BaseTexture
{
public:
	virtual Bool GetMemoryUsageInBytes(UInt32& rzMemoryUsageInBytes) const SEOUL_OVERRIDE
	{
		rzMemoryUsageInBytes = GetDataSizeForPixelFormat(m_iWidth, m_iHeight, m_eFormat);
		if (m_bSecondary) { rzMemoryUsageInBytes *= 2; }
		return true;
	}

	virtual UnsafeHandle GetTextureHandle() const SEOUL_OVERRIDE
	{
		return UnsafeHandle((NullGraphicsTexture*)this);
	}

	virtual Bool OnCreate() SEOUL_OVERRIDE;
	virtual void OnReset() SEOUL_OVERRIDE;
	virtual void OnLost() SEOUL_OVERRIDE;

private:
	Bool const m_bSecondary;

	NullGraphicsTexture(
		UInt uWidth,
		UInt uHeight,
		PixelFormat eFormat,
		Bool bSecondary);
	~NullGraphicsTexture();

	static Bool InternalCheckTexture(
		FilePath filePath,
		void* pRawTextureFileData,
		UInt32 zFileSizeInBytes,
		UInt32& rzEndingOffsetInBytes);

	SEOUL_REFERENCE_COUNTED_SUBCLASS(NullGraphicsTexture);

	friend class NullGraphicsDevice;
	friend class NullGraphicsRenderCommandStreamBuilder;
}; // class NullGraphicsTexture

} // namespace Seoul

#endif // include guard
