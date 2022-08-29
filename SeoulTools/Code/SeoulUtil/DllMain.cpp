/**
 * \file DllMain.cpp
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Compress.h"
#include "CookDatabase.h"
#include "Core.h"
#include "CoreSettings.h"
#include "DataStore.h"
#include "DataStoreParser.h"
#include "DiskFileSystem.h"
#include "FileManager.h"
#include "GamePaths.h"
#include "Logger.h"
#include "PackageFileSystem.h"
#include "Platform.h"
#include "ReflectionScriptStub.h"
#include "SeoulCrc32.h"
#include "SeoulFile.h"
#include "StringUtil.h"
#include "SeoulUtil.h"
#include "Thread.h"
#include "ThreadId.h"

namespace Seoul
{

// Use default core virtuals
const CoreVirtuals* g_pCoreVirtuals = &g_DefaultCoreVirtuals;

namespace
{

static Bool ReadDataStore(const String& sFileName, DataStore& ds, UInt32 uFlags = 0u)
{
	{
		void* pFile = nullptr;
		UInt32 uFile = 0u;
		if (!DiskSyncFile::ReadAll(sFileName, pFile, uFile, 0u, MemoryBudgets::TBD))
		{
			return false;
		}

		// Parse the input file.
		auto const bOk = DataStoreParser::FromString((Byte const*)pFile, uFile, ds, uFlags);
		MemoryManager::Deallocate(pFile);
		if (!bOk)
		{
			return false;
		}
	}

	return true;
}

template <UInt32 uFlags = 0u>
SharedPtr<DataStore> IncludeResolver(const String& sFileName, Bool bResolveCommands)
{
	// Read the data.
	DataStore ds;
	if (!ReadDataStore(sFileName, ds, uFlags))
	{
		return SharedPtr<DataStore>();
	}

	// If requested, resolve the commands.
	if (bResolveCommands && DataStoreParser::IsJsonCommandFile(ds))
	{
		if (!DataStoreParser::ResolveCommandFile(
			SEOUL_BIND_DELEGATE(&IncludeResolver<uFlags>),
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

static Bool AppendToJson(
	Byte const* sInOutFilename,
	void const* pIn,
	UInt32 uIn,
	String& rs,
	UInt32 uFlags = 0u)
{
	DataStore inputOutput;
	if (!ReadDataStore(sInOutFilename, inputOutput, uFlags))
	{
		fprintf(stderr, "Failed reading input file '%s' to process.\n", sInOutFilename);
		return false;
	}

	// Parse the chunk.
	DataStore chunk;
	if (!DataStoreParser::FromString((Byte const*)pIn, uIn, chunk, DataStoreParserFlags::kLogParseErrors | uFlags))
	{
		return false;
	}

	// Chunk must be a commands file.
	if (!DataStoreParser::IsJsonCommandFile(chunk))
	{
		fprintf(stderr, "Input chunk is not a JSON commands chunk.\n");
		return false;
	}

	// If target is a commands file, then we just append the chunk to that file.
	if (DataStoreParser::IsJsonCommandFile(inputOutput))
	{
		// Append commands to existing commands array.
		UInt32 uExistingCommands = 0u;
		if (!inputOutput.GetArrayCount(inputOutput.GetRootNode(), uExistingCommands))
		{
			return false;
		}

		UInt32 uNewCommands = 0u;
		if (!chunk.GetArrayCount(chunk.GetRootNode(), uNewCommands))
		{
			return false;
		}

		for (UInt32 i = 0u; i < uNewCommands; ++i)
		{
			DataNode arrayElem;
			if (!chunk.GetValueFromArray(chunk.GetRootNode(), i, arrayElem))
			{
				return false;
			}

			if (!inputOutput.DeepCopyToArray(chunk, arrayElem, inputOutput.GetRootNode(), i + uExistingCommands))
			{
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
			SEOUL_BIND_DELEGATE(IncludeResolver<0u>),
			sInOutFilename,
			chunk,
			inputOutput,
			target))
		{
			return false;
		}
	}

	// Done, save the file.
	inputOutput.ToString(inputOutput.GetRootNode(), rs, true, 0, true);
	return true;
}

static Bool AppendToJson(
	Byte const* sInOutFilename,
	void const* pIn,
	UInt32 uIn)
{
	String sOutput;
	if (!AppendToJson(sInOutFilename, pIn, uIn, sOutput))
	{
		return false;
	}

	// Write.
	{
		DiskSyncFile file(sInOutFilename, File::kWriteTruncate);
		if (sOutput.GetSize() != file.WriteRawData(sOutput.CStr(), sOutput.GetSize()))
		{
			return false;
		}
	}

	return true;
}

static Bool CookJson(
	void const* pIn,
	UInt32 uIn,
	Platform ePlatform,
	void*& rpOut,
	UInt32& ruOut)
{
	DataStore ds;
	if (!DataStoreParser::FromString((Byte const*)pIn, uIn, ds, DataStoreParserFlags::kLogParseErrors))
	{
		return false;
	}

	MemorySyncFile file;
	if (!ds.Save(file, ePlatform, true))
	{
		return false;
	}

	UInt32 uOut = 0u;
	file.GetBuffer().RelinquishBuffer(rpOut, uOut);
	ruOut = (UInt32)uOut;
	return true;
}

static Bool MinifyJson(
	void const* pIn,
	UInt32 uIn,
	void*& rpOut,
	UInt32& ruOut)
{
	DataStore ds;
	if (!DataStoreParser::FromString((Byte const*)pIn, uIn, ds, DataStoreParserFlags::kLogParseErrors))
	{
		return false;
	}

	String s;
	ds.ToString(ds.GetRootNode(), s, false, 0, true);
	s.RelinquishBuffer(rpOut, ruOut);
	return true;
}

static bool GetModifiedTimeOfFileInSar(
	const char* sSarPath,
	const char* sFilePath,
	uint64_t* puModifiedTime)
{
	PackageFileSystem pkg(sSarPath);
	if (!pkg.IsOk())
	{
		return false;
	}

	FilePath filePath;
	if (!DataStoreParser::StringAsFilePath(sFilePath, filePath))
	{
		return false;
	}

	if (!pkg.GetModifiedTime(filePath, *puModifiedTime))
	{
		return false;
	}

	return true;
}

static Bool FlattenIfNeeded(DataStore& r, Bool& rbDidFlatten)
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
		String(),
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
			fprintf(stderr, "Could not flatten commands.\n");
			return false;
		}
	}

	// Done, success.
	rbDidFlatten = true;
	r.Swap(ds);
	return true;
}

/**
 * Utility for external applications to format a JSON file
 * and (optionally) resolve command lists into a flat file.
 */
static Bool ExternalFormatJson(
	void const* pIn,
	UInt32 uIn,
	Bool bResolveJsonCommands,
	void*& rpOut,
	UInt32& ruOut)
{
	String sOut;
	{
		// Parse the input data.
		DataStore ds;
		if (!DataStoreParser::FromString((Byte const*)pIn, uIn, ds, DataStoreParserFlags::kLeaveFilePathAsString | DataStoreParserFlags::kLogParseErrors))
		{
			fprintf(stderr, "Parse error reading existing, cannot format.\n");
			return false;
		}

		// Try to flatten - may do nothing if the file is already flat.
		Bool bDidFlatten = false;
		if (bResolveJsonCommands)
		{
			if (!FlattenIfNeeded(ds, bDidFlatten))
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
				fprintf(stderr, "failed parsing for fattened hinting, cannot format.\n");
				return false;
			}
		}
		else
		{
			if (!DataStorePrinter::ParseHintsNoCopy((Byte const*)pIn, uIn, pHint))
			{
				fprintf(stderr, "failed parsing for hinting, cannot format.\n");
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

	// Done.
	sOut.RelinquishBuffer(rpOut, ruOut);
	return true;
}

/**
 * Utility for external applications to load
 * JSON - since it is string-to-string, this is effectively
 * a JSON minifier (removes comments, trailing commas, etc.)
 * coupled with (optional) functionality to resolve SeoulEngine
 * specific JSON "command lists".
 */
static Bool ExternalLoadJsonFile(
	const char* sJsonFile,
	Bool bResolveJsonCommands,
	void*& rpOut,
	UInt32& ruOut)
{
	// Load from disk.
	DataStore ds;
	if (!ReadDataStore(sJsonFile, ds, DataStoreParserFlags::kLogParseErrors | DataStoreParserFlags::kLeaveFilePathAsString))
	{
		fprintf(stderr, "Can't load, failed reading input file '%s'.\n", sJsonFile);
		return false;
	}

	// If a commands syntax DataStore, and requested
	// to do so, resolve.
	if (bResolveJsonCommands && DataStoreParser::IsJsonCommandFile(ds))
	{
		if (!DataStoreParser::ResolveCommandFile(
			SEOUL_BIND_DELEGATE(IncludeResolver<DataStoreParserFlags::kLogParseErrors | DataStoreParserFlags::kLeaveFilePathAsString>),
			sJsonFile,
			ds,
			ds))
		{
			fprintf(stderr, "Failed resolving json commands of file '%s'.\n", sJsonFile);
			return false;
		}
	}

	// Convert to a string and return.
	String s;
	ds.ToString(ds.GetRootNode(), s, false, 0, true);
	s.RelinquishBuffer(rpOut, ruOut);
	return true;
}

/**
 * Given a JSON blob, commit it to disk. If bUseExistingForHinting is true,
 * then an existing file on disk will be read into hinting data (includes
 * information about comments, table key order, etc.) in order to maintain
 * key elements of the existing file formatting.
 */
static Bool ExternalSaveJsonFile(
	void const* pIn,
	UInt32 uIn,
	Bool bUseExistingForHinting,
	Byte const* sOutputFilename)
{
	// Parse the input data to save.
	DataStore ds;
	if (!DataStoreParser::FromString((Byte const*)pIn, uIn, ds, DataStoreParserFlags::kLeaveFilePathAsString | DataStoreParserFlags::kLogParseErrors))
	{
		fprintf(stderr, "Cannot save, failed parsing input JSON.\n");
		return false;
	}

	// If bUseExistingForHinting is true, we check for an existing file at bUseExistingForHinting,
	// and if present, we parse it into a DataStore that includes formatting and comment hints that
	// can be used for printing.
	void* pHinting = nullptr; // Must remain valid as long as p is valid.
	UInt32 uHinting = 0u;
	SharedPtr<DataStoreHint> p;
	if (bUseExistingForHinting)
	{
		// Check for existence of file on disk.
		String s(sOutputFilename);
		if (DiskSyncFile::FileExists(s))
		{
			// Read.
			if (!DiskSyncFile::ReadAll(s, pHinting, uHinting, 0u, MemoryBudgets::TBD))
			{
				fprintf(stderr, "failed reading '%s' for hinting.\n", sOutputFilename);
				return false;
			}

			// Parse the input file for hints.
			if (!DataStorePrinter::ParseHintsNoCopy((Byte const*)pHinting, uHinting, p))
			{
				fprintf(stderr, "failed parsing '%s' for hinting.\n", sOutputFilename);
				return false;
			}
		}
	}

	// Must be non-null - use a placeholder if no hinting was available.
	if (!p.IsValid())
	{
		p.Reset(SEOUL_NEW(MemoryBudgets::DataStore) DataStoreHintNone);
	}

	// Pretty print with DataStorePrinter.
	String s;
	DataStorePrinter::PrintWithHints(ds, p, s);

	// Now we can release the underlying hint data if defined. As mentioned
	// above, pHinting must remain defined until the root hint data node has
	// been released (due to comments pointing into the original buffer).
	p.Reset();
	MemoryManager::Deallocate(pHinting);
	pHinting = nullptr;
	uHinting = 0u;

	// Commit the printed data to disk.
	Bool bReturn = false;
	{
		DiskSyncFile file(sOutputFilename, File::kWriteTruncate);
		bReturn = (s.GetSize() == file.WriteRawData(s.CStr(), s.GetSize()));
	}

	return bReturn;
}

static void ReleaseJson(void* p)
{
	MemoryManager::Deallocate(p);
}

///////////////////////////////////////////////////////////////////////////////
// Originally part of NativeLibUtil
///////////////////////////////////////////////////////////////////////////////
static Mutex s_Mutex;
static Atomic32Value<Bool> s_bCoreInit(false);

static void OnInitializeFileSystems()
{
	FileManager::Get()->RegisterFileSystem<DiskFileSystem>();
}

static Bool CookDatabaseCheckUpToDate(
	void* p,
	const char* sFilename)
{
	auto const filePath = FilePath::CreateContentFilePath(sFilename);
	CookDatabase* pDatabase = (CookDatabase*)p;
	return pDatabase->CheckUpToDate(filePath);
}

static CookDatabase* CookDatabaseCreate(
	Platform ePlatform)
{
	return SEOUL_NEW(Seoul::MemoryBudgets::Cooking) CookDatabase(ePlatform, true);
}

static void CookDatabaseRelease(void* p)
{
	CookDatabase* pDatabase = (CookDatabase*)p;
	SafeDelete(pDatabase);
}

/** Get the Cooker's base directory - the folder that contains the Cooker executable. */
static String GetNativeLibPath()
{
#if SEOUL_PLATFORM_WINDOWS
	WCHAR aBuffer[MAX_PATH];
	SEOUL_VERIFY(0u != ::GetModuleFileNameW(nullptr, aBuffer, SEOUL_ARRAY_COUNT(aBuffer)));

	// Resolve the exact path to the editor binaries directory.
	return Path::GetExactPathName(Path::GetDirectoryName(WCharTToUTF8(aBuffer)));
#else
#	error Define for this platform.
#endif
}

/** Get the App's base directory - we use the app's base directory for GamePaths. */
static String GetBaseDirectoryPath()
{
	// Otherwise, derive based on cooker location.
	auto const sDllPath = GetNativeLibPath();

	// Now resolve the App directory using assumed directory structure.
	auto const sPath = Path::GetExactPathName(Path::Combine(
		Path::GetDirectoryName(sDllPath, 5),
		Path::Combine(SEOUL_APP_ROOT_NAME, "Binaries", "PC", "Developer", "x64")));

	return sPath;
}

static void Seoul_InitCore()
{
	Lock lock(s_Mutex);

	if (s_bCoreInit)
	{
		return;
	}

	s_bCoreInit = true;

	// Disable message boxes on failed assertions.
	g_bHeadless = true;
	g_bShowMessageBoxesOnFailedAssertions = false;
	g_bEnableMessageBoxes = false;

	// Mark that we're now in the main function.
	BeginMainFunction();

	// Setup the main thread ID.
	SetMainThreadId(Thread::GetThisThreadId());

	// Silence all log channels except for Warnings and Assertion.
	for (Int i = 0; i < (Int)LoggerChannel::MaxChannel; ++i)
	{
		Logger::GetSingleton().EnableChannel((LoggerChannel)i, false);
	}
	Logger::GetSingleton().EnableChannel(LoggerChannel::Assertion, true);
	Logger::GetSingleton().EnableChannel(LoggerChannel::Warning, true);

	// Hookup the file system callback.
	g_pInitializeFileSystemsCallback = OnInitializeFileSystems;

	// Initialize Core support.
	CoreSettings settings;
	settings.m_bLoadLoggerConfigurationFile = false;
	settings.m_bOpenLogFile = false;
	settings.m_GamePathsSettings.m_sBaseDirectoryPath = GetBaseDirectoryPath();
	Core::Initialize(settings);
}

static void Seoul_DeInitCore()
{
	Lock lock(s_Mutex);

	if (!s_bCoreInit)
	{
		return;
	}

	// Shutdown Core handling.
	Core::ShutDown();

	// Clear the callback.
	g_pInitializeFileSystemsCallback = nullptr;

	// No longer in main.
	EndMainFunction();

	s_bCoreInit = false;
}

} // namespace Seoul

extern "C"
{

SEOUL_EXPORT bool SEOUL_STD_CALL Seoul_AppendToJson(
	const char* sInOutFilename,
	void const* pIn,
	unsigned int uIn)
{
	return Seoul::AppendToJson(
		sInOutFilename,
		pIn,
		uIn);
}

SEOUL_EXPORT bool SEOUL_STD_CALL Seoul_CookJson(
	void const* pIn,
	unsigned int uIn,
	int iPlatform,
	void** ppOut,
	unsigned int* rpOut)
{
	return Seoul::CookJson(
		pIn,
		uIn,
		(Seoul::Platform)iPlatform,
		*ppOut,
		*rpOut);
}

SEOUL_EXPORT bool SEOUL_STD_CALL Seoul_MinifyJson(
	void const* pIn,
	unsigned int uIn,
	void** ppOut,
	unsigned int* puOut)
{
	return Seoul::MinifyJson(
		pIn,
		uIn,
		*ppOut,
		*puOut);
}

SEOUL_EXPORT bool SEOUL_STD_CALL Seoul_GetModifiedTimeOfFileInSar(
	const char* sSarPath,
	const char* sSerializedUrl,
	uint64_t* puModifiedTime)
{
	return Seoul::GetModifiedTimeOfFileInSar(
		sSarPath,
		sSerializedUrl,
		puModifiedTime);
}

SEOUL_EXPORT void SEOUL_STD_CALL Seoul_ReleaseJson(void* p)
{
	Seoul::ReleaseJson(p);
}

// Meant for external utilities that perform processing
// on JSON but need some help handling JSON files with
// additional features (comments, SeoulEngine JSON "command lists", etc).
SEOUL_EXPORT int SEOUL_STD_CALL Seoul_ExternalAppendJsonFile(
	const char* sInOutFilename,
	void const* pIn,
	unsigned int uIn)
{
	Seoul::String s;
	if (!Seoul::AppendToJson(sInOutFilename, pIn, uIn, s, Seoul::DataStoreParserFlags::kLeaveFilePathAsString))
	{
		return 0;
	}

	return Seoul::ExternalSaveJsonFile(s.CStr(), s.GetSize(), true, sInOutFilename) ? 1 : 0;
}

SEOUL_EXPORT int SEOUL_STD_CALL Seoul_ExternalFormatJson(
	void const* pIn,
	unsigned int uIn,
	int bResolveJsonCommands,
	void** ppOut,
	unsigned int* puOut)
{
	return Seoul::ExternalFormatJson(pIn, uIn, (bResolveJsonCommands != 0), *ppOut, *puOut) ? 1 : 0;
}

SEOUL_EXPORT int SEOUL_STD_CALL Seoul_ExternalLoadJsonFile(
	const char* sJsonPath,
	int bResolveJsonCommands,
	void** ppOut,
	unsigned int* puOut)
{
	return Seoul::ExternalLoadJsonFile(sJsonPath, (bResolveJsonCommands != 0), *ppOut, *puOut) ? 1 : 0;
}

SEOUL_EXPORT int SEOUL_STD_CALL Seoul_ExternalSaveJsonFile(
	void const* pIn,
	unsigned int uIn,
	int bUseExistingForHinting,
	const char* sOutputFilename)
{
	return Seoul::ExternalSaveJsonFile(pIn, uIn, (bUseExistingForHinting != 0), sOutputFilename) ? 1 : 0;
}

///////////////////////////////////////////////////////////////////////////////
// Originally part of NativeLibUtil
///////////////////////////////////////////////////////////////////////////////
SEOUL_EXPORT bool SEOUL_STD_CALL Seoul_LZ4Compress(
	void const* pInputData,
	unsigned int zInputDataSizeInBytes,
	char** ppOutputData,
	unsigned int* pzOutputDataSizeInBytes)
{
	using namespace Seoul;

	void* pOutputData = nullptr;
	UInt32 zOutputDataSizeInBytes = 0u;
	if (!LZ4Compress(
		pInputData,
		zInputDataSizeInBytes,
		pOutputData,
		zOutputDataSizeInBytes))
	{
		return false;
	}

	*ppOutputData = (char*)pOutputData;
	*pzOutputDataSizeInBytes = zOutputDataSizeInBytes;
	return true;
}

SEOUL_EXPORT void SEOUL_STD_CALL Seoul_ReleaseLZ4CompressedData(void* p)
{
	Seoul::MemoryManager::Deallocate(p);
}

SEOUL_EXPORT bool SEOUL_STD_CALL Seoul_LZ4Decompress(
	void const* pInputData,
	unsigned int zInputDataSizeInBytes,
	char** ppOutputData,
	unsigned int* pzOutputDataSizeInBytes)
{
	using namespace Seoul;

	void* pOutputData = nullptr;
	UInt32 zOutputDataSizeInBytes = 0u;
	if (!LZ4Decompress(
		pInputData,
		zInputDataSizeInBytes,
		pOutputData,
		zOutputDataSizeInBytes))
	{
		return false;
	}

	*ppOutputData = (char*)pOutputData;
	*pzOutputDataSizeInBytes = zOutputDataSizeInBytes;
	return true;
}

SEOUL_EXPORT void SEOUL_STD_CALL Seoul_ReleaseLZ4DecompressedData(void* p)
{
	Seoul::MemoryManager::Deallocate(p);
}

SEOUL_EXPORT unsigned int SEOUL_STD_CALL Seoul_GetCrc32(
	unsigned int uCrc32,
	void const* pInputData,
	unsigned int zInputDataSizeInBytes)
{
	return Seoul::GetCrc32(uCrc32, (Seoul::UInt8 const*)pInputData, (size_t)zInputDataSizeInBytes);
}

SEOUL_EXPORT Seoul::UInt64 SEOUL_STD_CALL Seoul_GetFileSize(const char* sFilename)
{
	return Seoul::DiskSyncFile::GetFileSize(Seoul::String(sFilename));
}

SEOUL_EXPORT Seoul::UInt64 SEOUL_STD_CALL Seoul_GetModifiedTime(const char* sFilename)
{
	return Seoul::DiskSyncFile::GetModifiedTime(Seoul::String(sFilename));
}

SEOUL_EXPORT bool SEOUL_STD_CALL Seoul_SetModifiedTime(const char* sFilename, Seoul::UInt64 uModifiedTime)
{
	return Seoul::DiskSyncFile::SetModifiedTime(Seoul::String(sFilename), uModifiedTime);
}

SEOUL_EXPORT bool SEOUL_STD_CALL Seoul_ZSTDCompress(
	void const* pInputData,
	unsigned int zInputDataSizeInBytes,
	char** ppOutputData,
	unsigned int* pzOutputDataSizeInBytes)
{
	using namespace Seoul;

	void* pOutputData = nullptr;
	UInt32 zOutputDataSizeInBytes = 0u;
	if (!ZSTDCompress(
		pInputData,
		zInputDataSizeInBytes,
		pOutputData,
		zOutputDataSizeInBytes))
	{
		return false;
	}

	*ppOutputData = (char*)pOutputData;
	*pzOutputDataSizeInBytes = zOutputDataSizeInBytes;
	return true;
}

SEOUL_EXPORT void SEOUL_STD_CALL Seoul_ReleaseZSTDCompressedData(void* p)
{
	Seoul::MemoryManager::Deallocate(p);
}

SEOUL_EXPORT bool SEOUL_STD_CALL Seoul_ZSTDDecompress(
	void const* pInputData,
	unsigned int zInputDataSizeInBytes,
	char** ppOutputData,
	unsigned int* pzOutputDataSizeInBytes)
{
	using namespace Seoul;

	void* pOutputData = nullptr;
	UInt32 zOutputDataSizeInBytes = 0u;
	if (!ZSTDDecompress(
		pInputData,
		zInputDataSizeInBytes,
		pOutputData,
		zOutputDataSizeInBytes))
	{
		return false;
	}

	*ppOutputData = (char*)pOutputData;
	*pzOutputDataSizeInBytes = zOutputDataSizeInBytes;
	return true;
}

SEOUL_EXPORT void SEOUL_STD_CALL Seoul_ReleaseZSTDDecompressedData(void* p)
{
	Seoul::MemoryManager::Deallocate(p);
}

SEOUL_EXPORT bool SEOUL_STD_CALL Seoul_CookDatabaseCheckUpToDate(
	void* p,
	const char* sFilename)
{
	return Seoul::CookDatabaseCheckUpToDate(
		p,
		sFilename);
}

SEOUL_EXPORT void* SEOUL_STD_CALL Seoul_CookDatabaseCreate(int iPlatform)
{
	return Seoul::CookDatabaseCreate((Seoul::Platform)iPlatform);
}

SEOUL_EXPORT void SEOUL_STD_CALL Seoul_CookDatabaseRelease(void* p)
{
	Seoul::CookDatabaseRelease(p);
}

SEOUL_EXPORT void SEOUL_STD_CALL Seoul_InitCore()
{
	Seoul::Seoul_InitCore();
}

SEOUL_EXPORT void SEOUL_STD_CALL Seoul_DeInitCore()
{
	Seoul::Seoul_DeInitCore();
}

} // extern "C"
