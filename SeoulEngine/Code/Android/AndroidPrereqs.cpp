/**
 * \file AndroidPrereqs.cpp
 * \brief Utilities and headers necessary for working with JNI on Android.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "AndroidPrereqs.h"
#include "Coroutine.h"
#include "Logger.h"
#include "Thread.h"

namespace Seoul
{

namespace Java
{

namespace
{

/**
 * Utility - on some versions/configurations
 * of Android (those using ART), we cannot
 * invoke JNI from a coroutine context. To work
 * around this, we transfer binders over to a thread
 * and let it actually execute the JNI routine.
 */
class ThreadRunner SEOUL_SEALED
{
public:
	SEOUL_DELEGATE_TARGET(ThreadRunner);

	ThreadRunner()
		: m_bShutdown(false)
		, m_InSignal()
		, m_InTasks()
		, m_OutSignal()
		, m_OutTasks()
		, m_pThread()
	{
		// Instantiate our thread.
		m_pThread.Reset(SEOUL_NEW(MemoryBudgets::TBD) Thread(SEOUL_BIND_DELEGATE(&ThreadRunner::WorkerBody, this), false));
		m_pThread->Start("JNI ThreadRunner");
	}

	~ThreadRunner()
	{
		// Setup the thread for shutdown and kill it.
		m_bShutdown = true;
		m_InSignal.Activate();
		m_pThread->WaitUntilThreadIsNotRunning();
	}

	/** Invoke the binder in a unique thread context to workaround invoking JNI from coroutines. */
	void Run(InvokerBinder* pBinder)
	{
		// Push the binder into the input queue and then wait for it to complete.
		m_InTasks.Push(pBinder);
		m_InSignal.Activate();
		while (true)
		{
			// Pop the next output binder.
			auto pFinishedBinder = m_OutTasks.Pop();
			
			// Nothing yet, wait for it.
			if (nullptr == pFinishedBinder)
			{
				m_OutSignal.Wait();
				continue;
			}
			// Popped a binder but it wasn't the one we're looking for,
			// push it back on and try again.
			else if (pFinishedBinder != pBinder)
			{
				m_OutTasks.Push(pFinishedBinder);
				continue;
			}

			// Done.
			break;
		}
	}

private:
	SEOUL_DISABLE_COPY(ThreadRunner);

	typedef AtomicRingBuffer<InvokerBinder*, MemoryBudgets::TBD> Tasks;

	Int WorkerBody(const Thread&)
	{
		// Run forever until told to shutdown.
		while (!m_bShutdown)
		{
			// Pop the next input task.
			auto pTask = m_InTasks.Pop();
			if (nullptr == pTask)
			{
				// Nothing to do, wait for a task.
				m_InSignal.Wait();
				continue;
			}

			// Run the task.
			pTask->ThreadDoInvoke();
			
			// Push it onto the output queue and continue on.
			m_OutTasks.Push(pTask);
			m_OutSignal.Activate();
		}

		return 0;
	}

	Atomic32Value<Bool> m_bShutdown;
	Signal m_InSignal;
	Tasks m_InTasks;
	Signal m_OutSignal;
	Tasks m_OutTasks;
	ScopedPtr<Thread> m_pThread;
}; // class ThreadRunner

static Mutex s_RunnerMutex;
static ScopedPtr<ThreadRunner> s_pRunner;

/** Get the runner for executing jobs in a unique thread context. */
static ThreadRunner& GetRunner()
{
	// Lock while acquiring - once the runner exists, it is never released until
	// destruction, so we only need to lock on acquire.
	Lock lock(s_RunnerMutex);
	if (!s_pRunner.IsValid())
	{
		s_pRunner.Reset(SEOUL_NEW(MemoryBudgets::TBD) ThreadRunner);
	}

	return *s_pRunner;
}

} // namespace anonymous

void InvokerBinder::ThreadDoInvoke()
{
	// Need to update the environment to point at the environment for the current thread.
	CheckedPtr<JNIEnv> pEnvironment = m_pEnvironment;
	m_pEnvironment = Thread::GetThisThreadJNIEnv();

	// Perform the invocation.
	DoInvoke();

	// Reset the environment.
	m_pEnvironment = pEnvironment;
}

void InvokerBinder::PerformInvoke()
{
	// JNI invocation must happen from the origin coroutine (Android ANT
	// uses the same stack as the current thread and is very picky), so
	// we need to dispatch the invocation from a worker thread
	// if we're not in the origin.
	if (!IsInOriginCoroutine())
	{
		// Get the thread runner then use it to run the binder.
		auto& r = GetRunner();
		r.Run(this);
	}
	else
	{
		DoInvoke();
	}
}

}  // namespace Java

} // namespace Seoul
