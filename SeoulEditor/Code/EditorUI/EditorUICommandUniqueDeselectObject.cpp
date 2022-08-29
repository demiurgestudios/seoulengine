/**
 * \file EditorUICommandUniqueDeselectObject.cpp
 * \brief UndoAction for mutations of an Object's Components.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "EditorUICommandUniqueDeselectObject.h"
#include "ReflectionDefine.h"
#include "SceneObject.h"

#if SEOUL_WITH_SCENE

namespace Seoul
{

SEOUL_BEGIN_TYPE(EditorUI::CommandUniqueDeselectObject, TypeFlags::kDisableNew)
	SEOUL_PARENT(DevUI::Command)
SEOUL_END_TYPE()

namespace EditorUI
{

CommandUniqueDeselectObject::CommandUniqueDeselectObject(
	SharedPtr<Scene::Object>& rpLastSelection,
	SelectedObjects& rtSelectedObjects)
	: m_rpLastSelection(rpLastSelection)
	, m_rtSelectedObjects(rtSelectedObjects)
	, m_pPrevLastSelection(rpLastSelection)
	, m_tPrevSelection(m_rtSelectedObjects)
{
}

CommandUniqueDeselectObject::~CommandUniqueDeselectObject()
{
}

void CommandUniqueDeselectObject::Do()
{
	m_rtSelectedObjects.Clear();
	m_rpLastSelection.Reset();
}

const String& CommandUniqueDeselectObject::GetDescription() const
{
	static const String ksDescription("Select None");
	return ksDescription;
}

UInt32 CommandUniqueDeselectObject::GetSizeInBytes() const
{
	return m_tPrevSelection.GetMemoryUsageInBytes() + sizeof(*this);
}

void CommandUniqueDeselectObject::Undo()
{
	m_rtSelectedObjects = m_tPrevSelection;
	m_rpLastSelection = m_pPrevLastSelection;
}

} // namespace EditorUI

} // namespace Seoul

#endif // /#if SEOUL_WITH_SCENE
