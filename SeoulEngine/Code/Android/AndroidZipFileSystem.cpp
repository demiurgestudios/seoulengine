/**
 * \file AndroidZipFileSystem.cpp
 * \brief Specialization of IFileSystem that services file requests
 * from the current application's APK file. Unlike AndroidFileSystem,
 * this implementation uses .zip file reading based on mz_zip, to workaround
 * an issue in AAssetManager when loading very large files, due to the entire
 * file being accessed using mmap.
 *
 * See also: https://github.com/google/ExoPlayer/issues/5153
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "AndroidZipFileSystem.h"
#include "DiskFileSystem.h"
#include "GamePaths.h"
#include "ZipFile.h"

namespace Seoul
{

/* Max size that will be fully decompressed into memory. */
static const UInt64 kuMaxCompressedSize = (2 * 1024 * 1024); // 2 MBs.

/**
 * @return The subset of sAbsoluteFilename that should be used to query
 * the asset manager for a file or directory.
 */
inline static String ToRelativeName(const String& sAbsoluteFilename)
{
	// Remove the base directory part of the absolute path.
	if (GamePaths::Get() && !GamePaths::Get()->GetBaseDir().IsEmpty())
	{
		auto const& sBaseDir = GamePaths::Get()->GetBaseDir();
		if (sAbsoluteFilename.StartsWithASCIICaseInsensitive(sBaseDir))
		{
			return Path::Combine("assets", String(sAbsoluteFilename.CStr() + sBaseDir.GetSize()));
		}
	}

	return Path::Combine("assets", sAbsoluteFilename);
}

AndroidZipSyncFile::AndroidZipSyncFile(
	AndroidZipFileSystem& rOwner,
	const String& sAbsoluteFilename,
	UInt64 uFileSize,
	Int64 iAbsoluteFileOffset)
	: m_rOwner(rOwner)
	, m_sAbsoluteFilename(sAbsoluteFilename)
	, m_uFileSize(uFileSize)
	, m_iAbsoluteFileOffset(iAbsoluteFileOffset)
	, m_iCurrentRelativeOffset(0)
{
}

AndroidZipSyncFile::~AndroidZipSyncFile()
{
}

/**
 * Attempt to read zSizeInBytes raw bytes from this file into pOut.
 *
 * @return The actual number of bytes read.
 */
UInt32 AndroidZipSyncFile::ReadRawData(void* pOut, UInt32 uSizeInBytes)
{
	auto const uToRead = Min(uSizeInBytes, (UInt32)(m_uFileSize - (UInt64)m_iCurrentRelativeOffset));
	auto const iAbsoluteOffset = (m_iAbsoluteFileOffset + m_iCurrentRelativeOffset);

	UInt32 uRead = 0u;
	{
		Lock lock(m_rOwner.m_Mutex);
		if (!m_rOwner.m_pApkFile->Seek(iAbsoluteOffset, File::kSeekFromStart))
		{
			return 0u;
		}

		uRead = m_rOwner.m_pApkFile->ReadRawData(pOut, uToRead);
	}

	// Advance.
	m_iCurrentRelativeOffset += (Int64)uRead;
	// Return.
	return uRead;
}

/**
 * @return True if the file was successfully opened, false otherwise.
 */
Bool AndroidZipSyncFile::IsOpen() const
{
	Lock lock(m_rOwner.m_Mutex);
	return m_rOwner.m_pApkFile->IsOpen();
}

// Attempt to get the current absolute file pointer position.
Bool AndroidZipSyncFile::GetCurrentPositionIndicator(Int64& riPosition) const
{
	riPosition = m_iCurrentRelativeOffset;
	return true;
}

/**
 * Attempt a seek operation on this AndroidZipSyncFile.
 *
 * @return True if the seek succeeds, false otherwise. If this method
 * returns true, then the file pointer will be at the position defined
 * by iPosition and the mode eMode. Otherwise, the file position
 * is undefined.
 */
Bool AndroidZipSyncFile::Seek(Int64 iPosition, File::SeekMode eMode)
{
	Int64 iNewOffset = m_iCurrentRelativeOffset;
	switch (eMode)
	{
	case File::kSeekFromCurrent:
		iNewOffset += iPosition;
		break;
	case File::kSeekFromEnd:
		iNewOffset = (m_uFileSize - iPosition);
		break;
	case File::kSeekFromStart:
		iNewOffset = iPosition;
		break;
	default:
		SEOUL_FAIL("Enum out of sync, bug a programmer.\n");
		return false;
	};

	if (iNewOffset < 0 || iNewOffset > (Int64)m_uFileSize)
	{
		return false;
	}
	else
	{
		m_iCurrentRelativeOffset = iNewOffset;
		return true;
	}
}

AndroidZipFileSystem::AndroidZipFileSystem(const String& sApkPath)
	: m_sApkPath(sApkPath)
	, m_Mutex()
	, m_pApkFile(SEOUL_NEW(MemoryBudgets::Io) DiskSyncFile(sApkPath, File::kRead))
	, m_pZipFile(SEOUL_NEW(MemoryBudgets::Io) ZipFileReader)
	, m_bOk(m_pZipFile->Init(*m_pApkFile))
{
}

AndroidZipFileSystem::~AndroidZipFileSystem()
{
	m_pZipFile.Reset();
	m_pApkFile.Reset();
}

/**
 * Get the size in bytes of sAbsoluteFilename and assign it to ruFileSize.
 *
 * @return True on success, false otherwise. If this method returns false, ruFileSize
 * is left unmodified.
 */
Bool AndroidZipFileSystem::GetFileSize(
	const String& sAbsoluteFilename,
	UInt64& ruFileSize) const
{
	if (!m_bOk) { return false; } // Quick check.

	Lock lock(m_Mutex);
	return m_pZipFile->GetFileSize(ToRelativeName(sAbsoluteFilename), ruFileSize);
}

/**
 * @return True if sAbsoluteFilename exists on disk, false otherwise.
 */
Bool AndroidZipFileSystem::Exists(const String& sAbsoluteFilename) const
{
	if (!m_bOk) { return false; } // Quick check.

	Lock lock(m_Mutex);

	UInt64 uUnusedModTime = 0u;
	return m_pZipFile->GetModifiedTime(ToRelativeName(sAbsoluteFilename), uUnusedModTime);
}

/**
 * @return True if sAbsoluteFilename exists on disk and is a directory,
 *         false otherwise.
 */
Bool AndroidZipFileSystem::IsDirectory(const String& sAbsoluteFilename) const
{
	if (!m_bOk) { return false; } // Quick check.

	Lock lock(m_Mutex);
	return m_pZipFile->IsDirectory(ToRelativeName(sAbsoluteFilename));
}

/**
 * @return True if sAbsoluteFilename can be opened with the given mode eMode.
 *
 * If this method returns true, rpFile will contain a non-null SyncFile
 * pointer and that object will return true for calls to SyncFile::IsOpen().
 * If this method returns false, rpFile is left unmodified.
 */
Bool AndroidZipFileSystem::Open(
	const String& sAbsoluteFilename,
	File::Mode eMode,
	ScopedPtr<SyncFile>& rpFile)
{
	if (!m_bOk) { return false; } // Quick check.
	// Can only open files for kRead.
	if (File::kRead != eMode)
	{
		return false;
	}

	// Used for all lookups.
	auto const sRelative(ToRelativeName(sAbsoluteFilename));

	Lock lock(m_Mutex);

	// Try getting the offset to the file - this only succeeds
	// if the file is not compressed.
	Int64 iAbsoluteFileOffset = 0;
	UInt64 uFileSize = 0;
	if (m_pZipFile->GetInternalFileOffset(sRelative, iAbsoluteFileOffset) &&
		m_pZipFile->GetFileSize(sRelative, uFileSize))
	{
		// Uncompressed case.
		rpFile.Reset(SEOUL_NEW(MemoryBudgets::Io) AndroidZipSyncFile(*this, sAbsoluteFilename, uFileSize, iAbsoluteFileOffset));
		return true;
	}
	else
	{
		// Compressed case - must decompress into a memory buffer.
		void* p = nullptr;
		UInt32 u = 0u;
		if (!m_pZipFile->ReadAll(sRelative, p, u, 0u, MemoryBudgets::Io, kuMaxCompressedSize))
		{
			return false;
		}

		rpFile.Reset(SEOUL_NEW(MemoryBudgets::Io) FullyBufferedSyncFile(p, u, true, sAbsoluteFilename));
		return true;
	}
}

/**
 * @return True if a directory list for sAbsoluteDirectoryPath could
 * be generated, false otherwise. If this method returns true,
 * rvResults will contain a list of files and directories that fulfill
 * the other arguments to this function. Otherwise, rvResults will
 * be left unmodified.
 */
Bool AndroidZipFileSystem::GetDirectoryListing(
	const String& sAbsoluteDirectoryPath,
	Vector<String>& rvResults,
	Bool bIncludeDirectoriesInResults,
	Bool bRecursive,
	const String& sFileExtension) const
{
	// TODO: Implement.
	return false;
}

} // namespace Seoul
