/**
 * \file AndroidMainThreadQueue.h
 * \brief Specialized Jobs::Manager style queue for executing
 * tasks on the main thread for Android only. Used to cover
 * cases where the general purpose Jobs::Manager does not
 * exists.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ANDROID_MAIN_THREAD_QUEUE_H
#define ANDROID_MAIN_THREAD_QUEUE_H

#include "Prereqs.h"

namespace Seoul
{

/**
 * Helper class for running jobs on the main thread, usable at all times, even
 * if the Jobs::Manager hasn't been initialized yet.
 */
class AndroidMainThreadJob SEOUL_ABSTRACT
{
public:
	virtual ~AndroidMainThreadJob() {}

	/** Runs the actual job task */
	virtual void Run() = 0;
};

/**
 * Helper class for running a 0-argument function on the main thread
 */
class AndroidMainThreadJob0Args : public AndroidMainThreadJob
{
public:
	AndroidMainThreadJob0Args(void (*pFunction)())
		: m_pFunction(pFunction)
	{
	}

	virtual void Run() SEOUL_OVERRIDE
	{
		m_pFunction();
	}

private:
	void (*m_pFunction)();
};

/**
 * Helper class for running a 1-argument function on the main thread
 */
template <typename A1>
class AndroidMainThreadJob1Arg : public AndroidMainThreadJob
{
public:
	AndroidMainThreadJob1Arg(void (*pFunction)(A1), const A1& arg1)
		: m_pFunction(pFunction)
		, m_Arg1(arg1)
	{
	}

	virtual void Run() SEOUL_OVERRIDE
	{
		m_pFunction(m_Arg1);
	}

private:
	void (*m_pFunction)(A1);

	A1 m_Arg1;
};

/**
 * Helper class for running a 2-argument function on the main thread
 */
template <typename A1, typename A2>
class AndroidMainThreadJob2Args : public AndroidMainThreadJob
{
public:
	AndroidMainThreadJob2Args(void (*pFunction)(A1, A2), const A1& arg1, const A2& arg2)
		: m_pFunction(pFunction)
		, m_Arg1(arg1)
		, m_Arg2(arg2)
	{
	}

	virtual void Run() SEOUL_OVERRIDE
	{
		m_pFunction(m_Arg1, m_Arg2);
	}

private:
	void (*m_pFunction)(A1, A2);

	A1 m_Arg1;
	A2 m_Arg2;
};

/**
 * Helper class for running a 3-argument function on the main thread
 */
template <typename A1, typename A2, typename A3>
class AndroidMainThreadJob3Args : public AndroidMainThreadJob
{
public:
	AndroidMainThreadJob3Args(void (*pFunction)(A1, A2, A3), const A1& arg1, const A2& arg2, const A3& arg3)
		: m_pFunction(pFunction)
		, m_Arg1(arg1)
		, m_Arg2(arg2)
		, m_Arg3(arg3)
	{
	}

	virtual void Run() SEOUL_OVERRIDE
	{
		m_pFunction(m_Arg1, m_Arg2, m_Arg3);
	}

private:
	void (*m_pFunction)(A1, A2, A3);

	A1 m_Arg1;
	A2 m_Arg2;
	A3 m_Arg3;
};

/**
 * Helper class for running a 4-argument function on the main thread
 */
template <typename A1, typename A2, typename A3, typename A4>
class AndroidMainThreadJob4Args : public AndroidMainThreadJob
{
public:
	AndroidMainThreadJob4Args(void (*pFunction)(A1, A2, A3, A4), const A1& arg1, const A2& arg2, const A3& arg3, const A4& arg4)
		: m_pFunction(pFunction)
		, m_Arg1(arg1)
		, m_Arg2(arg2)
		, m_Arg3(arg3)
		, m_Arg4(arg4)
	{
	}

	virtual void Run() SEOUL_OVERRIDE
	{
		m_pFunction(m_Arg1, m_Arg2, m_Arg3, m_Arg4);
	}

private:
	void (*m_pFunction)(A1, A2, A3, A4);

	A1 m_Arg1;
	A2 m_Arg2;
	A3 m_Arg3;
	A4 m_Arg4;
};

/**
 * Helper class for running a 5-argument function on the main thread
 */
template <typename A1, typename A2, typename A3, typename A4, typename A5>
class AndroidMainThreadJob5Args : public AndroidMainThreadJob
{
public:
	AndroidMainThreadJob5Args(void (*pFunction)(A1, A2, A3, A4, A5), const A1& arg1, const A2& arg2, const A3& arg3, const A4& arg4, const A5& arg5)
		: m_pFunction(pFunction)
		, m_Arg1(arg1)
		, m_Arg2(arg2)
		, m_Arg3(arg3)
		, m_Arg4(arg4)
		, m_Arg5(arg5)
	{
	}

	virtual void Run() SEOUL_OVERRIDE
	{
		m_pFunction(m_Arg1, m_Arg2, m_Arg3, m_Arg4, m_Arg5);
	}

private:
	void (*m_pFunction)(A1, A2, A3, A4, A5);

	A1 m_Arg1;
	A2 m_Arg2;
	A3 m_Arg3;
	A4 m_Arg4;
	A5 m_Arg5;
};

// Queues a job to run on the main thread, if this thread is not the main
// thread, or runs it directly if this is the main thread.
void RunOnMainThread(AndroidMainThreadJob *pJob);

/**
 * Queues a 0-argument function to be called on the main thread
 */
inline static void RunOnMainThread(void (*pFunction)())
{
	RunOnMainThread(
		SEOUL_NEW(MemoryBudgets::TBD) AndroidMainThreadJob0Args(pFunction));
}

/**
 * Queues a 1-argument function to be called on the main thread
 */
template <typename A1>
inline static void RunOnMainThread(void (*pFunction)(A1), const A1& arg1)
{
	RunOnMainThread(
		SEOUL_NEW(MemoryBudgets::TBD) AndroidMainThreadJob1Arg<A1>(pFunction, arg1));
}

/**
 * Queues a 2-argument function to be called on the main thread
 */
template <typename A1, typename A2>
inline static void RunOnMainThread(void (*pFunction)(A1, A2), const A1& arg1, const A2& arg2)
{
	RunOnMainThread(
		SEOUL_NEW(MemoryBudgets::TBD) AndroidMainThreadJob2Args<A1, A2>(pFunction, arg1, arg2));
}

/**
 * Queues a 3-argument function to be called on the main thread
 */
template <typename A1, typename A2, typename A3>
inline static void RunOnMainThread(void (*pFunction)(A1, A2, A3), const A1& arg1, const A2& arg2, const A3& arg3)
{
	RunOnMainThread(
		SEOUL_NEW(MemoryBudgets::TBD) AndroidMainThreadJob3Args<A1, A2, A3>(pFunction, arg1, arg2, arg3));
}

/**
 * Queues a 4-argument function to be called on the main thread
 */
template <typename A1, typename A2, typename A3, typename A4>
inline static void RunOnMainThread(void (*pFunction)(A1, A2, A3, A4), const A1& arg1, const A2& arg2, const A3& arg3, const A4& arg4)
{
	RunOnMainThread(
		SEOUL_NEW(MemoryBudgets::TBD) AndroidMainThreadJob4Args<A1, A2, A3, A4>(pFunction, arg1, arg2, arg3, arg4));
}

/**
 * Queues a 5-argument function to be called on the main thread
 */
template <typename A1, typename A2, typename A3, typename A4, typename A5>
inline static void RunOnMainThread(void (*pFunction)(A1, A2, A3, A4, A5), const A1& arg1, const A2& arg2, const A3& arg3, const A4& arg4, const A5& arg5)
{
	RunOnMainThread(
		SEOUL_NEW(MemoryBudgets::TBD) AndroidMainThreadJob5Args<A1, A2, A3, A4, A5>(pFunction, arg1, arg2, arg3, arg4, arg5));
}

// Runs all currently queued main thread jobs and clears ou the job queue.
// MUST be called from the main thread (IsMainThread() == true)
void RunMainThreadJobs();

} // namespace Seoul

#endif // include guard
