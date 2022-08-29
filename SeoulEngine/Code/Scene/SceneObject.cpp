/**
 * \file SceneObject.cpp
 * \brief Object is the basic building block of a 3D scene.
 *
 * Scenes are made up of Prefabs, and Prefabs
 * are made up of Objects. Components fully define
 * and qualify the behavior of Objects.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionDefine.h"
#include "ReflectionType.h"
#include "SceneObject.h"

#if SEOUL_WITH_SCENE

namespace Seoul
{

SEOUL_BEGIN_TYPE(Scene::Object, TypeFlags::kDisableNew)
	SEOUL_PROPERTY_N_EXT("Components", m_vComponents, PropertyFlags::kDisableSet)
	SEOUL_PROPERTY_N("Id", m_sId)
		SEOUL_DEV_ONLY_ATTRIBUTE(Description,
			"Unique identifier (within a Prefab) of this Object")

	// TODO: Probably want to hoist this into Editor only data
	// so it can be completely stripped by the cooker. Or, we need
	// to make the cooker smarter (perhaps use Reflection to export
	// a schema and tag this as editor only). Not a high priority -
	// this data is small and will not be deserialized in runtimes anyway.
#if SEOUL_EDITOR_AND_TOOLS
	SEOUL_PROPERTY_N("Category", m_EditorCategory)
		SEOUL_ATTRIBUTE(NotRequired)
		SEOUL_ATTRIBUTE(Description,
			"Editor only. Provides support for organizing Objects\n"
			"within their Prefab.")
#endif // /#if SEOUL_EDITOR_AND_TOOLS

SEOUL_END_TYPE()

namespace Scene
{

Object::Object(const String& sId /*= String()*/)
	: m_vComponents()
	, m_pGetTransformComponent()
	, m_pSetTransformComponent()
	, m_sId(sId)
#if SEOUL_EDITOR_AND_TOOLS
	, m_EditorCategory()
	, m_bVisibleInEditor(true)
#endif // /#if SEOUL_EDITOR_AND_TOOLS
{
}

Object::~Object()
{
	while (!m_vComponents.IsEmpty())
	{
		SharedPtr<Component> pComponent(m_vComponents.Back());
		pComponent->RemoveFromOwner();
	}

	// Free our handle.
	SceneObjectHandleTable::Free(m_hThis);
}

/**
 * Insert a new Component into this Object.
 */
void Object::AddComponent(const SharedPtr<Component>& pComponent)
{
	if (pComponent->m_pOwner)
	{
		pComponent->RemoveFromOwner();

		// Sanity check.
		SEOUL_ASSERT(!pComponent->m_pOwner.IsValid());
	}

	if (pComponent->CanSetTransform())
	{
		// Sanity check, in case someone decides to implement CanSetTransform()
		// for a Component that does not inherit from SetTransformComponent.
		SEOUL_ASSERT(pComponent->GetReflectionThis().GetType().IsSubclassOf(TypeOf<SetTransformComponent>()));

		m_pGetTransformComponent.Reset((GetTransformComponent*)pComponent.GetPtr());
		m_pSetTransformComponent.Reset((SetTransformComponent*)pComponent.GetPtr());
	}
	else if (pComponent->CanGetTransform())
	{
		// Sanity check, in case someone decides to implement CanGetTransform()
		// for a Component that does not inherit from GetTransformComponent.
		SEOUL_ASSERT(pComponent->GetReflectionThis().GetType().IsSubclassOf(TypeOf<GetTransformComponent>()));

		m_pGetTransformComponent.Reset((GetTransformComponent*)pComponent.GetPtr());
	}

	m_vComponents.PushBack(pComponent);

	pComponent->m_pOwner = this;
}

CheckedPtr<Object> Object::Clone(const String& sQualifier) const
{
	CheckedPtr<Object> pClone(SEOUL_NEW(MemoryBudgets::SceneObject) Object);

	UInt32 const uSize = m_vComponents.GetSize();

	pClone->m_sId = m_sId;
	QualifyId(sQualifier, pClone->m_sId);

#if SEOUL_EDITOR_AND_TOOLS
	pClone->m_EditorCategory = m_EditorCategory;
#endif // /#if SEOUL_EDITOR_AND_TOOLS

	pClone->m_vComponents.Reserve(uSize);
	for (UInt32 i = 0u; i < uSize; ++i)
	{
		SharedPtr<Component> pCloneComponent(m_vComponents[i]->Clone(sQualifier));
		pClone->AddComponent(pCloneComponent);
	}

	return pClone;
}

/** Reflection generic version of GetComponent<T>(). */
SharedPtr<Component> Object::GetComponent(
	const Reflection::Type& type,
	Bool bExact /*= true*/) const
{
	Components::ConstIterator const iBegin = m_vComponents.Begin();
	Components::ConstIterator const iEnd = m_vComponents.End();

	if (bExact)
	{
		for (Components::ConstIterator i = iBegin; iEnd != i; ++i)
		{
			const Reflection::Type& typeI = (*i)->GetReflectionThis().GetType();
			if (typeI == type)
			{
				return *i;
			}
		}
	}
	else
	{
		for (Components::ConstIterator i = iBegin; iEnd != i; ++i)
		{
			const Reflection::Type& typeI = (*i)->GetReflectionThis().GetType();
			if (typeI == type || typeI.IsSubclassOf(type))
			{
				return *i;
			}
		}
	}

	return SharedPtr<Component>();
}

const SceneObjectHandle& Object::AcquireHandle()
{
	if (!m_hThis.IsInternalValid())
	{
		// Allocate a handle for this.
		m_hThis = SceneObjectHandleTable::Allocate(this);
	}

	return m_hThis;
}

namespace
{

struct ComponentSorter SEOUL_SEALED
{
	Bool operator()(const SharedPtr<Component>& pA, const SharedPtr<Component>& pB) const
	{
		// Transform component is always first.
		if (pA->CanGetTransform() && !pB->CanGetTransform())
		{
			return true;
		}
		else if (!pA->CanGetTransform() && pB->CanGetTransform())
		{
			return false;
		}
		else
		{
			// Lexographical sort of the typenames.
			return (strcmp(
				pA->GetReflectionThis().GetType().GetName().CStr(),
				pB->GetReflectionThis().GetType().GetName().CStr()) < 0);
		}
	}
}; // struct ComponentSorter

} // namespace anonymous

#if SEOUL_EDITOR_AND_TOOLS
void Object::EditorOnlySortComponents()
{
	// Component order is defined as:
	// - transform component first.
	// - all remaining components lexographical by their type name.
	ComponentSorter sorter;
	QuickSort(m_vComponents.Begin(), m_vComponents.End(), sorter);
}
#endif // /#if SEOUL_EDITOR_AND_TOOLS

void Object::FriendRemoveComponent(CheckedPtr<Component> pComponent)
{
	// Sanity check - should be enforced by Component.
	SEOUL_ASSERT(pComponent);

	// Hard ownership of pComponent until the method exist.
	SharedPtr<Component> p(pComponent);

	// Release the transform component reference if present.
	if (m_pGetTransformComponent == p)
	{
		m_pGetTransformComponent.Reset();
	}
	if (m_pSetTransformComponent == p)
	{
		m_pSetTransformComponent.Reset();
	}

	Components::Iterator const i = m_vComponents.FindFromBack(p);
	SEOUL_ASSERT(m_vComponents.End() != i);

	m_vComponents.Erase(i);
}

} // namespace Seoul

} // namespace Seoul

#endif // /#if SEOUL_WITH_SCENE
