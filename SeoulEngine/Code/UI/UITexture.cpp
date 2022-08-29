/**
 * \file UITexture.cpp
 * \brief Implementation of Falcon::Texture for the UI project.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "DepthStencilSurface.h"
#include "FalconTexturePacker.h"
#include "ReflectionDataStoreTableUtil.h"
#include "ReflectionUtil.h"
#include "RenderDevice.h"
#include "RenderTarget.h"
#include "TextureManager.h"
#include "UITexture.h"

#if SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_IOS
#include "FileManager.h"
#endif

namespace Seoul::UI
{

/** Apply pruning and post processing to the list of texture loading data. */
static void inline PostProcessLoadEntries(
	FilePath filePath,
	Falcon::TextureLoadingData::Entries& ra)
{
	// TODO: This shouldn't be platform specific here. Ideally, we'd know
	// ahead of a time, per texture, what the compression format is and whether
	// the texture is compressed.
	//
	// This value must be kept in sync with the body of NeedsCompression() in the SeoulTools codebase.
	static const Int32 kiMaxDimensionsForNoCompression = (128 * 128);

	// On iOS and Android, we need to use the next highest mip
	// level at thresholds which use compressed data, if
	// ETC1 is not a supported hardware format, to avoid using 4-8x
	// the memory on those devices.
	if (TextureManager::Get()->GetPlatformCompressionClass() == TextureCompressionClass::kETC1 &&
		!RenderDevice::Get()->GetCaps().m_bETC1)
	{
		for (auto i = ra.Begin(); ra.End() != i; ++i)
		{
			if (i->m_iDimensions > kiMaxDimensionsForNoCompression)
			{
				i->m_eType = (FileType)Min((Int32)FileType::LAST_TEXTURE_TYPE, (Int32)i->m_eType + 1);
			}
		}
	}

	// On Android and iOS, mip0 is sometimes exclude. Clamp the highest
	// resolution as needed based on whether the mip0 version exists or not.
#if SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_IOS
	{
		auto mip0(filePath);
		mip0.SetType(FileType::kTexture0);
		if (!FileManager::Get()->Exists(mip0)) // TODO: Limit to package systems so this is not unusually expensive.
		{
			for (auto& re : ra)
			{
				re.m_eType = Max(re.m_eType, FileType::kTexture1);
			}
		}
	}
#endif
}

Texture::Texture()
	: m_hTexture()
	, m_FilePath()
{
}

Texture::Texture(FilePath filePath)
	: m_hTexture(TextureManager::Get()->GetTexture(filePath))
	, m_FilePath(filePath)
{
}

Texture::Texture(
	void const* pData,
	UInt32 uWidth,
	UInt32 uHeight,
	UInt32 uStride,
	Bool bIsFullOccluder)
	: m_hTexture()
	, m_FilePath()
{
	auto const uSize = uWidth * uHeight * kStride;
	UInt8* pTextureData = (UInt8*)MemoryManager::Allocate(uSize, MemoryBudgets::Rendering);

	// Mirror the alpha channel in all 4 channels of the output.
	if (1u == uStride)
	{
		UInt32 uOut = 0u;

		UInt32 const iSourcePitch = uWidth;
		for (UInt32 i = 0; i < (uHeight * iSourcePitch); ++i)
		{
			UInt8 const uIn = ((UInt8*)pData)[i];
			pTextureData[uOut++] = uIn;
			pTextureData[uOut++] = uIn;
			pTextureData[uOut++] = uIn;
			pTextureData[uOut++] = uIn;
		}
	}
	else
	{
		// Only support strides of 1 and 4.
		SEOUL_ASSERT(4u == uStride);

		memcpy((void*)pTextureData, pData, uSize);

		UInt32 const iSourcePitch = (uWidth * kStride);
		for (UInt32 i = 0; i < (uHeight * iSourcePitch); i += 4)
		{
			Swap(pTextureData[i+0], pTextureData[i+2]);
		}
	}

	auto eFormat = PixelFormat::kA8R8G8B8;
	TextureData data = TextureData::CreateFromInMemoryBuffer(pTextureData, uSize, eFormat);
	TextureConfig config;
	auto pTexture(RenderDevice::Get()->CreateTexture(
		config,
		data,
		uWidth,
		uHeight,
		eFormat));
	m_hTexture = TextureContentHandle(pTexture.GetPtr());

	// This marks the texture as fully opaque, which means
	// it is a "perfect" occluder (no alpha bits).
	if (bIsFullOccluder && pTexture.IsValid())
	{
		pTexture->SetIsFullOccluder();
	}
}

Texture::~Texture()
{
}

const TextureContentHandle& Texture::GetTextureContentHandle()
{
	return m_hTexture;
}

Bool Texture::HasDimensions() const
{
	return m_hTexture.IsPtrValid();
}

Bool Texture::IsAtlas() const
{
	SharedPtr<BaseTexture> pTexture(m_hTexture.GetPtr());
	if (pTexture.IsValid())
	{
		return (pTexture->GetTexcoordsScale() != Vector2D::One());
	}
	else
	{
		return false;
	}
}

Bool Texture::IsLoading() const
{
	// Indirect textures are loaded once they returned a texture handle
	// that isn't the placeholder.
	if (m_hTexture.IsIndirect())
	{
		return (m_hTexture.GetPtr() == TextureManager::Get()->GetPlaceholderTexture());
	}
	// Direct textures, wait to leave the kDestroyed state.
	else
	{
		auto pTexture(m_hTexture.GetPtr());
		if (!pTexture.IsValid())
		{
			return false;
		}

		return pTexture->GetState() == BaseGraphicsObject::kDestroyed;
	}
}

Bool Texture::ResolveLoadingData(
	const FilePath& filePath,
	Falcon::TextureLoadingData& rData) const
{
	// Can't resolve while the texture is still actively loading.
	if (IsLoading())
	{
		return false;
	}

	// Can't resolve until the texture is fully ready.
	SharedPtr<BaseTexture> pTexture(m_hTexture.GetPtr());
	if (!pTexture.IsValid())
	{
		return false;
	}

	// First compute mip4 resolution - need to rescale it based on what mip level we actually are.
	Float fMip4Threshold = Max(
		(Float)pTexture->GetWidth() * pTexture->GetTexcoordsScale().X,
		(Float)pTexture->GetHeight() * pTexture->GetTexcoordsScale().Y);
	Int iMip4Dimensions = pTexture->GetWidth() * pTexture->GetHeight();

	switch (m_FilePath.GetType())
	{
	case FileType::kTexture0: fMip4Threshold /= 16.0f; iMip4Dimensions /= 256; break;
	case FileType::kTexture1: fMip4Threshold /= 8.0f; iMip4Dimensions /= 64; break;
	case FileType::kTexture2: fMip4Threshold /= 4.0f; iMip4Dimensions /= 16; break;
	case FileType::kTexture3: fMip4Threshold /= 2.0f; iMip4Dimensions /= 4; break;
	case FileType::kTexture4: break;
	default:
		// Unknown or in memory texture, no loading data available.
		return false;
	};

	// Now populate the levels.
	{
		auto& r = rData.m_aEntries.Front();
		r.m_eType = FileType::LAST_TEXTURE_TYPE;
		r.m_fThreshold = fMip4Threshold;
		r.m_iDimensions = iMip4Dimensions;
	}
	for (UInt32 i = 1u; i < rData.m_aEntries.GetSize(); ++i)
	{
		auto& r = rData.m_aEntries[i];
		auto const& prev = rData.m_aEntries[i-1];
		r.m_eType = (FileType)((Int32)prev.m_eType - 1);
		r.m_fThreshold = (prev.m_fThreshold * 2.0f);
		r.m_iDimensions = (prev.m_iDimensions * 4);
	}

	// Final step - applies some additional processing based on platform.
	PostProcessLoadEntries(m_FilePath, rData.m_aEntries);

	// Make sure we always have some usable level - set the last to max float.
	rData.m_aEntries.Back().m_fThreshold = FloatMax;

	return true;
}

Bool Texture::ResolveTextureMetrics(Falcon::TextureMetrics& r) const
{
	// Can't resolve while the texture is still actively loading.
	if (IsLoading())
	{
		return false;
	}

	// Can't resolve until the texture is fully ready.
	SharedPtr<BaseTexture> pTexture(m_hTexture.GetPtr());
	if (!pTexture.IsValid())
	{
		return false;
	}

	r.m_iHeight = pTexture->GetHeight();
	r.m_iWidth = pTexture->GetWidth();
	r.m_vAtlasOffset = Vector2D::Zero();
	r.m_vAtlasScale = pTexture->GetTexcoordsScale();
	r.m_vOcclusionOffset = pTexture->GetOcclusionRegionScaleAndOffset().GetZW();
	r.m_vOcclusionScale = pTexture->GetOcclusionRegionScaleAndOffset().GetXY();
	r.m_vVisibleOffset = pTexture->GetVisibleRegionScaleAndOffset().GetZW();
	r.m_vVisibleScale = pTexture->GetVisibleRegionScaleAndOffset().GetXY();
	return true;
}

Bool Texture::DoResolveMemoryUsageInBytes(Int32& riMemoryUsageInBytes) const
{
	if (!m_hTexture.IsLoading())
	{
		SharedPtr<BaseTexture> pTexture(m_hTexture.GetPtr());
		if (pTexture.IsValid())
		{
			UInt32 zMemoryUsageInBytes = 0;
			if (pTexture->GetMemoryUsageInBytes(zMemoryUsageInBytes))
			{
				riMemoryUsageInBytes = (Int32)zMemoryUsageInBytes;
				return true;
			}
		}
	}

	return false;
}

AtlasTexture::AtlasTexture(const Falcon::TexturePacker& packer)
	: Texture()
{
	// TODO: Add the runtime API and stop using the
	// DataStore API here.
	static const HString ksFormat("Format");
	static const HString ksHeight("Height");
	static const HString ksSameFormatAsBackBuffer("SameFormatAsBackBuffer");
	static const HString ksWidth("Width");

	DataStore dataStore;
	dataStore.MakeTable();
	dataStore.SetStringToTable(
		dataStore.GetRootNode(),
		ksFormat,
		EnumToString<PixelFormat>(RenderDevice::Get()->GetCompatible32bit4colorRenderTargetFormat()));
	dataStore.SetInt32ValueToTable(dataStore.GetRootNode(), ksHeight, packer.GetHeight());
	dataStore.SetInt32ValueToTable(dataStore.GetRootNode(), ksWidth, packer.GetWidth());

	// TODO: Eliminate the need for a depth-stencil surface bound during
	// packing, when possible. Most platforms/APIs require it though. :/

	DataStoreTableUtil util(dataStore, dataStore.GetRootNode(), HString());
	m_pTarget = RenderDevice::Get()->CreateRenderTarget(util);
	dataStore.EraseValueFromTable(dataStore.GetRootNode(), ksFormat);
	dataStore.SetBooleanValueToTable(dataStore.GetRootNode(), ksSameFormatAsBackBuffer, true);
	m_pDepth = RenderDevice::Get()->CreateDepthStencilSurface(util);
	m_hTexture = TextureContentHandle(m_pTarget.GetPtr());
}

AtlasTexture::~AtlasTexture()
{
}

const SharedPtr<DepthStencilSurface>& AtlasTexture::GetDepth() const
{
	return m_pDepth;
}

const SharedPtr<RenderTarget>& AtlasTexture::GetTarget() const
{
	return m_pTarget;
}

Bool AtlasTexture::IsAtlas() const
{
	return true;
}

} // namespace Seoul::UI
