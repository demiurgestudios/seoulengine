/**
 * \file ThreadId.h
 * \brief Class used to uniquely identify a thread. Can be invalid.
 *
 * ThreadId is only useful to determine if 2 threads are equal, it
 * cannot in general be used to acquire a thread.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef THREAD_ID_H
#define THREAD_ID_H

#include "Prereqs.h"

#if SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
#include <pthread.h>
#endif

namespace Seoul
{

/** Platform-independent thread ID type */
class ThreadId SEOUL_SEALED
{
public:
#if SEOUL_PLATFORM_WINDOWS
	typedef UInt32 ThreadIdValueType;
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
	typedef size_t ThreadIdValueType;
#else
#	error "Define for this platform"
#endif

	ThreadId()
		: m_bValid(false)
	{
		memset(&m_ThreadId, 0, sizeof(m_ThreadId));
	}

	ThreadId(const ThreadId& b)
		: m_bValid(b.m_bValid)
	{
		memcpy(&m_ThreadId, &(b.m_ThreadId), sizeof(m_ThreadId));
	}

	explicit ThreadId(ThreadIdValueType value)
		: m_bValid(true)
	{
		memcpy(&m_ThreadId, &value, sizeof(value));
	}

	ThreadId& operator=(ThreadIdValueType value)
	{
		memcpy(&m_ThreadId, &value, sizeof(value));
		m_bValid = true;

		return *this;
	}

	ThreadId& operator=(const ThreadId& b)
	{
		memcpy(&m_ThreadId, &(b.m_ThreadId), sizeof(m_ThreadId));
		m_bValid = b.m_bValid;

		return *this;
	}

	Bool operator==(const ThreadId& b) const
	{
		return ((!m_bValid && !b.m_bValid) ||
			(m_bValid && b.m_bValid && m_ThreadId == b.m_ThreadId));
	}

	Bool operator!=(const ThreadId& b) const
	{
		return !(*this == b);
	}

	Bool IsValid() const
	{
		return m_bValid;
	}

	ThreadIdValueType GetValue() const
	{
		return m_ThreadId;
	}

private:
	ThreadIdValueType m_ThreadId;
	Bool m_bValid;
}; // class ThreadId

/**
 * Fixed enum set of identifiers used for threads that have
 * special meanings in Seoul Engine.
 */
enum class FixedThreadId
{
	/** Thread that should be used for all file IO (FileManager::*) operations. */
	kFileIO,

	/** Game simulation/main thread - on applicable platforms, the int main() entry point. */
	kMain,

	/** Thread on which all calls to the current platform's graphics API are made. */
	kRender,

	/** Placeholder used to compute the # of fixed threads. */
	COUNT,
};

// Return the ThreadId of the corresponding FixedThreadId.
ThreadId GetFixedThreadId(FixedThreadId eFixedThreadId);

// Update the thread that corresponds to eFixedThreadId - this should be called as early as possible in engine startup,
// the results of a thread id changing after being used are undefined.
void SetFixedThreadId(FixedThreadId eFixedThreadId, const ThreadId& id);

// Return true if the current thread is eFixedThreadId, false otherwise.
Bool IsFixedThread(FixedThreadId eFixedThreadId);

// Used on some platforms to clear thread ids on shutdown.
void ResetAllFixedThreadIds();

inline static ThreadId GetFileIOThreadId() { return GetFixedThreadId(FixedThreadId::kFileIO); }
inline static ThreadId GetMainThreadId() { return GetFixedThreadId(FixedThreadId::kMain); }
inline static ThreadId GetRenderThreadId() { return GetFixedThreadId(FixedThreadId::kRender); }

inline static void SetFileIOThreadId(const ThreadId& id) { SetFixedThreadId(FixedThreadId::kFileIO, id); }
inline static void SetMainThreadId(const ThreadId& id) { SetFixedThreadId(FixedThreadId::kMain, id); }
inline static void SetRenderThreadId(const ThreadId& id) { SetFixedThreadId(FixedThreadId::kRender, id); }

inline static Bool IsFileIOThread() { return IsFixedThread(FixedThreadId::kFileIO); }
inline static Bool IsMainThread() { return IsFixedThread(FixedThreadId::kMain); }
inline static Bool IsRenderThread() { return IsFixedThread(FixedThreadId::kRender); }

} // namespace Seoul

#endif // include guard
