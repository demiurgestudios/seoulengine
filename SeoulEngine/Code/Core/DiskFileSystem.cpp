/**
 * \file DiskFileSystem.cpp
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

#include "Directory.h"
#include "DiskFileSystem.h"
#include "DiskFileSystemInternal.h"
#include "Logger.h"
#include "SeoulTime.h"

namespace Seoul
{

/** Attempt to open an (existing) file for read as memory mapped I/O. */
DiskMemoryMappedFile* DiskSyncFile::MemoryMapReadFile(const String& sAbsoluteFilename)
{
	return DiskFileSystemDetail::MemoryMapReadFile(sAbsoluteFilename);
}

/** Attempt to open an (existing) file for write as memory mapped I/O. */
DiskMemoryMappedFile* DiskSyncFile::MemoryMapWriteFile(const String& sAbsoluteFilename, UInt64 uCapacity)
{
	return DiskFileSystemDetail::MemoryMapWriteFile(sAbsoluteFilename, uCapacity);
}

/** Accessors. */
void const* DiskSyncFile::GetMemoryMapReadPtr(DiskMemoryMappedFile const* p)
{
	return DiskFileSystemDetail::GetMemoryMapReadPtr(p);
}

void* DiskSyncFile::GetMemoryMapWritePtr(DiskMemoryMappedFile const* p)
{
	return DiskFileSystemDetail::GetMemoryMapWritePtr(p);
}

UInt64 DiskSyncFile::GetMemoryMapSize(DiskMemoryMappedFile const* p)
{
	return DiskFileSystemDetail::GetMemoryMapSize(p);
}

/** Close an existing memory map - uFinalSize only has an affect if the map was opened for write. */
Bool DiskSyncFile::CloseMemoryMap(DiskMemoryMappedFile*& rp, UInt64 uFinalSize)
{
	return DiskFileSystemDetail::CloseMemoryMap(rp, uFinalSize);
}

/**
 * Attempt to copy sAbsoluteSourceFilename to sAbsoluteDestinationFilename on disk.
 */
Bool DiskSyncFile::CopyFile(const String& sAbsoluteSourceFilename, const String& sAbsoluteDestinationFilename, Bool bOverwrite /*= false*/)
{
	return DiskFileSystemDetail::CopyFile(sAbsoluteSourceFilename, sAbsoluteDestinationFilename, bOverwrite);
}

/**
 * Attempt to delete sAbsoluteFilename from disk.
 *
 * @return True if the file was successfully deleted, false otherwise.
 */
Bool DiskSyncFile::DeleteFile(const String& sAbsoluteFilename)
{
	return DiskFileSystemDetail::DeleteFile(sAbsoluteFilename);
}

/**
 * Attempt to rename sAbsoluteSourceFilename to sAbsoluteDestinationFilename.
 *
 * @return True if file was successfully renamed, false otherwise.
 */
RenameResult DiskSyncFile::RenameFileEx(const String& sSourceAbsoluteFilename, const String& sDestinationAbsoluteFilename)
{
	return DiskFileSystemDetail::RenameFileEx(sSourceAbsoluteFilename, sDestinationAbsoluteFilename);
}

/**
 * Create an empty file on disk that is marked as sparse (unused regions will contain zero
 * but may not actually use the physical disk space and generally will be faster to initialize).
 *
 * NOTE: uSizeHintInBytes is a hint - on some platforms, the file will be presized (with 0s)
 * to this size, on others, the file will be 0 sized. This size *can* cause an out-of-disk condition,
 * so it should be realistic (ideally, the max or fixed expected size of the file).
 */
Bool DiskSyncFile::CreateAllZeroSparseFile(const String& sAbsoluteFilename, UInt64 uSizeHintInBytes)
{
	return DiskFileSystemDetail::CreateAllZeroSparseFile(sAbsoluteFilename, uSizeHintInBytes);
}

/**
 * On platforms that support this flag, marks a file as "do not backup".
 */
Bool DiskSyncFile::SetDoNotBackupFlag(const String& sAbsoluteFilename)
{
	return DiskFileSystemDetail::SetDoNotBackupFlag(sAbsoluteFilename);
}

/**
 * On supported platforms, update the read-only status of a file.
 */
Bool DiskSyncFile::SetReadOnlyBit(const String& sAbsoluteFilename, Bool bReadOnly)
{
	return DiskFileSystemDetail::SetReadOnlyBit(sAbsoluteFilename, bReadOnly);
}

/** Read a disk file into the given buffer of the given size. */
Bool DiskSyncFile::Read(
	const String& sAbsoluteFilename,
	void* pOutputBuffer,
	UInt32 uOutputSizeInBytes)
{
	return DiskFileSystemDetail::Read(sAbsoluteFilename, pOutputBuffer, uOutputSizeInBytes);
}

/**
 * Read all the data in file sAbsoluteFilename into a new
 * buffer.
 *
 * @return True if the read succeeds, false otherwise. If this method
 * returns true, rpOutputBuffer will be a valid pointer to memory
 * that the caller must deallocate with MemoryManager::Deallocate().
 *
 * The largest file that this method can successfully read
 * must have a size in bytes <= kDefaultMaxReadSize.
 */
Bool DiskSyncFile::ReadAll(
	const String& sAbsoluteFilename,
	void*& rpOutputBuffer,
	UInt32& rzOutputSizeInBytes,
	UInt32 zAlignmentOfOutputBuffer,
	MemoryBudgets::Type eOutputBufferMemoryType,
	UInt32 zMaxReadSize)
{
	DiskSyncFile file(sAbsoluteFilename, File::kRead);

	return file.ReadAll(
		rpOutputBuffer,
		rzOutputSizeInBytes,
		zAlignmentOfOutputBuffer,
		eOutputBufferMemoryType,
		zMaxReadSize);
}

/**
 * @return True if sAbsoluteFilename is a file and that
 * file exists, false otherwise.
 */
Bool DiskSyncFile::FileExists(const String& sAbsoluteFilename)
{
	return DiskFileSystemDetail::FileExists(sAbsoluteFilename);
}

/**
 * @return The size of this file in bytes, or 0u if this
 * file does not exist.
 */
UInt64 DiskSyncFile::GetFileSize(const String& sAbsoluteFilename)
{
	return DiskFileSystemDetail::GetFileSize(sAbsoluteFilename);
}

/**
 * @return The modification time of file sAbsoluteFilename or 0u
 * if sAbsoluteFilename does not point to an existing file.
 */
UInt64 DiskSyncFile::GetModifiedTime(const String& sAbsoluteFilename)
{
	return DiskFileSystemDetail::GetModifiedTime(sAbsoluteFilename);
}

Bool DiskSyncFile::SetModifiedTime(const String& sAbsoluteFilename, UInt64 uModifiedTime)
{
	return DiskFileSystemDetail::SetModifiedTime(sAbsoluteFilename, uModifiedTime);
}

/** Write an entire file from a buffer to disk, returns true on success. */
Bool DiskSyncFile::WriteAll(
	const String& sAbsoluteFilename,
	void const* pIn,
	UInt32 uSizeInBytes)
{
	DiskSyncFile file(sAbsoluteFilename, File::kWriteTruncate);

	return file.WriteAll(pIn, uSizeInBytes);
}

DiskSyncFile::DiskSyncFile(FilePath filePath, File::Mode eMode)
	: m_eMode(eMode)
	, m_iFileHandle(-1)
	, m_eOpenResult(OpenResult::ErrorUnknown)
	, m_sAbsoluteFilename(filePath.GetAbsoluteFilename())
{
	InternalOpen();
}

DiskSyncFile::DiskSyncFile(const String& sAbsoluteFilename, File::Mode eMode)
	: m_eMode(eMode)
	, m_iFileHandle(-1)
	, m_eOpenResult(OpenResult::ErrorUnknown)
	, m_sAbsoluteFilename(sAbsoluteFilename)
{
	InternalOpen();
}

DiskSyncFile::~DiskSyncFile()
{
	InternalClose();
}

/**
 * Attempt to read uSizeInBytes raw bytes from this file into pOut.
 *
 * @return The actual number of bytes read.
 */
UInt32 DiskSyncFile::ReadRawData(void* pOut, UInt32 uSizeInBytes)
{
	if (CanRead())
	{
		return DiskFileSystemDetail::Read(m_iFileHandle, pOut, uSizeInBytes);
	}

	return 0u;
}

/** For additional WriteRawData error reporting. */
static inline WriteResult OpenResultToWriteResult(OpenResult eResult)
{
	switch (eResult)
	{
	case OpenResult::Success: return WriteResult::Success;
	case OpenResult::ErrorAccess: return WriteResult::ErrorAccess;
	case OpenResult::ErrorExist: return WriteResult::ErrorExist;
	case OpenResult::ErrorInvalid: return WriteResult::ErrorInvalid;
	case OpenResult::ErrorIo: return WriteResult::ErrorIo;
	case OpenResult::ErrorIsDir: return WriteResult::ErrorIsDir;
	case OpenResult::ErrorNameTooLong: return WriteResult::ErrorNameTooLong;
	case OpenResult::ErrorNoEntity: return WriteResult::ErrorNoEntity;
	case OpenResult::ErrorNoSpace: return WriteResult::ErrorNoSpace;
	case OpenResult::ErrorReadOnly: return WriteResult::ErrorReadOnly;
	case OpenResult::ErrorTooManyProcess: return WriteResult::ErrorTooManyProcess;
	case OpenResult::ErrorTooManySystem: return WriteResult::ErrorTooManySystem;
	case OpenResult::ErrorUnknown: return WriteResult::ErrorUnknown;
	default:
		return WriteResult::ErrorWriteNotSupported;
	};
}

/**
 * DiskSyncFile specific write extension, includes result/error information
 * on the write operation.
 *
 * Attempt to write uSizeInBytes raw bytes to this file from pIn.
 *
 * @return The number of bytes written and the result of the operation.
 */
Pair<UInt32, WriteResult> DiskSyncFile::WriteRawDataEx(void const* pIn, UInt32 uSizeInBytes)
{
	if (CanWrite())
	{
		return DiskFileSystemDetail::WriteEx(m_iFileHandle, pIn, uSizeInBytes);
	}

	// Additional specificity if not open and we have a specific open error
	// result.
	if (!IsOpen() && OpenResult::Success != m_eOpenResult)
	{
		return MakePair(0u, OpenResultToWriteResult(m_eOpenResult));
	}

	return MakePair(0u, WriteResult::ErrorWriteNotSupported);
}

/**
 * @return True if the file was successfully opened, false otherwise.
 */
Bool DiskSyncFile::IsOpen() const
{
	return (m_iFileHandle >= 0);
}

/**
 * If this file supports write operations, this will commit
 * any pending writes to permanent storage.
 */
Bool DiskSyncFile::Flush()
{
	if (CanWrite())
	{
		// Flush can be very expensive (since it blocks on the OS)
		// so we log to the file IO channel any flush calls, with
		// a timing value, to help with diagnosing problems due
		// to excessive flushing.
#if SEOUL_LOGGING_ENABLED
		auto const iStartTime = SeoulTime::GetGameTimeInTicks();
#endif // /#if SEOUL_LOGGING_ENABLED

		auto const bReturn = DiskFileSystemDetail::Flush(m_iFileHandle);

		SEOUL_LOG_FILEIO("[DiskSyncFile]: Flush %s(%.2f ms)",
			GetAbsoluteFilename().CStr(),
			SeoulTime::ConvertTicksToMilliseconds(SeoulTime::GetGameTimeInTicks() - iStartTime));

		return bReturn;
	}

	return false;
}

// Attempt to get the current absolute file pointer position.
Bool DiskSyncFile::GetCurrentPositionIndicator(Int64& riPosition) const
{
	return DiskFileSystemDetail::GetCurrentPositionIndicator(m_iFileHandle, riPosition);
}

// Return the total size of the data in this file.
UInt64 DiskSyncFile::GetSize() const
{
	return DiskFileSystemDetail::GetFileSize(m_iFileHandle);
}

/**
 * Attempt a seek operation on this DiskSyncFile.
 *
 * @return True if the seek succeeds, false otherwise. If this method
 * returns true, then the file pointer will be at the position defined
 * by iPosition and the mode eMode. Otherwise, the file position
 * is undefined.
 */
Bool DiskSyncFile::Seek(Int64 iPosition, File::SeekMode eMode)
{
	if (CanSeek())
	{
		return DiskFileSystemDetail::Seek(m_iFileHandle, iPosition, eMode);
	}

	return false;
}

/**
 * Attempt to open the file sAbsoluteFilename.
 * If this operation fails, m_iFileHandle will be an invalid handle.
 */
void DiskSyncFile::InternalOpen()
{
	InternalClose();
	m_iFileHandle = DiskFileSystemDetail::SeoulCreateFile(
		m_sAbsoluteFilename,
		m_eMode,
		m_eOpenResult);
}

/**
 * Terminate the file - any operations after this are undefined,
 * so subclasses must limit calls to their destructor.
 */
void DiskSyncFile::InternalClose()
{
	m_eOpenResult = OpenResult::ErrorUnknown;
	DiskFileSystemDetail::DestroyFile(m_iFileHandle);
	SEOUL_ASSERT(m_iFileHandle < 0);
}

DiskFileSystem::DiskFileSystem()
{
}

DiskFileSystem::~DiskFileSystem()
{
}

Bool DiskFileSystem::CreateDirPath(FilePath dirPath)
{
	return Directory::CreateDirPath(dirPath.GetAbsoluteFilename());
}

Bool DiskFileSystem::CreateDirPath(const String& sAbsoluteDirPath)
{
	return Directory::CreateDirPath(sAbsoluteDirPath);
}

Bool DiskFileSystem::DeleteDirectory(FilePath dirPath, Bool bRecursive)
{
	return Directory::Delete(dirPath.GetAbsoluteFilename(), bRecursive);
}

Bool DiskFileSystem::DeleteDirectory(const String& sAbsoluteDirPath, Bool bRecursive)
{
	return Directory::Delete(sAbsoluteDirPath, bRecursive);
}

/**
 * @return True if filePath exists on disk, false otherwise.
 */
Bool DiskFileSystem::Exists(FilePath filePath) const
{
	return DiskSyncFile::FileExists(filePath);
}

/**
 * @return True if sAbsoluteFilename exists on disk, false otherwise.
 */
Bool DiskFileSystem::Exists(const String& sAbsoluteFilename) const
{
	return DiskSyncFile::FileExists(sAbsoluteFilename);
}

/**
 * @return True if filePath exists on disk and is a directory, false
 *         otherwise.
 */
Bool DiskFileSystem::IsDirectory(FilePath filePath) const
{
	return DiskFileSystemDetail::IsDirectory(filePath.GetAbsoluteFilename());
}

/**
 * @return True if sAbsoluteFilename exists on disk and is a directory,
 *         false otherwise.
 */
Bool DiskFileSystem::IsDirectory(const String& sAbsoluteFilename) const
{
	return DiskFileSystemDetail::IsDirectory(sAbsoluteFilename);
}

/**
 * @return True if filePath can be opened with the given mode eMode.
 *
 * If this method returns true, rpFile will contain a non-null SyncFile
 * pointer and that object will return true for calls to SyncFile::IsOpen().
 * If this method returns false, rpFile is left unmodified.
 */
Bool DiskFileSystem::Open(
	FilePath filePath,
	File::Mode eMode,
	ScopedPtr<SyncFile>& rpFile)
{
	ScopedPtr<SyncFile> pFile(SEOUL_NEW(MemoryBudgets::Io) DiskSyncFile(filePath, eMode));
	if (pFile.IsValid() && pFile->IsOpen())
	{
		rpFile.Swap(pFile);
		return true;
	}

	return false;
}

/**
 * @return True if sAbsoluteFilename can be opened with the given mode eMode.
 *
 * If this method returns true, rpFile will contain a non-null SyncFile
 * pointer and that object will return true for calls to SyncFile::IsOpen().
 * If this method returns false, rpFile is left unmodified.
 */
Bool DiskFileSystem::Open(
	const String& sAbsoluteFilename,
	File::Mode eMode,
	ScopedPtr<SyncFile>& rpFile)
{
	ScopedPtr<SyncFile> pFile(SEOUL_NEW(MemoryBudgets::Io) DiskSyncFile(sAbsoluteFilename, eMode));
	if (pFile.IsValid() && pFile->IsOpen())
	{
		rpFile.Swap(pFile);
		return true;
	}

	return false;
}

/**
 * @return True if a directory list for dirPath could
 * be generated, false otherwise. If this method returns true,
 * rvResults will contain a list of files and directories that fulfill
 * the other arguments to this function. Otherwise, rvResults will
 * be left unmodified.
 */
Bool DiskFileSystem::GetDirectoryListing(
	FilePath dirPath,
	Vector<String>& rvResults,
	Bool bIncludeDirectoriesInResults,
	Bool bRecursive,
	const String& sFileExtension) const
{
	return Directory::GetDirectoryListing(
		dirPath.GetAbsoluteFilename(),
		rvResults,
		bIncludeDirectoriesInResults,
		bRecursive,
		sFileExtension);
}

/**
 * @return True if a directory list for sAbsoluteDirectoryPath could
 * be generated, false otherwise. If this method returns true,
 * rvResults will contain a list of files and directories that fulfill
 * the other arguments to this function. Otherwise, rvResults will
 * be left unmodified.
 */
Bool DiskFileSystem::GetDirectoryListing(
	const String& sAbsoluteDirectoryPath,
	Vector<String>& rvResults,
	Bool bIncludeDirectoriesInResults,
	Bool bRecursive,
	const String& sFileExtension) const
{
	return Directory::GetDirectoryListing(
		sAbsoluteDirectoryPath,
		rvResults,
		bIncludeDirectoriesInResults,
		bRecursive,
		sFileExtension);
}

Bool DiskFileSystem::WriteAll(
	FilePath filePath,
	void const* pIn,
	UInt32 uSizeInBytes,
	UInt64 uModifiedTime /*= 0*/)
{
	return WriteAll(filePath.GetAbsoluteFilename(), pIn, uSizeInBytes, uModifiedTime);
}

Bool DiskFileSystem::WriteAll(
	const String& sAbsoluteFilename,
	void const* pIn,
	UInt32 uSizeInBytes,
	UInt64 uModifiedTime /*= 0*/)
{
	// Make sure we can write to the target.
	if (!Directory::CreateDirPath(Path::GetDirectoryName(sAbsoluteFilename)))
	{
		return false;
	}

	// Commit.
	if (DiskSyncFile::WriteAll(sAbsoluteFilename, pIn, uSizeInBytes))
	{
		// If specified, set the mod time.
		if (0 != uModifiedTime)
		{
			return SetModifiedTime(sAbsoluteFilename, uModifiedTime);
		}
		else
		{
			// Otherwise, success.
			return true;
		}
	}

	return false;
}

/**
 * Need to strip a trailing slash from StripTrailingSlash, so that
 * the root directory itself is considered accessible.
 */
static inline String StripTrailingSlash(const String& s)
{
	if (s.EndsWith(Path::DirectorySeparatorChar()) || s.EndsWith(Path::AltDirectorySeparatorChar()))
	{
		String sReturn(s);
		sReturn.PopBack();
		return sReturn;
	}

	return s;
}

/**
 * RestrictedDiskFileSystem constructor
 *
 * @param[in] allowedDirectoryPath Directory under which file access will be allowed
 * @param[in] bReadOnly If true, write operations will not be allowed.
 */
RestrictedDiskFileSystem::RestrictedDiskFileSystem(
	FilePath allowedDirectoryPath,
	Bool bReadOnly)
	: m_AllowedDirectoryPath(allowedDirectoryPath)
	, m_sAbsoluteAllowedDirectoryPath(StripTrailingSlash(allowedDirectoryPath.GetAbsoluteFilename()))
	, m_bReadOnly(bReadOnly)
{
}

/**
 * RestrictedDiskFileSystem constructor
 *
 * @param[in] allowedDirectoryPath Directory under which file access will be allowed
 * @param[in] sAbsoluteAllowedDirectoryPath Typically derived from allowedDirectoryPath,
 *            subclasses are allowed to override this value.
 * @param[in] bReadOnly If true, write operations will not be allowed.
 */
RestrictedDiskFileSystem::RestrictedDiskFileSystem(
	FilePath allowedDirectoryPath,
	String sAbsoluteAllowedDirectoryPath,
	Bool bReadOnly)
	: m_AllowedDirectoryPath(allowedDirectoryPath)
	, m_sAbsoluteAllowedDirectoryPath(StripTrailingSlash(sAbsoluteAllowedDirectoryPath))
	, m_bReadOnly(bReadOnly)
{
}

/**
 * RestrictedDiskFileSystem destructor
 */
RestrictedDiskFileSystem::~RestrictedDiskFileSystem()
{
}

/**
 * Attempt to copy from -> to.
 *
 * @return true if the copy was successful,
 * false otherwise.
 */
Bool RestrictedDiskFileSystem::Copy(FilePath from, FilePath to, Bool bAllowOverwrite /*= false*/)
{
	if (IsAccessible(from) && IsWriteAccessible(to))
	{
		return DiskFileSystem::Copy(from, to, bAllowOverwrite);
	}
	else
	{
		return false;
	}
}

/**
 * Attempt to copy sAbsoluteFrom -> sAbsoluteTo.
 *
 * @return true if the copy was successful,
 * false otherwise.
 */
Bool RestrictedDiskFileSystem::Copy(const String& sAbsoluteFrom, const String& sAbsoluteTo, Bool bAllowOverwrite /*= false*/)
{
	if (IsAccessible(sAbsoluteFrom) && IsWriteAccessible(sAbsoluteTo))
	{
		return DiskFileSystem::Copy(sAbsoluteFrom, sAbsoluteTo, bAllowOverwrite);
	}
	else
	{
		return false;
	}
}

/**
 * Attempt to create directory dirPath and its parents
 *
 * @return true if the directory exists after the operation,
 * false otherwise.
 */
Bool RestrictedDiskFileSystem::CreateDirPath(FilePath dirPath)
{
	if (IsWriteAccessible(dirPath))
	{
		return DiskFileSystem::CreateDirPath(dirPath);
	}
	else
	{
		return false;
	}
}

/**
 * Attempt to create directory sAbsoluteDirPath and its parents
 *
 * @return true if the directory exists after the operation,
 * false otherwise.
 */
Bool RestrictedDiskFileSystem::CreateDirPath(const String& sAbsoluteDirPath)
{
	if (IsWriteAccessible(sAbsoluteDirPath))
	{
		return DiskFileSystem::CreateDirPath(sAbsoluteDirPath);
	}
	else
	{
		return false;
	}
}

/**
 * Attempt to delete the directory.
 * @param[in] bRecursive If true, also attempt to delete
 * any child files and directories.
 *
 * @return true if the directory exists after the operation,
 * false otherwise.
 */
Bool RestrictedDiskFileSystem::DeleteDirectory(FilePath dirPath, Bool bRecursive)
{
	if (IsWriteAccessible(dirPath))
	{
		return DiskFileSystem::DeleteDirectory(dirPath, bRecursive);
	}
	else
	{
		return false;
	}
}

/**
 * Attempt to delete the directory.
 * @param[in] bRecursive If true, also attempt to delete
 * any child files and directories.
 *
 * @return true if the directory exists after the operation,
 * false otherwise.
 */
Bool RestrictedDiskFileSystem::DeleteDirectory(const String& sAbsoluteDirPath, Bool bRecursive)
{
	if (IsWriteAccessible(sAbsoluteDirPath))
	{
		return DiskFileSystem::DeleteDirectory(sAbsoluteDirPath, bRecursive);
	}
	else
	{
		return false;
	}
}

/* Attempt to delete filePath, return true on success, false on failure. */
Bool RestrictedDiskFileSystem::Delete(FilePath filePath)
{
	if (IsWriteAccessible(filePath))
	{
		return DiskFileSystem::Delete(filePath);
	}
	else
	{
		return false;
	}
}

/* Attempt to delete sAbsoluteFilename, return true on success, false on failure. */
Bool RestrictedDiskFileSystem::Delete(const String& sAbsoluteFilename)
{
	if (IsWriteAccessible(sAbsoluteFilename))
	{
		return DiskFileSystem::Delete(sAbsoluteFilename);
	}
	else
	{
		return false;
	}
}

/**
 * Gets the size of the given file, if it exists
 */
Bool RestrictedDiskFileSystem::GetFileSize(FilePath filePath, UInt64& rzFileSize) const
{
	if (IsAccessible(filePath))
	{
		return DiskFileSystem::GetFileSize(filePath, rzFileSize);
	}
	else
	{
		return false;
	}
}

/**
 * Gets the size of the given file, if it exists
 */
Bool RestrictedDiskFileSystem::GetFileSize(const String& sAbsoluteFilename, UInt64& rzFileSize) const
{
	if (IsAccessible(sAbsoluteFilename))
	{
		return DiskFileSystem::GetFileSize(sAbsoluteFilename, rzFileSize);
	}
	else
	{
		return false;
	}
}

/**
 * Gets the file's last modification time, if it exists
 */
Bool RestrictedDiskFileSystem::GetModifiedTime(FilePath filePath, UInt64& ruModifiedTime) const
{
	if (IsAccessible(filePath))
	{
		return DiskFileSystem::GetModifiedTime(filePath, ruModifiedTime);
	}
	else
	{
		return false;
	}
}

/**
 * Gets the file's last modification time, if it exists
 */
Bool RestrictedDiskFileSystem::GetModifiedTime(const String& sAbsoluteFilename, UInt64& ruModifiedTime) const
{
	if (IsAccessible(sAbsoluteFilename))
	{
		return DiskFileSystem::GetModifiedTime(sAbsoluteFilename, ruModifiedTime);
	}
	else
	{
		return false;
	}
}

/**
 * Attempt to rename from -> to.
 *
 * @return true if the rename was successful,
 * false otherwise.
 */
Bool RestrictedDiskFileSystem::Rename(FilePath from, FilePath to)
{
	if (IsWriteAccessible(from) && IsWriteAccessible(to))
	{
		return DiskFileSystem::Rename(from, to);
	}
	else
	{
		return false;
	}
}

/**
 * Attempt to rename sAbsoluteFrom -> sAbsoluteTo.
 *
 * @return true if the rename was successful,
 * false otherwise.
 */
Bool RestrictedDiskFileSystem::Rename(const String& sAbsoluteFrom, const String& sAbsoluteTo)
{
	if (IsWriteAccessible(sAbsoluteFrom) && IsWriteAccessible(sAbsoluteTo))
	{
		return DiskFileSystem::Rename(sAbsoluteFrom, sAbsoluteTo);
	}
	else
	{
		return false;
	}
}

/**
 * Sets the file's last modification time, if it exists
 */
Bool RestrictedDiskFileSystem::SetModifiedTime(FilePath filePath, UInt64 uModifiedTime)
{
	if (IsWriteAccessible(filePath))
	{
		return DiskFileSystem::SetModifiedTime(filePath, uModifiedTime);
	}
	else
	{
		return false;
	}
}

/**
 * Sets the file's last modification time, if it exists
 */
Bool RestrictedDiskFileSystem::SetModifiedTime(const String& sAbsoluteFilename, UInt64 uModifiedTime)
{
	if (IsWriteAccessible(sAbsoluteFilename))
	{
		return DiskFileSystem::SetModifiedTime(sAbsoluteFilename, uModifiedTime);
	}
	else
	{
		return false;
	}
}

/**
 * Attempt to update the read/write status of filePath.
 *
 * @return true if the read/write change was successful,
 * false otherwise.
 */
Bool RestrictedDiskFileSystem::SetReadOnlyBit(FilePath filePath, Bool bReadOnlyBit)
{
	if (IsWriteAccessible(filePath))
	{
		return DiskFileSystem::SetReadOnlyBit(filePath, bReadOnlyBit);
	}
	else
	{
		return false;
	}
}

/**
 * Attempt to update the read/write status of sAbsoluteFilename.
 *
 * @return true if the read/write change was successful,
 * false otherwise.
 */
Bool RestrictedDiskFileSystem::SetReadOnlyBit(const String& sAbsoluteFilename, Bool bReadOnlyBit)
{
	if (IsWriteAccessible(sAbsoluteFilename))
	{
		return DiskFileSystem::SetReadOnlyBit(sAbsoluteFilename, bReadOnlyBit);
	}
	else
	{
		return false;
	}
}

/**
 * Tests if the file exists
 */
Bool RestrictedDiskFileSystem::Exists(FilePath filePath) const
{
	if (IsAccessible(filePath))
	{
		return DiskFileSystem::Exists(filePath);
	}
	else
	{
		return false;
	}
}

/**
 * Tests if the file exists
 */
Bool RestrictedDiskFileSystem::Exists(const String& sAbsoluteFilename) const
{
	if (IsAccessible(sAbsoluteFilename))
	{
		return DiskFileSystem::Exists(sAbsoluteFilename);
	}
	else
	{
		return false;
	}
}

Bool RestrictedDiskFileSystem::IsDirectory(FilePath filePath) const
{
	if (IsAccessible(filePath))
	{
		return DiskFileSystem::IsDirectory(filePath);
	}
	else
	{
		return false;
	}
}

Bool RestrictedDiskFileSystem::IsDirectory(const String& sAbsoluteFilename) const
{
	if (IsAccessible(sAbsoluteFilename))
	{
		return DiskFileSystem::IsDirectory(sAbsoluteFilename);
	}
	else
	{
		return false;
	}
}

/**
 * Opens the given file in the given mode
 */
Bool RestrictedDiskFileSystem::Open(FilePath filePath, File::Mode eMode, ScopedPtr<SyncFile>& rpFile)
{
	if ((File::kRead == eMode && IsAccessible(filePath)) ||
		(File::kRead != eMode && IsWriteAccessible(filePath)))
	{
		return DiskFileSystem::Open(filePath, eMode, rpFile);
	}
	else
	{
		return false;
	}
}

/**
 * Opens the given file in the given mode
 */
Bool RestrictedDiskFileSystem::Open(const String& sAbsoluteFilename, File::Mode eMode, ScopedPtr<SyncFile>& rpFile)
{
	if ((File::kRead == eMode && IsAccessible(sAbsoluteFilename)) ||
		(File::kRead != eMode && IsWriteAccessible(sAbsoluteFilename)))
	{
		return DiskFileSystem::Open(sAbsoluteFilename, eMode, rpFile);
	}
	else
	{
		return false;
	}
}

Bool RestrictedDiskFileSystem::GetDirectoryListing(FilePath dirPath, Vector<String>& rvResults, Bool bIncludeDirectoriesInResults /*= true*/, Bool bRecursive /*= true*/, const String& sFileExtension /*= String()*/) const
{
	if (IsAccessible(dirPath))
	{
		return DiskFileSystem::GetDirectoryListing(dirPath, rvResults, bIncludeDirectoriesInResults, bRecursive, sFileExtension);
	}
	else
	{
		return false;
	}
}

Bool RestrictedDiskFileSystem::GetDirectoryListing(const String& sAbsoluteDirectoryPath, Vector<String>& rvResults, Bool bIncludeDirectoriesInResults /*= true*/, Bool bRecursive /*= true*/, const String& sFileExtension /*= String()*/) const
{
	if (IsAccessible(sAbsoluteDirectoryPath))
	{
		return DiskFileSystem::GetDirectoryListing(sAbsoluteDirectoryPath, rvResults, bIncludeDirectoriesInResults, bRecursive, sFileExtension);
	}
	else
	{
		return false;
	}
}

/** @return True if filePath is accessible via this RestrictedDiskFileSystem, false otherwise. */
Bool RestrictedDiskFileSystem::IsAccessible(FilePath filePath) const
{
	return (
		m_AllowedDirectoryPath.GetDirectory() == filePath.GetDirectory() &&
		0 == STRNCMP_CASE_INSENSITIVE(
			filePath.GetRelativeFilenameWithoutExtension().CStr(),
			m_AllowedDirectoryPath.GetRelativeFilenameWithoutExtension().CStr(),
			m_AllowedDirectoryPath.GetRelativeFilenameWithoutExtension().GetSizeInBytes()));
}

/** @return True if sAbsoluteFilename is accessible via this RestrictedDiskFileSystem, false otherwise. */
Bool RestrictedDiskFileSystem::IsAccessible(const String& sAbsoluteFilename) const
{
	return sAbsoluteFilename.StartsWith(m_sAbsoluteAllowedDirectoryPath);
}

/** @return True if filePath is accessible via this RestrictedDiskFileSystem, and can be written to, false otherwise. */
Bool RestrictedDiskFileSystem::IsWriteAccessible(FilePath filePath) const
{
	return !m_bReadOnly && IsAccessible(filePath);
}

/** @return True if sAbsoluteFilename is accessible via this RestrictedDiskFileSystem, and can be written to, false otherwise. */
Bool RestrictedDiskFileSystem::IsWriteAccessible(const String& sAbsoluteFilename) const
{
	return !m_bReadOnly && IsAccessible(sAbsoluteFilename);
}

RemapDiskFileSystem::RemapDiskFileSystem(
	FilePath allowedDirectoryPath,
	String sAbsoluteTargetBaseDirectory,
	Bool bReadOnly)
	: RestrictedDiskFileSystem(allowedDirectoryPath, Path::GetExactPathName(Path::Combine(sAbsoluteTargetBaseDirectory, allowedDirectoryPath.GetRelativeFilename())), bReadOnly)
	, m_sAbsoluteTargetBaseDirectory(sAbsoluteTargetBaseDirectory)
{
}

RemapDiskFileSystem::~RemapDiskFileSystem()
{
}

/**
 * Attempt to copy from -> to.
 *
 * @return true if the copy was successful,
 * false otherwise.
 */
Bool RemapDiskFileSystem::Copy(FilePath from, FilePath to, Bool bAllowOverwrite /*= false*/)
{
	if (IsAccessible(from) && IsWriteAccessible(to))
	{
		return DiskFileSystem::Copy(ResolveAbsoluteFilename(from), ResolveAbsoluteFilename(to), bAllowOverwrite);
	}
	else
	{
		return false;
	}
}

/**
 * Attempt to create directory dirPath and its parents
 *
 * @return true if the directory exists after the operation,
 * false otherwise.
 */
Bool RemapDiskFileSystem::CreateDirPath(FilePath dirPath)
{
	if (IsWriteAccessible(dirPath))
	{
		return DiskFileSystem::CreateDirPath(ResolveAbsoluteFilename(dirPath));
	}
	else
	{
		return false;
	}
}

/**
 * Attempt to delete the directory.
 * @param[in] bRecursive If true, also attempt to delete
 * any child files and directories.
 *
 * @return true if the directory exists after the operation,
 * false otherwise.
 */
Bool RemapDiskFileSystem::DeleteDirectory(FilePath dirPath, Bool bRecursive)
{
	if (IsWriteAccessible(dirPath))
	{
		return DiskFileSystem::DeleteDirectory(ResolveAbsoluteFilename(dirPath), bRecursive);
	}
	else
	{
		return false;
	}
}

/* Attempt to delete filePath, return true on success, false on failure. */
Bool RemapDiskFileSystem::Delete(FilePath filePath)
{
	if (IsWriteAccessible(filePath))
	{
		return DiskFileSystem::Delete(ResolveAbsoluteFilename(filePath));
	}
	else
	{
		return false;
	}
}

/**
 * Gets the size of the given file, if it exists
 */
Bool RemapDiskFileSystem::GetFileSize(FilePath filePath, UInt64& rzFileSize) const
{
	if (IsAccessible(filePath))
	{
		return DiskFileSystem::GetFileSize(ResolveAbsoluteFilename(filePath), rzFileSize);
	}
	else
	{
		return false;
	}
}

/**
 * Gets the file's last modification time, if it exists
 */
Bool RemapDiskFileSystem::GetModifiedTime(FilePath filePath, UInt64& ruModifiedTime) const
{
	if (IsAccessible(filePath))
	{
		return DiskFileSystem::GetModifiedTime(ResolveAbsoluteFilename(filePath), ruModifiedTime);
	}
	else
	{
		return false;
	}
}

/**
 * Attempt to rename from -> to.
 *
 * @return true if the rename was successful,
 * false otherwise.
 */
Bool RemapDiskFileSystem::Rename(FilePath from, FilePath to)
{
	if (IsWriteAccessible(from) && IsWriteAccessible(to))
	{
		return DiskFileSystem::Rename(ResolveAbsoluteFilename(from), ResolveAbsoluteFilename(to));
	}
	else
	{
		return false;
	}
}

/**
 * Sets the file's last modification time, if it exists
 */
Bool RemapDiskFileSystem::SetModifiedTime(FilePath filePath, UInt64 uModifiedTime)
{
	if (IsWriteAccessible(filePath))
	{
		return DiskFileSystem::SetModifiedTime(ResolveAbsoluteFilename(filePath), uModifiedTime);
	}
	else
	{
		return false;
	}
}

/**
 * Attempt to update the read/write status of filePath.
 *
 * @return true if the read/write change was successful,
 * false otherwise.
 */
Bool RemapDiskFileSystem::SetReadOnlyBit(FilePath filePath, Bool bReadOnlyBit)
{
	if (IsWriteAccessible(filePath))
	{
		return DiskFileSystem::SetReadOnlyBit(ResolveAbsoluteFilename(filePath), bReadOnlyBit);
	}
	else
	{
		return false;
	}
}

/**
 * Tests if the file exists
 */
Bool RemapDiskFileSystem::Exists(FilePath filePath) const
{
	if (IsAccessible(filePath))
	{
		return DiskFileSystem::Exists(ResolveAbsoluteFilename(filePath));
	}
	else
	{
		return false;
	}
}

Bool RemapDiskFileSystem::IsDirectory(FilePath filePath) const
{
	if (IsAccessible(filePath))
	{
		return DiskFileSystem::IsDirectory(ResolveAbsoluteFilename(filePath));
	}
	else
	{
		return false;
	}
}

/**
 * Opens the given file in the given mode
 */
Bool RemapDiskFileSystem::Open(FilePath filePath, File::Mode eMode, ScopedPtr<SyncFile>& rpFile)
{
	if ((File::kRead == eMode && IsAccessible(filePath)) ||
		(File::kRead != eMode && IsWriteAccessible(filePath)))
	{
		return DiskFileSystem::Open(ResolveAbsoluteFilename(filePath), eMode, rpFile);
	}
	else
	{
		return false;
	}
}

Bool RemapDiskFileSystem::GetDirectoryListing(FilePath dirPath, Vector<String>& rvResults, Bool bIncludeDirectoriesInResults /*= true*/, Bool bRecursive /*= true*/, const String& sFileExtension /*= String()*/) const
{
	if (IsAccessible(dirPath))
	{
		// If listing succeeds, we need to renormalize the paths to the
		// path we're remapping from.
		if (DiskFileSystem::GetDirectoryListing(ResolveAbsoluteFilename(dirPath), rvResults, bIncludeDirectoriesInResults, bRecursive, sFileExtension))
		{
			auto const sNewBase(GetAllowedDirectoryPath().GetAbsoluteFilename());
			UInt32 uToTrim = m_sAbsoluteTargetBaseDirectory.GetSize();
			if (!m_sAbsoluteTargetBaseDirectory.EndsWith(Path::DirectorySeparatorChar()))
			{
				++uToTrim;
			}

			for (auto& rs : rvResults)
			{
				rs = Path::Combine(sNewBase, rs.Substring(uToTrim));
			}

			return true;
		}

		return false;
	}
	else
	{
		return false;
	}
}

/**
 * For FilePath entries, remaps to the alternative absolute path.
 */
String RemapDiskFileSystem::ResolveAbsoluteFilename(FilePath filePath) const
{
	return Path::Combine(m_sAbsoluteTargetBaseDirectory, filePath.GetRelativeFilename());
}

} // namespace Seoul
