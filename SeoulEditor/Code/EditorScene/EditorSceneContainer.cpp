/**
 * \file EditorSceneContainer.cpp
 * \brief A Scene container (tree of Scene::Prefabs).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ContentLoadManager.h"
#include "EditorSceneContainer.h"
#include "EditorSceneState.h"
#include "EditorSceneStateLoadJob.h"
#include "FileManager.h"
#include "EventsManager.h"
#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionSerialize.h"
#include "Renderer.h"
#include "SceneObject.h"
#include "ScenePrefabManager.h"
#include "SceneRenderer.h"
#include "SceneTicker.h"

#if SEOUL_WITH_SCENE

namespace Seoul::EditorScene
{

static const HString kEditor("Editor");
static const HString kObjects("Objects");

Container::Container(const EditorScene::Settings& settings)
	: m_Settings(settings)
	, m_pStateLoadJob()
	, m_pSceneTicker(SEOUL_NEW(MemoryBudgets::Scene) Scene::Ticker)
	, m_pState()
	, m_hLoadTracker()
	, m_MarkedLoadsCount(0)
	, m_UniqueIdSet()
{
	// Allocate and start the initial load job. Nop if empty FilePath.
	if (settings.m_RootScenePrefabFilePath.IsValid())
	{
		m_pStateLoadJob.Reset(SEOUL_NEW(MemoryBudgets::TBD) StateLoadJob(settings));
		m_pStateLoadJob->StartJob();
	}
	else
	{
		m_pState.Reset(SEOUL_NEW(MemoryBudgets::Scene) State);
	}
}

Container::~Container()
{
	if (m_pStateLoadJob.IsValid())
	{
		m_pStateLoadJob->WaitUntilJobIsNotRunning();
		{
			ScopedPtr<State> p;
			m_pStateLoadJob->AcquireNewStateDestroyOldState(p);
		}
		m_pStateLoadJob.Reset();
	}
}

/** @return True if the root scene is still loading, false otherwise. */
Bool Container::IsLoading() const
{
	return m_pStateLoadJob.IsValid() && m_pStateLoadJob->IsJobRunning();
}

/**
 * @return true if the last marked load count is out-of-sync with
 * the count on disk.
 */
Bool Container::IsOutOfDate() const
{
	// Done immediately if load count is in sync.
	if (m_MarkedLoadsCount >= m_hLoadTracker.GetTotalLoadsCount())
	{
		return false;
	}

	return true;
}

/**
 * Flag this scene as up-to-date, this will cause IsOutOfDate()
 * to return false.
 */
void Container::MarkUpToDate()
{
	m_MarkedLoadsCount = m_hLoadTracker.GetTotalLoadsCount();
}

void Container::AddObject(const SharedPtr<Scene::Object>& pObject)
{
	// Nothing to do if we don't have a state.
	if (!InternalCheckState())
	{
		return;
	}

	m_pState->GetObjects().PushBack(pObject);
}

void Container::RemoveObject(const SharedPtr<Scene::Object>& pObject)
{
	// Nothing to do if we don't have a state.
	if (!InternalCheckState())
	{
		return;
	}

	auto& r = m_pState->GetObjects();
	auto i = r.Find(pObject);
	if (r.End() != i)
	{
		r.Erase(i);
	}
}

const Container::Objects& Container::GetObjects() const
{
	static const Objects s_vEmptyObjects; // TODO:

	if (!m_pState.IsValid())
	{
		return s_vEmptyObjects;
	}

	return m_pState->GetObjects();
}

namespace
{

struct ObjectSorter
{
	Bool operator()(const SharedPtr<Scene::Object>& pA, const SharedPtr<Scene::Object>& pB) const
	{
		if (pA->GetEditorCategory() == pB->GetEditorCategory())
		{
			return (pA->GetId() < pB->GetId());
		}
		else
		{
			return (strcmp(pA->GetEditorCategory().CStr(), pB->GetEditorCategory().CStr()) < 0);
		}
	}
};

} // namespace anonymous

void Container::SortObjects()
{
	if (!m_pState.IsValid())
	{
		return;
	}

	// Prior to sort, make sure all object ids are unique.
	InternalEnforceUniqueAndValidObjectIds();

	ObjectSorter sorter;
	QuickSort(m_pState->GetObjects().Begin(), m_pState->GetObjects().End(), sorter);
}

/** Run per-frame simulation and updates. */
void Container::Tick(Float fDeltaTimeInSeconds)
{
	// Nothing to do if we don't have a state.
	if (!InternalCheckState())
	{
		return;
	}

	// Process the append queue.
	m_pState->ProcessAddPrefabQueue();

	// Tick native scene.
	m_pSceneTicker->Tick(*m_pState, m_pState->GetObjects(), fDeltaTimeInSeconds);
}

/* Edit commit of root scene. */
Bool Container::Save(FilePath filePath)
{
	// Nothing to do if we don't have a state.
	if (!m_pState.IsValid())
	{
		return false; // TODO: Report/warn.
	}

	DataStore dataStore;
	dataStore.MakeTable();
	if (!Reflection::SerializeObjectToTable(
		filePath,
		dataStore,
		dataStore.GetRootNode(),
		kEditor,
		&m_pState->GetEditState()))
	{
		return false; // TODO: Report/warn.
	}

	if (!Reflection::SerializeObjectToTable(
		filePath,
		dataStore,
		dataStore.GetRootNode(),
		kObjects,
		&m_pState->GetObjects()))
	{
		return false; // TODO: Report/warn.
	}

	// Abort the save if the file is actively loading.
	if (!Scene::PrefabManager::Get()->CanSave(filePath))
	{
		return false; // TODO: Report/warn.
	}
	// Don't react to hot loads of this file since the engine is the
	// source of the change.
	Content::LoadManager::Get()->TempSuppressSpecificHotLoad(filePath);
	// On failure, early out.
	if (!Reflection::SaveDataStore(dataStore, dataStore.GetRootNode(), filePath))
	{
		return false; // TODO: Report/warn.
	}

	// Make sure we're fresh on FilePath changes.
	m_Settings.m_RootScenePrefabFilePath = filePath;
	// Grab the prefab for tracking purposes.
	m_hLoadTracker = Scene::PrefabManager::Get()->GetPrefab(filePath);
	// We will set the loading tracking to +1 to mask out the reload from
	// the change we're about to make.
	m_MarkedLoadsCount = m_hLoadTracker.GetTotalLoadsCount() + 1;
	return true;
}

/**
 * Checks the current state pimpl and potentially hot loads
 * or refreshes it.
 *
 * A true return valid means the state is ready to access,
 * false implies m_pState.IsValid() is false and no operations
 * against m_pState are possible.
 */
Bool Container::InternalCheckState()
{
	if (m_pStateLoadJob.IsValid())
	{
		if (m_pStateLoadJob->IsJobRunning())
		{
			return m_pState.IsValid();
		}

		m_pStateLoadJob->AcquireNewStateDestroyOldState(m_pState);
		m_pStateLoadJob.Reset();
		m_hLoadTracker = Scene::PrefabManager::Get()->GetPrefab(m_Settings.m_RootScenePrefabFilePath);
		MarkUpToDate();

		SortObjects();
	}

	return m_pState.IsValid();
}

// TODO: Once we've wrangled the object id
// type into something more permanent, this should
// probably be pushed into a general utility in Scene.

/**
 * Equivalent to Sanitize, but simply checks that an ID is already conformant
 * to our id requirements.
 */
static inline Bool IsSanitized(const String& s)
{
	if (s.IsEmpty())
	{
		return false;
	}

	UniChar prev = '\0';
	auto const iBegin = s.Begin();
	auto const iEnd = s.End();
	for (auto i = iBegin; iEnd != i; ++i)
	{
		auto ch = *i;

		// Character needs to be sanitized.
		if (!(
			(ch >= 'a' && ch <= 'z') ||
			(ch >= 'A' && ch <= 'Z') ||
			(ch >= '0' && ch <= '9') ||
			'_' == ch))
		{
			return false;
		}

		// Two underscores in a row in the input, also
		// needs to be sanitized.
		if ('_' == ch && '_' == prev)
		{
			return false;
		}

		prev = ch;
	}

	return true;
}

/**
 * Object id sanitizing.
 */
static inline String Sanitize(const String& s)
{
	// Nothing to do if an empty string.
	if (s.IsEmpty())
	{
		return "Object";
	}

	String sReturn;
	UniChar prev = '\0';
	auto const iBegin = s.Begin();
	auto const iEnd = s.End();
	for (auto i = iBegin; iEnd != i; ++i)
	{
		auto ch = *i;

		// Sanitize - must be an alphanumeric ASCII
		// character or an underscore, otherwise
		// we replace it with an underscore.
		if (!(
			(ch >= 'a' && ch <= 'z') ||
			(ch >= 'A' && ch <= 'Z') ||
			(ch >= '0' && ch <= '9') ||
			'_' == ch))
		{
			ch = '_';
		}

		// Don't allow more than one underscore
		// in a row.
		if ('_' == prev && '_' == ch)
		{
			continue;
		}

		// Append the original/sanitized character.
		sReturn.Append(ch);

		// Update prev.
		prev = ch;
	}

	return sReturn;
}

/**
 * Given an existing id, apply a numeric suffix of i
 * to attempt to make it unique.
 */
static inline String GenerateId(const String& sId, Int i)
{
	// Always start with something.
	String s(sId.IsEmpty() ? String("Object") : sId);

	// Trim any existing number suffix and insert our replacement number.
	auto iEnd = --s.End();
	while (*iEnd >= '0' && *iEnd <= '9')
	{
		--iEnd;
	}
	++iEnd;

	// Result is the trimmed input (without number suffix)
	// followed by our number padded to 3 digits.
	return s.Substring(0u, (UInt)iEnd.GetIndexInBytes()) + String::Printf("%03d", i);
}

/**
 * Utility function, renames, as needed, any object
 * that has a duplicate id.
 */
void Container::InternalEnforceUniqueAndValidObjectIds()
{
	// TODO: This function should apply sanitizing
	// rules to object ids.
	// TODO: We want to replace object id with a type that:
	// - is case insensitive.
	// - is cheap and cached, like HString, but is referenced counted
	//   and releases the strings when all references are released.

	// Clear the existing set.
	m_UniqueIdSet.Clear();

	// Now accumulate and rename as needed.
	String sId;
	for (auto& p : m_pState->GetObjects())
	{
		// Fast path, id is fine as-is.
		if (IsSanitized(p->GetId()) &&
			m_UniqueIdSet.Insert(p->GetId()).Second)
		{
			continue;
		}

		// Need to sanitize and/or regenerate the name.
		sId = Sanitize(p->GetId());
		Int i = 1;
		while (m_UniqueIdSet.HasKey(sId))
		{
			sId = GenerateId(sId, i++);
		}

		// Update the id.
		p->SetId(sId);

		// Add this key.
		SEOUL_VERIFY(m_UniqueIdSet.Insert(sId).Second);
	}
}

} // namespace Seoul::EditorScene

#endif // /#if SEOUL_WITH_SCENE
