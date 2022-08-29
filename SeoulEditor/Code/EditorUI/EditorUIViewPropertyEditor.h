/**
 * \file EditorUIViewPropertyEditor.h
 * \brief An editor view that displays
 * a tool for working with hierarchical data.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef EDITOR_UI_VIEW_PROPERTY_EDITOR_H
#define EDITOR_UI_VIEW_PROPERTY_EDITOR_H

#include "DevUIView.h"
#include "EditorUIPropertyUtil.h"
#include "HashTable.h"
#include "ReflectionAny.h"
namespace Seoul { namespace EditorUI { class IControllerPropertyEditor; } }
namespace Seoul { namespace Reflection { class Array; } }
namespace Seoul { namespace Reflection { class AttributeCollection; } }
namespace Seoul { namespace Reflection { class Property; } }
namespace Seoul { namespace Reflection { class Type; } }

namespace Seoul::EditorUI
{

namespace ViewPropertyEditorUtil
{

struct Storage
{
	Vector<Byte, MemoryBudgets::Editor> m_vText;
};

} // namespace ViewPropertyEditorUtil

class ViewPropertyEditor : public DevUI::View
{
public:
	typedef Vector<Reflection::Any, MemoryBudgets::Editor> Scratch;
	typedef Vector<Reflection::WeakAny, MemoryBudgets::Editor> Stack;

	ViewPropertyEditor();
	virtual ~ViewPropertyEditor();

	// DevUI::View overrides
	virtual HString GetId() const SEOUL_OVERRIDE
	{
		static const HString kId("Property Editor");
		return kId;
	}
	// /DevUI::View overrides

protected:
	// DevUI::View overrides
	virtual void DoPrePose(DevUI::Controller& rController, RenderPass& rPass) SEOUL_OVERRIDE;
	// /DevUI::View overrides

	ViewPropertyEditorUtil::Storage m_Storage;
	typedef Bool(*ValueFunc)(
		IControllerPropertyEditor& rController,
		PropertyUtil::Path& rvPath,
		ViewPropertyEditorUtil::Storage& r,
		const ViewPropertyEditor::Stack::ConstIterator& iBegin,
		const ViewPropertyEditor::Stack::ConstIterator& iEnd,
		const Reflection::Property& prop);
	typedef HashTable<Reflection::TypeInfo const*, ValueFunc> ValueTypes;
	ValueTypes m_tValueTypes;

	Scratch m_vScratch;
	Stack m_vStack;

	Bool Complex(
		IControllerPropertyEditor& rController,
		PropertyUtil::Path& rvPath,
		const Reflection::Type& type,
		UInt32 uBegin,
		UInt32 uEnd);
	Bool ComplexDataStore(
		IControllerPropertyEditor& rController,
		PropertyUtil::Path& rvPath,
		const Reflection::Type& type,
		UInt32 uBegin,
		UInt32 uEnd);
	Bool ComplexType(
		IControllerPropertyEditor& rController,
		PropertyUtil::Path& rvPath,
		const Reflection::Type& type,
		UInt32 uBegin,
		UInt32 uEnd);
	Bool Prop(
		IControllerPropertyEditor& rController,
		PropertyUtil::Path& rvPath,
		const Reflection::Property& prop,
		UInt32 uBegin,
		UInt32 uEnd);

	Bool GetValueFunc(const Reflection::Property& prop, ValueFunc& rFunc) const;
	void PopulateValueTypes();

	SEOUL_DISABLE_COPY(ViewPropertyEditor);
}; // class ViewPropertyEditor

} // namespace Seoul::EditorUI

#endif // include guard
