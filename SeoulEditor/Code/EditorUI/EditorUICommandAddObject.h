/**
 * \file EditorUICommandAddObject.h
 * \brief DevUI::Command for wrapping a command that deletes
 * or cuts one or more objects from a prefab instance.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef EDITOR_UI_COMMAND_ADD_OBJECT_H
#define EDITOR_UI_COMMAND_ADD_OBJECT_H

#include "DevUICommand.h"
#include "HashSet.h"
#include "SharedPtr.h"
namespace Seoul { namespace EditorScene { class Container; } }
namespace Seoul { namespace Scene { class Object; } }

#if SEOUL_WITH_SCENE

namespace Seoul::EditorUI
{

class CommandAddObject SEOUL_SEALED : public DevUI::Command
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(CommandAddObject);

	typedef HashSet<SharedPtr<Scene::Object>, MemoryBudgets::Editor> Objects;

	CommandAddObject(
		EditorScene::Container& rScene,
		SharedPtr<Scene::Object>& rpLastSelection,
		Objects& rtSelectedObjects,
		const SharedPtr<Scene::Object>& pObject);
	~CommandAddObject();

	virtual Bool CanUndo() const SEOUL_OVERRIDE
	{
		return true;
	}

	virtual void Do() SEOUL_OVERRIDE;
	virtual const String& GetDescription() const SEOUL_OVERRIDE;
	virtual UInt32 GetSizeInBytes() const SEOUL_OVERRIDE;
	virtual void Undo() SEOUL_OVERRIDE;

private:
	EditorScene::Container& m_rScene;
	SharedPtr<Scene::Object>& m_rpLastSelection;
	Objects& m_rtSelectedObjects;
	SharedPtr<Scene::Object> m_pObject;
	Objects const m_tPrevSelection;
	SharedPtr<Scene::Object> const m_pLastSelection;
	String const m_sDescription;

	SEOUL_DISABLE_COPY(CommandAddObject);
}; // class CommandAddObject

} // namespace Seoul::EditorUI

#endif // /#if SEOUL_WITH_SCENE

#endif // include guard
