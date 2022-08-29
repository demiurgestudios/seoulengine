/**
 * \file Platform.h
 * \brief Handles inclusion of standard OS header files based
 * on the current platform. Can be viewed as the equivalent of
 * Prereqs.h when "polluting" platform-dependent functionality
 * is needed (e.g. Windows.h header).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef PLATFORM_H
#define PLATFORM_H

#include "StandardPlatformIncludes.h"

#if SEOUL_PLATFORM_WINDOWS
#	include <intrin.h>
#	include <windows.h>
#	ifndef GET_X_LPARAM
#		define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#	endif
#	ifndef GET_Y_LPARAM
#		define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))
#	endif
#	ifdef CopyFile
#		undef CopyFile
#	endif
#	ifdef DeleteFile
#		undef DeleteFile
#	endif
#	ifdef GetClassName
#		undef GetClassName
#	endif
#	ifdef GetObject
#		undef GetObject
#	endif
#	ifdef MoveFile
#		undef MoveFile
#	endif
#	ifdef RemoveDirectory
#		undef RemoveDirectory
#	endif
#	ifdef SendMessage
#		undef SendMessage
#	endif

#ifndef WM_DWMSENDICONICLIVEPREVIEWBITMAP
#define WM_DWMSENDICONICLIVEPREVIEWBITMAP 0x0326
#endif

#ifndef WM_DWMSENDICONICTHUMBNAIL
#define WM_DWMSENDICONICTHUMBNAIL 0x0323
#endif

#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
#	include <pthread.h>
#	include <sys/time.h>

inline void MillisecondsToTimespec(double fMilliseconds, timespec& rTimespec)
{
	time_t uSeconds = (time_t)(fMilliseconds / 1000.0);
	fMilliseconds -= (uSeconds * 1000.0);

	rTimespec.tv_sec = uSeconds;
	rTimespec.tv_nsec = (fMilliseconds * 1000000L);
}

inline void AddTimespec(const timespec& a, const timespec& b, timespec& rOut)
{
	rOut.tv_sec = (a.tv_sec + b.tv_sec);
	rOut.tv_nsec = (a.tv_nsec + b.tv_nsec);

	if (rOut.tv_nsec >= 1000000000L)
	{
		rOut.tv_sec++;
		rOut.tv_nsec -= 1000000000L;
	}
}
#else
#	error Unsupported platform.
#endif

#endif // include guard
