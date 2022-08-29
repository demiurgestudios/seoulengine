/**
 * \file MemoryBarrier.h
 * \brief SeoulEngine memory barrier primitive. Used to preserve ordering
 * in code that needs to be thread safe.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef MEMORY_BARRIER_H
#define MEMORY_BARRIER_H

namespace Seoul
{

/**
 * Performs a memory barrier to prevent CPU reordering of loads and stores
 * across the barrier.  This does not perform a full memory barrier on all
 * platforms.
 */
inline void SeoulMemoryBarrier()
{
#if defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__x86_64__)
	// No-op on x86/x86-64
#elif SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX || SEOUL_PLATFORM_IOS
	__atomic_thread_fence(__ATOMIC_SEQ_CST);
#else
#error "Define for this platform"
#endif
}

} // namespace Seoul

#endif // include guard
