/**
 * \file MoriartyFileSystem.h
 * \brief File system implementation for using the Moriarty client to access
 * files over the network
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef MORIARTY_FILE_SYSTEM_H
#define MORIARTY_FILE_SYSTEM_H

#include "IFileSystem.h"
#include "StandardPlatformIncludes.h"

#if SEOUL_WITH_MORIARTY

namespace Seoul
{

/**
 * File system subclass for accessing remote files using MoriartyClient
 */
class MoriartyFileSystem SEOUL_SEALED : public IFileSystem
{
public:
	MoriartyFileSystem();
	~MoriartyFileSystem();

	// Attempt to copy from -> to.
	virtual Bool Copy(FilePath from, FilePath to, Bool bAllowOverwrite = false) SEOUL_OVERRIDE;

	// Only files that can be represented as a FilePath are supported
	virtual Bool Copy(const String& sAbsoluteFrom, const String& sAbsoluteTo, Bool bAllowOverwrite = false) SEOUL_OVERRIDE
	{
		return false;
	}

	// Try to create the directory. If necessary,
	// will also attempt to create all parent directories that do
	// not exist.
	virtual Bool CreateDirPath(FilePath dirPath) SEOUL_OVERRIDE;

	// Only files that can be represented as a FilePath are supported
	virtual Bool CreateDirPath(const String& sAbsoluteDir) SEOUL_OVERRIDE
	{
		return false;
	}

	// Try to delete the directory.
	virtual Bool DeleteDirectory(FilePath dirPath, Bool bRecursive) SEOUL_OVERRIDE;

	// Only files that can be represented as a FilePath are supported
	virtual Bool DeleteDirectory(const String& sAbsoluteDirectoryPath, Bool bRecursive) SEOUL_OVERRIDE
	{
		return false;
	}

	// Return true if this FileSystem contains filePath, false otherwise. If
	// this method returns true, rzFileSize will contain the file
	// size that this FileSystem tracks for the file.
	virtual Bool GetFileSize(FilePath filePath, UInt64& rzFileSize) const SEOUL_OVERRIDE;

	// Return true if this FileSystem contains filePath, false otherwise. If
	// this method returns true, rzFileSize will contain the file
	// size that this FileSystem tracks for the file.
	virtual Bool GetFileSizeForPlatform(Platform ePlatform, FilePath filePath, UInt64& rzFileSize) const SEOUL_OVERRIDE
	{
		if (keCurrentPlatform != ePlatform)
		{
			return false;
		}

		return GetFileSize(filePath, rzFileSize);
	}

	// Return true if this FileSystem contains filePath, false otherwise. If
	// this method returns true, rzFileSize will contain the file
	// size that this FileSystem tracks for the file.
	virtual Bool GetFileSize(const String& sAbsoluteFilename, UInt64& rzFileSize) const SEOUL_OVERRIDE
	{
		// Only files that can be represented as a FilePath are supported
		return false;
	}

	// Return true if this FileSystem contains filePath, false otherwise. If
	// this method returns true, ruModifiedTime will contain the modified
	// time that this FileSystem tracks for the file. This value may be 0u
	// if the FileSystem does not track modified times.
	virtual Bool GetModifiedTime(FilePath filePath, UInt64& ruModifiedTime) const SEOUL_OVERRIDE;

	// Return true if this FileSystem contains filePath, false otherwise. If
	// this method returns true, ruModifiedTime will contain the modified
	// time that this FileSystem tracks for the file. This value may be 0u
	// if the FileSystem does not track modified times.
	virtual Bool GetModifiedTimeForPlatform(Platform ePlatform, FilePath filePath, UInt64& ruModifiedTime) const SEOUL_OVERRIDE
	{
		if (keCurrentPlatform != ePlatform)
		{
			return false;
		}

		return GetModifiedTime(filePath, ruModifiedTime);
	}

	// Return true if this FileSystem contains filePath, false otherwise. If
	// this method returns true, ruModifiedTime will contain the modified
	// time that this FileSystem tracks for the file. This value may be 0u
	// if the FileSystem does not track modified times.
	virtual Bool GetModifiedTime(const String& sAbsoluteFilename, UInt64& ruModifiedTime) const SEOUL_OVERRIDE
	{
		// Only files that can be represented as a FilePath are supported
		return false;
	}

	// Attempt to rename the file or directory.
	virtual Bool Rename(FilePath from, FilePath to) SEOUL_OVERRIDE;

	// Only files that can be represented as a FilePath are supported
	virtual Bool Rename(const String& sAbsoluteFrom, const String& sAbsoluteTo) SEOUL_OVERRIDE
	{
		return false;
	}

	// Return true if this FileSystem successfully updated the modified time
	// of filePath to uModifiedTime, false otherwise. This method can
	// return false if the file does not exist in this IFileSystem, or if
	// the set failed for some os/filesystem specific reason (insufficient privileges).
	virtual Bool SetModifiedTime(FilePath filePath, UInt64 uModifiedTime) SEOUL_OVERRIDE;

	// Return true if this FileSystem successfully updated the modified time
	// of filePath to uModifiedTime, false otherwise. This method can
	// return false if the file does not exist in this IFileSystem, or if
	// the set failed for some os/filesystem specific reason (insufficient privileges).
	virtual Bool SetModifiedTime(const String& sAbsoluteFilename, UInt64 uModifiedTime) SEOUL_OVERRIDE
	{
		// Only files that can be represented as a FilePath are supported
		return false;
	}

	// Attempt to update the read/write status of a file.
	virtual Bool SetReadOnlyBit(FilePath filePath, Bool bReadOnly) SEOUL_OVERRIDE;

	// Only files that can be represented as a FilePath are supported
	virtual Bool SetReadOnlyBit(const String& sAbsoluteFilename, Bool bReadOnlyBit) SEOUL_OVERRIDE
	{
		return false;
	}

	// Attempt to delete filePath, return true on success.
	virtual Bool Delete(FilePath filePath) SEOUL_OVERRIDE;

	// Attempt to delete sAbsoluteFilename, return true on success.
	virtual Bool Delete(const String& sAbsoluteFilename) SEOUL_OVERRIDE
	{
		// Not supported.
		return false;
	}

	// Return true if the file described by filePath exists in this
	// file system, false otherwise.
	virtual Bool Exists(FilePath filePath) const SEOUL_OVERRIDE;

	// Return true if the file described by filePath exists in this
	// file system, false otherwise.
	virtual Bool ExistsForPlatform(Platform ePlatform, FilePath filePath) const SEOUL_OVERRIDE
	{
		if (keCurrentPlatform != ePlatform)
		{
			return false;
		}

		return Exists(filePath);
	}

	// Return true if the file described by sAbsoluteFilename exists in this
	// file system, false otherwise.
	virtual Bool Exists(const String& sAbsoluteFilename) const SEOUL_OVERRIDE
	{
		// Only files that can be represented as a FilePath are supported
		return false;
	}

	// Return true if the entry described by filePath exists in this file
	// system and is a directory, false otherwise
	virtual Bool IsDirectory(FilePath filePath) const SEOUL_OVERRIDE;

	// Return true if the entry described by sAbsoluteFilename exists in this
	// file system and is a directory, false otherwise
	virtual Bool IsDirectory(const String& sAbsoluteFilename) const SEOUL_OVERRIDE
	{
		// Only files that can be represented as a FilePath are supported
		return false;
	}

	// Return true if the file could be opened, false otherwise.
	// If this method returns true, rpFile is guaranteed to be
	// non-null and SyncFile::IsOpen() is guaranteed to return true.
	virtual Bool Open(
		FilePath filePath,
		File::Mode eMode,
		ScopedPtr<SyncFile>& rpFile) SEOUL_OVERRIDE;

	// Return true if the file could be opened, false otherwise.
	// If this method returns true, rpFile is guaranteed to be
	// non-null and SyncFile::IsOpen() is guaranteed to return true.
	virtual Bool Open(
		const String& sAbsoluteFilename,
		File::Mode eMode,
		ScopedPtr<SyncFile>& rpFile) SEOUL_OVERRIDE
	{
		// Only files that can be represented as a FilePath are supported
		return false;
	}

	// Attempt to populate rvResults with a list of files contained
	// in the directory represented by dirPath based on arguments. Returns
	// true if successful, false otherwise. If this method returns false,
	// rvResults is left unmodified.
	virtual Bool GetDirectoryListing(
		FilePath dirPath,
		Vector<String>& rvResults,
		Bool bIncludeDirectoriesInResults = true,
		Bool bRecursive = true,
		const String& sFileExtension = String()) const SEOUL_OVERRIDE;

	// Attempt to populate rvResults with a list of files contained
	// in the directory sAbsoluteDirectoryPath based on arguments. Returns
	// true if successful, false otherwise. If this method returns false,
	// rvResults is left unmodified.
	virtual Bool GetDirectoryListing(
		const String& sAbsoluteDirectoryPath,
		Vector<String>& rvResults,
		Bool bIncludeDirectoriesInResults = true,
		Bool bRecursive = true,
		const String& sFileExtension = String()) const SEOUL_OVERRIDE
	{
		// Only directories that can be represented as a FilePath are supported
		return false;
	}

private:
	SEOUL_DISABLE_COPY(MoriartyFileSystem);
};

} // namespace Seoul

#endif  // SEOUL_WITH_MORIARTY

#endif  // include guard
