/**
 * \file CookDatabase.h
 * \brief Utility class that monitors cookable files. Used to
 * determine if a file needs to be recooked, as well as to
 * query certain properties about cookable files.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef COOK_DATABASE_H
#define COOK_DATABASE_H

#include "Atomic32.h"
#include "FileChangeNotifier.h"
#include "FilePath.h"
#include "HashSet.h"
#include "HashTable.h"
#include "Mutex.h"
#include "Prereqs.h"
#include "Vector.h"
namespace Seoul { class DataNode; }
namespace Seoul { class DataStore; }
namespace Seoul { class FileChangeNotifier; }

namespace Seoul
{

struct CookMetadata SEOUL_SEALED
{
	UInt64 m_uCookedTimestamp{}; // Timestamp for the output file that corresponds to this metadata.
	UInt64 m_uMetadataTimestamp{}; // Timestamp of the last metadata update.
	UInt32 m_uCookerVersion{}; // Version of the cooker used to generate the file.
	UInt32 m_uDataVersion{}; // Version of the cooked file.

	struct Source SEOUL_SEALED
	{
		UInt64 m_uTimestamp{};
		FilePath m_Source{};
	};
	typedef Vector<Source, MemoryBudgets::Cooking> Sources;

	// Rarely used (currently needed for audio banks). For files that generate sibling
	// cooked outputs, this will contain those outputs, for both up-to-date checking
	// as well as miscellaneous file operations. Changes to these files trigger
	// a recook of the output.
	Sources m_vSiblings;

	// List of files in Source that contribute to this output file. Changes to these
	// files trigger a recook of the output.
	Sources m_vSources;

	/**
	 * A directory source tracks the file count in the directory (recursively).
	 * This is to detect the addition of news files that will be implicitly
	 * part of an output file. Since we only track the count,
	 * you still need to add individual files as a source to track changes
	 * or deletions.
	 */
	struct DirectorySource SEOUL_SEALED
	{
		UInt32 m_uFileCount{};
		FilePath m_Source{};
	};
	typedef Vector<DirectorySource, MemoryBudgets::Cooking> DirectorySources;
	DirectorySources m_vDirectorySources;
}; // struct CookMetadata

// Defines a source for metadata dependencies.
struct CookSource SEOUL_SEALED
{
	CookSource(FilePath filePath = FilePath(), Bool bDirectory = false, Bool bDebugOnly = false, Bool bSibling = false)
		: m_FilePath(filePath)
		, m_bDirectory(bDirectory)
		, m_bDebugOnly(bDebugOnly)
		, m_bSibling(bSibling)
	{
	}

	FilePath m_FilePath{};
	Bool m_bDirectory{};
	Bool m_bDebugOnly{};
	Bool m_bSibling{};
}; // struct CookSource

class CookDatabase SEOUL_SEALED
{
public:
	SEOUL_DELEGATE_TARGET(CookDatabase);

	// One-to-one file types do not have extended metadata, instead,
	// files are considered up-to-date based on mod time different
	// and global version tracking.
	static Bool IsOneToOneType(FileType eType);
	// Current global version of the cook databse.
	static UInt32 GetCookerVersion();
	// Lookup the data version of a given file type.
	static UInt32 GetDataVersion(FileType eType);

	typedef Vector<FilePath, MemoryBudgets::Cooking> Dependents;
	typedef HashTable<FilePath, CookMetadata, MemoryBudgets::Cooking> Metadata;

	CookDatabase(Platform ePlatform, Bool bProcessOneToOneVersions);
	~CookDatabase();

	// Return true if the cooked version of the given
	// file is up-to-date.
	//
	// NOTE: This function may read or re-read metadata
	// from disk (synchronously), so it is not recommended
	// to call it from within time sensitive functions.
	Bool CheckUpToDate(FilePath filePath);

	// Equivalent to CheckUpToDate(), except also populates
	// rvOut with the list of FilePaths that have changed
	// when not up to date. Note that this makes this function
	// quite a bit more expensive than CheckUpToDate(),
	// so it should only be called when the extra information
	// is actually needed.
	//
	// NOTE: rvOut may be empty, in which case a version mismatch
	// or some other "global" case has occured, in which case
	// the caller should assume all dependencies are out-of-date.
	Bool CheckUpToDateWithDetails(FilePath filePath, Dependents& rvOut);

	// Output a vector of any output files that
	// need to be recooked if the give source file changes.
	void GetDependents(FilePath filePath, Dependents& rvDependents) const;

	/** @return The platform to which this CookDatabase was constructed. */
	Platform GetPlatform() const { return m_ePlatform; }

	// For special cases and situations where you know
	// that you will query file attributes immediately
	// after a mutation.
	void ManualOnFileChange(FilePath filePath);

	// Update the metdata for a given file. Commits the data to disk.
	void UpdateMetadata(
		FilePath filePath,
		UInt64 uCookedTimestamp,
		CookSource const* pBeginSources,
		CookSource const* pEndSources);

#if SEOUL_UNIT_TESTS
	Bool UnitTestHook_GetMetadata(FilePath filePath, CookMetadata& rMetadata);
#endif // /SEOUL_UNIT_TEST

private:
	typedef HashSet<FilePath, MemoryBudgets::Cooking> DepSet;
	typedef HashTable<FilePath, DepSet, MemoryBudgets::Cooking> DepTable;
	typedef HashTable<FilePath, Bool, MemoryBudgets::Cooking> UpToDate;

	Platform const m_ePlatform;
	Mutex m_Mutex;
	DepSet m_Changed;
	UpToDate m_tUpToDate;
	DepTable m_tDep;
	DepTable m_tDepDir;
	Metadata m_tMetadata;
	ScopedPtr<FileChangeNotifier> m_pContentNotifier;

	static Bool CommitSingleMetadata(const CookMetadata& metadata, DataStore& ds, const DataNode& root);
	static Bool WriteSingleMetadataToDisk(Platform ePlatform, FilePath filePath, const DataStore& ds, const DataNode& dataNode);

	void InsideLockAddDependents(FilePath filePath, const CookMetadata& metadata, DepTable& rtDep, DepTable& rtDepDir) const;
	void InsideLockGetDependents(FilePath filePath, Dependents& rvDependents) const;
	void InsideLockOnFileChange(FilePath filePath);
	void InsideLockHandleFileChange(FilePath filePath);
	void InsideLockRemoveFromCaches(FilePath filePath);
	const CookMetadata& InsideLockResolveMetadata(FilePath filePath);
	void OnFileChange(const String& sOldPath, const String& sNewPath, FileChangeNotifier::FileEvent eEvent);
	void InsideLockRemoveDependents(FilePath filePath);

	CookMetadata InsideLockReadMetadata(FilePath filePath);

	void ProcessOneToOneVersions();

	SEOUL_DISABLE_COPY(CookDatabase);
}; // class CookDatabase

} // namespace Seoul

#endif // include guard
