/**
 * \file MoriartySyncFile.cpp
 * \brief File class for accessing a file served from a Moriarty server.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FileManager.h"
#include "Logger.h"
#include "MoriartySyncFile.h"
#include "Path.h"

#include <stdint.h>

#if SEOUL_WITH_MORIARTY

namespace Seoul
{

/** Tries to open a remote file */
MoriartySyncFile::MoriartySyncFile(FilePath filePath, File::Mode eMode)
	: m_uModifiedTime(0)
	, m_uFileSize(0)
	, m_uFileOffset(0)
	, m_FilePath(filePath)
	, m_eMode(eMode)
{
	if (MoriartyClient::Get())
	{
		FileStat stat;
		m_File = MoriartyClient::Get()->OpenFile(filePath, eMode, &stat);
		if (m_File != MoriartyClient::kInvalidFileHandle)
		{
			m_uFileSize = stat.m_uFileSize;
			m_uModifiedTime = stat.m_uModifiedTime;
		}
	}
	else
	{
		m_File = MoriartyClient::kInvalidFileHandle;
	}
}

MoriartySyncFile::~MoriartySyncFile()
{
	(void)MoriartyClient::Get()->CloseFile(m_File);
}

/**
 * Attempts to read zSizeInBytes data from the file into pOut. Returns the
 * number of bytes actually read. Will return 0u if CanRead() is false.
 */
UInt32 MoriartySyncFile::ReadRawData(void* pOut, UInt32 zSizeInBytes)
{
	if (!CanRead() || (m_uFileOffset >= m_uFileSize))
	{
		return 0;
	}

	// Don't read past EOF
	zSizeInBytes = (UInt32)Min<UInt64>(zSizeInBytes, m_uFileSize - m_uFileOffset);

	Int64 nBytesRead = MoriartyClient::Get()->ReadFile(m_File, pOut, zSizeInBytes, m_uFileOffset);
	SEOUL_ASSERT(nBytesRead <= (Int64)zSizeInBytes);
	if (nBytesRead <= 0)
	{
		return 0;
	}

	// Cache for future reads - if we just read the entire file, attempt to cache it to local storage.
	if (0 != m_uModifiedTime &&
		0 == m_uFileOffset &&
		(UInt64)nBytesRead == m_uFileSize &&
		nBytesRead <= (Int64)UIntMax)
	{
		// Construct the absolute filename.
		String const sAbsoluteFilename(m_FilePath.GetAbsoluteFilename());

		// Create the directory structure to the filename.
		if (FileManager::Get()->CreateDirPath(Path::GetDirectoryName(sAbsoluteFilename)))
		{
			Bool bSetModifiedTime = false;
			{
				// Write the file.
				ScopedPtr<SyncFile> pFile;
				if (FileManager::Get()->OpenFile(sAbsoluteFilename, File::kWriteTruncate, pFile) && pFile->CanWrite())
				{
					if ((UInt32)nBytesRead == pFile->WriteRawData(pOut, (UInt32)nBytesRead))
					{
						// If successful, mark that we need to update the modified time.
						bSetModifiedTime = true;
					}
				}
			}

			// Update the modified time if specified.
			if (bSetModifiedTime)
			{
				// If setting the modified time fails, delete the file so we don't leave
				// useless cruft around.
				if (!FileManager::Get()->SetModifiedTime(sAbsoluteFilename, m_uModifiedTime))
				{
					(void)FileManager::Get()->Delete(sAbsoluteFilename);
				}
			}
		}
	}

	m_uFileOffset += nBytesRead;
	return (UInt32)nBytesRead;
}

/**
 * Attempts to writes zSizeInBytes of data to pOut buffer from the file.
 * Returns the number of bytes actually written. Will return 0u if
 * CanWrite() is false.
 */
UInt32 MoriartySyncFile::WriteRawData(const void *pIn, UInt32 zSizeInBytes)
{
	if (!CanWrite())
	{
		return 0;
	}

	Int64 nBytesWritten = MoriartyClient::Get()->WriteFile(m_File, pIn, zSizeInBytes, m_uFileOffset);
	SEOUL_ASSERT(nBytesWritten <= (Int64)zSizeInBytes);
	if (nBytesWritten <= 0)
	{
		return 0;
	}

	m_uFileOffset += nBytesWritten;
	return (UInt32)nBytesWritten;
}

/** Returns an absolute filename that identifies this SyncFile. */
String MoriartySyncFile::GetAbsoluteFilename() const
{
	return m_FilePath.GetAbsoluteFilename();
}

/**
 * Return the total current size of the file, in bytes. Returns 0u if the file
 * is empty, or if IsOpen() returns false.
 */
UInt64 MoriartySyncFile::GetSize() const
{
	if (!IsOpen())
	{
		return 0;
	}

	// For read-only files, just use the cached value from when we opened it
	// (we're assuming that nobody is concurrently modifying the file)
	if (!File::CanWrite(m_eMode))
	{
		return m_uFileSize;
	}

	FileStat stat;
	if (MoriartyClient::Get()->StatFile(m_FilePath, &stat))
	{
		return stat.m_uFileSize;
	}
	else
	{
		SEOUL_LOG("MoriartySyncFile::GetSize(): failed to stat file which is currently open\n");
		return 0;
	}
}

/**
 * Attempt to relocate the file pointer to the position iPosition. If
 * this method returns true, the file pointer will now be at the position
 * specified by iPosition, in consideration of the mode flag eMode. If this
 * method returns false, the file pointer position is undefined.
 */
Bool MoriartySyncFile::Seek(Int64 iPosition, File::SeekMode eMode)
{
	if (!IsOpen())
	{
		return false;
	}

	switch(eMode)
	{
	case File::kSeekFromStart:
		// Watch out for underflow
		if (iPosition < 0)
		{
			return false;
		}

		m_uFileOffset = (UInt64)iPosition;
		return true;

	case File::kSeekFromCurrent:
		// Watch out for underflow and overflow
		if (iPosition < 0 && m_uFileOffset <= INT64_MAX && (Int64)m_uFileOffset + iPosition < 0)
		{
			return false;
		}
		else if (iPosition > 0 && m_uFileOffset + (UInt64)iPosition < m_uFileOffset)
		{
			return false;
		}

		m_uFileOffset += iPosition;
		return true;

	case File::kSeekFromEnd:
		// Watch out for underflow and overflow
		if (iPosition < 0 && m_uFileSize <= INT64_MAX && (Int64)m_uFileSize + iPosition < 0)
		{
			return false;
		}
		else if (iPosition > 0 && m_uFileSize + (UInt64)iPosition < m_uFileSize)
		{
			return false;
		}

		m_uFileOffset = m_uFileSize + iPosition;
		return true;

	default:
		SEOUL_FAIL("Invalid enum");
		return false;
	}
}

} // namespace Seoul

#endif  // SEOUL_WITH_MORIARTY
