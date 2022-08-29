/**
 * \file MaterialManager.cpp
 * \brief Singleton manager for loading Materials and merging identical
 * materials into single material objects to improve render batching.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Material.h"
#include "MaterialManager.h"
#include "SeoulFileReaders.h"

namespace Seoul
{

MaterialManager::MaterialManager()
{
}

MaterialManager::~MaterialManager()
{
}

void MaterialManager::MergeMaterial(SharedPtr<Material>& rpMaterial)
{
	if (!rpMaterial.IsValid())
	{
		return;
	}

	MergedMaterialEntry newEntry(rpMaterial);

	Lock lock(m_Mutex);
	Pair< MergedMaterials::Iterator, Bool> res = m_MergedMaterials.Insert(newEntry);
	if (!res.Second)
	{
		rpMaterial = res.First->GetMaterial();
	}
}

} // namespace Seoul
