/**
 * \file ScriptSceneStateBinder.cpp
 * \brief Middleman instance that handles communication between
 * the script VM and the internal state of a scriptable scene.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Camera.h"
#include "Logger.h"
#include "ReflectionDefine.h"
#include "ReflectionUtil.h"
#include "SceneObject.h"
#include "ScriptEngineCamera.h"
#include "ScriptFunctionInterface.h"
#include "ScriptFunctionInvoker.h"
#include "ScriptSceneState.h"
#include "ScriptSceneStateBinder.h"
#include "ScriptSceneObject.h"
#include "ScriptVm.h"

#if SEOUL_WITH_SCENE

namespace Seoul
{

SEOUL_BEGIN_TYPE(ScriptSceneStateBinder)
	SEOUL_METHOD(AsyncAddPrefab)
	SEOUL_METHOD(GetCamera)
	SEOUL_METHOD(GetObjectById)
	SEOUL_METHOD(GetObjectIdFromHandle)
	SEOUL_METHOD(SetScriptInterface)
SEOUL_END_TYPE()

struct ScriptAsyncAddPrefabCallback SEOUL_SEALED
{
	ScriptAsyncAddPrefabCallback(const SharedPtr<Script::VmObject>& pCallback)
		: m_pCallback(pCallback)
	{
	}

	~ScriptAsyncAddPrefabCallback()
	{
	}

	static void Apply(void* pUserData, const String& sId, Bool bSuccess)
	{
		SEOUL_ASSERT(nullptr != pUserData);
		ScriptAsyncAddPrefabCallback* pScriptCallback = ((ScriptAsyncAddPrefabCallback*)pUserData);
		Script::FunctionInvoker invoker(pScriptCallback->m_pCallback);
		if (invoker.IsValid())
		{
			invoker.PushString(sId);
			invoker.PushBoolean(bSuccess);
			(void)invoker.TryInvoke();
		}

		SafeDelete(pScriptCallback);
	}

	SharedPtr<Script::VmObject> m_pCallback;
}; // struct ScriptAsyncAddPrefabCallback

ScriptSceneStateBinder::ScriptSceneStateBinder()
{
}

void ScriptSceneStateBinder::Construct(const ScriptSceneStateHandle& hScriptSceneState)
{
	m_hScriptSceneState = hScriptSceneState;
}

ScriptSceneStateBinder::~ScriptSceneStateBinder()
{
}

/**
 * Add a nested ObjectSceneGroup, non-blocking.
 *
 * This function loads the specified ObjectSceneGroup data, and if successful,
 * spawns it into the scene with given transform. On success or failure,
 * a provided callback will be invoked.
 */
void ScriptSceneStateBinder::AsyncAddPrefab(Script::FunctionInterface* pInterface) const
{
	CheckedPtr<ScriptSceneState> pScriptSceneState(GetPtr(m_hScriptSceneState));
	if (!pScriptSceneState.IsValid())
	{
		return;
	}

	// Required argument - file path to sub scene.
	FilePath filePath;
	if (!pInterface->GetFilePath(1u, filePath))
	{
		pInterface->RaiseError(1, "expected sub scene FilePath.");
		return;
	}

	// Required argument - prefab spawn id.
	String sId;
	if (!pInterface->GetString(2u, sId))
	{
		pInterface->RaiseError(2, "expected sub scene id.");
		return;
	}

	// Required argument - fX, fY, fZ position.
	Vector3D vPosition;
	if (!pInterface->GetNumber(3u, vPosition.X) ||
		!pInterface->GetNumber(4u, vPosition.Y) ||
		!pInterface->GetNumber(5u, vPosition.Z))
	{
		pInterface->RaiseError(3, "expected 3 numbers as 3 component sub scene position.");
		return;
	}

	// Remaining arguments are optional - rotation and callback.
	Quaternion qRotation = Quaternion::Identity();
	SharedPtr<Script::VmObject> pCallback;
	Int iArgument = 6;
	if (!pInterface->IsNilOrNone(iArgument))
	{
		// Number means rotation is expected in the next 4 arguments.
		if (pInterface->IsNumberExact(iArgument))
		{
			// Number means the rotation is now required.
			if (!pInterface->GetNumber(iArgument + 0, qRotation.X) ||
				!pInterface->GetNumber(iArgument + 1, qRotation.Y) ||
				!pInterface->GetNumber(iArgument + 2, qRotation.Z) ||
				!pInterface->GetNumber(iArgument + 3, qRotation.W))
			{
				pInterface->RaiseError(iArgument, "expected 4 numbers as 4 component sub scene rotation.");
				return;
			}

			iArgument += 4;
		}

		// Function means callback.
		if (pInterface->IsFunction(iArgument))
		{
			if (!pInterface->GetFunction(iArgument, pCallback))
			{
				pInterface->RaiseError(iArgument, "expected callback function.");
			}

			iArgument++;
		}
	}

	// Now, if needed, generate the callback wrapper.
	AsyncAddPrefabCallback callback;
	if (pCallback.IsValid())
	{
		ScriptAsyncAddPrefabCallback* pScriptCallback = SEOUL_NEW(MemoryBudgets::Scripting) ScriptAsyncAddPrefabCallback(pCallback);
		callback = SEOUL_BIND_DELEGATE(&ScriptAsyncAddPrefabCallback::Apply, (void*)pScriptCallback);
	}

	// Finally, call the root sceen.
	pScriptSceneState->AsyncAddPrefab(
		filePath,
		sId,
		vPosition,
		qRotation,
		callback);
}

/**
 * Acquire a scene Camera by index.
 *
 * Scenes support multiple cameras, identified by index.
 *
 * Enabled cameras are rendered based on their projection and transform settings
 */
void ScriptSceneStateBinder::GetCamera(Script::FunctionInterface* pInterface) const
{
	CheckedPtr<ScriptSceneState> pScriptSceneState(GetPtr(m_hScriptSceneState));
	if (!pScriptSceneState.IsValid())
	{
		pInterface->PushReturnNil();
		return;
	}

	Int32 iCameraIndex = 0;
	if (!pInterface->GetInteger(1u, iCameraIndex))
	{
		pInterface->RaiseError(1, "expected integer camera index.");
		return;
	}

	SharedPtr<Camera> pCamera;
	if (!pScriptSceneState->GetOrCreateCamera(iCameraIndex, pCamera))
	{
		pInterface->PushReturnNil();
		return;
	}

	ScriptEngineCamera* pScriptEngineCamera = pInterface->PushReturnUserData<ScriptEngineCamera>();
	pScriptEngineCamera->Construct(pCamera);
}

/**
 * Lookup and return the native user data for an Object based
 * on fully qualified id.
 */
void ScriptSceneStateBinder::GetObjectById(Script::FunctionInterface* pInterface) const
{
	CheckedPtr<ScriptSceneState> pScriptSceneState(GetPtr(m_hScriptSceneState));
	if (!pScriptSceneState.IsValid())
	{
		pInterface->PushReturnNil();
		return;
	}

	String sId;
	if (!pInterface->GetString(1u, sId))
	{
		pInterface->RaiseError(1, "expected string id.");
		return;
	}

	SharedPtr<Scene::Object> pObject;
	if (!pScriptSceneState->GetObjectById(sId, pObject))
	{
		pInterface->PushReturnNil();
		return;
	}

	ScriptSceneObject* pBinder = pInterface->PushReturnUserData<ScriptSceneObject>();
	if (nullptr == pBinder)
	{
		pInterface->RaiseError(-1, "failed allocating script binding, programmer error.");
		return;
	}

	pBinder->m_pSceneObject = pObject;
	return;
}

/** Get an object's id given a light user data handle of the object. */
void ScriptSceneStateBinder::GetObjectIdFromHandle(Script::FunctionInterface* pInterface) const
{
	// Get the light user data handle value.
	void* pHandle = nullptr;
	if (!pInterface->GetLightUserData(1, pHandle))
	{
		pInterface->RaiseError(1, "expected handle light user data.");
		return;
	}

	// Resolve to its object.
	auto const hObject = SceneObjectHandle::ToHandle(pHandle);
	auto const pObject(GetPtr(hObject));

	// If not valid (possible if the object has been destroyed),
	// return nil.
	if (!pObject.IsValid())
	{
		pInterface->PushReturnNil();
		return;
	}
	// Otherwise, push the object's identifier.
	else
	{
		pInterface->PushReturnString(pObject->GetId());
		return;
	}
}

/**
 * Commit the script table that will be used for native <-> script
 * interactions to the current scene. Should be caleld only once
 * by the script Scene module.
 */
void ScriptSceneStateBinder::SetScriptInterface(Script::FunctionInterface* pInterface)
{
	CheckedPtr<ScriptSceneState> pScriptSceneState(GetPtr(m_hScriptSceneState));
	if (!pScriptSceneState.IsValid())
	{
		return;
	}

	SharedPtr<Script::VmObject> pScriptInterface;
	if (!pInterface->GetObject(1u, pScriptInterface))
	{
		pInterface->RaiseError(1, "expected table as script interface.");
		return;
	}

	pScriptSceneState->SetScriptInterface(pScriptInterface);
}

/** @return this Binder's instance as a ptr, may be nullptr if the scene state has been released. */
CheckedPtr<ScriptSceneState> ScriptSceneStateBinder::GetSceneStatePtr() const
{
	return GetPtr(m_hScriptSceneState);
}

} // namespace Seoul

#endif // /#if SEOUL_WITH_SCENE
