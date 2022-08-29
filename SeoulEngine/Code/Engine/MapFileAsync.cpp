/**
 * \file MapFileAsync.cpp
 * \brief Asynchronous implementation of IMapFile. If a binary (cooked)
 * map file is not available, this class will load and parse the text
 * version of the map file. Once complete, the map file will be automatically
 * written as a binary (cooked) BMAP map file. Until the source map file
 * changes, the binary map file will be used on future runs.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Engine.h"
#include "FileManager.h"
#include "GamePaths.h"
#include "JobsManager.h"
#include "Logger.h"
#include "MapFileAsync.h"
#include "Path.h"
#include "Platform.h"
#include "SeoulFile.h"
#include "SeoulFileReaders.h"
#include "SeoulFileWriters.h"
#include "StringUtil.h"

#include <stdio.h>

namespace Seoul
{

/**
 * Gets the name of the map file used to symbolify stack traces
 */
inline String GetSourceMapAbsoluteFilename()
{
	if (Engine::Get() && !Engine::Get()->GetExecutableName().IsEmpty())
	{
#if SEOUL_PLATFORM_WINDOWS || SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_LINUX
		return String(DEFAULT_PATH) + Path::ReplaceExtension(Engine::Get()->GetExecutableName(), ".map");
#elif SEOUL_PLATFORM_ANDROID
		return Path::ReplaceExtension(Engine::Get()->GetExecutableName(), ".map");
#else
#error "Define for this platform."
#endif
	}
	else
	{
		return String();
	}
}

#if SEOUL_ENABLE_STACK_TRACES
MapFileAsync::MapFileAsync()
	: Job(GetFileIOThreadId())	// use the file IO thread.
	, m_sSourceMapFileAbsoluteFilename(nullptr)
	, m_vEntries()
	, m_Mutex()
	, m_bAbortLoad(false)
	, m_bCompletelyLoaded(false)
{
	// Give this class a single self reference.
	++m_AtomicReferenceCount;

	// The absolute map file is stored in a custom buffer that
	// uses the MemoryBudgets::Debug type so it is excluded from
	// leak detection - the MapFile is intentionally kept alive until
	// after leak detection so it can be used to generate stack traces.
	String sAbsoluteFilename = GetSourceMapAbsoluteFilename();
	UInt32 const zSize = sAbsoluteFilename.GetSize();

	m_sSourceMapFileAbsoluteFilename.Reset(
		SEOUL_NEW(MemoryBudgets::Debug) Byte[zSize + 1u]);

	memcpy(m_sSourceMapFileAbsoluteFilename.Get(), sAbsoluteFilename.CStr(), zSize);
	m_sSourceMapFileAbsoluteFilename[zSize] = '\0';
}

MapFileAsync::~MapFileAsync()
{
	m_bAbortLoad = true;

	// Hack - MapFiles can be around very late in shutdown,
	// we want to avoid calling WaitUntilJobIsNotRunning()
	// here because the profiling hooks can cause a crash
	// on shutdown, since they allocate heap memory.
	if (IsJobRunning())
	{
		WaitUntilJobIsNotRunning();
	}

	// Sanity check that the only remaining reference is to ourself.
	SEOUL_ASSERT(1 == m_AtomicReferenceCount);
}

/**
 * Starts loading the map file asynchronously
 */
void MapFileAsync::StartLoad()
{
	StartJob(false);  // This is a no-op if we've already started the job
}

/**
 * Waits until the map file has finished loading
 */
void MapFileAsync::WaitUntilLoaded()
{
	StartJob(false);  // This is a no-op if we've already started the job
	WaitUntilJobIsNotRunning();
}

/**
 * Attempt to find the human readable name for the function at
 * physical address zAddress.
 */
void MapFileAsync::ResolveFunctionAddress(size_t zAddress, Byte *sFunctionName, size_t zMaxNameLen)
{
	Lock lock(m_Mutex);
	auto i = LowerBound(m_vEntries.Begin(), m_vEntries.End(), zAddress);
	if (i != m_vEntries.Begin() &&
		(m_bCompletelyLoaded || i != m_vEntries.End()))
	{
		const MapFileEntry& entry = *(--i);
		STRNCPY(sFunctionName, entry.m_sFunctionName, zMaxNameLen);
		return;
	}

	SNPRINTF(
		sFunctionName,
		zMaxNameLen,
		"0x%08x <map file loading>",
		(UInt32)zAddress);
}

/**
 * Where the magic happens - if a binary file exists and has
 * a time stamp >= the source map file, load that directly (much faster).
 * Otherwise, parse the source file. If this succeeds, it will
 * be saved as a binary map file for future runs.
 */
void MapFileAsync::InternalExecuteJob(Jobs::State& reNextState, ThreadId& rNextThreadId)
{
	m_bCompletelyLoaded = false;
	m_Timer.StartTimer();

	const String sSource = m_sSourceMapFileAbsoluteFilename.Get();
	const String sBinary = Path::ReplaceExtension(
		m_sSourceMapFileAbsoluteFilename.Get(),
		".bmap");

	UInt64 uBinaryModTime = FileManager::Get()->GetModifiedTime(sBinary);
	UInt64 uSourceModTime = FileManager::Get()->GetModifiedTime(sSource);
	if (uBinaryModTime >= uSourceModTime && uBinaryModTime != 0)
	{
		InternalLoadBinaryMapFile(sBinary);
	}
	else
	{
		InternalLoadSourceMapFile(sSource, sBinary);
	}
	m_Timer.StopTimer();

	SEOUL_LOG("Map file read complete, took %f seconds.\n",
		m_Timer.GetElapsedTimeInSeconds());

	reNextState = Jobs::State::kComplete;
}

/**
 * Load a binary map (.bmap) file directly.
 */
void MapFileAsync::InternalLoadBinaryMapFile(const String& sFilename)
{
	ScopedPtr<SyncFile> pFile;
	if (FileManager::Get()->OpenFile(sFilename, File::kRead, pFile))
	{
		MapFileEntries vEntries;
		if (ReadBuffer(*pFile, vEntries))
		{
			Lock lock(m_Mutex);
			m_vEntries.Swap(vEntries);

			m_bCompletelyLoaded = true;
		}
	}
	else
	{
		SEOUL_LOG_ENGINE("Warning: failed to open bmap file: %s\n", sFilename.CStr());
	}
}

/**
 * Load and parse a source map file. If this succeeds, write
 * it back out to the binary map file for future runs.
 */
void MapFileAsync::InternalLoadSourceMapFile(
	const String& sSourceFilename,
	const String& sBinaryFilename)
{
	if (InternalParseSourceMapFile(sSourceFilename))
	{
		ScopedPtr<SyncFile> pFile;
		if (FileManager::Get()->OpenFile(sBinaryFilename, File::kWriteTruncate, pFile) &&
			pFile->CanWrite())
		{
			Lock lock(m_Mutex);
			WriteBuffer(*pFile, m_vEntries);
		}

		m_bCompletelyLoaded = true;
	}
}

/**
 * Parse a source map file.
 */
Bool MapFileAsync::InternalParseSourceMapFile(const String& sSourceFilename)
{
	ScopedPtr<SyncFile> pUnbufferedFile;
	if (!FileManager::Get()->OpenFile(sSourceFilename, File::kRead, pUnbufferedFile) ||
		!pUnbufferedFile->CanRead())
	{
		SEOUL_LOG_ENGINE("Warning: failed to open map file: %s\n", sSourceFilename.CStr());
		return false;
	}

	ScopedPtr<BufferedSyncFile> pFile(SEOUL_NEW(MemoryBudgets::Debug) BufferedSyncFile(pUnbufferedFile.Get(), false));

	// Clear existing entries.
	{
		Lock lock(m_Mutex);
		m_vEntries.Clear();
	}

	// Every this many lines of input, we'll yield CPU to the job manager
	static const int kYieldInterval = 1000;
	Int nLinesRead = 0;

#if SEOUL_PLATFORM_WINDOWS
	static const String kAddressColumnHeader("Rva+Base");
	static const UniChar kColumnDelimiter(' ');
	static const UniChar kFunctionScopeDelimiter('@');
	static const String kPatternsToReplaceWithEmptyString("?");
	static const String kScopeDelimiter("::");

	String sLine;
	while (!m_bAbortLoad && pFile->ReadLine(sLine))
	{
		nLinesRead++;

		if (String::NPos != sLine.Find(kAddressColumnHeader))
		{
			// Skip the next line.
			(void)pFile->ReadLine(sLine);
			nLinesRead++;
			break;
		}

		// Don't hog the file IO thread.
		if (nLinesRead % kYieldInterval == 0)
		{
			SEOUL_ASSERT(Jobs::Manager::Get());
			Jobs::Manager::Get()->YieldThreadTime();
		}
	}

	while (!m_bAbortLoad && pFile->ReadLine(sLine))
	{
		nLinesRead++;

		Vector<String> vTokens;
		SplitString(sLine, kColumnDelimiter, vTokens);

		// Remove all empty string entries from the list of tokens.
		Bool bDone = false;
		while (!bDone)
		{
			bDone = true;
			for (UInt32 i = 0u; i < vTokens.GetSize(); ++i)
			{
				if (vTokens[i].IsEmpty())
				{
					bDone = false;
					vTokens.Erase(vTokens.Begin() + i);
					break;
				}
			}
		}

		// If we have at least three tokens
		if (vTokens.GetSize() >= 3u)
		{
			// Parse the address column (column 3) as a hexadecimal number
			UInt32 uAddress = 0u;
			if (1 == SSCANF(vTokens[2].CStr(), "%x", &uAddress))
			{
				Vector<String> vFunctionTokens;
				SplitString(vTokens[1], kFunctionScopeDelimiter, vFunctionTokens);
				String sFunctionName;

				// Start at the very end of the list of function scopes -
				// in the map, scopes are in reverse order.
				auto iter = vFunctionTokens.Find(String());
				Int iStart = (Int)vFunctionTokens.GetSize() - 1;

				// If an empty string is the list of function tokens, start
				// at the element just before the first empty string token
				// in the list - essentially, skip everything after it.
				if (vFunctionTokens.End() != iter)
				{
					iStart = (Int)(iter - vFunctionTokens.Begin() - 1);
				}

				// Clean and push function names.
				for (Int i = iStart; i >= 0; --i)
				{
					vFunctionTokens[i] = vFunctionTokens[i].ReplaceAll(
						kPatternsToReplaceWithEmptyString,
						String());
					sFunctionName += kScopeDelimiter + vFunctionTokens[i];
				}

				MapFileEntry entry;
				entry.m_zAddress = uAddress;
				STRNCPY(entry.m_sFunctionName, sFunctionName.CStr(), sizeof(entry.m_sFunctionName));
				entry.m_sFunctionName[sizeof(entry.m_sFunctionName) - 1u] = '\0';

				{
					Lock lock(m_Mutex);
					m_vEntries.PushBack(entry);
				}
			}
		}

		// Don't hog the file IO thread.
		if (nLinesRead % kYieldInterval == 0)
		{
			SEOUL_ASSERT(Jobs::Manager::Get());
			Jobs::Manager::Get()->YieldThreadTime();
		}
	}
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
	String sLine;
	while (!m_bAbortLoad && pFile->ReadLine(sLine))
	{
		nLinesRead++;

		MapFileEntry entry;
		UInt32 uSize, uAlign;

		// There's not really a good way to convert from a constant variable
		// to a format string that defines a length limit for a string to be
		// read, so we just hard-code it here, using a SEOUL_STATIC_ASSERT to make
		// sure that this fails to compile if kMaxFunctionNameLength changes.
		// Alternatively, we could sprintf the format string at runtime.
		SEOUL_STATIC_ASSERT(kMaxFunctionNameLength == 96);
		if (SSCANF(sLine.CStr(), "%zx %x %d %96[^\n]", &entry.m_zAddress, &uSize, &uAlign, entry.m_sFunctionName) == 4)
		{
			// Each symbol can appear 2-3 times in the map file.  I don't know
			// if this is documented anywhere, but it seems in practice that
			// the one we want (i.e. the one with the demangled symbol name) is
			// the one with an alignment of 0.
			if (uAlign == 0)
			{
				Lock lock(m_Mutex);
				m_vEntries.PushBack(entry);
			}
		}

		// Don't hog the file IO thread.
		if (nLinesRead % kYieldInterval == 0)
		{
			SEOUL_ASSERT(Jobs::Manager::Get());
			Jobs::Manager::Get()->YieldThreadTime();
		}
	}
#else
#error "Implement map file parsing for your platform"
#endif

	{
		Lock lock(m_Mutex);
		QuickSort(m_vEntries.Begin(), m_vEntries.End());
	}

	return !m_bAbortLoad;
}

#endif // /#if SEOUL_ENABLE_STACK_TRACES

} // namespace Seoul
