/**
 * \file Animation3DManager.cpp
 * \brief Global singleton that manages animation and network data in
 * the SeoulEngine content system.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "AnimationNetworkDefinitionManager.h"
#include "Animation3DClipDefinition.h"
#include "Animation3DData.h"
#include "Animation3DManager.h"
#include "Animation3DNetworkInstance.h"
#include "Animation3DState.h"

#if SEOUL_WITH_ANIMATION_3D

namespace Seoul::Animation3D
{

Manager::Manager()
	: m_DataContent()
#if !SEOUL_SHIP
	, m_vInstances()
	, m_Mutex()
#endif // /#if !SEOUL_SHIP
{
}

Manager::~Manager()
{
	// TODO:
#if !SEOUL_SHIP
	Lock lock(m_Mutex);
	m_vInstances.Clear();
#endif

	SEOUL_VERIFY(m_DataContent.Clear());
}

/** Get a copy of the current list of network instances. */
void Manager::GetActiveNetworkInstances(Instances& rvInstances) const
{
#if !SEOUL_SHIP
	Lock lock(m_Mutex);
	rvInstances = m_vInstances;
#endif // /#if !SEOUL_SHIP
}

/** @return A new network instance. In development builds, instances are tracked for debugging purposes. */
SharedPtr<Animation3D::NetworkInstance> Manager::CreateInstance(
	const AnimationNetworkContentHandle& hNetwork,
	const Animation3DDataContentHandle& hData,
	const SharedPtr<Animation::EventInterface>& pEventInterface,
	const InverseBindPoses& vInverseBindPoses)
{
	auto pData(SEOUL_NEW(MemoryBudgets::Animation3D) Data(hData));
	SharedPtr<Animation3D::NetworkInstance> pReturn(SEOUL_NEW(MemoryBudgets::Animation3D) NetworkInstance(
		hNetwork,
		pData,
		pEventInterface,
		vInverseBindPoses));

	// Track the instance in developer builds.
#if !SEOUL_SHIP
	{
		Lock lock(m_Mutex);
		m_vInstances.PushBack(pReturn);
	}
#endif // /#if !SEOUL_SHIP

	return pReturn;
}

/** @return A new network instance. In development builds, instances are tracked for debugging purposes. */
SharedPtr<Animation3D::NetworkInstance> Manager::CreateInstance(
	FilePath networkFilePath,
	FilePath dataFilePath,
	const SharedPtr<Animation::EventInterface>& pEventInterface,
	const InverseBindPoses& vInverseBindPoses)
{
	auto pData(SEOUL_NEW(MemoryBudgets::Animation3D) Data(GetData(dataFilePath)));
	SharedPtr<Animation3D::NetworkInstance> pReturn(SEOUL_NEW(MemoryBudgets::Animation3D) NetworkInstance(
		Animation::NetworkDefinitionManager::Get()->GetNetwork(networkFilePath),
		pData,
		pEventInterface,
		vInverseBindPoses));

	// Track the instance in developer builds.
#if !SEOUL_SHIP
	{
		Lock lock(m_Mutex);
		m_vInstances.PushBack(pReturn);
	}
#endif // /#if !SEOUL_SHIP

	return pReturn;
}

/** Per-frame maintenance. */
void Manager::Tick(Float fDeltaTimeInSeconds)
{
	// Prune stale instances.
#if !SEOUL_SHIP
	{
		Lock lock(m_Mutex);
		Int32 iCount = (Int32)m_vInstances.GetSize();
		for (Int32 i = 0; i < iCount; ++i)
		{
			if (m_vInstances[i].IsUnique())
			{
				Swap(m_vInstances[i], m_vInstances[iCount-1]);
				--iCount;
				--i;
			}
		}
		m_vInstances.Resize((UInt32)iCount);
	}
#endif // /#if !SEOUL_SHIP
}

} // namespace Seoul::Animation3D

#endif // /#if SEOUL_WITH_ANIMATION_3D
