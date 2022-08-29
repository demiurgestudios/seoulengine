/**
 * \file TextureManager.h
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

#pragma once
#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H

#include "ContentLoadManager.h"
#include "ContentStore.h"
#include "FilePath.h"
#include "HashTable.h"
#include "SeoulString.h"
#include "Singleton.h"
#include "StandardPlatformIncludes.h"
#include "Texture.h"
#include "TextureConfig.h"

namespace Seoul
{

// Forward declarations
namespace Content { struct ChangeEvent; }
class RenderDevice;

enum class TextureCompressionClass
{
	kDXTn,
	kETC1,
	kPVRTC,
};

class TextureManager SEOUL_SEALED : public Singleton<TextureManager>
{
public:
	typedef HashTable<FilePath, TextureConfig, MemoryBudgets::Rendering> TextureConfigTable;

	TextureManager();
	~TextureManager();

	/**
	 * @return A SharedPtr<> to the error texture. The error texture should
	 * be used to indicate a file load failure.
	 */
	SharedPtr<BaseTexture> GetErrorTexture() const
	{
		return m_pErrorTexture;
	}

	/*
	 * @return A SharedPtr<> to the placeholder texture. The placeholder
	 * texture should be used to substitute for a texture pending load.
	 */
	SharedPtr<BaseTexture> GetPlaceholderTexture() const
	{
		return m_pPlaceholderTexture;
	}

	// Return the class of the compressed textures are expected to use on the current platform.
	TextureCompressionClass GetPlatformCompressionClass() const
	{
#if SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_IOS
		return TextureCompressionClass::kETC1; // Not a typo for iOS, we use ETC1 on both platforms.
#elif SEOUL_PLATFORM_LINUX || SEOUL_PLATFORM_WINDOWS
		return TextureCompressionClass::kDXTn;
#else
#		error "Define for this platform."
		return TextureCompressionClass::kDXTn;
#endif
	}

	/**
	 * @return A persistent Content::Handle<> to the texture filePath.
	 */
	TextureContentHandle GetTexture(FilePath filePath)
	{
		return m_Content.GetContent(filePath);
	}

	/**
	 * @return TextureConfig associated with filePath, or the default
	 * if nothing has set new texture state for filePath.
	 */
	TextureConfig GetTextureConfig(FilePath filePath)
	{
		TextureConfig ret;
		{
			Lock lock(m_TextureConfigMutex);
			(void)m_tTextureConfig.GetValue(filePath, ret);
		}
		return ret;
	}

	/**
	 * Update the global texture state for filePath.
	 *
	 * Called to associate texture state with filePath. This will
	 * be applied the next time filePath is loaded, and does not
	 * update the state of an already loaded instance of filePath.
	 */
	void UpdateTextureConfig(FilePath filePath, const TextureConfig& textureConfig)
	{
		Lock lock(m_TextureConfigMutex);
		(void)m_tTextureConfig.Overwrite(filePath, textureConfig);
	}

	enum Result
	{
		/** Memory usage data is not available on the current platform. */
		kNoMemoryUsageAvailable,

		/**
		 * Not all textures expose memory usage, so the estimate is
		 * a low estimate of memory usage.
		 */
		kApproximateMemoryUsage,

		/**
		 * All textures returned memory usage data, so the
		 * returned value is the exact number of bytes occupied
		 * by textures on the current platform.
		 */
		kExactMemoryUsage
	};

	Result GetTextureMemoryUsageInBytes(UInt32& rzMemoryUsageInBytes) const;

private:
	void InternalCreateBuiltinTextures();

	friend class TextureContentLoader;
	Content::Store<BaseTexture> m_Content;
	TextureConfigTable m_tTextureConfig;
	Mutex m_TextureConfigMutex;
	SharedPtr<BaseTexture> m_pErrorTexture;
	SharedPtr<BaseTexture> m_pPlaceholderTexture;
}; // class TextureManager

} // namespace Seoul

#endif // include guard
