/**
 * \file AssetContentLoader.h
 * \brief Specialization of Content::LoaderBase for loading assets.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ASSET_CONTENT_LOADER_H
#define ASSET_CONTENT_LOADER_H

#include "Asset.h"
#include "ContentLoaderBase.h"
#include "ContentKey.h"

namespace Seoul
{

/**
 * Specialization of Content::LoaderBase for loading assets.
 */
class AssetContentLoader SEOUL_SEALED : public Content::LoaderBase
{
public:
	AssetContentLoader(FilePath filePath, const AssetContentHandle& hEntry);
	virtual ~AssetContentLoader();

protected:
	virtual Content::LoadState InternalExecuteContentLoadOp() SEOUL_OVERRIDE;

private:
	void InternalFreeAssetData();
	void InternalReleaseEntry();

	AssetContentHandle m_hEntry;
	void* m_pRawAssetFileData;
	UInt32 m_zFileSizeInBytes;
	Bool m_bNetworkPrefetched;
}; // class AssetContentLoader

} // namespace Seoul

#endif // include guard
