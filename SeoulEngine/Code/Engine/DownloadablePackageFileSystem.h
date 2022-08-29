/**
 * \file DownloadablePackageFileSystem.h
 * \brief PackageFileSystem wrapper, supports on-the-fly piece-by-piece
 * downloading of its contents.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef DOWNLOADABLE_PACKAGE_FILE_SYSTEM_H
#define DOWNLOADABLE_PACKAGE_FILE_SYSTEM_H

#include "Atomic32.h"
#include "Delegate.h"
#include "DownloadablePackageFileSystemStats.h"
#include "HashTable.h"
#include "SharedPtr.h"
#include "PackageFileSystem.h"
#include "SeoulSignal.h"

namespace Seoul
{

// Forward declarations
class DownloadPackageJob;
namespace HTTP { class RequestList; }
class Thread;

// Configure of a particular DownloadablePackageFileSystem.
struct DownloadablePackageFileSystemSettings SEOUL_SEALED
{
	DownloadablePackageFileSystemSettings()
		: m_sInitialURL()
		, m_sAbsolutePackageFilename()
		// These are reasonable defaults for the standard use case of
		// a DownloadablePackageFileSystem (on-demand downloading of assets).
		//
		// They trade responsiveness (smaller download chunks) for
		// per-request overhead.
		, m_uMaxRedownloadSizeThresholdInBytes(8192)
		, m_uLowerBoundMaxSizePerDownloadInBytes(32 * 1024)
		, m_uUpperBoundMaxSizePerDownloadInBytes(256 * 1024)
		, m_fTargetPerDownloadTimeInSeconds(0.5)
		, m_bNormalPriority(false)
	{
	}

	// Optional list of packages on disk used to populate
	// this archive without performing a network download.
	typedef Vector<String, MemoryBudgets::Io> PopulatePackages;
	PopulatePackages m_vPopulatePackages;

	String m_sInitialURL;
	String m_sAbsolutePackageFilename;

	// Files below this size are allowed to be "redownloaded", to allow as many contiguous
	// downloads a possible.
	UInt32 m_uMaxRedownloadSizeThresholdInBytes;

	// Lower bound download size, min size to be downloaded
	// in single operations.
	UInt32 m_uLowerBoundMaxSizePerDownloadInBytes;

	// Upper bound download size, max size that will ever be
	// downloaded in single operations.
	UInt32 m_uUpperBoundMaxSizePerDownloadInBytes;

	// Target time per download operation in seconds of download operations.
	// We trade download efficiency (more requests) for faster response
	Double m_fTargetPerDownloadTimeInSeconds;

	// If true, worker thread that handles package updates is a normal
	// priority thread. Otherwise, it is low priority.
	Bool m_bNormalPriority;
}; // struct DownloadablePackageFileSystemSettings

/**
 * DownloadablePackageFileSystem supports content "streaming" over the network,
 * downloading parts of its underlying package file-by-file on demand.
 */
class DownloadablePackageFileSystem SEOUL_SEALED : public IPackageFileSystem
{
public:
	typedef Vector<FilePath> Files;
	typedef Delegate<void (UInt64 zDownloadSizeInBytes, UInt64 zDownloadSoFarInBytes)> ProgressCallback;

	SEOUL_DELEGATE_TARGET(DownloadablePackageFileSystem);

	DownloadablePackageFileSystem(const DownloadablePackageFileSystemSettings& settings);
	~DownloadablePackageFileSystem();

	virtual void OnNetworkInitialize() SEOUL_OVERRIDE;
	virtual void OnNetworkShutdown() SEOUL_OVERRIDE;

	// Call this function to download a single file. Synchronous, blocks until the operation
	// completes or fails (failure only occurs if the system shuts down while the operation
	// is pending).
	Bool Fetch(FilePath filePath, NetworkFetchPriority ePriority = NetworkFetchPriority::kDefault);

	// Call this function to download a file set. Synchronous, blocks until the operation
	// completes or fails (failure only occurs if the system shuts down while the operation
	// is pending).
	Bool Fetch(
		const Files& vInFilesToPrefetch,
		ProgressCallback progressCallback = ProgressCallback(),
		NetworkFetchPriority ePriority = NetworkFetchPriority::kDefault);

	// Retrieve stats that track initialization and events over time of this
	// DownloadablePackageFileSystem.
	void GetStats(DownloadablePackageFileSystemStats& rStats) const
	{
		m_StatTracker.Get(rStats);
	}

	// Call this function to schedule a file for download. Asynchronous, returns immediately.
	Bool Prefetch(FilePath filePath, NetworkFetchPriority ePriority = NetworkFetchPriority::kDefault);

	// Call this function to schedule a set of files for download. Asynchronous, returns immediately.
	Bool Prefetch(const Files& vInFilesToPrefetch, NetworkFetchPriority ePriority = NetworkFetchPriority::kDefault);

	/** @return The URL used to instantiate this DownloadablePackageFileSystem. */
	const String& GetURL() const
	{
		return m_Settings.m_sInitialURL;
	}

	/** @return True if startup initialization has experienced a write failure, false otherwise. */
	Bool HasExperiencedWriteFailure() const
	{
		return m_bHasExperiencedWriteFailure;
	}

	/**
	 * @return true if there is pending asynchronous work, either in the task queue or
	 * being operated on by the worker thread.
	 */
	Bool HasWork() const
	{
		return (m_TaskQueue.HasEntries() || !m_bWorkerThreadWaiting);
	}

	/**
	 * @return true if initialization has completed, false otherwise.
	 */
	Bool IsInitialized() const
	{
		return (m_bInitializationStarted && m_bInitializationComplete);
	}

	/**
	 * @return true if initialization has completed, false otherwise.
	 */
	Bool IsInitializationStarted() const
	{
		return (m_bInitializationStarted);
	}

	/**
	 * @return true if initialization has completed, false otherwise.
	 */
	Bool IsInitializationComplete() const
	{
		return (m_bInitializationComplete);
	}

	// IPackageFileSystem overrides
	/** @return The number of active files pointing at this archive. */
	virtual Atomic32Type GetActiveSyncFileCount() const SEOUL_OVERRIDE
	{
		// If IsInitialized() returns false, it means PostEngineInitialize() has not been called,
		// so we should not be handling file operations yet.
		if (!IsInitialized())
		{
			return 0;
		}

		return m_pPackageFileSystem->GetActiveSyncFileCount();
	}

	/** Return the absolute filename of this DownloadablePackageFileSystem. */
	virtual const String& GetAbsolutePackageFilename() const SEOUL_OVERRIDE
	{
		return m_Settings.m_sAbsolutePackageFilename;
	}

	/**
	 * @return The build changelist of the currently active package.
	 */
	virtual UInt32 GetBuildChangelist() const SEOUL_OVERRIDE
	{
		// If IsInitialized() returns false, it means PostEngineInitialize() has not been called,
		// so we should not be handling file operations yet.
		if (!IsInitialized())
		{
			return 0;
		}

		return m_pPackageFileSystem->GetBuildChangelist();
	}

	/**
	 * @return The build changelist of the currently active package.
	 */
	virtual UInt32 GetPackageVariation() const SEOUL_OVERRIDE
	{
		// If IsInitialized() returns false, it means PostEngineInitialize() has not been called,
		// so we should not be handling file operations yet.
		if (!IsInitialized())
		{
			return 0;
		}

		return m_pPackageFileSystem->GetPackageVariation();
	}

	/**
	 * @return The build major version of the currently active package.
	 */
	virtual UInt32 GetBuildVersionMajor() const SEOUL_OVERRIDE
	{
		// If IsInitialized() returns false, it means PostEngineInitialize() has not been called,
		// so we should not be handling file operations yet.
		if (!IsInitialized())
		{
			return 0;
		}

		return m_pPackageFileSystem->GetBuildVersionMajor();
	}

	virtual Bool HasPostCrc32() const SEOUL_OVERRIDE
	{
		// If IsInitialized() returns false, it means PostEngineInitialize() has not been called,
		// so we should not be handling file operations yet.
		if (!IsInitialized())
		{
			return false;
		}

		return m_pPackageFileSystem->HasPostCrc32();
	}

	virtual Bool IsOk() const SEOUL_OVERRIDE
	{
		// If IsInitialized() returns false, it means PostEngineInitialize() has not been called,
		// so we should not be handling file operations yet.
		if (!IsInitialized())
		{
			return false;
		}

		return m_pPackageFileSystem->IsOk();
	}

	virtual Atomic32Type GetNetworkFileRequestsIssued() const SEOUL_OVERRIDE { return m_NetworkFileRequestsIssued; }
	virtual Atomic32Type GetNetworkFileRequestsCompleted() const SEOUL_OVERRIDE { return m_NetworkFileRequestsCompleted; }
	virtual Atomic32Type GetNetworkTimeMillisecond() const SEOUL_OVERRIDE { return m_NetworkTimeMilliseconds; }
	virtual Atomic32Type GetNetworkBytes() const SEOUL_OVERRIDE { return m_NetworkBytes; }

	virtual Bool PerformCrc32Check(PackageCrc32Entries* pvInOutEntries = nullptr) SEOUL_OVERRIDE
	{
		// If IsInitialized() returns false, it means PostEngineInitialize() has not been called,
		// so we should not be handling file operations yet.
		if (!IsInitialized())
		{
			return false;
		}

		return m_pPackageFileSystem->PerformCrc32Check(pvInOutEntries);
	}
	// /IPackageFileSystem overrides

	/**
	 * Populate rtFileTable with this PackageFileSystem's
	 * table and return true, or leave rtFileTable unmodified
	 * and return false.
	 */
	virtual Bool GetFileTable(PackageFileSystem::FileTable& rtFileTable) const SEOUL_OVERRIDE
	{
		// If IsInitialized() returns false, it means PostEngineInitialize() has not been called,
		// so we should not be handling file operations yet.
		if (!IsInitialized())
		{
			return false;
		}

		rtFileTable = m_pPackageFileSystem->GetFileTable();
		return true;
	}

	/**
	 * @return True if operations must be completed before this FileSystem is
	 * fully initialized, false otherwise.
	 */
	virtual Bool IsInitializing() const SEOUL_OVERRIDE
	{
		return !m_bDoneInitializing;
	}

	/**
	 * @return True if operations on filePath may be serviced
	 * over a (relatively high latency, low bandwidth) network connection.
	 */
	virtual Bool IsServicedByNetwork(FilePath filePath) const SEOUL_OVERRIDE
	{
		if (!IsInitialized())
		{
			return false;
		}

		if (!Exists(filePath))
		{
			return false;
		}

		return !m_tCrc32CheckTable.IsCrc32Ok(filePath);
	}

	/**
	 * If serviced by this file system and on the network, synchronously download
	 * filePath.
	 */
	virtual Bool NetworkFetch(FilePath filePath, NetworkFetchPriority ePriority = NetworkFetchPriority::kDefault) SEOUL_OVERRIDE
	{
		return Fetch(filePath, ePriority);
	}

	/**
	 * If serviced by this file system and on the network, prepare filePath
	 * for service (download it from the network).
	 */
	virtual Bool NetworkPrefetch(FilePath filePath, NetworkFetchPriority ePriority = NetworkFetchPriority::kDefault) SEOUL_OVERRIDE
	{
		return Prefetch(filePath, ePriority);
	}

	/**
	 * @return True if operations on sAbsoluteFilename may be serviced
	 * over a (relatively high latency, low bandwidth) network connection.
	 */
	virtual Bool IsServicedByNetwork(const String& sAbsoluteFilename) const SEOUL_OVERRIDE
	{
		return false;
	}

	/**
	 * @return Always false - DownloadablePackageFileSystem is not mutable.
	 */
	virtual Bool Copy(FilePath from, FilePath to, Bool bAllowOverwrite = false) SEOUL_OVERRIDE
	{
		return false;
	}

	/**
	 * @return Always false - DownloadablePackageFileSystem is not mutable.
	 */
	virtual Bool Copy(const String& sFrom, const String& sTo, Bool bAllowOverwrite = false) SEOUL_OVERRIDE
	{
		return false;
	}

	/**
	 * @return Always false - DownloadablePackageFileSystem is not mutable.
	 */
	virtual Bool CreateDirPath(FilePath dirPath) SEOUL_OVERRIDE
	{
		return false;
	}

	/**
	 * @return Always false - DownloadablePackageFileSystem is not mutable.
	 */
	virtual Bool CreateDirPath(const String& sDirPath) SEOUL_OVERRIDE
	{
		return false;
	}

	/**
	 * @return Always false - DownloadablePackageFileSystem is not mutable.
	 */
	virtual Bool DeleteDirectory(FilePath dirPath, Bool bRecursive) SEOUL_OVERRIDE
	{
		return false;
	}

	/**
	 * @return Always false - DownloadablePackageFileSystem is not mutable.
	 */
	virtual Bool DeleteDirectory(const String& sAbsoluteDirPath, Bool bRecursive) SEOUL_OVERRIDE
	{
		return false;
	}

	/**
	 * @return True if the file size of filePath is defined
	 * in this DownloadablePackageFileSystem, false otherwise. If defined,
	 * rzFileSize will contain the file size, otherwise it will be left unmodified.
	 */
	virtual Bool GetFileSize(
		FilePath filePath,
		UInt64& rzFileSize) const SEOUL_OVERRIDE
	{
		// If IsInitialized() returns false, it means PostEngineInitialize() has not been called,
		// so we should not be handling file operations yet.
		if (!IsInitialized())
		{
			return false;
		}

		return m_pPackageFileSystem->GetFileSize(filePath, rzFileSize);
	}

	/**
	 * @return True if the file size of filePath is defined
	 * in this DownloadablePackageFileSystem, false otherwise. If defined,
	 * rzFileSize will contain the file size, otherwise it will be left unmodified.
	 */
	virtual Bool GetFileSizeForPlatform(
		Platform ePlatform,
		FilePath filePath,
		UInt64& rzFileSize) const SEOUL_OVERRIDE
	{
		// If IsInitialized() returns false, it means PostEngineInitialize() has not been called,
		// so we should not be handling file operations yet.
		if (!IsInitialized())
		{
			return false;
		}

		return m_pPackageFileSystem->GetFileSizeForPlatform(ePlatform, filePath, rzFileSize);
	}

	/**
	 * @return Always false - DownloadablePackageFileSystem cannot service requests for
	 * files that cannot be tagged with a FilePath.
	 */
	virtual Bool GetFileSize(
		const String& sAbsoluteFilename,
		UInt64& rzFileSize) const  SEOUL_OVERRIDE
	{
		return false;
	}

	/**
	 * @return True if the modification time of filePath is defined
	 * in this DownloadablePackageFileSystem, false otherwise. If defined,
	 * ruModifiedTime will contain the time, otherwise it will be left unmodified.
	 */
	virtual Bool GetModifiedTime(
		FilePath filePath,
		UInt64& ruModifiedTime) const SEOUL_OVERRIDE
	{
		// If IsInitialized() returns false, it means PostEngineInitialize() has not been called,
		// so we should not be handling file operations yet.
		if (!IsInitialized())
		{
			return false;
		}

		return m_pPackageFileSystem->GetModifiedTime(filePath, ruModifiedTime);
	}

	/**
	 * @return True if the modification time of filePath is defined
	 * in this DownloadablePackageFileSystem, false otherwise. If defined,
	 * ruModifiedTime will contain the time, otherwise it will be left unmodified.
	 */
	virtual Bool GetModifiedTimeForPlatform(
		Platform ePlatform,
		FilePath filePath,
		UInt64& ruModifiedTime) const SEOUL_OVERRIDE
	{
		// If IsInitialized() returns false, it means PostEngineInitialize() has not been called,
		// so we should not be handling file operations yet.
		if (!IsInitialized())
		{
			return false;
		}

		return m_pPackageFileSystem->GetModifiedTimeForPlatform(ePlatform, filePath, ruModifiedTime);
	}

	/**
	 * @return Always false - DownloadablePackageFileSystem cannot service requests for
	 * files that cannot be tagged with a FilePath.
	 */
	virtual Bool GetModifiedTime(
		const String& sAbsoluteFilename,
		UInt64& ruModifiedTime) const  SEOUL_OVERRIDE
	{
		return false;
	}

	/**
	 * @return Always false - DownloadablePackageFileSystem is not mutable.
	 */
	virtual Bool Rename(FilePath from, FilePath to) SEOUL_OVERRIDE
	{
		return false;
	}

	/**
	 * @return Always false - DownloadablePackageFileSystem is not mutable.
	 */
	virtual Bool Rename(const String& sFrom, const String& sTo) SEOUL_OVERRIDE
	{
		return false;
	}

	/**
	 * @return Always false - DownloadablePackageFileSystem cannot update modified times.
	 */
	virtual Bool SetModifiedTime(FilePath filePath, UInt64 uModifiedTime) SEOUL_OVERRIDE
	{
		return false;
	}

	/**
	 * @return Always false - DownloadablePackageFileSystem cannot update modified times.
	 */
	virtual Bool SetModifiedTime(const String& sAbsoluteFilename, UInt64 uModifiedTime) SEOUL_OVERRIDE
	{
		return false;
	}

	/**
	 * @return Always false - DownloadablePackageFileSystem is not mutable.
	 */
	virtual Bool SetReadOnlyBit(FilePath filePath, Bool bReadOnly) SEOUL_OVERRIDE
	{
		return false;
	}

	/**
	 * @return Always false - DownloadablePackageFileSystem is not mutable.
	 */
	virtual Bool SetReadOnlyBit(const String& sAbsoluteFilename, Bool bReadOnly) SEOUL_OVERRIDE
	{
		return false;
	}

	/**
	 * Attempt to delete filePath, return true on success.
	 */
	virtual Bool Delete(FilePath filePath) SEOUL_OVERRIDE
	{
		// TODO: Support?

		// Not supported.
		return false;
	}

	/**
	 * Attempt to delete sAbsoluteFilename, return true on success.
	 */
	virtual Bool Delete(const String& sAbsoluteFilename) SEOUL_OVERRIDE
	{
		// Not supported.
		return false;
	}

	/**
	 * @return True if filePath is a file that exists in this DownloadablePackageFileSystem, false otherwise.
	 */
	virtual Bool Exists(FilePath filePath) const SEOUL_OVERRIDE
	{
		// If IsInitialized() returns false, it means PostEngineInitialize() has not been called,
		// so we should not be handling file operations yet.
		if (!IsInitialized())
		{
			return false;
		}

		return m_pPackageFileSystem->Exists(filePath);
	}

	/**
	 * @return True if filePath is a file that exists in this DownloadablePackageFileSystem, false otherwise.
	 */
	virtual Bool ExistsForPlatform(Platform ePlatform, FilePath filePath) const SEOUL_OVERRIDE
	{
		// If IsInitialized() returns false, it means PostEngineInitialize() has not been called,
		// so we should not be handling file operations yet.
		if (!IsInitialized())
		{
			return false;
		}

		return m_pPackageFileSystem->ExistsForPlatform(ePlatform, filePath);
	}

	/**
	 * @return Always false - DownloadablePackageFileSystem cannot open files which cannot be
	 * described by a FilePath.
	 */
	virtual Bool Exists(const String& sAbsoluteFilename) const SEOUL_OVERRIDE
	{
		return false;
	}

	/**
	 * @return True if filePath is a directory in this DownloadablePackageFileSystem, false otherwise.
	 */
	virtual Bool IsDirectory(FilePath filePath) const SEOUL_OVERRIDE
	{
		// If IsInitialized() returns false, it means PostEngineInitialize() has not been called,
		// so we should not be handling file operations yet.
		if (!IsInitialized())
		{
			return false;
		}

		return m_pPackageFileSystem->IsDirectory(filePath);
	}

	/**
	 * @return Always false - DownloadablePackageFileSystem cannot open files which cannot be
	 * described by a FilePath.
	 */
	virtual Bool IsDirectory(const String& sAbsoluteFilename) const SEOUL_OVERRIDE
	{
		return false;
	}

	// Attempt to open filePath with mode eMode.
	virtual Bool Open(
		FilePath filePath,
		File::Mode eMode,
		ScopedPtr<SyncFile>& rpFile) SEOUL_OVERRIDE;

	// Attempt to open filePath with mode eMode.
	virtual Bool OpenForPlatform(
		Platform ePlatform,
		FilePath filePath,
		File::Mode eMode,
		ScopedPtr<SyncFile>& rpFile) SEOUL_OVERRIDE
	{
		// If IsInitialized() returns false, it means PostEngineInitialize() has not been called,
		// so we should not be handling file operations yet.
		if (!IsInitialized())
		{
			return false;
		}

		// Platform mismatch.
		if (m_pPackageFileSystem->GetHeader().GetPlatform() != ePlatform)
		{
			return false;
		}

		return Open(filePath, eMode, rpFile);
	}

	/**
	 * @return Always false - DownloadablePackageFileSystem cannot open files which cannot be
	 * described by a FilePath.
	 */
	virtual Bool Open(
		const String& sAbsoluteFilename,
		File::Mode eMode,
		ScopedPtr<SyncFile>& rpFile) SEOUL_OVERRIDE
	{
		return false;
	}

	/**
	 * Attempt to populate rvResults with a list of files and (optionally)
	 * directories contained within the directory represented by dirPath
	 */
	virtual Bool GetDirectoryListing(
		FilePath dirPath,
		Vector<String>& rvResults,
		Bool bIncludeDirectoriesInResults = true,
		Bool bRecursive = true,
		const String& sFileExtension = String()) const SEOUL_OVERRIDE
	{
		// If IsInitialized() returns false, it means PostEngineInitialize() has not been called,
		// so we should not be handling file operations yet.
		if (!IsInitialized())
		{
			return false;
		}

		return m_pPackageFileSystem->GetDirectoryListing(
			dirPath,
			rvResults,
			bIncludeDirectoriesInResults,
			bRecursive,
			sFileExtension);
	}

	/**
	 * @return Always false - DownloadablePackageFileSystem cannot open files which cannot be
	 * described by a FilePath.
	 */
	virtual Bool GetDirectoryListing(
		const String& sAbsoluteDirectoryPath,
		Vector<String>& rvResults,
		Bool bIncludeDirectoriesInResults = true,
		Bool bRecursive = true,
		const String& sFileExtension = String()) const SEOUL_OVERRIDE
	{
		return false;
	}

	// Specialization for DownloadablePackageFileSystem, avoids the overhead
	// of caching an entire file in memory in cases where the entire file
	// will be read into memory anyway.
	virtual Bool ReadAll(
		FilePath filePath,
		void*& rpOutputBuffer,
		UInt32& rzOutputSizeInBytes,
		UInt32 zAlignmentOfOutputBuffer,
		MemoryBudgets::Type eOutputBufferMemoryType,
		UInt32 zMaxReadSize = kDefaultMaxReadSize) SEOUL_OVERRIDE;

	// Specialization for DownloadablePackageFileSystem, avoids the overhead
	// of caching an entire file in memory in cases where the entire file
	// will be read into memory anyway.
	virtual Bool ReadAllForPlatform(
		Platform ePlatform,
		FilePath filePath,
		void*& rpOutputBuffer,
		UInt32& rzOutputSizeInBytes,
		UInt32 zAlignmentOfOutputBuffer,
		MemoryBudgets::Type eOutputBufferMemoryType,
		UInt32 zMaxReadSize = kDefaultMaxReadSize) SEOUL_OVERRIDE
	{
		// If IsInitialized() returns false, it means PostEngineInitialize() has not been called,
		// so we should not be handling file operations yet.
		if (!IsInitialized())
		{
			return false;
		}

		// Platform mismatch.
		if (m_pPackageFileSystem->GetHeader().GetPlatform() != ePlatform)
		{
			return false;
		}

		return ReadAll(filePath, rpOutputBuffer, rzOutputSizeInBytes, zAlignmentOfOutputBuffer, eOutputBufferMemoryType, zMaxReadSize);
	}

	/**
	 * @return Always false - DownloadablePackageFileSystem cannot read all files which cannot be
	 * described by a FilePath.
	 */
	virtual Bool ReadAll(
		const String& sAbsoluteFilename,
		void*& rpOutputBuffer,
		UInt32& rzOutputSizeInBytes,
		UInt32 zAlignmentOfOutputBuffer,
		MemoryBudgets::Type eOutputBufferMemoryType,
		UInt32 zMaxReadSize = kDefaultMaxReadSize) SEOUL_OVERRIDE
	{
		return false;
	}

	/** Convenience - can return nullptr. */
	PackageFileSystem::FileTable const* GetFileTable() const
	{
		// If IsInitialized() returns false, it means PostEngineInitialize() has not been called,
		// so we should not be handling file operations yet.
		if (!IsInitialized())
		{
			return nullptr;
		}

		return &m_pPackageFileSystem->GetFileTable();
	}

	/** Timed out wait for initialization to complete. */
	virtual Bool WaitForInit(UInt32 uTimeoutInMs) SEOUL_OVERRIDE;

private:
	friend class DownloadPackageInitializationUtility;
	friend class DownloadablePackageSyncFile;
	friend class DownloadablePackageFileSystemHelpers;

	/** Utility structure used to track file entries on the worker thread. */
	struct FetchEntry SEOUL_SEALED
	{
		FetchEntry()
			: m_zInProgressBytesCommitted(0)
			, m_Entry()
			, m_FilePath()
			, m_ePriority(NetworkFetchPriority::kDefault)
		{
		}

		Bool operator<(const FetchEntry& entry) const
		{
			return (m_ePriority == entry.m_ePriority
				? (m_Entry.m_Entry.m_uOffsetToFile < entry.m_Entry.m_Entry.m_uOffsetToFile)
				: ((Int)m_ePriority > (Int)entry.m_ePriority));
		}

		UInt64 m_zInProgressBytesCommitted;
		PackageFileTableEntry m_Entry;
		FilePath m_FilePath;
		NetworkFetchPriority m_ePriority;
	}; // struct FetchEntry
	typedef Vector<FetchEntry, MemoryBudgets::Io> FetchEntries;
	typedef HashTable<FilePath, FetchEntry, MemoryBudgets::Io> FetchTable;

	/** Utility structure, tracks a group of fetch entries to download in one download operation. */
	struct FetchSet SEOUL_SEALED
	{
		FetchSet()
			: m_zSizeInBytes(0)
			, m_iFirstDownload(-1)
			, m_iLastDownload(-1)
			, m_ePriority(NetworkFetchPriority::kDefault)
		{
		}

		/**
		 * @return A ratio of (# of entries / size in bytes). Effectively,
		 * a large ratio indicates that this set will deliver more files, faster, than
		 * a set with a small ratio.
		 */
		Double GetRatio() const
		{
			Double fRatio = 0.0;
			if (m_zSizeInBytes > 0)
			{
				fRatio = Max(0.0, (Double)(m_iLastDownload - m_iFirstDownload + 1) / (Double)m_zSizeInBytes);
			}
			return fRatio;
		}

		UInt64 m_zSizeInBytes;
		Int32 m_iFirstDownload;
		Int32 m_iLastDownload;
		NetworkFetchPriority m_ePriority;
	}; // struct FetchSet
	typedef Vector<FetchSet, MemoryBudgets::Io> FetchSets;

	struct OrderByOffset
	{
		Bool operator()(const FetchSet& a, const FetchSet& b) const
		{
			if (a.m_ePriority != b.m_ePriority)
			{
				return (Int)a.m_ePriority > (Int)b.m_ePriority;
			}
			if (a.m_iFirstDownload != b.m_iFirstDownload)
			{
				return a.m_iFirstDownload < b.m_iFirstDownload;
			}

			return a.m_iLastDownload < b.m_iLastDownload;
		}
	};
	struct OrderByRatio
	{
		Bool operator()(const FetchSet& a, const FetchSet& b) const
		{
			return (a.m_ePriority == b.m_ePriority
				? (a.GetRatio() > b.GetRatio())
				: ((Int)a.m_ePriority > (Int)b.m_ePriority));
		}
	};

	DownloadablePackageFileSystemSettings const m_Settings;
	String m_sURL;
	ScopedPtr<HTTP::RequestList> m_pRequestList;
	ScopedPtr<Thread> m_pWorkerThread;
	Signal m_Signal;

	Atomic32 m_NetworkFileRequestsIssued;
	Atomic32 m_NetworkFileRequestsCompleted;
	Atomic32 m_NetworkTimeMilliseconds;
	Atomic32 m_NetworkBytes;

	UInt32 m_zMaxSizePerDownloadInBytes;
	ScopedPtr<PackageFileSystem> m_pPackageFileSystem;
	Atomic32Value<Bool> m_bDoneInitializing;
	Atomic32Value<Bool> m_bInitializationStarted;
	Atomic32Value<Bool> m_bInitializationComplete;
	Atomic32Value<Bool> m_bWorkerThreadRunning;
	Atomic32Value<Bool> m_bHasExperiencedWriteFailure;
	Atomic32Value<Bool> m_bWorkerThreadWaiting;

	/**
	 * Internal wrapper around the Crc32 closed hash table, with the following
	 * properties:
	 * - access is thread-safe.
	 * - the value starts out false, and can only be set to true (once a Crc32
	 *   has been validated, we assume, and the semantics of DownloadablePackageFileSystem
	 *   must enforce, that it never again becomes invalid).
	 */
	class Crc32CheckTable SEOUL_SEALED
	{
	public:
		/**
		 * Utility - (optionally) populates a query list for input with all files
		 * that are still not CRC32 ok. Updates the internal AllOk flag based
		 * on the results.
		 */
		void GetRemainingNotOk(PackageCrc32Entries& rv) const;

		/** Bulk initial population used during initialization. */
		void Initialize(const PackageCrc32Entries& v);

		Bool AllCrc32Ok() const { return (m_NotOkCount == 0); }

		Bool IsCrc32Ok(FilePath filePath) const;

		void SetCrc32Ok(FilePath filePath);

	private:
		typedef HashTable<FilePath, Bool, MemoryBudgets::Io> Table;
		Table m_tCrc32CheckTable;
		Mutex m_Mutex;
		Atomic32 m_NotOkCount;
	}; // class Crc32CheckTable
	Crc32CheckTable m_tCrc32CheckTable;

public:
	class StatTracker SEOUL_SEALED
	{
	public:
		StatTracker() = default;

		void Get(DownloadablePackageFileSystemStats& rStats) const
		{
			Lock lock(m_Mutex);
			rStats = m_Stats;
		}

		void OnEvent(HString key, UInt32 uIncrement = 1u)
		{
			Lock lock(m_Mutex);
			auto p = m_Stats.m_tEvents.Find(key);
			if (nullptr == p)
			{
				SEOUL_VERIFY(m_Stats.m_tEvents.Insert(key, uIncrement).Second);
			}
			else
			{
				(*p) += uIncrement;
			}
		}

		void OnDeltaTime(HString key, Int64 iDeltaTimeInTicks)
		{
			Lock lock(m_Mutex);
			auto p = m_Stats.m_tTimes.Find(key);
			if (nullptr == p)
			{
				SEOUL_VERIFY(m_Stats.m_tTimes.Insert(key, iDeltaTimeInTicks).Second);
			}
			else
			{
				(*p) += iDeltaTimeInTicks;
			}
		}

	private:
		SEOUL_DISABLE_COPY(StatTracker);

		Mutex m_Mutex;
		DownloadablePackageFileSystemStats m_Stats;
	}; // class StatTracker
	StatTracker m_StatTracker;

private:
	class TaskQueue SEOUL_SEALED
	{
	public:
		struct FetchEntry SEOUL_SEALED
		{
			FetchEntry()
				: m_FilePath()
				, m_ePriority(NetworkFetchPriority::kDefault)
			{
			}

			FilePath m_FilePath;
			NetworkFetchPriority m_ePriority;
		}; // struct FetchEntry
		typedef Vector<FetchEntry, MemoryBudgets::Io> FetchList;

		typedef HashTable<FilePath, NetworkFetchPriority, MemoryBudgets::Io> Table;

		TaskQueue();
		~TaskQueue();

		void OnNetworkInitialize();
		void OnNetworkShutdown();

		void Fetch(FilePath filePath, NetworkFetchPriority ePriority);
		void Fetch(const Files& vFiles, NetworkFetchPriority ePriority);
		Bool HasEntries() const;
		void PopAll(
			FetchList& rvFetchList);

	private:
		Table m_tFetchTable;
		Mutex m_Mutex;

		SEOUL_DISABLE_COPY(TaskQueue);
	}; // class TaskQueue
	TaskQueue m_TaskQueue;

	// Checks if all vFiles have been fetched (downloaded and have valid Crc32 codes).
	// This method assumes that Exists() returns true for all files in vFiles.
	void InternalBuildFetchSets(const FetchEntries& vFetchEntries, FetchSets& rvFetchSets);
	void InternalPerformFetch(
		const PackageCrc32Entries& vEntriesByFileOrder,
		FetchEntries& rvFetchEntries);
	void InternalPerformPopulateFrom(
		void*& pBuffer,
		UInt32& uBuffer,
		const String& sAbsolutePathToPackageFile,
		Bool bDeleteAfterPopulate);
	Bool InternalPrefetch(Files& rvFiles, NetworkFetchPriority ePriority);
	Bool InternalAreFilesFetched(const Files& vFiles) const;
	void InternalPruneFilesThatDoNotExist(Files& rvFiles) const;
	void InternalUpdateMaxSizePerDownloadInBytes(
		Int64 iDownloadStartTimeInTicks,
		Int64 iDownloadEndTimeInTicks,
		UInt64 zDownloadSizeInBytes);
	Int InternalWorkerThread(const Thread&);
	void InternalWorkerThreadInitialize(PackageCrc32Entries& rvAllEntries);

	SEOUL_DISABLE_COPY(DownloadablePackageFileSystem);
}; // class DownloadablePackageFileSystem

} // namespace Seoul

#endif // include guard
