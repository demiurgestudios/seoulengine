/**
 * \file MaterialLibrary.cpp
 * \brief A MaterialLibrary is a collection of materials. It can be a
 * standalone asset, to be dynamically used for geometries at runtime,
 * or as part of a Mesh or AnimatedMesh.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "MaterialLibrary.h"
#include "MaterialManager.h"
#include "ReflectionDefine.h"
#include "SeoulFileReaders.h"

namespace Seoul
{

MaterialLibrary::MaterialLibrary()
	: m_vMaterials()
	, m_zGraphicsMemoryUsageInBytes(0u)
{
}

MaterialLibrary::~MaterialLibrary()
{
}

Bool MaterialLibrary::Load(FilePath filePath, SyncFile& rFile)
{
	Materials vMaterials;
	UInt32 uMaterialParameters = 0u;
	SharedPtr<Material> pMaterial(SEOUL_NEW(MemoryBudgets::Rendering) Material);

	// Verify the material library delimiter
	if (!VerifyDelimiter(DataTypeMaterialLibrary, rFile))
	{
		goto error;
	}

	// Read the materials count.
	if (!ReadUInt32(rFile, uMaterialParameters))
	{
		goto error;
	}

	// Read the materials.
	vMaterials.Reserve(uMaterialParameters);
	for (UInt32 i = 0u; i < uMaterialParameters; ++i)
	{
		pMaterial.Reset(SEOUL_NEW(MemoryBudgets::Rendering) Material);
		if (!pMaterial->Load(rFile))
		{
			goto error;
		}

		MaterialManager::Get()->MergeMaterial(pMaterial);

		vMaterials.PushBack(pMaterial);
	}

	m_zGraphicsMemoryUsageInBytes = vMaterials.GetSizeInBytes();
	m_vMaterials.Swap(vMaterials);
	return true;

error:
	return false;
}

void MaterialLibrary::InternalDestroy()
{
	m_zGraphicsMemoryUsageInBytes = 0u;
	m_vMaterials.Clear();
}

} // namespace Seoul
