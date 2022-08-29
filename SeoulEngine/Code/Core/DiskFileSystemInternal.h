/**
 * \file DiskFileSystemInternal.h
 * \brief Internal header file included by DiskFileSystem.cpp. Do not
 * include in other header files.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#if !defined(DISK_FILE_SYSTEM_H)
#error "Internal header file, must only be included by DiskFileSystem.cpp"
#endif

#include "Path.h"
#include "Platform.h"
#include "ScopedAction.h"
#include "Vector.h"

#include "stdio.h"
#include "string.h"

#if SEOUL_PLATFORM_WINDOWS
#include <fcntl.h>
#include <io.h>
#include <share.h>
#include <sys/stat.h>
#include <sys/utime.h>

#ifndef FSCTL_SET_SPARSE
#	define FSCTL_SET_SPARSE (0x900C4)
#endif

#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
#include <fcntl.h>
#if SEOUL_PLATFORM_IOS
#include <sys/clonefile.h>
#else
#include <sys/sendfile.h>
#endif
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <utime.h>
#endif

namespace Seoul
{

#if SEOUL_PLATFORM_IOS
Bool IOSSetDoNotBackupFlag(const String& sAbsoluteFilename);
#endif

#if SEOUL_PLATFORM_WINDOWS
/**
 * Wrapper around the details necessary for a memory mapped
 * file on the PC platform.
 */
class DiskMemoryMappedFile SEOUL_SEALED
{
public:
	DiskMemoryMappedFile(HANDLE hFile, HANDLE hMapping, LARGE_INTEGER sz, void* p, Bool bWritable)
		: m_hFile(hFile)
		, m_hMapping(hMapping)
		, m_sz(sz)
		, m_p(p)
		, m_bWritable(bWritable)
	{
	}

	~DiskMemoryMappedFile()
	{
		// Cleanup - must have been opened successfully
		// if we were constructed so must closed successfully.
		SEOUL_VERIFY(FALSE != ::CloseHandle(m_hMapping));
		SEOUL_VERIFY(FALSE != ::CloseHandle(m_hFile));
	}

	// Accessors.
	HANDLE GetFile() const { return m_hFile; }
	void* GetPtr() const { return m_p; }
	UInt64 GetSize() const { return m_sz.QuadPart; }
	Bool IsWritable() const { return m_bWritable; }

private:
	SEOUL_DISABLE_COPY(DiskMemoryMappedFile);

	HANDLE const m_hFile;
	HANDLE const m_hMapping;
	LARGE_INTEGER const m_sz;
	void* const m_p;
	Bool const m_bWritable;
};
#endif // /SEOUL_PLATFORM_WINDOWS

namespace DiskFileSystemDetail
{

//
// Android, iOS, Linux, and PC common implementations
//
#if SEOUL_PLATFORM_WINDOWS || SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX

#if SEOUL_PLATFORM_WINDOWS
#define SEOUL_CLOSE _close
// See: http://grokbase.com/t/postgresql/pgsql-hackers/0323970k9k/win32-and-fsync
#define SEOUL_FSYNC _commit
#define SEOUL_READ _read
#define SEOUL_WRITE _write

static const Int kCopyNoOverwriteMode = (_O_WRONLY | _O_CREAT | _O_TRUNC | _O_EXCL);
static const Int kCopyWithOverwriteMode = (_O_WRONLY | _O_CREAT | _O_TRUNC);

inline Int ToFileOpenFlags(File::Mode eMode)
{
	switch (eMode)
	{
	case File::kRead:
		return _O_BINARY | _O_RDONLY;
	case File::kWriteTruncate:
		return _O_BINARY | _O_CREAT | _O_TRUNC | _O_WRONLY;
	case File::kWriteAppend:
		return _O_BINARY | _O_APPEND | _O_WRONLY;
	case File::kReadWrite:
		return _O_BINARY | _O_CREAT | _O_RDWR;
	default:
		SEOUL_FAIL("ToFileMode: Out-of-date enum, bug a programmer.");
		return 0;
	};
}

inline Int ToFileModeFlags(File::Mode eMode)
{
	switch (eMode)
	{
	case File::kRead:
		return _S_IREAD;
	case File::kWriteTruncate: // fall-through
	case File::kWriteAppend: // fall-through
	case File::kReadWrite:
		return _S_IREAD | _S_IWRITE;
	default:
		SEOUL_FAIL("ToFileMode: Out-of-date enum, bug a programmer.");
		return 0;
	};
}

#else
#define SEOUL_CLOSE close
#define SEOUL_FSYNC fsync
#define SEOUL_READ read
#define SEOUL_WRITE write

static const Int kCopyNoOverwriteMode = (O_WRONLY | O_CREAT | O_TRUNC | O_EXCL);
static const Int kCopyWithOverwriteMode = (O_WRONLY | O_CREAT | O_TRUNC);

// lseek is always 64-bit on iOS and there is
// no "lseek64".
#if SEOUL_PLATFORM_IOS
	SEOUL_STATIC_ASSERT(sizeof(off_t) == 8);
#	define SEOUL_SEEK lseek
#else
#	define SEOUL_SEEK lseek64
#endif // /#if !SEOUL_PLATFORM_IOS

inline Int ToFileOpenFlags(File::Mode eMode)
{
	switch (eMode)
	{
	case File::kRead:
		return O_RDONLY;
	case File::kWriteTruncate:
		return O_CREAT | O_TRUNC | O_WRONLY;
	case File::kWriteAppend:
		return O_APPEND | O_WRONLY;
	case File::kReadWrite:
		return O_CREAT | O_RDWR;
	default:
		SEOUL_FAIL("ToFileMode: Out-of-date enum, bug a programmer.");
		return 0;
	};
}

inline Int ToFileModeFlags(File::Mode eMode)
{
	switch (eMode)
	{
	case File::kRead:
		return S_IRUSR;
	case File::kWriteTruncate: // fall-through
	case File::kWriteAppend: // fall-through
	case File::kReadWrite:
		return S_IRUSR | S_IWUSR;
	default:
		SEOUL_FAIL("ToFileMode: Out-of-date enum, bug a programmer.");
		return 0;
	};
}

#endif

inline void DestroyFile(Int& riHandle)
{
	if (riHandle >= 0)
	{
		(void)SEOUL_CLOSE(riHandle);
		riHandle = -1;
	}
}

inline Bool Flush(Int iHandle)
{
	// Handle for the EINTR case, which is temporary.
	while (true)
	{
		auto const iResult = SEOUL_FSYNC(iHandle);
		if (iResult < 0)
		{
			// Try again on EINTR.
			if (EINTR == errno)
			{
				continue;
			}
		}

		// Return result.
		return (0 == iResult);
	}
}

inline UInt32 Read(Int iHandle, void* pOut, UInt32 uSizeInBytes)
{
	// Edge case, return read of 0 immediately.
	if (iHandle < 0) { return 0u; }
	// Edge case, return read of 0 immediately.
	if (nullptr == pOut) { return 0u; }

	// "Robust" read (derived from implementation in sqlite). Filter
	// interrupts since they are fundamentally temporary and also
	// issue (potentially) multiple reads to deal with "partial" interrupts.
	UInt32 uReadInBytes = 0u;
	while (uSizeInBytes > 0u)
	{
		auto const iReturn = SEOUL_READ(iHandle, pOut, uSizeInBytes);
		if (iReturn < 0)
		{
			// Try again on EINTR.
			if (EINTR == errno)
			{
				continue;
			}
			else
			{
				// Immediately return 0.
				return 0u;
			}
		}
		// Accumulate into uReadInBytes.
		else
		{
			// Sanity check - if iReturn is somehow
			// greater than uSizeInBytes, it means
			// that the read function introduced a
			// buffer overflow.
			SEOUL_ASSERT((UInt32)iReturn <= uSizeInBytes);

			// Accumulate, adjust size and output pointer.
			uReadInBytes += (UInt32)iReturn;
			auto const uAdjust = Min((UInt32)iReturn, uSizeInBytes);
			uSizeInBytes -= uAdjust;
			pOut = (void*)((Byte*)pOut + (size_t)uAdjust);

			// If we read 0 bytes, break immediately, as this indicates
			// EOF.
			if (0 == iReturn)
			{
				break;
			}

			// Otherwise, continue looping until we've read the target of uSizeInBytes.
		}
	}

	return uReadInBytes;
}

static inline OpenResult ConvertToOpenResult(Int iErrno)
{
	switch (iErrno)
	{
	case EACCES: return OpenResult::ErrorAccess;
	case EEXIST: return OpenResult::ErrorExist;
	case EINVAL: return OpenResult::ErrorInvalid;
	case EIO: return OpenResult::ErrorIo;
	case EISDIR: return OpenResult::ErrorIsDir;
	case ENAMETOOLONG: return OpenResult::ErrorNameTooLong;
	case ENOENT: return OpenResult::ErrorNoEntity;
	case ENOSPC: return OpenResult::ErrorNoSpace;
	case EROFS: return OpenResult::ErrorReadOnly;
	case EMFILE: return OpenResult::ErrorTooManyProcess;
	case ENFILE: return OpenResult::ErrorTooManySystem;
	default:
		return OpenResult::ErrorUnknown;
	};
}

static inline WriteResult ConvertToWriteResult(Int iErrno)
{
	switch (iErrno)
	{
	case EACCES: return WriteResult::ErrorAccess; // A write was attempted on a socket and the calling process does not have appropriate privileges.
	case EBADF: return WriteResult::ErrorBadFileDescriptor; // The fildes argument is not a valid file descriptor open for writing.
	case EFBIG: return WriteResult::ErrorBigFile; // An attempt was made to write a file that exceeds the implementation - defined maximum file size[XSI][Option Start] or the process' file size limit, [Option End]  and there was no room for any bytes to be written.
	case EINVAL: return WriteResult::ErrorInvalid; // The offset argument is invalid.The value is negative.[Option End]
	case EIO: return WriteResult::ErrorIo; // A physical I / O error has occurred.
	case ENOBUFS: return WriteResult::ErrorNoBufferSpace; // Insufficient resources were available in the system to perform the operation.
	case ENOSPC: return WriteResult::ErrorNoSpace; // There was no free space remaining on the device containing the file.
	case ENXIO: return WriteResult::ErrorIo; // A request was made of a nonexistent device, or the request was outside the capabilities of the device.

		// All map to "error unknown" since we don't expect these results.
	case EAGAIN: // The O_NONBLOCK flag is set for the file descriptor and the thread would be delayed in the write() operation.
	case ECONNRESET: // A write was attempted on a socket that is not connected.
	case EINTR: // The write operation was terminated due to the receipt of a signal, and no data was transferred.
	case ENETDOWN: // A write was attempted on a socket and the local network interface used to reach the destination is down.
	case ENETUNREACH: // A write was attempted on a socket and no route to the network is present.
	case EPIPE: // An attempt is made to write to a pipe or FIFO that is not open for reading by any process, or that only has one end open.A SIGPIPE signal shall also be sent to the thread.
	case ERANGE: // The transfer request size was outside the range supported by the STREAMS file associated with fildes.[Option End]
	case ESPIPE: // fildes is associated with a pipe or FIFO.[Option End]
	default:
		return WriteResult::ErrorUnknown;
	}
}

inline Pair<UInt32, WriteResult> WriteEx(Int iHandle, void const* pIn, UInt32 uSizeInBytes)
{
	// Edge case, return write of 0 immediately.
	if (iHandle < 0) { return MakePair(0u, WriteResult::ErrorInvalid); }
	// Edge case, return write of 0 immediately.
	if (nullptr == pIn) { return MakePair(0u, WriteResult::ErrorInvalid); }

	// "Robust" write (derived from implementation in sqlite). Filter
	// interrupts since they are fundamentally temporary and also
	// issue (potentially) multiple reads to deal with "partial" interrupts.
	UInt32 uWriteInBytes = 0u;
	while (uSizeInBytes > 0u)
	{
		auto const iReturn = SEOUL_WRITE(iHandle, pIn, uSizeInBytes);
		if (iReturn < 0)
		{
			// Cache result.
			auto const eErrorResult = errno;

			// Try again on EINTR.
			if (EINTR == eErrorResult)
			{
				continue;
			}
			else
			{
				// Immediately return 0 with converted error code.
				return MakePair(0u, ConvertToWriteResult(eErrorResult));
			}
		}
		// Accumulate into uWriteInBytes.
		else
		{
			// Sanity check - if iReturn is somehow
			// greater than uSizeInBytes, it means
			// that the write function introduced a
			// buffer overflow.
			SEOUL_ASSERT((UInt32)iReturn <= uSizeInBytes);

			// Accumulate, adjust size and input pointer.
			uWriteInBytes += (UInt32)iReturn;
			auto const uAdjust = Min((UInt32)iReturn, uSizeInBytes);
			uSizeInBytes -= uAdjust;
			pIn = (void*)((Byte*)pIn + (size_t)uAdjust);

			// If we wrote 0 bytes, break immediately, as this indicates
			// EOF.
			if (0 == iReturn)
			{
				return MakePair(uWriteInBytes, WriteResult::ErrorEOF);
			}

			// Otherwise, continue looping until we've written the target of uSizeInBytes.
		}
	}

	// Done, consider a success whether the total number was written or not.
	return MakePair(uWriteInBytes, WriteResult::Success);
}

inline Int ToSeekMode(File::SeekMode eMode)
{
	switch (eMode)
	{
	case File::kSeekFromCurrent: return SEEK_CUR;
	case File::kSeekFromEnd: return SEEK_END;
	case File::kSeekFromStart: return SEEK_SET;
	default:
		SEOUL_FAIL("Invalid seek mode.");
		return SEEK_CUR;
	};
}
#else
#error "Define for this platform."
#endif

//
// PC implementations
//
#if SEOUL_PLATFORM_WINDOWS
inline Int ToShareMode(File::Mode eMode)
{
	switch (eMode)
	{
		// Allow all sharing when we only care about reading.
	case File::kRead:
		return _SH_DENYNO;

		// Disallow writing when we care about writing.
	case File::kReadWrite: // fall-through
	case File::kWriteAppend: // fall-through
	case File::kWriteTruncate:
		return _SH_DENYWR;

	default:
		SEOUL_FAIL("Out-of-sync enum.");
		return _SH_DENYNO;
	}
}

inline DiskMemoryMappedFile* MemoryMapReadFile(const String& sAbsoluteFilename)
{
	// Open file.
	auto const hFile = ::CreateFileW(
		sAbsoluteFilename.WStr(),
		GENERIC_READ,
		FILE_SHARE_READ,
		nullptr,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		nullptr);
	if (nullptr == hFile || INVALID_HANDLE_VALUE == hFile)
	{
		return nullptr;
	}

	// Get size.
	LARGE_INTEGER sz;
	memset(&sz, 0, sizeof(sz));
	if (FALSE == ::GetFileSizeEx(hFile, &sz))
	{
		SEOUL_VERIFY(FALSE != ::CloseHandle(hFile));
		return nullptr;
	}

	// Vmem mapping to the current size.
	auto const hMapping = ::CreateFileMappingW(
		hFile,
		nullptr,
		PAGE_READONLY,
		sz.HighPart,
		sz.LowPart,
		nullptr);
	if (nullptr == hMapping || INVALID_HANDLE_VALUE == hMapping)
	{
		SEOUL_VERIFY(FALSE != ::CloseHandle(hFile));
		return nullptr;
	}

	// Pointer to data.
	void* p = ::MapViewOfFile(
		hMapping,
		FILE_MAP_READ,
		0,
		0,
		(SIZE_T)sz.QuadPart);
	if (nullptr == p)
	{
		SEOUL_VERIFY(FALSE != ::CloseHandle(hMapping));
		SEOUL_VERIFY(FALSE != ::CloseHandle(hFile));
		return nullptr;
	}

	// Configure return and complete.
	auto pReturn = SEOUL_NEW(MemoryBudgets::Io) DiskMemoryMappedFile(
		hFile,
		hMapping,
		sz,
		p,
		false);
	return pReturn;
}

inline DiskMemoryMappedFile* MemoryMapWriteFile(const String& sAbsoluteFilename, UInt64 uCapacity)
{
	// Make sure the directory exists.
	if (!Directory::CreateDirPath(Path::GetDirectoryName(sAbsoluteFilename)))
	{
		return nullptr;
	}

	// Size out the file and mark it as sparse (so the sizing doesn't take forever).
	if (!DiskSyncFile::CreateAllZeroSparseFile(sAbsoluteFilename, uCapacity))
	{
		return nullptr;
	}

	// Open file - existing, since we created it with the previous operation.
	auto const hFile = ::CreateFileW(
		sAbsoluteFilename.WStr(),
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ,
		nullptr,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		nullptr);
	if (nullptr == hFile || INVALID_HANDLE_VALUE == hFile)
	{
		return nullptr;
	}

	// Setup size using the given capacity.
	LARGE_INTEGER sz;
	memset(&sz, 0, sizeof(sz));
	sz.QuadPart = uCapacity;

	// Vmem mapping.
	auto const hMapping = ::CreateFileMappingW(
		hFile,
		nullptr,
		PAGE_READWRITE,
		sz.HighPart,
		sz.LowPart,
		nullptr);
	if (nullptr == hMapping || INVALID_HANDLE_VALUE == hMapping)
	{
		SEOUL_VERIFY(FALSE != ::CloseHandle(hFile));
		return nullptr;
	}

	// Pointer to data.
	void* p = ::MapViewOfFile(
		hMapping,
		FILE_MAP_WRITE,
		0,
		0,
		(SIZE_T)sz.QuadPart);
	if (nullptr == p)
	{
		SEOUL_VERIFY(FALSE != ::CloseHandle(hMapping));
		SEOUL_VERIFY(FALSE != ::CloseHandle(hFile));
		return nullptr;
	}

	// Configure return and complete.
	auto pReturn = SEOUL_NEW(MemoryBudgets::Io) DiskMemoryMappedFile(
		hFile,
		hMapping,
		sz,
		p,
		true);
	return pReturn;
}

// Accessors.
inline void const* GetMemoryMapReadPtr(DiskMemoryMappedFile const* p)
{
	return p->GetPtr();
}

inline void* GetMemoryMapWritePtr(DiskMemoryMappedFile const* p)
{
	return p->IsWritable() ? p->GetPtr() : nullptr;
}

inline UInt64 GetMemoryMapSize(DiskMemoryMappedFile const* p)
{
	return p->GetSize();
}

inline Bool CloseMemoryMap(DiskMemoryMappedFile*& rp, UInt64 uFinalSize)
{
	auto p = rp;
	rp = nullptr;

	// Done.
	if (nullptr == p)
	{
		return true;
	}

	// Apply final size if the mapping was opened with write privileges.
	Bool bReturn = true;
	if (p->IsWritable())
	{
		auto const hFile = p->GetFile();

		// Truncate the file to the final size.
		LARGE_INTEGER offset;
		memset(&offset, 0, sizeof(offset));
		offset.QuadPart = uFinalSize;
		bReturn = bReturn && (FALSE != ::SetFilePointerEx(hFile, offset, nullptr, FILE_BEGIN));
		bReturn = bReturn && (FALSE != ::SetEndOfFile(hFile));
	}

	// Done, destroy the mapping.
	SafeDelete(p);
	return bReturn;
}

inline Bool CopyFile(const String& sSourceAbsoluteFilename, const String& sDestinationAbsoluteFilename, Bool bOverwrite)
{
	return (FALSE != ::CopyFileW(sSourceAbsoluteFilename.WStr(), sDestinationAbsoluteFilename.WStr(), bOverwrite ? FALSE : TRUE));
}

inline Bool DeleteFile(const String& sAbsoluteFilename)
{
	return (FALSE != ::DeleteFileW(sAbsoluteFilename.WStr()));
}

inline RenameResult RenameFileEx(const String& sSourceAbsoluteFilename, const String& sDestinationAbsoluteFilename)
{
	auto const bResult = (FALSE != ::MoveFileW(sSourceAbsoluteFilename.WStr(), sDestinationAbsoluteFilename.WStr()));
	if (bResult)
	{
		return RenameResult::Success;
	}

	// TODO: Fortunately we're distinguishing errors due to failures on mobile where the
	// error codes are clearly defined. Windows is "who knows" - its's just the giant bucket
	// of codes possible (and then some) from ::GetLastError().
	return RenameResult::ErrorUnknown;
}

inline Bool CreateAllZeroSparseFile(const String& sAbsoluteFilename, UInt64 uSizeHintInBytes)
{
	// Create the file.
	auto hFile = ::CreateFileW(
		sAbsoluteFilename.WStr(),
		GENERIC_WRITE,
		0,
		nullptr,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		nullptr);
	if (nullptr == hFile || INVALID_HANDLE_VALUE == hFile)
	{
		// Try again with CREATE_NEW if error is ERROR_FILE_NOT_FOUND
		if (ERROR_FILE_NOT_FOUND == ::GetLastError())
		{
			hFile = ::CreateFileW(
				sAbsoluteFilename.WStr(),
				GENERIC_WRITE,
				0,
				nullptr,
				CREATE_NEW,
				FILE_ATTRIBUTE_NORMAL,
				nullptr);
		}

		if (nullptr == hFile || INVALID_HANDLE_VALUE == hFile)
		{
			return false;
		}
	}

	// Make sure we close on exit.
	auto const deferred(MakeDeferredAction([&]() { SEOUL_VERIFY(FALSE != ::CloseHandle(hFile)); }));

	// Mark it as sparse.
	DWORD uUnused = 0u;
	if (FALSE == ::DeviceIoControl(hFile, FSCTL_SET_SPARSE, nullptr, 0, nullptr, 0, &uUnused, nullptr))
	{
		return false;
	}

	// Size it to the hint.
	LARGE_INTEGER offset;
	memset(&offset, 0, sizeof(offset));
	offset.QuadPart = uSizeHintInBytes;
	if (FALSE == ::SetFilePointerEx(hFile, offset, nullptr, FILE_BEGIN))
	{
		return false;
	}
	if (FALSE == ::SetEndOfFile(hFile))
	{
		return false;
	}

	// Done, success.
	return true;
}

inline Bool SetDoNotBackupFlag(const String& sAbsoluteFilename)
{
	// No such flag on these platforms.
	return false;
}

inline Bool SetReadOnlyBit(const String& sAbsoluteFilename, Bool bReadOnly)
{
	auto const wideFilename(sAbsoluteFilename.WStr());
	DWORD uAttributes = ::GetFileAttributesW(wideFilename);
	if (bReadOnly)
	{
		uAttributes |= FILE_ATTRIBUTE_READONLY;
	}
	else
	{
		uAttributes &= ~FILE_ATTRIBUTE_READONLY;
	}

	return (FALSE != ::SetFileAttributesW(wideFilename, uAttributes));
}

inline Int SeoulCreateFile(const String& sAbsoluteFilename, File::Mode eMode, OpenResult& reResult)
{
	// TODO: Surface sharing on all platforms.

	// Allow shared read-write access to the file.
	Int iReturn = -1;
	auto const eResult = _wsopen_s(
		&iReturn,
		sAbsoluteFilename.WStr(),
		ToFileOpenFlags(eMode),
		ToShareMode(eMode),
		ToFileModeFlags(eMode)); // File will be created as readable and writable.

	if (0 == eResult && iReturn >= 0)
	{
		reResult = OpenResult::Success;
		return iReturn;
	}
	else
	{
		reResult = ConvertToOpenResult(eResult);
		return -1;
	}
}

inline Bool FileExists(const String& sAbsoluteFilename)
{
	struct _stati64 statResults;
	memset(&statResults, 0, sizeof(statResults));

	if (0 == _wstati64(sAbsoluteFilename.WStr(), &statResults))
	{
		// Check to make sure that we didn't stat a directory
		//    because stat works on files and file systems (i.e. the root directory)
		return (0 == (statResults.st_mode & _S_IFDIR));
	}
	else
	{
		return false;
	}
}

inline Bool IsDirectory(const String& sAbsoluteFilename)
{
	DWORD uAttributes = ::GetFileAttributesW(sAbsoluteFilename.WStr());
	return (uAttributes != INVALID_FILE_ATTRIBUTES &&
			(uAttributes & FILE_ATTRIBUTE_DIRECTORY));
}

inline UInt64 GetFileSize(const String& sAbsoluteFilename)
{
	WIN32_FILE_ATTRIBUTE_DATA res;
	memset(&res, 0, sizeof(res));

	// Failure, return 0 size.
	if (FALSE == ::GetFileAttributesExW(sAbsoluteFilename.WStr(), GetFileExInfoStandard, &res))
	{
		return 0;
	}

	return (UInt64)res.nFileSizeLow | ((UInt64)res.nFileSizeHigh << (UInt64)32);
}

inline UInt64 GetModifiedTime(const String& sAbsoluteFilename)
{
	struct _stati64 statResults;
	memset(&statResults, 0, sizeof(statResults));

	if (0 == _wstati64(sAbsoluteFilename.WStr(), &statResults))
	{
		return (UInt64)statResults.st_mtime;
	}
	else
	{
		return 0u;
	}
}

inline Bool SetModifiedTime(const String& sAbsoluteFilename, UInt64 uModifiedTime)
{
	struct __utimbuf64 fileTimes;
	memset(&fileTimes, 0, sizeof(fileTimes));

	fileTimes.actime = uModifiedTime;
	fileTimes.modtime = uModifiedTime;

	return (0 == _wutime64(sAbsoluteFilename.WStr(), &fileTimes));
}

inline Bool GetCurrentPositionIndicator(Int iHandle, Int64& riPosition)
{
	if (iHandle >= 0)
	{
		riPosition = _telli64(iHandle);
		return true;
	}

	return false;
}

inline UInt64 GetFileSize(Int iHandle)
{
	// Edge case.
	if (iHandle < 0) { return 0; }

	// Capture current position - if this fails, size query
	// also fails.
	Int64 const iStart = _telli64(iHandle);
	if (iStart < 0) { return 0; }

	// Seek to end to get size - if this fails, size query
	// also fails.
	Int64 const iSize = _lseeki64(iHandle, 0, SEEK_END);
	if (iSize < 0) { return 0; }

	// Restore original position - this must always succeed or
	// we're screwed.
	SEOUL_VERIFY(iStart == _lseeki64(iHandle, iStart, SEEK_SET));

	// Normalize to 0 on failure.
	return (iSize < 0 ? 0 : (UInt64)iSize);
}

inline Bool Seek(Int iHandle, Int64 iPosition, File::SeekMode eMode)
{
	if (iHandle >= 0)
	{
		return (_lseeki64(iHandle, iPosition, ToSeekMode(eMode)) >= 0);
	}

	return false;
}

inline Bool Read(
	const String& sAbsoluteFilename,
	void* pOutputBuffer,
	UInt32 uOutputSizeInBytes)
{
	// Open the file for read.
	auto const hFile = ::CreateFileW(
		sAbsoluteFilename.WStr(),
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		nullptr,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		nullptr);
	if (INVALID_HANDLE_VALUE == hFile) { return false; }

	// Loop and read the target size.
	DWORD uToRead = (DWORD)uOutputSizeInBytes;
	Byte* pOut = (Byte*)pOutputBuffer;
	while (uToRead > 0u)
	{
		DWORD uRead = 0u;
		if (FALSE == ::ReadFile(
			hFile,
			pOut,
			uToRead,
			&uRead,
			nullptr) || 0u == uRead)
		{
			break;
		}

		// Adjust and (potentially) loop.
		DWORD const uAdjust = Min(uToRead, uRead);
		uToRead -= uAdjust;
		pOut += uAdjust;
	}

	// Close the open handle.
	SEOUL_VERIFY(FALSE != ::CloseHandle(hFile));

	// Success if all data read.
	return (0u == uToRead);
}

//
// Android, iOS, and Linux implementations.
//
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
inline DiskMemoryMappedFile* MemoryMapReadFile(const String& sAbsoluteFilename)
{
	return nullptr; // TODO:
}

inline DiskMemoryMappedFile* MemoryMapWriteFile(const String& sAbsoluteFilename, UInt64 uCapacity)
{
	return nullptr; // TODO:
}

// Accessors.
inline void const* GetMemoryMapReadPtr(DiskMemoryMappedFile const* p)
{
	return nullptr; // TODO:
}

inline void* GetMemoryMapWritePtr(DiskMemoryMappedFile const* p)
{
	return nullptr; // TODO:
}

inline UInt64 GetMemoryMapSize(DiskMemoryMappedFile const* p)
{
	return 0u; // TODO:
}

inline Bool CloseMemoryMap(DiskMemoryMappedFile*& rp, UInt64 uFinalSize)
{
	return false; // TODO:
}

inline Bool CopyFile(const String& sSourceAbsoluteFilename, const String& sDestinationAbsoluteFilename, Bool bOverwrite)
{
#if SEOUL_PLATFORM_IOS
	// Clear the output if it already exists.
	if (bOverwrite)
	{
		(void)unlink(sDestinationAbsoluteFilename.CStr());
	}

	// Perform the copy.
	if (0 != clonefile(sSourceAbsoluteFilename.CStr(), sDestinationAbsoluteFilename.CStr(), 0))
	{
		return false;
	}
#else // !SEOUL_PLATFORM_IOS
	// Open input for read.
	auto const iInput = open(sSourceAbsoluteFilename.CStr(), ToFileOpenFlags(File::kRead));
	if (iInput < 0)
	{
		return false;
	}

	// Open output for write - if not overwrite, set exclusivity bit
	// to prevent overwrites.
	auto const eOutputMode = (bOverwrite ? kCopyWithOverwriteMode : kCopyNoOverwriteMode);
	auto const iOutput = open(sDestinationAbsoluteFilename.CStr(), eOutputMode, ToFileModeFlags(File::kWriteTruncate));
	if (iOutput < 0)
	{
		SEOUL_CLOSE(iInput);
		return false;
	}

	// Stat the input file to determine the copy size and mode
	// flags to copy through.
	struct stat fileStat;
	memset(&fileStat, 0, sizeof(fileStat));
	if (0 != fstat(iInput, &fileStat))
	{
		SEOUL_CLOSE(iOutput);
		SEOUL_CLOSE(iInput);
		return false;
	}

	// Perform the copy.
	off_t offset = 0;
	off_t const size = fileStat.st_size;
	while (offset < size)
	{
		auto const iResult = sendfile(iOutput, iInput, &offset, (size - offset));
		if (iResult < 0)
		{
			SEOUL_CLOSE(iOutput);
			SEOUL_CLOSE(iInput);
			(void)unlink(sDestinationAbsoluteFilename.CStr());
			return false;
		}
	}

	// Close files.
	SEOUL_CLOSE(iOutput);
	SEOUL_CLOSE(iInput);

	// Now match attributes - we ignore return values as we don't
	// want the copy operation to fail if something unusual happens
	// and we can't match attributes.
	(void)chmod(sDestinationAbsoluteFilename.CStr(), fileStat.st_mode);

	struct utimbuf fileTimes;
	memset(&fileTimes, 0, sizeof(fileTimes));
	fileTimes.actime = time(nullptr); // Use now for access time, consistent behavior across platforms.
	fileTimes.modtime = fileStat.st_mode;
	(void)utime(sDestinationAbsoluteFilename.CStr(), &fileTimes);
#endif // /!SEOUL_PLATFORM_IOS

	// Done.
	return true;
}

inline Bool DeleteFile(const String& sAbsoluteFilename)
{
	return (0 == unlink(sAbsoluteFilename.CStr()));
}

inline RenameResult RenameFileEx(const String& sSourceAbsoluteFilename, const String& sDestinationAbsoluteFilename)
{
	auto const iResult = rename(sSourceAbsoluteFilename.CStr(), sDestinationAbsoluteFilename.CStr());
	if (0 == iResult)
	{
		return RenameResult::Success;
	}

	auto const err = errno;
	switch (err)
	{
	case EACCES: // fall-through
	case EPERM:
		return RenameResult::ErrorAccess;
	case EBUSY: // fall-through
	case ETXTBSY:
		return RenameResult::ErrorBusy;
	case EEXIST: // fall-through
	case ENOTEMPTY:
		return RenameResult::ErrorExist;
	case EINVAL:
		return RenameResult::ErrorInvalid;
	case EIO:
		return RenameResult::ErrorIo;
	case ENAMETOOLONG:
		return RenameResult::ErrorNameTooLong;
	case ENOENT:
		return RenameResult::ErrorNoEntity;
	case ENOSPC:
		return RenameResult::ErrorNoSpace;
	case EROFS:
		return RenameResult::ErrorReadOnly;

	case EISDIR: // fall-through
	case ELOOP: // fall-through
	case EMLINK: // fall-through
	case EXDEV: // fall-through
	default:
		return RenameResult::ErrorUnknown;
	};
}

inline Bool CreateAllZeroSparseFile(const String& sAbsoluteFilename, UInt64 uSizeHintInBytes)
{
	// TODO: Verify that all iOS files are sparse. Unclear,
	// and also dependent on whether AFS is supported (I think that came about in iOS 10):
	// https://developer.apple.com/documentation/foundation/file_system/about_apple_file_system
	//
	// Open the file for writing, then flush it to commit a 0 byte file.
	DiskSyncFile emptyFile(sAbsoluteFilename, File::kWriteTruncate);
	return emptyFile.Flush();

	// We don't apply the hint on Android/iOS, does not appear to be a win.
}

inline Bool SetDoNotBackupFlag(const String& sAbsoluteFilename)
{
#if SEOUL_PLATFORM_IOS
	return IOSSetDoNotBackupFlag(sAbsoluteFilename);
#else
	// No such flag on these platforms.
	return false;
#endif
}

inline Bool SetReadOnlyBit(const String& sAbsoluteFilename, Bool bReadOnlyBit)
{
	struct stat fileStat;
	memset(&fileStat, 0, sizeof(fileStat));
	if (0 != stat(sAbsoluteFilename.CStr(), &fileStat))
	{
		return false;
	}

	if (bReadOnlyBit)
	{
		fileStat.st_mode &= ~(S_IWUSR | S_IWGRP | S_IWOTH);
	}
	else
	{
		fileStat.st_mode |= (S_IWUSR | S_IWGRP | S_IWOTH);
	}

	return (0 == chmod(sAbsoluteFilename.CStr(), fileStat.st_mode));
}

inline Int SeoulCreateFile(const String& sAbsoluteFilename, File::Mode eMode, OpenResult& reResult)
{
	// Track of errno.
	Int iResult = 0;

	// Create the file as readable and writable by owner and other. Note
	// that we *must* exclude the additional mode bits if we're just reading,
	// or the open will fail.
	Int iReturn = -1;

	// "Robust" open (derived from implementation in sqlite). Filter
	// interrupts since they are fundamentally temporary.
	while (true)
	{
		if (File::kRead == eMode)
		{
			iReturn = open(sAbsoluteFilename.CStr(), ToFileOpenFlags(eMode));
		}
		else
		{
			iReturn = open(sAbsoluteFilename.CStr(), ToFileOpenFlags(eMode), ToFileModeFlags(eMode));
		}

		if (iReturn < 0)
		{
			// Track errno.
			iResult = errno;

			// Try again on EINTR.
			if (EINTR == iResult)
			{
				iReturn = -1;
				continue;
			}
		}
		else
		{
			// Make sure we return success.
			iResult = 0;
		}

		// Done - success or failure.
		break;
	}

	// Return handle on success - will be >= 0.
	if (iReturn >= 0)
	{
		reResult = OpenResult::Success;
		return iReturn;
	}
	else
	{
		reResult = ConvertToOpenResult(iResult);
		return -1;
	}
}

inline Bool FileExists(const String& sAbsoluteFilename)
{
	struct stat statResults;
	// Check to make sure that we didn't stat a directory because stat works on
	// files and file systems (i.e. the root directory)
	return (stat(sAbsoluteFilename.CStr(), &statResults) == 0 &&
			S_ISREG(statResults.st_mode));
}

inline Bool IsDirectory(const String& sAbsoluteFilename)
{
	struct stat statResults;
	return (stat(sAbsoluteFilename.CStr(), &statResults) == 0 &&
			S_ISDIR(statResults.st_mode));
}

inline UInt64 GetModifiedTime(const String& sAbsoluteFilename)
{
	struct stat statResults;
	memset(&statResults, 0, sizeof(statResults));

	if (0 == stat(sAbsoluteFilename.CStr(), &statResults))
	{
#if SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
		return (UInt64)statResults.st_mtime;
#else
		return (statResults.st_mtime >= 0)
			? (UInt64)statResults.st_mtime
			: 0u;
#endif
	}
	else
	{
		return 0u;
	}
}

inline Bool SetModifiedTime(const String& sAbsoluteFilename, UInt64 uModifiedTime)
{
	// Get existing to maintain access time.
	struct stat fileStat;
	memset(&fileStat, 0, sizeof(fileStat));
	if (0 != stat(sAbsoluteFilename.CStr(), &fileStat))
	{
		return false;
	}

	struct utimbuf fileTimes;
	memset(&fileTimes, 0, sizeof(fileTimes));

	fileTimes.actime = fileStat.st_atime;
	fileTimes.modtime = (time_t)uModifiedTime;

	return (0 == utime(sAbsoluteFilename.CStr(), &fileTimes));
}

inline UInt64 GetFileSize(const String& sAbsoluteFilename)
{
	struct stat statResults;
	memset(&statResults, 0, sizeof(statResults));

	if (0 == stat(sAbsoluteFilename.CStr(), &statResults))
	{
		return statResults.st_size;
	}
	else
	{
		return 0u;
	}
}

inline Bool GetCurrentPositionIndicator(Int iHandle, Int64& riPosition)
{
	if (iHandle >= 0)
	{
		// TODO: tell/tell64 appears to missing from Android,
		// so the equivalent (I think) is to issue a relative
		// seek of 0, in which case SEOUL_SEEK() should return
		// the position.

		Int64 const iReturn = SEOUL_SEEK(iHandle, 0, SEEK_CUR);
		if (iReturn >= 0)
		{
			riPosition = iReturn;
			return true;
		}
	}

	return false;
}

inline UInt64 GetFileSize(Int iHandle)
{
	// Edge case.
	if (iHandle < 0) { return 0; }

	// Capture current position - if this fails, size query
	// also fails.
	Int64 const iStart = SEOUL_SEEK(iHandle, 0, SEEK_CUR);
	if (iStart < 0) { return 0; }

	// Seek to end to get size - if this fails, size query
	// also fails.
	Int64 const iSize = SEOUL_SEEK(iHandle, 0, SEEK_END);
	if (iSize < 0) { return 0; }

	// Restore original position - this must always succeed or
	// we're screwed.
	SEOUL_VERIFY(iStart == SEOUL_SEEK(iHandle, iStart, SEEK_SET));

	// Normalize to 0 on failure.
	return (iSize < 0 ? 0 : (UInt64)iSize);
}

inline Bool Seek(Int iHandle, Int64 iPosition, File::SeekMode eMode)
{
	if (iHandle >= 0)
	{
		return (SEOUL_SEEK(iHandle, iPosition, ToSeekMode(eMode)) >= 0);
	}

	return false;
}

inline Bool Read(
	const String& sAbsoluteFilename,
	void* pOutputBuffer,
	UInt32 uOutputSizeInBytes)
{
	// Open the file for read.
	OpenResult eUnusedResult = OpenResult::Success;
	auto iFile = SeoulCreateFile(sAbsoluteFilename, File::kRead, eUnusedResult);
	if (iFile < 0) { return false; }

	// Loop and read the target size.
	UInt32 uToRead = uOutputSizeInBytes;
	Byte* pOut = (Byte*)pOutputBuffer;
	while (uToRead > 0u)
	{
		auto const uRead = Read(iFile, pOut, uToRead);
		if (0u == uRead) { break; }

		// Adjust and (potentially) loop.
		auto const uAdjust = Min(uToRead, uRead);
		uToRead -= uAdjust;
		pOut += uAdjust;
	}

	// Close the open handle.
	DestroyFile(iFile);

	// Success if all data read.
	return (0u == uToRead);
}
#else
#error "Define for this platform."
#endif

#undef SEOUL_SEEK
#undef SEOUL_WRITE
#undef SEOUL_READ
#undef SEOUL_FSYNC
#undef SEOUL_CLOSE

} // namespace DiskFileSystemDetail

} // namespace SeoulFile
