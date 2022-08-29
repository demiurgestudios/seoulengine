/**
 * \file DirectoryTest.cpp
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Directory.h"
#include "DirectoryTest.h"
#include "DiskFileSystem.h"
#include "Logger.h"
#include "ReflectionDefine.h"
#include "UnitTesting.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(DirectoryTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(CreateDirPath)
	SEOUL_METHOD(Delete)
	SEOUL_METHOD(DirectoryExists)
	SEOUL_METHOD(GetDirectoryListing)
	SEOUL_METHOD(GetDirectoryListingEx)
SEOUL_END_TYPE()

static inline UInt64 WriteTestData(const String& sDir, const String& sName, UInt32 uSize)
{
	auto const sPath(Path::Combine(sDir, sName));
	{
		DiskSyncFile file(sPath, File::kWriteTruncate);
		for (UInt32 i = 0u; i < uSize; ++i)
		{
			UInt8 const a = (UInt8)GlobalRandom::UniformRandomUInt32n(256u);
			SEOUL_UNITTESTING_ASSERT(1u == file.WriteRawData(&a, sizeof(a)));
		}
		SEOUL_UNITTESTING_ASSERT(file.Flush());
	}

	SEOUL_UNITTESTING_ASSERT(DiskSyncFile::FileExists(sPath));
	SEOUL_UNITTESTING_ASSERT_EQUAL(uSize, (UInt32)DiskSyncFile::GetFileSize(sPath));

	return DiskSyncFile::GetModifiedTime(sPath);
}

void DirectoryTest::CreateDirPath()
{
	auto const sTempDir(Path::GetTempDirectory());

	// Simple
	{
		auto const sPath(Path::Combine(sTempDir, "TestDir"));

		// Cleanup in case of previous test failure.
		(void)Directory::Delete(sPath, true);

		SEOUL_UNITTESTING_ASSERT(!Directory::DirectoryExists(sPath));
		SEOUL_UNITTESTING_ASSERT(Directory::CreateDirPath(sPath));
		SEOUL_UNITTESTING_ASSERT(Directory::DirectoryExists(sPath));
		SEOUL_UNITTESTING_ASSERT(!DiskSyncFile::FileExists(sPath));
		SEOUL_UNITTESTING_ASSERT(Directory::Delete(sPath, false));
		SEOUL_UNITTESTING_ASSERT(!Directory::DirectoryExists(sPath));
	}

	// Nested
	{
		auto const sPathRoot(Path::Combine(sTempDir, "TestDir"));
		auto const sPath(Path::Combine(sTempDir, "TestDir/InnerDir"));

		// Cleanup in case of previous test failure.
		(void)Directory::Delete(sPath, true);

		SEOUL_UNITTESTING_ASSERT(!Directory::DirectoryExists(sPath));
		SEOUL_UNITTESTING_ASSERT(Directory::CreateDirPath(sPath));
		SEOUL_UNITTESTING_ASSERT(Directory::DirectoryExists(sPath));
		SEOUL_UNITTESTING_ASSERT(!DiskSyncFile::FileExists(sPath));
		SEOUL_UNITTESTING_ASSERT(!Directory::Delete(sPathRoot, false)); // Will fail, not recursive.
		SEOUL_UNITTESTING_ASSERT(Directory::DirectoryExists(sPath));
		SEOUL_UNITTESTING_ASSERT(Directory::Delete(sPathRoot, true));
		SEOUL_UNITTESTING_ASSERT(!Directory::DirectoryExists(sPath));
	}
}

void DirectoryTest::Delete()
{
	auto const sTempDir(Path::GetTempDirectory());

	// Simple
	{
		auto const sPath(Path::Combine(sTempDir, "TestDir"));

		// Cleanup in case of previous test failure.
		(void)Directory::Delete(sPath, true);

		SEOUL_UNITTESTING_ASSERT(!Directory::DirectoryExists(sPath));
		SEOUL_UNITTESTING_ASSERT(Directory::CreateDirPath(sPath));
		SEOUL_UNITTESTING_ASSERT(Directory::DirectoryExists(sPath));
		SEOUL_UNITTESTING_ASSERT(!DiskSyncFile::FileExists(sPath));
		SEOUL_UNITTESTING_ASSERT(Directory::Delete(sPath, false));
		SEOUL_UNITTESTING_ASSERT(!Directory::DirectoryExists(sPath));
	}

	// Nested
	{
		auto const sPathRoot(Path::Combine(sTempDir, "TestDir"));
		auto const sPath(Path::Combine(sTempDir, "TestDir/InnerDir"));

		// Cleanup in case of previous test failure.
		(void)Directory::Delete(sPathRoot, true);

		SEOUL_UNITTESTING_ASSERT(!Directory::DirectoryExists(sPath));
		SEOUL_UNITTESTING_ASSERT(Directory::CreateDirPath(sPath));
		SEOUL_UNITTESTING_ASSERT(Directory::DirectoryExists(sPath));
		SEOUL_UNITTESTING_ASSERT(!DiskSyncFile::FileExists(sPath));
		SEOUL_UNITTESTING_ASSERT(!Directory::Delete(sPathRoot, false)); // Will fail, not recursive.
		SEOUL_UNITTESTING_ASSERT(Directory::DirectoryExists(sPath));
		SEOUL_UNITTESTING_ASSERT(Directory::Delete(sPathRoot, true));
		SEOUL_UNITTESTING_ASSERT(!Directory::DirectoryExists(sPath));
	}

	// With files.
	{
		auto const sPathRoot(Path::Combine(sTempDir, "TestDir"));
		auto const sPath(Path::Combine(sTempDir, "TestDir/InnerDir"));
		static const UInt32 kuTestFiles = 5u;

		// Cleanup in case of previous test failure.
		(void)Directory::Delete(sPathRoot, true);

		SEOUL_UNITTESTING_ASSERT(!Directory::DirectoryExists(sPath));
		SEOUL_UNITTESTING_ASSERT(Directory::CreateDirPath(sPath));
		SEOUL_UNITTESTING_ASSERT(Directory::DirectoryExists(sPath));
		SEOUL_UNITTESTING_ASSERT(!DiskSyncFile::FileExists(sPath));

		// Add some files.
		for (UInt32 i = 0u; i < kuTestFiles; ++i)
		{
			WriteTestData(sPath, String::Printf("Test%u", i), i + 1);
		}

		SEOUL_UNITTESTING_ASSERT(!Directory::Delete(sPathRoot, false)); // Will fail, not recursive.
		SEOUL_UNITTESTING_ASSERT(Directory::DirectoryExists(sPath));

		// Check files.
		for (UInt32 i = 0u; i < kuTestFiles; ++i)
		{
			auto const sFileName(Path::Combine(sPath, String::Printf("Test%u", i)));
			SEOUL_UNITTESTING_ASSERT(DiskSyncFile::FileExists(sFileName));
			SEOUL_UNITTESTING_ASSERT_EQUAL(i + 1, (UInt32)DiskSyncFile::GetFileSize(sFileName));
		}

		SEOUL_UNITTESTING_ASSERT(Directory::Delete(sPathRoot, true));
		SEOUL_UNITTESTING_ASSERT(!Directory::DirectoryExists(sPath));

		// Check files.
		for (UInt32 i = 0u; i < kuTestFiles; ++i)
		{
			auto const sFileName(Path::Combine(sPath, String::Printf("Test%u", i)));
			SEOUL_UNITTESTING_ASSERT(!DiskSyncFile::FileExists(sFileName));
		}
	}
}

void DirectoryTest::DirectoryExists()
{
	auto const sTempDir(Path::GetTempDirectory());

	// Simple
	{
		auto const sPath(Path::Combine(sTempDir, "TestDir"));

		// Cleanup in case of previous test failure.
		(void)Directory::Delete(sPath, true);

		SEOUL_UNITTESTING_ASSERT(!Directory::DirectoryExists(sPath));
		SEOUL_UNITTESTING_ASSERT(Directory::CreateDirPath(sPath));
		SEOUL_UNITTESTING_ASSERT(Directory::DirectoryExists(sPath));
		SEOUL_UNITTESTING_ASSERT(!DiskSyncFile::FileExists(sPath));
		SEOUL_UNITTESTING_ASSERT(Directory::Delete(sPath, false));
		SEOUL_UNITTESTING_ASSERT(!Directory::DirectoryExists(sPath));
	}

	// Nested
	{
		auto const sPathRoot(Path::Combine(sTempDir, "TestDir"));
		auto const sPath(Path::Combine(sTempDir, "TestDir/InnerDir"));

		// Cleanup in case of previous test failure.
		(void)Directory::Delete(sPathRoot, true);

		SEOUL_UNITTESTING_ASSERT(!Directory::DirectoryExists(sPath));
		SEOUL_UNITTESTING_ASSERT(Directory::CreateDirPath(sPath));
		SEOUL_UNITTESTING_ASSERT(Directory::DirectoryExists(sPath));
		SEOUL_UNITTESTING_ASSERT(!DiskSyncFile::FileExists(sPath));
		SEOUL_UNITTESTING_ASSERT(!Directory::Delete(sPathRoot, false)); // Will fail, not recursive.
		SEOUL_UNITTESTING_ASSERT(Directory::DirectoryExists(sPath));
		SEOUL_UNITTESTING_ASSERT(Directory::Delete(sPathRoot, true));
		SEOUL_UNITTESTING_ASSERT(!Directory::DirectoryExists(sPath));
	}

	// Multiple nested.
	{
		auto const sPathRoot(Path::Combine(sTempDir, "TestDir"));
		auto const sPath1(Path::Combine(sTempDir, "TestDir/InnerDir"));
		auto const sPath2(Path::Combine(sTempDir, "TestDir/InnerDir2"));

		// Cleanup in case of previous test failure.
		(void)Directory::Delete(sPathRoot, true);

		SEOUL_UNITTESTING_ASSERT(!Directory::DirectoryExists(sPath1));
		SEOUL_UNITTESTING_ASSERT(!Directory::DirectoryExists(sPath2));
		SEOUL_UNITTESTING_ASSERT(Directory::CreateDirPath(sPath1));
		SEOUL_UNITTESTING_ASSERT(Directory::DirectoryExists(sPath1));
		SEOUL_UNITTESTING_ASSERT(!DiskSyncFile::FileExists(sPath1));
		SEOUL_UNITTESTING_ASSERT(Directory::CreateDirPath(sPath2));
		SEOUL_UNITTESTING_ASSERT(Directory::DirectoryExists(sPath2));
		SEOUL_UNITTESTING_ASSERT(!DiskSyncFile::FileExists(sPath2));

		SEOUL_UNITTESTING_ASSERT(!Directory::Delete(sPathRoot, false)); // Will fail, not recursive.
		SEOUL_UNITTESTING_ASSERT(Directory::DirectoryExists(sPath1));
		SEOUL_UNITTESTING_ASSERT(Directory::DirectoryExists(sPath2));
		SEOUL_UNITTESTING_ASSERT(Directory::Delete(sPathRoot, true));
		SEOUL_UNITTESTING_ASSERT(!Directory::DirectoryExists(sPath1));
		SEOUL_UNITTESTING_ASSERT(!Directory::DirectoryExists(sPath2));
	}
}

void DirectoryTest::GetDirectoryListing()
{
	auto const sTempDir(Path::GetTempDirectory());

	// Empty
	{
		auto const sPath(Path::Combine(sTempDir, "TestDir"));

		// Cleanup in case of previous test failure.
		(void)Directory::Delete(sPath, true);

		SEOUL_UNITTESTING_ASSERT(!Directory::DirectoryExists(sPath));
		SEOUL_UNITTESTING_ASSERT(Directory::CreateDirPath(sPath));
		SEOUL_UNITTESTING_ASSERT(Directory::DirectoryExists(sPath));
		SEOUL_UNITTESTING_ASSERT(!DiskSyncFile::FileExists(sPath));

		Vector<String> vs;
		SEOUL_UNITTESTING_ASSERT(Directory::GetDirectoryListing(
			sPath,
			vs,
			false,
			false,
			String()));
		SEOUL_UNITTESTING_ASSERT(vs.IsEmpty());
		SEOUL_UNITTESTING_ASSERT(Directory::GetDirectoryListing(
			sPath,
			vs,
			true,
			false,
			String()));
		SEOUL_UNITTESTING_ASSERT(vs.IsEmpty());
		SEOUL_UNITTESTING_ASSERT(Directory::GetDirectoryListing(
			sPath,
			vs,
			false,
			true,
			String()));
		SEOUL_UNITTESTING_ASSERT(vs.IsEmpty());
		SEOUL_UNITTESTING_ASSERT(Directory::GetDirectoryListing(
			sPath,
			vs,
			true,
			true,
			String()));
		SEOUL_UNITTESTING_ASSERT(vs.IsEmpty());

		SEOUL_UNITTESTING_ASSERT(Directory::Delete(sPath, false));
		SEOUL_UNITTESTING_ASSERT(!Directory::DirectoryExists(sPath));
	}

	// One file.
	{
		auto const sPath(Path::Combine(sTempDir, "TestDir"));

		// Cleanup in case of previous test failure.
		(void)Directory::Delete(sPath, true);

		SEOUL_UNITTESTING_ASSERT(!Directory::DirectoryExists(sPath));
		SEOUL_UNITTESTING_ASSERT(Directory::CreateDirPath(sPath));
		SEOUL_UNITTESTING_ASSERT(Directory::DirectoryExists(sPath));
		SEOUL_UNITTESTING_ASSERT(!DiskSyncFile::FileExists(sPath));

		// Write a single file.
		auto const sFilePath(Path::Combine(sPath, "TestFile"));
		WriteTestData(sPath, "TestFile", 1u);

		Vector<String> vs;
		SEOUL_UNITTESTING_ASSERT(Directory::GetDirectoryListing(
			sPath,
			vs,
			false,
			false,
			String()));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1u, vs.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(sFilePath, vs.Front());

		SEOUL_UNITTESTING_ASSERT(Directory::GetDirectoryListing(
			sPath,
			vs,
			true,
			false,
			String()));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1u, vs.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(sFilePath, vs.Front());

		SEOUL_UNITTESTING_ASSERT(Directory::GetDirectoryListing(
			sPath,
			vs,
			false,
			true,
			String()));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1u, vs.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(sFilePath, vs.Front());

		SEOUL_UNITTESTING_ASSERT(Directory::GetDirectoryListing(
			sPath,
			vs,
			true,
			true,
			String()));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1u, vs.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(sFilePath, vs.Front());

		SEOUL_UNITTESTING_ASSERT(Directory::Delete(sPath, true));
		SEOUL_UNITTESTING_ASSERT(!Directory::DirectoryExists(sPath));
	}

	// One directory.
	{
		auto const sPath(Path::Combine(sTempDir, "TestDir"));

		// Cleanup in case of previous test failure.
		(void)Directory::Delete(sPath, true);

		SEOUL_UNITTESTING_ASSERT(!Directory::DirectoryExists(sPath));
		SEOUL_UNITTESTING_ASSERT(Directory::CreateDirPath(sPath));
		SEOUL_UNITTESTING_ASSERT(Directory::DirectoryExists(sPath));
		SEOUL_UNITTESTING_ASSERT(!DiskSyncFile::FileExists(sPath));

		// Write a single inner directory.
		auto const sTestDirPath(Path::Combine(sPath, "TestDirectory"));
		SEOUL_UNITTESTING_ASSERT(Directory::CreateDirPath(sTestDirPath));

		Vector<String> vs;
		SEOUL_UNITTESTING_ASSERT(Directory::GetDirectoryListing(
			sPath,
			vs,
			false,
			false,
			String()));
		SEOUL_UNITTESTING_ASSERT(vs.IsEmpty());

		SEOUL_UNITTESTING_ASSERT(Directory::GetDirectoryListing(
			sPath,
			vs,
			true,
			false,
			String()));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1u, vs.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(sTestDirPath, vs.Front());

		SEOUL_UNITTESTING_ASSERT(Directory::GetDirectoryListing(
			sPath,
			vs,
			false,
			true,
			String()));
		SEOUL_UNITTESTING_ASSERT(vs.IsEmpty());

		SEOUL_UNITTESTING_ASSERT(Directory::GetDirectoryListing(
			sPath,
			vs,
			true,
			true,
			String()));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1u, vs.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(sTestDirPath, vs.Front());

		SEOUL_UNITTESTING_ASSERT(Directory::Delete(sPath, true));
		SEOUL_UNITTESTING_ASSERT(!Directory::DirectoryExists(sPath));
	}

	// File and directory.
	{
		auto const sPath(Path::Combine(sTempDir, "TestDir"));

		// Cleanup in case of previous test failure.
		(void)Directory::Delete(sPath, true);

		SEOUL_UNITTESTING_ASSERT(!Directory::DirectoryExists(sPath));
		SEOUL_UNITTESTING_ASSERT(Directory::CreateDirPath(sPath));
		SEOUL_UNITTESTING_ASSERT(Directory::DirectoryExists(sPath));
		SEOUL_UNITTESTING_ASSERT(!DiskSyncFile::FileExists(sPath));

		// Write a single file.
		auto const sFilePath(Path::Combine(sPath, "TestFile"));
		WriteTestData(sPath, "TestFile", 1u);
		// Write a single inner directory.
		auto const sTestDirPath(Path::Combine(sPath, "TestDirectory"));
		SEOUL_UNITTESTING_ASSERT(Directory::CreateDirPath(sTestDirPath));

		Vector<String> vs;
		SEOUL_UNITTESTING_ASSERT(Directory::GetDirectoryListing(
			sPath,
			vs,
			false,
			false,
			String()));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1u, vs.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(sFilePath, vs.Front());

		SEOUL_UNITTESTING_ASSERT(Directory::GetDirectoryListing(
			sPath,
			vs,
			true,
			false,
			String()));
		SEOUL_UNITTESTING_ASSERT_EQUAL(2u, vs.GetSize());
		QuickSort(vs.Begin(), vs.End());
		SEOUL_UNITTESTING_ASSERT_EQUAL(sTestDirPath, vs.Front());
		SEOUL_UNITTESTING_ASSERT_EQUAL(sFilePath, vs.Back());

		SEOUL_UNITTESTING_ASSERT(Directory::GetDirectoryListing(
			sPath,
			vs,
			false,
			true,
			String()));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1u, vs.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(sFilePath, vs.Front());

		SEOUL_UNITTESTING_ASSERT(Directory::GetDirectoryListing(
			sPath,
			vs,
			true,
			true,
			String()));
		SEOUL_UNITTESTING_ASSERT_EQUAL(2u, vs.GetSize());
		QuickSort(vs.Begin(), vs.End());
		SEOUL_UNITTESTING_ASSERT_EQUAL(sTestDirPath, vs.Front());
		SEOUL_UNITTESTING_ASSERT_EQUAL(sFilePath, vs.Back());

		SEOUL_UNITTESTING_ASSERT(Directory::Delete(sPath, true));
		SEOUL_UNITTESTING_ASSERT(!Directory::DirectoryExists(sPath));
	}

	// File and directory recursive (recursive file).
	{
		auto const sPath(Path::Combine(sTempDir, "TestDir"));

		// Cleanup in case of previous test failure.
		(void)Directory::Delete(sPath, true);

		SEOUL_UNITTESTING_ASSERT(!Directory::DirectoryExists(sPath));
		SEOUL_UNITTESTING_ASSERT(Directory::CreateDirPath(sPath));
		SEOUL_UNITTESTING_ASSERT(Directory::DirectoryExists(sPath));
		SEOUL_UNITTESTING_ASSERT(!DiskSyncFile::FileExists(sPath));

		// Write a single file.
		auto const sFilePath1(Path::Combine(sPath, "TestFile"));
		WriteTestData(sPath, "TestFile", 1u);
		// Write a single inner directory.
		auto const sTestDirPath1(Path::Combine(sPath, "TestDirectory"));
		auto const sTestDirPath2(Path::Combine(sPath, "TestDirectory", "TestDirectory2"));
		SEOUL_UNITTESTING_ASSERT(Directory::CreateDirPath(sTestDirPath2));
		// Write a single file.
		auto const sFilePath2(Path::Combine(sTestDirPath1, "TestFile"));
		WriteTestData(sTestDirPath1, "TestFile", 2u);

		Vector<String> vs;
		SEOUL_UNITTESTING_ASSERT(Directory::GetDirectoryListing(
			sPath,
			vs,
			false,
			false,
			String()));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1u, vs.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(sFilePath1, vs.Front());

		SEOUL_UNITTESTING_ASSERT(Directory::GetDirectoryListing(
			sPath,
			vs,
			true,
			false,
			String()));
		SEOUL_UNITTESTING_ASSERT_EQUAL(2u, vs.GetSize());
		QuickSort(vs.Begin(), vs.End());
		SEOUL_UNITTESTING_ASSERT_EQUAL(sTestDirPath1, vs.Front());
		SEOUL_UNITTESTING_ASSERT_EQUAL(sFilePath1, vs.Back());

		SEOUL_UNITTESTING_ASSERT(Directory::GetDirectoryListing(
			sPath,
			vs,
			false,
			true,
			String()));
		SEOUL_UNITTESTING_ASSERT_EQUAL(2u, vs.GetSize());
		QuickSort(vs.Begin(), vs.End());
		SEOUL_UNITTESTING_ASSERT_EQUAL(sFilePath2, vs.Front());
		SEOUL_UNITTESTING_ASSERT_EQUAL(sFilePath1, vs.Back());

		SEOUL_UNITTESTING_ASSERT(Directory::GetDirectoryListing(
			sPath,
			vs,
			true,
			true,
			String()));
		SEOUL_UNITTESTING_ASSERT_EQUAL(4u, vs.GetSize());
		QuickSort(vs.Begin(), vs.End());
		SEOUL_UNITTESTING_ASSERT_EQUAL(sTestDirPath1, vs[0]);
		SEOUL_UNITTESTING_ASSERT_EQUAL(sTestDirPath2, vs[1]);
		SEOUL_UNITTESTING_ASSERT_EQUAL(sFilePath2, vs[2]);
		SEOUL_UNITTESTING_ASSERT_EQUAL(sFilePath1, vs[3]);

		SEOUL_UNITTESTING_ASSERT(Directory::Delete(sPath, true));
		SEOUL_UNITTESTING_ASSERT(!Directory::DirectoryExists(sPath));
	}
}

namespace
{

Bool Append(void* p, Directory::DirEntryEx& r)
{
	auto& rv = *reinterpret_cast<Vector<Directory::DirEntryEx>*>(p);
	rv.PushBack(RvalRef(r));
	return true;
}

Bool Count(void* p, Directory::DirEntryEx& r)
{
	(*reinterpret_cast<UInt32*>(p))++;
	return true;
}

struct ByFileName
{
	Bool operator()(const Directory::DirEntryEx& a, const Directory::DirEntryEx& b) const
	{
		return a.m_sFileName < b.m_sFileName;
	}
};

} // namespace

void DirectoryTest::GetDirectoryListingEx()
{
	auto const sTempDir(Path::GetTempDirectory());

	// Empty
	{
		auto const sPath(Path::Combine(sTempDir, "TestDir"));

		// Cleanup in case of previous test failure.
		(void)Directory::Delete(sPath, true);

		SEOUL_UNITTESTING_ASSERT(!Directory::DirectoryExists(sPath));
		SEOUL_UNITTESTING_ASSERT(Directory::CreateDirPath(sPath));
		SEOUL_UNITTESTING_ASSERT(Directory::DirectoryExists(sPath));
		SEOUL_UNITTESTING_ASSERT(!DiskSyncFile::FileExists(sPath));

		UInt32 uCount = 0u;
		SEOUL_UNITTESTING_ASSERT(Directory::GetDirectoryListingEx(
			sPath,
			SEOUL_BIND_DELEGATE(Count, (void*)&uCount)));
		SEOUL_UNITTESTING_ASSERT_EQUAL(0u, uCount);

		SEOUL_UNITTESTING_ASSERT(Directory::Delete(sPath, false));
		SEOUL_UNITTESTING_ASSERT(!Directory::DirectoryExists(sPath));
	}

	// One file.
	{
		auto const sPath(Path::Combine(sTempDir, "TestDir"));

		// Cleanup in case of previous test failure.
		(void)Directory::Delete(sPath, true);

		SEOUL_UNITTESTING_ASSERT(!Directory::DirectoryExists(sPath));
		SEOUL_UNITTESTING_ASSERT(Directory::CreateDirPath(sPath));
		SEOUL_UNITTESTING_ASSERT(Directory::DirectoryExists(sPath));
		SEOUL_UNITTESTING_ASSERT(!DiskSyncFile::FileExists(sPath));

		// Write a single file.
		auto const sFilePath(Path::Combine(sPath, "TestFile"));
		auto const uModifiedTime = WriteTestData(sPath, "TestFile", 1u);

		UInt32 uCount = 0u;
		SEOUL_UNITTESTING_ASSERT(Directory::GetDirectoryListingEx(sPath, SEOUL_BIND_DELEGATE(Count, &uCount)));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1u, uCount);

		Vector<Directory::DirEntryEx> v;
		SEOUL_UNITTESTING_ASSERT(Directory::GetDirectoryListingEx(sPath, SEOUL_BIND_DELEGATE(Append, &v)));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1u, v.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(sFilePath, v.Front().m_sFileName);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1u, v.Front().m_uFileSize);
		SEOUL_UNITTESTING_ASSERT_EQUAL(uModifiedTime, v.Front().m_uModifiedTime);

		SEOUL_UNITTESTING_ASSERT(Directory::Delete(sPath, true));
		SEOUL_UNITTESTING_ASSERT(!Directory::DirectoryExists(sPath));
	}

	// Multiple files.
	{
		auto const sPath(Path::Combine(sTempDir, "TestDir"));

		// Cleanup in case of previous test failure.
		(void)Directory::Delete(sPath, true);

		SEOUL_UNITTESTING_ASSERT(!Directory::DirectoryExists(sPath));
		SEOUL_UNITTESTING_ASSERT(Directory::CreateDirPath(sPath));
		SEOUL_UNITTESTING_ASSERT(Directory::DirectoryExists(sPath));
		SEOUL_UNITTESTING_ASSERT(!DiskSyncFile::FileExists(sPath));

		// Multiple files.
		static const UInt32 kuCount = 5u;
		Vector<Directory::DirEntryEx> vExpected;
		for (UInt32 i = 0u; i < kuCount; ++i)
		{
			Directory::DirEntryEx expected;
			expected.m_sFileName = Path::Combine(sPath, String::Printf("TestFile%u", i));
			expected.m_uFileSize = i + 1;
			expected.m_uModifiedTime = WriteTestData(sPath, String::Printf("TestFile%u", i), i + 1);
			vExpected.PushBack(expected);
		}

		UInt32 uCount = 0u;
		SEOUL_UNITTESTING_ASSERT(Directory::GetDirectoryListingEx(sPath, SEOUL_BIND_DELEGATE(Count, &uCount)));
		SEOUL_UNITTESTING_ASSERT_EQUAL(kuCount, uCount);

		Vector<Directory::DirEntryEx> v;
		SEOUL_UNITTESTING_ASSERT(Directory::GetDirectoryListingEx(sPath, SEOUL_BIND_DELEGATE(Append, &v)));
		SEOUL_UNITTESTING_ASSERT_EQUAL(vExpected.GetSize(), v.GetSize());
		ByFileName sorter;
		QuickSort(v.Begin(), v.End(), sorter);
		UInt32 i = 0u;
		for (auto const& expected : vExpected)
		{
			auto const& actual = v[i]; ++i;
			SEOUL_UNITTESTING_ASSERT_EQUAL(expected.m_sFileName, actual.m_sFileName);
			SEOUL_UNITTESTING_ASSERT_EQUAL(expected.m_uFileSize, actual.m_uFileSize);
			SEOUL_UNITTESTING_ASSERT_EQUAL(expected.m_uModifiedTime, actual.m_uModifiedTime);
		}

		SEOUL_UNITTESTING_ASSERT(Directory::Delete(sPath, true));
		SEOUL_UNITTESTING_ASSERT(!Directory::DirectoryExists(sPath));
	}

	// Multiple files recursive.
	{
		auto const sPath1(Path::Combine(sTempDir, "TestDir"));
		auto const sPath2(Path::Combine(sTempDir, "TestDir", "TestOuter"));

		// Cleanup in case of previous test failure.
		(void)Directory::Delete(sPath1, true);

		SEOUL_UNITTESTING_ASSERT(!Directory::DirectoryExists(sPath2));
		SEOUL_UNITTESTING_ASSERT(Directory::CreateDirPath(sPath2));
		SEOUL_UNITTESTING_ASSERT(Directory::DirectoryExists(sPath2));
		SEOUL_UNITTESTING_ASSERT(!DiskSyncFile::FileExists(sPath2));

		// Multiple files.
		static const UInt32 kuCount = 5u;
		Vector<Directory::DirEntryEx> vExpected;
		for (UInt32 i = 0u; i < kuCount; ++i)
		{
			Directory::DirEntryEx expected;
			expected.m_sFileName = Path::Combine(sPath1, String::Printf("TestFile%u", i));
			expected.m_uFileSize = i + 1;
			expected.m_uModifiedTime = WriteTestData(sPath1, String::Printf("TestFile%u", i), i + 1);
			vExpected.PushBack(expected);
		}
		for (UInt32 i = 0u; i < kuCount; ++i)
		{
			Directory::DirEntryEx expected;
			expected.m_sFileName = Path::Combine(sPath2, String::Printf("TestFile%u", i));
			expected.m_uFileSize = i + 1;
			expected.m_uModifiedTime = WriteTestData(sPath2, String::Printf("TestFile%u", i), i + 1);
			vExpected.PushBack(expected);
		}

		UInt32 uCount = 0u;
		SEOUL_UNITTESTING_ASSERT(Directory::GetDirectoryListingEx(sPath1, SEOUL_BIND_DELEGATE(Count, &uCount)));
		SEOUL_UNITTESTING_ASSERT_EQUAL(2 * kuCount, uCount);
		uCount = 0u;
		SEOUL_UNITTESTING_ASSERT(Directory::GetDirectoryListingEx(sPath2, SEOUL_BIND_DELEGATE(Count, &uCount)));
		SEOUL_UNITTESTING_ASSERT_EQUAL(kuCount, uCount);

		Vector<Directory::DirEntryEx> v;
		SEOUL_UNITTESTING_ASSERT(Directory::GetDirectoryListingEx(sPath1, SEOUL_BIND_DELEGATE(Append, &v)));
		SEOUL_UNITTESTING_ASSERT_EQUAL(vExpected.GetSize(), v.GetSize());
		ByFileName sorter;
		QuickSort(v.Begin(), v.End(), sorter);
		UInt32 i = 0u;
		for (auto const& expected : vExpected)
		{
			auto const& actual = v[i]; ++i;
			SEOUL_UNITTESTING_ASSERT_EQUAL(expected.m_sFileName, actual.m_sFileName);
			SEOUL_UNITTESTING_ASSERT_EQUAL(expected.m_uFileSize, actual.m_uFileSize);
			SEOUL_UNITTESTING_ASSERT_EQUAL(expected.m_uModifiedTime, actual.m_uModifiedTime);
		}

		SEOUL_UNITTESTING_ASSERT(Directory::Delete(sPath1, true));
		SEOUL_UNITTESTING_ASSERT(!Directory::DirectoryExists(sPath1));
	}
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
