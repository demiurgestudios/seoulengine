/**
 * \file Directory.cpp
 * \brief Functions to query and manipulate directories on disk.
 *
 * These functions only interact with the current platform's
 * persistent media - they will not interact with pack files and other
 * FileSystems through FileManager::Get().
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Directory.h"
#include "DirectoryInternal.h"
#include "DiskFileSystem.h"
#include "StringUtil.h"

namespace Seoul::Directory
{

/**
 * Delete the directory - if bRecursive is true,
 * also delete its contents.
 */
Bool Delete(const String& sAbsolutePath, Bool bRecursive)
{
	// Simple case.
	if (!Directory::DirectoryExists(sAbsolutePath))
	{
		// Error checking.
		if (DiskSyncFile::FileExists(sAbsolutePath))
		{
			return false;
		}

		return true;
	}

	// Simple case.
	if (!bRecursive)
	{
		return DirectoryDetail::RemoveDirectory(sAbsolutePath);
	}

	// Get a file listing, then recursively delete any nested entries.
	Vector<String> vs;
	if (!GetDirectoryListing(sAbsolutePath, vs, true, false, String()))
	{
		return false;
	}

	// Iterate and handle appropriately.
	for (auto const& s : vs)
	{
		if (Directory::DirectoryExists(s))
		{
			if (!Delete(s, true))
			{
				return false;
			}
		}
		else
		{
			if (!DiskSyncFile::DeleteFile(s))
			{
				return false;
			}
		}
	}

	// Now remove the directory itself.
	return DirectoryDetail::RemoveDirectory(sAbsolutePath);
}

/**
 * @return True if sAbsoluteDirectoryPath is a directory and that
 * directory exists, false otherwise.
 */
Bool DirectoryExists(const String& sAbsoluteDirectoryPath)
{
	return DirectoryDetail::DirectoryExists(sAbsoluteDirectoryPath);
}

/**
 * Populate rvResults with files and directories (if bIncludeDirectoriesInResults
 * is true) contained within the directory sAbsoluteDirectoryPath.
 */
Bool GetDirectoryListing(
	const String& sAbsoluteDirectoryPath,
	Vector<String>& rvResults,
	Bool bIncludeDirectoriesInResults,
	Bool bRecursive,
	const String& sFileExtension)
{
	rvResults.Clear();

	return DirectoryDetail::GetDirectoryListing(
		sAbsoluteDirectoryPath,
		rvResults,
		bIncludeDirectoriesInResults,
		bRecursive,
		sAbsoluteDirectoryPath,
		sFileExtension);
}

/**
 * Create the directory sDirectoryPath. Only succeeds
 * if all parent directories of sDirectoyPath already exist.
 *
 * @return True if the directory was created successfully,
 * false otherwise.
 */
Bool CreateDirectory(const String& sAbsoluteDirectoryPath)
{
	return DirectoryDetail::CreateDirectory(sAbsoluteDirectoryPath);
}

/**
 * @return True if sDirectory appears to point at the root
 * of the file system, false otherwise.
 */
inline static Bool IsRoot(const String& sDirectory)
{
	// Cases:
	// - directory is empty - path was initially relative.
	// - get the directory of sDirectory results in an empty
	//   string (after removing any whitespace) - sDirectory
	//   was either a drive delimiter (i.e. D:) or a root (i.e. /)
	return (
		sDirectory.IsEmpty() ||
		TrimWhiteSpace(Path::GetDirectoryName(sDirectory)).IsEmpty());
}

/**
 * Helper function used by CreateDirPath(), does not path normalization or
 * validation.
 */
inline static Bool InternalCreateDirPath(const String& sAbsoluteDirectoryPath)
{
	// Nothing to do if the directory exists already.
	if (DirectoryExists(sAbsoluteDirectoryPath))
	{
		return true;
	}

	String sParentDirectory = Path::GetDirectoryName(sAbsoluteDirectoryPath);

	// If the parent directory is not empty, attempt to create it (and its
	// dependencies) recursively.
	if (!IsRoot(sParentDirectory))
	{
		// Fail if the we failed to create any parents of the desired directory path.
		if (!InternalCreateDirPath(sParentDirectory))
		{
			return false;
		}
	}

	// Try to create the current level of the path.
	return CreateDirectory(sAbsoluteDirectoryPath);
}

/**
 * Create each directory in the path if it doesnt exist.
 * Returns true if the final directory was created sucessfully
 * or if was already created and false otherwise.
 */
Bool CreateDirPath(const String& sAbsoluteDirectoryPath)
{
	String sNormalizedAbsoluteDirectoryPath;

	// Normalize the path - CombineAndSimply with an empty string is an aggresive
	// normalization that will remove inline .\ and ..\, etc.
	if (!Path::CombineAndSimplify(String(), sAbsoluteDirectoryPath, sNormalizedAbsoluteDirectoryPath))
	{
		return false;
	}

	// Only valid for absolute paths.
	if (!Path::IsRooted(sNormalizedAbsoluteDirectoryPath))
	{
		return false;
	}

	// Remove the trailing directory separator, if there is one.
	if (Path::HasTrailingDirectorySeparator(sNormalizedAbsoluteDirectoryPath))
	{
		sNormalizedAbsoluteDirectoryPath =
			sNormalizedAbsoluteDirectoryPath.Substring(0, sNormalizedAbsoluteDirectoryPath.GetSize() - 1);
	}

	// Hand off creation to the helper function, which will attempt
	// to recursively create the path.
	return InternalCreateDirPath(sNormalizedAbsoluteDirectoryPath);
}

/** Specialized version of GetDirectoryListing() for bulk operation. */
Bool GetDirectoryListingEx(
	const String& sAbsoluteDirectoryPath,
	const GetDirectoryListingExCallback& callback)
{
	// Normalize the path, exclude the trailing slash.
	auto sNormalized(Path::GetExactPathName(sAbsoluteDirectoryPath));
	if (sNormalized.EndsWith(Path::DirectorySeparatorChar())) { sNormalized.PopBack(); }

	return DirectoryDetail::PlatformGetDirectoryListingEx(sNormalized, callback);
}

} // namespace Seoul::Directory
