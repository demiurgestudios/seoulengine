/**
 * \file UITexture.h
 * \brief Implementation of Falcon::Texture for the UI project.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef UI_TEXTURE_H
#define UI_TEXTURE_H

#include "FalconTexture.h"
#include "Prereqs.h"
#include "Texture.h"
namespace Seoul { namespace Falcon { class TexturePacker; } }

namespace Seoul::UI
{

class Texture : public Falcon::Texture
{
public:
	static const Int32 kStride = 4;

	Texture();
	Texture(FilePath filePath);
	Texture(void const* pData, UInt32 uWidth, UInt32 uHeight, UInt32 uStride, Bool bIsFullOccluder);
	virtual ~Texture();

	// Falcon::Texture overrides
	virtual const TextureContentHandle& GetTextureContentHandle();
	virtual Bool HasDimensions() const SEOUL_OVERRIDE;
	virtual Bool IsAtlas() const SEOUL_OVERRIDE;
	virtual Bool IsLoading() const SEOUL_OVERRIDE;
	virtual Bool ResolveLoadingData(
		const FilePath& filePath,
		Falcon::TextureLoadingData& rData) const SEOUL_OVERRIDE;
	virtual Bool ResolveTextureMetrics(Falcon::TextureMetrics& r) const SEOUL_OVERRIDE;
	// /Falcon::Texture overrides

protected:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(Texture);

	TextureContentHandle m_hTexture;
	FilePath m_FilePath;

private:
	virtual bool DoResolveMemoryUsageInBytes(Int32& riMemoryUsageInBytes) const SEOUL_OVERRIDE;

	SEOUL_DISABLE_COPY(Texture);
}; // class UI::Texture

/** A UI::Texture specialization that defines sub regions. */
class AtlasTexture SEOUL_SEALED : public Texture
{
public:
	AtlasTexture(const Falcon::TexturePacker& packer);
	~AtlasTexture();

	const SharedPtr<DepthStencilSurface>& GetDepth() const;
	const SharedPtr<RenderTarget>& GetTarget() const;
	virtual Bool IsAtlas() const SEOUL_OVERRIDE;

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(AtlasTexture);

	SharedPtr<RenderTarget> m_pTarget;
	SharedPtr<DepthStencilSurface> m_pDepth;

	SEOUL_DISABLE_COPY(AtlasTexture);
}; // class UI::AtlasTexture

} // namespace Seoul::UI

#endif // include guard
