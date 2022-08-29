/**
 * \file ZipFileTest.h
 * \brief Unit test code for the ZipFileReader and ZipFileWriter classes.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ZIP_FILE_TEST_H
#define ZIP_FILE_TEST_H

#include "Prereqs.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class ZipFileTest SEOUL_SEALED
{
public:
	void TestBasic();
	void TestNoFinalize();
	void TestNoInit();
};

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
