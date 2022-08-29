/**
 * \file ScenePrefabComponent.cpp
 * \brief Component that defines a nested Prefab within another Prefab.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ReflectionDefine.h"
#include "SceneObject.h"
#include "ScenePrefabComponent.h"
#include "ScenePrefabManager.h"

#if SEOUL_WITH_SCENE

namespace Seoul
{

SEOUL_BEGIN_TYPE(Scene::PrefabComponent, TypeFlags::kDisableCopy)
	SEOUL_DEV_ONLY_ATTRIBUTE(DisplayName, "Prefab")
	SEOUL_DEV_ONLY_ATTRIBUTE(EditorDefaultExpanded)
	SEOUL_PARENT(Scene::Component)
	SEOUL_PROPERTY_PAIR_N("FilePath", GetFilePath, SetFilePath)
		SEOUL_DEV_ONLY_ATTRIBUTE(EditorFileSpec, GameDirectory::kContent, FileType::kScenePrefab)
SEOUL_END_TYPE()

namespace Scene
{

PrefabComponent::PrefabComponent()
	: m_hPrefab()
#if SEOUL_HOT_LOADING
	, m_LastLoadId(0)
#endif // /#if SEOUL_HOT_LOADING
{
}

PrefabComponent::~PrefabComponent()
{
}

#if SEOUL_HOT_LOADING
void PrefabComponent::CheckHotLoad()
{
	Atomic32Type const loadId = m_hPrefab.GetTotalLoadsCount();
	if (loadId == m_LastLoadId)
	{
		// Early out, no changes.
		return;
	}

	m_LastLoadId = loadId;

	// Regenerate instances.
	CreateObjects();
}
#endif

SharedPtr<Component> PrefabComponent::Clone(const String& sQualifier) const
{
	SharedPtr<PrefabComponent> pReturn(SEOUL_NEW(MemoryBudgets::SceneComponent) PrefabComponent);
	pReturn->m_hPrefab = m_hPrefab;
	return pReturn;
}

void PrefabComponent::SetFilePath(FilePath filePath)
{
	SetPrefab(PrefabManager::Get()->GetPrefab(filePath));
}

Bool PrefabComponent::GetObjectById(const String& sId, SharedPtr<Object>& rpObject) const
{
	// TODO: Profile once we have a scene of decent size and decide
	// if this should have a shadow table to make this O(1). My expectation
	// is that all accesses will go through script, so it may be better
	// to pre-emptively populate the script lookup tables instead of
	// maintaining a native lookup table also.

	auto const& vObjects = m_vObjects;
	UInt32 const uObjects = vObjects.GetSize();
	for (UInt32 i = 0u; i < uObjects; ++i)
	{
		if (vObjects[i]->GetId() == sId)
		{
			rpObject = vObjects[i];
			return true;
		}
	}

	return false;
}

// TODO: It is not readily apparent from this function
// that is it editor time only. At runtime, we flatten Prefab
// graphs into a root list of objects. As a result, an PrefabComponent
// will never appear at runtime, and the only instance of a valid CreateObjects()
// call is during Editor eval and rendering for in-editor viz.
//
// Much about this class is unclear as a result - in particular, m_vObjects is
// also Editor time only.

void PrefabComponent::CreateObjects()
{
	// Reset any objects we currently have.
	m_vObjects.Clear();

	// Resolve the group.
	auto pGroup(m_hPrefab.GetPtr());

	// Early out if no group.
	if (!pGroup.IsValid())
	{
		return;
	}

	// Cache for iteration.
	auto const& t = pGroup->GetTemplate();
	auto const& v = t.m_vObjects;

	m_vObjects.Reserve(v.GetSize());

	// TODO: Not important to get this off the stack as long
	// as this function is editor only. If that changes, this may
	// become a problem.
	Vector<SharedPtr<Component>, MemoryBudgets::SceneComponent> vComponents;

	// Iterate and clone.
	for (auto const& p : v)
	{
		// Instantiate the object clone.
		SharedPtr<Object> pObject(p->Clone(String()));

		// Track components that need a post instantiate call.
		for (auto const& pComponent : pObject->GetComponents())
		{
			if (pComponent->NeedsOnGroupInstantiateComplete())
			{
				vComponents.PushBack(pComponent);
			}
		}

		// Add the object.
		m_vObjects.PushBack(pObject);
	}

	// Now process any group instantiate components.
	for (auto const& pComponent : vComponents)
	{
		pComponent->OnGroupInstantiateComplete(*this);
	}
}

} // namespace Scene

} // namespace Seoul

#endif // /#if SEOUL_WITH_SCENE
