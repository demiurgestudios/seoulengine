/**
 * \file ZipFile.cpp
 * \brief Utility class for reading and writing .zip files
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ZipFile.h"
#include "Logger.h"

#include <miniz.h>

namespace Seoul
{

/** Common path for normalizing a file for lookup. */
static inline String NormalizeFile(const String& s)
{
	return s.ReplaceAll("\\", "/");
}

/** Convert the ZlibCompression enum into a valid compression level. */
static inline Bool ConvertCompressionLevel(ZlibCompressionLevel eLevel, UInt& ru)
{
	UInt u = 0u;
	if ((Int)eLevel < 0)
	{
		u = MZ_DEFAULT_LEVEL;
	}
	else
	{
		u = (UInt)eLevel;
	}

	// Checking - must be [0, 10].
	if (u > 10)
	{
		SEOUL_LOG_CORE("ZipFile: Invalid compression level (%u)", u);
		return false;
	}

	ru = u;
	return true;
}

// Memory management functions for miniz
static void* ZipAlloc(void*, size_t zItems, size_t zSize)
{
	return MemoryManager::Allocate(zItems * zSize, MemoryBudgets::Compression);
}

static void ZipFree(void*, void* pAddress)
{
	MemoryManager::Deallocate(pAddress);
}

static void* ZipRealloc(void*, void* pAddress, size_t zItems, size_t zSize)
{
	return MemoryManager::Reallocate(pAddress, zItems * zSize, MemoryBudgets::Compression);
}

static size_t ZipRead(void* pOpaque, mz_uint64 zOffset, void* pData, size_t zData)
{
	auto& r = *((SyncFile*)pOpaque);

	Int64 iOffset = 0;
	if (!r.GetCurrentPositionIndicator(iOffset)) { return 0; }

	if (iOffset != (Int64)zOffset)
	{
		if (!r.Seek((Int64)zOffset, File::kSeekFromStart)) { return 0; }
	}

	return (size_t)r.ReadRawData(pData, (UInt32)zData);
}

static size_t ZipWrite(void* pOpaque, mz_uint64 zOffset, void const* pData, size_t zData)
{
	auto& r = *((SyncFile*)pOpaque);

	Int64 iOffset = 0;
	if (!r.GetCurrentPositionIndicator(iOffset)) { return 0; }

	if (iOffset != (Int64)zOffset)
	{
		if (!r.Seek((Int64)zOffset, File::kSeekFromStart)) { return 0; }
	}

	return (size_t)r.WriteRawData(pData, (UInt32)zData);
}

ZipFileReader::ZipFileReader(UInt32 uFlags /* = 0u */)
	: m_pZip(SEOUL_NEW(MemoryBudgets::Compression) mz_zip_archive)
	, m_uFlags(uFlags)
	, m_uEntries(0u)
{
	memset(m_pZip.Get(), 0, sizeof(*m_pZip));
	m_pZip->m_pAlloc = ZipAlloc;
	m_pZip->m_pFree = ZipFree;
	m_pZip->m_pRealloc = ZipRealloc;
	m_pZip->m_pRead = ZipRead;
}

ZipFileReader::~ZipFileReader()
{
	// Use the presence of the file opaque to indicate if we need
	// to terminate.
	if (nullptr != m_pZip->m_pIO_opaque)
	{
		(void)mz_zip_reader_end(m_pZip.Get());
	}
	m_pZip.Reset();
}

/** Convert ZipFile flags to mz_zip flags. */
static inline UInt32 ConvertFlags(UInt32 uFlags)
{
	UInt32 u = 0u;
	u |= (ZipFileReader::kAcceptRecoverableCorruption & uFlags) != 0 ? MZ_ZIP_FLAG_ACCEPT_RECOVERABLE_CORRUPTION : 0;
	return u;
}

/** The .zip archive bytes will be written to rInputFile. */
Bool ZipFileReader::Init(SyncFile& rInputFile)
{
	if (nullptr != m_pZip->m_pIO_opaque)
	{
		SEOUL_LOG_CORE("ZipFileWriter: Init called twice");
		return false;
	}

	m_pZip->m_pIO_opaque = &rInputFile;
	if (!mz_zip_reader_init(m_pZip.Get(), rInputFile.GetSize(), ConvertFlags(m_uFlags)))
	{
		SEOUL_LOG_CORE("ZipFileWriter: failed to initialize writer.");
		m_pZip->m_pIO_opaque = nullptr; // clear so we don't try to double release.
		return false;
	}

	m_uEntries = (UInt32)mz_zip_reader_get_num_files(m_pZip.Get());
	return true;
}

/** @return the total number of file entries in this .zip file. */
UInt32 ZipFileReader::GetEntryCount() const
{
	return m_uEntries;
}

/** Get the entry name at index uIndex of this .zip file. */
Bool ZipFileReader::GetEntryName(UInt32 uIndex, String& rs) const
{
	if (uIndex >= m_uEntries) { return false; }

	UInt32 uSize = (UInt32)mz_zip_reader_get_filename(m_pZip.Get(), uIndex, nullptr, 0u);
	if (0 == uSize) { return false; }

	void* p = MemoryManager::Allocate(uSize, MemoryBudgets::Strings);
	(void)mz_zip_reader_get_filename(m_pZip.Get(), uIndex, (char*)p, (mz_uint)uSize);
	
	// String expects the size to be excluding the 0 terminator.
	--uSize;
	rs.TakeOwnership(p, uSize);
	return true;
}

/**
 * Query file size of a file in this .zip file.
 *
 * @return false if the file is not found or on other error, true otherwise.
 */
Bool ZipFileReader::GetFileSize(String const& sName, UInt64& ruSize) const
{
	mz_zip_archive_file_stat info;
	if (!Stat(sName, info))
	{
		return false;
	}
	if (0 != info.m_is_directory) // Don't retrieve file size for a directory.
	{
		return false;
	}

	ruSize = (UInt64)info.m_uncomp_size;
	return true;
}

/**
 * Low-level IO function - if the sub file in the .zip file
 * is not compressed, provides the absolute offset to that data
 * within the SyncFile used to initialize this archive. Allows
 * direct reads of the file for uncompressed data.
 *
 * WARNING: It is (as with all other API) the responsibility of the
 * caller to ensure thread exclusion of operations against the sync
 * file.
 */
Bool ZipFileReader::GetInternalFileOffset(String const& sName, Int64& riAbsoluteFileOffset) const
{
	mz_zip_archive_file_stat info;
	if (!Stat(sName, info))
	{
		return false;
	}

	// Unsupported.
	if (0 != info.m_method || info.m_comp_size != info.m_uncomp_size)
	{
		return false;
	}

	// Query.
	UInt64 uAbsoluteFileOffset = 0u;
	if (0 == mz_zip_read_get_file_offset(m_pZip.Get(), info.m_file_index, &uAbsoluteFileOffset))
	{
		return false;
	}

	// Done, success.
	riAbsoluteFileOffset = (Int64)uAbsoluteFileOffset;
	return true;
}

/**
 * Query mod time of a file in this .zip file.
 *
 * @return false if the file is not found or on other error, true otherwise.
 */
Bool ZipFileReader::GetModifiedTime(String const& sName, UInt64& ruModTime) const
{
	mz_zip_archive_file_stat info;
	if (!Stat(sName, info))
	{
		return false;
	}

	ruModTime = (UInt64)info.m_time;
	return true;
}

/**
 * Query whether the specified name associates to a directory or not.
 *
 * @return true on success, false otherwise.
 */
Bool ZipFileReader::IsDirectory(String const& sName) const
{
	// Query.
	mz_zip_archive_file_stat info;
	if (!Stat(sName, info))
	{
		return false;
	}

	return (0 != info.m_is_directory);
}

/**
 * Read the entire body of the given file - will be uncompressed if needed.
 * Allocates the necessary buffer (which must be freed with MemoryManager::Deallocate).
 *
 * @return true on success, false otherwise. On true, rpOutputBuffer will contain
 * a pointer that must be freed with a call to MemoryManager::Deallocate().
 */
Bool ZipFileReader::ReadAll(
	String const& sName,
	void*& rpOutputBuffer,
	UInt32& rzOutputSizeInBytes,
	UInt32 zAlignmentOfOutputBuffer,
	MemoryBudgets::Type eOutputBufferMemoryType,
	UInt32 zMaxReadSize /*= kDefaultMaxReadSize*/) const
{
	// Query.
	mz_zip_archive_file_stat info;
	if (!Stat(sName, info))
	{
		return false;
	}
	if (0 != info.m_is_directory) // Cannot read the body of a directory.
	{
		return false;
	}

	// Check size
	if (info.m_uncomp_size > zMaxReadSize)
	{
		return false;
	}

	// Decompress into a memory buffer.
	void* p = MemoryManager::AllocateAligned((size_t)info.m_uncomp_size, eOutputBufferMemoryType, (size_t)zAlignmentOfOutputBuffer);
	if (0 == mz_zip_reader_extract_to_mem(m_pZip.Get(), info.m_file_index, p, (size_t)info.m_uncomp_size, 0))
	{
		MemoryManager::Deallocate(p);
		return false;
	}

	rpOutputBuffer = p;
	rzOutputSizeInBytes = (UInt32)info.m_uncomp_size;
	return true;
}

/** Common state query method. */
Bool ZipFileReader::Stat(String const& sName, mz_zip_archive_file_stat& rInfo) const
{
	// Setup.
	auto const sNormalizedName(NormalizeFile(sName));

	// Query.
	UInt32 uIndex = 0;
	if (0 == mz_zip_reader_locate_file_v2(m_pZip.Get(), sNormalizedName.CStr(), nullptr, 0, &uIndex))
	{
		return false;
	}

	if (0 == mz_zip_reader_file_stat(m_pZip.Get(), uIndex, &rInfo))
	{
		return false;
	}

	return true;
}

ZipFileWriter::ZipFileWriter()
	: m_pZip(SEOUL_NEW(MemoryBudgets::Compression) mz_zip_archive)
{
	memset(m_pZip.Get(), 0, sizeof(*m_pZip));
	m_pZip->m_pAlloc = ZipAlloc;
	m_pZip->m_pFree = ZipFree;
	m_pZip->m_pRealloc = ZipRealloc;
	m_pZip->m_pWrite = ZipWrite;
}

ZipFileWriter::~ZipFileWriter()
{
	// Use the presence of the file opaque to indicate if we need
	// to terminate.
	if (nullptr != m_pZip->m_pIO_opaque)
	{
		(void)mz_zip_writer_end(m_pZip.Get());
	}
	m_pZip.Reset();
}

/**
 * Initialize the ZipFileWriter and validate settings. Only call this once per ZipFileWriter.
 * 
 * @param outputFille The SyncFile to write the .zip archive to. Zip files can
 * include extra bytes at the beginning, but not at the end.
 * @returns False if there were errors. Always logs some extra detail if false.
 */
Bool ZipFileWriter::Init(SyncFile& rOutputFile)
{
	if (nullptr != m_pZip->m_pIO_opaque)
	{
		SEOUL_LOG_CORE("ZipFileWriter: Init called twice");
		return false;
	}

	m_pZip->m_pIO_opaque = &rOutputFile;
	if (!mz_zip_writer_init(m_pZip.Get(), 0))
	{
		SEOUL_LOG_CORE("ZipFileWriter: failed to initialize writer.");
		m_pZip->m_pIO_opaque = nullptr; // clear so we don't try to double release.
		return false;
	}

	return true;
}

/**
 * Finish writing the .zip file. Only call this once per ZipFileWriter; after
 * finalizing, no more files can be added.
 * 
 * @returns False if there were errors. Always logs some extra detail if false.
 */
Bool ZipFileWriter::Finalize()
{
	if (!mz_zip_writer_finalize_archive(m_pZip.Get()))
	{
		SEOUL_LOG_CORE("Failed to finalize archive");
		return false;
	}

	return true;
}

/**
* Write a string to a single named file in the .zip
*
* @returns False if there were errors. Always logs some extra detail if false.
*/
Bool ZipFileWriter::AddFileString(String const& sName, String const& sContents, ZlibCompressionLevel eCompressionLevel)
{
	return AddFileBytes(sName, sContents.CStr(), sContents.GetSize(), eCompressionLevel);
}

/**
 * Add a file to the archive, from a String. Explicitly specify the modified time of the data.
 *
 * @return False if there were errors. Always log some extra defailt if false.
 */
Bool ZipFileWriter::AddFileString(String const& sName, String const& sContents, ZlibCompressionLevel eCompressionLevel, UInt64 uModifiedTime)
{
	return AddFileBytes(sName, sContents.CStr(), sContents.GetSize(), eCompressionLevel, uModifiedTime);
}

/**
* Write a series of bytes to a single named file in the .zip
 * 
 * @return False if there were errors. Always logs some extra detail if false.
 */
Bool ZipFileWriter::AddFileBytes(String const& sName, void const* pBytes, size_t zBufferSize, ZlibCompressionLevel eCompressionLevel)
{
	UInt uCompressionLevel = 0;
	if (!ConvertCompressionLevel(eCompressionLevel, uCompressionLevel))
	{
		return false;
	}

	auto const sNormalizedName(NormalizeFile(sName));
	if (!mz_zip_writer_add_mem(m_pZip.Get(), sNormalizedName.CStr(), pBytes, zBufferSize, uCompressionLevel))
	{
		SEOUL_LOG_CORE("Failed to add file %s", sName.CStr());
		return false;
	}

	return true;
}

/**
 * Add a file to the archive, from a raw bytes pointer and size. Explicitly specify the modified time of the data.
 *
 * @return False if there were errors. Always logs some extra detail if false.
 */
Bool ZipFileWriter::AddFileBytes(
	String const& sName, 
	void const* pBytes, 
	size_t zBufferSize, 
	ZlibCompressionLevel eCompressionLevel, 
	UInt64 uModifiedTime)
{
	UInt uCompressionLevel = 0;
	if (!ConvertCompressionLevel(eCompressionLevel, uCompressionLevel))
	{
		return false;
	}

	// Setup the last modification time.
	MZ_TIME_T lastModified;
	memset(&lastModified, 0, sizeof(lastModified));
	lastModified = (MZ_TIME_T)uModifiedTime;

	auto const sNormalizedName(NormalizeFile(sName));
	if (!mz_zip_writer_add_mem_ex_v2(
		m_pZip.Get(),
		sNormalizedName.CStr(),
		pBytes,
		zBufferSize,
		nullptr,
		0u,
		uCompressionLevel,
		0u,
		0u,
		&lastModified,
		nullptr,
		0u,
		nullptr,
		0u))
	{
		SEOUL_LOG_CORE("Failed to add file %s", sName.CStr());
		return false;
	}

	return true;
}

} // namespace Seoul
