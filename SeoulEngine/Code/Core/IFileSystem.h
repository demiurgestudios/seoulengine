/**
 * \file IFileSystem.h
 * \brief Base interface for classes that can service file open requests.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef IFILE_SYSTEM_H
#define IFILE_SYSTEM_H

#include "Delegate.h"
#include "Prereqs.h"
#include "ScopedPtr.h"
#include "SeoulFile.h"

namespace Seoul
{

// Forward declarations
class SyncFile;

enum class NetworkFetchPriority
{
	/** Allow for client use cases - absolute lowest priority fetch operation. */
	kLow,

	/** Allow for client uses cases - seond tier, lower priority fetch operation. */
	kMedium,

	/** Default priority of all Prefetch() and Fetch() operations. */
	kDefault,

	/** Allow for client use cases - second tier, higher priority fetch operation. */
	kHigh,

	/** Allow for client use cases - absolute highest priority fetch operation. */
	kCritical,
};

class IFileSystem SEOUL_ABSTRACT
{
public:
	virtual ~IFileSystem()
	{
	}

	/**
	 * @return True if operations must be completed before this FileSystem is
	 * fully initialized, false otherwise.
	 */
	virtual Bool IsInitializing() const
	{
		return false;
	}

	/**
	 * Wait for the file system to finish initializing, if it is still in the
	 * process of initializing.
	 *
	 * @return true if initialization completed successfully, false otherwise.
	 */
	virtual Bool WaitForInit(UInt32 uMaxTimeInMs)
	{
		return !IsInitializing();
	}

	/**
	 * @return True if operations on filePath may be serviced
	 * over a (relatively high latency, low bandwidth) network connection.
	 */
	virtual Bool IsServicedByNetwork(FilePath filePath) const
	{
		return false;
	}

	/**
	 * @return True if operations on sAbsoluteFilename may be serviced
	 * over a (relatively high latency, low bandwidth) network connection.
	 */
	virtual Bool IsServicedByNetwork(const String& sAbsoluteFilename) const
	{
		return false;
	}

	/**
	 * If serviced by this network and on the network, synchronously download
	 * filePath.
	 */
	virtual Bool NetworkFetch(FilePath filePath, NetworkFetchPriority ePriority = NetworkFetchPriority::kDefault)
	{
		return false;
	}

	/**
	 * If serviced by this network and on the network, prepare filePath
	 * for service (download it from the network).
	 */
	virtual Bool NetworkPrefetch(FilePath filePath, NetworkFetchPriority ePriority = NetworkFetchPriority::kDefault)
	{
		return false;
	}

	/** Called by FileManager when networking is up and running. */
	virtual void OnNetworkInitialize()
	{
	}

	/** Called by FileManager when networking is shutting down. */
	virtual void OnNetworkShutdown()
	{
	}

	// Attempt to copy the file.
	virtual Bool Copy(FilePath from, FilePath to, Bool bAllowOverwrite = false) = 0;

	// Attempt to copy the file.
	virtual Bool Copy(const String& sAbsoluteFrom, const String& sAbsoluteTo, Bool bAllowOverwrite = false) = 0;

	// Return true if this FileSystem contains filePath, false otherwise. If
	// this method returns true, rzFileSize will contain the file
	// size that this FileSystem tracks for the file.
	virtual Bool GetFileSize(FilePath filePath, UInt64& rzFileSize) const = 0;

	// Return true if this FileSystem contains filePath, false otherwise. If
	// this method returns true, rzFileSize will contain the file
	// size that this FileSystem tracks for the file.
	virtual Bool GetFileSize(const String& sAbsoluteFilename, UInt64& rzFileSize) const = 0;

	// Return true if the file described by sAbsoluteFilename exists in this
	// file system, false otherwise.
	virtual Bool GetFileSizeForPlatform(Platform ePlatform, FilePath filePath, UInt64& rzFileSize) const
	{
		return GetFileSize(filePath.GetAbsoluteFilenameForPlatform(ePlatform), rzFileSize);
	}

	// Return true if this FileSystem contains filePath, false otherwise. If
	// this method returns true, ruModifiedTime will contain the modified
	// time that this FileSystem tracks for the file. This value may be 0u
	// if the FileSystem does not track modified times.
	virtual Bool GetModifiedTime(FilePath filePath, UInt64& ruModifiedTime) const = 0;

	// Return true if this FileSystem contains filePath, false otherwise. If
	// this method returns true, ruModifiedTime will contain the modified
	// time that this FileSystem tracks for the file. This value may be 0u
	// if the FileSystem does not track modified times.
	virtual Bool GetModifiedTime(const String& sAbsoluteFilename, UInt64& ruModifiedTime) const = 0;

	// Return true if the file described by sAbsoluteFilename exists in this
	// file system, false otherwise.
	virtual Bool GetModifiedTimeForPlatform(Platform ePlatform, FilePath filePath, UInt64& ruModifiedTime) const
	{
		return GetModifiedTime(filePath.GetAbsoluteFilenameForPlatform(ePlatform), ruModifiedTime);
	}

	// Return true if the file described by filePath exists and has a valid
	// modification time in the project's Source/ folder.
	virtual Bool GetModifiedTimeInSource(FilePath filePath, UInt64& ruModifiedTime) const
	{
		return GetModifiedTime(filePath.GetAbsoluteFilenameInSource(), ruModifiedTime);
	}

	// Attempt to rename the file or directory.
	virtual Bool Rename(FilePath from, FilePath to) = 0;

	// Attempt to rename the file or directory.
	virtual Bool Rename(const String& sAbsoluteFrom, const String& sAbsoluteTo) = 0;

	// Return true if this FileSystem successfully updated the modified time
	// of filePath to uModifiedTime, false otherwise. This method can
	// return false if the file does not exist in this IFileSystem, or if
	// the set failed for some os/filesystem specific reason (insufficient privileges).
	virtual Bool SetModifiedTime(FilePath filePath, UInt64 uModifiedTime) = 0;

	// Return true if this FileSystem successfully updated the modified time
	// of filePath to uModifiedTime, false otherwise. This method can
	// return false if the file does not exist in this IFileSystem, or if
	// the set failed for some os/filesystem specific reason (insufficient privileges).
	//
	// Initially identical to SetModifiedTime(), except resolves filePath based
	// on ePlatform.
	virtual Bool SetModifiedTimeForPlatform(Platform ePlatform, FilePath filePath, UInt64 uModifiedTime)
	{
		return SetModifiedTime(filePath.GetAbsoluteFilenameForPlatform(ePlatform), uModifiedTime);
	}

	// Return true if this FileSystem successfully updated the modified time
	// of filePath to uModifiedTime, false otherwise. This method can
	// return false if the file does not exist in this IFileSystem, or if
	// the set failed for some os/filesystem specific reason (insufficient privileges).
	virtual Bool SetModifiedTime(const String& sAbsoluteFilename, UInt64 uModifiedTime) = 0;

	// Attempt to update the read/write status of a file.
	virtual Bool SetReadOnlyBit(FilePath filePath, Bool bReadOnly) = 0;

	// Attempt to update the read/write status of a file.
	virtual Bool SetReadOnlyBit(const String& sAbsoluteFilename, Bool bReadOnly) = 0;

	// Try to create the directory. If necessary,
	// will also attempt to create all parent directories that do
	// not exist.
	virtual Bool CreateDirPath(FilePath dirPath) = 0;

	// Try to create the directory. If necessary,
	// will also attempt to create all parent directories that do
	// not exist.
	virtual Bool CreateDirPath(const String& sAbsoluteDir) = 0;

	// Attempt to delete filePath, return true on success.
	virtual Bool Delete(FilePath filePath) = 0;

	// Attempt to delete sAbsoluteFilename, return true on success.
	virtual Bool Delete(const String& sAbsoluteFilename) = 0;

	// Attempt to delete dirPath, return true on success.
	virtual Bool DeleteDirectory(FilePath dirPath, Bool bRecursive) = 0;

	// Attempt to delete sAbsoluteDirPath, return true on success.
	virtual Bool DeleteDirectory(const String& sAbsoluteDirPath, Bool bRecursive) = 0;

	// Return true if the file described by filePath exists in this
	// file system, false otherwise.
	virtual Bool Exists(FilePath filePath) const = 0;

	// Return true if the file described by sAbsoluteFilename exists in this
	// file system, false otherwise.
	virtual Bool Exists(const String& sAbsoluteFilename) const = 0;

	// Return true if the file described by sAbsoluteFilename exists in this
	// file system, false otherwise.
	virtual Bool ExistsForPlatform(Platform ePlatform, FilePath filePath) const
	{
		return Exists(filePath.GetAbsoluteFilenameForPlatform(ePlatform));
	}

	// Return true if the file described by filePath exists in the project's
	// Source/ folder, false otherwise.
	virtual Bool ExistsInSource(FilePath filePath) const
	{
		return Exists(filePath.GetAbsoluteFilenameInSource());
	}

	// Return true if the entry described by filePath exists in this file
	// system and is a directory, false otherwise
	virtual Bool IsDirectory(FilePath filePath) const = 0;

	// Return true if the entry described by sAbsoluteFilename exists in this
	// file system and is a directory, false otherwise
	virtual Bool IsDirectory(const String& sAbsoluteFilename) const = 0;

	// Return true if the file could be opened, false otherwise.
	// If this method returns true, rpFile is guaranteed to be
	// non-nullptr and SyncFile::IsOpen() is guaranteed to return true.
	virtual Bool Open(
		FilePath filePath,
		File::Mode eMode,
		ScopedPtr<SyncFile>& rpFile) = 0;

	// Return true if the file could be opened, false otherwise.
	// If this method returns true, rpFile is guaranteed to be
	// non-nullptr and SyncFile::IsOpen() is guaranteed to return true.
	//
	// By default, identical to Open() but resolves filePath based
	// on ePlatform.
	virtual Bool OpenForPlatform(
		Platform ePlatform,
		FilePath filePath,
		File::Mode eMode,
		ScopedPtr<SyncFile>& rpFile)
	{
		return Open(filePath.GetAbsoluteFilenameForPlatform(ePlatform), eMode, rpFile);
	}

	// Return true if the file could be opened, false otherwise.
	// If this method returns true, rpFile is guaranteed to be
	// non-nullptr and SyncFile::IsOpen() is guaranteed to return true.
	virtual Bool Open(
		const String& sAbsoluteFilename,
		File::Mode eMode,
		ScopedPtr<SyncFile>& rpFile) = 0;

	// Attempt to populate rvResults with a list of files contained
	// in the directory represented by filePath based on arguments. Returns
	// true if successful, false otherwise. If this method returns false,
	// rvResults is left unmodified.
	virtual Bool GetDirectoryListing(
		FilePath filePath,
		Vector<String>& rvResults,
		Bool bIncludeDirectoriesInResults = true,
		Bool bRecursive = true,
		const String& sFileExtension = String()) const = 0;

	// Attempt to populate rvResults with a list of files contained
	// in the directory sAbsoluteDirectoryPath based on arguments. Returns
	// true if successful, false otherwise. If this method returns false,
	// rvResults is left unmodified.
	virtual Bool GetDirectoryListing(
		const String& sAbsoluteDirectoryPath,
		Vector<String>& rvResults,
		Bool bIncludeDirectoriesInResults = true,
		Bool bRecursive = true,
		const String& sFileExtension = String()) const = 0;

	// Attempt to read the entire contents of a file into a new memory buffer
	// rpOutputBuffer. The default implementation opens the file, reads its contents,
	// and then closes the file. Specializations of IFileSystem can customize
	// this method to optimize it further.
	virtual Bool ReadAll(
		FilePath filePath,
		void*& rpOutputBuffer,
		UInt32& rzOutputSizeInBytes,
		UInt32 zAlignmentOfOutputBuffer,
		MemoryBudgets::Type eOutputBufferMemoryType,
		UInt32 zMaxReadSize = kDefaultMaxReadSize)
	{
		ScopedPtr<SyncFile> pFile;
		if (Open(filePath, File::kRead, pFile))
		{
			return pFile->ReadAll(
				rpOutputBuffer,
				rzOutputSizeInBytes,
				zAlignmentOfOutputBuffer,
				eOutputBufferMemoryType,
				zMaxReadSize);
		}

		return false;
	}

	// Attempt to read the entire contents of a file into a new memory buffer
	// rpOutputBuffer. The default implementation opens the file, reads its contents,
	// and then closes the file. Specializations of IFileSystem can customize
	// this method to optimize it further.
	//
	// Identical to ReadAll(), except resolves filePath based on ePlatform.
	virtual Bool ReadAllForPlatform(
		Platform ePlatform,
		FilePath filePath,
		void*& rpOutputBuffer,
		UInt32& rzOutputSizeInBytes,
		UInt32 zAlignmentOfOutputBuffer,
		MemoryBudgets::Type eOutputBufferMemoryType,
		UInt32 zMaxReadSize = kDefaultMaxReadSize)
	{
		ScopedPtr<SyncFile> pFile;
		if (OpenForPlatform(ePlatform, filePath, File::kRead, pFile))
		{
			return pFile->ReadAll(
				rpOutputBuffer,
				rzOutputSizeInBytes,
				zAlignmentOfOutputBuffer,
				eOutputBufferMemoryType,
				zMaxReadSize);
		}

		return false;
	}

	// Attempt to read the entire contents of a file into a new memory buffer
	// rpOutputBuffer. The default implementation opens the file, reads its contents,
	// and then closes the file. Specializations of IFileSystem can customize
	// this method to optimize it further.
	virtual Bool ReadAll(
		const String& sAbsoluteFilename,
		void*& rpOutputBuffer,
		UInt32& rzOutputSizeInBytes,
		UInt32 zAlignmentOfOutputBuffer,
		MemoryBudgets::Type eOutputBufferMemoryType,
		UInt32 zMaxReadSize = kDefaultMaxReadSize)
	{
		ScopedPtr<SyncFile> pFile;
		if (Open(sAbsoluteFilename, File::kRead, pFile))
		{
			return pFile->ReadAll(
				rpOutputBuffer,
				rzOutputSizeInBytes,
				zAlignmentOfOutputBuffer,
				eOutputBufferMemoryType,
				zMaxReadSize);
		}

		return false;
	}

	// Attempt to write the entire contents of a file to disk.
	virtual Bool WriteAll(
		FilePath filePath,
		void const* pInputBuffer,
		UInt32 uInputSizeInBytes,
		UInt64 uModifiedTime = 0)
	{
		ScopedPtr<SyncFile> pFile;
		if (Open(filePath, File::kWriteTruncate, pFile))
		{
			if (pFile->WriteAll(pInputBuffer, uInputSizeInBytes))
			{
				pFile.Reset();

				// On successful write, when specified, set the modified
				// time.
				if (0 != uModifiedTime)
				{
					return SetModifiedTime(filePath, uModifiedTime);
				}
				else
				{
					return true;
				}
			}
		}

		return false;
	}

	// Attempt to write the entire contents of a file to disk.
	//
	// Identical to WriteAll, except resolves filePath based
	// on ePlatform.
	virtual Bool WriteAllForPlatform(
		Platform ePlatform,
		FilePath filePath,
		void const* pInputBuffer,
		UInt32 uInputSizeInBytes,
		UInt64 uModifiedTime = 0)
	{
		return WriteAll(filePath.GetAbsoluteFilenameForPlatform(ePlatform), pInputBuffer, uInputSizeInBytes, uModifiedTime);
	}

	// Attempt to write the entire contents of a file to disk.
	virtual Bool WriteAll(
		const String& sAbsoluteFilename,
		void const* pInputBuffer,
		UInt32 uInputSizeInBytes,
		UInt64 uModifiedTime = 0)
	{
		ScopedPtr<SyncFile> pFile;
		if (Open(sAbsoluteFilename, File::kWriteTruncate, pFile))
		{
			if (pFile->WriteAll(pInputBuffer, uInputSizeInBytes))
			{
				pFile.Reset();

				// On successful write, when specified, set the modified
				// time.
				if (0 != uModifiedTime)
				{
					return SetModifiedTime(sAbsoluteFilename, uModifiedTime);
				}
				else
				{
					return true;
				}
			}
		}

		return false;
	}

protected:
	// Must be specialized
	IFileSystem()
	{
	}

private:
	SEOUL_DISABLE_COPY(IFileSystem);
}; // class IFileSystem

} // namespace Seoul

#endif // include guard
