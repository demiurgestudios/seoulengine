/**
 * \file EditorUICommandUniqueSelectObject.h
 * \brief UndoAction for mutations of an Object's Components.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef EDITOR_UI_COMMAND_UNIQUE_SELECT_OBJECT_H
#define EDITOR_UI_COMMAND_UNIQUE_SELECT_OBJECT_H

#include "DevUICommand.h"
#include "HashSet.h"
#include "SharedPtr.h"
namespace Seoul { namespace Scene { class Object; } }

#if SEOUL_WITH_SCENE

namespace Seoul::EditorUI
{

class CommandUniqueSelectObject SEOUL_SEALED : public DevUI::Command
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(CommandUniqueSelectObject);

	typedef HashSet<SharedPtr<Scene::Object>, MemoryBudgets::Editor> SelectedObjects;

	CommandUniqueSelectObject(
		SharedPtr<Scene::Object>& rpLastSelection,
		SelectedObjects& rtSelectedObjects,
		const SharedPtr<Scene::Object>& pObject);
	~CommandUniqueSelectObject();

	/**
	 * Selection commands do not force a save,
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
	SharedPtr<Scene::Object>& m_rpLastSelection;
	SelectedObjects& m_rtSelectedObjects;
	SharedPtr<Scene::Object> const m_pObject;
	SharedPtr<Scene::Object> const m_pPrevLastSelection;
	SelectedObjects const m_tPrevSelection;
	String const m_sDescription;

	SEOUL_DISABLE_COPY(CommandUniqueSelectObject);
}; // class CommandUniqueSelectObject

} // namespace Seoul::EditorUI

#endif // /#if SEOUL_WITH_SCENE

#endif // include guard
