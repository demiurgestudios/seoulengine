/**
 * \file Settings.cpp
 * \brief Specialization of Content::Traits<> for DataStore, so it can be used
 * in SettingsManager.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ContentLoadManager.h"
#include "Settings.h"
#include "SettingsContentLoader.h"

namespace Seoul
{

SharedPtr<DataStore> Content::Traits<DataStore>::GetPlaceholder(FilePath filePath)
{
	return SharedPtr<DataStore>();
}

Bool Content::Traits<DataStore>::FileChange(FilePath filePath, const SettingsContentHandle& pEntry)
{
	// Only react to FileChange events if the filePath is an json file.
	if (FileType::kJson == filePath.GetType())
	{
		Load(filePath, pEntry);
		return true;
	}

	return false;
}

void Content::Traits<DataStore>::Load(FilePath filePath, const SettingsContentHandle& pEntry)
{
	// Only trigger a load if the filePath is an json file - otherwise, the entry
	// must be populated another way (i.e. SettingsManager::ParseSettings().
	if (FileType::kJson == filePath.GetType())
	{
		Content::LoadManager::Get()->Queue(
			SharedPtr<Content::LoaderBase>(SEOUL_NEW(MemoryBudgets::Content) SettingsContentLoader(filePath, pEntry)));
	}
}

Bool Content::Traits<DataStore>::PrepareDelete(FilePath filePath, Content::Entry<DataStore, KeyType>& entry)
{
	return true;
}

void Content::Traits<DataStore>::SyncLoad(FilePath filePath, const Content::Handle<DataStore>& hEntry)
{
	(void)SettingsContentLoader::SyncLoad(filePath, hEntry);
}

/** @return The memory usage of the DataStore referenced by p. */
UInt32 Content::Traits<DataStore>::GetMemoryUsage(const SharedPtr<DataStore>& p)
{
	if (!p.IsValid())
	{
		return 0u;
	}

	return p->GetTotalMemoryUsageInBytes();
}

} // namespace Seoul
