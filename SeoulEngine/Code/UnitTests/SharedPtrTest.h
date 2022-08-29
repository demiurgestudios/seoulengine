/**
 * \file SharedPtrTest.h
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SHARED_PTR_TEST_H
#define SHARED_PTR_TEST_H

#include "Prereqs.h"
#include "SeoulMath.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class SharedPtrTest SEOUL_SEALED
{
public:
	void TestBasic();
	void TestBoxedValue();
}; // class SharedPtrTest

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif  // include guard
