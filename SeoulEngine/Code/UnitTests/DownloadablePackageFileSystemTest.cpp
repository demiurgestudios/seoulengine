/**
 * \file DownloadablePackageFileSystemTest.cpp
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

#include "Compress.h"
#include "DownloadablePackageFileSystem.h"
#include "DownloadablePackageFileSystemTest.h"
#include "FileManager.h"
#include "GamePaths.h"
#include "HTTPManager.h"
#include "HTTPRequest.h"
#include "HTTPRequestCancellationToken.h"
#include "HTTPServer.h"
#include "JobsManager.h"
#include "Logger.h"
#include "PackageFileSystem.h"
#include "PatchablePackageFileSystem.h"
#include "Path.h"
#include "PseudoRandom.h"
#include "ReflectionDefine.h"
#include "SeoulCrc32.h"
#include "Thread.h"
#include "UnitTesting.h"
#include "UnitTestsEngineHelper.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

inline SerializedGameDirectory Convert(GameDirectory eGameDirectory)
{
	switch (eGameDirectory)
	{
	case GameDirectory::kConfig: return SerializedGameDirectory::kConfig;
	case GameDirectory::kContent: return SerializedGameDirectory::kContent;
	default:
		return SerializedGameDirectory::kUnknown;
	};
}

static Byte const* const s_kaFiles[] =
{
	"Authored/Engine/monkey_font.sif0",
	"Authored/Engine/monkey_font.sif1",
	"Authored/Engine/monkey_font.sif2",
	"Authored/Engine/monkey_font.sif3",
};

SEOUL_BEGIN_TYPE(DownloadablePackageFileSystemTest, TypeFlags::kDisableCopy)
	SEOUL_ATTRIBUTE(UnitTest, Attributes::UnitTest::kInstantiateForEach) // Want Engine and other resources to be recreated for each test.

	SEOUL_METHOD(V19_MeasureAllDownload)
	SEOUL_METHOD(V19_MeasureAllDownloadAdjusted)
	SEOUL_METHOD(V19_MeasureAllFallback)
	SEOUL_METHOD(V19_MeasureAllLocal)
	SEOUL_METHOD(V19_MeasureAllMigrated)
	SEOUL_METHOD(V19_MeasurePartialDownload)
	SEOUL_METHOD(V19_MeasurePartialDownloadAdjusted)
	SEOUL_METHOD(V19_MeasurePartialFallback)

	SEOUL_METHOD(V20_MeasureAllDownload)
	SEOUL_METHOD(V20_MeasureAllDownloadAdjusted)
	SEOUL_METHOD(V20_MeasureAllFallback)
	SEOUL_METHOD(V20_MeasureAllLocal)
	SEOUL_METHOD(V20_MeasureAllMigrated)
	SEOUL_METHOD(V20_MeasurePartialDownload)
	SEOUL_METHOD(V20_MeasurePartialDownloadAdjusted)
	SEOUL_METHOD(V20_MeasurePartialFallback)

	SEOUL_METHOD(V21_MeasureAllDownload)
	SEOUL_METHOD(V21_MeasureAllDownloadAdjusted)
	SEOUL_METHOD(V21_MeasureAllFallback)
	SEOUL_METHOD(V21_MeasureAllLocal)
	SEOUL_METHOD(V21_MeasureAllMigrated)
	SEOUL_METHOD(V21_MeasurePartialDownload)
	SEOUL_METHOD(V21_MeasurePartialDownloadAdjusted)
	SEOUL_METHOD(V21_MeasurePartialFallback)


	// TODO: Re-enable on mobile once I can find
	// a way to catch unreasonable behavior but also account
	// for the inherit volatile of mobile device testing
	// (sleep states and such).
#if !SEOUL_PLATFORM_ANDROID && !SEOUL_PLATFORM_IOS
	// Not a useful test in debug since too much overhead in
	// debug to get realistic numbers.
#if !SEOUL_DEBUG
	SEOUL_METHOD(V19_MeasureTimePartialDownload)
	SEOUL_METHOD(V20_MeasureTimePartialDownload)
	SEOUL_METHOD(V21_MeasureTimePartialDownload)
#endif
#endif // /#if !SEOUL_PLATFORM_ANDROID && !SEOUL_PLATFORM_IOS

	SEOUL_METHOD(TestBadHeader)
	SEOUL_METHOD(TestBasic)
	SEOUL_METHOD(TestBasicCompressed)
	SEOUL_METHOD(TestCompressionDictPaths)
	SEOUL_METHOD(TestEdgeCases)
	SEOUL_METHOD(TestEdgeCases2)
	SEOUL_METHOD(TestExisting)
	SEOUL_METHOD(TestFetch)
	SEOUL_METHOD(TestGarbageFile)
	SEOUL_METHOD(TestGetDirectoryListing)
	SEOUL_METHOD(TestLargeFile)
	SEOUL_METHOD(V19_TestMiscApi)
	SEOUL_METHOD(V20_TestMiscApi)
	SEOUL_METHOD(V21_TestMiscApi)
	SEOUL_METHOD(TestNoServer)
	SEOUL_METHOD(TestPopulate)
	SEOUL_METHOD(TestPopulateFromIncompatibleArchives)
	SEOUL_METHOD(TestReadOnlyFileFailures)
	SEOUL_METHOD(TestRegressCrcIncorrect)
	SEOUL_METHOD(TestRegressInfiniteLoop)
	SEOUL_METHOD(TestRequestCount)
	SEOUL_METHOD(TestRequestCount2)

	SEOUL_METHOD(V19_TestSettingsAdjusted)
	SEOUL_METHOD(V19_TestSettingsDefault)
	SEOUL_METHOD(V19_TestSettingsSparse)
	SEOUL_METHOD(V19_TestUpdate)

	SEOUL_METHOD(V20_TestSettingsAdjusted)
	SEOUL_METHOD(V20_TestSettingsDefault)
	SEOUL_METHOD(V20_TestSettingsSparse)
	SEOUL_METHOD(V20_TestUpdate)

	SEOUL_METHOD(V21_TestSettingsAdjusted)
	SEOUL_METHOD(V21_TestSettingsDefault)
	SEOUL_METHOD(V21_TestSettingsSparse)
	SEOUL_METHOD(V21_TestUpdate)
SEOUL_END_TYPE()

namespace
{

struct Sorter
{
	Bool operator()(
		const Pair<FilePath, PackageFileTableEntry>& a,
		const Pair<FilePath, PackageFileTableEntry>& b)
	{
		return (a.Second.m_Entry.m_uOffsetToFile < b.Second.m_Entry.m_uOffsetToFile);
	}
};

typedef Vector< Pair<FilePath, PackageFileTableEntry>, MemoryBudgets::Developer> AllFiles;

static void GetAll(const IPackageFileSystem& pkg, AllFiles& rv)
{
	// Prefetch every other file in the .sar.
	PackageFileSystem::FileTable t;
	SEOUL_UNITTESTING_ASSERT(pkg.GetFileTable(t));
	AllFiles v;
	for (auto const& e : t) { v.PushBack(MakePair(e.First, e.Second)); }
	Sorter sorter;
	QuickSort(v.Begin(), v.End(), sorter);

	rv.Swap(v);
}

template <typename F>
void Check(
	const DownloadablePackageFileSystemSettings& settings,
	const IPackageFileSystem& pkg,
	const AllFiles& v,
	F requested)
{
	// Check individual files - because of overflow,
	// some files not explicitly requested will also
	// have been cached locally.
	UInt64 uTotal = 0u;
	for (auto i = 0u; i < v.GetSize(); )
	{
		auto const& e = v[i];
		auto const& entry = e.Second.m_Entry;
		auto bDownloaded = requested(i);

		// Check against size increase if downloading.
		if (bDownloaded)
		{
			uTotal += entry.m_uCompressedFileSize;
			if (uTotal > settings.m_uUpperBoundMaxSizePerDownloadInBytes)
			{
				uTotal = entry.m_uCompressedFileSize;
			}

			// If we're still too big, then this is a single download that
			// leaves nothing in the total.
			if (uTotal > settings.m_uUpperBoundMaxSizePerDownloadInBytes)
			{
				uTotal = 0u;
			}

			// Check.
			SEOUL_UNITTESTING_ASSERT(!pkg.IsServicedByNetwork(e.First));

			// Advance.
			++i;
		}
		// Need to have at least one explicit requested in the current
		// set or no overflow will occur.
		else if (uTotal > 0u)
		{
			// Now advance until we hit another that will be explicitly requested.
			auto const iFirstOverflow = i;
			++i;
			while (i < v.GetSize())
			{
				if (requested(i)) { break; }
				++i;
			}

			// Compute total overflow.
			UInt64 uOverflowTotal = 0;
			if (i < v.GetSize())
			{
				SEOUL_ASSERT(iFirstOverflow > 0u);

				// From first to start of next, plus any padding between last and first.
				uOverflowTotal = (
					v[i].Second.m_Entry.m_uOffsetToFile -
					v[iFirstOverflow - 1].Second.m_Entry.m_uOffsetToFile -
					v[iFirstOverflow - 1].Second.m_Entry.m_uCompressedFileSize);
			}

			// If we hit the end, or if the overflow total is too big,
			// or if the overflow total plus the first explicit are all
			// to big, then all overflow entries are not downloaded.
			if (i >= v.GetSize() ||
				uOverflowTotal > settings.m_uMaxRedownloadSizeThresholdInBytes ||
				uTotal + uOverflowTotal + v[i].Second.m_Entry.m_uCompressedFileSize > settings.m_uUpperBoundMaxSizePerDownloadInBytes)
			{
				// Total is reset, next explicit starts a new set.
				uTotal = 0u;
				bDownloaded = false;
			}
			else
			{
				// Total expands to include overflow.
				uTotal += uOverflowTotal;
				bDownloaded = true;
			}

			// Check.
			for (auto j = iFirstOverflow; j < i; ++j)
			{
				SEOUL_UNITTESTING_ASSERT_EQUAL(bDownloaded, !pkg.IsServicedByNetwork(v[j].First));
			}

			// i is already pointing at end or the next explicit entry, no advance.
		}
		// Otherwise, just advance - skip the overflow entry
		// that will not be included.
		else
		{
			++i;
		}
	}
}

static inline Bool IsCrc32Ok(IPackageFileSystem& pkg)
{
	PackageCrc32Entries v;
	auto const b = pkg.PerformCrc32Check(&v);

	// Sanity against null version and empty list version.
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(b, pkg.PerformCrc32Check());
		PackageCrc32Entries v2;
		PackageCrc32Entry entry;
		entry.m_FilePath = FilePath::CreateContentFilePath("DoesNotExist.dat");
		v2.PushBack(entry);
		SEOUL_UNITTESTING_ASSERT(pkg.PerformCrc32Check(&v2));
	}

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

static FilePath GenCompressionDictFilePath(GameDirectory eGameDirectory, Platform ePlatform)
{
	return FilePath::CreateFilePath(
		eGameDirectory,
		String::Printf(ksPackageCompressionDictNameFormat, kaPlatformNames[(UInt32)ePlatform]));
}

static String GetFileTablePseudoFilename(UInt32 uBuildVersionMajor, UInt32 uBuildChangelist)
{
	String const sFileTablePsuedoFilename =
		String::Printf("%u", uBuildVersionMajor) +
		String::Printf("%u", uBuildChangelist);
	return sFileTablePsuedoFilename;
}

struct Entry
{
	FilePath m_FilePath{};
	void* m_pData{};
	UInt32 m_uData{};
};
typedef Vector<Entry, MemoryBudgets::Developer> Files;

static inline UInt32 GetFilesSize(const Files& vFiles)
{
	UInt32 u = 0u;
	for (auto const& e : vFiles)
	{
		u += (UInt32)RoundUpToAlignment(e.m_uData, 8u);
	}
	return u;
}

static inline UInt32 GetFileTableSize(UInt32 uVersion, const Files& vFiles)
{
	UInt32 u = sizeof(PackageFileEntry) * vFiles.GetSize();
	for (auto const& e : vFiles)
	{
		u += sizeof(UInt32); // Size.
		u += e.m_FilePath.GetRelativeFilename().GetSize(); // String.
		u += 1u; // Null terminator.
	}

	// Add in the space for the crc32 if requested.
	if (uVersion > PackageFileHeader::ku19_PreFileTablePostCrc32)
	{
		u += sizeof(UInt32);
	}

	return u;
}

static inline void WriteFiles(StreamBuffer& r, const PackageFileHeader& header, const Files& vFiles)
{
	Vector<PackageFileEntry, MemoryBudgets::Developer> vEntries;
	for (auto const& e : vFiles)
	{
		auto const uOffset = r.GetOffset();

		PackageFileEntry entry;
		entry.m_uCompressedFileSize = e.m_uData;
		entry.m_uCrc32Post = Seoul::GetCrc32((UInt8 const*)e.m_pData, e.m_uData);
		entry.m_uCrc32Pre = entry.m_uCrc32Post;
		entry.m_uModifiedTime = 0u;
		entry.m_uOffsetToFile = uOffset;
		entry.m_uUncompressedFileSize = entry.m_uCompressedFileSize;
		vEntries.PushBack(entry);
		r.Write(e.m_pData, e.m_uData);

		if (header.IsObfuscated())
		{
			PackageFileSystem::Obfuscate(
				PackageFileSystem::GenerateObfuscationKey(e.m_FilePath.GetRelativeFilename()),
				r.GetBuffer() + uOffset,
				e.m_uData,
				0);
		}

		r.PadTo((UInt32)RoundUpToAlignment(r.GetOffset(), 8u));
	}

	auto const uOffset = r.GetOffset();
	UInt32 uSize = 0u;
	if (!vEntries.IsEmpty())
	{
		for (UInt32 i = 0u; i < vFiles.GetSize(); ++i)
		{
			r.Write(vEntries.Get(i), sizeof(PackageFileEntry));

			auto const s(vFiles[i].m_FilePath.GetRelativeFilename());
			r.Write((UInt32)(s.GetSize() + 1u));
			r.Write(s.CStr(), s.GetSize() + 1u);
		}

		uSize = (r.GetOffset() - uOffset);

		// Obfuscate the file table.
		PackageFileSystem::Obfuscate(
			PackageFileSystem::GenerateObfuscationKey(GetFileTablePseudoFilename(header.GetBuildVersionMajor(), header.GetBuildChangelist())),
			r.GetBuffer() + uOffset,
			uSize,
			0);
	}

	// Add the CRC32 if requested.
	if (header.m_uVersion > PackageFileHeader::ku19_PreFileTablePostCrc32)
	{
		auto const uCrc32 = GetCrc32((UInt8 const*)(r.GetBuffer() + uOffset), uSize);
		r.Write(uCrc32);
	}
}

static String GenArchive(
	UInt8 uVersion,
	GameDirectory eGameDirectory,
	UInt32 uBuildVersion,
	UInt32 uBuildChangelist,
	Bool bObfuscated,
	Platform ePlatform,
	const Vector<Entry, MemoryBudgets::Developer>& vFiles = Vector<Entry, MemoryBudgets::Developer>())
{
	auto const sTempFile(Path::GetTempFileAbsoluteFilename());

	auto const uFileTableSize = GetFileTableSize(uVersion, vFiles);
	auto const uFilesSize = GetFilesSize(vFiles);

	PackageFileHeader header;
	memset(&header, 0, sizeof(header));
	header.m_uSignature = kuPackageSignature;
	header.m_uVersion = uVersion;
	header.SetTotalPackageFileSizeInBytes(sizeof(header) + uFilesSize + uFileTableSize);
	header.SetOffsetToFileTableInBytes(sizeof(header) + uFilesSize);
	header.SetTotalEntriesInFileTable(vFiles.GetSize());
	header.SetGameDirectory(Convert(eGameDirectory));
	header.SetHasCompressedFileTable(false);
	header.SetSizeOfFileTableInBytes(uFileTableSize);
	header.SetBuildVersionMajor(uBuildVersion);
	header.SetBuildChangelist(uBuildChangelist);
	header.SetHasSupportDirectoryQueries(false);
	header.SetPlatformAndObfuscation(ePlatform, bObfuscated);

	StreamBuffer buffer;
	buffer.Write(&header, sizeof(header));
	WriteFiles(buffer, header, vFiles);

	SEOUL_UNITTESTING_ASSERT(FileManager::Get()->WriteAll(sTempFile, buffer.GetBuffer(), buffer.GetTotalDataSizeInBytes()));

	// Sanity check.
	PackageFileSystem pkg(sTempFile);
	SEOUL_UNITTESTING_ASSERT(pkg.IsOk());
	SEOUL_UNITTESTING_ASSERT(IsCrc32Ok(pkg));

	return sTempFile;
}

} // namespace anonymous

DownloadablePackageFileSystemTest::DownloadablePackageFileSystemTest()
	: m_pHelper()
	, m_sSourcePackageFilename()
	, m_sTargetPackageFilename(Path::GetTempFileAbsoluteFilename())
	, m_pServer()
	, m_pSystem()
{
	Init();
}

DownloadablePackageFileSystemTest::~DownloadablePackageFileSystemTest()
{
	Destroy();
}

static inline UInt32 GetEventCount(HString key, const DownloadablePackageFileSystemStats& stats)
{
	auto p = stats.m_tEvents.Find(key);
	return (nullptr == p ? 0u : *p);
}

static inline Double GetMeasureMs(HString key, const DownloadablePackageFileSystemStats& stats)
{
	auto p = stats.m_tTimes.Find(key);
	return (nullptr == p ? 0.0 : SeoulTime::ConvertTicksToMilliseconds(*p));
}

#define GET_TIME_MS(name) GetMeasureMs(HString(#name), stats)
#define T_EVT(name, expected_count) \
	SEOUL_UNITTESTING_ASSERT_EQUAL(expected_count, GetEventCount(HString(#name), stats))

void DownloadablePackageFileSystemTest::MeasureDownloadBytesCheck(UInt32 uCdictBytes, UInt32 uLoopBytes)
{
	// Test stats.
	DownloadablePackageFileSystemStats stats;
	m_pSystem->GetStats(stats);

	T_EVT(init_cdict_download_bytes, uCdictBytes);
	T_EVT(loop_download_bytes, uLoopBytes);
}

void DownloadablePackageFileSystemTest::MeasureEventCheck(
	UInt32 uRequests /*= 0u*/,
	UInt32 uCdictDownloads /*= 0u*/,
	UInt32 uLoopDownloads /*= 0u*/,
	UInt32 uFetchSet /*= 0u*/,
	UInt32 uLoopProcess /*= 0u*/,
	UInt32 uPopulate /*= 0u*/)
{
	// Test stats.
	DownloadablePackageFileSystemStats stats;
	m_pSystem->GetStats(stats);

	// Request count.
	SEOUL_UNITTESTING_ASSERT_EQUAL(uRequests, m_pServer->GetReceivedRequestCount());

	// Event counts - expected to be all 0.
	T_EVT(init_cdict_download_count, uCdictDownloads);
	T_EVT(loop_download_count, uLoopDownloads);
	T_EVT(loop_fetch_set_count, uFetchSet);
	T_EVT(loop_process_count, uLoopProcess);
}

#undef T_EVT

/** Test to measure state changes - all files must be downloaded, expecting a certain number of requests and no population from cache. */
void DownloadablePackageFileSystemTest::MeasureAllDownload(Byte const* sPrefix)
{
	// No requests should have yet been made.
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, m_pServer->GetReceivedRequestCount());

	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), String::Printf("UnitTests/DownloadablePackageFileSystem/%sMeasure_PC_ConfigUpdate1.sar", sPrefix));

	DownloadablePackageFileSystemSettings settings;
	settings.m_sAbsolutePackageFilename = m_sTargetPackageFilename;
	settings.m_sInitialURL = String::Printf("http://localhost:8057/%sMeasure_PC_ConfigUpdate1.sar", sPrefix);
	m_pSystem = FileManager::Get()->RegisterFileSystem<DownloadablePackageFileSystem>(settings);

	WaitForPackageInitialize();

	MeasureEventCheck(3u, 1u);
	MeasureDownloadBytesCheck(51904, 0u); // Size of the compression dictionary - header/table not tracked.

	// Download all files.
	SEOUL_UNITTESTING_ASSERT(m_pSystem->Fetch(DownloadablePackageFileSystem::Files()));
	WaitForPackageWorkCompletion();
	MeasureEventCheck(11u, 1u, 8u, 8u, 1u);
	MeasureDownloadBytesCheck(51904, 1790936); // Total size of all data excluding header and file table.

	// Verify basic state.
	SEOUL_UNITTESTING_ASSERT(IsCrc32Ok(*m_pSystem));

	// The downloaded archive should be exactly the same as the source archive now.
	SEOUL_UNITTESTING_ASSERT(FilesAreEqual(m_sSourcePackageFilename, m_sTargetPackageFilename));

	// Now perform a fetch and verify still no events.
	SEOUL_UNITTESTING_ASSERT(m_pSystem->Fetch(DownloadablePackageFileSystem::Files()));
	WaitForPackageWorkCompletion();
	MeasureEventCheck(11u, 1u, 8u, 8u, 1u);
	MeasureDownloadBytesCheck(51904, 1790936); // Total size of all data excluding header and file table.
}

/** Test to measure state changes - all files must be downloaded, expecting a certain number of requests and no population from cache. */
void DownloadablePackageFileSystemTest::MeasureAllDownloadAdjusted(Byte const* sPrefix)
{
	// No requests should have yet been made.
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, m_pServer->GetReceivedRequestCount());

	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), String::Printf("UnitTests/DownloadablePackageFileSystem/%sMeasure_PC_ConfigUpdate1.sar", sPrefix));

	DownloadablePackageFileSystemSettings settings;
	settings.m_sAbsolutePackageFilename = m_sTargetPackageFilename;
	settings.m_sInitialURL = String::Printf("http://localhost:8057/%sMeasure_PC_ConfigUpdate1.sar", sPrefix);
	PatchablePackageFileSystem::AdjustSettings(settings);

	m_pSystem = FileManager::Get()->RegisterFileSystem<DownloadablePackageFileSystem>(settings);

	WaitForPackageInitialize();

	MeasureEventCheck(3u, 1u); // Three requests for header, file table, and compression dictionary.
	MeasureDownloadBytesCheck(51904u, 0u);

	// Download all files.
	SEOUL_UNITTESTING_ASSERT(m_pSystem->Fetch(DownloadablePackageFileSystem::Files()));
	WaitForPackageWorkCompletion();
	MeasureEventCheck(7u, 1u, 4u, 4u, 1u);
	MeasureDownloadBytesCheck(51904u, 1790948);

	// Verify basic state.
	SEOUL_UNITTESTING_ASSERT(IsCrc32Ok(*m_pSystem));

	// The downloaded archive should be exactly the same as the source archive now.
	SEOUL_UNITTESTING_ASSERT(FilesAreEqual(m_sSourcePackageFilename, m_sTargetPackageFilename));

	// Now perform a fetch and verify still no events.
	SEOUL_UNITTESTING_ASSERT(m_pSystem->Fetch(DownloadablePackageFileSystem::Files()));
	WaitForPackageWorkCompletion();
	MeasureEventCheck(7u, 1u, 4u, 4u, 1u);
	MeasureDownloadBytesCheck(51904u, 1790948);
}

/** Test to measure stat changes - expected that with all content local, init is fast with only a single request for a header. */
void DownloadablePackageFileSystemTest::MeasureAllFallback(Byte const* sPrefix)
{
	// No requests should have yet been made.
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, m_pServer->GetReceivedRequestCount());

	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), String::Printf("UnitTests/DownloadablePackageFileSystem/%sMeasure_PC_ConfigUpdate1.sar", sPrefix));

	DownloadablePackageFileSystemSettings settings;
	settings.m_sAbsolutePackageFilename = m_sTargetPackageFilename;
	settings.m_sInitialURL = String::Printf("http://localhost:8057/%sMeasure_PC_ConfigUpdate1.sar", sPrefix);
	settings.m_vPopulatePackages.PushBack(m_sSourcePackageFilename); // Add to the initial populate set.
	m_pSystem = FileManager::Get()->RegisterFileSystem<DownloadablePackageFileSystem>(settings);

	WaitForPackageInitialize();

	// Verify basic state.
	SEOUL_UNITTESTING_ASSERT(IsCrc32Ok(*m_pSystem));

	// Test stats.
	MeasureEventCheck(2u, 0u, 0u, 0u, 0u, 1u); // Request for header and file table.
	MeasureDownloadBytesCheck(0u, 0u);

	// Now perform a fetch and verify still no events.
	SEOUL_UNITTESTING_ASSERT(m_pSystem->Fetch(DownloadablePackageFileSystem::Files()));
	WaitForPackageWorkCompletion();
	MeasureEventCheck(2u, 0u, 0u, 0u, 0u, 1u);
	MeasureDownloadBytesCheck(0u, 0u);
}

/** Test to measure stat changes - expected that with all content local, init is fast with only a single request for a header. */
void DownloadablePackageFileSystemTest::MeasureAllLocal(Byte const* sPrefix)
{
	// No requests should have yet been made.
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, m_pServer->GetReceivedRequestCount());

	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), String::Printf("UnitTests/DownloadablePackageFileSystem/%sMeasure_PC_ConfigUpdate1.sar", sPrefix));
	SEOUL_UNITTESTING_ASSERT(CopyFile(m_sSourcePackageFilename, m_sTargetPackageFilename)); // Copy so all local at startup.

	DownloadablePackageFileSystemSettings settings;
	settings.m_sAbsolutePackageFilename = m_sTargetPackageFilename;
	settings.m_sInitialURL = String::Printf("http://localhost:8057/%sMeasure_PC_ConfigUpdate1.sar", sPrefix);
	m_pSystem = FileManager::Get()->RegisterFileSystem<DownloadablePackageFileSystem>(settings);

	WaitForPackageInitialize();

	// Verify basic state.
	SEOUL_UNITTESTING_ASSERT(IsCrc32Ok(*m_pSystem));

	// Test stats.
	MeasureEventCheck(1u); // Single request for header.
	MeasureDownloadBytesCheck(0u, 0u);

	// Now perform a fetch and verify still no events.
	SEOUL_UNITTESTING_ASSERT(m_pSystem->Fetch(DownloadablePackageFileSystem::Files()));
	WaitForPackageWorkCompletion();
	MeasureEventCheck(1u);
	MeasureDownloadBytesCheck(0u, 0u);
}

/**
 * Test to measure stat changes - on a .sar change in which case the old sar
 * contains all files, expectation is only 2 downloads (header and file table)
 * and a single populate_from call.
 */
void DownloadablePackageFileSystemTest::MeasureAllMigrated(Byte const* sPrefix)
{
	// No requests should have yet been made.
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, m_pServer->GetReceivedRequestCount());

	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), String::Printf("UnitTests/DownloadablePackageFileSystem/%sMeasure_PC_ConfigUpdate1.sar", sPrefix));
	SEOUL_UNITTESTING_ASSERT(CopyFile(m_sSourcePackageFilename, m_sTargetPackageFilename)); // Copy so all local at startup.

	DownloadablePackageFileSystemSettings settings;
	settings.m_sAbsolutePackageFilename = m_sTargetPackageFilename;
	settings.m_sInitialURL = String::Printf("http://localhost:8057/%sMeasure_PC_ConfigUpdate2.sar", sPrefix); // Point at v2, which is the exact same archive but in reverse.
	m_pSystem = FileManager::Get()->RegisterFileSystem<DownloadablePackageFileSystem>(settings);

	WaitForPackageInitialize();

	// Verify basic state.
	SEOUL_UNITTESTING_ASSERT(IsCrc32Ok(*m_pSystem));

	// Test stats.
	MeasureEventCheck(2u, 0u, 0u, 0u, 0u, 1u); // Two requests only, header and file table and a single populate action.
	MeasureDownloadBytesCheck(0u, 0u);

	// Now perform a fetch and verify still no events.
	SEOUL_UNITTESTING_ASSERT(m_pSystem->Fetch(DownloadablePackageFileSystem::Files()));
	WaitForPackageWorkCompletion();
	MeasureEventCheck(2u, 0u, 0u, 0u, 0u, 1u);
	MeasureDownloadBytesCheck(0u, 0u);
}

void DownloadablePackageFileSystemTest::MeasurePartialDownload(Byte const* sPrefix)
{
	// No requests should have yet been made.
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, m_pServer->GetReceivedRequestCount());

	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), String::Printf("UnitTests/DownloadablePackageFileSystem/%sMeasure_PC_ConfigUpdate1.sar", sPrefix));
	SEOUL_UNITTESTING_ASSERT(CopyFile(m_sSourcePackageFilename, m_sTargetPackageFilename)); // Copy so all local at startup.

	DownloadablePackageFileSystemSettings settings;
	settings.m_sAbsolutePackageFilename = m_sTargetPackageFilename;
	settings.m_sInitialURL = String::Printf("http://localhost:8057/%sMeasure_PC_ConfigUpdate3.sar", sPrefix); // Point at v3, which is older data and will result in partial local population.
	m_pSystem = FileManager::Get()->RegisterFileSystem<DownloadablePackageFileSystem>(settings);

	WaitForPackageInitialize();

	// Test stats.
	MeasureEventCheck(2u, 0u, 0u, 0u, 0u, 1u); // Two requests only, header and file table and a single populate action.
	MeasureDownloadBytesCheck(0u, 0u);

	// Now perform a fetch and verify some download events.
	SEOUL_UNITTESTING_ASSERT(m_pSystem->Fetch(DownloadablePackageFileSystem::Files()));
	WaitForPackageWorkCompletion();
	MeasureEventCheck(19u, 0u, 17u, 17u, 1u, 1u);
	MeasureDownloadBytesCheck(0u, 645700);

	// Verify basic state.
	SEOUL_UNITTESTING_ASSERT(IsCrc32Ok(*m_pSystem));
}

void DownloadablePackageFileSystemTest::MeasurePartialDownloadAdjusted(Byte const* sPrefix)
{
	// No requests should have yet been made.
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, m_pServer->GetReceivedRequestCount());

	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), String::Printf("UnitTests/DownloadablePackageFileSystem/%sMeasure_PC_ConfigUpdate1.sar", sPrefix));
	SEOUL_UNITTESTING_ASSERT(CopyFile(m_sSourcePackageFilename, m_sTargetPackageFilename)); // Copy so all local at startup.

	DownloadablePackageFileSystemSettings settings;
	settings.m_sAbsolutePackageFilename = m_sTargetPackageFilename;
	settings.m_sInitialURL = String::Printf("http://localhost:8057/%sMeasure_PC_ConfigUpdate3.sar", sPrefix); // Point at v3, which is older data and will result in partial local population.
	PatchablePackageFileSystem::AdjustSettings(settings);

	m_pSystem = FileManager::Get()->RegisterFileSystem<DownloadablePackageFileSystem>(settings);

	WaitForPackageInitialize();

	// Test stats.
	MeasureEventCheck(2u, 0u, 0u, 0u, 0u, 1u); // Two requests only, header and file table and a single populate action.
	MeasureDownloadBytesCheck(0u, 0u);

	// Now perform a fetch and verify some download events.
	SEOUL_UNITTESTING_ASSERT(m_pSystem->Fetch(DownloadablePackageFileSystem::Files()));
	WaitForPackageWorkCompletion();
	MeasureEventCheck(10u, 0u, 8u, 8u, 1u, 1u);
	MeasureDownloadBytesCheck(0u, 766293);

	// Verify basic state.
	SEOUL_UNITTESTING_ASSERT(IsCrc32Ok(*m_pSystem));
}

void DownloadablePackageFileSystemTest::MeasurePartialFallback(Byte const* sPrefix)
{
	// No requests should have yet been made.
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, m_pServer->GetReceivedRequestCount());

	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), String::Printf("UnitTests/DownloadablePackageFileSystem/%sMeasure_PC_ConfigUpdate1.sar", sPrefix));

	DownloadablePackageFileSystemSettings settings;
	settings.m_sAbsolutePackageFilename = m_sTargetPackageFilename;
	settings.m_sInitialURL = String::Printf("http://localhost:8057/%sMeasure_PC_ConfigUpdate3.sar", sPrefix); // Point at v3, which is older data and will result in partial local population.
	settings.m_vPopulatePackages.PushBack(m_sSourcePackageFilename);
	PatchablePackageFileSystem::AdjustSettings(settings);

	m_pSystem = FileManager::Get()->RegisterFileSystem<DownloadablePackageFileSystem>(settings);

	WaitForPackageInitialize();

	// Test stats.
	MeasureEventCheck(2u, 0u, 0u, 0u, 0u, 1u); // Two requests only, header and file table and a single populate action.
	MeasureDownloadBytesCheck(0u, 0u);

	// Now perform a fetch and verify some download events.
	SEOUL_UNITTESTING_ASSERT(m_pSystem->Fetch(DownloadablePackageFileSystem::Files()));
	WaitForPackageWorkCompletion();
	MeasureEventCheck(10u, 0u, 8u, 8u, 1u, 1u);
	MeasureDownloadBytesCheck(0u, 766293);

	// Verify basic state.
	SEOUL_UNITTESTING_ASSERT(IsCrc32Ok(*m_pSystem));
}

namespace
{

struct Utility SEOUL_SEALED
{
	SEOUL_DELEGATE_TARGET(Utility);

	HTTP::CallbackResult OnComplete(HTTP::Result eResult, HTTP::Response* pResponse)
	{
		SeoulMemoryBarrier();
		m_bComplete = true;
		SeoulMemoryBarrier();

		m_Signal.Activate();
		return HTTP::CallbackResult::kSuccess;
	}

	Atomic32Value<Bool> m_bComplete;
	Signal m_Signal;
}; // struct HTTP::TestUtility

} // namespace

/**
 * This test would normally be a Benchmark, but we're timing
 * very specific internal values of the entire downloader process,
 * so it is implemented instead as a regular unit test.
 */
void DownloadablePackageFileSystemTest::MeasureTimePartialDownload(Byte const* sPrefix)
{
	// TODO: Not entirely sure why, but curl seems to spin for almost a half second
	// to a second on the first request that is ever issued. So I'm issuing this
	// dummy request to "prime" so it doesn't show up in time measurements below.
	{
		Utility utility;
		{
			auto& r = HTTP::Manager::Get()->CreateRequest();
			r.SetCallback(SEOUL_BIND_DELEGATE(&Utility::OnComplete, &utility));
			r.SetDispatchCallbackOnMainThread(false);
			r.SetResendOnFailure(false);
			r.SetURL("http://localhost:8057/");
			r.Start();
		}
		while (!utility.m_bComplete)
		{
			utility.m_Signal.Wait();
		}
	}

	// No requests should have yet been made.
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, m_pServer->GetReceivedRequestCount());

	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), String::Printf("UnitTests/DownloadablePackageFileSystem/%sMeasure_PC_ConfigUpdate1.sar", sPrefix));
	SEOUL_UNITTESTING_ASSERT(CopyFile(m_sSourcePackageFilename, m_sTargetPackageFilename)); // Copy so all local at startup.

	DownloadablePackageFileSystemSettings settings;
	settings.m_sAbsolutePackageFilename = m_sTargetPackageFilename;
	settings.m_sInitialURL = String::Printf("http://localhost:8057/%sMeasure_PC_ConfigUpdate3.sar", sPrefix); // Point at v3, which is older data and will result in partial local population.
	PatchablePackageFileSystem::AdjustSettings(settings);

	// We also time the overall operation until the first
	// work completion.
	auto const iStart = SeoulTime::GetGameTimeInTicks();
	m_pSystem = FileManager::Get()->RegisterFileSystem<DownloadablePackageFileSystem>(settings);

	WaitForPackageInitialize();

	// Test stats.
	MeasureEventCheck(3u, 0u, 0u, 0u, 0u, 1u); // Two requests only, header and file table and a single populate action.
	MeasureDownloadBytesCheck(0u, 0u);

	// Now perform a fetch and verify some download events.
	SEOUL_UNITTESTING_ASSERT(m_pSystem->Fetch(DownloadablePackageFileSystem::Files()));
	WaitForPackageWorkCompletion();
	MeasureEventCheck(11u, 0u, 8u, 8u, 1u, 1u);
	MeasureDownloadBytesCheck(0u, 766293);

	// End time of the overall operation.
	auto const iEnd = SeoulTime::GetGameTimeInTicks();

	// Verify basic state.
	SEOUL_UNITTESTING_ASSERT(IsCrc32Ok(*m_pSystem));

	// Stat testing - this is the interesting part of the test.
	// If any of the critical times are above a threshold (this
	// was empirically derived on a min spec Android device,
	// Nexus 7 (2012), we fail the test after first reporting
	// the times).
	//
	// Also note that download time assumes a local server connection,
	// therefore eliminating/ignoring bandwidth. We're measuring
	// thread contention and overhead of the HTTP system and interaction
	// with this time.
	static const Double kfOverallMs = 700.0;
	static const Double kfInitMs = 500.0;
	static const Double kfInitCdictDownloadMs = 0.0;
	static const Double kfInitPopulateMs = 200.0;
	static const Double kfLoopDownloadMs = 150.0;

	// Acquire stats and check - if any fail, write out
	// all values and then fail the test.
	{
		// Test stats.
		DownloadablePackageFileSystemStats stats;
		m_pSystem->GetStats(stats);

		// Simplicity.
		SEOUL_UNITTESTING_ASSERT(stats.m_tTimes.Insert(HString("test_overall"), iEnd - iStart).Second);

		// Check values.
		if (GET_TIME_MS(test_overall) > kfOverallMs ||
			GET_TIME_MS(init) > kfInitMs ||
			GET_TIME_MS(init_cdict_download) > kfInitCdictDownloadMs ||
			GET_TIME_MS(init_populate) > kfInitPopulateMs ||
			GET_TIME_MS(loop_download) > kfLoopDownloadMs)
		{
			// Gather.
			Vector< Pair<HString, Int64> > v;
			for (auto const& pair : stats.m_tTimes)
			{
				v.PushBack(MakePair(pair.First, pair.Second));
			}

			// Arrange from largest to smallest.
			QuickSort(v.Begin(), v.End(), [&](const Pair<HString, Int64>& a, const Pair<HString, Int64>& b)
			{
				return (a.Second > b.Second);
			});

			// Log.
			for (auto const& pair : v)
			{
				SEOUL_LOG("%s: %.2f ms",
					pair.First.CStr(),
					SeoulTime::ConvertTicksToMilliseconds(pair.Second));
			}

			// Fail the test.
			SEOUL_UNITTESTING_ASSERT_MESSAGE(false, "one or more time thresholds exceeded target.");
		}
	}
}

void DownloadablePackageFileSystemTest::TestBadHeader()
{
	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/DownloadablePackageFileSystem/PC_BadHeader.sar");

	// Configure downloader with default settings.
	DownloadablePackageFileSystemSettings settings;
	settings.m_sAbsolutePackageFilename = m_sTargetPackageFilename;
	settings.m_sInitialURL = "http://localhost:8057/PC_BadHeader.sar";
	m_pSystem = FileManager::Get()->RegisterFileSystem<DownloadablePackageFileSystem>(settings);

	InternalInitializeFailureCommon(false);
}

void DownloadablePackageFileSystemTest::TestBasic()
{
	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/DownloadablePackageFileSystem/PC_Content.sar");

	// Configure downloader with default settings.
	DownloadablePackageFileSystemSettings settings;
	settings.m_sAbsolutePackageFilename = m_sTargetPackageFilename;
	settings.m_sInitialURL = "http://localhost:8057/PC_Content.sar";

	m_pSystem = FileManager::Get()->RegisterFileSystem<DownloadablePackageFileSystem>(settings);

	InternalTestCommon();
}

void DownloadablePackageFileSystemTest::TestBasicCompressed()
{
	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/DownloadablePackageFileSystem/PC_Config.sar");

	// Configure downloader with default settings.
	DownloadablePackageFileSystemSettings settings;
	settings.m_sAbsolutePackageFilename = m_sTargetPackageFilename;
	settings.m_sInitialURL = "http://localhost:8057/PC_Config.sar";

	m_pSystem = FileManager::Get()->RegisterFileSystem<DownloadablePackageFileSystem>(settings);

	WaitForPackageInitialize();

	m_pSystem->Prefetch(DownloadablePackageFileSystem::Files());

	WaitForPackageWorkCompletion();

	IPackageFileSystem::FileTable t;
	SEOUL_UNITTESTING_ASSERT(m_pSystem->GetFileTable(t));
	SEOUL_UNITTESTING_ASSERT_EQUAL(26, t.GetSize());
}

// Test specifically designed to hit certain paths
// in downloading and verifying an archive's compression dict.
void DownloadablePackageFileSystemTest::TestCompressionDictPaths()
{
	{
		// No requests should have yet been made.
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, m_pServer->GetReceivedRequestCount());

		m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/DownloadablePackageFileSystem/Regress1_PC_Config.sar");
		SEOUL_UNITTESTING_ASSERT(CopyFile(m_sSourcePackageFilename, m_sTargetPackageFilename));

		DownloadablePackageFileSystemSettings settings;
		settings.m_sAbsolutePackageFilename = m_sTargetPackageFilename;
		settings.m_sInitialURL = "http://localhost:8057/Regress1_PC_ConfigUpdateB.sar";
		PatchablePackageFileSystem::AdjustSettings(settings);

		m_pSystem = FileManager::Get()->RegisterFileSystem<DownloadablePackageFileSystem>(settings);

		WaitForPackageInitialize();

		// Should now have 3 requests (header, file table, and compression dict).
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, m_pServer->GetReceivedRequestCount());

		// Fetch all.
		SEOUL_UNITTESTING_ASSERT(m_pSystem->Fetch(DownloadablePackageFileSystem::Files()));

		WaitForPackageWorkCompletion();

		// One more request for changed data.
		SEOUL_UNITTESTING_ASSERT_EQUAL(4, m_pServer->GetReceivedRequestCount());

		// Fully downloaded.
		SEOUL_UNITTESTING_ASSERT(IsCrc32Ok(*m_pSystem));

		// Check download state.
		AllFiles v;
		GetAll(*m_pSystem, v);
		Check(settings, *m_pSystem, v, [&](UInt32 uIndex) { return true; });

		// Check service.
		for (auto const& e : v)
		{
			SEOUL_UNITTESTING_ASSERT(!m_pSystem->IsServicedByNetwork(e.First));
		}

		// The downloaded archive should be exactly the same as the desired archive now.
		SEOUL_UNITTESTING_ASSERT(FilesAreEqual(
			Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/DownloadablePackageFileSystem/Regress1_PC_ConfigUpdateB.sar"),
			m_sTargetPackageFilename));
	}

	// Reset.
	auto const s = m_sTargetPackageFilename;
	Destroy();
	m_sTargetPackageFilename = s;
	Init();

	// Next pass
	{
		// No requests should have yet been made.
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, m_pServer->GetReceivedRequestCount());

		m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/DownloadablePackageFileSystem/Regress1_PC_ConfigUpdateB.sar");

		DownloadablePackageFileSystemSettings settings;
		settings.m_sAbsolutePackageFilename = m_sTargetPackageFilename;
		settings.m_sInitialURL = "http://localhost:8057/Regress1_PC_ConfigUpdateB.sar";
		PatchablePackageFileSystem::AdjustSettings(settings);

		m_pSystem = FileManager::Get()->RegisterFileSystem<DownloadablePackageFileSystem>(settings);

		WaitForPackageInitialize();

		// Only one request (for the header).
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, m_pServer->GetReceivedRequestCount());

		// Fetch all.
		SEOUL_UNITTESTING_ASSERT(m_pSystem->Fetch(DownloadablePackageFileSystem::Files()));

		WaitForPackageWorkCompletion();

		SEOUL_UNITTESTING_ASSERT_EQUAL(1, m_pServer->GetReceivedRequestCount());

		// Fully downloaded.
		SEOUL_UNITTESTING_ASSERT(IsCrc32Ok(*m_pSystem));

		// Check download state.
		AllFiles v;
		GetAll(*m_pSystem, v);
		Check(settings, *m_pSystem, v, [&](UInt32 uIndex) { return true; });

		// Check service.
		for (auto const& e : v)
		{
			SEOUL_UNITTESTING_ASSERT(!m_pSystem->IsServicedByNetwork(e.First));
		}

		// The downloaded archive should be exactly the same as the desired archive now.
		SEOUL_UNITTESTING_ASSERT(FilesAreEqual(
			Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/DownloadablePackageFileSystem/Regress1_PC_ConfigUpdateB.sar"),
			m_sTargetPackageFilename));
	}
}

void DownloadablePackageFileSystemTest::TestEdgeCases()
{
	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/DownloadablePackageFileSystem/PC_Content.sar");

	// Configure downloader with default settings.
	DownloadablePackageFileSystemSettings settings;
	settings.m_sAbsolutePackageFilename = m_sTargetPackageFilename;
	settings.m_sInitialURL = "http://localhost:8057/PC_Content.sar";

	m_pSystem = FileManager::Get()->RegisterFileSystem<DownloadablePackageFileSystem>(settings);

	WaitForPackageInitialize();

	// Prefetch of non-existent files.
	SEOUL_UNITTESTING_ASSERT(!m_pSystem->Prefetch(FilePath::CreateContentFilePath("a")));
	SEOUL_UNITTESTING_ASSERT(!m_pSystem->Prefetch(DownloadablePackageFileSystem::Files(1, FilePath::CreateContentFilePath("a"))));

	// Prefetch, then increase priority.
	SEOUL_UNITTESTING_ASSERT(m_pSystem->Prefetch(DownloadablePackageFileSystem::Files()));
	SEOUL_UNITTESTING_ASSERT(m_pSystem->Prefetch(DownloadablePackageFileSystem::Files(), NetworkFetchPriority::kCritical));

	WaitForPackageWorkCompletion();

	// Prefetch again, this should now be a nop (no new requests should occur).
	auto const before = m_pServer->GetReceivedRequestCount();
	SEOUL_UNITTESTING_ASSERT(m_pSystem->Prefetch(DownloadablePackageFileSystem::Files()));
	WaitForPackageWorkCompletion();
	SEOUL_UNITTESTING_ASSERT_EQUAL(before, m_pServer->GetReceivedRequestCount());
}

void DownloadablePackageFileSystemTest::TestEdgeCases2()
{
	// CRC32 checks with invalid package.
	{
		PackageFileSystem pkg{ String() };
		SEOUL_UNITTESTING_ASSERT(!pkg.PerformCrc32Check());
		PackageCrc32Entries v;
		SEOUL_UNITTESTING_ASSERT(!pkg.PerformCrc32Check(&v));
		SEOUL_UNITTESTING_ASSERT(v.IsEmpty());
		PackageCrc32Entry entry;
		entry.m_FilePath = FilePath::CreateContentFilePath("DoesNotExist.png");
		entry.m_bCrc32Ok = true;
		v.PushBack(entry);
		SEOUL_UNITTESTING_ASSERT(!pkg.PerformCrc32Check(&v));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1u, v.GetSize());
		SEOUL_UNITTESTING_ASSERT(!v.Front().m_bCrc32Ok);
	}

	// Invalid variations header.
	{
		{
			PackageFileSystem pkg{ nullptr, 0u, false };
			SEOUL_UNITTESTING_ASSERT(!pkg.IsOk());
			SEOUL_UNITTESTING_ASSERT(!pkg.PerformCrc32Check());
		}
		{
			PackageFileSystem pkg{ nullptr, 0u, true };
			SEOUL_UNITTESTING_ASSERT(!pkg.IsOk());
			SEOUL_UNITTESTING_ASSERT(!pkg.PerformCrc32Check());
		}
		// ReadPackageHeader
		{
			PackageFileHeader header{};
			SEOUL_UNITTESTING_ASSERT(!PackageFileSystem::ReadPackageHeader(nullptr, 0u, header));
			SEOUL_UNITTESTING_ASSERT(!PackageFileSystem::ReadPackageHeader((void*)1, 1u, header));

			SEOUL_UNITTESTING_ASSERT(!PackageFileSystem::ReadPackageHeader(&header, sizeof(header), header));

			header.m_uSignature = kuPackageSignature;
			SEOUL_UNITTESTING_ASSERT(!PackageFileSystem::ReadPackageHeader(&header, sizeof(header), header));
			header.m_uVersion = PackageFileHeader::ku16_LZ4CompressionVersion;
			SEOUL_UNITTESTING_ASSERT(!PackageFileSystem::ReadPackageHeader(&header, sizeof(header), header));
			header.m_uVersion = PackageFileHeader::ku17_PreCompressionDictVersion;
			SEOUL_UNITTESTING_ASSERT(!PackageFileSystem::ReadPackageHeader(&header, sizeof(header), header));
			header.m_uVersion = PackageFileHeader::ku18_PreDualCrc32Version;
			SEOUL_UNITTESTING_ASSERT(!PackageFileSystem::ReadPackageHeader(&header, sizeof(header), header));
			header.m_uVersion = kuPackageVersion;
			SEOUL_UNITTESTING_ASSERT(!PackageFileSystem::ReadPackageHeader(&header, sizeof(header), header));

			header.SetGameDirectory(Convert(GameDirectory::kConfig));
			SEOUL_UNITTESTING_ASSERT(PackageFileSystem::ReadPackageHeader(&header, sizeof(header), header));
			header.SetPlatformAndObfuscation(Platform::kLinux, true);
			SEOUL_UNITTESTING_ASSERT(PackageFileSystem::ReadPackageHeader(&header, sizeof(header), header));
			header.SetPlatformAndObfuscation((Platform)-1, true);
			SEOUL_UNITTESTING_ASSERT(!PackageFileSystem::ReadPackageHeader(&header, sizeof(header), header));
		}
		// CheckSarHeader
		{
			PackageFileHeader header{};
			SEOUL_UNITTESTING_ASSERT(!PackageFileSystem::CheckSarHeader(nullptr, 0u));
			SEOUL_UNITTESTING_ASSERT(!PackageFileSystem::CheckSarHeader((void*)1, 1u));

			SEOUL_UNITTESTING_ASSERT(!PackageFileSystem::CheckSarHeader(&header, sizeof(header)));

			header.m_uSignature = kuPackageSignature;
			SEOUL_UNITTESTING_ASSERT(!PackageFileSystem::CheckSarHeader(&header, sizeof(header)));
			header.m_uVersion = PackageFileHeader::ku16_LZ4CompressionVersion;
			SEOUL_UNITTESTING_ASSERT(!PackageFileSystem::CheckSarHeader(&header, sizeof(header)));
			header.m_uVersion = PackageFileHeader::ku17_PreCompressionDictVersion;
			SEOUL_UNITTESTING_ASSERT(!PackageFileSystem::CheckSarHeader(&header, sizeof(header)));
			header.m_uVersion = PackageFileHeader::ku18_PreDualCrc32Version;
			SEOUL_UNITTESTING_ASSERT(!PackageFileSystem::CheckSarHeader(&header, sizeof(header)));
			header.m_uVersion = kuPackageVersion;
			SEOUL_UNITTESTING_ASSERT(!PackageFileSystem::CheckSarHeader(&header, sizeof(header)));

			header.SetGameDirectory(Convert(GameDirectory::kConfig));
			SEOUL_UNITTESTING_ASSERT(PackageFileSystem::CheckSarHeader(&header, sizeof(header)));
			header.SetPlatformAndObfuscation(Platform::kLinux, true);
			SEOUL_UNITTESTING_ASSERT(PackageFileSystem::CheckSarHeader(&header, sizeof(header)));
			header.SetPlatformAndObfuscation((Platform)-1, true);
			SEOUL_UNITTESTING_ASSERT(!PackageFileSystem::CheckSarHeader(&header, sizeof(header)));
		}
	}
}

void DownloadablePackageFileSystemTest::TestExisting()
{
	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/DownloadablePackageFileSystem/PC_Content.sar");
	SEOUL_UNITTESTING_ASSERT(CopyFile(m_sSourcePackageFilename, m_sTargetPackageFilename));

	// Configure downloader with default settings.
	DownloadablePackageFileSystemSettings settings;
	settings.m_sAbsolutePackageFilename = m_sTargetPackageFilename;
	settings.m_sInitialURL = "http://localhost:8057/PC_Content.sar";

	m_pSystem = FileManager::Get()->RegisterFileSystem<DownloadablePackageFileSystem>(settings);

	WaitForPackageInitialize();
	WaitForPackageWorkCompletion();

	// Entire archive should be downloaded and ready, as it was populated from the cache.
	SEOUL_UNITTESTING_ASSERT(IsCrc32Ok(*m_pSystem));
	SEOUL_UNITTESTING_ASSERT(m_pSystem->IsOk());

	// Run the normal test.
	InternalTestCommon();

	// Make sure only 1 request was made (for the header).
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, m_pServer->GetReceivedRequestCount());
}

void DownloadablePackageFileSystemTest::TestFetch()
{
	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/DownloadablePackageFileSystem/PC_Content.sar");

	// Configure downloader with default settings.
	DownloadablePackageFileSystemSettings settings;
	settings.m_sAbsolutePackageFilename = m_sTargetPackageFilename;
	settings.m_sInitialURL = "http://localhost:8057/PC_Content.sar";

	m_pSystem = FileManager::Get()->RegisterFileSystem<DownloadablePackageFileSystem>(settings);

	WaitForPackageInitialize();

	DownloadablePackageFileSystem::Files vFiles;
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

void DownloadablePackageFileSystemTest::TestGarbageFile()
{
	// Identical to TestBasic(), except the file data is cleared with garbage
	// prior to initializing the file system, to make sure the DownloadablePackageFileSystem
	// is correct.
	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/DownloadablePackageFileSystem/PC_Content.sar");
	WriteGarbageToTargetFile();

	// Configure downloader with default settings.
	DownloadablePackageFileSystemSettings settings;
	settings.m_sAbsolutePackageFilename = m_sTargetPackageFilename;
	settings.m_sInitialURL = "http://localhost:8057/PC_Content.sar";

	m_pSystem = FileManager::Get()->RegisterFileSystem<DownloadablePackageFileSystem>(settings);

	InternalTestCommon();
}

void DownloadablePackageFileSystemTest::TestGetDirectoryListing()
{
	// No requests should have yet been made.
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, m_pServer->GetReceivedRequestCount());

	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/DownloadablePackageFileSystem/Regress1_PC_Config.sar");
	SEOUL_UNITTESTING_ASSERT(CopyFile(m_sSourcePackageFilename, m_sTargetPackageFilename));

	DownloadablePackageFileSystemSettings settings;
	settings.m_sAbsolutePackageFilename = m_sTargetPackageFilename;
	settings.m_sInitialURL = "http://localhost:8057/Regress1_PC_ConfigUpdateB.sar";

	m_pSystem = FileManager::Get()->RegisterFileSystem<DownloadablePackageFileSystem>(settings);

	WaitForPackageInitialize();

	// Directory listing failure.
	{
		FilePath dirPath;
		dirPath.SetDirectory(GameDirectory::kContent);
		Vector<String> vs;
		SEOUL_UNITTESTING_ASSERT(!m_pSystem->GetDirectoryListing(
			dirPath,
			vs,
			false,
			true,
			String()));
	}

	// Directory listing success.
	{
		FilePath dirPath;
		dirPath.SetDirectory(GameDirectory::kConfig);
		Vector<String> vs;
		SEOUL_UNITTESTING_ASSERT(m_pSystem->GetDirectoryListing(
			dirPath,
			vs,
			false,
			true,
			String()));
		SEOUL_UNITTESTING_ASSERT_EQUAL(24u, vs.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			FilePath::CreateConfigFilePath(vs.Front()),
			FilePath::CreateConfigFilePath("Animation2Ds/Test.json"));
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			FilePath::CreateConfigFilePath(vs.Back()),
			FilePath::CreateConfigFilePath("UI/Screens2.json"));

		// Directory inclusion not supported.
		SEOUL_UNITTESTING_ASSERT(!m_pSystem->GetDirectoryListing(
			dirPath,
			vs,
			true,
			true,
			String()));
		SEOUL_UNITTESTING_ASSERT_EQUAL(24u, vs.GetSize());

		SEOUL_UNITTESTING_ASSERT(m_pSystem->GetDirectoryListing(
			dirPath,
			vs,
			false,
			false,
			String()));
		SEOUL_UNITTESTING_ASSERT_EQUAL(7u, vs.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			FilePath::CreateConfigFilePath(vs.Front()),
			FilePath::CreateConfigFilePath("app_root_cert.pem"));
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			FilePath::CreateConfigFilePath(vs.Back()),
			FilePath::CreateConfigFilePath("TextEffectSettings.json"));

		SEOUL_UNITTESTING_ASSERT(m_pSystem->GetDirectoryListing(
			dirPath,
			vs,
			false,
			false,
			".json"));
		SEOUL_UNITTESTING_ASSERT_EQUAL(5u, vs.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			FilePath::CreateConfigFilePath(vs.Front()),
			FilePath::CreateConfigFilePath("Application.json"));
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			FilePath::CreateConfigFilePath(vs.Back()),
			FilePath::CreateConfigFilePath("TextEffectSettings.json"));

		SEOUL_UNITTESTING_ASSERT(m_pSystem->GetDirectoryListing(
			dirPath,
			vs,
			false,
			false,
			".dat"));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1u, vs.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			FilePath::CreateConfigFilePath(vs.Front()),
			FilePath::CreateConfigFilePath("pkgcdict_PC.dat"));
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			FilePath::CreateConfigFilePath(vs.Back()),
			FilePath::CreateConfigFilePath("pkgcdict_PC.dat"));

		dirPath.SetRelativeFilenameWithoutExtension(FilePathRelativeFilename("Animation2Ds"));
		SEOUL_UNITTESTING_ASSERT(m_pSystem->GetDirectoryListing(
			dirPath,
			vs,
			false,
			false,
			".json"));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1u, vs.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			FilePath::CreateConfigFilePath(vs.Front()),
			FilePath::CreateConfigFilePath("Animation2Ds/Test.json"));
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			FilePath::CreateConfigFilePath(vs.Back()),
			FilePath::CreateConfigFilePath("Animation2Ds/Test.json"));
	}
}

void DownloadablePackageFileSystemTest::TestLargeFile()
{
	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/DownloadablePackageFileSystem/PC_MusicContent.sar");

	// Configure downloader with default settings.
	DownloadablePackageFileSystemSettings settings;
	settings.m_sAbsolutePackageFilename = m_sTargetPackageFilename;
	settings.m_sInitialURL = "http://localhost:8057/PC_MusicContent.sar";

	m_pSystem = FileManager::Get()->RegisterFileSystem<DownloadablePackageFileSystem>(settings);

	WaitForPackageInitialize();

	ScopedPtr<SyncFile> pFile;
	SEOUL_UNITTESTING_ASSERT(m_pSystem->Open(
		FilePath::CreateContentFilePath("Authored/Sound/Music_bank01.bank"), File::kRead, pFile));
	SEOUL_UNITTESTING_ASSERT(!pFile->CanWrite());
	SEOUL_UNITTESTING_ASSERT(!pFile->Flush());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, pFile->WriteRawData(nullptr, 0u));
	SEOUL_UNITTESTING_ASSERT_EQUAL(
		FilePath::CreateContentFilePath("Authored/Sound/Music_bank01.bank").GetAbsoluteFilename(),
		pFile->GetAbsoluteFilename());
	Int64 iPos = -1;
	SEOUL_UNITTESTING_ASSERT(pFile->GetCurrentPositionIndicator(iPos));
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, iPos);

	SEOUL_UNITTESTING_ASSERT(pFile->Seek(10, File::kSeekFromCurrent));
	SEOUL_UNITTESTING_ASSERT(pFile->GetCurrentPositionIndicator(iPos));
	SEOUL_UNITTESTING_ASSERT_EQUAL(10, iPos);

	SEOUL_UNITTESTING_ASSERT(pFile->Seek(5, File::kSeekFromStart));
	SEOUL_UNITTESTING_ASSERT(pFile->GetCurrentPositionIndicator(iPos));
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, iPos);

	SEOUL_UNITTESTING_ASSERT(pFile->Seek(5, File::kSeekFromEnd));
	SEOUL_UNITTESTING_ASSERT(pFile->GetCurrentPositionIndicator(iPos));
	SEOUL_UNITTESTING_ASSERT_EQUAL((Int64)pFile->GetSize() - 5, iPos);

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
		"UnitTests/DownloadablePackageFileSystem/Music_bank01.bank"),
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

void DownloadablePackageFileSystemTest::TestMiscApi(Byte const* sPrefix)
{
	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), String::Printf("UnitTests/DownloadablePackageFileSystem/%sMeasure_PC_ConfigUpdate1.sar", sPrefix));

	// Configure downloader with default settings.
	DownloadablePackageFileSystemSettings settings;
	settings.m_sAbsolutePackageFilename = m_sTargetPackageFilename;
	settings.m_sInitialURL = String::Printf("http://localhost:8057/%sMeasure_PC_ConfigUpdate1.sar", sPrefix);

	m_pSystem = FileManager::Get()->RegisterFileSystem<DownloadablePackageFileSystem>(settings);

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
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, m_pSystem->GetActiveSyncFileCount());
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
		SEOUL_UNITTESTING_ASSERT_EQUAL(1537939686u, u);
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

void DownloadablePackageFileSystemTest::TestNoServer()
{
	m_pServer.Reset();

	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/DownloadablePackageFileSystem/PC_Content.sar");

	// Configure downloader with default settings.
	DownloadablePackageFileSystemSettings settings;
	settings.m_sAbsolutePackageFilename = m_sTargetPackageFilename;
	settings.m_sInitialURL = "http://localhost:8057/PC_Content.sar";

	m_pSystem = FileManager::Get()->RegisterFileSystem<DownloadablePackageFileSystem>(settings);

	InternalInitializeFailureCommon(false);
}

void DownloadablePackageFileSystemTest::TestPopulate()
{
	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/DownloadablePackageFileSystem/PC_Content.sar");

	// Configure downloader with default settings.
	DownloadablePackageFileSystemSettings settings;
	settings.m_sAbsolutePackageFilename = m_sTargetPackageFilename;
	settings.m_sInitialURL = "http://localhost:8057/PC_Content.sar";
	settings.m_vPopulatePackages.PushBack(m_sSourcePackageFilename); // Populate from source.

	m_pSystem = FileManager::Get()->RegisterFileSystem<DownloadablePackageFileSystem>(settings);

	WaitForPackageInitialize();

	// All files should be fully present now, the archive should be ok.
	SEOUL_UNITTESTING_ASSERT(IsCrc32Ok(*m_pSystem));

	// The downloaded archive should be exactly the same as the source archive now.
	SEOUL_UNITTESTING_ASSERT(FilesAreEqual(m_sSourcePackageFilename, m_sTargetPackageFilename));
}

void DownloadablePackageFileSystemTest::TestPopulateFromIncompatibleArchives()
{
	// First set of tests.
	{
		m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/DownloadablePackageFileSystem/PC_Content.sar");

		// Configure downloader with default settings.
		DownloadablePackageFileSystemSettings settings;
		settings.m_sAbsolutePackageFilename = m_sTargetPackageFilename;
		settings.m_sInitialURL = "http://localhost:8057/PC_Content.sar";

		// Incompatible obfuscation setting (this call succeeds but the operation under the hood will do nothing).
		settings.m_vPopulatePackages.PushBack(Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/DownloadablePackageFileSystem/PC_ConfigObfuscated.sar"));

		// Incompatible compression version (old LZ4 vs. new ZSTD).
		{
			auto const s(GenArchive(kuPackageVersion, GameDirectory::kContent, 1u, 1u, false, Platform::kPC));
			settings.m_vPopulatePackages.PushBack(s);
		}

		// Incompatible compression dict settings.
		{
			static const UInt32 kuSamples = 10u;

			FixedArray< FixedArray<UInt8, 128u>, kuSamples> aaData;
			for (UInt32 j = 0u; j < kuSamples; ++j)
			{
				for (UInt32 i = 0u; i < aaData[j].GetSize(); ++i) { aaData[j][i] = (UInt8)i; }
			}
			FixedArray<size_t, kuSamples> au;
			for (UInt32 j = 0u; j < kuSamples; ++j) { au[j] = aaData[j].GetSizeInBytes(); }

			FixedArray<UInt8, 1024u> aDict;
			SEOUL_UNITTESTING_ASSERT(ZSTDPopulateDict(aaData.Data(), kuSamples, au.Data(), aDict.Data(), aDict.GetSizeInBytes()));
			Entry entry;
			entry.m_FilePath = GenCompressionDictFilePath(GameDirectory::kContent, Platform::kPC);
			entry.m_pData = aDict.Data();
			entry.m_uData = aDict.GetSizeInBytes();
			auto const s(GenArchive(PackageFileHeader::ku16_LZ4CompressionVersion, GameDirectory::kContent, 1u, 1u, false, Platform::kPC, Files(1u, entry)));
			settings.m_vPopulatePackages.PushBack(s);
		}

		// Run and check.
		m_pSystem = FileManager::Get()->RegisterFileSystem<DownloadablePackageFileSystem>(settings);
		WaitForPackageInitialize();
	}

	auto const sTarget(m_sTargetPackageFilename);
	Destroy();
	m_sTargetPackageFilename = sTarget;
	Init();

	// Additional tests around compression dicts.
	{
		m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/DownloadablePackageFileSystem/Regress1_PC_Config.sar");

		// Configure downloader with default settings.
		DownloadablePackageFileSystemSettings settings;
		settings.m_sAbsolutePackageFilename = m_sTargetPackageFilename;
		settings.m_sInitialURL = "http://localhost:8057/Regress1_PC_Config.sar";

		// Compression dict size mismatch.
		{
			static const UInt32 kuSamples = 10u;

			FixedArray< FixedArray<UInt8, 128u>, kuSamples> aaData;
			for (UInt32 j = 0u; j < kuSamples; ++j)
			{
				for (UInt32 i = 0u; i < aaData[j].GetSize(); ++i) { aaData[j][i] = (UInt8)i; }
			}
			FixedArray<size_t, kuSamples> au;
			for (UInt32 j = 0u; j < kuSamples; ++j) { au[j] = aaData[j].GetSizeInBytes(); }

			FixedArray<UInt8, 1024u> aDict;
			SEOUL_UNITTESTING_ASSERT(ZSTDPopulateDict(aaData.Data(), kuSamples, au.Data(), aDict.Data(), aDict.GetSizeInBytes()));
			Entry entry;
			entry.m_FilePath = GenCompressionDictFilePath(GameDirectory::kConfig, Platform::kPC);
			entry.m_pData = aDict.Data();
			entry.m_uData = aDict.GetSizeInBytes();
			auto const s(GenArchive(PackageFileHeader::ku18_PreDualCrc32Version, GameDirectory::kConfig, 1u, 1u, true, Platform::kPC, Files(1u, entry)));
			settings.m_vPopulatePackages.PushBack(s);
		}

		// Run and check.
		m_pSystem = FileManager::Get()->RegisterFileSystem<DownloadablePackageFileSystem>(settings);
		WaitForPackageInitialize();
	}
}

void DownloadablePackageFileSystemTest::TestReadOnlyFileFailures()
{
	// Recompute the target filename, use the non-writable temp file.
	m_sTargetPackageFilename = GetNotWritableTempFileAbsoluteFilename();

	// Configure source and package.
	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/DownloadablePackageFileSystem/PC_Content.sar");

	// Configure downloader with default settings.
	DownloadablePackageFileSystemSettings settings;
	settings.m_sAbsolutePackageFilename = m_sTargetPackageFilename;
	settings.m_sInitialURL = "http://localhost:8057/PC_Content.sar";

	m_pSystem = FileManager::Get()->RegisterFileSystem<DownloadablePackageFileSystem>(settings);

	// Run tests - expect failure with a write failure.
	InternalInitializeFailureCommon(true);
}

/**
* Regression for a bug during development of .sar version 19,
* the downloader believes it has written a fully valid
* archive but a manual crc32 check fails.
*/
void DownloadablePackageFileSystemTest::TestRegressCrcIncorrect()
{
	// No requests should have yet been made.
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, m_pServer->GetReceivedRequestCount());

	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/DownloadablePackageFileSystem/Regress1_PC_Config.sar");
	SEOUL_UNITTESTING_ASSERT(CopyFile(m_sSourcePackageFilename, m_sTargetPackageFilename));

	DownloadablePackageFileSystemSettings settings;
	settings.m_sAbsolutePackageFilename = m_sTargetPackageFilename;
	settings.m_sInitialURL = "http://localhost:8057/Regress1_PC_ConfigUpdateA.sar";
	PatchablePackageFileSystem::AdjustSettings(settings);

	m_pSystem = FileManager::Get()->RegisterFileSystem<DownloadablePackageFileSystem>(settings);

	WaitForPackageInitialize();

	// Should now have 3 requests (one for the header, one for the file table as part
	// of initialization, and one fo the compression dictionary).
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, m_pServer->GetReceivedRequestCount());

	// Fetch all.
	SEOUL_UNITTESTING_ASSERT(m_pSystem->Fetch(DownloadablePackageFileSystem::Files()));

	WaitForPackageWorkCompletion();

	SEOUL_UNITTESTING_ASSERT_EQUAL(4, m_pServer->GetReceivedRequestCount());

	// Fully downloaded.
	SEOUL_UNITTESTING_ASSERT(IsCrc32Ok(*m_pSystem));

	// Check download state.
	AllFiles v;
	GetAll(*m_pSystem, v);
	Check(settings, *m_pSystem, v, [&](UInt32 uIndex) { return true; });

	// Check service.
	for (auto const& e : v)
	{
		SEOUL_UNITTESTING_ASSERT(!m_pSystem->IsServicedByNetwork(e.First));
	}

	// The downloaded archive should be exactly the same as the desired archive now.
	SEOUL_UNITTESTING_ASSERT(FilesAreEqual(
		Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/DownloadablePackageFileSystem/Regress1_PC_ConfigUpdateA.sar"),
		m_sTargetPackageFilename));
}

/**
* Regression for a bug during development of .sar version 19,
* full fetch became stuck in an infinite loop.
*/
void DownloadablePackageFileSystemTest::TestRegressInfiniteLoop()
{
	// No requests should have yet been made.
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, m_pServer->GetReceivedRequestCount());

	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/DownloadablePackageFileSystem/Regress1_PC_Config.sar");
	SEOUL_UNITTESTING_ASSERT(CopyFile(m_sSourcePackageFilename, m_sTargetPackageFilename));

	DownloadablePackageFileSystemSettings settings;
	settings.m_sAbsolutePackageFilename = m_sTargetPackageFilename;
	settings.m_sInitialURL = "http://localhost:8057/Regress1_PC_ConfigUpdateB.sar";
	PatchablePackageFileSystem::AdjustSettings(settings);

	m_pSystem = FileManager::Get()->RegisterFileSystem<DownloadablePackageFileSystem>(settings);

	WaitForPackageInitialize();

	// Should now have 3 requests (one for the header, one for the file table as part
	// of initialization, and one fo the compression dictionary).
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, m_pServer->GetReceivedRequestCount());

	// Fetch all.
	SEOUL_UNITTESTING_ASSERT(m_pSystem->Fetch(DownloadablePackageFileSystem::Files()));

	WaitForPackageWorkCompletion();

	SEOUL_UNITTESTING_ASSERT_EQUAL(4, m_pServer->GetReceivedRequestCount());

	// Fully downloaded.
	SEOUL_UNITTESTING_ASSERT(IsCrc32Ok(*m_pSystem));

	// Check download state.
	AllFiles v;
	GetAll(*m_pSystem, v);
	Check(settings, *m_pSystem, v, [&](UInt32 uIndex) { return true; });

	// Check service.
	for (auto const& e : v)
	{
		SEOUL_UNITTESTING_ASSERT(!m_pSystem->IsServicedByNetwork(e.First));
	}

	// The downloaded archive should be exactly the same as the desired archive now.
	SEOUL_UNITTESTING_ASSERT(FilesAreEqual(
		Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/DownloadablePackageFileSystem/Regress1_PC_ConfigUpdateB.sar"),
		m_sTargetPackageFilename));
}

void DownloadablePackageFileSystemTest::TestRequestCount()
{
	// No requests should have yet been made.
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, m_pServer->GetReceivedRequestCount());

	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/DownloadablePackageFileSystem/PC_Content.sar");

	// Configure downloader with default settings.
	DownloadablePackageFileSystemSettings settings;
	settings.m_sAbsolutePackageFilename = m_sTargetPackageFilename;
	settings.m_sInitialURL = "http://localhost:8057/PC_Content.sar";

	m_pSystem = FileManager::Get()->RegisterFileSystem<DownloadablePackageFileSystem>(settings);

	WaitForPackageInitialize();

	// Should now have 2 requests (one for the header, one for the file table as part
	// of initialization.
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, m_pServer->GetReceivedRequestCount());

	// Prefetch all files in the .sar - this should issue a single big request, given the
	// size of the files in the archive.
	{
		DownloadablePackageFileSystem::Files v;
		for (size_t i = 0u; i < SEOUL_ARRAY_COUNT(s_kaFiles); ++i)
		{
			v.PushBack(FilePath::CreateContentFilePath(s_kaFiles[i]));
		}
		SEOUL_UNITTESTING_ASSERT(m_pSystem->Prefetch(v));
	}

	WaitForPackageWorkCompletion();

	// Should now have 3 requests.
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, m_pServer->GetReceivedRequestCount());

	// All files should be fully present now, the archive should be ok.
	SEOUL_UNITTESTING_ASSERT(IsCrc32Ok(*m_pSystem));

	// The downloaded archive should be exactly the same as the source archive now.
	SEOUL_UNITTESTING_ASSERT(FilesAreEqual(m_sSourcePackageFilename, m_sTargetPackageFilename));
}

// Identical to TestRequestCount(), except with
// a different variation of Prefetch().
void DownloadablePackageFileSystemTest::TestRequestCount2()
{
	// No requests should have yet been made.
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, m_pServer->GetReceivedRequestCount());

	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/DownloadablePackageFileSystem/PC_Content.sar");

	// Configure downloader with default settings.
	DownloadablePackageFileSystemSettings settings;
	settings.m_sAbsolutePackageFilename = m_sTargetPackageFilename;
	settings.m_sInitialURL = "http://localhost:8057/PC_Content.sar";

	m_pSystem = FileManager::Get()->RegisterFileSystem<DownloadablePackageFileSystem>(settings);

	WaitForPackageInitialize();

	// Should now have 2 requests (one for the header, one for the file table as part
	// of initialization.
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, m_pServer->GetReceivedRequestCount());

	// Prefetch all files in the .sar - this should issue a single big request, given the
	// size of the files in the archive.
	SEOUL_UNITTESTING_ASSERT(m_pSystem->Prefetch(DownloadablePackageFileSystem::Files()));

	WaitForPackageWorkCompletion();

	// Should now have 3 requests.
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, m_pServer->GetReceivedRequestCount());

	// All files should be fully present now, the archive should be ok.
	SEOUL_UNITTESTING_ASSERT(IsCrc32Ok(*m_pSystem));

	// The downloaded archive should be exactly the same as the source archive now.
	SEOUL_UNITTESTING_ASSERT(FilesAreEqual(m_sSourcePackageFilename, m_sTargetPackageFilename));
}

void DownloadablePackageFileSystemTest::TestSettingsAdjusted()
{
	CommonTestSettingsAdjusted(String());
}

void DownloadablePackageFileSystemTest::TestSettingsDefault()
{
	CommonTestSettingsDefault(String());
}

void DownloadablePackageFileSystemTest::TestSettingsSparse()
{
	CommonTestSettingsSparse(String());
}

void DownloadablePackageFileSystemTest::TestUpdate()
{
	CommonTestUpdate(String());
}

void DownloadablePackageFileSystemTest::Destroy()
{
	m_pSystem.Reset();
	m_pServer.Reset();
	m_sTargetPackageFilename.Clear();
	m_pHelper.Reset();
}

void DownloadablePackageFileSystemTest::Init()
{
	m_pHelper.Reset(SEOUL_NEW(MemoryBudgets::Developer) UnitTestsEngineHelper);
	{
		HTTP::ServerSettings settings;
		settings.m_sRootDirectory = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/DownloadablePackageFileSystem");
		settings.m_iPort = 8057;
		settings.m_iThreadCount = 1;
		m_pServer.Reset(SEOUL_NEW(MemoryBudgets::Developer) HTTP::Server(settings));
	}
}

void DownloadablePackageFileSystemTest::InternalInitializeFailureCommon(Bool bExpectWriteFailure)
{
	// Sleep for a bit, we don't expect the system to connect.
	Thread::Sleep(1000);

	SEOUL_UNITTESTING_ASSERT(!m_pSystem->IsInitializationComplete());
	SEOUL_UNITTESTING_ASSERT(m_pSystem->IsInitializationStarted());
	SEOUL_UNITTESTING_ASSERT(!m_pSystem->IsInitialized());

	// All functions should fail when initialization has not occurred.
	SEOUL_UNITTESTING_ASSERT(!m_pSystem->Exists(FilePath::CreateContentFilePath("a")));
	SEOUL_UNITTESTING_ASSERT(!m_pSystem->Fetch(FilePath::CreateContentFilePath("a")));
	SEOUL_UNITTESTING_ASSERT(!m_pSystem->Fetch(DownloadablePackageFileSystem::Files(1, FilePath::CreateConfigFilePath("a"))));
	Vector<String> vsUnused;
	SEOUL_UNITTESTING_ASSERT(!m_pSystem->GetDirectoryListing(FilePath::CreateContentFilePath("a"), vsUnused));
	UInt64 uUnused;
	SEOUL_UNITTESTING_ASSERT(!m_pSystem->GetFileSize(FilePath::CreateContentFilePath("a"), uUnused));
	SEOUL_UNITTESTING_ASSERT(!m_pSystem->GetModifiedTime(FilePath::CreateContentFilePath("a"), uUnused));
	SEOUL_UNITTESTING_ASSERT(!m_pSystem->IsDirectory(FilePath::CreateContentFilePath("a")));

	// When waiting for a write failure, need to wait, since this is timing dependent.
	if (bExpectWriteFailure)
	{
		WaitForWriteFailure();
	}

	SEOUL_UNITTESTING_ASSERT_EQUAL(bExpectWriteFailure, m_pSystem->HasExperiencedWriteFailure());

	ScopedPtr<SyncFile> pUnusedFile;
	SEOUL_UNITTESTING_ASSERT(!m_pSystem->Open(FilePath::CreateContentFilePath("a"), File::kRead, pUnusedFile));

	SEOUL_UNITTESTING_ASSERT(!m_pSystem->Prefetch(FilePath::CreateContentFilePath("a")));
	SEOUL_UNITTESTING_ASSERT(!m_pSystem->Prefetch(DownloadablePackageFileSystem::Files(1, FilePath::CreateConfigFilePath("a"))));

	void* pUnused = nullptr;
	UInt32 uUnusedSize = 0u;
	SEOUL_UNITTESTING_ASSERT(!m_pSystem->ReadAll(FilePath::CreateContentFilePath("a"), pUnused, uUnusedSize, 0u, MemoryBudgets::Developer));

	SEOUL_UNITTESTING_ASSERT(!m_pSystem->SetModifiedTime(FilePath::CreateContentFilePath("a"), uUnused));

	PackageFileSystem::FileTable t;
	SEOUL_UNITTESTING_ASSERT(!m_pSystem->GetFileTable(t));

	SEOUL_UNITTESTING_ASSERT(!m_pSystem->PerformCrc32Check());
	{
		PackageCrc32Entries vUnused;
		SEOUL_UNITTESTING_ASSERT(!m_pSystem->PerformCrc32Check(&vUnused));
		PackageCrc32Entry entry;
		vUnused.PushBack(entry);
		SEOUL_UNITTESTING_ASSERT(!m_pSystem->PerformCrc32Check(&vUnused));
	}
	SEOUL_UNITTESTING_ASSERT(!m_pSystem->IsOk());

	SEOUL_UNITTESTING_ASSERT_EQUAL(0, m_pSystem->GetActiveSyncFileCount());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, m_pSystem->GetBuildChangelist());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, m_pSystem->GetBuildVersionMajor());
	SEOUL_UNITTESTING_ASSERT(!m_pSystem->HasPostCrc32());
}

void DownloadablePackageFileSystemTest::InternalTestCommon()
{
	WaitForPackageInitialize();

	DownloadablePackageFileSystem::FileTable tFileTable;
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
			"UnitTests/DownloadablePackageFileSystem",
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

void DownloadablePackageFileSystemTest::WaitForPackageInitialize()
{
	// Wait for initialization to complete.
	{
		auto const iStartTicks = SeoulTime::GetGameTimeInTicks();
		while (!m_pSystem->IsInitializationComplete())
		{
			SEOUL_UNITTESTING_ASSERT(
				SeoulTime::ConvertTicksToSeconds(SeoulTime::GetGameTimeInTicks() - iStartTicks) < 30.0f);

			// Simulate a 60 FPS frame so we're not starving devices with not many cores.
			auto const iBegin = SeoulTime::GetGameTimeInTicks();
			m_pHelper->Tick();
			auto const iEnd = SeoulTime::GetGameTimeInTicks();
			auto const uSleep = (UInt32)Floor(Clamp(SeoulTime::ConvertTicksToMilliseconds(iEnd - iBegin), 0.0, 17.0));
			Thread::Sleep(uSleep);
		}
	}
}

void DownloadablePackageFileSystemTest::WaitForPackageWorkCompletion()
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

void DownloadablePackageFileSystemTest::WaitForWriteFailure()
{
	// Wait for work to complete.
	{
		auto const iStartTicks = SeoulTime::GetGameTimeInTicks();
		while (!m_pSystem->HasExperiencedWriteFailure())
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

void DownloadablePackageFileSystemTest::WriteGarbageToTargetFile()
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

void DownloadablePackageFileSystemTest::CommonTestSettingsAdjusted(const String& sPrefix)
{
	// No requests should have yet been made.
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, m_pServer->GetReceivedRequestCount());

	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), String::Printf("UnitTests/DownloadablePackageFileSystem/%sPC_ConfigUpdate1.sar", sPrefix.CStr()));

	DownloadablePackageFileSystemSettings settings;
	settings.m_sAbsolutePackageFilename = m_sTargetPackageFilename;
	settings.m_sInitialURL = String::Printf("http://localhost:8057/%sPC_ConfigUpdate1.sar", sPrefix.CStr());
	settings.m_uUpperBoundMaxSizePerDownloadInBytes = 1024 * 1024;
	m_pSystem = FileManager::Get()->RegisterFileSystem<DownloadablePackageFileSystem>(settings);

	WaitForPackageInitialize();

	// Should now have 2 requests (one for the header, one for the file table as part
	// of initialization.
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, m_pServer->GetReceivedRequestCount());

	// Sanity failing CRC32 checks.
	{
		SEOUL_UNITTESTING_ASSERT(!m_pSystem->PerformCrc32Check());
		PackageCrc32Entries v;
		SEOUL_UNITTESTING_ASSERT(!m_pSystem->PerformCrc32Check(&v));
		SEOUL_UNITTESTING_ASSERT(!m_pSystem->PerformCrc32Check(&v));
	}

	// Prefetch all files in the .sar - this should pull all files
	// from the read-only fallback.
	SEOUL_UNITTESTING_ASSERT(m_pSystem->Fetch(DownloadablePackageFileSystem::Files()));

	WaitForPackageWorkCompletion();

	// Adjusted settings break the operation into 4 requests.
	// (total archive of 3,976,480 bytes minus the header+file table,
	// which are 48 + 71,961 = 72,009 bytes. In other words, we're
	// downloading 3,976,480 - 72,009 = 3,904,471 bytes in requests that
	// can download at most 1024 KB each.
	SEOUL_UNITTESTING_ASSERT_EQUAL(6, m_pServer->GetReceivedRequestCount());

	// All files should be fully present now, the archive should be ok.
	SEOUL_UNITTESTING_ASSERT(IsCrc32Ok(*m_pSystem));

	// The downloaded archive should be exactly the same as the source archive now.
	SEOUL_UNITTESTING_ASSERT(FilesAreEqual(m_sSourcePackageFilename, m_sTargetPackageFilename));
}

void DownloadablePackageFileSystemTest::CommonTestSettingsDefault(const String& sPrefix)
{
	// No requests should have yet been made.
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, m_pServer->GetReceivedRequestCount());

	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), String::Printf("UnitTests/DownloadablePackageFileSystem/%sPC_ConfigUpdate1.sar", sPrefix.CStr()));

	DownloadablePackageFileSystemSettings settings;
	settings.m_sAbsolutePackageFilename = m_sTargetPackageFilename;
	settings.m_sInitialURL = String::Printf("http://localhost:8057/%sPC_ConfigUpdate1.sar", sPrefix.CStr());
	m_pSystem = FileManager::Get()->RegisterFileSystem<DownloadablePackageFileSystem>(settings);

	WaitForPackageInitialize();

	// Should now have 2 requests (one for the header, one for the file table as part
	// of initialization.
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, m_pServer->GetReceivedRequestCount());

	// Prefetch all files in the .sar - this should pull all files
	// from the read-only fallback.
	SEOUL_UNITTESTING_ASSERT(m_pSystem->Fetch(DownloadablePackageFileSystem::Files()));

	WaitForPackageWorkCompletion();

	// Default settings will break the operation into 17 requests
	// (total archive of 3,976,480 bytes minus the header+file table,
	// which are 48 + 71,961 = 72,009 bytes. In other words, we're
	// downloading 3,976,480 - 72,009 = 3,904,471 bytes in requests that
	// can download at most 256 KB each.
	//
	// This generates 15 requests -
	// 2 more requests are introduced due to large files that cause
	// 2 of the requests to be split.
	SEOUL_UNITTESTING_ASSERT_EQUAL(19, m_pServer->GetReceivedRequestCount());

	// All files should be fully present now, the archive should be ok.
	SEOUL_UNITTESTING_ASSERT(IsCrc32Ok(*m_pSystem));

	// The downloaded archive should be exactly the same as the source archive now.
	SEOUL_UNITTESTING_ASSERT(FilesAreEqual(m_sSourcePackageFilename, m_sTargetPackageFilename));
}

void DownloadablePackageFileSystemTest::CommonTestSettingsSparse(const String& sPrefix)
{
	// No requests should have yet been made.
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, m_pServer->GetReceivedRequestCount());

	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), String::Printf("UnitTests/DownloadablePackageFileSystem/%sPC_ConfigUpdate1.sar", sPrefix.CStr()));

	DownloadablePackageFileSystemSettings settings;
	settings.m_sAbsolutePackageFilename = m_sTargetPackageFilename;
	settings.m_sInitialURL = String::Printf("http://localhost:8057/%sPC_ConfigUpdate1.sar", sPrefix.CStr());

	// Redownload threshold.
	settings.m_uMaxRedownloadSizeThresholdInBytes = 16384u;

	m_pSystem = FileManager::Get()->RegisterFileSystem<DownloadablePackageFileSystem>(settings);

	WaitForPackageInitialize();

	// Should now have 2 requests (one for the header, one for the file table as part
	// of initialization.
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, m_pServer->GetReceivedRequestCount());

	// Prefetch every other file in the .sar.
	AllFiles v;
	GetAll(*m_pSystem, v);

	Vector<FilePath> vFiles;
	for (auto i = 0u; i < v.GetSize(); i += 2u)
	{
		vFiles.PushBack(v[i + 0u].First);
	}

	SEOUL_UNITTESTING_ASSERT(m_pSystem->Fetch(vFiles));

	WaitForPackageWorkCompletion();

	SEOUL_UNITTESTING_ASSERT_EQUAL(18, m_pServer->GetReceivedRequestCount());

	// The entire archive has not yet been downloaded
	// so it will not yet pass a crc32 check. Overflow is
	// to small to include all files not explicitly downloaded.
	SEOUL_UNITTESTING_ASSERT(!IsCrc32Ok(*m_pSystem));

	// Capture for later comparison.
	PackageCrc32Entries vCrc;
	m_pSystem->PerformCrc32Check(&vCrc);

	// Check download state.
	Check(settings, *m_pSystem, v, [&](UInt32 uIndex) { return (uIndex % 2 == 0u); });

	// Check download state against crc also.
	for (auto const& e : vCrc)
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(e.m_bCrc32Ok, !m_pSystem->IsServicedByNetwork(e.m_FilePath));
	}

	// Now gather entries that didn't pass.
	PackageCrc32Entries vRemaining;
	for (auto const& e : vCrc) { if (!e.m_bCrc32Ok) { vRemaining.PushBack(e); } }

	// Check.
	SEOUL_UNITTESTING_ASSERT_EQUAL(15u, vRemaining.GetSize());

	// Shuffle the list (using a fix generator so behavior is predictable).
	PseudoRandom gen(PseudoRandomSeed(10357030100123, 258005120358091235));
	RandomShuffle(vRemaining.Begin(), vRemaining.End(), [&](size_t u) { return (size_t)gen.UniformRandomUInt64n((UInt64)u); });

	// Now fetch each and recheck.
	UInt32 uExpectedRequests = 18u;
	for (auto const& e : vRemaining)
	{
		SEOUL_UNITTESTING_ASSERT(m_pSystem->IsServicedByNetwork(e.m_FilePath));
		SEOUL_UNITTESTING_ASSERT(m_pSystem->Fetch(e.m_FilePath));
		WaitForPackageWorkCompletion();
		SEOUL_UNITTESTING_ASSERT(!m_pSystem->IsServicedByNetwork(e.m_FilePath));

		++uExpectedRequests;
		SEOUL_UNITTESTING_ASSERT_EQUAL(uExpectedRequests, m_pServer->GetReceivedRequestCount());
	}

	// Now expected to be entirely valid and complete.
	SEOUL_UNITTESTING_ASSERT_EQUAL(33u, m_pServer->GetReceivedRequestCount());

	// All files should be fully present now, the archive should be ok.
	SEOUL_UNITTESTING_ASSERT(IsCrc32Ok(*m_pSystem));

	// The downloaded archive should be exactly the same as the source archive now.
	SEOUL_UNITTESTING_ASSERT(FilesAreEqual(m_sSourcePackageFilename, m_sTargetPackageFilename));
}

void DownloadablePackageFileSystemTest::CommonTestUpdate(const String& sPrefix)
{
	// No requests should have yet been made.
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, m_pServer->GetReceivedRequestCount());

	// Common.
	AllFiles v;

	PackageCrc32Entries vBeforeCrc;
	PackageCrc32Entries vAfterCrc;

	// First
	{
		m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), String::Printf("UnitTests/DownloadablePackageFileSystem/%sPC_ConfigUpdate1.sar", sPrefix.CStr()));

		DownloadablePackageFileSystemSettings settings;
		settings.m_sAbsolutePackageFilename = m_sTargetPackageFilename;
		settings.m_sInitialURL = String::Printf("http://localhost:8057/%sPC_ConfigUpdate1.sar", sPrefix.CStr());

		// Redownload threshold.
		settings.m_uMaxRedownloadSizeThresholdInBytes = 16384u;

		m_pSystem = FileManager::Get()->RegisterFileSystem<DownloadablePackageFileSystem>(settings);

		WaitForPackageInitialize();

		// Should now have 2 requests (one for the header, one for the file table as part
		// of initialization.
		SEOUL_UNITTESTING_ASSERT_EQUAL(2, m_pServer->GetReceivedRequestCount());

		// Prefetch every other file in the .sar.
		GetAll(*m_pSystem, v);

		Vector<FilePath> vFiles;
		for (auto i = 0u; i < v.GetSize(); i += 2u)
		{
			vFiles.PushBack(v[i + 0u].First);
		}

		SEOUL_UNITTESTING_ASSERT(m_pSystem->Fetch(vFiles));

		WaitForPackageWorkCompletion();

		SEOUL_UNITTESTING_ASSERT_EQUAL(18, m_pServer->GetReceivedRequestCount());

		// The entire archive has not yet been downloaded
		// so it will not yet pass a crc32 check. Overflow is
		// to small to include all files not explicitly downloaded.
		SEOUL_UNITTESTING_ASSERT(!IsCrc32Ok(*m_pSystem));

		// Capture for later comparison.
		m_pSystem->PerformCrc32Check(&vBeforeCrc);

		// Check download state.
		Check(settings, *m_pSystem, v, [&](UInt32 uIndex) { return (uIndex % 2 == 0u); });

		// Check download state against crc also.
		for (auto const& e : vBeforeCrc)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(e.m_bCrc32Ok, !m_pSystem->IsServicedByNetwork(e.m_FilePath));
		}
	}

	// Destroy and recreate - to simulate an update.
	{
		auto const s(m_sTargetPackageFilename);
		Destroy();
		m_sTargetPackageFilename = s;
		Init();
	}

	// Second - set a new target .sar
	{
		m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), String::Printf("UnitTests/DownloadablePackageFileSystem/%sPC_ConfigUpdate2.sar", sPrefix.CStr()));

		DownloadablePackageFileSystemSettings settings;
		settings.m_sAbsolutePackageFilename = m_sTargetPackageFilename;
		settings.m_sInitialURL = String::Printf("http://localhost:8057/%sPC_ConfigUpdate2.sar", sPrefix.CStr());

		// Redownload threshold.
		settings.m_uMaxRedownloadSizeThresholdInBytes = 16384u;

		m_pSystem = FileManager::Get()->RegisterFileSystem<DownloadablePackageFileSystem>(settings);

		WaitForPackageInitialize();

		// Now wait for startup ops, no explicit actions, but should end up with the
		// same results as the first pass.
		WaitForPackageWorkCompletion();

		// In total, should end up in the same state with only 2 requests,
		// to download header and file table.
		SEOUL_UNITTESTING_ASSERT_EQUAL(2, m_pServer->GetReceivedRequestCount());

		// Because of overflow, all files not explicitly
		// requested will also have been cached locally.
		SEOUL_UNITTESTING_ASSERT(!IsCrc32Ok(*m_pSystem));

		// Validate size - test .sar files were built specially to have
		// the exact same contents but in reverse of each other.
		AllFiles vNew;
		GetAll(*m_pSystem, vNew);

		SEOUL_UNITTESTING_ASSERT_EQUAL(v.GetSize(), vNew.GetSize());
		for (UInt32 i = 0u; i < v.GetSize(); ++i)
		{
			auto const& a = v[i];
			auto const& b = vNew[vNew.GetSize() - 1u - i];
			SEOUL_UNITTESTING_ASSERT_EQUAL(a.First, b.First);
			SEOUL_UNITTESTING_ASSERT_EQUAL(a.Second.m_uXorKey, b.Second.m_uXorKey);
			SEOUL_UNITTESTING_ASSERT_EQUAL(a.Second.m_Entry.m_uCompressedFileSize, b.Second.m_Entry.m_uCompressedFileSize);
			SEOUL_UNITTESTING_ASSERT_EQUAL(a.Second.m_Entry.m_uCrc32Post, b.Second.m_Entry.m_uCrc32Post);
			SEOUL_UNITTESTING_ASSERT_EQUAL(a.Second.m_Entry.m_uCrc32Pre, b.Second.m_Entry.m_uCrc32Pre);
			SEOUL_UNITTESTING_ASSERT_EQUAL(a.Second.m_Entry.m_uModifiedTime, b.Second.m_Entry.m_uModifiedTime);
			SEOUL_UNITTESTING_ASSERT_EQUAL(a.Second.m_Entry.m_uUncompressedFileSize, b.Second.m_Entry.m_uUncompressedFileSize);
		}

		// Capture for later comparison.
		m_pSystem->PerformCrc32Check(&vAfterCrc);

		// Check download state against crc also.
		for (auto const& e : vAfterCrc)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(e.m_bCrc32Ok, !m_pSystem->IsServicedByNetwork(e.m_FilePath));
		}

		// Check download state.
		Check(settings, *m_pSystem, v, [&](UInt32 uIndex) { return (uIndex % 2 == 0u); });
	}

	// Compare.
	SEOUL_UNITTESTING_ASSERT_EQUAL(vBeforeCrc.GetSize(), vAfterCrc.GetSize());
	for (UInt32 i = 0u; i < vBeforeCrc.GetSize(); ++i)
	{
		auto const& a = vBeforeCrc[i];
		auto const& b = vAfterCrc[vAfterCrc.GetSize() - 1u - i];

		SEOUL_UNITTESTING_ASSERT_EQUAL(a.m_bCrc32Ok, b.m_bCrc32Ok);
		SEOUL_UNITTESTING_ASSERT_EQUAL(a.m_Entry.m_uCompressedFileSize, b.m_Entry.m_uCompressedFileSize);
		SEOUL_UNITTESTING_ASSERT_EQUAL(a.m_Entry.m_uCrc32Post, b.m_Entry.m_uCrc32Post);
		SEOUL_UNITTESTING_ASSERT_EQUAL(a.m_Entry.m_uCrc32Pre, b.m_Entry.m_uCrc32Pre);
		SEOUL_UNITTESTING_ASSERT_EQUAL(a.m_Entry.m_uModifiedTime, b.m_Entry.m_uModifiedTime);
		SEOUL_UNITTESTING_ASSERT_EQUAL(a.m_Entry.m_uUncompressedFileSize, b.m_Entry.m_uUncompressedFileSize);
		SEOUL_UNITTESTING_ASSERT_EQUAL(a.m_FilePath, b.m_FilePath);
	}
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
