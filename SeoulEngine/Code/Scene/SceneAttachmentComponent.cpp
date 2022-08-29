/**
 * \file SceneAttachmentComponent.cpp
 * \brief Spatial position, parented to another object.
 *
 * AttachmentComponent is similar to FreeTransformComponent,
 * except a parent Object can be defined, which acts as a transform parent
 * to the current object.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Logger.h"
#include "ReflectionDefine.h"
#include "SceneAttachmentComponent.h"
#include "SceneEditorUtil.h"
#include "SceneInterface.h"
#include "SceneObject.h"

#if SEOUL_WITH_SCENE

namespace Seoul
{

SEOUL_BEGIN_TYPE(Scene::AttachmentComponent, TypeFlags::kDisableCopy)
	SEOUL_DEV_ONLY_ATTRIBUTE(Category, "Transform")
	SEOUL_DEV_ONLY_ATTRIBUTE(DisplayName, "Attachment")
	SEOUL_DEV_ONLY_ATTRIBUTE(EditorDefaultExpanded)
	SEOUL_PARENT(Scene::GetTransformComponent)
	SEOUL_PROPERTY_N("RelativeRotation", m_qRelativeRotation)
		SEOUL_ATTRIBUTE(DoNotEdit)
		SEOUL_ATTRIBUTE(NotRequired)

#if SEOUL_EDITOR_AND_TOOLS
	SEOUL_PROPERTY_PAIR_N("RelativeRotationInDegrees", EditorGetEulerRelativeRotation, EditorSetEulerRelativeRotation)
		SEOUL_ATTRIBUTE(DoNotSerialize)
		SEOUL_DEV_ONLY_ATTRIBUTE(Description, "Relative Orientation in degrees (pitch, yaw, roll).")
		SEOUL_DEV_ONLY_ATTRIBUTE(DisplayName, "RelativeRotation")
#endif // /#if SEOUL_EDITOR_AND_TOOLS

	SEOUL_PROPERTY_N("RelativePosition", m_vRelativePosition)
		SEOUL_ATTRIBUTE(NotRequired)
	SEOUL_PROPERTY_N("ParentId", m_sParentId)
		SEOUL_ATTRIBUTE(NotRequired)
SEOUL_END_TYPE()

namespace Scene
{

AttachmentComponent::AttachmentComponent()
	: m_qRelativeRotation(Quaternion::Identity())
	, m_vRelativePosition(Vector3D::Zero())
	, m_pParent(nullptr)
	, m_sParentId()
#if SEOUL_EDITOR_AND_TOOLS
	, m_vEulerRelativeRotation(Vector3D::Zero())
#endif // /#if SEOUL_EDITOR_AND_TOOLS
{
}

AttachmentComponent::~AttachmentComponent()
{
}

SharedPtr<Component> AttachmentComponent::Clone(const String& sQualifier) const
{
	// Generate the clone and shallow copy appropriate parameters.
	SharedPtr<AttachmentComponent> pReturn(SEOUL_NEW(MemoryBudgets::SceneComponent) AttachmentComponent);
	pReturn->m_qRelativeRotation = m_qRelativeRotation;
	pReturn->m_vRelativePosition = m_vRelativePosition;

	// Qualify the id to the object's parent id.
	pReturn->m_sParentId = m_sParentId;
	Object::QualifyId(sQualifier, pReturn->m_sParentId);

#if SEOUL_EDITOR_AND_TOOLS
	pReturn->m_vEulerRelativeRotation = m_vEulerRelativeRotation;
#endif // /#if SEOUL_EDITOR_AND_TOOLS

	return pReturn;
}

Quaternion AttachmentComponent::GetRotation() const
{
	return (m_pParent.IsValid()
		? (m_pParent->GetRotation() * m_qRelativeRotation)
		: m_qRelativeRotation);
}

Vector3D AttachmentComponent::GetPosition() const
{
	return (m_pParent.IsValid()
		? (Quaternion::Transform(m_pParent->GetRotation(), m_vRelativePosition) + m_pParent->GetPosition())
		: m_vRelativePosition);
}

void AttachmentComponent::OnGroupInstantiateComplete(Interface& rInterface)
{
	Attach(rInterface);
}

void AttachmentComponent::Attach(Interface& rInterface)
{
	if (!m_sParentId.IsEmpty())
	{
		if (!rInterface.GetObjectById(m_sParentId, m_pParent))
		{
			// This is an error case.
			SEOUL_WARN("AttachmentComponent of Object %s has parent %s, which could "
				"not be resolved.",
				GetOwner().IsValid() ? GetOwner()->GetId().CStr() : "<unknown>",
				m_sParentId.CStr());
		}
	}
}

#if SEOUL_EDITOR_AND_TOOLS
Vector3D AttachmentComponent::EditorGetEulerRelativeRotation() const
{
	return GetEulerDegrees(m_vEulerRelativeRotation, m_qRelativeRotation);
}

void AttachmentComponent::EditorSetEulerRelativeRotation(Vector3D vInDegrees)
{
	SetEulerDegrees(vInDegrees, m_vEulerRelativeRotation, m_qRelativeRotation);
}

#endif // /#if SEOUL_EDITOR_AND_TOOLS

} // namespace Scene

} // namespace Seoul

#endif // /#if SEOUL_WITH_SCENE
