/**
 * \file EditorUIControllerBase.h
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

#pragma once
#ifndef EDITOR_UI_CONTROLLER_BASE_H
#define EDITOR_UI_CONTROLLER_BASE_H

#include "DevUIController.h"
#include "ScopedPtr.h"
namespace Seoul { class DevUI::Command; }
namespace Seoul { namespace EditorUI { class CommandHistory; } }

namespace Seoul::EditorUI
{

class ControllerBase SEOUL_ABSTRACT : public DevUI::Controller
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(ControllerBase);

	virtual ~ControllerBase();

	// DevUI::Controller overrides
	virtual Bool CanRedo() const SEOUL_OVERRIDE SEOUL_SEALED;
	virtual Bool CanUndo() const SEOUL_OVERRIDE SEOUL_SEALED;
	virtual void ClearHistory() SEOUL_OVERRIDE;
	virtual UInt32 GetCommandHistoryTotalSizeInBytes() const SEOUL_OVERRIDE SEOUL_SEALED;
	virtual DevUI::Command const* GetHeadCommand() const SEOUL_OVERRIDE SEOUL_SEALED;
	virtual void Redo() SEOUL_OVERRIDE SEOUL_SEALED;
	virtual void Undo() SEOUL_OVERRIDE SEOUL_SEALED;
	// /DevUI::Controller overrides

protected:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(ControllerBase);

	ControllerBase();

	// Access point for querying and manipulating the
	// state of the command history.
	Bool CanReachMarkedCommand() const;
	void LockHeadCommand();
	void MarkHeadCommand();

	// Access point for subclasses when they wish to mutate the model.
	// Similar to CommandHistory::AddCommand(), except also
	// calls DevUI::Command::Do(), then adds it.
	void ExecuteCommand(DevUI::Command* pCommand);

private:
	ScopedPtr<CommandHistory> m_pCommandHistory;

	SEOUL_DISABLE_COPY(ControllerBase);
}; // class ControllerBase

} // namespace Seoul::EditorUI

#endif // include guard
