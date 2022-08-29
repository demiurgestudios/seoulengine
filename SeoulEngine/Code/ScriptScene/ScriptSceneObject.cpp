/**
 * \file ScriptSceneObject.cpp
 * \brief Binder instance for exposing a SceneObject
 * instance to script.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ReflectionDefine.h"
#include "SceneFxComponent.h"
#include "SceneMeshDrawComponent.h"
#include "SceneObject.h"
#include "ScriptFunctionInterface.h"
#include "ScriptSceneObject.h"

#if SEOUL_WITH_SCENE

namespace Seoul
{

SEOUL_BEGIN_TYPE(ScriptSceneObject)
	SEOUL_METHOD(GetId)
	SEOUL_METHOD(ResolveRelativeId)
	SEOUL_METHOD(GetPosition)
	SEOUL_METHOD(GetRotation)
	SEOUL_METHOD(GetFxDuration)
	SEOUL_METHOD(SetLookAt)
	SEOUL_METHOD(SetPosition)
	SEOUL_METHOD(SetRotation)
	SEOUL_METHOD(StartFx)
	SEOUL_METHOD(StopFx)
	SEOUL_METHOD(SetMeshVisible)
	SEOUL_METHOD(TransformPosition)
SEOUL_END_TYPE()

ScriptSceneObject::ScriptSceneObject()
	: m_pSceneObject()
{
}

ScriptSceneObject::~ScriptSceneObject()
{
}

/** Binding of SceneObject::GetId() into script. */
String ScriptSceneObject::GetId() const
{
	return m_pSceneObject->GetId();
}

/** Given a relative Id, convert it to a full Id resolved relative to this SceneObject. */
String ScriptSceneObject::ResolveRelativeId(const String& sRelativeId)
{
	String sPath(m_pSceneObject->GetId());
	Scene::Object::RemoveLeafId(sPath);

	String sId(sRelativeId);
	Scene::Object::QualifyId(sPath, sId);
	return sId;
}

/** Binding of SceneObject::GetRotation() into script. */
void ScriptSceneObject::GetRotation(Script::FunctionInterface* pInterface) const
{
	Quaternion const qRotation(m_pSceneObject->GetRotation());
	pInterface->PushReturnNumber(qRotation.X);
	pInterface->PushReturnNumber(qRotation.Y);
	pInterface->PushReturnNumber(qRotation.Z);
	pInterface->PushReturnNumber(qRotation.W);
}

/** Binding of SceneObject::GetPosition() into script. */
void ScriptSceneObject::GetPosition(Script::FunctionInterface* pInterface) const
{
	Vector3D const vPosition(m_pSceneObject->GetPosition());
	pInterface->PushReturnNumber(vPosition.X);
	pInterface->PushReturnNumber(vPosition.Y);
	pInterface->PushReturnNumber(vPosition.Z);
}

/**
 * Get the total duration of this SceneObject's SceneFxComponent.
 *
 * Returns 0.0f if this SceneObject has no SceneFxComponent.
 */
Float ScriptSceneObject::GetFxDuration() const
{
	SharedPtr<Scene::FxComponent> pSceneFxComponent(m_pSceneObject->GetComponent<Scene::FxComponent>());
	if (pSceneFxComponent.IsValid())
	{
		return pSceneFxComponent->GetFxDuration();
	}

	return 0.0f;
}

/** Specialized SceneObject orientation update, look at the target at (X, Y, Z). */
void ScriptSceneObject::SetLookAt(Float fTargetX, Float fTargetY, Float fTargetZ)
{
	Vector3D const vPosition(m_pSceneObject->GetPosition());
	Vector3D const vDirection(Vector3D::Normalize(Vector3D(fTargetX, fTargetY, fTargetZ) - vPosition));
	Quaternion const qOrientation(Quaternion::CreateFromDirection(vDirection));
	m_pSceneObject->SetRotation(qOrientation);
}

/** Generic position update for script. */
void ScriptSceneObject::SetPosition(Float fX, Float fY, Float fZ)
{
	m_pSceneObject->SetPosition(Vector3D(fX, fY, fZ));
}

/** Generic rotation update for script. */
void ScriptSceneObject::SetRotation(Float fX, Float fY, Float fZ, Float fW)
{
	m_pSceneObject->SetRotation(Quaternion(fX, fY, fZ, fW));
}

/**
 * Start this ScriptSceneObject's Fx playing.
 *
 * Return true if the Fx was started, false otherwise.
 */
Bool ScriptSceneObject::StartFx()
{
	SharedPtr<Scene::FxComponent> pSceneFxComponent(m_pSceneObject->GetComponent<Scene::FxComponent>());
	if (pSceneFxComponent.IsValid())
	{
		return pSceneFxComponent->StartFx();
	}

	return false;
}

/** Stop this ScriptSceneObject's Fx, if it has one. */
void ScriptSceneObject::StopFx(Script::FunctionInterface* pInterface)
{
	// Get the SceneFxComponent - nop if we don't have one.
	SharedPtr<Scene::FxComponent> pSceneFxComponent(m_pSceneObject->GetComponent<Scene::FxComponent>());
	if (pSceneFxComponent.IsValid())
	{
		// Default is to not stop immediately.
		Bool bStopImmediately = false;

		// Get true/false bStopImmediately.
		if (!pInterface->IsNilOrNone(1u))
		{
			// If it's defined, must be a boolean.
			if (!pInterface->GetBoolean(1u, bStopImmediately))
			{
				pInterface->RaiseError(1, "expected optional boolean bStopImmediately");
				return;
			}
		}

		// Issue the stop.
		pSceneFxComponent->StopFx(bStopImmediately);
	}
}

/**
 * Update the visibility of this ScriptSceneObject's
 * SceneMeshDrawComponent, if it has one.
 */
void ScriptSceneObject::SetMeshVisible(Bool bVisible)
{
	SharedPtr<Scene::MeshDrawComponent> pSceneMeshDrawComponent(m_pSceneObject->GetComponent<Scene::MeshDrawComponent>());
	if (pSceneMeshDrawComponent.IsValid())
	{
		pSceneMeshDrawComponent->SetVisible(bVisible);
	}
}

/**
 * Script exclusive convenience utility.
 *
 * Given a 3D position vector as input (fX, fY, fZ),
 * transform that coordinate by this Object's
 * transform and return the result.
 */
void ScriptSceneObject::TransformPosition(Script::FunctionInterface* pInterface) const
{
	Float fX = 0.0f;
	Float fY = 0.0f;
	Float fZ = 0.0f;
	if (!pInterface->GetNumber(1u, fX) ||
		!pInterface->GetNumber(2u, fY) ||
		!pInterface->GetNumber(3u, fZ))
	{
		pInterface->RaiseError(1, "expected 3 component number position.");
		return;
	}

	Vector3D const v = Matrix4D::TransformPosition(
		m_pSceneObject->ComputeNormalTransform(),
		Vector3D(fX, fY, fZ));

	pInterface->PushReturnNumber(v.X);
	pInterface->PushReturnNumber(v.Y);
	pInterface->PushReturnNumber(v.Z);
}

} // namespace Seoul

#endif // /#if SEOUL_WITH_SCENE
