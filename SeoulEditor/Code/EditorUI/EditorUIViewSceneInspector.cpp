/**
 * \file EditorUIViewSceneInspector.cpp
 * \brief A specialized editor for scene objects.
 * Wraps an ViewPropertyEditor and
 * adds some additional functionality.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "DevUIController.h"
#include "DevUIImGui.h"
#include "EditorUIControllerScene.h"
#include "EditorUIViewSceneInspector.h"
#include "ReflectionAttributes.h"
#include "ReflectionRegistry.h"
#include "ReflectionType.h"
#include "SceneComponent.h"
#include "SceneObject.h"
#include "ScenePrefabComponent.h"

#if SEOUL_WITH_SCENE

namespace Seoul::EditorUI
{

ViewSceneInspector::ViewSceneInspector()
	: m_vComponentTypes(SceneComponentUtil::PopulateComponentTypes(false, true))
{
}

ViewSceneInspector::~ViewSceneInspector()
{
}

void ViewSceneInspector::DoPrePose(DevUI::Controller& rController, RenderPass& rPass)
{
	static const HString kComponents("Components"); // TODO:

	using namespace Reflection;
	using namespace Reflection::Attributes;

	ControllerScene* pRoot = DynamicCast<ControllerScene*>(&rController);
	if (nullptr == pRoot)
	{
		return;
	}

	const IControllerSceneRoot::SelectedObjects& tSelectedObjects = pRoot->GetSelectedObjects();
	Bool const bEnabled = !tSelectedObjects.IsEmpty();
	if (!bEnabled)
	{
		return;
	}

	// Reserve work space.
	m_vStack.Reserve(tSelectedObjects.GetSize());

	// Structure for path storage.
	PropertyUtil::Path vPath;

	// Use the parent implementation to handle all properties
	// except for Components.
	{
		// Populate with object pointers.
		for (auto const& pObject : tSelectedObjects)
		{
			m_vStack.PushBack(pObject.GetPtr());
		}
		UInt32 const uBegin = 0u;
		UInt32 const uEnd = m_vStack.GetSize();

		// Property loop.
		auto const& type = TypeOf<Scene::Object>();
		UInt32 const uProperties = type.GetPropertyCount();
		for (UInt32 i = 0u; i < uProperties; ++i)
		{
			const Property& prop = *type.GetProperty(i);

			// Skip Components, handled specially.
			if (kComponents == prop.GetName())
			{
				continue;
			}

			Prop(*pRoot, vPath, prop, uBegin, uEnd);
		}

		// Cleanup.
		m_vStack.Clear();
		SEOUL_ASSERT(vPath.IsEmpty());
		vPath.Clear();
	}

	// Managed components button - only available if controller says so.
	if (pRoot->CanModifyComponents())
	{
		if (ImGui::Button("Manage Components"))
		{
			ImGui::OpenPopup("Manage Components");
		}
		if (ImGui::BeginPopup("Manage Components"))
		{
			InternalPoseComponentMenus(*pRoot);
			ImGui::EndPopup();
		}
	}

	// TODO: This lookup is very expensive for multiple
	// selection - for each component, we take the type of the
	// Component in the first
	// object, then search for that type in each additional objects.
	//
	// - GetComponent<> optimizations will help here.
	// - after that, some form of caching will be necessary.

	// Now components.
	{
		UInt32 const uComponents = (*tSelectedObjects.Begin())->GetComponents().GetSize();
		WeakAny value;
		for (UInt32 i = 0u; i < uComponents; ++i)
		{
			Bool bReady = true;
			TypeInfo const* pInfo = nullptr;
			for (auto const& pObject : tSelectedObjects)
			{
				auto const& v = pObject->GetComponents();

				// If this is the first object, get the type,
				// and acquire the value directly.
				if (nullptr == pInfo)
				{
					value = v[i]->GetReflectionThis();
					pInfo = &value.GetTypeInfo();
				}
				// Additional entries, try to acquire a component with the
				// same type as pInfo.
				else
				{
					// TODO: This inner loop is a most of why this
					// overall loop is slow. It's O(n * m), where n is the number
					// of objects and m is the (average) number of components per
					// object.

					value.Reset();
					for (auto const& pComponent : v)
					{
						value = pComponent->GetReflectionThis();
						if (value.GetTypeInfo() == *pInfo)
						{
							break;
						}
						else
						{
							value.Reset();
						}
					}

					// Done, no corresponding component.
					if (!value.IsValid())
					{
						bReady = false;
						break;
					}
				}

				// Add the value.
				m_vStack.PushBack(value);
			}

			if (bReady)
			{
				auto const& type = pInfo->GetType();
				Byte const* sLabel = type.GetName().CStr();
				if (auto pAttr = type.GetAttribute<DisplayName>())
				{
					sLabel = pAttr->m_DisplayName.CStr();
				}

				ImGuiTreeNodeFlags uFlags = 0u;
				if (auto pAttr = type.GetAttribute<EditorDefaultExpanded>())
				{
					uFlags |= ImGuiTreeNodeFlags_DefaultOpen;
				}

				if (ImGui::CollapsingHeader(sLabel, uFlags))
				{
					vPath.PushBack(kComponents);
					vPath.PushBack(i);
					Complex(*pRoot, vPath, type, 0u, m_vStack.GetSize());
					vPath.Clear();
				}
			}

			m_vStack.Clear();
		}
	}
}

void ViewSceneInspector::InternalPoseComponentMenus(
	IControllerSceneRoot& rController) const
{
	using namespace ImGui;

	// Early out if no selection.
	if (rController.GetSelectedObjects().IsEmpty())
	{
		return;
	}

	auto const& pObject = *rController.GetSelectedObjects().Begin();
	HString category;
	Bool bAddCurrent = false;
	for (auto type : m_vComponentTypes)
	{
		if (type.m_Category != category)
		{
			if (bAddCurrent)
			{
				EndMenu();
			}

			category = type.m_Category;
			bAddCurrent = BeginMenu(category.CStr());
		}

		if (bAddCurrent)
		{
			auto bSelected = pObject->GetComponent(*type.m_pType).IsValid();
			if (MenuItem(type.m_DisplayName.CStr(), nullptr, &bSelected, true))
			{
				HString const displayName = type.m_DisplayName;
				HString const typeName = type.m_pType->GetName();
				if (bSelected)
				{
					rController.SelectedObjectAddComponent(typeName);
				}
				else
				{
					rController.SelectedObjectRemoveComponent(typeName);
				}
			}
		}
	}

	if (bAddCurrent)
	{
		EndMenu();
	}
}

} // namespace Seoul::EditorUI

#endif // /#if SEOUL_WITH_SCENE
