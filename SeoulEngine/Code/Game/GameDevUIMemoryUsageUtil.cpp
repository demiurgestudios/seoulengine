/**
 * \file GameDevUIMemoryUsageUtil.cpp
 * \brief Wraps asynchronous process memory queries.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "GameDevUIMemoryUsageUtil.h"
#include "Engine.h"
#include "Thread.h"

#if SEOUL_ENABLE_DEV_UI

namespace Seoul::Game
{

DevUIMemoryUsageUtil::DevUIMemoryUsageUtil()
	: m_pWorker()
	, m_WorkerSignal()
	, m_uLastMemoryUsageWorking(0u)
	, m_uLastMemoryUsagePrivate(0u)
	, m_bWorkerRunning(true)
{
	m_pWorker.Reset(SEOUL_NEW(MemoryBudgets::DevUI) Thread(
		SEOUL_BIND_DELEGATE(&DevUIMemoryUsageUtil::WorkerThread, this)));
	m_pWorker->Start("DevUI Memory Util");
}

DevUIMemoryUsageUtil::~DevUIMemoryUsageUtil()
{
	m_bWorkerRunning = false;
	m_WorkerSignal.Activate();
	SeoulMemoryBarrier();
	m_pWorker.Reset();
}

Int DevUIMemoryUsageUtil::WorkerThread(const Thread&)
{
	UInt32 uSleepTimeInMilliseconds = 1000u;

	while (m_bWorkerRunning)
	{
		size_t zWorking = 0u, zPrivate = 0u;
		Int64 const iBegin = SeoulTime::GetGameTimeInTicks();
		Bool const bSuccess = Engine::Get()->QueryProcessMemoryUsage(zWorking, zPrivate);
		Int64 const iEnd = SeoulTime::GetGameTimeInTicks();

		if (bSuccess)
		{
			m_uLastMemoryUsageWorking = (UInt64)zWorking;
			m_uLastMemoryUsagePrivate = (UInt64)zPrivate;

			Double const fTimeInMilliseconds = SeoulTime::ConvertTicksToMilliseconds(iEnd - iBegin);
			if (fTimeInMilliseconds / (Double)uSleepTimeInMilliseconds > 0.25)
			{
				uSleepTimeInMilliseconds = Min(5000u, uSleepTimeInMilliseconds * 2u);
			}
		}
		m_WorkerSignal.Wait(uSleepTimeInMilliseconds);
	}

	return 0;
}

} // namespace Seoul::Game

#endif // /SEOUL_ENABLE_DEV_UI
