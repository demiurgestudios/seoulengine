/**
 * \file ScriptSceneState.h
 * \brief Internal structure used by ScriptScene.
 *
 * ScriptSceneState encapsulates all parts of a ScriptScene
 * tied to a scene instance that may be loaded or initialized
 * by a ScriptSceneLoadJob.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCRIPT_SCENE_STATE_H
#define SCRIPT_SCENE_STATE_H

#include "Delegate.h"
#include "FixedArray.h"
#include "SceneInterface.h"
#include "ScenePrefab.h"
#include "ScriptSceneStateHandle.h"
#include "ScriptSceneTicker.h"
#include "SeoulString.h"
#include "Vector.h"
namespace Seoul { class Camera; }
namespace Seoul { namespace Scene { class Component; } }
namespace Seoul { namespace Scene { class Object; } }
namespace Seoul { namespace Script { class Vm; } }
namespace Seoul { namespace Script { class VmObject; } }

#if SEOUL_WITH_SCENE

namespace Seoul
{

typedef Delegate<void(const String& sInstanceId, Bool bSuccess)> AsyncAddPrefabCallback;
struct AsyncAddPrefabEntry SEOUL_SEALED
{
	AsyncAddPrefabEntry()
		: m_hPrefab()
		, m_vPosition()
		, m_qRotation()
		, m_Callback()
		, m_sId()
	{
	}

	Scene::PrefabContentHandle m_hPrefab;
	Vector3D m_vPosition;
	Quaternion m_qRotation;
	AsyncAddPrefabCallback m_Callback;
	String m_sId;
}; // struct AsyncAddPrefabEntry
typedef Vector<AsyncAddPrefabEntry, MemoryBudgets::Scene> AsyncAddPrefabQueue;

class ScriptSceneState SEOUL_SEALED : public Scene::Interface
{
public:
	typedef HashTable<FilePath, Scene::PrefabContentHandle, MemoryBudgets::SceneObject> ScenePrefabHandleTable;
	typedef Vector<SharedPtr<Camera>, MemoryBudgets::Rendering> Cameras;
	typedef Vector<SharedPtr<Scene::Component>, MemoryBudgets::Rendering> Components;
	typedef Vector<SharedPtr<Scene::Object>, MemoryBudgets::SceneObject> Objects;

	ScriptSceneState();
	~ScriptSceneState();

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

	// Entry points for native -> script communication.
	void CallScriptOnAdd();
	void CallScriptOnLoad();
	void CallScriptSendEvent(
		const Reflection::MethodArguments& aArguments,
		Int iArgumentCount);
	void CallScriptTick(Float fDeltaTimeInSeconds);

	// When defined, equivalent to Camera::ConvertScreenSpaceToWorldSpace() for the scene.
	virtual Bool ConvertScreenSpaceToWorldSpace(const Vector3D& vScreenSpace, Vector3D& rvOut) const SEOUL_OVERRIDE;

	// Get the SceneObject with id or return false if not found.
	virtual Bool GetObjectById(const String& sId, SharedPtr<Scene::Object>& rpSceneObject) const SEOUL_OVERRIDE;

	// Output the ith Camera of this Scene, or create one
	// (disabled by default). Return false
	// if iCamera < 0 or iCamera > kiMaxSceneCameras.
	Bool GetOrCreateCamera(Int32 iCamera, SharedPtr<Camera>& rpCamera);

	/** @return The current tracked set of prefabs previously added to the scene, dynamically. */
	const ScenePrefabHandleTable& GetPrefabAddCache() const
	{
		return m_tPrefabAddCache;
	}

	/** @return The list of Cameras currently setup in this state. */
	const Cameras& GetCameras() const
	{
		return m_vCameras;
	}

	/**
	 * @return Indirect handle reference to this ScriptSceneState.
	 */
	const ScriptSceneStateHandle& GetHandle() const
	{
		return m_hThis;
	}

	/** @return The full list of Objects in this ScriptSceneState. */
	virtual const Objects& GetObjects() const SEOUL_OVERRIDE
	{
		return m_vObjects;
	}

	/** @return The physics simulator of this scene. */
	virtual Physics::Simulator* GetPhysicsSimulator() const SEOUL_OVERRIDE
	{
		return m_pPhysicsSimulator.Get();
	}

	/** @return The Script::Vm that drives this ScriptSceneState. */
	const SharedPtr<Script::Vm>& GetVm() const
	{
		return m_pVm;
	}

	/** Associate an ticker with this scene state. */
	void InsertTicker(ScriptSceneTicker& rTicker)
	{
		rTicker.InsertInList(m_TickerList);
	}

	// Processes the async sub scene add-to-scene queue.
	void ProcessAddPrefabQueue(UInt32 uTimeSliceInMilliseconds = 1u);

	// Call tick on any registered tickers.
	void ProcessTickers(Float fDeltaTimeInSeconds);

	// Step the physics simulator.
	void StepPhysics(Float fDeltaTimeInSeconds);

private:
	// We're an aggregate of ScriptSceneStateLoadJob.
	friend class ScriptSceneStateLoadJob;

	// Called by ScriptSceneStateLoadJob after the script VM is ready,
	// to bind self as a native interface into the VM.
	Bool BindSelfIntoScriptVm();

	// Script gets special access, specifically to
	// call SetScriptInterface().
	friend struct ScriptSceneStateBinder;

	// Update the native -> script interface.
	void SetScriptInterface(const SharedPtr<Script::VmObject>& pInterface);

	Components m_vComponentsScratch;
	ScriptSceneStateHandle m_hThis;
	ScopedPtr<Physics::Simulator> m_pPhysicsSimulator;
	SharedPtr<Script::VmObject> m_pScriptInterface;
	Scene::PrefabContentHandle m_hRootScenePrefab;
	SharedPtr<Script::Vm> m_pVm;
	Objects m_vObjects;
	Cameras m_vCameras;
	AsyncAddPrefabQueue m_vAddQueue;
	ScenePrefabHandleTable m_tPrefabAddCache;
	ScriptSceneTickerList m_TickerList;

	SEOUL_DISABLE_COPY(ScriptSceneState);
}; // struct ScriptSceneState

} // namespace Seoul

#endif // /#if SEOUL_WITH_SCENE

#endif // include guard
