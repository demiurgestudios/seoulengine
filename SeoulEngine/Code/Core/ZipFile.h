/**
 * \file ZipFile.h
 * \brief Utility class for reading and writing .zip files
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ZIP_FILE_H
#define ZIP_FILE_H

#include "Compress.h"
#include "ScopedPtr.h"
#include "SeoulFile.h"
#include "SeoulTypes.h"

extern "C" { struct mz_zip_archive; }
extern "C" { struct mz_zip_archive_file_stat; }

namespace Seoul
{

/**
 * ZipFileReader: Wraps a SyncFile to read the contents of a .zip archive.
 *
 * Usage:
 * ZipFileReader zip;
 * if (!zip.Init(rInputFile)) { ... } // Do this exactly once
 *
 * If Init() returns false, the zip file should be assumed invalid.
 *
 * NOTE: API is not thread-safe. To call from multiple threads, you must
 * wrap all functionality in an explicit Mutex lock.
 */
class ZipFileReader SEOUL_SEALED
{
public:
	enum
	{
		kFlagNone = 0,

		/**
		 * Used in SeoulEngine for compatibility with opening .fla files - allows certain
		 * recoverable forms of corruption. .fla files are .zip files with an invalid/incomplete
		 * central directory.
		 */
		kAcceptRecoverableCorruption = (1 << 0),
	};

	ZipFileReader(UInt32 uFlags = 0u);
	~ZipFileReader();

	// The .zip archive bytes will be written to rInputFile.
	Bool Init(SyncFile& rInputFile);

	// Return the total number of file entries in this .zip file.
	UInt32 GetEntryCount() const;

	// Get the entry name at index uIndex of this .zip file.
	Bool GetEntryName(UInt32 uIndex, String& rs) const;

	// Query file size of a file in this .zip file.
	Bool GetFileSize(String const& sName, UInt64& ruSize) const;

	// Low-level IO function - if the sub file in the .zip file
	// is not compressed, provides the absolute offset to that data
	// within the SyncFile used to initialize this archive. Allows
	// direct reads of the file for uncompressed data.
	//
	// WARNING: It is (as with all other API) the responsibility of the
	// caller to ensure thread exclusion of operations against the sync
	// file.
	Bool GetInternalFileOffset(String const& sName, Int64& riAbsoluteFileOffset) const;

	// Query modified time of a file in this .zip file.
	Bool GetModifiedTime(String const& sName, UInt64& ruModTime) const;

	// Query whether the specified name associates to a directory or not.
	Bool IsDirectory(String const& sName) const;

	// Read the entire body of the given file - will be uncompressed if needed.
	// Allocates the necessary buffer (which must be freed with MemoryManager::Deallocate).
	Bool ReadAll(
		String const& sName,
		void*& rpOutputBuffer,
		UInt32& rzOutputSizeInBytes,
		UInt32 zAlignmentOfOutputBuffer,
		MemoryBudgets::Type eOutputBufferMemoryType,
		UInt32 zMaxReadSize = kDefaultMaxReadSize) const;

private:
	SEOUL_DISABLE_COPY(ZipFileReader);

	Bool Stat(String const& sName, mz_zip_archive_file_stat& rInfo) const;

	ScopedPtr<mz_zip_archive> m_pZip;
	UInt32 const m_uFlags;
	UInt32 m_uEntries;
}; // class ZipFileReader

/**
 * ZipFileWriter: Wraps a SyncFile to write a .zip archive.
 *
 * Usage:
 * ZipFileWriter zip;
 * if (!zip.Init(pOutputFile)) { ... } // Do this exactly once
 * if (!zip.AddFileBytes(...)) { ... } // Do this (or another AddFile API) 0+ times
 * if (!zip.Finalize()) { ... } // Do this exactly once, when you're done adding to the archive
 *
 * If any of these methods returns false, the zip file should be assumed invalid.
 *
 * NOTE: API is not thread-safe. To call from multiple threads, you must
 * wrap all functionality in an explicit Mutex lock.
 */
class ZipFileWriter SEOUL_SEALED
{
public:
	ZipFileWriter();
	~ZipFileWriter();

	// The .zip archive bytes will be written to pOutputFile.
	Bool Init(SyncFile& rOutputFile);

	// Finish writing the .zip archive.
	Bool Finalize();

	// Add a file to the archive, from a String
	Bool AddFileString(String const& sName, String const& sContents, ZlibCompressionLevel eCompressionLevel);

	// Add a file to the archive, from a String. Explicitly specify the modified time of the data.
	Bool AddFileString(String const& sName, String const& sContents, ZlibCompressionLevel eCompressionLevel, UInt64 uModifiedTime);

	// Add a file to the archive, from a raw bytes pointer and size
	Bool AddFileBytes(String const& sName, void const* pBytes, size_t zBufferSize, ZlibCompressionLevel eCompressionLevel);

	// Add a file to the archive, from a raw bytes pointer and size. Explicitly specify the modified time of the data.
	Bool AddFileBytes(String const& sName, void const* pBytes, size_t zBufferSize, ZlibCompressionLevel eCompressionLevel, UInt64 uModifiedTime);

private:
	SEOUL_DISABLE_COPY(ZipFileWriter);

	ScopedPtr<mz_zip_archive> m_pZip;
}; // class ZipFileWriter

} // namespace Seoul

#endif  // include guard
