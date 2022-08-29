/**
 * \file TextureManager.cpp
 * \brief TextureManager is the singleton manager for persistent textures
 * that must be loaded from disk. For volatile textures that are
 * created procedurally, use RenderDevice::CreateTexture() to instantiate
 * textures directly.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "JobsFunction.h"
#include "JobsManager.h"
#include "RenderDevice.h"
#include "TextureManager.h"

namespace Seoul
{

TextureManager::TextureManager()
	: m_Content()
	, m_pErrorTexture()
	, m_pPlaceholderTexture()
{
	SEOUL_ASSERT_DEBUG(IsMainThread());

	InternalCreateBuiltinTextures();
}

TextureManager::~TextureManager()
{
	SEOUL_ASSERT_DEBUG(IsMainThread());
}

/**
 * Initializes the error texture, which is used as the replacement
 * for textures that cannot be loaded or have not been loaded yet.
 */
void TextureManager::InternalCreateBuiltinTextures()
{
	SEOUL_ASSERT(IsMainThread());

	// Create the error texture.
	{
		auto const uSize = 2u * 2u * sizeof(ColorARGBu8);
		ColorARGBu8* pData = (ColorARGBu8*)MemoryManager::AllocateAligned(
			uSize,
			MemoryBudgets::Rendering,
			SEOUL_ALIGN_OF(ColorARGBu8));

		pData[0] = ColorARGBu8::Magenta();
		pData[1] = ColorARGBu8::Black();
		pData[2] = ColorARGBu8::Black();
		pData[3] = ColorARGBu8::Magenta();

		auto eFormat = PixelFormat::kA8R8G8B8;
		TextureData data = TextureData::CreateFromInMemoryBuffer((void const*)pData, uSize, eFormat);
		TextureConfig config;
		m_pErrorTexture = RenderDevice::Get()->CreateTexture(
			config,
			data,
			2u,
			2u,
			eFormat);

		// This marks the texture as fully opaque, which means
		// it is a "perfect" occluder (no alpha bits).
		m_pErrorTexture->SetIsFullOccluder();
	}

	// Create the placeholder texture.
	{
		// Placeholder texture is filled with transparent black pixels.
		static const ColorARGBu8 kPlaceholderColor(ColorARGBu8::Create(0u, 0u, 0u, 0u));

		auto const uSize = 1u * 1u * sizeof(ColorARGBu8);
		ColorARGBu8* pData = (ColorARGBu8*)MemoryManager::AllocateAligned(
			uSize,
			MemoryBudgets::Rendering,
			SEOUL_ALIGN_OF(ColorARGBu8));

		pData[0] = kPlaceholderColor;

		auto eFormat = PixelFormat::kA8R8G8B8;
		TextureData data = TextureData::CreateFromInMemoryBuffer((void const*)pData, uSize, eFormat);
		TextureConfig config;
		m_pPlaceholderTexture = RenderDevice::Get()->CreateTexture(
			config,
			data,
			1u,
			1u,
			eFormat);
	}
}

struct TextureMemoryUsageCompute
{
	SEOUL_DELEGATE_TARGET(TextureMemoryUsageCompute);

	TextureMemoryUsageCompute()
		: m_pErrorTexture()
		, m_pPlaceholderTexture()
		, m_zTotalInBytes(0u)
		, m_bOneResult(false)
		, m_bAllResults(true)
	{
	}

	Bool Apply(const TextureContentHandle& h)
	{
		SharedPtr<BaseTexture> p(h.GetPtr());
		if (p.IsValid() &&
			p != m_pErrorTexture &&
			p != m_pPlaceholderTexture)
		{
			// If this succeeds, at the memory usage to the total
			// and set the flag that we have at least one
			// valid texture's memory usage.
			UInt32 zUsage = 0u;
			if (p->GetMemoryUsageInBytes(zUsage))
			{
				m_bOneResult = true;
				m_zTotalInBytes += zUsage;
			}
			// Otherwise, set the flag that we don't have
			// *all* textures memory usage, only some.
			else
			{
				m_bAllResults = false;
			}
		}

		// Return false to indicate "not handled", tells the Content::Store to
		// keep walking entries.
		return false;
	}

	SharedPtr<BaseTexture> m_pErrorTexture;
	SharedPtr<BaseTexture> m_pPlaceholderTexture;
	UInt32 m_zTotalInBytes;
	Bool m_bOneResult;
	Bool m_bAllResults;
};

/**
 * @return kNoMemoryUsageAvailable if memory usage is not available
 * for textures, kApproximateMemoryUsage is memory usage does not
 * necessarily reflect all textures, or kExactMemoryUsage if
 * memory usage equals the exact number of bytes that is being
 * used by texture data.
 *
 * If this method returns kApproximateMemoryUsage or
 * kExactMemoryUsage, rzMemoryUsageInBytes will contain the
 * associated value. Otherwise, rzMemoryUsageInBytes is
 * left unchanged.
 */
TextureManager::Result TextureManager::GetTextureMemoryUsageInBytes(UInt32& rzMemoryUsageInBytes) const
{
	TextureMemoryUsageCompute compute;
	compute.m_pErrorTexture = m_pErrorTexture;
	compute.m_pPlaceholderTexture = m_pPlaceholderTexture;
	m_Content.Apply(SEOUL_BIND_DELEGATE(&TextureMemoryUsageCompute::Apply, &compute));

	// In either case, we have a valid or approximate memory usage
	if (compute.m_bOneResult || compute.m_bAllResults)
	{
		// Set the memory usage and report the correct results.
		rzMemoryUsageInBytes = compute.m_zTotalInBytes;
		return (compute.m_bAllResults)
			? kExactMemoryUsage
			: kApproximateMemoryUsage;
	}
	else
	{
		return kNoMemoryUsageAvailable;
	}
}

} // namespace Seoul
