/**
 * \file ScriptEngineInputManager.cpp
 * \brief Binder instance for exposing the FileManager Singleton into
 * a script VM.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "InputManager.h"
#include "ReflectionDefine.h"
#include "ScriptEngineInputManager.h"
#include "ScriptFunctionInterface.h"

namespace Seoul
{

SEOUL_BEGIN_TYPE(ScriptEngineInputManager, TypeFlags::kDisableCopy)
	SEOUL_METHOD(GetMousePosition)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "(double, double)")
	SEOUL_METHOD(HasSystemBindingLock)
	SEOUL_METHOD(IsBindingDown)
	SEOUL_METHOD(WasBindingPressed)
	SEOUL_METHOD(WasBindingReleased)
SEOUL_END_TYPE()

ScriptEngineInputManager::ScriptEngineInputManager()
{
}

ScriptEngineInputManager::~ScriptEngineInputManager()
{
}

void ScriptEngineInputManager::GetMousePosition(Script::FunctionInterface* pInterface) const
{
	auto pos = InputManager::Get()->GetMousePosition();
	pInterface->PushReturnInteger(pos.X);
	pInterface->PushReturnInteger(pos.Y);
}

Bool ScriptEngineInputManager::HasSystemBindingLock() const
{
	return InputManager::Get()->HasSystemBindingLock();
}

Bool ScriptEngineInputManager::IsBindingDown(HString bindingName) const
{
	return InputManager::Get()->IsBindingDown(bindingName);
}

Bool ScriptEngineInputManager::WasBindingPressed(HString bindingName) const
{
	return InputManager::Get()->WasBindingPressed(bindingName);
}

Bool ScriptEngineInputManager::WasBindingReleased(HString bindingName) const
{
	return InputManager::Get()->WasBindingReleased(bindingName);
}

} // namespace Seoul
