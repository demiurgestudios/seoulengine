/**
 * \file PatchablePackageFileSystem.h
 * \brief Wrapper around DownloadablePackageFileSystem that adds
 * handling for updating the URL that drives the system, as well
 * as specifying a read-only fallback that is always locally available.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef PATCHABLE_PACKAGE_FILE_SYSTEM_H
#define PATCHABLE_PACKAGE_FILE_SYSTEM_H

#include "PackageFileSystem.h"
namespace Seoul { class DownloadablePackageFileSystem; }
namespace Seoul { struct DownloadablePackageFileSystemSettings; }
namespace Seoul { struct DownloadablePackageFileSystemStats; }

namespace Seoul
{

/**
 * Wrapper around PackageFileSystem, allows for safe
 * reloading of the package file at runtime.
 */
class PatchablePackageFileSystem SEOUL_SEALED : public IPackageFileSystem
{
public:
	// Public access to the customized settings that a patchable
	// system uses to apply to its underlying downloadable systems.
	static void AdjustSettings(DownloadablePackageFileSystemSettings& r);

	PatchablePackageFileSystem(
		const String& sReadOnlyFallbackAbsoluteFilename,
		const String& sPackageAbsoluteFilename);
	~PatchablePackageFileSystem();

	// Retrieve stats from the internal downloadable file system, if
	// present.
	Bool GetStats(DownloadablePackageFileSystemStats& rStats) const;

	// Return the URL set to the downloadable file system, if defined.
	String GetURL() const;

	// Return if the downloadable file system has/is experiencing a write failure.
	Bool HasExperiencedWriteFailure() const;

	// Returns true if the downloadable file system has work to do.
	Bool HasWork() const;

	// Issue fetch in the downloadable system, if defined. This is a blocking
	// operation and should never be called from the main thread.
	typedef Vector<FilePath> Files;
	typedef Delegate<void (UInt64 zDownloadSizeInBytes, UInt64 zDownloadSoFarInBytes)> ProgressCallback;
	Bool Fetch(
		const Files& vInFilesToPrefetch,
		ProgressCallback progressCallback = ProgressCallback(),
		NetworkFetchPriority ePriority = NetworkFetchPriority::kDefault);

	// Blocking operation - can be expensive. Update the active URL of the
	// downloadable file system in this patchable file system. If empty, reverts
	// to the builtin file system.
	void SetURL(const String& sURL);

	// IPackageFileSystem overrides.
	virtual Atomic32Type GetActiveSyncFileCount() const SEOUL_OVERRIDE
	{
		CounterLock lock(*this);
		return m_pActive->GetActiveSyncFileCount();
	}

	virtual const String& GetAbsolutePackageFilename() const SEOUL_OVERRIDE
	{
		CounterLock lock(*this);
		return m_pActive->GetAbsolutePackageFilename();
	}

	virtual UInt32 GetBuildChangelist() const SEOUL_OVERRIDE
	{
		CounterLock lock(*this);
		return m_pActive->GetBuildChangelist();
	}

	virtual UInt32 GetPackageVariation() const SEOUL_OVERRIDE
	{
		CounterLock lock(*this);
		return m_pActive->GetPackageVariation();
	}

	virtual UInt32 GetBuildVersionMajor() const SEOUL_OVERRIDE
	{
		CounterLock lock(*this);
		return m_pActive->GetBuildVersionMajor();
	}

	virtual Bool GetFileTable(FileTable& rtFileTable) const SEOUL_OVERRIDE
	{
		CounterLock lock(*this);
		return m_pActive->GetFileTable(rtFileTable);
	}

	virtual Bool PerformCrc32Check(PackageCrc32Entries* pvInOutEntries = nullptr) SEOUL_OVERRIDE
	{
		CounterLock lock(*this);
		return m_pActive->PerformCrc32Check(pvInOutEntries);
	}

	virtual Bool HasPostCrc32() const SEOUL_OVERRIDE
	{
		CounterLock lock(*this);
		return m_pActive->HasPostCrc32();
	}

	virtual Bool IsOk() const SEOUL_OVERRIDE
	{
		CounterLock lock(*this);
		return m_pActive->IsOk();
	}

	// IFileSystem overrides.
	virtual Bool IsInitializing() const SEOUL_OVERRIDE
	{
		CounterLock lock(*this);
		return m_pActive->IsInitializing();
	}

	virtual Bool WaitForInit(UInt32 uTimeoutInMs) SEOUL_OVERRIDE
	{
		CounterLock lock(*this);
		return m_pActive->WaitForInit(uTimeoutInMs);
	}

	virtual Bool IsServicedByNetwork(FilePath filePath) const  SEOUL_OVERRIDE
	{
		CounterLock lock(*this);
		return m_pActive->IsServicedByNetwork(filePath);
	}

	virtual Bool IsServicedByNetwork(const String& sAbsoluteFilename) const SEOUL_OVERRIDE
	{
		CounterLock lock(*this);
		return m_pActive->IsServicedByNetwork(sAbsoluteFilename);
	}

	virtual Bool NetworkFetch(FilePath filePath, NetworkFetchPriority ePriority = NetworkFetchPriority::kDefault) SEOUL_OVERRIDE
	{
		CounterLock lock(*this);
		return m_pActive->NetworkFetch(filePath, ePriority);
	}

	virtual Bool NetworkPrefetch(FilePath filePath, NetworkFetchPriority ePriority = NetworkFetchPriority::kDefault) SEOUL_OVERRIDE
	{
		CounterLock lock(*this);
		return m_pActive->NetworkPrefetch(filePath, ePriority);
	}

	virtual void OnNetworkInitialize() SEOUL_OVERRIDE
	{
		CounterLock lock(*this);
		m_pActive->OnNetworkInitialize();
	}

	virtual void OnNetworkShutdown() SEOUL_OVERRIDE
	{
		CounterLock lock(*this);
		m_pActive->OnNetworkShutdown();
	}

	virtual Bool Copy(FilePath from, FilePath to, Bool bAllowOverwrite = false) SEOUL_OVERRIDE
	{
		CounterLock lock(*this);
		return m_pActive->Copy(from, to, bAllowOverwrite);
	}

	virtual Bool Copy(const String& sFrom, const String& sTo, Bool bAllowOverwrite = false) SEOUL_OVERRIDE
	{
		CounterLock lock(*this);
		return m_pActive->Copy(sFrom, sTo, bAllowOverwrite);
	}

	virtual Bool CreateDirPath(FilePath dirPath) SEOUL_OVERRIDE
	{
		CounterLock lock(*this);
		return m_pActive->CreateDirPath(dirPath);
	}

	virtual Bool CreateDirPath(const String& sDirPath) SEOUL_OVERRIDE
	{
		CounterLock lock(*this);
		return m_pActive->CreateDirPath(sDirPath);
	}

	virtual Bool DeleteDirectory(FilePath dirPath, Bool bRecursive) SEOUL_OVERRIDE
	{
		CounterLock lock(*this);
		return m_pActive->DeleteDirectory(dirPath, bRecursive);
	}

	virtual Bool DeleteDirectory(const String& sAbsoluteDirPath, Bool bRecursive) SEOUL_OVERRIDE
	{
		CounterLock lock(*this);
		return m_pActive->DeleteDirectory(sAbsoluteDirPath, bRecursive);
	}

	virtual Bool GetFileSize(
		FilePath filePath,
		UInt64& rzFileSize) const SEOUL_OVERRIDE
	{
		CounterLock lock(*this);
		return m_pActive->GetFileSize(filePath, rzFileSize);
	}

	virtual Bool GetFileSizeForPlatform(
		Platform ePlatform,
		FilePath filePath,
		UInt64& rzFileSize) const SEOUL_OVERRIDE
	{
		CounterLock lock(*this);
		return m_pActive->GetFileSizeForPlatform(ePlatform, filePath, rzFileSize);
	}

	virtual Bool GetFileSize(
		const String& sAbsoluteFilename,
		UInt64& rzFileSize) const  SEOUL_OVERRIDE
	{
		CounterLock lock(*this);
		return m_pActive->GetFileSize(sAbsoluteFilename, rzFileSize);
	}

	virtual Bool GetModifiedTime(
		FilePath filePath,
		UInt64& ruModifiedTime) const SEOUL_OVERRIDE
	{
		CounterLock lock(*this);
		return m_pActive->GetModifiedTime(filePath, ruModifiedTime);
	}

	virtual Bool GetModifiedTimeForPlatform(
		Platform ePlatform,
		FilePath filePath,
		UInt64& ruModifiedTime) const SEOUL_OVERRIDE
	{
		CounterLock lock(*this);
		return m_pActive->GetModifiedTimeForPlatform(ePlatform, filePath, ruModifiedTime);
	}

	virtual Bool GetModifiedTime(const String& sAbsoluteFilename, UInt64& ruModifiedTime) const  SEOUL_OVERRIDE
	{
		CounterLock lock(*this);
		return m_pActive->GetModifiedTime(sAbsoluteFilename, ruModifiedTime);
	}

	virtual Bool Rename(FilePath from, FilePath to) SEOUL_OVERRIDE
	{
		CounterLock lock(*this);
		return m_pActive->Rename(from, to);
	}

	virtual Bool Rename(const String& sFrom, const String& sTo) SEOUL_OVERRIDE
	{
		CounterLock lock(*this);
		return m_pActive->Rename(sFrom, sTo);
	}

	virtual Bool SetModifiedTime(FilePath filePath, UInt64 uModifiedTime) SEOUL_OVERRIDE
	{
		CounterLock lock(*this);
		return m_pActive->SetModifiedTime(filePath, uModifiedTime);
	}

	virtual Bool SetModifiedTime(const String& sAbsoluteFilename, UInt64 uModifiedTime) SEOUL_OVERRIDE
	{
		CounterLock lock(*this);
		return m_pActive->SetModifiedTime(sAbsoluteFilename, uModifiedTime);
	}

	virtual Bool SetReadOnlyBit(FilePath filePath, Bool bReadOnly) SEOUL_OVERRIDE
	{
		CounterLock lock(*this);
		return m_pActive->SetReadOnlyBit(filePath, bReadOnly);
	}

	virtual Bool SetReadOnlyBit(const String& sAbsoluteFilename, Bool bReadOnly) SEOUL_OVERRIDE
	{
		CounterLock lock(*this);
		return m_pActive->SetReadOnlyBit(sAbsoluteFilename, bReadOnly);
	}

	/**
	 * Attempt to delete filePath, return true on success.
	 */
	virtual Bool Delete(FilePath filePath) SEOUL_OVERRIDE
	{
		CounterLock lock(*this);
		return m_pActive->Delete(filePath);
	}

	/**
	 * Attempt to delete sAbsoluteFilename, return true on success.
	 */
	virtual Bool Delete(const String& sAbsoluteFilename) SEOUL_OVERRIDE
	{
		CounterLock lock(*this);
		return m_pActive->Delete(sAbsoluteFilename);
	}

	virtual Bool Exists(FilePath filePath) const SEOUL_OVERRIDE
	{
		CounterLock lock(*this);
		return m_pActive->Exists(filePath);
	}

	virtual Bool ExistsForPlatform(Platform ePlatform, FilePath filePath) const SEOUL_OVERRIDE
	{
		CounterLock lock(*this);
		return m_pActive->ExistsForPlatform(ePlatform, filePath);
	}

	virtual Bool Exists(const String& sAbsoluteFilename) const SEOUL_OVERRIDE
	{
		CounterLock lock(*this);
		return m_pActive->Exists(sAbsoluteFilename);
	}

	virtual Bool IsDirectory(FilePath filePath) const SEOUL_OVERRIDE
	{
		CounterLock lock(*this);
		return m_pActive->IsDirectory(filePath);
	}

	virtual Bool IsDirectory(const String& sAbsoluteFilename) const SEOUL_OVERRIDE
	{
		CounterLock lock(*this);
		return m_pActive->IsDirectory(sAbsoluteFilename);
	}

	virtual Bool Open(
		FilePath filePath,
		File::Mode eMode,
		ScopedPtr<SyncFile>& rpFile) SEOUL_OVERRIDE
	{
		CounterLock lock(*this);

		// TODO: This is not a good solution for the cases where it
		// will be hit (i.e. FMOD loading bits of an .bank sound back), since
		// it will load the entire file into memory for operations that will
		// almost always be on small parts of very large files.

		// Return a FullyBufferedSyncFile, to avoid dangling references
		// to the underlying PackageFileSystem.
		ScopedPtr<SyncFile> pFile;
		if (m_pActive->Open(filePath, eMode, pFile))
		{
			rpFile.Reset(SEOUL_NEW(MemoryBudgets::Io) FullyBufferedSyncFile(*pFile));
			return true;
		}

		return false;
	}

	virtual Bool OpenForPlatform(
		Platform ePlatform,
		FilePath filePath,
		File::Mode eMode,
		ScopedPtr<SyncFile>& rpFile) SEOUL_OVERRIDE
	{
		CounterLock lock(*this);

		// TODO: This is not a good solution for the cases where it
		// will be hit (i.e. FMOD loading bits of an .bank sound back), since
		// it will load the entire file into memory for operations that will
		// almost always be on small parts of very large files.

		// Return a FullyBufferedSyncFile, to avoid dangling references
		// to the underlying PackageFileSystem.
		ScopedPtr<SyncFile> pFile;
		if (m_pActive->OpenForPlatform(ePlatform, filePath, eMode, pFile))
		{
			rpFile.Reset(SEOUL_NEW(MemoryBudgets::Io) FullyBufferedSyncFile(*pFile));
			return true;
		}

		return false;
	}

	virtual Bool Open(
		const String& sAbsoluteFilename,
		File::Mode eMode,
		ScopedPtr<SyncFile>& rpFile) SEOUL_OVERRIDE
	{
		CounterLock lock(*this);

		// TODO: This is not a good solution for the cases where it
		// will be hit (i.e. FMOD loading bits of an .bank sound back), since
		// it will load the entire file into memory for operations that will
		// almost always be on small parts of very large files.

		// Return a FullyBufferedSyncFile, to avoid dangling references
		// to the underlying PackageFileSystem.
		ScopedPtr<SyncFile> pFile;
		if (m_pActive->Open(sAbsoluteFilename, eMode, pFile))
		{
			rpFile.Reset(SEOUL_NEW(MemoryBudgets::Io) FullyBufferedSyncFile(*pFile));
			return true;
		}

		return false;
	}

	virtual Bool GetDirectoryListing(
		FilePath dirPath,
		Vector<String>& rvResults,
		Bool bIncludeDirectoriesInResults = true,
		Bool bRecursive = true,
		const String& sFileExtension = String()) const SEOUL_OVERRIDE
	{
		CounterLock lock(*this);
		return m_pActive->GetDirectoryListing(dirPath, rvResults, bIncludeDirectoriesInResults, bRecursive, sFileExtension);
	}

	virtual Bool GetDirectoryListing(
		const String& sAbsoluteDirectoryPath,
		Vector<String>& rvResults,
		Bool bIncludeDirectoriesInResults = true,
		Bool bRecursive = true,
		const String& sFileExtension = String()) const SEOUL_OVERRIDE
	{
		CounterLock lock(*this);
		return m_pActive->GetDirectoryListing(sAbsoluteDirectoryPath, rvResults, bIncludeDirectoriesInResults, bRecursive, sFileExtension);
	}

	virtual Bool ReadAll(
		FilePath filePath,
		void*& rpOutputBuffer,
		UInt32& rzOutputSizeInBytes,
		UInt32 zAlignmentOfOutputBuffer,
		MemoryBudgets::Type eOutputBufferMemoryType,
		UInt32 zMaxReadSize = kDefaultMaxReadSize) SEOUL_OVERRIDE
	{
		CounterLock lock(*this);
		return m_pActive->ReadAll(
			filePath,
			rpOutputBuffer,
			rzOutputSizeInBytes,
			zAlignmentOfOutputBuffer,
			eOutputBufferMemoryType,
			zMaxReadSize);
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
		CounterLock lock(*this);
		return m_pActive->ReadAllForPlatform(
			ePlatform,
			filePath,
			rpOutputBuffer,
			rzOutputSizeInBytes,
			zAlignmentOfOutputBuffer,
			eOutputBufferMemoryType,
			zMaxReadSize);
	}

	virtual Bool ReadAll(
		const String& sAbsoluteFilename,
		void*& rpOutputBuffer,
		UInt32& rzOutputSizeInBytes,
		UInt32 zAlignmentOfOutputBuffer,
		MemoryBudgets::Type eOutputBufferMemoryType,
		UInt32 zMaxReadSize = kDefaultMaxReadSize) SEOUL_OVERRIDE
	{
		CounterLock lock(*this);
		return m_pActive->ReadAll(
			sAbsoluteFilename,
			rpOutputBuffer,
			rzOutputSizeInBytes,
			zAlignmentOfOutputBuffer,
			eOutputBufferMemoryType,
			zMaxReadSize);
	}

	virtual Bool WriteAll(
		FilePath filePath,
		void const* pInputBuffer,
		UInt32 uInputSizeInBytes,
		UInt64 uModifiedTime = 0) SEOUL_OVERRIDE
	{
		CounterLock lock(*this);
		return m_pActive->WriteAll(
			filePath,
			pInputBuffer,
			uInputSizeInBytes,
			uModifiedTime);
	}

	virtual Bool WriteAllForPlatform(
		Platform ePlatform,
		FilePath filePath,
		void const* pInputBuffer,
		UInt32 uInputSizeInBytes,
		UInt64 uModifiedTime = 0) SEOUL_OVERRIDE
	{
		CounterLock lock(*this);
		return m_pActive->WriteAllForPlatform(
			ePlatform,
			filePath,
			pInputBuffer,
			uInputSizeInBytes,
			uModifiedTime);
	}

	virtual Bool WriteAll(
		const String& sAbsoluteFilename,
		void const* pInputBuffer,
		UInt32 uInputSizeInBytes,
		UInt64 uModifiedTime = 0) SEOUL_OVERRIDE
	{
		CounterLock lock(*this);
		return m_pActive->WriteAll(
			sAbsoluteFilename,
			pInputBuffer,
			uInputSizeInBytes,
			uModifiedTime);
	}
	// /IFileSystem overrides.

private:
	String const m_sReadOnlyFallbackAbsoluteFilename;
	String const m_sAbsoluteFilename;
	ScopedPtr<PackageFileSystem> m_pFallback;
	ScopedPtr<DownloadablePackageFileSystem> m_pDownloadable;
	CheckedPtr<IPackageFileSystem> m_pActive;
	Mutex m_CounterMutex;
	Atomic32 m_Counter;

	struct CounterLock SEOUL_SEALED
	{
		CounterLock(const PatchablePackageFileSystem& r)
			: m_r(r)
		{
			Lock lock(m_r.m_CounterMutex);
			++const_cast<PatchablePackageFileSystem&>(m_r).m_Counter;
		}

		~CounterLock()
		{
			Lock lock(m_r.m_CounterMutex);
			--const_cast<PatchablePackageFileSystem&>(m_r).m_Counter;
		}

		const PatchablePackageFileSystem& m_r;
	}; // struct CounterLock

	struct JobAwareLock
	{
		JobAwareLock(const PatchablePackageFileSystem& r);
		~JobAwareLock();

		const PatchablePackageFileSystem& m_r;
	}; // struct JobAwareLock

	SEOUL_DISABLE_COPY(PatchablePackageFileSystem);
}; // class PatchablePackageFileSystem

} // namespace Seoul

#endif // include guard
