/**
 * \file SecureRandomInternal.h
 * \brief Internal implementation for SecureRandom.cpp.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#ifndef SECURE_RANDOM_H
#error "Internal header file, must only be included by SecureRandom.cpp"
#endif

#include "SeoulMath.h"

#if SEOUL_PLATFORM_WINDOWS
#	include "Platform.h"
#	include <WinCrypt.h>
#else
#	include <fcntl.h>
#	include <unistd.h>
#endif

namespace Seoul::SecureRandomDetail
{

#if SEOUL_PLATFORM_WINDOWS

// Global utility used to initialize and destroy our global security context.
namespace
{

struct SecureRandomContext SEOUL_SEALED
{
	SecureRandomContext()
		: m_hContext(0u)
	{
		SEOUL_VERIFY(FALSE != ::CryptAcquireContext(
			&m_hContext,
			nullptr,
			nullptr,
			PROV_RSA_FULL,
			CRYPT_VERIFYCONTEXT));
	}

	~SecureRandomContext()
	{
		auto const h = m_hContext;
		m_hContext = 0u;
		SEOUL_VERIFY(FALSE != ::CryptReleaseContext(h, 0u));
	}

	void GetBytes(UInt8* p, UInt32 uSizeInBytes)
	{
		SEOUL_VERIFY(FALSE != ::CryptGenRandom(m_hContext, (DWORD)uSizeInBytes, (BYTE*)p));
	}

private:
	HCRYPTPROV m_hContext;

	SEOUL_DISABLE_COPY(SecureRandomContext);
}; // class SecureRandomContext
static SecureRandomContext s_SecureRandomContext;

} // namespace anonymous

#else // !#if SEOUL_PLATFORM_WINDOWS

// Global utility used to initialize and destroy our global random context.
namespace
{

struct SecureRandomContext SEOUL_SEALED
{
	SecureRandomContext()
		: m_iURandom(open("/dev/urandom", O_RDONLY))
	{
		SEOUL_ASSERT(-1 != m_iURandom);
	}

	~SecureRandomContext()
	{
		auto const i = m_iURandom;
		m_iURandom = -1;
		SEOUL_VERIFY(0 == close(i));
	}

	void GetBytes(UInt8* p, UInt32 uSizeInBytes)
	{
		while (uSizeInBytes > 0u)
		{
			auto const iRead = read(m_iURandom, (void*)p, uSizeInBytes);
			if (iRead <= 0)
			{
				// Yell in non-ship.
				SEOUL_FAIL("Failed reading /dev/urandom.");
				continue;
			}

			UInt32 const uRead = Min((UInt32)iRead, uSizeInBytes);
			p += uRead;
			uSizeInBytes -= uRead;
		}
	}

private:
	Int m_iURandom;

	SEOUL_DISABLE_COPY(SecureRandomContext);
}; // class SecureRandomContext
static SecureRandomContext s_SecureRandomContext;

} // namespace anonymous

#endif // /!#if SEOUL_PLATFORM_WINDOWS

static inline void GetBytes(UInt8* p, UInt32 uSizeInBytes)
{
	s_SecureRandomContext.GetBytes(p, uSizeInBytes);
}

} // namespace Seoul::SecureRandomDetail
