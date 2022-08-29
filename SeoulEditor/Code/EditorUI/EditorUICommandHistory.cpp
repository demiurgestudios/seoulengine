/**
 * \file EditorUICommandHistory.cpp
 * \brief A list of executed editor commands. Primarily
 * used for undo/redo functionality.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "EditorUICommandHistory.h"

namespace Seoul::EditorUI
{

CommandHistory::CommandHistory(UInt32 uMaxCommands /*= kuDefaultMaxCommands*/)
	: m_uMaxCommands(uMaxCommands)
	, m_pHeadCommand(SEOUL_NEW(MemoryBudgets::Editor) CommandNop)
	, m_pTailCommand(m_pHeadCommand)
	, m_pMarkedCommand(m_pHeadCommand)
	, m_uCommands(0u) // We lie, and don't include the sentinel (the nop command) in the list.
	, m_bMarkedReachable(true)
{
}

CommandHistory::~CommandHistory()
{
	Destroy();
}

/** Release all entries in the command history. */
void CommandHistory::Clear()
{
	// Tracked marked state.
	auto const bMarkedReachable = m_bMarkedReachable;

	// Cleanup.
	Destroy();

	// Restore default nop.
	m_pHeadCommand.Reset(SEOUL_NEW(MemoryBudgets::Editor) CommandNop);
	m_pTailCommand = m_pHeadCommand;
	m_uCommands = 0u;

	// Configured marked depending on previous state.
	if (bMarkedReachable)
	{
		m_pMarkedCommand = m_pHeadCommand;
		m_bMarkedReachable = true;
	}
	else
	{
		m_pMarkedCommand.Reset();
		m_bMarkedReachable = false;
	}
}

/** Insert a new action into the command history. */
void CommandHistory::AddCommand(CheckedPtr<DevUI::Command> pCommand)
{
	// Prune commands first, if they exist,
	// since in that case we're inserting a command
	// into the middle of our command list and are
	// discarding those commands.
	{
		CheckedPtr<DevUI::Command> p = m_pHeadCommand->GetNextCommand();
		m_pHeadCommand->SetNextCommand(nullptr);
		while (nullptr != p)
		{
			// Clear the marked command if we encounter it.
			if (m_pMarkedCommand == p) { m_pMarkedCommand.Reset(); }

			// Delete the command.
			CheckedPtr<DevUI::Command> t = p;
			p = p->GetNextCommand();
			SafeDelete(t);
		}
	}

	// Attempt to merge the command - if successful,
	// just destroy it.
	if (m_pHeadCommand->Merge(pCommand))
	{
		SafeDelete(pCommand);

		// Refresh marked reachability prior to return.
		RefreshMarkReachable();

		return;
	}

	// Lock the head command before it changes.
	m_pHeadCommand->Lock();

	// Now update head.
	m_pHeadCommand->SetNextCommand(pCommand);
	m_pHeadCommand = m_pHeadCommand->GetNextCommand();

	// If we're at the limit, prune.
	if (m_uCommands >= m_uMaxCommands && nullptr != m_pTailCommand->GetNextCommand())
	{
		// Tail always exists, it's our nop sentinel,
		// so we can just access it here.
		auto pToDelete = m_pTailCommand->GetNextCommand();

		// Clear the marked command if it's pointing at the tail sentinel,
		// as the tail no longer represents the start of the command list anymore.
		if (m_pMarkedCommand == m_pTailCommand) { m_pMarkedCommand.Reset(); }

		// Clear the marked command if we encounter it.
		if (m_pMarkedCommand == pToDelete) { m_pMarkedCommand.Reset(); }

		// Set next of tail to one passed its current next.
		m_pTailCommand->SetNextCommand(pToDelete->GetNextCommand());

		// Finally, destroy it.
		SafeDelete(pToDelete);
	}
	// Otherwise, just advance.
	else
	{
		++m_uCommands;
	}

	// Refresh marked reachability.
	RefreshMarkReachable();
}

/** Return the total size of this command history in bytes. */
UInt32 CommandHistory::GetTotalSizeInBytes() const
{
	// TODO: Cache this.

	auto p = m_pHeadCommand;

	// Find the real head.
	while (nullptr != p)
	{
		if (p->GetNextCommand() == nullptr)
		{
			break;
		}

		p = p->GetNextCommand();
	}

	UInt32 uTotalSizeInBytes = 0u;

	// Now accumulate all.
	while (nullptr != p)
	{
		uTotalSizeInBytes += p->GetSizeInBytes();
		p = p->GetPrevCommand();
	}

	return uTotalSizeInBytes;
}

/** Redo the previously undone action. */
void CommandHistory::Redo()
{
	// Early out if can't redo.
	if (!CanRedo())
	{
		return;
	}

	// Lock the head command before it changes.
	m_pHeadCommand->Lock();

	// Now update head.
	m_pHeadCommand = m_pHeadCommand->GetNextCommand();
	m_pHeadCommand->Redo();

	// Refresh marked reachability.
	RefreshMarkReachable();
}

/** Undo the head action. */
void CommandHistory::Undo()
{
	// Early out if can't undo.
	if (!CanUndo())
	{
		return;
	}

	// Lock the head command before it changes.
	m_pHeadCommand->Lock();

	// Now update head.
	m_pHeadCommand->Undo();
	m_pHeadCommand = m_pHeadCommand->GetPrevCommand();

	// Refresh marked reachability.
	RefreshMarkReachable();
}

/** Free all resources. */
void CommandHistory::Destroy()
{
	// Cache head.
	CheckedPtr<DevUI::Command> p = m_pHeadCommand;

	// Reset members.
	m_pHeadCommand.Reset();
	m_pTailCommand.Reset();
	m_pMarkedCommand.Reset();
	m_uCommands = 0u;
	m_bMarkedReachable = false;

	// Find the real head.
	while (nullptr != p)
	{
		if (p->GetNextCommand() == nullptr)
		{
			break;
		}

		p = p->GetNextCommand();
	}

	// Now destroy all.
	while (nullptr != p)
	{
		CheckedPtr<DevUI::Command> t = p;
		p = p->GetPrevCommand();
		SafeDelete(t);
	}
}

/** Scan and update mark reachable. */
void CommandHistory::RefreshMarkReachable()
{
	auto p = m_pHeadCommand;

	// Initially not reachable.
	m_bMarkedReachable = false;

	// Scan backwards - backwards scan
	// requires a check of inheritable on p
	// before advancing backwards.
	while (p.IsValid() && p != m_pMarkedCommand)
	{
		// Before backwards step, check inheritable.
		// If not, break immediately.
		if (!p->CanInheritMarker())
		{
			break;
		}

		// Go to previous.
		p = p->GetPrevCommand();
	}

	// Done if we hit the marker.
	if (p.IsValid() && p == m_pMarkedCommand)
	{
		m_bMarkedReachable = true;
		return;
	}

	// Scan forward.
	p = m_pHeadCommand;
	while (p.IsValid() && p != m_pMarkedCommand)
	{
		// Step forward.
		p = p->GetNextCommand();

		// Check that the command we stepped to is reachable - if
		// it is not, we've failed, so return immediately.
		if (p.IsValid() && !p->CanInheritMarker())
		{
			return;
		}
	}

	// Done.
	m_bMarkedReachable = (p.IsValid() && p == m_pMarkedCommand);
}

} // namespace Seoul::EditorUI
