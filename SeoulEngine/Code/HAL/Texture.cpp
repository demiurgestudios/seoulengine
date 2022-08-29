/**
 * \file Texture.cpp
 * \brief Platform-independent representation of a graphics
 * texture resource
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FileManager.h"
#include "ReflectionDefine.h"
#include "RenderDevice.h"
#include "Texture.h"
#include "TextureContentLoader.h"
#include "TextureManager.h"

namespace Seoul
{

SEOUL_BEGIN_TYPE(TextureContentHandle)
SEOUL_END_TYPE()

SharedPtr<BaseTexture> Content::Traits<BaseTexture>::GetPlaceholder(FilePath filePath)
{
	return TextureManager::Get()->GetPlaceholderTexture();
}

Bool Content::Traits<BaseTexture>::FileChange(FilePath filePath, const TextureContentHandle& hEntry)
{
	// Sanity check that the type is a texture type - Content::Store<> should have
	// done most of the filtering for us, making sure that the target already
	// exists in our store.
	if (IsTextureFileType(filePath.GetType()))
	{
		Load(filePath, hEntry);
		return true;
	}

	return false;
}

void Content::Traits<BaseTexture>::Load(FilePath filePath, const TextureContentHandle& hEntry)
{
	Content::LoadManager::Get()->Queue(
		SharedPtr<Content::LoaderBase>(SEOUL_NEW(MemoryBudgets::Content) TextureContentLoader(filePath, hEntry)));
}

Bool Content::Traits<BaseTexture>::PrepareDelete(FilePath filePath, Content::Entry<BaseTexture, KeyType>& entry)
{
	return true;
}

/** Call to append a level, must be smaller than all previous levels. */
TextureData TextureData::PushBackLevel(const TextureData& base, const SharedPtr<TextureLevelData>& pLevel)
{
	// Initially identical to base.
	TextureData ret;
	ret.m_vLevels = base.m_vLevels;
	ret.m_bHasSecondary = base.m_bHasSecondary;
	
	// Sanity - next level must always be smaller than previous level.
	SEOUL_ASSERT(ret.m_vLevels.IsEmpty() || ret.m_vLevels.Back()->GetAllSizeInBytes() > pLevel->GetAllSizeInBytes());

	// Add the additional level.
	ret.m_vLevels.PushBack(pLevel);
	
	// Secondary only carries through if all levels have a secondary data blob.
	ret.m_bHasSecondary = (ret.m_bHasSecondary && (nullptr != pLevel->GetTextureDataSecondary()));
	return ret;
}

/** Call to prepend a level, must be larger than all previous levels. */
TextureData TextureData::PushFrontLevel(const TextureData& base, const SharedPtr<TextureLevelData>& pLevel)
{
	// Sanity - next level must always be larger than the first level.
	SEOUL_ASSERT(base.m_vLevels.IsEmpty() || base.m_vLevels.Front()->GetAllSizeInBytes() < pLevel->GetAllSizeInBytes());

	// Return data.
	TextureData ret;
	ret.m_bHasSecondary = base.m_bHasSecondary;

	// Add the new level.
	ret.m_vLevels.PushBack(pLevel);
	// Append existing.
	ret.m_vLevels.Append(base.m_vLevels);

	// Secondary only carries through if all levels have a secondary data blob.
	ret.m_bHasSecondary = (ret.m_bHasSecondary && (nullptr != pLevel->GetTextureDataSecondary()));
	return ret;
}

TextureData TextureData::CreateFromInMemoryBuffer(
	void const* pData,
	UInt32 uDataSizeInBytes,
	PixelFormat& reFormat)
{
	// On devices that do not support BGRA format when provided
	// a BGRA buffer, swap the channels.
	if (PixelFormat::kA8R8G8B8 == reFormat && !RenderDevice::Get()->GetCaps().m_bBGRA)
	{
		SEOUL_ASSERT(uDataSizeInBytes % 4u == 0u);
		ColorSwapR8B8((UInt32*)pData, (UInt32*)((UInt8*)pData + uDataSizeInBytes));
		reFormat = PixelFormat::kA8B8G8R8;
	}

	// Instantiate the level - it takes ownership of the passed in buffer.
	SharedPtr<TextureLevelData> p(SEOUL_NEW(MemoryBudgets::Content) TextureLevelData(
		pData,
		uDataSizeInBytes,
		pData,
		nullptr));

	// Append to an empty texture data object.
	return PushBackLevel(TextureData(), p);
}

TextureLevelData::TextureLevelData(
	void const* pAllData,
	UInt32 uAllSizeInBytes,
	void const* pTextureData,
	void const* pTextureDataSecondary)
	: m_pAllData(pAllData)
	, m_uAllSizeInBytes(uAllSizeInBytes)
	, m_pTextureData(pTextureData)
	, m_pTextureDataSecondary(pTextureDataSecondary)
{
}

TextureLevelData::~TextureLevelData()
{
	MemoryManager::Deallocate((void*)m_pAllData);
}

TextureData::TextureData()
	: m_vLevels()
	, m_bHasSecondary(true)
{
}

TextureData::~TextureData()
{
}

} // namespace Seoul
