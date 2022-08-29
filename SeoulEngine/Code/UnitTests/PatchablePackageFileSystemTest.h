/**
 * \file PatchablePackageFileSystemTest.h
 * \brief Test for the PatchablePackageFileSystem,
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
#ifndef PATCHABLE_PACKAGE_FILE_SYSTEM_TEST_H
#define PATCHABLE_PACKAGE_FILE_SYSTEM_TEST_H

#include "Prereqs.h"
namespace Seoul { namespace HTTP { class Server; } }
namespace Seoul { class UnitTestsEngineHelper; }

namespace Seoul
{

#if SEOUL_UNIT_TESTS

/**
 * Text fixture class for threads.
 */
class PatchablePackageFileSystemTest SEOUL_SEALED
{
public:
	PatchablePackageFileSystemTest();
	~PatchablePackageFileSystemTest();

	void TestBadHeader();
	void TestBasic();
	void TestBasicCompressed();
	void TestEdgeCases();
	void TestExisting();
	void TestFetch();
	void TestGarbageFile();
	void TestLargeFile();
	void TestMiscApi();
	void TestNoServer();
	void TestObfuscated();
	void TestPopulate();
	void TestReadOnlyFileFailures();
	void TestRequestCount();
	void TestRequestCount2();
	void TestSettingsDefault();
	void TestSetUrl();

private:
	ScopedPtr<UnitTestsEngineHelper> m_pHelper;
	String m_sReadOnlyFallbackPackageFilename;
	String m_sSourcePackageFilename;
	String m_sTargetPackageFilename;
	ScopedPtr<HTTP::Server> m_pServer;
	CheckedPtr<PatchablePackageFileSystem> m_pSystem;

	void InternalInitializeFailureCommon(Bool bExpectWriteFailure);
	void InternalTestCommon();
	void WaitForPackageInitialize();
	void WaitForPackageWorkCompletion();
	void WriteGarbageToTargetFile();

	SEOUL_DISABLE_COPY(PatchablePackageFileSystemTest);
}; // class PatchablePackageFileSystemTest

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
