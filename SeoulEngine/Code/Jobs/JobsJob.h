/**
 * \file JobsJob.h
 * \brief Base class for any object which can be executed in the Jobs::Manager,
 * meaning, it has work that can be done concurrently with other work.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef JOBS_JOB_H
#define JOBS_JOB_H

#include "AtomicPointer.h"
#include "CheckedPtr.h"
#include "MemoryBarrier.h"
#include "Pair.h"
#include "Prereqs.h"
#include "SharedPtr.h"
#include "ThreadId.h"
namespace Seoul { namespace Jobs { class Manager; } }
namespace Seoul { namespace Jobs { struct RunnerCoroutine; } }

namespace Seoul::Jobs
{

/**
 * Enum to hint at the execution scheduling of a Job.
 */
enum class Quantum
{
	/**
	 * Job must be scheduled for immediate execution. Takes priority
	 * over kDefault and other jobs at higher quantums, but will not completely
	 * starve other jobs.
	 */
	kTimeCritical,

	/**
	 * At default priority, Jobs are run immediately in round-robin
	 * fashion.
	 */
	kDefault,

	/**
	 * Jobs at this quantum will be scheduled in 1 ms intervals.
	 * They may run more frequently (if no jobs at default priority are available)
	 * or they may run less frequently (if another job takes a very long time).
	 */
	k1ms,

	/** Equivalent to k1ms, but at a quantum of 4 ms. */
	k4ms,

	/** Equivalent to k4ms, but at a quantum of 8 ms. */
	k8ms,

	/** Equivalent to k8ms, but at a quantum of 16 ms. */
	k16ms,

	/** Equivalent to k16ms, but at a quantum of 32 ms. */
	k32ms,

	/** Special value, number of quantums. Not to be used as an quantum itself. */
	COUNT,

	/** Convenience enum, we use a 1 ms quantum for "sleeping" jobs that are waiting on another job. */
	kWaitingForDependency = k1ms,

	/** Convenience enum, quantum to use for jobs tied to display refresh. */
	kDisplayRefreshPeriodic = k4ms, // TODO: Derive from display instead?

	/** Special value, equal to the min and max quantums. Not meant to be used as a quantum itself. */
	MIN_QUANTUM = kTimeCritical,
	MAX_QUANTUM = k32ms,
}; // enum class Quantum

/**
 * Enum states that indicate the current state of
 * a Job.
 */
enum class State
{
	/**
	 * The Job has not yet been started and is ready to start.
	 */
	kNotStarted = 0,

	/**
	 * The Job is either scheduled to be running, or is actively
	 * running, in the Jobs::Manager.
	 */
	kScheduledForOrRunning,

	/**
	 * The Job was started and successfully completed.
	 *
	 * Job::ResetJob(), or Job::StartJob() with a bForceStart argument
	 * of true, must be called on the Job before it can be run again.
	 */
	kComplete,

	/**
	 * The Job was started but encountered an error. The Job
	 * cannot be run again unless the specialized class that inherits
	 * from Job exposes a mechanism to place the Job back in a functional
	 * state.
	 */
	kError
}; // enum class State

/**
 * Common base class for any object which has work to be
 * executed concurrently to other work in the Jobs::Manager.
 *
 * Some Jobs are expected to be multi-stage execution Jobs,
 * where each stage may execute on a different thread. For example,
 * a Job to load a Texture might first execute on the Jobs::Manager thread
 * (to lock a texture buffer), then on a worker thread (to read texture
 * data from a file), then on the Jobs::Manager thread (to unlock the texture
 * buffer). These sorts of Jobs can be handled by tracking the current Job
 * stage internally in the Job specialization, and returning the
 * appropriate State from InternalExecuteJob() and increment the stage
 * counter with each call to InternalExecuteJob(), until the Job completes,
 * at which point InternalExecuteJob() should return kComplete.
 *
 * \warning Any specialization of this class *must* call WaitUntilDone()
 * in its destructor. ~Job() asserts that the Job IsDone() before
 * destruction.
 *
 * \warning It is up to specializations of Job to guarantee that the
 * work that they do in InternalExecuteJob() is thread-safe - the only
 * assumption that a job can make about the execution context when it is
 * running is that it will only run on the thread specified by its
 * associated ThreadId.
 */
class Job SEOUL_ABSTRACT
{
public:
	/**
	 * Associated thread of this Job - if any value besides
	 * ThreadId() (invalid), the Jobs::Manager will schedule this Job
	 * to run on that thread, unless the thread is not registered
	 * with the Jobs::Manager. If ThreadId is invalid, it will run
	 * on the first available worker thread.
	 */
	const ThreadId& GetThreadId() const
	{
		return m_ThreadId;
	}

	/**
	 * Gets the current state of this Job. The state indicates
	 * whether this Job is executing and on what thread, or whether
	 * this Job is complete, or in an error state.
	 */
	State GetJobState() const
	{
		return m_eJobState;
	}

	/**
	 * Helper method, returns true if eJobState is in a state
	 * in which a Job is considered running, false otherwise. A Job
	 * can only be safely destroyed if it is not running.
	 */
	static Bool IsJobRunningState(State eJobState)
	{
		return (
			State::kNotStarted != eJobState &&
			State::kComplete != eJobState &&
			State::kError != eJobState);
	}

	/**
	 * Returns true if this Job is running, false otherwise.
	 * This job is not running if it is in the kComplete, kNotStarted, or
	 * kError state.
	 */
	Bool IsJobRunning() const
	{
		return IsJobRunningState(GetJobState());
	}

	/**
	 * Returns true if this Job is in any state besides
	 * kNotStarted, false otherwise.
	 */
	Bool WasJobStarted() const
	{
		return (State::kNotStarted != GetJobState());
	}

	/**
	 * Puts this Job back into its default state so that it can be
	 * executed again.
	 *
	 * This method will *not* reset this Job if it is in
	 * the kError state.
	 *-
	 * \warning This method will block the calling thread until this Job
	 * reaches a state within which IsDone() returns true.
	 */
	void ResetJob()
	{
		// Don't reset if this Job is in the error state.
		if (WasJobStarted() && State::kError != GetJobState())
		{
			WaitUntilJobIsNotRunning();
			m_eJobState = State::kNotStarted;
		}
	}

	/** @return The scheduling hint for the job. */
	Quantum GetJobQuantum() const
	{
		return m_eJobQuantum;
	}

	/** Update the scheduling hint for this Job. */
	void SetJobQuantum(Quantum eQuantum)
	{
		m_eJobQuantum = eQuantum;
	}

	void StartJob(Bool bForceStart = true);
	void WaitUntilJobIsNotRunning();

protected:
	SEOUL_REFERENCE_COUNTED(Job);

	/**
	 * Job constructor, can only be called by specialized classes
	 * of Job.
	 *
	 * @param[in] threadId If defined to a valid id, this Job will
	 * be scheduled to run on the associated worker thread, otherwise it
	 * will be scheduled to run on the first available thread.
	 * @param[in] eQuantum The initial execution quantum of this Job.
	 * Can be modified at any time with a call to SetJobQuantum(), but the
	 * change will not take effect until the next time the Job is scheduled.
	 */
	Job(ThreadId threadId = ThreadId(), Quantum eQuantum = Quantum::kDefault)
		: m_eJobQuantum(eQuantum)
		, m_eJobState(State::kNotStarted)
		, m_ThreadId(threadId)
		, m_pRunnerCoroutine()
	{
	}

	/**
	 * Job's destructor validates that the job is done before
	 * the destructor is called. It is up to class specializations to
	 * enforce this with a call to WaitUntilDone()
	 */
	virtual ~Job()
	{
		// NOTE: If you hit this assert, it means your Job subclass
		// needs to call WaitUntilJobIsNotRunning() in the subclass
		// destructor.
		SEOUL_ASSERT(!IsJobRunning());
	}

	virtual void InternalExecuteJob(State& reNextState, ThreadId& rNextThreadId) = 0;

	// Friend function for Jobs::Manager. The Jobs::Manager
	// should be the only object to call InternalExecuteJob() on
	// this Job.
	//
	// NOTE: Polymorphic to allow for special cases (currently, to
	// allow job completion to be signaled before a final callback
	// is invoked). You should almost never do this - in general,
	// override InternalExecuteJob() instead for standard handling.
	virtual void Friend_JobManagerExecute();
	Pair<State, ThreadId> PreExecute();
	void PostExecute(State eNextState, ThreadId nextThreadId);

private:
	Atomic32Value<Quantum> m_eJobQuantum;
	Atomic32Value<State> m_eJobState;
	ThreadId m_ThreadId;

	friend class Manager;

	// NOTE: This member is manipulated and used only by Jobs::Manager.
	AtomicPointer<RunnerCoroutine> m_pRunnerCoroutine;

	SEOUL_DISABLE_COPY(Job);
}; // class Job

/** Utility to temporarily change the quantum of a Job. */
class ScopedQuantum SEOUL_SEALED
{
public:
	ScopedQuantum(Job& r, Quantum eDesired)
		: m_r(r)
		, m_eOriginal(r.GetJobQuantum())
		, m_eDesired(eDesired)
	{
		m_r.SetJobQuantum(m_eDesired);
	}

	~ScopedQuantum()
	{
		if (m_r.GetJobQuantum() == m_eDesired)
		{
			m_r.SetJobQuantum(m_eOriginal);
		}
	}

private:
	Job& m_r;
	Quantum const m_eOriginal;
	Quantum const m_eDesired;

	SEOUL_DISABLE_COPY(ScopedQuantum);
}; // class ScopedQuantum

} // namespace Seoul::Jobs

#endif // include guard
