/**
 * \file ScenePrefabManager.cpp
 * \brief PrefabManager is the singleton manager for persistent Prefab
 * that must be loaded from disk.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ScenePrefab.h"
#include "ScenePrefabManager.h"

#if SEOUL_WITH_SCENE

namespace Seoul::Scene
{

PrefabManager::PrefabManager()
	: m_Content()
{
	SEOUL_ASSERT_DEBUG(IsMainThread());
}

PrefabManager::~PrefabManager()
{
	SEOUL_ASSERT_DEBUG(IsMainThread());
}

struct PrefabMemoryUsageCompute
{
	SEOUL_DELEGATE_TARGET(PrefabMemoryUsageCompute);

	PrefabMemoryUsageCompute()
		: m_zTotalInBytes(0u)
		, m_bOneResult(false)
		, m_bAllResults(true)
	{
	}

	Bool Apply(const PrefabContentHandle& h)
	{
		SharedPtr<Prefab> p(h.GetPtr());
		if (p.IsValid())
		{
			// If this succeeds, at the memory usage to the total
			// and set the flag that we have at least one
			// valid texture's memory usage.
			UInt32 zUsage = p->GetMemoryUsageInBytes();
			m_bOneResult = true;
			m_zTotalInBytes += zUsage;
		}

		// Return false to indicate "not handled", tells the Content::Store to
		// keep walking entries.
		return false;
	}

	UInt32 m_zTotalInBytes;
	Bool m_bOneResult;
	Bool m_bAllResults;
};

/**
 * @return kNoMemoryUsageAvailable if memory usage is not available
 * for meshes, kApproximateMemoryUsage is memory usage does not
 * necessarily reflect all meshes, or kExactMemoryUsage if
 * memory usage equals the exact number of bytes that is being
 * used by mesh data.
 *
 * If this method returns kApproximateMemoryUsage or
 * kExactMemoryUsage, rzMemoryUsageInBytes will contain the
 * associated value. Otherwise, rzMemoryUsageInBytes is
 * left unchanged.
 */
PrefabManager::Result PrefabManager::GetPrefabMemoryUsageInBytes(UInt32& rzMemoryUsageInBytes) const
{
	PrefabMemoryUsageCompute compute;
	m_Content.Apply(SEOUL_BIND_DELEGATE(&PrefabMemoryUsageCompute::Apply, &compute));

	// In either case, we have a valid or approximate memory usage
	if (compute.m_bOneResult || compute.m_bAllResults)
	{
		// Set the memory usage and report the correct results.
		rzMemoryUsageInBytes = compute.m_zTotalInBytes;
		return (compute.m_bAllResults)
			? kExactMemoryUsage
			: kApproximateMemoryUsage;
	}
	else
	{
		return kNoMemoryUsageAvailable;
	}
}

} // namespace Seoul::Scene

#endif // /#if SEOUL_WITH_SCENE
