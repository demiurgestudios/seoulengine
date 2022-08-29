/**
 * \file SettingsContentLoader.cpp
 * \brief Handles loading of DataStores from JSON files
 * into SettingsManager.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ContentLoadManager.h"
#include "DataStoreParser.h"
#include "SettingsContentLoader.h"
#include "SettingsManager.h"

namespace Seoul
{

namespace
{

/**
 * Wraps file loading and tracks dependencies via ["$include", ...] commands
 * when hot loading is enabled.
 */
struct IncludeHelper SEOUL_SEALED
{
	SEOUL_DELEGATE_TARGET(IncludeHelper);

	IncludeHelper(FilePath baseFilePath)
		: m_BaseFilePath(baseFilePath)
	{
	}

	// Track the include dependency when hot loading is enabled.
#if SEOUL_HOT_LOADING
	void AddDependency(FilePath dependencyFilePath)
	{
		if (!m_vDependencies.ContainsFromBack(dependencyFilePath))
		{
			m_vDependencies.PushBack(dependencyFilePath);
		}
	}

	// Commit tracked include dependencies to the Content::LoadManager for hot loading.
	void CommitDependencies()
	{
		Content::LoadManager::Get()->SetDependencies(m_BaseFilePath, m_vDependencies);
	}
#endif // #if SEOUL_HOT_LOADING

	// Resolve the file needed by an ["$include"] directive.
	SharedPtr<DataStore> CommandFileIncludeResolver(const String& sFileName, Bool bResolveCommands)
	{
		auto const filePath(FilePath::CreateConfigFilePath(sFileName));

		// Track for hot loading.
#if SEOUL_HOT_LOADING
		AddDependency(filePath);
#endif

		// If the resolved command file should be fully resolved, then we
		// can just load it (possibly cached) from SettingsManager.
		if (bResolveCommands)
		{
			return SettingsManager::Get()->WaitForSettings(filePath);
		}
		// Otherwise, we just acquire the file directly and leave it in commands
		// format, presumably to be applied inline to an in-progress DataStore.
		else
		{
			// TODO: Should add caching to this, however, in practice, the only
			// files that need to be loaded in this way are multiple $include cases, which
			// are only ever loaded via this mechanism, so caching doesn't help.
		
			// Load the DataStore, leave as a command file, if it is in that format.
			DataStore dataStore;
			if (!DataStoreParser::FromFile(filePath, dataStore, DataStoreParserFlags::kLogParseErrors))
			{
				return SharedPtr<DataStore>();
			}

			SharedPtr<DataStore> pDataStore(SEOUL_NEW(MemoryBudgets::DataStore) DataStore);
			pDataStore->Swap(dataStore);
			return pDataStore;
		}
	}

	FilePath const m_BaseFilePath;
	Content::LoadManager::DepVector m_vDependencies;
};

} // namespace anonymous

/** Entry point for synchronous load, special case for WaitOnContent() cases. */
Content::LoadState SettingsContentLoader::SyncLoad(FilePath filePath, const SettingsContentHandle& hEntry)
{
	SharedPtr<DataStore> pDataStore;
	{
		DataStoreSchemaCache* pCache = nullptr;
#if !SEOUL_SHIP
		pCache = SettingsManager::Get()->GetSchemaCache();
#endif // /#if !SEOUL_SHIP

		DataStore dataStore;
		if (!DataStoreParser::FromFile(pCache, filePath, dataStore, DataStoreParserFlags::kLogParseErrors))
		{
			return Content::LoadState::kError;
		}

		pDataStore.Reset(SEOUL_NEW(MemoryBudgets::DataStore) DataStore);
		pDataStore->Swap(dataStore);
	}

	IncludeHelper helper(filePath);

	// A JSON command file is an outer array, each entry is an array,
	// and the first value of each inner array is a known command (currently,
	// "$append", "$set", "$erase", "$include", and "$object".
	if (DataStoreParser::IsJsonCommandFile(*pDataStore))
	{
		// Resolve the command file and replace the DataStore with the resolved content.
		if (!DataStoreParser::ResolveCommandFile(
			SEOUL_BIND_DELEGATE(&IncludeHelper::CommandFileIncludeResolver, &helper),
			filePath.GetAbsoluteFilename(), 
			*pDataStore,
			*pDataStore,
			DataStoreParserFlags::kLogParseErrors))
		{
			return Content::LoadState::kError;
		}
	}

	// If we're tracking for hot loading, commit those values now.
#if SEOUL_HOT_LOADING
	helper.CommitDependencies();
#endif // /#if SEOUL_HOT_LOADING

	SharedPtr< Content::Entry<DataStore, FilePath> > pEntry(hEntry.GetContentEntry());
	if (!pEntry.IsValid())
	{
		return Content::LoadState::kError;
	}

	pEntry->AtomicReplace(pDataStore);
	pDataStore.Reset();

	return Content::LoadState::kLoaded;
}

SettingsContentLoader::SettingsContentLoader(
	FilePath filePath,
	const SettingsContentHandle& hEntry)
	: Content::LoaderBase(filePath)
	, m_hEntry(hEntry)
{
	m_hEntry.GetContentEntry()->IncrementLoaderCount();
}

SettingsContentLoader::~SettingsContentLoader()
{
	// Block until this Content::Loaded is in a non-loading state.
	WaitUntilContentIsNotLoading();

	// NOTE: We need to release our reference before decrementing the loader count.
	// This is safe, because a Content::Entry's Content::Store always maintains 1 reference,
	// and does not release it until the content is done loading.
	SettingsContentHandle::EntryType* pEntry = m_hEntry.GetContentEntry().GetPtr();
	m_hEntry.Reset();

	pEntry->DecrementLoaderCount();
}

Content::LoadState SettingsContentLoader::InternalExecuteContentLoadOp()
{
	if (Content::LoadState::kLoadingOnFileIOThread != GetContentLoadState())
	{
		return Content::LoadState::kError;
	}

	return SyncLoad(GetFilePath(), m_hEntry);
}

} // namespace Seoul
