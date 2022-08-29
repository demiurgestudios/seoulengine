/**
 * \file SeoulMathTest.h
 * \brief Unit tests for Seoul engine global math functions.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SEOUL_MATH_TEST_H
#define SEOUL_MATH_TEST_H

#include "Prereqs.h"
#include "SeoulMath.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class SeoulMathTest SEOUL_SEALED
{
public:
	void TestBitCount();
	void TestClampCircular();
	void TestLinearCurveDefault();
	void TestLinearCurveBasic();
	void TestMathFunctions();
	void TestInt32Clamped();
}; // class SeoulMathTest

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif  // include guard
