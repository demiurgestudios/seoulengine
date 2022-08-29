/**
 * \file Main.cpp
 * \brief JsonFormatter is used to pretty print and "flatten" .json files
 * used by SeoulEngine. In particular, it can convert files in SeoulEngine's
 * "commands" syntax into flattened .json files.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "DataStore.h"
#include "DataStoreParser.h"
#include "Directory.h"
#include "DiskFileSystem.h"
#include "Path.h"
#include "ReflectionCommandLineArgs.h"
#include "ReflectionDefine.h"
#include "ReflectionScriptStub.h"
#include "ScopedAction.h"
#include "SeoulFile.h"
#include "SeoulUtil.h"

#define SEOUL_ERR(fmt, ...) fprintf(stderr, fmt "\n", ##__VA_ARGS__)

namespace Seoul
{

/**
 * Root level command-line arguments - handled by reflection, can be
 * configured via the literal command-line, environment variables, or
 * a configuration file.
 */
class JsonFormatterCommandLineArgs SEOUL_SEALED
{
public:
	static String FilenameOrDirectory;
	static String AppendFile;
	static String Oldest;
	static Bool Recursive;
	static Bool Flatten;

private:
	JsonFormatterCommandLineArgs() = delete; // Static class.
	SEOUL_DISABLE_COPY(JsonFormatterCommandLineArgs);
};

String JsonFormatterCommandLineArgs::FilenameOrDirectory{};
String JsonFormatterCommandLineArgs::AppendFile{};
String JsonFormatterCommandLineArgs::Oldest{};
Bool JsonFormatterCommandLineArgs::Recursive{};
Bool JsonFormatterCommandLineArgs::Flatten{};

SEOUL_BEGIN_TYPE(JsonFormatterCommandLineArgs, TypeFlags::kDisableNew | TypeFlags::kDisableCopy)
	SEOUL_CMDLINE_PROPERTY(FilenameOrDirectory, 0, "file_or_dir", true)
	SEOUL_CMDLINE_PROPERTY(AppendFile, "append", "filename")
		SEOUL_ATTRIBUTE(Description, "Append JSON to JSON, see remarks")
		SEOUL_ATTRIBUTE(Remarks,
			"The --append argument is only valid in single file "
			"mode passing this argument when running on a directory is "
			"invalid). When specified, the contents of the append file "
			"will be appended to the target file and the append file  will "
			"be deleted.")
	SEOUL_CMDLINE_PROPERTY(Oldest, "oldest", "filename")
		SEOUL_ATTRIBUTE(Description, "Defines the oldest file, see remarks")
		SEOUL_ATTRIBUTE(Remarks,
			"For directory processing, the mod time "
			"of the file specified to --oldest will "
			"be used as a baseline. Only writable files "
			"at or newer than the mod time file will be "
			"processed.")
	SEOUL_CMDLINE_PROPERTY(Recursive, "r")
		SEOUL_ATTRIBUTE(Description, "When run on a directory, should also traverse subdirectories")
	SEOUL_CMDLINE_PROPERTY(Flatten, "flat")
		SEOUL_ATTRIBUTE(Description, R"(Except for files with ["$includes", ...], JSON commands are flattened)")
SEOUL_END_TYPE()

Bool IsReadOnly(const String& sFilename);

// Use default core virtuals
const CoreVirtuals* g_pCoreVirtuals = &g_DefaultCoreVirtuals;

namespace
{

struct Args
{
	String m_sFilenameOrDirectory{};
	String m_sAppendFile{};
	UInt64 m_uOldest{};
	Bool m_bRecursive{};
	Bool m_bFlatten{};
};

// We don't flatten files that have $include directives,
// so our include resolver records the encounter and fails,
// which will then cause the root to ignore the processing.
struct IncludeTracker
{
	SEOUL_DELEGATE_TARGET(IncludeTracker);

	Bool m_bIncluded = false;

	SharedPtr<DataStore> Resolve(const String& sFileName, Bool bResolveCommands)
	{
		m_bIncluded = true;
		return SharedPtr<DataStore>();
	}
};

} // namespace anonymous

static Bool FlattenIfNeeded(const String& sFilename, DataStore& r, Bool& rbDidFlatten)
{
	// Don't need to flatten if not JSON commands.
	if (!DataStoreParser::IsJsonCommandFile(r))
	{
		rbDidFlatten = false;
		return true;
	}

	// Includes cause the flatten to be cancelled.
	DataStore ds;
	IncludeTracker tracker;
	if (!DataStoreParser::ResolveCommandFile(
		SEOUL_BIND_DELEGATE(&IncludeTracker::Resolve, &tracker),
		sFilename,
		r,
		ds,
		DataStoreParserFlags::kLeaveFilePathAsString))
	{
		// Encountered an include, ignore and don't flatten.
		if (tracker.m_bIncluded)
		{
			rbDidFlatten = false;
			return true;
		}
		else
		{
			SEOUL_ERR("Could not flatten commands in '%s'", sFilename.CStr());
			return false;
		}
	}

	// Done, success.
	rbDidFlatten = true;
	r.Swap(ds);
	return true;
}

SharedPtr<DataStore> IncludeResolver(const String& sFileName, Bool bResolveCommands)
{
	// Read the data.
	DataStore ds;
	{
		// Read the existing file.
		void* pIn = nullptr;
		UInt32 uIn = 0u;
		if (!DiskSyncFile::ReadAll(sFileName, pIn, uIn, 0u, MemoryBudgets::Cooking))
		{
			return SharedPtr<DataStore>();
		}
		auto const scoped(MakeScopedAction([]() {}, [&]() { MemoryManager::Deallocate(pIn); pIn = nullptr; uIn = 0u; }));

		if (!DataStoreParser::FromString((Byte const*)pIn, uIn, ds, DataStoreParserFlags::kLeaveFilePathAsString | DataStoreParserFlags::kLogParseErrors))
		{
			return SharedPtr<DataStore>();
		}
	}

	// If requested, resolve the commands.
	if (bResolveCommands && DataStoreParser::IsJsonCommandFile(ds))
	{
		if (!DataStoreParser::ResolveCommandFile(
			SEOUL_BIND_DELEGATE(&IncludeResolver),
			sFileName,
			ds,
			ds))
		{
			return SharedPtr<DataStore>();
		}
	}

	SharedPtr<DataStore> p(SEOUL_NEW(MemoryBudgets::Io) DataStore);
	p->Swap(ds);
	return p;
}

static Bool AppendToJson(
	DataStore& inputOutput,
	const String& sAppendFilename)
{
	// Parse the append file.
	DataStore chunk;
	{
		// Read the existing file.
		void* pIn = nullptr;
		UInt32 uIn = 0u;
		if (!DiskSyncFile::ReadAll(sAppendFilename, pIn, uIn, 0u, MemoryBudgets::Cooking))
		{
			SEOUL_ERR("Failed reading append file '%s' for append operation.", sAppendFilename.CStr());
			return false;
		}
		auto const scoped(MakeScopedAction([]() {}, [&]() { MemoryManager::Deallocate(pIn); pIn = nullptr; uIn = 0u; }));

		if (!DataStoreParser::FromString((Byte const*)pIn, uIn, chunk, DataStoreParserFlags::kLeaveFilePathAsString | DataStoreParserFlags::kLogParseErrors))
		{
			SEOUL_ERR("Failed parsing append file '%s' for append operation.", sAppendFilename.CStr());
			return false;
		}
	}

	// Chunk must be a commands file.
	if (!DataStoreParser::IsJsonCommandFile(chunk))
	{
		SEOUL_ERR("Append file is not a JSON commands format file.");
		return false;
	}

	// If target is a commands file, then we just append the chunk to that file.
	if (DataStoreParser::IsJsonCommandFile(inputOutput))
	{
		// Append commands to existing commands array.
		UInt32 uExistingCommands = 0u;
		if (!inputOutput.GetArrayCount(inputOutput.GetRootNode(), uExistingCommands))
		{
			SEOUL_ERR("Append operation failure, likely invalid base file structure");
			return false;
		}

		UInt32 uNewCommands = 0u;
		if (!chunk.GetArrayCount(chunk.GetRootNode(), uNewCommands))
		{
			SEOUL_ERR("Append operation failure, likely invalid append file structure");
			return false;
		}

		for (UInt32 i = 0u; i < uNewCommands; ++i)
		{
			DataNode arrayElem;
			if (!chunk.GetValueFromArray(chunk.GetRootNode(), i, arrayElem))
			{
				SEOUL_ERR("Append operation failure, likely invalid append file structure");
				return false;
			}

			if (!inputOutput.DeepCopyToArray(chunk, arrayElem, inputOutput.GetRootNode(), i + uExistingCommands))
			{
				SEOUL_ERR("Append operation failure, invalid append or base file structure");
				return false;
			}
		}
	}
	// Otherwise, we apply it to that file "in place". The initial state is
	// the initial state of the data store and we apply any appended commands
	// to that state.
	else
	{
		DataNode target = inputOutput.GetRootNode();
		if (!DataStoreParser::ResolveCommandFileInPlace(
			SEOUL_BIND_DELEGATE(IncludeResolver),
			sAppendFilename,
			chunk,
			inputOutput,
			target))
		{
			return false;
		}
	}

	// Done.
	return true;
}

// Post process (clean) a .json file emitted from a spreadsheet. Operation:
// - check if the file is writable. Skip files which are read-only.
// - read the file. Use it as its own hinting for formatting.
// - pretty print it.
// - (atomically) overwrite the file.
static Bool Format(const String& sFilename, Bool bFlattenJsonCommands, String sAppendFilename = String())
{
	String sOut;
	{
		// Read the existing file.
		void* pIn = nullptr;
		UInt32 uIn = 0u;
		if (!DiskSyncFile::ReadAll(sFilename, pIn, uIn, 0u, MemoryBudgets::Cooking))
		{
			SEOUL_ERR("Failed reading '%s' to format.", sFilename.CStr());
			return false;
		}
		auto const scoped(MakeScopedAction([]() {}, [&]() { MemoryManager::Deallocate(pIn); pIn = nullptr; uIn = 0u; }));

		// Parse the input data to save.
		DataStore ds;
		if (!DataStoreParser::FromString((Byte const*)pIn, uIn, ds, DataStoreParserFlags::kLeaveFilePathAsString | DataStoreParserFlags::kLogParseErrors))
		{
			SEOUL_ERR("Parse error reading existing '%s', cannot format.", sFilename.CStr());
			return false;
		}

		// If not empty, perform the append now.
		if (!sAppendFilename.IsEmpty())
		{
			if (!AppendToJson(ds, sAppendFilename))
			{
				return false;
			}
		}

		// Try to flatten - may do nothing if the file is already flat.
		Bool bDidFlatten = false;
		if (bFlattenJsonCommands)
		{
			if (!FlattenIfNeeded(sFilename, ds, bDidFlatten))
			{
				return false;
			}
		}

		// Derive hinting from existing file.
		SharedPtr<DataStoreHint> pHint;
		if (bDidFlatten)
		{
			if (!DataStorePrinter::ParseHintsNoCopyWithFlattening((Byte const*)pIn, uIn, pHint))
			{
				SEOUL_ERR("failed parsing '%s' for fattened hinting, cannot format.", sFilename.CStr());
				return false;
			}
		}
		else
		{
			if (!DataStorePrinter::ParseHintsNoCopy((Byte const*)pIn, uIn, pHint))
			{
				SEOUL_ERR("failed parsing '%s' for hinting, cannot format.", sFilename.CStr());
				return false;
			}
		}

		// Must be non-null - use a placeholder if no hinting was available.
		if (!pHint.IsValid())
		{
			pHint.Reset(SEOUL_NEW(MemoryBudgets::DataStore) DataStoreHintNone);
		}

		// Pretty print with DataStorePrinter.
		DataStorePrinter::PrintWithHints(ds, pHint, sOut);
	}

	// Atomic commit - move existing file, write new file, delete old file.
	{
		// Prepare.
		auto const sBak(sFilename + ".bak");
		auto const sTmp(sFilename + ".tmp");

		(void)DiskSyncFile::DeleteFile(sBak);
		(void)DiskSyncFile::DeleteFile(sTmp);

		// Perform the write.
		Bool bSuccess = false;
		{
			DiskSyncFile file(sTmp, File::kWriteTruncate);
			bSuccess = (sOut.GetSize() == file.WriteRawData(sOut.CStr(), sOut.GetSize()));
		}

		// Move existing to backup.
		bSuccess = bSuccess && DiskSyncFile::RenameFile(sFilename, sBak);
		// Move tmp to slot.
		bSuccess = bSuccess && DiskSyncFile::RenameFile(sTmp, sFilename);
		// Delete backup.
		bSuccess = bSuccess && DiskSyncFile::DeleteFile(sBak);

		if (!bSuccess)
		{
			// Attempt to restore if we have a backup file.
			if (DiskSyncFile::FileExists(sBak))
			{
				(void)DiskSyncFile::DeleteFile(sFilename);
				(void)DiskSyncFile::RenameFile(sBak, sFilename);
			}

			// Cleanup.
			(void)DiskSyncFile::DeleteFile(sTmp);

			// Done.
			SEOUL_ERR("Failed commiting pretty printed output to temporary file '%s'", sTmp.CStr());
			return false;
		}
	}

	// Done.
	return true;
}

static Bool FormatDirectory(const Args& args)
{
	// Enumerate .json files in the directory and process them. Not recursive.
	Vector<String> vs;
	if (!Directory::GetDirectoryListing(args.m_sFilenameOrDirectory, vs, false, args.m_bRecursive, ".json"))
	{
		SEOUL_ERR("Failed enumerating directory '%s', cannot format.", args.m_sFilenameOrDirectory.CStr());
		return 1;
	}

	Bool bSuccess = true;
	for (auto const& sFilename : vs)
	{
		// Skip read-only files.
		if (IsReadOnly(sFilename)) { continue; }

		// Check mod time.
		if (0 != args.m_uOldest)
		{
			auto const u = DiskSyncFile::GetModifiedTime(sFilename);
			if (u < args.m_uOldest)
			{
				continue;
			}
		}

		bSuccess = Format(sFilename, args.m_bFlatten) && bSuccess;
	}

	return bSuccess;
}

static Bool GetCommandLineArgs(Int iArgC, Byte** ppArgV, Args& rArgs)
{
	if (!Reflection::CommandLineArgs::Parse(ppArgV + 1, ppArgV + iArgC))
	{
		return false;
	}

	rArgs.m_sFilenameOrDirectory = Path::GetExactPathName(JsonFormatterCommandLineArgs::FilenameOrDirectory);
	rArgs.m_bRecursive = JsonFormatterCommandLineArgs::Recursive;
	rArgs.m_bFlatten = JsonFormatterCommandLineArgs::Flatten;

	// Sanity check requirements around append.
	if (!JsonFormatterCommandLineArgs::AppendFile.IsEmpty())
	{
		// Oldest invalid if --append specified.
		if (!JsonFormatterCommandLineArgs::Oldest.IsEmpty())
		{
			SEOUL_ERR("--oldest is invalid when --append is provided");
			return false;
		}

		// Recursive invalid if --append specified.
		if (rArgs.m_bRecursive)
		{
			SEOUL_ERR("-r is invalid when --append is provided");
			return false;
		}

		// Cannot run on directories with --append.
		if (Directory::DirectoryExists(rArgs.m_sFilenameOrDirectory))
		{
			SEOUL_ERR("--append cannot be provided when running on a directory (single file mode only).");
			return false;
		}

		// Assign and resolve, then check that it exists.
		rArgs.m_sAppendFile = Path::GetExactPathName(Path::Combine(Path::GetDirectoryName(rArgs.m_sFilenameOrDirectory), JsonFormatterCommandLineArgs::AppendFile));
		if (!DiskSyncFile::FileExists(rArgs.m_sAppendFile))
		{
			SEOUL_ERR("--append file '%s' does not exist", rArgs.m_sAppendFile.CStr());
			return false;
		}
	}

	// Resolve oldest and check it.
	if (!JsonFormatterCommandLineArgs::Oldest.IsEmpty())
	{
		auto const sOldest = Path::GetExactPathName(Path::Combine(rArgs.m_sFilenameOrDirectory, JsonFormatterCommandLineArgs::Oldest));
		rArgs.m_uOldest = DiskSyncFile::GetModifiedTime(sOldest);
		if (0 == rArgs.m_uOldest)
		{
			SEOUL_ERR("Failed checking modified time of --oldest file '%s'", sOldest.CStr());
			return false;
		}
	}

	// Done success.
	return true;
}

} // namespace Seoul

int main(int iArgC, char** ppArgV)
{
	using namespace Seoul;

	Args args;
	if (!GetCommandLineArgs(iArgC, ppArgV, args))
	{
		return 1;
	}

	// Directory or single file processing.
	if (Directory::DirectoryExists(args.m_sFilenameOrDirectory))
	{
		if (!FormatDirectory(args))
		{
			return 1;
		}
	}
	else
	{
		// If we get here, check for existence of the output file. If it does not exist,
		// but the append file *does* exist, move the append file to the output file
		// and continue as if no append.
		if (!DiskSyncFile::FileExists(args.m_sFilenameOrDirectory) && !args.m_sAppendFile.IsEmpty() &&
			DiskSyncFile::FileExists(args.m_sAppendFile))
		{
			if (!DiskSyncFile::RenameFile(args.m_sAppendFile, args.m_sFilenameOrDirectory))
			{
				SEOUL_ERR("--append failure, could not rename '%s' to '%s'", args.m_sAppendFile.CStr(), args.m_sFilenameOrDirectory.CStr());
				return 1;
			}

			// Blank out append, normal operation now.
			args.m_sAppendFile = String();
		}

		if (!Format(args.m_sFilenameOrDirectory, args.m_bFlatten, args.m_sAppendFile))
		{
			return 1;
		}
	}

	// If we get here and an append file was specified, delete it.
	if (!args.m_sAppendFile.IsEmpty() && DiskSyncFile::FileExists(args.m_sAppendFile))
	{
		if (!DiskSyncFile::DeleteFile(args.m_sAppendFile))
		{
			SEOUL_ERR("failed deleting append file '%s'", args.m_sAppendFile.CStr());
			return 1;
		}
	}

	return 0;
}
