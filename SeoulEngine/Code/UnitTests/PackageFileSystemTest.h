/**
 * \file PackageFileSystemTest.h
 * \brief Test for the PackageFileSystem, the basic
 * type of all .sar based file systems.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef PACKAGE_FILE_SYSTEM_TEST_H
#define PACKAGE_FILE_SYSTEM_TEST_H

#include "Prereqs.h"
namespace Seoul { class UnitTestsEngineHelper; }

namespace Seoul
{

#if SEOUL_UNIT_TESTS

/**
 * Text fixture class for threads.
 */
class PackageFileSystemTest SEOUL_SEALED
{
public:
	PackageFileSystemTest();
	~PackageFileSystemTest();

	void TestBadHeader();
	void TestBasic();
	void TestBasicCompressed();
	void TestBasicInMemory();
	void TestBasicCompressedInMemory();
	void TestCommitChangeToSarFileFail();
	void TestCommitChangeToSarFileSucceed();
	void V19_TestCompressionDictApi() { TestCompressionDictApi("V19_"); }
	void V20_TestCompressionDictApi() { TestCompressionDictApi("V20_"); }
	void V21_TestCompressionDictApi() { TestCompressionDictApi("V21_"); }
	void TestCompressionDictApi(Byte const* sPrefix);
	void V19_TestCompressionDictApiDeferred() { TestCompressionDictApiDeferred("V19_"); }
	void V20_TestCompressionDictApiDeferred() { TestCompressionDictApiDeferred("V20_"); }
	void V21_TestCompressionDictApiDeferred() { TestCompressionDictApiDeferred("V21_"); }
	void TestCompressionDictApiDeferred(Byte const* sPrefix);
	void V19_TestCompressionFile() { TestCompressionFile("V19_"); }
	void V20_TestCompressionFile() { TestCompressionFile("V20_"); }
	void V21_TestCompressionFile() { TestCompressionFile("V21_"); }
	void TestCompressionFile(Byte const* sPrefix);
	void TestEdgeCases();
	void TestGarbageFile();
	void TestGetDirectoryListing();
	void TestHeader();
	void TestLargeFile();
	void V19_TestMiscApi() { TestMiscApi("V19_"); }
	void V20_TestMiscApi() { TestMiscApi("V20_"); }
	void V21_TestMiscApi() { TestMiscApi("V21_"); }
	void TestMiscApi(Byte const* sPrefix);
	void TestPerformCrc32EdgeCases();
	void V19_TestReadRaw() { TestReadRaw("V19_"); }
	void V20_TestReadRaw() { TestReadRaw("V20_"); }
	void V21_TestReadRaw() { TestReadRaw("V21_"); }
	void TestReadRaw(Byte const* sPrefix);
	void TestSeekFail();
	void TestCorruptedFileTable();
	void TestCorruptedFileTableV20();
	void TestCorruptedFileTableV21();

private:
	ScopedPtr<UnitTestsEngineHelper> m_pHelper;
	String m_sSourcePackageFilename;
	CheckedPtr<PackageFileSystem> m_pSystem;

	void Destroy();
	void Init();

	void InternalInitializeFailureCommon();
	void InternalTestCommon();
	void WriteGarbageToSourceFile(const String& sInput);

	SEOUL_DISABLE_COPY(PackageFileSystemTest);
}; // class PackageFileSystemTest

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
