/**
 * \file EditorUIControllerBase.cpp
 * \brief Base implementation for a controller (of model-view-controller).
 * Implements common, shared functionality of a controller and should
 * be the base of most or all controller specializations.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "EditorUICommandHistory.h"
#include "EditorUIControllerBase.h"
#include "EditorUIIControllerPropertyEditor.h"
#include "ReflectionDefine.h"

namespace Seoul
{

SEOUL_BEGIN_TYPE(EditorUI::ControllerBase, TypeFlags::kDisableNew)
	SEOUL_PARENT(DevUI::Controller)
SEOUL_END_TYPE()
SEOUL_TYPE(EditorUI::IControllerPropertyEditor, TypeFlags::kDisableNew)

namespace EditorUI
{

ControllerBase::ControllerBase()
	: m_pCommandHistory(SEOUL_NEW(MemoryBudgets::Editor) CommandHistory)
{
}

ControllerBase::~ControllerBase()
{
}

Bool ControllerBase::CanRedo() const
{
	return m_pCommandHistory->CanRedo();
}

Bool ControllerBase::CanUndo() const
{
	return m_pCommandHistory->CanUndo();
}

void ControllerBase::ClearHistory()
{
	m_pCommandHistory->Clear();
}

UInt32 ControllerBase::GetCommandHistoryTotalSizeInBytes() const
{
	return m_pCommandHistory->GetTotalSizeInBytes();
}

DevUI::Command const* ControllerBase::GetHeadCommand() const
{
	return m_pCommandHistory->GetHeadCommand();
}

void ControllerBase::Redo()
{
	m_pCommandHistory->Redo();
}

void ControllerBase::Undo()
{
	m_pCommandHistory->Undo();
}

Bool ControllerBase::CanReachMarkedCommand() const
{
	return m_pCommandHistory->CanReachMarkedCommand();
}

void ControllerBase::LockHeadCommand()
{
	m_pCommandHistory->LockHeadCommand();
}

void ControllerBase::MarkHeadCommand()
{
	m_pCommandHistory->MarkHeadCommand();
}

/**
 * Access point for subclasses when they wish to mutate the model.
 * Similar to CommandHistory::AddCommand(), except also
 * calls DevUI::Command::Do(), then adds it.
 */
void ControllerBase::ExecuteCommand(DevUI::Command* pCommand)
{
	// Execute the command.
	pCommand->Do();

	// Must be last - AddCommand takes ownership of pCommand and may destroy it.
	m_pCommandHistory->AddCommand(pCommand);
}

} // namespace EditorUI

} // namespace Seoul
