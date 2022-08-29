/**
 * \file EditorUICommandSetComponent.cpp
 * \brief Command for mutation of an Object's attached components.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "EditorUICommandSetComponent.h"
#include "ReflectionAttributes.h"
#include "ReflectionDefine.h"
#include "SceneObject.h"

#if SEOUL_WITH_SCENE

namespace Seoul
{

SEOUL_BEGIN_TYPE(EditorUI::CommandSetComponent, TypeFlags::kDisableNew)
	SEOUL_PARENT(DevUI::Command)
SEOUL_END_TYPE()

namespace EditorUI
{

static inline Byte const* InternalGetComponentName(const Scene::Component& comp)
{
	using namespace Reflection;
	using namespace Reflection::Attributes;

	auto const& type = comp.GetReflectionThis().GetType();
	HString ret = type.GetName();
	if (auto p = type.GetAttribute<DisplayName>())
	{
		ret = p->m_DisplayName;
	}

	return ret.CStr();
}

static inline String InternalGetDescription(
	const SharedPtr<Scene::Object>& pObject,
	const SharedPtr<Scene::Component>& pOld,
	const SharedPtr<Scene::Component>& pNew)
{
	String sReturn;
	if (pOld.IsValid())
	{
		if (pNew.IsValid())
		{
			sReturn = String::Printf("Replaced %s in %s with %s",
				InternalGetComponentName(*pOld),
				pObject->GetId().CStr(),
				InternalGetComponentName(*pNew));
		}
		else
		{
			sReturn = String::Printf("Deleted %s from %s",
				InternalGetComponentName(*pOld),
				pObject->GetId().CStr());
		}
	}
	else
	{
		if (pNew.IsValid())
		{
			sReturn = String::Printf("Added %s to %s",
				InternalGetComponentName(*pNew),
				pObject->GetId().CStr());
		}
	}

	return sReturn;
}

CommandSetComponent::CommandSetComponent(
	const SharedPtr<Scene::Object>& pObject,
	const SharedPtr<Scene::Component>& pOld,
	const SharedPtr<Scene::Component>& pNew)
	: m_pObject(pObject)
	, m_pOld(pOld)
	, m_pNew(pNew)
	, m_sDescription(InternalGetDescription(pObject, pOld, pNew))
{
}

CommandSetComponent::~CommandSetComponent()
{
}

void CommandSetComponent::Do()
{
	if (m_pOld.IsValid())
	{
		m_pOld->RemoveFromOwner();
	}
	if (m_pNew.IsValid())
	{
		m_pObject->AddComponent(m_pNew);
	}
	m_pObject->EditorOnlySortComponents();
}

const String& CommandSetComponent::GetDescription() const
{
	return m_sDescription;
}

UInt32 CommandSetComponent::GetSizeInBytes() const
{
	return sizeof(*this); // TODO:
}

void CommandSetComponent::Undo()
{
	if (m_pNew.IsValid())
	{
		m_pNew->RemoveFromOwner();
	}
	if (m_pOld.IsValid())
	{
		m_pObject->AddComponent(m_pOld);
	}
	m_pObject->EditorOnlySortComponents();
}

} // namespace EditorUI

} // namespace Seoul

#endif // /#if SEOUL_WITH_SCENE
