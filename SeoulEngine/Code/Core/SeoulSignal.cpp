/**
 * \file SeoulSignal.cpp
 * \brief Signal represents an object that can be used to trigger events
 * across threads.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Atomic32.h"
#include "SeoulSignal.h"
#include "Platform.h"
#include "UnsafeHandle.h"

#include <string.h>

namespace Seoul
{

#if SEOUL_PLATFORM_WINDOWS
Signal::Signal()
	: m_hHandle(CreateEvent(
		// See also: http://msdn.microsoft.com/en-us/library/ms682396%28VS.85%29.aspx
		nullptr,	// default security attributes
		FALSE,	// automatic reset - signal will be "unsignaled" when a waiting thread stops waiting.
		FALSE,	// initially unsignaled.
		nullptr))	// no name.
{}

Signal::~Signal()
{
	CloseHandle(StaticCast<HANDLE>(m_hHandle));
}

/**
 * Causes any threads that are Wait()ing on this Signal to
 * wake-up and unblock.
 *
 * @return True if this Signal was successfully activated, false
 * otherwise.
 *
 * One call to Activate() will be queued for a later call to Wait().
 * As a result, it is safe for one thread to Wait() on a signal that another
 * thread Activate()s, even if it is possible for the activating thread to
 * call Activate() before the waiting thread calls Wait(). However, this
 * is not true for multiple calls to Activate(). For example, two calls
 * to Activate() will only result in one Waiting()ing thread to be unblocked,
 * if they occur before any threads have called Wait().
 */
Bool Signal::Activate() const
{
	return (FALSE != SetEvent(StaticCast<HANDLE>(m_hHandle)));
}

/**
 * Causes the calling thread to block indefinitely
 * until Activate() is called.
 *
 * One call to Activate() will be queued for a later call to Wait().
 * As a result, it is safe for one thread to Wait() on a signal that another
 * thread Activate()s, even if it is possible for the activating thread to
 * call Activate() before the waiting thread calls Wait(). However, this
 * is not true for multiple calls to Activate(). For example, two calls
 * to Activate() will only result in one Waiting()ing thread to be unblocked,
 * if they occur before any threads have called Wait().
 */
void Signal::Wait() const
{
	DWORD uResult = WaitForSingleObject(
		StaticCast<HANDLE>(m_hHandle),
		INFINITE);

	SEOUL_ASSERT_NO_LOG(WAIT_OBJECT_0 == uResult);
	(void)uResult;
}

/**
 * Causes the calling thread to block until this Signal
 * is activated.
 *
 * @param[in] uTimeInMilliseconds The time to wait for a signal.
 *
 * @return True if this Signal was activated, false if the specified
 * time elapsed without this Signal being activated.
 *
 * One call to Activate() will be queued for a later call to Wait().
 * As a result, it is safe for one thread to Wait() on a signal that another
 * thread Activate()s, even if it is possible for the activating thread to
 * call Activate() before the waiting thread calls Wait(). However, this
 * is not true for multiple calls to Activate(). For example, two calls
 * to Activate() will only result in one Waiting()ing thread to be unblocked,
 * if they occur before any threads have called Wait().
 */
Bool Signal::Wait(UInt32 uTimeInMilliseconds) const
{
	DWORD uResult = WaitForSingleObject(
		StaticCast<HANDLE>(m_hHandle),
		uTimeInMilliseconds);

	SEOUL_ASSERT_NO_LOG(WAIT_OBJECT_0 == uResult || WAIT_TIMEOUT == uResult);

	return (WAIT_OBJECT_0 == uResult);
}
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
/**
 * Data necessary for implementing Signal on Native Client.
 */
struct SignalImpl
{
	// Conditional variable, roughly equivalent to the windows Event without
	// the lock.
	pthread_cond_t m_ConditionID;

	// Mutex lock, required for safe issuing of conditions in Unix.
	pthread_mutex_t m_Mutex;

	// Used to implement the expected Win32 behavior - cache signals
	// until a thread waits for the signal.
	Atomic32Value<Bool> m_bSignaled;

	// Used to implement the expected Win32 behavior - set a waiting
	// flag so a triggered signal knows whether to set m_bSignaled,
	// or trigger the signal condition.
	Atomic32Value<Bool> m_bWaiting;
}; // struct SignalImpl

/**
 * Create the data necessary for implementing signals.
 */
inline UnsafeHandle CreateEvent()
{
	// Instantiate the SignalImpl structure and initialize to 0.
	SignalImpl* pImpl = SEOUL_NEW(MemoryBudgets::Threading) SignalImpl;
	memset(pImpl, 0, sizeof(*pImpl));

	// Initially unsignaled.
	pImpl->m_bSignaled = false;
	// Initially nothing is waiting.
	pImpl->m_bWaiting = false;

	// setup attributes that define the mutex.
	pthread_mutexattr_t mutex_attributes;
	SEOUL_VERIFY_NO_LOG(0 == pthread_mutexattr_init(&mutex_attributes));

	// default behavior (no error checking, no recursive locking).
	SEOUL_VERIFY_NO_LOG(0 == pthread_mutexattr_settype(&mutex_attributes, PTHREAD_MUTEX_DEFAULT));

	// Create the mutex that the signal will use.
	SEOUL_VERIFY_NO_LOG(0 == pthread_mutex_init(&(pImpl->m_Mutex), &mutex_attributes));

	// Destroy the attributes object.
	SEOUL_VERIFY_NO_LOG(0 == pthread_mutexattr_destroy(&mutex_attributes));

	// Create the condition that the signal will use.
	SEOUL_VERIFY_NO_LOG(0 == pthread_cond_init(&(pImpl->m_ConditionID), nullptr));

	return UnsafeHandle(pImpl);
}

/**
 * Destroy the data created by a call to CreateEvent().
 */
inline void DestroyEvent(UnsafeHandle& rhHandle)
{
	SignalImpl* pImpl = StaticCast<SignalImpl*>(rhHandle);
	SEOUL_ASSERT_NO_LOG(pImpl);

	rhHandle.Reset();

	// Destroy the condition variable
	{
		Int iResult = pthread_cond_destroy(&pImpl->m_ConditionID);

		while (IsEBusy(iResult))
		{
			iResult = pthread_cond_destroy(&pImpl->m_ConditionID);
		}

		// If we ended up with any error code besides 0,
		// something bad happened.
		SEOUL_ASSERT_NO_LOG(0 == iResult);
	}

	// Destroy the mutex
	{
		Int iResult = pthread_mutex_destroy(&pImpl->m_Mutex);

		while (IsEBusy(iResult))
		{
			iResult = pthread_mutex_destroy(&pImpl->m_Mutex);
		}

		// If we ended up with any error code besides 0,
		// something bad happened.
		SEOUL_ASSERT_NO_LOG(0 == iResult);
	}

	// Deallocate the data object
	SafeDelete(pImpl);
}

Signal::Signal()
	: m_hHandle(CreateEvent())
{}

Signal::~Signal()
{
	DestroyEvent(m_hHandle);
}

Bool Signal::Activate() const
{
	SignalImpl* pImpl = StaticCast<SignalImpl*>(m_hHandle);
	SEOUL_ASSERT_NO_LOG(pImpl);

	// The lock must always succeed or we have a usage error -
	// either there is no waiting thread, or the thread has already
	// hit sys_cond_wait(), in which case the waiting thread's lock on the
	// mutex is released until that call returns.
	SEOUL_VERIFY_NO_LOG(0 == pthread_mutex_lock(&pImpl->m_Mutex));

	// If there is no waiting thread, set the flag
	// indicating that a signal has been triggered and
	// return success.
	if (!pImpl->m_bWaiting)
	{
		pImpl->m_bSignaled = true;
		SEOUL_VERIFY_NO_LOG(0 == pthread_mutex_unlock(&pImpl->m_Mutex));
		return true;
	}
	// Otherwise a thread is waiting, so we signal it immediately.
	else
	{
		pImpl->m_bSignaled = true;
		Bool const bReturn = (0 == pthread_cond_signal(&StaticCast<SignalImpl*>(m_hHandle)->m_ConditionID));
		SEOUL_VERIFY_NO_LOG(0 == pthread_mutex_unlock(&pImpl->m_Mutex));
		return bReturn;
	}
}

void Signal::Wait() const
{
	SignalImpl* pImpl = StaticCast<SignalImpl*>(m_hHandle);
	SEOUL_ASSERT_NO_LOG(pImpl);

	// The lock must always succeed or we have a usage error.
	SEOUL_VERIFY_NO_LOG(0 == pthread_mutex_lock(&pImpl->m_Mutex));

	// If a signal was queued up, do it immediately.
	if (pImpl->m_bSignaled)
	{
		// Unset the signaled state.
		pImpl->m_bSignaled = false;

		// Just to be thorough
		pImpl->m_bWaiting = false;

		// Unlock the mutex before returning.
		SEOUL_VERIFY_NO_LOG(0 == pthread_mutex_unlock(&pImpl->m_Mutex));
	}
	else
	{
		// Indicate that a thread is waiting.
		pImpl->m_bWaiting = true;

		// Do the wait on the signal.
		Int iResult = -1;

		// http://pubs.opengroup.org/onlinepubs/000095399/functions/pthread_cond_timedwait.html - this function
		// can return spuriously before the pthread_cond_signal() is called. The only valid return from this
		// function is 0, since it doesn't have a timeout, so we wait until both m_bSignaled are true
		//
		do
		{
			iResult = pthread_cond_wait(&StaticCast<SignalImpl*>(m_hHandle)->m_ConditionID, &StaticCast<SignalImpl*>(m_hHandle)->m_Mutex);
		} while (0 == iResult && !pImpl->m_bSignaled);

		// Unset the waiting and signaled flags.
		pImpl->m_bSignaled = false;
		pImpl->m_bWaiting = false;

		// 0 means we were successfully signaled.
		// Any other error code indicates a usage error.
		SEOUL_ASSERT_NO_LOG(0 == iResult);

		// Unlock the mutex before returning.
		SEOUL_VERIFY_NO_LOG(0 == pthread_mutex_unlock(&pImpl->m_Mutex));
	}
}

Bool Signal::Wait(UInt32 uTimeInMilliseconds) const
{
	SignalImpl* pImpl = StaticCast<SignalImpl*>(m_hHandle);
	SEOUL_ASSERT_NO_LOG(pImpl);

	// The lock must always succeed or we have a usage error.
	SEOUL_VERIFY_NO_LOG(0 == pthread_mutex_lock(&pImpl->m_Mutex));

	// If a signal was queued up, do it immediately.
	if (pImpl->m_bSignaled)
	{
		// Unset the signaled state.
		pImpl->m_bSignaled = false;

		// Just to be thorough
		pImpl->m_bWaiting = false;

		// Unlock the mutex before returning.
		SEOUL_VERIFY_NO_LOG(0 == pthread_mutex_unlock(&pImpl->m_Mutex));

		return true;
	}
	else
	{
		// Indicate that a thread is waiting.
		pImpl->m_bWaiting = true;

		// Do the wait on the signal.
		Int iResult = -1;

		// Special case - wait time of 0 means
		// "infinite wait" to pthread_cond_timedwait, but
		// we want it to mean "no wait" on all
		// platforms, so we just set result to ETIMEDOUT
		// immediately here.
		if (0u == uTimeInMilliseconds)
		{
			iResult = ETIMEDOUT;
		}
		// Otherwise, perform the actual timed wait operation.
		else
		{
			struct timespec timeSpec;
			MillisecondsToTimespec(uTimeInMilliseconds, timeSpec);

			// pthread_cond_timewait takes	 absolute time, not relative time.
			{
				struct timespec currentTimeSpec;
				memset(&currentTimeSpec, 0, sizeof(currentTimeSpec));

#if SEOUL_PLATFORM_IOS
				struct timeval timeOfDay;
				memset(&timeOfDay, 0, sizeof(timeOfDay));
				SEOUL_VERIFY_NO_LOG(0 == gettimeofday(&timeOfDay, nullptr));

				currentTimeSpec.tv_sec = timeOfDay.tv_sec;
				currentTimeSpec.tv_nsec = timeOfDay.tv_usec * 1000;
#else
				SEOUL_VERIFY_NO_LOG(0 == clock_gettime(CLOCK_REALTIME, &currentTimeSpec));
#endif

				AddTimespec(timeSpec, currentTimeSpec, timeSpec);
			}

			// http://pubs.opengroup.org/onlinepubs/000095399/functions/pthread_cond_timedwait.html - this function
			// can return spuriously before the pthread_cond_signal() is called.
			do
			{
				iResult = pthread_cond_timedwait(&StaticCast<SignalImpl*>(m_hHandle)->m_ConditionID, &StaticCast<SignalImpl*>(m_hHandle)->m_Mutex, &timeSpec);
			} while (0 == iResult && !pImpl->m_bSignaled);
		}

		// Unset the waiting and signaled flags.
		pImpl->m_bSignaled = false;
		pImpl->m_bWaiting = false;

		// 0 means we were successfully signaled, ETIMEDOUT means
		// the elapsed timeout was reached. Any other error code indicates
		// a usage error.
		SEOUL_ASSERT_NO_LOG(0 == iResult || IsETimedOut(iResult));

		// Unlock the mutex before returning.
		SEOUL_VERIFY_NO_LOG(0 == pthread_mutex_unlock(&pImpl->m_Mutex));

		// Return true if we were successfully signaled, false otherwise.
		return (0 == iResult);
	}
}
#else
#error "Define for this platform."
#endif

} // namespace Seoul
