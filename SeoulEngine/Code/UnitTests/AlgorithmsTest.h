/**
 * \file AlgorithmsTest.h
 * \brief Unit test header file for global utilities
 * defined in Algorithms.h
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ALGORITHMS_TEST_H
#define ALGORITHMS_TEST_H

#include "Prereqs.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class AlgorithmsTest SEOUL_SEALED
{
public:
	void TestContains();
	void TestContainsFromBack();
	void TestCopy();
	void TestCopyBackward();
	void TestDestroyRange();
	void TestFill();
	void TestFind();
	void TestFindIf();
	void TestFindFromBack();
	void TestLowerBound();
	void TestRandomShuffle();
	void TestReverse();
	void TestRotate();
	void TestSort();
	void TestSwap();
	void TestSwapRanges();
	void TestUninitializedCopy();
	void TestUninitializedCopyBackward();
	void TestUninitializedFill();
	void TestUninitializedMove();
	void TestUpperBound();
	void TestZeroFillSimple();
};

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
