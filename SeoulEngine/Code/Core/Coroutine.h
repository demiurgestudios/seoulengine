/**
 * \file Coroutine.h
 * \brief A Coroutine is a concept that allows for the implementation of
 * cooperative multi-tasking - switching context between 2 coroutines stores
 * the current stack and replaces it with a different stack.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef COROUTINE_H
#define COROUTINE_H

#include "Prereqs.h"
#include "UnsafeHandle.h"

namespace Seoul
{

/*
 * Coroutines are low-level and must be used much more carefully than Threads. In particular:
 *
 * - the entry point function used in CreateCoroutine() must never return.
 *   - as a consequence, this function should not contain any automatic local variables for which
 *     the destructor must be invoked.
 * - do not call SwitchToCoroutine() in a function with exception handling semantics - for example:
 *   void Call()
 *   {
 *       if (...)
 *           throw std::exception();
 *
 *       SwitchToCoroutine();
 *   }
 *
 *   this will not be handled correctly, as the exception handling context will not be unwound on the call
 *   to SwitchToCoroutine(). This is fine:
 *   void FunctionWithExceptions()
 *   {
 *       try
 *       {
 *           if (...)
 *               throw std::exception();
 *       }
 *       catch (...)
 *       {
 *       }
 *   }
 *   void Call()
 *   {
 *       FunctionWithExceptions();
 *       SwitchToCoroutine();
 *   }
 *
 *   It's also fine if the caller of Call() contains exception handling semantics.
 */

/**
 * Function that must be passed to CreateCoroutine to define
 * the Coroutine's entry point - pUserData will be equal to the pCoroutineData
 * passed to CreateCoroutine.
 *
 * \warning This function must never return.
 */
typedef void (SEOUL_STD_CALL *CoroutineEntryPoint)(void* pUserData);

// Convert the current thread to a coroutine thread - allocates resources
// related to the coroutine system and is necessary to use any other
// coroutines functions on the current thread.
UnsafeHandle ConvertThreadToCoroutine(void* pUserData);

// Convert the current coroutine thread back to a regular thread - it is
// necessary to call this function to cleanup resources used on the current
// thread for coroutines functionality before the thread shuts down or
// when using the coroutine system is being relinquished.
void ConvertCoroutineToThread();

// Generates a new coroutine with the desired stack size in bytes -
// when switched to, will invoke pEntryPoint. Reserved size is the
// total available memory. Commit size is the portion of reserved size
// that will be initially backed by physical memory. Commit size must
// be <= reserved size.
UnsafeHandle CreateCoroutine(
	size_t zStackCommitSize,
	size_t zStackReservedSize,
	CoroutineEntryPoint pEntryPoint, void* pUserData);

// Must be called on all coroutines created with CreateCoroutine.
void DeleteCoroutine(UnsafeHandle& rhCoroutine);

// Swap the current context to the context described by pCoroutine.
void SwitchToCoroutine(UnsafeHandle hCoroutine);

// Return the currently active coroutine on the current thread
UnsafeHandle GetCurrentCoroutine();

// Get the currently stored coroutine user data.
void* GetCoroutineUserData();

// Return true if the current thread is in the origin coroutine, false
// if it is in another coroutine.
Bool IsInOriginCoroutine();

// WARNING: Low-level function, use with care. View this as equivalent
// to a free/new() call of the Coroutine. Used to reclaim Coroutine
// stack memory beyond the given size. Nop on some platforms. Stack
// memory beyond zStackSizeToLeaveCommitted will have an undefined
// value after this call.
//
// Pre: hCoroutine *must not* be the current coroutine.
void PartialDecommitCoroutineStack(UnsafeHandle hCoroutine, size_t zStackSizeToLeaveCommitted);

} // namespace Seoul

#endif // include guard
