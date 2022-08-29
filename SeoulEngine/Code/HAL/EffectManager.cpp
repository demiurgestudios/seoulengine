/**
 * \file EffectManager.cpp
 * \brief Singleton manager for loading graphics effects and
 * guaranteeing that each loaded Effect is unique.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "EffectManager.h"
#include "GamePaths.h"
#include "JobsFunction.h"
#include "Path.h"

namespace Seoul
{

EffectManager::EffectManager()
	: m_Content()
{
}

EffectManager::~EffectManager()
{
}

struct EffectMemoryUsageCompute SEOUL_SEALED
{
	SEOUL_DELEGATE_TARGET(EffectMemoryUsageCompute);

	EffectMemoryUsageCompute()
		: m_zTotalInBytes(0u)
	{
	}

	Bool Apply(const EffectContentHandle& h)
	{
		SharedPtr<Effect> p(h.GetPtr());
		if (p.IsValid())
		{
			m_zTotalInBytes += p->GetGraphicsMemoryUsageInBytes();
		}

		// Return false to indicate "not handled", tells the Content::Store to
		// keep walking entries.
		return false;
	}

	UInt32 m_zTotalInBytes;
}; // class EffectMemoryUsageCompute

/**
 * Return the amount of graphics memory being used by active shader effects.
 */
UInt32 EffectManager::GetEffectGraphicsMemoryUsageInBytes() const
{
	EffectMemoryUsageCompute compute;
	m_Content.Apply(SEOUL_BIND_DELEGATE(&EffectMemoryUsageCompute::Apply, &compute));
	return compute.m_zTotalInBytes;
}

Bool EffectManagerUnsetAllTextures(const EffectContentHandle& h)
{
	SharedPtr<Effect> p(h.GetPtr());
	if (p.IsValid())
	{
		p->UnsetAllTextures();
	}

	// Return false to indicate "not handled", tells the Content::Store to
	// keep walking entries.
	return false;
}

/**
 * Before Content::LoadManager performs any unloads, we need
 * to unset textures from all active effects to prevent
 * dangling references on some platforms.
 */
void EffectManager::UnsetAllTextures()
{
	if (IsRenderThread())
	{
		m_Content.Apply(SEOUL_BIND_DELEGATE(EffectManagerUnsetAllTextures));
	}
	else
	{
		Jobs::AwaitFunction(
			GetRenderThreadId(),
			SEOUL_BIND_DELEGATE(&EffectManager::UnsetAllTextures, this));
	}
}

} // namespace Seoul
