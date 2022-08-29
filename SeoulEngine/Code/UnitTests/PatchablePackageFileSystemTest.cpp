/**
 * \file PatchablePackageFileSystemTest.cpp
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

#include "FileManager.h"
#include "GamePaths.h"
#include "HTTPManager.h"
#include "HTTPServer.h"
#include "Logger.h"
#include "PatchablePackageFileSystem.h"
#include "PatchablePackageFileSystemTest.h"
#include "Path.h"
#include "ReflectionDefine.h"
#include "Thread.h"
#include "UnitTesting.h"
#include "UnitTestsEngineHelper.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

static Byte const* const s_kaFiles[] =
{
	"Authored/Engine/monkey_font.sif0",
	"Authored/Engine/monkey_font.sif1",
	"Authored/Engine/monkey_font.sif2",
	"Authored/Engine/monkey_font.sif3",
};

SEOUL_BEGIN_TYPE(PatchablePackageFileSystemTest, TypeFlags::kDisableCopy)
	SEOUL_ATTRIBUTE(UnitTest, Attributes::UnitTest::kInstantiateForEach) // Want Engine and other resources to be recreated for each test.
	SEOUL_METHOD(TestBadHeader)
	SEOUL_METHOD(TestBasic)
	SEOUL_METHOD(TestBasicCompressed)
	SEOUL_METHOD(TestEdgeCases)
	SEOUL_METHOD(TestExisting)
	SEOUL_METHOD(TestFetch)
	SEOUL_METHOD(TestGarbageFile)
	SEOUL_METHOD(TestLargeFile)
	SEOUL_METHOD(TestMiscApi)
	SEOUL_METHOD(TestNoServer)
	SEOUL_METHOD(TestObfuscated)
	SEOUL_METHOD(TestPopulate)
	SEOUL_METHOD(TestReadOnlyFileFailures)
	SEOUL_METHOD(TestRequestCount)
	SEOUL_METHOD(TestRequestCount2)
	SEOUL_METHOD(TestSettingsDefault)
	SEOUL_METHOD(TestSetUrl)
SEOUL_END_TYPE()

static inline Bool IsCrc32Ok(IPackageFileSystem& pkg)
{
	PackageCrc32Entries v;
	auto const b = pkg.PerformCrc32Check(&v);

	PackageFileSystem::FileTable t;
	SEOUL_UNITTESTING_ASSERT(pkg.GetFileTable(t));
	SEOUL_UNITTESTING_ASSERT_EQUAL(v.GetSize(), t.GetSize());
	if (b)
	{
		for (auto const& e : v)
		{
			SEOUL_UNITTESTING_ASSERT(e.m_bCrc32Ok);
			auto pEntry = t.Find(e.m_FilePath);
			SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(nullptr, pEntry);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(&e.m_Entry, &pEntry->m_Entry, sizeof(e.m_Entry)));
		}
	}
	else
	{
		// At least one entry must have crc32 == false.
		UInt32 uOk = 0u;
		UInt32 uNotOk = 0u;
		for (auto const& e : v)
		{
			uOk += (e.m_bCrc32Ok ? 1u : 0u);
			uNotOk += (e.m_bCrc32Ok ? 0u : 1u);
			auto pEntry = t.Find(e.m_FilePath);
			SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(nullptr, pEntry);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(&e.m_Entry, &pEntry->m_Entry, sizeof(e.m_Entry)));
		}

		SEOUL_UNITTESTING_ASSERT_EQUAL(uOk + uNotOk, t.GetSize());
		SEOUL_UNITTESTING_ASSERT_LESS_THAN(0u, uNotOk);
		SEOUL_UNITTESTING_ASSERT_LESS_THAN(uOk, t.GetSize());
	}

	return b;
}

PatchablePackageFileSystemTest::PatchablePackageFileSystemTest()
	: m_pHelper()
	, m_sReadOnlyFallbackPackageFilename()
	, m_sSourcePackageFilename()
	, m_sTargetPackageFilename(Path::GetTempFileAbsoluteFilename())
	, m_pServer()
	, m_pSystem()
{
	m_pHelper.Reset(SEOUL_NEW(MemoryBudgets::Developer) UnitTestsEngineHelper);
	{
		HTTP::ServerSettings settings;
		settings.m_sRootDirectory = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/PatchablePackageFileSystem");
		settings.m_iPort = 8057;
		settings.m_iThreadCount = 1;
		m_pServer.Reset(SEOUL_NEW(MemoryBudgets::Developer) HTTP::Server(settings));
	}
}

PatchablePackageFileSystemTest::~PatchablePackageFileSystemTest()
{
	m_pSystem.Reset();
	m_pServer.Reset();
	m_sTargetPackageFilename.Clear();
	m_sSourcePackageFilename.Clear();
	m_sReadOnlyFallbackPackageFilename.Clear();
	m_pHelper.Reset();
}

void PatchablePackageFileSystemTest::TestBadHeader()
{
	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/PatchablePackageFileSystem/PC_BadHeader.sar");
	m_sReadOnlyFallbackPackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/PatchablePackageFileSystem/PC_ReadOnlyFallback.sar");
	m_pSystem = FileManager::Get()->RegisterFileSystem<PatchablePackageFileSystem>(
		m_sReadOnlyFallbackPackageFilename,
		m_sTargetPackageFilename);
	m_pSystem->SetURL("http://localhost:8057/PC_BadHeader.sar");

	InternalInitializeFailureCommon(false);
}

void PatchablePackageFileSystemTest::TestBasic()
{
	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/PatchablePackageFileSystem/PC_Content.sar");
	m_sReadOnlyFallbackPackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/PatchablePackageFileSystem/PC_ReadOnlyFallback.sar");
	m_pSystem = FileManager::Get()->RegisterFileSystem<PatchablePackageFileSystem>(
		m_sReadOnlyFallbackPackageFilename,
		m_sTargetPackageFilename);
	m_pSystem->SetURL("http://localhost:8057/PC_Content.sar");

	InternalTestCommon();
}

void PatchablePackageFileSystemTest::TestBasicCompressed()
{
	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/PatchablePackageFileSystem/PC_Config.sar");
	m_sReadOnlyFallbackPackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/PatchablePackageFileSystem/PC_ReadOnlyFallback.sar");
	m_pSystem = FileManager::Get()->RegisterFileSystem<PatchablePackageFileSystem>(
		m_sReadOnlyFallbackPackageFilename,
		m_sTargetPackageFilename);
	m_pSystem->SetURL("http://localhost:8057/PC_Config.sar");

	WaitForPackageInitialize();

	m_pSystem->Fetch(PatchablePackageFileSystem::Files());

	WaitForPackageWorkCompletion();

	IPackageFileSystem::FileTable t;
	SEOUL_UNITTESTING_ASSERT(m_pSystem->GetFileTable(t));
	SEOUL_UNITTESTING_ASSERT_EQUAL(26, t.GetSize());
}

void PatchablePackageFileSystemTest::TestEdgeCases()
{
	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/PatchablePackageFileSystem/PC_Content.sar");
	m_sReadOnlyFallbackPackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/PatchablePackageFileSystem/PC_ReadOnlyFallback.sar");
	m_pSystem = FileManager::Get()->RegisterFileSystem<PatchablePackageFileSystem>(
		m_sReadOnlyFallbackPackageFilename,
		m_sTargetPackageFilename);
	m_pSystem->SetURL("http://localhost:8057/PC_Content.sar");

	WaitForPackageInitialize();

	// Fetch of non-existent files.
	SEOUL_UNITTESTING_ASSERT(!m_pSystem->Fetch(PatchablePackageFileSystem::Files(1u, FilePath::CreateContentFilePath("a"))));
	SEOUL_UNITTESTING_ASSERT(!m_pSystem->Fetch(PatchablePackageFileSystem::Files(1, FilePath::CreateContentFilePath("a"))));

	// Fetch.
	SEOUL_UNITTESTING_ASSERT(m_pSystem->Fetch(PatchablePackageFileSystem::Files(), PatchablePackageFileSystem::ProgressCallback(), NetworkFetchPriority::kCritical));

	WaitForPackageWorkCompletion();

	// Fetch again, this should now be a nop (no new requests should occur).
	auto const before = m_pServer->GetReceivedRequestCount();
	SEOUL_UNITTESTING_ASSERT(m_pSystem->Fetch(PatchablePackageFileSystem::Files()));
	WaitForPackageWorkCompletion();
	SEOUL_UNITTESTING_ASSERT_EQUAL(before, m_pServer->GetReceivedRequestCount());
}

void PatchablePackageFileSystemTest::TestExisting()
{
	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/PatchablePackageFileSystem/PC_Content.sar");
	m_sReadOnlyFallbackPackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/PatchablePackageFileSystem/PC_ReadOnlyFallback.sar");
	SEOUL_UNITTESTING_ASSERT(CopyFile(m_sSourcePackageFilename, m_sTargetPackageFilename));

	m_pSystem = FileManager::Get()->RegisterFileSystem<PatchablePackageFileSystem>(
		m_sReadOnlyFallbackPackageFilename,
		m_sTargetPackageFilename);
	m_pSystem->SetURL("http://localhost:8057/PC_Content.sar");

	WaitForPackageInitialize();
	WaitForPackageWorkCompletion();

	// Entire archive should be downloaded and ready, as it was populated from the cache.
	SEOUL_UNITTESTING_ASSERT(IsCrc32Ok(*m_pSystem));
	SEOUL_UNITTESTING_ASSERT(m_pSystem->IsOk());

	// Run the normal test.
	InternalTestCommon();

	// The only request should be for the header.
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, m_pServer->GetReceivedRequestCount());
}

void PatchablePackageFileSystemTest::TestFetch()
{
	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/PatchablePackageFileSystem/PC_Content.sar");
	m_sReadOnlyFallbackPackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/PatchablePackageFileSystem/PC_ReadOnlyFallback.sar");
	m_pSystem = FileManager::Get()->RegisterFileSystem<PatchablePackageFileSystem>(
		m_sReadOnlyFallbackPackageFilename,
		m_sTargetPackageFilename);
	m_pSystem->SetURL("http://localhost:8057/PC_Content.sar");

	WaitForPackageInitialize();

	PatchablePackageFileSystem::Files vFiles;
	for (size_t i = 0; i < SEOUL_ARRAY_COUNT(s_kaFiles); ++i)
	{
		vFiles.PushBack(FilePath::CreateContentFilePath(s_kaFiles[i]));
	}

	SEOUL_UNITTESTING_ASSERT(m_pSystem->Fetch(
		vFiles));

	for (UInt32 i = 0; i < SEOUL_ARRAY_COUNT(s_kaFiles); ++i)
	{
		SEOUL_UNITTESTING_ASSERT(!m_pSystem->IsServicedByNetwork(vFiles[i]));
	}

	InternalTestCommon();
}

void PatchablePackageFileSystemTest::TestGarbageFile()
{
	// Identical to TestBasic(), except the file data is cleared with garbage
	// prior to initializing the file system, to make sure the PatchablePackageFileSystem
	// is correct.
	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/PatchablePackageFileSystem/PC_Content.sar");
	m_sReadOnlyFallbackPackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/PatchablePackageFileSystem/PC_ReadOnlyFallback.sar");
	WriteGarbageToTargetFile();
	m_pSystem = FileManager::Get()->RegisterFileSystem<PatchablePackageFileSystem>(
		m_sReadOnlyFallbackPackageFilename,
		m_sTargetPackageFilename);
	m_pSystem->SetURL("http://localhost:8057/PC_Content.sar");

	InternalTestCommon();
}

void PatchablePackageFileSystemTest::TestLargeFile()
{
	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/PatchablePackageFileSystem/PC_MusicContent.sar");
	m_sReadOnlyFallbackPackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/PatchablePackageFileSystem/PC_ReadOnlyFallback.sar");
	m_pSystem = FileManager::Get()->RegisterFileSystem<PatchablePackageFileSystem>(
		m_sReadOnlyFallbackPackageFilename,
		m_sTargetPackageFilename);
	m_pSystem->SetURL("http://localhost:8057/PC_MusicContent.sar");

	WaitForPackageInitialize();

	ScopedPtr<SyncFile> pFile;
	SEOUL_UNITTESTING_ASSERT(m_pSystem->Open(
		FilePath::CreateContentFilePath("Authored/Sound/Music_bank01.bank"), File::kRead, pFile));

	void* p = nullptr;
	UInt32 u = 0u;
	SEOUL_UNITTESTING_ASSERT(pFile->ReadAll(
		p,
		u,
		0u,
		MemoryBudgets::Developer));

	void* pActualData = nullptr;
	UInt32 uActualData = 0u;
	SEOUL_UNITTESTING_ASSERT(FileManager::Get()->ReadAll(Path::Combine(
		GamePaths::Get()->GetConfigDir(),
		"UnitTests/PatchablePackageFileSystem/Music_bank01.bank"),
		pActualData,
		uActualData,
		0u,
		MemoryBudgets::Developer));

	SEOUL_UNITTESTING_ASSERT_EQUAL(u, uActualData);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(p, pActualData, uActualData));

	MemoryManager::Deallocate(pActualData);
	pActualData = nullptr;
	MemoryManager::Deallocate(p);
	p = nullptr;
}

void PatchablePackageFileSystemTest::TestMiscApi()
{
	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/PatchablePackageFileSystem/V19_Measure_PC_ConfigUpdate1.sar");
	m_sReadOnlyFallbackPackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/PatchablePackageFileSystem/V19_Measure_PC_ConfigUpdate1.sar");
	m_pSystem = FileManager::Get()->RegisterFileSystem<PatchablePackageFileSystem>(
		m_sReadOnlyFallbackPackageFilename,
		m_sTargetPackageFilename);
	m_pSystem->SetURL("http://localhost:8057/V19_Measure_PC_ConfigUpdate1.sar");

	WaitForPackageInitialize();

	SEOUL_UNITTESTING_ASSERT(m_pSystem->IsOk());

	// Delete.
	{
		SEOUL_UNITTESTING_ASSERT(!m_pSystem->Delete(FilePath::CreateConfigFilePath("application.json")));
	}
	{
		SEOUL_UNITTESTING_ASSERT(!m_pSystem->Delete("application.json"));
	}
	// Exists.
	{
		SEOUL_UNITTESTING_ASSERT(m_pSystem->Exists(FilePath::CreateConfigFilePath("application.json")));
	}
	{
		SEOUL_UNITTESTING_ASSERT(!m_pSystem->Exists("application.json"));
	}
	{
		SEOUL_UNITTESTING_ASSERT(m_pSystem->ExistsForPlatform(Platform::kPC, FilePath::CreateConfigFilePath("application.json")));
		SEOUL_UNITTESTING_ASSERT(!m_pSystem->ExistsForPlatform(Platform::kAndroid, FilePath::CreateConfigFilePath("application.json")));
		SEOUL_UNITTESTING_ASSERT(!m_pSystem->ExistsForPlatform(Platform::kIOS, FilePath::CreateConfigFilePath("application.json")));
		SEOUL_UNITTESTING_ASSERT(!m_pSystem->ExistsForPlatform(Platform::kLinux, FilePath::CreateConfigFilePath("application.json")));
	}
	// File size.
	{
		UInt64 u = 0u;
		SEOUL_UNITTESTING_ASSERT(m_pSystem->GetFileSize(FilePath::CreateConfigFilePath("application.json"), u));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1229, u);
	}
	{
		UInt64 u = 257u;
		SEOUL_UNITTESTING_ASSERT(!m_pSystem->GetFileSize("application.json", u));
		SEOUL_UNITTESTING_ASSERT_EQUAL(257u, u);
	}
	{
		UInt64 u = 0u;
		SEOUL_UNITTESTING_ASSERT(m_pSystem->GetFileSizeForPlatform(Platform::kPC, FilePath::CreateConfigFilePath("application.json"), u));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1229, u);
		u = 257u;
		SEOUL_UNITTESTING_ASSERT(!m_pSystem->GetFileSizeForPlatform(Platform::kAndroid, FilePath::CreateConfigFilePath("application.json"), u));
		SEOUL_UNITTESTING_ASSERT_EQUAL(257u, u);
		SEOUL_UNITTESTING_ASSERT(!m_pSystem->GetFileSizeForPlatform(Platform::kIOS, FilePath::CreateConfigFilePath("application.json"), u));
		SEOUL_UNITTESTING_ASSERT_EQUAL(257u, u);
		SEOUL_UNITTESTING_ASSERT(!m_pSystem->GetFileSizeForPlatform(Platform::kLinux, FilePath::CreateConfigFilePath("application.json"), u));
		SEOUL_UNITTESTING_ASSERT_EQUAL(257u, u);
	}
	// Get directory list (with string path).
	{
		Vector<String> vs;
		SEOUL_UNITTESTING_ASSERT(!m_pSystem->GetDirectoryListing(String(), vs, false, false, String()));
		SEOUL_UNITTESTING_ASSERT(vs.IsEmpty());
		SEOUL_UNITTESTING_ASSERT(!m_pSystem->GetDirectoryListing(String(), vs, false, true, String()));
		SEOUL_UNITTESTING_ASSERT(vs.IsEmpty());
		SEOUL_UNITTESTING_ASSERT(!m_pSystem->GetDirectoryListing(String(), vs, true, false, String()));
		SEOUL_UNITTESTING_ASSERT(vs.IsEmpty());
		SEOUL_UNITTESTING_ASSERT(!m_pSystem->GetDirectoryListing(String(), vs, true, true, String()));
		SEOUL_UNITTESTING_ASSERT(vs.IsEmpty());
	}
	// GetActiveSyncFileCount().
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, m_pSystem->GetActiveSyncFileCount());
		{
			ScopedPtr<SyncFile> p;
			SEOUL_UNITTESTING_ASSERT(m_pSystem->Open(FilePath::CreateConfigFilePath("application.json"), File::kRead, p));
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, m_pSystem->GetActiveSyncFileCount());
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, m_pSystem->GetActiveSyncFileCount());
	}
	// GetAbsolutePackageFilename()
	SEOUL_UNITTESTING_ASSERT_EQUAL(m_sTargetPackageFilename, m_pSystem->GetAbsolutePackageFilename());
	// GetBuildChangelist()
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(1u, m_pSystem->GetBuildChangelist());
	}
	// GetBuildVersionMajor()
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(44u, m_pSystem->GetBuildVersionMajor());
	}
	// IsDirectory.
	{
		SEOUL_UNITTESTING_ASSERT(!m_pSystem->IsDirectory(FilePath::CreateConfigFilePath("application.json")));
		SEOUL_UNITTESTING_ASSERT(!m_pSystem->IsDirectory(FilePath::CreateConfigFilePath("Chat")));
	}
	{
		SEOUL_UNITTESTING_ASSERT(!m_pSystem->IsDirectory("application.json"));
		SEOUL_UNITTESTING_ASSERT(!m_pSystem->IsDirectory("Chat"));
	}
	// IsServicedByNetwork.
	{
		SEOUL_UNITTESTING_ASSERT(!m_pSystem->IsServicedByNetwork(FilePath::CreateConfigFilePath("application.json")));
		SEOUL_UNITTESTING_ASSERT(!m_pSystem->IsServicedByNetwork(FilePath::CreateConfigFilePath("Chat")));
	}
	{
		SEOUL_UNITTESTING_ASSERT(!m_pSystem->IsServicedByNetwork("application.json"));
		SEOUL_UNITTESTING_ASSERT(!m_pSystem->IsServicedByNetwork("Chat"));
	}
	// Modified time.
	{
		UInt64 u = 0u;
		SEOUL_UNITTESTING_ASSERT(m_pSystem->GetModifiedTime(FilePath::CreateConfigFilePath("application.json"), u));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1537939686u, u);
	}
	{
		UInt64 u = 257u;
		SEOUL_UNITTESTING_ASSERT(!m_pSystem->GetModifiedTime("application.json", u));
		SEOUL_UNITTESTING_ASSERT_EQUAL(257u, u);
	}
	{
		UInt64 u = 0u;
		SEOUL_UNITTESTING_ASSERT(m_pSystem->GetModifiedTimeForPlatform(Platform::kPC, FilePath::CreateConfigFilePath("application.json"), u));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1537939686, u);
		u = 257u;
		SEOUL_UNITTESTING_ASSERT(!m_pSystem->GetModifiedTimeForPlatform(Platform::kAndroid, FilePath::CreateConfigFilePath("application.json"), u));
		SEOUL_UNITTESTING_ASSERT_EQUAL(257u, u);
		SEOUL_UNITTESTING_ASSERT(!m_pSystem->GetModifiedTimeForPlatform(Platform::kIOS, FilePath::CreateConfigFilePath("application.json"), u));
		SEOUL_UNITTESTING_ASSERT_EQUAL(257u, u);
		SEOUL_UNITTESTING_ASSERT(!m_pSystem->GetModifiedTimeForPlatform(Platform::kLinux, FilePath::CreateConfigFilePath("application.json"), u));
		SEOUL_UNITTESTING_ASSERT_EQUAL(257u, u);
	}
	{
		SEOUL_UNITTESTING_ASSERT(!m_pSystem->SetModifiedTime(FilePath::CreateConfigFilePath("application.json"), 1234));
		UInt64 u = 257u;
		SEOUL_UNITTESTING_ASSERT(m_pSystem->GetModifiedTime(FilePath::CreateConfigFilePath("application.json"), u));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1537939686, u);
	}
	{
		SEOUL_UNITTESTING_ASSERT(!m_pSystem->SetModifiedTime("application.json", 1234));
		UInt64 u = 257u;
		SEOUL_UNITTESTING_ASSERT(!m_pSystem->GetModifiedTime("application.json", u));
		SEOUL_UNITTESTING_ASSERT_EQUAL(257u, u);
	}
	// NetworkFetch.
	{
		SEOUL_UNITTESTING_ASSERT(m_pSystem->NetworkFetch(FilePath::CreateConfigFilePath("application.json")));
		SEOUL_UNITTESTING_ASSERT(!m_pSystem->NetworkFetch(FilePath::CreateConfigFilePath("Chat")));
	}
}

void PatchablePackageFileSystemTest::TestNoServer()
{
	m_pServer.Reset();

	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/PatchablePackageFileSystem/PC_Content.sar");
	m_sReadOnlyFallbackPackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/PatchablePackageFileSystem/PC_ReadOnlyFallback.sar");
	m_pSystem = FileManager::Get()->RegisterFileSystem<PatchablePackageFileSystem>(
		m_sReadOnlyFallbackPackageFilename,
		m_sTargetPackageFilename);
	m_pSystem->SetURL("http://localhost:8057/PC_Content.sar");

	InternalInitializeFailureCommon(false);
}

void PatchablePackageFileSystemTest::TestObfuscated()
{
	// No requests should have yet been made.
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, m_pServer->GetReceivedRequestCount());

	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/PatchablePackageFileSystem/PC_ConfigObfuscated.sar");
	m_sReadOnlyFallbackPackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/PatchablePackageFileSystem/PC_ConfigObfuscatedReversed.sar");
	m_pSystem = FileManager::Get()->RegisterFileSystem<PatchablePackageFileSystem>(
		m_sReadOnlyFallbackPackageFilename,
		m_sTargetPackageFilename);
	m_pSystem->SetURL("http://localhost:8057/PC_ConfigObfuscated.sar");

	WaitForPackageInitialize();

	// Should now have 2 requests (one for the header, one for the file table as part
	// of initialization.
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, m_pServer->GetReceivedRequestCount());

	// Prefetch all files in the .sar - this should pull all files
	// from the read-only fallback - there should be 0 new
	// requests at the end.
	SEOUL_UNITTESTING_ASSERT(m_pSystem->Fetch(PatchablePackageFileSystem::Files()));

	WaitForPackageWorkCompletion();

	// Should now still have 2 requests.
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, m_pServer->GetReceivedRequestCount());

	// All files should be fully present now, the archive should be ok.
	SEOUL_UNITTESTING_ASSERT(IsCrc32Ok(*m_pSystem));

	// The downloaded archive should be exactly the same as the source archive now.
	SEOUL_UNITTESTING_ASSERT(FilesAreEqual(m_sSourcePackageFilename, m_sTargetPackageFilename));
}

void PatchablePackageFileSystemTest::TestPopulate()
{
	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/PatchablePackageFileSystem/PC_Content.sar");
	m_sReadOnlyFallbackPackageFilename = m_sSourcePackageFilename;
	m_pSystem = FileManager::Get()->RegisterFileSystem<PatchablePackageFileSystem>(
		m_sReadOnlyFallbackPackageFilename,
		m_sTargetPackageFilename);
	m_pSystem->SetURL("http://localhost:8057/PC_Content.sar");

	WaitForPackageInitialize();

	// All files should be fully present now, the archive should be ok.
	SEOUL_UNITTESTING_ASSERT(IsCrc32Ok(*m_pSystem));

	// The downloaded archive should be exactly the same as the source archive now.
	SEOUL_UNITTESTING_ASSERT(FilesAreEqual(m_sSourcePackageFilename, m_sTargetPackageFilename));
}

void PatchablePackageFileSystemTest::TestReadOnlyFileFailures()
{
	// Recompute the target filename, use the non-writable temp file.
	m_sTargetPackageFilename = GetNotWritableTempFileAbsoluteFilename();

	// Configure source and package.
	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/PatchablePackageFileSystem/PC_Content.sar");
	m_sReadOnlyFallbackPackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/PatchablePackageFileSystem/PC_ReadOnlyFallback.sar");
	m_pSystem = FileManager::Get()->RegisterFileSystem<PatchablePackageFileSystem>(
		m_sReadOnlyFallbackPackageFilename,
		m_sTargetPackageFilename);
	m_pSystem->SetURL("http://localhost:8057/PC_Content.sar");

	// Run tests - expect failure with a write failure.
	InternalInitializeFailureCommon(true);
}

void PatchablePackageFileSystemTest::TestRequestCount()
{
	// No requests should have yet been made.
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, m_pServer->GetReceivedRequestCount());

	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/PatchablePackageFileSystem/PC_Content.sar");
	m_sReadOnlyFallbackPackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/PatchablePackageFileSystem/PC_ReadOnlyFallback.sar");
	m_pSystem = FileManager::Get()->RegisterFileSystem<PatchablePackageFileSystem>(
		m_sReadOnlyFallbackPackageFilename,
		m_sTargetPackageFilename);
	m_pSystem->SetURL("http://localhost:8057/PC_Content.sar");

	WaitForPackageInitialize();

	// Should now have 2 requests (one for the header, one for the file table as part
	// of initialization.
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, m_pServer->GetReceivedRequestCount());

	// Prefetch all files in the .sar - this should issue a single big request, given the
	// size of the files in the archive.
	{
		PatchablePackageFileSystem::Files v;
		for (size_t i = 0u; i < SEOUL_ARRAY_COUNT(s_kaFiles); ++i)
		{
			v.PushBack(FilePath::CreateContentFilePath(s_kaFiles[i]));
		}
		SEOUL_UNITTESTING_ASSERT(m_pSystem->Fetch(v));
	}

	WaitForPackageWorkCompletion();

	// Should now have 3 requests.
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, m_pServer->GetReceivedRequestCount());

	// All files should be fully present now, the archive should be ok.
	SEOUL_UNITTESTING_ASSERT(IsCrc32Ok(*m_pSystem));

	// The downloaded archive should be exactly the same as the source archive now.
	SEOUL_UNITTESTING_ASSERT(FilesAreEqual(m_sSourcePackageFilename, m_sTargetPackageFilename));
}

void PatchablePackageFileSystemTest::TestSettingsDefault()
{
	// No requests should have yet been made.
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, m_pServer->GetReceivedRequestCount());

	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/PatchablePackageFileSystem/PC_ConfigObfuscated.sar");
	m_sReadOnlyFallbackPackageFilename = String();
	m_pSystem = FileManager::Get()->RegisterFileSystem<PatchablePackageFileSystem>(
		m_sReadOnlyFallbackPackageFilename,
		m_sTargetPackageFilename);
	m_pSystem->SetURL("http://localhost:8057/PC_ConfigObfuscated.sar");

	WaitForPackageInitialize();

	// Should now have 2 requests (one for the header, one for the file table as part
	// of initialization.
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, m_pServer->GetReceivedRequestCount());

	// Prefetch all files in the .sar - this should pull all files
	// from the read-only fallback.
	SEOUL_UNITTESTING_ASSERT(m_pSystem->Fetch(PatchablePackageFileSystem::Files()));

	WaitForPackageWorkCompletion();

	// Default settings will break the operation into 8 requests
	// (total archive of 3,976,480 bytes minus the header+file table,
	// which are 48 + 71,961 = 72,009 bytes. In other words, we're
	// downloading 3,976,480 - 72,009 = 3,904,471 bytes in requests that
	// can download at most 512 KB each.
	SEOUL_UNITTESTING_ASSERT_EQUAL(10, m_pServer->GetReceivedRequestCount());

	// All files should be fully present now, the archive should be ok.
	SEOUL_UNITTESTING_ASSERT(IsCrc32Ok(*m_pSystem));

	// The downloaded archive should be exactly the same as the source archive now.
	SEOUL_UNITTESTING_ASSERT(FilesAreEqual(m_sSourcePackageFilename, m_sTargetPackageFilename));
}

// Identical to TestRequestCount(), except with
// a different variation of Prefetch().
void PatchablePackageFileSystemTest::TestRequestCount2()
{
	// No requests should have yet been made.
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, m_pServer->GetReceivedRequestCount());

	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/PatchablePackageFileSystem/PC_Content.sar");
	m_sReadOnlyFallbackPackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/PatchablePackageFileSystem/PC_ReadOnlyFallback.sar");
	m_pSystem = FileManager::Get()->RegisterFileSystem<PatchablePackageFileSystem>(
		m_sReadOnlyFallbackPackageFilename,
		m_sTargetPackageFilename);
	m_pSystem->SetURL("http://localhost:8057/PC_Content.sar");

	WaitForPackageInitialize();

	// Should now have 2 requests (one for the header, one for the file table as part
	// of initialization.
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, m_pServer->GetReceivedRequestCount());

	// Fetch all files in the .sar - this should issue a single big request, given the
	// size of the files in the archive.
	SEOUL_UNITTESTING_ASSERT(m_pSystem->Fetch(PatchablePackageFileSystem::Files()));

	WaitForPackageWorkCompletion();

	// Should now have 3 requests.
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, m_pServer->GetReceivedRequestCount());

	// All files should be fully present now, the archive should be ok.
	SEOUL_UNITTESTING_ASSERT(IsCrc32Ok(*m_pSystem));

	// The downloaded archive should be exactly the same as the source archive now.
	SEOUL_UNITTESTING_ASSERT(FilesAreEqual(m_sSourcePackageFilename, m_sTargetPackageFilename));
}

void PatchablePackageFileSystemTest::TestSetUrl()
{
	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/PatchablePackageFileSystem/PC_BadHeader.sar");
	m_sReadOnlyFallbackPackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/PatchablePackageFileSystem/PC_MusicContent.sar");
	m_pSystem = FileManager::Get()->RegisterFileSystem<PatchablePackageFileSystem>(
		m_sReadOnlyFallbackPackageFilename,
		m_sTargetPackageFilename);

	// Start with an invalid URL.
	m_pSystem->SetURL("http://localhost:8057/ThisDoesNotExist.sar");

	// Now switch to a valid URL.
	m_pSystem->SetURL("http://localhost:8057/PC_BadHeader.sar");

	// Failure.
	InternalInitializeFailureCommon(false);

	// Change to the valid archive.
	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/PatchablePackageFileSystem/PC_Content.sar");
	m_pSystem->SetURL("http://localhost:8057/PC_Content.sar");

	// Success.
	InternalTestCommon();

	// Now back to invalid.
	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/PatchablePackageFileSystem/PC_BadHeader.sar");
	m_pSystem->SetURL("http://localhost:8057/PC_BadHeader.sar");

	// Failure.
	InternalInitializeFailureCommon(false);

	// Now switch to the compress config archive.
	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/PatchablePackageFileSystem/PC_Config.sar");
	m_pSystem->SetURL("http://localhost:8057/PC_Config.sar");

	WaitForPackageInitialize();

	m_pSystem->Fetch(PatchablePackageFileSystem::Files());

	WaitForPackageWorkCompletion();

	IPackageFileSystem::FileTable t;
	SEOUL_UNITTESTING_ASSERT(m_pSystem->GetFileTable(t));
	SEOUL_UNITTESTING_ASSERT_EQUAL(26, t.GetSize());

	// Now finally, switch to the read-only fallback.
	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/PatchablePackageFileSystem/PC_MusicContent.sar");
	m_pSystem->SetURL(String());

	WaitForPackageInitialize();

	ScopedPtr<SyncFile> pFile;
	SEOUL_UNITTESTING_ASSERT(m_pSystem->Open(
		FilePath::CreateContentFilePath("Authored/Sound/Music_bank01.bank"), File::kRead, pFile));

	void* p = nullptr;
	UInt32 u = 0u;
	SEOUL_UNITTESTING_ASSERT(pFile->ReadAll(
		p,
		u,
		0u,
		MemoryBudgets::Developer));

	void* pActualData = nullptr;
	UInt32 uActualData = 0u;
	SEOUL_UNITTESTING_ASSERT(FileManager::Get()->ReadAll(Path::Combine(
		GamePaths::Get()->GetConfigDir(),
		"UnitTests/PatchablePackageFileSystem/Music_bank01.bank"),
		pActualData,
		uActualData,
		0u,
		MemoryBudgets::Developer));

	SEOUL_UNITTESTING_ASSERT_EQUAL(u, uActualData);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(p, pActualData, uActualData));

	MemoryManager::Deallocate(pActualData);
	pActualData = nullptr;
	MemoryManager::Deallocate(p);
	p = nullptr;
}

void PatchablePackageFileSystemTest::InternalInitializeFailureCommon(Bool bExpectWriteFailure)
{
	// Sleep for a bit, we don't expect the system to connect.
	Thread::Sleep(1000);

	// All functions should fail when initialization has not occurred.
	SEOUL_UNITTESTING_ASSERT(!m_pSystem->Exists(FilePath::CreateContentFilePath("a")));
	SEOUL_UNITTESTING_ASSERT(!m_pSystem->Fetch(PatchablePackageFileSystem::Files(1, FilePath::CreateConfigFilePath("a"))));
	Vector<String> vsUnused;
	SEOUL_UNITTESTING_ASSERT(!m_pSystem->GetDirectoryListing(FilePath::CreateContentFilePath("a"), vsUnused));
	UInt64 uUnused;
	SEOUL_UNITTESTING_ASSERT(!m_pSystem->GetFileSize(FilePath::CreateContentFilePath("a"), uUnused));
	SEOUL_UNITTESTING_ASSERT(!m_pSystem->GetModifiedTime(FilePath::CreateContentFilePath("a"), uUnused));
	// Give this some time to enter the expected state.
	{
		auto uCount = 0u;
		while (bExpectWriteFailure != m_pSystem->HasExperiencedWriteFailure())
		{
			Thread::Sleep(1000);
			if (++uCount > 5u)
			{
				break;
			}
		}
	}
	SEOUL_UNITTESTING_ASSERT_EQUAL(bExpectWriteFailure, m_pSystem->HasExperiencedWriteFailure());

	ScopedPtr<SyncFile> pUnusedFile;
	SEOUL_UNITTESTING_ASSERT(!m_pSystem->Open(FilePath::CreateContentFilePath("a"), File::kRead, pUnusedFile));

	void* pUnused = nullptr;
	UInt32 uUnusedSize = 0u;
	SEOUL_UNITTESTING_ASSERT(!m_pSystem->ReadAll(FilePath::CreateContentFilePath("a"), pUnused, uUnusedSize, 0u, MemoryBudgets::Developer));

	SEOUL_UNITTESTING_ASSERT(!m_pSystem->SetModifiedTime(FilePath::CreateContentFilePath("a"), uUnused));
}

void PatchablePackageFileSystemTest::InternalTestCommon()
{
	WaitForPackageInitialize();

	IPackageFileSystem::FileTable tFileTable;
	SEOUL_UNITTESTING_ASSERT(m_pSystem->GetFileTable(tFileTable));
	SEOUL_UNITTESTING_ASSERT_EQUAL(4u, tFileTable.GetSize());

	PackageFileTableEntry entry;
	SEOUL_UNITTESTING_ASSERT(tFileTable.GetValue(FilePath::CreateContentFilePath(s_kaFiles[0]), entry));
	SEOUL_UNITTESTING_ASSERT_EQUAL(4771, entry.m_Entry.m_uCompressedFileSize);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1474242421, entry.m_Entry.m_uModifiedTime);
	SEOUL_UNITTESTING_ASSERT_EQUAL(7056, entry.m_Entry.m_uOffsetToFile);
	SEOUL_UNITTESTING_ASSERT_EQUAL(4771, entry.m_Entry.m_uUncompressedFileSize);

	SEOUL_UNITTESTING_ASSERT(tFileTable.GetValue(FilePath::CreateContentFilePath(s_kaFiles[1]), entry));
	SEOUL_UNITTESTING_ASSERT_EQUAL(3182, entry.m_Entry.m_uCompressedFileSize);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1474242421, entry.m_Entry.m_uModifiedTime);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3872, entry.m_Entry.m_uOffsetToFile);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3182, entry.m_Entry.m_uUncompressedFileSize);

	SEOUL_UNITTESTING_ASSERT(tFileTable.GetValue(FilePath::CreateContentFilePath(s_kaFiles[2]), entry));
	SEOUL_UNITTESTING_ASSERT_EQUAL(2359, entry.m_Entry.m_uCompressedFileSize);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1474242421, entry.m_Entry.m_uModifiedTime);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1512, entry.m_Entry.m_uOffsetToFile);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2359, entry.m_Entry.m_uUncompressedFileSize);

	SEOUL_UNITTESTING_ASSERT(tFileTable.GetValue(FilePath::CreateContentFilePath(s_kaFiles[3]), entry));
	SEOUL_UNITTESTING_ASSERT_EQUAL(1464, entry.m_Entry.m_uCompressedFileSize);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1474242421, entry.m_Entry.m_uModifiedTime);
	SEOUL_UNITTESTING_ASSERT_EQUAL(48, entry.m_Entry.m_uOffsetToFile);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1464, entry.m_Entry.m_uUncompressedFileSize);

	// Test data.
	for (size_t i = 0u; i < SEOUL_ARRAY_COUNT(s_kaFiles); ++i)
	{
		void* pTestData = nullptr;
		UInt32 uTestData = 0u;
		SEOUL_UNITTESTING_ASSERT(m_pSystem->ReadAll(FilePath::CreateContentFilePath(s_kaFiles[i]), pTestData, uTestData, 0u, MemoryBudgets::TBD));

		void* pActualData = nullptr;
		UInt32 uActualData = 0u;
		SEOUL_UNITTESTING_ASSERT(FileManager::Get()->ReadAll(Path::Combine(
			GamePaths::Get()->GetConfigDir(),
			"UnitTests/PatchablePackageFileSystem",
			Path::GetFileName(s_kaFiles[i])),
			pActualData,
			uActualData,
			0u,
			MemoryBudgets::Developer));
		SEOUL_UNITTESTING_ASSERT_EQUAL(uTestData, uActualData);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(pTestData, pActualData, uActualData));

		MemoryManager::Deallocate(pActualData);
		MemoryManager::Deallocate(pTestData);
	}

	// All files should be fully present now, the archive should be ok.
	SEOUL_UNITTESTING_ASSERT(IsCrc32Ok(*m_pSystem));

	// The downloaded archive should be exactly the same as the source archive now.
	SEOUL_UNITTESTING_ASSERT(FilesAreEqual(m_sSourcePackageFilename, m_sTargetPackageFilename));
}

void PatchablePackageFileSystemTest::WaitForPackageInitialize()
{
	// Wait for initialization to complete.
	{
		auto const iStartTicks = SeoulTime::GetGameTimeInTicks();
		while (m_pSystem->IsInitializing())
		{
			SEOUL_UNITTESTING_ASSERT(
				SeoulTime::ConvertTicksToSeconds(SeoulTime::GetGameTimeInTicks() - iStartTicks) < 10.0f);

			// Simulate a 60 FPS frame so we're not starving devices with not many cores.
			auto const iBegin = SeoulTime::GetGameTimeInTicks();
			m_pHelper->Tick();
			auto const iEnd = SeoulTime::GetGameTimeInTicks();
			auto const uSleep = (UInt32)Floor(Clamp(SeoulTime::ConvertTicksToMilliseconds(iEnd - iBegin), 0.0, 17.0));
			Thread::Sleep(uSleep);
		}
	}
}

void PatchablePackageFileSystemTest::WaitForPackageWorkCompletion()
{
	// Wait for work to complete.
	{
		auto const iStartTicks = SeoulTime::GetGameTimeInTicks();
		while (m_pSystem->HasWork())
		{
			SEOUL_UNITTESTING_ASSERT(
				SeoulTime::ConvertTicksToSeconds(SeoulTime::GetGameTimeInTicks() - iStartTicks) < 10.0f);

			// Simulate a 60 FPS frame so we're not starving devices with not many cores.
			auto const iBegin = SeoulTime::GetGameTimeInTicks();
			m_pHelper->Tick();
			auto const iEnd = SeoulTime::GetGameTimeInTicks();
			auto const uSleep = (UInt32)Floor(Clamp(SeoulTime::ConvertTicksToMilliseconds(iEnd - iBegin), 0.0, 17.0));
			Thread::Sleep(uSleep);
		}
	}
}

void PatchablePackageFileSystemTest::WriteGarbageToTargetFile()
{
	void* p = nullptr;
	UInt32 u = 0u;
	SEOUL_UNITTESTING_ASSERT(FileManager::Get()->ReadAll(
		m_sSourcePackageFilename,
		p,
		u,
		0u,
		MemoryBudgets::Developer));

	UInt8* pOut = (UInt8*)p;
	UInt8 const* const pEnd = (pOut + u);
	for (UInt8* p = (pOut + sizeof(PackageFileHeader)); p < pEnd; ++p)
	{
		*p = (UInt8)GlobalRandom::UniformRandomUInt32n(256);
	}

	SEOUL_UNITTESTING_ASSERT(FileManager::Get()->WriteAll(
		m_sTargetPackageFilename,
		p,
		u));

	MemoryManager::Deallocate(p);
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
