/**
 * \file Thread.cpp
 * \brief Class that represents a thread in a multi-threaded environment.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Atomic32.h"
#include "FixedArray.h"
#include "PerThreadStorage.h"
#include "Platform.h"
#include "SeoulProfiler.h"
#include "SeoulString.h"
#include "Thread.h"
#include "ThreadInternal.h"

namespace Seoul
{

/**
 * @return A ThreadId object uniquely identifying this thread amongst other
 * threads active threads.
 *
 * ThreadId is not guaranteed to be unique forever. The raw
 * platform dependent value used by ThreadId can be reassigned if
 * a thread is destroyed and a new thread is created.
 */
ThreadId Thread::GetThisThreadId()
{
	return ThreadDetail::GetThisThreadId();
}

/**
 * @return True if the thread identified by id has not been destroyed,
 * false otherwise.
 *
 * This function is conservative. A return value of true does not
 * necessarily mean the thread is running, just that it can be found
 * by the platform's thread system and has not been destroyed.
 */
Bool Thread::IsThreadStillAlive(ThreadId id)
{
	return ThreadDetail::IsThreadStillAlive(id);
}

// Sanity check of behavior on the current platform.
SEOUL_STATIC_ASSERT(sizeof(void*) == sizeof(size_t));

/**
 * Creates a new thread. If bStart is true, this Thread
 * will begin running immediately, otherwise it will not run until
 * Thread::Start() is called explicitly.
 */
Thread::Thread(ThreadFunc func, Bool bStart)
	: m_uStackSize(kDefaultStackReservedSize)
	, m_Func(func)
	, m_hHandle()
	, m_iReturnValue(-1)
	, m_eState(kNotStarted)
{
	if (bStart)
	{
		Start();
	}
}

/**
 * Creates a new thread. If bStart is true, this Thread
 * will begin running immediately, otherwise it will not run until
 * Thread::Start() is called explicitly.
 */
Thread::Thread(ThreadFunc func, UInt uStackSize, Bool bStart)
	: m_uStackSize(uStackSize)
	, m_Func(func)
	, m_hHandle()
	, m_iReturnValue(-1)
	, m_eState(kNotStarted)
{
	if (bStart)
	{
		Start();
	}
}

Thread::~Thread()
{
	(void)WaitUntilThreadIsNotRunning();
	SEOUL_ASSERT(!IsRunning());

	ThreadDetail::DestroyThread(m_hHandle);
	SEOUL_ASSERT(!m_hHandle.IsValid());
}

/**
 * Interrupt synchronous I/O on the thread. Limited support:
 * Win32 - supported.
 * Android/iOS/Linux - not supported.
 */
Bool Thread::CancelSynchronousIO()
{
	// Lock for checking.
	Lock lock(m_Mutex);

	// Early out if no thread.
	if (!m_hHandle.IsValid() || kRunning != m_eState)
	{
		return false;
	}

	// Interrupt.
	return ThreadDetail::CancelSynchronousIO(m_hHandle);
}

/**
 * Start this Thread with a default name - otherwise identical to Start(const String&).
 *
 * @return True if this Thread was succesfully started, false otherwise.
 */
Bool Thread::Start()
{
	return Start("Unnamed SeoulEngine Thread");
}

/**
 * Starts execution of the thread owned by this Thread. Has
 * no effect if the thread has already been started, either by previously
 * calling this function or by instantiating this Thread with
 * bStart = true.
 *
 * @return True if this Thread was successfully started, false otherwise.
 * If this method returns false, it can be because this Thread was
 * previously started or because there was an error and this Thread could
 * not be started. Check the return value of Thread::GetState() for more
 * information about why Start() returned false.
 *
 * \warning Start() itself is not thread-safe. The results of two
 * different threads calling Start() on this Thread are undefined.
 */
Bool Thread::Start(const String& sThreadName)
{
	// Only attempt to start if this Thread's handle is
	// not already valid and if this Thead's state is
	// kNotStarted.
	if (!m_hHandle.IsValid() && kNotStarted == m_eState)
	{
		// Need to set the state before attempting to start
		// or the thread could start and finish before the
		// the state is set, resulting in the state being stuck
		// as kRunning.
		m_sThreadName = sThreadName;
		m_eState = kRunning;

		if (!ThreadDetail::Start(
			sThreadName.CStr(),
			m_uStackSize,
			&_ThreadStart,
			this,
			m_hHandle))
		{
			m_eState = kError;
			return false;
		}
		else
		{
			// Wait for thread body to entre before returning success.
			m_StartupSignal.Wait();
			return true;
		}
	}

	return false;
}

/**
 * When called, this function blocks until this Thread's
 * IsRunning() method returns false.
 *
 * This method will return immediately if this Thread has
 * not yet been started or if this Thread has already completed
 * running. It will also return immediately if an attempt was made
 * to start this Thread but it failed.
 */
Int Thread::WaitUntilThreadIsNotRunning()
{
	// Lock for the body - will release the
	// lock temporarily around WaitForThread
	// if we get there.
	Lock lock(m_Mutex);

	// Early out if the thread is not in the
	// running state.
	if (!m_hHandle.IsValid() || kRunning != m_eState)
	{
		return m_iReturnValue;
	}

	// On some platforms, a wait operation may modify
	// the thread running state (typically from running
	// to done running).
	m_Mutex.Unlock();
	ThreadDetail::WaitForThread(m_eState, m_hHandle);
	m_Mutex.Lock();

	SEOUL_ASSERT(!IsRunning());
	return m_iReturnValue;
}

/**
 * Sets the execution priority of this Thread.
 */
void Thread::SetPriority(Priority ePriority)
{
	// Need to protect against shutdown in the middle of
	// ThreadDetail::SetThreadPriority.
	Lock lock(m_Mutex);

	if (IsRunning())
	{
		ThreadDetail::SetThreadPriority(m_hHandle, ePriority);
	}
}

/**
 * Puts this Thread to sleep for the specified number
 * of milliseconds.
 *
 * There is no guarantee that this Thread will stay asleep for
 * exactly uTimeInMilliseconds.
 */
void Thread::Sleep(UInt32 uTimeInMilliseconds)
{
	ThreadDetail::Sleep(uTimeInMilliseconds);
}

/**
 * Tells the OS that it can switch execution to a different thread
 * on the processor that this Thread is executing on.
 */
void Thread::YieldToAnotherThread()
{
	ThreadDetail::YieldToAnotherThread();
}

/**
 * Returns the number of hardware processors available on the
 * current platform.
 */
UInt32 Thread::GetProcessorCount()
{
	// Sanitize - always returns at least a count of 1.
	return Max(1u, ThreadDetail::GetProcessorCount());
}

/**
 * @return The thread name of the current thread - can be nullptr if no
 * name was set.
 */
Byte const* Thread::GetThisThreadName()
{
	return ThreadDetail::GetThreadName();
}

/**
 * Update the name of the current thread.
 */
void Thread::SetThisThreadName(Byte const* sName, UInt32 zNameLengthInBytes)
{
	ThreadDetail::SetThreadName(sName, zNameLengthInBytes);
}

/** Fixed thread ids. */
static FixedArray<ThreadId, (UInt32)FixedThreadId::COUNT> s_FixedThreadIds;

/**
 * @return The ThreadId of the corresonding eFixedThreadId.
 */
ThreadId GetFixedThreadId(FixedThreadId eFixedThreadId)
{
	// Sanity, since an unassigned value here can cause scheduling havoc.
	SEOUL_ASSERT(s_FixedThreadIds[(UInt32)eFixedThreadId].IsValid());

	return s_FixedThreadIds[(UInt32)eFixedThreadId];
}

/**
 * Update the thread that corresponds to eFixedThreadId - this should be called as early as possible in engine startup,
 * the results of a thread id changing after being used are undefined.
 */
void SetFixedThreadId(FixedThreadId eFixedThreadId, const ThreadId& id)
{
	s_FixedThreadIds[(UInt32)eFixedThreadId] = id;

	// If this was a main thread id set, also set any other thread ids that haven't already been set.
	if (FixedThreadId::kMain == eFixedThreadId)
	{
		for (Int i = 0; i < (Int)FixedThreadId::COUNT; ++i)
		{
			if (!s_FixedThreadIds[i].IsValid())
			{
				s_FixedThreadIds[i] = id;
			}
		}
	}
}

/**
 * @return True if the current thread is eFixedThreadId, false otherwise.
 */
Bool IsFixedThread(FixedThreadId eFixedThreadId)
{
	// To allow startup logic on the main thread, before the id is set, return true
	// for the main thread if it is the current thread -or- if the current
	// id matches the fixed id.
	if (FixedThreadId::kMain == eFixedThreadId)
	{
		return (!s_FixedThreadIds[(UInt32)eFixedThreadId].IsValid() || Thread::GetThisThreadId() == s_FixedThreadIds[(UInt32)eFixedThreadId]);
	}
	else
	{
		// All other threads must have their IDs set already.
		SEOUL_ASSERT(s_FixedThreadIds[(UInt32)eFixedThreadId].IsValid());
		return (Thread::GetThisThreadId() == s_FixedThreadIds[(UInt32)eFixedThreadId]);
	}
}

/**
 * Called on shutdown right before main returns on some
 * platforms (Android) to clear thread ID state.
 */
void ResetAllFixedThreadIds()
{
	for (UInt32 i = 0u; i < s_FixedThreadIds.GetSize(); ++i)
	{
		s_FixedThreadIds[i] = ThreadId();
	}
}

} // namespace Seoul
