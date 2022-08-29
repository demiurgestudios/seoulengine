/**
 * \file AssetManager.h
 * \brief AssetManager is the singleton manager for persistent assets
 * that must be loaded from disk.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ASSET_MANAGER_H
#define ASSET_MANAGER_H

#include "Asset.h"
#include "ContentStore.h"
#include "Singleton.h"

namespace Seoul
{

class AssetManager SEOUL_SEALED : public Singleton<AssetManager>
{
public:
	AssetManager();
	~AssetManager();

	/**
	 * @return A persistent Content::Handle<> to the asset filePath.
	 */
	AssetContentHandle GetAsset(FilePath filePath)
	{
		return m_Content.GetContent(filePath);
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

	Result GetAssetMemoryUsageInBytes(UInt32& rzMemoryUsageInBytes) const;

private:
	friend class AssetContentLoader;
	Content::Store<Asset> m_Content;
}; // class AssetManager

} // namespace Seoul

#endif // include guard
