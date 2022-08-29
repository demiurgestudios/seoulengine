/**
 * \file AndroidMainThreadQueue.cpp
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

#include "AndroidMainThreadQueue.h"
#include "Mutex.h"
#include "ThreadId.h"
#include "Vector.h"

namespace Seoul
{

/** Mutex to protect s_JobQueue */
static Mutex s_JobQueueMutex;

/** Queue of jobs to run on the main thread */
static Vector<AndroidMainThreadJob*> s_JobQueue;

/**
 * Queues a job to run on the main thread, if this thread is not the main
 * thread, or runs it directly if this is the main thread.
 */
void RunOnMainThread(AndroidMainThreadJob* pJob)
{
	if (IsMainThread())
	{
		pJob->Run();
		SEOUL_DELETE pJob;
	}
	else
	{
		Lock lock(s_JobQueueMutex);
		s_JobQueue.PushBack(pJob);
	}
}

/**
 * Runs all currently queued main thread jobs and clears ou the job queue
 */
void RunMainThreadJobs()
{
	SEOUL_ASSERT(IsMainThread());

	Lock lock(s_JobQueueMutex);

	for (UInt i = 0; i < s_JobQueue.GetSize(); i++)
	{
		s_JobQueue[i]->Run();
	}

	SafeDeleteVector(s_JobQueue);
}

} // namespace Seoul
