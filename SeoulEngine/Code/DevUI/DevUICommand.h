/**
 * \file DevUICommand.h
 * \brief Base class for undo/redo style command processing in a DevUI::Root subclass/project.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef DEV_UI_COMMAND_H
#define DEV_UI_COMMAND_H

#include "Prereqs.h"
#include "ReflectionDeclare.h"
#include "SeoulHString.h"

namespace Seoul::DevUI
{

class Command SEOUL_ABSTRACT
{
public:
	SEOUL_REFLECTION_POLYMORPHIC_BASE(Command);

	virtual ~Command()
	{
	}

	// When true, this command will inherit the marker,
	// if the previous marked command currently has
	// the marker or the inherited marker.
	virtual Bool CanInheritMarker() const { return false; }

	// Must always be defined.
	virtual void Do() = 0;

	// Return a human readable description of this command.
	virtual const String& GetDescription() const = 0;

	// Return the (can be estimated) size of this command in-memory, in bytes.
	virtual UInt32 GetSizeInBytes() const = 0;

	// Must be defined to fully specify a command that
	// can be undone/redone.
	virtual void Redo()
	{
		Do();
	}
	virtual void Undo() = 0;
	virtual Bool CanUndo() const = 0;

	/**
	 * Prevent this command from accepting merges.
	 *
	 * Only necessary for special cases (e.g. disabling merges
	 * after an active input change). The command history
	 * will lock commands when they are no longer the
	 * head command.
	 */
	void Lock()
	{
		 m_bLocked = true;
	}

	/** Attempt to merge a command into this command. */
	Bool Merge(Command const* pCommand)
	{
		if (m_bLocked)
		{
			return false;
		}

		return DoMerge(pCommand);
	}

	/** Get the previous command in the history. */
	Command const* GetPrevCommand() const { return m_pPrevCommand; }
	Command* GetPrevCommand() { return m_pPrevCommand; }

	/** Get or set the next command in the history. */
	Command const* GetNextCommand() const { return m_pNextCommand; }
	Command* GetNextCommand() { return m_pNextCommand; }

	void SetNextCommand(Command* pNextCommand)
	{
		// Unlink from our current next, if we have one.
		if (nullptr != m_pNextCommand)
		{
			m_pNextCommand->m_pPrevCommand = nullptr;
		}

		// Update our next command.
		m_pNextCommand = pNextCommand;

		// Link the next command to us, if defined.
		if (nullptr != m_pNextCommand)
		{
			m_pNextCommand->m_pPrevCommand = this;
		}
	}

protected:
	Command(Bool bStartLocked = true)
		: m_pPrevCommand(nullptr)
		, m_pNextCommand(nullptr)
		, m_bLocked(bStartLocked)
	{
	}

	virtual Bool DoMerge(Command const* pCommand)
	{
		// Nop by default.
		return false;
	}

private:
	Command* m_pPrevCommand;
	Command* m_pNextCommand;
	Bool m_bLocked;

	SEOUL_DISABLE_COPY(Command);
}; // class DevUI::Command

} // namespace Seoul::DevUI

#endif // include guard
