/**
 * \file DownloadablePackageFileSystemTest.h
 * \brief Test for the DownloadablePackageFileSystem,
 * which implements on-demand downloading of file data into
 * a single .sar (SeoulEngine Archive) file.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef DOWNLOADABLE_PACKAGE_FILE_SYSTEM_TEST_H
#define DOWNLOADABLE_PACKAGE_FILE_SYSTEM_TEST_H

#include "Prereqs.h"
namespace Seoul { namespace HTTP { class Server; } }
namespace Seoul { class UnitTestsEngineHelper; }

namespace Seoul
{

#if SEOUL_UNIT_TESTS

/**
 * Text fixture class for threads.
 */
class DownloadablePackageFileSystemTest SEOUL_SEALED
{
public:
	DownloadablePackageFileSystemTest();
	~DownloadablePackageFileSystemTest();

	void V19_MeasureAllDownload() { MeasureAllDownload("V19_"); }
	void V19_MeasureAllDownloadAdjusted() { MeasureAllDownloadAdjusted("V19_"); }
	void V19_MeasureAllFallback() { MeasureAllFallback("V19_"); }
	void V19_MeasureAllLocal() { MeasureAllLocal("V19_"); }
	void V19_MeasureAllMigrated() { MeasureAllMigrated("V19_"); }
	void V19_MeasurePartialDownload() { MeasurePartialDownload("V19_"); }
	void V19_MeasurePartialDownloadAdjusted() { MeasurePartialDownloadAdjusted("V19_"); }
	void V19_MeasurePartialFallback() { MeasurePartialFallback("V19_"); }
	void V19_MeasureTimePartialDownload() { MeasureTimePartialDownload("V19_"); }

	void V20_MeasureAllDownload() { MeasureAllDownload("V20_"); }
	void V20_MeasureAllDownloadAdjusted() { MeasureAllDownloadAdjusted("V20_"); }
	void V20_MeasureAllFallback() { MeasureAllFallback("V20_"); }
	void V20_MeasureAllLocal() { MeasureAllLocal("V20_"); }
	void V20_MeasureAllMigrated() { MeasureAllMigrated("V20_"); }
	void V20_MeasurePartialDownload() { MeasurePartialDownload("V20_"); }
	void V20_MeasurePartialDownloadAdjusted() { MeasurePartialDownloadAdjusted("V20_"); }
	void V20_MeasurePartialFallback() { MeasurePartialFallback("V20_"); }
	void V20_MeasureTimePartialDownload() { MeasureTimePartialDownload("V20_"); }

	void V21_MeasureAllDownload() { MeasureAllDownload("V21_"); }
	void V21_MeasureAllDownloadAdjusted() { MeasureAllDownloadAdjusted("V21_"); }
	void V21_MeasureAllFallback() { MeasureAllFallback("V21_"); }
	void V21_MeasureAllLocal() { MeasureAllLocal("V21_"); }
	void V21_MeasureAllMigrated() { MeasureAllMigrated("V21_"); }
	void V21_MeasurePartialDownload() { MeasurePartialDownload("V21_"); }
	void V21_MeasurePartialDownloadAdjusted() { MeasurePartialDownloadAdjusted("V21_"); }
	void V21_MeasurePartialFallback() { MeasurePartialFallback("V21_"); }
	void V21_MeasureTimePartialDownload() { MeasureTimePartialDownload("V21_"); }

	void MeasureAllDownload(Byte const* sPrefix);
	void MeasureAllDownloadAdjusted(Byte const* sPrefix);
	void MeasureAllFallback(Byte const* sPrefix);
	void MeasureAllLocal(Byte const* sPrefix);
	void MeasureAllMigrated(Byte const* sPrefix);
	void MeasurePartialDownload(Byte const* sPrefix);
	void MeasurePartialDownloadAdjusted(Byte const* sPrefix);
	void MeasurePartialFallback(Byte const* sPrefix);
	void MeasureTimePartialDownload(Byte const* sPrefix);

	void TestBadHeader();
	void TestBasic();
	void TestBasicCompressed();
	void TestCompressionDictPaths();
	void TestEdgeCases();
	void TestEdgeCases2();
	void TestExisting();
	void TestFetch();
	void TestGarbageFile();
	void TestGetDirectoryListing();
	void TestLargeFile();
	void V19_TestMiscApi() { TestMiscApi("V19_"); }
	void V20_TestMiscApi() { TestMiscApi("V20_"); }
	void V21_TestMiscApi() { TestMiscApi("V21_"); }
	void TestMiscApi(Byte const* sPrefix);
	void TestNoServer();
	void TestPopulate();
	void TestPopulateFromIncompatibleArchives();
	void TestReadOnlyFileFailures();
	void TestRegressCrcIncorrect();
	void TestRegressInfiniteLoop();
	void TestRequestCount();
	void TestRequestCount2();
	void TestSettingsAdjusted();
	void TestSettingsDefault();
	void TestSettingsSparse();
	void TestUpdate();

	// Tests explicitly against V19 and V20 (newest) versions of the archive format.
	void V19_TestSettingsAdjusted() { CommonTestSettingsAdjusted("V19_"); }
	void V19_TestSettingsDefault() { CommonTestSettingsDefault("V19_"); }
	void V19_TestSettingsSparse() { CommonTestSettingsSparse("V19_"); }
	void V19_TestUpdate() { CommonTestUpdate("V19_"); }

	void V20_TestSettingsAdjusted() { CommonTestSettingsAdjusted("V20_"); }
	void V20_TestSettingsDefault() { CommonTestSettingsDefault("V20_"); }
	void V20_TestSettingsSparse() { CommonTestSettingsSparse("V20_"); }
	void V20_TestUpdate() { CommonTestUpdate("V20_"); }

	void V21_TestSettingsAdjusted() { CommonTestSettingsAdjusted("V21_"); }
	void V21_TestSettingsDefault() { CommonTestSettingsDefault("V21_"); }
	void V21_TestSettingsSparse() { CommonTestSettingsSparse("V21_"); }
	void V21_TestUpdate() { CommonTestUpdate("V21_"); }

private:
	ScopedPtr<UnitTestsEngineHelper> m_pHelper;
	String m_sSourcePackageFilename;
	String m_sTargetPackageFilename;
	ScopedPtr<HTTP::Server> m_pServer;
	CheckedPtr<DownloadablePackageFileSystem> m_pSystem;

	void Destroy();
	void Init();

	void InternalInitializeFailureCommon(Bool bExpectWriteFailure);
	void InternalTestCommon();
	void WaitForPackageInitialize();
	void WaitForPackageWorkCompletion();
	void WaitForWriteFailure();
	void WriteGarbageToTargetFile();

	void CommonTestSettingsAdjusted(const String& sPrefix);
	void CommonTestSettingsDefault(const String& sPrefix);
	void CommonTestSettingsSparse(const String& sPrefix);
	void CommonTestUpdate(const String& sPrefix);

	void MeasureDownloadBytesCheck(UInt32 uCdictBytes, UInt32 uLoopBytes);
	void MeasureEventCheck(
		UInt32 uRequests = 0u,
		UInt32 uCdictDownloads = 0u,
		UInt32 uLoopDownloads = 0u,
		UInt32 uFetchSet = 0u,
		UInt32 uLoopProcess = 0u,
		UInt32 uPopulate = 0u);

	SEOUL_DISABLE_COPY(DownloadablePackageFileSystemTest);
}; // class DownloadablePackageFileSystemTest

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
