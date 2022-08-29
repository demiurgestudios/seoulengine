/**
 * \file AssetManager.cpp
 * \brief AssetManager is the singleton manager for persistent assets
 * that must be loaded from disk.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "AssetManager.h"
#include "Mesh.h"

namespace Seoul
{

AssetManager::AssetManager()
	: m_Content()
{
	SEOUL_ASSERT_DEBUG(IsMainThread());
}

AssetManager::~AssetManager()
{
	SEOUL_ASSERT_DEBUG(IsMainThread());
}

struct AssetMemoryUsageCompute
{
	SEOUL_DELEGATE_TARGET(AssetMemoryUsageCompute);

	AssetMemoryUsageCompute()
		: m_zTotalInBytes(0u)
		, m_bOneResult(false)
		, m_bAllResults(true)
	{
	}

	Bool Apply(const AssetContentHandle& h)
	{
		SharedPtr<Asset> p(h.GetPtr());
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
 * for assets, kApproximateMemoryUsage is memory usage does not
 * necessarily reflect all assets, or kExactMemoryUsage if
 * memory usage equals the exact number of bytes that is being
 * used by asset data.
 *
 * If this method returns kApproximateMemoryUsage or
 * kExactMemoryUsage, rzMemoryUsageInBytes will contain the
 * associated value. Otherwise, rzMemoryUsageInBytes is
 * left unchanged.
 */
AssetManager::Result AssetManager::GetAssetMemoryUsageInBytes(UInt32& rzMemoryUsageInBytes) const
{
	AssetMemoryUsageCompute compute;
	m_Content.Apply(SEOUL_BIND_DELEGATE(&AssetMemoryUsageCompute::Apply, &compute));

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

} // namespace Seoul
