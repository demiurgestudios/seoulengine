/**
 * \file GameDevUIMemoryUsageUtil.h
 * \brief Wraps asynchronous process memory queries.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef GAME_DEV_UI_MEMORY_USAGE_UTIL_H
#define GAME_DEV_UI_MEMORY_USAGE_UTIL_H

#include "Atomic64.h"
#include "Delegate.h"
#include "Prereqs.h"
#include "ScopedPtr.h"
#include "SeoulSignal.h"
namespace Seoul { class Thread; }

#if SEOUL_ENABLE_DEV_UI

namespace Seoul::Game
{

class DevUIMemoryUsageUtil SEOUL_SEALED
{
public:
	SEOUL_DELEGATE_TARGET(DevUIMemoryUsageUtil);

	DevUIMemoryUsageUtil();
	~DevUIMemoryUsageUtil();

	/**
	 * On supported platforms, the "working set" is
	 * the memory that is reserved for the process
	 * and cannot be paged out.
	 *
	 * On platforms without a page file, this
	 * value will be equal to the private set.
	 */
	size_t GetLastMemoryUsageWorkingSample() const
	{
		return (size_t)m_uLastMemoryUsageWorking;
	}

	/**
	 * On supported platforms, the "private set" is
	 * the memory that is reserved for the process.
	 *
	 * Some of this memory may be paged out if not
	 * in the working set.
	 *
	 * On platforms without a page file, this
	 * value will be equal to the working set.
	 */
	size_t GetLastMemoryUsagePrivateSample() const
	{
		return (size_t)m_uLastMemoryUsagePrivate;
	}

private:
	ScopedPtr<Thread> m_pWorker;
	Signal m_WorkerSignal;
	Atomic64Value<UInt64> m_uLastMemoryUsageWorking;
	Atomic64Value<UInt64> m_uLastMemoryUsagePrivate;
	Atomic32Value<Bool> m_bWorkerRunning;

	Int WorkerThread(const Thread&);

	SEOUL_DISABLE_COPY(DevUIMemoryUsageUtil);
}; // class Game::DevUIMemoryUsageUtil

} // namespace Seoul::Game

#endif // /SEOUL_ENABLE_DEV_UI

#endif // include guard
