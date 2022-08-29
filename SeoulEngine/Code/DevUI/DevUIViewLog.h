/**
 * \file DevUIViewLog.h
 * \brief In-game log display.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef DEV_UI_VIEW_LOG_H
#define DEV_UI_VIEW_LOG_H

#include "DevUIView.h"
#include "List.h"
#include "Logger.h"
#include "SeoulTime.h"
#include "Singleton.h"

#if (SEOUL_ENABLE_DEV_UI && SEOUL_LOGGING_ENABLED)

namespace Seoul
{

namespace DevUI
{

struct LogMessage SEOUL_SEALED
{
	LogMessage(
		const String& sLine = String(),
		const WorldTime& timestamp = WorldTime(),
		LoggerChannel eChannel = LoggerChannel::Default,
		UInt32 uHash = 0u)
		: m_sLine(sLine)
		, m_Timestamp(timestamp)
		, m_eChannel(eChannel)
		, m_uHash(uHash)
		, m_uCount(1u)
		, m_bViewed(false)
		, m_bArchived(false)
	{
	}

	String m_sLine;
	WorldTime m_Timestamp;
	LoggerChannel m_eChannel;
	UInt32 m_uHash;
	UInt32 m_uCount;
	Bool m_bViewed;
	Bool m_bArchived;
};

struct HashableLogBufferIterator SEOUL_SEALED
{
	typedef List<LogMessage, MemoryBudgets::DevUI> LogBuffer;

	HashableLogBufferIterator()
		: m_Iter{}
		, m_bIsValid(false)
	{
	}

	explicit HashableLogBufferIterator(LogBuffer::Iterator const& iter)
		: m_Iter(iter)
		, m_bIsValid(true)
	{
	}

	LogBuffer::Iterator m_Iter;
	Bool m_bIsValid;

	Bool operator==(HashableLogBufferIterator const& other) const
	{
		if (other.m_bIsValid && m_bIsValid)
		{
			return other.m_Iter->m_eChannel == m_Iter->m_eChannel && other.m_Iter->m_sLine == m_Iter->m_sLine;
		}
		else
		{
			return other.m_bIsValid == m_bIsValid;
		}
	}
	Bool operator!=(HashableLogBufferIterator const& other) const
	{
		return !(*this == other);
	}
};

static inline UInt32 GetHash(const HashableLogBufferIterator& iter)
{
	return (iter.m_bIsValid) ? iter.m_Iter->m_uHash : 0;
}

} // namespace DevUI

template <>
struct DefaultHashTableKeyTraits<DevUI::HashableLogBufferIterator>
{
	inline static Float GetLoadFactor()
	{
		return 0.75f;
	}

	inline static DevUI::HashableLogBufferIterator GetNullKey()
	{
		return DevUI::HashableLogBufferIterator();
	}

	static const Bool kCheckHashBeforeEquals = true;
};

namespace DevUI
{

class ViewLog SEOUL_SEALED : public View, public Singleton<ViewLog>
{
public:
	ViewLog();
	~ViewLog();

	static HString GetStaticId()
	{
		static const HString kId("Log");
		return kId;
	}

	// DevUI::View overrides
	virtual HString GetId() const SEOUL_OVERRIDE
	{
		return GetStaticId();
	}

private:
	virtual Bool IsCloseable() const SEOUL_OVERRIDE;

	virtual void DoPrePose(Controller& rController, RenderPass& rPass) SEOUL_OVERRIDE;
	virtual void DoPrePoseAlways(Controller& rController, RenderPass& rPass, Bool bVisible) SEOUL_OVERRIDE;
	virtual UInt32 GetFlags() const SEOUL_OVERRIDE;
	virtual Vector2D GetInitialSize() const SEOUL_OVERRIDE
	{
		return Vector2D(500, 600);
	}
	// /DevUIView overrides

	// Warning handling.
	static Bool OnLogMessageStatic(const String& sLine, const WorldTime& timestamp, LoggerChannel eChannel)
	{
		if (!ViewLog::Get())
		{
			return false;
		}

		return ViewLog::Get()->OnLogMessage(sLine, timestamp, eChannel);
	}
	Bool OnLogMessage(const String& sLine, const WorldTime& timestamp, LoggerChannel eChannel);

	typedef LogMessage Message;
	typedef List<Message, MemoryBudgets::DevUI> LogBuffer;
	typedef HashSet<HashableLogBufferIterator, MemoryBudgets::DevUI> DuplicateTable;
	FixedArray<Bool, (UInt32)LoggerChannel::MaxChannel> m_abChannelsEnabled;
	DuplicateTable m_tDuplicates;
	LogBuffer m_lLogBuffer;
	Int32 m_iImportantCount;
	Mutex m_Mutex;
	Atomic32Value<Bool> m_bImportantShow;
	Double m_fImportantTimeRemainingInSeconds;

	SEOUL_DISABLE_COPY(ViewLog);
}; // class DevUI::ViewLog

} // namespace DevUI

} // namespace Seoul

#endif // /(SEOUL_ENABLE_DEV_UI && SEOUL_LOGGING_ENABLED)

#endif // include guard
