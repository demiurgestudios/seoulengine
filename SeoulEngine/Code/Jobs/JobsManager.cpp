/**
 * \file JobsManager.cpp
 * \brief Singleton manager for multithreaded programming. Implements a multithreaded
 * cooperative multitasking environment, in which work is divided into units called
 * "Jobs".
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "JobsManager.h"
#include "Logger.h"
#include "ScopedAction.h"
#include "SeoulMath.h"
#include "SeoulString.h"
#include "SeoulTime.h"
#include "UnsafeHandle.h"

#if SEOUL_PLATFORM_IOS
#include "IOSUtil.h"
#endif

namespace Seoul::Jobs
{

/** Absolute maximum number of JobRunnerCoroutines that we will cache. */
static const UInt32 kuMaxJobRunnerCoroutines = 64u;

/**
 * Minimum number of processors to enable a render thread. 2 chosen to
 * allow for per processor:
 * - main thread
 * - render thread
 */
static const UInt32 kuMinimumProcessorCountForSeparateRenderThread = 2u;

/**
 * Minimum number of general purpose worker threads to create - 2
 * so that one can run an expensive job with one still remaining for
 * regular background work.
 */
static const UInt32 kuMinimumGeneralPurposeCount = 2u;

// Sanity check if more quantums are added.
SEOUL_STATIC_ASSERT((Int)Quantum::COUNT == 7);

/** Mapping of periodic quantum enums to times in milliseconds. */
static const Pair<Quantum, Double> s_kaPeriodicQuantums[] =
{
	MakePair(Quantum::k1ms, 1.0),
	MakePair(Quantum::k4ms, 4.0),
	MakePair(Quantum::k8ms, 8.0),
	MakePair(Quantum::k16ms, 16.0),
	MakePair(Quantum::k32ms, 32.0),
};

/** Instantiation of the Singleton pointer. */
CheckedPtr<Manager> Manager::s_pSingleton;

// Sanity check - kDefaultStackCommittedSize must be a minimum size so that
// we don't decommit our coroutine's frame0.
SEOUL_STATIC_ASSERT(Thread::kDefaultStackCommittedSize >= 4096);

RunnerCoroutine::RunnerCoroutine()
	: m_hCoroutine()
	, m_pJob()
	, m_bInJobExecute(false)
{
	// Initialize the coroutine system handle - Manager coroutines use a stack size
	// equal to the default stack size for a Thread.
	m_hCoroutine = CreateCoroutine(
		Thread::kDefaultStackCommittedSize,
		Thread::kDefaultStackReservedSize,
		&Manager::CoroutineMainEntry, this);
}

RunnerCoroutine::~RunnerCoroutine()
{
	UnsafeHandle hCoroutine = m_hCoroutine;
	m_hCoroutine.Reset();

	DeleteCoroutine(hCoroutine);
}

Manager::PerThreadData::PerThreadData(Atomic32Type index)
	: m_ThreadIndex(index)
	, m_aTimes()
	, m_aQueues()
	, m_Signal()
	, m_ThreadId(Thread::GetThisThreadId())
	, m_hThreadCoroutine()
	, m_pLastJobRunnerCoroutine()
#if SEOUL_PLATFORM_IOS
	, m_pAutoReleasePool(nullptr)
	, m_iAutoReleasePoolReferenceCount(0)
#endif // SEOUL_PLATFORM_IOS
	, m_bLastJobWasThreadSpecific(false)
	, m_bThreadSpecificJobsOnly(false)
{
}

Manager::PerThreadData::~PerThreadData()
{
#if SEOUL_PLATFORM_IOS
	SEOUL_ASSERT(nullptr == m_pAutoReleasePool);
	SEOUL_ASSERT(0 == m_iAutoReleasePoolReferenceCount);
#endif // SEOUL_PLATFORM_IOS
}

Manager::Manager()
	: m_GeneralQueue()
	, m_JobRunnerCoroutines()
	, m_vThreads()
	, m_PerThreadStorage()
	, m_vGeneralPurpose()
	, m_nNextGeneralPurpose(0)
	, m_nExecutingJobs(0)
	, m_nRunningThreads(0)
	, m_nWaitUntilJobIsNotRunningCount(0)
	, m_bShuttingDown(false)
	, m_bInBackground(false)
{
	// Sanity check that singletons are being handled as required.
	SEOUL_ASSERT(!s_pSingleton.IsValid());

	// Only valid to call this on the main thread.
	SEOUL_ASSERT(IsMainThread());
	// Sanity - the cores, they keep increasing. Although it is not guaranteed
	// to be sufficient, we want per-thread storage capacity to be 2x the
	// processor count on the system.
	SEOUL_ASSERT(Thread::GetProcessorCount() * 2 <= m_PerThreadStorage.GetCapacity());

	// Convert the Manager's thread/the main thread to a coroutine -
	// needs to be called before any other interaction with the coroutine
	// API can occur on this thread. Also mark the main thread as thread
	// specific.
	auto& r = m_PerThreadStorage.Get();
	r.m_hThreadCoroutine = ConvertThreadToCoroutine(nullptr);
	r.m_bThreadSpecificJobsOnly = true;

	// Assign the singleton now that Manager is in a usable state, before
	// worker threads have been constructed, so external worker threads
	// can register themselves.
	s_pSingleton = this;

	// Initialize all worker threads.
	InitializeThreads();
}

Manager::~Manager()
{
	// Sanity check that singletons are being handled as required.
	SEOUL_ASSERT(this == s_pSingleton);

	// Only valid to call this on the main thread.
	SEOUL_ASSERT(IsMainThread());

	// Not in the background on shutdown.
	m_bInBackground = false;

	ShutdownThreads();

	// Sanity check - make sure all queues are empty on destruction, and no jobs are being run.
#if !SEOUL_ASSERTIONS_DISABLED
	auto const& objects = m_PerThreadStorage.GetAllObjects();
	auto const nObjects = m_PerThreadStorage.GetCount();

	// Check specific queues.
	for (auto j = 0; j < nObjects; ++j)
	{
		auto const& queues = objects[j]->m_aQueues;
		for (auto i = queues.Begin(); queues.End() != i; ++i)
		{
			SEOUL_ASSERT(i->IsEmpty());
		}
	}

	// Check the general queues.
	SEOUL_ASSERT(m_GeneralQueue.IsEmpty());

	SEOUL_ASSERT(0 == m_nExecutingJobs);
#endif

	// Destroy all the runnner coroutines.
	CheckedPtr<RunnerCoroutine> pJobRunnerCoroutine = m_JobRunnerCoroutines.Pop();
	while (pJobRunnerCoroutine)
	{
		// Sanity check - Job should have been unset before inserting the coroutine
		// into the ring buffer.
		SEOUL_ASSERT(!pJobRunnerCoroutine->m_pJob.IsValid());

		// Destroy the runner coroutine.
		SafeDelete(pJobRunnerCoroutine);
		pJobRunnerCoroutine = m_JobRunnerCoroutines.Pop();
	}

	SEOUL_ASSERT(m_JobRunnerCoroutines.IsEmpty());

	s_pSingleton = nullptr;

	// Shutdown coroutine support for this thread.
	ConvertCoroutineToThread();
}

CheckedPtr<Job> Manager::GetCurrentThreadJob() const
{
	// May be null if we're not a Manager thread.
	auto pJobRunnerCoroutine = (RunnerCoroutine*)GetCoroutineUserData();
	if (nullptr == pJobRunnerCoroutine)
	{
		return nullptr;
	}

	return pJobRunnerCoroutine->m_pJob;
}

void Manager::OnEnterBackground()
{
	// Log for testing and debug tracking.
	SEOUL_LOG("Manager::OnEnterBackground()");

	// Now in the background.
	m_bInBackground = true;
}

void Manager::OnLeaveBackground()
{
	// Log for testing and debug tracking.
	SEOUL_LOG("Manager::OnLeaveBackground()");

	if (m_bInBackground)
	{
		// No longer in the background.
		m_bInBackground = false;

		WakeUpAll();
	}
}

/**
 * Schedules pJob for execution. If pJob hasn't been started,
 * it will be started and added to the appropriate queue based on its initial
 * state. Otherwise, it will be advanced through its Job progression. If
 * the job is complete, no action will be taken on it.
 */
void Manager::Schedule(Job& rJob)
{
	CheckedPtr<PerThreadData> pData;
	CheckedPtr<Queue> pQueue;

	// Sanitize the quantum for propery selection.
	auto const eQuantum = ((Int)rJob.GetJobQuantum() >= 0 && (Int)rJob.GetJobQuantum() < (Int)Quantum::COUNT
		? rJob.GetJobQuantum()
		: Quantum::kDefault);
	auto const eState = rJob.GetJobState();

	// Get the correct per-thread data and queue for the job, based on its state.
	switch (eState)
	{
	case State::kNotStarted:
		rJob.StartJob(true);
		return;
	case State::kScheduledForOrRunning:
		if (rJob.GetThreadId().IsValid())
		{
			pData = GetPerThreadData(rJob.GetThreadId());
			pQueue = &pData->m_aQueues[(Int)eQuantum];
		}
		else
		{
			// If the job already has an execution context, keep it on the current thread. It
			// would be fine to switch it to a different thread, except this could result
			// in hard to anticipate behavior in client code (if the client has cached
			// a per-thread value, and then calls YieldThreadTime(), when that method returns to
			// the caller, the per-thread value may now be invalid, because the execution context has
			// switch to a different thread).
			//
			// IMPORTANT: This behavior is required on iOS due to how we use NSAutoRelease pool
			// per thread - switching the thread context of a Coroutine mid execution
			// could result in autorelease objects spread across multiple pools and the
			// ultimate form of sadness (bad access in Cocoa code).
			if (rJob.m_pRunnerCoroutine)
			{
				// A Job that already has a coroutine must only be scheduled from
				// a valid Manager thread.
				SEOUL_ASSERT(nullptr != m_PerThreadStorage.TryGet());

				pData = &m_PerThreadStorage.Get();
				pQueue = &pData->m_aQueues[(Int)eQuantum];
			}
			// Otherwise, place the Job on the general queue.
			else
			{
				pData = nullptr;
				pQueue = &m_GeneralQueue;
			}
		}
		break;
	case State::kComplete: // fall-through
	case State::kError: // fall-through
	default:
		return;
	};

	// Insert the job in its associated queue and then activate its owning thread.
	SeoulGlobalIncrementReferenceCount(&rJob);
	pQueue->Push(&rJob);

	// If the Job was placed on a specific queue, signal the associated thread.
	if (pData)
	{
		pData->m_Signal.Activate();
	}
	// Otherwise, wake up a general purpose worker thread.
	else
	{
		WakeUpNextGeneralPurpose();
	}
}

/**
 * Yields the resources of this thread to a job waiting to be
 * executed.
 *
 * @return True if a job was run, false otherwise.
 */
Bool Manager::YieldThreadTime()
{
	// Optional - handle Yield*() called on non Manager threads.
	auto pData = m_PerThreadStorage.TryGet();
	if (nullptr != pData)
	{
		// Get thread data.
		PerThreadData& rData = *pData;

		// Don't run jobs on threads without a coroutine (external library worker threads will not have one,
		// as well as Seoul::Thread instances now managed by Manager).
		if (rData.m_hThreadCoroutine.IsValid())
		{
			// If the last job was thread specific, try to run a general job,
			// unless the thread is limited to thread specific jobs.
			if (!rData.m_bThreadSpecificJobsOnly && rData.m_bLastJobWasThreadSpecific)
			{
				// If we succeeded in running a general job, mark the change and return success.
				if (ExecuteJob(m_GeneralQueue))
				{
					rData.m_bLastJobWasThreadSpecific = false;
					return true;
				}

				// Try to run a specific job if no general job was available.
				auto eMinQuantumRun = Quantum::MAX_QUANTUM;
				if (ExecuteJob(rData, eMinQuantumRun))
				{
					rData.m_bLastJobWasThreadSpecific = true;

					// Only return true if eMinQuantumRun is at kDefault or smaller.
					if ((Int)eMinQuantumRun <= (Int)Quantum::kDefault)
					{
						return true;
					}
				}
			}
			// Otherwise, if the last job was a general job, try running a specific job
			else
			{
				// Try to run a specific job.
				auto eMinQuantumRun = Quantum::MAX_QUANTUM;
				if (ExecuteJob(rData, eMinQuantumRun))
				{
					rData.m_bLastJobWasThreadSpecific = true;

					// Only return true if eMinQuantumRun is at kDefault or smaller.
					if ((Int)eMinQuantumRun <= (Int)Quantum::kDefault)
					{
						return true;
					}
				}
				// If we didn't run a specific job, try running a general job, unless
				// the current thread is thread specific. If successful, last run
				// status does not change but return success.
				else if (!rData.m_bThreadSpecificJobsOnly && ExecuteJob(m_GeneralQueue))
				{
					return true;
				}
			}
		}

		// Switch back to the thread coroutine if on the main thread.
		if (IsMainThread())
		{
			// Switch back to the current thread's coroutine if we're in a job
			// on the main thread.
			auto pJobRunnerCoroutine = (RunnerCoroutine*)GetCoroutineUserData();
			if (nullptr != pJobRunnerCoroutine)
			{
				m_PerThreadStorage.Get().m_pLastJobRunnerCoroutine = pJobRunnerCoroutine;
				SwitchToCoroutine(m_PerThreadStorage.Get().m_hThreadCoroutine);
				return true;
			}
		}
	}

	// If no Job was run, let the thread yield,
	// so we're not hogging CPU resources.
	Thread::YieldToAnotherThread();

	// If we get here, we did not succeed in running a job.
	return false;
}

/**
 * Wakes up the next general purpose worker threads.
 */
void Manager::WakeUpNextGeneralPurpose()
{
	Atomic32Type const size = (Atomic32Type)m_vGeneralPurpose.GetSize();

	// Shared state.
	Bool bWaiting = false;
	Atomic32Type attempts = 0;

	// Always poke at least 1 - we poke more until we find
	// a thread not already running, or until we've poked
	// them all.
	do
	{
		// Select.
		++attempts;
		Atomic32Type const next = (++m_nNextGeneralPurpose) - 1;
		Atomic32Type const nextIndex = (next % size);
		auto const p = m_vGeneralPurpose[nextIndex];

		// Capture and activate - capture waiting before we potentially wake up the thread.
		bWaiting = p->m_bWaiting;
		p->m_Signal.Activate();
	} while (!bWaiting && attempts < size);
}

/**
 * Wakes up all worker threads.
 */
void Manager::WakeUpAll()
{
	Atomic32Type const nCount = m_PerThreadStorage.GetCount();
	const PerThreadStorage::Objects& objects = m_PerThreadStorage.GetAllObjects();
	for (Atomic32Type i = 0; i < nCount; ++i)
	{
		objects[i]->m_Signal.Activate();
	}
}

/**
 * Called by Job when it a WaitUntilJobIsNotRunning() has started.
 */
void Manager::JobFriend_BeginWaitUntilJobIsNotRunning()
{
	Atomic32Type const nCount = (++m_nWaitUntilJobIsNotRunningCount);

	// If this is the first wait while in the background, wake all
	// to allow work to continue.
	if (1 == nCount && m_bInBackground)
	{
		WakeUpAll();
	}
}

/**
 * Called by Job when it a WaitUntilJobIsNotRunning() has finished.
 */
void Manager::JobFriend_EndWaitUntilJobIsNotRunning()
{
	--m_nWaitUntilJobIsNotRunningCount;
}

/**
 * Setup worker threads - used to process Jobs available in the Manager.
 */
void Manager::InitializeThreads()
{
	// Only valid to call this on the main thread.
	SEOUL_ASSERT(IsMainThread());

	// Setup the file IO thread.
	m_vThreads.PushBack(SEOUL_NEW(MemoryBudgets::Jobs) Thread(SEOUL_BIND_DELEGATE(&Manager::FileIOThreadMain, this)));
	m_vThreads.Back()->Start("FileIO");
	m_vThreads.Back()->SetPriority(Thread::kMed);

	// Cache the processor count.
	UInt32 const uProcessorCount = Thread::GetProcessorCount();

	// We can have a separate render thread if
	// we have at least kuMinimumProcessorCountForSeparateRenderThread processors.
	if (uProcessorCount >= kuMinimumProcessorCountForSeparateRenderThread)
	{
		m_vThreads.PushBack(SEOUL_NEW(MemoryBudgets::Jobs) Thread(SEOUL_BIND_DELEGATE(&Manager::RenderThreadMain, this)));
		m_vThreads.Back()->Start("Render");
	}
	// Otherwise, render thread is just the main thread.
	else
	{
		// Otherwise, the main thread is also the render thread.
		SetRenderThreadId(Thread::GetThisThreadId());
	}

	// Now add the minimum number of general purpose threads.
	for (UInt32 i = 0u; i < kuMinimumGeneralPurposeCount; ++i)
	{
		m_vThreads.PushBack(SEOUL_NEW(MemoryBudgets::Jobs) Thread(SEOUL_BIND_DELEGATE(&Manager::ThreadMain, this)));
		m_vThreads.Back()->Start("Worker");
		m_vThreads.Back()->SetPriority(Thread::kMed);
	}

	// Finally, create enough to utilize every available processor that
	// isn't already utilized by the job system.
	while (m_vThreads.GetSize() < uProcessorCount)
	{
		m_vThreads.PushBack(SEOUL_NEW(MemoryBudgets::Jobs) Thread(SEOUL_BIND_DELEGATE(&Manager::ThreadMain, this)));
		m_vThreads.Back()->Start("Worker");
		m_vThreads.Back()->SetPriority(Thread::kMed);
	}

	// Wait for all threads to signal they are running
	// by incrementing m_nRunningThreads.
	while (m_nRunningThreads < (Atomic32Type)m_vThreads.GetSize())
	{
		Thread::YieldToAnotherThread();
	}

	// Finally, cache the per-thread data of all threads which are not
	// thread-specific as general purpose threads to signal for general
	// work.
	{
		auto const nCount = m_PerThreadStorage.GetCount();
		auto const& objects = m_PerThreadStorage.GetAllObjects();
		for (auto i = 0; i < nCount; ++i)
		{
			auto pData = objects[i];
			if (!pData->m_bThreadSpecificJobsOnly)
			{
				m_vGeneralPurpose.PushBack(pData);
			}
		}
	}
}

/**
 * Stop and cleanup worker threads.
 */
void Manager::ShutdownThreads()
{
	// Only valid to call this on the main thread.
	SEOUL_ASSERT(IsMainThread());

	const PerThreadStorage::Objects& objects = m_PerThreadStorage.GetAllObjects();
	Atomic32Type const nObjects = m_PerThreadStorage.GetCount();

	PerThreadData& rData = m_PerThreadStorage.Get();

	// Finish off all pending jobs.
	Bool bDone = false;
	while (!bDone)
	{
		bDone = true;

		// Process any jobs on the main thread's specific queue.
		for (Queues::SizeType i = 0u; i < rData.m_aQueues.GetSize(); ++i)
		{
			ExecuteJob(rData.m_aQueues[i]);
		}

		// Now check if there are any jobs remaining in any queues.
		bDone = bDone && m_GeneralQueue.IsEmpty();
		for (Atomic32Type j = 0; j < nObjects; ++j)
		{
			for (Queues::SizeType i = 0u; i < rData.m_aQueues.GetSize(); ++i)
			{
				bDone = bDone && objects[j]->m_aQueues[i].IsEmpty();
			}
		}

		// This is intentional - we only want to wake threads up if there are
		// still items on a queue, not if jobs are being executed. This
		// allows the m_nExecutingJobs counter to settle back to 0, as threads
		// will eventually go to sleep and stop processing their queues.
		if (!bDone)
		{
			WakeUpAll();
		}

		// We're done if there are no jobs in any queues, and if no jobs are running.
		bDone = bDone && m_GeneralQueue.IsEmpty();
		bDone = bDone && (0 == m_nExecutingJobs);
	}

	m_bShuttingDown = true;

	WakeUpAll();
	SafeDeleteVector(m_vThreads);

	// Sanity check.
	SEOUL_ASSERT(0 == m_nRunningThreads);
}

/**
 * Body of the worker thread that handles file input/output.
 */
Int Manager::FileIOThreadMain(const Thread& thread)
{
	SetFileIOThreadId(Thread::GetThisThreadId());
	m_PerThreadStorage.Get().m_bThreadSpecificJobsOnly = true;

	return ThreadMain(thread);
}

/**
 * Body of the worker thread that handles the render thread (graphics call submission
 * and graphics object ownership).
 */
Int Manager::RenderThreadMain(const Thread& thread)
{
	SetRenderThreadId(Thread::GetThisThreadId());
	m_PerThreadStorage.Get().m_bThreadSpecificJobsOnly = true;

	return ThreadMain(thread);
}

/**
 * Common main function of all worker threads - either
 * a general purpose thread, or an inner call of FileIOThreadMain
 * or RenderThreadMain.
 */
Int Manager::ThreadMain(const Thread& /* thread */)
{
	// Cache per-thread storage for this worker thread.
	PerThreadData& rPerThreadData = m_PerThreadStorage.Get();

	// Now running.
	++m_nRunningThreads;

	// Convert this thread to a coroutine - necessary before interacting
	// with any other functions of the coroutine API.
	rPerThreadData.m_hThreadCoroutine = ConvertThreadToCoroutine(nullptr);

	while (!m_bShuttingDown)
	{
		// Go to sleep when we enter the background, or if we did not run any jobs.
		if ((m_bInBackground && 0 == m_nWaitUntilJobIsNotRunningCount) || !Manager::Get()->YieldThreadTime())
		{
			// If a job was not run and we're not shutting down, wait
			// for a signal that there is work to do.
			if (!m_bShuttingDown)
			{
				// Set a wake up alarm if we have jobs in our periodic quantum queues.
				UInt32 uWakeUpTimeInMs = 0u;
				if (GetWakeUpTimeInMs(rPerThreadData, uWakeUpTimeInMs))
				{
					// If uWakeUpTimeInMs is zero, just don't wait.
					if (0u != uWakeUpTimeInMs)
					{
						auto const scoped(MakeScopedAction(
							[&]() { rPerThreadData.m_bWaiting = true; },
							[&]() { rPerThreadData.m_bWaiting = false; }));

						rPerThreadData.m_Signal.Wait(uWakeUpTimeInMs);
					}
				}
				// Otherwise, sleep until signaled.
				else
				{
					// Wait to be woken up to do work.
					auto const scoped(MakeScopedAction(
						[&]() { rPerThreadData.m_bWaiting = true; },
						[&]() { rPerThreadData.m_bWaiting = false; }));

					rPerThreadData.m_Signal.Wait();
				}
			}
		}
	}

	// Once we're done running, convert our coroutine back to a thread
	// to cleanup our coroutine resources.
	ConvertCoroutineToThread();

	// Done running.
	-- m_nRunningThreads;

	return 0;
}

/**
 * Attempts to execute a job from rQueue
 *
 * @return True if a job was executed, false otherwise.
 */
Bool Manager::ExecuteJob(Queue& rQueue)
{
	++m_nExecutingJobs;
	Job* pJob = rQueue.Pop();

	// If we failed to grab a job, return immediately.
	if (nullptr == pJob)
	{
		--m_nExecutingJobs;
		return false;
	}

	// If the Job doesn't have a runner yet, create one.
	if (!pJob->m_pRunnerCoroutine)
	{
		// Try to reuse a coroutine from the pool.
		CheckedPtr<RunnerCoroutine> pJobRunnerCoroutine(m_JobRunnerCoroutines.Pop());

		// If we don't have any more on the pool., instantiate a new one.
		if (!pJobRunnerCoroutine.IsValid())
		{
			// Create the RunnerCoroutine object.
			pJobRunnerCoroutine = SEOUL_NEW(MemoryBudgets::Jobs) RunnerCoroutine;

			// If the raw coroutine UnsafeHandle is invalid, we've exceeded system resources
			// and no more coroutines can be created right now. Delete the runner object
			// and push the job by on the queue, then return false, indicating we didn't
			// run a job.
			if (!pJobRunnerCoroutine->m_hCoroutine.IsValid())
			{
				SafeDelete(pJobRunnerCoroutine);
				Schedule(*pJob);
				SeoulGlobalDecrementReferenceCount(pJob);
				--m_nExecutingJobs;
				return false;
			}
		}

		// Sanity checks - neither the runner nor the job should be associated with
		// a job or a runner.
		SEOUL_ASSERT(!pJob->m_pRunnerCoroutine);
		SEOUL_ASSERT(!pJobRunnerCoroutine->m_pJob.IsValid());

		// Set the job as the coroutine's active job.
		pJob->m_pRunnerCoroutine.Set(pJobRunnerCoroutine.Get());
		pJobRunnerCoroutine->m_pJob = pJob;
	}

	// Sanity check that the current thread has been converted to a coroutine before
	// attempting a context switch.
	SEOUL_ASSERT(m_PerThreadStorage.Get().m_hThreadCoroutine.IsValid());

	// Before the context switch, set the current runner to the per-thread data, so
	// the new context can process it after the switch.
	m_PerThreadStorage.Get().m_pLastJobRunnerCoroutine = (RunnerCoroutine*)GetCoroutineUserData();

	// Perform the context switch - if you're debugging this code, depending on the platform,
	// you may or may not "step into" this call properly. This call will jump into
	// Manager::CoroutineMain(). Once Manager::CoroutineMain() switches back
	// to the thread coroutine, this call will appear to "return", although it will
	// actually be a stack context switch.
	CheckedPtr<RunnerCoroutine> pJobRunnerCoroutine((RunnerCoroutine*)pJob->m_pRunnerCoroutine);
	SwitchToCoroutine(pJobRunnerCoroutine->m_hCoroutine);

	// This must always happen after a context switch - it performs correct handling
	// after the switch back, either cleaning up or rescheduling the Job that we
	// switched away from (if there is one).
	AfterContextSwitch();

	// Tell the caller that we successfully ran a job.
	return true;
}

/**
 * Attempt to run a job in one of the queues of PerThreadData,
 * applies a basic scheduling algorithm.
 */
Bool Manager::ExecuteJob(PerThreadData& rData, Quantum& reMinQuantumRun)
{
	Bool bReturn = false;

	Int64 const iTimeInTicks = SeoulTime::GetGameTimeInTicks();

	// Always attempt to run a time critical job.
	bReturn = ExecuteJob(rData, Quantum::kTimeCritical, iTimeInTicks, reMinQuantumRun) || bReturn;

	// Always attempt to run a default quantum job.
	bReturn = ExecuteJob(rData, Quantum::kDefault, iTimeInTicks, reMinQuantumRun) || bReturn;

	// Now run periodic quantum jobs. Periodic jobs run if no job at a smaller quantum
	// has yet run (we had no other work to do), or if we've hit the quantum interval
	// for the periodic job.
	for (size_t i = 0u; i < SEOUL_ARRAY_COUNT(s_kaPeriodicQuantums); ++i)
	{
		auto const e = s_kaPeriodicQuantums[i].First;
		auto const f = s_kaPeriodicQuantums[i].Second;
		if (!bReturn || SeoulTime::ConvertTicksToMilliseconds(iTimeInTicks - (Int64)e) >= f)
		{
			bReturn = ExecuteJob(rData, e, iTimeInTicks, reMinQuantumRun) || bReturn;
		}
	}

	return bReturn;
}

/** Utility used by ExecuteJob(PerThreadData&) */
Bool Manager::ExecuteJob(
	PerThreadData& rData,
	Quantum eQuantum,
	Int64 iCurrentTimeInTicks,
	Quantum& reMinQuantumRun)
{
	Bool const bReturn = ExecuteJob(rData.m_aQueues[(Int)eQuantum]);
	if (bReturn)
	{
		rData.m_aTimes[(Int)eQuantum] = iCurrentTimeInTicks;
		reMinQuantumRun = (Quantum)Min((Int)reMinQuantumRun, (Int)eQuantum);
	}

	return bReturn;
}

/** Check the queues of rData and use the shortest as a wake up time, if any has work to do. */
Bool Manager::GetWakeUpTimeInMs(PerThreadData& rData, UInt32& ruTimeInMs) const
{
	for (size_t i = 0u; i < SEOUL_ARRAY_COUNT(s_kaPeriodicQuantums); ++i)
	{
		auto const e = s_kaPeriodicQuantums[i].First;
		auto const f = s_kaPeriodicQuantums[i].Second;
		if (!rData.m_aQueues[(Int)e].IsEmpty())
		{
			Int64 const iElapsedTime = SeoulTime::GetGameTimeInTicks() - rData.m_aTimes[(Int)e];
			Double const fElapsedTime = SeoulTime::ConvertTicksToMilliseconds(iElapsedTime);

			ruTimeInMs = (UInt32)Max(0.0, f - fElapsedTime);
			return true;
		}
	}

	return false;
}

/**
 * must be called after any coroutine context switch - typically, this is after
 * a call to SwitchToCoroutine(), or at the entry point of a Coroutine main function.
 */
void Manager::AfterContextSwitch()
{
	// Get the last job runner and then set the last runner to nullptr.
	PerThreadData& rPerThreadData = m_PerThreadStorage.Get();
	CheckedPtr<RunnerCoroutine> pLastJobRunnerCoroutine = rPerThreadData.m_pLastJobRunnerCoroutine;
	rPerThreadData.m_pLastJobRunnerCoroutine = nullptr;

	// If non-null, we just context switched from the middle of a running
	// job, so evaluate whether it needs to be rescheduled or whether it's
	// finished running. If this value is nullptr, we just context switched
	// from a thread's main entry point and there's nothing to do.
	if (pLastJobRunnerCoroutine)
	{
		// Cache the last job that was run.
		CheckedPtr<Job> pLastJob = pLastJobRunnerCoroutine->m_pJob;

		// Sanity check - either we've outside the Job's executing function, or it's
		// still in a running state.
		SEOUL_ASSERT(!pLastJobRunnerCoroutine->m_bInJobExecute || pLastJob->IsJobRunning());

		// Whether the job is getting reschedule or not, free up the runner coroutine
		// if the job is not inside it's InternalExecuteJob() member function.
		if (!pLastJobRunnerCoroutine->m_bInJobExecute)
		{
			// Disassociate the job and the runner.
			pLastJob->m_pRunnerCoroutine.Reset();
			pLastJobRunnerCoroutine->m_pJob.Reset();

			// If we already have the maximum, destroy the runner immediately.
			if (m_JobRunnerCoroutines.GetCount() >= kuMaxJobRunnerCoroutines)
			{
				SafeDelete(pLastJobRunnerCoroutine);
			}
			else
			{
				// Prune the runner's coroutine stack prior to release.
				PartialDecommitCoroutineStack(
					pLastJobRunnerCoroutine->m_hCoroutine,
					Thread::kDefaultStackCommittedSize);

				// Hold onto the runner for later use.
				m_JobRunnerCoroutines.Push(pLastJobRunnerCoroutine.Get());
			}
		}

		// If the job is still running, reschedule it.
		if (pLastJob->IsJobRunning())
		{
			Schedule(*pLastJob);
		}

		// In all situations, release the reference on the job - either this
		// releases the Manager's reference on the job completely, or
		// Schedule() acquired a new reference, so we're releasing the reference
		// from the previous run.
		SeoulGlobalDecrementReferenceCount(pLastJob.Get());
		--m_nExecutingJobs;
	}
}

/**
 * Entry point for the coroutines that are used to execute jobs.
 */
void Manager::CoroutineMain(CheckedPtr<RunnerCoroutine> pJobRunnerCoroutine)
{
	SEOUL_ASSERT(nullptr != pJobRunnerCoroutine); // Sanity check.

	// Coroutine entry points must never return - this function doesn't
	// get "called" in the standard sense, there is a context switch
	// that jumps to a new stack with this function at the base.
	while (true)
	{
		// Either this context was first entered, or we just looped
		// around after returning from the SwitchToCoroutine() context switch
		// point below.
		AfterContextSwitch();

		// Need to mark this pointer volatile, so that the compiler
		// re-evaluates it with each loop (since it can change after
		// context switches).
		CheckedPtr<Job> pJob = (Job*)reinterpret_cast<Job volatile*>(pJobRunnerCoroutine->m_pJob.Get());

#if SEOUL_PLATFORM_IOS
		// Acquire the auto releasepool - create a new one if it doesn't exist
		// yet for the current thread. Reference counting is necessary because,
		// if a context switch occurs in Friend_JobManagerExecute(), another
		// Job may be using the pool when this Job returns from Friend_JobManagerExecute().
		if (!IsMainThread())
		{
			PerThreadData& rData = m_PerThreadStorage.Get();
			if (nullptr == rData.m_pAutoReleasePool)
			{
				SEOUL_ASSERT(0 == rData.m_iAutoReleasePoolReferenceCount);
				rData.m_pAutoReleasePool = IOSInitAutoreleasePool();
			}
			++rData.m_iAutoReleasePoolReferenceCount;
		}
#endif

		// Run the job.
		pJobRunnerCoroutine->m_bInJobExecute = true;
		((Job*)pJob)->Friend_JobManagerExecute();
		pJobRunnerCoroutine->m_bInJobExecute = false;

#if SEOUL_PLATFORM_IOS
		// Release our reference to the autorelease pool and if the reference
		// count is 0, drain the pool for the current thread.
		if (!IsMainThread())
		{
			PerThreadData& rData = m_PerThreadStorage.Get();
			SEOUL_ASSERT(rData.m_iAutoReleasePoolReferenceCount > 0);
			--rData.m_iAutoReleasePoolReferenceCount;

			if (0 == rData.m_iAutoReleasePoolReferenceCount)
			{
				void* p = rData.m_pAutoReleasePool;
				rData.m_pAutoReleasePool = nullptr;

				IOSReleaseAutoreleasePool(p);
			}
		}
#endif

		// Switch back to the current thread's coroutine.
		m_PerThreadStorage.Get().m_pLastJobRunnerCoroutine = pJobRunnerCoroutine;
		SwitchToCoroutine(m_PerThreadStorage.Get().m_hThreadCoroutine);
	}
}

} // namespace Seoul::Jobs
