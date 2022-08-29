/**
 * \file ScenePrefabContentLoader.h
 * \brief Specialization of Content::LoaderBase for loading Prefab.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCENE_PREFAB_CONTENT_LOADER_H
#define SCENE_PREFAB_CONTENT_LOADER_H

#include "ContentLoaderBase.h"
#include "ContentKey.h"
#include "ScenePrefab.h"

#if SEOUL_WITH_SCENE

namespace Seoul::Scene
{

/**
 * Specialization of Content::LoaderBase for loading Prefab.
 */
class PrefabContentLoader SEOUL_SEALED : public Content::LoaderBase
{
public:
	PrefabContentLoader(FilePath filePath, const PrefabContentHandle& hEntry);
	virtual ~PrefabContentLoader();

protected:
	virtual Content::LoadState InternalExecuteContentLoadOp() SEOUL_OVERRIDE;

private:
	void InternalFreePrefabData();
	void InternalReleaseEntry();

	PrefabContentHandle m_hEntry;
	void* m_pRawPrefabFileData;
	UInt32 m_zFileSizeInBytes;
	Bool m_bNetworkPrefetched;
}; // class PrefabContentLoader

} // namespace Seoul::Scene

#endif // /#if SEOUL_WITH_SCENE

#endif // include guard
