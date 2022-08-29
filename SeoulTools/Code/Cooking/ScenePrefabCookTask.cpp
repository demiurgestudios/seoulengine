/**
 * \file ScenePrefabCookTask.cpp
 * \brief Cooking tasks for cooking SeoulEngine .prefab files into runtime
 * SeoulEngine .spf files.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "BaseCookTask.h"
#include "Compress.h"
#include "CookPriority.h"
#include "FileManager.h"
#include "ICookContext.h"
#include "Logger.h"
#include "Prereqs.h"
#include "ReflectionDefine.h"
#include "ScopedAction.h"

namespace Seoul
{

namespace Cooking
{

class ScenePrefabCookTask SEOUL_SEALED : public BaseCookTask
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(ScenePrefabCookTask);

	ScenePrefabCookTask()
	{
	}

	virtual Bool CanCook(FilePath filePath) const SEOUL_OVERRIDE
	{
		return (filePath.GetType() == FileType::kScenePrefab);
	}

	virtual Bool CookAllOutOfDateContent(ICookContext& rContext) SEOUL_OVERRIDE
	{
		ContentFiles v;
		if (!DefaultOutOfDateCook(rContext, FileType::kScenePrefab, v, true))
		{
			return false;
		}

		return true;
	}

	virtual Int32 GetPriority() const SEOUL_OVERRIDE
	{
		return CookPriority::kScenePrefab;
	}

private:
	SEOUL_DISABLE_COPY(ScenePrefabCookTask);

	virtual Bool InternalCook(ICookContext& rContext, FilePath filePath) SEOUL_OVERRIDE
	{
		String s;
		if (!FileManager::Get()->ReadAll(filePath.GetAbsoluteFilenameInSource(), s))
		{
			return false;
		}

		DataStore dataStore;
		if (!DataStoreParser::FromString(s, dataStore, DataStoreParserFlags::kLogParseErrors, filePath))
		{
			SEOUL_LOG_COOKING("failed loading scene prefab JSON data into a DataStore.\n");
			return false;
		}

		{
			MemorySyncFile file;
			if (!dataStore.Save(file, rContext.GetPlatform()))
			{
				SEOUL_LOG_COOKING("failed serializing DataStore.\n");
				return false;
			}

			void* p = nullptr;
			UInt32 u = 0u;
			auto const deferred(MakeDeferredAction([&]()
				{
					MemoryManager::Deallocate(p);
					p = nullptr;
					u = 0u;
				}));

			if (!ZSTDCompress(
				file.GetBuffer().GetBuffer(),
				file.GetBuffer().GetTotalDataSizeInBytes(),
				p,
				u))
			{
				return false;
			}

			return AtomicWriteFinalOutput(rContext, p, u, filePath);
		}
	}
}; // class ScenePrefabCookTask

} // namespace Cooking

SEOUL_BEGIN_TYPE(Cooking::ScenePrefabCookTask, TypeFlags::kDisableCopy)
	SEOUL_PARENT(Cooking::BaseCookTask)
SEOUL_END_TYPE()

} // namespace Seoul
