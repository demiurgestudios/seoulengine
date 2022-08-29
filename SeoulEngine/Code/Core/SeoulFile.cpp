/**
 * \file SeoulFile.cpp
 * \brief Objects for access to files on persistent stroage.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Logger.h"
#include "SeoulFile.h"
#include "SeoulFileReaders.h"
#include "Vector.h"

namespace Seoul
{

/**
 * Shared inner body for ReadLine() used by BufferedSyncFile
 * and FullyBufferedSyncFile.
 *
 * @return True if a line terminator was found, false otherwise. If this
 * function returns true, rsLine contains a full line with an appended
 * '\n'. Otherwise, rsLine will either be empty or contain part of a line
 * with no appended '\n'.
 *
 * The caller can use rsLine.IsEmpty() to determine if
 * a line is in progress when this function returns false.
 *
 * Scanning will begin at ruOffset and ruOffset will be left
 * at the point where the scan finished, which will either be at the
 * end of the data (ruOffset == nDataSizeInBytes) or one passed
 * the terminating newline character, if found.
 */
static Bool InternalStaticReadLine(
	void const* pBuffer,
	UInt32 nDataSizeInBytes,
	UInt32& ruOffset,
	String& rsLine)
{
	// Start at the current offset.
	UInt32 const zStartingPosition = ruOffset;

	// Cast the buffer to a byte pointer for easier manipulation.
	Byte const* pData = reinterpret_cast<Byte const*>(pBuffer);

	// Until we hit EOF.
	while (ruOffset < nDataSizeInBytes)
	{
		switch (pData[ruOffset])
		{
			// If we hit a newline character, we're at an end of line.
			// Store the string, advance the read pointer past the newline,
			// and return true.
		case '\n':
			++ruOffset;
			rsLine.Append((pData + zStartingPosition), (UInt32)(ruOffset - zStartingPosition));
			return true;

			// If we hit a carriage return character, we're at an end of line.
		case '\r':
			// Store the string, terminated with a '\n', and then advance passed
			// the '\r'.
			rsLine.Append((pData + zStartingPosition), (UInt32)(ruOffset - zStartingPosition));
			rsLine.Append('\n');
			++ruOffset;

			// Handle CR+LF - if there is a '\n' immediately after the '\r',
			// skip it without treating this as a separate line.
			if (ruOffset < nDataSizeInBytes && '\n' == pData[ruOffset])
			{
				++ruOffset;
			}
			return true;

		default:
			// All other characters, just advance the offset.
			++ruOffset;
			break;
		};
	}

	// Append any characters to the running buffer before
	// returning false.
	if (ruOffset > zStartingPosition)
	{
		rsLine.Append((pData + zStartingPosition), (UInt32)(ruOffset - zStartingPosition));
	}

	return false;
}

/**
 * Reads the entire contents of this SyncFile into a buffer.
 *
 * this SyncFile must have been constructed with read permissions.
 *
 * If this method returns true, the file pointer position will be at
 * EOF. Otherwise, it is undefined.
 */
Bool SyncFile::ReadAll(
	void*& rpOutputBuffer,
	UInt32& rzOutputSizeInBytes,
	UInt32 zAlignmentOfOutputBuffer,
	MemoryBudgets::Type eOutputBufferMemoryType,
	UInt32 zMaxReadSize)
{
	if (CanRead() && CanSeek())
	{
		// Return to the beginning of the file.
		if (!Seek(0, File::kSeekFromStart))
		{
			return false;
		}

		const UInt64 zFileSizeInBytes = GetSize();
		if (zFileSizeInBytes <= zMaxReadSize)
		{
			const UInt32 zDataSizeInBytes = (UInt32)zFileSizeInBytes;
			void* pOutput = MemoryManager::AllocateAligned(
				zDataSizeInBytes,
				eOutputBufferMemoryType,
				zAlignmentOfOutputBuffer);

			if (pOutput)
			{
				if (zDataSizeInBytes == ReadRawData(pOutput, zDataSizeInBytes))
				{
					rpOutputBuffer = pOutput;
					rzOutputSizeInBytes = zDataSizeInBytes;
					return true;
				}
			}

			MemoryManager::Deallocate(pOutput);
			pOutput = nullptr;
		}
		else
		{
			SEOUL_WARN("Failed reading \"%s\", file is too large.\n",
				GetAbsoluteFilename().CStr());
		}
	}

	return false;
}

/**
 * Helper method - reads this entire SyncFile into a buffer, based on
 * provided arguments. this SyncFile must have been constructed with
 * read permissions enabled.
 */
Bool SyncFile::WriteAll(
	void const* pIn,
	UInt32 uSizeInBytes)
{
	if (CanWrite() && CanSeek())
	{
		// Return to the beginning of the file.
		if (!Seek(0, File::kSeekFromStart))
		{
			return false;
		}

		return (uSizeInBytes == WriteRawData(pIn, uSizeInBytes));
	}

	return false;
}

/**
 * Constructs a BufferedSyncFile using the given source unbuffered file and the
 * given buffer size.
 *
 * @param[in] pSourceFile Underlying unbuffered file
 * @param[in] bOwnsSourceFile True if we are to take ownership of the source
 *              file and delete it when we're done, or false otherwise
 * @param[in] uBufferSize Buffer size to use for file operations
 */
BufferedSyncFile::BufferedSyncFile(SyncFile *pSourceFile, Bool bOwnsSourceFile, UInt32 uBufferSize /*= kDefaultBufferSize*/)
	: m_pFile(pSourceFile)
	, m_bOwnsFile(bOwnsSourceFile)
	, m_uReadBufferOffset(0)
{
	SetBufferSize(uBufferSize);
}

/**
 * Virtual destructor
 */
BufferedSyncFile::~BufferedSyncFile()
{
	Flush();

	if (m_bOwnsFile)
	{
		SEOUL_DELETE m_pFile;
	}
}

/**
 * Sets the buffer size to be used for buffering file data.  The read buffer is
 * only allocated if the file is opened for reading, and the write buffer is
 * only allocated if the file is opened for writing.
 *
 * @param[in] zBufferSize Size of the buffer to use for read and write
 *              operations. Will be clamped to a minimum of 1
 */
void BufferedSyncFile::SetBufferSize(UInt32 uBufferSize)
{
	// Require at least a 1 byte buffer.
	uBufferSize = Max(uBufferSize, 1u);

	if (CanRead())
	{
		// If decreasing the buffer size, discard any unread buffered data and
		// seek backwards
		UInt32 uUnreadData = m_vReadBuffer.GetSize() - m_uReadBufferOffset;
		if (uBufferSize < uUnreadData)
		{
			UInt32 uBytesToDiscard = uUnreadData - uBufferSize;
			SEOUL_VERIFY_MESSAGE(m_pFile->Seek(-(Int64)uBytesToDiscard, File::kSeekFromCurrent), "You cannot decrease the buffer size of a non-seekable file if any file data has already been read");

			// Allocate new buffer and swap it in
			Vector<Byte, MemoryBudgets::Io> vNewBuffer;
			vNewBuffer.Reserve(uBufferSize);
			vNewBuffer.Insert(vNewBuffer.Begin(), m_vReadBuffer.Begin() + m_uReadBufferOffset, m_vReadBuffer.Begin() + m_uReadBufferOffset + uBufferSize);
			m_vReadBuffer.Swap(vNewBuffer);
			m_uReadBufferOffset = 0;
		}
		else if (uBufferSize < m_vReadBuffer.GetCapacity())
		{
			// Allocate new buffer and swap it in, since Reserve() doesn't
			// decrease the buffer size
			Vector<Byte, MemoryBudgets::Io> vNewBuffer;
			vNewBuffer.Reserve(uBufferSize);
			vNewBuffer.Insert(vNewBuffer.Begin(), m_vReadBuffer.Begin() + m_uReadBufferOffset, m_vReadBuffer.End());
			m_vReadBuffer.Swap(vNewBuffer);
			m_uReadBufferOffset = 0;
		}
		else
		{
			// Otherwise, we can just resize the buffer
			m_vReadBuffer.Reserve(uBufferSize);
		}

		SEOUL_ASSERT(m_vReadBuffer.GetCapacity() == uBufferSize);
	}

	if (CanWrite())
	{
		// If decreasing the buffer size beyond how much is already buffered,
		// flush any unwritten data
		if (uBufferSize < m_vWriteBuffer.GetSize())
		{
			Flush();
		}

		// Allocate new buffer and swap it in, since Reserve() doesn't decrease
		// the buffer size
		Vector<Byte, MemoryBudgets::Io> vNewBuffer;
		vNewBuffer.Reserve(uBufferSize);
		vNewBuffer.Insert(vNewBuffer.Begin(), m_vWriteBuffer.Begin(), m_vWriteBuffer.End());
		m_vWriteBuffer.Swap(vNewBuffer);
	}
}

/**
 * Attempts to read uSizeInBytes data from the file into pOut. Returns the
 * number of bytes actually read. Will return 0u if CanRead() is false.
 */
UInt32 BufferedSyncFile::ReadRawData(void *pOut, UInt32 uSizeInBytes)
{
	if (!CanRead())
	{
		return 0;
	}

	// Read buffered data, if possible
	UInt32 uBytesRead = 0;
	if (m_uReadBufferOffset < m_vReadBuffer.GetSize())
	{
		// If we have all the data, just copy and return it
		if (uSizeInBytes <= m_vReadBuffer.GetSize() - m_uReadBufferOffset)
		{
			memcpy(pOut, m_vReadBuffer.Get(m_uReadBufferOffset), uSizeInBytes);
			m_uReadBufferOffset += uSizeInBytes;
			return uSizeInBytes;
		}
		else
		{
			// We only have some of the data in the buffer
			UInt32 uPartialReadSize = m_vReadBuffer.GetSize() - m_uReadBufferOffset;
			memcpy(pOut, m_vReadBuffer.Get(m_uReadBufferOffset), uPartialReadSize);
			pOut = (Byte *)pOut + uPartialReadSize;
			uSizeInBytes -= uPartialReadSize;
			uBytesRead += uPartialReadSize;
			m_uReadBufferOffset += uPartialReadSize;
		}
	}

	// If we're reading more than one buffer's worth, just do an unbuffered
	// read
	if (uSizeInBytes >= m_vReadBuffer.GetCapacity())
	{
		return uBytesRead + m_pFile->ReadRawData(pOut, uSizeInBytes);
	}

	// Try to fill up the buffer and then copy out of it
	m_vReadBuffer.Resize(m_vReadBuffer.GetCapacity());
	UInt32 uRawDataRead = m_pFile->ReadRawData(m_vReadBuffer.Get(0), m_vReadBuffer.GetSize());
	m_vReadBuffer.Resize(uRawDataRead);

	UInt32 uCopySize = Min(uSizeInBytes, uRawDataRead);
	if (uCopySize > 0)
	{
		memcpy(pOut, m_vReadBuffer.Get(0), uCopySize);
		uBytesRead += uCopySize;
	}

	m_uReadBufferOffset = uCopySize;

	return uBytesRead;
}

/**
 * Reads a line of text, returning true if a line was read, false otherwise.
 *
 * If this method returns false, rsLine will be left unmodified.
 */
Bool BufferedSyncFile::ReadLine(String& rsLine)
{
	if (!CanRead())
	{
		return false;
	}

	// Sanity check - code below assumes that the read buffer is not zero size.
	// This is enforced by SetBufferSize().
	SEOUL_ASSERT(m_vReadBuffer.GetCapacity() > 0);

	String sLine;

	while(1)
	{
		// Search through the buffered data for a newline
		if (m_uReadBufferOffset < m_vReadBuffer.GetSize())
		{
			if (InternalStaticReadLine(
				m_vReadBuffer.Get(0),
				m_vReadBuffer.GetSize(),
				m_uReadBufferOffset,
				sLine))
			{
				rsLine.Swap(sLine);
				return true;
			}
		}

		// Refill our buffer
		m_vReadBuffer.Resize(m_vReadBuffer.GetCapacity());
		UInt32 uRawDataRead = m_pFile->ReadRawData(m_vReadBuffer.Get(0), m_vReadBuffer.GetSize());
		m_vReadBuffer.Resize(uRawDataRead);
		m_uReadBufferOffset = 0;

		// EOF?
		if (uRawDataRead == 0)
		{
			// If we have characters, then treat the remaining data
			// as a line - terminate it with a '\n'.
			if (!sLine.IsEmpty())
			{
				sLine.Append('\n');
				rsLine.Swap(sLine);
				return true;
			}

			return false;
		}
	}
}

/**
 * Attempts to writes uSizeInBytes of data to pOut buffer from the file.
 * Returns the number of bytes actually written. Will return 0u if
 * CanWrite() is false.
 */
UInt32 BufferedSyncFile::WriteRawData(const void *pIn, UInt32 uSizeInBytes)
{
	if (!CanWrite())
	{
		return 0;
	}

	// Buffer the data if possible
	const Byte *pData = (const Byte *)pIn;
	UInt32 uSpaceLeft = m_vWriteBuffer.GetCapacity() - m_vWriteBuffer.GetSize();
	if (uSizeInBytes <= uSpaceLeft)
	{
		m_vWriteBuffer.Insert(m_vWriteBuffer.End(), pData, pData + uSizeInBytes);
		return uSizeInBytes;
	}

	// Do a partial write if possible and flush our buffer
	m_vWriteBuffer.Insert(m_vWriteBuffer.End(), pData, pData + uSpaceLeft);
	pData += uSpaceLeft;
	uSizeInBytes -= uSpaceLeft;
	UInt32 uBytesWritten = uSpaceLeft;
	Flush();

	// If we're writing more than one buffer's worth, just do an unbuffered
	// write
	if (uSizeInBytes >= m_vWriteBuffer.GetCapacity())
	{
		return uBytesWritten + m_pFile->WriteRawData(pData, uSizeInBytes);
	}

	// Buffer the remaining data
	m_vWriteBuffer.Insert(m_vWriteBuffer.End(), pData, pData + uSizeInBytes);
	return uBytesWritten + uSizeInBytes;
}

/**
 * If writing is supported, commits any data in a pending write buffer
 * to persistent storage.
 */
Bool BufferedSyncFile::Flush()
{
	if (!CanWrite())
	{
		return false;
	}

	// Write any buffered data
	if (m_vWriteBuffer.GetSize() > 0)
	{
		SEOUL_VERIFY(m_pFile->WriteRawData(m_vWriteBuffer.Get(0), m_vWriteBuffer.GetSize()) == m_vWriteBuffer.GetSize());
		m_vWriteBuffer.Clear();
	}

	// This should be a no-op, but just in case it's not...
	return m_pFile->Flush();
}

/**
 * Print a formatted string to this file
 */
void BufferedSyncFile::Printf(const Byte *sFormat, ...)
{
	if (!CanWrite())
	{
		return;
	}

	va_list args;
	va_start(args, sFormat);
	VPrintf(sFormat, args);
	va_end(args);
}

/**
 * Print a formatted string to this file
 */
void BufferedSyncFile::VPrintf(const Byte *sFormat, va_list argList)
{
	auto const sString(String::VPrintf(sFormat, argList));

	SEOUL_VERIFY(WriteRawData(sString.CStr(), sString.GetSize()) == sString.GetSize());
}

/**
 * Attempt to relocate the file pointer to the position iPosition. If
 * this method returns true, the file pointer will now be at the position
 * specified by iPosition, in consideration of the mode flag eMode. If this
 * method returns false, the file pointer position is undefined.
 */
Bool BufferedSyncFile::Seek(Int64 iPosition, File::SeekMode eMode)
{
	if (!CanSeek())
	{
		return false;
	}

	// Adjust the position for any unread data in the read buffer
	if (eMode == File::kSeekFromCurrent)
	{
		UInt32 uUnreadData = m_vReadBuffer.GetSize() - m_uReadBufferOffset;
		iPosition -= uUnreadData;
	}

	// Discard any unread data in the read buffer & flush the write buffer
	m_vReadBuffer.Clear();
	Flush();

	return m_pFile->Seek(iPosition, eMode);
}

FullyBufferedSyncFile::FullyBufferedSyncFile(
	void* pData,
	UInt32 zDataSizeInBytes,
	Bool bTakeOwnershipOfData /* = true */,
	const String& sAbsoluteFilename /* = String() */)
	: m_pData(pData)
	, m_zDataSize(zDataSizeInBytes)
	, m_iOffset(0)
	, m_sAbsoluteFilename(sAbsoluteFilename)
	, m_bOwnsData(bTakeOwnershipOfData)
{
}

FullyBufferedSyncFile::FullyBufferedSyncFile(SyncFile& rSourceFile)
	: m_pData(nullptr)
	, m_zDataSize(0u)
	, m_iOffset(0)
	, m_sAbsoluteFilename(rSourceFile.GetAbsoluteFilename())
	, m_bOwnsData(true)
{
	(void)rSourceFile.ReadAll(
		m_pData,
		m_zDataSize,
		SEOUL_ALIGN_OF(UInt),
		MemoryBudgets::Io);
}

FullyBufferedSyncFile::~FullyBufferedSyncFile()
{
	if (m_pData && m_bOwnsData)
	{
		MemoryManager::Deallocate(m_pData);
	}
}

/**
 * Attempt to read zSizeInBytes data from this FullyBufferedSyncFile,
 * returning the actual number of bytes read.
 */
UInt32 FullyBufferedSyncFile::ReadRawData(void* pOut, UInt32 zSizeInBytes)
{
	if (CanRead())
	{
		SEOUL_ASSERT(m_iOffset >= 0 && m_zDataSize >= (UInt32)m_iOffset);
		const UInt32 zToRead = Min(zSizeInBytes, (UInt32)(m_zDataSize - m_iOffset));
		memcpy(pOut, reinterpret_cast<Byte*>(m_pData) + m_iOffset, zToRead);
		m_iOffset += zToRead;

		return zToRead;
	}

	return 0u;
}

/**
 * Read a line of text, returning true if a line was read,
 * false otherwise.
 *
 * If this method returns false, rsLine will be left unmodified.
 */
Bool FullyBufferedSyncFile::ReadLine(String& rsLine)
{
	// Nothing more to do if the file was not opened successfully,
	// of if the read offset is out of bounds.
	if (!CanRead() || m_iOffset < 0)
	{
		return false;
	}

	// Read a line starting at m_iOffset. If this succeeds, we have
	// a complete line with a terminator, so immediately return true.
	String sLine;
	UInt32 uOffset = (UInt32)m_iOffset;
	Bool const bReadLine = InternalStaticReadLine(
		m_pData,
		m_zDataSize,
		uOffset,
		sLine);
	m_iOffset = (Int64)uOffset;

	if (bReadLine)
	{
		rsLine.Swap(sLine);
		return true;
	}

	// If we didn't find a line but sLine was modified, it means
	// we hit the end of the data without finding a line ending -
	// treat this as a line. We need to terminate it with a '\n'
	// before returning.
	if (!sLine.IsEmpty())
	{
		sLine.Append('\n');
		rsLine.Swap(sLine);
		return true;
	}

	return false;
}

} // namespace SeoulFile
