/**
 * \file UIContentLoader.cpp
 * \brief Specialization of Content::LoaderBase for loading Falcon FCN files.
 *
 * UI::ContentLoader loads cooked, compressed Falcon FCN files and generates
 * a template scene graph for later instantiation, typically into a UI::Movie
 * instance.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Compress.h"
#include "CookManager.h"
#include "DataStoreParser.h"
#include "Engine.h"
#include "FalconFCNFile.h"
#include "FileManager.h"
#include "UIContentLoader.h"
#include "UIManager.h"
#include "UIUtil.h"

namespace Seoul::UI
{

ContentLoader::ContentLoader(FilePath filePath, const Content::Handle<FCNFileData>& hEntry)
	: Content::LoaderBase(filePath)
	, m_hFCNFileEntry(hEntry)
	, m_pTotalFileData(nullptr)
	, m_zTotalDataSizeInBytes(0u)
{
	m_hFCNFileEntry.GetContentEntry()->IncrementLoaderCount();
}

ContentLoader::~ContentLoader()
{
	// Block until this Content::Loaded is in a non-loading state.
	WaitUntilContentIsNotLoading();

	// If a failure happened after we loaded but before the header could process
	// the data, free it
	if (nullptr != m_pTotalFileData)
	{
		MemoryManager::Deallocate(m_pTotalFileData);
		m_pTotalFileData = nullptr;
	}

	InternalReleaseEntry();
}

Content::LoadState ContentLoader::InternalExecuteContentLoadOp()
{
	// Must be on the file IO thread to load the movie.
	if (Content::LoadState::kLoadingOnFileIOThread == GetContentLoadState())
	{
#if !SEOUL_SHIP
		// Check if cooking occurred - if so, validate the file.
		auto const eResult = CookManager::Get()->CookIfOutOfDate(GetFilePath());
		if (CookManager::kSuccess == eResult)
		{
			// Return value ignored since this is an asynchronous dispatch.
			(void)Manager::Get()->ValidateUiFile(GetFilePath(), false);
		}
#else // SEOUL_SHIP
		// Cook the SWF if necessary.
		CookManager::Get()->CookIfOutOfDate(GetFilePath());
#endif // /SEOUL_SHIP

		// Read the data into a buffer - if this succeeds, cache the data and then
		// either wait for dependencies to load, or finish the load.
		if (FileManager::Get()->ReadAll(
			GetFilePath(),
			m_pTotalFileData,
			m_zTotalDataSizeInBytes,
			kLZ4MinimumAlignment,
			MemoryBudgets::UIData))
		{
			// Finish loading off the file IO thread.
			return Content::LoadState::kLoadingOnWorkerThread;
		}
	}
	// We get here to finish processing the Falcon data, or to wait for dependencies to
	// finish loading. Check if they're done (or have failed to load), and continue.
	else if (Content::LoadState::kLoadingOnWorkerThread == GetContentLoadState())
	{
		// If the file data is still not nullptr, it means we need to pass it to the
		// head.
		if (nullptr != m_pTotalFileData)
		{
			// Decompress the data.
			{
				void* pUncompressedFileData = nullptr;
				UInt32 zUncompressedFileDataSizeInBytes = 0u;

				if (!ZSTDDecompress(
					m_pTotalFileData,
					m_zTotalDataSizeInBytes,
					pUncompressedFileData,
					zUncompressedFileDataSizeInBytes,
					MemoryBudgets::UIData))
				{
					return Content::LoadState::kError;
				}
				else
				{
					Swap(m_pTotalFileData, pUncompressedFileData);
					Swap(m_zTotalDataSizeInBytes, zUncompressedFileDataSizeInBytes);
					MemoryManager::Deallocate(pUncompressedFileData);
				}
			}
		}

		// Instantiate the FCNFile.
		String const sAbsoluteFilename(GetFilePath().GetAbsoluteFilename());
		SharedPtr<Falcon::FCNFile> pFCNFile(SEOUL_NEW(MemoryBudgets::Falcon) Falcon::FCNFile(
			HString(sAbsoluteFilename.CStr(), sAbsoluteFilename.GetSize()),
			(UInt8 const*)m_pTotalFileData,
			m_zTotalDataSizeInBytes));

		// Invalid or corrupt file.
		if (!pFCNFile.IsValid() || !pFCNFile->IsOk())
		{
			return Content::LoadState::kError;
		}
		// Otherwise, instantiate the FCNFileData, set the entry and finish the load.
		else
		{
			SharedPtr<FCNFileData> pFCNFileData(SEOUL_NEW(MemoryBudgets::UIRuntime) FCNFileData(pFCNFile, GetFilePath()));

			m_hFCNFileEntry.GetContentEntry()->AtomicReplace(pFCNFileData);
			InternalReleaseEntry();
			return Content::LoadState::kLoaded;
		}
	}

	return Content::LoadState::kError;
}

/**
 * Release the loader's reference on its content entry - doing this as
 * soon as loading completes allows anything waiting for the load to react
 * as soon as possible.
 */
void ContentLoader::InternalReleaseEntry()
{
	if (m_hFCNFileEntry.IsInternalPtrValid())
	{
		// NOTE: We need to release our reference before decrementing the loader count.
		// This is safe, because a Content::Entry's Content::Store always maintains 1 reference,
		// and does not release it until the content is done loading.
		Content::Handle<FCNFileData>::EntryType* pEntry = m_hFCNFileEntry.GetContentEntry().GetPtr();
		m_hFCNFileEntry.Reset();
		pEntry->DecrementLoaderCount();
	}
}

} // namespace Seoul::UI
