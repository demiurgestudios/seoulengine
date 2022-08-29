/**
 * \file FileManagerRemap.h
 * \brief FileManager internal utility. Used to remap FilePaths to other
 * FilePaths.
 *
 * The intended use case of the FileManagerRemap is A/B testing. It can
 * be (re)-configured with mapping tables that re-route file requests.
 *
 * For example, a request for Data/Config/Test.json will resolve to
 * Data/Config/TestA.json or Data/Config/TestB.json depending on the
 * definition of remap tables.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FILE_MANAGER_REMAP_H
#define FILE_MANAGER_REMAP_H

#include "FilePath.h"
#include "HashTable.h"
#include "Mutex.h"
namespace Seoul { class DataNode; }
namespace Seoul { class DataStore; }

namespace Seoul
{

class FileManagerRemap SEOUL_SEALED
{
public:
	typedef HashTable<FilePath, FilePath, MemoryBudgets::Io> RemapTable;

	// Utility, generates the has for a list of remap file paths.
	static UInt32 ComputeHash(FilePath const* iBegin, FilePath const* iEnd);

	// Utility. Merges remap entries defined in DataStore into rtRemap.
	//
	// NOTE: rtRemap may be partially modified if Merge() returns false.
	static Bool Merge(const DataStore& dataStore, const DataNode& dataNode, RemapTable& rtRemap);

	FileManagerRemap();
	~FileManagerRemap();

	// Configure or reconfigure this remap table.
	void Configure(const RemapTable& t, UInt32 uHash);

	/** @return The last set remap hash. */
	UInt32 GetRemapHash() const
	{
		Lock lock(m_Mutex);
		return m_uHash;
	}

	/** Single entry point - remaps rFilePath to its specified target, and returns true, or false if no remap. */
	Bool Remap(FilePath& rFilePath) const
	{
		Lock lock(m_Mutex);
		return m_tRemap.GetValue(rFilePath, rFilePath);
	}

private:
	RemapTable m_tRemap;
	UInt32 m_uHash;
	Mutex m_Mutex;

	SEOUL_DISABLE_COPY(FileManagerRemap);
}; // class FileManagerRemap

} // namespace Seoul

#endif // include guard
