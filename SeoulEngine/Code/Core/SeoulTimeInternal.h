/**
 * \file SeoulTimeInternal.h
 * \brief Internal header file used by SeoulTime.cpp, do not include
 * in other files.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#if !defined(SEOUL_TIME_H)
#error "Internal header file, must only be included by SeoulTime.cpp"
#endif

#include "Platform.h"

#if SEOUL_PLATFORM_IOS
#include <mach/mach_time.h>
#endif

namespace Seoul
{

#if SEOUL_PLATFORM_WINDOWS
struct SeoulTimeImpl
{
	SeoulTimeImpl()
		: m_MillisecondsToCounter(0)
		, m_fCounterToMilliseconds(0.0)
		, m_GameStartTick(0)
	{
		LARGE_INTEGER frequency;
		SEOUL_VERIFY(FALSE != QueryPerformanceFrequency(&frequency));

		m_MillisecondsToCounter = (LONGLONG)(frequency.QuadPart / 1000.0);
		m_fCounterToMilliseconds = (1000.0 / frequency.QuadPart);
	}

	LONGLONG m_MillisecondsToCounter;
	Double m_fCounterToMilliseconds;
	LONGLONG m_GameStartTick;
};
#elif SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
struct SeoulTimeImpl
{
	SeoulTimeImpl()
		: m_GameStartTick(0)
	{
	}

	Int64 m_GameStartTick;
};
#elif SEOUL_PLATFORM_IOS
struct SeoulTimeImpl
{
	SeoulTimeImpl()
		: m_fMillisecondsToCounter(0.0)
		, m_fCounterToMilliseconds(0.0)
		, m_GameStartTick(0u)
	{
		mach_timebase_info_data_t info;
		memset(&info, 0, sizeof(info));

		SEOUL_VERIFY(0 == mach_timebase_info(&info));

		m_fCounterToMilliseconds = ((Double)info.numer / (Double)info.denom) * 1e-6;
		m_fMillisecondsToCounter = (1.0 / m_fCounterToMilliseconds);
	}

	Double m_fMillisecondsToCounter;
	Double m_fCounterToMilliseconds;
	UInt64 m_GameStartTick;
};
#else
#error "Define for this platform."
#endif

namespace SeoulTimeInternal
{

#if SEOUL_PLATFORM_WINDOWS
inline void MarkGameStartTick(SeoulTimeImpl& rImpl)
{
	LARGE_INTEGER counter;
	SEOUL_VERIFY(FALSE != QueryPerformanceCounter(&counter));
	rImpl.m_GameStartTick = counter.QuadPart;
}

inline Int64 ConvertMillisecondsToTicks(const SeoulTimeImpl& impl, Double fMilliseconds)
{
	return (Int64)(impl.m_MillisecondsToCounter * fMilliseconds);
}

inline Double ConvertTicksToMilliseconds(const SeoulTimeImpl& impl, Int64 iTicks)
{
	return impl.m_fCounterToMilliseconds * iTicks;
}

inline Int64 GetGameStartTick(const SeoulTimeImpl& impl)
{
	return impl.m_GameStartTick;
}

inline Int64 GetCurrentTick()
{
	LARGE_INTEGER counter;
	SEOUL_VERIFY(FALSE != QueryPerformanceCounter(&counter));

	return counter.QuadPart;
}
#elif SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
inline Int64 GetCurrentTick()
{
	struct timespec currentTimeSpec;
	memset(&currentTimeSpec, 0, sizeof(currentTimeSpec));

	SEOUL_VERIFY(0 == clock_gettime(CLOCK_MONOTONIC, &currentTimeSpec));

	static const Int64 kFactor = (Int64)1000000000;
	Int64 const iSeconds = (Int64)currentTimeSpec.tv_sec;
	Int64 const iNanoSeconds = (Int64)currentTimeSpec.tv_nsec;

	return (iSeconds * kFactor + iNanoSeconds);
}

inline void MarkGameStartTick(SeoulTimeImpl& impl)
{
	impl.m_GameStartTick = GetCurrentTick();
}

inline Int64 ConvertMillisecondsToTicks(const SeoulTimeImpl& /*impl*/, Double fMilliseconds)
{
	return (Int64)(fMilliseconds * 1000000.0);
}

inline Double ConvertTicksToMilliseconds(const SeoulTimeImpl& /*impl*/, Int64 iTicks)
{
	return (Double)iTicks / 1000000.0;
}

inline Int64 GetGameStartTick(const SeoulTimeImpl& impl)
{
	return impl.m_GameStartTick;
}
#elif SEOUL_PLATFORM_IOS
inline void MarkGameStartTick(SeoulTimeImpl& rImpl)
{
	rImpl.m_GameStartTick = mach_absolute_time();
}

inline Int64 ConvertMillisecondsToTicks(const SeoulTimeImpl& impl, Double fMilliseconds)
{
	return (Int64)(impl.m_fMillisecondsToCounter * fMilliseconds);
}

inline Double ConvertTicksToMilliseconds(const SeoulTimeImpl& impl, Int64 iTicks)
{
	return impl.m_fCounterToMilliseconds * iTicks;
}

inline Int64 GetGameStartTick(const SeoulTimeImpl& impl)
{
	return (Int64)impl.m_GameStartTick;
}

inline Int64 GetCurrentTick()
{
	UInt64 counter = mach_absolute_time();

	return (Int64)counter;
}
#else
#error "Define for this platform."
#endif

#if SEOUL_PLATFORM_WINDOWS
inline TimeValue GetCurrentTimeOfDay()
{
	// Adjustment of Win32 system time in microseconds to Unix epoch time.
	static const Int64 kDeltaEpochInMicroseconds = 11644473600000000Ui64;

	// Get the current system time as a file time.
	FILETIME fileTime;
	memset(&fileTime, 0, sizeof(fileTime));
	::GetSystemTimeAsFileTime(&fileTime);

	// Convert the time into microseconds.
	Int64 iTime = fileTime.dwHighDateTime;
	iTime <<= 32;
	iTime |= fileTime.dwLowDateTime;
	iTime /= 10;

	// Adjust to Unix epoch.
	iTime -= kDeltaEpochInMicroseconds;

	// Convert the total time in microseconds to a seconds/microseconds pair.
	TimeValue ret;
	ret.tv_sec = (iTime / 1000000);
	ret.tv_usec = (Int32)(iTime % 1000000);

	return ret;
}
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
inline TimeValue GetCurrentTimeOfDay()
{
	// FIXME: Support dates after 2038-01-19 03:14:07 UTC
	struct timeval tv;
	(void)gettimeofday(&tv, nullptr);

	// Copy from platform-specific timeval to platform-agnostic TimeValue
	TimeValue ret;
	ret.tv_sec = tv.tv_sec;
	ret.tv_usec = tv.tv_usec;

	return ret;
}
#else
#error "Define for this platform."
#endif

} // namespace SeoulTimeInternal

} // namespace Seoul
