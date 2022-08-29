/**
 * \file ThreadsTest.cpp
 * \brief Implementation file for threading system unit tests
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Logger.h"
#include "MemoryManager.h"
#include "ReflectionDefine.h"
#include "ScopedPtr.h"
#include "ThreadsTest.h"
#include "UnitTesting.h"
#include <algorithm>

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(ThreadTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestThreads)
	SEOUL_METHOD(TestMutexes)
	SEOUL_METHOD(TestSignalInfiniteWait)
	SEOUL_METHOD(TestSignal0Wait)
	SEOUL_METHOD(TestSignalTimedWait)
SEOUL_END_TYPE()

// Thread testing
struct BasicTest
{
	SEOUL_DELEGATE_TARGET(BasicTest);

	BasicTest()
		: m_iSetToTwo(0)
	{ }

	Int Run(const Thread& thread)
	{
		m_iSetToTwo = 2;
		return 3;
	}

	Int m_iSetToTwo;
}; // struct BasicTest

void ThreadTest::TestThreads()
{
	BasicTest test;

	// Run the the thread, wait for it to finish, and make sure
	// values are what we expect.
	Int iExitStatus = -1;
	Thread thread(SEOUL_BIND_DELEGATE(&BasicTest::Run, &test));
	SEOUL_VERIFY(thread.Start());
	iExitStatus = thread.WaitUntilThreadIsNotRunning();

	SEOUL_UNITTESTING_ASSERT_EQUAL((Int)3, iExitStatus);
	SEOUL_UNITTESTING_ASSERT_EQUAL((Int)2, test.m_iSetToTwo);
}
// /Thread testing

// Mutex testing
static const Int32 kMutexTestIterations = 1000;

struct MutexTest
{
	SEOUL_DELEGATE_TARGET(MutexTest);

	static UInt32 s_uSharedResource;
	static Mutex s_SharedResourceMutex;

	Int Run(const Thread& thread)
	{
		for (Int i = 0; i < kMutexTestIterations; ++i)
		{
			Lock lock(s_SharedResourceMutex);
			s_uSharedResource++;
		}

		return -1;
	}
};

UInt32 MutexTest::s_uSharedResource = 0u;
Mutex MutexTest::s_SharedResourceMutex;

void ThreadTest::TestMutexes()
{
	static const Int32 kTestThreadCount = 5;
	MutexTest test;

	ScopedPtr<Thread> apThreads[kTestThreadCount];
	apThreads[0].Reset(SEOUL_NEW(MemoryBudgets::TBD) Thread(SEOUL_BIND_DELEGATE(&MutexTest::Run, &test)));
	apThreads[1].Reset(SEOUL_NEW(MemoryBudgets::TBD) Thread(SEOUL_BIND_DELEGATE(&MutexTest::Run, &test)));
	apThreads[2].Reset(SEOUL_NEW(MemoryBudgets::TBD) Thread(SEOUL_BIND_DELEGATE(&MutexTest::Run, &test)));
	apThreads[3].Reset(SEOUL_NEW(MemoryBudgets::TBD) Thread(SEOUL_BIND_DELEGATE(&MutexTest::Run, &test)));
	apThreads[4].Reset(SEOUL_NEW(MemoryBudgets::TBD) Thread(SEOUL_BIND_DELEGATE(&MutexTest::Run, &test)));

	Int result[kTestThreadCount];

	// Start threads
	for (Int32 i = 0; i < kTestThreadCount; ++i)
	{
		SEOUL_VERIFY(apThreads[i]->Start());
	}

	// Wait for threads to finish.
	for (Int32 i = 0; i < kTestThreadCount; ++i)
	{
		result[i] = apThreads[i]->WaitUntilThreadIsNotRunning();
	}

	// Make sure that the shared resource count is what we expect,
	SEOUL_UNITTESTING_ASSERT(MutexTest::s_uSharedResource ==
		(kTestThreadCount * kMutexTestIterations));

	// Check that return values are what we expect (-1).
	for (Int32 i = 0; i < kTestThreadCount; ++i)
	{
		SEOUL_UNITTESTING_ASSERT(-1 == result[i]);
	}
}
// /Mutex testing

// Signal testing
struct SignalTest
{
	SEOUL_DELEGATE_TARGET(SignalTest);

	Signal const m_Signal;
	Bool const m_bUseTime;
	UInt32 const m_uWaitTime;

	SignalTest(Bool bUseTime, UInt32 uWaitTime)
		: m_bUseTime(bUseTime)
		, m_uWaitTime(uWaitTime)
	{
	}

	Int Run(const Thread& thread)
	{
		if (m_bUseTime)
		{
			return m_Signal.Wait(m_uWaitTime) ? 1 : 0;
		}
		else
		{
			m_Signal.Wait();
			return true;
		}
	}
};

void ThreadTest::TestSignalInfiniteWait()
{
	static const UInt32 kSignalWaitTime = 256u;

	static const Int32 kSignalTestThreadCount = 7;
	SignalTest signal[kSignalTestThreadCount] =
	{
		SignalTest(true, kSignalWaitTime),
		SignalTest(true, kSignalWaitTime),
		SignalTest(true, kSignalWaitTime),
		SignalTest(true, kSignalWaitTime),
		SignalTest(true, kSignalWaitTime),
		SignalTest(true, kSignalWaitTime),
		SignalTest(true, kSignalWaitTime),
	};

	ScopedPtr<Thread> apThreads[kSignalTestThreadCount];
	for (Int32 i = 0; i < kSignalTestThreadCount; ++i)
	{
		apThreads[i].Reset(SEOUL_NEW(MemoryBudgets::TBD) Thread(SEOUL_BIND_DELEGATE(&SignalTest::Run, &(signal[i]))));
	}

	Int result[kSignalTestThreadCount];

	// Start threads
	for (Int32 i = 0; i < kSignalTestThreadCount; ++i)
	{
		SEOUL_VERIFY(apThreads[i]->Start());
	}

	// Signal
	for (Int32 i = 0; i < kSignalTestThreadCount; ++i)
	{
		signal[i].m_Signal.Activate();
	}

	// Wait to finish
	for (Int32 i = 0; i < kSignalTestThreadCount; ++i)
	{
		result[i] = apThreads[i]->WaitUntilThreadIsNotRunning();
	}

	// Check return values - these should all be true as we signaled
	// these threads.
	for (Int32 i = 0; i < kSignalTestThreadCount; ++i)
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL((int)true, result[i]);
	}
}

void ThreadTest::TestSignal0Wait()
{
	Signal signal;
	for (UInt32 i = 0u; i < 100u; ++i)
	{
		SEOUL_UNITTESTING_ASSERT(!signal.Wait(0u));
	}
}

void ThreadTest::TestSignalTimedWait()
{
	static const UInt32 kSignalWaitTime = 128u;

	static const Int32 kNoSignalTestThreadCount = 5;
	static const Int32 kSignalTestThreadCount = 2;
	static const Int32 kTestThreadCount = (kNoSignalTestThreadCount + kSignalTestThreadCount);
	SignalTest noSignal(true, kSignalWaitTime);
	SignalTest signal[kSignalTestThreadCount] =
	{
		SignalTest(true, kSignalWaitTime),
		SignalTest(true, kSignalWaitTime),
	};

	ScopedPtr<Thread> apThreads[kTestThreadCount];
	apThreads[0].Reset(SEOUL_NEW(MemoryBudgets::TBD) Thread(SEOUL_BIND_DELEGATE(&SignalTest::Run, &noSignal)));
	apThreads[1].Reset(SEOUL_NEW(MemoryBudgets::TBD) Thread(SEOUL_BIND_DELEGATE(&SignalTest::Run, &noSignal)));
	apThreads[2].Reset(SEOUL_NEW(MemoryBudgets::TBD) Thread(SEOUL_BIND_DELEGATE(&SignalTest::Run, &noSignal)));
	apThreads[3].Reset(SEOUL_NEW(MemoryBudgets::TBD) Thread(SEOUL_BIND_DELEGATE(&SignalTest::Run, &noSignal)));
	apThreads[4].Reset(SEOUL_NEW(MemoryBudgets::TBD) Thread(SEOUL_BIND_DELEGATE(&SignalTest::Run, &noSignal)));
	apThreads[5].Reset(SEOUL_NEW(MemoryBudgets::TBD) Thread(SEOUL_BIND_DELEGATE(&SignalTest::Run, &(signal[0]))));
	apThreads[6].Reset(SEOUL_NEW(MemoryBudgets::TBD) Thread(SEOUL_BIND_DELEGATE(&SignalTest::Run, &(signal[1]))));

	Int result[kTestThreadCount];

	// Start threads
	for (Int32 i = 0; i < kTestThreadCount; ++i)
	{
		SEOUL_VERIFY(apThreads[i]->Start());
	}

	// Signal
	for (Int32 i = 0; i < kSignalTestThreadCount; ++i)
	{
		signal[i].m_Signal.Activate();
	}

	// Wait to finish
	for (Int32 i = 0; i < kTestThreadCount; ++i)
	{
		result[i] = apThreads[i]->WaitUntilThreadIsNotRunning();
	}

	// Check return values - these should all be false as we did not
	// signal any of these threads.
	for (Int32 i = 0; i < kNoSignalTestThreadCount; ++i)
	{
		SEOUL_UNITTESTING_ASSERT((Int)false == result[i]);
	}

	// Check return values - these should all be true as we signaled
	// these threads.
	for (Int32 i = 0; i < kSignalTestThreadCount; ++i)
	{
		SEOUL_UNITTESTING_ASSERT((int)true == result[kNoSignalTestThreadCount + i]);
	}
}
// /Mutex testing

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
