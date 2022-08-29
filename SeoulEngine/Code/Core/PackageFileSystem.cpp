/**
 * \file PackageFileSystem.cpp
 * \brief Specialization of IFileSystem that services file requests
 * into contiguous package files.
 *
 * Packages are read-only. As a result, packages used by SeoulEngine
 * runtime code must be generated offline using the PackageCooker command-line
 * tool.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Compress.h"
#include "DiskFileSystem.h"
#include "FileManager.h"
#include "Logger.h"
#include "PackageFileSystem.h"
#include "Path.h"
#include "ScopedAction.h"
#include "SeoulCrc32.h"
#include "SeoulFileReaders.h"
#include "UnsafeBuffer.h"

namespace Seoul
{

namespace
{

/**
 * Used to sort the master list of file entries in the package, if
 * the package supports directory queries.
 */
struct FileListSort
{
	Bool operator()(FilePath a, FilePath b) const
	{
		return (STRCMP_CASE_INSENSITIVE(a.CStr(), b.CStr()) < 0);
	}
};

/**
 * Used to resolve a directory query, where sDirectory
 * is expected to contain a normalized, relative directory path
 * to the game directory of the package attempting to satisfy
 * the query.
 */
struct DirectoryQuery
{
	Bool operator()(const String& a, FilePath b) const
	{
		return (STRNCMP_CASE_INSENSITIVE(a.CStr(), b.CStr(), a.GetSize()) < 0);
	}

	Bool operator()(FilePath a, FilePath b) const
	{
		return (STRCMP_CASE_INSENSITIVE(a.CStr(), b.CStr()) < 0);
	}

	Bool operator()(FilePath a, const String& b) const
	{
		return (STRNCMP_CASE_INSENSITIVE(a.CStr(), b.CStr(), b.GetSize()) < 0);
	}
};

struct OffsetSorter SEOUL_SEALED
{
	Bool operator()(const PackageCrc32Entry& a, const PackageCrc32Entry& b) const
	{
		// Very unlikely case but we still want it to be deterministic - can
		// occur if a file has size 0. When this happens, sort by
		// name, which is guaranteed to be strictly ordered, since no
		// two files can have the same name.
		if (a.m_Entry.m_uOffsetToFile == b.m_Entry.m_uOffsetToFile)
		{
			return (strcmp(a.m_FilePath.CStr(), b.m_FilePath.CStr()) < 0);
		}
		else
		{
			return (a.m_Entry.m_uOffsetToFile < b.m_Entry.m_uOffsetToFile);
		}
	}
};

} // namespace anonymous

/**
 * Endian swap a PackageFileEntry.
 */
void EndianSwap(PackageFileEntry& rEntry)
{
	rEntry.m_uCrc32Pre = EndianSwap32(rEntry.m_uCrc32Pre);
	rEntry.m_uModifiedTime = EndianSwap64(rEntry.m_uModifiedTime);
	rEntry.m_uCrc32Post = EndianSwap32(rEntry.m_uCrc32Post);
	rEntry.m_uCompressedFileSize = EndianSwap64(rEntry.m_uCompressedFileSize);
	rEntry.m_uOffsetToFile = EndianSwap64(rEntry.m_uOffsetToFile);
	rEntry.m_uUncompressedFileSize = EndianSwap64(rEntry.m_uUncompressedFileSize);
}

/**
 * Endian swap a PackageFileHeader.
 */
void PackageFileHeader::EndianSwap(PackageFileHeader& rHeader)
{
	// For handling.
	UInt32 uVersion = rHeader.m_uVersion;
	if (EndianSwap32(kuPackageSignature) == rHeader.m_uSignature)
	{
		uVersion = EndianSwap32(uVersion);
	}

	rHeader.m_uSignature = EndianSwap32(rHeader.m_uSignature);
	rHeader.m_uVersion = EndianSwap32(rHeader.m_uVersion);
	if (13u == uVersion)
	{
		auto& r = rHeader.m_V13;
		r.m_bObfuscated = r.m_bObfuscated;
		r.m_bSupportDirectoryQueries = r.m_bSupportDirectoryQueries;
		r.m_uPackageVariation = EndianSwap16(r.m_uPackageVariation);
		r.m_eGameDirectory = EndianSwap16(r.m_eGameDirectory);
		r.m_bCompressedFileTable = EndianSwap16(r.m_bCompressedFileTable);
		r.m_uTotalEntriesInFileTable = EndianSwap32(r.m_uTotalEntriesInFileTable);
		r.m_uBuildChangelist = EndianSwap32(r.m_uBuildChangelist);
		r.m_uBuildVersionMajor = EndianSwap32(r.m_uBuildVersionMajor);
		r.m_uOffsetToFileTableInBytes = EndianSwap64(r.m_uOffsetToFileTableInBytes);
		r.m_uSizeOfFileTableInBytes = EndianSwap32(r.m_uSizeOfFileTableInBytes);
		r.m_uTotalPackageFileSizeInBytes = EndianSwap64(r.m_uTotalPackageFileSizeInBytes);
	}
	else if (uVersion >= 16u && uVersion <= 17u)
	{
		auto& r = rHeader.m_V16a17;
		r.m_bObfuscated = EndianSwap16(r.m_bObfuscated);
		r.m_bSupportDirectoryQueries = EndianSwap16(r.m_bSupportDirectoryQueries);
		r.m_eGameDirectory = EndianSwap16(r.m_eGameDirectory);
		r.m_bCompressedFileTable = EndianSwap16(r.m_bCompressedFileTable);
		r.m_uTotalEntriesInFileTable = EndianSwap32(r.m_uTotalEntriesInFileTable);
		r.m_uBuildChangelist = EndianSwap32(r.m_uBuildChangelist);
		r.m_uBuildVersionMajor = EndianSwap32(r.m_uBuildVersionMajor);
		r.m_uOffsetToFileTableInBytes = EndianSwap64(r.m_uOffsetToFileTableInBytes);
		r.m_uSizeOfFileTableInBytes = EndianSwap32(r.m_uSizeOfFileTableInBytes);
		r.m_uTotalPackageFileSizeInBytes = EndianSwap64(r.m_uTotalPackageFileSizeInBytes);
	}
	else if (uVersion >= 18u && uVersion <= 20u)
	{
		auto& r = rHeader.m_V18a19a20;
		r.m_ePlatform = r.m_ePlatform;
		r.m_bObfuscated = r.m_bObfuscated;
		r.m_bSupportDirectoryQueries = EndianSwap16(r.m_bSupportDirectoryQueries);
		r.m_eGameDirectory = EndianSwap16(r.m_eGameDirectory);
		r.m_bCompressedFileTable = EndianSwap16(r.m_bCompressedFileTable);
		r.m_uTotalEntriesInFileTable = EndianSwap32(r.m_uTotalEntriesInFileTable);
		r.m_uBuildChangelist = EndianSwap32(r.m_uBuildChangelist);
		r.m_uBuildVersionMajor = EndianSwap32(r.m_uBuildVersionMajor);
		r.m_uOffsetToFileTableInBytes = EndianSwap64(r.m_uOffsetToFileTableInBytes);
		r.m_uSizeOfFileTableInBytes = EndianSwap32(r.m_uSizeOfFileTableInBytes);
		r.m_uTotalPackageFileSizeInBytes = EndianSwap64(r.m_uTotalPackageFileSizeInBytes);
	}
	else if (uVersion >= 21u)
	{
		auto& r = rHeader.m_V21;
		r.m_ePlatform = r.m_ePlatform;
		r.m_bObfuscated = r.m_bObfuscated;
		r.m_bSupportDirectoryQueries = EndianSwap16(r.m_bSupportDirectoryQueries);
		r.m_eGameDirectory = EndianSwap16(r.m_eGameDirectory);
		r.m_bCompressedFileTable = EndianSwap16(r.m_bCompressedFileTable);
		r.m_uTotalEntriesInFileTable = EndianSwap32(r.m_uTotalEntriesInFileTable);
		r.m_uBuildChangelist = EndianSwap32(r.m_uBuildChangelist);
		r.m_uPackageVariation = EndianSwap16(r.m_uPackageVariation);
		r.m_uBuildVersionMajor = EndianSwap16(r.m_uBuildVersionMajor);
		r.m_uOffsetToFileTableInBytes = EndianSwap64(r.m_uOffsetToFileTableInBytes);
		r.m_uSizeOfFileTableInBytes = EndianSwap32(r.m_uSizeOfFileTableInBytes);
		r.m_uTotalPackageFileSizeInBytes = EndianSwap64(r.m_uTotalPackageFileSizeInBytes);
	}
}

/**
 * Deobfuscate a block of data based on provided parameters.
 */
void PackageFileSystem::Obfuscate(UInt32 uXorKey, void *pData, size_t zDataSize, Int64 iFileOffset)
{
	UByte *pDataPtr = (UByte *)pData;
	UByte *pEndPtr = pDataPtr + zDataSize;

	Int32 i = (Int32)iFileOffset;
	while (pDataPtr < pEndPtr)
	{
		*pDataPtr ^= (UByte)((uXorKey >> ((i % 4) << 3)) + (i/4) * 101);
		pDataPtr++;
		i++;
	}
}

/**
 * @return An XorKey used to Obfuscate.
 */
UInt32 PackageFileSystem::GenerateObfuscationKey(Byte const* s, UInt32 u)
{
	UInt32 uXorKey = 0x54007b47;  // "shoot bot", roughly
	for (UInt i = 0; i < u; i++)
	{
		uXorKey = uXorKey * 33 + tolower(s[i]);
	}
	return uXorKey;
}

/**
 * Custom variation of ReadBuffer() for filename strings to handle conditional endian swapping.
 */
static Bool ReadFilenameAndExtension(
	SyncFile& file,
	Vector<Byte, MemoryBudgets::Io>& rvWorkArea,
	Bool bEndianSwap,
	Bool bObfuscated,
	String& rsFilenameWithoutExtension,
	String& rsExtension,
	UInt32& ruXorKey)
{
	UInt32 uSizeInBytes = 0u;

	// Get the size of the string (including null terminator).
	if (!ReadUInt32(file, uSizeInBytes))
	{
		return false;
	}

	// Swap the size if needed.
	if (bEndianSwap)
	{
		uSizeInBytes = EndianSwap32(uSizeInBytes);
	}

	// If this fails, the input data is likely corrupted.
	if (uSizeInBytes > kDefaultMaxReadSize)
	{
		return false;
	}

	// Empty, return false.
	if (0 == uSizeInBytes)
	{
		return false;
	}

	// Make room for the string data.
	rvWorkArea.Resize(uSizeInBytes);

	// If this fails, the input data is likely corrupted.
	if (file.ReadRawData(rvWorkArea.Data(), uSizeInBytes) != uSizeInBytes)
	{
		return false;
	}

	// Verify that the input string was written correctly and has
	// a null terminator.
	if ('\0' != rvWorkArea[uSizeInBytes - 1u])
	{
		return false;
	}

	// Generate XOR key prior to normalizing the path. It was
	// derived based on the full relative path with Windows
	// style separators.
	if (bObfuscated)
	{
		ruXorKey = PackageFileSystem::GenerateObfuscationKey(rvWorkArea.Data(), uSizeInBytes - 1u);
	}
	else
	{
		ruXorKey = 0u;
	}

	// Handle directory separator change. The input
	// path has windows separators but is otherwise a
	// normalized, relative path, so we only
	// need to correct separators and split
	// the extension from the base filename.
	if ('\\' != Path::kDirectorySeparatorChar)
	{
		for (auto& e : rvWorkArea)
		{
			if (e == '\\') { e = Path::kDirectorySeparatorChar; }
		}
	}

	// Fine the end of the filename.
	auto uFilenameEnd = uSizeInBytes;
	while (uFilenameEnd > 0u)
	{
		--uFilenameEnd;
		if ('.' == rvWorkArea[uFilenameEnd])
		{
			break;
		}
	}

	// Filename is the entire string if we terminated at index
	// 0 (-1 because uSizeInBytes includes the null terminator).
	if (0u == uFilenameEnd) { uFilenameEnd = (uSizeInBytes - 1); }

	// Assign output - extension always includes the '.' in SeoulEngine
	// convention.
	rsFilenameWithoutExtension.Assign(rvWorkArea.Data(), uFilenameEnd);
	rsExtension.Assign(rvWorkArea.Data() + uFilenameEnd);

	// Data is valid.
	return true;
}

/**
 * Internal class used by PackageFileSystem, actually implements
 * sync file semantics for reading from the package file system.
 */
class PackageSyncFile : public SyncFile
{
public:
	PackageSyncFile(
		PackageFileSystem& rOwner,
		FilePath filePath,
		Int64 iOffsetInPackageFile,
		UInt64 uFileSize,
		UInt32 uXorKey);
	virtual ~PackageSyncFile();

	/**
	 * Read uSizeInBytes data from the file into pOut. Returns the
	 * number of bytes actually read.
	 */
	virtual UInt32 ReadRawData(void *pOut, UInt32 uSizeInBytes) SEOUL_OVERRIDE;

	/**
	 * @return 0 - PackageSyncFile is not writeable.
	 */
	virtual UInt32 WriteRawData(void const* pIn, UInt32 uSizeInBytes) SEOUL_OVERRIDE
	{
		return 0;
	}

	/**
	 * @return An absolute filename that identifies this PackageSyncFile.
	 */
	virtual String GetAbsoluteFilename() const SEOUL_OVERRIDE
	{
		return m_FilePath.GetAbsoluteFilename();
	}

	/**
	 * @return True if this PackageSyncFile was opened successfully,
	 * false otherwise.
	 */
	virtual Bool IsOpen() const SEOUL_OVERRIDE;

	virtual Bool CanRead() const SEOUL_OVERRIDE;

	/**
	 * @return False, PackageSyncFile is never writeable.
	 */
	virtual Bool CanWrite() const SEOUL_OVERRIDE
	{
		return false;
	}

	/**
	 * Nop - PackageSyncFile is not writeable.
	 */
	virtual Bool Flush() SEOUL_OVERRIDE
	{
		return false;
	}

	/**
	 * @return The size of the data loaded by this PackageSyncFile.
	 */
	virtual UInt64 GetSize() const SEOUL_OVERRIDE
	{
		return m_uFileSize;
	}

	/**
	 * @return True if the file is open.
	 */
	virtual Bool CanSeek() const SEOUL_OVERRIDE
	{
		return IsOpen();
	}

	// Attempt to get the current absolute file pointer position.
	virtual Bool GetCurrentPositionIndicator(Int64& riPosition) const SEOUL_OVERRIDE
	{
		riPosition = m_iCurrentOffset;
		return true;
	}

	virtual Bool Seek(Int64 iPosition, File::SeekMode eMode) SEOUL_OVERRIDE;

	PackageFileSystem& GetOwner()
	{
		return m_rOwner;
	}

private:
	void DeObfuscate(void *pData, size_t zDataSize, Int64 iFileOffset);

	PackageFileSystem& m_rOwner;
	FilePath m_FilePath;
	Int64 m_iBaseOffsetInPackageFile;
	Int64 m_iCurrentOffset;
	UInt64 m_uFileSize;
	UInt32 m_uXorKey;

	SEOUL_DISABLE_COPY(PackageSyncFile);
}; // class PackageSyncFile

PackageSyncFile::PackageSyncFile(
	PackageFileSystem& rOwner,
	FilePath filePath,
	Int64 iOffsetInPackageFile,
	UInt64 uFileSize,
	UInt32 uXorKey)
	: m_rOwner(rOwner)
	, m_FilePath(filePath)
	, m_iBaseOffsetInPackageFile(iOffsetInPackageFile)
	, m_iCurrentOffset(0)
	, m_uFileSize(uFileSize)
	, m_uXorKey(uXorKey)
{
	++m_rOwner.m_ActiveSyncFileCount;
}

PackageSyncFile::~PackageSyncFile()
{
	--m_rOwner.m_ActiveSyncFileCount;
}

/**
 * De-obfuscates the given data which was read from the given file offset,
 *
 * See also the _Obfuscate() function in PackageCookerMain.cs.
 */
void PackageSyncFile::DeObfuscate(void *pData, size_t zDataSize, Int64 iFileOffset)
{
	PackageFileSystem::Obfuscate(m_uXorKey, pData, zDataSize, iFileOffset);
}

UInt32 PackageSyncFile::ReadRawData(void *pOut, UInt32 uSizeInBytes)
{
	if (m_iCurrentOffset >= (Int64)m_uFileSize)
	{
		return 0;
	}

	// Synchronize access to the PackageFileSystem resources.
	Lock lock(m_rOwner.m_Mutex);

	// If we're not at the correct offset, seek there
	Int64 iOffset = m_iBaseOffsetInPackageFile + m_iCurrentOffset;
	if (iOffset == m_rOwner.m_iCurrentFileOffset ||
		m_rOwner.m_pPackageFile->Seek(iOffset, File::kSeekFromStart))
	{
		UInt32 uReadSizeInBytes = (UInt32)Min<UInt64>(m_uFileSize - m_iCurrentOffset, uSizeInBytes);
		UInt32 uBytesRead = m_rOwner.m_pPackageFile->ReadRawData(pOut, uReadSizeInBytes);

		// De-obfuscate the data
		if (m_rOwner.IsObfuscated())
		{
			DeObfuscate(pOut, uBytesRead, m_iCurrentOffset);
		}

		m_iCurrentOffset += uBytesRead;
		m_rOwner.m_iCurrentFileOffset = m_iBaseOffsetInPackageFile + m_iCurrentOffset;

		return uBytesRead;
	}
	else
	{
		// Seek failed
		return 0;
	}
}

Bool PackageSyncFile::IsOpen() const
{
	return m_rOwner.m_pPackageFile->IsOpen();
}

Bool PackageSyncFile::CanRead() const
{
	return
		m_rOwner.m_pPackageFile->CanRead() &&
		m_rOwner.m_pPackageFile->CanSeek();
}

/**
 * Seek the next read pointer based on eMode and iPosition.
 */
Bool PackageSyncFile::Seek(Int64 iPosition, File::SeekMode eMode)
{
	switch (eMode)
	{
	case File::kSeekFromCurrent:
		m_iCurrentOffset += iPosition;
		break;
	case File::kSeekFromEnd:
		m_iCurrentOffset = (m_uFileSize - iPosition);
		break;
	case File::kSeekFromStart:
		m_iCurrentOffset = iPosition;
		break;
	default:
		SEOUL_FAIL("Enum out of sync, bug a programmer.\n");
		return false;
	};

	return (
		m_iCurrentOffset >= 0 &&
		m_iCurrentOffset <= (Int64)m_uFileSize);
}

/**
 * Internal class used by PackageFileSystem, actually implements
 * sync file semantics for reading from the package file system when a
 * file is compressed.
 */
class CompressedPackageSyncFile : public SyncFile
{
public:
	CompressedPackageSyncFile(
		PackageFileSystem& rOwner,
		FilePath filePath,
		Int64 iOffsetInPackageFile,
		UInt64 uCompressedFileSize,
		UInt64 uUncompressedFileSize,
		UInt32 uXorKey,
		Bool bUseDict);
	virtual ~CompressedPackageSyncFile();

	/**
	 * Read uSizeInBytes data from the file into pOut. Returns the
	 * number of bytes actually read.
	 */
	virtual UInt32 ReadRawData(void *pOut, UInt32 uSizeInBytes) SEOUL_OVERRIDE;

	/**
	 * @return 0 - CompressedPackageSyncFile is not writeable.
	 */
	virtual UInt32 WriteRawData(void const* pIn, UInt32 uSizeInBytes) SEOUL_OVERRIDE
	{
		return 0;
	}

	/**
	 * @return An absolute filename that identifies this CompressedPackageSyncFile.
	 */
	virtual String GetAbsoluteFilename() const SEOUL_OVERRIDE
	{
		return m_File.GetAbsoluteFilename();
	}

	/**
	 * @return True if this CompressedPackageSyncFile was opened successfully,
	 * false otherwise.
	 */
	virtual Bool IsOpen() const SEOUL_OVERRIDE
	{
		return m_File.IsOpen();
	}

	/**
	 * @return True if this CompressedPackageSyncFile supports a read operation,
	 * false otherwise.
	 */
	virtual Bool CanRead() const SEOUL_OVERRIDE
	{
		return m_File.CanRead();
	}

	/**
	 * @return False, CompressedPackageSyncFile is never writeable.
	 */
	virtual Bool CanWrite() const SEOUL_OVERRIDE
	{
		return false;
	}

	/**
	 * Nop - CompressedPackageSyncFile is not writeable.
	 */
	virtual Bool Flush() SEOUL_OVERRIDE
	{
		return false;
	}

	/**
	 * @return The size of the data loaded by this CompressedPackageSyncFile.
	 */
	virtual UInt64 GetSize() const SEOUL_OVERRIDE
	{
		return m_uUncompressedFileSize;
	}

	/**
	 * @return True if the file is open.
	 */
	virtual Bool CanSeek() const SEOUL_OVERRIDE
	{
		return IsOpen();
	}

	// Attempt to get the current absolute file pointer position.
	virtual Bool GetCurrentPositionIndicator(Int64& riPosition) const SEOUL_OVERRIDE
	{
		riPosition = m_iCurrentOffset;
		return true;
	}

	virtual Bool Seek(Int64 iPosition, File::SeekMode eMode) SEOUL_OVERRIDE;

	// Optimized version to avoid some memory copies in this case.
	virtual Bool ReadAll(
		void*& rpOutputBuffer,
		UInt32& ruOutputSizeInBytes,
		UInt32 uAlignmentOfOutputBuffer,
		MemoryBudgets::Type eOutputBufferMemoryType,
		UInt32 uMaxReadSize = kDefaultMaxReadSize) SEOUL_OVERRIDE;

private:
	Int32 m_iCurrentOffset;
	UInt32 m_uUncompressedFileSize;
	PackageSyncFile m_File;
	void* m_pData;
	Bool const m_bUseDict;

	SEOUL_DISABLE_COPY(CompressedPackageSyncFile);
}; // class CompressedPackageSyncFile

CompressedPackageSyncFile::CompressedPackageSyncFile(
	PackageFileSystem& rOwner,
	FilePath filePath,
	Int64 iOffsetInPackageFile,
	UInt64 uCompressedFileSize,
	UInt64 uUncompressedFileSize,
	UInt32 uXorKey,
	Bool bUseDict)
	: m_iCurrentOffset(0)
	, m_uUncompressedFileSize((UInt32)uUncompressedFileSize)
	, m_File(rOwner, filePath, iOffsetInPackageFile, uCompressedFileSize, uXorKey)
	, m_pData(nullptr)
	, m_bUseDict(bUseDict)
{
}

CompressedPackageSyncFile::~CompressedPackageSyncFile()
{
	if (nullptr != m_pData)
	{
		MemoryManager::Deallocate(m_pData);
	}
}

/**
 * Read uSizeInBytes raw bytes from the file. CompressedPackageSyncFile
 * will read the entire comrpessed data into memory and uncompress it if that
 * has not already been done, and then read from the byte buffer.
 *
 * @return The number of bytes read from the file.
 */
UInt32 CompressedPackageSyncFile::ReadRawData(void *pOut, UInt32 uSizeInBytes)
{
	if (m_iCurrentOffset < 0 || (UInt32)m_iCurrentOffset >= m_uUncompressedFileSize)
	{
		return 0;
	}

	// Read and decompress the entire file if we don't have data yet.
	if (nullptr == m_pData)
	{
		UInt32 uReadSizeInBytes = 0u;
		void* pCompressedData = nullptr;

		// If the file read fail, the entire read operation fails.
		if (!m_File.ReadAll(
			pCompressedData,
			uReadSizeInBytes,
			kLZ4MinimumAlignment,
			MemoryBudgets::Io))
		{
			return 0;
		}

		UInt32 uUncompressedFileSize = 0u;

		// Decompress the data - if this fails, or if the output size differs
		// from what we expected, fail.
		Bool bResult = false;

		// Backwards compatibility.
		if (m_File.GetOwner().GetHeader().IsOldLZ4Compression())
		{
			bResult = LZ4Decompress(
				pCompressedData,
				uReadSizeInBytes,
				m_pData,
				uUncompressedFileSize,
				MemoryBudgets::Io);
		}
		else
		{
			if (m_bUseDict)
			{
				bResult = ZSTDDecompressWithDict(
					m_File.GetOwner().GetDecompressionDict(),
					pCompressedData,
					uReadSizeInBytes,
					m_pData,
					uUncompressedFileSize,
					MemoryBudgets::Io);
			}
			else
			{
				bResult = ZSTDDecompress(
					pCompressedData,
					uReadSizeInBytes,
					m_pData,
					uUncompressedFileSize,
					MemoryBudgets::Io);
			}
		}
		bResult = bResult && (uUncompressedFileSize == m_uUncompressedFileSize);

		MemoryManager::Deallocate(pCompressedData);

		if (!bResult)
		{
			return 0;
		}
	}

	// This condition should have been enforced above.
	SEOUL_ASSERT(m_iCurrentOffset >= 0 && m_uUncompressedFileSize >= (UInt32) m_iCurrentOffset);

	// Actual read size is the minimum of the requested read size and the data remaining in the buffer.
	UInt32 const uActualReadSizeInBytes = Min(uSizeInBytes, (UInt32)(m_uUncompressedFileSize - m_iCurrentOffset));

	// Copy the data to the output buffer.
	memcpy(pOut, (Byte*)(m_pData) + m_iCurrentOffset, uActualReadSizeInBytes);

	// Advance the read pointer.
	m_iCurrentOffset += uActualReadSizeInBytes;

	// Return the bytes read.
	return uActualReadSizeInBytes;
}

/**
 * Seek the next read pointer based on eMode and iPosition.
 */
Bool CompressedPackageSyncFile::Seek(Int64 iPosition, File::SeekMode eMode)
{
	// If the offset is outside the range of a 32-bit int, fail the seek.
	if (iPosition < IntMin || iPosition > IntMax)
	{
		return false;
	}

	// Perform the seek.
	switch (eMode)
	{
	case File::kSeekFromCurrent:
		m_iCurrentOffset += (Int32)iPosition;
		break;
	case File::kSeekFromEnd:
		m_iCurrentOffset = (m_uUncompressedFileSize - (Int32)iPosition);
		break;
	case File::kSeekFromStart:
		m_iCurrentOffset = (Int32)iPosition;
		break;
	default:
		SEOUL_FAIL("Enum out of sync, bug a programmer.\n");
		return false;
	};

	// The seek was successful if the resulting position is still within the range
	// of the file.
	return (
		m_iCurrentOffset >= 0 &&
		(UInt32)m_iCurrentOffset <= m_uUncompressedFileSize);
}

/** Optimized version to avoid some memory copies in this case. */
Bool CompressedPackageSyncFile::ReadAll(
	void*& rpOutputBuffer,
	UInt32& ruOutputSizeInBytes,
	UInt32 uAlignmentOfOutputBuffer,
	MemoryBudgets::Type eOutputBufferMemoryType,
	UInt32 uMaxReadSize /*= kDefaultMaxReadSize*/)
{
	// If the uncompressed data has already been cached, it will be faster
	// to call the base SyncFile::ReadAll(), since it will use ReadRawData(),
	// which will just perform a copy.
	if (nullptr != m_pData)
	{
		return SyncFile::ReadAll(
			rpOutputBuffer,
			ruOutputSizeInBytes,
			uAlignmentOfOutputBuffer,
			eOutputBufferMemoryType,
			uMaxReadSize);
	}

	// If the file is in a valid state, perform the read.
	if (CanRead() && CanSeek())
	{
		// Return to the beginning of the file.
		if (!Seek(0, File::kSeekFromStart))
		{
			return false;
		}

		// Only valid if the uncompressed file size is below the max read.
		const UInt64 uFileSizeInBytes = GetSize();
		if (uFileSizeInBytes <= uMaxReadSize)
		{
			UInt32 uReadSizeInBytes = 0u;
			void* pCompressedData = nullptr;

			// If the file read fail, the entire read operation fails.
			if (!m_File.ReadAll(
				pCompressedData,
				uReadSizeInBytes,
				kLZ4MinimumAlignment,
				MemoryBudgets::Io))
			{
				return false;
			}

			UInt32 uUncompressedFileSize = 0u;
			void* pUncompressedData = nullptr;

			// Decompress the data - if this fails, or if the output size differs
			// from what we expected, fail.
			Bool bResult = false;
			if (m_File.GetOwner().GetHeader().IsOldLZ4Compression())
			{
				bResult = LZ4Decompress(
					pCompressedData,
					uReadSizeInBytes,
					pUncompressedData,
					uUncompressedFileSize,
					eOutputBufferMemoryType,
					uAlignmentOfOutputBuffer);
			}
			else
			{
				if (m_bUseDict)
				{
					bResult = ZSTDDecompressWithDict(
						m_File.GetOwner().GetDecompressionDict(),
						pCompressedData,
						uReadSizeInBytes,
						pUncompressedData,
						uUncompressedFileSize,
						eOutputBufferMemoryType,
						uAlignmentOfOutputBuffer);
				}
				else
				{
					bResult = ZSTDDecompress(
						pCompressedData,
						uReadSizeInBytes,
						pUncompressedData,
						uUncompressedFileSize,
						eOutputBufferMemoryType,
						uAlignmentOfOutputBuffer);
				}
			}

			bResult = bResult && (uUncompressedFileSize == m_uUncompressedFileSize);
			MemoryManager::Deallocate(pCompressedData);

			if (!bResult)
			{
				MemoryManager::Deallocate(pUncompressedData);
				return false;
			}

			// Update the current offset.
			m_iCurrentOffset = (Int32)m_uUncompressedFileSize;

			rpOutputBuffer = pUncompressedData;
			ruOutputSizeInBytes = uUncompressedFileSize;
			return true;
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
 * Given a data block in pData that points at the beginning of a
 * .sar file, validates the header.
 *
 * @return True if pData starts with a valid PackageFileHeader, where valid
 * is defined as:
 * - zDataSizeInBytes is >= the size of the PackageFileHeader.
 * - expected version.
 * - expected signature.
 * - valid game directory value.
 */
Bool PackageFileSystem::CheckSarHeader(void const* pData, size_t zDataSizeInBytes)
{
	PackageFileHeader header;
	return ReadPackageHeader(pData, zDataSizeInBytes, header);
}

/**
 * Given a data block in pData that points at the beginning of a
 * .sar file, validates the header and if valid, outputs the results to rHeader.
 *
 * @return True if pData starts with a valid PackageFileHeader, where valid
 * is defined as:
 * - zDataSizeInBytes is >= the size of the PackageFileHeader.
 * - expected version.
 * - expected signature.
 * - valid game directory value.
 *
 * Currently includes support for loading version 11 archives in addition to latest.
 */
Bool PackageFileSystem::ReadPackageHeader(void const* pData, size_t zDataSizeInBytes, PackageFileHeader& rHeader)
{
	// Empty or too small data is invalid.
	if (nullptr == pData ||
		sizeof(rHeader) > zDataSizeInBytes)
	{
		return false;
	}

	// "Read" the PackageFileHeader.
	PackageFileHeader header;
	memcpy(&header, pData, sizeof(header));

	// If endian swapping is necessary, perform it now.
	if (header.RequiresEndianSwap())
	{
		EndianSwap(header);
	}

	// Validate the header chunk.
	if (kuPackageSignature != header.m_uSignature)
	{
		return false;
	}

	// Validate the header chunk after above fixups.
	if (!header.IsVersionValid())
	{
		return false;
	}
	// Validate the header chunk.
	if (GameDirectory::kUnknown == SerializedToGameDirectory(header.GetGameDirectory()))
	{
		return false;
	}
	if ((Int)header.GetPlatform() < (Int)Platform::SEOUL_PLATFORM_FIRST || (Int)header.GetPlatform() > (Int)Platform::SEOUL_PLATFORM_LAST)
	{
		return false;
	}

	rHeader = header;
	return true;
}

/**
 * Construct this PackageFileSystem with an existing memory buffer
 * that contains the package data.
 */
PackageFileSystem::PackageFileSystem(
	void* pInMemoryPackageData,
	UInt32 uPackageFileSizeInBytes,
	Bool bTakeOwnershipOfData /* = true */,
	const String& sAbsolutePackageFilename /* = String() */)
	: m_sAbsolutePackageFilename(sAbsolutePackageFilename)
	, m_pPackageFile(SEOUL_NEW(MemoryBudgets::Io) FullyBufferedSyncFile(
	pInMemoryPackageData,
	uPackageFileSizeInBytes,
	bTakeOwnershipOfData,
	sAbsolutePackageFilename))
	, m_ePackageGameDirectory(GameDirectory::kUnknown)
	, m_iCurrentFileOffset(0)
	, m_tFileTable()
	, m_vSortedFileList()
	, m_vDictMemory()
	, m_pDecompressionDict(nullptr)
	, m_Header()
	, m_CompressionDictFilePath()
	, m_Mutex()
	, m_ActiveSyncFileCount(0)
	, m_bProcessedCompressionDict(false)
	, m_bHasPostCrc32(false)
	, m_bOk(false)
{
	memset(&m_Header, 0, sizeof(m_Header));
	InternalProcessPackageFile(false);
}

PackageFileSystem::PackageFileSystem(
	const String& sAbsolutePackageFilename,
	Bool bLoadIntoMemory /*= false */,
	Bool bOpenPackageFileWithWritePermissions /*= false*/,
	Bool bDeferCompressionDictProcessing /*= false*/)
	: m_sAbsolutePackageFilename(sAbsolutePackageFilename)
	, m_pPackageFile(nullptr)
	, m_ePackageGameDirectory(GameDirectory::kUnknown)
	, m_iCurrentFileOffset(0)
	, m_tFileTable()
	, m_vSortedFileList()
	, m_vDictMemory()
	, m_pDecompressionDict(nullptr)
	, m_Header()
	, m_CompressionDictFilePath()
	, m_Mutex()
	, m_ActiveSyncFileCount(0)
	, m_bProcessedCompressionDict(false)
	, m_bHasPostCrc32(false)
	, m_bOk(false)
{
	memset(&m_Header, 0, sizeof(m_Header));

	// Sanity check - bLoadIntoMemory and bOpenPackageFileWithWritePermissions are mutually exclusive
	SEOUL_ASSERT(!bLoadIntoMemory || !bOpenPackageFileWithWritePermissions);

	// Open mode.
	File::Mode const eOpenMode = (bOpenPackageFileWithWritePermissions ? File::kReadWrite : File::kRead);

	// Try to load the package file via the FileManager - if this fails, directly grab
	// it via a DiskSyncFile (to handle cases where PackageFileSystem access is prioritized
	// after DiskFileSystem access, and the FileManager does not have a DiskFileSystem yet).
	//
	// We check for the existence of FileManager to handle a few use cases inside tools that do
	// not construct Core dependencies.
	ScopedPtr<SyncFile> pPackageFile;
	if (!FileManager::Get() || !FileManager::Get()->OpenFile(sAbsolutePackageFilename, eOpenMode, pPackageFile))
	{
		pPackageFile.Reset(SEOUL_NEW(MemoryBudgets::Io) DiskSyncFile(m_sAbsolutePackageFilename, eOpenMode));
	}

	// If true, we load the entire package file into memory by using a
	// FullyBufferedSyncFile as the package file type, instead of a DiskSyncFile.
	if (bLoadIntoMemory)
	{
		m_pPackageFile.Reset(SEOUL_NEW(MemoryBudgets::Io) FullyBufferedSyncFile(*pPackageFile));
	}
	else
	{
		m_pPackageFile.Swap(pPackageFile);
	}

	InternalProcessPackageFile(bDeferCompressionDictProcessing);
}

PackageFileSystem::~PackageFileSystem()
{
	// Sanity check - if not true, a crash is likely
	// imminent, since a PackageSyncFile has dangling
	// references to this PackageFileSystem.
	SEOUL_ASSERT(0 == m_ActiveSyncFileCount);

	// Free up the decompression dictionary.
	ZSTDFreeDecompressionDict(m_pDecompressionDict);
}

/**
 * Low level operation - advanced usage only!
 *
 * Commits an update to the sar data on disk. No validation is performed, this method assumes
 * that you know what you are doing.
 */
Bool PackageFileSystem::CommitChangeToSarFile(
	void const* pData,
	UInt32 uSizeInBytes,
	Int64 iWritePositionFromStartOfArchive)
{
	// Synchronize access to the PackageFileSystem resources.
	Lock lock(m_Mutex);

	// If this PackageFileSystem was not open to allow write operations, we're done.
	if (!m_pPackageFile->CanWrite())
	{
		return false;
	}

	// Update the file system position to the target.
	if (!m_pPackageFile->Seek(iWritePositionFromStartOfArchive, File::kSeekFromStart))
	{
		// Failed seek operations may leave the position in an unknown state, so
		// set the cached value to an invalid position.
		m_iCurrentFileOffset = -1;

		return false;
	}

	// Perform the write operation.
	Bool const bReturn = (uSizeInBytes == m_pPackageFile->WriteRawData(pData, uSizeInBytes));

	// If the write operation succeeded, flush the file and update the cached write position.
	if (bReturn)
	{
		// Update the current file offset.
		m_iCurrentFileOffset = (iWritePositionFromStartOfArchive + (Int64)uSizeInBytes);
	}
	// Otherwise, attempt to handle the error.
	else
	{
		// Worst case fallback - if this fails for some reason, set the file offset to
		// an invalid value so it will force the next standard operation to seek.
		if (!m_pPackageFile->Seek(m_iCurrentFileOffset, File::kSeekFromStart))
		{
			m_iCurrentFileOffset = -1;
		}
	}

	// Done.
	return bReturn;
}

// Low level operation - advanced usage only!
//
// Force a blocking flush (fsync) for any writes.
Bool PackageFileSystem::FlushChanges()
{
	// Synchronize access to the PackageFileSystem resources.
	Lock lock(m_Mutex);

	// If this PackageFileSystem was not open to allow write operations, we're done.
	if (!m_pPackageFile->CanWrite())
	{
		return false;
	}

	return m_pPackageFile->Flush();
}

Bool PackageFileSystem::ReadRaw(
	UInt64 uOffsetToDataInFile,
	void* pBuffer,
	UInt32 uSizeInBytes)
{
	// Synchronize access to the PackageFileSystem resources.
	Lock lock(m_Mutex);

	// Update the file system position to the target.
	if (!m_pPackageFile->Seek(uOffsetToDataInFile, File::kSeekFromStart))
	{
		// Failed seek operations may leave the position in an unknown state, so
		// set the cached value to an invalid position.
		m_iCurrentFileOffset = -1;

		return false;
	}

	// Perform the read operation.
	Bool const bReturn = (uSizeInBytes == m_pPackageFile->ReadRawData(pBuffer, uSizeInBytes));

	// If the read operation succeeded, cached the new position.
	if (bReturn)
	{
		// Update the current file offset.
		m_iCurrentFileOffset = (uOffsetToDataInFile + (Int64)uSizeInBytes);
	}
	// Otherwise, attempt to handle the error.
	else
	{
		// Worst case fallback - if this fails for some reason, set the file offset to
		// an invalid value so it will force the next standard operation to seek.
		if (!m_pPackageFile->Seek(m_iCurrentFileOffset, File::kSeekFromStart))
		{
			m_iCurrentFileOffset = -1;
		}
	}

	// Done.
	return bReturn;
}

/**
 * @return True if filePath is stored in this PackageFileSystem,
 * false otherwise. If this method returns true, ruFileSize will
 * contain the size of the file.
 */
Bool PackageFileSystem::GetFileSize(
	FilePath filePath,
	UInt64& ruFileSize) const
{
	PackageFileTableEntry entry;
	if (m_tFileTable.GetValue(filePath, entry))
	{
		ruFileSize = entry.m_Entry.m_uUncompressedFileSize;
		return true;
	}

	return false;
}

/**
 * @return True if filePath is stored in this PackageFileSystem,
 * false otherwise. If this method returns true, ruFileModifiedTime will
 * contain the last modification time of the file when it was stored in
 * the PackageFileSystem.
 */
Bool PackageFileSystem::GetModifiedTime(
	FilePath filePath,
	UInt64& ruModifiedTime) const
{
	PackageFileTableEntry entry;
	if (m_tFileTable.GetValue(filePath, entry))
	{
		ruModifiedTime = entry.m_Entry.m_uModifiedTime;
		return true;
	}

	return false;
}

/**
 * @return True if filePath is in this PackageFileSystem, false
 * otherwise.
 */
Bool PackageFileSystem::Exists(FilePath filePath) const
{
	return (m_tFileTable.HasValue(filePath));
}

Bool PackageFileSystem::Open(
	FilePath filePath,
	File::Mode eMode,
	ScopedPtr<SyncFile>& rpFile)
{
	if (m_pPackageFile->CanRead() && File::kRead == eMode)
	{
		PackageFileTableEntry entry;
		if (m_tFileTable.GetValue(filePath, entry))
		{
			// If the uncompressed size is equal to the compressed size, just use a regular
			// PackageSyncFile.
			if (entry.m_Entry.m_uUncompressedFileSize == entry.m_Entry.m_uCompressedFileSize)
			{
				rpFile.Reset(SEOUL_NEW(MemoryBudgets::Io) PackageSyncFile(
					*this,
					filePath,
					(Int64)entry.m_Entry.m_uOffsetToFile,
					entry.m_Entry.m_uCompressedFileSize,
					entry.m_uXorKey));
			}
			// Otherwise, the data is compressed, use a CompressedPacakageSyncFile.
			else
			{
				// Decide if this read should use the compression dict or not -
				// must be non-null and must not be reading the compression
				// dict itself.
				auto const bUseDict = (
					m_bProcessedCompressionDict &&
					nullptr != m_pDecompressionDict &&
					filePath != m_CompressionDictFilePath);

				rpFile.Reset(SEOUL_NEW(MemoryBudgets::Io) CompressedPackageSyncFile(
					*this,
					filePath,
					(Int64)entry.m_Entry.m_uOffsetToFile,
					entry.m_Entry.m_uCompressedFileSize,
					entry.m_Entry.m_uUncompressedFileSize,
					entry.m_uXorKey,
					bUseDict));
			}

			return true;
		}
	}

	return false;
}

/**
 * @return True if a directory listing for filePath could
 * be generated, false otherwise. If this method returns true, then
 * rvResults will contain a file/directory listing, based on the
 * remaining arguments to this function. If this method returns false,
 * rvResults will be left unmodified.
 */
Bool PackageFileSystem::GetDirectoryListing(
	FilePath dirPath,
	Vector<String>& rvResults,
	Bool bIncludeDirectoriesInResults,
	Bool bRecursive,
	const String& sFileExtension) const
{
	// Directories in results not supported.
	if (bIncludeDirectoriesInResults)
	{
		return false;
	}

	// The file path must be under our package directory
	if (dirPath.GetDirectory() != m_ePackageGameDirectory)
	{
		return false;
	}

	// Can only service directory queries if we have a sorted
	// list of files, which only happens if the package
	// was generated with directory query support enabled.
	if (m_vSortedFileList.IsEmpty())
	{
		return false;
	}

	// Generate a relative path to the directory
	String sRelativeDirectoryPath = dirPath.GetRelativeFilename();

	// The start of potentially valid entries will either be the entire list
	// (if the relative directory is empty) or the lower bound of a binary
	// search of the list of total entries.
	DirectoryQuery directoryQuery;
	auto const iBegin = (sRelativeDirectoryPath.IsEmpty())
		? m_vSortedFileList.Begin()
		: LowerBound(m_vSortedFileList.Begin(), m_vSortedFileList.End(), sRelativeDirectoryPath, directoryQuery);

	// The end of potentially valid entries will either be the entire list
	// (if the relative directory is empty) or the upper bound of a binary
	// search of the list of total entries.
	auto const iEnd = (sRelativeDirectoryPath.IsEmpty())
		? m_vSortedFileList.End()
		: UpperBound(
		m_vSortedFileList.Begin(),
		m_vSortedFileList.End(),
		sRelativeDirectoryPath,
		directoryQuery);

	// Sanity check.
	SEOUL_ASSERT(iBegin <= iEnd);

	rvResults.Clear();

	// Walk the list of entries - if they pass other checks,
	// add them to the results list.
	for (auto i = iBegin; iEnd != i; ++i)
	{
		// Only valid if no file type was provided, or if
		// the file type matches the type provided.
		auto const& sExt = FileTypeToCookedExtension(i->GetType());
		if (sFileExtension.IsEmpty() || 0 == sExt.CompareASCIICaseInsensitive(sFileExtension))
		{
			// Include all entries if recursive, or include the entry
			// if the directory of the entry matches the relative directory
			// that was generated.
			String s(i->GetRelativeFilenameWithoutExtension().ToString());
			if (bRecursive ||
				(Path::GetDirectoryName(s) == sRelativeDirectoryPath))
			{
				rvResults.PushBack(Path::Combine(
					GameDirectoryToString(m_ePackageGameDirectory),
					s + sExt));
			}
		}
	}

	return true;
}

/**
 * Convenience, return the entire file table as a PackageCrc32Entries vector, properly sorted
 * so that entries are in file offset order.
 */
void PackageFileSystem::GetFileTableAsEntries(PackageCrc32Entries& rv) const
{
	// Reserve, iterate and populate.
	rv.Clear();
	rv.Reserve(m_tFileTable.GetSize());
	{
		PackageCrc32Entry entry;
		for (auto const& pair : m_tFileTable)
		{
			entry.m_bCrc32Ok = false;
			entry.m_FilePath = pair.First;
			entry.m_Entry = pair.Second.m_Entry;
			rv.PushBack(entry);
		}
	}

	// Finally, sort.
	OffsetSorter sorter;
	QuickSort(rv.Begin(), rv.End(), sorter);
}

/**
 * Perform a CRC-32 check on all files in this PackageFileSystem, returns true if all
 * pass, false otherwise.
 *
 * Exact behavior of this method depends on the value of pvInOutEntries:
 * - if null, this function checks all files until a file fails the
 *   CRC32 check, at which point it early outs and returns false. Otherwise,
 *   it returns true.
 * - if non-null but empty, this function behaves similarly to when
 *   the pvInOutEntries argument is null, except that is does not early
 *   out. Also, pvInOutEntries will be populated with all files in
 *   this archive and their CRC32 state (true or false depending on
 *   the state on disk).
 * - if non-null and not empty, this function only checks crc32 for
 *   the files listed. On output, crc32 state and entry state will
 *   be popualted.
 *
 * This method is expensive - you probably want to call it off the main thread.
 */
Bool PackageFileSystem::PerformCrc32Check(PackageCrc32Entries* pvInOutEntries /*= nullptr*/)
{
	// Target read size - we read file in chunks no larger than this size
	// and verify them in batches.
	static const UInt32 kuTargetReadSize = 4096;

	// Max overflow size - this is how far apart a file can be from its adjacent file
	// such that we include it in a read. This is "extra" data.
	static const UInt32 kuOverflowSize = 128u;

	// Done immediately if the basic structure of the package is not valid
	// (this means we have not a valid header or file table or both).
	if (!IsOk())
	{
		// If pvInOutEntries is non-null and not-empty, set the state
		// we can set (CRC32).
		if (nullptr != pvInOutEntries && !pvInOutEntries->IsEmpty())
		{
			for (auto& e : *pvInOutEntries)
			{
				e.m_bCrc32Ok = false;
				e.m_Entry = PackageFileEntry{};
			}
		}

		return false;
	}

	// Accumulate files order sorted by file offset for efficient seeking.
	// Use a local vector if pvEntries was null.
	auto const bEntriesOut = (nullptr != pvInOutEntries);
	PackageCrc32Entries vEntries;
	if (!bEntriesOut)
	{
		pvInOutEntries = &vEntries;
	}

	// Now perform - need to populate pvInOutEntries if
	// not already populated, otherwise we just need to fill
	// in the output fields.
	if (pvInOutEntries->IsEmpty())
	{
		GetFileTableAsEntries(*pvInOutEntries);
	}
	else
	{
		PackageFileTableEntry tableEntry;

		// Enumerate pvInOutEntries - remove entries
		// we don't have and fill in the entry field of entries
		// we do.
		Int32 iCount = (Int32)pvInOutEntries->GetSize();
		for (Int32 i = 0; i < iCount; ++i)
		{
			// Get the entry to update its out fields.
			auto& entry = (*pvInOutEntries)[i];

			// Always false initially.
			entry.m_bCrc32Ok = false;

			// If we have an entry, assign it.
			if (m_tFileTable.GetValue(entry.m_FilePath, tableEntry))
			{
				entry.m_Entry = tableEntry.m_Entry;
			}
			// Otherwise, remove it from the in-out vector.
			else
			{
				Swap(entry, (*pvInOutEntries)[iCount - 1]);
				--iCount;
				--i;
			}
		}

		// Trim to remove any entries that we removed.
		pvInOutEntries->Resize((UInt32)iCount);

		// Sort.
		OffsetSorter sorter;
		QuickSort(pvInOutEntries->Begin(), pvInOutEntries->End(), sorter);
	}

	// If trimming results in 0 entries, early out. Return true
	// in this case, although the proper return value here is
	// unclear.
	if (pvInOutEntries->IsEmpty())
	{
		return true;
	}

	// Fallback handling if we don't have post crc32 values.
	if (!HasPostCrc32())
	{
		// Special body that is also more expensive for supporting
		// old archives.
		return InternalPerformPreCrc32Check(*pvInOutEntries, bEntriesOut);
	}

	// Process - lock for the remainder of this processing.
	Lock lock(m_Mutex);

	// Iterate and process.
	Bool bReturn = true;
	UInt32 uBufferSize = kuTargetReadSize;
	void* pBuffer = MemoryManager::Allocate(kuTargetReadSize, MemoryBudgets::Io);
	auto const scoped(MakeScopedAction([]() {}, [&]()
	{
		auto p = pBuffer;
		pBuffer = nullptr;

		MemoryManager::Deallocate(p);
	}));

	auto const uEntries = pvInOutEntries->GetSize();

	// Base offset for data in the current fetched buffer.
	auto const fetch = [&](UInt32 i)
	{
		// Compute how many entries we can prefetch - always include the first.
		auto const& first = (*pvInOutEntries)[i];
		auto pPrev = &first;
		UInt32 uToRead = (UInt32)first.m_Entry.m_uCompressedFileSize;
		++i;

		// Now batch up as many as possible while considering
		// our tuning constants.
		while (i < uEntries)
		{
			// Acquire next.
			auto const& next = (*pvInOutEntries)[i];

			// Overflow is the extra bytes between
			// the end of the previous entry
			// and the start of the next entry.
			auto const uOverflow = (UInt32)(next.m_Entry.m_uOffsetToFile - (pPrev->m_Entry.m_uOffsetToFile + pPrev->m_Entry.m_uCompressedFileSize));

			// Break this set if overflow is too high.
			if (uOverflow > kuOverflowSize)
			{
				break;
			}

			// Next is the total size we will add to the running total
			// if we include the next entry - this is the size of the next
			// entry plus any overflow between previous and next.
			auto const uNext = (UInt32)next.m_Entry.m_uCompressedFileSize + uOverflow;

			// Skip if the next entry would put us over the total target batch
			// size.
			if (uToRead + uNext > kuTargetReadSize)
			{
				break;
			}

			// Otherwise, increase and advance.
			i++;
			uToRead += uNext;
			pPrev = &next;
		}

		// Size buffer as needed.
		if (uToRead > uBufferSize)
		{
			pBuffer = MemoryManager::Reallocate(pBuffer, uToRead, MemoryBudgets::Io);
			uBufferSize = uToRead;
		}

		// Cache starting offset of the entire batch.
		auto const iTargetOffset = (Int64)first.m_Entry.m_uOffsetToFile;

		// If we're not at the correct offset, seek to that position.
		if (m_iCurrentFileOffset != iTargetOffset)
		{
			// Fail on seek fail.
			if (!m_pPackageFile->Seek(iTargetOffset, File::kSeekFromStart))
			{
				// Return 0u entries to indicate failure.
				return 0u;
			}

			// Update offset.
			m_iCurrentFileOffset = iTargetOffset;
		}

		// Read the data.
		auto const uRead = m_pPackageFile->ReadRawData(pBuffer, uToRead);

		// Advance offset by read amount.
		m_iCurrentFileOffset += uRead;

		// On read failure, fail the query.
		if (uRead != uToRead)
		{
			// Return 0u entries to indicate failure.
			return 0u;
		}

		// Done, successful. Return the end "iterator",
		// which will always be >= 1u (which allows 0u
		// to be a failure value).
		return i;
	};

	// For all entries, read, check, and update.
	for (UInt32 i = 0u; i < uEntries; )
	{
		// Fetch the next n to read.
		auto const uEnd = fetch(i);

		// End of 0u means a fetch failure, handle this
		// as an error for all remaining entries.
		if (0u == uEnd)
		{
			// If writing output state, update.
			if (bEntriesOut)
			{
				for (; i < uEntries; ++i)
				{
					auto& res = (*pvInOutEntries)[i];
					res.m_bCrc32Ok = false;
				}
			}

			// Done, failure.
			return false;
		}

		// Now process the set.
		auto const uBase = (*pvInOutEntries)[i].m_Entry.m_uOffsetToFile;
		for (; i < uEnd; ++i)
		{
			auto& res = (*pvInOutEntries)[i];
			const PackageFileEntry& entry = res.m_Entry;

			// Fail if an entry is too big (need to change the package format
			// to only support 32-bit sizes since this case never happens in
			// practice).
			if (entry.m_uCompressedFileSize > UIntMax)
			{
				// If writing output state, continue. Otherwise, we can return immediately.
				if (bEntriesOut)
				{
					res.m_bCrc32Ok = false;
					bReturn = false;
					continue;
				}
				else
				{
					return false;
				}
			}

			// Skip if the file is size 0. Immediately ok.
			if (0 == entry.m_uCompressedFileSize)
			{
				if (bEntriesOut)
				{
					res.m_bCrc32Ok = true;
				}
				continue;
			}

			// Size to check.
			auto const uFileSize = (UInt32)entry.m_uCompressedFileSize;

			// Offset into buffer.
			auto const uOffset = (UInt32)(entry.m_uOffsetToFile - uBase);

			// Now compute the CRC32 for this entry.
			auto const uActualCrc32Post = Seoul::GetCrc32(
				(UInt8 const*)pBuffer + uOffset,
				uFileSize);

			// Fail if comparison fails.
			if (entry.m_uCrc32Post != uActualCrc32Post)
			{
				// If writing output state, continue. Otherwise, we can return immediately.
				if (bEntriesOut)
				{
					res.m_bCrc32Ok = false;
					bReturn = false;
					continue;
				}
				else
				{
					return false;
				}
			}

			// If we get here and we're writing output state, we can mark this entry
			// as valid.
			if (bEntriesOut)
			{
				res.m_bCrc32Ok = true;
			}
		}
	}

	return bReturn;
}

/**
 * Perform a CRC-32 check on an individual file in this PackageFileSystem.
 *
 * This method returns false if filePath is not present in the PackageFileSystem.
 */
Bool PackageFileSystem::PerformCrc32Check(FilePath filePath)
{
	// Done immediately if the basic structure of the package is not valid
	// (this means we have not a valid header or file table or both).
	if (!IsOk())
	{
		return false;
	}

	// Lookup the entry for the given file.
	auto pEntry = m_tFileTable.Find(filePath);
	if (nullptr == pEntry)
	{
		return false;
	}

	// Fail if an entry is too big (need to change the package format
	// to only support 32-bit sizes since this case can never happen
	// in practice, individual files are never bigger than 2^32).
	auto const& entry = pEntry->m_Entry;
	if (entry.m_uCompressedFileSize > UIntMax ||
		entry.m_uUncompressedFileSize > UIntMax)
	{
		return false;
	}

	// Immediate success on file size of 0.
	if (0 == entry.m_uCompressedFileSize)
	{
		return true;
	}

	// Allocate a buffer for checking.
	auto const uSize = (UInt32)(m_bHasPostCrc32 ? entry.m_uCompressedFileSize : entry.m_uUncompressedFileSize);
	void* pBuffer = MemoryManager::Allocate(uSize, MemoryBudgets::Io);
	auto const scoped(MakeScopedAction([]() {},
	[&]()
	{
		auto p = pBuffer;
		pBuffer = nullptr;
		MemoryManager::Deallocate(p);
	}));

	// Lock for actual check.
	if (m_bHasPostCrc32)
	{
		Bool bReturn = false;
		UInt32 uCrc32Post = 0u;
		{
			Lock lock(m_Mutex);
			bReturn = InsideLockComputeCrc32Post(entry, pBuffer, uCrc32Post);
		}

		// Return results.
		return (bReturn && (uCrc32Post == entry.m_uCrc32Post));
	}
	else
	{
		{
			ScopedPtr<SyncFile> pFile;
			if (!Open(filePath, File::kRead, pFile) ||
				uSize != pFile->ReadRawData(pBuffer, uSize))
			{
				return false;
			}
		}

		// Return results.
		auto const uCrc32Pre = Seoul::GetCrc32((UInt8 const*)pBuffer, uSize);
		return (uCrc32Pre == entry.m_uCrc32Pre);
	}
}

Bool PackageFileSystem::ProcessCompressionDict()
{
	// Early out if already processed.
	if (m_bProcessedCompressionDict)
	{
		return true;
	}

	// Early out if no dict.
	if (!m_CompressionDictFilePath.IsValid())
	{
		return true;
	}

	// Popupate the compression dictionary, then remove it from the file
	// table, if present.
	DictMemory vDict;
	{
		PackageFileTableEntry entry;
		if (m_tFileTable.GetValue(m_CompressionDictFilePath, entry))
		{
			// Sanity check - must not be zero and must
			// not be above max read size.
			if (entry.m_Entry.m_uUncompressedFileSize == 0u ||
				entry.m_Entry.m_uUncompressedFileSize > kDefaultMaxReadSize)
			{
				m_sLoadError.Assign(String::Printf("Failed reading package file \"%s\", compression dictionary exists "
					"but has invalid size %" PRIu64 "\n",
					m_sAbsolutePackageFilename.CStr(),
					entry.m_Entry.m_uUncompressedFileSize));
				return false;
			}

			// Bind and read.
			{
				ScopedPtr<SyncFile> pFile;
				if (!Open(m_CompressionDictFilePath, File::kRead, pFile))
				{
					m_sLoadError.Assign(String::Printf("Failed reading package file \"%s\", unexpected error opening "
						"compression dict for read.\n",
						m_sAbsolutePackageFilename.CStr()));
					return false;
				}

				// Read the entire dict in our buffer.
				vDict.Resize((UInt32)pFile->GetSize());
				if (vDict.GetSizeInBytes() != pFile->ReadRawData(vDict.Data(), vDict.GetSizeInBytes()))
				{
					m_sLoadError.Assign(String::Printf("Failed reading package file \"%s\", compression dictionary exists "
						"but file read failure occured while loading file.",
						m_sAbsolutePackageFilename.CStr()));
					return false;
				}
			}
		}
	}

	// Synchronization lock and completion.
	Lock lock(m_Mutex);

	// Now that we're locked, early out if another thread already
	// populate the dict.
	if (m_bProcessedCompressionDict)
	{
		return true;
	}

	// If we get here, should be the one and only, so sanity check.
	SEOUL_ASSERT(m_vDictMemory.IsEmpty());
	SEOUL_ASSERT(nullptr == m_pDecompressionDict);

	// Done - swap in the dict, then create the decompression object.
	m_vDictMemory.Swap(vDict);
	if (!m_vDictMemory.IsEmpty())
	{
		m_pDecompressionDict = ZSTDCreateDecompressionDictWeak(m_vDictMemory.Data(), m_vDictMemory.GetSizeInBytes());

		// If m_pDecompressionDict is now null, return failure.
		if (nullptr == m_pDecompressionDict)
		{
			return false;
		}
	}

	// Must be last.
	SeoulMemoryBarrier();
	m_bProcessedCompressionDict = true;

	return true;
}

/**
 * Common/fast case, uses the "post" CRC32 value for
 * checking (post CRC32 is computed after any package level
 * obfuscation or compression and can be used without
 * deobfuscation or decompression of the data).
 */
Bool PackageFileSystem::InsideLockComputeCrc32Post(const PackageFileEntry& entry, void* pBuffer, UInt32& ruCrc32)
{
	// Cache offset.
	auto const iTargetOffset = (Int64)entry.m_uOffsetToFile;

	// If we're not at the correct offset, seek there
	if (m_iCurrentFileOffset != iTargetOffset)
	{
		// Fail on seek fail.
		if (!m_pPackageFile->Seek(iTargetOffset, File::kSeekFromStart))
		{
			return false;
		}

		// Update offset.
		m_iCurrentFileOffset = iTargetOffset;
	}

	// Read the data then compute the crc32 from it.
	auto const uToRead = (UInt32)entry.m_uCompressedFileSize;
	auto const uRead = m_pPackageFile->ReadRawData(pBuffer, uToRead);

	// Advance offset by read amount.
	m_iCurrentFileOffset += uRead;

	// On read failure, fail the query.
	if (uRead != uToRead)
	{
		return false;
	}

	// Return computed crc32.
	ruCrc32 = Seoul::GetCrc32((UInt8 const*)pBuffer, uRead);
	return true;
}

/**
 * Fallback for old version archives. Perform CRC32 check
 * using pre crc32 values (values generated against the
 * data before obfuscation or compression has been applied).
 *
 * Note common, exists exclusively for backwards compatibility
 * with old format .sar files.
 */
Bool PackageFileSystem::InternalPerformPreCrc32Check(PackageCrc32Entries& rv, Bool bEntriesOut)
{
	Bool bReturn = true;
	for (auto& e : rv)
	{
		auto const& entry = e.m_Entry;

		// Skip if the file is size 0. Immediately ok.
		if (0 == entry.m_uCompressedFileSize)
		{
			if (bEntriesOut)
			{
				e.m_bCrc32Ok = true;
			}
			continue;
		}

		// For each file, read in the contents, perform a Crc32 calculation,
		// deallocate the contents, and then check the file size and Crc32
		// against expected values.
		void* pBuffer = nullptr;
		UInt32 uFileSizeInBytes = 0u;
		{
			ScopedPtr<SyncFile> pFile;
			if (!Open(e.m_FilePath, File::kRead, pFile) ||
				!pFile->ReadAll(pBuffer, uFileSizeInBytes, 0u, MemoryBudgets::Io))
			{
				// If writing output state, continue. Otherwise, we can return immediately.
				if (bEntriesOut)
				{
					e.m_bCrc32Ok = false;
					bReturn = false;
					continue;
				}
				else
				{
					return false;
				}
			}
		}

		// Compute the crc32 value.
		UInt32 const uCrc32Pre = Seoul::GetCrc32((UInt8 const*)pBuffer, (size_t)uFileSizeInBytes);
		MemoryManager::Deallocate(pBuffer);
		pBuffer = nullptr;

		// Fail on file size or crc32 value mismatch.
		if (uFileSizeInBytes != entry.m_uUncompressedFileSize ||
			uCrc32Pre != entry.m_uCrc32Pre)
		{
			// If writing output state, continue. Otherwise, we can return immediately.
			if (bEntriesOut)
			{
				e.m_bCrc32Ok = false;
				bReturn = false;
				continue;
			}
			else
			{
				return false;
			}
		}

		// Set true, verified.
		e.m_bCrc32Ok = true;
	}

	return bReturn;
}

void PackageFileSystem::InternalProcessPackageFile(Bool bDeferCompressionDictProcessing)
{
	if (m_pPackageFile->CanRead())
	{
		PackageFileHeader inHeader;
		if (sizeof(inHeader) != m_pPackageFile->ReadRawData(
			reinterpret_cast<void*>(&inHeader),
			sizeof(inHeader)))
		{
			m_sLoadError.Assign(String::Printf("Failed reading package file \"%s\", an error occured "
				"reading the package header.\n",
				m_sAbsolutePackageFilename.CStr()));

			return;
		}

		// Record whether the header needed endian swapping.
		Bool const bEndianSwap = inHeader.RequiresEndianSwap();

		// Read the header - this handles endian swapping and some compatibility.
		PackageFileHeader header;
		if (!ReadPackageHeader(&inHeader, sizeof(inHeader), header))
		{
			m_sLoadError.Assign(String::Printf("Failed reading package file \"%s\", an error occured "
				"reading the package header.\n",
				m_sAbsolutePackageFilename.CStr()));

			return;
		}

		if (m_pPackageFile->GetSize() != header.GetTotalPackageFileSizeInBytes())
		{
			m_sLoadError.Assign(String::Printf("Failed reading package file \"%s\", expected size "
				"is %" PRIu64 ", actual file size is %" PRIu64 ".\n",
				m_sAbsolutePackageFilename.CStr(),
				header.GetTotalPackageFileSizeInBytes(),
				m_pPackageFile->GetSize()));

			return;
		}

		auto const eDirectory = SerializedToGameDirectory(header.GetGameDirectory());
		Bool bHasPostCrc32 = false;
		if (InternalProcessPackageFileTable(header, bHasPostCrc32, bEndianSwap))
		{
			m_ePackageGameDirectory = eDirectory;
			m_Header = header;
			m_bHasPostCrc32 = bHasPostCrc32;
			auto const compressionDictFilePath = FilePath::CreateFilePath(
				eDirectory,
				String::Printf(ksPackageCompressionDictNameFormat, kaPlatformNames[(UInt32)m_Header.GetPlatform()]));

			// If a compression dict exists, cache it.
			if (m_tFileTable.HasValue(compressionDictFilePath))
			{
				m_CompressionDictFilePath = compressionDictFilePath;
			}

			// Mark as ok - may be temporary.
			m_bOk = true;

			// If using a compression dict and not deferring, process it now.
			if (m_CompressionDictFilePath.IsValid() &&
				!bDeferCompressionDictProcessing)
			{
				// Failure to process in this case marks the package
				// file as "not ok".
				if (!ProcessCompressionDict())
				{
					m_bOk = false;
					m_bHasPostCrc32 = false;
					m_CompressionDictFilePath = FilePath();
					m_Header = PackageFileHeader{};
					m_ePackageGameDirectory = GameDirectory::kUnknown;

					FileList vEmpty;
					m_vSortedFileList.Swap(vEmpty);

					FileTable tEmpty;
					m_tFileTable.Swap(tEmpty);
				}
			}
		}
	}
	else
	{
		m_sLoadError.Assign(String::Printf("Cannot read package file \"%s\"\n",
			m_sAbsolutePackageFilename.CStr()));
	}
}

Bool PackageFileSystem::InternalProcessPackageFileTable(
	const PackageFileHeader& header,
	Bool& rbHasPostCrc32,
	Bool bEndianSwap)
{
	if (header.GetOffsetToFileTableInBytes() > (UInt64)Int64Max)
	{
		m_sLoadError.Assign(String::Printf("Failed reading package file \"%s\", package file table "
			"is at invalid file position %" PRIu64 ".\n",
			m_sAbsolutePackageFilename.CStr(),
			header.GetOffsetToFileTableInBytes()));

		return false;
	}

	if (!m_pPackageFile->Seek((Int64)header.GetOffsetToFileTableInBytes(), File::kSeekFromStart))
	{
		m_sLoadError.Assign(String::Printf("Failed reading package file \"%s\", could not seek to "
			"the file table position.\n",
			m_sAbsolutePackageFilename.CStr()));

		return false;
	}

	ScopedPtr<FullyBufferedSyncFile> pFile;
	{
		// Read the entire block of file table memory.
		UInt32 uDataSizeInBytes = header.GetSizeOfFileTableInBytes();
		void* pData = MemoryManager::AllocateAligned((size_t)uDataSizeInBytes, MemoryBudgets::Io, kLZ4MinimumAlignment);
		if (uDataSizeInBytes != m_pPackageFile->ReadRawData(pData, uDataSizeInBytes))
		{
			MemoryManager::Deallocate(pData);
			m_sLoadError.Assign(String::Printf("Failed reading package file \"%s\", could not read file table data.",
				m_sAbsolutePackageFilename.CStr()));

			return false;
		}

		// If we're on a new enough version of the package that contains a post CRC32 for the file table,
		// apply it now, and then fixup the data size.
		if (header.HasFileTablePostCRC32())
		{
			// If data size is less than the crc32 value size, this is an error.
			UInt32 uExpectedCrc32Post = 0u;
			if (uDataSizeInBytes < sizeof(uExpectedCrc32Post))
			{
				MemoryManager::Deallocate(pData);
				m_sLoadError.Assign(String::Printf("Failed reading package file \"%s\", file table is only %u bytes, it must be at least %u bytes and contain a CRC32 post value for the rest of the body.",
					m_sAbsolutePackageFilename.CStr(),
					uDataSizeInBytes,
					(UInt32)sizeof(uExpectedCrc32Post)));

				return false;
			}

			// Populate crc32 value then immediately adjust the size for the remainder of processing.
			memcpy(&uExpectedCrc32Post, (Byte*)pData + uDataSizeInBytes - sizeof(uExpectedCrc32Post), sizeof(uExpectedCrc32Post));
			uDataSizeInBytes -= sizeof(uExpectedCrc32Post);

			// Now check the CRC32 value.
			auto const uActualCrc32Post = GetCrc32((UInt8 const*)pData, uDataSizeInBytes);

			// Validate - fail if not a match.
			if (uExpectedCrc32Post != uActualCrc32Post)
			{
				MemoryManager::Deallocate(pData);
				m_sLoadError.Assign(String::Printf("Failed reading package file \"%s\", file table CRC32 post mismatch, expected %u got %u.",
					m_sAbsolutePackageFilename.CStr(),
					uExpectedCrc32Post,
					uActualCrc32Post));

				return false;
			}
		}

		// The data is always obfuscated, deobfuscate it prior to further processing.
		{
			UInt32 uXorKey = 0;
			{
				String const sFileTablePsuedoFilename =
					String::Printf("%u", (UInt32)header.GetBuildVersionMajor()) +
					String::Printf("%u", (UInt32)header.GetBuildChangelist());
				uXorKey = GenerateObfuscationKey(sFileTablePsuedoFilename);
			}
			PackageFileSystem::Obfuscate(uXorKey, pData, uDataSizeInBytes, 0);
			uXorKey = 0;
		}

		// If the file table was written with compression, decompress it now.
		if (header.HasCompressedFileTable())
		{
			void* pDecompressedData = nullptr;
			UInt32 uDecompressedFileTableSizeInBytes = 0;

			Bool bResult = false;
			if (header.IsOldLZ4Compression())
			{
				bResult = LZ4Decompress(pData, uDataSizeInBytes, pDecompressedData, uDecompressedFileTableSizeInBytes, MemoryBudgets::Io);
			}
			else
			{
				bResult = ZSTDDecompress(pData, uDataSizeInBytes, pDecompressedData, uDecompressedFileTableSizeInBytes, MemoryBudgets::Io);
			}

			if (!bResult)
			{
				MemoryManager::Deallocate(pData);
				m_sLoadError.Assign(String::Printf("Failed decompressing compressed file table data in package file \"%s\".",
					m_sAbsolutePackageFilename.CStr()));

				return false;
			}

			MemoryManager::Deallocate(pData);
			pData = pDecompressedData;
			uDataSizeInBytes = uDecompressedFileTableSizeInBytes;
		}

		// Wrap the file table in a sync file for further processing.
		pFile.Reset(SEOUL_NEW(MemoryBudgets::Io) FullyBufferedSyncFile(pData, uDataSizeInBytes, true));
	}

	FileTable tFileTable;
	FileList vFileList;
	UInt32 nTotalEntriesRead = 0u;

	// Variables used for parsing filenames.
	Vector<Byte, MemoryBudgets::Io> vWorkArea;
	String sFilenameWithoutExtension;
	String sExtension;

	// Track if we have post CRC32 values for all entries (post CRC32
	// is the CRC32 computed after any obfuscation or compression
	// has been applied at the package level). If true,
	// we can migrate certain header versions (specifically,
	// ku18_PreDualCrc32Version can be migrated to head).
	auto const bHeaderHasPostCRC32 = header.HasPostCRC32();
	auto const bIsObfuscated = header.IsObfuscated();
	Bool bHasPostCrc32 = true;
	auto const eDirectory = SerializedToGameDirectory(header.GetGameDirectory());;
	UInt64 const uPackageFileSize = header.GetTotalPackageFileSizeInBytes();

	// Reserve space ahead of time if we support directory queries.
	if (header.HasSupportDirectoryQueries())
	{
		vFileList.Reserve(header.GetTotalEntriesInFileTable());
	}

	// Running FilePath, directory is always the same - ony
	// need to set relative filename and file type with
	// each iteration.
	FilePath filePath;
	filePath.SetDirectory(eDirectory);
	UInt32 uOrder = 0u;
	while (nTotalEntriesRead < header.GetTotalEntriesInFileTable())
	{
		PackageFileEntry entry;
		UInt32 uBytesRead = pFile->ReadRawData(&entry, sizeof(entry));

		if (uBytesRead != sizeof(entry))
		{
			m_sLoadError.Assign(String::Printf("Failed reading package file \"%s\", could not read "
				"an entry in the file table.\n",
				m_sAbsolutePackageFilename.CStr()));

			return false;
		}

		// Swap the entry if necessary.
		if (bEndianSwap)
		{
			EndianSwap(entry);
		}

		// Backwards compatibility - if ku18_PreDualCrc32Version version,
		// migrate pre crc32 to post when possible.
		if (!bHeaderHasPostCRC32)
		{
			// If no special options, these will be equal.
			if (!bIsObfuscated && entry.m_uCompressedFileSize == entry.m_uUncompressedFileSize)
			{
				// Can just grant post from pre.
				entry.m_uCrc32Post = entry.m_uCrc32Pre;
			}
			// Otherwise, set it to 0, force a slow operation later.
			else
			{
				// Lack some post crc32 values so
				// consider the entire archive devoid
				// of these values.
				bHasPostCrc32 = false;

				// Reasonable default.
				entry.m_uCrc32Post = 0u;
			}
		}

		// Sanity check the offset/size and watch out for overflow
		if (entry.m_uOffsetToFile > uPackageFileSize ||
			entry.m_uOffsetToFile + entry.m_uCompressedFileSize > uPackageFileSize ||
			entry.m_uOffsetToFile + entry.m_uCompressedFileSize < entry.m_uOffsetToFile)
		{
			m_sLoadError.Assign(String::Printf("Invalid file offset/size for package file: %s", m_sAbsolutePackageFilename.CStr()));
			return false;
		}

		// Fill out the table entry.
		PackageFileTableEntry fileTableEntry;
		fileTableEntry.m_Entry = entry;
		fileTableEntry.m_uOrder = uOrder++;

		// Read the filename, extension, and xor key if
		// this package was obfuscated.
		if (!ReadFilenameAndExtension(*pFile, vWorkArea, bEndianSwap, bIsObfuscated, sFilenameWithoutExtension, sExtension, fileTableEntry.m_uXorKey))
		{
			m_sLoadError.Assign(String::Printf("Failed reading package file \"%s\", could not read "
				"an entry in the file table.\n",
				m_sAbsolutePackageFilename.CStr()));

			return false;
		}

		// Assemble the FilePath - relative filename has
		// been setup (it is a case insensitive HString)
		// and type can be derived from the extension.
		auto const eType = ExtensionToFileType(sExtension);
		filePath.SetRelativeFilenameWithoutExtension(FilePathRelativeFilename(sFilenameWithoutExtension));
		filePath.SetType(eType);
		if (!tFileTable.Insert(filePath, fileTableEntry).Second)
		{
			m_sLoadError.Assign(String::Printf("Failed reading package file \"%s\", could not insert "
				"filename \"%s.%s\" into the file table. This likely indicates "
				"a duplicate file entry.\n",
				m_sAbsolutePackageFilename.CStr(),
				sFilenameWithoutExtension.CStr(),
				sExtension.CStr()));

			return false;
		}

		// If supporting directory queries, add the entry to the mast list of
		// entries.
		if (header.HasSupportDirectoryQueries())
		{
			// Use the raw work area, which will contain the filename + extension.
			vFileList.PushBack(filePath);
		}

		nTotalEntriesRead++;
	}

	if (!m_pPackageFile->Seek(0, File::kSeekFromStart))
	{
		m_sLoadError.Assign(String::Printf("Failed reading package file \"%s\", could not restore "
			"the current file pointer.\n",
			m_sAbsolutePackageFilename.CStr()));

		return false;
	}

	m_iCurrentFileOffset = 0;
	m_tFileTable.Swap(tFileTable);

	// Sort and swap the master list of entries for directory queries - this
	// will be empty if directory query support is not enabled.
	FileListSort fileListSort;
	QuickSort(vFileList.Begin(), vFileList.End(), fileListSort);
	m_vSortedFileList.Swap(vFileList);

	// Output.
	rbHasPostCrc32 = bHasPostCrc32;
	return true;
}

} // namespace Seoul
