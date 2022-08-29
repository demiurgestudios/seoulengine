/**
 * \file SceneAnimation3DComponent.cpp
 * \brief Binds a 3D animation rig into a Scene object.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "AnimationEventInterface.h"
#include "Animation3DManager.h"
#include "AnimationNetworkDefinitionManager.h"
#include "Animation3DNetworkInstance.h"
#include "Mesh.h"
#include "ReflectionDefine.h"
#include "SceneAnimation3DComponent.h"
#include "SceneMeshDrawComponent.h"
#include "SceneObject.h"

#if SEOUL_WITH_ANIMATION_3D && SEOUL_WITH_SCENE

namespace Seoul
{

SEOUL_BEGIN_TYPE(Scene::Animation3DComponent, TypeFlags::kDisableCopy)
	SEOUL_DEV_ONLY_ATTRIBUTE(DisplayName, "Animation 3D")
	SEOUL_DEV_ONLY_ATTRIBUTE(Category, "Animation")
	SEOUL_PARENT(Scene::Component)
	SEOUL_PROPERTY_PAIR_N("NetworkFilePath", GetNetworkFilePath, SetNetworkFilePath)
		SEOUL_DEV_ONLY_ATTRIBUTE(Description, "SeoulEngine Animation Network in a .json file that defines the blending and states of this AnimationComponent.")
		SEOUL_DEV_ONLY_ATTRIBUTE(EditorFileSpec, GameDirectory::kConfig, FileType::kJson)
SEOUL_END_TYPE()

namespace Scene
{

Animation3DComponent::Animation3DComponent()
	: m_NetworkFilePath()
	, m_pNetworkInstance()
{
}

Animation3DComponent::~Animation3DComponent()
{
}

SharedPtr<Component> Animation3DComponent::Clone(const String& sQualifier) const
{
	// Generate the clone and shallow copy appropriate parameters.
	SharedPtr<Animation3DComponent> pReturn(SEOUL_NEW(MemoryBudgets::SceneComponent) Animation3DComponent);
	pReturn->m_NetworkFilePath = m_NetworkFilePath;
	pReturn->m_pNetworkInstance.Reset(m_pNetworkInstance.IsValid() ? static_cast<Animation3D::NetworkInstance*>(m_pNetworkInstance->Clone()) : nullptr);
	return pReturn;
}

FilePath Animation3DComponent::GetNetworkFilePath() const
{
	return m_NetworkFilePath;
}

void Animation3DComponent::SetNetworkFilePath(FilePath filePath)
{
	if (filePath != m_NetworkFilePath)
	{
		m_NetworkFilePath = filePath;
		m_pNetworkInstance.Reset();
	}
}

void Animation3DComponent::Tick(Float fDeltaTimeInSeconds)
{
	Prep();

	// Early out if no network.
	if (!m_pNetworkInstance.IsValid())
	{
		return;
	}

	m_pNetworkInstance->Tick(fDeltaTimeInSeconds);
}

const SharedPtr<Animation::EventInterface>& Animation3DComponent::GetEventInterface() const
{
	return m_pEventInterface;
}

void Animation3DComponent::SetEventInterface(const SharedPtr<Animation::EventInterface>& pEventInterface)
{
	m_pEventInterface = pEventInterface;
}

void Animation3DComponent::Prep()
{
	// Check for configuration and potentially reset.
	{
		SharedPtr<MeshDrawComponent> pMeshDrawComponent(GetOwner()->GetComponent<MeshDrawComponent>());
		SharedPtr<Mesh> pMesh(GetMeshPtr(pMeshDrawComponent.IsValid() ? pMeshDrawComponent->GetMesh() : AssetContentHandle()));
		if (m_NetworkFilePath.IsValid() && pMeshDrawComponent.IsValid() && pMesh.IsValid())
		{
			auto const oldDataFilePath = (m_pNetworkInstance.IsValid() ? m_pNetworkInstance->GetDataHandle().GetKey() : FilePath());
			auto const newDataFilePath = pMeshDrawComponent->GetMesh().GetKey();
			if (newDataFilePath != oldDataFilePath)
			{
				m_pNetworkInstance.Reset();
				if (newDataFilePath.IsValid())
				{
					m_pNetworkInstance = Animation3D::Manager::Get()->CreateInstance(
						m_NetworkFilePath,
						newDataFilePath,
						m_pEventInterface,
						pMesh->GetInverseBindPoses());
				}
			}
		}
		else
		{
			m_pNetworkInstance.Reset();
		}
	}
}

} // namespace Scene

} // namespace Seoul

#endif // /#if SEOUL_WITH_ANIMATION_3D && SEOUL_WITH_SCENE
