/**
 * \file QuaternionTest.h
 * \brief Unit tests for the Quaternion class. Quaternions are
 * used to represent 3D rotations in a way that does not
 * suffer from gimbal lock.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef QUATERNION_TEST_H
#define QUATERNION_TEST_H

#include "Prereqs.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class QuaternionTest SEOUL_SEALED
{
public:
	void TestMethods();
	void TestToMatrix();
	void TestFromMatrix();
	void TestTransformation();
	void TestUtilities();
	void TestTransformationRegressions();
};

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
