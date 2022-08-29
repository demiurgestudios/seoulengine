/**
 * \file ScriptUIMotionCompletion.cpp
 * \brief Subclass of ScriptUIMotionCompletion, invokes a script callback.
 *
 * ScriptUIMotionCompletion binds a Script::VmObject as a UI::Motion completion callback.
 * Completion of the UI::Motion invokes the script function.
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
#include "ScriptUIMotionCompletion.h"
#include "ScriptVm.h"

namespace Seoul
{

ScriptUIMotionCompletion::ScriptUIMotionCompletion(const SharedPtr<Script::VmObject>& pObject)
	: m_pObject(pObject)
{
}

ScriptUIMotionCompletion::~ScriptUIMotionCompletion()
{
}

void ScriptUIMotionCompletion::OnComplete()
{
	Script::FunctionInvoker invoker(m_pObject);
	if (invoker.IsValid())
	{
		(void)invoker.TryInvoke();
	}
}

} // namespace Seoul
