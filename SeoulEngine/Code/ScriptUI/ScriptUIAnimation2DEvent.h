/**
 * \file ScriptUIAnimation2DEvent.h
 * \brief Binds a script function as an Animation2D::EventInterface
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCRIPT_UI_ANIMATION2D_EVENT_H
#define SCRIPT_UI_ANIMATION2D_EVENT_H

#include "AnimationEventInterface.h"
#include "SeoulHString.h"
#include "SeoulString.h"
#include "Vector.h"
namespace Seoul { namespace Script { class VmObject; } }

#if SEOUL_WITH_ANIMATION_2D

namespace Seoul
{

class ScriptUIAnimation2DEvent SEOUL_SEALED : public Animation::EventInterface
{
public:
	ScriptUIAnimation2DEvent(const SharedPtr<Script::VmObject>& p);
	~ScriptUIAnimation2DEvent();

	virtual void DispatchEvent(HString name, Int32 i, Float32 f, const String& s) SEOUL_OVERRIDE;
	virtual void Tick(Float fDeltaTimeInSeconds) SEOUL_OVERRIDE;

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(ScriptUIAnimation2DEvent);

	SharedPtr<Script::VmObject> m_p;

	struct EventEntry
	{
		EventEntry(HString id = HString(), Int32 i = 0, Float32 f = 0.0f, const String& s = String())
			: m_i(i)
			, m_f(f)
			, m_s(s)
			, m_Id(id)
		{
		}

		Int32 m_i;
		Float32 m_f;
		String m_s;
		HString m_Id;
	}; // struct EventEntry
	typedef Vector<EventEntry, MemoryBudgets::Animation2D> Entries;
	Entries m_vEntries;

	SEOUL_DISABLE_COPY(ScriptUIAnimation2DEvent);
}; // class ScriptUIAnimation2DEvent

} // namespace Seoul

#endif // /#if SEOUL_WITH_ANIMATION_2D

#endif // include guard
