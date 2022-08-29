/**
 * \file EditorUICommandSetComponent.h
 * \brief Command for mutation of an Object's attached components.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef EDITOR_UI_COMMAND_SET_COMPONENT_H
#define EDITOR_UI_COMMAND_SET_COMPONENT_H

#include "DevUICommand.h"
#include "HashSet.h"
#include "SharedPtr.h"
namespace Seoul { namespace Scene { class Component; } }
namespace Seoul { namespace Scene { class Object; } }

#if SEOUL_WITH_SCENE

namespace Seoul::EditorUI
{

class CommandSetComponent SEOUL_SEALED : public DevUI::Command
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(CommandSetComponent);

	CommandSetComponent(
		const SharedPtr<Scene::Object>& pObject,
		const SharedPtr<Scene::Component>& pOld,
		const SharedPtr<Scene::Component>& pNew);
	~CommandSetComponent();

	virtual Bool CanUndo() const SEOUL_OVERRIDE
	{
		return true;
	}

	virtual void Do() SEOUL_OVERRIDE;
	virtual const String& GetDescription() const SEOUL_OVERRIDE;
	virtual UInt32 GetSizeInBytes() const SEOUL_OVERRIDE;
	virtual void Undo() SEOUL_OVERRIDE;

private:
	SharedPtr<Scene::Object> const m_pObject;
	SharedPtr<Scene::Component> const m_pOld;
	SharedPtr<Scene::Component> const m_pNew;
	String const m_sDescription;

	SEOUL_DISABLE_COPY(CommandSetComponent);
}; // class CommandSetComponent

} // namespace Seoul::EditorUI

#endif // /#if SEOUL_WITH_SCENE

#endif // include guard
