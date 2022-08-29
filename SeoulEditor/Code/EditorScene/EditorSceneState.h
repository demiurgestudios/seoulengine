/**
 * \file EditorSceneState.h
 * \brief Internal structure used by Container.
 *
 * State encapsulates all parts of a Container
 * tied to a scene instance that may be loaded or initialized
 * by a StateLoadJob.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef EDITOR_SCENE_STATE_H
#define EDITOR_SCENE_STATE_H

#include "Delegate.h"
#include "EditorSceneEditState.h"
#include "FixedArray.h"
#include "SceneInterface.h"
#include "ScenePrefab.h"
#include "SeoulString.h"
#include "Vector.h"

#if SEOUL_WITH_SCENE

namespace Seoul::EditorScene
{

typedef Delegate<void(const String& sInstanceId, Bool bSuccess)> AsyncAddPrefabCallback;
struct AsyncAddPrefabEntry SEOUL_SEALED
{
	AsyncAddPrefabEntry()
		: m_hScenePrefab()
		, m_vPosition()
		, m_qRotation()
		, m_Callback()
		, m_sId()
	{
	}

	Scene::PrefabContentHandle m_hScenePrefab;
	Vector3D m_vPosition;
	Quaternion m_qRotation;
	AsyncAddPrefabCallback m_Callback;
	String m_sId;
}; // struct AsyncAddPrefabEntry
typedef Vector<AsyncAddPrefabEntry, MemoryBudgets::Scene> AsyncAddPrefabQueue;

class State SEOUL_SEALED : public Scene::Interface
{
public:
	typedef Vector<SharedPtr<Scene::Component>, MemoryBudgets::Rendering> Components;
	typedef HashTable<FilePath, Scene::PrefabContentHandle, MemoryBudgets::SceneObject> ScenePrefabHandleTable;

	State();
	~State();

	// Instantiates and adds objects defined by t into this state.
	Bool AppendScenePrefab(
		FilePath scenePrefabFilePath,
		const Scene::PrefabTemplate& t,
		const Matrix4D& mParentTransform,
		const String& sQualifier);

	// Add a sub scene instantiation to the queue.
	void AsyncAddPrefab(const AsyncAddPrefabEntry& entry)
	{
		m_vAddQueue.PushBack(entry);
	}

	// Queue a prefab for add to the root. Completes when the prefab has finished loading.
	void AsyncAddPrefab(
		FilePath filePath,
		const String& sId,
		const Vector3D& vPosition,
		const Quaternion& qRotation = Quaternion::Identity(),
		const AsyncAddPrefabCallback& callback = AsyncAddPrefabCallback());

	// Get the SceneObject with id or return false if not found.
	virtual Bool GetObjectById(const String& sId, SharedPtr<Scene::Object>& rpSceneObject) const SEOUL_OVERRIDE;

	/** @return The current tracked set of prefabs previously added to the scene, dynamically. */
	const ScenePrefabHandleTable& GetPrefabAddCache() const
	{
		return m_tPrefabAddCache;
	}

	/** @return The editor specific, global data associated with this State. */
	EditState& GetEditState() { return m_EditState; }
	const EditState& GetEditState() const { return m_EditState; }

	/** @return The full list of Objects in this State. */
	Objects& GetObjects() { return m_vObjects; }
	virtual const Objects& GetObjects() const SEOUL_OVERRIDE  { return m_vObjects; }

	/** @return The physics simulator of this scene - always nullptr in the editor. */
	virtual Physics::Simulator* GetPhysicsSimulator() const SEOUL_OVERRIDE
	{
		return nullptr;
	}

	// Processes the async sub scene add-to-scene queue.
	void ProcessAddPrefabQueue(UInt32 uTimeSliceInMilliseconds = 1u);

private:
	// We're an aggregate of StateLoadJob.
	friend class StateLoadJob;

	Components m_vComponentsScratch;
	EditState m_EditState;
	Objects m_vObjects;
	AsyncAddPrefabQueue m_vAddQueue;
	ScenePrefabHandleTable m_tPrefabAddCache; // TODO:

	SEOUL_DISABLE_COPY(State);
}; // class State

} // namespace Seoul::EditorScene

#endif // /#if SEOUL_WITH_SCENE

#endif // include guard
