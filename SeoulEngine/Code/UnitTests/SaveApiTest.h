/**
 * \file SaveApiTest.h
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SAVE_API_TEST_H
#define SAVE_API_TEST_H

#include "UnitTesting.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class SaveApiTest SEOUL_SEALED
{
public:
	void TestGeneric();
}; // class SaveApiTest

#endif // /#if SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
