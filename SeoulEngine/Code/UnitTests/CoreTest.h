/**
 * \file CoreTest.h
 * \brief Unit test header file for core functionality
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef CORE_TEST_H
#define CORE_TEST_H

#include "Prereqs.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class CoreTest SEOUL_SEALED
{
public:
	void TestRoundUpToPowerOf2(void);
};

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
