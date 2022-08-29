/**
 * \file ScriptUITweenCompletion.cpp
 * \brief Subclass of UI::TweenCompletionInterface, invokes a script callback.
 *
 * ScriptUITweenCompletion binds a Script::VmObject as a tween completion callback.
 * Completion of the tween invokes the script function.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Logger.h"
#include "ReflectionUtil.h"
#include "ScriptFunctionInvoker.h"
#include "ScriptUITweenCompletion.h"
#include "ScriptVm.h"

namespace Seoul
{

ScriptUITweenCompletion::ScriptUITweenCompletion(const SharedPtr<Script::VmObject>& pObject)
	: m_pObject(pObject)
{
}

ScriptUITweenCompletion::~ScriptUITweenCompletion()
{
}

void ScriptUITweenCompletion::OnComplete()
{
	Script::FunctionInvoker invoker(m_pObject);
	if (invoker.IsValid())
	{
		(void)invoker.TryInvoke();
	}
}

} // namespace Seoul
