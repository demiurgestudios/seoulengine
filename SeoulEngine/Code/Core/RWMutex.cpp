/**
 * \file RWMutex.cpp
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

#include "RWMutex.h"

namespace Seoul
{

ReadLock::ReadLock(RWMutex& mutex)
	: m_Mutex(mutex)
{
	m_Mutex.ReadLock();
}

ReadLock::~ReadLock()
{
	m_Mutex.ReadUnlock();
}

TryReadLock::TryReadLock(RWMutex& mutex)
	: m_Mutex(mutex)
{
	m_bLocked = m_Mutex.TryReadLock();
}

TryReadLock::~TryReadLock()
{
	if (m_bLocked)
	{
		m_Mutex.ReadUnlock();
	}
}

WriteLock::WriteLock(RWMutex& mutex)
	: m_Mutex(mutex)
{
	m_Mutex.WriteLock();
}

WriteLock::~WriteLock()
{
	m_Mutex.WriteUnlock();
}

TryWriteLock::TryWriteLock(RWMutex& mutex)
	: m_Mutex(mutex)
{
	m_bLocked = m_Mutex.TryWriteLock();
}

TryWriteLock::~TryWriteLock()
{
	if (m_bLocked)
	{
		m_Mutex.WriteUnlock();
	}
}

} // namespace Seoul
