/**
 * \file RemapDiskFileSystemTest.h
 * \brief Unit tests for RemapDiskFileSystem.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef REMAP_FILE_SYSTEM_TEST_H
#define REMAP_FILE_SYSTEM_TEST_H

#include "Prereqs.h"
#include "SeoulFile.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class RemapDiskFileSystemTest SEOUL_SEALED
{
public:
	void TestBase();
	void TestPatchA();
	void TestPatchB();
}; // class

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
