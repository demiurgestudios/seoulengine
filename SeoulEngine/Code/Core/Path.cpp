/**
 * \file Path.cpp
 * \brief Functions for manipulating file path strings in platform
 * independent ways.
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
#include "Mutex.h"
#include "Path.h"
#include "Platform.h"
#include "Prereqs.h"
#include "StringUtil.h"
#include "Vector.h"

#if SEOUL_PLATFORM_LINUX || SEOUL_PLATFORM_ANDROID
#include <unistd.h>
#endif // /#if SEOUL_PLATFORM_LINUX || SEOUL_PLATFORM_ANDROID

namespace Seoul::Path
{

/**
 * @return True if the current platform supports drive letter delimiters
 * (i.e. D:) or if all path rooting is achieved using directory delimiters.
 */
Bool PlatformSupportsDriveLetters()
{
#if SEOUL_PLATFORM_WINDOWS
	return true;
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
	return false;
#else
#error "Define for this platform."
	return false;
#endif
}

static const UniChar kWindowsDirectoryDelimiter = ':';
static const UniChar kExtensionDelimiter = '.';

// IMPORTANT: These need to be simple types - if they were
// static const of String, there would be no guarantee
// that they are initialized before other, complex static
// const variables that use Path:: functions.
static const char* kWhitespace = " \t\r\n\f";
static const char* kUpDelimiter = "..";
static const char* kDirectorySeparators = "/\\";

/** @return True if filenames on the current platform are case sensitive, false otherwise. */
Bool PlatformFileNamesAreCaseSensitive()
{
#if SEOUL_PLATFORM_WINDOWS
	return false;
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
	// TODO: Can be incorrect on Linux.
	return true;
#else
#	error "Define for this platform."
#endif
}

/**
 * @return The directory separator char for the current platform.
 */
const String& DirectorySeparatorChar()
{
	static const String ksReturn(kDirectorySeparatorChar);
	return ksReturn;
}

/**
 * @return A valid directory separator char that is not the standard
 * directory separator char for the current platform.
 */
const String& AltDirectorySeparatorChar()
{
	static const String ksReturn(kAltDirectorySeparatorChar);
	return ksReturn;
}

/**
 * @return A path that is the combination of sPathA and sPathB. If sPathA
 * is empty or sPathB is rooted, sPathB will be returned in normalized form.
 * Otherwise, sPathA will be combined with sPathB, and a directory separator
 * char will be added between the two if necessary. Both sPathA and sPathB
 * will be normalized before being combined in this case.
 */
String Combine(const String& sPathA, const String& sPathB)
{
	String const sNormalizedPathA = Normalize(sPathA);
	String const sNormalizedPathB = Normalize(sPathB);

	if (sNormalizedPathA.IsEmpty() || IsRooted(sNormalizedPathB))
	{
		return sNormalizedPathB;
	}
	else
	{
		const UInt lastCharOfA = (sNormalizedPathA.GetSize() - 1u);
		if (sNormalizedPathA[lastCharOfA] == kWindowsSeparator ||
			sNormalizedPathA[lastCharOfA] == kUnixSeparator)
		{
			return (sNormalizedPathA + sNormalizedPathB);
		}
		else
		{
			return (sNormalizedPathA + DirectorySeparatorChar() + sNormalizedPathB);
		}
	}
}

/**
 * @return The path string excluding the last directory separator
 * and anything following it. If the path does not contain a directory
 * separator, the empty string is returned.
 */
String GetDirectoryName(const String& sPath)
{
	UInt32 index = sPath.FindLastOf(kDirectorySeparators);
	if (index != String::NPos)
	{
		return sPath.Substring(0u, index);
	}
	else
	{
		return String();
	}
}

/** Recursive calls of GetExactPathName(). */
static String GetExactPathNameInternal(const String& sPath)
{
#if SEOUL_PLATFORM_WINDOWS
	// First check if the file or directory exists.
	{
		struct _stati64 statResults;
		memset(&statResults, 0, sizeof(statResults));
		if (0 != _wstati64(sPath.WStr(), &statResults))
		{
			// Normalize drive letters.
			Bool const bDriveLetter = (
				sPath.GetSize() >= 2u &&
				((sPath[0] >= 'A' && sPath[0] <= 'Z') || (sPath[0] >= 'a' && sPath[0] <= 'z')) &&
				sPath[1] == ':');

			// Done.
			if (bDriveLetter && (sPath[0] >= 'a' && sPath[0] <= 'z'))
			{
				// Convert to uppercase letter.
				auto sCopy(sPath);
				sCopy[0] += ('A' - 'a');
				return sCopy;
			}
			else
			{
				return sPath;
			}
		}
	}

	WIN32_FIND_DATAW findFileData;
	HANDLE hFindHandle = ::FindFirstFileW(
		sPath.WStr(),
		&findFileData);

	// On failure, return sPath unmodified.
	if (INVALID_HANDLE_VALUE == hFindHandle || nullptr == hFindHandle)
	{
		return sPath;
	}
	// Release the handle, and continue processing.
	else
	{
		SEOUL_VERIFY(FALSE != FindClose(hFindHandle));
	}

	// Get the new filename part.
	String const sExactFileName(WCharTToUTF8(findFileData.cFileName));

	// Get the parent name.
	String const sParent(GetDirectoryName(sPath));

	// Check if sParent is just a drive letter. If so, return it
	// with canonical casing (capital D).
	Bool const bDriveLetter = (
		sParent.GetSize() == 2u &&
		((sParent[0] >= 'A' && sParent[0] <= 'Z') || (sParent[0] >= 'a' && sParent[0] <= 'z')) &&
		sParent[1] == ':');
	String const sExactDirectory(bDriveLetter ? sParent.ToUpperASCII() : GetExactPathNameInternal(sParent));

	// The return is the found value, and the exact path name of
	// the remainder.
	String const sReturn(Combine(sExactDirectory, sExactFileName));

	return sReturn;
#else
	// Other platforms, the normalized path is the exact path, so just return it.
	return sPath;
#endif
}

/**
 * Attempts to convert the path to its canonical version. The path
 * must exist on disk, or the path is returned unmodified.
 */
String GetExactPathName(const String& sInPath)
{
	// First normalize the path.
	String sPath;
	if (!CombineAndSimplify(String(), sInPath, sPath))
	{
		return sInPath;
	}

	// Temporarily remove the trailing slash if present.
	if (sPath.EndsWith(DirectorySeparatorChar()))
	{
		return GetExactPathNameInternal(sPath.Substring(0u, sPath.GetSize() - 1)) + DirectorySeparatorChar();
	}
	// Otherwise, just run the path.
	else
	{
		return GetExactPathNameInternal(sPath);
	}
}

/**
 * @return The extension of the path including the '.'. If a '.' is not
 * found in the path string, or if a directory separator is found after
 * the '.' in the path string, then the empty string is returned.
 */
String GetExtension(const String& sPath)
{
	for (Int64 i = (Int64)(sPath.GetSize()) - 1; i >= 0; --i)
	{
		if (sPath[(UInt)i] == kWindowsSeparator || sPath[(UInt)i] == kUnixSeparator)
		{
			break;
		}
		else if (sPath[(UInt)i] == kExtensionDelimiter)
		{
			return sPath.Substring((UInt)i);
		}
	}

	return String();
}

/**
 * @return The path string excluding the last directory separator
 * and anything preceeding it. If the path does not contain a directory
 * separator, then the input path string is returned unchanged.
 */
String GetFileName(const String& sPath)
{
	UInt32 index = sPath.FindLastOf(kDirectorySeparators);
	if (index != String::NPos)
	{
		return sPath.Substring(index + 1u);
	}
	else
	{
		return sPath;
	}
}

/**
 * @return The string returned by GetFileName() without the last '.' and
 * anything following it, if the last '.' denotes a valid extension (see
 * the description of GetExtension()).
 */
String GetFileNameWithoutExtension(const String& sPath)
{
	String ret = GetFileName(sPath);
	for (Int64 i = (Int64)(ret.GetSize()) - 1; i >= 0; --i)
	{
		if (ret[(UInt)i] == kWindowsSeparator ||
			ret[(UInt)i] == kUnixSeparator)
		{
			break;
		}
		else if (ret[(UInt)i] == kExtensionDelimiter)
		{
			return ret.Substring(0u, (UInt)i);
		}
	}

	return ret;
}

/**
 * @return The string sPath without the last '.' and anything
 * following it, if the last '.' denotes a valid extension (see
 * the description of GetExtension()).
 */
String GetPathWithoutExtension(const String& sPath)
{
	for (Int64 i = (Int64)(sPath.GetSize()) - 1; i >= 0; --i)
	{
		if (sPath[(UInt)i] == kWindowsSeparator ||
			sPath[(UInt)i] == kUnixSeparator)
		{
			break;
		}
		else if (sPath[(UInt)i] == kExtensionDelimiter)
		{
			return sPath.Substring(0u, (UInt)i);
		}
	}

	return sPath;
}

/**
 * @return true if the path is an absolute path, false otherwise.
 * Note that IsRooted(), unlike other functions in Path::, will
 * ignore leading whitespace in the path. For example, IsRooted() will return
 * true for both "C:\" and "   C:\".
 */
Bool IsRooted(const String& sPath)
{
	UInt32 startIndex = sPath.FindFirstNotOf(kWhitespace);
	if (startIndex != String::NPos)
	{
		if (sPath[startIndex] == kWindowsSeparator ||
			sPath[startIndex] == kUnixSeparator)
		{
			return true;
		}
		else if (startIndex + 1u < sPath.GetSize())
		{
			UInt32 directoryDelimiterIndex = sPath.Find(kWindowsDirectoryDelimiter);
			if (directoryDelimiterIndex != String::NPos)
			{
				for (UInt32 i = 0u; i < directoryDelimiterIndex; ++i)
				{
					if (sPath[(UInt)i] == kWindowsSeparator ||
						sPath[(UInt)i] == kUnixSeparator)
					{
						return false;
					}
				}

				return true;
			}
		}
	}

	return false;
}

/**
 * @return A normalized version of the specified path. A normalized path will
 * obey the following:
 * - trailing and leading whitespace is removed.
 * - directory separator characters are converted to the current platform.
 */
String Normalize(const String& sPath)
{
	String ret = TrimWhiteSpace(sPath);
	ret = ret.ReplaceAll(AltDirectorySeparatorChar(), DirectorySeparatorChar());
	return ret;
}

/**
 * @return true if the paths can be combined and simplified, false otherwise.
 * CombineAndSimplify performs Combine(), except that it attempts to remove
 * any .. delimiters while it is combining paths. It will also
 * Normalize() the path.
 */
Bool CombineAndSimplify(const String& sPathA, const String& sPathB, String& rOut)
{
	String const sNormalizedPathA = Normalize(sPathA);
	String const sNormalizedPathB = Normalize(sPathB);
	String ret = Combine(sNormalizedPathA, sNormalizedPathB);

	// Remove any "./" (or ".\") in the path. Do this first so that ..
	// processing is not confused.
	ret = ret.ReplaceAll(DirectorySeparatorChar() + String(".") + DirectorySeparatorChar(), DirectorySeparatorChar());
	if (ret.StartsWith(String(".") + DirectorySeparatorChar()))
	{
		ret = ret.Substring(2u);
	}

	UInt nextDoubleDot = ret.Find(kUpDelimiter);
	while (nextDoubleDot != String::NPos)
	{
		UInt afterDotPosition = ret.FindFirstNotOf(DirectorySeparatorChar(), nextDoubleDot + 2u);
		if (afterDotPosition != String::NPos)
		{
			bool inPrevDirectory = false;
			bool bFound = false;
			for (Int64 i = ((Int64)nextDoubleDot) - 1; i >= 0; --i)
			{
				if (!inPrevDirectory)
				{
					if (ret[(UInt)i] != DirectorySeparatorChar()[0])
					{
						inPrevDirectory = true;
					}
				}
				else if (ret[(UInt)i] == DirectorySeparatorChar()[0])
				{
					bFound = true;
					ret = ret.Substring(0u, (UInt)i + 1u) + ret.Substring(afterDotPosition);
					break;
				}
			}

			if (!bFound)
			{
				return false;
			}
		}
		else
		{
			return false;
		}

		nextDoubleDot = ret.Find(kUpDelimiter);
	}

	// Also remove the leading / or \ if we're left with it and the original
	// sPathA didn't start with it.
	if (!ret.IsEmpty() &&
		ret[0u] == DirectorySeparatorChar()[0u] &&
		!sNormalizedPathA.IsEmpty() &&
		(sNormalizedPathA[0u] != DirectorySeparatorChar()[0u] &&
		sNormalizedPathA[0u] != AltDirectorySeparatorChar()[0u]))
	{
		if (!(ret.GetSize() > 1u) || ret[1u] != DirectorySeparatorChar()[0u])
		{
			ret = ret.Substring(1u);
		}
	}

	rOut = ret;
	return true;
}

#if SEOUL_PLATFORM_ANDROID
String AndroidGetCacheDir();
#elif SEOUL_PLATFORM_IOS
String IOSGetProcessPath();
String IOSGetTempPath();
#endif // /#if SEOUL_PLATFORM_IOS

/**
 * @return the absolute path to the directory of the current process
 * (not the current directory, but the directory that is the location
 * of the process binary).
 */
String GetProcessDirectory()
{
	return GetDirectoryName(GetProcessPath());
}

// Return the absolute path to the current process.
String GetProcessPath()
{
	// PC, use GetModuleFileNameW() to get the process path.
#if SEOUL_PLATFORM_WINDOWS
	Vector<WCHAR, MemoryBudgets::Io> v(MAX_PATH);

	// Size == result size means we didn't succeed, so need to increase the buffer size.
	auto uResult = ::GetModuleFileNameW(nullptr, v.Data(), v.GetSize());
	while (v.GetSize() <= uResult)
	{
		v.Resize(v.GetSize() * 2u);
		uResult = ::GetModuleFileNameW(nullptr, v.Data(), v.GetSize());
	}

	// Handle the failure case.
	if (0u == uResult)
	{
		return String();
	}
	else
	{
		return WCharTToUTF8(v.Data());
	}
#elif SEOUL_PLATFORM_IOS
	return IOSGetProcessPath();
#elif SEOUL_PLATFORM_LINUX || SEOUL_PLATFORM_ANDROID
	Vector<Byte, MemoryBudgets::Io> v(128u);

	// Size >= v.GetSize() means output was truncated,
	// so double size and try again.
	auto iResult = readlink("/proc/self/exe", v.Data(), v.GetSize());
	while (iResult >= 0 && v.GetSize() <= (UInt32)iResult)
	{
		v.Resize(v.GetSize() * 2u);
		iResult = readlink("/proc/self/exe", v.Data(), v.GetSize());
	}

	// Handle the failure case.
	if (iResult < 0)
	{
		return String();
	}
	else
	{
		return String(v.Data(), (UInt)iResult);
	}
#else
#	error "Define for this platform."
#endif
}

static Atomic32Value<Bool> s_bHasTempDir;
static String s_sTempDir;
static Mutex s_TempDirMutex;

/** @return The temporary directory for this platform. */
String GetTempDirectory()
{
	// Cheap early out case.
	if (s_bHasTempDir)
	{
		return s_sTempDir;
	}

	String sTempDir;

	// Windows - use a folder next to the process path. This is a very Demiurge
	// choice, assuming that the process will be in a Perforce folder and will
	// be near its data files. As such, %TEMP% is a bad choice - it is generally
	// on another drive (causing move/rename operations to turn into copies) and
	// also generally won't be part of anti-virus filters that will likely otherwise
	// apply to the process path.
#if SEOUL_PLATFORM_WINDOWS
	// Get the process path - if this fails, use the %TEMP% directory.
	auto const sProcessPath = GetProcessPath();
	if (!sProcessPath.IsEmpty())
	{
		sTempDir = Path::GetExactPathName(Path::Combine(Path::GetDirectoryName(sProcessPath), "SeoulTmp"));
	}
	// Use %TEMP% directory.
	else
	{
		WCHAR aPathBuffer[MAX_PATH];
		DWORD zSizeInCharacters = ::GetTempPathW(MAX_PATH, aPathBuffer);
		(void)zSizeInCharacters;

		// Sanity check results.
		SEOUL_ASSERT(0u != zSizeInCharacters && zSizeInCharacters < MAX_PATH);

		// Place our temp files in a sub folder, so it isn't huge, and so
		// any file listing queries don't enumerate a huge set of files.
		//
		// Need GetExactPathName to resolve short vs. long names, consistently.
		sTempDir = Path::GetExactPathName(Path::Combine(WCharTToUTF8(aPathBuffer), "SeoulTmp"));
	}
	// TODO: Fine for development, knowing Android, this will break
	// at some point, so probably not find if we ever need temp files for
	// shipping applications.
#elif SEOUL_PLATFORM_ANDROID
	{
		auto const sCacheDir(AndroidGetCacheDir());
		if (sCacheDir.IsEmpty())
		{
			sTempDir = "/data/local/tmp/SeoulTmp";
		}
		else
		{
			sTempDir = Path::Combine(sCacheDir, "SeoulTmp");
		}
	}

	// Linux, use absolute /tmp path.
#elif SEOUL_PLATFORM_LINUX
	sTempDir = "/tmp/SeoulTmp";

	// iOS, use NSTemporaryDirectory()
#elif SEOUL_PLATFORM_IOS
	sTempDir = Path::Combine(IOSGetTempPath(), "SeoulTmp");

	// TODO: Use a better method for this.
	// Other platforms, use the default path in a special sub folder.
#else
	// Otherwise, just a folder in our data path.
	sTempDir = Path::Combine(DEFAULT_PATH, "SeoulTmp");
#endif

	// Synchronize, recheck, and then commit.
	{
		Lock lock(s_TempDirMutex);
		if (!s_bHasTempDir)
		{
			s_sTempDir = sTempDir;
			s_bHasTempDir = true;

			// Make sure the temp directory exists.
			(void)Directory::CreateDirPath(sTempDir);
		}
	}

	return s_sTempDir;
}

/** Counter utility for temporary file generation via GetTempFileAbsoluteFilename(). */
static Atomic32 s_TempFileSuffix;

/**
* @return True if a temp filename could be generated for the current
* platform, false otherwise. If this method returns true, rsFilename
* will contain the temp filename, otherwise it will be left unchanged.
*/
String GetTempFileAbsoluteFilename()
{
	// Get path to the temporary directory.
	auto const sTempDir(GetTempDirectory());

	// Loop until success.
	while (true)
	{
		// Generate a suffix.
		Atomic32Type const suffix = (++s_TempFileSuffix) - 1;

		// Now create a full path.
		String const sReturn(Combine(sTempDir, String::Printf("SEOUL_TEMP_FILE%05u.tmp", (UInt32)suffix)));

		// Skip this file if it already exists on disk.
		if (!DiskSyncFile::FileExists(sReturn))
		{
			return sReturn;
		}
	}
}

} // namespace Seoul::Path
