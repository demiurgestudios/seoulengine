/**
 * \file CachingDiskFileSystem.cpp
 * \brief Specialization of DiskFileSystem that caches
 * size and modification time information in-memory.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "CachingDiskFileSystem.h"
#include "Directory.h"
#include "GamePaths.h"
#include "Thread.h"

namespace Seoul
{

CachingDiskFileSystem::CachingDiskFileSystem(Platform ePlatform, GameDirectory eDirectory)
	: m_ePlatform(ePlatform)
	, m_eDirectory(eDirectory)
	, m_bSource(false)
	, m_Disk(ePlatform, eDirectory, false)
	, m_Mutex()
	, m_tModTimes()
	, m_tSizes()
{
	Construct();
}

CachingDiskFileSystem::~CachingDiskFileSystem()
{
	m_pNotifier.Reset();
}

Bool CachingDiskFileSystem::ImplCopy(FilePath from, FilePath to, Bool bAllowOverwrite /*= false*/)
{
	// One or the other must be in our directory.
	if (m_eDirectory != from.GetDirectory() && m_eDirectory != to.GetDirectory())
	{
		return false;
	}

	if (m_Disk.Copy(from, to, bAllowOverwrite))
	{
		Lock lock(m_Mutex);
		InsideLockCheckDirty(from);

		// If we have data for from, easy case.
		UInt64 uFromMod = 0;
		UInt64 uFromSize = 0;
		if (m_tModTimes.GetValue(from, uFromMod) &&
			m_tSizes.GetValue(from, uFromSize))
		{
			m_tModTimes.Overwrite(to, uFromMod);
			m_tSizes.Overwrite(to, uFromSize);
		}
		// Otherwise, lookup new values and insert.
		else
		{
			// Populate with new lookup.
			m_tModTimes.Overwrite(to, m_Disk.GetModifiedTime(to));
			m_tSizes.Overwrite(to, m_Disk.GetFileSize(to));
		}

		return true;
	}

	return false;
}

Bool CachingDiskFileSystem::ImplCopy(const String& sAbsoluteFrom, const String& sAbsoluteTo, Bool bAllowOverwrite /*= false*/)
{
	// From or to must be in our directory.
	auto const from = ToFilePath(sAbsoluteFrom);
	auto const to = ToFilePath(sAbsoluteTo);
	if ((!from.IsValid() || m_eDirectory != from.GetDirectory()) &&
		(!to.IsValid() || m_eDirectory != to.GetDirectory()))
	{
		return false;
	}

	if (m_Disk.Copy(sAbsoluteFrom, sAbsoluteTo, bAllowOverwrite))
	{
		Lock lock(m_Mutex);
		InsideLockCheckDirty(from);

		// If we have data for from, easy case.
		UInt64 uFromMod = 0;
		UInt64 uFromSize = 0;
		if (from.IsValid() &&
			m_eDirectory == from.GetDirectory() &&
			m_tModTimes.GetValue(from, uFromMod) &&
			m_tSizes.GetValue(from, uFromSize))
		{
			m_tModTimes.Overwrite(to, uFromMod);
			m_tSizes.Overwrite(to, uFromSize);
		}
		// Otherwise, lookup new values and insert.
		else
		{
			// Populate with new lookup.
			m_tModTimes.Overwrite(to, m_Disk.GetModifiedTime(to));
			m_tSizes.Overwrite(to, m_Disk.GetFileSize(to));
		}

		return true;
	}

	return false;
}

Bool CachingDiskFileSystem::ImplCreateDirPath(FilePath dirPath)
{
	if (m_eDirectory != dirPath.GetDirectory())
	{
		return false;
	}

	// Create the directory.
	return m_Disk.CreateDirPath(dirPath);
}

Bool CachingDiskFileSystem::ImplDeleteDirectory(FilePath dirPath, Bool bRecursive)
{
	if (m_eDirectory != dirPath.GetDirectory())
	{
		return false;
	}

	// If not recursive, can just perform - it can't affect anything we care about
	// (we don't cache/track empty directories).
	if (!bRecursive)
	{
		return m_Disk.DeleteDirectory(dirPath, bRecursive);
	}

	// Otherwise, need to prune after delete.
	if (m_Disk.DeleteDirectory(dirPath, bRecursive))
	{
		Lock lock(m_Mutex);

		// IMPORTANT: Must uses sizes to be equivalent to Exists, *not* m_tModTimes,
		// although we try to keep them in sync, m_Disk.GetModifiedTime() can
		// return a value for a directory where as GetFileSize() will not.

		// Now find files we need to delete.
		Vector<FilePath, MemoryBudgets::Io> vToDelete;
		for (auto const& pair : m_tSizes)
		{
			if (strstr(pair.First.CStr(), dirPath.CStr()) == pair.First.CStr())
			{
				vToDelete.PushBack(pair.First);
			}
		}

		// Now purge.
		for (auto const& e : vToDelete)
		{
			(void)m_tModTimes.Erase(e);
			(void)m_tSizes.Erase(e);
		}

		return true;
	}

	return false;
}

Bool CachingDiskFileSystem::ImplDelete(FilePath filePath)
{
	if (m_eDirectory != filePath.GetDirectory())
	{
		return false;
	}

	// Attempt the delete.
	if (m_Disk.Delete(filePath))
	{
		Lock lock(m_Mutex);
		m_tModTimes.Erase(filePath);
		m_tSizes.Erase(filePath);
		return true;
	}

	return false;
}

Bool CachingDiskFileSystem::ImplExists(FilePath filePath) const
{
	if (m_eDirectory != filePath.GetDirectory())
	{
		return false;
	}

	// IMPORTANT: Must uses sizes to be equivalent to Exists, *not* m_tModTimes,
	// although we try to keep them in sync, m_Disk.GetModifiedTime() can
	// return a value for a directory where as GetFileSize() will not.
	Lock lock(m_Mutex);
	const_cast<CachingDiskFileSystem*>(this)->InsideLockCheckDirty(filePath);
	return m_tSizes.HasValue(filePath);
}

Bool CachingDiskFileSystem::ImplGetDirectoryListing(
	FilePath dirPath,
	Vector<String>& rvResults,
	Bool bIncludeDirectoriesInResults /*= true*/,
	Bool bRecursive /*= true*/,
	const String& sFileExtension /*= String()*/) const
{
	if (m_eDirectory != dirPath.GetDirectory())
	{
		return false;
	}

	// Must go directly to disk if the query wants directories in the result,
	// or if the extension is unexpected.
	//
	// TODO: Always true?
	// We also go to disk if the query is not recursive, since we lose a lot
	// of the benefit for shallow queries.
	auto const eType = ExtensionToFileType(sFileExtension);
	if (bIncludeDirectoriesInResults || !bRecursive || (FileType::kUnknown == eType && !sFileExtension.IsEmpty()))
	{
		return m_Disk.GetDirectoryListing(dirPath, rvResults, bIncludeDirectoriesInResults, bRecursive, sFileExtension);
	}
	// Otherwise, assemble from file size table.
	else
	{
		// Prep - if the relative filename in the FilePath doesn't
		// trail a directory separator, add one (we don't expect it to,
		// but use a conditional here for robustness/future proofing).
		auto const sep = (Byte)Path::kDirectorySeparatorChar;
		// DirPaths are expected to have no extension, but if they do
		// have an extension, we must include it.
		auto sRel(dirPath.GetRelativeFilename());
		if (!sRel.IsEmpty() && sep != sRel[sRel.GetSize() - 1])
		{
			sRel += sep;
		}
		auto const relstr = sRel.CStr();

		// Prep.
		rvResults.Clear();

		// Perform.
		{
			Lock lock(m_Mutex);
			const_cast<CachingDiskFileSystem*>(this)->InsideLockCheckDirtyDir(relstr);
			// Optimization for "all files" case.
			if (sRel.IsEmpty())
			{
				for (auto const& pair : m_tSizes)
				{
					if (FileType::kUnknown != eType && eType != pair.First.GetType())
					{
						continue;
					}

					rvResults.PushBack(m_Disk.ToFilename(pair.First));
				}
			}
			else
			{
				for (auto const& pair : m_tSizes)
				{
					if (FileType::kUnknown != eType && eType != pair.First.GetType())
					{
						continue;
					}

					auto const cstr = pair.First.GetRelativeFilenameWithoutExtension().CStr();
					if (cstr == strstr(cstr, relstr))
					{
						rvResults.PushBack(m_Disk.ToFilename(pair.First));
					}
				}
			}
		}

		QuickSort(rvResults.Begin(), rvResults.End());

		// For consistency with DiskFileSystem, if rvResults.IsEmpty() is true,
		// then we only return true if the directory exists.
		if (rvResults.IsEmpty())
		{
			return m_Disk.IsDirectory(dirPath);
		}

		return true;
	}
}

Bool CachingDiskFileSystem::ImplGetFileSize(FilePath filePath, UInt64& rzFileSize) const
{
	if (m_eDirectory != filePath.GetDirectory())
	{
		return false;
	}

	Lock lock(m_Mutex);
	const_cast<CachingDiskFileSystem*>(this)->InsideLockCheckDirty(filePath);
	return m_tSizes.GetValue(filePath, rzFileSize);
}

Bool CachingDiskFileSystem::ImplGetModifiedTime(FilePath filePath, UInt64& ruModifiedTime) const
{
	if (m_eDirectory != filePath.GetDirectory())
	{
		return false;
	}

	Lock lock(m_Mutex);
	const_cast<CachingDiskFileSystem*>(this)->InsideLockCheckDirty(filePath);
	return m_tModTimes.GetValue(filePath, ruModifiedTime);
}

Bool CachingDiskFileSystem::ImplIsDirectory(FilePath dirPath) const
{
	if (m_eDirectory != dirPath.GetDirectory())
	{
		return false;
	}

	return m_Disk.IsDirectory(dirPath);
}

class CachingDiskSyncFile SEOUL_SEALED : public DiskSyncFile
{
public:
	CachingDiskSyncFile(CachingDiskFileSystem& rOwner, FilePath filePath, File::Mode eMode)
		: DiskSyncFile(rOwner.m_Disk.ToFilename(filePath), eMode)
		, m_FilePath(filePath)
		, m_rOwner(rOwner)
		, m_bNeedsCommit(false)
	{
	}

	~CachingDiskSyncFile()
	{
		InternalClose();
		Commit();
	}

	virtual Bool Flush() SEOUL_OVERRIDE
	{
		return DiskSyncFile::Flush() && Commit();
	}

	virtual UInt32 WriteRawData(void const* pIn, UInt32 zSizeInBytes) SEOUL_OVERRIDE
	{
		if (zSizeInBytes > 0u)
		{
			m_bNeedsCommit = true;
		}

		return DiskSyncFile::WriteRawData(pIn, zSizeInBytes);
	}

private:
	SEOUL_DISABLE_COPY(CachingDiskSyncFile);

	FilePath const m_FilePath;
	CachingDiskFileSystem& m_rOwner;
	Bool m_bNeedsCommit;

	Bool Commit()
	{
		if (!m_bNeedsCommit)
		{
			return true;
		}

		Bool bReturn = true;
		UInt64 uModTime = 0;
		if (m_rOwner.m_Disk.GetModifiedTime(m_FilePath, uModTime))
		{
			Lock lock(m_rOwner.m_Mutex);
			m_rOwner.m_tModTimes.Overwrite(m_FilePath, uModTime);
		}
		else
		{
			bReturn = false;
		}

		UInt64 uFileSize = 0;
		if (m_rOwner.m_Disk.GetFileSize(m_FilePath, uFileSize))
		{
			Lock lock(m_rOwner.m_Mutex);
			m_rOwner.m_tSizes.Overwrite(m_FilePath, uFileSize);
		}
		else
		{
			bReturn = false;
		}

		// Update
		m_bNeedsCommit = !bReturn;
		return bReturn;
	}
}; // class CachingDiskSyncFile

Bool CachingDiskFileSystem::ImplOpen(
	FilePath filePath,
	File::Mode eMode,
	ScopedPtr<SyncFile>& rpFile)
{
	if (m_eDirectory != filePath.GetDirectory())
	{
		return false;
	}

	// Plain read, just open.
	if (File::kRead == eMode)
	{
		return m_Disk.Open(filePath, eMode, rpFile);
	}
	// Any write, need to update caches after the file
	// has been closed.
	else
	{
		ScopedPtr<SyncFile> pFile(SEOUL_NEW(MemoryBudgets::Io) CachingDiskSyncFile(*this, filePath, eMode));
		if (pFile.IsValid() && pFile->IsOpen())
		{
			rpFile.Swap(pFile);
			return true;
		}

		return false;
	}
}

Bool CachingDiskFileSystem::ImplReadAll(
	FilePath filePath,
	void*& rpOutputBuffer,
	UInt32& rzOutputSizeInBytes,
	UInt32 zAlignmentOfOutputBuffer,
	MemoryBudgets::Type eOutputBufferMemoryType,
	UInt32 zMaxReadSize /*= kDefaultMaxReadSize*/)
{
	if (m_eDirectory != filePath.GetDirectory())
	{
		return false;
	}

	return m_Disk.ReadAll(
		filePath,
		rpOutputBuffer,
		rzOutputSizeInBytes,
		zAlignmentOfOutputBuffer,
		eOutputBufferMemoryType,
		zMaxReadSize);
}

Bool CachingDiskFileSystem::ImplRename(FilePath from, FilePath to)
{
	// One or the other must be in our directory.
	if (m_eDirectory != from.GetDirectory() && m_eDirectory != to.GetDirectory())
	{
		return false;
	}

	if (m_Disk.Rename(from, to))
	{
		Lock lock(m_Mutex);
		InsideLockCheckDirty(from);

		// If we have data for from, easy case.
		UInt64 uFromMod = 0;
		UInt64 uFromSize = 0;
		if (m_tModTimes.GetValue(from, uFromMod) &&
			m_tSizes.GetValue(from, uFromSize))
		{
			m_tModTimes.Erase(from);
			m_tSizes.Erase(from);
			m_tModTimes.Overwrite(to, uFromMod);
			m_tSizes.Overwrite(to, uFromSize);
		}
		// Otherwise, lookup new values and insert.
		else
		{
			// Erase for sanity.
			m_tModTimes.Erase(from);
			m_tSizes.Erase(from);
			// Populate with new lookup.
			m_tModTimes.Overwrite(to, m_Disk.GetModifiedTime(to));
			m_tSizes.Overwrite(to, m_Disk.GetFileSize(to));
		}

		return true;
	}

	return false;
}

Bool CachingDiskFileSystem::ImplRename(const String& sAbsoluteFrom, const String& sAbsoluteTo)
{
	// From or to must be in our directory.
	auto const from = ToFilePath(sAbsoluteFrom);
	auto const to = ToFilePath(sAbsoluteTo);
	if ((!from.IsValid() || m_eDirectory != from.GetDirectory()) &&
		(!to.IsValid() || m_eDirectory != to.GetDirectory()))
	{
		return false;
	}

	if (m_Disk.Rename(sAbsoluteFrom, sAbsoluteTo))
	{
		Lock lock(m_Mutex);
		InsideLockCheckDirty(from);

		// If we have data for from, easy case.
		UInt64 uFromMod = 0;
		UInt64 uFromSize = 0;
		if (from.IsValid() &&
			m_eDirectory == from.GetDirectory() &&
			m_tModTimes.GetValue(from, uFromMod) &&
			m_tSizes.GetValue(from, uFromSize))
		{
			m_tModTimes.Erase(from);
			m_tSizes.Erase(from);
			m_tModTimes.Overwrite(to, uFromMod);
			m_tSizes.Overwrite(to, uFromSize);
		}
		// Otherwise, lookup new values and insert.
		else
		{
			// Erase for sanity.
			m_tModTimes.Erase(from);
			m_tSizes.Erase(from);
			// Populate with new lookup.
			m_tModTimes.Overwrite(to, m_Disk.GetModifiedTime(to));
			m_tSizes.Overwrite(to, m_Disk.GetFileSize(to));
		}

		return true;
	}

	return false;
}

Bool CachingDiskFileSystem::ImplSetModifiedTime(FilePath filePath, UInt64 uModifiedTime)
{
	if (m_eDirectory != filePath.GetDirectory())
	{
		return false;
	}

	if (m_Disk.SetModifiedTime(filePath, uModifiedTime))
	{
		Lock lock(m_Mutex);
		m_tModTimes.Overwrite(filePath, uModifiedTime);
		return true;
	}

	return false;
}

Bool CachingDiskFileSystem::ImplSetReadOnlyBit(FilePath filePath, Bool bReadOnly)
{
	if (m_eDirectory != filePath.GetDirectory())
	{
		return false;
	}

	return m_Disk.SetReadOnlyBit(filePath, bReadOnly);
}

Bool CachingDiskFileSystem::ImplWriteAll(
	FilePath filePath,
	void const* pInputBuffer,
	UInt32 uInputSizeInBytes,
	UInt64 uModifiedTime /* = 0u */)
{
	if (m_eDirectory != filePath.GetDirectory())
	{
		return false;
	}

	if (m_Disk.WriteAll(filePath, pInputBuffer, uInputSizeInBytes, uModifiedTime))
	{
		// Populate with new lookup.
		if (0u == uModifiedTime)
		{
			uModifiedTime = m_Disk.GetModifiedTime(filePath);
		}

		Lock lock(m_Mutex);
		m_tModTimes.Overwrite(filePath, uModifiedTime);
		m_tSizes.Overwrite(filePath, uInputSizeInBytes);
		return true;
	}

	return false;
}

/**
 * Subclass only entry point, special override, used
 * by SourceCachingDiskFileSystem.
 */
CachingDiskFileSystem::CachingDiskFileSystem(Platform ePlatform, GameDirectory eDirectory, Bool bSource)
	: m_ePlatform(ePlatform)
	, m_eDirectory(eDirectory)
	, m_bSource(bSource)
	, m_Disk(ePlatform, eDirectory, bSource)
	, m_Mutex()
	, m_tModTimes()
	, m_tSizes()
{
	Construct();
}

/** Common construct body for constructor variations. */
void CachingDiskFileSystem::Construct()
{
	Lock lock(m_Mutex);
	InsideLockPopulateCaches();

	// Register a notifier to detect changes.
	m_pNotifier.Reset(SEOUL_NEW(MemoryBudgets::Cooking) FileChangeNotifier(
		GetRootDirectory(),
		SEOUL_BIND_DELEGATE(&CachingDiskFileSystem::OnFileChange, this),
		(UInt32)FileChangeNotifier::kAll));
}

/**
 * Called when our notifier monitoring registers a file change.
 */
void CachingDiskFileSystem::OnFileChange(
	const String& sOldPath,
	const String& sNewPath,
	FileChangeNotifier::FileEvent eEvent)
{
	auto const oldPath = ToFilePath(sOldPath);
	auto const newPath = ToFilePath(sNewPath);

	if (oldPath.IsValid() && oldPath.GetDirectory() == m_eDirectory)
	{
		Lock lock(m_DirtyMutex);
		m_Dirty.Insert(oldPath);
		if (oldPath != newPath &&
			newPath.IsValid() &&
			newPath.GetDirectory() == m_eDirectory)
		{
			m_Dirty.Insert(newPath);
		}
	}
	else if (newPath.IsValid() && newPath.GetDirectory() == m_eDirectory)
	{
		Lock lock(m_DirtyMutex);
		m_Dirty.Insert(newPath);
	}

	++m_OnFileChangesCount; // Tracking.
}

/** @return The root directory of the file system as configured. */
const String& CachingDiskFileSystem::GetRootDirectory() const
{
	if (m_bSource && GameDirectory::kContent == m_eDirectory)
	{
		return GamePaths::Get()->GetSourceDir();
	}
	else
	{
		return GameDirectoryToStringForPlatform(m_eDirectory, m_ePlatform);
	}
}

/**
 * Customized body of FilePath::CreateFilePath(), avoid implicit coercion
 * of Source paths if we're reference kContent.
 */
FilePath CachingDiskFileSystem::ToFilePath(const String& sAbsoluteFilename) const
{
	// Handle empty filenames.
	if (sAbsoluteFilename.IsEmpty())
	{
		FilePath ret;
		ret.SetDirectory(m_eDirectory);
		return ret;
	}

	if (GameDirectory::kContent == m_eDirectory)
	{
		// Normalize the path.
		String sRelativePath = Path::Normalize(sAbsoluteFilename);

		// Strip the trailing slash, if there is one.
		if (!sRelativePath.IsEmpty() &&
			sRelativePath[sRelativePath.GetSize() - 1] == Path::kDirectorySeparatorChar)
		{
			sRelativePath.PopBack();
		}

		FileType eFileType;

		// Extract the extension.
		auto const sExtension(Path::GetExtension(sRelativePath));

		// Try to determine the FileType. It's ok if this is kUnknown.
		eFileType = ExtensionToFileType(sExtension);

		// If eFileType is unknown but the extension is not empty,
		// return an invalid file path.
		if (FileType::kUnknown == eFileType && !sExtension.IsEmpty())
		{
			return FilePath();
		}

		// The relative path is the path specified in sFilename without the
		// extension.
		sRelativePath = Path::GetPathWithoutExtension(sRelativePath);

		// Further processing if the path is absolute.
		if (Path::IsRooted(sRelativePath))
		{
			// Get the string representing the game directory of the filename.
			auto const& sFileBase = GetRootDirectory();

			// Find the index of the base directory in the normalized path.
			auto const bStartsWith = sRelativePath.StartsWithASCIICaseInsensitive(sFileBase);

			// If the base path exists and its total size is less than that of the absolute path,
			// relativize the absolute path.
			if (bStartsWith)
			{
				// Update the relative path to exclude the absolute directory portion.
				sRelativePath = sRelativePath.Substring(sFileBase.GetSize());
			}
			else
			{
				// Last check - if relative path is equal to the file base - 1 (strip the trailing
				// slash) *and* it is of file type kUnknown, then it is the root directory.
				if (FileType::kUnknown == eFileType &&
					sRelativePath.GetSize() + 1 == sFileBase.GetSize() &&
					0 == strncmp(sRelativePath.CStr(), sFileBase.CStr(), sRelativePath.GetSize()))
				{
					FilePath ret;
					ret.SetDirectory(m_eDirectory);
					return ret;
				}

				// Otherwise there is hinky going on and we return an invalid path.
				return FilePath();
			}
		}

		// If we get here, either the path was already relative, or it was
		// successfully made relative.

		// Combine the path with an empty root in order to simplify
		// away patterns such as "..\" and ".\"
		if (!Path::CombineAndSimplify(
			String(),
			sRelativePath,
			sRelativePath))
		{
			return FilePath();
		}

		// Initialize the file path and return it. Use the case insensitive
		// constructor so that the relative filename is case insensitive.
		FilePath ret;
		ret.SetRelativeFilenameWithoutExtension(FilePathRelativeFilename(sRelativePath));
		ret.SetDirectory(m_eDirectory);
		ret.SetType(eFileType);
		return ret;
	}
	else
	{
		return FilePath::CreateFilePath(m_eDirectory, sAbsoluteFilename);
	}
}

Bool CachingDiskFileSystem::InsideLockAddToCache(Directory::DirEntryEx& rEntry)
{
	auto const filePath(ToFilePath(rEntry.m_sFileName));
	m_tModTimes.Overwrite(filePath, rEntry.m_uModifiedTime);
	m_tSizes.Overwrite(filePath, rEntry.m_uFileSize);
	return true;
}

void CachingDiskFileSystem::InsideLockCheckDirty(FilePath filePath)
{
	Bool bErased = false;
	{
		Lock lock(m_DirtyMutex);
		bErased = m_Dirty.Erase(filePath);
	}

	if (bErased)
	{
		// IMPORTANT: We use GetFileSize as double duty to indicate
		// existence of the file - GetModifiedTime can return true
		// for a directory as well, and we need to know whether
		// this is a file or not.
		UInt64 uFileSize = 0;
		if (m_Disk.GetFileSize(filePath, uFileSize))
		{
			m_tModTimes.Overwrite(filePath, m_Disk.GetModifiedTime(filePath));
			m_tSizes.Overwrite(filePath, uFileSize);
		}
		else
		{
			m_tModTimes.Erase(filePath);
			m_tSizes.Erase(filePath);
		}
	}
}

void CachingDiskFileSystem::InsideLockCheckDirtyDir(Byte const* sRelDir)
{
	Vector<FilePath, MemoryBudgets::Io> v;
	{
		Lock lock(m_DirtyMutex);
		
		// Gather files marked as directy within the directory we're
		// about to query.
		for (auto const& filePath : m_Dirty)
		{
			auto const cstr = filePath.GetRelativeFilenameWithoutExtension().CStr();
			if (cstr == strstr(cstr, sRelDir))
			{
				v.PushBack(filePath);
			}
		}
		
		// Now remove any files from dirty that we're refreshing (prior
		// to releasing dirty mutex).
		for (auto const& filePath : v)
		{
			m_Dirty.Erase(filePath);
		}
	}

	// Finally, refresh any files that we removed from dirty.
	if (!v.IsEmpty())
	{
		for (auto const& filePath : v)
		{
			// IMPORTANT: We use GetFileSize as double duty to indicate
			// existence of the file - GetModifiedTime can return true
			// for a directory as well, and we need to know whether
			// this is a file or not.
			UInt64 uFileSize = 0;
			if (m_Disk.GetFileSize(filePath, uFileSize))
			{
				m_tModTimes.Overwrite(filePath, m_Disk.GetModifiedTime(filePath));
				m_tSizes.Overwrite(filePath, uFileSize);
			}
			else
			{
				m_tModTimes.Erase(filePath);
				m_tSizes.Erase(filePath);
			}
		}
	}
}

void CachingDiskFileSystem::InsideLockPopulateCaches()
{
	// Clear.
	m_tModTimes.Clear();
	m_tSizes.Clear();

	(void)Directory::GetDirectoryListingEx(
		GetRootDirectory(),
		SEOUL_BIND_DELEGATE(&CachingDiskFileSystem::InsideLockAddToCache, this));
}

Bool CachingDiskFileSystem::Disk::Copy(FilePath from, FilePath to, Bool bAllowOverwrite /*= false*/)
{
	return m_Internal.Copy(ToFilename(from), ToFilename(to), bAllowOverwrite);
}

Bool CachingDiskFileSystem::Disk::Copy(const String& sAbsoluteFrom, const String& sAbsoluteTo, Bool bAllowOverwrite /*= false*/)
{
	return m_Internal.Copy(sAbsoluteFrom, sAbsoluteTo, bAllowOverwrite);
}

Bool CachingDiskFileSystem::Disk::CreateDirPath(FilePath dirPath)
{
	return m_Internal.CreateDirPath(ToFilename(dirPath));
}

Bool CachingDiskFileSystem::Disk::Delete(FilePath filePath)
{
	return m_Internal.Delete(ToFilename(filePath));
}

Bool CachingDiskFileSystem::Disk::DeleteDirectory(FilePath dirPath, Bool bRecursive)
{
	return m_Internal.DeleteDirectory(ToFilename(dirPath), bRecursive);
}

Bool CachingDiskFileSystem::Disk::Exists(FilePath filePath) const
{
	return m_Internal.Exists(ToFilename(filePath));
}

Bool CachingDiskFileSystem::Disk::IsDirectory(FilePath filePath) const
{
	return m_Internal.IsDirectory(ToFilename(filePath));
}

Bool CachingDiskFileSystem::Disk::GetDirectoryListing(FilePath dirPath, Vector<String>& rvResults, Bool bIncludeDirectoriesInResults /*= true*/, Bool bRecursive /*= true*/, const String& sFileExtension /*= String()*/) const
{
	return m_Internal.GetDirectoryListing(ToFilename(dirPath), rvResults, bIncludeDirectoriesInResults, bRecursive, sFileExtension);
}

Bool CachingDiskFileSystem::Disk::GetFileSize(FilePath filePath, UInt64& ru) const
{
	return m_Internal.GetFileSize(ToFilename(filePath), ru);
}

UInt64 CachingDiskFileSystem::Disk::GetFileSize(FilePath filePath) const
{
	return DiskSyncFile::GetFileSize(ToFilename(filePath));
}

Bool CachingDiskFileSystem::Disk::GetModifiedTime(FilePath filePath, UInt64& ru) const
{
	return m_Internal.GetModifiedTime(ToFilename(filePath), ru);
}

UInt64 CachingDiskFileSystem::Disk::GetModifiedTime(FilePath filePath) const
{
	return DiskSyncFile::GetModifiedTime(ToFilename(filePath));
}

Bool CachingDiskFileSystem::Disk::Open(FilePath filePath, File::Mode eMode, ScopedPtr<SyncFile>& rpFile)
{
	return m_Internal.Open(ToFilename(filePath), eMode, rpFile);
}

Bool CachingDiskFileSystem::Disk::ReadAll(FilePath filePath, void*& rpOutputBuffer, UInt32& rzOutputSizeInBytes, UInt32 zAlignmentOfOutputBuffer, MemoryBudgets::Type eOutputBufferMemoryType, UInt32 zMaxReadSize /*= kDefaultMaxReadSize*/)
{
	return m_Internal.ReadAll(ToFilename(filePath), rpOutputBuffer, rzOutputSizeInBytes, zAlignmentOfOutputBuffer, eOutputBufferMemoryType, zMaxReadSize);
}

Bool CachingDiskFileSystem::Disk::Rename(FilePath from, FilePath to)
{
	return m_Internal.Rename(ToFilename(from), ToFilename(to));
}

Bool CachingDiskFileSystem::Disk::Rename(const String& sAbsoluteFrom, const String& sAbsoluteTo)
{
	return m_Internal.Rename(sAbsoluteFrom, sAbsoluteTo);
}

Bool CachingDiskFileSystem::Disk::SetModifiedTime(FilePath filePath, UInt64 uModifiedTime)
{
	return m_Internal.SetModifiedTime(ToFilename(filePath), uModifiedTime);
}

Bool CachingDiskFileSystem::Disk::SetReadOnlyBit(FilePath filePath, Bool bReadOnly)
{
	return m_Internal.SetReadOnlyBit(ToFilename(filePath), bReadOnly);
}

Bool CachingDiskFileSystem::Disk::WriteAll(FilePath filePath, void const* pIn, UInt32 uSizeInBytes, UInt64 uModifiedTime /*= 0*/)
{
	return m_Internal.WriteAll(ToFilename(filePath), pIn, uSizeInBytes, uModifiedTime);
}

String CachingDiskFileSystem::Disk::ToFilename(FilePath filePath) const
{
	if (m_bSource && GameDirectory::kContent == m_eDirectory)
	{
		return filePath.GetAbsoluteFilenameInSource();
	}
	else
	{
		return filePath.GetAbsoluteFilenameForPlatform(m_ePlatform);
	}
}

SourceCachingDiskFileSystem::SourceCachingDiskFileSystem(Platform ePlatform)
	: CachingDiskFileSystem(ePlatform, GameDirectory::kContent, true)
{
}

SourceCachingDiskFileSystem::~SourceCachingDiskFileSystem()
{
}

} // namespace Seoul
