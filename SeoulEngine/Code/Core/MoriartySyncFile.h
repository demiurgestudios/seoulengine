/**
 * \file MoriartySyncFile.h
 * \brief File class for accessing a file served from a Moriarty server.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef MORIARTY_SYNC_FILE_H
#define MORIARTY_SYNC_FILE_H

#include "MoriartyClient.h"
#include "SeoulFile.h"

#if SEOUL_WITH_MORIARTY

namespace Seoul
{

/**
 * Class for accessing a file served from a Moriarty server
 */
class MoriartySyncFile : public SyncFile
{
public:
	// Tries to open a remote file
	MoriartySyncFile(FilePath filePath, File::Mode eMode);

	virtual ~MoriartySyncFile();

	// Attempts to read zSizeInBytes data from the file into pOut. Returns the
	// number of bytes actually read. Will return 0u if CanRead() is false.
	virtual UInt32 ReadRawData(void *pOut, UInt32 zSizeInBytes) SEOUL_OVERRIDE;

	// Attempts to writes zSizeInBytes of data to pOut buffer from the file.
	// Returns the number of bytes actually written. Will return 0u if
	// CanWrite() is false.
	virtual UInt32 WriteRawData(const void *pIn, UInt32 zSizeInBytes) SEOUL_OVERRIDE;

	// Returns an absolute filename that identifies this SyncFile.
	virtual String GetAbsoluteFilename() const SEOUL_OVERRIDE;

	// Returns true if this file was opened successfully, false otherwise.
	virtual Bool IsOpen() const SEOUL_OVERRIDE
	{
		return (m_File != MoriartyClient::kInvalidFileHandle);
	}

	// Returns true if this specialization of SyncFile can read data.
	// If this method returns false, any read methods will either fail
	// silently or return appropriate failure codes or nop values (i.e.
	// Read() will return 0u bytes read.
	virtual Bool CanRead() const SEOUL_OVERRIDE
	{
		return (IsOpen() && File::CanRead(m_eMode));
	}

	// Returns true if this specialization of SyncFile can write data.
	// If this method returns false, any write methods will either fail
	// silently or return appropriate failure codes or nop values (i.e.
	// Write() will return 0u bytes written.
	virtual Bool CanWrite() const SEOUL_OVERRIDE
	{
		return (IsOpen() && File::CanWrite(m_eMode));
	}

	// If writing is supported, commits any data in a pending write buffer
	// to persistent storage.
	virtual Bool Flush() SEOUL_OVERRIDE
	{
		return false;
	}

	// Return the total current size of the file, in bytes. Returns 0u
	// if the file is empty, or if IsOpen() returns false.
	virtual UInt64 GetSize() const SEOUL_OVERRIDE;

	// Returns true if this specialization of SyncFile can seek.
	// If this method returns false, calls to Seek() will always return false.
	virtual Bool CanSeek() const SEOUL_OVERRIDE
	{
		return IsOpen();
	}

	// Attempt to get the current absolute file pointer position.
	virtual Bool GetCurrentPositionIndicator(Int64& riPosition) const SEOUL_OVERRIDE
	{
		if (m_uFileOffset <= (UInt64)Int64Max)
		{
			riPosition = (Int64)m_uFileOffset;
			return true;
		}

		return false;
	}

	// Attempt to relocate the file pointer to the position iPosition. If
	// this method returns true, the file pointer will now be at the position
	// specified by iPosition, in consideration of the mode flag eMode. If this
	// method returns false, the file pointer position is undefined.
	virtual Bool Seek(Int64 iPosition, File::SeekMode eMode) SEOUL_OVERRIDE;

private:
	/** Cached expected modified time of the file being served by Moriarty. */
	UInt64 m_uModifiedTime;

	/**
	 * File size at the time the file was opened (will not be accurate for
	 * writable files)
	 */
	UInt64 m_uFileSize;

	/** Current file offset */
	UInt64 m_uFileOffset;

	/** Remote file path of the file */
	FilePath m_FilePath;

	/** File mode in which the file was opened */
	File::Mode m_eMode;

	/** Remote file handle */
	MoriartyClient::FileHandle m_File;
};

} // namespace Seoul

#endif  // SEOUL_WITH_MORIARTY

#endif // include guard
