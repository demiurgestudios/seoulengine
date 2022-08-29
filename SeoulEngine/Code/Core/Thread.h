/**
 * \file Thread.h
 * \brief Class that represents a thread in a multi-threaded environment.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef THREAD_H
#define THREAD_H

#include "Delegate.h"
#include "Mutex.h"
#include "Prereqs.h"
#include "SeoulSignal.h"
#include "SeoulString.h"
#include "ThreadId.h"
#include "UnsafeHandle.h"

#if SEOUL_PLATFORM_ANDROID
// Forward declarations
struct _JNIEnv;
struct _JavaVM;
typedef _JavaVM JavaVM;
typedef _JNIEnv JNIEnv;
#endif // /#if SEOUL_PLATFORM_ANDROID

namespace Seoul
{

/**
 * Represents a hardware thread, which can be used for
 * concurrent programming.
 */
class Thread SEOUL_SEALED
{
public:
	// Returns a ThreadId object that uniquely identifies the current thread
	// amongst other threads.
	static ThreadId GetThisThreadId();

	// Return true if the thread identified by ID is considered alive,
	// false otherwise.
	//
	// WARNING: Can be expensive on some platforms. Do not use in tight
	// inner loops - evaluate in a profiler if you're concerned.
	static Bool IsThreadStillAlive(ThreadId id);

	/**
	 * States of execution of this Thread.
	 */
	enum State
	{
		/** this Thread is not running and has not been started. */
		kNotStarted,

		/** this Thread is running. */
		kRunning,

		/**
		 * this Thread was started and completed successfully.
		 *
		 * Thread is not guaranteed to enter this state until a
		 * call to WaitUntilThreadIsNotRunning(). Call that method
		 * before checking the Thread return value.
		 */
		kDoneRunning,

		/**
		 * An attempt was made to start this Thread but it failed.
		 * This can happen due to out-of-memory or other platform specific
		 * reasons.
		 */
		kError
	};

	/**
	 * Thread priority flags, used to control how much CPU time
	 * the OS gives to a thread.
	 *
	 * \warning Priorities are typically recommendations. There is no
	 * guarantee that a thread will get more or less execution time
	 * with a change in priority.
	 */
	enum Priority
	{
		kLow = 0,
		kMed = 1,
		kHigh = 2,
		kCritical = 3
	};

	// Default committed memory for a thread's stack. This
	// is the portion of the reserved memory that is initially backed
	// by physical memory pages. This value must be <= the reserved
	// size.
	static const size_t kDefaultStackCommittedSize = (1 << 13); // 8192

#if SEOUL_PLATFORM_64
	// Default total size of the reserved memory for a thread's stack.
	// Reserved memory is not necessarily backed by physical memory
	// pages until it is committed.
	static const size_t kDefaultStackReservedSize = (1 << 19); // 524,288
#else
	// Default total size of the reserved memory for a thread's stack.
	// Reserved memory is not necessarily backed by physical memory
	// pages until it is committed.
	static const size_t kDefaultStackReservedSize = (1 << 17); // 131,072
#endif

	typedef Delegate<Int(const Thread&)> ThreadFunc;

	// Android specific functionality for setting up native<->Java
	// associations
#if SEOUL_PLATFORM_ANDROID
	// Initialize Java association - must be called from the main thread.
	// Should be called as early as possible.
	static JNIEnv* GetThisThreadJNIEnv();
	static void InitializeJavaNativeThreading(JavaVM* pJavaVM);
	static void ShutdownJavaNativeThreading();
#endif // /#if SEOUL_PLATFORM_ANDROID

	Thread(ThreadFunc func, Bool bStart = false);
	Thread(ThreadFunc func, UInt uStackSize, Bool bStart = false);
	~Thread();

	Bool Start();
	Bool Start(const String& sThreadName);

	/**
	 * Interrupt synchronous I/O on the thread. Limited support:
	 * Win32 - supported.
	 * Android/iOS/Linux - not supported.
	 */
	Bool CancelSynchronousIO();

	/**
	 * Whether the thread is currently executing. The thread must have
	 * been started and must not be finished to be "running".
	 */
	Bool IsRunning() const
	{
		return (kRunning == m_eState);
	}

	/**
	 * Returns the current execution state of this Thread.
	 */
	State GetState() const
	{
		return m_eState;
	}

	/**
	 * Gets the return value from this thread's previous execution.
	 *
	 * Will return -1 if the thread has not been started or has not yet
	 * finished running.
	 */
	Int GetReturnValue() const
	{
		return m_iReturnValue;
	}

	Int WaitUntilThreadIsNotRunning();

	void SetPriority(Priority ePriority);

	static void Sleep(UInt32 uTimeInMilliseconds);
	static void YieldToAnotherThread();

	static UInt32 GetProcessorCount();

	/**
	 * @return True if this Thread object contains the same thread
	 * as threadB, based on the UnsafeHandle of the thread.
	 */
	Bool operator==(const Thread& threadB) const
	{
		return m_hHandle == threadB.m_hHandle;
	}

	/**
	 * @return True if this Thread object does not contain the same thread
	 * as threadB, based on the UnsafeHandle of the thread.
	 */
	Bool operator!=(const Thread& threadB) const
	{
		return m_hHandle != threadB.m_hHandle;
	}

	// Return the thread name of the current thread - can be nullptr if no name was set.
	static Byte const* GetThisThreadName();

	// Update the thread name of the current thread.
	static void SetThisThreadName(Byte const* sName, UInt32 zNameLengthInBytes);
	static void SetThisThreadName(Byte const* sName)
	{
		SetThisThreadName(sName, StrLen(sName));
	}
	static void SetThisThreadName(const String& sName)
	{
		SetThisThreadName(sName.CStr(), sName.GetSize());
	}

	/**
	 * @return The name of this Thread - may be empty if this Thread has not
	 * been started yet.
	 */
	const String& GetThreadName() const
	{
		return m_sThreadName;
	}

private:
#if SEOUL_PLATFORM_WINDOWS
	static UInt32 SEOUL_STD_CALL _ThreadStart(void* pVoidThread);
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
	static void* _ThreadStart(void* pVoidThread);
#endif

	String m_sThreadName;
	UInt32 m_uStackSize;
	ThreadFunc m_Func;
	UnsafeHandle m_hHandle;

	Signal m_StartupSignal;
	Mutex m_Mutex;
	Atomic32Value<Int> m_iReturnValue;
	Atomic32Value<State> m_eState;

private:
	SEOUL_DISABLE_COPY(Thread);
}; // class Thread

} // namespace Seoul

#endif // include guard
