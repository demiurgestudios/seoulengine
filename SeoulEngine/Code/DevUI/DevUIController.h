/**
 * \file DevUIController.h
 * \brief Base interface for a controller component (in the
 * model-view-controller).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef DEV_UI_ICONTROLLER_H
#define DEV_UI_ICONTROLLER_H

#include "FilePath.h"
#include "SharedPtr.h"
#include "Prereqs.h"
#include "ReflectionDeclare.h"
namespace Seoul { namespace DevUI { class Command; } }
namespace Seoul { class DevUIIModel; }

namespace Seoul::DevUI
{

class Controller SEOUL_ABSTRACT
{
public:
	SEOUL_REFLECTION_POLYMORPHIC_BASE(Controller);

	virtual ~Controller()
	{
	}

	// Undo/redo interface of a controller.
	virtual Bool CanRedo() const = 0;
	virtual Bool CanUndo() const = 0;
	virtual void ClearHistory() = 0;
	virtual UInt32 GetCommandHistoryTotalSizeInBytes() const = 0;
	virtual Command const* GetHeadCommand() const = 0;
	virtual void Redo() = 0;
	virtual void Undo() = 0;

	// Edit interface of a controller.
	virtual Bool CanCopy() const = 0;
	virtual Bool CanCut() const = 0;
	virtual Bool CanDelete() const = 0;
	virtual Bool CanPaste() const = 0;
	virtual void Copy() = 0;
	virtual void Cut() = 0;
	virtual void Delete() = 0;
	virtual void Paste() = 0;

	// Save interface of a controller.
	virtual FilePath GetSaveFilePath() const = 0;
	virtual Bool HasSaveFilePath() const = 0;
	virtual Bool IsOutOfDate() const = 0;
	virtual void MarkUpToDate() = 0;
	virtual Bool NeedsSave() const = 0;
	virtual Bool Save() = 0;
	virtual void SetSaveFilePath(FilePath filePath) = 0;

	// Update interface.
	virtual void Tick(Float fDeltaTimeInSeconds) = 0;

protected:
	SEOUL_REFERENCE_COUNTED(Controller);

	Controller()
	{
	}

private:
	SEOUL_DISABLE_COPY(Controller);
}; // class DevUI::Controller

class NullController SEOUL_SEALED : public Controller
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(NullController);

	NullController()
	{
	}

	~NullController()
	{
	}

	// Undo/redo interface of a controller.
	virtual Bool CanRedo() const SEOUL_OVERRIDE { return false; }
	virtual Bool CanUndo() const SEOUL_OVERRIDE { return false; }
	virtual void ClearHistory() SEOUL_OVERRIDE { }
	virtual UInt32 GetCommandHistoryTotalSizeInBytes() const SEOUL_OVERRIDE { return 0u; }
	virtual Command const* GetHeadCommand() const SEOUL_OVERRIDE { return nullptr; }
	virtual void Redo() SEOUL_OVERRIDE {}
	virtual void Undo() SEOUL_OVERRIDE {}

	// Edit interface of a controller.
	virtual Bool CanCopy() const SEOUL_OVERRIDE { return false; }
	virtual Bool CanCut() const SEOUL_OVERRIDE { return false; }
	virtual Bool CanDelete() const SEOUL_OVERRIDE { return false; }
	virtual Bool CanPaste() const SEOUL_OVERRIDE { return false; }
	virtual void Copy() SEOUL_OVERRIDE {}
	virtual void Cut() SEOUL_OVERRIDE {}
	virtual void Delete() SEOUL_OVERRIDE {}
	virtual void Paste() SEOUL_OVERRIDE {}

	// Save interface of a controller.
	virtual FilePath GetSaveFilePath() const SEOUL_OVERRIDE { return FilePath(); }
	virtual Bool HasSaveFilePath() const SEOUL_OVERRIDE { return false; }
	virtual Bool IsOutOfDate() const SEOUL_OVERRIDE { return false; }
	virtual void MarkUpToDate() SEOUL_OVERRIDE {}
	virtual Bool NeedsSave() const SEOUL_OVERRIDE { return false; }
	virtual Bool Save() SEOUL_OVERRIDE { return false; }
	virtual void SetSaveFilePath(FilePath filePath) SEOUL_OVERRIDE {}

	// Update interface.
	void Tick(Float fDeltaTimeInSeconds) SEOUL_OVERRIDE {}

private:
	SEOUL_DISABLE_COPY(NullController);
	SEOUL_REFERENCE_COUNTED_SUBCLASS(NullController);
}; // class DevUI::NullController

} // namespace Seoul::DevUI

#endif // include guard
