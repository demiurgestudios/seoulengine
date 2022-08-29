/**
 * \file FxStudioContentLoader.cpp
 *
 * \brief Specialization of Content::LoaderBase for loading FxStudio banks
 * (.FXB files).
 *
 * \warning Don't instantiate this class to load a FxStudio banks
 * directly unless you know what you are doing. Loading FxStudio banks
 * this way prevents the bank from being managed by ContentLoadManager.
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
#include "FxStudioBankFile.h"
#include "FxStudioContentLoader.h"
#include "FxStudioManager.h"

#if SEOUL_WITH_FX_STUDIO
#include "FxStudioRT.h"

namespace Seoul::FxStudio
{

ContentLoader::ContentLoader(
	FilePath filePath,
	const Content::Handle<BankFile>& pEntry)
	: Content::LoaderBase(filePath)
	, m_hEntry(pEntry)
	, m_pCompressedFileData(nullptr)
	, m_zCompressedFileDataSizeInBytes(0u)
	, m_Data()
{
	m_hEntry.GetContentEntry()->IncrementLoaderCount();
}

ContentLoader::~ContentLoader()
{
	// Block until this FxStudio::ContentLoader is in a non-loading state.
	WaitUntilContentIsNotLoading();

	// If a failure happened after we loaded but before the script
	// could take ownership of the data, free it.
	InternalFreeCompressedData();

	// Release the content populate entry if it is still valid.
	InternalReleaseEntry();

	// Free the data.
	m_Data.DeallocateAndClear();
}

/**
 * Method in which actual loading occurs. Only expected to be
 * called once, on the file IO thread.
 */
Content::LoadState ContentLoader::InternalExecuteContentLoadOp()
{
	// Must be on the file IO thread to load the fx data.
	if (Content::LoadState::kLoadingOnFileIOThread == GetContentLoadState())
	{
		// Cache the filePath.
		FilePath const filePath(GetFilePath());

#if !SEOUL_SHIP
		// Conditionally cook if the cooked file is not up to date with the source file.
		(void)CookManager::Get()->CookIfOutOfDate(filePath);
#endif

		// Read the data into a buffer - if this succeeds, cache the data
		// and switch to a worker thread to perform decompression.
		if (FileManager::Get()->ReadAll(
			filePath,
			m_pCompressedFileData,
			m_zCompressedFileDataSizeInBytes,
			kLZ4MinimumAlignment,
			MemoryBudgets::Fx))
		{
			// Finish loading off the file IO thread.
			return Content::LoadState::kLoadingOnWorkerThread;
		}
	}
	// We get here to decompress the FX data.
	else if (Content::LoadState::kLoadingOnWorkerThread == GetContentLoadState())
	{
		// Sanity check, should have been insured by the previous step.
		SEOUL_ASSERT(nullptr != m_pCompressedFileData);

		// Decompress the data.
		void* pUncompressedFileData = nullptr;
		UInt32 zUncompressedFileDataSizeInBytes = 0u;

		// If failed, return with an error.
		if (!ZSTDDecompress(
			m_pCompressedFileData,
			m_zCompressedFileDataSizeInBytes,
			pUncompressedFileData,
			zUncompressedFileDataSizeInBytes,
			MemoryBudgets::Scripting))
		{
			return Content::LoadState::kError;
		}

		// Done with compressed data, free it.
		InternalFreeCompressedData();

		// Must validate FxStudio bank streams before instantiating a SeoulEngine FxStudio::BankFile
		// object on the data.
		if (!BankFile::PopulateData(GetFilePath(), pUncompressedFileData, zUncompressedFileDataSizeInBytes, m_Data))
		{
			return Content::LoadState::kError;
		}

		return Content::LoadState::kLoadingOnMainThread;
	}
	else if (Content::LoadState::kLoadingOnMainThread == GetContentLoadState())
	{
		SharedPtr< Content::Entry<BankFile, FilePath> > pEntry(m_hEntry.GetContentEntry());
		if (!pEntry.IsValid())
		{
			return Content::LoadState::kError;
		}

		// If an entry already exists for the bank, swap the data
		// into it (this is a reload), otherwise create a new FxStudio::BankFile instance.
		SharedPtr< BankFile > pFxStudioBankFile(pEntry->GetPtr());
		if (!pFxStudioBankFile.IsValid())
		{
			pEntry->AtomicReplace(
				SharedPtr<BankFile>(SEOUL_NEW(MemoryBudgets::Fx) BankFile(
					Manager::Get()->GetFxStudioManager(),
					m_Data)));

			// The bank now owns the memory, nullptr out the loader's values so it does not free
			// it on destruction.
			m_Data.Clear();
		}
		else
		{
			// Swap will assign any existing memory to m_pData, and we'll destroy it on destruction.
			FilePath filePath = GetFilePath();
			pFxStudioBankFile->Swap(filePath, m_Data);

			// Tell the manager that a bank was reloaded.
			Manager::Get()->OnBankReloaded(GetFilePath());
		}

		InternalReleaseEntry();
		return Content::LoadState::kLoaded;
	}

	return Content::LoadState::kError;
}

/**
 * Frees loaded compressed data if non-null.
 */
void ContentLoader::InternalFreeCompressedData()
{
	if (nullptr != m_pCompressedFileData)
	{
		MemoryManager::Deallocate(m_pCompressedFileData);
		m_pCompressedFileData = nullptr;
	}
	m_zCompressedFileDataSizeInBytes = 0u;
}

/**
 * Release the loader's reference on its content entry - doing this as
 * soon as loading completes allows anything waiting for the load to react
 * as soon as possible.
 */
void ContentLoader::InternalReleaseEntry()
{
	if (m_hEntry.IsInternalPtrValid())
	{
		// NOTE: We need to release our reference before decrementing the loader count.
		// This is safe, because a Content::Entry's Content::Store always maintains 1 reference,
		// and does not release it until the content is done loading.
		Content::Handle<BankFile>::EntryType* pEntry = m_hEntry.GetContentEntry().GetPtr();
		m_hEntry.Reset();
		pEntry->DecrementLoaderCount();
	}
}

} // namespace Seoul::FxStudio

#endif // /#if SEOUL_WITH_FX_STUDIO
