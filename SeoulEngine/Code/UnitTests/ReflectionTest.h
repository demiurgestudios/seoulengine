/**
 * \file ReflectionTest.h
 * \brief Unit tests to verify basic functionality of the Reflection
 * namespace.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef REFLECTION_TEST_H
#define REFLECTION_TEST_H

#include "Prereqs.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class ReflectionTest SEOUL_SEALED
{
public:
	void TestReflectionDeserializeBasic();
	void TestReflectionDeserializeComplexProperties();
	void TestReflectionSerializeToArrayBasic();
	void TestReflectionSerializeToTableBasic();
	void TestReflectionTypeBasic();
	void TestReflectionTypeAdvanced();
	void TestReflectionTypeAttributes();
	void TestReflectionTypeConstructors();
	void TestReflectionArray();
	void TestReflectionConstArray();
	void TestReflectionFixedArray();
	void TestReflectionEnum();
	void TestReflectionTable();
	void TestReflectionConstTable();
	void TestReflectionTypeMethods();
	void TestReflectionTypeProperties();
	void TestReflectionFieldProperies();
}; // class ReflectionTest

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
