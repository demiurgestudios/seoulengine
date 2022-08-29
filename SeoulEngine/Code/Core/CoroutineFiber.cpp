/**
 * \file CoroutineFiber.cpp
 * \brief A Coroutine is a concept that allows for the implementation of
 * cooperative multi-tasking - switching context between 2 coroutines stores
 * the current stack and replaces it with a different stack.
 *
 * This file contains an implementation of the SeoulEngine Coroutine
 * API using Windows Fibers.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Coroutine.h"
#include "PerThreadStorage.h"
#include "Platform.h"
#include "SeoulMath.h"

namespace Seoul
{

// Fiber API is exclusive to Windows.
#if SEOUL_PLATFORM_WINDOWS

/** Global per-thread data used by the Coroutine system. */
static PerThreadStorage s_PerThreadCoroutineData;

UnsafeHandle ConvertThreadToCoroutine(void* pUserData /* = nullptr */)
{
	// Undocumented, hacky check that thread is not already a fiber - see
	// http://www.crystalclearsoftware.com/soc/coroutine/coroutine/fibers.html
	SEOUL_ASSERT(nullptr == ::GetCurrentFiber() || 0x1E00 == (size_t)::GetCurrentFiber());

	LPVOID pFiber = ::ConvertThreadToFiber(pUserData);
	SEOUL_ASSERT(nullptr != pFiber);

	// Cache the thread fiber in s_PerThreadCoroutineData.
	s_PerThreadCoroutineData.SetPerThreadStorage(pFiber);

	return UnsafeHandle(pFiber);
}

void ConvertCoroutineToThread()
{
	SEOUL_ASSERT(nullptr != ::GetCurrentFiber() && 0x1E00 != (size_t)::GetCurrentFiber());
	SEOUL_ASSERT(::GetCurrentFiber() == s_PerThreadCoroutineData.GetPerThreadStorage());

	// Clear s_PerThreadCoroutineData
	s_PerThreadCoroutineData.SetPerThreadStorage(nullptr);

	SEOUL_VERIFY(FALSE != ::ConvertFiberToThread());
}

UnsafeHandle CreateCoroutine(
	size_t zStackCommitSize,
	size_t zStackReservedSize,
	CoroutineEntryPoint pEntryPoint,
	void* pUserData)
{
	// Sanity check.
	SEOUL_ASSERT(zStackCommitSize <= zStackReservedSize);

	// TODO: Not sure why, but a small initial commit
	// size causes stack overflow crashes with some of our
	// PC development tools (e.g. RenderDoc and FRAPS). Possibly
	// due to what they're working with (GPU memory) and how
	// they're doing it (process injection), they may not react
	// to a low commit size. So we just set the commit size
	// to the reserved size on this platform, given it typically
	// has plenty of memory.
#if SEOUL_PLATFORM_WINDOWS
	zStackCommitSize = zStackReservedSize;
#endif // /#if SEOUL_PLATFORM_WINDOWS

	LPVOID pFiber = ::CreateFiberEx(
		zStackCommitSize,
		zStackReservedSize,
		0u,
		pEntryPoint,
		pUserData);

	// Check that, if fiber creation failed, it failed for an expected reason (out of memory).
	if (nullptr == pFiber)
	{
		DWORD uResult = ::GetLastError();
		SEOUL_ASSERT(ERROR_NOT_ENOUGH_MEMORY == uResult);
	}

	return UnsafeHandle(pFiber);
}

void DeleteCoroutine(UnsafeHandle& rhCoroutine)
{
	LPVOID pFiber = StaticCast<LPVOID>(rhCoroutine);
	rhCoroutine.Reset();

	if (nullptr != pFiber)
	{
		::DeleteFiber(pFiber);
	}
}

void SwitchToCoroutine(UnsafeHandle hCoroutine)
{
	::SwitchToFiber(StaticCast<LPVOID>(hCoroutine));
}

UnsafeHandle GetCurrentCoroutine()
{
	return ::GetCurrentFiber();
}

void* GetCoroutineUserData()
{
	return GetFiberData();
}

Bool IsInOriginCoroutine()
{
	return (
		s_PerThreadCoroutineData.GetPerThreadStorage() == ::GetCurrentFiber() ||
		0x1E00 == (size_t)::GetCurrentFiber());
}

void PartialDecommitCoroutineStack(
	UnsafeHandle hCoroutine,
	size_t zStackSizeToLeaveCommitted)
{
	// Nop
}

#endif // /SEOUL_PLATFORM_WINDOWS

} // namespace Seoul
