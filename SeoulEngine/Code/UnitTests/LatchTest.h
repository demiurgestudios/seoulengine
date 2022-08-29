/**
 * \file LatchTest.h
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef LATCH_TEST_H
#define LATCH_TEST_H

#include "Prereqs.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class LatchTest SEOUL_SEALED
{
public:
	void TestBasic();
	void TestConditions();
	void TestConditionsTrigger();
	void TestRequire();
}; // class LatchTest

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif  // include guard
