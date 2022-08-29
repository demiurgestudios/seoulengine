/**
 * \file Directory.h
 * \brief Functions to query and manipulate directories on disk.
 *
 * These functions only interact with the current platform's
 * persistent media - they will not interact with pack files and other
 * FileSystems through FileManager::Get().
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef DIRECTORY_H
#define DIRECTORY_H

#include "Delegate.h"
#include "Prereqs.h"
#include "SeoulString.h"

namespace Seoul::Directory
{

// Delete the directory - if bRecursive is true,
// also delete its contents.
Bool Delete(const String& sAbsolutePath, Bool bRecursive);

// Return true if sAbsoluteDirectoryPath specifies an existing directory,
// false if the directory does not exist, or if sAbsoluteDirectoryPath does
// not reference a directory.
Bool DirectoryExists(const String& sAbsoluteDirectoryPath);

// Populate rvResults with a list of files (and directories based on
// arguments) in the directory sAbsoluteDirectoryPath.
Bool GetDirectoryListing(
	const String& sAbsoluteDirectoryPath,
	Vector<String>& rvResults,
	Bool bIncludeDirectoriesInResults = true,
	Bool bRecursive = true,
	const String& sFileExtension = String());

// Try to create the directory sAbsoluteDirectoryPath. If necessary,
// will also attempt to create all parent directories that do
// not exist.
Bool CreateDirPath(const String& sAbsoluteDirectoryPath);

struct DirEntryEx SEOUL_SEALED
{
	UInt64 m_uModifiedTime{};
	String m_sFileName{};
	UInt32 m_uFileSize{};

	void Swap(DirEntryEx& r)
	{
		Seoul::Swap(m_uModifiedTime, r.m_uModifiedTime);
		m_sFileName.Swap(r.m_sFileName);
		Seoul::Swap(m_uFileSize, r.m_uFileSize);
	}
}; // struct DirEntryEx
typedef Delegate<Bool(DirEntryEx& entry)> GetDirectoryListingExCallback;

// Variation of GetDirectoryListing designed for gathering
// data of an entire directory tree. Currently used in the Cooker
// to prefetch files that will be part of a cook.
//
// Compare to GetDirectoryListing(), this function has the following
// limitations/differences:
// - listing is always recursive.
// - directories can never be included in results.
// - file extension masking is not available (it is always equivalent to String()).
// - each result is passed back to the caller via a callback.
// - results include:
//   - file name
//   - modification time (or 'last write' time)
//   - file size.
//
// IMPORTANT: callback is somewhat odd - it passes a
// reference to the entry, so the entry can be swapped
// away and acquired by the caller. It supports move
// semantics of the entry passed to callback.
Bool GetDirectoryListingEx(
	const String& sAbsoluteDirectoryPath,
	const GetDirectoryListingExCallback& callback);

} // namespace Seoul::Directory

#endif // include guard
