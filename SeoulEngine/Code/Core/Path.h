/**
 * \file Path.h
 * \brief Functions for manipulating file path strings in platform
 * independent ways.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef PATH_H
#define PATH_H

#include "SeoulString.h"

namespace Seoul
{

/**
 * Path functions provide string manipulation functions for operating
 * on file paths. Note that (almost) all Path functions are conservative, in
 * that they perform the minimum number of operations necessary to achieve
 * their description. For example, Combine() will check for the presence or
 * lack of a trailing directory separator char in sPathA, but it will not
 * ignore whitespace or normalize the directory separators of the path to the
 * current platform. As a result, if you want to minimize the chance that an
 * operation will produce erroneous results, you should Normalize() a path
 * before further processing.
 */
namespace Path
{

static const UniChar kWindowsSeparator = '\\';
static const UniChar kUnixSeparator = '/';

#if SEOUL_PLATFORM_WINDOWS
static const UniChar kDirectorySeparatorChar = kWindowsSeparator;
static const UniChar kAltDirectorySeparatorChar = kUnixSeparator;
#define SEOUL_DIR_SEPARATOR "\\"
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
static const UniChar kDirectorySeparatorChar = kUnixSeparator;
static const UniChar kAltDirectorySeparatorChar = kWindowsSeparator;
#define SEOUL_DIR_SEPARATOR "/"
#else
#error "Define for this platform."
#endif

static inline UniChar GetDirectorySeparatorChar(Platform ePlatform)
{
	switch (ePlatform)
	{
	case Platform::kPC: // fall-through
		return kWindowsSeparator;
	default:
		return kUnixSeparator;
	};
}

Bool PlatformFileNamesAreCaseSensitive();

Bool PlatformSupportsDriveLetters();

const String& DirectorySeparatorChar();

const String& AltDirectorySeparatorChar();

String Combine(const String& sPathA, const String& sPathB);

/**
 * @return A path that is the combination of sPathA, sPathB, and sPathC.
 */
inline String Combine(
	const String& sPathA,
	const String& sPathB,
	const String& sPathC)
{
	return Combine(Combine(sPathA, sPathB), sPathC);
}

/**
 * @return A path that is the combination of sPathA, sPathB, sPathC, and sPathD.
 */
inline String Combine(
	const String& sPathA,
	const String& sPathB,
	const String& sPathC,
	const String& sPathD)
{
	return Combine(Combine(Combine(sPathA, sPathB), sPathC), sPathD);
}

/**
 * @return A path that is the combination of sPathA, sPathB, sPathC, sPathD,
 * and sPathE.
 */
inline String Combine(
	const String& sPathA,
	const String& sPathB,
	const String& sPathC,
	const String& sPathD,
	const String& sPathE)
{
	return Combine(Combine(Combine(Combine(sPathA, sPathB), sPathC), sPathD), sPathE);
}

// Removes the last part of sPath, leaving (what must always be) the
// containing directory of that part (e.g. D:\Test\It will be converted
// to D:\Test). 
String GetDirectoryName(const String& sPath);

// Convenience variation of GetDirectoryName() that repeats the operation
// n times, stripping multiple elements from the input path.
inline static String GetDirectoryName(const String& sPath, Int32 n)
{
	String sReturn(sPath);
	while (n-- > 0)
	{
		sReturn = GetDirectoryName(sReturn);
	}

	return sReturn;
}

// Attempts to convert the path to its canonical version. The path
// must exist on disk, or the path is returned unmodified.
//
// WARNING: This function can be expensive, as it accessed OS
// routines and checks file status on disk. Not recommended
// as part of a normal path handling flow.
String GetExactPathName(const String& sPath);

String GetExtension(const String& sPath);

String GetFileName(const String& sPath);

String GetFileNameWithoutExtension(const String& sPath);

String GetPathWithoutExtension(const String& sPath);

/**
 * @return true if the string returned by GetExtension() is not the empty
 * string, false otherwise.
 */
inline Bool HasExtension(const String& sPath)
{
	return (!GetExtension(sPath).IsEmpty());
}

/**
 * @return True if the path sPath ends with a directory
 * separator character, false otherwise.
 */
inline Bool HasTrailingDirectorySeparator(const String& sPath)
{
	return (!sPath.IsEmpty() &&
			(sPath[sPath.GetSize() - 1] == DirectorySeparatorChar()[0] ||
			 sPath[sPath.GetSize() - 1] == AltDirectorySeparatorChar()[0]));
}

Bool IsRooted(const String& sPath);

String Normalize(const String& sPath);

Bool CombineAndSimplify(const String& sPathA, const String& sPathB, String& rsOut);

/**
 * @return The string sPath with any existing extension removed and the
 * string sNewExtension appended.
 *
 * \pre sNewExtension is expected to be a valid extension string including
 * the leading '.'.
 */
inline String ReplaceExtension(const String& sPath, const String& sNewExtension)
{
	return GetPathWithoutExtension(sPath) + sNewExtension;
}

// Return the absolute path to the directory of the current process
// (not the current directory, but the directory that is the location
// of the process binary).
String GetProcessDirectory();

// Return the absolute path to the current process.
String GetProcessPath();

// Return the current platform's root folder for temporary files.
String GetTempDirectory();

// Return a path to a file (already created as 0 size) in the
// current platform's temp directory.
String GetTempFileAbsoluteFilename();

} // namespace Path

} // namespace Seoul

#endif // include guard
