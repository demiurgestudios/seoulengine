/**
 * \file RemapDiskFileSystemTest.cpp
 * \brief Unit tests for RemapeFileSystem.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "DiskFileSystem.h"
#include "GamePaths.h"
#include "GamePathsSettings.h"
#include "Logger.h"
#include "ReflectionDefine.h"
#include "RemapDiskFileSystemTest.h"
#include "UnitTesting.h"
#include "UnitTestsFileManagerHelper.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(RemapDiskFileSystemTest)
	SEOUL_ATTRIBUTE(UnitTest)
	// Disabled on mobile - we don't have reliable disk
	// access on mobile to test this (on Android for example,
	// there are essentially no files on disk outside
	// of the APK itself that we don't deliberately write
	// ourselves).
#if !SEOUL_PLATFORM_ANDROID && !SEOUL_PLATFORM_IOS
	SEOUL_METHOD(TestBase)
	SEOUL_METHOD(TestPatchA)
	SEOUL_METHOD(TestPatchB)
#endif // /#if !SEOUL_PLATFORM_ANDROID && !SEOUL_PLATFORM_IOS // Disabled on mobile
SEOUL_END_TYPE()

static void TestImpl(Byte const* sRemapPath, UInt64 uApplicationSize, UInt32 uDirListingCount)
{
	UnitTestsFileManagerHelper scoped;

	FilePath filePath;
	filePath.SetDirectory(GameDirectory::kConfig);

	auto const sPath(Path::Combine(GamePaths::Get()->GetConfigDir(), sRemapPath));

	RemapDiskFileSystem fileSystem(filePath, sPath, true);
	SEOUL_UNITTESTING_ASSERT(!fileSystem.Delete(FilePath::CreateConfigFilePath("app_root_cert.pem")));
	SEOUL_UNITTESTING_ASSERT(fileSystem.Exists(FilePath::CreateConfigFilePath("app_root_cert.pem")));

	Vector<String> vResults;
	SEOUL_UNITTESTING_ASSERT(fileSystem.GetDirectoryListing(FilePath::CreateConfigFilePath(String()), vResults));
	SEOUL_UNITTESTING_ASSERT_EQUAL(uDirListingCount, vResults.GetSize());

	UInt64 uSize = 0u;
	SEOUL_UNITTESTING_ASSERT(fileSystem.GetFileSize(FilePath::CreateConfigFilePath("application.json"), uSize));
	SEOUL_UNITTESTING_ASSERT_EQUAL(uApplicationSize, uSize);

	UInt64 uModifiedTime = 0u;
	auto const uExpectedModifiedTime = DiskSyncFile::GetModifiedTime(Path::Combine(sPath, "gui.json"));
	SEOUL_UNITTESTING_ASSERT(fileSystem.GetModifiedTime(FilePath::CreateConfigFilePath("gui.json"), uModifiedTime));
	SEOUL_UNITTESTING_ASSERT_EQUAL(uExpectedModifiedTime, uModifiedTime);

	SEOUL_UNITTESTING_ASSERT(!fileSystem.IsDirectory(FilePath::CreateConfigFilePath("gui.json")));
	SEOUL_UNITTESTING_ASSERT(fileSystem.IsDirectory(FilePath::CreateConfigFilePath("Loc")));

	SEOUL_UNITTESTING_ASSERT(!fileSystem.IsInitializing());

	for (auto const& vs : vResults)
	{
		auto const filePath(FilePath::CreateConfigFilePath(vs));
		SEOUL_UNITTESTING_ASSERT(filePath.IsValid());
		SEOUL_UNITTESTING_ASSERT(!fileSystem.IsServicedByNetwork(filePath));
		SEOUL_UNITTESTING_ASSERT(!fileSystem.SetModifiedTime(filePath, 0u));

		UInt64 uModifiedTime = 0u;
		SEOUL_UNITTESTING_ASSERT(fileSystem.GetModifiedTime(filePath, uModifiedTime));
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(0u, uModifiedTime);
	}

	void* p = nullptr;
	UInt32 u = 0u;
	SEOUL_UNITTESTING_ASSERT(fileSystem.ReadAll(FilePath::CreateConfigFilePath("gui.json"), p, u, 0u, MemoryBudgets::Developer));

	ScopedPtr<SyncFile> pFile;
	SEOUL_UNITTESTING_ASSERT(fileSystem.Open(FilePath::CreateConfigFilePath("gui.json"), File::kRead, pFile));

	void* p2 = nullptr;
	UInt32 u2 = 0u;
	SEOUL_UNITTESTING_ASSERT(pFile->ReadAll(p2, u2, 0u, MemoryBudgets::Developer));

	SEOUL_UNITTESTING_ASSERT_EQUAL(u, u2);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, memcmp(p, p2, u));
	MemoryManager::Deallocate(p2);
	MemoryManager::Deallocate(p);

	SEOUL_UNITTESTING_ASSERT(!fileSystem.Open(FilePath::CreateConfigFilePath("gui.json"), File::kReadWrite, pFile));
}

void RemapDiskFileSystemTest::TestBase()
{
	TestImpl("UnitTests/GamePatcher/Base/Data/Config", 784, 31u);
}

void RemapDiskFileSystemTest::TestPatchA()
{
	TestImpl("UnitTests/GamePatcher/PatchA/Data/Config", 793, 31u);
}

void RemapDiskFileSystemTest::TestPatchB()
{
	TestImpl("UnitTests/GamePatcher/PatchB/Data/Config", 794, 32u);
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
