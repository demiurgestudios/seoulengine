/**
 * \file EditorUICommandSelectObjects.h
 * \brief Command for a multiple selection or deselection
 * event.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef EDITOR_UI_COMMAND_SELECT_OBJECTS_H
#define EDITOR_UI_COMMAND_SELECT_OBJECTS_H

#include "DevUICommand.h"
#include "HashSet.h"
#include "SharedPtr.h"
namespace Seoul { namespace Scene { class Object; } }

#if SEOUL_WITH_SCENE

namespace Seoul::EditorUI
{

class CommandSelectObjects SEOUL_SEALED : public DevUI::Command
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(CommandSelectObjects);

	typedef HashSet<SharedPtr<Scene::Object>, MemoryBudgets::Editor> SelectedObjects;

	CommandSelectObjects(
		SharedPtr<Scene::Object>& rpLastSelection,
		SelectedObjects& rtSelectedObjects,
		const SelectedObjects& tPrevObjects,
		const SharedPtr<Scene::Object>& pLastSelection,
		const SelectedObjects& tTargetObjects);
	~CommandSelectObjects();

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
	SharedPtr<Scene::Object> const m_pPrevLastSelection;
	SelectedObjects const m_tPrevSelection;
	SharedPtr<Scene::Object> const m_pLastSelection;
	SelectedObjects const m_tTargetSelection;

	SEOUL_DISABLE_COPY(CommandSelectObjects);
}; // class CommandSelectObjects

} // namespace Seoul::EditorUI

#endif // /#if SEOUL_WITH_SCENE

#endif // include guard
