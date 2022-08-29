/**
 * \file CoroutineTest.h
 * \brief Unit tests for SeoulEngine coroutines.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef COROUTINE_TEST_H
#define COROUTINE_TEST_H

#include "Prereqs.h"
#include "Coroutine.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

/**
 * Text fixture class for SeoulEngine Coroutines.
 */
class CoroutineTest SEOUL_SEALED
{
public:
	void TestBasicCoroutines();
}; // class CoroutineTest

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
