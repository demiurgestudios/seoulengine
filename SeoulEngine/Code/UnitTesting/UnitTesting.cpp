/**
 * \file UnitTesting.cpp
 * \brief Classes and utilities for implementing unit testing.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Atomic32.h"
#include "Directory.h"
#include "DiskFileSystem.h"
#include "FileManager.h"
#include "GamePaths.h"
#include "Logger.h"
#include "Path.h"
#include "Platform.h"
#include "ReflectionSerialize.h"
#include "StringUtil.h"
#include "UnitTesting.h"

#if SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_LINUX
#	include <sys/stat.h>
#endif

namespace Seoul
{

#if SEOUL_UNIT_TESTS

static Bool SetReadOnlyBit(const String& sFilename, Bool bReadOnly)
{
#if SEOUL_PLATFORM_WINDOWS
	DWORD uAttributes = ::GetFileAttributesW(sFilename.WStr());
	if (bReadOnly)
	{
		uAttributes |= FILE_ATTRIBUTE_READONLY;
	}
	else
	{
		uAttributes &= ~FILE_ATTRIBUTE_READONLY;
	}

	return (FALSE != ::SetFileAttributesW(sFilename.WStr(), uAttributes));
#else // !SEOUL_PLATFORM_WINDOWS
	struct stat fileStat;
	memset(&fileStat, 0, sizeof(fileStat));
	if (0 != stat(sFilename.CStr(), &fileStat))
	{
		return false;
	}

	if (bReadOnly)
	{
		fileStat.st_mode &= ~(S_IWUSR | S_IWGRP | S_IWOTH);
	}
	else
	{
		fileStat.st_mode |= (S_IWUSR | S_IWGRP | S_IWOTH);
	}

	return (0 == chmod(sFilename.CStr(), fileStat.st_mode));
#endif // /!SEOUL_PLATFORM_WINDOWS
}

/**
 * @return A path that can't be written to.
 */
String GetNotWritableTempFileAbsoluteFilename()
{
	// Make the directory read-only for POSIX platforms, make a 0-length
	// read-only file for Windows.
	String const sTempDir(Path::Combine(Path::GetTempDirectory(), "NotWritableFolder"));
	String const sTempFile(Path::Combine(sTempDir, "SEOUL_TEMP_FILE0.tmp"));

	// If the temp file does not exist, create it.
	if (!DiskSyncFile::FileExists(sTempFile))
	{
		if (Directory::DirectoryExists(sTempDir))
		{
			// Make the path readable.
			SEOUL_VERIFY(SetReadOnlyBit(sTempDir, false));
		}
		else
		{
			// Make sure the temp directory exists.
			SEOUL_VERIFY(Directory::CreateDirPath(sTempDir));
		}

		// Create an empty file.
		{
			DiskSyncFile file(sTempFile, File::kWriteTruncate);
			SEOUL_VERIFY(file.CanWrite());
			file.Flush();
		}
	}

	// Set read-only permissions on the file.
	SEOUL_VERIFY(SetReadOnlyBit(sTempFile, true));

	// Set read-only permissions on the path.
	SEOUL_VERIFY(SetReadOnlyBit(sTempDir, true));

	// Create a file in the dir - doesn't matter, since it can't be written to.
	return sTempFile;
}

/**
 * Delete all temporary files created so far by this unit testing harness.
 */
void DeleteAllTempFiles()
{
	// Read-writable files.
	{
		String const sTempDir(Path::GetTempDirectory());

		Vector<String> vsFiles;
		if (Directory::GetDirectoryListing(sTempDir, vsFiles, false, false))
		{
			for (auto const& s : vsFiles)
			{
				(void)DiskSyncFile::DeleteFile(s);
			}
		}
	}

	// Read-only file.
	{
		String const sTempDir(Path::Combine(Path::GetTempDirectory(), "NotWritableFolder"));
		String const sTempFile(Path::Combine(sTempDir, "SEOUL_TEMP_FILE0.tmp"));

		(void)SetReadOnlyBit(sTempDir, false);
		(void)SetReadOnlyBit(sTempFile, false);

		(void)DiskSyncFile::DeleteFile(sTempFile);
	}
}

/** @return true if file a and file b are equal, false otherwise. Files are assumed to be on disk. */
Bool FilesAreEqual(const String& sFilenameA, const String& sFilenameB)
{
	void* pA = nullptr;
	UInt32 uA = 0u;
	if (FileManager::Get())
	{
		if (!FileManager::Get()->ReadAll(sFilenameA, pA, uA, 0u, MemoryBudgets::Developer))
		{
			return false;
		}
	}
	else if (!DiskSyncFile::ReadAll(sFilenameA, pA, uA, 0u, MemoryBudgets::Developer))
	{
		return false;
	}
	void* pB = nullptr;
	UInt32 uB = 0u;
	if (FileManager::Get())
	{
		if (!FileManager::Get()->ReadAll(sFilenameB, pB, uB, 0u, MemoryBudgets::Developer))
		{
			MemoryManager::Deallocate(pA);
			return false;
		}
	}
	else if (!DiskSyncFile::ReadAll(sFilenameB, pB, uB, 0u, MemoryBudgets::Developer))
	{
		MemoryManager::Deallocate(pA);
		return false;
	}

	Bool bReturn = true;
	bReturn = (uA == uB);
	bReturn = bReturn && (0 == memcmp(pA, pB, uB));

	MemoryManager::Deallocate(pB);
	MemoryManager::Deallocate(pA);
	return bReturn;
}

// Floating point 0.0, hidden, to suppress warnings about divide by
// zero in an explicit indeterminate test.
Float const kfUnitTestZeroConstant = 0.0f;

// Floating point max value, hidden, to suppress warnings about
// overflow.
Float const kfUnitTestMaxConstant = FloatMax;

Bool CopyFile(const String& sSourceFilename, const String& sDestinationFilename)
{
	void* p = nullptr;
	UInt32 u = 0u;
	if (FileManager::Get())
	{
		if (!FileManager::Get()->ReadAll(
			sSourceFilename,
			p,
			u,
			0u,
			MemoryBudgets::Developer))
		{
			return false;
		}
	}
	else if (!DiskSyncFile::ReadAll(
		sSourceFilename,
		p,
		u,
		0u,
		MemoryBudgets::Developer))
	{
		return false;
	}

	Bool bReturn = false;
	if (FileManager::Get())
	{
		bReturn = FileManager::Get()->WriteAll(
			sDestinationFilename,
			p,
			u);
	}
	else
	{
		bReturn = DiskSyncFile::WriteAll(
			sDestinationFilename,
			p,
			u);
	}

	MemoryManager::Deallocate(p);
	return bReturn;
}

static Bool TestFiles(const String& sA, const String& sB)
{
	void* pA = nullptr;
	UInt32 uA = 0u;
	if (FileManager::Get())
	{
		if (!FileManager::Get()->ReadAll(sA, pA, uA, 0u, MemoryBudgets::Developer))
		{
			SEOUL_LOG("Failed reading file A \"%s\" for identical test.", sA.CStr());
			return false;
		}
	}
	else if (!DiskSyncFile::ReadAll(sA, pA, uA, 0u, MemoryBudgets::Developer))
	{
		SEOUL_LOG("Failed reading file A \"%s\" for identical test.", sA.CStr());
		return false;
	}

	void* pB = nullptr;
	UInt32 uB = 0u;
	if (FileManager::Get())
	{
		if (!FileManager::Get()->ReadAll(sB, pB, uB, 0u, MemoryBudgets::Developer))
		{
			MemoryManager::Deallocate(pA);
			SEOUL_LOG("Failed reading file B \"%s\" for identical test.", sB.CStr());
			return false;
		}
	}
	else if (!DiskSyncFile::ReadAll(sB, pB, uB, 0u, MemoryBudgets::Developer))
	{
		MemoryManager::Deallocate(pA);
		SEOUL_LOG("Failed reading file B \"%s\" for identical test.", sB.CStr());
		return false;
	}

	if (uA != uB)
	{
		MemoryManager::Deallocate(pB);
		MemoryManager::Deallocate(pA);
		SEOUL_LOG("File A %s is %u bytes but file B %s is %u bytes.",
			sA.CStr(),
			uA,
			sB.CStr(),
			uB);
		return false;
	}

	auto const bReturn = (0 == memcmp(pA, pB, uA));
	MemoryManager::Deallocate(pB);
	MemoryManager::Deallocate(pA);

	if (!bReturn)
	{
		SEOUL_LOG("File A %s is not binary equal to file B %s.",
			sA.CStr(),
			sB.CStr());
		return false;
	}
	else
	{
		return true;
	}
}

Bool TestDirCountFiles(
	const String& sDir,
	const String& sExtension,
	Int32& riCount)
{
	// Early out check if directory does not exist.
	if (!Directory::DirectoryExists(sDir))
	{
		riCount = 0;
		return true;
	}

	Vector<String> vs;
	if (!Directory::GetDirectoryListing(sDir, vs, false, true, sExtension))
	{
		SEOUL_LOG("Failed listing directory A \"%s\" for file count test.", sDir.CStr());
		return false;
	}

	riCount = (Int32)vs.GetSize();
	return true;
}

Bool TestDirIdenticalRecursive(
	const String& sA,
	const String& sB,
	const String& sExtension,
	Int32 iExpectedFiles)
{
	Vector<String> vsA;
	if (!Directory::GetDirectoryListing(sA, vsA, false, true, sExtension))
	{
		SEOUL_LOG("Failed listing directory A \"%s\" for identical test.", sA.CStr());
		return false;
	}

	Vector<String> vsB;
	if (!Directory::GetDirectoryListing(sB, vsB, false, true, sExtension))
	{
		SEOUL_LOG("Failed listing directory B \"%s\" for identical test.", sB.CStr());
		return false;
	}

	if (vsA.GetSize() != vsB.GetSize())
	{
		SEOUL_LOG("Dir A %s has %u files but dir B %s has %u files.",
			sA.CStr(),
			vsA.GetSize(),
			sB.CStr(),
			vsB.GetSize());
		return false;
	}

	if (iExpectedFiles >= 0 && (UInt32)iExpectedFiles != vsA.GetSize())
	{
		SEOUL_LOG("Dir A %s and dir B %s have %u files but expected %d files.",
			sA.CStr(),
			sB.CStr(),
			vsA.GetSize(),
			iExpectedFiles);
		return false;
	}

	for (UInt32 i = 0u; i < vsA.GetSize(); ++i)
	{
		if (!TestFiles(vsA[i], vsB[i]))
		{
			return false;
		}
	}

	return true;
}

// General purpose converted for output unit test values.
String GenericUnitTestingToString(const Reflection::WeakAny& p)
{
	DataStore dataStore;
	dataStore.MakeArray();
	if (Reflection::SerializeObjectToArray(
		ContentKey(),
		dataStore,
		dataStore.GetRootNode(),
		0u,
		p))
	{
		DataNode node;
		SEOUL_VERIFY(dataStore.GetValueFromArray(dataStore.GetRootNode(), 0u, node));

		String sReturn;
		dataStore.ToString(node, sReturn, false, 0, true);

		if (sReturn.GetSize() > kMaxUnitTestPrintLength)
		{
			sReturn.ShortenTo(kMaxUnitTestPrintLength - 3u);
			sReturn += "...";
			return sReturn;
		}

		return sReturn;
	}
	else
	{
		return String::Printf("Unknown \"%s\"", p.GetType().GetName().CStr());
	}
}
String ToStringUtil<Byte const*>::ToString(Byte const* s)
{
	auto const u = StrLen(s);
	if (u > kMaxUnitTestPrintLength)
	{
		String sReturn(s, kMaxUnitTestPrintLength - 3u);
		sReturn += "...";
		return sReturn;
	}
	return s;
}

/** Convenience, directory to use for in flight unit test files during testing. */
String GetUnitTestingSaveDir()
{
	return Path::Combine(GamePaths::Get()->GetSaveDir(), "UnitTests");
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
