/**
 * \file CachingDiskFileSystem.h
 * \brief Specialization of DiskFileSystem that caches
 * size and modification time information in-memory.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef CACHING_DISK_FILE_SYSTEM_H
#define CACHING_DISK_FILE_SYSTEM_H

#include "Atomic32.h"
#include "DiskFileSystem.h"
#include "FileChangeNotifier.h"
#include "IFileSystem.h"
#include "HashSet.h"
#include "HashTable.h"
#include "Mutex.h"
#include "ScopedPtr.h"
namespace Seoul { namespace Directory { struct DirEntryEx; } }

namespace Seoul
{

/**
 * Specialization of DiskFileSystem that caches
 * size and modification time information in-memory.
 *
 * CachingDiskFileSystem must be specified on a Seoul Engine
 * GameDirectory and Platform. It also has (relatively) high
 * startup time, as it performs a directory enumeration at startup
 * to pre-populate its size/mod time caches. It should be used in
 * situations where that bulk operation will be a win (relatively
 * to querying the size/mod time lazily).
 *
 * Dirtying of the cache is achieved through overloads of mutation
 * methods as well as a monitoring file notifier (to catch changes
 * outside the CachingDiskFileSystem's view).
 */
class CachingDiskFileSystem : public IFileSystem
{
public:
	SEOUL_DELEGATE_TARGET(CachingDiskFileSystem);

	CachingDiskFileSystem(Platform ePlatform, GameDirectory eDirectory);
	virtual ~CachingDiskFileSystem();

	// IFileSystem overrides.
	virtual Bool Copy(FilePath from, FilePath to, Bool bAllowOverwrite = false) SEOUL_OVERRIDE { return ImplCopy(from, to, bAllowOverwrite); }
	virtual Bool Copy(const String& sAbsoluteFrom, const String& sAbsoluteTo, Bool bAllowOverwrite = false) SEOUL_OVERRIDE { return ImplCopy(sAbsoluteFrom, sAbsoluteTo, bAllowOverwrite); }

	virtual Bool CreateDirPath(FilePath dirPath) SEOUL_OVERRIDE { return ImplCreateDirPath(dirPath); }
	virtual Bool CreateDirPath(const String& sAbsoluteDir) SEOUL_OVERRIDE
	{
		auto const dirPath = ToFilePath(sAbsoluteDir);
		if (!dirPath.IsValid())
		{
			return false;
		}

		return ImplCreateDirPath(dirPath);
	}

	virtual Bool DeleteDirectory(FilePath dirPath, Bool bRecursive) SEOUL_OVERRIDE { return ImplDeleteDirectory(dirPath, bRecursive); }
	virtual Bool DeleteDirectory(const String& sAbsoluteDirPath, Bool bRecursive) SEOUL_OVERRIDE
	{
		auto const dirPath = ToFilePath(sAbsoluteDirPath);
		if (!dirPath.IsValid())
		{
			return false;
		}

		return ImplDeleteDirectory(dirPath, bRecursive);
	}

	virtual Bool Delete(FilePath filePath) SEOUL_OVERRIDE { return ImplDelete(filePath); }
	virtual Bool Delete(const String& sAbsoluteFilename) SEOUL_OVERRIDE
	{
		auto const filePath = ToFilePath(sAbsoluteFilename);
		if (!filePath.IsValid())
		{
			return false;
		}

		return ImplDelete(filePath);
	}

	virtual Bool Exists(FilePath filePath) const SEOUL_OVERRIDE { return ImplExists(filePath); }
	virtual Bool Exists(const String& sAbsoluteFilename) const SEOUL_OVERRIDE
	{
		auto const filePath = ToFilePath(sAbsoluteFilename);
		if (!filePath.IsValid())
		{
			return false;
		}

		return ImplExists(filePath);
	}
	virtual Bool ExistsForPlatform(Platform ePlatform, FilePath filePath) const SEOUL_OVERRIDE
	{
		if (m_ePlatform != ePlatform)
		{
			return false;
		}

		return ImplExists(filePath);
	}
	virtual Bool ExistsInSource(FilePath filePath) const SEOUL_OVERRIDE
	{
		// False by default - will be overriden in SourceCachingDiskFileSystem.
		return false;
	}

	virtual Bool GetDirectoryListing(
		FilePath filePath,
		Vector<String>& rvResults,
		Bool bIncludeDirectoriesInResults = true,
		Bool bRecursive = true,
		const String& sFileExtension = String()) const SEOUL_OVERRIDE { return ImplGetDirectoryListing(filePath, rvResults, bIncludeDirectoriesInResults, bRecursive, sFileExtension); }
	virtual Bool GetDirectoryListing(
		const String& sAbsoluteDirectoryPath,
		Vector<String>& rvResults,
		Bool bIncludeDirectoriesInResults = true,
		Bool bRecursive = true,
		const String& sFileExtension = String()) const SEOUL_OVERRIDE
	{
		auto const dirPath = ToFilePath(sAbsoluteDirectoryPath);
		if (!dirPath.IsValid())
		{
			return false;
		}

		return ImplGetDirectoryListing(dirPath, rvResults, bIncludeDirectoriesInResults, bRecursive, sFileExtension);
	}

	virtual Bool GetFileSize(FilePath filePath, UInt64& rzFileSize) const SEOUL_OVERRIDE { return ImplGetFileSize(filePath, rzFileSize); }
	virtual Bool GetFileSize(const String& sAbsoluteFilename, UInt64& rzFileSize) const SEOUL_OVERRIDE
	{
		auto const filePath = ToFilePath(sAbsoluteFilename);
		if (!filePath.IsValid())
		{
			return false;
		}

		return ImplGetFileSize(filePath, rzFileSize);
	}
	virtual Bool GetFileSizeForPlatform(Platform ePlatform, FilePath filePath, UInt64& rzFileSize) const SEOUL_OVERRIDE
	{
		if (m_ePlatform != ePlatform)
		{
			return false;
		}

		return ImplGetFileSize(filePath, rzFileSize);
	}

	virtual Bool GetModifiedTime(FilePath filePath, UInt64& ruModifiedTime) const SEOUL_OVERRIDE { return ImplGetModifiedTime(filePath, ruModifiedTime); }
	virtual Bool GetModifiedTime(const String& sAbsoluteFilename, UInt64& ruModifiedTime) const SEOUL_OVERRIDE
	{
		auto const filePath = ToFilePath(sAbsoluteFilename);
		if (!filePath.IsValid())
		{
			return false;
		}

		return ImplGetModifiedTime(filePath, ruModifiedTime);
	}
	virtual Bool GetModifiedTimeForPlatform(Platform ePlatform, FilePath filePath, UInt64& ruModifiedTime) const SEOUL_OVERRIDE
	{
		if (m_ePlatform != ePlatform)
		{
			return false;
		}

		return ImplGetModifiedTime(filePath, ruModifiedTime);
	}
	virtual Bool GetModifiedTimeInSource(FilePath filePath, UInt64& ruModifiedTime) const SEOUL_OVERRIDE
	{
		// False by default - will be overriden in SourceCachingDiskFileSystem.
		return false;
	}

	virtual Bool IsDirectory(FilePath filePath) const SEOUL_OVERRIDE { return ImplIsDirectory(filePath); }
	virtual Bool IsDirectory(const String& sAbsoluteFilename) const SEOUL_OVERRIDE
	{
		auto const filePath = ToFilePath(sAbsoluteFilename);
		if (!filePath.IsValid())
		{
			return false;
		}

		return ImplIsDirectory(filePath);
	}

	virtual Bool Open(FilePath filePath, File::Mode eMode, ScopedPtr<SyncFile>& rpFile) SEOUL_OVERRIDE { return ImplOpen(filePath, eMode, rpFile); }
	virtual Bool Open(const String& sAbsoluteFilename, File::Mode eMode, ScopedPtr<SyncFile>& rpFile) SEOUL_OVERRIDE
	{
		auto const filePath = ToFilePath(sAbsoluteFilename);
		if (!filePath.IsValid())
		{
			return false;
		}

		return ImplOpen(filePath, eMode, rpFile);
	}
	virtual Bool OpenForPlatform(Platform ePlatform, FilePath filePath, File::Mode eMode, ScopedPtr<SyncFile>& rpFile) SEOUL_OVERRIDE
	{
		if (m_ePlatform != ePlatform)
		{
			return false;
		}

		return ImplOpen(filePath, eMode, rpFile);
	}

	virtual Bool Rename(FilePath from, FilePath to) SEOUL_OVERRIDE { return ImplRename(from, to); }
	virtual Bool Rename(const String& sAbsoluteFrom, const String& sAbsoluteTo) SEOUL_OVERRIDE { return ImplRename(sAbsoluteFrom, sAbsoluteTo); }

	virtual Bool ReadAll(
		FilePath filePath,
		void*& rpOutputBuffer,
		UInt32& rzOutputSizeInBytes,
		UInt32 zAlignmentOfOutputBuffer,
		MemoryBudgets::Type eOutputBufferMemoryType,
		UInt32 zMaxReadSize = kDefaultMaxReadSize) SEOUL_OVERRIDE { return ImplReadAll(filePath, rpOutputBuffer, rzOutputSizeInBytes, zAlignmentOfOutputBuffer, eOutputBufferMemoryType, zMaxReadSize); }
	virtual Bool ReadAll(
		const String& sAbsoluteFilename,
		void*& rpOutputBuffer,
		UInt32& rzOutputSizeInBytes,
		UInt32 zAlignmentOfOutputBuffer,
		MemoryBudgets::Type eOutputBufferMemoryType,
		UInt32 zMaxReadSize = kDefaultMaxReadSize) SEOUL_OVERRIDE
	{
		auto const filePath = ToFilePath(sAbsoluteFilename);
		if (!filePath.IsValid())
		{
			return false;
		}

		return ImplReadAll(filePath, rpOutputBuffer, rzOutputSizeInBytes, zAlignmentOfOutputBuffer, eOutputBufferMemoryType, zMaxReadSize);
	}
	virtual Bool ReadAllForPlatform(
		Platform ePlatform,
		FilePath filePath,
		void*& rpOutputBuffer,
		UInt32& rzOutputSizeInBytes,
		UInt32 zAlignmentOfOutputBuffer,
		MemoryBudgets::Type eOutputBufferMemoryType,
		UInt32 zMaxReadSize = kDefaultMaxReadSize) SEOUL_OVERRIDE
	{
		if (m_ePlatform != ePlatform)
		{
			return false;
		}

		return ImplReadAll(filePath, rpOutputBuffer, rzOutputSizeInBytes, zAlignmentOfOutputBuffer, eOutputBufferMemoryType, zMaxReadSize);
	}

	virtual Bool SetModifiedTime(FilePath filePath, UInt64 uModifiedTime) SEOUL_OVERRIDE { return ImplSetModifiedTime(filePath, uModifiedTime); }
	virtual Bool SetModifiedTime(const String& sAbsoluteFilename, UInt64 uModifiedTime) SEOUL_OVERRIDE
	{
		auto const filePath = ToFilePath(sAbsoluteFilename);
		if (!filePath.IsValid())
		{
			return false;
		}

		return ImplSetModifiedTime(filePath, uModifiedTime);
	}
	virtual Bool SetModifiedTimeForPlatform(Platform ePlatform, FilePath filePath, UInt64 uModifiedTime) SEOUL_OVERRIDE
	{
		if (m_ePlatform != ePlatform)
		{
			return false;
		}

		return ImplSetModifiedTime(filePath, uModifiedTime);
	}

	virtual Bool SetReadOnlyBit(FilePath filePath, Bool bReadOnly) SEOUL_OVERRIDE { return ImplSetReadOnlyBit(filePath, bReadOnly); }
	virtual Bool SetReadOnlyBit(const String& sAbsoluteFilename, Bool bReadOnly) SEOUL_OVERRIDE
	{
		auto const filePath = ToFilePath(sAbsoluteFilename);
		if (!filePath.IsValid())
		{
			return false;
		}

		return ImplSetReadOnlyBit(filePath, bReadOnly);
	}

	virtual Bool WriteAll(
		FilePath filePath,
		void const* pInputBuffer,
		UInt32 uInputSizeInBytes,
		UInt64 uModifiedTime = 0u) SEOUL_OVERRIDE { return ImplWriteAll(filePath, pInputBuffer, uInputSizeInBytes, uModifiedTime); }
	virtual Bool WriteAll(
		const String& sAbsoluteFilename,
		void const* pInputBuffer,
		UInt32 uInputSizeInBytes,
		UInt64 uModifiedTime = 0u) SEOUL_OVERRIDE
	{
		auto const filePath = ToFilePath(sAbsoluteFilename);
		if (!filePath.IsValid())
		{
			return false;
		}

		return ImplWriteAll(filePath, pInputBuffer, uInputSizeInBytes, uModifiedTime);
	}
	virtual Bool WriteAllForPlatform(
		Platform ePlatform,
		FilePath filePath,
		void const* pInputBuffer,
		UInt32 uInputSizeInBytes,
		UInt64 uModifiedTime = 0u) SEOUL_OVERRIDE
	{
		if (m_ePlatform != ePlatform)
		{
			return false;
		}

		return ImplWriteAll(filePath, pInputBuffer, uInputSizeInBytes, uModifiedTime);
	}

	/** Query the number of file notification events this caching system has received. */
	Atomic32Type GetOnFileChangesCount() const
	{
		return m_OnFileChangesCount;
	}

	// Custom version of FilePath::CreateFilePath() that is less flexible/more strict,
	// to avoid (e.g.) content/source mismatch.
	FilePath ToFilePath(const String& sAbsoluteFilename) const;

protected:
	Bool ImplCopy(FilePath from, FilePath to, Bool bAllowOverwrite);
	Bool ImplCopy(const String& sAbsoluteFrom, const String& sAbsoluteTo, Bool bAllowOverwrite);
	Bool ImplCreateDirPath(FilePath dirPath);
	Bool ImplDeleteDirectory(FilePath dirPath, Bool bRecursive);
	Bool ImplDelete(FilePath filePath);
	Bool ImplExists(FilePath filePath) const;
	Bool ImplGetDirectoryListing(FilePath dirPath, Vector<String>& rvResults, Bool bIncludeDirectoriesInResults, Bool bRecursive, const String& sFileExtension) const;
	Bool ImplGetFileSize(FilePath filePath, UInt64& rzFileSize) const;
	Bool ImplGetModifiedTime(FilePath filePath, UInt64& ruModifiedTime) const;
	Bool ImplIsDirectory(FilePath dirPath) const;
	Bool ImplOpen(FilePath filePath, File::Mode eMode, ScopedPtr<SyncFile>& rpFile);
	Bool ImplReadAll(FilePath filePath, void*& rpOutputBuffer, UInt32& rzOutputSizeInBytes, UInt32 zAlignmentOfOutputBuffer, MemoryBudgets::Type eOutputBufferMemoryType, UInt32 zMaxReadSize);
	Bool ImplRename(FilePath from, FilePath to);
	Bool ImplRename(const String& sAbsoluteFrom, const String& sAbsoluteTo);
	Bool ImplSetModifiedTime(FilePath filePath, UInt64 uModifiedTime);
	Bool ImplSetReadOnlyBit(FilePath filePath, Bool bReadOnly);
	Bool ImplWriteAll(FilePath filePath, void const* pInputBuffer, UInt32 uInputSizeInBytes, UInt64 uModifiedTime);

	CachingDiskFileSystem(Platform ePlatform, GameDirectory eDirectory, Bool bSource);

private:
	SEOUL_DISABLE_COPY(CachingDiskFileSystem);

	void Construct();
	void OnFileChange(const String& sOldPath, const String& sNewPath, FileChangeNotifier::FileEvent eEvent);
	const String& GetRootDirectory() const;

	Platform const m_ePlatform;
	GameDirectory const m_eDirectory;
	Bool const m_bSource;

	class Disk SEOUL_SEALED
	{
	public:
		Disk(Platform ePlatform, GameDirectory eDirectory, Bool bSource)
			: m_ePlatform(ePlatform)
			, m_eDirectory(eDirectory)
			, m_bSource(bSource)
		{
		}

		Bool Copy(FilePath from, FilePath to, Bool bAllowOverwrite = false);
		Bool Copy(const String& sAbsoluteFrom, const String& sAbsoluteTo, Bool bAllowOverwrite = false);
		Bool CreateDirPath(FilePath dirPath);
		Bool Delete(FilePath filePath);
		Bool DeleteDirectory(FilePath dirPath, Bool bRecursive);
		Bool Exists(FilePath filePath) const;
		Bool IsDirectory(FilePath filePath) const;
		Bool GetDirectoryListing(FilePath dirPath, Vector<String>& rvResults, Bool bIncludeDirectoriesInResults = true, Bool bRecursive = true, const String& sFileExtension = String()) const;
		Bool GetFileSize(FilePath filePath, UInt64& ru) const;
		UInt64 GetFileSize(FilePath filePath) const;
		Bool GetModifiedTime(FilePath filePath, UInt64& ru) const;
		UInt64 GetModifiedTime(FilePath filePath) const;
		Bool Open(FilePath filePath, File::Mode eMode, ScopedPtr<SyncFile>& rpFile);
		Bool ReadAll(FilePath filePath, void*& rpOutputBuffer, UInt32& rzOutputSizeInBytes, UInt32 zAlignmentOfOutputBuffer, MemoryBudgets::Type eOutputBufferMemoryType, UInt32 zMaxReadSize = kDefaultMaxReadSize);
		Bool Rename(FilePath from, FilePath to);
		Bool Rename(const String& sAbsoluteFrom, const String& sAbsoluteTo);
		Bool SetModifiedTime(FilePath filePath, UInt64 uModifiedTime);
		Bool SetReadOnlyBit(FilePath filePath, Bool bReadOnly);
		Bool WriteAll(FilePath filePath, void const* pIn, UInt32 uSizeInBytes, UInt64 uModifiedTime = 0);

		String ToFilename(FilePath filePath) const;

	private:
		SEOUL_DISABLE_COPY(Disk);

		Platform const m_ePlatform;
		GameDirectory const m_eDirectory;
		Bool const m_bSource;
		DiskFileSystem m_Internal;
	} m_Disk;

	ScopedPtr<FileChangeNotifier> m_pNotifier;
	typedef HashSet<FilePath, MemoryBudgets::Io> Dirty;
	Mutex m_DirtyMutex;
	Dirty m_Dirty;
	Atomic32 m_OnFileChangesCount;

	Mutex m_Mutex;
	friend class CachingDiskSyncFile;
	typedef HashTable<FilePath, UInt64, MemoryBudgets::Io> Lookup;
	Lookup m_tModTimes;
	Lookup m_tSizes;

	Bool InsideLockAddToCache(Directory::DirEntryEx& rEntry);
	void InsideLockCheckDirty(FilePath filePath);
	void InsideLockCheckDirtyDir(Byte const* sRelDir);
	void InsideLockPopulateCaches();
}; // class CachingDiskFileSystem

class SourceCachingDiskFileSystem SEOUL_SEALED : public CachingDiskFileSystem
{
public:
	SEOUL_DELEGATE_TARGET(SourceCachingDiskFileSystem);

	SourceCachingDiskFileSystem(Platform ePlatform);
	~SourceCachingDiskFileSystem();

	// CachingDiskFileSystem overrides - FilePath methods are never handled directly,
	// we only respond to source files, which arrive as absolute String paths. Otherwise,
	// CachingDiskFileSystem has the necessary logic when constructed with bSource = true.
	virtual Bool Copy(FilePath from, FilePath to, Bool bAllowOverwrite = false) SEOUL_OVERRIDE { return false; }
	virtual Bool CreateDirPath(FilePath dirPath) SEOUL_OVERRIDE { return false; }
	virtual Bool Delete(FilePath filePath) SEOUL_OVERRIDE { return false; }
	virtual Bool DeleteDirectory(FilePath dirPath, Bool bRecursive) SEOUL_OVERRIDE { return false; }
	virtual Bool Exists(FilePath filePath) const SEOUL_OVERRIDE { return false; }
	virtual Bool ExistsForPlatform(Platform ePlatform, FilePath filePath) const SEOUL_OVERRIDE { return false; }
	virtual Bool GetDirectoryListing(FilePath filePath, Vector<String>& rvResults, Bool bIncludeDirectoriesInResults = true, Bool bRecursive = true, const String& sFileExtension = String()) const SEOUL_OVERRIDE { return false; }
	virtual Bool GetFileSize(FilePath filePath, UInt64& rzFileSize) const SEOUL_OVERRIDE { return false; }
	virtual Bool GetFileSizeForPlatform(Platform ePlatform, FilePath filePath, UInt64& rzFileSize) const SEOUL_OVERRIDE { return false; }
	virtual Bool GetModifiedTime(FilePath filePath, UInt64& ruModifiedTime) const SEOUL_OVERRIDE { return false; }
	virtual Bool GetModifiedTimeForPlatform(Platform ePlatform, FilePath filePath, UInt64& ruModifiedTime) const SEOUL_OVERRIDE { return false; }
	virtual Bool IsDirectory(FilePath filePath) const SEOUL_OVERRIDE { return false; }
	virtual Bool Open(FilePath filePath, File::Mode eMode, ScopedPtr<SyncFile>& rpFile) SEOUL_OVERRIDE { return false; }
	virtual Bool OpenForPlatform(Platform ePlatform, FilePath filePath, File::Mode eMode, ScopedPtr<SyncFile>& rpFile) SEOUL_OVERRIDE { return false; }
	virtual Bool Rename(FilePath from, FilePath to) SEOUL_OVERRIDE { return false; }
	virtual Bool ReadAll(FilePath filePath, void*& rpOutputBuffer, UInt32& rzOutputSizeInBytes, UInt32 zAlignmentOfOutputBuffer, MemoryBudgets::Type eOutputBufferMemoryType, UInt32 zMaxReadSize = kDefaultMaxReadSize) SEOUL_OVERRIDE { return false; }
	virtual Bool ReadAllForPlatform(Platform ePlatform, FilePath filePath, void*& rpOutputBuffer, UInt32& rzOutputSizeInBytes, UInt32 zAlignmentOfOutputBuffer, MemoryBudgets::Type eOutputBufferMemoryType, UInt32 zMaxReadSize = kDefaultMaxReadSize) SEOUL_OVERRIDE { return false; }
	virtual Bool SetModifiedTime(FilePath filePath, UInt64 uModifiedTime) SEOUL_OVERRIDE { return false; }
	virtual Bool SetModifiedTimeForPlatform(Platform ePlatform, FilePath filePath, UInt64 uModifiedTime) SEOUL_OVERRIDE { return false; }
	virtual Bool SetReadOnlyBit(FilePath filePath, Bool bReadOnly) SEOUL_OVERRIDE { return false; }
	virtual Bool WriteAll(FilePath filePath, void const* pInputBuffer, UInt32 uInputSizeInBytes, UInt64 uModifiedTime = 0u) SEOUL_OVERRIDE { return false; }
	virtual Bool WriteAllForPlatform(Platform ePlatform, FilePath filePath, void const* pInputBuffer, UInt32 uInputSizeInBytes, UInt64 uModifiedTime = 0u) SEOUL_OVERRIDE { return false; }
	// /CachingDiskFileSystem overrides

	/**
	 * @return true if filePath exists in Source/, false otherwise.
	 */
	virtual Bool ExistsInSource(FilePath filePath) const SEOUL_OVERRIDE
	{
		// TODO: Need to generalize this sort of pattern - texture files are the
		// only situations where this currently applies (a one-to-one mapping maps from
		// a single source file to multiple cooked files).
		//
		// Normalize.
		if (IsTextureFileType(filePath.GetType()))
		{
			filePath.SetType(FileType::kTexture0);
		}

		return ImplExists(filePath);
	}

	/**
	 * @return true if filePath exists in Source/ and has a valid modification time.
	 */
	virtual Bool GetModifiedTimeInSource(FilePath filePath, UInt64& ruModifiedTime) const SEOUL_OVERRIDE
	{
		// TODO: Need to generalize this sort of pattern - texture files are the
		// only situations where this currently applies (a one-to-one mapping maps from
		// a single source file to multiple cooked files).
		//
		// Normalize.
		if (IsTextureFileType(filePath.GetType()))
		{
			filePath.SetType(FileType::kTexture0);
		}

		return ImplGetModifiedTime(filePath, ruModifiedTime);
	}

private:
	SEOUL_DISABLE_COPY(SourceCachingDiskFileSystem);
}; // class SourceCachingDiskFileSystem

} // namespace Seoul

#endif // include guard
