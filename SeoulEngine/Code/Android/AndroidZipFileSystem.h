/**
 * \file AndroidZipFileSystem.h
 * \brief Specialization of IFileSystem that services file requests
 * from the current application's APK file. Unlike AndroidFileSystem,
 * this implementation uses .zip file reading based on mz_zip, to workaround
 * an issue in AAssetManager when loading very large files, due to the entire
 * file being accessed using mmap.
 *
 * See also: https://github.com/google/ExoPlayer/issues/5153
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ANDROID_ZIP_FILE_SYSTEM_H
#define ANDROID_ZIP_FILE_SYSTEM_H

#include "IFileSystem.h"
#include "Mutex.h"
namespace Seoul { class AndroidZipFileSystem; }
namespace Seoul { class DiskSyncFile; }
namespace Seoul { class ZipFileReader; }

namespace Seoul
{

/**
 * Concrete specialization of SyncFile for accessing file
 * data from within an Android APK file.
 */
class AndroidZipSyncFile SEOUL_SEALED : public SyncFile
{
public:
	AndroidZipSyncFile(
		AndroidZipFileSystem& rOwner,
		const String& sAbsoluteFilename,
		UInt64 uFileSize,
		Int64 iAbsoluteFileOffset);
	virtual ~AndroidZipSyncFile();

	// Read zSizeInBytes data from the file into pOut. Returns the
	// number of bytes actually read.
	virtual UInt32 ReadRawData(void* pOut, UInt32 zSizeInBytes) SEOUL_OVERRIDE;

	/**
	 * @return 0u - AndroidZipSyncFile is not writeable.
	 */
	virtual UInt32 WriteRawData(void const* pIn, UInt32 zSizeInBytes) SEOUL_OVERRIDE
	{
		return 0u;
	}

	/**
	 * @return An absolute filename that identifies this AndroidZipSyncFile.
	 */
	virtual String GetAbsoluteFilename() const SEOUL_OVERRIDE
	{
		return m_sAbsoluteFilename;
	}

	// Returns true if this file was opened successfully, false otherwise.
	virtual Bool IsOpen() const SEOUL_OVERRIDE;

	// Returns true if this file is open and can be read from.
	virtual Bool CanRead() const SEOUL_OVERRIDE
	{
		return IsOpen();
	}

	/**
	 * @return Always false - AndroidZipSyncFile is not writeable.
	 */
	virtual Bool CanWrite() const SEOUL_OVERRIDE
	{
		return false;
	}

	/**
	 * Nop - AndroidZipSyncFile is not writeable.
	 */
	virtual Bool Flush() SEOUL_OVERRIDE
	{
		return false;
	}

	/** @return the total size of the data in this AndroidZipSyncFile. */
	virtual UInt64 GetSize() const SEOUL_OVERRIDE
	{
		return m_uFileSize;
	}

	/**
	 * @return True if this AndroidZipSyncFile IsOpen(), false otherwise.
	 */
	virtual Bool CanSeek() const SEOUL_OVERRIDE
	{
		return IsOpen();
	}

	// Attempt to get the current absolute file pointer position.
	virtual Bool GetCurrentPositionIndicator(Int64& riPosition) const SEOUL_OVERRIDE;

	// Attempt a seek operation on the disk file.
	virtual Bool Seek(Int64 iPosition, File::SeekMode eMode) SEOUL_OVERRIDE;

private:
	AndroidZipFileSystem& m_rOwner;
	String const m_sAbsoluteFilename;
	UInt64 const m_uFileSize;
	Int64 const m_iAbsoluteFileOffset;
	Int64 m_iCurrentRelativeOffset;

	SEOUL_DISABLE_COPY(AndroidZipSyncFile);
}; // class AndroidZipSyncFile

/**
 * AndroidZipFileSystem services file open requests for
 * files contained in the current application's APK file.
 */
class AndroidZipFileSystem SEOUL_SEALED : public IFileSystem
{
public:
	AndroidZipFileSystem(const String& sApkPath);
	~AndroidZipFileSystem();

	/**
	 * @return Always false - AndroidZipFileSystem is not mutable.
	 */
	virtual Bool Copy(FilePath from, FilePath to, Bool bAllowOverwrite = false) SEOUL_OVERRIDE
	{
		return false;
	}

	/**
	 * @return Always false - AndroidZipFileSystem is not mutable.
	 */
	virtual Bool Copy(const String& sAbsoluteFrom, const String& sAbsoluteTo, Bool bAllowOverwrite = false) SEOUL_OVERRIDE
	{
		return false;
	}

	/**
	 * @return Always false - AndroidZipFileSystem is not mutable.
	 */
	virtual Bool CreateDirPath(FilePath dirPath) SEOUL_OVERRIDE
	{
		return false;
	}

	/**
	 * @return Always false - AndroidZipFileSystem is not mutable.
	 */
	virtual Bool CreateDirPath(const String& sDirPath) SEOUL_OVERRIDE
	{
		return false;
	}

	/**
	 * @return Always false - AndroidZipFileSystem is not mutable.
	 */
	virtual Bool DeleteDirectory(FilePath dirPath, Bool bRecursive) SEOUL_OVERRIDE
	{
		return false;
	}

	/**
	 * @return Always false - AndroidZipFileSystem is not mutable.
	 */
	virtual Bool DeleteDirectory(const String& sAbsoluteDirPath, Bool bRecursive) SEOUL_OVERRIDE
	{
		return false;
	}

	/**
	 * @return Always false - AndroidZipFileSystem is not mutable.
	 */
	virtual Bool Rename(FilePath from, FilePath to) SEOUL_OVERRIDE
	{
		return false;
	}

	/**
	 * @return Always false - AndroidZipFileSystem is not mutable.
	 */
	virtual Bool Rename(const String& sFrom, const String& sTo) SEOUL_OVERRIDE
	{
		return false;
	}

	/**
	 * @return Always false - AndroidZipFileSystem is not mutable.
	 */
	virtual Bool SetReadOnlyBit(FilePath filePath, Bool bReadOnly) SEOUL_OVERRIDE
	{
		return false;
	}

	/**
	 * @return Always false - AndroidZipFileSystem is not mutable.
	 */
	virtual Bool SetReadOnlyBit(const String& sAbsoluteFilename, Bool bReadOnly) SEOUL_OVERRIDE
	{
		return false;
	}

	virtual Bool GetFileSize(
		FilePath filePath,
		UInt64& ruFileSize) const SEOUL_OVERRIDE
	{
		return GetFileSize(filePath.GetAbsoluteFilename(), ruFileSize);
	}

	virtual Bool GetFileSize(
		const String& sAbsoluteFilename,
		UInt64& ruFileSize) const  SEOUL_OVERRIDE;

	virtual Bool GetModifiedTime(
		FilePath filePath,
		UInt64& ruModifiedTime) const SEOUL_OVERRIDE
	{
		return GetModifiedTime(filePath.GetAbsoluteFilename(), ruModifiedTime);
	}

	virtual Bool GetModifiedTime(
		const String& sAbsoluteFilename,
		UInt64& ruModifiedTime) const SEOUL_OVERRIDE
	{
		return false;
	}

	virtual Bool SetModifiedTime(FilePath filePath, UInt64 uModifiedTime) SEOUL_OVERRIDE
	{
		return false;
	}

	virtual Bool SetModifiedTime(const String& sAbsoluteFilename, UInt64 uModifiedTime) SEOUL_OVERRIDE
	{
		return false;
	}

	virtual Bool Delete(FilePath filePath) SEOUL_OVERRIDE
	{
		// Not supported.
		return false;
	}

	virtual Bool Delete(const String& sAbsoluteFilename) SEOUL_OVERRIDE
	{
		// Not supported.
		return false;
	}

	virtual Bool Exists(FilePath filePath) const SEOUL_OVERRIDE
	{
		return Exists(filePath.GetAbsoluteFilename());
	}

	virtual Bool Exists(const String& sAbsoluteFilename) const SEOUL_OVERRIDE;

	virtual Bool IsDirectory(FilePath filePath) const SEOUL_OVERRIDE
	{
		return IsDirectory(filePath.GetAbsoluteFilename());
	}

	virtual Bool IsDirectory(const String& sAbsoluteFilename) const SEOUL_OVERRIDE;

	virtual Bool Open(
		FilePath filePath,
		File::Mode eMode,
		ScopedPtr<SyncFile>& rpFile) SEOUL_OVERRIDE
	{
		return Open(filePath.GetAbsoluteFilename(), eMode, rpFile);
	}

	virtual Bool Open(
		const String& sAbsoluteFilename,
		File::Mode eMode,
		ScopedPtr<SyncFile>& rpFile) SEOUL_OVERRIDE;

	virtual Bool GetDirectoryListing(
		FilePath dirPath,
		Vector<String>& rvResults,
		Bool bIncludeDirectoriesInResults = true,
		Bool bRecursive = true,
		const String& sFileExtension = String()) const SEOUL_OVERRIDE
	{
		return GetDirectoryListing(
			dirPath.GetAbsoluteFilename(),
			rvResults,
			bIncludeDirectoriesInResults,
			bRecursive,
			sFileExtension);
	}

	virtual Bool GetDirectoryListing(
		const String& sAbsoluteDirectoryPath,
		Vector<String>& rvResults,
		Bool bIncludeDirectoriesInResults = true,
		Bool bRecursive = true,
		const String& sFileExtension = String()) const SEOUL_OVERRIDE;

private:
	String const m_sApkPath;
	friend class AndroidZipSyncFile;
	Mutex m_Mutex;
	ScopedPtr<DiskSyncFile> m_pApkFile;
	ScopedPtr<ZipFileReader> m_pZipFile;
	Bool const m_bOk;

	SEOUL_DISABLE_COPY(AndroidZipFileSystem);
}; // class AndroidZipFileSystem

} // namespace Seoul

#endif // include guard
