/**
 * \file DirectoryTest.h
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef DIRECTORY_TEST_H
#define DIRECTORY_TEST_H

#include "Prereqs.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class DirectoryTest SEOUL_SEALED
{
public:
	void CreateDirPath();
	void Delete();
	void DirectoryExists();
	void GetDirectoryListing();
	void GetDirectoryListingEx();
}; // class AABBTest

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif  // include guard
