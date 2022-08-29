/**
 * \file EditorUILogBuffer.h
 * \brief Global singleton that hooks into logging
 * for the editor and manages a rotating log buffer.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef EDITOR_UI_LOG_BUFFER_H
#define EDITOR_UI_LOG_BUFFER_H

#include "List.h"
#include "Logger.h"
#include "SeoulTime.h"
#include "Singleton.h"

namespace Seoul::EditorUI
{

#if SEOUL_LOGGING_ENABLED

class LogBuffer SEOUL_SEALED : public Singleton<LogBuffer>
{
public:
	struct Message SEOUL_SEALED
	{
		Message(const String& sLine = String(), const WorldTime& timestamp = WorldTime(), LoggerChannel eChannel = LoggerChannel::Default, UInt64 uHash = 0u)
			: m_sLine(sLine)
			, m_Timestamp(timestamp)
			, m_eChannel(eChannel)
			, m_uHash(uHash)
			, m_uCount(1u)
		{
		}

		String m_sLine;
		WorldTime m_Timestamp;
		LoggerChannel m_eChannel;
		UInt64 m_uHash;
		UInt32 m_uCount;
	};
	typedef List<Message, MemoryBudgets::DevUI> InternalLogBuffer;

	LogBuffer();
	~LogBuffer();

private:
	// Warning handling.
	static Bool OnLogMessageStatic(
		const String& sLine,
		const WorldTime& timestamp,
		LoggerChannel eChannel)
	{
		if (!LogBuffer::Get())
		{
			return false;
		}

		return LogBuffer::Get()->OnLogMessage(sLine, timestamp, eChannel);
	}
	Bool OnLogMessage(
		const String& sLine,
		const WorldTime& timestamp,
		LoggerChannel eChannel);

	friend class LogBufferLock;

	// Deliberately using a hash as the key for perf. - incorrect collisions
	// are not a big deal, so we're accepting them in this case.
	typedef HashTable<UInt64, InternalLogBuffer::Iterator, MemoryBudgets::DevUI> DuplicateTable;
	DuplicateTable m_tDuplicates;
	InternalLogBuffer m_lLogBuffer;
	Mutex m_Mutex;
	Atomic32 m_UpdateCount;

	SEOUL_DISABLE_COPY(LogBuffer);
}; // class LogBuffer

class LogBufferLock SEOUL_SEALED
{
public:
	LogBufferLock()
		: m_Lock(LogBuffer::Get()->m_Mutex)
	{
	}

	~LogBufferLock()
	{
	}

	Atomic32Type GetUpdateCount() const
	{
		return LogBuffer::Get()->m_UpdateCount;
	}

	const LogBuffer::InternalLogBuffer& GetLogBuffer() const
	{
		return LogBuffer::Get()->m_lLogBuffer;
	}

private:
	Lock m_Lock;

	SEOUL_DISABLE_COPY(LogBufferLock);
}; // class LogBufferLock

#endif // /#if SEOUL_LOGGING_ENABLED

} // namespace Seoul::EditorUI

#endif // include guard
