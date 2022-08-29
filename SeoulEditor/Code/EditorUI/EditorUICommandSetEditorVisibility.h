/**
 * \file EditorUICommandSetEditorVisibility.h
 * \brief Command entry for setting the (editor only) visibility
 * of an Object.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef EDITOR_UI_COMMAND_SET_EDITOR_VISIBILITY_H
#define EDITOR_UI_COMMAND_SET_EDITOR_VISIBILITY_H

#include "DevUICommand.h"
#include "HashSet.h"
#include "SharedPtr.h"
namespace Seoul { namespace Scene { class Object; } }

#if SEOUL_WITH_SCENE

namespace Seoul::EditorUI
{

class CommandSetEditorVisibility SEOUL_SEALED : public DevUI::Command
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(CommandSetEditorVisibility);

	typedef HashSet<SharedPtr<Scene::Object>, MemoryBudgets::Editor> Objects;
	typedef HashTable<SharedPtr<Scene::Object>, Bool, MemoryBudgets::Editor> OldVisibility;

	CommandSetEditorVisibility(
		const Objects& tObjects,
		Bool bTargetVisibility);
	~CommandSetEditorVisibility();

	/**
	 * Editor visibility changes do not force a save,
	 * so they inherit markers.
	 */
	virtual Bool CanInheritMarker() const SEOUL_OVERRIDE
	{
		return true;
	}

	virtual Bool CanUndo() const SEOUL_OVERRIDE
	{
		return true;
	}

	virtual void Do() SEOUL_OVERRIDE;
	virtual const String& GetDescription() const SEOUL_OVERRIDE;
	virtual UInt32 GetSizeInBytes() const SEOUL_OVERRIDE;
	virtual void Undo() SEOUL_OVERRIDE;

private:
	OldVisibility const m_tOldVisibility;
	Bool const m_bTargetVisibility;
	String const m_sDescription;

	SEOUL_DISABLE_COPY(CommandSetEditorVisibility);
}; // class CommandSetEditorVisibility

} // namespace Seoul::EditorUI

#endif // /#if SEOUL_WITH_SCENE

#endif // include guard
