/**
 * \file CachingDiskFileSystemTest.cpp
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "CachingDiskFileSystem.h"
#include "CachingDiskFileSystemTest.h"
#include "Directory.h"
#include "GamePaths.h"
#include "PseudoRandom.h"
#include "ReflectionDefine.h"
#include "ScopedAction.h"
#include "Thread.h"
#include "UnitTesting.h"
#include "UnitTestsFileManagerHelper.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(CachingDiskFileSystemTest, TypeFlags::kDisableCopy)
	SEOUL_ATTRIBUTE(UnitTest)
// TODO: Only PC implements the file change notifier
// behavior necessary for CachingDiskFileSystem to work
// as expected in these tests.
#if SEOUL_PLATFORM_WINDOWS
	SEOUL_METHOD(TestCopy)
	SEOUL_METHOD(TestCopyOverwrite)
	SEOUL_METHOD(TestCreateDirPath)
	SEOUL_METHOD(TestDeleteDirectory)
	SEOUL_METHOD(TestDeleteDirectoryRecursive)
	SEOUL_METHOD(TestDelete)
	SEOUL_METHOD(TestExists)
	SEOUL_METHOD(TestExistsForPlatform)
	SEOUL_METHOD(TestGetDirectoryListing)
	SEOUL_METHOD(TestGetDirectoryListingRobustness)
	SEOUL_METHOD(TestGetFileSize)
	SEOUL_METHOD(TestGetFileSizeForPlatform)
	SEOUL_METHOD(TestGetModifiedTime)
	SEOUL_METHOD(TestGetModifiedTimeForPlatform)
	SEOUL_METHOD(TestIsDirectory)
	SEOUL_METHOD(TestOpen)
	SEOUL_METHOD(TestOpenForPlatform)
	SEOUL_METHOD(TestRename)
	SEOUL_METHOD(TestReadAll)
	SEOUL_METHOD(TestReadAllForPlatform)
	SEOUL_METHOD(TestSetModifiedTime)
	SEOUL_METHOD(TestSetModifiedTimeForPlatform)
	SEOUL_METHOD(TestSetReadOnlyBit)
	SEOUL_METHOD(TestWriteAll)
	SEOUL_METHOD(TestWriteAllModifiedTime)
	SEOUL_METHOD(TestWriteAllForPlatform)
	SEOUL_METHOD(TestWriteAllForPlatformModifiedTime)
#endif // /SEOUL_PLATFORM_WINDOWS
SEOUL_END_TYPE()

extern String g_sUnitTestsBaseDirectoryPath;

CachingDiskFileSystemTest::CachingDiskFileSystemTest()
	: m_sOrig(g_sUnitTestsBaseDirectoryPath)
{
	auto const sDir(Path::Combine(Path::GetTempDirectory(), "CachingFileSystemTest"));
	g_sUnitTestsBaseDirectoryPath = Path::Combine(sDir, "Binaries", "PC", "Developer", "x64");

	m_pFileManager.Reset(SEOUL_NEW(MemoryBudgets::Io) UnitTestsFileManagerHelper);
	if (Directory::DirectoryExists(GamePaths::Get()->GetBaseDir()))
	{
		SEOUL_VERIFY(Directory::Delete(GamePaths::Get()->GetBaseDir(), true));
	}
	SEOUL_VERIFY(Directory::CreateDirPath(GamePaths::Get()->GetContentDir()));

	m_pExpected.Reset(SEOUL_NEW(MemoryBudgets::Io) DiskFileSystem);
	m_pFileSystem.Reset(SEOUL_NEW(MemoryBudgets::Io) CachingDiskFileSystem(keCurrentPlatform, GameDirectory::kContent));
}

CachingDiskFileSystemTest::~CachingDiskFileSystemTest()
{
	m_pFileSystem.Reset();
	m_pExpected.Reset();

	if (Directory::DirectoryExists(GamePaths::Get()->GetBaseDir()))
	{
		SEOUL_VERIFY(Directory::Delete(GamePaths::Get()->GetBaseDir(), true));
	}

	m_pFileManager.Reset();
	g_sUnitTestsBaseDirectoryPath = m_sOrig;
}

FilePath CachingDiskFileSystemTest::GetTestPath() const
{
	FilePath ret;
	ret.SetDirectory(GameDirectory::kContent);
	ret.SetRelativeFilenameWithoutExtension(FilePathRelativeFilename("test"));
	ret.SetType(FileType::kJson);
	return ret;
}

FilePath CachingDiskFileSystemTest::GetTestPathB() const
{
	FilePath ret;
	ret.SetDirectory(GameDirectory::kContent);
	ret.SetRelativeFilenameWithoutExtension(FilePathRelativeFilename("test2"));
	ret.SetType(FileType::kJson);
	return ret;
}

namespace
{

struct WaitForFileChange
{
	WaitForFileChange(const CachingDiskFileSystem& r)
		: m_StartCount(r.GetOnFileChangesCount())
		, m_r(r)
	{
	}

	~WaitForFileChange()
	{
		// We can't guarantee the exact timing of a file notification event
		// that will dirty the cache, so we wait for it to receive it.
		auto lastValue = m_r.GetOnFileChangesCount();
		while (m_StartCount == lastValue)
		{
			Thread::YieldToAnotherThread();
			lastValue = m_r.GetOnFileChangesCount();
		}

		// Now give a little bit for a series of changes - unfortunately, doesn't
		// seem to be a robust way to do this otherwise. Even Win32 will generally
		// emit REMOVE -> ADD events in sequence insetad of the expected RENAME event.
		auto iStart = SeoulTime::GetGameTimeInTicks();
		while (true)
		{
			auto const iTicks = SeoulTime::GetGameTimeInTicks();
			auto const val = m_r.GetOnFileChangesCount();
			if (val != lastValue)
			{
				lastValue = val;
				iStart = iTicks;
			}
			else if (SeoulTime::ConvertTicksToMilliseconds(iTicks - iStart) >= 1.0)
			{
				break;
			}

			Thread::YieldToAnotherThread();
		}
	}

	Atomic32Type const m_StartCount;
	const CachingDiskFileSystem& m_r;
};

} // namespace anonymous

void CachingDiskFileSystemTest::DeleteTestFile() const
{
	if (DiskSyncFile::FileExists(GetTestPath()))
	{
		WaitForFileChange wait(*m_pFileSystem);
		SEOUL_UNITTESTING_ASSERT(DiskSyncFile::DeleteFile(GetTestPath()));
	}
	else
	{
		SEOUL_UNITTESTING_ASSERT(Directory::Delete(GetTestPath().GetAbsoluteFilename(), false));
	}
}

void CachingDiskFileSystemTest::DeleteTestFileB() const
{
	if (DiskSyncFile::FileExists(GetTestPathB()))
	{
		WaitForFileChange wait(*m_pFileSystem);
		SEOUL_UNITTESTING_ASSERT(DiskSyncFile::DeleteFile(GetTestPathB()));
	}
	else
	{
		SEOUL_UNITTESTING_ASSERT(Directory::Delete(GetTestPathB().GetAbsoluteFilename(), false));
	}
}

void CachingDiskFileSystemTest::WriteTestDir() const
{
	SEOUL_UNITTESTING_ASSERT(Directory::CreateDirPath(GetTestPath().GetAbsoluteFilename()));
}

void CachingDiskFileSystemTest::WriteTestFile() const
{
	auto const k =
R"(
	{
		"test": true
	}
)";

	WaitForFileChange wait(*m_pFileSystem);
	SEOUL_UNITTESTING_ASSERT(DiskSyncFile::WriteAll(GetTestPath(), k, StrLen(k)));
}

void CachingDiskFileSystemTest::WriteTestFileB() const
{
	auto const k =
R"(
	{
		"testb": true
	}
)";

	WaitForFileChange wait(*m_pFileSystem);
	SEOUL_UNITTESTING_ASSERT(DiskSyncFile::WriteAll(GetTestPathB(), k, StrLen(k)));
}

void CachingDiskFileSystemTest::VerifyEqualImpl(FilePath filePath)
{
	auto const a = m_pExpected->Exists(filePath);
	auto const b = m_pFileSystem->Exists(filePath);
	SEOUL_UNITTESTING_ASSERT_EQUAL(a, b);
	for (Int i = 0; i < (Int)Platform::SEOUL_PLATFORM_COUNT; ++i)
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(m_pExpected->ExistsForPlatform((Platform)i, filePath), m_pFileSystem->ExistsForPlatform((Platform)i, filePath));
	}
	{
		Vector<String> vA, vB;
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			m_pExpected->GetDirectoryListing(filePath, vA),
			m_pFileSystem->GetDirectoryListing(filePath, vB));
		SEOUL_UNITTESTING_ASSERT_EQUAL(vA, vB);
	}
	{
		Vector<String> vA, vB;
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			m_pExpected->GetDirectoryListing(filePath, vA, false),
			m_pFileSystem->GetDirectoryListing(filePath, vB, false));
		SEOUL_UNITTESTING_ASSERT_EQUAL(vA, vB);
	}
	{
		Vector<String> vA, vB;
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			m_pExpected->GetDirectoryListing(filePath, vA, false, false),
			m_pFileSystem->GetDirectoryListing(filePath, vB, false, false));
		SEOUL_UNITTESTING_ASSERT_EQUAL(vA, vB);
	}
	{
		Vector<String> vA, vB;
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			m_pExpected->GetDirectoryListing(filePath, vA, false, false, ".json"),
			m_pFileSystem->GetDirectoryListing(filePath, vB, false, false, ".json"));
		SEOUL_UNITTESTING_ASSERT_EQUAL(vA, vB);
	}
	{
		Vector<String> vA, vB;
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			m_pExpected->GetDirectoryListing(filePath, vA, false, false, ".txt"),
			m_pFileSystem->GetDirectoryListing(filePath, vB, false, false, ".txt"));
		SEOUL_UNITTESTING_ASSERT_EQUAL(vA, vB);
	}
	SEOUL_UNITTESTING_ASSERT_EQUAL(m_pExpected->IsDirectory(filePath), m_pFileSystem->IsDirectory(filePath));
	{
		UInt64 uExpected = 0u, uTest = 0u;
		SEOUL_UNITTESTING_ASSERT_EQUAL(m_pExpected->GetFileSize(filePath, uExpected), m_pFileSystem->GetFileSize(filePath, uTest));
		SEOUL_UNITTESTING_ASSERT_EQUAL(uExpected, uTest);
		for (Int i = 0; i < (Int)Platform::SEOUL_PLATFORM_COUNT; ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(m_pExpected->GetFileSizeForPlatform((Platform)i, filePath, uExpected), m_pFileSystem->GetFileSizeForPlatform((Platform)i, filePath, uTest));
			SEOUL_UNITTESTING_ASSERT_EQUAL(uExpected, uTest);
		}

		// Caching file system is expected to not return modified time values for directories - it does not
		// track them and instead requires a fallback DiskFileSystem to be available and used. This
		// is critical for performance, since falling back to a real risk IO check for modified time
		// can be extremely slow in certain usage scenarios.
		if (m_pExpected->IsDirectory(filePath))
		{
			SEOUL_UNITTESTING_ASSERT(m_pExpected->GetModifiedTime(filePath, uExpected));
			SEOUL_UNITTESTING_ASSERT(!m_pFileSystem->GetModifiedTime(filePath, uTest));
			SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(0, uExpected);
			for (Int i = 0; i < (Int)Platform::SEOUL_PLATFORM_COUNT; ++i)
			{
				SEOUL_UNITTESTING_ASSERT_EQUAL(((Int)keCurrentPlatform == i), m_pExpected->GetModifiedTimeForPlatform((Platform)i, filePath, uExpected));
				SEOUL_UNITTESTING_ASSERT(!m_pFileSystem->GetModifiedTimeForPlatform((Platform)i, filePath, uTest));
				SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(0, uExpected);
			}
		}
		else
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(m_pExpected->GetModifiedTime(filePath, uExpected), m_pFileSystem->GetModifiedTime(filePath, uTest));
			SEOUL_UNITTESTING_ASSERT_EQUAL(uExpected, uTest);
			for (Int i = 0; i < (Int)Platform::SEOUL_PLATFORM_COUNT; ++i)
			{
				SEOUL_UNITTESTING_ASSERT_EQUAL(m_pExpected->GetModifiedTimeForPlatform((Platform)i, filePath, uExpected), m_pFileSystem->GetModifiedTimeForPlatform((Platform)i, filePath, uTest));
				SEOUL_UNITTESTING_ASSERT_EQUAL(uExpected, uTest);
			}
		}
	}
	{
		void* pExpected = nullptr;
		UInt32 uExpected = 0u;
		void* pTest = nullptr;
		UInt32 uTest = 0u;
		auto const deferred(MakeDeferredAction([&]() { MemoryManager::Deallocate(pTest); MemoryManager::Deallocate(pExpected); }));
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			m_pExpected->ReadAll(filePath, pExpected, uExpected, 0u, MemoryBudgets::Developer),
			m_pFileSystem->ReadAll(filePath, pTest, uTest, 0u, MemoryBudgets::Developer));
		SEOUL_UNITTESTING_ASSERT_EQUAL(uExpected, uTest);
		SEOUL_UNITTESTING_ASSERT(0 == memcmp(pExpected, pTest, uExpected));
	}
	for (Int i = 0; i < (Int)Platform::SEOUL_PLATFORM_COUNT; ++i)
	{
		void* pExpected = nullptr;
		UInt32 uExpected = 0u;
		void* pTest = nullptr;
		UInt32 uTest = 0u;
		auto const deferred(MakeDeferredAction([&]() { MemoryManager::Deallocate(pTest); MemoryManager::Deallocate(pExpected); }));
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			m_pExpected->ReadAllForPlatform((Platform)i, filePath, pExpected, uExpected, 0u, MemoryBudgets::Developer),
			m_pFileSystem->ReadAllForPlatform((Platform)i, filePath, pTest, uTest, 0u, MemoryBudgets::Developer));
		SEOUL_UNITTESTING_ASSERT_EQUAL(uExpected, uTest);
		SEOUL_UNITTESTING_ASSERT(0 == memcmp(pExpected, pTest, uExpected));
	}
}

void CachingDiskFileSystemTest::VerifyEqualImpl(const String& sAbsoluteFilename)
{
	SEOUL_UNITTESTING_ASSERT_EQUAL(m_pExpected->Exists(sAbsoluteFilename), m_pFileSystem->Exists(sAbsoluteFilename));
	SEOUL_UNITTESTING_ASSERT_EQUAL(m_pExpected->IsDirectory(sAbsoluteFilename), m_pFileSystem->IsDirectory(sAbsoluteFilename));
	{
		UInt64 uExpected = 0u, uTest = 0u;
		SEOUL_UNITTESTING_ASSERT_EQUAL(m_pExpected->GetFileSize(sAbsoluteFilename, uExpected), m_pFileSystem->GetFileSize(sAbsoluteFilename, uTest));
		SEOUL_UNITTESTING_ASSERT_EQUAL(uExpected, uTest);

		// Caching file system is expected to not return modified time values for directories - it does not
		// track them and instead requires a fallback DiskFileSystem to be available and used. This
		// is critical for performance, since falling back to a real risk IO check for modified time
		// can be extremely slow in certain usage scenarios.
		if (m_pExpected->IsDirectory(sAbsoluteFilename))
		{
			SEOUL_UNITTESTING_ASSERT(m_pExpected->GetModifiedTime(sAbsoluteFilename, uExpected));
			SEOUL_UNITTESTING_ASSERT(!m_pFileSystem->GetModifiedTime(sAbsoluteFilename, uTest));
			SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(0, uExpected);
		}
		else
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(m_pExpected->GetModifiedTime(sAbsoluteFilename, uExpected), m_pFileSystem->GetModifiedTime(sAbsoluteFilename, uTest));
			SEOUL_UNITTESTING_ASSERT_EQUAL(uExpected, uTest);
		}
	}
	{
		void* pExpected = nullptr;
		UInt32 uExpected = 0u;
		void* pTest = nullptr;
		UInt32 uTest = 0u;
		auto const deferred(MakeDeferredAction([&]() { MemoryManager::Deallocate(pTest); MemoryManager::Deallocate(pExpected); }));
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			m_pExpected->ReadAll(sAbsoluteFilename, pExpected, uExpected, 0u, MemoryBudgets::Developer),
			m_pFileSystem->ReadAll(sAbsoluteFilename, pTest, uTest, 0u, MemoryBudgets::Developer));
		SEOUL_UNITTESTING_ASSERT_EQUAL(uExpected, uTest);
		SEOUL_UNITTESTING_ASSERT(0 == memcmp(pExpected, pTest, uExpected));
	}
}

void CachingDiskFileSystemTest::VerifyEqual(FilePath filePath)
{
	VerifyEqualImpl(filePath);
	VerifyEqualImpl(filePath.GetAbsoluteFilename());
}

void CachingDiskFileSystemTest::VerifyEqual(const String& sAbsoluteFilename)
{
	VerifyEqualImpl(sAbsoluteFilename);
	VerifyEqualImpl(FilePath::CreateContentFilePath(sAbsoluteFilename));
}

void CachingDiskFileSystemTest::TestCopy()
{
	WriteTestFile();

	// Cache copy
	{
		SEOUL_UNITTESTING_ASSERT(m_pFileSystem->Copy(GetTestPath().GetAbsoluteFilename(), GetTestPathB().GetAbsoluteFilename()));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		DeleteTestFileB();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
	}
	// Cache copy
	{
		SEOUL_UNITTESTING_ASSERT(m_pFileSystem->Copy(GetTestPath(), GetTestPathB()));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		DeleteTestFileB();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
	}
	// Disk copy
	{
		SEOUL_UNITTESTING_ASSERT(m_pExpected->Copy(GetTestPath().GetAbsoluteFilename(), GetTestPathB().GetAbsoluteFilename()));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		DeleteTestFileB();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
	}
	// Disk copy
	{
		SEOUL_UNITTESTING_ASSERT(m_pExpected->Copy(GetTestPath(), GetTestPathB()));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		DeleteTestFileB();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
	}

	DeleteTestFile();
	VerifyEqual(GetTestPath());
	VerifyEqual(GetTestPathB());
}

void CachingDiskFileSystemTest::TestCopyOverwrite()
{
	WriteTestFile();

	// Cache copy
	{
		WriteTestFileB();
		SEOUL_UNITTESTING_ASSERT(!m_pFileSystem->Copy(GetTestPath().GetAbsoluteFilename(), GetTestPathB().GetAbsoluteFilename()));
		SEOUL_UNITTESTING_ASSERT(m_pFileSystem->Copy(GetTestPath().GetAbsoluteFilename(), GetTestPathB().GetAbsoluteFilename(), true));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		DeleteTestFileB();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
	}
	// Cache copy
	{
		WriteTestFileB();
		SEOUL_UNITTESTING_ASSERT(!m_pFileSystem->Copy(GetTestPath(), GetTestPathB()));
		SEOUL_UNITTESTING_ASSERT(m_pFileSystem->Copy(GetTestPath(), GetTestPathB(), true));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		DeleteTestFileB();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
	}
	// Disk copy
	{
		WriteTestFileB();
		SEOUL_UNITTESTING_ASSERT(!m_pExpected->Copy(GetTestPath().GetAbsoluteFilename(), GetTestPathB().GetAbsoluteFilename()));
		SEOUL_UNITTESTING_ASSERT(m_pExpected->Copy(GetTestPath().GetAbsoluteFilename(), GetTestPathB().GetAbsoluteFilename(), true));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		DeleteTestFileB();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
	}
	// Disk copy
	{
		WriteTestFileB();
		SEOUL_UNITTESTING_ASSERT(!m_pExpected->Copy(GetTestPath(), GetTestPathB()));
		SEOUL_UNITTESTING_ASSERT(m_pExpected->Copy(GetTestPath(), GetTestPathB(), true));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		DeleteTestFileB();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
	}

	DeleteTestFile();
	VerifyEqual(GetTestPath());
	VerifyEqual(GetTestPathB());
}

void CachingDiskFileSystemTest::TestCreateDirPath()
{
	// Cache create
	{
		SEOUL_UNITTESTING_ASSERT(m_pFileSystem->CreateDirPath(GetTestPath().GetAbsoluteFilename()));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		SEOUL_UNITTESTING_ASSERT(Directory::Delete(GetTestPath().GetAbsoluteFilename(), false));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
	}
	// Cache create
	{
		SEOUL_UNITTESTING_ASSERT(m_pFileSystem->CreateDirPath(GetTestPath()));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		SEOUL_UNITTESTING_ASSERT(Directory::Delete(GetTestPath().GetAbsoluteFilename(), false));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
	}
	// Disk create
	{
		SEOUL_UNITTESTING_ASSERT(m_pExpected->CreateDirPath(GetTestPath().GetAbsoluteFilename()));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		SEOUL_UNITTESTING_ASSERT(Directory::Delete(GetTestPath().GetAbsoluteFilename(), false));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
	}
	// Disk create
	{
		SEOUL_UNITTESTING_ASSERT(m_pExpected->CreateDirPath(GetTestPath()));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		SEOUL_UNITTESTING_ASSERT(Directory::Delete(GetTestPath().GetAbsoluteFilename(), false));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
	}
}

void CachingDiskFileSystemTest::TestDeleteDirectory()
{
	// Cache delete
	{
		SEOUL_UNITTESTING_ASSERT(Directory::CreateDirPath(GetTestPath().GetAbsoluteFilename()));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		SEOUL_UNITTESTING_ASSERT(m_pFileSystem->DeleteDirectory(GetTestPath().GetAbsoluteFilename(), false));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
	}
	// Cache delete
	{
		SEOUL_UNITTESTING_ASSERT(Directory::CreateDirPath(GetTestPath().GetAbsoluteFilename()));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		SEOUL_UNITTESTING_ASSERT(m_pFileSystem->DeleteDirectory(GetTestPath(), false));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
	}
	// Disk delete
	{
		SEOUL_UNITTESTING_ASSERT(Directory::CreateDirPath(GetTestPath().GetAbsoluteFilename()));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		SEOUL_UNITTESTING_ASSERT(m_pExpected->DeleteDirectory(GetTestPath().GetAbsoluteFilename(), false));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
	}
	// Disk delete
	{
		SEOUL_UNITTESTING_ASSERT(Directory::CreateDirPath(GetTestPath().GetAbsoluteFilename()));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		SEOUL_UNITTESTING_ASSERT(m_pExpected->DeleteDirectory(GetTestPath(), false));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
	}
}

void CachingDiskFileSystemTest::TestDeleteDirectoryRecursive()
{
	// Cache delete
	{
		SEOUL_UNITTESTING_ASSERT(Directory::CreateDirPath(GetTestPath().GetAbsoluteFilename()));
		WriteTestFileB();
		{
			WaitForFileChange wait(*m_pFileSystem);
			SEOUL_UNITTESTING_ASSERT(DiskSyncFile::RenameFile(GetTestPathB().GetAbsoluteFilename(), Path::Combine(GetTestPath().GetAbsoluteFilename(), "testc.json")));
		}
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		SEOUL_UNITTESTING_ASSERT(m_pFileSystem->DeleteDirectory(GetTestPath().GetAbsoluteFilename(), true));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
	}
	// Cache delete
	{
		SEOUL_UNITTESTING_ASSERT(Directory::CreateDirPath(GetTestPath().GetAbsoluteFilename()));
		WriteTestFileB();
		{
			WaitForFileChange wait(*m_pFileSystem);
			SEOUL_UNITTESTING_ASSERT(DiskSyncFile::RenameFile(GetTestPathB().GetAbsoluteFilename(), Path::Combine(GetTestPath().GetAbsoluteFilename(), "testc.json")));
		}
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		SEOUL_UNITTESTING_ASSERT(m_pFileSystem->DeleteDirectory(GetTestPath(), true));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
	}
	// Disk delete
	{
		SEOUL_UNITTESTING_ASSERT(Directory::CreateDirPath(GetTestPath().GetAbsoluteFilename()));
		WriteTestFileB();
		{
			WaitForFileChange wait(*m_pFileSystem);
			SEOUL_UNITTESTING_ASSERT(DiskSyncFile::RenameFile(GetTestPathB().GetAbsoluteFilename(), Path::Combine(GetTestPath().GetAbsoluteFilename(), "testc.json")));
		}
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		SEOUL_UNITTESTING_ASSERT(m_pExpected->DeleteDirectory(GetTestPath().GetAbsoluteFilename(), true));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
	}
	// Disk delete
	{
		SEOUL_UNITTESTING_ASSERT(Directory::CreateDirPath(GetTestPath().GetAbsoluteFilename()));
		WriteTestFileB();
		{
			WaitForFileChange wait(*m_pFileSystem);
			SEOUL_UNITTESTING_ASSERT(DiskSyncFile::RenameFile(GetTestPathB().GetAbsoluteFilename(), Path::Combine(GetTestPath().GetAbsoluteFilename(), "testc.json")));
		}
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		SEOUL_UNITTESTING_ASSERT(m_pExpected->DeleteDirectory(GetTestPath(), true));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
	}
}

void CachingDiskFileSystemTest::TestDelete()
{
	// Cache delete
	{
		WriteTestFile();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		SEOUL_UNITTESTING_ASSERT(m_pFileSystem->Delete(GetTestPath().GetAbsoluteFilename()));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
	}
	// Cache delete
	{
		WriteTestFile();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		SEOUL_UNITTESTING_ASSERT(m_pFileSystem->Delete(GetTestPath()));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
	}
	// Disk delete
	{
		WriteTestFile();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		{
			WaitForFileChange wait(*m_pFileSystem);
			SEOUL_UNITTESTING_ASSERT(m_pExpected->Delete(GetTestPath().GetAbsoluteFilename()));
		}
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
	}
	// Disk delete
	{
		WriteTestFile();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		{
			WaitForFileChange wait(*m_pFileSystem);
			SEOUL_UNITTESTING_ASSERT(m_pExpected->Delete(GetTestPath()));
		}
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
	}
}

void CachingDiskFileSystemTest::TestExists()
{
	// Cache exists
	{
		WriteTestFile();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		SEOUL_UNITTESTING_ASSERT(m_pFileSystem->Exists(GetTestPath().GetAbsoluteFilename()));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		DeleteTestFile();
	}
	// Cache exists
	{
		WriteTestFile();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		SEOUL_UNITTESTING_ASSERT(m_pFileSystem->Exists(GetTestPath()));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		DeleteTestFile();
	}
	// Disk exists
	{
		WriteTestFile();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		SEOUL_UNITTESTING_ASSERT(m_pExpected->Exists(GetTestPath().GetAbsoluteFilename()));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		DeleteTestFile();
	}
	// Disk exists
	{
		WriteTestFile();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		SEOUL_UNITTESTING_ASSERT(m_pExpected->Exists(GetTestPath()));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		DeleteTestFile();
	}
}

void CachingDiskFileSystemTest::TestExistsForPlatform()
{
	// Cache exists
	{
		WriteTestFile();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		for (Int i = 0; i < (Int)Platform::SEOUL_PLATFORM_COUNT; ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(i == (Int)keCurrentPlatform, m_pFileSystem->ExistsForPlatform((Platform)i, GetTestPath()));
			VerifyEqual(GetTestPath());
			VerifyEqual(GetTestPathB());
		}
		DeleteTestFile();
	}
	// Disk exists
	{
		WriteTestFile();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		for (Int i = 0; i < (Int)Platform::SEOUL_PLATFORM_COUNT; ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(i == (Int)keCurrentPlatform, m_pExpected->ExistsForPlatform((Platform)i, GetTestPath()));
			VerifyEqual(GetTestPath());
			VerifyEqual(GetTestPathB());
		}
		DeleteTestFile();
	}
}

void CachingDiskFileSystemTest::TestGetDirectoryListing()
{
	Vector<String> v;

	// Cache listing
	{
		SEOUL_UNITTESTING_ASSERT(Directory::CreateDirPath(GetTestPath().GetAbsoluteFilename()));
		WriteTestFileB();
		SEOUL_UNITTESTING_ASSERT(DiskSyncFile::RenameFile(GetTestPathB().GetAbsoluteFilename(), Path::Combine(GetTestPath().GetAbsoluteFilename(), "testc.json")));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		SEOUL_UNITTESTING_ASSERT(m_pFileSystem->GetDirectoryListing(GetTestPath().GetAbsoluteFilename(), v));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, v.GetSize());
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		SEOUL_UNITTESTING_ASSERT(Directory::Delete(GetTestPath().GetAbsoluteFilename(), true));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
	}
	// Cache listing
	{
		SEOUL_UNITTESTING_ASSERT(Directory::CreateDirPath(GetTestPath().GetAbsoluteFilename()));
		WriteTestFileB();
		SEOUL_UNITTESTING_ASSERT(DiskSyncFile::RenameFile(GetTestPathB().GetAbsoluteFilename(), Path::Combine(GetTestPath().GetAbsoluteFilename(), "testc.json")));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		SEOUL_UNITTESTING_ASSERT(m_pFileSystem->GetDirectoryListing(GetTestPath().GetAbsoluteFilename(), v));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, v.GetSize());
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		SEOUL_UNITTESTING_ASSERT(Directory::Delete(GetTestPath().GetAbsoluteFilename(), true));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
	}
	// Disk listing
	{
		SEOUL_UNITTESTING_ASSERT(Directory::CreateDirPath(GetTestPath().GetAbsoluteFilename()));
		WriteTestFileB();
		SEOUL_UNITTESTING_ASSERT(DiskSyncFile::RenameFile(GetTestPathB().GetAbsoluteFilename(), Path::Combine(GetTestPath().GetAbsoluteFilename(), "testc.json")));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		SEOUL_UNITTESTING_ASSERT(m_pFileSystem->GetDirectoryListing(GetTestPath().GetAbsoluteFilename(), v));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, v.GetSize());
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		SEOUL_UNITTESTING_ASSERT(Directory::Delete(GetTestPath().GetAbsoluteFilename(), true));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
	}
	// Disk listing
	{
		SEOUL_UNITTESTING_ASSERT(Directory::CreateDirPath(GetTestPath().GetAbsoluteFilename()));
		WriteTestFileB();
		SEOUL_UNITTESTING_ASSERT(DiskSyncFile::RenameFile(GetTestPathB().GetAbsoluteFilename(), Path::Combine(GetTestPath().GetAbsoluteFilename(), "testc.json")));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		SEOUL_UNITTESTING_ASSERT(m_pFileSystem->GetDirectoryListing(GetTestPath().GetAbsoluteFilename(), v));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, v.GetSize());
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		SEOUL_UNITTESTING_ASSERT(Directory::Delete(GetTestPath().GetAbsoluteFilename(), true));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
	}
}

void CachingDiskFileSystemTest::TestGetDirectoryListingRobustness()
{
	// Use random generator for convenience, seeding so
	// behavior is deterministic and repeatable.
	PseudoRandom random(PseudoRandomSeed(0xD3E3C425A47E911F, 0xEDC11D7A3A01D1E8));
	for (Int i = 0; i < 277; ++i)
	{
		if (i % 64 == 0)
		{
			m_pFileSystem->DeleteDirectory(Path::Combine(
				GamePaths::Get()->GetContentDir(),
				"b", "a"), true);
		}
		if (i % 128 == 1)
		{
			m_pFileSystem->DeleteDirectory(Path::Combine(
				GamePaths::Get()->GetContentDir(),
				"a", "b"), true);
		}
		if (i % 256 == 2)
		{
			m_pFileSystem->DeleteDirectory(Path::Combine(
				GamePaths::Get()->GetContentDir(),
				"a"), true);
		}

		String sFilename(ToString(random.UniformRandomUInt64()));
		if (i % 2 == 0)
		{
			sFilename = Path::Combine("a", sFilename);
		}
		if (i % 4 == 1)
		{
			sFilename = Path::Combine("b", "a", sFilename);
		}
		if (i % 8 == 2)
		{
			sFilename = Path::Combine("a", "b", sFilename);
		}
		if (i % 8 == 3)
		{
			sFilename = Path::Combine("a", "b", "c", sFilename);
		}

		FileType eFileType = FileType::kUnknown;
		if (i % 4 == 0)
		{
			eFileType = FileType::kCsv;
		}
		else if (i % 8 == 1)
		{
			eFileType = FileType::kUnknown;
		}
		else
		{
			eFileType = FileType::kJson;
		}

		// Put in a specific folder so we can clean at end.
		sFilename = Path::Combine("robust", sFilename);

		FilePath filePath;
		filePath.SetDirectory(GameDirectory::kContent);
		filePath.SetRelativeFilenameWithoutExtension(FilePathRelativeFilename(sFilename));
		filePath.SetType(eFileType);
		SEOUL_UNITTESTING_ASSERT(m_pFileSystem->WriteAll(filePath, sFilename.CStr(), sFilename.GetSize()));

		FilePath root;
		root.SetDirectory(GameDirectory::kContent);

		for (auto const& sExt : { "", ".json" })
		{
			Vector<String> vA, vB;
			SEOUL_UNITTESTING_ASSERT_EQUAL(
				m_pExpected->GetDirectoryListing(root, vA, false, true, sExt),
				m_pFileSystem->GetDirectoryListing(root, vB, false, true, sExt));
			if (sExt == String())
			{
				SEOUL_UNITTESTING_ASSERT_LESS_THAN(0u, vA.GetSize());
				SEOUL_UNITTESTING_ASSERT_LESS_THAN(0u, vB.GetSize());
			}
			SEOUL_UNITTESTING_ASSERT_EQUAL(vA.GetSize(), vB.GetSize());
			for (UInt i = 0; i < vA.GetSize(); ++i)
			{
				SEOUL_UNITTESTING_ASSERT_EQUAL(vA[i], vB[i]);
			}
		}
	}

	m_pFileSystem->DeleteDirectory(Path::Combine(GamePaths::Get()->GetContentDir(), "robust"), true);
}

void CachingDiskFileSystemTest::TestGetFileSize()
{
	// Cache file size
	{
		WriteTestFile();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		UInt64 uFileSize = 1u;
		SEOUL_UNITTESTING_ASSERT(m_pFileSystem->GetFileSize(GetTestPath().GetAbsoluteFilename(), uFileSize));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		DeleteTestFile();
	}
	// Cache file size
	{
		WriteTestFile();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		UInt64 uFileSize = 1u;
		SEOUL_UNITTESTING_ASSERT(m_pFileSystem->GetFileSize(GetTestPath(), uFileSize));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		DeleteTestFile();
	}
	// Disk file size
	{
		WriteTestFile();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		UInt64 uFileSize = 1u;
		SEOUL_UNITTESTING_ASSERT(m_pExpected->GetFileSize(GetTestPath().GetAbsoluteFilename(), uFileSize));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		DeleteTestFile();
	}
	// Disk file size
	{
		WriteTestFile();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		UInt64 uFileSize = 1u;
		SEOUL_UNITTESTING_ASSERT(m_pExpected->GetFileSize(GetTestPath(), uFileSize));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		DeleteTestFile();
	}
}

void CachingDiskFileSystemTest::TestGetFileSizeForPlatform()
{
	// Cache file size
	{
		WriteTestFile();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		UInt64 uFileSize = 1u;
		for (Int i = 0; i < (Int)Platform::SEOUL_PLATFORM_COUNT; ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(i == (Int)keCurrentPlatform, m_pFileSystem->GetFileSizeForPlatform((Platform)i, GetTestPath(), uFileSize));
			VerifyEqual(GetTestPath());
			VerifyEqual(GetTestPathB());
		}
		DeleteTestFile();
	}
	// Disk file size
	{
		WriteTestFile();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		UInt64 uFileSize = 1u;
		for (Int i = 0; i < (Int)Platform::SEOUL_PLATFORM_COUNT; ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(i == (Int)keCurrentPlatform, m_pExpected->GetFileSizeForPlatform((Platform)i, GetTestPath(), uFileSize));
			VerifyEqual(GetTestPath());
			VerifyEqual(GetTestPathB());
		}
		DeleteTestFile();
	}
}

void CachingDiskFileSystemTest::TestGetModifiedTime()
{
	// Cache modified time
	{
		WriteTestFile();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		UInt64 uModifiedTime = 1u;
		SEOUL_UNITTESTING_ASSERT(m_pFileSystem->GetModifiedTime(GetTestPath().GetAbsoluteFilename(), uModifiedTime));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		DeleteTestFile();
	}
	// Cache modified time
	{
		WriteTestFile();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		UInt64 uModifiedTime = 1u;
		SEOUL_UNITTESTING_ASSERT(m_pFileSystem->GetModifiedTime(GetTestPath(), uModifiedTime));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		DeleteTestFile();
	}
	// Disk modified time
	{
		WriteTestFile();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		UInt64 uModifiedTime = 1u;
		SEOUL_UNITTESTING_ASSERT(m_pExpected->GetModifiedTime(GetTestPath().GetAbsoluteFilename(), uModifiedTime));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		DeleteTestFile();
	}
	// Disk modified time
	{
		WriteTestFile();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		UInt64 uModifiedTime = 1u;
		SEOUL_UNITTESTING_ASSERT(m_pExpected->GetModifiedTime(GetTestPath(), uModifiedTime));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		DeleteTestFile();
	}
}

void CachingDiskFileSystemTest::TestGetModifiedTimeForPlatform()
{
	// Cache modified time
	{
		WriteTestFile();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		UInt64 uModifiedTime = 1u;
		for (Int i = 0; i < (Int)Platform::SEOUL_PLATFORM_COUNT; ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(i == (Int)keCurrentPlatform, m_pFileSystem->GetModifiedTimeForPlatform((Platform)i, GetTestPath(), uModifiedTime));
			VerifyEqual(GetTestPath());
			VerifyEqual(GetTestPathB());
		}
		DeleteTestFile();
	}
	// Disk modified time
	{
		WriteTestFile();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		UInt64 uModifiedTime = 1u;
		for (Int i = 0; i < (Int)Platform::SEOUL_PLATFORM_COUNT; ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(i == (Int)keCurrentPlatform, m_pExpected->GetModifiedTimeForPlatform((Platform)i, GetTestPath(), uModifiedTime));
			VerifyEqual(GetTestPath());
			VerifyEqual(GetTestPathB());
		}
		DeleteTestFile();
	}
}

void CachingDiskFileSystemTest::TestIsDirectory()
{
	// Cache is file (not directory)
	{
		WriteTestFile();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		SEOUL_UNITTESTING_ASSERT(!m_pFileSystem->IsDirectory(GetTestPath().GetAbsoluteFilename()));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		DeleteTestFile();
	}
	// Cache is directory
	{
		WriteTestDir();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		SEOUL_UNITTESTING_ASSERT(m_pFileSystem->IsDirectory(GetTestPath().GetAbsoluteFilename()));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		DeleteTestFile();
	}
	// Cache is file (not directory)
	{
		WriteTestFile();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		SEOUL_UNITTESTING_ASSERT(!m_pFileSystem->IsDirectory(GetTestPath()));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		DeleteTestFile();
	}
	// Cache is directory
	{
		WriteTestDir();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		SEOUL_UNITTESTING_ASSERT(m_pFileSystem->IsDirectory(GetTestPath()));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		DeleteTestFile();
	}
	// Disk is file (not directory)
	{
		WriteTestFile();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		SEOUL_UNITTESTING_ASSERT(!m_pExpected->IsDirectory(GetTestPath().GetAbsoluteFilename()));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		DeleteTestFile();
	}
	// Disk is directory
	{
		WriteTestDir();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		SEOUL_UNITTESTING_ASSERT(m_pExpected->IsDirectory(GetTestPath().GetAbsoluteFilename()));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		DeleteTestFile();
	}
	// Disk is file (not directory)
	{
		WriteTestFile();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		SEOUL_UNITTESTING_ASSERT(!m_pExpected->IsDirectory(GetTestPath()));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		DeleteTestFile();
	}
	// Disk is directory
	{
		WriteTestDir();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		SEOUL_UNITTESTING_ASSERT(m_pExpected->IsDirectory(GetTestPath()));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		DeleteTestFile();
	}
}

void CachingDiskFileSystemTest::TestOpen()
{
	// Cache open
	{
		Int64 iPosition = -1;
		WriteTestFile();
		ScopedPtr<SyncFile> pFile;
		SEOUL_UNITTESTING_ASSERT(m_pFileSystem->Open(GetTestPath().GetAbsoluteFilename(), File::kRead, pFile));
		SEOUL_UNITTESTING_ASSERT_EQUAL(GetTestPath().GetAbsoluteFilename(), pFile->GetAbsoluteFilename());
		SEOUL_UNITTESTING_ASSERT_EQUAL(DiskSyncFile::GetFileSize(GetTestPath().GetAbsoluteFilename()), pFile->GetSize());
		SEOUL_UNITTESTING_ASSERT(pFile->IsOpen());
		SEOUL_UNITTESTING_ASSERT(pFile->CanRead());
		SEOUL_UNITTESTING_ASSERT(pFile->CanSeek());
		SEOUL_UNITTESTING_ASSERT(!pFile->CanWrite());
		SEOUL_UNITTESTING_ASSERT(pFile->GetCurrentPositionIndicator(iPosition));
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, iPosition);
		SEOUL_UNITTESTING_ASSERT(pFile->Seek(0, File::kSeekFromStart));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		pFile.Reset();
		DeleteTestFile();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
	}
	// Cache open
	{
		Int64 iPosition = -1;
		WriteTestFile();
		ScopedPtr<SyncFile> pFile;
		SEOUL_UNITTESTING_ASSERT(m_pFileSystem->Open(GetTestPath(), File::kRead, pFile));
		SEOUL_UNITTESTING_ASSERT_EQUAL(GetTestPath().GetAbsoluteFilename(), pFile->GetAbsoluteFilename());
		SEOUL_UNITTESTING_ASSERT_EQUAL(DiskSyncFile::GetFileSize(GetTestPath()), pFile->GetSize());
		SEOUL_UNITTESTING_ASSERT(pFile->IsOpen());
		SEOUL_UNITTESTING_ASSERT(pFile->CanRead());
		SEOUL_UNITTESTING_ASSERT(pFile->CanSeek());
		SEOUL_UNITTESTING_ASSERT(!pFile->CanWrite());
		SEOUL_UNITTESTING_ASSERT(pFile->GetCurrentPositionIndicator(iPosition));
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, iPosition);
		SEOUL_UNITTESTING_ASSERT(pFile->Seek(0, File::kSeekFromStart));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		pFile.Reset();
		DeleteTestFile();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
	}
	// Disk open
	{
		Int64 iPosition = -1;
		WriteTestFile();
		ScopedPtr<SyncFile> pFile;
		SEOUL_UNITTESTING_ASSERT(m_pExpected->Open(GetTestPath().GetAbsoluteFilename(), File::kRead, pFile));
		SEOUL_UNITTESTING_ASSERT_EQUAL(GetTestPath().GetAbsoluteFilename(), pFile->GetAbsoluteFilename());
		SEOUL_UNITTESTING_ASSERT_EQUAL(DiskSyncFile::GetFileSize(GetTestPath().GetAbsoluteFilename()), pFile->GetSize());
		SEOUL_UNITTESTING_ASSERT(pFile->IsOpen());
		SEOUL_UNITTESTING_ASSERT(pFile->CanRead());
		SEOUL_UNITTESTING_ASSERT(pFile->CanSeek());
		SEOUL_UNITTESTING_ASSERT(!pFile->CanWrite());
		SEOUL_UNITTESTING_ASSERT(pFile->GetCurrentPositionIndicator(iPosition));
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, iPosition);
		SEOUL_UNITTESTING_ASSERT(pFile->Seek(0, File::kSeekFromStart));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		pFile.Reset();
		DeleteTestFile();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
	}
	// Disk open
	{
		Int64 iPosition = -1;
		WriteTestFile();
		ScopedPtr<SyncFile> pFile;
		SEOUL_UNITTESTING_ASSERT(m_pExpected->Open(GetTestPath(), File::kRead, pFile));
		SEOUL_UNITTESTING_ASSERT_EQUAL(GetTestPath().GetAbsoluteFilename(), pFile->GetAbsoluteFilename());
		SEOUL_UNITTESTING_ASSERT_EQUAL(DiskSyncFile::GetFileSize(GetTestPath()), pFile->GetSize());
		SEOUL_UNITTESTING_ASSERT(pFile->IsOpen());
		SEOUL_UNITTESTING_ASSERT(pFile->CanRead());
		SEOUL_UNITTESTING_ASSERT(pFile->CanSeek());
		SEOUL_UNITTESTING_ASSERT(!pFile->CanWrite());
		SEOUL_UNITTESTING_ASSERT(pFile->GetCurrentPositionIndicator(iPosition));
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, iPosition);
		SEOUL_UNITTESTING_ASSERT(pFile->Seek(0, File::kSeekFromStart));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		pFile.Reset();
		DeleteTestFile();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
	}
}

void CachingDiskFileSystemTest::TestOpenForPlatform()
{
	// Cache read all
	for (Int i = 0; i < (Int)Platform::SEOUL_PLATFORM_COUNT; ++i)
	{
		Int64 iPosition = -1;
		WriteTestFile();
		ScopedPtr<SyncFile> pFile;
		if ((Int)keCurrentPlatform == i)
		{
			SEOUL_UNITTESTING_ASSERT(m_pFileSystem->OpenForPlatform((Platform)i, GetTestPath(), File::kRead, pFile));
			SEOUL_UNITTESTING_ASSERT_EQUAL(GetTestPath().GetAbsoluteFilename(), pFile->GetAbsoluteFilename());
			SEOUL_UNITTESTING_ASSERT_EQUAL(DiskSyncFile::GetFileSize(GetTestPath()), pFile->GetSize());
			SEOUL_UNITTESTING_ASSERT(pFile->IsOpen());
			SEOUL_UNITTESTING_ASSERT(pFile->CanRead());
			SEOUL_UNITTESTING_ASSERT(pFile->CanSeek());
			SEOUL_UNITTESTING_ASSERT(!pFile->CanWrite());
			SEOUL_UNITTESTING_ASSERT(pFile->GetCurrentPositionIndicator(iPosition));
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, iPosition);
			SEOUL_UNITTESTING_ASSERT(pFile->Seek(0, File::kSeekFromStart));
			VerifyEqual(GetTestPath());
			VerifyEqual(GetTestPathB());
			pFile.Reset();
			DeleteTestFile();
			VerifyEqual(GetTestPath());
			VerifyEqual(GetTestPathB());
		}
		else
		{
			SEOUL_UNITTESTING_ASSERT(!m_pFileSystem->OpenForPlatform((Platform)i, GetTestPath(), File::kRead, pFile));
		}
	}
	// Disk read all
	for (Int i = 0; i < (Int)Platform::SEOUL_PLATFORM_COUNT; ++i)
	{
		Int64 iPosition = -1;
		WriteTestFile();
		ScopedPtr<SyncFile> pFile;
		if ((Int)keCurrentPlatform == i)
		{
			SEOUL_UNITTESTING_ASSERT(m_pExpected->OpenForPlatform((Platform)i, GetTestPath(), File::kRead, pFile));
			SEOUL_UNITTESTING_ASSERT_EQUAL(GetTestPath().GetAbsoluteFilename(), pFile->GetAbsoluteFilename());
			SEOUL_UNITTESTING_ASSERT_EQUAL(DiskSyncFile::GetFileSize(GetTestPath()), pFile->GetSize());
			SEOUL_UNITTESTING_ASSERT(pFile->IsOpen());
			SEOUL_UNITTESTING_ASSERT(pFile->CanRead());
			SEOUL_UNITTESTING_ASSERT(pFile->CanSeek());
			SEOUL_UNITTESTING_ASSERT(!pFile->CanWrite());
			SEOUL_UNITTESTING_ASSERT(pFile->GetCurrentPositionIndicator(iPosition));
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, iPosition);
			SEOUL_UNITTESTING_ASSERT(pFile->Seek(0, File::kSeekFromStart));
			VerifyEqual(GetTestPath());
			VerifyEqual(GetTestPathB());
			pFile.Reset();
			DeleteTestFile();
			VerifyEqual(GetTestPath());
			VerifyEqual(GetTestPathB());
		}
		else
		{
			SEOUL_UNITTESTING_ASSERT(!m_pExpected->OpenForPlatform((Platform)i, GetTestPath(), File::kRead, pFile));
		}
	}
}

void CachingDiskFileSystemTest::TestRename()
{
	// Cache rename
	{
		WriteTestFile();
		SEOUL_UNITTESTING_ASSERT(m_pFileSystem->Rename(GetTestPath().GetAbsoluteFilename(), GetTestPathB().GetAbsoluteFilename()));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		DeleteTestFileB();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
	}
	// Cache rename
	{
		WriteTestFile();
		SEOUL_UNITTESTING_ASSERT(m_pFileSystem->Rename(GetTestPath(), GetTestPathB()));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		DeleteTestFileB();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
	}
	// Disk rename
	{
		WriteTestFile();
		SEOUL_UNITTESTING_ASSERT(m_pExpected->Rename(GetTestPath().GetAbsoluteFilename(), GetTestPathB().GetAbsoluteFilename()));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		DeleteTestFileB();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
	}
	// Disk rename
	{
		WriteTestFile();
		SEOUL_UNITTESTING_ASSERT(m_pExpected->Rename(GetTestPath(), GetTestPathB()));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		DeleteTestFileB();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
	}
}

void CachingDiskFileSystemTest::TestReadAll()
{
	// Cache read all
	{
		WriteTestFile();
		void* p = nullptr;
		UInt32 u = 0u;
		SEOUL_UNITTESTING_ASSERT(m_pFileSystem->ReadAll(GetTestPath().GetAbsoluteFilename(), p, u, 0u, MemoryBudgets::Developer));
		MemoryManager::Deallocate(p);
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		DeleteTestFile();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
	}
	// Cache read all
	{
		WriteTestFile();
		void* p = nullptr;
		UInt32 u = 0u;
		SEOUL_UNITTESTING_ASSERT(m_pFileSystem->ReadAll(GetTestPath(), p, u, 0u, MemoryBudgets::Developer));
		MemoryManager::Deallocate(p);
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		DeleteTestFile();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
	}
	// Disk read all
	{
		WriteTestFile();
		void* p = nullptr;
		UInt32 u = 0u;
		SEOUL_UNITTESTING_ASSERT(m_pExpected->ReadAll(GetTestPath().GetAbsoluteFilename(), p, u, 0u, MemoryBudgets::Developer));
		MemoryManager::Deallocate(p);
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		DeleteTestFile();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
	}
	// Disk read all
	{
		WriteTestFile();
		void* p = nullptr;
		UInt32 u = 0u;
		SEOUL_UNITTESTING_ASSERT(m_pExpected->ReadAll(GetTestPath(), p, u, 0u, MemoryBudgets::Developer));
		MemoryManager::Deallocate(p);
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		DeleteTestFile();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
	}
}

void CachingDiskFileSystemTest::TestReadAllForPlatform()
{
	// Cache read all
	{
		WriteTestFile();
		void* p = nullptr;
		UInt32 u = 0u;
		for (Int i = 0; i < (Int)Platform::SEOUL_PLATFORM_COUNT; ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(i == (Int)keCurrentPlatform, m_pFileSystem->ReadAllForPlatform((Platform)i, GetTestPath(), p, u, 0u, MemoryBudgets::Developer));
			MemoryManager::Deallocate(p);
			p = nullptr;
			VerifyEqual(GetTestPath());
			VerifyEqual(GetTestPathB());
		}
		DeleteTestFile();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
	}
	// Disk read all
	{
		WriteTestFile();
		void* p = nullptr;
		UInt32 u = 0u;
		for (Int i = 0; i < (Int)Platform::SEOUL_PLATFORM_COUNT; ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(i == (Int)keCurrentPlatform, m_pExpected->ReadAllForPlatform((Platform)i, GetTestPath(), p, u, 0u, MemoryBudgets::Developer));
			MemoryManager::Deallocate(p);
			p = nullptr;
			VerifyEqual(GetTestPath());
			VerifyEqual(GetTestPathB());
		}
		DeleteTestFile();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
	}
}

void CachingDiskFileSystemTest::TestSetModifiedTime()
{
	auto const uTime = WorldTime::GetUTCTime().GetSeconds();

	// Cache read only
	{
		WriteTestFile();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		SEOUL_UNITTESTING_ASSERT(m_pFileSystem->SetModifiedTime(GetTestPath().GetAbsoluteFilename(), uTime));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		DeleteTestFile();
	}
	// Cache read only
	{
		WriteTestFile();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		SEOUL_UNITTESTING_ASSERT(m_pFileSystem->SetModifiedTime(GetTestPath(), uTime));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		DeleteTestFile();
	}
	// Disk read only
	{
		WriteTestFile();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		SEOUL_UNITTESTING_ASSERT(m_pExpected->SetModifiedTime(GetTestPath().GetAbsoluteFilename(), uTime));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		DeleteTestFile();
	}
	// Disk read only
	{
		WriteTestFile();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		SEOUL_UNITTESTING_ASSERT(m_pExpected->SetModifiedTime(GetTestPath(), uTime));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		DeleteTestFile();
	}
}

void CachingDiskFileSystemTest::TestSetModifiedTimeForPlatform()
{
	auto uTime = WorldTime::GetUTCTime().GetSeconds();

	// Cache read only
	{
		WriteTestFile();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		for (Int i = 0; i < (Int)Platform::SEOUL_PLATFORM_COUNT; ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(i == (Int)keCurrentPlatform, m_pFileSystem->SetModifiedTimeForPlatform((Platform)i, GetTestPath(), uTime));
			++uTime;
			VerifyEqual(GetTestPath());
			VerifyEqual(GetTestPathB());
		}
		DeleteTestFile();
	}
	// Disk read only
	{
		WriteTestFile();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		for (Int i = 0; i < (Int)Platform::SEOUL_PLATFORM_COUNT; ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(i == (Int)keCurrentPlatform, m_pExpected->SetModifiedTimeForPlatform((Platform)i, GetTestPath(), uTime));
			++uTime;
			VerifyEqual(GetTestPath());
			VerifyEqual(GetTestPathB());
		}
		DeleteTestFile();
	}
}

void CachingDiskFileSystemTest::TestSetReadOnlyBit()
{
	// Cache read only
	{
		WriteTestFile();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		SEOUL_UNITTESTING_ASSERT(m_pFileSystem->SetReadOnlyBit(GetTestPath().GetAbsoluteFilename(), true));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		SEOUL_UNITTESTING_ASSERT(!m_pFileSystem->WriteAll(GetTestPath().GetAbsoluteFilename(), "asdf", 4));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		SEOUL_UNITTESTING_ASSERT(m_pFileSystem->SetReadOnlyBit(GetTestPath().GetAbsoluteFilename(), false));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		SEOUL_UNITTESTING_ASSERT(m_pFileSystem->WriteAll(GetTestPath().GetAbsoluteFilename(), "asdf", 4));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		DeleteTestFile();
	}
	// Cache read only
	{
		WriteTestFile();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		SEOUL_UNITTESTING_ASSERT(m_pFileSystem->SetReadOnlyBit(GetTestPath(), true));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		SEOUL_UNITTESTING_ASSERT(!m_pFileSystem->WriteAll(GetTestPath(), "asdf", 4));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		SEOUL_UNITTESTING_ASSERT(m_pFileSystem->SetReadOnlyBit(GetTestPath(), false));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		SEOUL_UNITTESTING_ASSERT(m_pFileSystem->WriteAll(GetTestPath(), "asdf", 4));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		DeleteTestFile();
	}
	// Disk read only
	{
		WriteTestFile();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		SEOUL_UNITTESTING_ASSERT(m_pExpected->SetReadOnlyBit(GetTestPath().GetAbsoluteFilename(), true));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		SEOUL_UNITTESTING_ASSERT(!m_pExpected->WriteAll(GetTestPath().GetAbsoluteFilename(), "asdf", 4));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		SEOUL_UNITTESTING_ASSERT(m_pExpected->SetReadOnlyBit(GetTestPath().GetAbsoluteFilename(), false));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		SEOUL_UNITTESTING_ASSERT(m_pExpected->WriteAll(GetTestPath().GetAbsoluteFilename(), "asdf", 4));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		DeleteTestFile();
	}
	// Disk read only
	{
		WriteTestFile();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		SEOUL_UNITTESTING_ASSERT(m_pExpected->SetReadOnlyBit(GetTestPath(), true));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		SEOUL_UNITTESTING_ASSERT(!m_pExpected->WriteAll(GetTestPath(), "asdf", 4));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		SEOUL_UNITTESTING_ASSERT(m_pExpected->SetReadOnlyBit(GetTestPath(), false));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		SEOUL_UNITTESTING_ASSERT(m_pExpected->WriteAll(GetTestPath(), "asdf", 4));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		DeleteTestFile();
	}
}

void CachingDiskFileSystemTest::TestWriteAll()
{
	// Cache write all
	{
		SEOUL_UNITTESTING_ASSERT(m_pFileSystem->WriteAll(GetTestPath().GetAbsoluteFilename(), "asdf", 4));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		DeleteTestFile();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
	}
	// Cache write all
	{
		SEOUL_UNITTESTING_ASSERT(m_pFileSystem->WriteAll(GetTestPath(), "asdf", 4));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		DeleteTestFile();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
	}
	// Disk write all
	{
		SEOUL_UNITTESTING_ASSERT(m_pExpected->WriteAll(GetTestPath().GetAbsoluteFilename(), "asdf", 4));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		DeleteTestFile();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
	}
	// Disk write all
	{
		SEOUL_UNITTESTING_ASSERT(m_pExpected->WriteAll(GetTestPath(), "asdf", 4));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		DeleteTestFile();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
	}
}

void CachingDiskFileSystemTest::TestWriteAllModifiedTime()
{
	auto const uModifiedTime = WorldTime::GetUTCTime().GetSeconds();

	// Cache write all
	{
		SEOUL_UNITTESTING_ASSERT(m_pFileSystem->WriteAll(GetTestPath().GetAbsoluteFilename(), "asdf", 4, uModifiedTime));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		DeleteTestFile();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
	}
	// Cache write all
	{
		SEOUL_UNITTESTING_ASSERT(m_pFileSystem->WriteAll(GetTestPath(), "asdf", 4, uModifiedTime));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		DeleteTestFile();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
	}
	// Disk write all
	{
		SEOUL_UNITTESTING_ASSERT(m_pExpected->WriteAll(GetTestPath().GetAbsoluteFilename(), "asdf", 4, uModifiedTime));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		DeleteTestFile();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
	}
	// Disk write all
	{
		SEOUL_UNITTESTING_ASSERT(m_pExpected->WriteAll(GetTestPath(), "asdf", 4, uModifiedTime));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		DeleteTestFile();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
	}
}

void CachingDiskFileSystemTest::TestWriteAllForPlatform()
{
	// Cache write all
	{
		for (Int i = 0; i < (Int)Platform::SEOUL_PLATFORM_COUNT; ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL((Int)i == (Int)keCurrentPlatform, m_pFileSystem->WriteAllForPlatform((Platform)i, GetTestPath(), "asdf", 4));
			VerifyEqual(GetTestPath());
			VerifyEqual(GetTestPathB());
		}
		DeleteTestFile();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
	}
	// Disk write all
	{
		SEOUL_UNITTESTING_ASSERT(m_pExpected->WriteAllForPlatform(keCurrentPlatform, GetTestPath(), "asdf", 4));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		DeleteTestFile();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
	}
}

void CachingDiskFileSystemTest::TestWriteAllForPlatformModifiedTime()
{
	auto const uModifiedTime = WorldTime::GetUTCTime().GetSeconds();

	// Cache write all
	{
		for (Int i = 0; i < (Int)Platform::SEOUL_PLATFORM_COUNT; ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL((Int)i == (Int)keCurrentPlatform, m_pFileSystem->WriteAllForPlatform((Platform)i, GetTestPath(), "asdf", 4, uModifiedTime));
			VerifyEqual(GetTestPath());
			VerifyEqual(GetTestPathB());
		}
		DeleteTestFile();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
	}
	// Disk write all
	{
		SEOUL_UNITTESTING_ASSERT(m_pExpected->WriteAllForPlatform(keCurrentPlatform, GetTestPath(), "asdf", 4, uModifiedTime));
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
		DeleteTestFile();
		VerifyEqual(GetTestPath());
		VerifyEqual(GetTestPathB());
	}
}


#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
