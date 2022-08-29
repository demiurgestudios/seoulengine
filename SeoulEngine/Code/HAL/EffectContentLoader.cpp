/**
 * \file EffectContentLoader.cpp
 * \brief Specialization of Content::LoaderBase for loading effects.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Compress.h"
#include "CookManager.h"
#include "Effect.h"
#include "EffectContentLoader.h"
#include "EffectManager.h"
#include "FileManager.h"
#include "RenderDevice.h"
#include "SeoulFile.h"

namespace Seoul
{

EffectContentLoader::EffectContentLoader(FilePath filePath, const EffectContentHandle& pEntry)
	: Content::LoaderBase(filePath)
	, m_hEntry(pEntry)
	, m_pEffect(nullptr)
	, m_pRawEffectFileData(nullptr)
	, m_zFileSizeInBytes(0u)
	, m_bNetworkPrefetched(false)
{
	m_hEntry.GetContentEntry()->IncrementLoaderCount();

	// Kick off prefetching of the asset (this will be a nop for local files)
	m_bNetworkPrefetched = FileManager::Get()->NetworkPrefetch(filePath);
}

EffectContentLoader::~EffectContentLoader()
{
	// Block until this Content::LoaderBase is in a non-loading state.
	WaitUntilContentIsNotLoading();

	InternalReleaseEntry();
	InternalFreeEffectData();
}

/**
 * Method which handles actual loading of effects.
 */
Content::LoadState EffectContentLoader::InternalExecuteContentLoadOp()
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
				m_pEffect.Reset();
				InternalFreeEffectData();

				// Swap the invalid effect into the slot.
				m_hEntry.GetContentEntry()->AtomicReplace(SharedPtr<Effect>());

				// Done with loading body, decrement the loading count.
				return Content::LoadState::kError;
			}
		}

		// Conditionally cook if the cooked file is not up to date with the source file.
		CookManager::Get()->CookIfOutOfDate(GetFilePath());

		UInt32 const zMaxReadSize = kDefaultMaxReadSize;

		// If reading succeeds, check if we can immediately create
		// the effect object.
		if (FileManager::Get()->ReadAll(
			GetFilePath(),
			m_pRawEffectFileData,
			m_zFileSizeInBytes,
			kLZ4MinimumAlignment,
			MemoryBudgets::Content,
			zMaxReadSize))
		{
			// Decompress on a worker thread.
			return Content::LoadState::kLoadingOnWorkerThread;
		}
		// If reading fails, we have an error, so clear state
		// data and return an error code.
		else
		{
			m_pEffect.Reset();
			InternalFreeEffectData();

			// Swap the error effect into the slot.
			m_hEntry.GetContentEntry()->AtomicReplace(SharedPtr<Effect>());

			// Done with loading body, decrement the loading count.
			return Content::LoadState::kError;
		}
	}
	// Second step, decompress the data.
	else if (Content::LoadState::kLoadingOnWorkerThread == GetContentLoadState())
	{
		void* pUncompressedFileData = nullptr;
		UInt32 zUncompressedFileDataInBytes = 0u;

		if (LZ4Decompress(
			m_pRawEffectFileData,
			m_zFileSizeInBytes,
			pUncompressedFileData,
			zUncompressedFileDataInBytes,
			MemoryBudgets::Content))
		{
			InternalFreeEffectData();
			Swap(pUncompressedFileData, m_pRawEffectFileData);
			Swap(zUncompressedFileDataInBytes, m_zFileSizeInBytes);

			return Content::LoadState::kLoadingOnRenderThread;
		}
		else
		{
			m_pEffect.Reset();
			InternalFreeEffectData();

			// Swap the error effect into the slot.
			m_hEntry.GetContentEntry()->AtomicReplace(SharedPtr<Effect>());

			// Done with loading body, decrement the loading count.
			return Content::LoadState::kError;
		}
	}

	// Attempt to create the effect - nullptr means creation failed.
	SEOUL_ASSERT(!m_pEffect.IsValid());

	m_pEffect = RenderDevice::Get()->CreateEffectFromFileInMemory(
		GetFilePath(),
		m_pRawEffectFileData,
		m_zFileSizeInBytes);

	// If the device grabbed the data, just null it out.
	if (m_pEffect.IsValid() &&
		RenderDevice::Get()->TakesOwnershipOfEffectFileData())
	{
		m_pRawEffectFileData = nullptr;
		m_zFileSizeInBytes = 0u;
	}

	// Cleanup state.
	InternalFreeEffectData();

	// If we have a effect object, loading succeeded.
	if (m_pEffect.IsValid())
	{
		m_hEntry.GetContentEntry()->AtomicReplace(m_pEffect);
		InternalReleaseEntry();
		// Done with loading body, decrement the loading count.
		return Content::LoadState::kLoaded;
	}
	// If loading failed, place the error effect in the slot for this effect.
	else
	{
		m_hEntry.GetContentEntry()->AtomicReplace(SharedPtr<Effect>());
		// Done with loading body, decrement the loading count.
		return Content::LoadState::kError;
	}
}

/**
 * Frees loaded effect data if still owned by this EffectContentLoader.
 */
void EffectContentLoader::InternalFreeEffectData()
{
	if (nullptr != m_pRawEffectFileData)
	{
		MemoryManager::Deallocate(m_pRawEffectFileData);
		m_pRawEffectFileData = nullptr;
	}
	m_zFileSizeInBytes = 0u;
}

/**
 * Release the loader's reference on its content entry - doing this as
 * soon as loading completes allows anything waiting for the load to react
 * as soon as possible.
 */
void EffectContentLoader::InternalReleaseEntry()
{
	if (m_hEntry.IsInternalPtrValid())
	{
		// NOTE: We need to release our reference before decrementing the loader count.
		// This is safe, because a Content::Entry's Content::Store always maintains 1 reference,
		// and does not release it until the content is done loading.
		EffectContentHandle::EntryType* pEntry = m_hEntry.GetContentEntry().GetPtr();
		m_hEntry.Reset();
		pEntry->DecrementLoaderCount();
	}
}

} // namespace Seoul
