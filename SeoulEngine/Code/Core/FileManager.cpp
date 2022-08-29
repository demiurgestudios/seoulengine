/**
 * \file FileManager.cpp
 * \brief Singleton manager for abstracting file operations. File operations
 * (exists, time stamps, open, etc.) can be handled fomr package archive,
 * persistent storage, or in memory data stores under the hood.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "DiskFileSystem.h"
#include "FileManager.h"
#include "FileManagerRemap.h"
#include "GamePaths.h"
#include "Logger.h"
#include "Path.h"
#include "ThreadId.h"

namespace Seoul
{

/**
 * Callback that, if defined, will be called by FileManager to
 * initialize default file systems for the current project.
 */
InitializeFileSystemsCallback g_pInitializeFileSystemsCallback = nullptr;

/** Apply a new configureation to the FileManager's remap table. */
void FileManager::ConfigureRemap(const RemapTable& tRemap, UInt32 uHash)
{
	m_pRemap->Configure(tRemap, uHash);
}

/** Hash that was passed in with the last set remap table, used for detecting remap changes. */
UInt32 FileManager::GetRemapHash() const
{
	return m_pRemap->GetRemapHash();
}

/**
 * Construct the global FileManager object - must be called once and only
 * once at program startup.
 *
 * \pre FileManager::Get() must be nullptr - calling this method when g_pFilManager is non-null
 * will result in an assertion failure.
 */
void FileManager::Initialize()
{
	SEOUL_NEW(MemoryBudgets::Io) FileManager;

	// If a callback is defined, use it to setup initialize FileSystems.
	if (g_pInitializeFileSystemsCallback)
	{
		g_pInitializeFileSystemsCallback();
	}
	else
	{
		// Register a default disk file system.
		FileManager::Get()->RegisterFileSystem<DiskFileSystem>();
	}
}

/**
 * Destroy the global FileManager object - must be called once and only
 * once at program shutdown.
 *
 * \pre FileManager::Get() must be non-null - calling this method when g_pFilManager is nullptr
 * will result in an assertion failure.
 */
void FileManager::ShutDown()
{
	SEOUL_DELETE FileManager::Get();
}

/**
 * Attempt to copy the file, from -> to.
 *
 * @return true if the copy was successful, false otherwise.
 */
Bool FileManager::Copy(FilePath from, FilePath to, Bool bAllowOverwrite /* = false*/) const
{
	(void)m_pRemap->Remap(from);
	(void)m_pRemap->Remap(to);

	const Int32 nFileSystemCount = (Int32)m_vFileSystemStack.GetSize();
	for (Int32 i = (nFileSystemCount - 1); i >= 0; --i)
	{
		if (m_vFileSystemStack[i]->Copy(from, to, bAllowOverwrite))
		{
			return true;
		}
	}

	return false;
}

/**
 * Attempt to copy the file, sAbsoluteFrom -> sAbsoluteTo.
 *
 * @return true if the copy was successful, false otherwise.
 */
Bool FileManager::Copy(const String& sAbsoluteFrom, const String& sAbsoluteTo, Bool bAllowOverwrite /* = false*/) const
{
	const Int32 nFileSystemCount = (Int32)m_vFileSystemStack.GetSize();
	for (Int32 i = (nFileSystemCount - 1); i >= 0; --i)
	{
		if (m_vFileSystemStack[i]->Copy(sAbsoluteFrom, sAbsoluteTo, bAllowOverwrite))
		{
			return true;
		}
	}

	return false;
}

/**
 * Try to create the directory. If necessary,
 * will also attempt to create all parent directories that do
 * not exist.
 *
 * @return true if the directory already exists or was created successfully,
 * false otherwise.
 */
Bool FileManager::CreateDirPath(FilePath dirPath) const
{
	(void)m_pRemap->Remap(dirPath);

	const Int32 nFileSystemCount = (Int32)m_vFileSystemStack.GetSize();
	for (Int32 i = (nFileSystemCount - 1); i >= 0; --i)
	{
		if (m_vFileSystemStack[i]->CreateDirPath(dirPath))
		{
			return true;
		}
	}

	return false;
}

/**
 * Try to create the directory. If necessary,
 * will also attempt to create all parent directories that do
 * not exist.
 *
 * @return true if the directory already exists or was created successfully,
 * false otherwise.
 */
Bool FileManager::CreateDirPath(const String& sAbsoluteDir) const
{
	const Int32 nFileSystemCount = (Int32)m_vFileSystemStack.GetSize();
	for (Int32 i = (nFileSystemCount - 1); i >= 0; --i)
	{
		if (m_vFileSystemStack[i]->CreateDirPath(sAbsoluteDir))
		{
			return true;
		}
	}

	return false;
}

/**
 * Attempts to delete a file from the first virtual file system
 * that contains it.
 *
 * @return true on a successful deletion,
 * false otherwise.
 */
Bool FileManager::Delete(FilePath filePath) const
{
	(void)m_pRemap->Remap(filePath);

	const Int32 nFileSystemCount = (Int32)m_vFileSystemStack.GetSize();
	for (Int32 i = (nFileSystemCount - 1); i >= 0; --i)
	{
		if (m_vFileSystemStack[i]->Delete(filePath))
		{
			return true;
		}
	}

	return false;
}

/**
 * Attempts to delete a file from the first virtual file system
 * that contains it.
 *
 * @return true on a successful deletion,
 * false otherwise.
 */
Bool FileManager::Delete(const String& sAbsoluteFilename) const
{
	const Int32 nFileSystemCount = (Int32)m_vFileSystemStack.GetSize();
	for (Int32 i = (nFileSystemCount - 1); i >= 0; --i)
	{
		if (m_vFileSystemStack[i]->Delete(sAbsoluteFilename))
		{
			return true;
		}
	}

	return false;
}

/**
 * Try to delete the directory.
 *
 * @return true if the directory already exists or was created successfully,
 * false otherwise.
 */
Bool FileManager::DeleteDirectory(FilePath dirPath, Bool bRecursive) const
{
	(void)m_pRemap->Remap(dirPath);

	const Int32 nFileSystemCount = (Int32)m_vFileSystemStack.GetSize();
	for (Int32 i = (nFileSystemCount - 1); i >= 0; --i)
	{
		if (m_vFileSystemStack[i]->DeleteDirectory(dirPath, bRecursive))
		{
			return true;
		}
	}

	return false;
}

/**
 * Try to delete the directory.
 *
 * @return true if the directory already exists or was created successfully,
 * false otherwise.
 */
Bool FileManager::DeleteDirectory(const String& sAbsoluteDir, Bool bRecursive) const
{
	const Int32 nFileSystemCount = (Int32)m_vFileSystemStack.GetSize();
	for (Int32 i = (nFileSystemCount - 1); i >= 0; --i)
	{
		if (m_vFileSystemStack[i]->DeleteDirectory(sAbsoluteDir, bRecursive))
		{
			return true;
		}
	}

	return false;
}

/**
 * @return True if filePath exists in one of the FileSystems owned
 * by this FileManager, false otherwise.
 *
 * True will be returned if at least one FileSystem owned by this FileManager
 * contains the file filePath.
 */
Bool FileManager::Exists(FilePath filePath) const
{
	(void)m_pRemap->Remap(filePath);

	const Int32 nFileSystemCount = (Int32)m_vFileSystemStack.GetSize();
	for (Int32 i = (nFileSystemCount - 1); i >= 0; --i)
	{
		if (m_vFileSystemStack[i]->Exists(filePath))
		{
			return true;
		}
	}

	return false;
}

/**
 * @return True if sAbsoluteFilename exists in one of the FileSystems owned
 * by this FileManager, false otherwise.
 *
 * True will be returned if at least one FileSystem owned by this FileManager
 * contains the file sAbsoluteFilename.
 */
Bool FileManager::Exists(const String& sAbsoluteFilename) const
{
	const Int32 nFileSystemCount = (Int32)m_vFileSystemStack.GetSize();
	for (Int32 i = (nFileSystemCount - 1); i >= 0; --i)
	{
		if (m_vFileSystemStack[i]->Exists(sAbsoluteFilename))
		{
			return true;
		}
	}

	return false;
}

/**
 * @return True if sAbsoluteFilename exists in one of the FileSystems owned
 * by this FileManager, false otherwise.
 *
 * True will be returned if at least one FileSystem owned by this FileManager
 * contains the file sAbsoluteFilename.
 */
Bool FileManager::ExistsForPlatform(Platform ePlatform, FilePath filePath) const
{
	(void)m_pRemap->Remap(filePath);

	const Int32 nFileSystemCount = (Int32)m_vFileSystemStack.GetSize();
	for (Int32 i = (nFileSystemCount - 1); i >= 0; --i)
	{
		if (m_vFileSystemStack[i]->ExistsForPlatform(ePlatform, filePath))
		{
			return true;
		}
	}

	return false;
}

/**
 * Variation that resolve filePath to the project's Source/ folder. Used in
 * particular scenarios where files are tracked by FilePath by must
 * be compared against their source counterpart (e.g. the Cooker and the CookDatabase).
 */
Bool FileManager::ExistsInSource(FilePath filePath) const
{
	(void)m_pRemap->Remap(filePath);

	const Int32 nFileSystemCount = (Int32)m_vFileSystemStack.GetSize();
	for (Int32 i = (nFileSystemCount - 1); i >= 0; --i)
	{
		if (m_vFileSystemStack[i]->ExistsInSource(filePath))
		{
			return true;
		}
	}

	return false;
}

/**
 * @return True if filePath exists and is a directory in one of the
 * FileSystems owned by this FileManager, false otherwise.
 */
Bool FileManager::IsDirectory(FilePath filePath) const
{
	(void)m_pRemap->Remap(filePath);

	const Int32 nFileSystemCount = (Int32)m_vFileSystemStack.GetSize();
	for (Int32 i = (nFileSystemCount - 1); i >= 0; --i)
	{
		if (m_vFileSystemStack[i]->IsDirectory(filePath))
		{
			return true;
		}
	}

	return false;
}

/**
 * @return True if sAbsoluteFilename exists and is a directory in one of the
 * FileSystems owned by this FileManager, false otherwise.
 */
Bool FileManager::IsDirectory(const String& sAbsoluteFilename) const
{
	const Int32 nFileSystemCount = (Int32)m_vFileSystemStack.GetSize();
	for (Int32 i = (nFileSystemCount - 1); i >= 0; --i)
	{
		if (m_vFileSystemStack[i]->IsDirectory(sAbsoluteFilename))
		{
			return true;
		}
	}

	return false;
}

/**
 * @return The file size reported by the first FileSystem that reports
 * a time for filePath.
 *
 * FileSystems are evaluated in LIFO order.
 */
UInt64 FileManager::GetFileSize(FilePath filePath) const
{
	(void)m_pRemap->Remap(filePath);

	const Int32 nFileSystemCount = (Int32)m_vFileSystemStack.GetSize();
	for (Int32 i = (nFileSystemCount - 1); i >= 0; --i)
	{
		UInt64 zFileSize = 0u;
		if (m_vFileSystemStack[i]->GetFileSize(filePath, zFileSize))
		{
			return zFileSize;
		}
	}

	return 0u;
}

/**
 * @return The file size reported by the first FileSystem that reports
 * a time for filePath.
 *
 * FileSystems are evaluated in LIFO order.
 */
UInt64 FileManager::GetFileSize(const String& sAbsoluteFilename) const
{
	const Int32 nFileSystemCount = (Int32)m_vFileSystemStack.GetSize();
	for (Int32 i = (nFileSystemCount - 1); i >= 0; --i)
	{
		UInt64 zFileSize = 0u;
		if (m_vFileSystemStack[i]->GetFileSize(sAbsoluteFilename, zFileSize))
		{
			return zFileSize;
		}
	}

	return 0u;
}

/**
 * @return True if sAbsoluteFilename exists in one of the FileSystems owned
 * by this FileManager, false otherwise.
 *
 * True will be returned if at least one FileSystem owned by this FileManager
 * contains the file sAbsoluteFilename.
 */
UInt64 FileManager::GetFileSizeForPlatform(Platform ePlatform, FilePath filePath) const
{
	(void)m_pRemap->Remap(filePath);

	const Int32 nFileSystemCount = (Int32)m_vFileSystemStack.GetSize();
	for (Int32 i = (nFileSystemCount - 1); i >= 0; --i)
	{
		UInt64 zFileSize = 0u;
		if (m_vFileSystemStack[i]->GetFileSizeForPlatform(ePlatform, filePath, zFileSize))
		{
			return zFileSize;
		}
	}

	return 0u;
}

/**
 * @return The modified time reported by the first FileSystem that reports
 * a time for filePath.
 *
 * FileSystems are evaluated in LIFO order.
 *
 * The modified time may be 0u even if the file exists, if a FileSystem contains
 * the file but does not track modified times.
 */
UInt64 FileManager::GetModifiedTime(FilePath filePath) const
{
	(void)m_pRemap->Remap(filePath);

	const Int32 nFileSystemCount = (Int32)m_vFileSystemStack.GetSize();
	for (Int32 i = (nFileSystemCount - 1); i >= 0; --i)
	{
		UInt64 uModifiedTime = 0u;
		if (m_vFileSystemStack[i]->GetModifiedTime(filePath, uModifiedTime))
		{
			return uModifiedTime;
		}
	}

	return 0u;
}

/**
 * @return The modified time reported by the first FileSystem that reports
 * a time for filePath.
 *
 * FileSystems are evaluated in LIFO order.
 *
 * The modified time may be 0u even if the file exists, if a FileSystem contains
 * the file but does not track modified times.
 */
UInt64 FileManager::GetModifiedTime(const String& sAbsoluteFilename) const
{
	const Int32 nFileSystemCount = (Int32)m_vFileSystemStack.GetSize();
	for (Int32 i = (nFileSystemCount - 1); i >= 0; --i)
	{
		UInt64 uModifiedTime = 0u;
		if (m_vFileSystemStack[i]->GetModifiedTime(sAbsoluteFilename, uModifiedTime))
		{
			return uModifiedTime;
		}
	}

	return 0u;
}

/**
 * @return True if sAbsoluteFilename exists in one of the FileSystems owned
 * by this FileManager, false otherwise.
 *
 * True will be returned if at least one FileSystem owned by this FileManager
 * contains the file sAbsoluteFilename.
 */
UInt64 FileManager::GetModifiedTimeForPlatform(Platform ePlatform, FilePath filePath) const
{
	(void)m_pRemap->Remap(filePath);

	const Int32 nFileSystemCount = (Int32)m_vFileSystemStack.GetSize();
	for (Int32 i = (nFileSystemCount - 1); i >= 0; --i)
	{
		UInt64 uModifiedTime = 0u;
		if (m_vFileSystemStack[i]->GetModifiedTimeForPlatform(ePlatform, filePath, uModifiedTime))
		{
			return uModifiedTime;
		}
	}

	return 0u;
}

/**
 * Check the modification time of a filePath in the project's
 * Source/ folder.
 *
 * @return The modification time or 0 if the file does not
 * exist in Source/.
 */
UInt64 FileManager::GetModifiedTimeInSource(FilePath filePath) const
{
	(void)m_pRemap->Remap(filePath);

	const Int32 nFileSystemCount = (Int32)m_vFileSystemStack.GetSize();
	for (Int32 i = (nFileSystemCount - 1); i >= 0; --i)
	{
		UInt64 uModifiedTime = 0u;
		if (m_vFileSystemStack[i]->GetModifiedTimeInSource(filePath, uModifiedTime))
		{
			return uModifiedTime;
		}
	}

	return 0u;
}

/**
 * Attempt to rename the file, from -> to.
 *
 * @return true if the rename was successful, false otherwise.
 */
Bool FileManager::Rename(FilePath from, FilePath to) const
{
	(void)m_pRemap->Remap(from);
	(void)m_pRemap->Remap(to);

	const Int32 nFileSystemCount = (Int32)m_vFileSystemStack.GetSize();
	for (Int32 i = (nFileSystemCount - 1); i >= 0; --i)
	{
		if (m_vFileSystemStack[i]->Rename(from, to))
		{
			return true;
		}
	}

	return false;
}

/**
 * Attempt to rename the file, sAbsoluteFrom -> sAbsoluteTo.
 *
 * @return true if the rename was successful, false otherwise.
 */
Bool FileManager::Rename(const String& sAbsoluteFrom, const String& sAbsoluteTo) const
{
	const Int32 nFileSystemCount = (Int32)m_vFileSystemStack.GetSize();
	for (Int32 i = (nFileSystemCount - 1); i >= 0; --i)
	{
		if (m_vFileSystemStack[i]->Rename(sAbsoluteFrom, sAbsoluteTo))
		{
			return true;
		}
	}

	return false;
}

/**
 * Attempt to set the modification time of filePath to
 * uModifiedTime.
 *
 * @return True if the set was successful, false otherwise.
 */
Bool FileManager::SetModifiedTime(FilePath filePath, UInt64 uModifiedTime) const
{
	(void)m_pRemap->Remap(filePath);

	const Int32 nFileSystemCount = (Int32)m_vFileSystemStack.GetSize();
	for (Int32 i = (nFileSystemCount - 1); i >= 0; --i)
	{
		if (m_vFileSystemStack[i]->SetModifiedTime(filePath, uModifiedTime))
		{
			return true;
		}
	}

	return false;
}

/**
 * Attempt to set the modification time of sAbsoluteFilename to
 * uModifiedTime.
 *
 * @return True if the set was successful, false otherwise.
 */
Bool FileManager::SetModifiedTime(const String& sAbsoluteFilename, UInt64 uModifiedTime) const
{
	const Int32 nFileSystemCount = (Int32)m_vFileSystemStack.GetSize();
	for (Int32 i = (nFileSystemCount - 1); i >= 0; --i)
	{
		if (m_vFileSystemStack[i]->SetModifiedTime(sAbsoluteFilename, uModifiedTime))
		{
			return true;
		}
	}

	return false;
}

/**
 * Attempt to update the read/write status of a file.
 *
 * @return true on successful change, false otherwise.
 */
Bool FileManager::SetReadOnlyBit(FilePath filePath, Bool bReadOnly) const
{
	(void)m_pRemap->Remap(filePath);

	const Int32 nFileSystemCount = (Int32)m_vFileSystemStack.GetSize();
	for (Int32 i = (nFileSystemCount - 1); i >= 0; --i)
	{
		if (m_vFileSystemStack[i]->SetReadOnlyBit(filePath, bReadOnly))
		{
			return true;
		}
	}

	return false;
}

/**
 * Attempt to update the read/write status of a file.
 *
 * @return true on successful change, false otherwise.
 */
Bool FileManager::SetReadOnlyBit(const String& sAbsoluteFilename, Bool bReadOnly) const
{
	const Int32 nFileSystemCount = (Int32)m_vFileSystemStack.GetSize();
	for (Int32 i = (nFileSystemCount - 1); i >= 0; --i)
	{
		if (m_vFileSystemStack[i]->SetReadOnlyBit(sAbsoluteFilename, bReadOnly))
		{
			return true;
		}
	}

	return false;
}

/**
 * Attempt to open the file filePath with permissions eMode.
 *
 * @return True if the open was successful, false otherwise. If this method
 * returns true, then rpFile will contained a non-null pointer to the SyncFile.
 * Otherwise, rpFile will be left unmodified.
 *
 * FileSystems are asked to fulfill the request in LIFO with respect
 * to the order they were registered. The first FileSystem to fulfill the
 * request will be the system that provides the returned file pointer.
 */
Bool FileManager::OpenFile(
	FilePath filePath,
	File::Mode eMode,
	ScopedPtr<SyncFile>& rpFile) const
{
	(void)m_pRemap->Remap(filePath);

	SEOUL_LOG_FILEIO("[FileManager]: OpenFile %s", filePath.GetAbsoluteFilename().CStr());

	const Int32 nFileSystemCount = (Int32)m_vFileSystemStack.GetSize();
	for (Int32 i = (nFileSystemCount - 1); i >= 0; --i)
	{
		if (m_vFileSystemStack[i]->Open(filePath, eMode, rpFile))
		{
			return true;
		}
	}

	return false;
}

/**
 * Attempt to open the file sAbsoluteFilename with permissions eMode.
 *
 * @return True if the open was successful, false otherwise. If this method
 * returns true, then rpFile will contained a non-null pointer to the SyncFile.
 * Otherwise, rpFile will be left unmodified.
 *
 * FileSystems are asked to fulfill the request in LIFO with respect
 * to the order they were registered. The first FileSystem to fulfill the
 * request will be the system that provides the returned file pointer.
 */
Bool FileManager::OpenFile(
	const String& sAbsoluteFilename,
	File::Mode eMode,
	ScopedPtr<SyncFile>& rpFile) const
{
	SEOUL_LOG_FILEIO("[FileManager]: OpenFile %s", sAbsoluteFilename.CStr());

	const Int32 nFileSystemCount = (Int32)m_vFileSystemStack.GetSize();
	for (Int32 i = (nFileSystemCount - 1); i >= 0; --i)
	{
		if (m_vFileSystemStack[i]->Open(sAbsoluteFilename, eMode, rpFile))
		{
			return true;
		}
	}

	return false;
}

/**
 * Convenience function - same functionality as
 * DiskSyncFile::ReadAll(), but handles the open through FileManager,
 * so the actual data can be read from a pack file, off disk, etc.
 */
Bool FileManager::ReadAll(
	FilePath filePath,
	void*& rpOutputBuffer,
	UInt32& rzOutputSizeInBytes,
	UInt32 zAlignmentOfOutputBuffer,
	MemoryBudgets::Type eOutputBufferMemoryType,
	UInt32 zMaxReadSize) const
{
	(void)m_pRemap->Remap(filePath);

	return InternalReadAll(
		filePath,
		rpOutputBuffer,
		rzOutputSizeInBytes,
		zAlignmentOfOutputBuffer,
		eOutputBufferMemoryType,
		zMaxReadSize);
}

/**
 * Variation of ReadAll that resolves filePath based
 * on the given platform.
 */
Bool FileManager::ReadAllForPlatform(
	Platform ePlatform,
	FilePath filePath,
	void*& rpOutputBuffer,
	UInt32& rzOutputSizeInBytes,
	UInt32 zAlignmentOfOutputBuffer,
	MemoryBudgets::Type eOutputBufferMemoryType,
	UInt32 zMaxReadSize /*= kDefaultMaxReadSize*/) const
{
	SEOUL_LOG_FILEIO("[FileManager]: ReadAllForPlatform %s", filePath.GetAbsoluteFilenameForPlatform(ePlatform).CStr());

	const Int32 nFileSystemCount = (Int32)m_vFileSystemStack.GetSize();
	for (Int32 i = (nFileSystemCount - 1); i >= 0; --i)
	{
		if (m_vFileSystemStack[i]->ReadAllForPlatform(
			ePlatform,
			filePath,
			rpOutputBuffer,
			rzOutputSizeInBytes,
			zAlignmentOfOutputBuffer,
			eOutputBufferMemoryType,
			zMaxReadSize))
		{
			return true;
		}
	}

	return false;
}

/**
 * Convenience method to load all of a file's data into a Vector instead
 * of a bag of bytes
 *
 * @param[in] filePath File path to read in
 * @param[out] rOutData Receives the file data as a Vector<Byte>, if successful
 * @param[in] zMaxReadSize Maximum file size to attempt to read (sanity check)
 *
 * @return True if the file was read successfully, or false if an error
 *         occurred
 */
Bool FileManager::ReadAll(
	FilePath filePath,
	Vector<Byte>& rvOutData,
	UInt32 zMaxReadSize) const
{
	(void)m_pRemap->Remap(filePath);

	void* pRawData = nullptr;
	UInt32 uDataSize = 0u;
	if (InternalReadAll(filePath, pRawData, uDataSize, 0, MemoryBudgets::Io, zMaxReadSize))
	{
		rvOutData.Resize(uDataSize);
		if (uDataSize > 0u)
		{
			memcpy(rvOutData.Get(0u), pRawData, uDataSize);
		}
		MemoryManager::Deallocate(pRawData);
		return true;
	}
	else
	{
		return false;
	}
}

/**
 * Convenience method to load all of a file's data into a String instead
 * of a bag of bytes
 *
 * @param[in] filePath File path to read in
 * @param[out] rOutData Receives the file data as a string, if successful
 * @param[in] zMaxReadSize Maximum file size to attempt to read (sanity check)
 *
 * @return True if the file was read successfully, or false if an error
 *         occurred
 */
Bool FileManager::ReadAll(
	FilePath filePath,
	String& rsOutData,
	UInt32 zMaxReadSize) const
{
	(void)m_pRemap->Remap(filePath);

	void *pRawData = nullptr;
	UInt32 uDataSize = 0u;

	if (InternalReadAll(filePath, pRawData, uDataSize, 0, MemoryBudgets::Io, zMaxReadSize))
	{
		// FIXME: Don't copy the data here
		rsOutData.Assign((Byte *)pRawData, uDataSize);
		MemoryManager::Deallocate(pRawData);
		return true;
	}
	else
	{
		return false;
	}
}

/**
 * Convenience function - same functionality as
 * DiskSyncFile::ReadAll(), but handles the open through FileManager,
 * so the actual data can be read from a pack file, off disk, etc.
 */
Bool FileManager::ReadAll(
	const String& sAbsoluteFilename,
	void*& rpOutputBuffer,
	UInt32& rzOutputSizeInBytes,
	UInt32 zAlignmentOfOutputBuffer,
	MemoryBudgets::Type eOutputBufferMemoryType,
	UInt32 zMaxReadSize) const
{
	SEOUL_LOG_FILEIO("[FileManager]: ReadAll %s", sAbsoluteFilename.CStr());

	const Int32 nFileSystemCount = (Int32)m_vFileSystemStack.GetSize();
	for (Int32 i = (nFileSystemCount - 1); i >= 0; --i)
	{
		if (m_vFileSystemStack[i]->ReadAll(
			sAbsoluteFilename,
			rpOutputBuffer,
			rzOutputSizeInBytes,
			zAlignmentOfOutputBuffer,
			eOutputBufferMemoryType,
			zMaxReadSize))
		{
			return true;
		}
	}

	return false;
}

/**
 * Convenience method to load all of a file's data into a Vector instead
 * of a bag of bytes
 *
 * @param[in] sAbsoluteFilename Fully resolved filename to read in
 * @param[out] rOutData Receives the file data as a Vector<Byte>, if successful
 * @param[in] zMaxReadSize Maximum file size to attempt to read (sanity check)
 *
 * @return True if the file was read successfully, or false if an error
 *         occurred
 */
Bool FileManager::ReadAll(
	const String& sAbsoluteFilename,
	Vector<Byte>& rvOutData,
	UInt32 zMaxReadSize) const
{
	void *pRawData = nullptr;
	UInt32 uDataSize = 0u;
	if (ReadAll(sAbsoluteFilename, pRawData, uDataSize, 0, MemoryBudgets::Io, zMaxReadSize))
	{
		rvOutData.Resize(uDataSize);
		if (uDataSize > 0u)
		{
			memcpy(rvOutData.Get(0u), pRawData, uDataSize);
		}
		MemoryManager::Deallocate(pRawData);
		return true;
	}
	else
	{
		return false;
	}
}

/**
 * Convenience method to load all of a file's data into a String instead
 * of a bag of bytes
 *
 * @param[in] sAbsoluteFilename Fully resolved filename to read in
 * @param[out] rOutData Receives the file data as a string, if successful
 * @param[in] zMaxReadSize Maximum file size to attempt to read (sanity check)
 *
 * @return True if the file was read successfully, or false if an error
 *         occurred
 */
Bool FileManager::ReadAll(
	const String& sAbsoluteFilename,
	String& rsOutData,
	UInt32 zMaxReadSize) const
{
	void *pRawData;
	UInt32 uDataSize;

	if (ReadAll(sAbsoluteFilename, pRawData, uDataSize, 0, MemoryBudgets::Io, zMaxReadSize))
	{
		// FIXME: Don't copy the data here
		rsOutData.Assign((Byte *)pRawData, uDataSize);
		MemoryManager::Deallocate(pRawData);
		return true;
	}
	else
	{
		return false;
	}
}

/**
 * @return True if any networked file system is still initializing,
 * false otherwise.
 */
Bool FileManager::IsAnyFileSystemStillInitializing() const
{
	const Int32 nFileSystemCount = (Int32)m_vFileSystemStack.GetSize();
	for (Int32 i = (nFileSystemCount - 1); i >= 0; --i)
	{
		if (m_vFileSystemStack[i]->IsInitializing())
		{
			return true;
		}
	}

	return false;
}

/**
 * @return True if operations on filePath will be serviced by
 * a network file system, false otherwise.
 */
Bool FileManager::IsServicedByNetwork(FilePath filePath) const
{
	(void)m_pRemap->Remap(filePath);

	Bool bIsAnyFileSystemStillInitializing = false;
	const Int32 nFileSystemCount = (Int32)m_vFileSystemStack.GetSize();
	for (Int32 i = (nFileSystemCount - 1); i >= 0; --i)
	{
		IFileSystem* pFileSystem = m_vFileSystemStack[i];
		if (pFileSystem->Exists(filePath))
		{
			return pFileSystem->IsServicedByNetwork(filePath);
		}
		else if (pFileSystem->IsInitializing())
		{
			bIsAnyFileSystemStillInitializing = true;
		}
	}

	// If no FileSystem handles the request, we assume that
	// the file is networked serviced depending on whether *any*
	// FileSystem is still initializing - this is the conservative
	// default, since it assumes a file is network serviced until
	// proven otherwise.
	return bIsAnyFileSystemStillInitializing;
}

/**
 * @return True if operations on filePath will be serviced by
 * a network file system, false otherwise.
 */
Bool FileManager::IsServicedByNetwork(const String& sAbsoluteFilename) const
{
	Bool bIsAnyFileSystemStillInitializing = false;
	const Int32 nFileSystemCount = (Int32)m_vFileSystemStack.GetSize();
	for (Int32 i = (nFileSystemCount - 1); i >= 0; --i)
	{
		IFileSystem* pFileSystem = m_vFileSystemStack[i];
		if (pFileSystem->Exists(sAbsoluteFilename))
		{
			return pFileSystem->IsServicedByNetwork(sAbsoluteFilename);
		}
		else if (pFileSystem->IsInitializing())
		{
			bIsAnyFileSystemStillInitializing = true;
		}
	}

	// If no FileSystem handles the request, we assume that
	// the file is networked serviced depending on whether *any*
	// FileSystem is still initializing - this is the conservative
	// default, since it assumes a file is network serviced until
	// proven otherwise.
	return bIsAnyFileSystemStillInitializing;
}

Bool FileManager::NetworkFetch(FilePath filePath, NetworkFetchPriority ePriority /*= NetworkFetchPriority::kDefault*/) const
{
	(void)m_pRemap->Remap(filePath);

	const Int32 nFileSystemCount = (Int32)m_vFileSystemStack.GetSize();
	for (Int32 i = (nFileSystemCount - 1); i >= 0; --i)
	{
		if (m_vFileSystemStack[i]->Exists(filePath))
		{
			return m_vFileSystemStack[i]->NetworkFetch(filePath, ePriority);
		}
	}

	return false;
}

Bool FileManager::NetworkPrefetch(FilePath filePath, NetworkFetchPriority ePriority /*= kMedium*/) const
{
	(void)m_pRemap->Remap(filePath);

	const Int32 nFileSystemCount = (Int32)m_vFileSystemStack.GetSize();
	for (Int32 i = (nFileSystemCount - 1); i >= 0; --i)
	{
		if (m_vFileSystemStack[i]->Exists(filePath))
		{
			return m_vFileSystemStack[i]->NetworkPrefetch(filePath, ePriority);
		}
	}

	return false;
}

void FileManager::EnableNetworkFileIO()
{
	Lock lock(m_NetworkFileIOMutex);

	if (!m_bNetworkFileIOEnabled)
	{
		m_bNetworkFileIOShutdown = false;
		m_bNetworkFileIOEnabled = true;

		const Int32 nFileSystemCount = (Int32)m_vFileSystemStack.GetSize();
		for (Int32 i = (nFileSystemCount - 1); i >= 0; --i)
		{
			if (!m_RefOnly.HasKey(m_vFileSystemStack[i]))
			{
				m_vFileSystemStack[i]->OnNetworkInitialize();
			}
		}
	}
}

void FileManager::DisableNetworkFileIO()
{
	Lock lock(m_NetworkFileIOMutex);

	if (m_bNetworkFileIOEnabled)
	{
		const Int32 nFileSystemCount = (Int32)m_vFileSystemStack.GetSize();
		for (Int32 i = (nFileSystemCount - 1); i >= 0; --i)
		{
			if (!m_RefOnly.HasKey(m_vFileSystemStack[i]))
			{
				m_vFileSystemStack[i]->OnNetworkShutdown();
			}
		}

		m_bNetworkFileIOEnabled = false;
		m_bNetworkFileIOShutdown = true;
	}
}

/**
 * @return True if a directory listing could be generated for directory
 * dirPath, false otherwise. If this method returns
 * true, rvResults will contain files and (optionally) directories contained
 * within dirPath based on the other arguments to this
 * method. If this method returns false, rvResults will be left
 * unmodified.
 */
Bool FileManager::GetDirectoryListing(
	FilePath dirPath,
	Vector<String>& rvResults,
	Bool bIncludeDirectoriesInResults,
	Bool bRecursive,
	const String& sFileExtension) const
{
	(void)m_pRemap->Remap(dirPath);

	const Int32 nFileSystemCount = (Int32)m_vFileSystemStack.GetSize();
	for (Int32 i = (nFileSystemCount - 1); i >= 0; --i)
	{
		if (m_vFileSystemStack[i]->GetDirectoryListing(
			dirPath,
			rvResults,
			bIncludeDirectoriesInResults,
			bRecursive,
			sFileExtension))
		{
			return true;
		}
	}

	return false;
}

/**
 * @return True if a directory listing could be generated for directory
 * sAbsoluteDirectoryPath, false otherwise. If this method returns
 * true, rvResults will contain files and (optionally) directories contained
 * within sAbsoluteDirectoryPath based on the other arguments to this
 * method. If this method returns false, rvResults will be left
 * unmodified.
 */
Bool FileManager::GetDirectoryListing(
	const String& sAbsoluteDirectoryPath,
	Vector<String>& rvResults,
	Bool bIncludeDirectoriesInResults,
	Bool bRecursive,
	const String& sFileExtension) const
{
	const Int32 nFileSystemCount = (Int32)m_vFileSystemStack.GetSize();
	for (Int32 i = (nFileSystemCount - 1); i >= 0; --i)
	{
		if (m_vFileSystemStack[i]->GetDirectoryListing(
			sAbsoluteDirectoryPath,
			rvResults,
			bIncludeDirectoriesInResults,
			bRecursive,
			sFileExtension))
		{
			return true;
		}
	}

	return false;
}

/**
 * Convenience function - same functionality as
 * DiskSyncFile::WriteAll(), but handles the open through FileManager,
 * so the actual data can be read from a pack file, off disk, etc.
 */
Bool FileManager::WriteAll(
	FilePath filePath,
	void const* pInputBuffer,
	UInt32 uInputSizeInBytes,
	UInt64 uModifiedTime /*= 0*/) const
{
	(void)m_pRemap->Remap(filePath);

	SEOUL_LOG_FILEIO("[FileManager]: WriteAll %s", filePath.GetAbsoluteFilename().CStr());

	const Int32 nFileSystemCount = (Int32)m_vFileSystemStack.GetSize();
	for (Int32 i = (nFileSystemCount - 1); i >= 0; --i)
	{
		if (m_vFileSystemStack[i]->WriteAll(
			filePath,
			pInputBuffer,
			uInputSizeInBytes,
			uModifiedTime))
		{
			return true;
		}
	}

	return false;
}

/**
 * Convenience method - executes the same operations as DiskSyncFile::WriteAll(),
 * except that the save is processed using FileManager's FileSystem stack.
 */
Bool FileManager::WriteAllForPlatform(
	Platform ePlatform,
	FilePath filePath,
	void const* pInputBuffer,
	UInt32 uInputSizeInBytes,
	UInt64 uModifiedTime /*= 0*/) const
{
	(void)m_pRemap->Remap(filePath);

	SEOUL_LOG_FILEIO("[FileManager]: WriteAllForPlatform %s", filePath.GetAbsoluteFilenameForPlatform(ePlatform).CStr());

	const Int32 nFileSystemCount = (Int32)m_vFileSystemStack.GetSize();
	for (Int32 i = (nFileSystemCount - 1); i >= 0; --i)
	{
		if (m_vFileSystemStack[i]->WriteAllForPlatform(
			ePlatform,
			filePath,
			pInputBuffer,
			uInputSizeInBytes,
			uModifiedTime))
		{
			return true;
		}
	}

	return false;
}

/**
 * Convenience function - same functionality as
 * DiskSyncFile::WriteAll(), but handles the open through FileManager,
 * so the actual data can be read from a pack file, off disk, etc.
 */
Bool FileManager::WriteAll(
	const String& sAbsoluteFilename,
	void const* pInputBuffer,
	UInt32 uInputSizeInBytes,
	UInt64 uModifiedTime /*= 0*/) const
{
	SEOUL_LOG_FILEIO("[FileManager]: WriteAll %s", sAbsoluteFilename.CStr());

	const Int32 nFileSystemCount = (Int32)m_vFileSystemStack.GetSize();
	for (Int32 i = (nFileSystemCount - 1); i >= 0; --i)
	{
		if (m_vFileSystemStack[i]->WriteAll(
			sAbsoluteFilename,
			pInputBuffer,
			uInputSizeInBytes,
			uModifiedTime))
		{
			return true;
		}
	}

	return false;
}

FileManager::FileManager()
	: m_pRemap(SEOUL_NEW(MemoryBudgets::Io) FileManagerRemap)
	, m_vFileSystemStack()
	, m_NetworkFileIOMutex()
	, m_bNetworkFileIOEnabled(false)
	, m_bNetworkFileIOShutdown(false)
{
}

FileManager::~FileManager()
{
	// Sanity check, the environment must have called OnNetworkShutdown().
	SEOUL_ASSERT(!m_bNetworkFileIOEnabled);

	FileSystemStack v;
	v.Swap(m_vFileSystemStack);
	for (Int i = (Int)v.GetSize() - 1; i >= 0; --i)
	{
		if (m_RefOnly.HasKey(v[i]))
		{
			v[i] = nullptr;
		}
		else
		{
			SafeDelete(v[i]);
		}
	}
}

Bool FileManager::InternalReadAll(
	FilePath filePath,
	void*& rpOutputBuffer,
	UInt32& rzOutputSizeInBytes,
	UInt32 zAlignmentOfOutputBuffer,
	MemoryBudgets::Type eOutputBufferMemoryType,
	UInt32 zMaxReadSize) const
{
	SEOUL_LOG_FILEIO("[FileManager]: ReadAll %s", filePath.GetAbsoluteFilename().CStr());

	const Int32 nFileSystemCount = (Int32)m_vFileSystemStack.GetSize();
	for (Int32 i = (nFileSystemCount - 1); i >= 0; --i)
	{
		if (m_vFileSystemStack[i]->ReadAll(
			filePath,
			rpOutputBuffer,
			rzOutputSizeInBytes,
			zAlignmentOfOutputBuffer,
			eOutputBufferMemoryType,
			zMaxReadSize))
		{
			return true;
		}
	}

	return false;
}

} // namespace Seoul
