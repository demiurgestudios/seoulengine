/**
 * \file LocManagerTest.h
 * \brief Unit tests for the LocManager singleton.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#ifndef LOC_MANAGER_TEST_H
#define LOC_MANAGER_TEST_H

#include "UnitTesting.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

/**
 * Text fixture class for the LocManager.
 */
class LocManagerTest SEOUL_SEALED
{
public:
	void TestBasic();
	void TestTimeFormat();
	void TestPatchAdditive();
	void TestPatchSubtractive();
	void TestPatchSubtractiveRegression();
}; // class LocManagerTestFixture

#endif // /#if SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
