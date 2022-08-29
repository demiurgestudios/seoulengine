/**
 * \file EditorUICommandNop.h
 * \brief Specialization of DevUI::Command that does
 * nothing. Used as a placeholder or sentinel in
 * a command history.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef EDITOR_UI_COMMAND_NOP_H
#define EDITOR_UI_COMMAND_NOP_H

#include "DevUICommand.h"
#include "SeoulString.h"

namespace Seoul::EditorUI
{

class CommandNop SEOUL_SEALED : public DevUI::Command
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(CommandNop);

	CommandNop()
	{
	}

	~CommandNop()
	{
	}

	/**
	 * Nop command can inherit the marker,
	 * as it is only present at the beginning of the
	 * command history.
	 */
	virtual Bool CanInheritMarker() const SEOUL_OVERRIDE
	{
		return true;
	}

	virtual Bool CanUndo() const SEOUL_OVERRIDE
	{
		return false;
	}

	virtual void Do() SEOUL_OVERRIDE
	{
		// Nop
	}

	// Return a human readable description of this command.
	virtual const String& GetDescription() const SEOUL_OVERRIDE
	{
		static const String ksEmpty;
		return ksEmpty;
	}

	// Return the (can be estimated) size of this command in-memory, in bytes.
	virtual UInt32 GetSizeInBytes() const SEOUL_OVERRIDE
	{
		return 0u;
	}

	virtual void Redo() SEOUL_OVERRIDE
	{
		// Nop, must never be called.
		SEOUL_FAIL("Programmer error, Redo() on CommandNop.");
	}

	virtual void Undo() SEOUL_OVERRIDE
	{
		// Nop, must never be called.
		SEOUL_FAIL("Programmer error, Undo() on CommandNop.");
	}

private:
	SEOUL_DISABLE_COPY(CommandNop);
}; // class CommandNop

} // namespace Seoul::EditorUI

#endif // include guard
