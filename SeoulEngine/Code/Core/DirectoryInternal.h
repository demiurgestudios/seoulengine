/**
 * \file DirectoryInternal.h
 * \brief Internal header file included in Directory.cpp. Do not
 * include in other header files.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#if !defined(DIRECTORY_H)
#error "Internal header file, must only be included by Directory.cpp"
#endif

#include "DiskFileSystem.h"
#include "Path.h"
#include "Platform.h"
#include "Vector.h"
#include "Logger.h"
#include "StringUtil.h"

#include "stdio.h"
#include "string.h"

#if SEOUL_PLATFORM_WINDOWS
#include <io.h>
#include <share.h>
#include <sys/stat.h>
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace Seoul::DirectoryDetail
{

//
// PC implementations
//
#if SEOUL_PLATFORM_WINDOWS
/**
 * Helper function - normalizes sDirectory and adds
 * a wildcard character if necessary to execute directory list queries.
 */
static String InternalStaticNormalizeAndHandleWildcard(const String& sDirectory)
{
	static const String ksWildcard = "\\*";
	static const String ksAppendWildcard = ".\\*";

	String sNormalizedDirectory = Path::Normalize(sDirectory);
	String sDirectoryWithSlashStar;

	// Check if directory name ends in "*".
	if (sNormalizedDirectory.Find(ksWildcard) == sNormalizedDirectory.GetSize() - 1u)
	{
		sDirectoryWithSlashStar = sNormalizedDirectory;
	}
	// Otherwise, add the wildcard
	else
	{
		SEOUL_VERIFY(Path::CombineAndSimplify(sNormalizedDirectory, ksAppendWildcard, sDirectoryWithSlashStar));
	}

	return sDirectoryWithSlashStar;
}

inline Bool CreateDirectory(const String& sAbsoluteDirectoryPath)
{
	return (0 != ::CreateDirectoryW(sAbsoluteDirectoryPath.WStr(), nullptr));
}

inline Bool RemoveDirectory(const String& sAbsoluteDirectoryPath)
{
	// Normalize the directory path, and removing the trailing slash if it's present.
	String sNormalizedDirectory;
	SEOUL_VERIFY(Path::CombineAndSimplify(String(), sAbsoluteDirectoryPath, sNormalizedDirectory));
	if (sNormalizedDirectory.EndsWith(Path::DirectorySeparatorChar()))
	{
		sNormalizedDirectory = sNormalizedDirectory.Substring(0, sNormalizedDirectory.GetSize() - 1);
	}

	return (0 != ::RemoveDirectoryW(sNormalizedDirectory.WStr()));
}

inline Bool DirectoryExists(const String& sAbsoluteDirectoryPath)
{
	struct _stati64 statResults;
	memset(&statResults, 0, sizeof(statResults));

	// Normalize the directory path, and removing the trailing slash if it's present.
	String sNormalizedDirectory;
	SEOUL_VERIFY(Path::CombineAndSimplify(String(), sAbsoluteDirectoryPath, sNormalizedDirectory));
	if (sNormalizedDirectory.EndsWith(Path::DirectorySeparatorChar()))
	{
		sNormalizedDirectory = sNormalizedDirectory.Substring(0, sNormalizedDirectory.GetSize() - 1);
	}

	// Get the wide string version
	WString sDirectoryW = sNormalizedDirectory.WStr();

	// Stat reports whether the given path is a directory or not.
	Bool const bIsDirectory = (
		(0 == _wstati64(sDirectoryW, &statResults)) &&
		(0 != (statResults.st_mode & _S_IFDIR)));

	// _access_s will determine whether or not a given file or directory exists
	//   so we want to check if it exists and then make sure that the specified
	//   path is not a file (by using FileExists)
	return
		bIsDirectory &&
		(0 == _waccess_s(sDirectoryW, 0));
}

inline Bool GetDirectoryListing(
	const String& sAbsoluteDirectoryPath,
	Vector<String>& rvResults,
	Bool bIncludeDirectoriesInResults,
	Bool bRecursive,
	const String& sPrefix,
	const String& sFileExtension)
{
	const String sNormalizedDirectory = InternalStaticNormalizeAndHandleWildcard(
		sAbsoluteDirectoryPath);

	WIN32_FIND_DATAW findFileData;

	HANDLE hFindHandle = FindFirstFileW(
		sNormalizedDirectory.WStr(),
		&findFileData);

	// This can happen legitimately on some platforms if the directory
	// is empty, so we need to check if the directory we're trying
	// to enumerate exists before returning.
	if (INVALID_HANDLE_VALUE == hFindHandle)
	{
		return DirectoryExists(sNormalizedDirectory);
	}

	Bool bReturn = true;
	do
	{
		// Ignore "." and ".." entries.
		if (0 != WSTRCMP_CASE_INSENSITIVE(L".", findFileData.cFileName) &&
			0 != WSTRCMP_CASE_INSENSITIVE(L"..", findFileData.cFileName))
		{
			Bool bDirectory =
				(0u != (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY));

			String const sFindFileDataFileName = WCharTToUTF8(findFileData.cFileName);

			if (bIncludeDirectoriesInResults || !bDirectory)
			{
				// If a filetype was specified, only add files that match the
				// specified filetype.
				if (bDirectory || sFileExtension.IsEmpty() || sFindFileDataFileName.EndsWith(sFileExtension))
				{
					// Important - temporary here is a deliberate optimization to invoke
					// the move constructor instead of the copy constructor.
					rvResults.PushBack(Path::Combine(sPrefix, sFindFileDataFileName));
				}
			}

			if (bRecursive && bDirectory)
			{
				SEOUL_ASSERT(!sNormalizedDirectory.IsEmpty());
				String sBaseDirectory = sNormalizedDirectory.Substring(
					0u,
					sNormalizedDirectory.GetSize() - 1u);

				bReturn = (GetDirectoryListing(
					Path::Combine(sBaseDirectory, sFindFileDataFileName),
					rvResults,
					bIncludeDirectoriesInResults,
					bRecursive,
					Path::Combine(sPrefix, sFindFileDataFileName),
					sFileExtension) && bReturn);
			}
		}
	} while (FindNextFileW(hFindHandle, &findFileData) != 0);

	SEOUL_VERIFY(FindClose(hFindHandle) != 0);

	// Check to see if we succeeded in finding all of the files, or if an error
	// occurred somewhere.
	return (ERROR_NO_MORE_FILES == GetLastError() && bReturn);
}

/** Convert Win32 FILETIME value into equivalent Unix Epoch modification time in seconds. */
static inline UInt64 ToUnixFileTime(const FILETIME& time)
{
	ULARGE_INTEGER ull;
	ull.LowPart = time.dwLowDateTime;
	ull.HighPart = time.dwHighDateTime;
	auto const uReturn = (UInt64)(ull.QuadPart / 10000000ULL - 11644473600ULL);

	return uReturn;
}

/** Win32 implementation of specialized directory listing. */
inline Bool PlatformGetDirectoryListingEx(
	const String& sAbsoluteDirectoryPath,
	const Directory::GetDirectoryListingExCallback& callback)
{
	// Start the enumeration.
	WIN32_FIND_DATAW findFileData;
	auto hFindHandle = ::FindFirstFileW(
		(sAbsoluteDirectoryPath + Path::DirectorySeparatorChar() + "*").WStr(),
		&findFileData);

	// This can happen legitimately on some platforms if the directory
	// is empty, so we need to check if the directory we're trying
	// to enumerate exists before returning.
	if (INVALID_HANDLE_VALUE == hFindHandle)
	{
		return DirectoryExists(sAbsoluteDirectoryPath);
	}

	Bool bReturn = true;
	do
	{
		// Ignore "." and ".." entries.
		if (0 == wcscmp(L".", findFileData.cFileName) ||
			0 == wcscmp(L"..", findFileData.cFileName))
		{
			continue;
		}

		// Combine and convert.
		auto const sLeafName(WCharTToUTF8(findFileData.cFileName));
		auto const sAbsoluteName(Path::Combine(sAbsoluteDirectoryPath, sLeafName));

		// Add files, not directories.
		auto const bDirectory = (FILE_ATTRIBUTE_DIRECTORY == (FILE_ATTRIBUTE_DIRECTORY & findFileData.dwFileAttributes));
		if (!bDirectory)
		{
			// Populate the entry.
			Directory::DirEntryEx entry;
			entry.m_sFileName = sAbsoluteName;
			entry.m_uFileSize = ((UInt64)findFileData.nFileSizeLow | ((UInt64)findFileData.nFileSizeHigh << (UInt64)32));
			entry.m_uModifiedTime = ToUnixFileTime(findFileData.ftLastWriteTime);

			// Dispatch - on false, terminate loop.
			if (!callback(entry))
			{
				break;
			}
		}
		// Recurse into directories.
		else
		{
			if (!PlatformGetDirectoryListingEx(sAbsoluteName, callback))
			{
				bReturn = false;
				break;
			}
		}
	} while (FALSE != ::FindNextFileW(hFindHandle, &findFileData));

	// Close the search handle, always expected to succeed.
	SEOUL_VERIFY(FALSE != ::FindClose(hFindHandle));

	// Check to see if we succeeded in finding all of the files, or if an error
	// occurred somewhere.
	return (ERROR_NO_MORE_FILES == ::GetLastError() && bReturn);
}

#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX // /SEOUL_PLATFORM_WINDOWS

/**
 * Helper function - normalizes sDirectory and adds
 * a wildcard character if necessary to execute directory list queries.
 */
static String InternalStaticNormalizeAndHandleWildcard(const String& sDirectory)
{
	static const String ksWildcard = "/*";

	String sNormalizedDirectory = Path::Normalize(sDirectory);

	// Check if directory name ends in "*" - if so, remove it.
	if (sNormalizedDirectory.EndsWith(ksWildcard))
	{
		sNormalizedDirectory = sNormalizedDirectory.Substring(0u, sNormalizedDirectory.GetSize() - ksWildcard.GetSize());
	}

	return sNormalizedDirectory;
}

inline Bool CreateDirectory(const String& sAbsoluteDirectoryPath)
{
	return (0 == mkdir(sAbsoluteDirectoryPath.CStr(), S_IRWXU | S_IRWXG));
}

inline Bool RemoveDirectory(const String& sAbsoluteDirectoryPath)
{
	return (0 == rmdir(sAbsoluteDirectoryPath.CStr()));
}

inline Bool DirectoryExists(const String& sAbsoluteDirectoryPath)
{
	struct stat statResults;
	memset(&statResults, 0, sizeof(statResults));

	// Normalize the directory path, and removing the trailing slash if it's present.
	String sNormalizedDirectory;
	SEOUL_VERIFY(Path::CombineAndSimplify(String(), sAbsoluteDirectoryPath, sNormalizedDirectory));
	if (sNormalizedDirectory.EndsWith(Path::DirectorySeparatorChar()))
	{
		sNormalizedDirectory = sNormalizedDirectory.Substring(0, sNormalizedDirectory.GetSize() - 1);
	}

	// Stat reports whether the given path is a directory or not.
	Bool const bIsDirectory = (
		(0 == stat(sNormalizedDirectory.CStr(), &statResults)) &&
		(0 != (statResults.st_mode & S_IFDIR)));

	// access will determine whether or not a given file or directory exists
	//   so we want to check if it exists and then make sure that the specified
	//   path is not a file (by using FileExists)
	return
		bIsDirectory &&
		(0 == access(sNormalizedDirectory.CStr(), F_OK));
}

inline Bool GetDirectoryListing(
	const String& sAbsoluteDirectoryPath,
	Vector<String>& rvResults,
	Bool bIncludeDirectoriesInResults,
	Bool bRecursive,
	const String& sPrefix,
	const String& sFileExtension)
{
	const String sNormalizedDirectory = InternalStaticNormalizeAndHandleWildcard(
		sAbsoluteDirectoryPath);

	DIR* pDirectory = opendir(sNormalizedDirectory.CStr());
	if (nullptr == pDirectory)
	{
		return false;
	}

	Bool bReturn = true;
	struct dirent* pEntry = nullptr;
	while ((pEntry = readdir(pDirectory)) != nullptr)
	{
		// Ignore "." and ".." entries.
		if (0 != STRCMP_CASE_INSENSITIVE(".", pEntry->d_name) &&
			0 != STRCMP_CASE_INSENSITIVE("..", pEntry->d_name))
		{
			Bool const bDirectory = (DT_DIR == pEntry->d_type);
			if (bIncludeDirectoriesInResults || !bDirectory)
			{
				String const s(Path::Combine(sPrefix, pEntry->d_name));

				// If a filetype was specified, only add files that match the
				// specified filetype.
				if (bDirectory || sFileExtension.IsEmpty() || s.EndsWith(sFileExtension))
				{
					rvResults.PushBack(s);
				}
			}

			if (bRecursive && bDirectory)
			{
				SEOUL_ASSERT(!sNormalizedDirectory.IsEmpty());
				bReturn = (GetDirectoryListing(
					Path::Combine(sNormalizedDirectory, pEntry->d_name),
					rvResults,
					bIncludeDirectoriesInResults,
					bRecursive,
					Path::Combine(sPrefix, pEntry->d_name),
					sFileExtension) && bReturn);
			}
		}
	}

	// bReturn must come second to make sure cellFsClosedir is executed.
	return (0 == closedir(pDirectory)) && bReturn;
}

/** POSIX implementation of specialized directory listing. */
inline Bool PlatformGetDirectoryListingEx(
	const String& sAbsoluteDirectoryPath,
	const Directory::GetDirectoryListingExCallback& callback)
{
	DIR* pDirectory = opendir(sAbsoluteDirectoryPath.CStr());
	if (nullptr == pDirectory)
	{
		return false;
	}

	Bool bReturn = true;
	struct dirent* pEntry = nullptr;
	while ((pEntry = readdir(pDirectory)) != nullptr)
	{
		// Ignore "." and ".." entries.
		if (0 == strcmp(".", pEntry->d_name) ||
			0 == strcmp("..", pEntry->d_name))
		{
			continue;
		}

		// Combine and convert.
		auto const sLeafName(pEntry->d_name);
		auto const sAbsoluteName(Path::Combine(sAbsoluteDirectoryPath, sLeafName));

		// Add files, not directories.
		auto const bDirectory = (DT_DIR == pEntry->d_type);
		if (!bDirectory)
		{
			// Populate the entry.
			Directory::DirEntryEx entry;
			entry.m_sFileName = sAbsoluteName;
			entry.m_uFileSize = (UInt32)DiskSyncFile::GetFileSize(sAbsoluteName);
			entry.m_uModifiedTime = DiskSyncFile::GetModifiedTime(sAbsoluteName);

			// Dispatch - on false, terminate loop.
			if (!callback(entry))
			{
				break;
			}
		}
		// Recurse into directories.
		else
		{
			if (!PlatformGetDirectoryListingEx(sAbsoluteName, callback))
			{
				bReturn = false;
				break;
			}
		}
	}

	return (0 == closedir(pDirectory) && bReturn);
}

#else // /SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX

#error "Define for this platform."

#endif

} // namespace Seoul::DirectoryDetailFile
