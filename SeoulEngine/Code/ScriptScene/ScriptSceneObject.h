/**
 * \file ScriptSceneObject.h
 * \brief Binder instance for exposing a SceneObject
 * instance to script.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCRIPT_SCENE_OBJECT_H
#define SCRIPT_SCENE_OBJECT_H

#include "SharedPtr.h"
namespace Seoul { namespace Scene { class Object; } }
namespace Seoul { namespace Script { class FunctionInterface; } }
namespace Seoul { class String; }

#if SEOUL_WITH_SCENE

namespace Seoul
{

struct ScriptSceneObject SEOUL_SEALED
{
	ScriptSceneObject();
	~ScriptSceneObject();

	// Access to the object's Id.
	String GetId() const;

	// Given a relative Id, convert it to a full Id resolved relative to this SceneObject.
	String ResolveRelativeId(const String& sRelativeId);

	// Access to the object's transform.
	void GetRotation(Script::FunctionInterface* pInterface) const;
	void GetPosition(Script::FunctionInterface* pInterface) const;

	// Position and rotation mutations, specialized for script access.
	void SetLookAt(Float fTargetX, Float fTargetY, Float fTargetZ);
	void SetPosition(Float fX, Float fY, Float fZ);
	void SetRotation(Float fX, Float fY, Float fZ, Float fW);

	// Access to SceneFxComponent functionality.
	Float GetFxDuration() const;
	Bool StartFx();
	void StopFx(Script::FunctionInterface* pInterface);

	// Access to SceneMeshComponent functionality.
	void SetMeshVisible(Bool bVisible);

	// Script specific convenience.
	void TransformPosition(Script::FunctionInterface* pInterface) const;

	SharedPtr<Scene::Object> m_pSceneObject;
}; // struct ScriptSceneObject

} // namespace Seoul

#endif // /#if SEOUL_WITH_SCENE

#endif // include guard
