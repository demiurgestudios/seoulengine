/**
 * \file EditorUIIControllerPropertyEditor.h
 * \brief Shared interface for access to target objects
 * when bound to an property editor view.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef EDITOR_UI_ICONTROLLER_PROPERTY_EDITOR_H
#define EDITOR_UI_ICONTROLLER_PROPERTY_EDITOR_H

#include "EditorUIPropertyUtil.h"
#include "Prereqs.h"
#include "ReflectionDeclare.h"

namespace Seoul::EditorUI
{

class IControllerPropertyEditor SEOUL_ABSTRACT
{
public:
	SEOUL_REFLECTION_POLYMORPHIC_BASE(IControllerPropertyEditor);

	virtual ~IControllerPropertyEditor()
	{
	}

	typedef Vector<Reflection::Any, MemoryBudgets::Editor> PropertyValues;
	virtual void CommitPropertyEdit(
		const PropertyUtil::Path& vPath,
		const PropertyValues& vValues,
		const PropertyValues& vNewValues) = 0;

	virtual Bool GetPropertyButtonContext(Reflection::Any& rContext) const = 0;

	typedef Vector<Reflection::WeakAny, MemoryBudgets::Editor> Instances;
	virtual Bool GetPropertyTargets(Instances& rvInstances) const = 0;

protected:
	IControllerPropertyEditor()
	{
	}

private:
	SEOUL_DISABLE_COPY(IControllerPropertyEditor);
}; // class IControllerPropertyEditor

} // namespace Seoul::EditorUI

#endif // include guard
