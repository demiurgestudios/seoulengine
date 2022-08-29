/**
 * \file JobsTest.h
 * \brief Unit tests for the Jobs library.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#ifndef JOBS_TEST_H
#define JOBS_TEST_H

#include "UnitTesting.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

/**
 * Text fixture class for the Jobs library.
 */
class JobsTest SEOUL_SEALED
{
public:
	void TestBasic();
	void TestJobContinuity();
	void TestJobYieldOnMainRegression();
	void TestMakeJobFunction();
	void TestCallJobFunction();
	void TestAwaitJobFunction();
}; // class JobsTest

#endif // /#if SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
