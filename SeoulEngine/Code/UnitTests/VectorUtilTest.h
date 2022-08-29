/**
 * \file VectorUtilTest.h
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef VECTOR_UTIL_TEST_H
#define VECTOR_UTIL_TEST_H

#include "Prereqs.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class VectorUtilTest SEOUL_SEALED
{
public:
	void TestFindValueRandomTiebreaker();
};

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
