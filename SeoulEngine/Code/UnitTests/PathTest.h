/**
 * \file PathTest.h
 * \brief Unit tests to verify that the functions in namespace Path
 * correctly handle file paths.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef PATH_TEST_H
#define PATH_TEST_H

#include "Prereqs.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class PathTest SEOUL_SEALED
{
public:
	void TestAdvancedPath();
	void TestBasicPath();
	void TestCombine();
	void TestGetExactPathName();
	void TestGetDirectoryNameN();
	void TestGetProcessDirectory();
	void TestGetTempDirectory();
	void TestGetTempFileAbsoluteFilename();
	void TestSingleDotRegression();
}; // class PathTest

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
