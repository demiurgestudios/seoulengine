/**
 * \file SceneComponent.cpp
 * \brief Component specifies the behavior
 * and qualities of an Object.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionDefine.h"
#include "SceneComponent.h"
#include "SceneObject.h"

#if SEOUL_WITH_SCENE

namespace Seoul
{

SEOUL_SPEC_TEMPLATE_TYPE(Vector<SharedPtr<Scene::Component>, MemoryBudgets::SceneComponent>)
SEOUL_SPEC_TEMPLATE_TYPE(SharedPtr<Scene::Component>)
SEOUL_BEGIN_TYPE(Scene::Component, TypeFlags::kDisableNew)
	SEOUL_ATTRIBUTE(PolymorphicKey, "$type")
SEOUL_END_TYPE()

namespace Scene
{

Component::Component()
	: m_pOwner()
{
}

Component::~Component()
{
	// Sanity check.
	SEOUL_ASSERT(!m_pOwner);
}

/** Release this Component from its owner, removing its qualities from the Object. */
void Component::RemoveFromOwner()
{
	if (!m_pOwner)
	{
		return;
	}

	CheckedPtr<Object> pOwner(m_pOwner);
	m_pOwner.Reset();

	// IMPORTANT: Owner has a reference counted ownership
	// of this Component, so 'this' may be destroyed
	// after FriendRemoveComponent returns. Don't access
	// any members of Component after FriendRemoveComponent
	// returns.
	pOwner->FriendRemoveComponent(this);
}

} // namespace Scene

} // namespace Seoul

#endif // /#if SEOUL_WITH_SCENE
