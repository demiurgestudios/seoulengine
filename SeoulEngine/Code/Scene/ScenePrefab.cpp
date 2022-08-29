/**
 * \file ScenePrefab.cpp
 * \brief A Prefab contains loadable object and component data for
 * representing parts of a 3D scene.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ContentLoadManager.h"
#include "Logger.h"
#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionDefine.h"
#include "SceneObject.h"
#include "ScenePrefab.h"
#include "ScenePrefabContentLoader.h"
#include "ScenePrefabComponent.h"

#if SEOUL_WITH_SCENE

namespace Seoul
{

namespace Scene
{

static const HString kPropertyComponents("Components");
#if SEOUL_EDITOR_AND_TOOLS
static const HString kPropertyEditorCategory("Category");
#endif // /#if SEOUL_EDITOR_AND_TOOLS
static const HString kPropertyId("Id");
static const HString kPropertyObjects("Objects");

// Make sure Scene project types are included by the linker.
SEOUL_LINK_ME(class, Animation3DComponent);
SEOUL_LINK_ME(class, AttachmentComponent);
SEOUL_LINK_ME(class, FreeTransformComponent);
SEOUL_LINK_ME(class, FxComponent);
SEOUL_LINK_ME(class, MeshDrawComponent);
SEOUL_LINK_ME(class, NavigationGridComponent);
SEOUL_LINK_ME(class, PrefabComponent);
SEOUL_LINK_ME(class, RigidBodyComponent);
SEOUL_LINK_ME(class, ScriptComponent);

PrefabTemplate::PrefabTemplate()
	: m_Data()
	, m_vObjects()
{
}

PrefabTemplate::~PrefabTemplate()
{
}

/** Swap r for 'this'. */
void PrefabTemplate::Swap(PrefabTemplate& r)
{
	m_Data.Swap(r.m_Data);
	m_vObjects.Swap(r.m_vObjects);
	m_vPrefabs.Swap(r.m_vPrefabs);
}

Prefab::Prefab()
	: m_pTemplate(SEOUL_NEW(MemoryBudgets::Scene) PrefabTemplate)
{
}

Prefab::~Prefab()
{
}

/** @return True if any nested prefab is still loading. */
Bool Prefab::AreNestedPrefabsLoading() const
{
	const PrefabTemplate::Prefabs& vPrefabs = m_pTemplate->m_vPrefabs;
	UInt32 const uPrefabs = vPrefabs.GetSize();
	for (UInt32 i = 0u; i < uPrefabs; ++i)
	{
		if (vPrefabs[i].m_hPrefab.IsLoading())
		{
			return true;
		}
	}

	return false;
}

UInt32 Prefab::GetMemoryUsageInBytes() const
{
	// TODO: Include memory usage of template objects.
	return m_pTemplate->m_Data.GetTotalMemoryUsageInBytes();
}

Bool Prefab::Load(FilePath sceneFilePath, SyncFile& rFile)
{
	PrefabTemplate t;
	if (!t.m_Data.Load(rFile))
	{
		return false;
	}

	if (!LoadObjects(sceneFilePath, t))
	{
		return false;
	}

	m_pTemplate->Swap(t);
	return true;
}

Bool Prefab::LoadObjects(
	FilePath sceneFilePath,
	PrefabTemplate& rTemplate)
{
	using namespace Seoul::Reflection;

	const DataStore& prefab = rTemplate.m_Data;
	DataNode const rootNode = prefab.GetRootNode();
	DataNode objectsNode;
	if (!prefab.GetValueFromTable(rootNode, kPropertyObjects, objectsNode))
	{
		SEOUL_WARN("%s: scene has not Objects array.", sceneFilePath.CStr());
		return false;
	}

	UInt32 uObjectsCount = 0u;
	if (!prefab.GetArrayCount(objectsNode, uObjectsCount))
	{
		SEOUL_WARN("%s: root is not an array.", sceneFilePath.CStr());
		return false;
	}

	UInt32 uOutObject = 0u;
	rTemplate.m_vObjects.Reserve(uObjectsCount);
	for (UInt32 uObject = 0u; uObject < uObjectsCount; ++uObject)
	{
		DataNode objectNode;
		SEOUL_VERIFY(prefab.GetValueFromArray(objectsNode, uObject, objectNode));

		if (!objectNode.IsTable())
		{
			SEOUL_WARN("%s: object %u has no definition.",
				sceneFilePath.CStr(),
				uObject);
			continue;
		}

		// TODO: Eliminate this unfolding - use Deserialization instead.
		// It's too easy for someone to add a member to Object and not realize
		// this code needs to be updated.

		// Objects are allowed to have an empty identifier, but idNode must
		// be convertible to an identifier if it is present.
		DataNode idNode;
		String sId;
		if (prefab.GetValueFromTable(objectNode, kPropertyId, idNode) && !prefab.AsString(idNode, sId))
		{
			SEOUL_WARN("%s: object %u has no Id property.",
				sceneFilePath.CStr(),
				uObject);
			continue;
		}

		DataNode componentsNode;
		if (!prefab.GetValueFromTable(objectNode, kPropertyComponents, componentsNode))
		{
			SEOUL_WARN("%s: object %s has no Components property.",
				sceneFilePath.CStr(),
				sId.CStr());
			continue;
		}

		UInt32 uComponentsCount = 0u;
		if (!prefab.GetArrayCount(componentsNode, uComponentsCount))
		{
			SEOUL_WARN("%s: object %s has Components field that is not an array.",
				sceneFilePath.CStr(),
				sId.CStr());
			continue;
		}

		SharedPtr<Object> pObject(SEOUL_NEW(MemoryBudgets::SceneObject) Object(sId));

#if SEOUL_EDITOR_AND_TOOLS
		{
			HString category;
			DataNode categoryNode;
			if (prefab.GetValueFromTable(objectNode, kPropertyEditorCategory, categoryNode) &&
				!prefab.AsString(categoryNode, category))
			{
				SEOUL_WARN("%s: object %s has a Category property that is not a string.",
					sceneFilePath.CStr(),
					sId.CStr());
			}

			pObject->SetEditorCategory(category);
		}
#endif // /#if SEOUL_EDITOR_AND_TOOLS

		PrefabComponent* pPrefabComponent = nullptr;
		for (UInt32 uComponent = 0u; uComponent < uComponentsCount; ++uComponent)
		{
			DataNode componentNode;
			SEOUL_VERIFY(prefab.GetValueFromArray(componentsNode, uComponent, componentNode));

			SharedPtr<Component> pComponent;

			{
				WeakAny const weakThis(&pComponent);
				DefaultSerializeContext context(
					ContentKey(sceneFilePath),
					prefab,
					componentNode,
					weakThis.GetTypeInfo());
				if (!DeserializeObject(
					context,
					prefab,
					componentNode,
					weakThis) || !pComponent.IsValid())
				{
					SEOUL_WARN("%s: object %s, failed deserializing component %u.",
						sceneFilePath.CStr(),
						sId.CStr(),
						uComponent);
					continue;
				}

				// If the Component is an PrefabComponent,
				// track this. We'll encode this object specially
				// as a nested prefab reference.
				WeakAny const weakComponentThis(pComponent->GetReflectionThis());
				if (nullptr == pPrefabComponent && weakComponentThis.IsOfType<PrefabComponent*>())
				{
					pPrefabComponent = weakComponentThis.Cast<PrefabComponent*>();
				}
			}

			pObject->AddComponent(pComponent);
		}

		// If we had an PrefabComponent, we need to track
		// the object as a nested prefab reference instead of as a full
		// object.
		if (nullptr != pPrefabComponent)
		{
			NestedPrefab nestedPrefab;
			nestedPrefab.m_hPrefab = pPrefabComponent->GetPrefab();
			nestedPrefab.m_sId = pObject->GetId();
#if SEOUL_EDITOR_AND_TOOLS
			nestedPrefab.m_EditorCategory = pObject->GetEditorCategory();
#endif // /#if SEOUL_EDITOR_AND_TOOLS
			nestedPrefab.m_qRotation = pObject->GetRotation();
			nestedPrefab.m_vPosition = pObject->GetPosition();
			rTemplate.m_vPrefabs.PushBack(nestedPrefab);
		}
		// Otherwise, just add the object.
		else
		{
			rTemplate.m_vObjects.PushBack(pObject);
			uOutObject++;
		}
	}

	return true;
}

} // namespace Scene

SEOUL_BEGIN_TYPE(Scene::PrefabContentHandle)
SEOUL_END_TYPE()

SharedPtr<Scene::Prefab> Content::Traits<Scene::Prefab>::GetPlaceholder(FilePath filePath)
{
	return SharedPtr<Scene::Prefab>();
}

Bool Content::Traits<Scene::Prefab>::FileChange(FilePath filePath, const Scene::PrefabContentHandle& hEntry)
{
	if (FileType::kScenePrefab == filePath.GetType())
	{
		Load(filePath, hEntry);
		return true;
	}

	return false;
}

void Content::Traits<Scene::Prefab>::Load(FilePath filePath, const Scene::PrefabContentHandle& hEntry)
{
	Content::LoadManager::Get()->Queue(
		SharedPtr<Content::LoaderBase>(SEOUL_NEW(MemoryBudgets::Content) Scene::PrefabContentLoader(filePath, hEntry)));
}

Bool Content::Traits<Scene::Prefab>::PrepareDelete(FilePath filePath, Content::Entry<Scene::Prefab, KeyType>& entry)
{
	return true;
}

} // namespace Seoul

#endif // /#if SEOUL_WITH_SCENE
