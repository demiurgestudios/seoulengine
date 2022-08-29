/**
 * \file EditorUILogBuffer.cpp
 * \brief Global singleton that hooks into logging
 * for the editor and manages a rotating log buffer.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "EditorUILogBuffer.h"
#include "HashFunctions.h"

namespace Seoul
{

namespace EditorUI
{

#if SEOUL_LOGGING_ENABLED

static const UInt32 kuMaxLogBufferSize = 40u;

LogBuffer::LogBuffer()
	: m_lLogBuffer()
	, m_Mutex()
	, m_UpdateCount(0)
{
	// Set ourselves as the warning handler.
	Logger::GetSingleton().RegisterCallback(&LogBuffer::OnLogMessageStatic);
}

LogBuffer::~LogBuffer()
{
	// Done as the warning handler.
	Logger::GetSingleton().UnregisterCallback(&LogBuffer::OnLogMessageStatic);
}

Bool LogBuffer::OnLogMessage(
	const String& sLine,
	const WorldTime& timestamp,
	LoggerChannel eChannel)
{
	// Compute hash 64 on the line.
	UInt64 const uHash = GetHash64(sLine);

	Bool bNew = false;

	// Lock our mutex for the duration of processing.
	{
		Lock lock(m_Mutex);

		// Check for a duplicate.
		InternalLogBuffer::Iterator i;
		if (m_tDuplicates.GetValue(uHash, i))
		{
			// Increment count.
			i->m_uCount++;
		}
		// Otherwise, generate a new entry.
		else
		{
			i = m_lLogBuffer.Insert(m_lLogBuffer.End(), Message(sLine, timestamp, eChannel, uHash));
			SEOUL_VERIFY(m_tDuplicates.Insert(uHash, i).Second);
			bNew = true;
		}

		// Prune to stay below our max
		UInt32 const uCount = m_lLogBuffer.GetSize();
		for (UInt32 i = kuMaxLogBufferSize; i < uCount; ++i)
		{
			// Erase the entry.
			SEOUL_VERIFY(m_tDuplicates.Erase(m_lLogBuffer.Front().m_uHash));
			m_lLogBuffer.PopFront();
		}
	}

	// Update if a new entry was received.
	if (bNew)
	{
		++m_UpdateCount;
	}

	// Always handled.
	return true;
}

} // namespace EditorUI

#endif // /#if SEOUL_LOGGING_ENABLED

} // namespace Seoul
