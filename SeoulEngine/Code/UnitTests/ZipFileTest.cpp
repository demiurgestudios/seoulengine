/**
 * \file ZipFileTest.cpp
 * \brief Unit test code for the ZipFileReader and ZipFileWriter classes.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ReflectionDefine.h"
#include "SeoulFile.h"
#include "UnitTesting.h"
#include "ZipFile.h"
#include "ZipFileTest.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(ZipFileTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestBasic)
	SEOUL_METHOD(TestNoFinalize)
	SEOUL_METHOD(TestNoInit)
SEOUL_END_TYPE()

void ZipFileTest::TestBasic()
{
	// Time value here is specifically chosen - .zip archives represent time
	// using way old "dos time" spec, which is lossy in comparison to 1 second
	// unit UNIX epoch time - see also: mz_zip_time_t_to_dos_time.
	auto const uExpectedModTime = 1585430224u;
	MemorySyncFile file;

	{
		ZipFileWriter writer;
		SEOUL_UNITTESTING_ASSERT(writer.Init(file));
		SEOUL_UNITTESTING_ASSERT(writer.AddFileBytes("test1", R"(Test 1)", 6, ZlibCompressionLevel::kNone));
		SEOUL_UNITTESTING_ASSERT(writer.AddFileBytes("test2", R"(Test2)", 5, ZlibCompressionLevel::kBest));
		SEOUL_UNITTESTING_ASSERT(writer.AddFileBytes("test3", R"(Test  3)", 7, ZlibCompressionLevel::kDefault, uExpectedModTime));
		SEOUL_UNITTESTING_ASSERT(writer.AddFileBytes("test_Dir/", nullptr, 0u, ZlibCompressionLevel::kNone));
		SEOUL_UNITTESTING_ASSERT(writer.AddFileString("test_Dir/test1", R"(Test 1)", ZlibCompressionLevel::kDefault));
		SEOUL_UNITTESTING_ASSERT(writer.AddFileString("test_Dir/test2", R"(Test2)", ZlibCompressionLevel::kBest, uExpectedModTime));
		SEOUL_UNITTESTING_ASSERT(writer.AddFileString("test_Dir/test3", R"(Test  3)", ZlibCompressionLevel::kNone, uExpectedModTime));
		SEOUL_UNITTESTING_ASSERT(writer.Finalize());
	}

	// Read and verify contents.
	SEOUL_UNITTESTING_ASSERT(file.Seek(0, File::kSeekFromStart));
	{
		ZipFileReader reader;
		SEOUL_UNITTESTING_ASSERT(reader.Init(file));

		// Basic checks.
		SEOUL_UNITTESTING_ASSERT_EQUAL(7, reader.GetEntryCount());
		SEOUL_UNITTESTING_ASSERT(reader.IsDirectory("test_Dir/"));
		SEOUL_UNITTESTING_ASSERT(reader.IsDirectory("test_dir/"));

		// Size checks.
		UInt64 uSize = 25u;
		SEOUL_UNITTESTING_ASSERT(!reader.GetFileSize("test_Dir/", uSize));
		SEOUL_UNITTESTING_ASSERT_EQUAL(25u, uSize);
		SEOUL_UNITTESTING_ASSERT(!reader.GetFileSize("test_dir/", uSize));
		SEOUL_UNITTESTING_ASSERT_EQUAL(25u, uSize);
		SEOUL_UNITTESTING_ASSERT(!reader.GetFileSize("", uSize));
		SEOUL_UNITTESTING_ASSERT_EQUAL(25u, uSize);
		SEOUL_UNITTESTING_ASSERT(reader.GetFileSize("test1", uSize));
		SEOUL_UNITTESTING_ASSERT_EQUAL(6, uSize);
		SEOUL_UNITTESTING_ASSERT(reader.GetFileSize("test2", uSize));
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, uSize);
		SEOUL_UNITTESTING_ASSERT(reader.GetFileSize("test3", uSize));
		SEOUL_UNITTESTING_ASSERT_EQUAL(7, uSize);
		SEOUL_UNITTESTING_ASSERT(reader.GetFileSize("test_Dir/test1", uSize));
		SEOUL_UNITTESTING_ASSERT_EQUAL(6, uSize);
		SEOUL_UNITTESTING_ASSERT(reader.GetFileSize("test_Dir/test2", uSize));
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, uSize);
		SEOUL_UNITTESTING_ASSERT(reader.GetFileSize("test_Dir/test3", uSize));
		SEOUL_UNITTESTING_ASSERT_EQUAL(7, uSize);
		SEOUL_UNITTESTING_ASSERT(reader.GetFileSize("test_dir/test1", uSize));
		SEOUL_UNITTESTING_ASSERT_EQUAL(6, uSize);
		SEOUL_UNITTESTING_ASSERT(reader.GetFileSize("test_dir/test2", uSize));
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, uSize);
		SEOUL_UNITTESTING_ASSERT(reader.GetFileSize("test_dir/test3", uSize));
		SEOUL_UNITTESTING_ASSERT_EQUAL(7, uSize);

		// Time checks.
		UInt64 uModTime = 37u;
		SEOUL_UNITTESTING_ASSERT(reader.GetModifiedTime("test_Dir/", uModTime));
		uModTime = 37u;
		SEOUL_UNITTESTING_ASSERT(reader.GetModifiedTime("test_dir/", uModTime));
		uModTime = 37u;
		SEOUL_UNITTESTING_ASSERT(!reader.GetModifiedTime("", uModTime));
		SEOUL_UNITTESTING_ASSERT_EQUAL(37u, uModTime);
		SEOUL_UNITTESTING_ASSERT(reader.GetModifiedTime("test3", uModTime));
		SEOUL_UNITTESTING_ASSERT_EQUAL(uExpectedModTime, uModTime);
		uModTime = 37u;
		SEOUL_UNITTESTING_ASSERT(reader.GetModifiedTime("test_Dir/test2", uModTime));
		SEOUL_UNITTESTING_ASSERT_EQUAL(uExpectedModTime, uModTime);
		uModTime = 37u;
		SEOUL_UNITTESTING_ASSERT(reader.GetModifiedTime("test_Dir/test3", uModTime));
		SEOUL_UNITTESTING_ASSERT_EQUAL(uExpectedModTime, uModTime);
		uModTime = 37u;
		SEOUL_UNITTESTING_ASSERT(reader.GetModifiedTime("test_dir/test2", uModTime));
		SEOUL_UNITTESTING_ASSERT_EQUAL(uExpectedModTime, uModTime);
		uModTime = 37u;
		SEOUL_UNITTESTING_ASSERT(reader.GetModifiedTime("test_dir/test3", uModTime));
		SEOUL_UNITTESTING_ASSERT_EQUAL(uExpectedModTime, uModTime);

		// Data checks - uncompressed can be access via offset, otherwise cannot.
		Int64 iAbsoluteOffset = -1;
		SEOUL_UNITTESTING_ASSERT(reader.GetInternalFileOffset("test1", iAbsoluteOffset));
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(-1, iAbsoluteOffset);

		// Read by offset checks.
		Byte aBuffer[16];
		SEOUL_UNITTESTING_ASSERT(file.Seek(iAbsoluteOffset, File::kSeekFromStart));
		SEOUL_UNITTESTING_ASSERT_EQUAL(6, file.ReadRawData(aBuffer, 6));
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp("Test 1", aBuffer, 6));
		iAbsoluteOffset = -1;

		SEOUL_UNITTESTING_ASSERT(!reader.GetInternalFileOffset("test2", iAbsoluteOffset));
		SEOUL_UNITTESTING_ASSERT_EQUAL(-1, iAbsoluteOffset);
		SEOUL_UNITTESTING_ASSERT(!reader.GetInternalFileOffset("test3", iAbsoluteOffset));
		SEOUL_UNITTESTING_ASSERT_EQUAL(-1, iAbsoluteOffset);
		SEOUL_UNITTESTING_ASSERT(!reader.GetInternalFileOffset("test_Dir/test1", iAbsoluteOffset));
		SEOUL_UNITTESTING_ASSERT_EQUAL(-1, iAbsoluteOffset);
		SEOUL_UNITTESTING_ASSERT(!reader.GetInternalFileOffset("test_Dir/test2", iAbsoluteOffset));
		SEOUL_UNITTESTING_ASSERT_EQUAL(-1, iAbsoluteOffset);

		SEOUL_UNITTESTING_ASSERT(reader.GetInternalFileOffset("test_Dir/test3", iAbsoluteOffset));
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(-1, iAbsoluteOffset);
		SEOUL_UNITTESTING_ASSERT(file.Seek(iAbsoluteOffset, File::kSeekFromStart));
		SEOUL_UNITTESTING_ASSERT_EQUAL(7, file.ReadRawData(aBuffer, 7));
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp("Test  3", aBuffer, 7));
		iAbsoluteOffset = -1;

		SEOUL_UNITTESTING_ASSERT(!reader.GetInternalFileOffset("test_dir/test1", iAbsoluteOffset));
		SEOUL_UNITTESTING_ASSERT_EQUAL(-1, iAbsoluteOffset);
		SEOUL_UNITTESTING_ASSERT(!reader.GetInternalFileOffset("test_dir/test2", iAbsoluteOffset));
		SEOUL_UNITTESTING_ASSERT_EQUAL(-1, iAbsoluteOffset);

		SEOUL_UNITTESTING_ASSERT(reader.GetInternalFileOffset("test_dir/test3", iAbsoluteOffset));
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(-1, iAbsoluteOffset);
		SEOUL_UNITTESTING_ASSERT(file.Seek(iAbsoluteOffset, File::kSeekFromStart));
		SEOUL_UNITTESTING_ASSERT_EQUAL(7, file.ReadRawData(aBuffer, 7));
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp("Test  3", aBuffer, 7));
		iAbsoluteOffset = -1;

		// Read all checks.
		void* p = nullptr;
		UInt32 u = 13u;
		SEOUL_UNITTESTING_ASSERT(!reader.ReadAll("test_Dir/", p, u, 0u, MemoryBudgets::Developer));
		SEOUL_UNITTESTING_ASSERT_EQUAL(13, u);
		SEOUL_UNITTESTING_ASSERT(!reader.ReadAll("test_dir/", p, u, 0u, MemoryBudgets::Developer));
		SEOUL_UNITTESTING_ASSERT_EQUAL(13, u);
		SEOUL_UNITTESTING_ASSERT(!reader.ReadAll("", p, u, 0u, MemoryBudgets::Developer));
		SEOUL_UNITTESTING_ASSERT_EQUAL(13, u);
		SEOUL_UNITTESTING_ASSERT(reader.ReadAll("test1", p, u, 0u, MemoryBudgets::Developer));
		SEOUL_UNITTESTING_ASSERT_EQUAL(6, u);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp("Test 1", p, 6));
		MemoryManager::Deallocate(p);
		SEOUL_UNITTESTING_ASSERT(reader.ReadAll("test2", p, u, 0u, MemoryBudgets::Developer));
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, u);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp("Test2", p, 5));
		MemoryManager::Deallocate(p);
		SEOUL_UNITTESTING_ASSERT(reader.ReadAll("test3", p, u, 0u, MemoryBudgets::Developer));
		SEOUL_UNITTESTING_ASSERT_EQUAL(7, u);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp("Test  3", p, 7));
		MemoryManager::Deallocate(p);
		SEOUL_UNITTESTING_ASSERT(reader.ReadAll("test_Dir/test1", p, u, 0u, MemoryBudgets::Developer));
		SEOUL_UNITTESTING_ASSERT_EQUAL(6, u);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp("Test 1", p, 6));
		MemoryManager::Deallocate(p);
		SEOUL_UNITTESTING_ASSERT(reader.ReadAll("test_Dir/test2", p, u, 0u, MemoryBudgets::Developer));
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, u);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp("Test2", p, 5));
		MemoryManager::Deallocate(p);
		SEOUL_UNITTESTING_ASSERT(reader.ReadAll("test_Dir/test3", p, u, 0u, MemoryBudgets::Developer));
		SEOUL_UNITTESTING_ASSERT_EQUAL(7, u);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp("Test  3", p, 7));
		MemoryManager::Deallocate(p);
		SEOUL_UNITTESTING_ASSERT(reader.ReadAll("test_dir/test1", p, u, 0u, MemoryBudgets::Developer));
		SEOUL_UNITTESTING_ASSERT_EQUAL(6, u);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp("Test 1", p, 6));
		MemoryManager::Deallocate(p);
		SEOUL_UNITTESTING_ASSERT(reader.ReadAll("test_dir/test2", p, u, 0u, MemoryBudgets::Developer));
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, u);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp("Test2", p, 5));
		MemoryManager::Deallocate(p);
		SEOUL_UNITTESTING_ASSERT(reader.ReadAll("test_dir/test3", p, u, 0u, MemoryBudgets::Developer));
		SEOUL_UNITTESTING_ASSERT_EQUAL(7, u);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp("Test  3", p, 7));
		MemoryManager::Deallocate(p);

		// Finally, iteration.
		String sName;
		SEOUL_UNITTESTING_ASSERT_EQUAL(7, reader.GetEntryCount());
		SEOUL_UNITTESTING_ASSERT(reader.GetEntryName(0u, sName));
		SEOUL_UNITTESTING_ASSERT_EQUAL("test1", sName);
		SEOUL_UNITTESTING_ASSERT(reader.GetEntryName(1u, sName));
		SEOUL_UNITTESTING_ASSERT_EQUAL("test2", sName);
		SEOUL_UNITTESTING_ASSERT(reader.GetEntryName(2u, sName));
		SEOUL_UNITTESTING_ASSERT_EQUAL("test3", sName);
		SEOUL_UNITTESTING_ASSERT(reader.GetEntryName(3u, sName));
		SEOUL_UNITTESTING_ASSERT_EQUAL("test_Dir/", sName);
		SEOUL_UNITTESTING_ASSERT(reader.IsDirectory(sName));
		SEOUL_UNITTESTING_ASSERT(reader.GetEntryName(4u, sName));
		SEOUL_UNITTESTING_ASSERT_EQUAL("test_Dir/test1", sName);
		SEOUL_UNITTESTING_ASSERT(reader.GetEntryName(5u, sName));
		SEOUL_UNITTESTING_ASSERT_EQUAL("test_Dir/test2", sName);
		SEOUL_UNITTESTING_ASSERT(reader.GetEntryName(6u, sName));
		SEOUL_UNITTESTING_ASSERT_EQUAL("test_Dir/test3", sName);
	}
}

void ZipFileTest::TestNoFinalize()
{
	// Writer without a finalize call.
	{
		MemorySyncFile file;
		ZipFileWriter writer;
		SEOUL_UNITTESTING_ASSERT(writer.Init(file));
		SEOUL_UNITTESTING_ASSERT(writer.AddFileString("test", "test contents", ZlibCompressionLevel::kDefault));
	}
}

void ZipFileTest::TestNoInit() 
{
	// Reader without an init or finalize call.
	{
		ZipFileWriter reader;
	}
	// Writer without an init or finalize call.
	{
		ZipFileWriter writer;
	}
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
