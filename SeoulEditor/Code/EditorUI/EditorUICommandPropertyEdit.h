/**
 * \file EditorUICommandPropertyEdit.h
 * \brief Subclass of UndoAction for mutations via generic
 * reflection properties.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef EDITOR_UI_COMMAND_PROPERTY_EDIT_H
#define EDITOR_UI_COMMAND_PROPERTY_EDIT_H

#include "DevUICommand.h"
#include "EditorUIPropertyUtil.h"
#include "ReflectionAny.h"

namespace Seoul::EditorUI
{

class IPropertyChangeBinder SEOUL_ABSTRACT
{
public:
	SEOUL_REFLECTION_POLYMORPHIC_BASE(IPropertyChangeBinder);

	typedef Vector<Reflection::Any, MemoryBudgets::Editor> Values;

	virtual ~IPropertyChangeBinder()
	{
	}

	virtual Bool Equals(IPropertyChangeBinder const* pB) const = 0;

	virtual String GetDescription() const = 0;
	virtual UInt32 GetSizeInBytes() const = 0;

	virtual void SetValue(
		const PropertyUtil::Path& vPath,
		const Reflection::Any& anyValue) = 0;
	virtual void SetValues(
		const PropertyUtil::Path& vPath,
		const Values& vValues) = 0;

protected:
	IPropertyChangeBinder()
	{
	}

private:
	SEOUL_DISABLE_COPY(IPropertyChangeBinder);
}; // class IPropertyChangeBinder

class CommandPropertyEdit SEOUL_SEALED : public DevUI::Command
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(CommandPropertyEdit);

	CommandPropertyEdit(
		IPropertyChangeBinder* pBinder,
		const PropertyUtil::Path& vPath,
		const IPropertyChangeBinder::Values& vOldValues,
		const IPropertyChangeBinder::Values& vNewValues);
	~CommandPropertyEdit();

	virtual Bool CanUndo() const SEOUL_OVERRIDE
	{
		return true;
	}

	virtual void Do() SEOUL_OVERRIDE;
	virtual const String& GetDescription() const SEOUL_OVERRIDE;
	virtual UInt32 GetSizeInBytes() const SEOUL_OVERRIDE;
	virtual void Undo() SEOUL_OVERRIDE;

private:
	ScopedPtr<IPropertyChangeBinder> const m_pBinder;
	PropertyUtil::Path const m_vPath;
	IPropertyChangeBinder::Values const m_vOldValues;
	IPropertyChangeBinder::Values m_vNewValues;
	String const m_sDescription;

	virtual Bool DoMerge(DevUI::Command const* pCommand) SEOUL_OVERRIDE;

	SEOUL_DISABLE_COPY(CommandPropertyEdit);
}; // class CommandPropertyEdit

} // namespace Seoul::EditorUI

#endif // include guard
