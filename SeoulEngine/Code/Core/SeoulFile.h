/**
 * \file SeoulFile.h
 * \brief Objects for access to files on persistent stroage.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef	SEOUL_FILE_H
#define SEOUL_FILE_H

#include "FilePath.h"
#include "Prereqs.h"
#include "StreamBuffer.h"
#include "Vector.h"

namespace Seoul
{

// Bounding value to catch bad reads producing unreasonably large
// sizes, before we cause a crash due to out-of-bounds memory
// allocation. This value was chosen to be generous but also be
// small enough to avoid memory allocation failure.
static const UInt32 kDefaultMaxReadSize = (1u << 30u);

/**
 * Common functions and types for all types of file access.
 */
namespace File
{

/**
 * Modes that can be used when creating a file. These
 * control the type and behavior of read and write access to
 * the file.
 */
enum Mode
{
	/** Only readable. */
	kRead,

	/** Only writeable, an existing file will be zeroed out. */
	kWriteTruncate,

	/** Only writeable, data will be added to an existing file. */
	kWriteAppend,

	/** Read/write access, file will be modified in place. File must exist. */
	kReadWrite,
}; // enum Mode

/**
 * SeekMode is used to specify a seek operation, either as an absolute
 * seek, a seek relative to the current file position, or a seek relative
 * to the end of the file.
 */
enum SeekMode
{
	/** An absolute seek, or a seek from the beginning of a file. */
	kSeekFromStart,

	/** A relative seek, or a seek from the current file position. */
	kSeekFromCurrent,

	/** A relative seek from the end of a file. */
	kSeekFromEnd
}; // enum SeekMode

/**
 * @return True if eMode supports data reads, false otherwise.
 */
inline Bool CanRead(Mode eMode)
{
	return (
		kRead == eMode ||
		kReadWrite == eMode);
}

/**
 * @return True if eMode supports data writes, false otherwise.
 */
inline Bool CanWrite(Mode eMode)
{
	return (
		kWriteTruncate == eMode ||
		kWriteAppend == eMode ||
		kReadWrite == eMode);
}

} // namespace File

/**
 * Basic information about a file or directory (a platform-independent subset
 * of the POSIX struct stat)
 */
struct FileStat SEOUL_SEALED
{
	FileStat()
		: m_bIsDirectory(false)
		, m_uFileSize(0)
		, m_uModifiedTime(0)
	{
	}

	Bool operator == (const FileStat& rhs) const
	{
		return m_bIsDirectory == rhs.m_bIsDirectory &&
			m_uFileSize == rhs.m_uFileSize &&
			m_uModifiedTime == rhs.m_uModifiedTime;
	}

	Bool operator != (const FileStat& rhs) const
	{
		return !(*this == rhs);
	}

	/**
	 * True if it's a directory or false if it's a regular file (special
	 * files/devices are not supported)
	 */
	Bool m_bIsDirectory;

	/** File size, in bytes */
	UInt64 m_uFileSize;

	/** File's last modified time, in seconds since 1970-01-01 UTC */
	UInt64 m_uModifiedTime;
};

/**
 * Abstract base class for all concrete file implementations that
 * offer synchronous reads and writes from/to files.
 */
class SyncFile SEOUL_ABSTRACT
{
public:
	virtual ~SyncFile()
	{
	}

	// Attempts to read zSizeInBytes data from the file into pOut. Returns the
	// number of bytes actually read. Will return 0u if CanRead() is false.
	virtual UInt32 ReadRawData(void* pOut, UInt32 zSizeInBytes) = 0;

	// Attempts to writes zSizeInBytes of data to pOut buffer from the file.
	// Returns the number of bytes actually written. Will return 0u if
	// CanWrite() is false.
	virtual UInt32 WriteRawData(void const* pIn, UInt32 zSizeInBytes) = 0;

	// Returns an absolute filename that identifies this SyncFile.
	virtual String GetAbsoluteFilename() const = 0;

	// Returns true if this file was opened successfully, false otherwise.
	virtual Bool IsOpen() const = 0;

	// Returns true if this specialization of SyncFile can read data.
	// If this method returns false, any read methods will either fail
	// silently or return appropriate failure codes or nop values (i.e.
	// Read() will return 0u bytes read.
	virtual Bool CanRead() const = 0;

	// Returns true if this specialization of SyncFile can write data.
	// If this method returns false, any write methods will either fail
	// silently or return appropriate failure codes or nop values (i.e.
	// Write() will return 0u bytes written.
	virtual Bool CanWrite() const = 0;

	// If writing is supported, commits any data in a pending write buffer
	// to persistent storage.
	virtual Bool Flush() = 0;

	// Return the total current size of the file, in bytes. Returns 0u
	// if the file is empty, or if IsOpen() returns false.
	virtual UInt64 GetSize() const = 0;

	// Returns true if this specialization of SyncFile can seek.
	// If this method returns false, calls to Seek() will always return false.
	virtual Bool CanSeek() const = 0;

	// Attempt to get the current absolute file pointer position.
	virtual Bool GetCurrentPositionIndicator(Int64& riPosition) const = 0;

	// Attempt to relocate the file pointer to the position iPosition. If
	// this method returns true, the file pointer will now be at the position
	// specified by iPosition, in consideration of the mode flag eMode. If this
	// method returns false, the file pointer position is undefined.
	virtual Bool Seek(Int64 iPosition, File::SeekMode eMode) = 0;

	// Helper method - reads this entire SyncFile into a buffer, based on
	// provided arguments. this SyncFile must have been constructed with
	// read permissions enabled.
	virtual Bool ReadAll(
		void*& rpOutputBuffer,
		UInt32& rzOutputSizeInBytes,
		UInt32 zAlignmentOfOutputBuffer,
		MemoryBudgets::Type eOutputBufferMemoryType,
		UInt32 zMaxReadSize = kDefaultMaxReadSize);

	// Helper method - reads this entire SyncFile into a buffer, based on
	// provided arguments. this SyncFile must have been constructed with
	// read permissions enabled.
	virtual Bool WriteAll(
		void const* pIn,
		UInt32 uSizeInBytes);

protected:
	SyncFile()
	{
	}

private:
	SEOUL_DISABLE_COPY(SyncFile);
}; // class SyncFile

/**
 * Specialization of SyncFile that buffers reads and writes using a fixed
 * buffer size, similar the the FILE structure in the stdio library.
 */
class BufferedSyncFile SEOUL_SEALED : public SyncFile
{
public:
	/** Default buffer size used for buffering reads and writes */
	static const UInt32 kDefaultBufferSize = 64*1024;

	// Constructs a BufferedSyncFile using the given source unbuffered file and
	// the given buffer size.  Pass bOwnsSourceFile=true if we should take
	// ownership of the given source file and delete it when we're done.
	BufferedSyncFile(SyncFile *pSourceFile, Bool bOwnsSourceFile, UInt32 uBufferSize = kDefaultBufferSize);

	// Virtual destructor
	virtual ~BufferedSyncFile();

	// Sets the buffer size. Will be clamped to a minimum of 1 (BufferedSyncFile is always buffered).
	void SetBufferSize(UInt32 uBufferSize);

	// Attempts to read uSizeInBytes data from the file into pOut. Returns the
	// number of bytes actually read. Will return 0u if CanRead() is false.
	virtual UInt32 ReadRawData(void *pOut, UInt32 uSizeInBytes) SEOUL_OVERRIDE;

	// Reads a line of text, returning true if a line was read, false
	// otherwise.
	Bool ReadLine(String& rsLine);

	// Attempts to writes uSizeInBytes of data to pOut buffer from the file.
	// Returns the number of bytes actually written. Will return 0u if
	// CanWrite() is false.
	virtual UInt32 WriteRawData(const void *pIn, UInt32 uSizeInBytes) SEOUL_OVERRIDE;

	/** Returns an absolute filename that identifies this file */
	virtual String GetAbsoluteFilename() const SEOUL_OVERRIDE
	{
		return m_pFile->GetAbsoluteFilename();
	}

	/** Returns true if this file was opened successfully, false otherwise */
	virtual Bool IsOpen() const SEOUL_OVERRIDE
	{
		return m_pFile->IsOpen();
	}

	/**
	 * Returns true if this specialization of file can read data.
	 * If this method returns false, any read methods will either fail
	 * silently or return appropriate failure codes or nop values (i.e.
	 * Read() will return 0u bytes read.
	*/
	virtual Bool CanRead() const SEOUL_OVERRIDE
	{
		return m_pFile->CanRead();
	}

	/**
	 * Returns true if this specialization of file can write data.
	 * If this method returns false, any write methods will either fail
	 * silently or return appropriate failure codes or nop values (i.e.
	 * Write() will return 0u bytes written.
	 */
	virtual Bool CanWrite() const SEOUL_OVERRIDE
	{
		return m_pFile->CanWrite();
	}

	// If writing is supported, commits any data in a pending write buffer
	// to persistent storage.
	virtual Bool Flush() SEOUL_OVERRIDE;

	/**
	 * Return the total current size of the file, in bytes. Returns 0u
	 * if the file is empty, or if IsOpen() returns false.
	 */
	virtual UInt64 GetSize() const SEOUL_OVERRIDE
	{
		return m_pFile->GetSize();
	}

	// Print a formatted string to this file
	//
	// NOTE: W.R.T format attribute, member functions have an implicit
	// 'this' so counting starts at 2 for explicit arguments.
	void Printf(const Byte *sFormat, ...) SEOUL_PRINTF_FORMAT_ATTRIBUTE(2, 3);
	void VPrintf(const Byte *sFormat, va_list argList) SEOUL_PRINTF_FORMAT_ATTRIBUTE(2, 0);

	/**
	 * Returns true if this specialization of file can seek.
	 * If this method returns false, calls to Seek() will always return false.
	 */
	virtual Bool CanSeek() const SEOUL_OVERRIDE
	{
		return m_pFile->CanSeek();
	}

	// Attempt to get the current absolute file pointer position.
	virtual Bool GetCurrentPositionIndicator(Int64& riPosition) const SEOUL_OVERRIDE
	{
		return false;
	}

	// Attempt to relocate the file pointer to the position iPosition. If
	// this method returns true, the file pointer will now be at the position
	// specified by iPosition, in consideration of the mode flag eMode. If this
	// method returns false, the file pointer position is undefined.
	virtual Bool Seek(Int64 iPosition, File::SeekMode eMode) SEOUL_OVERRIDE;

private:
	SEOUL_DISABLE_COPY(BufferedSyncFile);

private:
	/** Underlying unbuffered file */
	SyncFile *m_pFile;

	/** True if we own m_pFile and should delete it when we're done */
	Bool m_bOwnsFile;

	/** Buffer used for file reads */
	Vector<Byte, MemoryBudgets::Io> m_vReadBuffer;

	/** Offset into the read buffer of the next data to read */
	UInt32 m_uReadBufferOffset;

	/** Buffer used for file writes */
	Vector<Byte, MemoryBudgets::Io> m_vWriteBuffer;
};

/**
 * Specialization of SyncFile that reads the entire
 * file it is constructed with into memory. All read operations
 * are than fulfilled from an in-memory buffer.
 *
 * FullyBufferedSyncFile is not writeable.
 */
class FullyBufferedSyncFile SEOUL_SEALED : public SyncFile
{
public:
	FullyBufferedSyncFile(
		void* pData,
		UInt32 zDataSizeInBytes,
		Bool bTakeOwnershipOfData = true,
		const String& sAbsoluteFilename = String());
	FullyBufferedSyncFile(SyncFile& rSourceFile);
	virtual ~FullyBufferedSyncFile();

	/**
	 * Read zSizeInBytes data from the file into pOut. Returns the
	 * number of bytes actually read.
	 */
	virtual UInt32 ReadRawData(void* pOut, UInt32 zSizeInBytes) SEOUL_OVERRIDE;

	// Reads a line of text, returning true if a line was read, false
	// otherwise.
	Bool ReadLine(String& rsLine);

	/**
	 * @return 0u - FullyBufferedSyncFile is not writeable.
	 */
	virtual UInt32 WriteRawData(void const* pIn, UInt32 zSizeInBytes) SEOUL_OVERRIDE
	{
		return 0u;
	}

	/**
	 * @return An absolute filename that identifies this FullyBufferedSyncFile.
	 */
	virtual String GetAbsoluteFilename() const SEOUL_OVERRIDE
	{
		return m_sAbsoluteFilename;
	}

	/**
	 * @return True if this FullyBufferedSyncFile was opened successfully,
	 * false otherwise.
	 */
	virtual Bool IsOpen() const SEOUL_OVERRIDE
	{
		return (nullptr != m_pData);
	}

	/**
	 * FullyBufferedSyncFile is always readable if it was opened
	 * successfully.
	 */
	virtual Bool CanRead() const SEOUL_OVERRIDE
	{
		return IsOpen();
	}

	/**
	 * @return False, FullyBufferedSyncFile is never writeable.
	 */
	virtual Bool CanWrite() const SEOUL_OVERRIDE
	{
		return false;
	}

	/**
	 * Nop - FullyBufferedSyncFile is not writeable.
	 */
	virtual Bool Flush() SEOUL_OVERRIDE
	{
		return false;
	}

	/**
	 * @return The size of the data loaded by this FullyBufferedSyncFile.
	 */
	virtual UInt64 GetSize() const SEOUL_OVERRIDE
	{
		return m_zDataSize;
	}

	/**
	 * @return True - FullyBufferedSyncFile always supports seeking.
	 */
	virtual Bool CanSeek() const SEOUL_OVERRIDE
	{
		return true;
	}

	// Attempt to get the current absolute file pointer position.
	virtual Bool GetCurrentPositionIndicator(Int64& riPosition) const SEOUL_OVERRIDE
	{
		riPosition = m_iOffset;
		return true;
	}

	/**
	 * Update the next read position.
	 *
	 * @return True if the seek was successful, false otherwise. If this
	 * method returns false, the read pointer will be in an unknown state.
	 */
	virtual Bool Seek(Int64 iPosition, File::SeekMode eMode) SEOUL_OVERRIDE
	{
		switch (eMode)
		{
		case File::kSeekFromStart:
			m_iOffset = iPosition;
			return true;
			break;
		case File::kSeekFromCurrent:
			m_iOffset += iPosition;
			return true;
			break;
		case File::kSeekFromEnd:
			m_iOffset = (m_zDataSize - iPosition);
			return true;
			break;
		default:
			return false;
			break;
		};
	}

private:
	void* m_pData;
	UInt32 m_zDataSize;
	Int64 m_iOffset;
	String m_sAbsoluteFilename;
	Bool m_bOwnsData;

	SEOUL_DISABLE_COPY(FullyBufferedSyncFile);
}; // class FullyBufferedSyncFile

class MemorySyncFile SEOUL_SEALED : public SyncFile
{
public:
	MemorySyncFile(const String& sFilename = String())
		: m_sFilename(sFilename)
		, m_Buffer()
	{
	}

	~MemorySyncFile()
	{
	}

	virtual UInt32 ReadRawData(void* pOut, UInt32 zSizeInBytes) SEOUL_OVERRIDE
	{
		if (m_Buffer.Read(pOut, zSizeInBytes))
		{
			return zSizeInBytes;
		}

		return 0u;
	}

	virtual UInt32 WriteRawData(void const* pIn, UInt32 zSizeInBytes) SEOUL_OVERRIDE
	{
		m_Buffer.Write(pIn, zSizeInBytes);
		return zSizeInBytes;
	}

	virtual String GetAbsoluteFilename() const SEOUL_OVERRIDE
	{
		return m_sFilename;
	}

	virtual Bool IsOpen() const SEOUL_OVERRIDE
	{
		return true;
	}

	virtual Bool CanRead() const SEOUL_OVERRIDE
	{
		return true;
	}

	virtual Bool CanWrite() const SEOUL_OVERRIDE
	{
		return true;
	}

	virtual Bool Flush() SEOUL_OVERRIDE
	{
		// Flush always succeeds, data is always immediatley
		// "flushed" in a memory stream.
		return true;
	}

	virtual UInt64 GetSize() const SEOUL_OVERRIDE
	{
		return (UInt64)m_Buffer.GetTotalDataSizeInBytes();
	}

	virtual Bool CanSeek() const SEOUL_OVERRIDE
	{
		return true;
	}

	virtual Bool GetCurrentPositionIndicator(Int64& riPosition) const SEOUL_OVERRIDE
	{
		riPosition = (Int64)m_Buffer.GetOffset();
		return true;
	}

	virtual Bool Seek(Int64 iPosition, File::SeekMode eMode) SEOUL_OVERRIDE
	{
		switch (eMode)
		{
		case File::kSeekFromCurrent:
			iPosition = (Int64)m_Buffer.GetOffset() + iPosition;
			break;

		case File::kSeekFromEnd:
			iPosition = (Int64)m_Buffer.GetTotalDataSizeInBytes() - iPosition;
			break;

		default:
			break;
		};

		if (iPosition < 0 || iPosition > (Int64)m_Buffer.GetTotalDataSizeInBytes())
		{
			return false;
		}

		m_Buffer.SeekToOffset((StreamBuffer::SizeType)iPosition);
		return true;
	}

	StreamBuffer& GetBuffer()
	{
		return m_Buffer;
	}

	void Swap(MemorySyncFile& r)
	{
		m_sFilename.Swap(r.m_sFilename);
		m_Buffer.Swap(r.m_Buffer);
	}

private:
	String m_sFilename;
	StreamBuffer m_Buffer;

	SEOUL_DISABLE_COPY(MemorySyncFile);
}; // class MemorySyncFile

} // namespace Seoul

#endif // include guard
