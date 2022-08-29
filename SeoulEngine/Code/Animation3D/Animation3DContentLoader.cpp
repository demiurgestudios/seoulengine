/**
* \file Animation3DContentLoader.cpp
* \brief Specialization of Content::LoaderBase for loading animation
* clip and skeleton data.
*
* Copyright (c) Demiurge Studios, Inc.
* 
* This source code is licensed under the MIT license.
* Full license details can be found in the LICENSE file
* in the root directory of this source tree.
*/

#include "Animation3DClipDefinition.h"
#include "Animation3DContentLoader.h"
#include "Compress.h"
#include "CookManager.h"
#include "FileManager.h"

#if SEOUL_WITH_ANIMATION_3D

namespace Seoul::Animation3D
{

ContentLoader::ContentLoader(FilePath filePath, const Animation3DDataContentHandle& hEntry)
	: Content::LoaderBase(filePath)
	, m_hEntry(hEntry)
	, m_pRawData(nullptr)
	, m_uDataSizeInBytes(0u)
	, m_bNetworkPrefetched(false)
{
	m_hEntry.GetContentEntry()->IncrementLoaderCount();

	// Kick off prefetching of the asset (this will be a nop for local files)
	m_bNetworkPrefetched = FileManager::Get()->NetworkPrefetch(filePath);
}

ContentLoader::~ContentLoader()
{
	// Block until this Content::LoaderBase is in a non-loading state.
	WaitUntilContentIsNotLoading();

	InternalReleaseEntry();
	InternalFreeData();
}

Content::LoadState ContentLoader::InternalExecuteContentLoadOp()
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
				InternalFreeData();

				// Swap an invalid entry into the slot.
				m_hEntry.GetContentEntry()->AtomicReplace(SharedPtr<DataDefinition>());

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
			m_pRawData,
			m_uDataSizeInBytes,
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
			m_pRawData,
			m_uDataSizeInBytes,
			pUncompressedFileData,
			zUncompressedFileDataInBytes,
			MemoryBudgets::Content))
		{
			InternalFreeData();

			SharedPtr<DataDefinition> pData(SEOUL_NEW(MemoryBudgets::Rendering) DataDefinition);
			Bool const bSuccess = pData->Load(GetFilePath(), pUncompressedFileData, zUncompressedFileDataInBytes);
			MemoryManager::Deallocate(pUncompressedFileData);

			if (bSuccess)
			{
				m_hEntry.GetContentEntry()->AtomicReplace(pData);
				InternalReleaseEntry();

				// Done with loading body, decrement the loading count.
				return Content::LoadState::kLoaded;
			}
		}
	}

	// If we get here, an error occured, so cleanup and return the error condition.
	InternalFreeData();

	// Swap an invalid entry into the slot.
	m_hEntry.GetContentEntry()->AtomicReplace(SharedPtr<DataDefinition>());

	// Done with loading body, decrement the loading count.
	return Content::LoadState::kError;
}

void ContentLoader::InternalFreeData()
{
	if (nullptr != m_pRawData)
	{
		MemoryManager::Deallocate(m_pRawData);
		m_pRawData = nullptr;
	}
	m_uDataSizeInBytes = 0u;
}

void ContentLoader::InternalReleaseEntry()
{
	if (m_hEntry.IsInternalPtrValid())
	{
		// NOTE: We need to release our reference before decrementing the loader count.
		// This is safe, because a Content::Entry's Content::Store always maintains 1 reference,
		// and does not release it until the content is done loading.
		Animation3DDataContentHandle::EntryType* pEntry = m_hEntry.GetContentEntry().GetPtr();
		m_hEntry.Reset();
		pEntry->DecrementLoaderCount();
	}
}

} // namespace Seoul::Animation3D

#endif // /#if SEOUL_WITH_ANIMATION_3D
