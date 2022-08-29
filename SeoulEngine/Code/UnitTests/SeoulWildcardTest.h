/**
 * \file SeoulWildcardTest.h
 * \brief Declaration of unit tests for the File class.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SEOUL_WILDCARD_TEST_H
#define SEOUL_WILDCARD_TEST_H

#include "Prereqs.h"
#include "SeoulWildcard.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class SeoulWildcardTest SEOUL_SEALED
{
public:
	void TestAsterisk();
	void TestMixed();
	void TestQuestionMark();
};

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
