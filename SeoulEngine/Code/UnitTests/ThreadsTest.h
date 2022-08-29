/**
 * \file ThreadsTest.h
 * \brief Header file for threading system unit tests
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef THREADS_TEST_H
#define THREADS_TEST_H

#include "Mutex.h"
#include "SeoulSignal.h"
#include "Thread.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

/**
 * Text fixture class for threads.
 */
class ThreadTest SEOUL_SEALED
{
public:
	void TestThreads();
	void TestMutexes();
	void TestSignalInfiniteWait();
	void TestSignal0Wait();
	void TestSignalTimedWait();
}; // class ThreadTest

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
