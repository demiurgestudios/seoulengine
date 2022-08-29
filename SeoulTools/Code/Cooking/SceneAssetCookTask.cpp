/**
 * \file SceneAssetCookTask.cpp
 * \brief Cooking tasks for cooking Autodesk .fbx files into runtime
 * SeoulEngine .ssa files.
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
#include "ICookContext.h"
#include "Logger.h"
#include "Prereqs.h"
#include "ReflectionDefine.h"
#include "ScopedAction.h"

namespace Seoul
{

namespace Cooking
{

Bool CookSceneAsset(
	Seoul::Platform ePlatform,
	Byte const* sInputFileName,
	void** ppOutputData,
	UInt32* pzOutputDataSizeInBytes);

class SceneAssetCookTask SEOUL_SEALED : public BaseCookTask
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(SceneAssetCookTask);

	SceneAssetCookTask()
	{
	}

	virtual Bool CanCook(FilePath filePath) const SEOUL_OVERRIDE
	{
		return (filePath.GetType() == FileType::kSceneAsset);
	}

	virtual Bool CookAllOutOfDateContent(ICookContext& rContext) SEOUL_OVERRIDE
	{
		ContentFiles v;
		if (!DefaultOutOfDateCook(rContext, FileType::kSceneAsset, v))
		{
			return false;
		}

		return true;
	}

	virtual Int32 GetPriority() const SEOUL_OVERRIDE
	{
		return CookPriority::kSceneAsset;
	}

private:
	SEOUL_DISABLE_COPY(SceneAssetCookTask);

	virtual Bool InternalCook(ICookContext& rContext, FilePath filePath) SEOUL_OVERRIDE
	{
		void* pC = nullptr;
		UInt32 uC = 0u;
		auto const deferredC(MakeDeferredAction([&]()
			{
				MemoryManager::Deallocate(pC);
				pC = nullptr;
				uC = 0u;
			}));

		{
			void* p = nullptr;
			UInt32 u = 0u;
			auto const deferredU(MakeDeferredAction([&]()
				{
					MemoryManager::Deallocate(p);
					p = nullptr;
					u = 0u;
				}));

			if (!CookSceneAsset(rContext.GetPlatform(), filePath.GetAbsoluteFilenameInSource().CStr(), &p, &u))
			{
				return false;
			}

			if (!ZSTDCompress(p, u, pC, uC, ZSTDCompressionLevel::kBest, MemoryBudgets::Cooking))
			{
				SEOUL_LOG_COOKING("%s: failed compressing asset data for asset cook.", filePath.CStr());
				return false;
			}
		}

		return AtomicWriteFinalOutput(rContext, pC, uC, filePath);
	}
}; // class SceneAssetCookTask

} // namespace Cooking

SEOUL_BEGIN_TYPE(Cooking::SceneAssetCookTask, TypeFlags::kDisableCopy)
	SEOUL_PARENT(Cooking::BaseCookTask)
SEOUL_END_TYPE()

} // namespace Seoul
