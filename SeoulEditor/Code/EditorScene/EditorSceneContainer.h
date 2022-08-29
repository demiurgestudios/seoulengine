/**
 * \file EditorSceneContainer.h
 * \brief A Scene container (tree of Scene::Prefabs).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef EDITOR_SCENE_CONTAINER_H
#define EDITOR_SCENE_CONTAINER_H

#include "Color.h"
#include "EditorSceneSettings.h"
#include "FilePath.h"
#include "FixedArray.h"
#include "HashTable.h"
#include "ScenePrefab.h"
#include "ScopedPtr.h"
#include "SeoulHString.h"
namespace Seoul { namespace Reflection { class Any; } }
namespace Seoul { class Camera; }
namespace Seoul { namespace EditorScene { class State; } }
namespace Seoul { namespace EditorScene { class StateLoadJob; } }
namespace Seoul { class RenderCommandStreamBuilder; }
namespace Seoul { class RenderPass; }
namespace Seoul { namespace Scene { class Object; } }
namespace Seoul { namespace Scene { class Renderer; } }
namespace Seoul { namespace Scene { class Ticker; } }

#if SEOUL_WITH_SCENE

namespace Seoul::EditorScene
{

/**
 * Container is a Scene container for the Seoul Editor.
 */
class Container SEOUL_SEALED
{
public:
	Container(const Settings& settings);
	~Container();

	// Get the scene state - only valid if finished loading.
	CheckedPtr<State> GetState() const { return (IsLoading() ? nullptr : m_pState.Get()); }

	// Return true if a scene load is active, false otherwise.
	Bool IsLoading() const;

	// Return true if the last marked load count is out-of-sync with
	// the count on disk.
	Bool IsOutOfDate() const;

	// Flag this scene as up-to-date, this will cause IsOutOfDate()
	// to return false.
	void MarkUpToDate();

	void AddObject(const SharedPtr<Scene::Object>& pObject);
	void RemoveObject(const SharedPtr<Scene::Object>& pObject);
	typedef Vector<SharedPtr<Scene::Object>, MemoryBudgets::SceneObject> Objects;
	const Objects& GetObjects() const;
	void SortObjects();

	// Entry point, called per frame to advance/simulate the current scene state.
	void Tick(Float fDeltaTimeInSeconds);

	// Edit commit of root scene.
	Bool Save(FilePath filePath);

private:
	Settings m_Settings;
	SharedPtr<StateLoadJob> m_pStateLoadJob;
	ScopedPtr<Scene::Ticker> m_pSceneTicker;
	ScopedPtr<State> m_pState;
	Content::Handle<Scene::Prefab> m_hLoadTracker;
	Atomic32Type m_MarkedLoadsCount;

	Bool InternalCheckState();

	typedef HashSet<String, MemoryBudgets::Editor> UniqueIdSet;
	UniqueIdSet m_UniqueIdSet;

	void InternalEnforceUniqueAndValidObjectIds();

	SEOUL_DISABLE_COPY(Container);
}; // class Container

} // namespace Seoul::EditorScene

#endif // /#if SEOUL_WITH_SCENE

#endif // include guard
