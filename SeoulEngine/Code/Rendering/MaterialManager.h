/**
 * \file MaterialManager.h
 * \brief Singleton manager for loading Materials and merging identical
 * materials into single material objects to improve render batching.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef MATERIAL_MANAGER_H
#define MATERIAL_MANAGER_H

#include "ContentStore.h"
#include "HashSet.h"
#include "Material.h"
#include "Singleton.h"

namespace Seoul
{

/**
 * Utility structure used internally by MaterialManager to
 * dedup Materials with identical configuration.
 */
class MergedMaterialEntry SEOUL_SEALED
{
public:
	MergedMaterialEntry()
		: m_pMaterial()
		, m_uHash(0u)
	{
	}

	MergedMaterialEntry(const MergedMaterialEntry& b)
		: m_pMaterial(b.m_pMaterial)
		, m_uHash(b.m_uHash)
	{
	}

	MergedMaterialEntry& operator=(const MergedMaterialEntry& b)
	{
		m_pMaterial = b.m_pMaterial;
		m_uHash = b.m_uHash;
		return *this;
	}

	explicit MergedMaterialEntry(const SharedPtr<Material>& pMaterial)
		: m_pMaterial(pMaterial)
		, m_uHash(pMaterial.IsValid() ? pMaterial->ComputeHash() : 0u)
	{
	}

	Bool operator==(const MergedMaterialEntry& b) const
	{
		return (
			(m_pMaterial == b.m_pMaterial) ||
			(m_pMaterial.IsValid() && b.m_pMaterial.IsValid() && (*m_pMaterial == *b.m_pMaterial)));
	}

	Bool operator!=(const MergedMaterialEntry& b) const
	{
		return !(*this == b);
	}

	UInt32 GetHash() const
	{
		return m_uHash;
	}

	const SharedPtr<Material>& GetMaterial() const
	{
		return m_pMaterial;
	}

private:
	SharedPtr<Material> m_pMaterial;
	UInt32 m_uHash;
}; // class MergedMaterialEntry

static inline UInt32 GetHash(const MergedMaterialEntry& entry)
{
	return entry.GetHash();
}

template <>
struct DefaultHashTableKeyTraits<MergedMaterialEntry>
{
	inline static Float GetLoadFactor()
	{
		return 0.75f;
	}

	inline static MergedMaterialEntry GetNullKey()
	{
		return MergedMaterialEntry();
	}

	static const Bool kCheckHashBeforeEquals = true;
};

/**
 * Singleton manager that caches and combines Materials that are identical.
 */
class MaterialManager SEOUL_SEALED : public Singleton<MaterialManager>
{
public:
	MaterialManager();
	~MaterialManager();

	void MergeMaterial(SharedPtr<Material>& rpMaterial);

private:
	typedef HashSet< MergedMaterialEntry, MemoryBudgets::Rendering > MergedMaterials;
	MergedMaterials m_MergedMaterials;
	Mutex m_Mutex;
}; // class MaterialManager

} // namespace Seoul

#endif // include guard
