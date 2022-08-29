/**
 * \file EditorUISceneComponentUtil.h
 * \brief Shared utilities for deal with Scene::Component
 * instances.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef EDITOR_UI_SCENE_COMPONENT_UTIL_H
#define EDITOR_UI_SCENE_COMPONENT_UTIL_H

#include "Prereqs.h"
#include "SeoulHString.h"
namespace Seoul { namespace Reflection { class Type; } }

#if SEOUL_WITH_SCENE

namespace Seoul::EditorUI
{

namespace SceneComponentUtil
{

struct ComponentEntry SEOUL_SEALED
{
	Reflection::Type const* m_pType;
	HString m_Category;
	HString m_DisplayName;
};
typedef Vector<ComponentEntry, MemoryBudgets::Editor> ComponentTypes;

ComponentTypes PopulateComponentTypes(Bool bIncludePrefabs, Bool bSortByCategory);

} // namespace SceneComponentUtil

} // namespace Seoul::EditorUI

#endif // /#if SEOUL_WITH_SCENE

#endif // include guard
