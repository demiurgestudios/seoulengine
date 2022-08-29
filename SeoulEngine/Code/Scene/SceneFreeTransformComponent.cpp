/**
 * \file SceneFreeTransformComponent.cpp
 * \brief Transform component, get and set position and rotation.
 *
 * FreeTransformComponent provides a mutable transform.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ReflectionDefine.h"
#include "SceneFreeTransformComponent.h"
#include "SceneEditorUtil.h"

#if SEOUL_WITH_SCENE

namespace Seoul
{

SEOUL_BEGIN_TYPE(Scene::FreeTransformComponent, TypeFlags::kDisableCopy)
	SEOUL_DEV_ONLY_ATTRIBUTE(Category, "Transform")
	SEOUL_DEV_ONLY_ATTRIBUTE(DisplayName, "Free Transform")
	SEOUL_DEV_ONLY_ATTRIBUTE(EditorDefaultExpanded)
	SEOUL_PARENT(Scene::SetTransformComponent)
	SEOUL_PROPERTY_N("Position", m_vPosition)
		SEOUL_ATTRIBUTE(NotRequired)
		SEOUL_DEV_ONLY_ATTRIBUTE(Description, "Absolute translation in meters.")
	SEOUL_PROPERTY_N("Rotation", m_qRotation)
		SEOUL_ATTRIBUTE(DoNotEdit)
		SEOUL_ATTRIBUTE(NotRequired)

#if SEOUL_EDITOR_AND_TOOLS
	SEOUL_PROPERTY_PAIR_N("RotationInDegrees", EditorGetEulerRotation, EditorSetEulerRotation)
		SEOUL_ATTRIBUTE(DoNotSerialize)
		SEOUL_DEV_ONLY_ATTRIBUTE(Description, "Orientation in degrees (pitch, yaw, roll).")
		SEOUL_DEV_ONLY_ATTRIBUTE(DisplayName, "Rotation")
#endif // /#if SEOUL_EDITOR_AND_TOOLS
SEOUL_END_TYPE()

namespace Scene
{

FreeTransformComponent::FreeTransformComponent()
	: m_qRotation(Quaternion::Identity())
	, m_vPosition(Vector3D::Zero())
#if SEOUL_EDITOR_AND_TOOLS
	, m_vEulerRotation(Vector3D::Zero())
#endif // /#if SEOUL_EDITOR_AND_TOOLS
{
}

FreeTransformComponent::~FreeTransformComponent()
{
}

SharedPtr<Component> FreeTransformComponent::Clone(const String& sQualifier) const
{
	SharedPtr<FreeTransformComponent> pReturn(SEOUL_NEW(MemoryBudgets::SceneComponent) FreeTransformComponent);
	pReturn->m_qRotation = m_qRotation;
	pReturn->m_vPosition = m_vPosition;
#if SEOUL_EDITOR_AND_TOOLS
	pReturn->m_vEulerRotation = m_vEulerRotation;
#endif // /#if SEOUL_EDITOR_AND_TOOLS

	return pReturn;
}

#if SEOUL_EDITOR_AND_TOOLS
Vector3D FreeTransformComponent::EditorGetEulerRotation() const
{
	return GetEulerDegrees(m_vEulerRotation, GetRotation());
}

void FreeTransformComponent::EditorSetEulerRotation(Vector3D vInDegrees)
{
	SetEulerDegrees(vInDegrees, m_vEulerRotation, m_qRotation);
}
#endif // /#if SEOUL_EDITOR_AND_TOOLS

} // namespace Scene

} // namespace Seoul

#endif // /#if SEOUL_WITH_SCENE
