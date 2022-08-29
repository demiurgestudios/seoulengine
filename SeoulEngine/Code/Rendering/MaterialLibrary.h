/**
 * \file MaterialLibrary.h
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

#pragma once
#ifndef MATERIAL_LIBRARY_H
#define MATERIAL_LIBRARY_H

#include "Asset.h"
#include "Material.h"

namespace Seoul
{

class MaterialLibrary SEOUL_SEALED
{
public:
	typedef Vector< SharedPtr<Material>, MemoryBudgets::Rendering> Materials;

	MaterialLibrary();
	~MaterialLibrary();

	const Materials& GetMaterials() const
	{
		return m_vMaterials;
	}

	/**
	 * Returns the amount of memory occupied by this
	 * mesh, assuming that its primitive groups (and their index buffers)
	 * and its vertex buffer are not shared.
	 */
	UInt32 GetMemoryUsageInBytes() const
	{
		return m_zGraphicsMemoryUsageInBytes;
	}

	Bool Load(FilePath filePath, SyncFile& rFile);

private:
	SEOUL_REFERENCE_COUNTED(MaterialLibrary);

	void InternalDestroy();

	Materials m_vMaterials;
	UInt32 m_zGraphicsMemoryUsageInBytes;

	SEOUL_DISABLE_COPY(MaterialLibrary);
}; // class MaterialLibrary

} // namespace Seoul

#endif // include guard
