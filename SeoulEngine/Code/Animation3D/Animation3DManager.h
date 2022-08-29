/**
 * \file Animation3DManager.h
 * \brief Global singleton that manages animation and network data in
 * the SeoulEngine content system.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ANIMATION3D_MANAGER_H
#define ANIMATION3D_MANAGER_H

#include "AnimationNetworkDefinition.h"
#include "Animation3DDataDefinition.h"
#include "ContentStore.h"
#include "Delegate.h"
#include "Singleton.h"
namespace Seoul { namespace Animation { class EventInterface; } }
namespace Seoul { namespace Animation3D { class NetworkInstance; } }

#if SEOUL_WITH_ANIMATION_3D

namespace Seoul::Animation3D
{

class Manager SEOUL_SEALED : public Singleton<Manager>
{
public:
	typedef Delegate<void(HString name)> EventCallback;
	typedef Vector<Matrix3x4, MemoryBudgets::Rendering> InverseBindPoses;
	typedef Vector<SharedPtr<Animation3D::NetworkInstance>, MemoryBudgets::Animation3D> Instances;

	Manager();
	~Manager();

	/** @return A new network instance. In development builds, instances are tracked for debugging purposes. */
	SharedPtr<Animation3D::NetworkInstance> CreateInstance(
		const AnimationNetworkContentHandle& hNetwork,
		const Animation3DDataContentHandle& hData,
		const SharedPtr<Animation::EventInterface>& pEventInterface,
		const InverseBindPoses& vInverseBindPoses);
	SharedPtr<Animation3D::NetworkInstance> CreateInstance(
		FilePath networkFilePath,
		FilePath dataFilePath,
		const SharedPtr<Animation::EventInterface>& pEventInterface,
		const InverseBindPoses& vInverseBindPoses);

	/**
	 * @return A persistent Content::Handle<> to the data filePath.
	 */
	Animation3DDataContentHandle GetData(FilePath filePath)
	{
		return m_DataContent.GetContent(filePath);
	}

	// Get a copy of the current list of network instances.
	void GetActiveNetworkInstances(Instances& rvInstances) const;

	// Per-frame maintenance.
	void Tick(Float fDeltaTimeInSeconds);

private:
	friend class DataContentLoader;
	Content::Store<DataDefinition> m_DataContent;

#if !SEOUL_SHIP
	Instances m_vInstances;
	Mutex m_Mutex;
#endif // /#if !SEOUL_SHIP

	SEOUL_DISABLE_COPY(Manager);
}; // class Manager

} // namespace Seoul::Animation3D

#endif // /#if SEOUL_WITH_ANIMATION_3D

#endif // include guard
