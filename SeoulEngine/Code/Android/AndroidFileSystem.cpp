/**
 * \file AndroidFileSystem.cpp
 * \brief Specialization of IFileSystem that services file requests
 * from the current application's APK file.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "AndroidFileSystem.h"
#include "DiskFileSystem.h"
#include "DiskFileSystemInternal.h"
#include "GamePaths.h"

#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

namespace Seoul
{

/**
 * @return The subset of sAbsoluteFilename that should be used to query
 * the asset manager for a file or directory.
 */
inline static Byte const* ToAssetManagerFilename(const String& sAbsoluteFilename)
{
	// Remove the base directory part of the absolute path.
	const String& sBaseDir = (GamePaths::Get().IsValid() ? GamePaths::Get()->GetBaseDir() : String());
	if (sAbsoluteFilename.GetSize() >= sBaseDir.GetSize())
	{
		return (sAbsoluteFilename.CStr() + sBaseDir.GetSize());
	}

	return sAbsoluteFilename.CStr();
}

AndroidSyncFile::AndroidSyncFile(
	CheckedPtr<AAssetManager> pAssetManager,
	const String& sAbsoluteFilename,
	File::Mode eMode)
	: m_pAssetManager(pAssetManager)
	, m_pAsset()
	, m_sAbsoluteFilename(sAbsoluteFilename)
	, m_zFileSize(0u)
{
	InternalOpen();
}

AndroidSyncFile::~AndroidSyncFile()
{
	InternalClose();
}

/**
 * Attempt to read zSizeInBytes raw bytes from this file into pOut.
 *
 * @return The actual number of bytes read.
 */
UInt32 AndroidSyncFile::ReadRawData(void* pOut, UInt32 zSizeInBytes)
{
	if (CanRead())
	{
		Int iResult = AAsset_read(m_pAsset.Get(), pOut, zSizeInBytes);

		return (iResult < 0 ? 0u : (UInt32)iResult);
	}

	return 0u;
}

/**
 * @return True if the file was successfully opened, false otherwise.
 */
Bool AndroidSyncFile::IsOpen() const
{
	return m_pAsset.IsValid();
}

// Attempt to get the current absolute file pointer position.
Bool AndroidSyncFile::GetCurrentPositionIndicator(Int64& riPosition) const
{
	// No tell64 for asset manager, so instead, we seek from current
	// with a seek size of 0, which will return the current position.
	auto const iResult = AAsset_seek64(m_pAsset.Get(), 0, SEEK_CUR);
	if (iResult < 0)
	{
		return false;
	}

	riPosition = iResult;
	return true;
}

/**
 * Attempt a seek operation on this AndroidSyncFile.
 *
 * @return True if the seek succeeds, false otherwise. If this method
 * returns true, then the file pointer will be at the position defined
 * by iPosition and the mode eMode. Otherwise, the file position
 * is undefined.
 */
Bool AndroidSyncFile::Seek(Int64 iPosition, File::SeekMode eMode)
{
	return (AAsset_seek64(m_pAsset.Get(), iPosition, DiskFileSystemDetail::ToSeekMode(eMode)) >= 0);
}

/**
 * Close and cleanup an existing m_pAsset entry.
 */
void AndroidSyncFile::InternalClose()
{
	if (m_pAsset.IsValid())
	{
		m_zFileSize = 0u;
		AAsset* pAsset = m_pAsset.Get();
		m_pAsset.Reset();

		AAsset_close(pAsset);
	}
}

/**
 * Attempt to open the file sAbsoluteFilename.
 * If this operation fails, m_hFileHandle will be an invalid handle.
 */
void AndroidSyncFile::InternalOpen()
{
	// Close the file if it is already opened.
	InternalClose();

	// Get the AAsset* from AAssetManager.
	m_pAsset.Reset(AAssetManager_open(
		m_pAssetManager,
		ToAssetManagerFilename(m_sAbsoluteFilename),
		AASSET_MODE_UNKNOWN));

	// If we succeeded in opening the file, cache its total size.
	if (IsOpen())
	{
		// Get the remaining size from the head of the file.
		off_t const zRemainingSize = AAsset_getRemainingLength(m_pAsset.Get());
		if (zRemainingSize < 0)
		{
			InternalClose();
		}
		else
		{
			m_zFileSize = (UInt64)zRemainingSize;
		}
	}
}

AndroidFileSystem::AndroidFileSystem(CheckedPtr<AAssetManager> pAssetManager)
	: m_pAssetManager(pAssetManager)
{
}

AndroidFileSystem::~AndroidFileSystem()
{
}

/**
 * Get the size in bytes of sAbsoluteFilename and assign it to rzFileSize.
 *
 * @return True on success, false otherwise. If this method returns false, rzFileSize
 * is left unmodified.
 */
Bool AndroidFileSystem::GetFileSize(
	const String& sAbsoluteFilename,
	UInt64& rzFileSize) const
{
	AAsset* pAsset = AAssetManager_open(
		m_pAssetManager.Get(),
		ToAssetManagerFilename(sAbsoluteFilename),
		AASSET_MODE_UNKNOWN);

	// If the file could not be opened, return false.
	if (nullptr == pAsset)
	{
		return false;
	}

	// Get the size of the asset.
	off_t const zRemainingSize = AAsset_getRemainingLength(pAsset);
	AAsset_close(pAsset);
	pAsset = nullptr;

	// If we fialed getting the size, return false.
	if (zRemainingSize < 0)
	{
		return false;
	}

	// Populate the size and return true.
	rzFileSize = (UInt64)zRemainingSize;
	return true;
}

/**
 * @return True if sAbsoluteFilename exists on disk, false otherwise.
 */
Bool AndroidFileSystem::Exists(const String& sAbsoluteFilename) const
{
	// Open the file with AAssetManager.
	AAsset* pAsset = AAssetManager_open(
		m_pAssetManager.Get(),
		ToAssetManagerFilename(sAbsoluteFilename),
		AASSET_MODE_UNKNOWN);

	// If we opened the asset, close it and return success.
	if (nullptr != pAsset)
	{
		AAsset_close(pAsset);
		return true;
	}

	return false;
}

/**
 * @return True if sAbsoluteFilename exists on disk and is a directory,
 *         false otherwise.
 */
Bool AndroidFileSystem::IsDirectory(const String& sAbsoluteFilename) const
{
	AAssetDir* pAssetDir = AAssetManager_openDir(
		m_pAssetManager.Get(),
		ToAssetManagerFilename(sAbsoluteFilename));

	// If we opened the asset directory, close it and return success.
	if (nullptr != pAssetDir)
	{
		AAssetDir_close(pAssetDir);
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
Bool AndroidFileSystem::Open(
	const String& sAbsoluteFilename,
	File::Mode eMode,
	ScopedPtr<SyncFile>& rpFile)
{
	// Can only open files for kRead.
	if (File::kRead != eMode)
	{
		return false;
	}

	// Open the file - if this succeeds, set it and return success.
	ScopedPtr<SyncFile> pFile(SEOUL_NEW(MemoryBudgets::TBD) AndroidSyncFile(m_pAssetManager, sAbsoluteFilename, eMode));
	if (pFile.IsValid() && pFile->IsOpen())
	{
		rpFile.Swap(pFile);
		return true;
	}

	return false;
}

/**
 * @return True if a directory list for sAbsoluteDirectoryPath could
 * be generated, false otherwise. If this method returns true,
 * rvResults will contain a list of files and directories that fulfill
 * the other arguments to this function. Otherwise, rvResults will
 * be left unmodified.
 */
Bool AndroidFileSystem::GetDirectoryListing(
	const String& sAbsoluteDirectoryPath,
	Vector<String>& rvResults,
	Bool bIncludeDirectoriesInResults,
	Bool bRecursive,
	const String& sFileExtension) const
{
	// Can't completely implement directory listing using AAssetManager, unless we want to fail
	// if bRecursive or bIncludeDirectoriesInResults is true (we can't enumerate
	// sub-directories).
	return false;
}

} // namespace Seoul
