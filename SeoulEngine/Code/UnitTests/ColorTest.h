/**
 * \file ColorTest.h
 * \brief Unit tests for structures and utilities in the Color.h header.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef COLOR_TEST_H
#define COLOR_TEST_H

#include "Prereqs.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class ColorTest SEOUL_SEALED
{
public:
	void TestAdd();
	void TestConvert();
	void TestDefault();
	void TestEqual();
	void TestGetData();
	void TestLerp();
	void TestModulate();
	void TestPremultiply();
	void TestPremultiply2();
	void TestSpecial();
	void TestStandard();
	void TestSubtract();
}; // class ColorTest

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif  // include guard
