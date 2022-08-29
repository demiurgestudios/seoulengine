/**
 * \file ThreadingInternal.h
 * \brief Internal helper functions for Thread.cpp
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#ifndef THREAD_H
#error "Internal header file, must only be included by Thread.cpp"
#endif

#if SEOUL_PLATFORM_WINDOWS
#include <process.h>
#elif SEOUL_PLATFORM_ANDROID
#include <cpu-features.h>
#include <jni.h>
#elif SEOUL_PLATFORM_LINUX
#include <unistd.h>
#elif SEOUL_PLATFORM_IOS
#include "IOSUtil.h"
#include <sys/sysctl.h>
#endif

// On Windows, we need to use this ...interesting... exceptions hack
// to set the thread name.
#if SEOUL_PLATFORM_WINDOWS
static const DWORD kPlatformPcSetThreadNameExceptionValue = 0x406D1388;

#pragma pack(push, 8)
typedef struct tagTHREADNAME_INFO
{
	DWORD dwType;     // Must be 0x1000.
	LPCSTR szName;    // Pointer to name (in user addr space).
	DWORD dwThreadID; // Thread ID (-1=caller thread).
	DWORD dwFlags;    // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

inline void SetThreadName(DWORD uThreadId, char const* sThreadName)
{
	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = sThreadName;
	info.dwThreadID = uThreadId;
	info.dwFlags = 0;

	__try
	{
		RaiseException(kPlatformPcSetThreadNameExceptionValue, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
	}
	__except(EXCEPTION_CONTINUE_EXECUTION)
	{
	}
}
#endif

namespace Seoul
{

/**
 * Limit the maximum thread name length to avoid runtime assertion failures on
 * platforms like iOS and Android.
 *
 * Limit is set by pthread_setname_np (16 characters including null terminator).
 *
 * \sa https://android.googlesource.com/platform/bionic/+/40eabe24e4e3ae8ebe437f1f4e43cf39cbba2e9e/libc/bionic/pthread_setname_np.cpp
 */
static const UInt32 kuMaxThreadNameLength = 15u;

/** Truncate the input name. */
static inline HString CleanThreadName(Byte const* s, UInt32 u)
{
	return HString(s, Min(u, kuMaxThreadNameLength));
}
static inline HString CleanThreadName(Byte const* s)
{
	return CleanThreadName(s, StrLen(s));
}
static inline HString CleanThreadName(const String& s)
{
	return CleanThreadName(s.CStr(), s.GetSize());
}

namespace ThreadDetail
{

/** Per-thread data used to store the current thread's thread name. */
static PerThreadStorage s_ThreadName;

/**
 * @return The current threads' thread name - can be nullptr if no name was set.
 */
static inline Byte const* GetThreadName()
{
	return (Byte const*)s_ThreadName.GetPerThreadStorage();
}

static inline void SetThreadName(Byte const* sName, UInt32 uNameLengthInBytes)
{
	s_ThreadName.SetPerThreadStorage((void*)CleanThreadName(sName, uNameLengthInBytes).CStr());
}

////////////////////////////////////////////////////////////////////////////////
// Windows Implementations
////////////////////////////////////////////////////////////////////////////////
#if SEOUL_PLATFORM_WINDOWS

/**
 * @return The value of the Win32 function ::GetCurrentThreadId().
 */
inline ThreadId GetThisThreadId()
{
	return ThreadId(::GetCurrentThreadId());
}

/**
 * @return True if id is still an active thread, false otherwise.
 */
inline Bool IsThreadStillAlive(ThreadId id)
{
	HANDLE hThreadHandle = ::OpenThread(
		THREAD_QUERY_INFORMATION,
		FALSE,
		id.GetValue());

	if (nullptr != hThreadHandle)
	{
		DWORD uExitCode = (DWORD)-1;
		if (FALSE != ::GetExitCodeThread(
				hThreadHandle,
				&uExitCode))
		{
			SEOUL_VERIFY(FALSE != ::CloseHandle(hThreadHandle));
			return (STILL_ACTIVE == uExitCode);
		}
		// TODO: Need to determine if there are any error
		// codes which can be returned from ::GetExitCodeThread()
		// which indicate a thread is no longer alive.
		else
		{
			SEOUL_VERIFY(FALSE != ::CloseHandle(hThreadHandle));
			// Conservative - assume all error codes indicate insufficient
			// security rights and the thread is still active.
			return true;
		}
	}
	else
	{
		DWORD uError = ::GetLastError();
		// TODO: Need to determine if there are other error
		// codes which can indicate a thread is no longer alive.
		if (ERROR_INVALID_PARAMETER == uError)
		{
			return false;
		}
		// Conservative - assume other error codes indicate insufficient
		// security rights and the thread is still active.
		else
		{
			return true;
		}
	}
}

/** Cancel synchronous I/O on Win32. */
inline Bool CancelSynchronousIO(UnsafeHandle hHandle)
{
	// Query for, available only on Vista+
	HINSTANCE hKernel = ::GetModuleHandleW(L"Kernel32");
	if (nullptr == hKernel || INVALID_HANDLE_VALUE == hKernel)
	{
		return false;
	}

	auto func = (BOOL(WINAPI*)(HANDLE hThread))::GetProcAddress(hKernel, "CancelSynchronousIo");
	if (nullptr == func)
	{
		return false;
	}

	// Ignore return - will return false if there is no I/O to cancel.
	(void)func(StaticCast<HANDLE>(hHandle));
	return true;
}

inline void WaitForThread(Atomic32Value<Thread::State>& reState, UnsafeHandle hHandle)
{
	(void)WaitForSingleObject(StaticCast<HANDLE>(hHandle), INFINITE);
	reState = Thread::kDoneRunning;
}

inline void DestroyThread(UnsafeHandle& rhHandle)
{
	HANDLE hThread = StaticCast<HANDLE>(rhHandle);
	rhHandle.Reset();

	CloseHandle(hThread);
}

typedef UInt32 (WINAPI* ThreadFunc)(LPVOID);
inline Bool Start(
	Byte const* sThreadName,
	UInt32 uStackSize,
	ThreadFunc threadFunctionCallback,
	Thread* pThreadFunctionArgument,
	UnsafeHandle& rhHandle)
{
	static const DWORD uFlags = 0u;

	UInt32 uThreadId = 0u;

	// See http://support.microsoft.com/kb/104641/en-us for why
	// we're using _beginthreadex here instead of CreateThread.
	//
	// Also see: http://msdn.microsoft.com/en-us/library/kdzttdcb%28v=VS.80%29.aspx
	rhHandle = (HANDLE)_beginthreadex(
		nullptr,											// default security
		uStackSize,										// stack size of the thread
		threadFunctionCallback,							// thread main function
		static_cast<LPVOID>(pThreadFunctionArgument),	// argument of the main function
		uFlags,											// flags
		&uThreadId);										// return-value address of the thread

	if (rhHandle.IsValid())
	{
		::SetThreadName(uThreadId, CleanThreadName(sThreadName).CStr());

		return true;
	}
	else
	{
		return false;
	}
}

inline void SetThreadPriority(
	UnsafeHandle hHandle,
	Thread::Priority ePriority)
{
	switch (ePriority)
	{
	case Thread::kLow:
		::SetThreadPriority(StaticCast<HANDLE>(hHandle), THREAD_PRIORITY_LOWEST);
	break;
	case Thread::kMed:
		::SetThreadPriority(StaticCast<HANDLE>(hHandle), THREAD_PRIORITY_NORMAL);
	break;
	case Thread::kHigh:
		::SetThreadPriority(StaticCast<HANDLE>(hHandle), THREAD_PRIORITY_HIGHEST);
	break;
	case Thread::kCritical:
		::SetThreadPriority(StaticCast<HANDLE>(hHandle), THREAD_PRIORITY_TIME_CRITICAL);
	break;
	default:
		SEOUL_FAIL(__FUNCTION__ ": out of sync enum, this is a bug.");
		break;
	}
}

inline void Sleep(UInt32 uTimeInMilliseconds)
{
	::Sleep(uTimeInMilliseconds);
}

inline void YieldToAnotherThread()
{
	::SwitchToThread();
}

inline UInt32 GetProcessorCount()
{
	SYSTEM_INFO systemInfo;
	memset(&systemInfo, 0, sizeof(SYSTEM_INFO));
	GetSystemInfo(&systemInfo);

	return systemInfo.dwNumberOfProcessors;
}
#endif // /#if SEOUL_PLATFORM_WINDOWS

////////////////////////////////////////////////////////////////////////////////
// Android, iOS, and Linux Implementations
////////////////////////////////////////////////////////////////////////////////
#if SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
class ThreadIdFactory SEOUL_SEALED
{
public:
	/** Maximum number of simultaneously active threads supported. */
	static const ThreadId::ThreadIdValueType kMaxThreads = 1024u;

	// Typedef of the fixed array used to track whether a thread slot is alive. 8/
	typedef FixedArray<ThreadId::ThreadIdValueType, kMaxThreads> AllocatedThreadIds;

	// Sanity check that kMaxThreads is power of 2.
	SEOUL_STATIC_ASSERT((0 == (kMaxThreads & (kMaxThreads - 1))));

	ThreadIdFactory()
		: m_aAllocatedThreadIds((ThreadId::ThreadIdValueType)0)
		, m_NextThreadId(0)
		, m_ThreadIdPerThreadStorage()
	{
		memset(&m_ThreadIdPerThreadStorage, 0, sizeof(m_ThreadIdPerThreadStorage));
		SEOUL_VERIFY(0 == pthread_key_create(&m_ThreadIdPerThreadStorage, OnThreadDestroyed));
	}

	~ThreadIdFactory()
	{
		SEOUL_VERIFY(0 == pthread_key_delete(m_ThreadIdPerThreadStorage));
		memset(&m_ThreadIdPerThreadStorage, 0, sizeof(m_ThreadIdPerThreadStorage));
	}

	/** @return The ThreadId for the current thread. */
	ThreadId GetThisThreadId()
	{
		// Check if a thread id has already been allocated.
		void* pValue = pthread_getspecific(m_ThreadIdPerThreadStorage);

		// If not, allocate one.
		if (nullptr == pValue)
		{
			// TODO: This will deadlock if we hit kMaxThreads.
			while (true)
			{
				// Atomic reserve a thread ID.
				ThreadId::ThreadIdValueType const value = (ThreadId::ThreadIdValueType)(++m_NextThreadId);

				// If we wrapped around and ended up with 0, try again.
				if (0 == value)
				{
					continue;
				}

				// Get the array index for the associated id.
				AllocatedThreadIds::SizeType const index = ToAllocatedThreadIdsIndex(value);

				// If that slot is not active, used it.
				if (0 == m_aAllocatedThreadIds[index])
				{
					// We're using this slot now.
					m_aAllocatedThreadIds[index] = value;

					// Convert the value to a pointer.
					pValue = ToPointer(value);

					// Set the value to the current thread's local storage.
					SEOUL_VERIFY(0 == pthread_setspecific(m_ThreadIdPerThreadStorage, pValue));

					// Done allocating.
					break;
				}
			}
		}

		// Return the thread id.
		return ThreadId(ToThreadIdValue(pValue));
	}

	/** @return True if the thread associated with id is still an active thread, false otherwise. */
	Bool IsThreadStillAlive(ThreadId id) const
	{
		if (!id.IsValid())
		{
			return false;
		}

		SEOUL_ASSERT(id.GetValue() > 0);
		Bool const bReturn = (id.GetValue() == m_aAllocatedThreadIds[ToAllocatedThreadIdsIndex(id.GetValue())]);
		return bReturn;
	}

private:
	static void OnThreadDestroyed(void* pValue);

	/** @return the slot index for the thread id value. */
	static AllocatedThreadIds::SizeType ToAllocatedThreadIdsIndex(ThreadId::ThreadIdValueType value)
	{
		SEOUL_ASSERT(value > 0);

		return (AllocatedThreadIds::SizeType)((value - 1) & (kMaxThreads - 1));
	}

	/** @return the slot index for the thread id defined by pointer pValue. */
	static AllocatedThreadIds::SizeType ToAllocatedThreadIdsIndex(void* pValue)
	{
		return ToAllocatedThreadIdsIndex(ToThreadIdValue(pValue));
	}

	/** @return The thread id defined by pointer pValue. */
	static ThreadId::ThreadIdValueType ToThreadIdValue(void* pValue)
	{
		SEOUL_STATIC_ASSERT(sizeof(ThreadId::ThreadIdValueType) == sizeof(pValue));

		union
		{
			void* pIn;
			ThreadId::ThreadIdValueType valueOut;
		};
		pIn = pValue;
		return valueOut;
	}

	/** @return The pointed that defines the thread id value. */
	static void* ToPointer(ThreadId::ThreadIdValueType value)
	{
		SEOUL_STATIC_ASSERT(sizeof(value) == sizeof(void*));

		union
		{
			ThreadId::ThreadIdValueType valueIn;
			void* pOut;
		};
		valueIn = value;
		return pOut;
	}

	AllocatedThreadIds m_aAllocatedThreadIds;
	Atomic32 m_NextThreadId;
	pthread_key_t m_ThreadIdPerThreadStorage;

	SEOUL_DISABLE_COPY(ThreadIdFactory);
}; // class ThreadIdFactory

static ThreadIdFactory& GetThreadIdFactory()
{
	static ThreadIdFactory s_ThreadIdFactory;
	return s_ThreadIdFactory;
}

/** Invoked by thread-local storage when a thread has been destroyed. */
void ThreadIdFactory::OnThreadDestroyed(void* pValue)
{
	ThreadIdFactory& idFactory = GetThreadIdFactory();

	// The thread's slot is now free for use by other threads.
	idFactory.m_aAllocatedThreadIds[ToAllocatedThreadIdsIndex(pValue)] = 0;
}

/**
 * @return The current thread's thread ID.	This is obtained from an abstract
 * thread-local variable and does not have any relation to the value of
 * pthread_self(), other than that different threads have different
 * IDs.
 */
inline ThreadId GetThisThreadId()
{
	ThreadIdFactory& idFactory = GetThreadIdFactory();

	return idFactory.GetThisThreadId();
}

/**
 * @return True if the thread identified by threadId is still alive, false
 * otherwise.
 */
inline Bool IsThreadStillAlive(ThreadId threadId)
{
	ThreadIdFactory& idFactory = GetThreadIdFactory();

	return idFactory.IsThreadStillAlive(threadId);
}

/** Nop on POSIX. */
inline Bool CancelSynchronousIO(UnsafeHandle hhHandle)
{
	return false;
}

inline void WaitForThread(Atomic32Value<Thread::State>& reState, UnsafeHandle hHandle)
{
	void* pReturnCode = nullptr;
	SEOUL_VERIFY(0 == pthread_join(
		*StaticCast<pthread_t*>(hHandle),
		&pReturnCode));
	reState = Thread::kDoneRunning;
}

inline void DestroyThread(UnsafeHandle& rhHandle)
{
	pthread_t* pThread = StaticCast<pthread_t*>(rhHandle);
	rhHandle.Reset();
	SafeDelete(pThread);
}

typedef void* (*ThreadFunc)(void*);
inline Bool Start(
	Byte const* sThreadName,
	UInt32 uStackSize,
	ThreadFunc threadFunctionCallback,
	Thread* pThreadFunctionArgument,
	UnsafeHandle& rhHandle)
{
#if SEOUL_PLATFORM_IOS
	// Make sure that Cocoa is aware that we are multithreaded
	IOSEnsureCocoaIsMultithreaded();
#endif

	pthread_t* pThread = nullptr;
	Bool bSuccess = false;

	// Construct the thread object and initialize it. Cleanup
	// initialization attributes once we're done.
	{
		pthread_attr_t attributes;
		SEOUL_VERIFY(0 == pthread_attr_init(&attributes));
		SEOUL_VERIFY(0 == pthread_attr_setstacksize(&attributes, uStackSize));
		pThread = SEOUL_NEW(MemoryBudgets::TBD) pthread_t;
		bSuccess = (0 == pthread_create(
			pThread,
			&attributes,
			threadFunctionCallback,
			pThreadFunctionArgument));
		SEOUL_VERIFY(0 == pthread_attr_destroy(&attributes));
	}

	// Handling based on result.
	if (bSuccess)
	{
		// On supported platforms (Android for now), setup
		// the name for the thread so it appears in
		// developer tools.
#if SEOUL_PLATFORM_ANDROID
		(void)pthread_setname_np(*pThread, CleanThreadName(sThreadName).CStr());
#endif
		rhHandle = pThread;
		return true;
	}
	// Cleanup the thread and fail.
	else
	{
		SafeDelete(pThread);
		return false;
	}
}

inline void SetThreadPriority(
	UnsafeHandle hHandle,
	Thread::Priority ePriority)
{
	pthread_t threadID = *StaticCast<pthread_t*>(hHandle);

	Int iPolicy = -1;
	struct sched_param scheduleParameter;
	memset(&scheduleParameter, 0, sizeof(scheduleParameter));
	SEOUL_VERIFY(0 == pthread_getschedparam(threadID, &iPolicy, &scheduleParameter));

	Int const iLowestPriority = sched_get_priority_min(iPolicy);
	Int const iCriticalPriority = sched_get_priority_max(iPolicy);
	Int const iMedPriority = (Int)((iCriticalPriority - iLowestPriority) / 3.0f) + iLowestPriority;
	Int const iHighPriority = (Int)((2.0f * (iCriticalPriority - iLowestPriority)) / 3.0f) + iLowestPriority;

	switch (ePriority)
	{
	case Thread::kLow:
		scheduleParameter.sched_priority = iLowestPriority;
		break;
	case Thread::kMed:
		scheduleParameter.sched_priority = iMedPriority;
		break;
	case Thread::kHigh:
		scheduleParameter.sched_priority = iHighPriority;
		break;
	case Thread::kCritical:
		scheduleParameter.sched_priority = iCriticalPriority;
		break;
	default:
		SEOUL_FAIL("SetThreadPriority: out of sync enum, this is a bug.");
		break;
	};

	SEOUL_VERIFY(0 == pthread_setschedparam(threadID, iPolicy, &scheduleParameter));
}

inline void Sleep(UInt32 uTimeInMilliseconds)
{
	struct timespec ts;
	MillisecondsToTimespec(uTimeInMilliseconds, ts);

	struct timespec remainder;
	memset(&remainder, 0, sizeof(remainder));

	while (0 != nanosleep(&ts, &remainder))
	{
		// Cache the error value.
		Int const iError = errno;

		// Error value should only be EINTR, which indicates a signal interrupt.
		SEOUL_ASSERT(EINTR == iError);

		// For sanity, if we hit this case, break out and abort the sleep.
		if (EINTR != iError)
		{
			break;
		}

		// Set the time spec to the remaining time, and clear out the remainder.
		memcpy(&ts, &remainder, sizeof(ts));
		memset(&remainder, 0, sizeof(remainder));
	}
}

inline void YieldToAnotherThread()
{
	// SeoulEngine API Yield() maps directly to
	// pthread sched_yield().
	SEOUL_VERIFY(0 == sched_yield());
}

inline UInt32 GetProcessorCount()
{
#if SEOUL_PLATFORM_IOS
	UInt32 nReturn = 0u;
	size_t zNcpuLength = sizeof(nReturn);
	SEOUL_VERIFY(0 == sysctlbyname("hw.ncpu", &nReturn, &zNcpuLength, nullptr, 0));
	return nReturn;
#elif SEOUL_PLATFORM_ANDROID
	return (UInt32)android_getCpuCount();
#elif SEOUL_PLATFORM_LINUX
	return sysconf( _SC_NPROCESSORS_ONLN );
#else
#	error "Define for this platform."
#endif
}

// Android specific functionality for JavaVM interaction.
#if SEOUL_PLATFORM_ANDROID
/** Global pointer to the Java VM instance. */
static JavaVM* s_pSeoulEngineGlobalJavaVM = nullptr;

/**
 * Attach the current thread for JNI invocations - must
 * be called once and only once per thread.
 */
inline static void AttachJavaVMToThisThread(Byte const* sName)
{
	JNIEnv* pJNIEnv = nullptr;

	// Proper usage dictates the current thread (main thread) is not yet
	// attached.
	SEOUL_VERIFY(JNI_EDETACHED == s_pSeoulEngineGlobalJavaVM->GetEnv((void**)&pJNIEnv, JNI_VERSION_1_6));

	// Proper usage dictates the current thread (main thread) is not yet
	// attached.
	JavaVMAttachArgs aArguments =
	{
		JNI_VERSION_1_6,
		sName,
		nullptr
	};

	// Attach a JNI Environment to the curren thread.
	SEOUL_VERIFY(JNI_OK == s_pSeoulEngineGlobalJavaVM->AttachCurrentThread(&pJNIEnv, &aArguments));
}

/**
 * Detach the current thread from JNI invocations - call
 * only once before thread shutdown.
 */
inline static void DetachJavaVMFromThisThread()
{
	// Release the current thread from the JavaVM.
	SEOUL_VERIFY(JNI_OK == s_pSeoulEngineGlobalJavaVM->DetachCurrentThread());
}
#endif // /#if SEOUL_PLATFORM_ANDROID

#endif // /#if SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX

} // namespace ThreadDetail

////////////////////////////////////////////////////////////////////////////////
// Windows Implementation of _ThreadStart
////////////////////////////////////////////////////////////////////////////////
#if SEOUL_PLATFORM_WINDOWS
/**
 * Helper method, passed as a function pointer to the Win32 _beginthreadex
 * function for creating a thread. Essentially, this is a wrapper function for
 * connecting a Seoul engine Delegate to the function pointer that
 * Win32 requires for executing a thread body.
 */
UInt32 WINAPI Thread::_ThreadStart(LPVOID pVoidThread)
{
	Thread* pThread = static_cast<Thread*>(pVoidThread);

	// Set the thread name.
	ThreadDetail::SetThreadName(pThread->GetThreadName().CStr(), pThread->GetThreadName().GetSize());

	// Signal the thread as started.
	pThread->m_StartupSignal.Activate();

	// The thread state should be kRunning before and after
	// the main function. Basically, this thread must own
	// the Thread state until the thread is done running.
	SEOUL_ASSERT(Thread::kRunning == pThread->m_eState);
	pThread->m_iReturnValue = (pThread->m_Func)(*pThread);
	SEOUL_ASSERT(Thread::kRunning == pThread->m_eState);

	// Lock before finishing - this prevents race conditions
	// with methods like SetPriority().
	{
		Lock lock(pThread->m_Mutex);
		pThread->m_eState = Thread::kDoneRunning;
	}

	// See: http://msdn.microsoft.com/en-us/library/hw264s73%28v=VS.80%29.aspx
	_endthreadex(0u);

	return 0u;
}
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
void* Thread::_ThreadStart(void* pArgument)
{
#if SEOUL_PLATFORM_IOS
	// Set up an autorelease pool for the thread, in case it ever creates any
	// autoreleased Objective-C objects
	void *pAutoreleasePool = IOSInitAutoreleasePool();
#endif

	Thread* pThread = reinterpret_cast<Thread*>(pArgument);

	// Set the thread name.
	Byte const* sThreadName = CleanThreadName(pThread->GetThreadName()).CStr();
	ThreadDetail::s_ThreadName.SetPerThreadStorage(const_cast<char*>(sThreadName));

	// Setup JNI for this thread on Android.
#if SEOUL_PLATFORM_ANDROID
	ThreadDetail::AttachJavaVMToThisThread(sThreadName);
#endif

#if SEOUL_PLATFORM_IOS
	SEOUL_VERIFY(pthread_setname_np(sThreadName) == 0);
#endif

	// Signal the thread as started.
	pThread->m_StartupSignal.Activate();

	// The thread state should be kRunning before and after
	// the main function. Basically, this thread must own
	// the Thread state until the thread is done running.
	SEOUL_ASSERT(Thread::kRunning == pThread->m_eState);
	pThread->m_iReturnValue = (pThread->m_Func)(*pThread);
	SEOUL_ASSERT(Thread::kRunning == pThread->m_eState);

	// Teardown JNI for this thread on Android.
#if SEOUL_PLATFORM_ANDROID
	ThreadDetail::DetachJavaVMFromThisThread();
#endif

#if SEOUL_PLATFORM_IOS
	IOSReleaseAutoreleasePool(pAutoreleasePool);
#endif

	// Lock before finishing - this prevents race conditions
	// with methods like SetPriority().
	{
		Lock lock(pThread->m_Mutex);
		pThread->m_eState = Thread::kDoneRunning;
	}

	// Exit the thread.
	pthread_exit(0u);

	return nullptr;
}
#else
#error "Define for this platform."
#endif

// Android specific functionality for setting up native<->Java
// associations
#if SEOUL_PLATFORM_ANDROID
JNIEnv* Thread::GetThisThreadJNIEnv()
{
	JNIEnv* pJNIEnv = nullptr;

	// Proper usage dictates the current thread is already
	// attached and ok, so this call must succeed.
	SEOUL_VERIFY(JNI_OK == ThreadDetail::s_pSeoulEngineGlobalJavaVM->GetEnv((void**)&pJNIEnv, JNI_VERSION_1_6));

	return pJNIEnv;
}

void Thread::InitializeJavaNativeThreading(JavaVM* pJavaVM)
{
	// Invalid behavior if the global VM pointer is already set.
	SEOUL_ASSERT(nullptr == ThreadDetail::s_pSeoulEngineGlobalJavaVM);

	// Cache the global VM pointer and then attach the main thread.
	ThreadDetail::s_pSeoulEngineGlobalJavaVM = pJavaVM;
#if !SEOUL_BUILD_UE4 // Handled for us by UE4 code.
	ThreadDetail::AttachJavaVMToThisThread("MainThread");
#endif
}

void Thread::ShutdownJavaNativeThreading()
{
	// Invalid behavior if the global VM pointer is not set.
	SEOUL_ASSERT(nullptr != ThreadDetail::s_pSeoulEngineGlobalJavaVM);

	// Detacth the main thread and clear the VM pointer.
#if !SEOUL_BUILD_UE4
	ThreadDetail::DetachJavaVMFromThisThread();
#endif
	ThreadDetail::s_pSeoulEngineGlobalJavaVM = nullptr;
}
#endif // /#if SEOUL_PLATFORM_ANDROID

} // namespace Seoul
