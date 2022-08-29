/**
 * \file WordFilterTest.h
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef WORD_FILTER_TEST_H
#define WORD_FILTER_TEST_H

#include "Prereqs.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class WordFilterTest SEOUL_SEALED
{
public:
	void TestBasic();
	void TestComplexNegatives();
	void TestComplexPositives();
	void TestFalsePositives();
	void TestKnownPositives();
	void TestLeetSpeak();
	void TestPhonetics();
	void TestUglyChatLog();
}; // class WordFilterTest

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
