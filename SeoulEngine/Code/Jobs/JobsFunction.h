/**
 * \file JobsFunction.h
 * \brief Convenience API, allows generic functions (functions, functors,
 * lambdas, and delegates) to be invoked as Job in the JobSystem without
 * explicitly defining a subclass of Job.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef JOBS_FUNCTION_H
#define JOBS_FUNCTION_H

#include "CheckedPtr.h"
#include "Delegate.h"
#include "JobsJob.h"
#include "JobsManager.h"
#include "MemoryBarrier.h"
#include "SharedPtr.h"
#include "Thread.h"

namespace Seoul::Jobs
{

template <typename FUNC>
class Function SEOUL_SEALED : public Jobs::Job
{
public:
	Function(ThreadId threadId, FUNC&& func)
		: Job(threadId)
		, m_Func(RvalRef(func))
	{
	}

private:
	virtual ~Function()
	{
		WaitUntilJobIsNotRunning();
	}

	virtual void InternalExecuteJob(State& reNextState, ThreadId& rNextThreadId) SEOUL_OVERRIDE
	{
		(void)m_Func();
		SeoulMemoryBarrier();
		reNextState = State::kComplete;
	}

	const FUNC m_Func;

	SEOUL_DISABLE_COPY(Function);
}; // class Function

/**
 * Create a Job instance wrapper around generic function \a func.
 *
 * @param[in] threadId Thread to run the generic function on.
 * @param[in] func     Generic function to invoke.
 *
 * @return The instantiated Job instance. Will not be started.
 */
template <typename FUNC>
static SharedPtr<Job> MakeFunction(ThreadId threadId, FUNC&& func)
{
	return SharedPtr<Job>(SEOUL_NEW(MemoryBudgets::Jobs) Function< FUNC >(threadId, RvalRef(func)));
}

/**
 * Create a Job instance wrapper around generic function \a func.
 *
 * @param[in] func Generic function to invoke.
 *
 * @return The instantiated Job instance. Will not be started.
 *
 * \note Convenience variation of MakeFunction with unspecified thread id. The Job will run
 * on the next available Job worker thread (an arbitrary worker thread).
 */
template <typename FUNC>
static SharedPtr<Job> MakeFunction(FUNC&& func)
{
	return MakeFunction(ThreadId(), RvalRef(func));
}

/**
 * Create a Job instance wrapper around generic function \a func.
 *
 * @param[in] threadId Thread to run the generic function on.
 * @param[in] func     Generic function to invoke.
 * @param[in] args     1-to-n arguments to bind to and use when invoking the generic function \a func.
 *
 * @return The instantiated Job instance. Will not be started.
 */
template <typename FUNC, typename... ARGS>
static SharedPtr<Job> MakeFunction(ThreadId threadId, FUNC&& func, ARGS... args)
{
	return MakeFunction(
		threadId,
		[func = RvalRef(func), args...]() { (void)func(args...); });
}

/**
 * Create a Job instance wrapper around generic function \a func.
 *
 * @param[in] func Generic function to invoke.
 * @param[in] args 1-to-n arguments to bind to and use when invoking the generic function \a func.
 *
 * @return The instantiated Job instance. Will not be started.
 *
 * \note Convenience variation of MakeFunction with unspecified thread id. The Job will run
 * on the next available Job worker thread (an arbitrary worker thread).
 */
template <typename FUNC, typename... ARGS>
static SharedPtr<Job> MakeFunction(FUNC&& func, ARGS... args)
{
	return MakeFunction(ThreadId(), RvalRef(func), args...);
}

/**
 * Call a 0 argument generic function (function, functor, lambda, or delegate) on
 * a target thread.
 *
 * @param[in] threadId Thread to run the generic function on.
 * @param[in] func     Generic function to invoke.
 *
 * @return The Job object created to run \a func.
 *
 * \note AsyncFunction always enqueues \a func for execution on the target thread, even
 * if the target thread is the current thread. As such, completion of AsyncFunction will
 * never imply completion of the Job. To check Job status, query the returned Job instance.
 */
template <typename FUNC>
static SharedPtr<Job> AsyncFunction(ThreadId threadId, FUNC&& func)
{
	auto const pReturn(MakeFunction(threadId, RvalRef(func)));
	pReturn->StartJob();
	return pReturn;
}

/**
 * Call a 0 argument generic function (function, functor, lambda, or delegate) on
 * an arbitrary worker thread.
 *
 * @param[in] func Generic function to invoke.
 *
 * @return The Job object created to run \a func.
 *
 * \note Convenience variation of AsyncFunction without an explicit thread target. The
 * Job will be enqueued for execution on the next available Job worker thread.
 */
template <typename FUNC>
static SharedPtr<Job> AsyncFunction(FUNC&& func)
{
	return AsyncFunction(ThreadId(), RvalRef(func));
}

/**
 * Call a 1-to-n argument generic function (function, functor, lambda, or delegate) on
 * a target thread.
 *
 * @param[in] threadId Thread to run the generic function on.
 * @param[in] func     Generic function to invoke.
 * @param[in] args     1-to-n arguments to the generic function.
 *
 * @return The Job object created to run \a func.
 *
 * \note AsyncFunction always enqueues \a func for execution on the target thread, even
 * if the target thread is the current thread. As such, completion of AsyncFunction will
 * never imply completion of the Job. To check Job status, query the returned Job instance.
 */
template <typename FUNC, typename... ARGS>
static SharedPtr<Job> AsyncFunction(ThreadId threadId, FUNC&& func, ARGS... args)
{
	auto const pReturn(MakeFunction(threadId, RvalRef(func), args...));
	pReturn->StartJob();
	return pReturn;
}

/**
 * Call a 1-to-n argument generic function (function, functor, lambda, or delegate) on
 * an arbitrary worker thread.
 *
 * @param[in] func Generic function to invoke.
 * @param[in] args 1-to-n arguments to the generic function.
 *
 * @return The Job object created to run \a func.
 *
 * \note Convenience variation of AsyncFunction without an explicit thread target. The
 * Job will be enqueued for execution on the next available Job worker thread.
 */
template <typename FUNC, typename... ARGS>
static SharedPtr<Job> AsyncFunction(FUNC&& func, ARGS... args)
{
	return AsyncFunction(ThreadId(), RvalRef(func), args...);
}

/**
 * Call a 0 argument generic function (function, functor, lambda, or delegate) on
 * a target thread. Waits for completion of the function.
 *
 * @param[in] threadId Thread to run the generic function on.
 * @param[in] func     Generic function to invoke.
 *
 * @return The Job object created to run \a func.
 *
 * \note AwaitFunction always waits for completion of the created Job. As such,
 * when AwaitFunction returns, the returned Job instance is guaranteed to be either in the
 * kComplete or kError state.
 */
template <typename FUNC>
static SharedPtr<Job> AwaitFunction(ThreadId threadId, FUNC&& func)
{
	auto const pReturn(AsyncFunction(threadId, RvalRef(func)));
	pReturn->WaitUntilJobIsNotRunning();
	return pReturn;
}

/**
 * Call a 0 argument generic function (function, functor, lambda, or delegate) on
 * an arbitrary worker thread. Waits for completion of the function.
 *
 * @param[in] func Generic function to invoke.
 *
 * @return The Job object created to run \a func.
 *
 * \note Convenience variation of AwaitFunction without an explicit thread target. The
 * Job will be enqueued for execution on the next available Job worker thread.
 *
 * \note AwaitFunction always waits for completion of the created Job. As such,
 * when AwaitFunction returns, the returned Job instance is guaranteed to be either in the
 * kComplete or kError state.
 */
template <typename FUNC>
static SharedPtr<Job> AwaitFunction(FUNC&& func)
{
	return AwaitFunction(ThreadId(), RvalRef(func));
}

/**
 * Call a 1-to-n argument generic function (function, functor, lambda, or delegate) on
 * a target thread. Waits for completion of the function.
 *
 * @param[in] threadId Thread to run the generic function on.
 * @param[in] func     Generic function to invoke.
 * @param[in] args     1-to-n arguments to the generic function.
 *
 * @return The Job object created to run \a func.
 *
 * \note AwaitFunction always waits for completion of the created Job. As such,
 * when AwaitFunction returns, the returned Job instance is guaranteed to be either in the
 * kComplete or kError state.
 */
template <typename FUNC, typename... ARGS>
static SharedPtr<Job> AwaitFunction(ThreadId threadId, FUNC&& func, ARGS... args)
{
	auto const pReturn(AsyncFunction(threadId, RvalRef(func), args...));
	pReturn->WaitUntilJobIsNotRunning();
	return pReturn;
}

/**
 * Call a 1-to-n argument generic function (function, functor, lambda, or delegate) on
 * an arbitrary worker thread. Waits for completion of the function.
 *
 * @param[in] func Generic function to invoke.
 * @param[in] args 1-to-n arguments to the generic function.
 *
 * @return The Job object created to run \a func.
 *
 * \note Convenience variation of AwaitFunction without an explicit thread target. The
 * Job will be enqueued for execution on the next available Job worker thread.
 *
 * \note AwaitFunction always waits for completion of the created Job. As such,
 * when AwaitFunction returns, the returned Job instance is guaranteed to be either in the
 * kComplete or kError state.
 */
template <typename FUNC, typename... ARGS>
static SharedPtr<Job> AwaitFunction(FUNC&& func, ARGS... args)
{
	return AwaitFunction(ThreadId(), RvalRef(func), args...);
}

} // namespace Seoul::Jobs

#endif // include guard
