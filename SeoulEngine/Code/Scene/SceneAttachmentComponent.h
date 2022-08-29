/**
 * \file SceneAttachmentComponent.h
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

#pragma once
#ifndef SCENE_ATTACHMENT_COMPONENT_H
#define SCENE_ATTACHMENT_COMPONENT_H

#if SEOUL_WITH_SCENE

#include "Quaternion.h"
#include "SceneGetTransformComponent.h"
#include "Vector3D.h"
namespace Seoul { namespace Scene { class Interface; } }
namespace Seoul { namespace Scene { class Object; } }

namespace Seoul::Scene
{

class AttachmentComponent SEOUL_SEALED : public GetTransformComponent
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(AttachmentComponent);

	AttachmentComponent();
	~AttachmentComponent();

	virtual SharedPtr<Component> Clone(const String& sQualifier) const SEOUL_OVERRIDE;

	virtual Quaternion GetRotation() const SEOUL_OVERRIDE;
	virtual Vector3D GetPosition() const SEOUL_OVERRIDE;

	virtual Bool NeedsOnGroupInstantiateComplete() const SEOUL_OVERRIDE { return true; }

	virtual void OnGroupInstantiateComplete(Interface& rInterface) SEOUL_OVERRIDE;

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(AttachmentComponent);
	SEOUL_REFLECTION_FRIENDSHIP(AttachmentComponent);

	void Attach(Interface& rInterface);

	Quaternion m_qRelativeRotation;
	Vector3D m_vRelativePosition;
	SharedPtr<Object> m_pParent;
	String m_sParentId;

#if SEOUL_EDITOR_AND_TOOLS
	/**
	 * Editor/tools only euler angle support. Quats at runtime to avoid
	 * gimbal lock, euler angles at editor time for understandable
	 * editing.
	 */
	Vector3D EditorGetEulerRelativeRotation() const;
	void EditorSetEulerRelativeRotation(Vector3D vInDegrees);
	Vector3D m_vEulerRelativeRotation;
#endif // /#if SEOUL_EDITOR_AND_TOOLS

	SEOUL_DISABLE_COPY(AttachmentComponent);
}; // class AttachmentComponent

} // namespace Seoul::Scene

#endif // /#if SEOUL_WITH_SCENE

#endif // include guard
