/**
 * \file StreamBuffer.cpp
 *
 * \brief In memory buffer with file IO semantics. Useful for
 * preparing a byte array for serialization or deserializing an
 * entire file in one operation, and then reading individual fields
 * out from memory.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "SeoulHString.h"
#include "SeoulTypes.h"
#include "Vector.h"
#include "List.h"
#include "Latch.h"
#include "Logger.h"

namespace Seoul
{

Latch::Latch()
{
	Reset();
}

Latch::Latch(const Vector<String>& vConditions)
{
	Reset(vConditions);
}

Latch::Latch(Byte const* aConditions[], UInt uLength)
{
	Reset(aConditions, uLength);
}

Latch::~Latch()
{
	Terminate();
}

ELatchStatus Latch::GetStatus() const
{
	return m_eStatus;
}

Bool Latch::Check(const String& sCondition) const
{
	return m_lConditions.End() != Find(m_lConditions.Begin(), m_lConditions.End(), sCondition);
}

ELatchStatus Latch::Trigger(const String& sCondition)
{
	SEOUL_ASSERT(m_eStatus != eLatchError);
	if (m_eStatus == eLatchNew)
	{
		m_eStatus = eLatchOpen;
	}
	if (m_eStatus == eLatchOpen)
	{
		if (m_lConditions.GetSize() > 0)
		{
			// As long as the latch has conditions, it's open. Remove
			// conditions with the matching name.
			m_lConditions.Remove(sCondition);
		}

		if (m_lConditions.GetSize() == 0)
		{
			// We weren't closed before, but we have no more conditions.
			// Close the latch and execute the action.
			m_eStatus = eLatchClosed;
			Execute();
		}
	}

	return m_eStatus;
}
ELatchStatus Latch::Trigger(const Vector<String>& vConditions)
{
	SEOUL_ASSERT(m_eStatus != eLatchError);
	if (m_eStatus == eLatchNew)
	{
		m_eStatus = eLatchOpen;
	}
	if (m_eStatus == eLatchOpen)
	{
		Vector<String>::ConstIterator iCondition = vConditions.Begin();
		Vector<String>::ConstIterator iEnd = vConditions.End();
		while ((iCondition != iEnd) && (m_lConditions.GetSize() > 0))
		{
			// As long as the latch has conditions, it's open. Remove
			// conditions with the matching name.
			m_lConditions.Remove(*iCondition);
			iCondition++;
		}

		if (m_lConditions.GetSize() == 0)
		{
			// We weren't closed before, but we have no more conditions.
			// Close the latch and execute the action.
			m_eStatus = eLatchClosed;
			Execute();
		}
	}

	return m_eStatus;
}
ELatchStatus Latch::Step(const String& sCondition)
{
	SEOUL_ASSERT(m_eStatus != eLatchError);
	if (m_eStatus == eLatchNew)
	{
		m_eStatus = eLatchOpen;
	}
	if (m_eStatus == eLatchOpen)
	{
		if (m_lConditions.GetSize() > 0)
		{
			if (m_lConditions.Front() == sCondition)
			{
				// As long as the latch has conditions, it's open. Remove
				// the first condition if it matches.
				m_lConditions.PopFront();
			}
		}

		if (m_lConditions.GetSize() == 0)
		{
			// We weren't closed before, but we have no more conditions.
			// Close the latch and execute the action.
			m_eStatus = eLatchClosed;
			Execute();
		}
	}

	return m_eStatus;
}

void Latch::Force()
{
	m_lConditions.Clear();

	SEOUL_ASSERT(m_eStatus != eLatchError);
	if (m_eStatus != eLatchClosed)
	{
		// We weren't closed before, but we have no more conditions.
		// Close the latch and execute the actor.
		m_eStatus = eLatchClosed;
		Execute();
	}
}

// Note: derivatives must call one of the base Reset() functions!
void Latch::Reset(void)
{
	m_lConditions.Clear();
	m_eStatus = eLatchNew;
}

void Latch::Reset(const Vector<String>& vConditions)
{
	UInt32 index = 0;
	m_lConditions.Clear();
	while (index < vConditions.GetSize())
	{
		m_lConditions.PushBack(vConditions[index]);
		index++;
	}
	m_eStatus = eLatchNew;
}

void Latch::Reset(Byte const* aConditions[], UInt32 uLength)
{
	UInt32 index = 0;
	m_lConditions.Clear();
	while (index < uLength)
	{
		m_lConditions.PushBack(aConditions[index]);
		index++;
	}
	m_eStatus = eLatchNew;
}

void Latch::Require(const String& sCondition)
{
	switch (m_eStatus)
	{
	case eLatchClosed:
		m_lConditions.Clear();
		m_eStatus = eLatchNew;
		// Note: fall through to the new/open case.

	case eLatchNew:
	case eLatchOpen:
		if (!Check(sCondition))
		{
			// Condition doesn't already exist.
			m_lConditions.PushBack(sCondition);
		}
		break;

	default:
		SEOUL_WARN("Latch is not in a changeable state.");
	}
}

void Latch::Terminate()
{
	m_eStatus = eLatchError;
	m_lConditions.Clear();
}

} // namespace Seoul
