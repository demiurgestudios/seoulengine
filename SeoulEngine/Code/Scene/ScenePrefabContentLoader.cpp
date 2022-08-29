/**
 * \file ScenePrefabContentLoader.cpp
 * \brief Specialization of Content::LoaderBase for loading Prefab.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Compress.h"
#include "CookManager.h"
#include "FileManager.h"
#include "ScenePrefab.h"
#include "ScenePrefabContentLoader.h"
#include "ScenePrefabManager.h"

#if SEOUL_WITH_SCENE

namespace Seoul::Scene
{

PrefabContentLoader::PrefabContentLoader(FilePath filePath, const PrefabContentHandle& hEntry)
	: Content::LoaderBase(filePath)
	, m_hEntry(hEntry)
	, m_pRawPrefabFileData(nullptr)
	, m_zFileSizeInBytes(0u)
	, m_bNetworkPrefetched(false)
{
	m_hEntry.GetContentEntry()->IncrementLoaderCount();

	// Kick off prefetching of the asset (this will be a nop for local files)
	m_bNetworkPrefetched = FileManager::Get()->NetworkPrefetch(filePath);
}

PrefabContentLoader::~PrefabContentLoader()
{
	// Block until this Content::LoaderBase is in a non-loading state.
	WaitUntilContentIsNotLoading();

	InternalReleaseEntry();
	InternalFreePrefabData();
}

/**
 * Method which handles actual loading of mesh data - can
 * perform a variety of ops depending on the platform and type
 * of mesh data.
 */
Content::LoadState PrefabContentLoader::InternalExecuteContentLoadOp()
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
				InternalFreePrefabData();

				// Swap an invalid entry into the slot.
				m_hEntry.GetContentEntry()->AtomicReplace(SharedPtr<Prefab>());

				// Done with loading body, decrement the loading count.
				return Content::LoadState::kError;
			}
		}

		// Cook the out of date file in developer builds.
		(void)CookManager::Get()->CookIfOutOfDate(GetFilePath());

		UInt32 const zMaxReadSize = kDefaultMaxReadSize;

		// If reading succeeds, continue on the background thread.
		if (FileManager::Get()->ReadAll(
			GetFilePath(),
			m_pRawPrefabFileData,
			m_zFileSizeInBytes,
			kLZ4MinimumAlignment,
			MemoryBudgets::Content,
			zMaxReadSize))
		{
			// Finish the load on the background thread.
			return Content::LoadState::kLoadingOnWorkerThread;
		}
	}
	// Second step, decompress the data.
	else if (Content::LoadState::kLoadingOnWorkerThread == GetContentLoadState())
	{
		void* pUncompressedFileData = nullptr;
		UInt32 zUncompressedFileDataInBytes = 0u;

		if (ZSTDDecompress(
			m_pRawPrefabFileData,
			m_zFileSizeInBytes,
			pUncompressedFileData,
			zUncompressedFileDataInBytes,
			MemoryBudgets::Content))
		{
			InternalFreePrefabData();

			// Give ownershipt of the compressed data to a buffered sync file
			// to complete loading.
			FullyBufferedSyncFile file(
				pUncompressedFileData,
				zUncompressedFileDataInBytes,
				true);

			SharedPtr<Prefab> pPrefab(SEOUL_NEW(MemoryBudgets::Rendering) Prefab);
			if (pPrefab->Load(GetFilePath(), file))
			{
				// TODO: Need to come up with a better mechanism in Jobs::Manager for this.
				// Task depenencies for example.

				// Yield until sub scenes are loaded.
				//
				// We are a low priority job waiting on other
				// work for the remainder of this block.
				{
					Jobs::ScopedQuantum scope(*this, Jobs::Quantum::kWaitingForDependency);
					while (pPrefab->AreNestedPrefabsLoading())
					{
						Jobs::Manager::Get()->YieldThreadTime();
					}
				}

				m_hEntry.GetContentEntry()->AtomicReplace(pPrefab);
				InternalReleaseEntry();

				// Done with loading body, decrement the loading count.
				return Content::LoadState::kLoaded;
			}
		}
	}

	// If we get here, an error occured, so cleanup and return the error condition.
	InternalFreePrefabData();

	// Swap an invalid entry into the slot.
	m_hEntry.GetContentEntry()->AtomicReplace(SharedPtr<Prefab>());

	// Done with loading body, decrement the loading count.
	return Content::LoadState::kError;
}

/**
 * Frees loaded mesh data if still owned by this PrefabContentLoader.
 */
void PrefabContentLoader::InternalFreePrefabData()
{
	if (nullptr != m_pRawPrefabFileData)
	{
		MemoryManager::Deallocate(m_pRawPrefabFileData);
		m_pRawPrefabFileData = nullptr;
	}
	m_zFileSizeInBytes = 0u;
}

/**
 * Release the loader's reference on its content entry - doing this as
 * soon as loading completes allows anything waiting for the load to react
 * as soon as possible.
 */
void PrefabContentLoader::InternalReleaseEntry()
{
	if (m_hEntry.IsInternalPtrValid())
	{
		// NOTE: We need to release our reference before decrementing the loader count.
		// This is safe, because a Content::Entry's Content::Store always maintains 1 reference,
		// and does not release it until the content is done loading.
		PrefabContentHandle::EntryType* pEntry = m_hEntry.GetContentEntry().GetPtr();
		m_hEntry.Reset();
		pEntry->DecrementLoaderCount();
	}
}

} // namespace Seoul::Scene

#endif // /#if SEOUL_WITH_SCENE
