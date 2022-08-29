/**
 * \file VectorTest.h
 * \brief Unit test header file for the Seoul Vector<> class
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef VECTOR_TEST_H
#define VECTOR_TEST_H

#include "Prereqs.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class VectorTest SEOUL_SEALED
{
public:
	void TestAppendBuiltin();
	void TestAppendComplex();
	void TestAppendSimple();
	void TestAssignBuiltin();
	void TestAssignComplex();
	void TestAssignSimple();
	void TestBasic();
	void TestClearBuiltin();
	void TestClearComplex();
	void TestClearSimple();
	void TestConstructorBuiltin();
	void TestConstructorComplex();
	void TestConstructorSimple();
	void TestEmptyBuiltin();
	void TestEmptyComplex();
	void TestEmptySimple();
	void TestEqualityBuiltin();
	void TestEqualityComplex();
	void TestEqualitySimple();
	void TestFind();
	void TestMethods();
	void TestInsertBuiltin();
	void TestInsertComplex();
	void TestInsertSimple();
	void TestEraseBuiltin();
	void TestEraseComplex();
	void TestEraseSimple();
	void TestEraseReturnBuiltin();
	void TestEraseReturnComplex();
	void TestEraseReturnSimple();
	void TestEraseRangeReturnBuiltin();
	void TestEraseRangeReturnComplex();
	void TestEraseRangeReturnSimple();
	void TestIterators();
	void TestRangedFor();
	void TestRemoveBuiltin();
	void TestRemoveComplex();
	void TestRemoveComplexCoerce();
	void TestRemoveSimple();
	void TestPushBackMoveBuiltin();
	void TestPushBackMoveComplex();
	void TestPushBackMoveSimple();
	void TestInsertMoveBuiltin();
	void TestInsertMoveComplex();
	void TestInsertMoveSimple();

	void TestAssignRegressBuiltin();
	void TestAssignRegressComplex();
	void TestAssignRegressSimple();

	void TestSelfAssignBuiltin();
	void TestSelfAssignComplex();
	void TestSelfAssignSimple();

	void TestRemoveRegressionBuiltin();
	void TestRemoveRegressionComplex();
	void TestRemoveRegressionSimple();
	
	void TestSelfAssignRegressionBuiltin();
	void TestSelfAssignRegressionComplex();
	void TestSelfAssignRegressionSimple();

	void TestSelfFillRegressionBuiltin();
	void TestSelfFillRegressionComplex();
	void TestSelfFillRegressionSimple();

	void TestSelfInsertRegressionBuiltin();
	void TestSelfInsertRegressionComplex();
	void TestSelfInsertRegressionSimple();
	
	void TestRemoveFirstInstanceBuiltin();
	void TestRemoveFirstInstanceComplex();
	void TestRemoveFirstInstanceComplexCoerce();
	void TestRemoveFirstInstanceSimple();

	void TestRemoveCountBuiltin();
	void TestRemoveCountComplex();
	void TestRemoveCountSimple();
};

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
