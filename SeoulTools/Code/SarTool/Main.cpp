/**
 * \file Main.cpp
 * \brief Miscellaneous utilities for working with SeoulEngine .SAR archives
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "CommandLineArgWrapper.h"
#include "Compress.h"
#include "CoreSettings.h"
#include "DataStoreParser.h"
#include "Directory.h"
#include "DiskFileSystem.h"
#include "FileManager.h"
#include "Logger.h"
#include "PackageFileSystem.h"
#include "Path.h"
#include "Platform.h"
#include "Prereqs.h"
#include "ReflectionCommandLineArgs.h"
#include "ReflectionDefine.h"
#include "ReflectionScriptStub.h"
#include "ReflectionSerialize.h"
#include "ScopedAction.h"
#include "SeoulMD5.h"
#include "SeoulUtil.h"
#include "StringUtil.h"
#include "Thread.h"
#include "Vector.h"

namespace Seoul
{

// Use default core virtuals
const CoreVirtuals* g_pCoreVirtuals = &g_DefaultCoreVirtuals;

static String GetAppName()
{
#if SEOUL_PLATFORM_WINDOWS
	WCHAR a[MAX_PATH];
	::GetModuleFileNameW(nullptr, a, MAX_PATH);

	return Path::GetFileName(WCharTToUTF8(a));
#else
	// TODO:
	return String();
#endif
}

#define SEOUL_ERR(fmt, ...) fprintf(stderr, "%s: error: " fmt "\n", GetAppName().CStr(), ##__VA_ARGS__)

// Sanity check - update this code if the package version is updated
SEOUL_STATIC_ASSERT(21u == kuPackageVersion);

/** For operations that require it, track the absolute filename to a PackageFileSystem to register with FileManager. */
static String s_sPackageFileSystemFilename;
static CheckedPtr<PackageFileSystem> s_pPackageFileSystem;

/** Function pointer to handlers of commandline operations. */
typedef Bool (*OpFunction)(const String& sInputFilename, const String& sOutputFilename);

enum class SarToolCommand
{
	None,
	DumpJson,
	DumpJsonGz,
	Extract,
	List,
	PrintChangelist,
	PrintVersion,
	Stats,
};
SEOUL_BEGIN_ENUM(SarToolCommand)
	SEOUL_ENUM_N("", SarToolCommand::None)
	SEOUL_ENUM_N("dump_json", SarToolCommand::DumpJson)
		SEOUL_ATTRIBUTE(Description, "dump the entire .sar file to a textual .json file")
	SEOUL_ENUM_N("dump_json_gz", SarToolCommand::DumpJsonGz)
		SEOUL_ATTRIBUTE(Description, "dump the entire .sar file to a compressed .json.gz file")
	SEOUL_ENUM_N("extract", SarToolCommand::Extract)
		SEOUL_ATTRIBUTE(Description, "extract a single file with name from the .sar")
	SEOUL_ENUM_N("list", SarToolCommand::List)
		SEOUL_ATTRIBUTE(Description, "list file contents of the .sar")
	SEOUL_ENUM_N("print_changelist", SarToolCommand::PrintChangelist)
		SEOUL_ATTRIBUTE(Description, "print the build changelist of the .sar to stdout")
	SEOUL_ENUM_N("print_version", SarToolCommand::PrintVersion)
		SEOUL_ATTRIBUTE(Description, "print the build version of the .sar to stdout")
	SEOUL_ENUM_N("stats", SarToolCommand::Stats)
		SEOUL_ATTRIBUTE(Description, "print statistics of the .sar to stdout")
SEOUL_END_ENUM()

struct SarToolCommandDesc
{
	OpFunction m_Function;
	Bool m_bNeedsOutput;
	Bool m_bNeedsInputAsFileSystem;
};

/**
 * Root level command-line arguments - handled by reflection, can be
 * configured via the literal command-line, environment variables, or
 * a configuration file.
 */
class SarToolCommandLineArgs SEOUL_SEALED
{
public:
	static SarToolCommand Command;
	static String Input;
	static String Output;
	static CommandLineArgWrapper<String> ToExtract;
	static CommandLineArgWrapper<UInt32> Changelist;
	static CommandLineArgWrapper<UInt32> Version;
	static Bool VerboseDump;

private:
	SarToolCommandLineArgs() = delete; // Static class.
	SEOUL_DISABLE_COPY(SarToolCommandLineArgs);
};

SarToolCommand SarToolCommandLineArgs::Command{};
String SarToolCommandLineArgs::Input{};
String SarToolCommandLineArgs::Output{};
CommandLineArgWrapper<String> SarToolCommandLineArgs::ToExtract{};
CommandLineArgWrapper<UInt32> SarToolCommandLineArgs::Changelist{};
CommandLineArgWrapper<UInt32> SarToolCommandLineArgs::Version{};
Bool SarToolCommandLineArgs::VerboseDump{};

SEOUL_BEGIN_TYPE(SarToolCommandLineArgs, TypeFlags::kDisableNew | TypeFlags::kDisableCopy)
	SEOUL_CMDLINE_PROPERTY(Command, 0, "command", true)
	SEOUL_CMDLINE_PROPERTY(Input, 1, "input", true)
	SEOUL_CMDLINE_PROPERTY(Output, "o", "file")
		SEOUL_ATTRIBUTE(Description, "output file")
	SEOUL_CMDLINE_PROPERTY(ToExtract, "to_extract", "file")
		SEOUL_ATTRIBUTE(Description, "name of file in .sar to extract to output")
	SEOUL_CMDLINE_PROPERTY(Changelist, "changelist", "value")
		SEOUL_ATTRIBUTE(Description, "target changelist for associated cmds (e.g. -set_changelist)")
	SEOUL_CMDLINE_PROPERTY(Version, "version", "value")
		SEOUL_ATTRIBUTE(Description, "target version for associated cmds (e.g. -set_version)")
	SEOUL_CMDLINE_PROPERTY(VerboseDump, "verbose_dump")
		SEOUL_ATTRIBUTE(Description, "in JSON dump, include offsets and file bodies")
SEOUL_END_TYPE()

/**
 * @return True if the type of file in filePath is plain text, false
 * otherwise.
 */
static Bool IsText(FilePath filePath)
{
	switch (filePath.GetType())
	{
	case FileType::kCsv: // fall-through
	case FileType::kHtml: // fall-through
	case FileType::kJson: // fall-through
	case FileType::kPEMCertificate: // fall-through
	case FileType::kText:
		return true;
	default:
		return false;
	};
}

/**
 * @return Read the package header and return true on success, false otherwise.
 */
static Bool ReadPackageHeader(const String& sFilename, PackageFileHeader& rHeader, Bool& rbEndianSwapped)
{
	// Open input and output files.
	DiskSyncFile in(sFilename, File::kRead);

	// Read the package header.
	PackageFileHeader inHeader;
	if (sizeof(inHeader) != in.ReadRawData(&inHeader, sizeof(inHeader)))
	{
		SEOUL_ERR("failed reading package header, corrupt file");
		return false;
	}

	// Record and endian swap the header if necessary.
	Bool const bEndianSwapped = inHeader.RequiresEndianSwap();

	// Read the data.
	PackageFileHeader header;
	if (!PackageFileSystem::ReadPackageHeader(&inHeader, sizeof(inHeader), header))
	{
		SEOUL_ERR("failed reading package header, header is corrupt");
		return false;
	}

	rHeader = header;
	rbEndianSwapped = bEndianSwapped;
	return true;
}

/**
 * Perform a diff between a and b, writing a SeoulEngine DataStore syntax file to rOut
 * with the results.
 */
static Bool WriteDiff(const DataStore& a, const DataStore& b, DiskSyncFile& rOut)
{
	// TODO: This diff tool can be more flexible (allow roots other than table).

	// Root of both must be a table.
	if (!a.GetRootNode().IsTable() || !b.GetRootNode().IsTable())
	{
		SEOUL_ERR("A root is type '%s' and B root is type '%s', type of both must be 'Table'",
			EnumToString<DataNode::Type>(a.GetRootNode().GetType()),
			EnumToString<DataNode::Type>(b.GetRootNode().GetType()));
		return false;
	}

	DataStore diff;
	if (!ComputeDiff(a, b, diff))
	{
		SEOUL_ERR("failed populating diff of root tables");
		return false;
	}

	String sOut;
	diff.ToString(diff.GetRootNode(), sOut, true, 0, true);

	if (!sOut.IsEmpty())
	{
		if (sOut.GetSize() != rOut.WriteRawData(sOut.CStr(), sOut.GetSize()))
		{
			SEOUL_ERR("failed writing diff data to disk");
			return false;
		}
	}

	return true;
}

/**
 * Utility structure, contains full archive file size, last modified
 * timestamp, and an MD5 hash of the .sar, to be used to identify a .sar.
 */
struct ArchiveIdentity
{
	UInt64 m_zSizeInBytes;
	UInt64 m_zTimestamp;
	String m_sMD5Hash;
};

SEOUL_BEGIN_TYPE(ArchiveIdentity)
	SEOUL_PROPERTY_N("SizeInBytes", m_zSizeInBytes)
	SEOUL_PROPERTY_N("Timestamp", m_zTimestamp)
	SEOUL_PROPERTY_N("MD5Hash", m_sMD5Hash)
SEOUL_END_TYPE()

/**
 * Converts the contents of sInputFilename into a JSON format
 * text file written to sOutputFilename.
 */
Bool DumpJsonCommon(const String& sInputFilename, const String& sOutputFilename, String& rsOutput)
{
	// Key name for the identity info of the .sar
	static const HString kIdentityKey("ArchiveIdentity");

	// Key name for the header info of the .sar
	static const HString kHeaderKey("Header");

	// Key name of the list of files and their contents.
	static const HString kFilesKey("Files");

	// Key name of the entry that contains the body of a file.
	static const HString kContents("Contents");

	using namespace Reflection;

	// Whether we should exclude or control bits that make diffing harder.
	Bool const bDiffFriendly = !SarToolCommandLineArgs::VerboseDump;

	// Populate identity info - file size, last modified time, and MD5 hash
	// of the entire archive file.
	ArchiveIdentity identity;
	identity.m_zSizeInBytes = DiskSyncFile::GetFileSize(sInputFilename);
	if (0u == identity.m_zSizeInBytes)
	{
		SEOUL_ERR("failed getting input file size");
		return false;
	}

	identity.m_zTimestamp = DiskSyncFile::GetModifiedTime(sInputFilename);
	if (0u == identity.m_zTimestamp)
	{
		SEOUL_ERR("failed getting input file timestamp");
		return false;
	}

	// Generate an MD5 hash of the entire archive file.
	FixedArray<UInt8, MD5::kResultSize> aMD5Hash;
	{
		DiskSyncFile file(sInputFilename, File::kRead);
		if (!file.CanRead())
		{
			SEOUL_ERR("failed opening input file to generated MD5");
			return false;
		}

		MD5 md5(aMD5Hash);

		// Local buffer for processing and sanity checks.
		static const UInt32 kBufferSize = 4096u;
		SEOUL_STATIC_ASSERT(0u == (kBufferSize % MD5::kBlockSize));
		Byte aBuffer[kBufferSize];

		// Read the entire file a kBufferSize block at a time.
		UInt32 zReadSize = file.ReadRawData(aBuffer, sizeof(aBuffer));
		while (zReadSize > 0u)
		{
			md5.AppendData(aBuffer, zReadSize);
			zReadSize = file.ReadRawData(aBuffer, sizeof(aBuffer));
		}
	}
	identity.m_sMD5Hash = HexDump(aMD5Hash.Data(), aMD5Hash.GetSize());

	// Now open the package to extract contents, run a full Crc32
	// check before continuing.
	PackageFileSystem package(sInputFilename);
	if (!package.PerformCrc32Check())
	{
		SEOUL_ERR("failed opening input file, package is corrupt");
		return false;
	}

	// The output data store root is a table.
	DataStore dataStore;
	dataStore.MakeTable();

	// Serialize identification information.
	if (!SerializeObjectToTable(ContentKey(), dataStore, dataStore.GetRootNode(), kIdentityKey, WeakAny(&identity)))
	{
		SEOUL_ERR("failed writing identity info to output JSON");
		return false;
	}

	// Serialize the header information.
	PackageFileHeader header = package.GetHeader();
	if (!SerializeObjectToTable(ContentKey(), dataStore, dataStore.GetRootNode(), kHeaderKey, WeakAny(&header)))
	{
		SEOUL_ERR("failed writing header info to output JSON");
		return false;
	}

	// Utility for sorting file entries.
	struct FileEntry
	{
		FilePath m_FilePath;
		PackageFileTableEntry m_Entry;

		Bool operator<(const FileEntry& b) const
		{
			return (m_Entry.m_Entry.m_uOffsetToFile < b.m_Entry.m_Entry.m_uOffsetToFile);
		}
	};

	// Gather file entries and resort them by offset
	// into the archive.
	Vector<FileEntry> vEntries;
	const PackageFileSystem::FileTable& tFileTable = package.GetFileTable();
	for (PackageFileSystem::FileTable::ConstIterator i = tFileTable.Begin();
		tFileTable.End() != i;
		++i)
	{
		FileEntry entry;
		entry.m_FilePath = i->First;
		entry.m_Entry = i->Second;
		vEntries.PushBack(entry);
	}

	QuickSort(vEntries.Begin(), vEntries.End());

	// Add an array for all files in the archive.
	DataNode filesArray;
	if (!dataStore.SetArrayToTable(dataStore.GetRootNode(), kFilesKey, vEntries.GetSize()))
	{
		SEOUL_ERR("failed writing files array to output JSON");
		return false;
	}

	// Get the array back out for processing.
	if (!dataStore.GetValueFromTable(dataStore.GetRootNode(), kFilesKey, filesArray))
	{
		SEOUL_ERR("failed getting files array");
		return false;
	}

	// Enumerate all file entries and write the data to the corresponding
	// array in the DataStore.
	for (UInt32 i = 0u; i < vEntries.GetSize(); ++i)
	{
		// Serialize the basic file header info at the array index.
		FileEntry entry = vEntries[i];

		{
			PackageFileEntry fileEntry = entry.m_Entry.m_Entry;

			// If bDiffFriendly is true, 0 out the offset so it doesn't change - any change
			// to a file will cause all entries after that file to have a different offset,
			// which causes a lot of noise in a diff.
			if (bDiffFriendly)
			{
				fileEntry.m_uOffsetToFile = 0;
			}

			if (!SerializeObjectToArray(ContentKey(), dataStore, filesArray, i, WeakAny(&fileEntry)))
			{
				SEOUL_ERR("failed writing file info to output JSON");
				return false;
			}
		}

		// The previous step should have inserted a table at the array
		// index - retrieve it so we can add additional data.
		DataNode tableEntry;
		if (!dataStore.GetValueFromArray(filesArray, i, tableEntry))
		{
			SEOUL_ERR("failed getting file info");
			return false;
		}

		// Add the FilePath to the file.
		if (!dataStore.SetFilePathToTable(tableEntry, HString("FilePath"), entry.m_FilePath))
		{
			SEOUL_ERR("failed writing FilePath info to output JSON");
			return false;
		}

		// Determine if the file is text or a binary file.
		Bool const bText = IsText(entry.m_FilePath);

		// bSuccess indicates that an error occured when set to false, bDone
		// indicates that the process has completed with success.
		Bool bSuccess = true;
		Bool bDone = false;

		// If the file is text, try to parse it as a DataStore so it can be encoded as JSON.
		if (bText)
		{
			// If the file is a JSON file, attempt to parse it as a DataStore. If this succeeds, merge the
			// contents into the appropriate node.
			if (FileType::kJson == entry.m_FilePath.GetType())
			{
				DataStore fileData;
				if (!DataStoreParser::FromFile(entry.m_FilePath, fileData, DataStoreParserFlags::kLogParseErrors))
				{
					SEOUL_ERR("'%s' cannot be converted to JSON, check for syntax errors", entry.m_FilePath.GetRelativeFilename().CStr());
					bSuccess = false;
				}
				else
				{
					// All actions in this block must succeed or something crazy has happened,
					// so track failures as an error.
					if (fileData.GetRootNode().IsTable())
					{
						bSuccess = bSuccess && dataStore.SetTableToTable(tableEntry, kContents);
					}
					else
					{
						bSuccess = bSuccess && dataStore.SetArrayToTable(tableEntry, kContents);
					}

					DataNode to;
					bSuccess = bSuccess && dataStore.GetValueFromTable(tableEntry, kContents, to);
					bSuccess = bSuccess && dataStore.DeepCopy(fileData, fileData.GetRootNode(), to, false);

					// We're done if all operations were successful.
					bDone = bSuccess;
				}
			}
		}

		// If the file type is binary and bDiffFriendly is true, just write <binary> to the entry.
		if (!bText && bDiffFriendly)
		{
			bSuccess = dataStore.SetStringToTable(tableEntry, kContents, "<binary>");
			bDone = bSuccess;
		}

		// If the file was processed as JSON text, try to read it in
		// and process it as a String or binary blob.
		if (!bDone && bSuccess)
		{
			// Read the entire contents of the file from the archive.
			ScopedPtr<SyncFile> pFile;
			if (!package.Open(entry.m_FilePath, File::kRead, pFile) ||
				!pFile->CanRead())
			{
				SEOUL_ERR("failed opening individual file to encode into JSON");
				return false;
			}

			void* pData = nullptr;
			UInt32 zSizeInBytes = 0u;
			if (!pFile->ReadAll(pData, zSizeInBytes, 0u, MemoryBudgets::TBD))
			{
				SEOUL_ERR("failed reading file body to encode into JSON");
				return false;
			}

			// If the type is kJson, parse it directly into the appropriate entry of the table.
			if (entry.m_FilePath.GetType() == FileType::kJson)
			{
				// Serialize into a new DataStore, as the input data may be a cooked binary.
				DataStore jsonDataStore;
				bSuccess = DataStoreParser::FromString((Byte const*)pData, zSizeInBytes, jsonDataStore, DataStoreParserFlags::kLogParseErrors, entry.m_FilePath);

				// Copy to the target node on success.
				if (bSuccess)
				{
					// All actions in this block must succeed or something crazy has happened,
					// so track failures as an error.
					if (jsonDataStore.GetRootNode().IsTable())
					{
						bSuccess = bSuccess && dataStore.SetTableToTable(tableEntry, kContents);
					}
					else
					{
						bSuccess = bSuccess && dataStore.SetArrayToTable(tableEntry, kContents);
					}

					DataNode to;
					bSuccess = bSuccess && dataStore.GetValueFromTable(tableEntry, kContents, to);
					bSuccess = bSuccess && dataStore.DeepCopy(jsonDataStore, jsonDataStore.GetRootNode(), to, false);
				}
			}
			// If we successfully read the contents of the archive, process
			// it based on whether it is text or not. In both cases, the
			// contents of the file are inserted as a String into the
			// DataStore, but if the file is binary, it is encoded as
			// Base64.
			else if (bText)
			{
				bSuccess = dataStore.SetStringToTable(tableEntry, kContents, (Byte const*)pData, zSizeInBytes);
			}
			else
			{
				bSuccess = dataStore.SetStringToTable(tableEntry, kContents, Base64Encode(pData, zSizeInBytes));
			}

			// Release memory allocated for the file.
			MemoryManager::Deallocate(pData);
		}

		// If processing the file failed, fail the entire operation.
		if (!bSuccess)
		{
			SEOUL_ERR("failed encoding file body");
			return false;
		}
	}

	// We've now built the entire DataStore, so write it to the output as JSON.
	if (bDiffFriendly)
	{
		// If diff friendly, alphabetize and write out multiline.
		dataStore.ToString(dataStore.GetRootNode(), rsOutput, true, 0, true);
	}
	else
	{
		// If not diff friendly, leave in DataStore order and write out as a single line.
		dataStore.ToString(dataStore.GetRootNode(), rsOutput, false, 0, false);
	}

	// Success.
	return true;
}

/**
 * Generate a .json version of the input .sar
 */
Bool DumpJson(const String& sInputFilename, const String& sOutputFilename)
{
	String sOutput;
	if (!DumpJsonCommon(sInputFilename, sOutputFilename, sOutput))
	{
		return false;
	}

	// If writing fails, the operation fails.
	DiskSyncFile out(sOutputFilename, File::kWriteTruncate);
	if (sOutput.GetSize() != out.WriteRawData(sOutput.CStr(), sOutput.GetSize()))
	{
		SEOUL_ERR("failed writing JSON output file");
		return false;
	}

	return true;
}

/**
 * Generate a .json version of the input .sar and compress the output with GzipCompress (the
 * resulting file can be opened by tools which support the .gz extension).
 */
Bool DumpJsonGz(const String& sInputFilename, const String& sOutputFilename)
{
	String sOutput;
	if (!DumpJsonCommon(sInputFilename, sOutputFilename, sOutput))
	{
		return false;
	}

	// Validate before compressing - make sure we can read the data back in.
	{
		DataStore dataStore;
		if (!DataStoreParser::FromString(sOutput.CStr(), sOutput.GetSize(), dataStore))
		{
			return false;
		}
	}

	// Compress the data.
	void* pCompressedData = nullptr;
	UInt32 zCompressedDataSizeInBytes = 0u;
	if (!GzipCompress(sOutput.CStr(), sOutput.GetSize(), pCompressedData, zCompressedDataSizeInBytes, ZlibCompressionLevel::kBest))
	{
		SEOUL_ERR("failed compressing JSON data");
		return false;
	}

	// If writing fails, the operation fails.
	DiskSyncFile out(sOutputFilename, File::kWriteTruncate);
	Bool const bReturn = (zCompressedDataSizeInBytes == out.WriteRawData(pCompressedData, zCompressedDataSizeInBytes));

	// Free the compression buffer.
	MemoryManager::Deallocate(pCompressedData);

	return bReturn;
}

Bool Extract(const String& sInputFilename, const String& sOutputFilename)
{
	if (!SarToolCommandLineArgs::ToExtract.IsSet())
	{
		SEOUL_ERR("missing required argument '-to_extract'");
		return false;
	}

	// Cache.
	String const sExtract(SarToolCommandLineArgs::ToExtract);

	// Construct the relative path inside the archive.
	auto const archiveFilePath(FilePath::CreateFilePath(s_pPackageFileSystem->GetPackageGameDirectory(), SarToolCommandLineArgs::ToExtract));

	// Check.
	if (!s_pPackageFileSystem->Exists(archiveFilePath))
	{
		SEOUL_ERR("does not contain '%s'", sExtract.CStr());
		return false;
	}

	// If opening the output file fails, fail the operation.
	DiskSyncFile out(sOutputFilename, File::kWriteTruncate);
	if (!out.CanWrite())
	{
		SEOUL_ERR("failed opening output file '%s' for write", sOutputFilename.CStr());
		return false;
	}

	void* pData = nullptr;
	UInt32 uDataSizeInByte = 0u;
	if (!s_pPackageFileSystem->ReadAll(
		archiveFilePath,
		pData,
		uDataSizeInByte,
		0u,
		MemoryBudgets::Io))
	{
		SEOUL_ERR("failed reading data of '%s'", sExtract.CStr());
		return false;
	}

	// Convert to text if a JSON file.
	if (FileType::kJson == archiveFilePath.GetType() &&
		DataStoreParser::IsCookedBinary((Byte const*)pData, uDataSizeInByte))
	{
		FullyBufferedSyncFile memFile(pData, uDataSizeInByte);
		DataStore ds;
		if (!ds.Load(memFile))
		{
			SEOUL_ERR("failed decoding binary JSON of '%s'", sExtract.CStr());
			return false;
		}

		String s;
		ds.ToString(ds.GetRootNode(), s, true, 0, true);
		s.RelinquishBuffer(pData, uDataSizeInByte);
	}

	if (uDataSizeInByte != out.WriteRawData(pData, uDataSizeInByte))
	{
		SEOUL_ERR("failed writing data of '%s' at %u bytes to output file '%s'", sExtract.CStr(), uDataSizeInByte, sOutputFilename.CStr());
		MemoryManager::Deallocate(pData);
		return false;
	}

	MemoryManager::Deallocate(pData);
	return true;
}

Bool List(const String& sInputFilename, const String&)
{
	PackageFileSystem pkg(sInputFilename);
	if (!pkg.IsOk())
	{
		SEOUL_ERR("failed reading package '%s'", sInputFilename.CStr());
		return false;
	}

	typedef Pair<FilePath, PackageFileTableEntry> PairT;
	Vector< PairT > v;
	for (auto const& e : pkg.GetFileTable())
	{
		v.PushBack(MakePair(e.First, e.Second));
	}
	QuickSort(v.Begin(), v.End(), [&](const PairT& a, const PairT& b)
	{
		return ((Int64)a.Second.m_Entry.m_uOffsetToFile) - ((Int64)b.Second.m_Entry.m_uOffsetToFile);
	});

	for (auto const& e : v)
	{
		fprintf(stdout, "%s\n", e.First.GetRelativeFilename().CStr());
	}
	return true;
}

Bool PrintVersion(const String& sInputFilename, const String& /*sOutputFilename*/)
{
	PackageFileHeader header;
	Bool bUnusedEndianSwapped = false;
	if (!ReadPackageHeader(sInputFilename, header, bUnusedEndianSwapped))
	{
		SEOUL_ERR("failed reading package header");
		return false;
	}
	(void)bUnusedEndianSwapped;

	fprintf(stdout, "%u", header.GetBuildVersionMajor());
	return true;
}

Bool PrintChangelist(const String& sInputFilename, const String& /*sOutputFilename*/)
{
	PackageFileHeader header;
	Bool bUnusedEndianSwapped = false;
	if (!ReadPackageHeader(sInputFilename, header, bUnusedEndianSwapped))
	{
		SEOUL_ERR("failed reading package header");
		return false;
	}
	(void)bUnusedEndianSwapped;

	fprintf(stdout, "%u", header.GetBuildChangelist());
	return true;
}

static inline String GetMemoryUsageString(UInt64 uSizeInBytes)
{
	// Handling for difference thresholds.
	if (uSizeInBytes > 1024 * 1024) { return ToString((uSizeInBytes / (1024 * 1024))) + " MBs"; }
	else if (uSizeInBytes > 1024) { return ToString((uSizeInBytes / 1024)) + " KBs"; }
	else { return ToString(uSizeInBytes) + " Bs"; }
}

Bool Stats(const String& sInputFilename, const String&)
{
	PackageFileSystem pkg(sInputFilename);
	if (!pkg.IsOk())
	{
		SEOUL_ERR("failed reading package '%s'", sInputFilename.CStr());
		return false;
	}

	FixedArray<UInt32, (UInt32)FileType::FILE_TYPE_COUNT> aFileCounts;
	FixedArray<UInt64, (UInt32)FileType::FILE_TYPE_COUNT> aFileSizes;

	auto const& t = pkg.GetFileTable();
	fprintf(stdout, "Total files: %u\n", t.GetSize());
	for (auto const& e : t)
	{
		aFileCounts[(UInt32)e.First.GetType()]++;
		aFileSizes[(UInt32)e.First.GetType()] += e.Second.m_Entry.m_uCompressedFileSize;
	}

	for (Int32 i = 0; i < (Int32)FileType::FILE_TYPE_COUNT; ++i)
	{
		auto const u = aFileSizes[i];

		// Skip if 0 size.
		if (0 == u)
		{
			continue;
		}

		fprintf(stdout, "%s: %s (%u)\n",
			EnumToString<FileType>(i),
			GetMemoryUsageString(u).CStr(),
			aFileCounts[i]);
	}

	return true;
}

struct ScopedCore SEOUL_SEALED
{
	ScopedCore()
	{
		// Silence all log channels except for Assertion.
		Logger::GetSingleton().EnableAllChannels(false);
		Logger::GetSingleton().EnableChannel(LoggerChannel::Assertion, true);

		// Initialize Core support.
		CoreSettings settings;
		settings.m_bLoadLoggerConfigurationFile = false;
		settings.m_bOpenLogFile = false;
		Core::Initialize(settings);

		// NOTE: Setting up FileSystems post Core is not the default method,
		// but we do it this way to deliberately prevent Core::Initialize() from
		// accessing files on disk. This is safe ONLY because we know there is
		// no code running in other threads that may try to use the Seoul FileSystem.

		// Register a package file system if defined.
		if (!s_sPackageFileSystemFilename.IsEmpty())
		{
			s_pPackageFileSystem = FileManager::Get()->RegisterFileSystem<PackageFileSystem>(s_sPackageFileSystemFilename);
		}

		// Register a default disk file system.
		FileManager::Get()->RegisterFileSystem<DiskFileSystem>();
	}

	~ScopedCore()
	{
		// Reset s_pPackageFileSystem
		s_pPackageFileSystem.Reset();

		// Shutdown Core handling.
		Core::ShutDown();

		// Clear the callback.
		g_pInitializeFileSystemsCallback = nullptr;
	}
}; // class ScopedCore

/**
 * Attempt to generate an absolute path to a temporary file.
 * If successful, rsFilename will contain the path and this method
 * returns true. Otherwise, this method returns false and rsFilename
 * is left unchanged.
 */
static Bool GetAbsoluteTempFilename(String& rsFilename)
{
	Byte aPathBuffer[MAX_PATH];
	Byte aFilenameBuffer[MAX_PATH];
	DWORD zSizeInCharacters = ::GetTempPathA(MAX_PATH, aPathBuffer);
	if (0u == zSizeInCharacters || zSizeInCharacters > MAX_PATH)
	{
		return false;
	}
	else
	{
		if (::GetTempFileNameA(aPathBuffer, "SEOUL_TEMP_FILE_", 0u, aFilenameBuffer) > 0u)
		{
			rsFilename = Path::Normalize(String(aFilenameBuffer));
			return true;
		}
		else
		{
			return false;
		}
	}
}

/**
 * Utility to split a command-line argument into a key-value pair.
 */
Bool GetKeyValuePair(Byte const* sArgument, String& rsKey, String& rsValue)
{
	// All arguments are of the form -key or -key=value
	if (nullptr == sArgument || sArgument[0] != '-')
	{
		return false;
	}

	// Skip the leading '-'
	sArgument++;

	Byte const* const sDelimiter = strchr(sArgument, '=');
	if (nullptr == sDelimiter)
	{
		rsKey.Assign(String(sArgument).ToLowerASCII());
		rsValue.Clear();
	}
	else
	{
		rsKey.Assign(String(sArgument, (UInt)(sDelimiter - sArgument)).ToLowerASCII());
		rsValue.Assign(sDelimiter + 1);
	}

	return true;
}

static const SarToolCommandDesc kaCommands[] =
{
	{ nullptr,         false, false },
	{ DumpJson,        true,  true  },
	{ DumpJsonGz,      true,  true  },
	{ Extract,         true,  true  },
	{ List,            false, false },
	{ PrintChangelist, false, false },
	{ PrintVersion,    false, false },
	{ Stats,           false, false },
};

} // namespace Seoul

int main(int iArgC, char** ppArgV)
{
	using namespace Seoul;

	// Disable message boxes on failed assertions.
	g_bHeadless = true;
	g_bShowMessageBoxesOnFailedAssertions = false;
	g_bEnableMessageBoxes = false;

	// Parse command-line.
	if (!Reflection::CommandLineArgs::Parse(ppArgV + 1, ppArgV + iArgC))
	{
		return 1;
	}

	// Check argument.
	if (SarToolCommand::None == SarToolCommandLineArgs::Command ||
		(Int)SarToolCommandLineArgs::Command < 0 ||
		(size_t)SarToolCommandLineArgs::Command >= SEOUL_ARRAY_COUNT(kaCommands))
	{
		SEOUL_ERR("unknown command '%s'", EnumToString<SarToolCommand>(SarToolCommandLineArgs::Command));
		return 1;
	}

	// Mark that we're now in the main function.
	auto const inMain(MakeScopedAction(&BeginMainFunction, &EndMainFunction));

	// Setup the main thread ID.
	SetMainThreadId(Thread::GetThisThreadId());

	// Get the command.
	auto const& cmd = kaCommands[(Int)SarToolCommandLineArgs::Command];

	// Additional handling.
	if (cmd.m_bNeedsOutput && SarToolCommandLineArgs::Output.IsEmpty())
	{
		SEOUL_ERR("missing required argument '-o'");
		return 1;
	}

	// Get the current directory.
	String sCurrentDirectory;
	{
		DWORD uLength = ::GetCurrentDirectoryW(0u, nullptr);
		if (0u == uLength)
		{
			SEOUL_ERR("failed getting current directory path");
			return 1;
		}

		Vector<WCHAR> v(uLength);
		if (0u == ::GetCurrentDirectoryW(uLength, v.Get(0u)))
		{
			SEOUL_ERR("failed getting current directory path");
			return 1;
		}

		sCurrentDirectory = WCharTToUTF8(v.Get(0u));
	}

	// Normalize filenames.
	{
		String const sPathA = (Path::IsRooted(SarToolCommandLineArgs::Input) ? String() : sCurrentDirectory);
		if (!Path::CombineAndSimplify(
			sPathA,
			SarToolCommandLineArgs::Input,
			SarToolCommandLineArgs::Input))
		{
			SEOUL_ERR("bad path: '%s'", SarToolCommandLineArgs::Input.CStr());
			return 1;
		}
	}
	{
		String const sPathA = (Path::IsRooted(SarToolCommandLineArgs::Output) ? String() : sCurrentDirectory);
		if (!Path::CombineAndSimplify(
			sPathA,
			SarToolCommandLineArgs::Output,
			SarToolCommandLineArgs::Output))
		{
			SEOUL_ERR("bad path: '%s'", SarToolCommandLineArgs::Output.CStr());
			return 1;
		}
	}

	String sTemporaryFilename;
	if (cmd.m_bNeedsOutput)
	{
		if (!GetAbsoluteTempFilename(sTemporaryFilename))
		{
			SEOUL_ERR("failed generating temp filename");
			return 1;
		}
	}

	// Set the package filename if needed
	if (cmd.m_bNeedsInputAsFileSystem)
	{
		s_sPackageFileSystemFilename = SarToolCommandLineArgs::Input;
	}

	// Initialize core - must be done after setting up startup variables, so
	// core can hookup the input .sar as a file system, if needed.
	ScopedCore core;

	// Check package before continuing.
	{
		PackageFileSystem pkg(SarToolCommandLineArgs::Input);
		if (!pkg.IsOk())
		{
			SEOUL_ERR("package is corrupt: %s", pkg.GetLoadError().CStr());
			return 1;
		}
	}

	Bool bSuccess = cmd.m_Function(SarToolCommandLineArgs::Input, sTemporaryFilename);

	if (cmd.m_bNeedsOutput)
	{
		// Create the directory structure for the output file.
		if (bSuccess && !Directory::CreateDirPath(Path::GetDirectoryName(SarToolCommandLineArgs::Output)))
		{
			SEOUL_ERR("failed creating dependent directories for output file");
			bSuccess = false;
		}

		// If successful, attempt to copy the file to the destination.
		if (bSuccess && 0 == ::CopyFileW(
			sTemporaryFilename.WStr(),
			SarToolCommandLineArgs::Output.WStr(),
			FALSE))
		{
			SEOUL_ERR("failed writing output file");
			bSuccess = false;
		}

		// Delete the temp file in all cases.
		(void)::DeleteFileW(sTemporaryFilename.WStr());
	}

	return (bSuccess ? 0 : 1);
}
