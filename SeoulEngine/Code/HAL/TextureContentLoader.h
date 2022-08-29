/**
 * \file TextureContentLoader.h
 * \brief Specialization of Content::LoaderBase for loading textures.
 *
 * \warning Don't instantiate this class to load a texture
 * directly unless you know what you are doing. Loading textures
 * this way prevents the texture from being managed by ContentLoadManager.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef TEXTURE_CONTENT_LOADER_H
#define TEXTURE_CONTENT_LOADER_H

#include "ContentLoaderBase.h"
#include "ContentKey.h"
#include "Delegate.h"
#include "Texture.h"
#include "TextureConfig.h"
namespace Seoul { class BaseTexture; }

namespace Seoul
{

/**
 * Specialization of Content::LoaderBase for loading textures.
 */
class TextureContentLoader SEOUL_SEALED : public Content::LoaderBase
{
public:
	SEOUL_DELEGATE_TARGET(TextureContentLoader);

	TextureContentLoader(FilePath filePath, const TextureContentHandle& pEntry);
	virtual ~TextureContentLoader();

protected:
	virtual Content::LoadState InternalExecuteContentLoadOp() SEOUL_OVERRIDE;

private:
	Content::LoadState InternalExecuteContentLoadOpBody();
	void InternalCreateTextureUtil(Bool bAsyncCreate, SharedPtr<BaseTexture>* ppTexture) const;
	Bool InternalDecodeTexture();
	void InternalFreeCurrentLevelData();
	void InternalReleaseEntry();

	struct CurrentLevelData SEOUL_SEALED
	{
		CurrentLevelData();
		~CurrentLevelData();

		void Free();
		SharedPtr<TextureLevelData> ReleaseAsTextureLevelData();

		void* m_pFileData{};
		UInt32 m_uFileSizeInBytes{};
		void const* m_pTextureData{};
		void const* m_pTextureDataSecondary{};
	};

	FilePath m_CurrentLevelFilePath;
	TextureContentHandle m_hEntry;
	TextureConfig m_TextureConfig;
	UInt32 m_uWidth;
	UInt32 m_uHeight;
	PixelFormat m_eFormat;
	CurrentLevelData m_CurrentLevelData;
	TextureData m_Data;
	SharedPtr<BaseTexture> m_pTexture;
	Bool m_bNetworkPrefetched;
#if !SEOUL_SHIP
	Bool m_bTriedRecook;
#endif
}; // class TextureContentLoader

} // namespace Seoul

#endif // include guard
