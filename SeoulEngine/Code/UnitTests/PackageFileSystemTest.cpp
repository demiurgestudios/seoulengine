/**
 * \file PackageFileSystemTest.cpp
 * \brief Test for the PackageFileSystem, the basic
 * type of all .sar based file systems.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "DiskFileSystem.h"
#include "FileManager.h"
#include "GamePaths.h"
#include "PackageFileSystem.h"
#include "PackageFileSystemTest.h"
#include "PseudoRandom.h"
#include "ReflectionDefine.h"
#include "SeoulCrc32.h"
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

SEOUL_BEGIN_TYPE(PackageFileSystemTest, TypeFlags::kDisableCopy)
	SEOUL_ATTRIBUTE(UnitTest, Attributes::UnitTest::kInstantiateForEach) // Want Engine and other resources to be recreated for each test.

	SEOUL_METHOD(TestBadHeader)
	SEOUL_METHOD(TestBasic)
	SEOUL_METHOD(TestBasicCompressed)
	SEOUL_METHOD(TestBasicInMemory)
	SEOUL_METHOD(TestBasicCompressedInMemory)
	SEOUL_METHOD(TestCommitChangeToSarFileFail)
	SEOUL_METHOD(TestCommitChangeToSarFileSucceed)
	SEOUL_METHOD(V19_TestCompressionDictApi)
	SEOUL_METHOD(V20_TestCompressionDictApi)
	SEOUL_METHOD(V21_TestCompressionDictApi)
	SEOUL_METHOD(V19_TestCompressionDictApiDeferred)
	SEOUL_METHOD(V20_TestCompressionDictApiDeferred)
	SEOUL_METHOD(V21_TestCompressionDictApiDeferred)
	SEOUL_METHOD(V19_TestCompressionFile)
	SEOUL_METHOD(V20_TestCompressionFile)
	SEOUL_METHOD(V21_TestCompressionFile)
	SEOUL_METHOD(TestEdgeCases)
	SEOUL_METHOD(TestGarbageFile)
	SEOUL_METHOD(TestGetDirectoryListing)
	SEOUL_METHOD(TestHeader)
	SEOUL_METHOD(TestLargeFile)
	SEOUL_METHOD(V19_TestMiscApi)
	SEOUL_METHOD(V20_TestMiscApi)
	SEOUL_METHOD(V21_TestMiscApi)
	SEOUL_METHOD(TestPerformCrc32EdgeCases)
	SEOUL_METHOD(V19_TestReadRaw)
	SEOUL_METHOD(V20_TestReadRaw)
	SEOUL_METHOD(V21_TestReadRaw)
	SEOUL_METHOD(TestSeekFail)
	SEOUL_METHOD(TestCorruptedFileTable)
	SEOUL_METHOD(TestCorruptedFileTableV20)
	SEOUL_METHOD(TestCorruptedFileTableV21)
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
	const Vector<Entry, MemoryBudgets::Developer>& vFiles = Vector<Entry, MemoryBudgets::Developer>(),
	PackageFileHeader* pHeader = nullptr,
	Bool bExpectInvalid = false)
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

	if (!bExpectInvalid)
	{
		// Sanity check.
		PackageFileSystem pkg(sTempFile);
		SEOUL_UNITTESTING_ASSERT(pkg.IsOk());
		SEOUL_UNITTESTING_ASSERT(IsCrc32Ok(pkg));
	}

	if (nullptr != pHeader) { *pHeader = header; }
	return sTempFile;
}

} // namespace anonymous

PackageFileSystemTest::PackageFileSystemTest()
	: m_pHelper()
	, m_sSourcePackageFilename()
	, m_pSystem()
{
	Init();
}

PackageFileSystemTest::~PackageFileSystemTest()
{
	Destroy();
}

void PackageFileSystemTest::TestBadHeader()
{
	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/PackageFileSystem/PC_BadHeader.sar");
	m_pSystem = FileManager::Get()->RegisterFileSystem<PackageFileSystem>(m_sSourcePackageFilename);
	InternalInitializeFailureCommon();
}

void PackageFileSystemTest::TestBasic()
{
	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/PackageFileSystem/PC_Content.sar");
	m_pSystem = FileManager::Get()->RegisterFileSystem<PackageFileSystem>(m_sSourcePackageFilename);
	InternalTestCommon();

	// Does not support directory listing.
	FilePath dirPath;
	dirPath.SetDirectory(m_pSystem->GetPackageGameDirectory());
	Vector<String> vs;
	SEOUL_UNITTESTING_ASSERT(!m_pSystem->GetDirectoryListing(dirPath, vs, false, true, String()));
}

void PackageFileSystemTest::TestBasicCompressed()
{
	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/PackageFileSystem/PC_Config.sar");
	m_pSystem = FileManager::Get()->RegisterFileSystem<PackageFileSystem>(m_sSourcePackageFilename);

	SEOUL_UNITTESTING_ASSERT(m_pSystem->IsOk());
	SEOUL_UNITTESTING_ASSERT(m_pSystem->PerformCrc32Check());
	{
		PackageCrc32Entries v;
		SEOUL_UNITTESTING_ASSERT(m_pSystem->PerformCrc32Check(&v));
		SEOUL_UNITTESTING_ASSERT_EQUAL(26, v.GetSize());
		for (UInt32 i = 0u; i < v.GetSize(); ++i)
		{
			auto const& e = v[i];
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, e.m_bCrc32Ok);
		}
		v.Clear();
		PackageCrc32Entry entry;
		entry.m_FilePath = FilePath::CreateContentFilePath(s_kaFiles[0]);
		v.PushBack(entry);
		entry.m_FilePath = FilePath::CreateContentFilePath("a.png");
		v.PushBack(entry);
		SEOUL_UNITTESTING_ASSERT(m_pSystem->PerformCrc32Check(&v));
	}

	IPackageFileSystem::FileTable t;
	SEOUL_UNITTESTING_ASSERT(m_pSystem->GetFileTable(t));
	SEOUL_UNITTESTING_ASSERT_EQUAL(26, t.GetSize());
}

void PackageFileSystemTest::TestBasicInMemory()
{
	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/PackageFileSystem/PC_Content.sar");
	m_pSystem = FileManager::Get()->RegisterFileSystem<PackageFileSystem>(m_sSourcePackageFilename, true);
	InternalTestCommon();

	// Does not support directory listing.
	FilePath dirPath;
	dirPath.SetDirectory(m_pSystem->GetPackageGameDirectory());
	Vector<String> vs;
	SEOUL_UNITTESTING_ASSERT(!m_pSystem->GetDirectoryListing(dirPath, vs, false, true, String()));
}

void PackageFileSystemTest::TestBasicCompressedInMemory()
{
	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/PackageFileSystem/PC_Config.sar");
	m_pSystem = FileManager::Get()->RegisterFileSystem<PackageFileSystem>(m_sSourcePackageFilename, true);

	SEOUL_UNITTESTING_ASSERT(m_pSystem->IsOk());
	SEOUL_UNITTESTING_ASSERT(m_pSystem->PerformCrc32Check());
	{
		PackageCrc32Entries v;
		SEOUL_UNITTESTING_ASSERT(m_pSystem->PerformCrc32Check(&v));
		SEOUL_UNITTESTING_ASSERT_EQUAL(26, v.GetSize());
		for (UInt32 i = 0u; i < v.GetSize(); ++i)
		{
			auto const& e = v[i];
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, e.m_bCrc32Ok);
		}
		v.Clear();
		PackageCrc32Entry entry;
		entry.m_FilePath = FilePath::CreateContentFilePath(s_kaFiles[0]);
		v.PushBack(entry);
		entry.m_FilePath = FilePath::CreateContentFilePath("a.png");
		v.PushBack(entry);
		SEOUL_UNITTESTING_ASSERT(m_pSystem->PerformCrc32Check(&v));
	}

	IPackageFileSystem::FileTable t;
	SEOUL_UNITTESTING_ASSERT(m_pSystem->GetFileTable(t));
	SEOUL_UNITTESTING_ASSERT_EQUAL(26, t.GetSize());
}

void PackageFileSystemTest::TestCommitChangeToSarFileFail()
{
	// Readonly, cannot write.
	m_sSourcePackageFilename = Path::GetTempFileAbsoluteFilename();
	SEOUL_UNITTESTING_ASSERT(CopyFile(Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/PackageFileSystem/PC_Content.sar"), m_sSourcePackageFilename));
	m_pSystem = FileManager::Get()->RegisterFileSystem<PackageFileSystem>(m_sSourcePackageFilename);
	InternalTestCommon();

	Vector<Byte> vZero;
	vZero.Resize((UInt32)m_pSystem->GetHeader().GetTotalPackageFileSizeInBytes(), 0);
	SEOUL_UNITTESTING_ASSERT(!m_pSystem->CommitChangeToSarFile(vZero.Data(), vZero.GetSizeInBytes(), 0));
	InternalTestCommon();
}

void PackageFileSystemTest::TestCommitChangeToSarFileSucceed()
{
	auto const sPath(Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/PackageFileSystem/PC_Content.sar"));

	// Write, write.
	m_sSourcePackageFilename = Path::GetTempFileAbsoluteFilename();
	m_pSystem = FileManager::Get()->RegisterFileSystem<PackageFileSystem>(m_sSourcePackageFilename);
	InternalInitializeFailureCommon();

	// Try to fill out the file with all 0 bytes.
	{
		auto const s(m_sSourcePackageFilename);
		Destroy();
		Init();
		Vector<Byte> vZero;
		vZero.Resize((UInt32)FileManager::Get()->GetFileSize(sPath));
		SEOUL_UNITTESTING_ASSERT(FileManager::Get()->WriteAll(s, vZero.Data(), vZero.GetSizeInBytes()));
		m_sSourcePackageFilename = s;
		m_pSystem = FileManager::Get()->RegisterFileSystem<PackageFileSystem>(m_sSourcePackageFilename, false, true);
	}
	InternalInitializeFailureCommon();

	// Now write out the body using commit.
	{
		Vector<Byte> vBody;
		SEOUL_UNITTESTING_ASSERT(FileManager::Get()->ReadAll(sPath, vBody));
		SEOUL_UNITTESTING_ASSERT(m_pSystem->CommitChangeToSarFile(vBody.Data(), vBody.GetSizeInBytes(), 0));

		auto const s(m_sSourcePackageFilename);
		Destroy();
		Init();
		m_sSourcePackageFilename = s;
		m_pSystem = FileManager::Get()->RegisterFileSystem<PackageFileSystem>(m_sSourcePackageFilename);
	}
	InternalTestCommon();
}

void PackageFileSystemTest::TestCompressionDictApi(Byte const* sPrefix)
{
	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), String::Printf("UnitTests/PackageFileSystem/%sMeasure_PC_ConfigUpdate1.sar", sPrefix));
	m_pSystem = FileManager::Get()->RegisterFileSystem<PackageFileSystem>(m_sSourcePackageFilename);

	SEOUL_UNITTESTING_ASSERT(m_pSystem->IsOk());

	SEOUL_UNITTESTING_ASSERT_EQUAL(GenCompressionDictFilePath(GameDirectory::kConfig, Platform::kPC), m_pSystem->GetCompressionDictFilePath());
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(nullptr, m_pSystem->GetDecompressionDict());
	SEOUL_UNITTESTING_ASSERT(m_pSystem->IsCompressionDictProcessed());
}

void PackageFileSystemTest::TestCompressionDictApiDeferred(Byte const* sPrefix)
{
	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), String::Printf("UnitTests/PackageFileSystem/%sMeasure_PC_ConfigUpdate1.sar", sPrefix));
	m_pSystem = FileManager::Get()->RegisterFileSystem<PackageFileSystem>(m_sSourcePackageFilename, false, false, true);

	SEOUL_UNITTESTING_ASSERT(m_pSystem->IsOk());

	SEOUL_UNITTESTING_ASSERT_EQUAL(GenCompressionDictFilePath(GameDirectory::kConfig, Platform::kPC), m_pSystem->GetCompressionDictFilePath());
	SEOUL_UNITTESTING_ASSERT_EQUAL(nullptr, m_pSystem->GetDecompressionDict());
	SEOUL_UNITTESTING_ASSERT(!m_pSystem->IsCompressionDictProcessed());

	SEOUL_UNITTESTING_ASSERT(m_pSystem->ProcessCompressionDict());

	SEOUL_UNITTESTING_ASSERT_EQUAL(GenCompressionDictFilePath(GameDirectory::kConfig, Platform::kPC), m_pSystem->GetCompressionDictFilePath());
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(nullptr, m_pSystem->GetDecompressionDict());
	SEOUL_UNITTESTING_ASSERT(m_pSystem->IsCompressionDictProcessed());

	SEOUL_UNITTESTING_ASSERT(m_pSystem->ProcessCompressionDict());

	SEOUL_UNITTESTING_ASSERT_EQUAL(GenCompressionDictFilePath(GameDirectory::kConfig, Platform::kPC), m_pSystem->GetCompressionDictFilePath());
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(nullptr, m_pSystem->GetDecompressionDict());
	SEOUL_UNITTESTING_ASSERT(m_pSystem->IsCompressionDictProcessed());
}

void PackageFileSystemTest::TestCompressionFile(Byte const* sPrefix)
{
	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), String::Printf("UnitTests/PackageFileSystem/%sMeasure_PC_ConfigUpdate1.sar", sPrefix));
	m_pSystem = FileManager::Get()->RegisterFileSystem<PackageFileSystem>(m_sSourcePackageFilename);

	SEOUL_UNITTESTING_ASSERT(m_pSystem->IsOk());

	{
		auto const filePath(FilePath::CreateConfigFilePath("application.json"));
		ScopedPtr<SyncFile> pFile;
		SEOUL_UNITTESTING_ASSERT(m_pSystem->Open(filePath, File::kRead, pFile));

		// Simple API.
		SEOUL_UNITTESTING_ASSERT(pFile->CanRead());
		SEOUL_UNITTESTING_ASSERT(pFile->CanSeek());
		SEOUL_UNITTESTING_ASSERT(!pFile->CanWrite());
		Byte a = 0;
		SEOUL_UNITTESTING_ASSERT(!pFile->WriteRawData(&a, sizeof(a)));
		SEOUL_UNITTESTING_ASSERT(pFile->IsOpen());
		SEOUL_UNITTESTING_ASSERT(!pFile->Flush());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1229, pFile->GetSize());
		Int64 iPosition = 0;
		SEOUL_UNITTESTING_ASSERT(pFile->GetCurrentPositionIndicator(iPosition));
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, iPosition);
		SEOUL_UNITTESTING_ASSERT_EQUAL(filePath.GetAbsoluteFilename(), pFile->GetAbsoluteFilename());

		// A few edge cases.
		SEOUL_UNITTESTING_ASSERT(!pFile->Seek(Int64Min, File::kSeekFromStart));
		SEOUL_UNITTESTING_ASSERT(!pFile->Seek(Int64Max, File::kSeekFromStart));

		// Read test.
		void* pData = nullptr;
		UInt32 uData = 0u;
		SEOUL_UNITTESTING_ASSERT(pFile->ReadAll(pData, uData, 0u, MemoryBudgets::TBD));

		// Should return 0 (at EOF).
		SEOUL_UNITTESTING_ASSERT_EQUAL(0u, pFile->ReadRawData(pData, 128u));

		// Test, reset.
		SEOUL_UNITTESTING_ASSERT(pFile->GetCurrentPositionIndicator(iPosition));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1229, iPosition);
		SEOUL_UNITTESTING_ASSERT(pFile->Seek(1229, File::kSeekFromEnd));
		SEOUL_UNITTESTING_ASSERT(pFile->GetCurrentPositionIndicator(iPosition));
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, iPosition);
		SEOUL_UNITTESTING_ASSERT(pFile->Seek(10, File::kSeekFromCurrent));
		SEOUL_UNITTESTING_ASSERT(pFile->GetCurrentPositionIndicator(iPosition));
		SEOUL_UNITTESTING_ASSERT_EQUAL(10, iPosition);
		SEOUL_UNITTESTING_ASSERT(pFile->Seek(0, File::kSeekFromStart));
		SEOUL_UNITTESTING_ASSERT(pFile->GetCurrentPositionIndicator(iPosition));
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, iPosition);

		// Now read the data manually and compare.
		void* pData2 = MemoryManager::Allocate(uData, MemoryBudgets::TBD);
		SEOUL_UNITTESTING_ASSERT_EQUAL(uData, pFile->ReadRawData(pData2, uData));
		SEOUL_UNITTESTING_ASSERT(pFile->GetCurrentPositionIndicator(iPosition));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1229, iPosition);

		// Compare.
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(pData, pData2, uData));

		MemoryManager::Deallocate(pData2);
		MemoryManager::Deallocate(pData);
	}

	// Again, read before read-all to check buffering.
	{
		auto const filePath(FilePath::CreateConfigFilePath("application.json"));
		ScopedPtr<SyncFile> pFile;
		SEOUL_UNITTESTING_ASSERT(m_pSystem->Open(filePath, File::kRead, pFile));

		void* pBuf = MemoryManager::Allocate((UInt32)pFile->GetSize(), MemoryBudgets::Io);
		SEOUL_UNITTESTING_ASSERT_EQUAL(25u, pFile->ReadRawData(pBuf, 25u));
		void* pBuf2 = nullptr;
		UInt32 uBuf2 = 0u;
		SEOUL_UNITTESTING_ASSERT(pFile->ReadAll(pBuf2, uBuf2, 0u, MemoryBudgets::Io));
		SEOUL_UNITTESTING_ASSERT(pFile->Seek(0u, File::kSeekFromStart));
		SEOUL_UNITTESTING_ASSERT_EQUAL((UInt32)pFile->GetSize(), pFile->ReadRawData(pBuf, (UInt32)pFile->GetSize()));
		SEOUL_UNITTESTING_ASSERT_EQUAL(uBuf2, (UInt32)pFile->GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(pBuf, pBuf2, (UInt32)pFile->GetSize()));

		MemoryManager::Deallocate(pBuf2);
		MemoryManager::Deallocate(pBuf);
	}
}

void PackageFileSystemTest::TestEdgeCases()
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

	// Invalid compression dict (0 size)
	{
		// Generate.
		Entry entry;
		entry.m_FilePath = GenCompressionDictFilePath(GameDirectory::kConfig, keCurrentPlatform);
		Files vFiles;
		vFiles.PushBack(entry);
		auto const s(GenArchive(kuPackageVersion, GameDirectory::kConfig, 1u, 1u, true, keCurrentPlatform, vFiles, nullptr, true));

		// Check.
		{
			PackageFileSystem pkg(s);
			SEOUL_UNITTESTING_ASSERT(!pkg.IsOk());
		}
		{
			PackageFileSystem pkg(s, false, false, true);
			SEOUL_UNITTESTING_ASSERT(pkg.IsOk());
			SEOUL_UNITTESTING_ASSERT(!pkg.ProcessCompressionDict());
		}
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
		// PackageFile
		{
			auto const s(Path::GetTempFileAbsoluteFilename());
			PackageFileHeader header{};
			{
				SEOUL_UNITTESTING_ASSERT(FileManager::Get()->WriteAll(s, &header, sizeof(header)));
				PackageFileSystem pkg(s);
				SEOUL_UNITTESTING_ASSERT(!pkg.IsOk());
			}

			header.m_uSignature = kuPackageSignature;
			{
				SEOUL_UNITTESTING_ASSERT(FileManager::Get()->WriteAll(s, &header, sizeof(header)));
				PackageFileSystem pkg(s);
				SEOUL_UNITTESTING_ASSERT(!pkg.IsOk());
			}
			header.m_uVersion = PackageFileHeader::ku16_LZ4CompressionVersion;
			{
				SEOUL_UNITTESTING_ASSERT(FileManager::Get()->WriteAll(s, &header, sizeof(header)));
				PackageFileSystem pkg(s);
				SEOUL_UNITTESTING_ASSERT(!pkg.IsOk());
			}
			header.m_uVersion = PackageFileHeader::ku17_PreCompressionDictVersion;
			{
				SEOUL_UNITTESTING_ASSERT(FileManager::Get()->WriteAll(s, &header, sizeof(header)));
				PackageFileSystem pkg(s);
				SEOUL_UNITTESTING_ASSERT(!pkg.IsOk());
			}
			header.m_uVersion = PackageFileHeader::ku18_PreDualCrc32Version;
			{
				SEOUL_UNITTESTING_ASSERT(FileManager::Get()->WriteAll(s, &header, sizeof(header)));
				PackageFileSystem pkg(s);
				SEOUL_UNITTESTING_ASSERT(!pkg.IsOk());
			}
			header.m_uVersion = kuPackageVersion;
			{
				SEOUL_UNITTESTING_ASSERT(FileManager::Get()->WriteAll(s, &header, sizeof(header)));
				PackageFileSystem pkg(s);
				SEOUL_UNITTESTING_ASSERT(!pkg.IsOk());
			}

			header.SetGameDirectory(Convert(GameDirectory::kConfig));
			{
				SEOUL_UNITTESTING_ASSERT(FileManager::Get()->WriteAll(s, &header, sizeof(header)));
				PackageFileSystem pkg(s);
				SEOUL_UNITTESTING_ASSERT(!pkg.IsOk());
			}
			header.SetPlatformAndObfuscation(Platform::kLinux, true);
			{
				SEOUL_UNITTESTING_ASSERT(FileManager::Get()->WriteAll(s, &header, sizeof(header)));
				PackageFileSystem pkg(s);
				SEOUL_UNITTESTING_ASSERT(!pkg.IsOk());
			}
			header.SetPlatformAndObfuscation((Platform) - 1, true);
			{
				SEOUL_UNITTESTING_ASSERT(FileManager::Get()->WriteAll(s, &header, sizeof(header)));
				PackageFileSystem pkg(s);
				SEOUL_UNITTESTING_ASSERT(!pkg.IsOk());
			}
		}
	}
}

void PackageFileSystemTest::TestGarbageFile()
{
	m_sSourcePackageFilename = Path::GetTempFileAbsoluteFilename();
	WriteGarbageToSourceFile(Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/DownloadablePackageFileSystem/PC_Content.sar"));
	m_pSystem = FileManager::Get()->RegisterFileSystem<PackageFileSystem>(m_sSourcePackageFilename);
	InternalInitializeFailureCommon();
}

void PackageFileSystemTest::TestGetDirectoryListing()
{
	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/PackageFileSystem/Regress1_PC_ConfigUpdateB.sar");
	m_pSystem = FileManager::Get()->RegisterFileSystem<PackageFileSystem>(m_sSourcePackageFilename);

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

void PackageFileSystemTest::TestHeader()
{
	PackageFileHeader header{};
	SEOUL_UNITTESTING_ASSERT_EQUAL(PackageFileHeader(), header);
	header.SetPlatformAndObfuscation(Platform::kLinux, true);
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(PackageFileHeader(), header);
	SEOUL_UNITTESTING_ASSERT_EQUAL(keCurrentPlatform, header.GetPlatform());
	SEOUL_UNITTESTING_ASSERT_EQUAL(true, header.IsObfuscated());

	header.m_uVersion = kuPackageVersion;
	header.SetPlatformAndObfuscation(Platform::kLinux, false);
	SEOUL_UNITTESTING_ASSERT_EQUAL(Platform::kLinux, header.GetPlatform());
	SEOUL_UNITTESTING_ASSERT_EQUAL(false, header.IsObfuscated());

	auto const orig = header;
	EndianSwap(header);
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(orig, header);
	EndianSwap(header);
	SEOUL_UNITTESTING_ASSERT_EQUAL(orig, header);
}

void PackageFileSystemTest::TestLargeFile()
{
	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/PackageFileSystem/PC_MusicContent.sar");
	m_pSystem = FileManager::Get()->RegisterFileSystem<PackageFileSystem>(m_sSourcePackageFilename);

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
		"UnitTests/PackageFileSystem/Music_bank01.bank"),
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

void PackageFileSystemTest::TestMiscApi(Byte const* sPrefix)
{
	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), String::Printf("UnitTests/PackageFileSystem/%sMeasure_PC_ConfigUpdate1.sar", sPrefix));
	m_pSystem = FileManager::Get()->RegisterFileSystem<PackageFileSystem>(m_sSourcePackageFilename);

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
	// Game directory.
	SEOUL_UNITTESTING_ASSERT_EQUAL(GameDirectory::kConfig, m_pSystem->GetPackageGameDirectory());
	SEOUL_UNITTESTING_ASSERT_EQUAL(SerializedGameDirectory::kConfig, m_pSystem->GetHeader().GetGameDirectory());
	SEOUL_UNITTESTING_ASSERT_EQUAL((Int)m_pSystem->GetHeader().GetGameDirectory(), (Int)m_pSystem->GetPackageGameDirectory());
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
	SEOUL_UNITTESTING_ASSERT_EQUAL(m_sSourcePackageFilename, m_pSystem->GetAbsolutePackageFilename());
	// GetBuildChangelist()
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(1u, m_pSystem->GetBuildChangelist());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1u, m_pSystem->GetHeader().GetBuildChangelist());
		SEOUL_UNITTESTING_ASSERT_EQUAL(m_pSystem->GetHeader().GetBuildChangelist(), m_pSystem->GetBuildChangelist());
	}
	// GetBuildVersionMajor()
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(44u, m_pSystem->GetBuildVersionMajor());
		SEOUL_UNITTESTING_ASSERT_EQUAL(44u, m_pSystem->GetHeader().GetBuildVersionMajor());
		SEOUL_UNITTESTING_ASSERT_EQUAL(m_pSystem->GetHeader().GetBuildVersionMajor(), m_pSystem->GetBuildVersionMajor());
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
}

void PackageFileSystemTest::TestPerformCrc32EdgeCases()
{
	// Bad package.
	{
		PackageFileSystem pkg{String()};
		SEOUL_UNITTESTING_ASSERT(!pkg.PerformCrc32Check());

		PackageCrc32Entry entry;
		entry.m_FilePath = FilePath::CreateConfigFilePath("application.json");
		auto vFiles(PackageCrc32Entries(1u, entry));
		SEOUL_UNITTESTING_ASSERT(!pkg.PerformCrc32Check(&vFiles));
		SEOUL_UNITTESTING_ASSERT(!pkg.PerformCrc32Check(entry.m_FilePath));
	}

	// Bad package (old version).
	{
		// Name.
		String s;

		// Create the bad archive.
		{
			Byte a[] = { 'a', 'b', 'c', 'd' };

			Entry entry;
			entry.m_FilePath = FilePath::CreateConfigFilePath("application.json");
			entry.m_pData = a;
			entry.m_uData = sizeof(a);
			Files vFiles;
			vFiles.PushBack(entry);

			PackageFileHeader header;
			s = GenArchive(PackageFileHeader::ku16_LZ4CompressionVersion, GameDirectory::kConfig, 1u, 1u, true, Platform::kPC, vFiles, &header);

			// Now 0 out the data so they have bad crc32 data.
			Vector<Byte> v;
			SEOUL_UNITTESTING_ASSERT(FileManager::Get()->ReadAll(s, v));
			memset(v.Data() + sizeof(header), 0, (size_t)(header.GetOffsetToFileTableInBytes() - sizeof(header)));
			SEOUL_UNITTESTING_ASSERT(FileManager::Get()->WriteAll(s, v.Data(), v.GetSize()));
		}

		PackageFileSystem pkg{s};
		SEOUL_UNITTESTING_ASSERT(!pkg.PerformCrc32Check());

		PackageCrc32Entry entry;
		entry.m_FilePath = FilePath::CreateConfigFilePath("application.json");
		auto vFiles(PackageCrc32Entries(1u, entry));
		SEOUL_UNITTESTING_ASSERT(!pkg.PerformCrc32Check(&vFiles));
		SEOUL_UNITTESTING_ASSERT(!pkg.PerformCrc32Check(entry.m_FilePath));
	}

	// No file.
	{
		PackageFileSystem pkg{ Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/PackageFileSystem/PC_Content.sar") };
		SEOUL_UNITTESTING_ASSERT(pkg.PerformCrc32Check());

		PackageCrc32Entry entry;
		entry.m_FilePath = FilePath::CreateConfigFilePath("application.json");
		auto vFiles(PackageCrc32Entries(1u, entry));
		SEOUL_UNITTESTING_ASSERT(pkg.PerformCrc32Check(&vFiles));
		SEOUL_UNITTESTING_ASSERT(!pkg.PerformCrc32Check(entry.m_FilePath));
	}

	// 0 size file.
	{
		String s;
		{
			Entry entry;
			entry.m_FilePath = FilePath::CreateConfigFilePath("application.json");

			Files vFiles;

			vFiles.PushBack(entry);
			entry.m_FilePath = FilePath::CreateConfigFilePath("loc.json");
			vFiles.PushBack(entry);
			entry.m_FilePath = FilePath::CreateConfigFilePath("input.json");
			vFiles.PushBack(entry);

			s = GenArchive(kuPackageVersion, GameDirectory::kConfig, 1u, 1u, false, keCurrentPlatform, vFiles);
		}

		{
			PackageFileSystem pkg{ s };
			SEOUL_UNITTESTING_ASSERT(pkg.PerformCrc32Check());

			{
				PackageCrc32Entry entry;
				entry.m_FilePath = FilePath::CreateConfigFilePath("loc.json");
				auto vFiles(PackageCrc32Entries(1u, entry));
				SEOUL_UNITTESTING_ASSERT(pkg.PerformCrc32Check(&vFiles));
				SEOUL_UNITTESTING_ASSERT(pkg.PerformCrc32Check(entry.m_FilePath));
			}
			{
				PackageCrc32Entry entry;
				entry.m_FilePath = FilePath::CreateConfigFilePath("loc.json");
				auto vFiles(PackageCrc32Entries(1u, entry));
				entry.m_FilePath = FilePath::CreateConfigFilePath("a.json");
				vFiles.PushBack(entry);
				SEOUL_UNITTESTING_ASSERT(pkg.PerformCrc32Check(&vFiles));
				SEOUL_UNITTESTING_ASSERT(!pkg.PerformCrc32Check(entry.m_FilePath));
			}
			{
				PackageCrc32Entry entry;
				entry.m_FilePath = FilePath::CreateConfigFilePath("loc.json");
				auto vFiles(PackageCrc32Entries(1u, entry));
				entry.m_FilePath = FilePath::CreateConfigFilePath("a.json");
				vFiles.PushBack(entry);
				entry.m_FilePath = FilePath::CreateConfigFilePath("application.json");
				vFiles.PushBack(entry);
				SEOUL_UNITTESTING_ASSERT(pkg.PerformCrc32Check(&vFiles));
				SEOUL_UNITTESTING_ASSERT(pkg.PerformCrc32Check(entry.m_FilePath));
			}
		}
	}
}

void PackageFileSystemTest::TestReadRaw(Byte const* sPrefix)
{
	m_sSourcePackageFilename = Path::Combine(GamePaths::Get()->GetConfigDir(), String::Printf("UnitTests/PackageFileSystem/%sMeasure_PC_ConfigUpdate1.sar", sPrefix));
	m_pSystem = FileManager::Get()->RegisterFileSystem<PackageFileSystem>(m_sSourcePackageFilename);

	SEOUL_UNITTESTING_ASSERT(m_pSystem->IsOk());
	SEOUL_UNITTESTING_ASSERT(m_pSystem->PerformCrc32Check());
	{
		PackageCrc32Entries v;
		SEOUL_UNITTESTING_ASSERT(m_pSystem->PerformCrc32Check(&v));
		SEOUL_UNITTESTING_ASSERT_EQUAL(4059, v.GetSize());
		for (UInt32 i = 0u; i < v.GetSize(); ++i)
		{
			auto const& e = v[i];
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, e.m_bCrc32Ok);
		}
		v.Clear();
		PackageCrc32Entry entry;
		entry.m_FilePath = FilePath::CreateContentFilePath(s_kaFiles[0]);
		v.PushBack(entry);
		entry.m_FilePath = FilePath::CreateContentFilePath("a.png");
		v.PushBack(entry);
		SEOUL_UNITTESTING_ASSERT(m_pSystem->PerformCrc32Check(&v));
	}

	IPackageFileSystem::FileTable t;
	SEOUL_UNITTESTING_ASSERT(m_pSystem->GetFileTable(t));
	SEOUL_UNITTESTING_ASSERT_EQUAL(4059, t.GetSize());

	for (auto const& pair : t)
	{
		auto const& entry = pair.Second.m_Entry;

		// Skip 0 entries.
		if (entry.m_uCompressedFileSize == 0u) { continue; }

		Vector<Byte> v;
		v.Resize((UInt32)entry.m_uCompressedFileSize);
		SEOUL_UNITTESTING_ASSERT(m_pSystem->ReadRaw(entry.m_uOffsetToFile, v.Data(), v.GetSizeInBytes()));

		auto const uActual = Seoul::GetCrc32((UInt8 const*)v.Data(), v.GetSizeInBytes());
		SEOUL_UNITTESTING_ASSERT_EQUAL(uActual, entry.m_uCrc32Post);
	}
}

namespace
{
	class SeekFailFile SEOUL_SEALED : public DiskSyncFile
	{
	public:
		static Atomic32Value<Bool> s_bSeekFail;

		SeekFailFile(const String& sAbsoluteFilename, File::Mode eMode)
			: DiskSyncFile(sAbsoluteFilename, eMode)
		{
		}

		virtual Bool Seek(Int64 iPosition, File::SeekMode eMode) SEOUL_OVERRIDE
		{
			if (s_bSeekFail) { return false; }

			return DiskSyncFile::Seek(iPosition, eMode);
		}

	private:
		SEOUL_DISABLE_COPY(SeekFailFile);
	};
	Atomic32Value<Bool> SeekFailFile::s_bSeekFail{};

	class SeekFailSystem SEOUL_SEALED : public DiskFileSystem
	{
		virtual Bool Open(
			const String& s,
			File::Mode eMode,
			ScopedPtr<SyncFile>& rpFile) SEOUL_OVERRIDE
		{
			rpFile.Reset(SEOUL_NEW(MemoryBudgets::Io) SeekFailFile(s, eMode));
			return true;
		}
	};
} // namespace

void PackageFileSystemTest::TestSeekFail()
{
	m_sSourcePackageFilename = Path::GetTempFileAbsoluteFilename();
	SEOUL_UNITTESTING_ASSERT(CopyFile(Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/PackageFileSystem/PC_Content.sar"), m_sSourcePackageFilename));

	FileManager::Get()->RegisterFileSystem<SeekFailSystem>();

	// True one with seeking disabled, should fail to initialize.
	{
		SeekFailFile::s_bSeekFail = true;
		PackageFileSystem pkg(m_sSourcePackageFilename);
		SEOUL_UNITTESTING_ASSERT(!pkg.IsOk());
		SeekFailFile::s_bSeekFail = false;
	}

	m_pSystem = FileManager::Get()->RegisterFileSystem<PackageFileSystem>(m_sSourcePackageFilename, false, true);
	SeekFailFile::s_bSeekFail = true;

	// Commit should fail due to seek failure.
	{
		Byte a = 1;
		SEOUL_UNITTESTING_ASSERT(!m_pSystem->CommitChangeToSarFile(&a, sizeof(a), 0));
	}

	// Read raw should fail.
	{
		Byte a = 0;
		SEOUL_UNITTESTING_ASSERT(!m_pSystem->ReadRaw(0u, &a, sizeof(a)));
	}

	// Same as common test but with expecting specific failures.
	{
		SEOUL_UNITTESTING_ASSERT(m_pSystem->IsOk());
		SEOUL_UNITTESTING_ASSERT(!m_pSystem->PerformCrc32Check());
		{
			PackageCrc32Entries v;
			SEOUL_UNITTESTING_ASSERT(!m_pSystem->PerformCrc32Check(&v));
			SEOUL_UNITTESTING_ASSERT_EQUAL(4u, v.GetSize());
			for (UInt32 i = 0u; i < v.GetSize(); ++i)
			{
				auto const& e = v[i];
				SEOUL_UNITTESTING_ASSERT_EQUAL(e.m_FilePath, FilePath::CreateContentFilePath(s_kaFiles[3 - i]));
				SEOUL_UNITTESTING_ASSERT_EQUAL(false, e.m_bCrc32Ok);
			}
			v.Clear();
			PackageCrc32Entry entry;
			entry.m_FilePath = FilePath::CreateContentFilePath(s_kaFiles[0]);
			v.PushBack(entry);
			entry.m_FilePath = FilePath::CreateContentFilePath("a.png");
			v.PushBack(entry);
			SEOUL_UNITTESTING_ASSERT(!m_pSystem->PerformCrc32Check(&v));
		}

		PackageFileSystem::FileTable tFileTable;
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
			SEOUL_UNITTESTING_ASSERT(!m_pSystem->ReadAll(FilePath::CreateContentFilePath(s_kaFiles[i]), pTestData, uTestData, 0u, MemoryBudgets::TBD));
			MemoryManager::Deallocate(pTestData);
		}

		// All files should be fully present now, the archive should be ok.
		SEOUL_UNITTESTING_ASSERT(!IsCrc32Ok(*m_pSystem));

		for (auto const& pair : tFileTable)
		{
			SEOUL_UNITTESTING_ASSERT(!m_pSystem->PerformCrc32Check(pair.First));
		}
	}

	// Does not support directory listing.
	FilePath dirPath;
	dirPath.SetDirectory(m_pSystem->GetPackageGameDirectory());
	Vector<String> vs;
	SEOUL_UNITTESTING_ASSERT(!m_pSystem->GetDirectoryListing(dirPath, vs, false, true, String()));

	// Now re-enable seeking and run standard tests, should all succeed.
	SeekFailFile::s_bSeekFail = false;
	InternalTestCommon();
}

void PackageFileSystemTest::TestCorruptedFileTable()
{
	Entry entry;
	Files vFiles;

	Byte aData[] = { 'a', 's', 'd', 'f' };

	entry.m_FilePath = FilePath::CreateConfigFilePath("application.json");
	vFiles.PushBack(entry);
	entry.m_FilePath = FilePath::CreateConfigFilePath("input.json");
	vFiles.PushBack(entry);
	entry.m_FilePath = FilePath::CreateConfigFilePath("loc.json");
	entry.m_pData = aData;
	entry.m_uData = sizeof(aData);
	vFiles.PushBack(entry);

	PackageFileHeader header;

	// Offset to file table out of range.
	{
		auto const s(GenArchive(kuPackageVersion, GameDirectory::kConfig, 1u, 1u, false, Platform::kPC, vFiles, &header));
		Vector<Byte> v;
		SEOUL_UNITTESTING_ASSERT(FileManager::Get()->ReadAll(s, v));
		header.SetOffsetToFileTableInBytes(UInt64Max);
		memcpy(v.Data(), &header, sizeof(header));
		SEOUL_UNITTESTING_ASSERT(FileManager::Get()->WriteAll(s, v.Data(), v.GetSize()));

		PackageFileSystem pkg(s);
		SEOUL_UNITTESTING_ASSERT(!pkg.IsOk());
	}

	// Offset to file table bad.
	{
		auto const s(GenArchive(kuPackageVersion, GameDirectory::kConfig, 1u, 1u, false, Platform::kPC, vFiles, &header));
		Vector<Byte> v;
		SEOUL_UNITTESTING_ASSERT(FileManager::Get()->ReadAll(s, v));
		header.SetOffsetToFileTableInBytes(v.GetSize());
		memcpy(v.Data(), &header, sizeof(header));
		SEOUL_UNITTESTING_ASSERT(FileManager::Get()->WriteAll(s, v.Data(), v.GetSize()));

		PackageFileSystem pkg(s);
		SEOUL_UNITTESTING_ASSERT(!pkg.IsOk());
	}

	// Invalid compressed.
	{
		auto const s(GenArchive(kuPackageVersion, GameDirectory::kConfig, 1u, 1u, false, Platform::kPC, vFiles, &header));
		Vector<Byte> v;
		SEOUL_UNITTESTING_ASSERT(FileManager::Get()->ReadAll(s, v));
		header.SetHasCompressedFileTable(true);
		memcpy(v.Data(), &header, sizeof(header));
		SEOUL_UNITTESTING_ASSERT(FileManager::Get()->WriteAll(s, v.Data(), v.GetSize()));

		PackageFileSystem pkg(s);
		SEOUL_UNITTESTING_ASSERT(!pkg.IsOk());
	}

	// Invalid compressed (old).
	{
		auto const s(GenArchive(kuPackageVersion, GameDirectory::kConfig, 1u, 1u, false, Platform::kPC, vFiles, &header));
		Vector<Byte> v;
		SEOUL_UNITTESTING_ASSERT(FileManager::Get()->ReadAll(s, v));
		header.SetHasCompressedFileTable(true);
		header.m_uVersion = PackageFileHeader::ku16_LZ4CompressionVersion;
		memcpy(v.Data(), &header, sizeof(header));
		SEOUL_UNITTESTING_ASSERT(FileManager::Get()->WriteAll(s, v.Data(), v.GetSize()));

		PackageFileSystem pkg(s);
		SEOUL_UNITTESTING_ASSERT(!pkg.IsOk());
	}

	// Data missing.
	{
		auto const s(GenArchive(kuPackageVersion, GameDirectory::kConfig, 1u, 1u, false, Platform::kPC, vFiles, &header));
		Vector<Byte> v;
		SEOUL_UNITTESTING_ASSERT(FileManager::Get()->ReadAll(s, v));
		header.SetOffsetToFileTableInBytes(v.GetSize());
		header.SetSizeOfFileTableInBytes(0u);
		memcpy(v.Data(), &header, sizeof(header));
		SEOUL_UNITTESTING_ASSERT(FileManager::Get()->WriteAll(s, v.Data(), v.GetSize()));

		PackageFileSystem pkg(s);
		SEOUL_UNITTESTING_ASSERT(!pkg.IsOk());
	}

	// Bad offset.
	{
		auto const s(GenArchive(kuPackageVersion, GameDirectory::kConfig, 1u, 1u, false, Platform::kPC, vFiles, &header));
		Vector<Byte> v;
		SEOUL_UNITTESTING_ASSERT(FileManager::Get()->ReadAll(s, v));
		auto const uTotal = header.GetTotalPackageFileSizeInBytes();
		memcpy((UInt8*)v.Data() + header.GetOffsetToFileTableInBytes(), &uTotal, sizeof(uTotal));
		SEOUL_UNITTESTING_ASSERT(FileManager::Get()->WriteAll(s, v.Data(), v.GetSize()));

		PackageFileSystem pkg(s);
		SEOUL_UNITTESTING_ASSERT(!pkg.IsOk());
	}

	// Bad size.
	{
		auto const s(GenArchive(kuPackageVersion, GameDirectory::kConfig, 1u, 1u, false, Platform::kPC, vFiles, &header));
		Vector<Byte> v;
		SEOUL_UNITTESTING_ASSERT(FileManager::Get()->ReadAll(s, v));
		auto const uTotal = header.GetTotalPackageFileSizeInBytes();
		memcpy((UInt8*)v.Data() + header.GetOffsetToFileTableInBytes() + sizeof(UInt64), &uTotal, sizeof(uTotal));
		SEOUL_UNITTESTING_ASSERT(FileManager::Get()->WriteAll(s, v.Data(), v.GetSize()));

		PackageFileSystem pkg(s);
		SEOUL_UNITTESTING_ASSERT(!pkg.IsOk());
	}

	// Bad filename.
	{
		auto const s(GenArchive(kuPackageVersion, GameDirectory::kConfig, 1u, 1u, false, Platform::kPC, vFiles, &header));
		Vector<Byte> v;
		SEOUL_UNITTESTING_ASSERT(FileManager::Get()->ReadAll(s, v));
		PackageFileSystem::Obfuscate(
			PackageFileSystem::GenerateObfuscationKey(GetFileTablePseudoFilename(1u, 1u)),
			(UInt8*)v.Data() + header.GetOffsetToFileTableInBytes(),
			header.GetSizeOfFileTableInBytes(),
			0);
		memcpy((UInt8*)v.Data() + header.GetOffsetToFileTableInBytes() + sizeof(PackageFileEntry), &UIntMax, sizeof(UIntMax));
		PackageFileSystem::Obfuscate(
			PackageFileSystem::GenerateObfuscationKey(GetFileTablePseudoFilename(1u, 1u)),
			(UInt8*)v.Data() + header.GetOffsetToFileTableInBytes(),
			header.GetSizeOfFileTableInBytes(),
			0);
		SEOUL_UNITTESTING_ASSERT(FileManager::Get()->WriteAll(s, v.Data(), v.GetSize()));

		PackageFileSystem pkg(s);
		SEOUL_UNITTESTING_ASSERT(!pkg.IsOk());
	}

	// Bad filename (0 size).
	{
		auto const s(GenArchive(kuPackageVersion, GameDirectory::kConfig, 1u, 1u, false, Platform::kPC, vFiles, &header));
		Vector<Byte> v;
		SEOUL_UNITTESTING_ASSERT(FileManager::Get()->ReadAll(s, v));
		PackageFileSystem::Obfuscate(
			PackageFileSystem::GenerateObfuscationKey(GetFileTablePseudoFilename(1u, 1u)),
			(UInt8*)v.Data() + header.GetOffsetToFileTableInBytes(),
			header.GetSizeOfFileTableInBytes(),
			0);
		UInt32 uZero = 0u;
		memcpy((UInt8*)v.Data() + header.GetOffsetToFileTableInBytes() + sizeof(PackageFileEntry), &uZero, sizeof(uZero));
		PackageFileSystem::Obfuscate(
			PackageFileSystem::GenerateObfuscationKey(GetFileTablePseudoFilename(1u, 1u)),
			(UInt8*)v.Data() + header.GetOffsetToFileTableInBytes(),
			header.GetSizeOfFileTableInBytes(),
			0);
		SEOUL_UNITTESTING_ASSERT(FileManager::Get()->WriteAll(s, v.Data(), v.GetSize()));

		PackageFileSystem pkg(s);
		SEOUL_UNITTESTING_ASSERT(!pkg.IsOk());
	}

	// Bad filename (too big).
	{
		auto const s(GenArchive(kuPackageVersion, GameDirectory::kConfig, 1u, 1u, false, Platform::kPC, vFiles, &header));
		Vector<Byte> v;
		SEOUL_UNITTESTING_ASSERT(FileManager::Get()->ReadAll(s, v));
		PackageFileSystem::Obfuscate(
			PackageFileSystem::GenerateObfuscationKey(GetFileTablePseudoFilename(1u, 1u)),
			(UInt8*)v.Data() + header.GetOffsetToFileTableInBytes(),
			header.GetSizeOfFileTableInBytes(),
			0);
		UInt32 uLarge = (UInt32)header.GetTotalPackageFileSizeInBytes();
		memcpy((UInt8*)v.Data() + header.GetOffsetToFileTableInBytes() + sizeof(PackageFileEntry), &uLarge, sizeof(uLarge));
		PackageFileSystem::Obfuscate(
			PackageFileSystem::GenerateObfuscationKey(GetFileTablePseudoFilename(1u, 1u)),
			(UInt8*)v.Data() + header.GetOffsetToFileTableInBytes(),
			header.GetSizeOfFileTableInBytes(),
			0);
		SEOUL_UNITTESTING_ASSERT(FileManager::Get()->WriteAll(s, v.Data(), v.GetSize()));

		PackageFileSystem pkg(s);
		SEOUL_UNITTESTING_ASSERT(!pkg.IsOk());
	}

	// Bad filename (no terminator).
	{
		auto const s(GenArchive(kuPackageVersion, GameDirectory::kConfig, 1u, 1u, false, Platform::kPC, vFiles, &header));
		Vector<Byte> v;
		SEOUL_UNITTESTING_ASSERT(FileManager::Get()->ReadAll(s, v));
		PackageFileSystem::Obfuscate(
			PackageFileSystem::GenerateObfuscationKey(GetFileTablePseudoFilename(1u, 1u)),
			(UInt8*)v.Data() + header.GetOffsetToFileTableInBytes(),
			header.GetSizeOfFileTableInBytes(),
			0);
		auto pOffset = v.Data() + header.GetOffsetToFileTableInBytes() + sizeof(PackageFileEntry) + sizeof(UInt32);
		memset(pOffset, 7, (size_t)(v.End() - pOffset));
		PackageFileSystem::Obfuscate(
			PackageFileSystem::GenerateObfuscationKey(GetFileTablePseudoFilename(1u, 1u)),
			(UInt8*)v.Data() + header.GetOffsetToFileTableInBytes(),
			header.GetSizeOfFileTableInBytes(),
			0);
		SEOUL_UNITTESTING_ASSERT(FileManager::Get()->WriteAll(s, v.Data(), v.GetSize()));

		PackageFileSystem pkg(s);
		SEOUL_UNITTESTING_ASSERT(!pkg.IsOk());
	}

	// Duplicate entries.
	{
		Files vFiles2;
		vFiles2.PushBack(entry);
		vFiles2.PushBack(entry);
		vFiles2.PushBack(entry);
		auto const s(GenArchive(kuPackageVersion, GameDirectory::kConfig, 1u, 1u, false, Platform::kPC, vFiles2, nullptr, true));

		PackageFileSystem pkg(s);
		SEOUL_UNITTESTING_ASSERT(!pkg.IsOk());
	}
}

void PackageFileSystemTest::TestCorruptedFileTableV20()
{
	auto const s(Path::Combine(GamePaths::Get()->GetConfigDir(), String::Printf("UnitTests/PackageFileSystem/V20_Measure_PC_ConfigUpdate1.sar")));

	void* p = nullptr;
	UInt32 u = 0u;
	SEOUL_UNITTESTING_ASSERT(FileManager::Get()->ReadAll(s, p, u, 0u, MemoryBudgets::TBD));

	// Scribble the file table.
	{
		PackageFileHeader header;
		memcpy(&header, p, sizeof(header));

		UInt8* pOut = (UInt8*)p + header.GetOffsetToFileTableInBytes();

		// Offset here is to leave the data "valid enough" so that the decompression
		// would crash if the CRC32 check was not present.
		pOut[171u] = ~pOut[171u];
	}

	PackageFileSystem system(p, u, true);
	SEOUL_UNITTESTING_ASSERT(!system.IsOk());
}

void PackageFileSystemTest::TestCorruptedFileTableV21()
{
	auto const s(Path::Combine(GamePaths::Get()->GetConfigDir(), String::Printf("UnitTests/PackageFileSystem/V21_Measure_PC_ConfigUpdate1.sar")));

	void* p = nullptr;
	UInt32 u = 0u;
	SEOUL_UNITTESTING_ASSERT(FileManager::Get()->ReadAll(s, p, u, 0u, MemoryBudgets::TBD));

	// Scribble the file table.
	{
		PackageFileHeader header;
		memcpy(&header, p, sizeof(header));

		UInt8* pOut = (UInt8*)p + header.GetOffsetToFileTableInBytes();

		// Offset here is to leave the data "valid enough" so that the decompression
		// would crash if the CRC32 check was not present.
		pOut[171u] = ~pOut[171u];
	}

	PackageFileSystem system(p, u, true);
	SEOUL_UNITTESTING_ASSERT(!system.IsOk());
}

void PackageFileSystemTest::Destroy()
{
	m_pSystem.Reset();
	m_sSourcePackageFilename.Clear();
	m_pHelper.Reset();
}

void PackageFileSystemTest::Init()
{
	m_pHelper.Reset(SEOUL_NEW(MemoryBudgets::Developer) UnitTestsEngineHelper);
	m_sSourcePackageFilename.Clear();
	m_pSystem.Reset();
}

void PackageFileSystemTest::InternalInitializeFailureCommon()
{
	// All functions should fail when initialization has not occurred.
	SEOUL_UNITTESTING_ASSERT(!m_pSystem->Exists(FilePath::CreateContentFilePath("a")));
	SEOUL_UNITTESTING_ASSERT(!m_pSystem->IsOk());
	SEOUL_UNITTESTING_ASSERT(!m_pSystem->NetworkFetch(FilePath::CreateContentFilePath("a")));
	SEOUL_UNITTESTING_ASSERT(!m_pSystem->NetworkPrefetch(FilePath::CreateContentFilePath("a")));
	Vector<String> vsUnused;
	SEOUL_UNITTESTING_ASSERT(!m_pSystem->GetDirectoryListing(FilePath::CreateContentFilePath("a"), vsUnused));
	UInt64 uUnused;
	SEOUL_UNITTESTING_ASSERT(!m_pSystem->GetFileSize(FilePath::CreateContentFilePath("a"), uUnused));
	SEOUL_UNITTESTING_ASSERT(!m_pSystem->GetModifiedTime(FilePath::CreateContentFilePath("a"), uUnused));

	ScopedPtr<SyncFile> pUnusedFile;
	SEOUL_UNITTESTING_ASSERT(!m_pSystem->Open(FilePath::CreateContentFilePath("a"), File::kRead, pUnusedFile));

	void* pUnused = nullptr;
	UInt32 uUnusedSize = 0u;
	SEOUL_UNITTESTING_ASSERT(!m_pSystem->ReadAll(FilePath::CreateContentFilePath("a"), pUnused, uUnusedSize, 0u, MemoryBudgets::Developer));

	SEOUL_UNITTESTING_ASSERT(!m_pSystem->SetModifiedTime(FilePath::CreateContentFilePath("a"), uUnused));
}

void PackageFileSystemTest::InternalTestCommon()
{
	SEOUL_UNITTESTING_ASSERT(m_pSystem->IsOk());
	SEOUL_UNITTESTING_ASSERT(m_pSystem->PerformCrc32Check());
	{
		PackageCrc32Entries v;
		SEOUL_UNITTESTING_ASSERT(m_pSystem->PerformCrc32Check(&v));
		SEOUL_UNITTESTING_ASSERT_EQUAL(4u, v.GetSize());
		for (UInt32 i = 0u; i < v.GetSize(); ++i)
		{
			auto const& e = v[i];
			SEOUL_UNITTESTING_ASSERT_EQUAL(e.m_FilePath, FilePath::CreateContentFilePath(s_kaFiles[3 - i]));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, e.m_bCrc32Ok);
		}
		v.Clear();
		PackageCrc32Entry entry;
		entry.m_FilePath = FilePath::CreateContentFilePath(s_kaFiles[0]);
		v.PushBack(entry);
		entry.m_FilePath = FilePath::CreateContentFilePath("a.png");
		v.PushBack(entry);
		SEOUL_UNITTESTING_ASSERT(m_pSystem->PerformCrc32Check(&v));
	}

	PackageFileSystem::FileTable tFileTable;
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
			"UnitTests/PackageFileSystem",
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

	for (auto const& pair : tFileTable)
	{
		SEOUL_UNITTESTING_ASSERT(m_pSystem->PerformCrc32Check(pair.First));
	}
}

void PackageFileSystemTest::WriteGarbageToSourceFile(const String& sInput)
{
	void* p = nullptr;
	UInt32 u = 0u;
	SEOUL_UNITTESTING_ASSERT(FileManager::Get()->ReadAll(
		sInput,
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
		m_sSourcePackageFilename,
		p,
		u));

	MemoryManager::Deallocate(p);
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
