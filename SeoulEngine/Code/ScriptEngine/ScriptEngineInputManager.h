/**
 * \file ScriptEngineInputManager.h
 * \brief Binder instance for exposing the InputManager Singleton into
 * a script VM.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCRIPT_ENGINE_INPUT_MANAGER_H
#define SCRIPT_ENGINE_INPUT_MANAGER_H

#include "Prereqs.h"
namespace Seoul { namespace Script { class FunctionInterface; } }

namespace Seoul
{

/** Binder, wraps a Camera instance and exposes functionality to a script VM. */
class ScriptEngineInputManager SEOUL_SEALED
{
public:
	ScriptEngineInputManager();
	~ScriptEngineInputManager();

	void GetMousePosition(Script::FunctionInterface* pInterface) const;
	Bool HasSystemBindingLock() const;
	Bool IsBindingDown(HString bindingName) const;
	Bool WasBindingPressed(HString bindingName) const;
	Bool WasBindingReleased(HString bindingName) const;

private:
	SEOUL_DISABLE_COPY(ScriptEngineInputManager);
}; // class ScriptEngineInputManager

} // namespace Seoul

#endif // include guard
