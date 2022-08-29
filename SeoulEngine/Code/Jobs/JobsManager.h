/**
 * \file JobsManager.h
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

#pragma once
#ifndef JOBS_MANAGER_H
#define JOBS_MANAGER_H

#include "AtomicRingBuffer.h"
#include "Coroutine.h"
#include "HeapAllocatedPerThreadStorage.h"
#include "FixedArray.h"
#include "JobsJob.h"
#include "Prereqs.h"
#include "SeoulSignal.h"
#include "Singleton.h"
#include "Thread.h"

namespace Seoul::Jobs
{

/**
 * Encapsulates a Seoul::Coroutine that is used to execute
 * a Job. There is a one-to-one relationship between an executing
 * Job and a RunnerCoroutine, however, JobRunnerCoroutines are
 * pooled and reused as soon as possible once Jobs enter a non-executing
 * state, as they tend to be a highly limited system resource.
 */
struct RunnerCoroutine SEOUL_SEALED
{
	RunnerCoroutine();
	~RunnerCoroutine();

	UnsafeHandle m_hCoroutine;
	CheckedPtr<Job> m_pJob;
	Atomic32Value<Bool> m_bInJobExecute;
}; // class RunnerCoroutine

/**
 * Manager is a singleton manager for multithreaded programming.
 * Jobs are Schedule()ed by the Manager and then executed on
 * threads as those threads become available.
 *
 * A Job must manage its own thread-safety. Any actions it takes
 * inside its InternalExecuteJob() method, which is called by the Manager,
 * must be thread-safe.
 *
 * Job dependencies can be handled by programming a Job to Job::Start()
 * other Jobs inside its InternalExecuteJob() method, and then
 * Job::WaitUntilJobIsNotRunning() for those Jobs to complete. This is not
 * a busy wait - Manager::Yield() will be called to give other Jobs a chance
 * to run on the waiting thread.
 */
class Manager SEOUL_SEALED
{
public:
	SEOUL_DELEGATE_TARGET(Manager);

	/** Queue used to manage Jobs. */
	typedef AtomicRingBuffer<Job*, MemoryBudgets::Jobs> Queue;
	typedef FixedArray< Queue, (UInt32)Quantum::COUNT > Queues;
	typedef FixedArray< Int64, (UInt32)Quantum::COUNT > Times;

	/**
	 * @return The global singleton instance. Will be nullptr if that instance has not
	 * yet been created.
	 *
	 * Manager deliberately does not inherit from Singleton<> so it can
	 * precisely control assignment of Singleton in a multi-threaded environment.
	 */
	static CheckedPtr<Manager> Get()
	{
		return s_pSingleton;
	}

	Manager();
	~Manager();

	CheckedPtr<Job> GetCurrentThreadJob() const;

	/** @return The total number of general purposes worker threads. */
	UInt32 GetGeneralPurposeWorkerThreadCount() const
	{
		return m_vGeneralPurpose.GetSize();
	}

	/** @return The total number of Manager threads. */
	UInt32 GetThreadCount() const
	{
		return (UInt32)m_PerThreadStorage.GetCount();
	}

	/** @return A unique index to identify the thread - fails if not a Manager thread. */
	Bool GetThreadIndex(UInt32& ru) const
	{
		auto p = m_PerThreadStorage.TryGet();
		if (!p)
		{
			return false;
		}

		ru = (UInt32)p->m_ThreadIndex;
		return true;
	}

	// Called by the application to indicate that the app has entered the background. Use
	// to suspend the Manager worker thread. OnEnterBackground() must be followed by OnLeaveBackground()
	// at the appropriate time.
	void OnEnterBackground();
	void OnLeaveBackground();

	// Schedule a job for execution with the given priority.
	void Schedule(Job& rJob);

	// Allow this thread to be used to execute arbitrary jobs.
	//
	// Returns true if a Job of kDefault or higher priority was run, false otherwise (this method
	// will return false if no Job was run at all, or if the only Job run was at kWaitingForDependency
	// priority).
	Bool YieldThreadTime();

	// Signal the next general purpose thread to wake up. Scheduling
	// is currently round-robin.
	void WakeUpNextGeneralPurpose();

	// Signal all threads to wake up and continue processing.
	void WakeUpAll();

private:
	// Called by Job, to scope a WaitUntilJobIsNotRunning() call. Used to prevent
	// deadlocks if a wait occurs on a Job when the app has been placed into the background.
	friend class Job;
	void JobFriend_BeginWaitUntilJobIsNotRunning();
	void JobFriend_EndWaitUntilJobIsNotRunning();

	struct PerThreadData SEOUL_SEALED
	{
		explicit PerThreadData(Atomic32Type index);
		~PerThreadData();

		Atomic32Type const m_ThreadIndex;
		Times m_aTimes;
		Queues m_aQueues;
		Signal m_Signal;
		ThreadId m_ThreadId;
		UnsafeHandle m_hThreadCoroutine;
		CheckedPtr<RunnerCoroutine> m_pLastJobRunnerCoroutine;
		Atomic32Value<Bool> m_bWaiting;
#if SEOUL_PLATFORM_IOS
		void* m_pAutoReleasePool;
		Int32 m_iAutoReleasePoolReferenceCount;
#endif
		// Configuration - both should be treated as const,
		// but can't be due to how PerThread values are instantiated.
		Bool m_bLastJobWasThreadSpecific;
		Bool m_bThreadSpecificJobsOnly;

	private:
		SEOUL_DISABLE_COPY(PerThreadData);
	}; // struct PerThreadData

	Queue m_GeneralQueue;

	typedef AtomicRingBuffer<RunnerCoroutine*, MemoryBudgets::Jobs> JobRunnerCoroutines;
	JobRunnerCoroutines m_JobRunnerCoroutines;

	typedef Vector<Thread*, MemoryBudgets::Jobs> Threads;
	Threads m_vThreads;

	typedef HeapAllocatedPerThreadStorage<PerThreadData, 256, MemoryBudgets::Jobs> PerThreadStorage;
	PerThreadStorage m_PerThreadStorage;

	typedef Vector<PerThreadData*, MemoryBudgets::Jobs> GeneralPurpose;
	GeneralPurpose m_vGeneralPurpose;
	Atomic32 m_nNextGeneralPurpose;

	Atomic32 m_nExecutingJobs;
	Atomic32 m_nRunningThreads;
	Atomic32 m_nWaitUntilJobIsNotRunningCount;
	Atomic32Value<Bool> m_bShuttingDown;
	Atomic32Value<Bool> m_bInBackground;

	/**
	 * @return The per-thread data associated with thread threadId.
	 */
	CheckedPtr<PerThreadData> GetPerThreadData(const ThreadId& threadId)
	{
		Atomic32Type const nCount = m_PerThreadStorage.GetCount();
		const PerThreadStorage::Objects& objects = m_PerThreadStorage.GetAllObjects();
		for (Atomic32Type i = 0; i < nCount; ++i)
		{
			if (objects[i]->m_ThreadId == threadId)
			{
				return objects[i];
			}
		}

		return nullptr;
	}

	// Setup and teardown of worker threads.
	void InitializeThreads();
	void ShutdownThreads();
	Int FileIOThreadMain(const Thread& thread);
	Int RenderThreadMain(const Thread& thread);
	Int ThreadMain(const Thread& thread);

	// Attempt to run a job in rQueue.
	Bool ExecuteJob(Queue& rQueue);

	// Attempt to run a job in one of the queues of PerThreadData,
	// applies a basic scheduling algorithm.
	Bool ExecuteJob(PerThreadData& rData, Quantum& reMinQuantumRun);
	Bool ExecuteJob(PerThreadData& rData, Quantum eQuantum, Int64 iCurrentTimeInTicks, Quantum& reMinQuantumRun);

	// Check our periodic quantum queues (k1ms, k4ms, k8ms, and k16ms) for work, and use the shortest as the wake up time.
	Bool GetWakeUpTimeInMs(PerThreadData& rData, UInt32& ruTimeInMs) const;

	static CheckedPtr<Manager> s_pSingleton;

	/**
	 * Main entry point for a RunnerCoroutine - this simply
	 * binds the coroutine into the CoroutineMain() member function of Manager.
	 */
	static void SEOUL_STD_CALL CoroutineMainEntry(void* pUserData)
	{
		Manager::Get()->CoroutineMain(reinterpret_cast<RunnerCoroutine*>(pUserData));
	}

	friend struct RunnerCoroutine;
	void AfterContextSwitch();
	void CoroutineMain(CheckedPtr<RunnerCoroutine> pJobRunnerCoroutine);

	SEOUL_DISABLE_COPY(Manager);
}; // class Manager

} // namespace Seoul::Jobs

#endif // include guard
