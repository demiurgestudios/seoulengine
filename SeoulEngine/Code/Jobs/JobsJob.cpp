/**
 * \file JobsJob.cpp
 * \brief Base class for any object which can be executed in the Jobs::Manager,
 * meaning, it has work that can be done concurrently with other work.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "JobsJob.h"
#include "JobsManager.h"
#include "ReflectionDefine.h"

namespace Seoul
{

SEOUL_BEGIN_ENUM(Jobs::Quantum)
	SEOUL_ENUM_N("TimeCritical", Jobs::Quantum::kTimeCritical)
	SEOUL_ENUM_N("Default", Jobs::Quantum::kDefault)
	SEOUL_ENUM_N("1ms", Jobs::Quantum::k1ms)
	SEOUL_ENUM_N("4ms", Jobs::Quantum::k4ms)
	SEOUL_ENUM_N("8ms", Jobs::Quantum::k8ms)
	SEOUL_ENUM_N("16ms", Jobs::Quantum::k16ms)
	SEOUL_ENUM_N("32ms", Jobs::Quantum::k32ms)
	SEOUL_ENUM_N("WaitingForDependency", Jobs::Quantum::kWaitingForDependency)
	SEOUL_ENUM_N("DisplayRefreshPeriodic", Jobs::Quantum::kDisplayRefreshPeriodic)
SEOUL_END_ENUM()

SEOUL_BEGIN_ENUM(Jobs::State)
	SEOUL_ENUM_N("NotStarted", Jobs::State::kNotStarted)
	SEOUL_ENUM_N("ScheduledForOrRunning", Jobs::State::kScheduledForOrRunning)
	SEOUL_ENUM_N("Complete", Jobs::State::kComplete)
	SEOUL_ENUM_N("Error", Jobs::State::kError)
SEOUL_END_ENUM()

namespace Jobs
{

/**
 * Starts this Job, adding it to the Jobs::Manager for execution.
 *
 * @param[in] bForceStart If necessary, block and restart the Job.
 *
 * If bForceStart is true and the Job is currently running,
 * this method will block until the Job is complete. It will then reset
 * the job and start it again. If bForceStart is false, the Job will
 * only be run if it is currently in the kNotStarted state. Calling
 * Start() with bForceStart = false is therefore a viable way to
 * start a job only if it has not already been run.
 *
 * If this Job is in the kError state, ResetJob() will not
 * restart the Job and the Job will not run, even if bForceStart
 * is true.
 */
void Job::StartJob(Bool bForceStart /*= true*/)
{
	// Reset the job if bForceStart is true.
	if (bForceStart)
	{
		ResetJob();
	}

	// If we're in the kNotStarted state, run this Job.
	if (!WasJobStarted())
	{
		m_eJobState = State::kScheduledForOrRunning;
		SEOUL_ASSERT(Jobs::Manager::Get());
		Jobs::Manager::Get()->Schedule(*this);
	}
}

/**
 * This method blocks until the IsJobRunning() method of
 * this Job returns false.
 *
 * \warning It is not guaranteed that this Job was actually
 * executed after this method returns, only that IsJobRunning() is false.
 * This method may return without running this Job when the Jobs::Manager
 * is shutting down (in which case the Jobs::Manager stops accepting
 * new Jobs). It may also return without running if this Job
 * is in the kError state.
 */
void Job::WaitUntilJobIsNotRunning()
{
	// Wait if the Job is currently running.
	if (IsJobRunning())
	{
		// Tell Jobs::Manager we're waiting.
		Jobs::Manager::Get()->JobFriend_BeginWaitUntilJobIsNotRunning();

		// Cache the initial priority.
		auto const eStartingQuantum = GetJobQuantum();

		// Switch to the highest priority.
		SetJobQuantum(Quantum::kTimeCritical);

		// Wait for the job to complete.
		while (IsJobRunning())
		{
			// Give the job time to run.
			Jobs::Manager::Get()->YieldThreadTime();
		}

		// If the Job priority has not changed since we set it to time critical,
		// switch back to the initial priority before we raised the priority of the job.
		if (Quantum::kTimeCritical == GetJobQuantum())
		{
			SetJobQuantum(eStartingQuantum);
		}

		// Done waiting.
		Jobs::Manager::Get()->JobFriend_EndWaitUntilJobIsNotRunning();
	}
}

/**
 * Friend function for Jobs::Manager. The Jobs::Manager
 * should be the only object to call InternalExecuteJob() on
 * this Job.
 */
void Job::Friend_JobManagerExecute()
{
	auto const pair = PreExecute();
	PostExecute(pair.First, pair.Second);
}

Pair<State, ThreadId> Job::PreExecute()
{
	State eNextState = m_eJobState;
	ThreadId nextThreadId = m_ThreadId;

	// IMPORTANT: Must set to local variables here, so the Job does not attempt
	// to exit out prematurely.
	InternalExecuteJob(eNextState, nextThreadId);

	// Jobs are not allowed to return kNotStarted as their new state
	// from InternalExecuteJob(), this would result in a Job executing
	// indefinitely and could result in infinite loops on calls
	// to WaitUntilJobIsNotRunning() and other methods that call this method.
	//
	// If a Job needs to execute continuously, it is up to client
	// code to call Start() periodically to rerun the job.
	SEOUL_ASSERT(State::kNotStarted != eNextState);

	return MakePair(eNextState, nextThreadId);
}

void Job::PostExecute(State eNextState, ThreadId nextThreadId)
{
	// IMPORTANT: State must be set last, as it is used to determine if the Job is finished running
	// and is the trigger to allow another thread to destroy this Job.
	m_ThreadId = nextThreadId;
	SeoulMemoryBarrier();
	m_eJobState = eNextState;
}

} // namespace Jobs

} // namespace Seoul
