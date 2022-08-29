/**
 * \file ScriptUIAnimation2DEvent.cpp
 * \brief Binds a script function as an Animation2D::EventInterface
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ScriptFunctionInvoker.h"
#include "ScriptUIAnimation2DEvent.h"
#include "ScriptVm.h"

#if SEOUL_WITH_ANIMATION_2D

namespace Seoul
{

ScriptUIAnimation2DEvent::ScriptUIAnimation2DEvent(const SharedPtr<Script::VmObject>& p)
	: m_p(p)
	, m_vEntries()
{
}

ScriptUIAnimation2DEvent::~ScriptUIAnimation2DEvent()
{
}

void ScriptUIAnimation2DEvent::DispatchEvent(HString name, Int32 i, Float32 f, const String& s)
{
	m_vEntries.PushBack(EventEntry(
		name,
		i,
		f,
		s));
}

void ScriptUIAnimation2DEvent::Tick(Float fDeltaTimeInSeconds)
{
	// Nothing to do if no events to dispatch.
	if (m_vEntries.IsEmpty())
	{
		return;
	}

	// TODO: Submit these in a batch instead of one
	// at a time.

	// Enumerate events and bundle.
	for (auto i = m_vEntries.Begin(); m_vEntries.End() != i; ++i)
	{
		auto const& e = *i;

		Script::FunctionInvoker invoker(m_p);
		if (!invoker.IsValid())
		{
			continue;
		}

		invoker.PushString(e.m_Id);
		invoker.PushInteger(e.m_i);
		invoker.PushNumber(e.m_f);
		invoker.PushString(e.m_s);

		// Dispatch event.
		if (!invoker.TryInvoke())
		{
			Bool bFalse = true;
			(void)bFalse;
		}
	}

	m_vEntries.Clear();
}

} // namespace Seoul

#endif // /#if SEOUL_WITH_ANIMATION_2D
