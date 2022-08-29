/**
 * \file SeoulFileTest.h
 * \brief Declaration of unit tests for the File class.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SEOUL_FILE_TEST_H
#define SEOUL_FILE_TEST_H

#include "Prereqs.h"
#include "SeoulFile.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class SeoulFileTest SEOUL_SEALED
{
public:
	void TestBinaryReadWriteDiskSyncFile();
	void TestBinaryReadWriteFullyBufferedSyncFile();
	void TestDiskSyncFileReadStatic();
	void TestTextReadWriteDiskSyncFile();
	void TestTextReadWriteFullyBufferedSyncFile();
	void TestReadWriteBufferedSyncFile();
	void TestUtilityFunctions();
};

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
