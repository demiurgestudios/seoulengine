/**
 * \file FileManager.h
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

#pragma once
#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include "Atomic32.h"
#include "HashSet.h"
#include "HashTable.h"
#include "IFileSystem.h"
#include "Mutex.h"
#include "Singleton.h"
#include "Vector.h"
namespace Seoul { class FileManagerRemap; }

namespace Seoul
{

// Global hook for initializing file systems - if defined,
// FileManager will call this pointer to set up file systems on
// initializations. This allows the current program to hook up
// file systems as desired at the earliest possible point, so all
// file operations are routed through desired file systems.
typedef void (*InitializeFileSystemsCallback)();

extern InitializeFileSystemsCallback g_pInitializeFileSystemsCallback;

/**
 * Singleton manager for abstracting operations on files.
 */
class FileManager SEOUL_SEALED : public Singleton<FileManager>
{
public:
	typedef Vector<IFileSystem*, MemoryBudgets::Io> FileSystemStack;
	typedef HashSet<IFileSystem*, MemoryBudgets::Io> RefOnly;

	// Initialize and shutdown the global FileManager object. Must be
	// called once at program startup and once at program shutdown.
	static void Initialize();
	static void ShutDown();

	// Apply a new configureation to the FileManager's remap table.
	typedef HashTable<FilePath, FilePath, MemoryBudgets::Io> RemapTable;
	void ConfigureRemap(const RemapTable& tRemap, UInt32 uHash);
	UInt32 GetRemapHash() const;

	/**
	 * Adds a new FileSystem to the stack of FileSystems owned
	 * by this FileManager.
	 *
	 * File requests are resolved in LIFO order with regards
	 * to the order that FileSystems are registered.
	 */
	template <typename T>
	T* RegisterFileSystem()
	{
		m_vFileSystemStack.PushBack(SEOUL_NEW(MemoryBudgets::Io) T);
		return (T*)m_vFileSystemStack.Back();
	}

	/**
	 * Adds a new FileSystem to the stack of FileSystems owned
	 * by this FileManager.
	 *
	 * File requests are resolved in LIFO order with regards
	 * to the order that FileSystems are registered.
	 */
	template <typename T, typename A1>
	T* RegisterFileSystem(const A1& a1)
	{
		m_vFileSystemStack.PushBack(SEOUL_NEW(MemoryBudgets::Io) T(a1));
		return (T*)m_vFileSystemStack.Back();
	}

	/**
	 * Adds a new FileSystem to the stack of FileSystems owned
	 * by this FileManager.
	 *
	 * File requests are resolved in LIFO order with regards
	 * to the order that FileSystems are registered.
	 */
	template <typename T, typename A1, typename A2>
	T* RegisterFileSystem(const A1& a1, const A2& a2)
	{
		m_vFileSystemStack.PushBack(SEOUL_NEW(MemoryBudgets::Io) T(a1, a2));
		return (T*)m_vFileSystemStack.Back();
	}

	/**
	 * Adds a new FileSystem to the stack of FileSystems owned
	 * by this FileManager.
	 *
	 * File requests are resolved in LIFO order with regards
	 * to the order that FileSystems are registered.
	 */
	template <typename T, typename A1, typename A2, typename A3>
	T* RegisterFileSystem(const A1& a1, const A2& a2, const A3& a3)
	{
		m_vFileSystemStack.PushBack(SEOUL_NEW(MemoryBudgets::Io) T(a1, a2, a3));
		return (T*)m_vFileSystemStack.Back();
	}

	/**
	 * Adds a new FileSystem to the stack of FileSystems owned
	 * by this FileManager.
	 *
	 * File requests are resolved in LIFO order with regards
	 * to the order that FileSystems are registered.
	 */
	template <typename T, typename A1, typename A2, typename A3, typename A4>
	T* RegisterFileSystem(const A1& a1, const A2& a2, const A3& a3, const A4& a4)
	{
		m_vFileSystemStack.PushBack(SEOUL_NEW(MemoryBudgets::Io) T(a1, a2, a3, a4));
		return (T*)m_vFileSystemStack.Back();
	}

	/**
	 * Adds a FileSystem that is not owned by the FileManager.
	 *
	 * It is the responsibility of exsternal code to maintain
	 * the lifespan of this IFileSystem so that it is greater
	 * than the FileManager and to deallocated any memory
	 * associated with it on shutdown.
	 */
	template <typename T>
	T* RegisterRefOnlyFileSystem(T* p)
	{
		m_vFileSystemStack.PushBack(p);
		m_RefOnly.Insert(p);
		return p;
	}

	/**
	 * Adds a new FileSystem to the stack of FileSystems owned
	 * by this FileManager.
	 *
	 * FileSystem will exist already, and FileSystem will take
	 * control of the system and its associated heap allocated
	 * memory.
	 */
	template <typename T>
	T* TakeOwnershipOfFileSystem(T* p)
	{
		m_vFileSystemStack.PushBack(p);
		return p;
	}

	// Attemp to copy from -> to.
	Bool Copy(FilePath from, FilePath to, Bool bAllowOverwrite = false) const;
	Bool Copy(const String& sAbsoluteFrom, const String& sAbsoluteTo, Bool bAllowOverwrite = false) const;

	// Try to create the directory. If necessary,
	// will also attempt to create all parent directories that do
	// not exist.
	Bool CreateDirPath(FilePath dirPath) const;
	Bool CreateDirPath(const String& sAbsoluteDir) const;

	// Attempts to delete a file from the first virtual file system
	// that contains it. Returns true on a successful deletion,
	// false otherwise.
	Bool Delete(FilePath filePath) const;
	Bool Delete(const String& sAbsoluteFilename) const;

	// Attempts to delete a directory from the first virtual file system
	// that contains it. Returns true on a successful deletion,
	// false otherwise.
	Bool DeleteDirectory(FilePath dirPath, Bool bRecursive) const;
	Bool DeleteDirectory(const String& sAbsoluteDirPath, Bool bRecursive) const;

	// Return true if the file exists in any FileSystem contained in this
	// FileManager, false otherwise.
	Bool Exists(FilePath filePath) const;
	Bool Exists(const String& sAbsoluteFilename) const;
	Bool ExistsForPlatform(Platform ePlatform, FilePath filePath) const;

	// Variation that resolve filePath to the project's Source/ folder. Used in
	// particular scenarios where files are tracked by FilePath by must
	// be compared against their source counterpart (e.g. the Cooker and the CookDatabase).
	Bool ExistsInSource(FilePath filePath) const;

	// Return true if the given path exists and is a directory in any
	// FileSystem contained in this FileManager, false otherwise
	Bool IsDirectory(FilePath filePath) const;
	Bool IsDirectory(const String& sAbsoluteFilename) const;

	// Return the file size reported by the first FileSystem that reports
	// a time for filePath. FileSystems are evaluated in LIFO order.
	UInt64 GetFileSize(FilePath filePath) const;
	UInt64 GetFileSize(const String& sAbsoluteFilename) const;
	UInt64 GetFileSizeForPlatform(Platform ePlatform, FilePath filePath) const;

	// Return the modified time reported by the first FileSystem that reports
	// a time for filePath. FileSystems are evaluated in LIFO order. The
	// modified time may be 0u even if the file exists, if a FileSystem contains
	// the file but does not track modified times.
	UInt64 GetModifiedTime(FilePath filePath) const;
	UInt64 GetModifiedTime(const String& sAbsoluteFilename) const;
	UInt64 GetModifiedTimeForPlatform(Platform ePlatform, FilePath filePath) const;

	// Variation that resolve filePath to the project's Source/ folder. Used in
	// particular scenarios where files are tracked by FilePath by must
	// be compared against their source counterpart (e.g. the Cooker and the CookDatabase).
	UInt64 GetModifiedTimeInSource(FilePath filePath) const;

	// Attempt to rename the file or directory.
	Bool Rename(FilePath from, FilePath to) const;
	Bool Rename(const String& sAbsoluteFrom, const String& sAbsoluteTo) const;

	// Attempt to update the modified time of a file. Returns true
	// if the set was successful, false otherwise. Setting the mod
	// time of a file can fail if the file does not exist or due
	// to os/filesystem specific reasons (insufficient privileges).
	Bool SetModifiedTime(FilePath filePath, UInt64 uModifiedTime) const;
	Bool SetModifiedTime(const String& sAbsoluteFilename, UInt64 uModifiedTime) const;

	// Attempt to update the read/write status of a file.
	Bool SetReadOnlyBit(FilePath filePath, Bool bReadOnly) const;
	Bool SetReadOnlyBit(const String& sAbsoluteFilename, Bool bReadOnly) const;

	// Attempt to open a file - return true if the file was successfully opened.
	Bool OpenFile(FilePath filePath, File::Mode eMode, ScopedPtr<SyncFile>& rpFile) const;
	Bool OpenFile(const String& sAbsoluteFilename, File::Mode eMode, ScopedPtr<SyncFile>& rpFile) const;

	// Convenience method - executes the same operations as DiskSyncFile::ReadAll(),
	// except that the load is processed using FileManager's FileSystem stack.
	Bool ReadAll(
		FilePath filePath,
		void*& rpOutputBuffer,
		UInt32& rzOutputSizeInBytes,
		UInt32 zAlignmentOfOutputBuffer,
		MemoryBudgets::Type eOutputBufferMemoryType,
		UInt32 zMaxReadSize = kDefaultMaxReadSize) const;
	Bool ReadAllForPlatform(
		Platform ePlatform,
		FilePath filePath,
		void*& rpOutputBuffer,
		UInt32& rzOutputSizeInBytes,
		UInt32 zAlignmentOfOutputBuffer,
		MemoryBudgets::Type eOutputBufferMemoryType,
		UInt32 zMaxReadSize = kDefaultMaxReadSize) const;

	// Convenience method to load all of a file's data into a Vector instead of
	// a bag of bytes
	Bool ReadAll(
		FilePath filePath,
		Vector<Byte>& rvOutData,
		UInt32 zMaxReadSize = kDefaultMaxReadSize) const;

	// Convenience method to load all of a file's data into a String instead
	// of a bag of bytes
	Bool ReadAll(
		FilePath filePath,
		String& rsOutData,
		UInt32 zMaxReadSize = kDefaultMaxReadSize) const;

	// Convenience method - executes the same operations as DiskSyncFile::ReadAll(),
	// except that the load is processed using FileManager's FileSystem stack.
	Bool ReadAll(
		const String& sAbsoluteFilename,
		void*& rpOutputBuffer,
		UInt32& rzOutputSizeInBytes,
		UInt32 zAlignmentOfOutputBuffer,
		MemoryBudgets::Type eOutputBufferMemoryType,
		UInt32 zMaxReadSize = kDefaultMaxReadSize) const;

	// Convenience method to load all of a file's data into a Vector instead of
	// a bag of bytes
	Bool ReadAll(
		const String& sAbsoluteFilename,
		Vector<Byte>& rvOutData,
		UInt32 zMaxReadSize = kDefaultMaxReadSize) const;

	// Convenience method to load all of a file's data into a String instead
	// of a bag of bytes
	Bool ReadAll(
		const String& sAbsoluteFilename,
		String& rsOutData,
		UInt32 zMaxReadSize = kDefaultMaxReadSize) const;

	// Returns true if any file system is still initializing,
	// false otherwise.
	Bool IsAnyFileSystemStillInitializing() const;

	// Returns true if file operations on filePath will be serviced
	// over the network - the caller can assume that operations on this
	// file will be slower and less reliable than operations not serviced
	// over the network.
	Bool IsServicedByNetwork(FilePath filePath) const;
	Bool IsServicedByNetwork(const String& sAbsoluteFilename) const;

	// Synchronous blocking call - performs a network fetch or
	// FilePath. A return value of true indicates the file was
	// successfully fetched, false indicates that the fetch failed,
	// or the file does not exist in any FileSystem.
	Bool NetworkFetch(FilePath filePath, NetworkFetchPriority ePriority = NetworkFetchPriority::kDefault) const;

	// As relevant, tell FileSystems to queue a file for download
	// that will be serviced by a network file system.
	Bool NetworkPrefetch(FilePath filePath, NetworkFetchPriority ePriority = NetworkFetchPriority::kDefault) const;

	/**
	 * Similar to IsNetworkFileIOEnabled(), except will only be true
	 * after network IO has been explicitly shutdown (it is false from
	 * startup through network startup until network shutdown).
	 */
	Bool HasNetworkFileIOShutdown() const
	{
		return m_bNetworkFileIOShutdown;
	}

	/** Functions to track global network state dependency of some FileSystems. */
	Bool IsNetworkFileIOEnabled() const
	{
		return m_bNetworkFileIOEnabled;
	}
	void EnableNetworkFileIO();
	void DisableNetworkFileIO();

	// Attempts to generate a listing of files (and optional directories)
	// that fulfill the other arguments. Returns true if successful, false
	// otherwise. If this method returns false, then rvResults is left
	// unmodified.
	Bool GetDirectoryListing(
		FilePath dirPath,
		Vector<String>& rvResults,
		Bool bIncludeDirectoriesInResults = true,
		Bool bRecursive = true,
		const String& sFileExtension = String()) const;

	// Attempts to generate a listing of files (and optional directories)
	// that fulfill the other arguments. Returns true if successful, false
	// otherwise. If this method returns false, then rvResults is left
	// unmodified.
	Bool GetDirectoryListing(
		const String& sAbsoluteDirectoryPath,
		Vector<String>& rvResults,
		Bool bIncludeDirectoriesInResults = true,
		Bool bRecursive = true,
		const String& sFileExtension = String()) const;

	/**
	 * @return The current stack of FileSystems registered
	 * with this FileManager.
	 */
	const FileSystemStack& GetFileSystemStack() const
	{
		return m_vFileSystemStack;
	}

	// Convenience method - executes the same operations as DiskSyncFile::WriteAll(),
	// except that the save is processed using FileManager's FileSystem stack.
	Bool WriteAll(
		FilePath filePath,
		void const* pInputBuffer,
		UInt32 uInputSizeInBytes,
		UInt64 uModifiedTime = 0) const;

	// Convenience method - executes the same operations as DiskSyncFile::WriteAll(),
	// except that the save is processed using FileManager's FileSystem stack.
	Bool WriteAllForPlatform(
		Platform ePlatform,
		FilePath filePath,
		void const* pInputBuffer,
		UInt32 uInputSizeInBytes,
		UInt64 uModifiedTime = 0) const;

	// Convenience method - executes the same operations as DiskSyncFile::WriteAll(),
	// except that the save is processed using FileManager's FileSystem stack.
	Bool WriteAll(
		const String& sAbsoluteFilename,
		void const* pInputBuffer,
		UInt32 uInputSizeInBytes,
		UInt64 uModifiedTime = 0) const;

private:
	FileManager();
	~FileManager();

	Bool InternalReadAll(
		FilePath filePath,
		void*& rpOutputBuffer,
		UInt32& rzOutputSizeInBytes,
		UInt32 zAlignmentOfOutputBuffer,
		MemoryBudgets::Type eOutputBufferMemoryType,
		UInt32 zMaxReadSize) const;

	ScopedPtr<FileManagerRemap> m_pRemap;
	FileSystemStack m_vFileSystemStack;
	RefOnly m_RefOnly;
	Mutex m_NetworkFileIOMutex;
	Atomic32Value<Bool> m_bNetworkFileIOEnabled;
	Atomic32Value<Bool> m_bNetworkFileIOShutdown;

	SEOUL_DISABLE_COPY(FileManager);
}; // class FileManager

} // namespace Seoul

#endif // include guard
