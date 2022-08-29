/**
 * \file MoriartyFileSystem.cpp
 * \brief File system implementation for using the Moriarty client to access
 * files over the network
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FileManager.h"
#include "MoriartyClient.h"
#include "MoriartyFileSystem.h"
#include "MoriartySyncFile.h"

#if SEOUL_WITH_MORIARTY

namespace Seoul
{

/** MoriartyFileSystem constructor */
MoriartyFileSystem::MoriartyFileSystem()
{
}

/** MoriartyFileSystem destructor */
MoriartyFileSystem::~MoriartyFileSystem()
{
}

// Attempt to copy from -> to.
Bool MoriartyFileSystem::Copy(FilePath from, FilePath to, Bool bAllowOverwrite /*= false*/)
{
	return (MoriartyClient::Get() && MoriartyClient::Get()->Copy(from, to, bAllowOverwrite));
}

/**
 * Try to create the directory. If necessary,
 * will also attempt to create all parent directories that do
 * not exist.
 */
Bool MoriartyFileSystem::CreateDirPath(FilePath dirPath)
{
	return (MoriartyClient::Get() && MoriartyClient::Get()->CreateDirPath(dirPath));
}

/**
 * Try to delete the directory.
 */
Bool MoriartyFileSystem::DeleteDirectory(FilePath dirPath, Bool bRecursive)
{
	return (MoriartyClient::Get() && MoriartyClient::Get()->DeleteDirectory(dirPath, bRecursive));
}

/**
 * Return true if this FileSystem contains filePath, false otherwise. If this
 * method returns true, rzFileSize will contain the file size that this
 * FileSystem tracks for the file.
 */
Bool MoriartyFileSystem::GetFileSize(FilePath filePath, UInt64& rzFileSize) const
{
	FileStat stat;
	if (MoriartyClient::Get() && MoriartyClient::Get()->StatFile(filePath, &stat))
	{
		rzFileSize = stat.m_uFileSize;
		return true;
	}
	else
	{
		return false;
	}
}

/**
 * Return true if this FileSystem contains filePath, false otherwise. If this
 * method returns true, ruModifiedTime will contain the modified time that this
 * FileSystem tracks for the file. This value may be 0u if the FileSystem does
 * not track modified times.
 */
Bool MoriartyFileSystem::GetModifiedTime(FilePath filePath, UInt64& ruModifiedTime) const
{
	FileStat stat;
	if (MoriartyClient::Get() && MoriartyClient::Get()->StatFile(filePath, &stat))
	{
		ruModifiedTime = stat.m_uModifiedTime;
		return true;
	}
	else
	{
		return false;
	}
}

/**
 * Return true if this FileSystem successfully updated the modified time of
 * filePath to uModifiedTime, false otherwise. This method can return false if
 * the file does not exist in this IFileSystem, or if the set failed for some
 * os/filesystem specific reason (insufficient privileges).
 */
Bool MoriartyFileSystem::SetModifiedTime(FilePath filePath, UInt64 uModifiedTime)
{
	return (MoriartyClient::Get() && MoriartyClient::Get()->SetFileModifiedTime(filePath, uModifiedTime));
}

/**
 * Attempt to delete filePath.
 *
 * @return true on success, false otherwise.
 */
Bool MoriartyFileSystem::Delete(FilePath dirPath)
{
	return (MoriartyClient::Get() && MoriartyClient::Get()->Delete(dirPath));
}

/**
 * Return true if the file described by filePath exists in this file system,
 * false otherwise.
 */
Bool MoriartyFileSystem::Exists(FilePath filePath) const
{
	FileStat stat;
	return (MoriartyClient::Get() && MoriartyClient::Get()->StatFile(filePath, &stat));
}

/**
 * Return true if the entry described by filePath exists in this file system
 * and is a directory, false otherwise
 */
Bool MoriartyFileSystem::IsDirectory(FilePath filePath) const
{
	FileStat stat;
	return (MoriartyClient::Get() &&
			MoriartyClient::Get()->StatFile(filePath, &stat) &&
			stat.m_bIsDirectory);
}

/**
 * Return true if the file could be opened, false otherwise.  If this method
 * returns true, rpFile is guaranteed to be non-nullptr and SyncFile::IsOpen() is
 * guaranteed to return true.
 */
Bool MoriartyFileSystem::Open(
	FilePath filePath,
	File::Mode eMode,
	ScopedPtr<SyncFile>& rpFile)
{
	if (!MoriartyClient::Get())
	{
		return false;
	}

	if (File::kRead == eMode)
	{
		// Optimization for read-only mode, based on the assumption that the user will have
		// most files accessible and available through a local FileSystem. We
		// get the time stamp for the file from Moriarty before opening it,
		// and then find the first file system which a mod time that matches.
		UInt64 uModifiedTime = 0u;
		if (!GetModifiedTime(filePath, uModifiedTime))
		{
			// If we failed getting a mod time, we won't be able to open the first
			return false;
		}

		// Now get the FileSystem stack, and find this Moriarty file system.
		const FileManager::FileSystemStack& vStack = FileManager::Get()->GetFileSystemStack();
		Int i = (Int)vStack.GetSize() - 1;
		for (; i >= 0; --i)
		{
			if (vStack[i] == this)
			{
				--i;
				break;
			}
		}

		// Now find the first file system with a matching time stamp.
		for (; i >= 0; --i)
		{
			UInt64 uOtherModifiedTime = 0u;
			if (vStack[i]->GetModifiedTime(filePath, uOtherModifiedTime) &&
				uOtherModifiedTime == uModifiedTime)
			{
				// If we found a match, open the file through the other file system.
				return vStack[i]->Open(filePath, eMode, rpFile);
			}
		}
	}

	SyncFile *pRawFile = SEOUL_NEW(MemoryBudgets::Io) MoriartySyncFile(filePath, eMode);
	if (pRawFile->IsOpen())
	{
		rpFile.Reset(SEOUL_NEW(MemoryBudgets::Io) BufferedSyncFile(pRawFile, true));
		SEOUL_ASSERT(rpFile->IsOpen());
		return true;
	}
	else
	{
		SEOUL_DELETE pRawFile;
		return false;
	}
}

/**
 * Attempt to populate rvResults with a list of files contained in the
 * directory represented by dirPath based on arguments. Returns true if
 * successful, false otherwise. If this method returns false, rvResults is left
 * unmodified.
 */
Bool MoriartyFileSystem::GetDirectoryListing(
	FilePath dirPath,
	Vector<String>& rvResults,
	Bool bIncludeDirectoriesInResults /*= true*/,
	Bool bRecursive /*= true*/,
	const String& sFileExtension /*= String()*/) const
{
	return (MoriartyClient::Get() &&
			MoriartyClient::Get()->GetDirectoryListing(
				dirPath,
				rvResults,
				bIncludeDirectoriesInResults,
				bRecursive,
				sFileExtension));
}

/**
 * Attempt to rename from -> to.
 *
 * @return true on success, false otherwise.
 */
Bool MoriartyFileSystem::Rename(FilePath from, FilePath to)
{
	return (MoriartyClient::Get() && MoriartyClient::Get()->Rename(from, to));
}

/**
 * Attempt to update the read-only bit on the target file.
 *
 * @return true on success, false otherwise.
 */
Bool MoriartyFileSystem::SetReadOnlyBit(FilePath filePath, Bool bReadOnlyBit)
{
	return (MoriartyClient::Get() && MoriartyClient::Get()->SetReadOnlyBit(filePath, bReadOnlyBit));
}

} // namespace Seoul

#endif  // SEOUL_WITH_MORIARTY
