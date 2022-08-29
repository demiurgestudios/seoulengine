/**
 * \file SceneMeshDrawComponent.cpp
 * \brief Binds a Mesh (a collection of drawable triangle
 * primitives) into a Scene object.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "AssetManager.h"
#include "Frustum.h"
#include "Mesh.h"
#include "ReflectionDefine.h"
#include "SceneAnimation3DComponent.h"
#include "SceneMeshDrawComponent.h"
#include "SceneMeshDrawFlags.h"
#include "SceneMeshRenderer.h"
#include "SceneObject.h"

#if SEOUL_WITH_SCENE

namespace Seoul
{

SEOUL_BEGIN_TYPE(Scene::MeshDrawComponent, TypeFlags::kDisableCopy)
	SEOUL_DEV_ONLY_ATTRIBUTE(Category, "Drawing")
	SEOUL_DEV_ONLY_ATTRIBUTE(DisplayName, "Mesh")
	SEOUL_PARENT(Scene::Component)
#if SEOUL_EDITOR_AND_TOOLS
	SEOUL_PROPERTY_N_EXT("MeshExtents", EditorGetMeshExtents)
		SEOUL_ATTRIBUTE(DoNotSerialize)
		SEOUL_ATTRIBUTE(Description, "Local (prior to scale) extents of the mesh's AABB.")
#endif // /#if SEOUL_EDITOR_AND_TOOLS
	SEOUL_PROPERTY_PAIR_N("MeshFilePath", GetMeshFilePath, SetMeshFilePath)
		SEOUL_DEV_ONLY_ATTRIBUTE(EditorFileSpec, GameDirectory::kContent, FileType::kSceneAsset)
		SEOUL_DEV_ONLY_ATTRIBUTE(Description, "Autodesk (*.fbx) file that provides the geometry and material.")
	SEOUL_PROPERTY_N("Scale", m_vScale)
		SEOUL_ATTRIBUTE(NotRequired)
		SEOUL_DEV_ONLY_ATTRIBUTE(Description, "Draw scale applied to the mesh. Visual only, does not affect attached objects.")
	SEOUL_PROPERTY_PAIR_N("InfiniteDepth", GetInfiniteDepth, SetInfiniteDepth)
		SEOUL_ATTRIBUTE(NotRequired)
		SEOUL_DEV_ONLY_ATTRIBUTE(Description,
			"Mesh depth will be projected to 'infinity' (to the far plane),\n"
			"effectively disabling depth testing and depth clipping.")
	SEOUL_PROPERTY_PAIR_N("Sky", GetSky, SetSky)
		SEOUL_ATTRIBUTE(NotRequired)
		SEOUL_DEV_ONLY_ATTRIBUTE(Description,
			"Mesh will be rendered at the origin (ignore its world transform)\n"
			"and projected to infinity.")
SEOUL_END_TYPE()

namespace Scene
{

MeshDrawComponent::MeshDrawComponent()
	: m_hMesh()
	, m_vScale(1, 1, 1)
	, m_uMeshDrawFlags(0u)
	, m_bVisible(true)
{
}

MeshDrawComponent::~MeshDrawComponent()
{
}

SharedPtr<Component> MeshDrawComponent::Clone(const String& sQualifier) const
{
	SharedPtr<MeshDrawComponent> pReturn(SEOUL_NEW(MemoryBudgets::SceneComponent) MeshDrawComponent);
	pReturn->m_hMesh = m_hMesh;
	pReturn->m_vScale = m_vScale;
	pReturn->m_uMeshDrawFlags = m_uMeshDrawFlags;
	pReturn->m_bVisible = m_bVisible;
	return pReturn;
}

Bool MeshDrawComponent::Render(
	const Frustum& frustum,
	MeshRenderer& rRenderer,
	HString techniqueOverride /*= HString()*/,
	HString skinnedTechniqueOverride /*= HString()*/)
{
	// Early out if we're not visible.
	if (!m_bVisible)
	{
		return false;
	}

	SharedPtr<Mesh> pMesh = GetMeshPtr(m_hMesh);
	if (!pMesh.IsValid())
	{
		return false;
	}

	// TODO: Cache full world transform in MeshDrawComponent
	// after dirty.
	Matrix4D const mWorldTransform(
		(GetOwner().IsValid() ? GetOwner()->ComputeNormalTransform() : Matrix4D::Identity()) *
		Matrix4D::CreateScale(m_vScale));

	// Don't cull sky meshes or infinite depth meshes.
	if (0u == ((MeshDrawFlags::kSky | MeshDrawFlags::kInfiniteDepth) & m_uMeshDrawFlags))
	{
		AABB const worldAABB = AABB::Transform(mWorldTransform, pMesh->GetBoundingBox());
		if (FrustumTestResult::kDisjoint == frustum.Intersects(worldAABB))
		{
			return false;
		}
	}

	// TODO: Cache and optimize.
	SharedPtr<Animation3DComponent> pAnimationComponent(GetOwner()->GetComponent<Animation3DComponent>());
	if (pAnimationComponent.IsValid() && nullptr != pAnimationComponent->GetNetworkInstance())
	{
		return rRenderer.DrawAnimatedMesh(
			m_uMeshDrawFlags,
			mWorldTransform,
			pMesh,
			*pAnimationComponent->GetNetworkInstance(),
			skinnedTechniqueOverride);
	}
	else
	{
		return rRenderer.DrawMesh(
			m_uMeshDrawFlags,
			mWorldTransform,
			pMesh,
			techniqueOverride);
	}
}

void MeshDrawComponent::SetMeshFilePath(FilePath filePath)
{
	m_hMesh = AssetManager::Get()->GetAsset(filePath);
}

#if SEOUL_EDITOR_AND_TOOLS
Vector3D MeshDrawComponent::EditorGetMeshExtents() const
{
	auto pMesh(GetMeshPtr(m_hMesh));
	if (!pMesh.IsValid())
	{
		return Vector3D::Zero();
	}

	return pMesh->GetBoundingBox().GetExtents();
}
#endif // /#if SEOUL_EDITOR_AND_TOOLS

} // namespace Scene

} // namespace Seoul

#endif // /#if SEOUL_WITH_SCENE
