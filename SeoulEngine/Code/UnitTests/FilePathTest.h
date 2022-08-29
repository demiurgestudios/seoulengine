/**
 * \file FilePathTest.h
 * \brief Unit tests to verify that the FilePath class handles
 * and normalizes file paths as we expect.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FILE_PATH_TEST_H
#define FILE_PATH_TEST_H

#include "Prereqs.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class FilePathTest SEOUL_SEALED
{
public:
	void TestBasicFilePath();
	void TestAdvancedFilePath();
	void TestFilePathUtil();
}; // class FilePathTest

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
