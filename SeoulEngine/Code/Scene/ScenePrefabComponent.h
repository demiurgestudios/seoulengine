/**
 * \file ScenePrefabComponent.h
 * \brief Component that defines a nested Prefab within another Prefab.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCENE_PREFAB_COMPONENT_H
#define SCENE_PREFAB_COMPONENT_H

#include "Matrix4D.h"
#include "SceneComponent.h"
#include "SceneInterface.h"
#include "ScenePrefab.h"

#if SEOUL_WITH_SCENE

namespace Seoul::Scene
{

// TODO: Scene::Interface subclassing, and several other features
// of this Component, are Editor only. At runtime, this class is
// just a placeholder that is discarded on scene instantiation. Probably,
// the Editor subset of this class should be moved into an Editor only
// component that is created from an PrefabComponent by the Editor
// scene.

class PrefabComponent SEOUL_SEALED : public Component, public Scene::Interface
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(PrefabComponent);

	typedef Vector<SharedPtr<Scene::Object>, MemoryBudgets::SceneObject> Objects;

	PrefabComponent();
	~PrefabComponent();

#if SEOUL_HOT_LOADING
	void CheckHotLoad();
#endif

	virtual SharedPtr<Component> Clone(const String& sQualifier) const SEOUL_OVERRIDE;

	const PrefabContentHandle& GetPrefab() const
	{
		return m_hPrefab;
	}

	FilePath GetFilePath() const
	{
		return m_hPrefab.GetKey();
	}

	void SetPrefab(const PrefabContentHandle& hPrefab)
	{
		m_hPrefab = hPrefab;
#if SEOUL_HOT_LOADING
		m_LastLoadId = 0;
#endif // /#if SEOUL_HOT_LOADING
	}

	void SetFilePath(FilePath filePath);

	// Scene::Interface overrides.
	virtual const Objects& GetObjects() const SEOUL_OVERRIDE { return m_vObjects; }
	virtual Bool GetObjectById(const String& sId, SharedPtr<Object>& rpObject) const SEOUL_OVERRIDE;
	virtual Physics::Simulator* GetPhysicsSimulator() const SEOUL_OVERRIDE { return nullptr; }
	// /Scene::Interface overrides.

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(PrefabComponent);

	PrefabContentHandle m_hPrefab;
	Objects m_vObjects;

	void CreateObjects();

#if SEOUL_HOT_LOADING
	Atomic32Type m_LastLoadId;
#endif // /#if SEOUL_HOT_LOADING

	SEOUL_DISABLE_COPY(PrefabComponent);
}; // class PrefabComponent

} // namespace Seoul::Scene

#endif // /#if SEOUL_WITH_SCENE

#endif // include guard
