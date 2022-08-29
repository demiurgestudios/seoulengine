/**
 * \file SeoulSignal.h
 * \brief Signal represents an object that can be used to trigger events
 * across threads.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SEOUL_SIGNAL_H
#define SEOUL_SIGNAL_H

#include "Prereqs.h"
#include "SeoulMath.h"
#include "UnsafeHandle.h"

namespace Seoul
{

/**
 * A Signal is used to communite between threads.
 * One thread can Wait() for a Signal to be Activate()ed by another
 * thread, at which point the Wait()ing thread will resume execution.
 */
class Signal SEOUL_SEALED
{
public:
	Signal();
	~Signal();

	// Wake up the Signal. Will release at least one waiting thread.
	Bool Activate() const;

	// Wait on the Signal. Waits indefinitely until Activate()
	// is called.
	void Wait() const;

	// Wait on the Signal. If a single Activate() call is pending,
	// will return immediately, otherwise will block for up to
	// uTimeInMilliseconds.
	Bool Wait(UInt32 uTimeInMilliseconds) const;

private:
	UnsafeHandle m_hHandle;
}; // class Signal

} // namespace Seoul

#endif // include guard
