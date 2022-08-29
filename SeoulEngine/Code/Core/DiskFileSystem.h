/**
 * \file DiskFileSystem.h
 * \brief Specialization of IFileSystem that services file requests
 * from persistent storage, using the current platform's standard file system.
 *
 * DiskFileSystem can service both read and write file requests.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef DISK_FILE_SYSTEM_H
#define DISK_FILE_SYSTEM_H

#include "IFileSystem.h"

namespace Seoul
{

enum class OpenResult
{
	Success,

	/** Our process does not have sufficient privileges to perform the open. */
	ErrorAccess,

	/** Target file already exists and open mode is incompatible. */
	ErrorExist,

	/** Specified flags are unsupported for the state of the file. */
	ErrorInvalid,

	/** Unknown IO failure, may be temporary. */
	ErrorIo,

	/** Attempting to open a regular file that already exists as a directory. */
	ErrorIsDir,

	/** Target open path is too long for the file system. */
	ErrorNameTooLong,

	/** Attempt to open a file that does not exist when it is required to exist by open flags. */
	ErrorNoEntity,

	/** Insufficient disk space to perform the open. */
	ErrorNoSpace,

	/** Attempt to open a file for write that is read-only on disk. */
	ErrorReadOnly,

	/** Too many files are opened by the current process. */
	ErrorTooManyProcess,

	/** Too many files are opened by the system. */
	ErrorTooManySystem,

	/** A non-specific error case. */
	ErrorUnknown,
};

enum class RenameResult
{
	Success,

	/** Our process does not have sufficient privileges to perform the rename. */
	ErrorAccess,

	/** Source or target is currently open in another process that prevents the rename. */
	ErrorBusy,

	/** Target file already exists in a form that prevents the rename. */
	ErrorExist,

	/** Rename operation that is fundamentally invalid (typically, rename of a directory to within the path of the old directory). */
	ErrorInvalid,

	/** Unknown IO failure, may be temporary. */
	ErrorIo,

	/** Destination path is too long for the file system. */
	ErrorNameTooLong,

	/** Rename of a symbolic link to a file that no longer exists. */
	ErrorNoEntity,

	/** Insufficient disk space to perform the rename. */
	ErrorNoSpace,

	/** Target is a read-only file or file system. */
	ErrorReadOnly,

	/** A non-specific error case. */
	ErrorUnknown,
};

enum class WriteResult
{
	Success,

	/** Our process does not have sufficient privileges to perform the rename. */
	ErrorAccess,

	/** Invalid file descriptor. */
	ErrorBadFileDescriptor,

	/** File is too big. */
	ErrorBigFile,

	/** Failed writing bytes due to EOF condition. */
	ErrorEOF,

	/** Open for write failed because file already exists. */
	ErrorExist,

	/** Invalid argument - typically, invalid offset into the file for the write operation. */
	ErrorInvalid,

	/** Unknown IO failure, may be temporary. */
	ErrorIo,

	/** Open for write failed because a directory exists at the given path. */
	ErrorIsDir,

	/** Open for write failed because the given path name is too long. */
	ErrorNameTooLong,

	/** System is out of internal buffer space to perform the write. */
	ErrorNoBufferSpace,

	/** Open failed becuase the given flags require an existing file but that file does not exist. */
	ErrorNoEntity,

	/** Insufficient disk space to perform the write. */
	ErrorNoSpace,

	/** Open for write failed becuase the file (or file system) is read-only. */
	ErrorReadOnly,

	/** Open for write failed because the process already has too many files opened. */
	ErrorTooManyProcess,

	/** Open for write failed because the system already has too many files opened. */
	ErrorTooManySystem,

	/** The underlying file type does not support writing. */
	ErrorWriteNotSupported,

	/** A non-specific error case. */
	ErrorUnknown,
};

/** Wrapper around the details of a memory mapped file. */
class DiskMemoryMappedFile;

/**
 * Concrete specialization of SyncFile for accessing regular
 * files on persistent storage.
 */
class DiskSyncFile : public SyncFile
{
public:
	// Attempt to open an (existing) file for read as memory mapped I/O.
	static DiskMemoryMappedFile* MemoryMapReadFile(const String& sAbsoluteFilename);
	// Attempt to open an (existing) file for write as memory mapped I/O.
	static DiskMemoryMappedFile* MemoryMapWriteFile(const String& sAbsoluteFilename, UInt64 uCapacity);
	// Accessors.
	static void const* GetMemoryMapReadPtr(DiskMemoryMappedFile const* p);
	static void* GetMemoryMapWritePtr(DiskMemoryMappedFile const* p);
	static UInt64 GetMemoryMapSize(DiskMemoryMappedFile const* p);
	// Close an existing memory map - uFinalSize only has an affect if the map was opened for write.
	static Bool CloseMemoryMap(DiskMemoryMappedFile*& rp, UInt64 uFinalSize = 0);

	// Attempt to duplicate sAbsoluteSourceFilename to sAbsoluteDestinationFilename.
	static Bool CopyFile(const String& sAbsoluteSourceFilename, const String& sAbsoluteDestinationFilename, Bool bOverwrite = false);
	static Bool CopyFile(FilePath sourceFilePath, FilePath destinationFilePath, Bool bOverwrite = false)
	{
		return CopyFile(sourceFilePath.GetAbsoluteFilename(), destinationFilePath.GetAbsoluteFilename(), bOverwrite);
	}

	// Attempt to delete sAbsoluteFilename, return true on success, false on failure.
	static Bool DeleteFile(const String& sAbsoluteFilename);
	static Bool DeleteFile(FilePath filePath)
	{
		return DeleteFile(filePath.GetAbsoluteFilename());
	}

	// Attempt to rename sAbsoluteSourceFilename to sAbsoluteDestinationFilename, return true on success, false on failure.
	static RenameResult RenameFileEx(const String& sAbsoluteSourceFilename, const String& sAbsoluteDestinationFilename);
	static RenameResult RenameFileEx(FilePath sourceFilePath, FilePath destinationFilePath)
	{
		return RenameFileEx(sourceFilePath.GetAbsoluteFilename(), destinationFilePath.GetAbsoluteFilename());
	}

	// Attempt to rename sAbsoluteSourceFilename to sAbsoluteDestinationFilename, return true on success, false on failure.
	//
	// Convenience variation of RenameFileEx() that just returns true on RenameResult::Success or false for other cases.
	static Bool RenameFile(const String& sAbsoluteSourceFilename, const String& sAbsoluteDestinationFilename)
	{
		return (RenameResult::Success == RenameFileEx(sAbsoluteSourceFilename, sAbsoluteDestinationFilename));
	}
	static Bool RenameFile(FilePath sourceFilePath, FilePath destinationFilePath)
	{
		return (RenameResult::Success == RenameFileEx(sourceFilePath.GetAbsoluteFilename(), destinationFilePath.GetAbsoluteFilename()));
	}

	// Create an empty file on disk that is marked as sparse (unused regions will contain zero
	// but may not actually use the physical disk space and generally will be faster to initialize).
	//
	// NOTE: uSizeHintInBytes is a hint - on some platforms, the file will be presized (with 0s)
	// to this size, on others, the file will be 0 sized. This size *can* cause an out-of-disk condition,
	// so it should be realistic (ideally, the max or fixed expected size of the file).
	static Bool CreateAllZeroSparseFile(const String& sAbsoluteFilename, UInt64 uSizeHintInBytes);
	static Bool CreateAllZeroSparseFile(FilePath filePath, UInt64 uSizeHintInBytes)
	{
		return CreateAllZeroSparseFile(filePath.GetAbsoluteFilename(), uSizeHintInBytes);
	}

	// On platforms that support this flag, marks a file as "do not backup".
	static Bool SetDoNotBackupFlag(const String& sAbsoluteFilename);
	static Bool SetDoNotBackupFlag(FilePath filePath)
	{
		return SetDoNotBackupFlag(filePath.GetAbsoluteFilename());
	}

	// On supported platforms, update the read-only status of a file.
	static Bool SetReadOnlyBit(const String& sAbsoluteFilename, Bool bReadOnly);
	static Bool SetReadOnlyBit(FilePath filePath, Bool bReadOnly)
	{
		return SetReadOnlyBit(filePath.GetAbsoluteFilename(), bReadOnly);
	}

	using SyncFile::ReadAll;
	using SyncFile::WriteAll;

	// Read a disk file into the given buffer of the given size.
	static Bool Read(
		const String& sAbsoluteFilename,
		void* pOutputBuffer,
		UInt32 uOutputSizeInBytes);
	static Bool Read(
		FilePath filePath,
		void* pOutputBuffer,
		UInt32 uOutputSizeInBytes)
	{
		return Read(
			filePath.GetAbsoluteFilename(),
			pOutputBuffer,
			uOutputSizeInBytes);
	}

	// Read an entire file into a new buffer, returns true on success.
	// If true, the output buffer is owned by the caller, and must be
	// deallocated with a call to MemoryManager::Deallocate().
	static Bool ReadAll(
		const String& sAbsoluteFilename,
		void*& rpOutputBuffer,
		UInt32& rzOutputSizeInBytes,
		UInt32 zAlignmentOfOutputBuffer,
		MemoryBudgets::Type eOutputBufferMemoryType,
		UInt32 zMaxReadSize = kDefaultMaxReadSize);

	// Read an entire file into a new buffer, returns true on success.
	// If true, the output buffer is owned by the caller, and must be
	// deallocated with a call to MemoryManager::Deallocate().
	static Bool ReadAll(
		FilePath filePath,
		void*& rpOutputBuffer,
		UInt32& rzOutputSizeInBytes,
		UInt32 zAlignmentOfOutputBuffer,
		MemoryBudgets::Type eOutputBufferMemoryType,
		UInt32 zMaxReadSize = kDefaultMaxReadSize)
	{
		return ReadAll(
			filePath.GetAbsoluteFilename(),
			rpOutputBuffer,
			rzOutputSizeInBytes,
			zAlignmentOfOutputBuffer,
			eOutputBufferMemoryType,
			zMaxReadSize);
	}

	// Return true if sAbsoluteFilename specifies an existing file.
	static Bool FileExists(const String& sAbsoluteFilename);
	static Bool FileExists(FilePath filePath)
	{
		return FileExists(filePath.GetAbsoluteFilename());
	}

	// Return the size of the file, or 0u if the file does not exist,
	// or is of size 0.
	static UInt64 GetFileSize(const String& sAbsoluteFilename);
	static UInt64 GetFileSize(FilePath filePath)
	{
		return GetFileSize(filePath.GetAbsoluteFilename());
	}

	// Return the last write time of the file specified by sAbsoluteFilename.
	// Returns 0u if the file does not exist.
	static UInt64 GetModifiedTime(const String& sAbsoluteFilename);
	static UInt64 GetModifiedTime(FilePath filePath)
	{
		return GetModifiedTime(filePath.GetAbsoluteFilename());
	}

	// Attempt to update the last write time of the file specified by sAbsoluteFilename.
	// Returns false if the file does not exist or cannot be updated.
	static Bool SetModifiedTime(const String& sAbsoluteFilename, UInt64 uModifiedTime);
	static Bool SetModifiedTime(FilePath filePath, UInt64 uModifiedTime)
	{
		return SetModifiedTime(filePath.GetAbsoluteFilename(), uModifiedTime);
	}

	// Write an entire file from a buffer to disk, returns true on success.
	static Bool WriteAll(
		const String& sAbsoluteFilename,
		void const* pIn,
		UInt32 uSizeInBytes);

	// Write an entire file from a buffer to disk, returns true on success.
	static Bool WriteAll(
		FilePath filePath,
		void const* pIn,
		UInt32 uSizeInBytes)
	{
		return WriteAll(
			filePath.GetAbsoluteFilename(),
			pIn,
			uSizeInBytes);
	}

	DiskSyncFile(FilePath filePath, File::Mode eMode);
	DiskSyncFile(const String& sAbsoluteFilename, File::Mode eMode);
	virtual ~DiskSyncFile();

	// Read uSizeInBytes data from the file into pOut. Returns the
	// number of bytes actually read.
	virtual UInt32 ReadRawData(void* pOut, UInt32 uSizeInBytes) SEOUL_OVERRIDE;

	// Writes uSizeInBytes data to pOut buffer from the file. Returns the
	// number of bytes actually written.
	virtual UInt32 WriteRawData(void const* pIn, UInt32 uSizeInBytes) SEOUL_OVERRIDE
	{
		auto const res = WriteRawDataEx(pIn, uSizeInBytes);
		return res.First;
	}

	// DiskSyncFile specific write extension, includes result/error information
	// on the write operation.
	Pair<UInt32, WriteResult> WriteRawDataEx(void const* pIn, UInt32 uSizeInBytes);

	/**
	 * @return An absolute filename that identifies this DiskSyncFile.
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
		return IsOpen() && File::CanRead(m_eMode);
	}

	// Returns true if this file is open and can be written to.
	virtual Bool CanWrite() const SEOUL_OVERRIDE
	{
		return IsOpen() && File::CanWrite(m_eMode);
	}

	// Commits data in any write buffers to persistent storage.
	virtual Bool Flush() SEOUL_OVERRIDE;

	// Returns true if this file is open.
	virtual Bool CanSeek() const SEOUL_OVERRIDE
	{
		return IsOpen();
	}

	// Attempt to get the current absolute file pointer position.
	virtual Bool GetCurrentPositionIndicator(Int64& riPosition) const SEOUL_OVERRIDE;

	// Return the total size of the data in this file.
	virtual UInt64 GetSize() const SEOUL_OVERRIDE;

	// Attempt a seek operation on the disk file.
	virtual Bool Seek(Int64 iPosition, File::SeekMode eMode) SEOUL_OVERRIDE;

protected:
	File::Mode m_eMode;
	Int m_iFileHandle;
	OpenResult m_eOpenResult;
	String m_sAbsoluteFilename;

	void InternalOpen();
	void InternalClose();

private:
	SEOUL_DISABLE_COPY(DiskSyncFile);
}; // class DiskSyncFile

/**
 * DiskFileSystem services file open requests for
 * files contained on persistent storage, on the current platform's
 * standard file system.
 */
class DiskFileSystem : public IFileSystem
{
public:
	DiskFileSystem();
	~DiskFileSystem();

	virtual Bool Copy(FilePath from, FilePath to, Bool bAllowOverwrite = false) SEOUL_OVERRIDE
	{
		return DiskSyncFile::CopyFile(from, to, bAllowOverwrite);
	}

	virtual Bool Copy(const String& sAbsoluteFrom, const String& sAbsoluteTo, Bool bAllowOverwrite = false) SEOUL_OVERRIDE
	{
		return DiskSyncFile::CopyFile(sAbsoluteFrom, sAbsoluteTo, bAllowOverwrite);
	}

	virtual Bool CreateDirPath(FilePath dirPath) SEOUL_OVERRIDE;
	virtual Bool CreateDirPath(const String& sAbsoluteDirPath) SEOUL_OVERRIDE;

	virtual Bool DeleteDirectory(FilePath dirPath, Bool bRecursive) SEOUL_OVERRIDE;
	virtual Bool DeleteDirectory(const String& sAbsoluteDirPath, Bool bRecursive) SEOUL_OVERRIDE;

	virtual Bool GetFileSize(
		FilePath filePath,
		UInt64& rzFileSize) const SEOUL_OVERRIDE
	{
		if (Exists(filePath))
		{
			rzFileSize = DiskSyncFile::GetFileSize(filePath);
			return true;
		}

		return false;
	}

	virtual Bool GetFileSize(
		const String& sAbsoluteFilename,
		UInt64& rzFileSize) const  SEOUL_OVERRIDE
	{
		if (Exists(sAbsoluteFilename))
		{
			rzFileSize = DiskSyncFile::GetFileSize(sAbsoluteFilename);
			return true;
		}

		return false;
	}

	virtual Bool GetModifiedTime(
		FilePath filePath,
		UInt64& ruModifiedTime) const SEOUL_OVERRIDE
	{
		UInt64 const uModifiedTime = DiskSyncFile::GetModifiedTime(filePath);
		if (0u != uModifiedTime)
		{
			ruModifiedTime = uModifiedTime;
			return true;
		}

		return false;
	}

	virtual Bool GetModifiedTime(
		const String& sAbsoluteFilename,
		UInt64& ruModifiedTime) const SEOUL_OVERRIDE
	{
		UInt64 const uModifiedTime = DiskSyncFile::GetModifiedTime(sAbsoluteFilename);
		if (0u != uModifiedTime)
		{
			ruModifiedTime = uModifiedTime;
			return true;
		}

		return false;
	}

	virtual Bool Rename(FilePath from, FilePath to) SEOUL_OVERRIDE
	{
		return DiskSyncFile::RenameFile(from, to);
	}

	virtual Bool Rename(const String& sAbsoluteFrom, const String& sAbsoluteTo) SEOUL_OVERRIDE
	{
		return DiskSyncFile::RenameFile(sAbsoluteFrom, sAbsoluteTo);
	}

	virtual Bool SetModifiedTime(
		FilePath filePath,
		UInt64 uModifiedTime) SEOUL_OVERRIDE
	{
		return DiskSyncFile::SetModifiedTime(filePath, uModifiedTime);
	}

	virtual Bool SetModifiedTime(
		const String& sAbsoluteFilename,
		UInt64 uModifiedTime) SEOUL_OVERRIDE
	{
		return DiskSyncFile::SetModifiedTime(sAbsoluteFilename, uModifiedTime);
	}

	virtual Bool SetReadOnlyBit(FilePath filePath, Bool bReadOnly) SEOUL_OVERRIDE
	{
		return DiskSyncFile::SetReadOnlyBit(filePath, bReadOnly);
	}

	virtual Bool SetReadOnlyBit(const String& sAbsoluteFilename, Bool bReadOnly) SEOUL_OVERRIDE
	{
		return DiskSyncFile::SetReadOnlyBit(sAbsoluteFilename, bReadOnly);
	}

	virtual Bool Delete(FilePath filePath) SEOUL_OVERRIDE
	{
		return DiskSyncFile::DeleteFile(filePath);
	}

	virtual Bool Delete(const String& sAbsoluteFilename) SEOUL_OVERRIDE
	{
		return DiskSyncFile::DeleteFile(sAbsoluteFilename);
	}

	virtual Bool Exists(FilePath filePath) const SEOUL_OVERRIDE;
	virtual Bool Exists(const String& sAbsoluteFilename) const SEOUL_OVERRIDE;

	virtual Bool IsDirectory(FilePath filePath) const SEOUL_OVERRIDE;
	virtual Bool IsDirectory(const String& sAbsoluteFilename) const SEOUL_OVERRIDE;

	virtual Bool Open(
		FilePath filePath,
		File::Mode eMode,
		ScopedPtr<SyncFile>& rpFile) SEOUL_OVERRIDE;

	virtual Bool Open(
		const String& sAbsoluteFilename,
		File::Mode eMode,
		ScopedPtr<SyncFile>& rpFile) SEOUL_OVERRIDE;

	virtual Bool GetDirectoryListing(
		FilePath dirPath,
		Vector<String>& rvResults,
		Bool bIncludeDirectoriesInResults = true,
		Bool bRecursive = true,
		const String& sFileExtension = String()) const SEOUL_OVERRIDE;

	virtual Bool GetDirectoryListing(
		const String& sAbsoluteDirectoryPath,
		Vector<String>& rvResults,
		Bool bIncludeDirectoriesInResults = true,
		Bool bRecursive = true,
		const String& sFileExtension = String()) const SEOUL_OVERRIDE;

	virtual Bool WriteAll(
		FilePath filePath,
		void const* pIn,
		UInt32 uSizeInBytes,
		UInt64 uModifiedTime = 0) SEOUL_OVERRIDE;
	virtual Bool WriteAll(
		const String& sAbsoluteFilename,
		void const* pIn,
		UInt32 uSizeInBytes,
		UInt64 uModifiedTime = 0) SEOUL_OVERRIDE;

private:
	SEOUL_DISABLE_COPY(DiskFileSystem);
}; // class DiskFileSystem

/**
 * Like a DiskFileSystem, but only files under certain directories can be
 * accessed for read-only.
 */
class RestrictedDiskFileSystem : public DiskFileSystem
{
public:
	RestrictedDiskFileSystem(FilePath allowedDirectoryPath, Bool bReadOnly);
	virtual ~RestrictedDiskFileSystem();

	virtual Bool Copy(FilePath from, FilePath to, Bool bAllowOverwrite = false) SEOUL_OVERRIDE;
	virtual Bool Copy(const String& sAbsoluteFrom, const String& sAbsoluteTo, Bool bAllowOverwrite = false) SEOUL_OVERRIDE;

	virtual Bool CreateDirPath(FilePath dirPath) SEOUL_OVERRIDE;
	virtual Bool CreateDirPath(const String& sAbsoluteDirPath) SEOUL_OVERRIDE;

	virtual Bool DeleteDirectory(FilePath dirPath, Bool bRecursive) SEOUL_OVERRIDE;
	virtual Bool DeleteDirectory(const String& sAbsoluteDirPath, Bool bRecursive) SEOUL_OVERRIDE;

	virtual Bool Delete(FilePath filePath) SEOUL_OVERRIDE;
	virtual Bool Delete(const String& sAbsoluteFilename) SEOUL_OVERRIDE;

	virtual Bool GetFileSize(FilePath filePath, UInt64& rzFileSize) const SEOUL_OVERRIDE;
	virtual Bool GetFileSize(const String& sAbsoluteFilename, UInt64& rzFileSize) const SEOUL_OVERRIDE;

	virtual Bool GetModifiedTime(FilePath filePath, UInt64& ruModifiedTime) const SEOUL_OVERRIDE;
	virtual Bool GetModifiedTime(const String& sAbsoluteFilename, UInt64& ruModifiedTime) const SEOUL_OVERRIDE;

	virtual Bool SetModifiedTime(FilePath filePath, UInt64 uModifiedTime) SEOUL_OVERRIDE;
	virtual Bool SetModifiedTime(const String& sAbsoluteFilename, UInt64 uModifiedTime) SEOUL_OVERRIDE;

	virtual Bool Exists(FilePath filePath) const SEOUL_OVERRIDE;
	virtual Bool Exists(const String& sAbsoluteFilename) const SEOUL_OVERRIDE;

	virtual Bool IsDirectory(FilePath filePath) const SEOUL_OVERRIDE;
	virtual Bool IsDirectory(const String& sAbsoluteFilename) const SEOUL_OVERRIDE;

	virtual Bool Rename(FilePath from, FilePath to) SEOUL_OVERRIDE;
	virtual Bool Rename(const String& sAbsoluteFrom, const String& sAbsoluteTo) SEOUL_OVERRIDE;

	virtual Bool Open(FilePath filePath, File::Mode eMode, ScopedPtr<SyncFile>& rpFile) SEOUL_OVERRIDE;
	virtual Bool Open(const String& sAbsoluteFilename, File::Mode eMode, ScopedPtr<SyncFile>& rpFile) SEOUL_OVERRIDE;

	virtual Bool GetDirectoryListing(FilePath dirPath, Vector<String>& rvResults, Bool bIncludeDirectoriesInResults = true, Bool bRecursive = true, const String& sFileExtension = String()) const SEOUL_OVERRIDE;
	virtual Bool GetDirectoryListing(const String& sAbsoluteDirectoryPath, Vector<String>& rvResults, Bool bIncludeDirectoriesInResults = true, Bool bRecursive = true, const String& sFileExtension = String()) const SEOUL_OVERRIDE;

	virtual Bool SetReadOnlyBit(FilePath filePath, Bool bReadOnly) SEOUL_OVERRIDE;
	virtual Bool SetReadOnlyBit(const String& sAbsoluteFilename, Bool bReadOnly) SEOUL_OVERRIDE;

protected:
	RestrictedDiskFileSystem(
		FilePath allowedDirectoryPath,
		String sAbsoluteAllowedDirectoryPath,
		Bool bReadOnly);

	FilePath GetAllowedDirectoryPath() const { return m_AllowedDirectoryPath; }
	Bool IsAccessible(FilePath filePath) const;
	Bool IsAccessible(const String& sAbsoluteFilename) const;
	Bool IsWriteAccessible(FilePath filePath) const;
	Bool IsWriteAccessible(const String& sAbsoluteFilename) const;

private:
	FilePath const m_AllowedDirectoryPath;
	String const m_sAbsoluteAllowedDirectoryPath;
	Bool const m_bReadOnly;

	SEOUL_DISABLE_COPY(RestrictedDiskFileSystem);
}; // class RestrictedDiskFileSystem

/**
 * Specialization of RestrictedDiskFileSystem that also remaps
 * filePaths to an alternative absolute path on disk.
 */
class RemapDiskFileSystem SEOUL_SEALED : public RestrictedDiskFileSystem
{
public:
	RemapDiskFileSystem(
		FilePath allowedDirectoryPath,
		String sAbsoluteTargetBaseDirectory,
		Bool bReadOnly);
	~RemapDiskFileSystem();

	virtual Bool Copy(FilePath from, FilePath to, Bool bAllowOverwrite = false) SEOUL_OVERRIDE;
	virtual Bool CreateDirPath(FilePath dirPath) SEOUL_OVERRIDE;
	virtual Bool DeleteDirectory(FilePath dirPath, Bool bRecursive) SEOUL_OVERRIDE;
	virtual Bool Delete(FilePath filePath) SEOUL_OVERRIDE;
	virtual Bool GetFileSize(FilePath filePath, UInt64& rzFileSize) const SEOUL_OVERRIDE;
	virtual Bool GetModifiedTime(FilePath filePath, UInt64& ruModifiedTime) const SEOUL_OVERRIDE;
	virtual Bool SetModifiedTime(FilePath filePath, UInt64 uModifiedTime) SEOUL_OVERRIDE;
	virtual Bool Exists(FilePath filePath) const SEOUL_OVERRIDE;
	virtual Bool IsDirectory(FilePath filePath) const SEOUL_OVERRIDE;
	virtual Bool Rename(FilePath from, FilePath to) SEOUL_OVERRIDE;
	virtual Bool Open(FilePath filePath, File::Mode eMode, ScopedPtr<SyncFile>& rpFile) SEOUL_OVERRIDE;
	virtual Bool GetDirectoryListing(FilePath dirPath, Vector<String>& rvResults, Bool bIncludeDirectoriesInResults = true, Bool bRecursive = true, const String& sFileExtension = String()) const SEOUL_OVERRIDE;
	virtual Bool SetReadOnlyBit(FilePath filePath, Bool bReadOnly) SEOUL_OVERRIDE;

private:
	String const m_sAbsoluteTargetBaseDirectory;

	String ResolveAbsoluteFilename(FilePath filePath) const;

	SEOUL_DISABLE_COPY(RemapDiskFileSystem);
}; // class RemapDiskFileSystem

} // namespace Seoul

#endif // include guard
