/**
 * \file AssetContentLoader.cpp
 * \brief Specialization of Content::LoaderBase for loading assets.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "AssetContentLoader.h"
#include "AssetManager.h"
#include "Compress.h"
#include "CookManager.h"
#include "FileManager.h"
#include "Mesh.h"
#include "RenderDevice.h"
#include "SeoulFile.h"

namespace Seoul
{

AssetContentLoader::AssetContentLoader(FilePath filePath, const AssetContentHandle& hEntry)
	: Content::LoaderBase(filePath)
	, m_hEntry(hEntry)
	, m_pRawAssetFileData(nullptr)
	, m_zFileSizeInBytes(0u)
	, m_bNetworkPrefetched(false)
{
	m_hEntry.GetContentEntry()->IncrementLoaderCount();

	// Kick off prefetching of the asset (this will be a nop for local files)
	m_bNetworkPrefetched = FileManager::Get()->NetworkPrefetch(filePath);
}

AssetContentLoader::~AssetContentLoader()
{
	// Block until this Content::LoaderBase is in a non-loading state.
	WaitUntilContentIsNotLoading();

	InternalReleaseEntry();
	InternalFreeAssetData();
}

/**
 * Method which handles actual loading of asset data - can
 * perform a variety of ops depending on the platform and type
 * of asset data.
 */
Content::LoadState AssetContentLoader::InternalExecuteContentLoadOp()
{
	// First step, load the data.
	if (Content::LoadState::kLoadingOnFileIOThread == GetContentLoadState())
	{
		// If we're the only reference to the content, "cancel" the load.
		if (m_hEntry.IsUnique())
		{
			m_hEntry.GetContentEntry()->CancelLoad();
			InternalReleaseEntry();
			return Content::LoadState::kLoaded;
		}

		// Only try to read from disk. Let the prefetch finish the download.
		if (FileManager::Get()->IsServicedByNetwork(GetFilePath()))
		{
			if (FileManager::Get()->IsNetworkFileIOEnabled())
			{
				// Kick off a prefetch if we have not yet done so.
				if (!m_bNetworkPrefetched)
				{
					m_bNetworkPrefetched = FileManager::Get()->NetworkPrefetch(GetFilePath());
				}

				return Content::LoadState::kLoadingOnFileIOThread;
			}
			else // This is a network download, but the network system isn't enabled so it will never complete
			{
				InternalFreeAssetData();

				// Swap an invalid entry into the slot.
				m_hEntry.GetContentEntry()->AtomicReplace(SharedPtr<Asset>());

				// Done with loading body, decrement the loading count.
				return Content::LoadState::kError;
			}
		}

		// Cook the out of date file in developer builds.
		(void)CookManager::Get()->CookIfOutOfDate(GetFilePath());

		UInt32 const zMaxReadSize = kDefaultMaxReadSize;

		// If reading succeeds, continue on a worker thread.
		if (FileManager::Get()->ReadAll(
			GetFilePath(),
			m_pRawAssetFileData,
			m_zFileSizeInBytes,
			kLZ4MinimumAlignment,
			MemoryBudgets::Content,
			zMaxReadSize))
		{
			// Finish the load on a worker thread.
			return Content::LoadState::kLoadingOnWorkerThread;
		}
	}
	// Second step, decompress the data.
	else if (Content::LoadState::kLoadingOnWorkerThread == GetContentLoadState())
	{
		void* pUncompressedFileData = nullptr;
		UInt32 zUncompressedFileDataInBytes = 0u;

		if (ZSTDDecompress(
			m_pRawAssetFileData,
			m_zFileSizeInBytes,
			pUncompressedFileData,
			zUncompressedFileDataInBytes,
			MemoryBudgets::Content))
		{
			InternalFreeAssetData();

			SharedPtr<Asset> pAsset(SEOUL_NEW(MemoryBudgets::Rendering) Asset);
			Bool const bSuccess = pAsset->Load(GetFilePath(), pUncompressedFileData, zUncompressedFileDataInBytes);
			MemoryManager::Deallocate(pUncompressedFileData);

			if (bSuccess)
			{
				m_hEntry.GetContentEntry()->AtomicReplace(pAsset);
				InternalReleaseEntry();

				// Done with loading body, decrement the loading count.
				return Content::LoadState::kLoaded;
			}
		}
	}

	// If we get here, an error occured, so cleanup and return the error condition.
	InternalFreeAssetData();

	// Swap an invalid entry into the slot.
	m_hEntry.GetContentEntry()->AtomicReplace(SharedPtr<Asset>());

	// Done with loading body, decrement the loading count.
	return Content::LoadState::kError;
}

/**
 * Frees loaded asset data if still owned by this AssetContentLoader.
 */
void AssetContentLoader::InternalFreeAssetData()
{
	if (nullptr != m_pRawAssetFileData)
	{
		MemoryManager::Deallocate(m_pRawAssetFileData);
		m_pRawAssetFileData = nullptr;
	}
	m_zFileSizeInBytes = 0u;
}

/**
 * Release the loader's reference on its content entry - doing this as
 * soon as loading completes allows anything waiting for the load to react
 * as soon as possible.
 */
void AssetContentLoader::InternalReleaseEntry()
{
	if (m_hEntry.IsInternalPtrValid())
	{
		// NOTE: We need to release our reference before decrementing the loader count.
		// This is safe, because a Content::Entry's Content::Store always maintains 1 reference,
		// and does not release it until the content is done loading.
		AssetContentHandle::EntryType* pEntry = m_hEntry.GetContentEntry().GetPtr();
		m_hEntry.Reset();
		pEntry->DecrementLoaderCount();
	}
}

} // namespace Seoul
