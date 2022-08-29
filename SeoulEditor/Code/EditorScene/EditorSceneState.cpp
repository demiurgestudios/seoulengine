/**
 * \file EditorSceneState.cpp
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

#include "EditorSceneState.h"
#include "Logger.h"
#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionUtil.h"
#include "SceneFreeTransformComponent.h"
#include "SceneObject.h"
#include "ScenePrefabComponent.h"
#include "ScenePrefabManager.h"

#if SEOUL_WITH_SCENE

namespace Seoul::EditorScene
{

/** Total number of cameras a State can contain. */
static const Int32 kiMaxCameras = 4;

State::State()
	: m_vComponentsScratch()
	, m_vObjects()
	, m_vAddQueue()
	, m_tPrefabAddCache()
{
}

State::~State()
{
	// On destruction, fail any remaining add jobs in the queue.
	while (!m_vAddQueue.IsEmpty())
	{
		AsyncAddPrefabEntry entry = m_vAddQueue.Front();
		m_vAddQueue.Erase(m_vAddQueue.Begin());
		if (entry.m_Callback)
		{
			entry.m_Callback(entry.m_sId, false);
		}
	}
}

/**
 * Asynchronously add a prefab to the root.
 *
 * Adds a prefab to the root. On completion, callback() will
 * invoked with instanceId and success or failure. Add can
 * fail if the prefab filePath is invalid, or the root
 * is destroyed before the prefab has a chance to load.
 *
 * @param[in] filePath Prefab to spawn.
 * @param[in] sId Identifier of the prefab to callback,
 *                and the namespace to apply of spawned objects.
 * @param[in] vPosition Translation to apply to spawned objects.
 * @param[in] qRotation Rotation to apply to spawned objects.
 * @param[in] callback Invoked on completion, success or failure.
 *            Guaranteed to be called "eventually" (prior to shutdown).
 */
void State::AsyncAddPrefab(
	FilePath filePath,
	const String& sId,
	const Vector3D& vPosition,
	const Quaternion& qRotation,
	const AsyncAddPrefabCallback& callback)
{
	AsyncAddPrefabEntry entry;
	entry.m_Callback = callback;
	entry.m_hScenePrefab = Scene::PrefabManager::Get()->GetPrefab(filePath);
	entry.m_sId = sId;
	entry.m_qRotation = qRotation;
	entry.m_vPosition = vPosition;
	AsyncAddPrefab(entry);
}

/** @return True with SceneObject with id or false if not found. */
Bool State::GetObjectById(const String& sId, SharedPtr<Scene::Object>& rpSceneObject) const
{
	// TODO: Profile once we have a scene of decent size and decide
	// if this should have a shadow table to make this O(1). My expectation
	// is that all accesses will go through script, so it may be better
	// to pre-emptively populate the script lookup tables instead of
	// maintaining a native lookup table also.

	const State::Objects& vObjects = m_vObjects;
	UInt32 const uObjects = vObjects.GetSize();
	for (UInt32 i = 0u; i < uObjects; ++i)
	{
		if (vObjects[i]->GetId() == sId)
		{
			rpSceneObject = vObjects[i];
			return true;
		}
	}

	return false;
}

/**
 * Processes the queue of prefabs to add asynchronously to a scene.
 *
 * Give the add-to-scene queue some time to perform add
 * operations. Time-sliced based on uTimeSliceInMilliseconds.
 */
void State::ProcessAddPrefabQueue(UInt32 uTimeSliceInMilliseconds /*= 1u*/)
{
	Int64 const iStartTimeInTicks = SeoulTime::GetGameTimeInTicks();
	do
	{
		if (m_vAddQueue.IsEmpty())
		{
			break;
		}

		// Want to add in the order they were queued, so break immediately
		// if the front is still loading.
		if (m_vAddQueue.Front().m_hScenePrefab.IsLoading())
		{
			break;
		}

		// Pop the front entry and process it.
		AsyncAddPrefabEntry const entry = m_vAddQueue.Front();
		m_vAddQueue.Erase(m_vAddQueue.Begin());

		SharedPtr<Scene::Prefab> pScenePrefab(entry.m_hScenePrefab.GetPtr());

		// Can't succeed if no scene data.
		Bool bSuccess = false;
		if (pScenePrefab.IsValid())
		{
			Matrix4D const mTransform = Matrix4D::CreateRotationTranslation(
				entry.m_qRotation,
				entry.m_vPosition);

			bSuccess = AppendScenePrefab(
				entry.m_hScenePrefab.GetKey(),
				pScenePrefab->GetTemplate(),
				mTransform,
				entry.m_sId);
		}

		// Report if we have a callback.
		if (entry.m_Callback)
		{
			entry.m_Callback(entry.m_sId, bSuccess);
		}

		// Finally, add to the cache on success.
		if (bSuccess)
		{
			// TODO: Probably want to remove these. Ideally, we'd remove this
			// once all objects spawned from a group have been removed from a scene
			// (the scene is no longer using the group at all).
			SEOUL_VERIFY(m_tPrefabAddCache.Overwrite(entry.m_hScenePrefab.GetKey(), entry.m_hScenePrefab).Second);
		}
	} while ((UInt32)SeoulTime::ConvertTicksToMilliseconds(SeoulTime::GetGameTimeInTicks() - iStartTimeInTicks) < uTimeSliceInMilliseconds) ;
}

/**
 * Instantiates and adds objects defined by t into this state.
 *
 * Main entry point for populating the list of objects in a root state.
 * Instances t and appends those SceneObject instances to this
 * State.
 */
Bool State::AppendScenePrefab(
	FilePath scenePrefabFilePath,
	const Scene::PrefabTemplate& t,
	const Matrix4D& mParentTransform,
	const String& sQualifier)
{
	// Track whether we need to apply the parent transform to
	// objects cloned for this prefab.
	Bool const bHasParentTransform = !Matrix4D::Identity().Equals(mParentTransform);

	// Setup variables to append the new objects to the existing list.
	UInt32 const uObjects = t.m_vObjects.GetSize();
	UInt32 const uPrefabs = t.m_vPrefabs.GetSize();
	UInt32 const uOutObjectBegin = m_vObjects.GetSize();
	m_vObjects.Reserve(uOutObjectBegin + uObjects + uPrefabs);

	// First, add any nested prefabs. For the editor, we just
	// add a new object, properly configured, with an PrefabComponent.
	for (UInt32 i = 0u; i < uPrefabs; ++i)
	{
		Scene::NestedPrefab const data = t.m_vPrefabs[i];

		// Construct the object.
		auto pObject = SEOUL_NEW(MemoryBudgets::SceneObject) Scene::Object(data.m_sId);
		pObject->SetEditorCategory(data.m_EditorCategory);

		// Add a free transform component for positioning.
		{
			pObject->AddComponent(SharedPtr<Scene::Component>(SEOUL_NEW(MemoryBudgets::SceneComponent) Scene::FreeTransformComponent));
			pObject->SetPosition(data.m_vPosition);
			pObject->SetRotation(data.m_qRotation);
		}

		// Add a prefab component.
		{
			auto pComponent(SEOUL_NEW(MemoryBudgets::SceneComponent) Scene::PrefabComponent);
			pComponent->SetPrefab(data.m_hPrefab);
			pObject->AddComponent(SharedPtr<Scene::Component>(pComponent));
		}

		// Add to set.
		m_vObjects.PushBack(SharedPtr<Scene::Object>(pObject));
	}

	// Iterate and clone.
	for (UInt32 i = 0u; i < uObjects; ++i)
	{
		// Clone the template to create a new instance.
		SharedPtr<Scene::Object> pObject(t.m_vObjects[i]->Clone(sQualifier));

		// Track post instantiate components for add later.
		for (auto const& pComponent : pObject->GetComponents())
		{
			if (pComponent->NeedsOnGroupInstantiateComplete())
			{
				m_vComponentsScratch.PushBack(pComponent);
			}
		}

		// If we have a parent transform, apply it now.
		if (bHasParentTransform)
		{
			// Compute the full world transform for the object.
			Matrix4D const mTransform =
				mParentTransform *
				Matrix4D::CreateRotationTranslation(
					pObject->GetRotation(),
					pObject->GetPosition());

			// No need to decompose here, since the scene graph
			// assumes (and enforces in tools) orthonormal transforms
			// up the stack.
			pObject->SetRotation(mTransform.GetRotation());
			pObject->SetPosition(mTransform.GetTranslation());
		}

		// Sort components for display purposes.
		pObject->EditorOnlySortComponents();

		// Add the object.
		m_vObjects.PushBack(pObject);
	}

	// Process post intantiate components.
	for (auto const& pComponent : m_vComponentsScratch)
	{
		pComponent->OnGroupInstantiateComplete(*this);
	}
	m_vComponentsScratch.Clear();

	return true;
}

} // namespace Seoul::EditorScene

#endif // /#if SEOUL_WITH_SCENE
