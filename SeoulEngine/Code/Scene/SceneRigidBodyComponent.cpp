/**
 * \file SceneRigidBodyComponent.cpp
 * \brief Binds an instance with a a physical bounds and
 * behavior into a 3D scene.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Mesh.h"
#include "PhysicsBody.h"
#include "PhysicsSimulator.h"
#include "ReflectionDefine.h"
#include "SceneEditorUtil.h"
#include "SceneInterface.h"
#include "SceneObject.h"
#include "SceneMeshDrawComponent.h"
#include "ScenePrimitiveRenderer.h"
#include "SceneRigidBodyComponent.h"
#include "StackOrHeapArray.h"

#if SEOUL_WITH_PHYSICS && SEOUL_WITH_SCENE

namespace Seoul
{

SEOUL_BEGIN_TYPE(Scene::RigidBodyComponent, TypeFlags::kDisableCopy)
	SEOUL_DEV_ONLY_ATTRIBUTE(DisplayName, "Rigid Body")
	SEOUL_DEV_ONLY_ATTRIBUTE(Category, "Transform")
	SEOUL_DEV_ONLY_ATTRIBUTE(EditorDefaultExpanded)
	SEOUL_PARENT(Scene::SetTransformComponent)
	SEOUL_PROPERTY_N_EXT("Type", ReflectionGetType)
		SEOUL_DEV_ONLY_ATTRIBUTE(Description, "Simulation type of the body.")
	SEOUL_PROPERTY_N_EXT("Position", ReflectionGetPosition)
		SEOUL_ATTRIBUTE(NotRequired)
		SEOUL_DEV_ONLY_ATTRIBUTE(Description, "Absolute translation in meters.")
	SEOUL_PROPERTY_N_EXT("Rotation", ReflectionGetRotation)
		SEOUL_ATTRIBUTE(DoNotEdit)
		SEOUL_ATTRIBUTE(NotRequired)
		SEOUL_DEV_ONLY_ATTRIBUTE(Description, "Orientation in degrees (pitch, yaw, roll).")

#if SEOUL_EDITOR_AND_TOOLS
	SEOUL_METHOD(EditorDrawPrimitives)
	SEOUL_PROPERTY_PAIR_N("RotationInDegrees", EditorGetEulerRotation, EditorSetEulerRotation)
		SEOUL_ATTRIBUTE(DoNotSerialize)
		SEOUL_DEV_ONLY_ATTRIBUTE(Description, "Orientation in degrees (pitch, yaw, roll).")
		SEOUL_DEV_ONLY_ATTRIBUTE(DisplayName, "Rotation")

	SEOUL_METHOD_N("Auto Fit Collision", EditorAutoFitCollision)
		SEOUL_ATTRIBUTE(EditorButton, "Shape")
#endif // /#if SEOUL_EDITOR_AND_TOOLS

	SEOUL_PROPERTY_N_EXT("InheritScale", m_bInheritScale)
		SEOUL_ATTRIBUTE(NotRequired)
		SEOUL_DEV_ONLY_ATTRIBUTE(Description,
			"When true, and when a Mesh Draw Component is also attached,\n"
			"the scale of the mesh will be applied to the collision shape.\n"
			"This only applies at initial shape creation (changes to the scale\n"
			"at runtime will *not* update the scale of the collision shape).")
	SEOUL_PROPERTY_N_EXT("Shape", ReflectionGetShape)
SEOUL_END_TYPE()

namespace Scene
{

RigidBodyComponent::RigidBodyComponent()
	: m_BodyDef()
	, m_pBody()
	, m_bInheritScale(true)
#if SEOUL_EDITOR_AND_TOOLS
	, m_vEulerRotation(Vector3D::Zero())
#endif // /#if SEOUL_EDITOR_AND_TOOLS
{
}

RigidBodyComponent::~RigidBodyComponent()
{
}

SharedPtr<Component> RigidBodyComponent::Clone(const String& sQualifier) const
{
	SharedPtr<RigidBodyComponent> pReturn(SEOUL_NEW(MemoryBudgets::SceneComponent) RigidBodyComponent);
	pReturn->m_BodyDef = m_BodyDef;
	pReturn->m_bInheritScale = m_bInheritScale;
#if SEOUL_EDITOR_AND_TOOLS
	pReturn->m_vEulerRotation = m_vEulerRotation;
#endif // /#if SEOUL_EDITOR_AND_TOOLS

	return pReturn;
}

Quaternion RigidBodyComponent::GetRotation() const
{
	return (m_pBody.IsValid() ? m_pBody->GetOrientation() : m_BodyDef.m_qOrientation);
}

Vector3D RigidBodyComponent::GetPosition() const
{
	return (m_pBody.IsValid() ? m_pBody->GetPosition() : m_BodyDef.m_vPosition);
}

void RigidBodyComponent::SetRotation(const Quaternion& qRotation)
{
	// TODO: SetTransform is costly - so we really only want
	// to call it once, not potentially twice if the caller
	// is manipulating both rotation and position.

	// TODO: Do we ever want to call SetTransform()
	// with bWake = false ?
	if (m_pBody.IsValid()) { m_pBody->SetTransform(GetPosition(), qRotation, true); }
	else { m_BodyDef.m_qOrientation = qRotation; }
}

void RigidBodyComponent::SetPosition(const Vector3D& vPosition)
{
	// TODO: SetTransform is costly - so we really only want
	// to call it once, not potentially twice if the caller
	// is manipulating both rotation and position.

	// TODO: Do we ever want to call SetTransform()
	// with bWake = false ?
	if (m_pBody.IsValid()) { m_pBody->SetTransform(vPosition, GetRotation(), true); }
	else { m_BodyDef.m_vPosition = vPosition; }
}

void RigidBodyComponent::OnGroupInstantiateComplete(Interface& rInterface)
{
	// On completion, create our physics body, if the interface
	// has a physics simulator.
	auto pSimulator = rInterface.GetPhysicsSimulator();
	if (nullptr == pSimulator)
	{
		// Early out if no simulator.
		return;
	}

	Vector3D vInitialScale(Vector3D::One());
	if (m_bInheritScale)
	{
		auto pMesh(GetOwner()->GetComponent<Scene::MeshDrawComponent>());
		if (pMesh.IsValid())
		{
			vInitialScale = pMesh->GetScale();
		}
	}

	// TODO: Need to formalize when we do and do not
	// pass a handle through to the body across the board.
	// This is motivated by the desired to be able to eliminate
	// the overhead of Objects in certain cases (e.g. the static
	// level geometry of a scene should be merged into a smaller
	// number of Objects).
	void* pUserData = nullptr;
	if (m_BodyDef.m_Shape.m_bSensor || m_BodyDef.m_eType != Physics::BodyType::kStatic)
	{
		pUserData = SceneObjectHandle::ToVoidStar(GetOwner()->AcquireHandle());
	}

	// Fallback, creation without scale.
	m_pBody = pSimulator->CreateBody(m_BodyDef, vInitialScale, pUserData);
}

#if SEOUL_EDITOR_AND_TOOLS
void RigidBodyComponent::EditorAutoFitCollision(Interface* pInterface)
{
	using namespace Physics;

	auto pMeshComponent(GetOwner()->GetComponent<Scene::MeshDrawComponent>());
	if (!pMeshComponent.IsValid())
	{
		// Nop if no mesh to fit to.
		return;
	}

	auto pMesh(GetMeshPtr(pMeshComponent->GetMesh()));
	if (!pMesh.IsValid())
	{
		// Nop if no mesh to fit to.
		return;
	}

	// Different fitting algorithms depending on shape type.
	auto& shape = m_BodyDef.m_Shape;
	switch (shape.GetType())
	{
	case ShapeType::kBox:
	{
		auto pBox = shape.GetData<Physics::BoxShapeData>();

		auto const aabb = pMesh->GetBoundingBox();
		pBox->m_vCenter = aabb.GetCenter();
		pBox->m_vExtents = aabb.GetExtents();
	}
	break;
	case ShapeType::kCapsule:
	{
		// TODO: Should fit to original points, not AABB.

		auto pCapsule = shape.GetData<Physics::CapsuleShapeData>();

		auto const aabb = pMesh->GetBoundingBox();
		auto const vCenter = aabb.GetCenter();
		auto vExtents = aabb.GetExtents();
		auto const fComponent = vExtents.GetMaxComponent();
		auto const eComponent = (fComponent == vExtents.X ? 0 : (fComponent == vExtents.Y ? 1 : 2));

		Vector3D vExpansion;
		vExpansion[eComponent] = vExtents[eComponent];
		pCapsule->m_vP0 = vCenter - vExpansion;
		pCapsule->m_vP1 = vCenter + vExpansion;

		Vector3D vRadius = vExtents;
		vRadius[eComponent] = 0.0f;

		pCapsule->m_fRadius = vRadius.Length();
	}
	break;
	case ShapeType::kConvexHull:
	{
		auto pConvexHull = shape.GetData<Physics::ConvexHullShapeData>();

		pConvexHull->CalculateFromPoints(pMesh->EditorGetVerticies().Begin(), pMesh->EditorGetVerticies().End());
	}
	break;
	case ShapeType::kSphere:
	{
		// TODO: Should fit to original points, not AABB.

		auto pSphere = shape.GetData<Physics::SphereShapeData>();

		auto const aabb = pMesh->GetBoundingBox();
		pSphere->m_vCenter = aabb.GetCenter();
		pSphere->m_fRadius = aabb.GetExtents().Length();
	}
	break;
	default:
		SEOUL_FAIL("Out-of-sync enum.");
		break;
	}
}

void RigidBodyComponent::EditorDrawPrimitives(PrimitiveRenderer* pRenderer) const
{
	static const ColorARGBu8 cColor(ColorARGBu8::CreateFromFloat(0.5f, 0.5f, 0.5f, 0.7f)); /* TODO: */

	auto const mNormal(GetOwner()->ComputeNormalTransform());

	Vector3D vScale(Vector3D::One());
	if (m_bInheritScale)
	{
		auto pMesh(GetOwner()->GetComponent<Scene::MeshDrawComponent>());
		if (pMesh.IsValid())
		{
			vScale = pMesh->GetScale();
		}
	}

	auto const& shape = m_BodyDef.m_Shape;
	switch (shape.GetType())
	{
	case Physics::ShapeType::kBox:
	{
		auto pBox = shape.GetData<Physics::BoxShapeData>();

		Physics::BoxShapeData data;
		pBox->ComputeScaled(vScale, data);

		pRenderer->TriangleBox(
			mNormal * Matrix4D::CreateTranslation(data.m_vCenter),
			data.m_vExtents,
			cColor);
	}
	break;

	case Physics::ShapeType::kCapsule:
	{
		auto pCapsule = shape.GetData<Physics::CapsuleShapeData>();

		Physics::CapsuleShapeData data;
		pCapsule->ComputeScaled(vScale, data);

		pRenderer->TriangleCapsule(
			Matrix4D::TransformPosition(mNormal, data.m_vP0),
			Matrix4D::TransformPosition(mNormal, data.m_vP1),
			data.m_fRadius,
			16, /* TODO: */
			true,
			cColor);
	}
	break;

	case Physics::ShapeType::kConvexHull:
	{
		vScale = Physics::IShapeData::SanitizeScale(vScale);

		auto pConvexHull = shape.GetData<Physics::ConvexHullShapeData>();

		StackOrHeapArray<Vector3D, 16u, MemoryBudgets::Physics> a(pConvexHull->GetPoints().GetSize());

		// Transfer and transform points.
		UInt32 const uPoints = pConvexHull->GetPoints().GetSize();
		for (UInt32 i = 0u; i < uPoints; ++i)
		{
			a[i] = Matrix4D::TransformPosition(
				mNormal,
				Vector3D::ComponentwiseMultiply(vScale, pConvexHull->GetPoints()[i]));
		}

		// Enumerate and render.
		UInt32 const uIndices = pConvexHull->GetIndices().GetSize();
		UInt32 uIndex = 0u;
		while (uIndex < uIndices)
		{
			UInt32 const uFaceIndexCount = pConvexHull->GetIndices()[uIndex++];
			UInt32 const uFaceCount = pConvexHull->GetIndices()[uIndex++];

			// Sanity check so we don't crash - means bad data though.
			if (uFaceCount * uFaceIndexCount > uIndices)
			{
				break;
			}

			for (UInt32 uFace = 0u; uFace < uFaceCount; ++uFace)
			{
				UInt8 uBaseIndex = pConvexHull->GetIndices()[uIndex];
				for (UInt32 u = uIndex + 2u; u < uIndex + uFaceIndexCount; ++u)
				{
					pRenderer->Triangle(
						a[uBaseIndex],
						a[pConvexHull->GetIndices()[u - 1]],
						a[pConvexHull->GetIndices()[u - 0]],
						cColor);
				}
				uIndex += uFaceIndexCount;
			}
		}
	}
	break;

	case Physics::ShapeType::kSphere:
	{
		auto pSphere = shape.GetData<Physics::SphereShapeData>();

		Physics::SphereShapeData data;
		pSphere->ComputeScaled(vScale, data);

		pRenderer->TriangleSphere(
			Matrix4D::TransformPosition(mNormal, data.m_vCenter),
			data.m_fRadius,
			16, /* TODO: */
			true,
			cColor);
	}
	break;

	case Physics::ShapeType::kNone:
	default:
		break;
	};
}

Vector3D RigidBodyComponent::EditorGetEulerRotation() const
{
	return GetEulerDegrees(m_vEulerRotation, GetRotation());
}

void RigidBodyComponent::EditorSetEulerRotation(Vector3D vInDegrees)
{
	Quaternion q(GetRotation());
	SetEulerDegrees(vInDegrees, m_vEulerRotation, q);
	SetRotation(q);
}

#endif // /#if SEOUL_EDITOR_AND_TOOLS

} // namespace Scene

} // namespace Seoul

#endif // /#if SEOUL_WITH_PHYSICS && SEOUL_WITH_SCENE
