/**
 * \file EditorUICommandUniqueSelectObject.cpp
 * \brief UndoAction for mutations of an Object's Components.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "EditorUICommandUniqueSelectObject.h"
#include "ReflectionDefine.h"
#include "SceneObject.h"

#if SEOUL_WITH_SCENE

namespace Seoul
{

SEOUL_BEGIN_TYPE(EditorUI::CommandUniqueSelectObject, TypeFlags::kDisableNew)
	SEOUL_PARENT(DevUI::Command)
SEOUL_END_TYPE()

namespace EditorUI
{

CommandUniqueSelectObject::CommandUniqueSelectObject(
	SharedPtr<Scene::Object>& rpLastSelection,
	SelectedObjects& rtSelectedObjects,
	const SharedPtr<Scene::Object>& pObject)
	: m_rpLastSelection(rpLastSelection)
	, m_rtSelectedObjects(rtSelectedObjects)
	, m_pObject(pObject)
	, m_pPrevLastSelection(rpLastSelection)
	, m_tPrevSelection(m_rtSelectedObjects)
	, m_sDescription(String::Printf("Select \"%s\"", pObject->GetId().CStr()))
{
}

CommandUniqueSelectObject::~CommandUniqueSelectObject()
{
}

void CommandUniqueSelectObject::Do()
{
	m_rtSelectedObjects.Clear();
	SEOUL_VERIFY(m_rtSelectedObjects.Insert(m_pObject).Second);
	m_rpLastSelection = m_pObject;
}

const String& CommandUniqueSelectObject::GetDescription() const
{
	return m_sDescription;
}

UInt32 CommandUniqueSelectObject::GetSizeInBytes() const
{
	return (UInt32)(m_sDescription.GetCapacity() + m_tPrevSelection.GetMemoryUsageInBytes() + sizeof(*this));
}

void CommandUniqueSelectObject::Undo()
{
	m_rtSelectedObjects = m_tPrevSelection;
	m_rpLastSelection = m_pPrevLastSelection;
}

} // namespace EditorUI

} // namespace Seoul

#endif // /#if SEOUL_WITH_SCENE
