/**
 * \file ScriptSceneState.cpp
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

#include "Camera.h"
#include "Logger.h"
#include "PhysicsSimulator.h"
#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionUtil.h"
#include "RenderDevice.h"
#include "SceneObject.h"
#include "ScenePrefabManager.h"
#include "SceneScriptComponent.h"
#include "ScriptFunctionInvoker.h"
#include "ScriptSceneObject.h"
#include "ScriptSceneState.h"
#include "ScriptSceneStateBinder.h"
#include "ScriptVm.h"

#if SEOUL_WITH_SCENE

namespace Seoul
{

/** Total number of cameras a ScriptSceneState can contain. */
static const Int32 kiMaxCameras = 4;

/** Native -> script entry points. */
static const HString kFunctionAddScriptComponent("AddScriptComponent");
static const HString kFunctionOnAdd("OnAdd");
static const HString kFunctionOnLoad("OnLoad");
static const HString kFunctionPhysicsSensors("PhysicsSensors");
static const HString kFunctionSendEvent("SendEvent");
static const HString kFunctionTick("Tick");

/** Name of the global user data set to the Script VM to attach to its owner ScriptSceneState. */
static const HString kScriptGlobalNameSceneState("g_udNativeScene");

ScriptSceneState::ScriptSceneState()
	: m_vComponentsScratch()
	, m_hThis()
	, m_pPhysicsSimulator(SEOUL_NEW(MemoryBudgets::Physics) Physics::Simulator)
	, m_hRootScenePrefab()
	, m_pVm()
	, m_vObjects()
	, m_vCameras()
	, m_vAddQueue()
	, m_tPrefabAddCache()
{
	// Allocate a handle for this.
	m_hThis = ScriptSceneStateHandleTable::Allocate(this);

	// Create a default camera.
	SharedPtr<Camera> pUnused;
	(void)GetOrCreateCamera(0, pUnused);
}

ScriptSceneState::~ScriptSceneState()
{
	// On destruction, remove all tickers from our list.
	while (!m_TickerList.IsEmpty())
	{
		m_TickerList.GetHead()->RemoveFromList();
	}

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

	// Free our handle.
	ScriptSceneStateHandleTable::Free(m_hThis);
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
void ScriptSceneState::AsyncAddPrefab(
	FilePath filePath,
	const String& sId,
	const Vector3D& vPosition,
	const Quaternion& qRotation,
	const AsyncAddPrefabCallback& callback)
{
	AsyncAddPrefabEntry entry;
	entry.m_Callback = callback;
	entry.m_hPrefab = Scene::PrefabManager::Get()->GetPrefab(filePath);
	entry.m_sId = sId;
	entry.m_qRotation = qRotation;
	entry.m_vPosition = vPosition;
	AsyncAddPrefab(entry);
}

/**
 * Entry point for native -> script, calls tScene.OnAdd, if defined.
 *
 * OnAdd() is the post launch variation of OnLoad().
 */
void ScriptSceneState::CallScriptOnAdd()
{
	{
		Script::FunctionInvoker invoker(m_pScriptInterface, kFunctionOnAdd);
		if (invoker.IsValid())
		{
			(void)invoker.TryInvoke();
		}
	}
}

/** Entry point for native -> script, calls tScene.OnLoad, if defined. */
void ScriptSceneState::CallScriptOnLoad()
{
	{
		Script::FunctionInvoker invoker(m_pScriptInterface, kFunctionOnLoad);
		if (invoker.IsValid())
		{
			(void)invoker.TryInvoke();
		}
	}
}

/** Entry point for native -> script, calls tScene.SendEvent, if defined. */
void ScriptSceneState::CallScriptSendEvent(
	const Reflection::MethodArguments& aArguments,
	Int iArgumentCount)
{
	{
		Script::FunctionInvoker invoker(m_pScriptInterface, kFunctionSendEvent);
		if (invoker.IsValid())
		{
			for (Int i = 0; i < iArgumentCount; ++i)
			{
				invoker.PushAny(aArguments[i]);
			}
			(void)invoker.TryInvoke();
		}
	}
}

/** Entry point for native -> script, calls tScene.Tick, if defined. */
void ScriptSceneState::CallScriptTick(Float fDeltaTimeInSeconds)
{
	{
		Script::FunctionInvoker invoker(m_pScriptInterface, kFunctionTick);
		if (invoker.IsValid())
		{
			invoker.PushNumber(fDeltaTimeInSeconds);
			(void)invoker.TryInvoke();
		}
	}
}

/** When defined, equivalent to Camera::ConvertScreenSpaceToWorldSpace() for the scene. */
Bool ScriptSceneState::ConvertScreenSpaceToWorldSpace(
	const Vector3D& vScreenSpace,
	Vector3D& rvOut) const
{
	if (m_vCameras.IsEmpty())
	{
		return false;
	}

	// TODO: Quite possibly not the right viewport.
	auto const viewport = RenderDevice::Get()->GetBackBufferViewport();

	rvOut = m_vCameras.Front()->ConvertScreenSpaceToWorldSpace(viewport, vScreenSpace);
	return true;
}

/** @return True with SceneObject with id or false if not found. */
Bool ScriptSceneState::GetObjectById(const String& sId, SharedPtr<Scene::Object>& rpSceneObject) const
{
	// TODO: Profile once we have a scene of decent size and decide
	// if this should have a shadow table to make this O(1). My expectation
	// is that all accesses will go through script, so it may be better
	// to pre-emptively populate the script lookup tables instead of
	// maintaining a native lookup table also.

	const ScriptSceneState::Objects& vObjects = m_vObjects;
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
 * Output the ith Camera of this Scene, or create one
 * (disabled by default). Return false
 * if iCamera < 0 or iCamera > kiMaxSceneCameras.
 */
Bool ScriptSceneState::GetOrCreateCamera(Int32 iCamera, SharedPtr<Camera>& rpCamera)
{
	if (iCamera < 0 || iCamera > kiMaxCameras)
	{
		return false;
	}

	UInt32 const uCamera = (UInt32)iCamera;
	while (m_vCameras.GetSize() <= uCamera)
	{
		SharedPtr<Camera> pCamera(SEOUL_NEW(MemoryBudgets::Rendering) Camera);
		m_vCameras.PushBack(pCamera);
	}

	rpCamera = m_vCameras[uCamera];
	return true;
}

/**
 * Processes the async add prefab to scene queue.
 *
 * Give the add-to-scene queue some time to perform add
 * operations. Time-sliced based on uTimeSliceInMilliseconds.
 */
void ScriptSceneState::ProcessAddPrefabQueue(UInt32 uTimeSliceInMilliseconds /*= 1u*/)
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
		if (m_vAddQueue.Front().m_hPrefab.IsLoading())
		{
			break;
		}

		// Pop the front entry and process it.
		AsyncAddPrefabEntry const entry = m_vAddQueue.Front();
		m_vAddQueue.Erase(m_vAddQueue.Begin());

		SharedPtr<Scene::Prefab> pScenePrefab(entry.m_hPrefab.GetPtr());

		// Can't succeed if no scene data.
		Bool bSuccess = false;
		if (pScenePrefab.IsValid())
		{
			Matrix4D const mTransform = Matrix4D::CreateRotationTranslation(
				entry.m_qRotation,
				entry.m_vPosition);

			bSuccess = AppendScenePrefab(
				entry.m_hPrefab.GetKey(),
				pScenePrefab->GetTemplate(),
				mTransform,
				entry.m_sId);

			// If success, call into OnAdd().
			if (bSuccess)
			{
				CallScriptOnAdd();
			}
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
			SEOUL_VERIFY(m_tPrefabAddCache.Overwrite(entry.m_hPrefab.GetKey(), entry.m_hPrefab).Second);
		}
	} while ((UInt32)SeoulTime::ConvertTicksToMilliseconds(SeoulTime::GetGameTimeInTicks() - iStartTimeInTicks) < uTimeSliceInMilliseconds) ;
}

/** Call tick on any registered tickers. */
void ScriptSceneState::ProcessTickers(Float fDeltaTimeInSeconds)
{
	// Iterate and tick. Special handling around Tick() in case
	// it removes itself from the list.
	for (auto t = m_TickerList.GetHead(); t.IsValid(); )
	{
		// Advance t to the next entry first.
		auto p = t;
		t = t->GetNext();

		// Now tick.
		p->Tick(*this, fDeltaTimeInSeconds);
	}
}

/* Step the physics simulator. */
void ScriptSceneState::StepPhysics(Float fDeltaTimeInSeconds)
{
	m_pPhysicsSimulator->Step(fDeltaTimeInSeconds);

	// Check for sensor events - pass the list along to script.
	auto const& vEvents = m_pPhysicsSimulator->GetSensorEvents();
	if (!vEvents.IsEmpty())
	{
		Script::FunctionInvoker invoker(m_pScriptInterface, kFunctionPhysicsSensors);
		if (invoker.IsValid())
		{
			for (auto const& e : vEvents)
			{
				invoker.PushBoolean(Physics::ContactEvent::kSensorEnter == e.m_eEvent);
				invoker.PushLightUserData(e.m_pSensor);
				invoker.PushLightUserData(e.m_pBody);
			}

			(void)invoker.TryInvoke();
		}
	}
}

/**
 * Instantiates and adds objects defined by t into this state.
 *
 * Main entry point for populating the list of objects in a root state.
 * Instances t and appends those SceneObject instances to this
 * ScriptSceneState.
 */
Bool ScriptSceneState::AppendScenePrefab(
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
	UInt32 const uOutObjectBegin = m_vObjects.GetSize();
	m_vObjects.Reserve(uOutObjectBegin + uObjects);

	SharedPtr<Script::VmObject> pBinding;
	ScriptSceneObject* pBoundObject = nullptr;

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

		// Handle script component linkage.
		auto pScriptComponent = pObject->GetComponent<Scene::ScriptComponent>();
		if (pScriptComponent.IsValid())
		{
			if (!m_pScriptInterface.IsValid())
			{
				SEOUL_WARN("%s: ScriptSceneState contains a script component but "
					"has no interface through which to create them.",
					scenePrefabFilePath.CStr());
			}
			else
			{
				// Binding fails, skip this component.
				if (!m_pVm->BindStrongInstance(pBinding, pBoundObject))
				{
					SEOUL_WARN("%s: ScriptComponent of object %s failed binding.",
						scenePrefabFilePath.CStr(),
						pObject->GetId().CStr());
					continue;
				}
				else
				{
					// Setup the bound object.
					pBoundObject->m_pSceneObject = pObject;

					// All invocations have a class name, and owner binding, and an owner id..
					Reflection::MethodArguments aArguments;

					// TODO: Messy - need to sort out a better way of specifying scripts.
					// We use a FilePath to enable asset drag and drop in the editor.

					aArguments[0] = Path::GetFileNameWithoutExtension(pScriptComponent->GetScriptFilePath().GetRelativeFilename());
					aArguments[1] = pBinding;
					aArguments[2] = pBoundObject->GetId();

					// The total arguments will be 3 or 4, depending on whether
					// we need to generate an in-place table with serialized
					// properties.
					Int32 iArguments = 3;

					// We have a settings file path.
					if (pScriptComponent->GetSettingsFilePath().IsValid())
					{
						aArguments[3] = pScriptComponent->GetSettingsFilePath();
						iArguments = 4;
					}
					// Else, no settings - 3 arguments.

					// Let the script interface add the script component to the
					// script shadow of the native scene.
					{
						Script::FunctionInvoker invoker(m_pScriptInterface, kFunctionAddScriptComponent);
						for (Int32 i = 0; i < iArguments; ++i)
						{
							invoker.PushAny(aArguments[i]);
						}
						(void)invoker.TryInvoke();
					}
				}
			}
		}

		// Add the object.
		m_vObjects.PushBack(pObject);
	}

	// Process post intantiate components.
	for (auto const& pComponent : m_vComponentsScratch)
	{
		pComponent->OnGroupInstantiateComplete(*this);
	}
	m_vComponentsScratch.Clear();

	// Finally, instantiate any nested prefabs.
	UInt32 const uPrefabs = t.m_vPrefabs.GetSize();
	for (UInt32 i = 0u; i < uPrefabs; ++i)
	{
		// Parent scene should have enforced complete loading of all dependent scenes,
		// so an invalid pointer here indicates an invalid nested scene.
		Scene::NestedPrefab const data = t.m_vPrefabs[i];
		SharedPtr<Scene::Prefab> pPrefabData(data.m_hPrefab.GetPtr());
		if (!pPrefabData.IsValid())
		{
			SEOUL_WARN("%s: failed loading nested Prefab %s.",
				scenePrefabFilePath.CStr(),
				data.m_hPrefab.GetKey().CStr());
			continue;
		}

		// Qualify the prefab's spawn ID.
		String sPrefabId(data.m_sId);
		Scene::Object::QualifyId(sQualifier, sPrefabId);

		// Now append the nested prefab.
		if (!AppendScenePrefab(
			data.m_hPrefab.GetKey(),
			pPrefabData->GetTemplate(),
			mParentTransform * Matrix4D::CreateRotationTranslation(data.m_qRotation, data.m_vPosition),
			sPrefabId))
		{
			SEOUL_WARN("%s: failed appending nested scene prefab %s objects to root.",
				scenePrefabFilePath.CStr(),
				data.m_hPrefab.GetKey().CStr());
			continue;
		}
	}

	return true;
}

/**
 * Called by ScriptSceneStateLoadJob after the script VM is ready,
 * to bind self as a native interface into the VM.
 *
 * Internal/coupled use by ScriptSceneStateLoadJob only.
 */
Bool ScriptSceneState::BindSelfIntoScriptVm()
{
	ScriptSceneStateBinder* p = nullptr;
	SharedPtr<Script::VmObject> pBinding;
	if (!m_pVm->BindStrongInstance<ScriptSceneStateBinder>(pBinding, p))
	{
		SEOUL_WARN("Failure binding ScriptSceneStateBinder into script, "
			"programmer error. Check for stripped reflection definition.");
		return false;
	}
	SEOUL_ASSERT(nullptr != p);

	// Set m_hThis.
	p->Construct(m_hThis);

	// Commit the binder to the global script table.
	if (!m_pVm->TrySetGlobal(kScriptGlobalNameSceneState, pBinding))
	{
		SEOUL_WARN("Failure binding ScriptSceneStateBinder into script, "
			"programmer error, could not set \"%s\" to the global table.",
			kScriptGlobalNameSceneState.CStr());
		return false;
	}

	return true;
}

/**
 * Update the native -> script interface.
 *
 * pInterface will be used to handle all native -> script
 * invocations (such as Tick, SendEvent and OnLoad).
 */
void ScriptSceneState::SetScriptInterface(const SharedPtr<Script::VmObject>& pInterface)
{
	m_pScriptInterface = pInterface;
}

} // namespace Seoul

#endif // /#if SEOUL_WITH_SCENE
