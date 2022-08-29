/**
 * \file Logger.h
 * \brief Internal header file included by Logger.cpp, do not include in other
 * header files.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#if !defined(LOGGER_H)
#error "Internal header file, must only be included by Logger.cpp"
#endif

#include "Platform.h"
#include "SeoulTime.h"

#if SEOUL_PLATFORM_WINDOWS
#include <share.h>  // For _SH_DENYWR constant
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
#include <unistd.h>
#endif

namespace Seoul::LoggerDetail
{

/**
 * Platform dependent implementation for creating a hard link.
 * May be a NOP on some platforms.
 */
inline void CreateHardLink(
	const String& sHardLinkFilename,
	const String& sFileToLinkTo)
{
#if SEOUL_PLATFORM_WINDOWS
	// Make a hard link from sFilename to sActualFilename, removing any previous
	// hard link, so that if we want to check the log file, we don't have to
	// spend time figuring out which log file is the most recent
	BOOL bResult = ::DeleteFileW(sHardLinkFilename.WStr());
	if (FALSE == bResult)
	{
		DWORD uExtendedError = ::GetLastError();

		// Allow the file not found error as a success condition.
		if (ERROR_SUCCESS == uExtendedError ||
			ERROR_FILE_NOT_FOUND == uExtendedError)
		{
			bResult = TRUE;
		}
	}

	if (FALSE != bResult)
	{
		(void)::CreateHardLinkW(
			sHardLinkFilename.WStr(),
			sFileToLinkTo.WStr(),
			nullptr);
	}
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
	bool bResult = (0 == unlink(sHardLinkFilename.CStr()));
	if (!bResult)
	{
		if (ENOENT == errno)
		{
			bResult = true;
		}
	}

	if (bResult)
	{
		(void)link(sFileToLinkTo.CStr(), sHardLinkFilename.CStr());
	}
#else
#error "Define for this platform."
#endif
}

/**
 * Platform dependent implementation for opening
 * the log stream at file sFilename. rStream will be nullptr
 * if this operation fails.
 *
 * \pre rpStream must be nullptr before calling this method.
 */
inline void OpenLogStream(const String& sFilename, FILE*& rpStream)
{
#if SEOUL_PLATFORM_WINDOWS
	// Allow shared reading but not shared writing
	rpStream = _wfsopen(
		sFilename.WStr(),
		L"wb",
		_SH_DENYWR);
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
	rpStream = fopen(sFilename.CStr(), "wb");
#else
#error "Define for this platform."
#endif

}

} // namespace Seoul::LoggerDetail
