/**
 * \file RWMutex.h
 * \brief RWMutex is like a Mutex, except it supports two classes of lockers -
 * a read lock and a write lock. Any number of simultaneous threads can hold
 * a read lock, while only one thread can hold the write lock (and all readers
 * are prevented from holding the lock while the writer is holding the lock).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef RW_MUTEX_H
#define RW_MUTEX_H

#include "Atomic32.h"
#include "Mutex.h"
#include "SeoulSignal.h"

namespace Seoul
{

class RWMutex SEOUL_SEALED
{
public:
	RWMutex() = default;
	~RWMutex() = default;

	/**
	 * Grants a reader access to shared resource. Returns once
	 * all writers have released write locks (multiple readers share
	 * an exclusive lock on the resource while one and only one writer
	 * can hold that exclusive lock).
	 */
	void ReadLock()
	{
		// Hold the underlying mutex for the body
		// of the ReadLock() - this prevents readers
		// from acquiring the lock while a writer
		// holds it.
		Seoul::Lock lock(m_Mutex);
		++m_ReaderCount;
	}

	/**
	 * Call in exact match with calls to ReadLock(). Indicates a single reader
	 * has released its exclusive access to the shared resource.
	 */
	void ReadUnlock()
	{
		// Explicitly, don't lock the mutex here. This implements
		// two behaviors:
		// - readers that have already acquired the lock "drain"
		//   until they all release to allow a writer to have exclusive
		//   access.
		// - no new readers can acquire the lock until a writer has
		//   released its exclusive access to the shared resource
		//   (by usage of locking the mutex in ReadLock().
		--m_ReaderCount;
		m_Signal.Activate();
	}

	/**
	 * Like ReadLock(), but returns false if the lock cannot be acquired
	 * without contention (note - this is *not* guaranteed to succeed if
	 * no writer is active. It can also fail if 2 readers attempt to lock
	 * their shared resource simultaneously).
	 */
	Bool TryReadLock()
	{
		Seoul::TryLock tryLock(m_Mutex);
		if (!tryLock.IsLocked()) { return false; }

		++m_ReaderCount;
		return true;
	}

	/**
	 * Like WriteLock(), but returns false if the lock cannot be acquired
	 * without contention.
	 */
	Bool TryWriteLock()
	{
		if (!m_Mutex.TryLock()) { return false; }

		// TODO: Damned if you do, damned if you don't -
		// waiting for readers to drain is content. However,
		// not waiting for drainage (if instead we returned false
		// here immediately) will almost certainly result in high
		// "thrashing" around TryWriteLock() in real usage scenarios,
		// since if the single writer is interleaving with many readers,
		// it is very unlikely the writer will successfully lock at the
		// perform moment when no reader shares the lock).
		//
		// We must let readers drain before we return the
		// lock as acquired.
		while (m_ReaderCount != 0)
		{
			m_Signal.Wait();
		}

		// Done.
		return true;
	}

	/**
	 * Lock the one and only one exclusive writer lock to the resource.
	 *
	 * All readers drain before this call returns true, at which point
	 * the writer has the single lock around the shared resource.
	 */
	void WriteLock()
	{
		m_Mutex.Lock();

		// Wait for readers to drain.
		while (m_ReaderCount != 0)
		{
			m_Signal.Wait();
		}
	}

	/**
	 * Release the writer's exclusive access to the shared resource. Must
	 * be called in sync with WriteLock().
	 */
	void WriteUnlock()
	{
		m_Mutex.Unlock();
	}

private:
	SEOUL_DISABLE_COPY(RWMutex);

	Mutex m_Mutex;
	Atomic32 m_ReaderCount;
	Signal m_Signal;
}; // class RWMutex

/**
 * This class provides scoped read locking of a RWMutex. It locks the RWMutex
 * on construction and unlocks the RWMutex on destruction. It will block until
 * the RWMutex is successfully locked, be wary of deadlocks.
 */
class ReadLock SEOUL_SEALED
{
public:
	explicit ReadLock(RWMutex& mutex);
	~ReadLock();

private:
	SEOUL_DISABLE_COPY(ReadLock);

	// Only store a reference to the mutex - lock objects are
	// expected to be stack allocated and temporary in nature
	RWMutex& m_Mutex;
}; // class ReadLock

/**
 * This class provides scoped read locking of a RWMutex. It attempts to lock
 * the RWMutex on construction and unlocks the RWMutex on destruction. The function
 * IsLocked() returns true if the RWMutex was succesfully locked and false
 * otherwise.
 */
class TryReadLock SEOUL_SEALED
{
public:
	explicit TryReadLock(RWMutex& mutex);
	~TryReadLock();

	Bool IsLocked() const
	{
		return m_bLocked;
	}

private:
	SEOUL_DISABLE_COPY(TryReadLock);

	// Only store a reference to the mutex - lock objects are
	// expected to be stack allocated and temporary in nature
	RWMutex& m_Mutex;

	Atomic32Value<Bool> m_bLocked;
}; // class TryReadLock

/**
 * This class provides scoped write locking of a RWMutex. It locks the RWMutex
 * on construction and unlocks the RWMutex on destruction. It will block until
 * the RWMutex is successfully locked, be wary of deadlocks.
 */
class WriteLock SEOUL_SEALED
{
public:
	explicit WriteLock(RWMutex& mutex);
	~WriteLock();

private:
	SEOUL_DISABLE_COPY(WriteLock);

	// Only store a reference to the mutex - lock objects are
	// expected to be stack allocated and temporary in nature
	RWMutex& m_Mutex;
}; // class WriteLock

/**
 * This class provides scoped write locking of a RWMutex. It attempts to lock
 * the RWMutex on construction and unlocks the RWMutex on destruction. The function
 * IsLocked() returns true if the RWMutex was succesfully locked and false
 * otherwise.
 */
class TryWriteLock SEOUL_SEALED
{
public:
	explicit TryWriteLock(RWMutex& mutex);
	~TryWriteLock();

	Bool IsLocked() const
	{
		return m_bLocked;
	}

private:
	SEOUL_DISABLE_COPY(TryWriteLock);

	// Only store a reference to the mutex - lock objects are
	// expected to be stack allocated and temporary in nature
	RWMutex& m_Mutex;

	Atomic32Value<Bool> m_bLocked;
}; // class TryWriteLock

} // namespace Seoul

#endif // include guard
