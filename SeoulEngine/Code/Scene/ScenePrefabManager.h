/**
 * \file ScenePrefabManager.h
 * \brief PrefabManager is the singleton manager for persistent Prefab
 * that must be loaded from disk.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCENE_PREFAB_MANAGER_H
#define SCENE_PREFAB_MANAGER_H

#include "ContentStore.h"
#include "ScenePrefab.h"
#include "Singleton.h"

#if SEOUL_WITH_SCENE

namespace Seoul::Scene
{

class PrefabManager SEOUL_SEALED : public Singleton<PrefabManager>
{
public:
	PrefabManager();
	~PrefabManager();

	/**
	 * @return true if the prefab associated with FilePath can be saved -
	 * true if the Prefab is not actively loading.
	 */
	Bool CanSave(FilePath filePath)
	{
		if (!m_Content.IsFileLoaded(filePath))
		{
			return true;
		}

		return !m_Content.GetContent(filePath).IsLoading();
	}

	/**
	 * @return A persistent Content::Handle<> to the mesh filePath.
	 */
	PrefabContentHandle GetPrefab(FilePath filePath)
	{
		return m_Content.GetContent(filePath);
	}

	enum Result
	{
		/** Memory usage data is not available on the current platform. */
		kNoMemoryUsageAvailable,

		/**
		 * Not all textures expose memory usage, so the estimate is
		 * a low estimate of memory usage.
		 */
		kApproximateMemoryUsage,

		/**
		 * All textures returned memory usage data, so the
		 * returned value is the exact number of bytes occupied
		 * by textures on the current platform.
		 */
		kExactMemoryUsage
	};

	Result GetPrefabMemoryUsageInBytes(UInt32& rzMemoryUsageInBytes) const;

private:
	friend class PrefabContentLoader;
	Content::Store<Prefab> m_Content;

	SEOUL_DISABLE_COPY(PrefabManager);
}; // class PrefabManager

} // namespace Seoul::Scene

#endif // /#if SEOUL_WITH_SCENE

#endif // include guard
