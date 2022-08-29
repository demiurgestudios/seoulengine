/**
 * \file ContentChangeNotifier.h
 * \brief Content::ChangeNotifier is the common base class for platform/config specific
 * subclasses which monitor and dispatch events to Source/ and Data/Config/ content,
 * allowing the game to react to these events (typically, to recook and reload the
 * content).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef CONTENT_CHANGE_NOTIFIER_H
#define CONTENT_CHANGE_NOTIFIER_H

#include "AtomicRingBuffer.h"
#include "FileChangeNotifier.h"
#include "FilePath.h"
#include "SharedPtr.h"
#include "Prereqs.h"
#include "SeoulTime.h"
#include "Singleton.h"

namespace Seoul
{

// Forward declarations
class Thread;

namespace Content
{

/**
 * Encapsulates a single change to content on disk.
 */
struct ChangeEvent
{
	ChangeEvent()
		: m_Old()
		, m_New()
		, m_eEvent(FileChangeNotifier::kUnknown)
		, m_iCurrentTimeInTicks(SeoulTime::GetGameTimeInTicks())
	{
	}

	ChangeEvent(FilePath oldFilePath, FilePath newFilePath, FileChangeNotifier::FileEvent eEvent)
		: m_Old(oldFilePath)
		, m_New(newFilePath)
		, m_eEvent(eEvent)
		, m_iCurrentTimeInTicks(SeoulTime::GetGameTimeInTicks())
	{
	}

	/** For rename events, the old name. Otherwise, equal to m_New. */
	FilePath m_Old;

	/** For all events, the current filename on disk. */
	FilePath m_New;

	/** Even type - add, modified, renamed, etc. */
	FileChangeNotifier::FileEvent m_eEvent;

	/** Time in ticks of the event - this is the Seoul::GetGameTimeInTicks() when the event was received. */
	Int64 m_iCurrentTimeInTicks;

private:
	SEOUL_REFERENCE_COUNTED(ChangeEvent);
}; // struct Content::ChangeEvent

/**
 * Content::ChangeNotifier is a wrapper around FileChangeNotifier, specifically designed
 * to monitor source and config content, and dispatch change events for this content.
 */
class ChangeNotifier SEOUL_ABSTRACT : public Singleton<ChangeNotifier>
{
public:
	typedef AtomicRingBuffer<ChangeEvent*> Changes;

	virtual ~ChangeNotifier();

	/**
	 * @return The next entry on the content changes queue, or nullptr if there are no
	 * entries.
	 */
	ChangeEvent* Pop()
	{
		return m_OutgoingContentChanges.Pop();
	}

protected:
	ChangeNotifier();

	Changes m_OutgoingContentChanges;

	SEOUL_DISABLE_COPY(ChangeNotifier);
}; // class Content::ChangeNotifier

/**
 * Specialization of Content::ChangeNotifier for platforms that have
 * no support for content change events.
 */
class NullChangeNotifier SEOUL_SEALED : public ChangeNotifier
{
public:
	NullChangeNotifier()
	{
	}

	~NullChangeNotifier()
	{
	}

private:
	SEOUL_DISABLE_COPY(NullChangeNotifier);
}; // class NullChangeNotifier

} // namespace Content

} // namespace Seoul

#endif // include guard
