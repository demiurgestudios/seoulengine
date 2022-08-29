/**
 * \file EditorUICommandPasteObjects.cpp
 * \brief DevUI::Command for wrapping a command that pastes
 * a Scene::Object into a EditorScene.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "EditorSceneContainer.h"
#include "EditorUICommandPasteObjects.h"
#include "ReflectionDefine.h"
#include "SceneObject.h"

#if SEOUL_WITH_SCENE

namespace Seoul
{

SEOUL_BEGIN_TYPE(EditorUI::CommandPasteObjects, TypeFlags::kDisableNew)
	SEOUL_PARENT(DevUI::Command)
SEOUL_END_TYPE()

namespace EditorUI
{

CommandPasteObjects::CommandPasteObjects(
	EditorScene::Container& rScene,
	SharedPtr<Scene::Object>& rpLastSelection,
	Objects& rtSelectedObjects,
	const Objects& tNewObjects)
	: m_rScene(rScene)
	, m_rpLastSelection(rpLastSelection)
	, m_rtSelectedObjects(rtSelectedObjects)
	, m_tNewObjects(tNewObjects)
	, m_tPrevSelection(m_rtSelectedObjects)
	, m_pLastSelection(rpLastSelection)
	, m_sDescription(String::Printf("Paste %u Objects", tNewObjects.GetSize()))
{
}

CommandPasteObjects::~CommandPasteObjects()
{
}

void CommandPasteObjects::Do()
{
	m_rtSelectedObjects.Clear();
	for (auto const& pObject : m_tNewObjects)
	{
		m_rScene.AddObject(pObject);
	}
	m_rtSelectedObjects = m_tNewObjects;
	if (!m_tNewObjects.IsEmpty())
	{
		m_rpLastSelection = *m_tNewObjects.Begin();
	}
	m_rScene.SortObjects();
}

const String& CommandPasteObjects::GetDescription() const
{
	return m_sDescription;
}

UInt32 CommandPasteObjects::GetSizeInBytes() const
{
	return (UInt32)(
		m_sDescription.GetCapacity() +
		m_tPrevSelection.GetMemoryUsageInBytes() +
		m_tNewObjects.GetMemoryUsageInBytes() +
		sizeof(*this));
}

void CommandPasteObjects::Undo()
{
	m_rtSelectedObjects = m_tPrevSelection;
	m_rpLastSelection = m_pLastSelection;
	for (auto const& pObject : m_tNewObjects)
	{
		m_rScene.RemoveObject(pObject);
	}
}

} // namespace EditorUI

} // namespace Seoul

#endif // /#if SEOUL_WITH_SCENE
