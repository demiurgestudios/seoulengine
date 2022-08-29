/**
 * \file EditorUICommandTransformObjects.h
 * \brief DevUI::Command for wrapping a command that pastes
 * a Scene::Object into a EditorScene.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef EDITOR_UI_COMMAND_TRANSFORM_OBJECTS_H
#define EDITOR_UI_COMMAND_TRANSFORM_OBJECTS_H

#include "DevUICommand.h"
#include "EditorUITransform.h"
#include "SharedPtr.h"
#include "Matrix4D.h"
#include "Vector.h"
namespace Seoul { namespace Scene { class Object; } }

#if SEOUL_WITH_SCENE

namespace Seoul::EditorUI
{

class CommandTransformObjects SEOUL_SEALED : public DevUI::Command
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(CommandTransformObjects);

	struct Entry SEOUL_SEALED
	{
		SharedPtr<Scene::Object> m_pObject;
		Transform m_mTransform;
	}; // struct Entry
	typedef Vector<Entry, MemoryBudgets::Editor> Entries;

	CommandTransformObjects(
		const Entries& vEntries,
		const Transform& mReferenceTransform,
		const Transform& mTargetTransform);
	~CommandTransformObjects();

	virtual Bool CanUndo() const SEOUL_OVERRIDE
	{
		return true;
	}

	virtual void Do() SEOUL_OVERRIDE;
	virtual const String& GetDescription() const SEOUL_OVERRIDE;
	virtual UInt32 GetSizeInBytes() const SEOUL_OVERRIDE;
	virtual void Undo() SEOUL_OVERRIDE;

private:
	Entries const m_vEntries;
	typedef Vector<Transform, MemoryBudgets::Editor> Transforms;
	Transforms m_vTransforms;
	String const m_sDescription;

	void ComputeEntries(
		const Entries& vEntries,
		const Transform& mReferenceTransform,
		const Transform& mTargetTransform);
	virtual Bool DoMerge(DevUI::Command const* pCommand) SEOUL_OVERRIDE;

	SEOUL_DISABLE_COPY(CommandTransformObjects);
}; // class CommandTransformObjects

} // namespace Seoul::EditorUI

#endif // /#if SEOUL_WITH_SCENE

#endif // include guard
