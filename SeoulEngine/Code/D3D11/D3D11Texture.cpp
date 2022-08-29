/**
 * \file D3D11Texture.cpp
 * \brief Specialization of the base Texture class for various D3D11 specific
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
#include "D3D11Device.h"
#include "D3D11Texture.h"
#include "TextureManager.h"
#include "ThreadId.h"

namespace Seoul
{

D3D11Texture::D3D11Texture(
	const TextureData& data,
	UInt uWidth,
	UInt uHeight,
	PixelFormat eFormat,
	UInt32 zGraphicsMemoryUsageInBytes,
	Bool bDynamic,
	Bool bCreateImmediate)
	: BaseTexture()
	, m_pTexture(nullptr)
	, m_pView(nullptr)
	, m_zGraphicsMemoryUsageInBytes(zGraphicsMemoryUsageInBytes)
	, m_Data(data)
	, m_bDynamic(bDynamic)
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

D3D11Texture::~D3D11Texture()
{
	SEOUL_ASSERT(IsRenderThread());

	SafeRelease(m_pView);
	SafeRelease(m_pTexture);
	InternalFreeData();
}

Bool D3D11Texture::OnCreate()
{
	SEOUL_ASSERT(IsRenderThread());

	// Early out if we have a valid texture instance already - this
	// would have occured due to an asynchronous immediate create.
	if (nullptr != m_pTexture)
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

Bool D3D11Texture::InternalCreateTexture()
{
	// Total levels - always at least 1.
	auto const uLevels = Max(m_Data.GetSize(), 1u);

	ID3D11Texture2D* pTexture = nullptr;
	{
		D3D11_TEXTURE2D_DESC desc;
		memset(&desc, 0, sizeof(desc));
		desc.ArraySize = 1;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = (m_bDynamic ? D3D11_CPU_ACCESS_WRITE : 0u);
		desc.Format = PixelFormatToD3D(m_eFormat);
		desc.Height = GetHeight();
		desc.MipLevels = uLevels;
		desc.MiscFlags = 0;
		desc.SampleDesc.Count = 1;
		desc.Usage = (m_bDynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT);
		desc.Width = GetWidth();

		UInt32 zMipWidth = (UInt32)GetWidth();
		UInt32 zMipHeight = (UInt32)GetHeight();
		FixedArray<D3D11_SUBRESOURCE_DATA, 16> aInitialData;
		memset(aInitialData.Get(0u), 0, aInitialData.GetSizeInBytes());

		auto const uSize = m_Data.GetSize();
		for (UInt32 i = 0u; i < uSize; ++i)
		{
			auto const& pLevel = m_Data.GetLevel(i);
			aInitialData[i].pSysMem = pLevel->GetTextureData();
			if (IsCompressedPixelFormat(m_eFormat))
			{
				UInt32 const nBlocksWide = Max(1u, (zMipWidth / 4u));
				UInt32 const uPitch = (PixelFormat::kDXT1 == m_eFormat ? 8u : 16u) * nBlocksWide;
				aInitialData[i].SysMemPitch = uPitch;
			}
			else
			{
				UInt32 uPitch = 0u;
				SEOUL_VERIFY(GetPitchForPixelFormat(zMipWidth, m_eFormat, uPitch));
				aInitialData[i].SysMemPitch = (UINT)uPitch;
			}

			zMipWidth = Max(zMipWidth >> 1, 1u);
			zMipHeight = Max(zMipHeight >> 1, 1u);
		}

		auto pData = (m_Data.HasLevels() ? aInitialData.Data() : nullptr);
		if (S_OK != GetD3D11Device().GetD3DDevice()->CreateTexture2D(
			&desc,
			pData,
			&pTexture) || nullptr == pTexture)
		{
			return false;
		}
	}

	ID3D11ShaderResourceView* pView = nullptr;
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC desc;
		memset(&desc, 0, sizeof(desc));
		desc.Format = PixelFormatToD3D(m_eFormat);
		desc.Texture2D.MipLevels = (UINT)uLevels;
		desc.Texture2D.MostDetailedMip = 0u;
		desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;

		if (S_OK != GetD3D11Device().GetD3DDevice()->CreateShaderResourceView(
			pTexture,
			&desc,
			&pView) || nullptr == pView)
		{
			SafeRelease(pTexture);
			return false;
		}
	}

	// Done success, assign to our members.
	m_pTexture = pTexture;
	m_pView = pView;

	// Cleanup and return.
	InternalFreeData();

	return true;
}

/**
 * If still valid, releases any buffers specified on creation to
 * generate a texture.
 */
void D3D11Texture::InternalFreeData()
{
	m_Data = TextureData();
}

} // namespace Seoul
