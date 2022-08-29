/**
 * \file PreThreadStorage.cpp
 * \brief Allows a void* size block of data to be stored such
 * that the value is unique per-thread.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Atomic32.h"
#include "PerThreadStorage.h"
#include "Platform.h"

namespace Seoul
{

#if SEOUL_PLATFORM_WINDOWS
PerThreadStorage::PerThreadStorage()
	: m_ThreadLocalStorageIndex(TlsAlloc())
{
	// Verify we didn't run out of slots.
	SEOUL_ASSERT(TLS_OUT_OF_INDEXES != m_ThreadLocalStorageIndex);
}

PerThreadStorage::~PerThreadStorage()
{
	// If the storage index is valid, this method will always succeed.
	SEOUL_VERIFY(FALSE != TlsFree(m_ThreadLocalStorageIndex));
}

void* PerThreadStorage::GetPerThreadStorage() const
{
	// Verify we didn't run out of slots.
	SEOUL_ASSERT(TLS_OUT_OF_INDEXES != m_ThreadLocalStorageIndex);

	return TlsGetValue(m_ThreadLocalStorageIndex);
}

void PerThreadStorage::SetPerThreadStorage(void* pData)
{
	// Verify we didn't run out of slots.
	SEOUL_ASSERT(TLS_OUT_OF_INDEXES != m_ThreadLocalStorageIndex);

	// If the storage index is valid, this method will always succeed.
	SEOUL_VERIFY(FALSE != TlsSetValue(m_ThreadLocalStorageIndex, pData));
}
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX // /SEOUL_PLATFORM_WINDOWS
namespace Tls
{

inline PerThreadStorageIndexType Alloc()
{
	pthread_key_t ret;
	memset(&ret, 0, sizeof(ret));

	SEOUL_VERIFY(0 == pthread_key_create(&ret, nullptr));

	return ret;
}

inline void Free(PerThreadStorageIndexType& riKey)
{
	PerThreadStorageIndexType key = riKey;
	riKey = 0;

	SEOUL_VERIFY(0 == pthread_key_delete(key));
}

inline void* Get(PerThreadStorageIndexType iKey)
{
	return pthread_getspecific(iKey);
}

inline void Set(PerThreadStorageIndexType iKey, void* pData)
{
	SEOUL_VERIFY(0 == pthread_setspecific(iKey, pData));
}

} // namespace Tls

PerThreadStorage::PerThreadStorage()
	: m_ThreadLocalStorageIndex(Tls::Alloc())
{
}

PerThreadStorage::~PerThreadStorage()
{
	Tls::Free(m_ThreadLocalStorageIndex);
}

void* PerThreadStorage::GetPerThreadStorage() const
{
	return Tls::Get(m_ThreadLocalStorageIndex);
}

void PerThreadStorage::SetPerThreadStorage(void* pData)
{
	Tls::Set(m_ThreadLocalStorageIndex, pData);
}
#else // /SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
#error "Define for this platform."
#endif

} // namespace Seoul
