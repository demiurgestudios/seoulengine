/**
 * \file EditorUICommandDeleteObjects.cpp
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
#include "EditorUICommandDeleteObjects.h"
#include "ReflectionDefine.h"
#include "SceneObject.h"

#if SEOUL_WITH_SCENE

namespace Seoul
{

SEOUL_BEGIN_TYPE(EditorUI::CommandDeleteObjects, TypeFlags::kDisableNew)
	SEOUL_PARENT(DevUI::Command)
SEOUL_END_TYPE()

namespace EditorUI
{

CommandDeleteObjects::CommandDeleteObjects(
	EditorScene::Container& rScene,
	SharedPtr<Scene::Object>& rpLastSelection,
	Objects& rtSelectedObjects,
	const Objects& tDeletedObjects,
	Bool bCutCommand)
	: m_rScene(rScene)
	, m_rpLastSelection(rpLastSelection)
	, m_rtSelectedObjects(rtSelectedObjects)
	, m_tDeletedObjects(tDeletedObjects)
	, m_tPrevSelection(m_rtSelectedObjects)
	, m_pLastSelection(rpLastSelection)
	, m_sDescription(bCutCommand
		? String::Printf("Cut %u Objects", tDeletedObjects.GetSize())
		: String::Printf("Delete %u Objects", tDeletedObjects.GetSize()))
{
}

CommandDeleteObjects::~CommandDeleteObjects()
{
}

void CommandDeleteObjects::Do()
{
	for (auto const& pObject : m_tDeletedObjects)
	{
		m_rScene.RemoveObject(pObject);
		(void)m_rtSelectedObjects.Erase(pObject);
		if (m_rpLastSelection == pObject)
		{
			m_rpLastSelection.Reset();
		}
	}
}

const String& CommandDeleteObjects::GetDescription() const
{
	return m_sDescription;
}

UInt32 CommandDeleteObjects::GetSizeInBytes() const
{
	return (UInt32)(
		m_sDescription.GetCapacity() +
		m_tPrevSelection.GetMemoryUsageInBytes() +
		m_tDeletedObjects.GetMemoryUsageInBytes() +
		sizeof(*this));
}

void CommandDeleteObjects::Undo()
{
	m_rtSelectedObjects = m_tPrevSelection;
	m_rpLastSelection = m_pLastSelection;
	for (auto const& pObject : m_tDeletedObjects)
	{
		m_rScene.AddObject(pObject);
	}
	m_rScene.SortObjects();
}

} // namespace EditorUI

} // namespace Seoul

#endif // /#if SEOUL_WITH_SCENE
