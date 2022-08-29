/**
 * \file Mutex.h
 * \brief Mutex represents a thread-safe lock used for handling single
 * access to shared resources.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef MUTEX_H
#define MUTEX_H

#include "Atomic32.h"
#include "Prereqs.h"

// Forward declare CRITICAL_SECTION to avoid polluting the
// codebase with the Windows.h header.
#if SEOUL_PLATFORM_WINDOWS
struct _RTL_CRITICAL_SECTION;
typedef _RTL_CRITICAL_SECTION CRITICAL_SECTION;
#else // !#if SEOUL_PLATFORM_WINDOWS
#include "Platform.h"
#endif // /!#if SEOUL_PLATFORM_WINDOWS

namespace Seoul
{

// Forward declarations
class Lock;
class TryLock;

class Mutex SEOUL_SEALED
{
public:
	Mutex();
	~Mutex();

	/**
	 * Gives the current thread an exclusive lock on this Mutex.
	 * Blocks until successful, be careful of deadlocks. Highly recommended to
	 * use the Lock and TryLock classes instead of calling this function
	 * directly.
	 */
	void Lock() const;

	/**
	 * Attempts to give the current thread an exclusive lock on this
	 * Mutex.  Does not block and will return prematurely if this Mutex is
	 * already  locked by another thread. Highly recommended to use the Lock
	 * and TryLock classes instead of calling this function directly.
	 *
	 * @return
	 *  true if this Mutex was successfully locked.
	 *  false otherwise.
	 */
	Bool TryLock() const;

	/**
	 * Release the lock on this Mutex by the current thread. Does
	 * nothing if this Mutex was not previously locked. Highly recommended
	 * to  use the Lock and TryLock classes instead of calling this function
	 * directly.
	 */
	void Unlock() const;

private:
#if SEOUL_PLATFORM_WINDOWS
	CRITICAL_SECTION mutable* m_pCritSec;
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
	pthread_mutex_t m_Mutex;
#else
#	error "Define for this platform."
#endif

	friend class Lock;
	friend class TryLock;

	SEOUL_DISABLE_COPY(Mutex);
}; // class Mutex

/**
 * This class provides scoped locking of a Mutex. It locks the Mutex
 * on construction and unlocks the Mutex on destruction. It will block until
 * the Mutex is successfully locked, be wary of deadlocks.
 */
class Lock SEOUL_SEALED
{
public:
	explicit Lock(const Mutex& mutex);
	~Lock();

private:
	SEOUL_DISABLE_COPY(Lock);

	// Only store a reference to the mutex - lock objects are
	// expected to be stack allocated and temporary in nature
	const Mutex& m_Mutex;
}; // class Lock

/**
 * This class provides scoped locking of a Mutex. It attempts to lock
 * the Mutex on construction and unlocks the Mutex on destruction. The function
 * IsLocked() returns true if the Mutex was succesfully locked and false
 * otherwise.
 */
class TryLock SEOUL_SEALED
{
public:
	explicit TryLock(const Mutex& mutex);
	~TryLock();

	Bool IsLocked() const
	{
		return m_bLocked;
	}

private:
	SEOUL_DISABLE_COPY(TryLock);

	// Only store a reference to the mutex - lock objects are
	// expected to be stack allocated and temporary in nature
	const Mutex& m_Mutex;

	Atomic32Value<Bool> m_bLocked;
}; // class TryLock

} // namespace Seoul

#endif // include guard
