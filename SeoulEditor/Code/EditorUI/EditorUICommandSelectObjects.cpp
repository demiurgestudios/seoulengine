/**
 * \file EditorUICommandSelectObjects.cpp
 * \brief UndoAction for mutations of an Object's Components.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "EditorUICommandSelectObjects.h"
#include "ReflectionDefine.h"
#include "SceneObject.h"

#if SEOUL_WITH_SCENE

namespace Seoul
{

SEOUL_BEGIN_TYPE(EditorUI::CommandSelectObjects, TypeFlags::kDisableNew)
	SEOUL_PARENT(DevUI::Command)
SEOUL_END_TYPE()

namespace EditorUI
{

CommandSelectObjects::CommandSelectObjects(
	SharedPtr<Scene::Object>& rpLastSelection,
	SelectedObjects& rtSelectedObjects,
	const SelectedObjects& tPrevObjects,
	const SharedPtr<Scene::Object>& pLastSelection,
	const SelectedObjects& tTargetObjects)
	: m_rpLastSelection(rpLastSelection)
	, m_rtSelectedObjects(rtSelectedObjects)
	, m_pPrevLastSelection(rpLastSelection)
	, m_tPrevSelection(tPrevObjects)
	, m_pLastSelection(pLastSelection)
	, m_tTargetSelection(tTargetObjects)
{
}

CommandSelectObjects::~CommandSelectObjects()
{
}

void CommandSelectObjects::Do()
{
	m_rtSelectedObjects = m_tTargetSelection;
	m_rpLastSelection = m_pLastSelection;
}

const String& CommandSelectObjects::GetDescription() const
{
	static const String ksDescription("Select Multiple");
	return ksDescription;
}

UInt32 CommandSelectObjects::GetSizeInBytes() const
{
	return
		m_tPrevSelection.GetMemoryUsageInBytes() +
		m_tTargetSelection.GetMemoryUsageInBytes() +
		sizeof(*this);
}

void CommandSelectObjects::Undo()
{
	m_rtSelectedObjects = m_tPrevSelection;
	m_rpLastSelection = m_pPrevLastSelection;
}

} // namespace EditorUI

} // namespace Seoul

#endif // /#if SEOUL_WITH_SCENE
