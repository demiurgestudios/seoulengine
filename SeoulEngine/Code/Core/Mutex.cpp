/**
 * \file Mutex.cpp
 * \brief Mutex represents a thread-safe lock used for handling single
 * access to shared resources.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Mutex.h"
#include "Platform.h"

#include <string.h>

namespace Seoul
{

#if SEOUL_PLATFORM_WINDOWS

Mutex::Mutex()
	: m_pCritSec(nullptr)
{
	// Mutex is low-level and used by our allocator classes,
	// so we can't use those allocators for this block of data.
	m_pCritSec = (CRITICAL_SECTION*)::HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*m_pCritSec));
	::InitializeCriticalSection(m_pCritSec);
}

Mutex::~Mutex()
{
	DeleteCriticalSection(m_pCritSec);
	SEOUL_VERIFY_NO_LOG(FALSE != ::HeapFree(::GetProcessHeap(), 0u, m_pCritSec));
	m_pCritSec = nullptr;
}

void Mutex::Lock() const
{
	::EnterCriticalSection(m_pCritSec);
}

Bool Mutex::TryLock() const
{
	return (::TryEnterCriticalSection(m_pCritSec) != 0);
}

void Mutex::Unlock() const
{
	::LeaveCriticalSection(m_pCritSec);
}

#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX

Mutex::Mutex()
	: m_Mutex()
{
	// Clear the mutex memory.
	memset(&m_Mutex, 0, sizeof(m_Mutex));

	// Use the no log variant for errors because Logger constructs a mutex.

	// setup attributes that define the mutex.
	pthread_mutexattr_t mutex_attributes;
	SEOUL_VERIFY_NO_LOG(0 == pthread_mutexattr_init(&mutex_attributes));

	// allow recursive behavior to match the usage of critical sections
	// on Win32.
	SEOUL_VERIFY_NO_LOG(0 == pthread_mutexattr_settype(&mutex_attributes, PTHREAD_MUTEX_RECURSIVE));

	// Create the mutex that the signal will use.
	SEOUL_VERIFY_NO_LOG(0 == pthread_mutex_init(&m_Mutex, &mutex_attributes));

	// Destroy the attributes object.
	SEOUL_VERIFY_NO_LOG(0 == pthread_mutexattr_destroy(&mutex_attributes));
}

Mutex::~Mutex()
{
	Int iResult = pthread_mutex_destroy(&m_Mutex);

	while (IsEBusy(iResult))
	{
		iResult = pthread_mutex_destroy(&m_Mutex);
	}

	// If we ended up with any error code besides 0,
	// something bad happened. Use the no log variant
	// for the same reason as the usage in CreateMutex().
	SEOUL_ASSERT_NO_LOG(0 == iResult);

	memset(&m_Mutex, 0, sizeof(m_Mutex));
}

void Mutex::Lock() const
{
	// The 0u second argument indicates an infinite wait time.
	SEOUL_VERIFY_NO_LOG(0 == pthread_mutex_lock(&const_cast<pthread_mutex_t&>(m_Mutex)));
}

Bool Mutex::TryLock() const
{
	return (0 == pthread_mutex_trylock(&const_cast<pthread_mutex_t&>(m_Mutex)));
}

void Mutex::Unlock() const
{
	SEOUL_VERIFY_NO_LOG(0 == pthread_mutex_unlock(&const_cast<pthread_mutex_t&>(m_Mutex)));
}

#else

#error "Define to this platform."

#endif

Lock::Lock(const Mutex& mutex)
	: m_Mutex(mutex)
{
	m_Mutex.Lock();
}

Lock::~Lock()
{
	m_Mutex.Unlock();
}

TryLock::TryLock(const Mutex& mutex)
	: m_Mutex(mutex)
{
	m_bLocked = m_Mutex.TryLock();
}

TryLock::~TryLock()
{
	if (m_bLocked)
	{
		m_Mutex.Unlock();
	}
}

} // namespace Seoul
