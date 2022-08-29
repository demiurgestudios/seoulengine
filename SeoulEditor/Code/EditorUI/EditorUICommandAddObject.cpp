/**
 * \file EditorUICommandAddObject.cpp
 * \brief DevUI::Command for wrapping a command that deletes
 * or cuts one or more objects from a prefab instance.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "EditorSceneContainer.h"
#include "EditorUICommandAddObject.h"
#include "ReflectionDefine.h"
#include "SceneObject.h"

#if SEOUL_WITH_SCENE

namespace Seoul
{

SEOUL_BEGIN_TYPE(EditorUI::CommandAddObject, TypeFlags::kDisableNew)
	SEOUL_PARENT(DevUI::Command)
SEOUL_END_TYPE()

namespace EditorUI
{

CommandAddObject::CommandAddObject(
	EditorScene::Container& rScene,
	SharedPtr<Scene::Object>& rpLastSelection,
	Objects& rtSelectedObjects,
	const SharedPtr<Scene::Object>& pObject)
	: m_rScene(rScene)
	, m_rpLastSelection(rpLastSelection)
	, m_rtSelectedObjects(rtSelectedObjects)
	, m_pObject(pObject)
	, m_tPrevSelection(m_rtSelectedObjects)
	, m_pLastSelection(rpLastSelection)
	, m_sDescription(String::Printf("Add Object %s", pObject->GetId().CStr()))
{
}

CommandAddObject::~CommandAddObject()
{
}

void CommandAddObject::Do()
{
	// Add the object.
	m_rScene.AddObject(m_pObject);

	// Apply selection.
	m_rtSelectedObjects.Clear();
	SEOUL_VERIFY(m_rtSelectedObjects.Insert(m_pObject).Second);
	m_rpLastSelection = m_pObject;

	// Sort the scene object list.
	m_rScene.SortObjects();
}

const String& CommandAddObject::GetDescription() const
{
	return m_sDescription;
}

UInt32 CommandAddObject::GetSizeInBytes() const
{
	return (UInt32)(
		m_sDescription.GetCapacity() +
		m_tPrevSelection.GetMemoryUsageInBytes() +
		sizeof(*this));
}

void CommandAddObject::Undo()
{
	m_rtSelectedObjects = m_tPrevSelection;
	m_rpLastSelection = m_pLastSelection;
	m_rScene.RemoveObject(m_pObject);
}

} // namespace EditorUI

} // namespace Seoul

#endif // /#if SEOUL_WITH_SCENE
