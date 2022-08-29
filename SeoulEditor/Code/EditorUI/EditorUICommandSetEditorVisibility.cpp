/**
 * \file EditorUICommandSetEditorVisibility.cpp
 * \brief Command entry for setting the (editor only) visibility
 * of an Object.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "EditorUICommandSetEditorVisibility.h"
#include "ReflectionDefine.h"
#include "SceneObject.h"

#if SEOUL_WITH_SCENE

namespace Seoul
{

SEOUL_BEGIN_TYPE(EditorUI::CommandSetEditorVisibility, TypeFlags::kDisableNew)
	SEOUL_PARENT(DevUI::Command)
SEOUL_END_TYPE()

namespace EditorUI
{

/** Capture old visibility of each object in the mutation set. */
static inline CommandSetEditorVisibility::OldVisibility GetOldVisibility(const CommandSetEditorVisibility::Objects& tObjects)
{
	CommandSetEditorVisibility::OldVisibility tReturn;
	for (auto const& pObject : tObjects)
	{
		SEOUL_VERIFY(tReturn.Insert(pObject, pObject->GetVisibleInEditor()).Second);
	}

	return tReturn;
}

/** Build the human readable description string based on input data. */
static inline String BuildDescription(
	const CommandSetEditorVisibility::Objects& tObjects,
	Bool bTargetVisibility)
{
	if (tObjects.GetSize() == 1u)
	{
		return String::Printf("Set \"%s\" Visibility to %s", (*tObjects.Begin())->GetId().CStr(), (bTargetVisibility ? "Visible" : "Hidden"));
	}
	else
	{
		return String::Printf("Set Multiple Visibility to %s", (bTargetVisibility ? "Visible" : "Hidden"));
	}
}

CommandSetEditorVisibility::CommandSetEditorVisibility(
	const Objects& tObjects,
	Bool bTargetVisibility)
	: m_tOldVisibility(GetOldVisibility(tObjects))
	, m_bTargetVisibility(bTargetVisibility)
	, m_sDescription(BuildDescription(tObjects, bTargetVisibility))
{
}

CommandSetEditorVisibility::~CommandSetEditorVisibility()
{
}

void CommandSetEditorVisibility::Do()
{
	auto const iBegin = m_tOldVisibility.Begin();
	auto const iEnd = m_tOldVisibility.End();
	for (auto i = iBegin; iEnd != i; ++i)
	{
		i->First->SetVisibleInEditor(m_bTargetVisibility);
	}
}

const String& CommandSetEditorVisibility::GetDescription() const
{
	return m_sDescription;
}

UInt32 CommandSetEditorVisibility::GetSizeInBytes() const
{
	return (UInt32)(
		m_sDescription.GetCapacity() +
		m_tOldVisibility.GetMemoryUsageInBytes() +
		sizeof(*this));
}

void CommandSetEditorVisibility::Undo()
{
	auto const iBegin = m_tOldVisibility.Begin();
	auto const iEnd = m_tOldVisibility.End();
	for (auto i = iBegin; iEnd != i; ++i)
	{
		i->First->SetVisibleInEditor(i->Second);
	}
}

} // namespace EditorUI

} // namespace Seoul

#endif // /#if SEOUL_WITH_SCENE
