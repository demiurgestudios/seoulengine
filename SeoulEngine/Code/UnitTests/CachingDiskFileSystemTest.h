/**
 * \file CachingDiskFileSystemTest.h
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef CACHING_DISK_FILE_SYSTEM_TEST_H
#define CACHING_DISK_FILE_SYSTEM_TEST_H

#include "Prereqs.h"
#include "ScopedPtr.h"
#include "SeoulString.h"
namespace Seoul { class CachingDiskFileSystem; }
namespace Seoul { class DiskFileSystem; }
namespace Seoul { class UnitTestsFileManagerHelper; }

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class CachingDiskFileSystemTest SEOUL_SEALED
{
public:
	CachingDiskFileSystemTest();
	~CachingDiskFileSystemTest();

	void TestCopy();
	void TestCopyOverwrite();
	void TestCreateDirPath();
	void TestDeleteDirectory();
	void TestDeleteDirectoryRecursive();
	void TestDelete();
	void TestExists();
	void TestExistsForPlatform();
	void TestGetDirectoryListing();
	void TestGetDirectoryListingRobustness();
	void TestGetFileSize();
	void TestGetFileSizeForPlatform();
	void TestGetModifiedTime();
	void TestGetModifiedTimeForPlatform();
	void TestIsDirectory();
	void TestOpen();
	void TestOpenForPlatform();
	void TestRename();
	void TestReadAll();
	void TestReadAllForPlatform();
	void TestSetModifiedTime();
	void TestSetModifiedTimeForPlatform();
	void TestSetReadOnlyBit();
	void TestWriteAll();
	void TestWriteAllModifiedTime();
	void TestWriteAllForPlatform();
	void TestWriteAllForPlatformModifiedTime();

private:
	String const m_sOrig;
	ScopedPtr<UnitTestsFileManagerHelper> m_pFileManager;
	ScopedPtr<DiskFileSystem> m_pExpected;
	ScopedPtr<CachingDiskFileSystem> m_pFileSystem;

	FilePath GetTestPath() const;
	FilePath GetTestPathB() const;
	void DeleteTestFile() const;
	void DeleteTestFileB() const;
	void WriteTestDir() const;
	void WriteTestFile() const;
	void WriteTestFileB() const;

	void VerifyEqualImpl(FilePath filePath);
	void VerifyEqualImpl(const String& sAbsoluteFilename);
	void VerifyEqual(FilePath filePath);
	void VerifyEqual(const String& sAbsoluteFilename);
}; // class CachingDiskFileSystemTest

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
