/**
 * \file Vector2DTest.h
 * \brief Unit test header file for 2D vector class
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef VECTOR2D_TEST_H
#define VECTOR2D_TEST_H

#include "Prereqs.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class Vector2DTest SEOUL_SEALED
{
public:
	void TestBasic(void);
	void TestOperators(void);
	void TestMiscMethods(void);
};

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
