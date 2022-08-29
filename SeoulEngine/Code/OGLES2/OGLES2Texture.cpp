/**
 * \file OGLES2Texture.cpp
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

#include "Logger.h"
#include "OGLES2RenderDevice.h"
#include "OGLES2Texture.h"
#include "ScopedAction.h"
#include "TextureManager.h"

namespace Seoul
{

/**
 * Utility function, sets up default sampling and wrap parameters for
 * an OpenGl ES2 texture.
 */
static void InternalStaticSetupDefaultTextureParameters(
	UInt32 uWidth,
	UInt32 uHeight,
	const TextureConfig& config,
	GLenum eTextureType,
	Int32 iLevels)
{
	// Compute whether we have a complete mip chain or not.
	Int32 iRequiredLevels = 0;
	do
	{
		++iRequiredLevels;
		uWidth >>= 1u;
		uHeight >>= 1u;
	} while (uWidth > 1u || uHeight > 1u);

	// Address mode - clamp-to-edge by default, unless the
	// texture has the texture config explicitly overriden in TextureManager.
	SEOUL_OGLES2_VERIFY(glTexParameteri(
		eTextureType,
		GL_TEXTURE_WRAP_S,
		(config.m_bWrapAddressU ? GL_REPEAT : GL_CLAMP_TO_EDGE)));
	SEOUL_OGLES2_VERIFY(glTexParameteri(
		eTextureType,
		GL_TEXTURE_WRAP_T,
		(config.m_bWrapAddressV ? GL_REPEAT : GL_CLAMP_TO_EDGE)));

	SEOUL_OGLES2_VERIFY(glTexParameteri(eTextureType, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

	// Can only support mipmaps if we have a complete mip chain,
	// or the device supports incomplete mip chains.
	if (iLevels > 1 &&
		(iLevels == iRequiredLevels || GetOGLES2RenderDevice().GetCaps().m_bIncompleteMipChain))
	{
		SEOUL_OGLES2_VERIFY(glTexParameteri(eTextureType, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST));
		SEOUL_OGLES2_VERIFY(glTexParameteri(eTextureType, GL_TEXTURE_MAX_LEVEL, (iLevels - 1)));
	}
	else
	{
		SEOUL_OGLES2_VERIFY(glTexParameteri(eTextureType, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	}
}

OGLES2Texture::OGLES2Texture(
	const TextureConfig& config,
	const TextureData& data,
	UInt uWidth,
	UInt uHeight,
	PixelFormat eFormat,
	UInt32 zGraphicsMemoryUsageInBytes,
	Bool bDynamic,
	Bool bCreateImmediate)
	: BaseTexture()
	, m_Config(config)
	, m_Texture(0)
	, m_zGraphicsMemoryUsageInBytes(zGraphicsMemoryUsageInBytes)
	, m_Data(data)
	, m_bDynamic(bDynamic)
	, m_TextureSecondary(0)
{
	// Cannot have initial data for a dynamic buffer.
	SEOUL_ASSERT(!m_bDynamic || !m_Data.HasLevels());

	m_iWidth = uWidth;
	m_iHeight = uHeight;
	m_eFormat = eFormat;

	if (bCreateImmediate)
	{
		(void)InternalCreateTexture();
	}
}

OGLES2Texture::~OGLES2Texture()
{
	SEOUL_ASSERT(IsRenderThread());

	if (0u != m_TextureSecondary)
	{
		SEOUL_OGLES2_VERIFY(glDeleteTextures(1, &m_TextureSecondary));
		m_TextureSecondary = 0u;
	}

	SEOUL_OGLES2_VERIFY(glDeleteTextures(1, &m_Texture));
	m_Texture = 0u;

	InternalFreeData();
}

Bool OGLES2Texture::OnCreate()
{
	SEOUL_ASSERT(IsRenderThread());

	// Early out if we have a valid texture instance already - this
	// would have occured due to an asynchronous immediate create.
	if (0 != m_Texture)
	{
		SEOUL_VERIFY(BaseTexture::OnCreate());
		return true;
	}

	// Otherwise, perform the creation.
	if (InternalCreateTexture())
	{
		SEOUL_VERIFY(BaseTexture::OnCreate());
		return true;
	}

	return false;
}

Bool OGLES2Texture::InternalCreateTexture()
{
	auto const deferred(MakeDeferredAction([&]()
	{
		// Only necessary if we're running on the render thread - if
		// this is an async create, no need to interact with state manager.
		if (IsRenderThread())
		{
			// Make sure the state manager's view of things is in sync once we're done.
			GetOGLES2RenderDevice().GetStateManager().RestoreActiveTextureIfSet(GL_TEXTURE_2D);
		}
	}));

	m_Texture = InternalCreate(false);
	if (0u == m_Texture)
	{
		return false;
	}

	if (m_Data.HasSecondary())
	{
		m_TextureSecondary = InternalCreate(true);
		if (0u == m_TextureSecondary)
		{
			SEOUL_OGLES2_VERIFY(glDeleteTextures(1, &m_Texture));
			m_Texture = 0u;
			return false;
		}
	}

	// Done - free data and return.
	InternalFreeData();
	return true;
}

/**
 * Internal details to create a texture object.
 */
GLuint OGLES2Texture::InternalCreate(Bool bSecondary) const
{
	GLuint texture = 0u;
	SEOUL_OGLES2_VERIFY(glGenTextures(1u, &texture));
	if (0u == texture)
	{
		return 0u;
	}

	SEOUL_OGLES2_VERIFY(glBindTexture(GL_TEXTURE_2D, texture));
	SEOUL_OGLES2_VERIFY(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));

	// Apply default sampling and wrap values to the texture.
	auto const uLevels = Max(m_Data.GetSize(), 1u);
	InternalStaticSetupDefaultTextureParameters(m_iWidth, m_iHeight, m_Config, GL_TEXTURE_2D, (Int32)uLevels);
	GLenum eOpenGlFormat = PixelFormatToOpenGlFormat(m_eFormat);
	GLenum eInternalOpenGlFormat = GL_INVALID_ENUM;
	GLenum eOpenGlType = GL_INVALID_ENUM;

	// Only calculate the internal and type enums if the
	// format is not compressed, these are invalids values for compressed
	// pixel formats.
	if (!IsCompressedPixelFormat(m_eFormat))
	{
		eInternalOpenGlFormat = PixelFormatToOpenGlInternalFormat(m_eFormat);
		eOpenGlType = PixelFormatToOpenGlElementType(m_eFormat);
	}

	// Base mip level.
	{
		auto pBase = (m_Data.HasLevels() ? m_Data.GetLevel(0u)->GetTextureData(bSecondary) : nullptr);
		if (IsCompressedPixelFormat(m_eFormat))
		{
			UInt32 const zDataSizeInBytes = GetDataSizeForPixelFormat(
				m_iWidth,
				m_iHeight,
				m_eFormat);

			SEOUL_OGLES2_VERIFY(GetOGLES2RenderDevice().CompressedTexImage2D(
				GL_TEXTURE_2D,
				0,
				eOpenGlFormat,
				m_iWidth,
				m_iHeight,
				GL_FALSE,
				zDataSizeInBytes,
				pBase));
		}
		else
		{
			SEOUL_OGLES2_VERIFY(GetOGLES2RenderDevice().TexImage2D(
				GL_TEXTURE_2D,
				0,
				eInternalOpenGlFormat,
				m_iWidth,
				m_iHeight,
				GL_FALSE,
				eOpenGlFormat,
				eOpenGlType,
				pBase));
		}
	}

	for (UInt32 i = 1u; i < uLevels; ++i)
	{
		Int32 iMipWidth = m_iWidth;
		Int32 iMipHeight = m_iHeight;
		AdjustWidthAndHeightForTextureLevel(i, iMipWidth, iMipHeight);

		auto pData = m_Data.GetLevel(i)->GetTextureData(bSecondary);
		if (IsCompressedPixelFormat(m_eFormat))
		{
			UInt32 const zDataSizeInBytes = GetDataSizeForPixelFormat(
				iMipWidth,
				iMipHeight,
				m_eFormat);

			SEOUL_OGLES2_VERIFY(GetOGLES2RenderDevice().CompressedTexImage2D(
				GL_TEXTURE_2D,
				i,
				eOpenGlFormat,
				iMipWidth,
				iMipHeight,
				GL_FALSE,
				zDataSizeInBytes,
				pData));
		}
		else
		{
			SEOUL_OGLES2_VERIFY(GetOGLES2RenderDevice().TexImage2D(
				GL_TEXTURE_2D,
				i,
				eInternalOpenGlFormat,
				iMipWidth,
				iMipHeight,
				GL_FALSE,
				eOpenGlFormat,
				eOpenGlType,
				pData));
		}
	}

	SEOUL_OGLES2_VERIFY(glPixelStorei(GL_UNPACK_ALIGNMENT, 4));
	SEOUL_OGLES2_VERIFY(glBindTexture(GL_TEXTURE_2D, 0));

	return texture;
}

/**
 * If still valid, releases any buffers specified on creation to
 * generate a texture.
 */
void OGLES2Texture::InternalFreeData()
{
	m_Data = TextureData();
}

} // namespace Seoul
