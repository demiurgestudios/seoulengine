/**
 * \file SoundEvent.cpp
 * \brief A Sound::Event can be thought of as a sound effect with more
 * flexibility and complexity. A single Sound::Event can contain multiple
 * raw wave files, various audio processing, and Sound::Events can also
 * react to runtime variables and modify their behavior based on changes
 * to these variables.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "SoundEvent.h"
#include "ThreadId.h"

namespace Seoul::Sound
{

/**
 * Initialize this Sound::Event to a default, invalid state.
 */
Event::Event()
	: m_uFlags(kNone)
{
}

Event::~Event()
{
	SEOUL_ASSERT(IsMainThread());
}

NullEvent::NullEvent()
	: m_Key()
{
}

NullEvent::~NullEvent()
{
}

Event* NullEvent::Clone() const
{
	NullEvent* pReturn = SEOUL_NEW(MemoryBudgets::Audio) NullEvent;
	pReturn->m_Key = m_Key;
	return pReturn;
}

} // namespace Seoul::Sound
