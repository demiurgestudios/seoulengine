/**
 * \file ScriptAnimation2DManager.h
 * \brief Script binding for Animation2DManager
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCRIPT_ANIMATION_2D_MANAGER_H
#define SCRIPT_ANIMATION_2D_MANAGER_H

#include "FilePath.h"
#include "Prereqs.h"
namespace Seoul { namespace Script { class FunctionInterface; } }

namespace Seoul
{

#if SEOUL_WITH_ANIMATION_2D

class ScriptAnimation2DManager SEOUL_SEALED
{
public:
	void GetQuery(Script::FunctionInterface* pInterface) const;
}; // class ScriptAnimation2DManager

#endif // /#if SEOUL_WITH_ANIMATION_2D

} // namespace Seoul

#endif // include guard
