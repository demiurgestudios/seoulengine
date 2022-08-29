/**
 * \file PseudoRandomTest.h
 * \brief Unit tests for the PseudoRandom class and corresponding
 * global math functions.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef PSEUDO_RANDOM_TEST_H
#define PSEUDO_RANDOM_TEST_H

#include "Prereqs.h"
#include "SeoulMath.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class PseudoRandomTest SEOUL_SEALED
{
public:
	void TestGlobalNormalRandomFloat();
	void TestGlobalNormalRandomFloatRange();
	void TestGlobalUniformFloat();
	void TestGlobalUniformIntRange();
	void TestInstanceNormalRandomFloat();
	void TestInstanceNormalRandomFloatRange();
	void TestInstanceUniformFloat();
	void TestInstanceUniformIntRange();
	void TestZeroZeroRegression();
	void TestBytesToSeed();
	void TestUniformRandomFloat64();
	void TestUniformRandomFloat32Regression();
}; // class PseudoRandomTest

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif  // include guard
