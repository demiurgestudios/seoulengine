/**
 * \file SeoulRegexTest.h
 * \brief Unit tests for the Regex class.
 *
 * \sa https://github.com/mattbucknall/subreg/blob/master/tests/subreg-tests.c
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
*/

#pragma once
#ifndef SEOUL_REGEX_TEST_H
#define SEOUL_REGEX_TEST_H

#include "Prereqs.h"
#include "SeoulRegex.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class SeoulRegexTest SEOUL_SEALED
{
public:
	void TestAny();
	void TestBackspace();
	void TestCarriageReturn();
	void TestEmptyFail();
	void TestEmptyPass();
	void TestFormFeed();
	void TestHorizontalTab();
	void TestLiteral();
	void TestMultiThreaded();
	void TestNewLine();
	void TestOneOrMoreFail();
	void TestOneOrMorePass();
	void TestOptionalNone();
	void TestOptionalOne();
	void TestOrNoneOfThree();
	void TestOrNoneOfTwo();
	void TestOrOneOfThree();
	void TestOrOneOfTwo();
	void TestOrThreeOfThree();
	void TestOrTwoOfThree();
	void TestOrTwoOfTwo();
	void TestSimpleFail();
	void TestSimplePass();
	void TestVerticalTab();
	void TestZeroOrMore();
};

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
