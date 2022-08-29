/**
 * \file ScenePrefab.h
 * \brief A Prefab contains loadable object and component data for
 * representing parts of a 3D scene.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCENE_PREFAB_H
#define SCENE_PREFAB_H

#include "ContentHandle.h"
#include "DataStore.h"
#include "FilePath.h"
#include "Quaternion.h"
#include "ScopedPtr.h"
#include "Vector.h"
namespace Seoul { namespace Scene { class Object; } }
namespace Seoul { namespace Scene { struct PrefabTemplate; } }
namespace Seoul { class SyncFile; }

#if SEOUL_WITH_SCENE

namespace Seoul
{

namespace Scene
{

class Prefab SEOUL_SEALED
{
public:
	Prefab();
	~Prefab();

	// Return true if any referenced prefab is still loading.
	Bool AreNestedPrefabsLoading() const;

	// Total memory usage of the scene data in bytes.
	UInt32 GetMemoryUsageInBytes() const;

	/** @return The template from which a scene can be instanced. */
	const PrefabTemplate& GetTemplate() const
	{
		return *m_pTemplate;
	}

	// Populate this Prefab from persistent data.
	Bool Load(FilePath sceneFilePath, SyncFile& rFile);

private:
	SEOUL_REFERENCE_COUNTED(Prefab);

	static Bool LoadObjects(
		FilePath sceneFilePath,
		PrefabTemplate& rTemplate);

	ScopedPtr<PrefabTemplate> m_pTemplate;

	SEOUL_DISABLE_COPY(Prefab);
}; // class Prefab
typedef Content::Handle<Prefab> PrefabContentHandle;

} // namespace Scene

namespace Content
{

/**
 * Specialization of Content::Traits<> for Prefab, allows Prefab to be managed
 * as loadable content in the content system.
 */
template <>
struct Traits<Scene::Prefab>
{
	typedef FilePath KeyType;
	static const Bool CanSyncLoad = false;

	static SharedPtr<Scene::Prefab> GetPlaceholder(FilePath filePath);
	static Bool FileChange(FilePath filePath, const Scene::PrefabContentHandle& hEntry);
	static void Load(FilePath filePath, const Scene::PrefabContentHandle& hEntry);
	static Bool PrepareDelete(FilePath filePath, Entry<Scene::Prefab, KeyType>& entry);
	static void SyncLoad(FilePath filePath, const Handle<Scene::Prefab>& hEntry) {}
	static UInt32 GetMemoryUsage(const SharedPtr<Scene::Prefab>& p) { return p->GetMemoryUsageInBytes(); }
}; // Content::Traits<Prefab>

} // namespace Content

namespace Scene
{

/** Reference to a prefab from within a prefab, used for object instancing. */
struct NestedPrefab SEOUL_SEALED
{
	NestedPrefab()
		: m_hPrefab()
		, m_qRotation(Quaternion::Identity())
		, m_vPosition(Vector3D::Zero())
		, m_sId()
#if SEOUL_EDITOR_AND_TOOLS
		, m_EditorCategory()
#endif // /#if SEOUL_EDITOR_AND_TOOLS
	{
	}

	PrefabContentHandle m_hPrefab;
	Quaternion m_qRotation;
	Vector3D m_vPosition;
	String m_sId;
#if SEOUL_EDITOR_AND_TOOLS
	HString m_EditorCategory;
#endif // /#if SEOUL_EDITOR_AND_TOOLS
}; // struct NestedPrefab

/** All data necessary to instance a scene. */
struct PrefabTemplate SEOUL_SEALED
{
	typedef Vector<SharedPtr<Object>, MemoryBudgets::SceneObject> Objects;
	typedef Vector<NestedPrefab, MemoryBudgets::Scene> Prefabs;

	DataStore m_Data;
	Objects m_vObjects;
	Prefabs m_vPrefabs;

	PrefabTemplate();
	~PrefabTemplate();

	// Swap r for 'this'.
	void Swap(PrefabTemplate& r);

private:
	SEOUL_DISABLE_COPY(PrefabTemplate);
}; // class PrefabTemplate

} // namespace Scene

} // namespace Seoul

#endif // /#if SEOUL_WITH_SCENE

#endif // include guard
