/**
 * \file SceneRigidBodyComponent.h
 * \brief Binds an instance with a a physical bounds and
 * behavior into a 3D scene.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCENE_RIGID_BODY_COMPONENT_H
#define SCENE_RIGID_BODY_COMPONENT_H

#include "PhysicsBodyDef.h"
#include "SceneComponent.h"
#include "SpatialId.h"
namespace Seoul { namespace Physics { class Body; } }
namespace Seoul { namespace Scene { class PrimitiveRenderer; } }

#if SEOUL_WITH_PHYSICS && SEOUL_WITH_SCENE

namespace Seoul::Scene
{

class RigidBodyComponent SEOUL_SEALED : public SetTransformComponent
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(RigidBodyComponent);

	RigidBodyComponent();
	~RigidBodyComponent();

	// Component overrides.
	virtual SharedPtr<Component> Clone(const String& sQualifier) const SEOUL_OVERRIDE;
	// /Component overrides

	// SetTransformComponent overrides.
	virtual Quaternion GetRotation() const SEOUL_OVERRIDE;
	virtual Vector3D GetPosition() const SEOUL_OVERRIDE;
	virtual void SetRotation(const Quaternion& qRotation) SEOUL_OVERRIDE;
	virtual void SetPosition(const Vector3D& vPosition) SEOUL_OVERRIDE;
	// /SetTransformComponent overrides.

	const Physics::BodyDef& GetBodyDef() const { return m_BodyDef; }

	/** true for subclasses that need an OnGroupInstantiateComplete call. */
	virtual Bool NeedsOnGroupInstantiateComplete() const SEOUL_OVERRIDE { return true; }

	/** Called on an object that has been cloned from a prefab, after the entire prefab has been instantiated. */
	virtual void OnGroupInstantiateComplete(Interface& rInterface) SEOUL_OVERRIDE;

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(RigidBodyComponent);

	SEOUL_REFLECTION_FRIENDSHIP(RigidBodyComponent);
	Physics::BodyDef m_BodyDef;
	SharedPtr<Physics::Body> m_pBody;
	Bool m_bInheritScale;

#if SEOUL_EDITOR_AND_TOOLS
	// Button hook in the editor.
	void EditorAutoFitCollision(Interface* pInterface);

	// Hooks for the editor.
	void EditorDrawPrimitives(PrimitiveRenderer* pRenderer) const;

	/**
	 * Editor only euler angle support. Quats at runtime to avoid
	 * gimbal lock, euler angles at editor time for understandable
	 * editing.
	 */
	Vector3D EditorGetEulerRotation() const;
	void EditorSetEulerRotation(Vector3D vInDegrees);
	Vector3D m_vEulerRotation;
#endif // /#if SEOUL_EDITOR_AND_TOOLS

	// Access for reflection.
	static Vector3D& ReflectionGetPosition(RigidBodyComponent& r) { return r.m_BodyDef.m_vPosition; }
	static Quaternion& ReflectionGetRotation(RigidBodyComponent& r) { return r.m_BodyDef.m_qOrientation; }
	static Physics::ShapeDef& ReflectionGetShape(RigidBodyComponent& r) { return r.m_BodyDef.m_Shape; }
	static Physics::BodyType& ReflectionGetType(RigidBodyComponent& r) { return r.m_BodyDef.m_eType; }

	SEOUL_DISABLE_COPY(RigidBodyComponent);
}; // class RigidBodyComponent

} // namespace Seoul::Scene

#endif // /#if SEOUL_WITH_PHYSICS && SEOUL_WITH_SCENE

#endif // include guard
