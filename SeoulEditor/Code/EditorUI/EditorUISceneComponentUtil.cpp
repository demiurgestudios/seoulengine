/**
 * \file EditorUISceneComponentUtil.cpp
 * \brief Shared utilities for deal with Scene::Component
 * instances.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "EditorUISceneComponentUtil.h"
#include "ReflectionAttributes.h"
#include "ReflectionRegistry.h"
#include "ReflectionType.h"
#include "SceneComponent.h"
#include "ScenePrefabComponent.h"

#if SEOUL_WITH_SCENE

namespace Seoul::EditorUI
{

namespace SceneComponentUtil
{

/** Category name to use when none was explicitly specified. */
static const HString kDefaultCategory("Miscellaneous");

namespace
{

struct FlatSort
{
	Bool operator()(const ComponentEntry& a, const ComponentEntry& b) const
	{
		return (strcmp(a.m_DisplayName.CStr(), b.m_DisplayName.CStr()) < 0);
	}
};

struct CategorySort
{
	Bool operator()(const ComponentEntry& a, const ComponentEntry& b) const
	{
		Int32 const i = strcmp(a.m_Category.CStr(), b.m_Category.CStr());
		if (0 == i)
		{
			return (strcmp(a.m_DisplayName.CStr(), b.m_DisplayName.CStr()) < 0);
		}
		else
		{
			return (i < 0);
		}
	}
};

} // namespace anonymous

ComponentTypes PopulateComponentTypes(Bool bIncludePrefabs, Bool bSortByCategory)
{
	using namespace Reflection;
	using namespace Reflection::Attributes;

	ComponentTypes vReturn;

	const Reflection::Type& componentBaseType(TypeOf<Scene::Component>());
	UInt32 const uTypes = Registry::GetRegistry().GetTypeCount();
	auto const& objGroupType = TypeOf<Scene::PrefabComponent>();
	for (UInt32 i = 0u; i < uTypes; ++i)
	{
		auto const& type = *Registry::GetRegistry().GetType(i);
		if (!type.CanNew())
		{
			continue;
		}

		// Special case - don't allow PrefabComponent to be added via
		// the Manage Components menu. A "Prefab object" is
		// treated as special, and must always be a Free Transform Component + Prefab Component.
		if (!bIncludePrefabs && type == objGroupType)
		{
			continue;
		}

		if (type.IsSubclassOf(componentBaseType))
		{
			ComponentEntry entry;
			Attributes::Category const* pCategory = type.GetAttribute<Category>();
			entry.m_Category = (nullptr != pCategory ? pCategory->m_CategoryName : kDefaultCategory);
			entry.m_DisplayName = type.GetName();
			if (Attributes::DisplayName const* pDisplayName = type.GetAttribute<DisplayName>())
			{
				entry.m_DisplayName = pDisplayName->m_DisplayName;
			}
			entry.m_pType = &type;
			vReturn.PushBack(entry);
		}
	}

	if (bSortByCategory)
	{
		CategorySort sorter;
		QuickSort(vReturn.Begin(), vReturn.End(), sorter);
	}
	else
	{
		FlatSort sorter;
		QuickSort(vReturn.Begin(), vReturn.End(), sorter);
	}

	return vReturn;
}

} // namespace SceneComponentUtil

} // namespace Seoul::EditorUI

#endif // /#if SEOUL_WITH_SCENE
