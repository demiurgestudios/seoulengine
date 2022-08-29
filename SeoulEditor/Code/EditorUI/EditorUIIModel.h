/**
 * \file EditorUIIModel.h
 * \brief Base interface for a model component (in the
 * model-view-controller).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef EDITOR_UI_IMODEL_H
#define EDITOR_UI_IMODEL_H

#include "Prereqs.h"
#include "ReflectionDeclare.h"

namespace Seoul::EditorUI
{

class IModel SEOUL_ABSTRACT
{
public:
	SEOUL_REFLECTION_POLYMORPHIC_BASE(IModel);

	virtual ~IModel()
	{
	}

protected:
	IModel()
	{
	}

private:
	SEOUL_DISABLE_COPY(IModel);
}; // class IModel

} // namespace Seoul::EditorUI

#endif // include guard
