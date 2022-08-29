/**
 * \file UIFontLoader.cpp
 * \brief Specialization of Content::LoaderBase for loading Falcon font data.
 *
 * UI::FontLoader reads cooked, compressed TTF font data and prepares
 * it for consumption by the Falcon UI system.
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
#include "FileManager.h"
#include "Path.h"
#include "UIFontLoader.h"
#include "UIManager.h"
#include "UIUtil.h"

namespace Seoul::UI
{

FontLoader::FontLoader(FilePath filePath, const Content::Handle<Falcon::CookedTrueTypeFontData>& hTrueTypeFontDataEntry)
	: Content::LoaderBase(filePath)
	, m_hTrueTypeFontDataEntry(hTrueTypeFontDataEntry)
	, m_pTotalFileData(nullptr)
	, m_zTotalDataSizeInBytes(0u)
{
	m_hTrueTypeFontDataEntry.GetContentEntry()->IncrementLoaderCount();
}

FontLoader::~FontLoader()
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

Content::LoadState FontLoader::InternalExecuteContentLoadOp()
{
	// Must be on the file IO thread to load the font data.
	if (Content::LoadState::kLoadingOnFileIOThread == GetContentLoadState())
	{
		// Cook the TTF if necessary.
		CookManager::Get()->CookIfOutOfDate(GetFilePath());

		// Read the data into a buffer - if this succeeds, cache the data and then
		// either wait for dependencies to load, or finish the load.
		if (FileManager::Get()->ReadAll(
			GetFilePath(),
			m_pTotalFileData,
			m_zTotalDataSizeInBytes,
			kLZ4MinimumAlignment,
			MemoryBudgets::FalconFont))
		{
			// Finish loading off the file IO thread.
			return Content::LoadState::kLoadingOnWorkerThread;
		}
	}
	// When we get here, we're finishing the load.
	else if (Content::LoadState::kLoadingOnWorkerThread == GetContentLoadState())
	{
		// Decompress.
		{
			void* p = nullptr;
			UInt32 u = 0u;
			if (!ZSTDDecompress(m_pTotalFileData, m_zTotalDataSizeInBytes, p, u, MemoryBudgets::FalconFont))
			{
				return Content::LoadState::kError;
			}
			MemoryManager::Deallocate(m_pTotalFileData);
			m_pTotalFileData = p;
			m_zTotalDataSizeInBytes = u;
		}

		// Create the data object.
		SharedPtr<Falcon::CookedTrueTypeFontData> p(SEOUL_NEW(MemoryBudgets::FalconFont) Falcon::CookedTrueTypeFontData(
			GetFilePath().GetRelativeFilenameWithoutExtension().ToHString(),
			m_pTotalFileData,
			m_zTotalDataSizeInBytes));

		// Set and finalize.
		m_hTrueTypeFontDataEntry.GetContentEntry()->AtomicReplace(p);

		m_pTotalFileData = nullptr;
		m_zTotalDataSizeInBytes = 0u;

		// Immediately release the entry so the font is considered loaded as soon as possible.
		InternalReleaseEntry();
		return Content::LoadState::kLoaded;
	}

	return Content::LoadState::kError;
}

/**
 * Release the loader's reference on its content entry - doing this as
 * soon as loading completes allows anything waiting for the load to react
 * as soon as possible.
 */
void FontLoader::InternalReleaseEntry()
{
	if (m_hTrueTypeFontDataEntry.IsInternalPtrValid())
	{
		// NOTE: We need to release our reference before decrementing the loader count.
		// This is safe, because a Content::Entry's Content::Store always maintains 1 reference,
		// and does not release it until the content is done loading.
		Content::Handle<Falcon::CookedTrueTypeFontData>::EntryType* pEntry = m_hTrueTypeFontDataEntry.GetContentEntry().GetPtr();
		m_hTrueTypeFontDataEntry.Reset();
		pEntry->DecrementLoaderCount();
	}
}

} // namespace Seoul::UI
