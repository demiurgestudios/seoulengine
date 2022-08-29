/**
 * \file SceneObject.h
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

#pragma once
#ifndef SCENE_OBJECT_H
#define SCENE_OBJECT_H

#include "Matrix4D.h"
#include "Prereqs.h"
#include "ReflectionTypeInfo.h"
#include "SceneObjectHandle.h"
#include "SceneSetTransformComponent.h"
#include "SeoulString.h"
#include "SharedPtr.h"
#include "Vector.h"
namespace Seoul { namespace Scene { class Interface; } }

#if SEOUL_WITH_SCENE

namespace Seoul::Scene
{

/**
 * Basic building block of 3D scenes.
 *
 * A collection of Objects forms a Prefab.
 */
class Object SEOUL_SEALED
{
public:
	typedef Vector<SharedPtr<Component>, MemoryBudgets::SceneComponent> Components;

	/**
	 * Given an id qualifier (e.g. "root.sub_group"), construct the
	 * fully qualified id to assign to a Object's id field.
	 */
	static void QualifyId(const String& sQualifier, String& rsCurrentId)
	{
		// Nop if sQualifier is empty.
		if (sQualifier.IsEmpty())
		{
			return;
		}

		// Otherwise, new id is <sQualifier>.<sCurrentId>
		String sNewId(sQualifier);
		sNewId.Append('.');
		sNewId.Append(rsCurrentId);
		rsCurrentId.Swap(sNewId);
	}

	/**
	 * Given a fully qualified id, find its path.
	 */
	static void RemoveLeafId(String& rsFullId)
	{
		UInt32 const uPos = rsFullId.FindLast('.');
		if (String::NPos != uPos)
		{
			rsFullId.ShortenTo(uPos);
		}
		else
		{
			rsFullId.Clear();
		}
	}

	Object(const String& sId = String());
	~Object();

	// Give this Object a new Component. pComponent
	// will be removed from its current owner, if it has one.
	void AddComponent(const SharedPtr<Component>& pComponent);

	// Transform query.
	Bool CanGetTransform() const { return m_pGetTransformComponent.IsValid(); }
	Bool CanSetTransform() const { return m_pSetTransformComponent.IsValid(); }

	// Generate a deep copy of this Object and all its Components.
	CheckedPtr<Object> Clone(const String& sQualifier) const;

	/**
	 * Derive the Object's normal transform.
	 *
	 * A normal transform is orthonormal and is appropriate
	 * for application to normal vectors. With regards to
	 * standard scene transformations, it includes translation
	 * and rotation but excludes skew and scale.
	 */
	Matrix4D ComputeNormalTransform() const
	{
		return
			Matrix4D::CreateRotationTranslation(
				GetRotation(),
				GetPosition());
	}

	/**
	 * Get Component T from this Object, or nullptr if not present.
	 *
	 * Returns the Component of type T. T must be the most derived
	 * class.
	 */
	template <typename T>
	SharedPtr<T> GetComponent() const
	{
		Components::ConstIterator const iBegin = m_vComponents.Begin();
		Components::ConstIterator const iEnd = m_vComponents.End();
		for (Components::ConstIterator i = iBegin; iEnd != i; ++i)
		{
			if ((*i)->GetReflectionThis().IsOfType<T*>())
			{
				return SharedPtr<T>((T*)i->GetPtr());
			}
		}

		return SharedPtr<T>();
	}

	// Reflection generic version of GetComponent<T>().
	// If exact is true, then (type == type), otherwise
	// type can be a parent class of the component.
	SharedPtr<Component> GetComponent(const Reflection::Type& type, Bool bExact = true) const;

	/**
	 * @return a read-only reference to the list of Components currently owned
	 * by this Object.
	 */
	const Components& GetComponents() const
	{
		return m_vComponents;
	}

	/** The fully qualified identifier of this Object. */
	const String& GetId() const
	{
		return m_sId;
	}

	/** Convenience, retrieve the rotation of this Object from its TransformComponent. */
	Quaternion GetRotation() const
	{
		return (m_pGetTransformComponent.IsValid()
			? m_pGetTransformComponent->GetRotation()
			: Quaternion::Identity());
	}

	/** Convenience, retrieve the position of this Object from its TransformComponent. */
	Vector3D GetPosition() const
	{
		return (m_pGetTransformComponent.IsValid()
			? m_pGetTransformComponent->GetPosition()
			: Vector3D::Zero());
	}

	// TODO: This method is risky if we ever
	// start caching the identifier for quick lookup.

	/** Update the fully qualified identifier of this Object. */
	void SetId(const String& sId)
	{
		m_sId = sId;
	}

	/** Convenience, commit a new rotation for this Object to its TransformComponent. */
	void SetRotation(const Quaternion& qRotation)
	{
		if (m_pSetTransformComponent.IsValid())
		{
			m_pSetTransformComponent->SetRotation(qRotation);
		}
	}

	/** Convenience, commit a new position for this Object to its TransformComponent. */
	void SetPosition(const Vector3D& vPosition)
	{
		if (m_pSetTransformComponent.IsValid())
		{
			m_pSetTransformComponent->SetPosition(vPosition);
		}
	}

	/**
	 * Lazy acquire this Object's handle. Objects do not allocate their handle
	 * until requested.
	 */
	const SceneObjectHandle& AcquireHandle();

#if SEOUL_EDITOR_AND_TOOLS
	/** Used for basic (1 level) organization without a Prefab - editor time only, discarded at runtime. */
	HString GetEditorCategory() const { return m_EditorCategory; }

	/** Editor only - sorts Components into a consistent order for display. */
	void EditorOnlySortComponents();

	/** @return true if any renderable Components of the object should be rendered in the editor. */
	Bool GetVisibleInEditor() const { return m_bVisibleInEditor; }

	/** Used for basic (1 level) organization without a Prefab - editor time only, discarded at runtime. */
	void SetEditorCategory(HString category) { m_EditorCategory = category; }

	/** Update editor time visibility of any renderable components of this Object. */
	void SetVisibleInEditor(Bool bVisibleInEditor) { m_bVisibleInEditor = bVisibleInEditor; }
#endif // /SEOUL_EDITOR_AND_TOOLS

private:
	SEOUL_REFLECTION_FRIENDSHIP(Object);
	SEOUL_REFERENCE_COUNTED(Object);

	SceneObjectHandle m_hThis;
	Components m_vComponents;
	SharedPtr<GetTransformComponent> m_pGetTransformComponent;
	SharedPtr<SetTransformComponent> m_pSetTransformComponent;
	String m_sId;

#if SEOUL_EDITOR_AND_TOOLS
	HString m_EditorCategory;
	Bool m_bVisibleInEditor;
#endif // /#if SEOUL_EDITOR_AND_TOOLS

	friend class Component;
	void FriendRemoveComponent(CheckedPtr<Component> pComponent);

	SEOUL_DISABLE_COPY(Object);
}; // class Object

} // namespace Seoul::Scene

#endif // /#if SEOUL_WITH_SCENE

#endif // include guard
