/**
 * \file EditorUICommandHistory.h
 * \brief A list of executed editor commands. Primarily
 * used for undo/redo functionality.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef EDITOR_UI_COMMAND_HISTORY_H
#define EDITOR_UI_COMMAND_HISTORY_H

#include "CheckedPtr.h"
#include "EditorUICommandNop.h"
#include "Prereqs.h"

namespace Seoul::EditorUI
{

class CommandHistory SEOUL_SEALED
{
public:
	/** Default of 500 commands in the history. */
	static const UInt32 kuDefaultMaxCommands = 5000u;

	CommandHistory(UInt32 uMaxCommands = kuDefaultMaxCommands);
	~CommandHistory();

	/** Insert a new action into the command history. */
	void AddCommand(CheckedPtr<DevUI::Command> pCommand);

	/** Get true if the command history can redo an undone action, false otherwise. */
	Bool CanRedo() const
	{
		return (nullptr != m_pHeadCommand->GetNextCommand());
	}

	/** Get true if the command history has an action to undo, false otherwise. */
	Bool CanUndo() const
	{
		return (m_pHeadCommand->CanUndo());
	}

	/** Release all entries in the command history. */
	void Clear();

	/** Return a read-only pointer to the current head command. */
	CheckedPtr<DevUI::Command const> GetHeadCommand() const
	{
		return m_pHeadCommand;
	}

	/** Return the maximum number of commands allowed in this history - 0u for unlimited. */
	UInt32 GetMaxCommands() const { return m_uMaxCommands; }

	/** Return the total number of commands in this history. */
	UInt32 GetTotalCommands() const { return m_uCommands; }

	/** Return the total size of this command history in bytes. */
	UInt32 GetTotalSizeInBytes() const;

	/**
	 * Return whether or not the command list is currently
	 * at or reachable to the marked command.
	 *
	 * "Reachable" means that all commands between
	 * the marked command and head can inherit
	 * the mark.
	 */
	Bool CanReachMarkedCommand() const
	{
		return m_bMarkedReachable;
	}

	/**
	 * Lock the command history's head. Once locked,
	 * the command can no longer be merged. Merging
	 * allows continuous changes to be combined into
	 * a single command entry in the history.
	 */
	void LockHeadCommand()
	{
		m_pHeadCommand->Lock();
	}

	/** Mark the command history's head. Typically done at save operations. */
	void MarkHeadCommand()
	{
		// Need to lock the head command when we mark it,
		// don't want to mutate the command after marking.
		m_pHeadCommand->Lock();
		m_pMarkedCommand = m_pHeadCommand;
		m_bMarkedReachable = true;
	}

	/** Redo the previously undone action. */
	void Redo();

	/** Undo the head action. */
	void Undo();

private:
	UInt32 const m_uMaxCommands;

	/**
	 * Head always starts as a NopCommand, so we can rely on m_pHead and m_pTail always be valid (it is a sentinel value.
	 * The tail also never changes (it always points to the NopCommand sentinel).
	 */
	CheckedPtr<DevUI::Command> m_pHeadCommand;
	CheckedPtr<DevUI::Command> m_pTailCommand;
	CheckedPtr<DevUI::Command> m_pMarkedCommand;
	UInt32 m_uCommands;
	Bool m_bMarkedReachable;

	void Destroy();
	void RefreshMarkReachable();

	SEOUL_DISABLE_COPY(CommandHistory);
}; // class CommandHistory

} // namespace Seoul::EditorUI

#endif // include guard
