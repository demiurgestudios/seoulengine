/**
 * \file MapFileAsync.h
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

#pragma once
#ifndef MAP_FILE_ASYNC_H
#define MAP_FILE_ASYNC_H

#include "JobsJob.h"
#include "Mutex.h"
#include "Prereqs.h"
#include "ScopedPtr.h"
#include "SeoulTime.h"
#include "Vector.h"

namespace Seoul
{

// Only available if threading is enabled.
#if SEOUL_ENABLE_STACK_TRACES

// Forward declarations
class SyncFile;

/**
 * One entry in the vector of map file entries - must be POD.
 */
struct MapFileEntry
{
	/** Relative address in the map file of the function. */
	size_t m_zAddress;

	/** Human readable function name, up to kMaxFunctionNameLength - 1 characters. */
	Byte m_sFunctionName[IMapFile::kMaxFunctionNameLength];
};

inline Bool operator<(const MapFileEntry& a, const MapFileEntry& b)
{
	return (a.m_zAddress < b.m_zAddress);
}

inline Bool operator<(size_t zAddress, const MapFileEntry& b)
{
	return (zAddress < b.m_zAddress);
}

inline Bool operator<(const MapFileEntry& a, size_t zAddress)
{
	return (a.m_zAddress < zAddress);
}

/** Container of map file entries - effectively, the map file. */
typedef Vector<MapFileEntry, MemoryBudgets::Debug> MapFileEntries;

/**
 * Async load and parse of a map file, useful
 * for performing function name lookups on stack trace captures.
 */
class MapFileAsync SEOUL_SEALED : public Jobs::Job, public IMapFile
{
public:
	MapFileAsync();

	virtual void StartLoad() SEOUL_OVERRIDE;
	virtual void WaitUntilLoaded() SEOUL_OVERRIDE;

	virtual void ResolveFunctionAddress(size_t zAddress, Byte *sFunctionName, size_t zMaxNameLen) SEOUL_OVERRIDE;

private:
	virtual ~MapFileAsync();
	SEOUL_REFERENCE_COUNTED_SUBCLASS(MapFileAsync);

	virtual void InternalExecuteJob(Jobs::State& reNextState, ThreadId& rNextThreadId) SEOUL_OVERRIDE;

	void InternalLoadBinaryMapFile(const String& sFilename);
	void InternalLoadSourceMapFile(
		const String& sSourceFilename,
		const String& sBinaryFilename);
	Bool InternalParseSourceMapFile(const String& sSourceFilename);

	ScopedArray<Byte> m_sSourceMapFileAbsoluteFilename;
	SeoulTime m_Timer;
	MapFileEntries m_vEntries;
	Mutex m_Mutex;
	Atomic32Value<Bool> m_bAbortLoad;
	Atomic32Value<Bool> m_bCompletelyLoaded;

	SEOUL_DISABLE_COPY(MapFileAsync);
}; // class MapFileAsync
#endif // /#if SEOUL_ENABLE_STACK_TRACES

} // namespace Seoul

#endif // include guard
