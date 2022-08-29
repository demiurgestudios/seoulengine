/**
 * \file SceneComponent.h
 * \brief Component specifies the behavior
 * and qualities of a Object.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCENE_COMPONENT_H
#define SCENE_COMPONENT_H

#include "SharedPtr.h"
#include "ReflectionDeclare.h"
namespace Seoul { namespace Scene { class Interface; } }
namespace Seoul { namespace Scene { class Object; } }

#if SEOUL_WITH_SCENE

namespace Seoul::Scene
{

/**
 * Components specifies the behavior
 * and qualities of a Object.
 *
 * Subclasses of Component are used to
 * give a Object different attributes.
 *
 * For example, a MeshDrawComponent associates
 * a visible Mesh with the Object.
 */
class Component SEOUL_ABSTRACT
{
public:
	SEOUL_REFLECTION_POLYMORPHIC_BASE(Component);

	virtual ~Component();

	virtual SharedPtr<Component> Clone(const String& sQualifier) const = 0;

	/** Get the Object of this component. Can be nullptr. */
	CheckedPtr<Object> GetOwner() const
	{
		return m_pOwner;
	}

	/** true for subclasses of a SceneGetTransformComponent. */
	virtual Bool CanGetTransform() const { return false; }

	/** true for subclasses of a SceneSetTransformComponent. */
	virtual Bool CanSetTransform() const { return false; }

	/** true for subclasses that need an OnGroupInstantiateComplete call. */
	virtual Bool NeedsOnGroupInstantiateComplete() const { return false; }

	/** Called on an object that has been cloned from a prefab, after the entire prefab has been instantiated. */
	virtual void OnGroupInstantiateComplete(Interface& rInterface) {}

	// Release this Component from its owner Object.
	void RemoveFromOwner();

protected:
	SEOUL_REFERENCE_COUNTED(Component);

	Component();

private:
	friend class Object;
	CheckedPtr<Object> m_pOwner;

	SEOUL_DISABLE_COPY(Component);
}; // class Component

} // namespace Seoul::Scene

#endif // /#if SEOUL_WITH_SCENE

#endif // include guard
