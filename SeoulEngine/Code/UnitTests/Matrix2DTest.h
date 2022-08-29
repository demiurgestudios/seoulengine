/**
 * \file Matrix2DTest.h
 * \brief Unit tests for the Matrix2D class. Matrix2D is the work horse
 * of our linear algebra classes, and is used to represet a variety
 * of 3D transformations.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef MATRIX2D_TEST_H
#define MATRIX2D_TEST_H

#include "Prereqs.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class Matrix2DTest SEOUL_SEALED
{
public:
	void TestMethods();
	void TestTransformation();
	void TestUtilities();
};

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
