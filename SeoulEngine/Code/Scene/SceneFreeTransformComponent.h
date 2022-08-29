/**
 * \file SceneFreeTransformComponent.h
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

#pragma once
#ifndef SCENE_FREE_TRANSFORM_COMPONENT_H
#define SCENE_FREE_TRANSFORM_COMPONENT_H

#include "Quaternion.h"
#include "SceneSetTransformComponent.h"
#include "Vector3D.h"

#if SEOUL_WITH_SCENE

namespace Seoul::Scene
{

class FreeTransformComponent SEOUL_SEALED : public SetTransformComponent
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(FreeTransformComponent);

	FreeTransformComponent();
	~FreeTransformComponent();

	// Component overrides.
	virtual SharedPtr<Component> Clone(const String& sQualifier) const SEOUL_OVERRIDE;
	// /Component overrides.

	// SetTransformComponent overrides.
	virtual Quaternion GetRotation() const SEOUL_OVERRIDE
	{
		return m_qRotation;
	}

	virtual Vector3D GetPosition() const SEOUL_OVERRIDE
	{
		return m_vPosition;
	}

	virtual void SetRotation(const Quaternion& qRotation) SEOUL_OVERRIDE
	{
		m_qRotation = qRotation;
	}

	virtual void SetPosition(const Vector3D& vPosition) SEOUL_OVERRIDE
	{
		m_vPosition = vPosition;
	}
	// /SetTransformComponent overrides.

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(FreeTransformComponent);
	SEOUL_REFLECTION_FRIENDSHIP(FreeTransformComponent);

	Quaternion m_qRotation;
	Vector3D m_vPosition;

#if SEOUL_EDITOR_AND_TOOLS
	/**
	* Editor only euler angle support. Quats at runtime to avoid
	* gimbal lock, euler angles at editor time for understandable
	* editing.
	*/
	Vector3D EditorGetEulerRotation() const;
	void EditorSetEulerRotation(Vector3D vInDegrees);
	Vector3D m_vEulerRotation;
#endif // /#if SEOUL_EDITOR_AND_TOOLS

	SEOUL_DISABLE_COPY(FreeTransformComponent);
}; // class FreeTransformComponent

} // namespace Seoul::Scene

#endif // /#if SEOUL_WITH_SCENE

#endif // include guard
