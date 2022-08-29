/**
 * \file SeoulTime.cpp
 * \brief High resolution timing functionality.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include <stdlib.h>
#include <time.h>

#include "HashFunctions.h"
#include "Mutex.h"
#include "SeoulMath.h"
#include "SeoulString.h"
#include "SeoulTime.h"
#include "SeoulTimeInternal.h"

namespace Seoul
{

/** From http://stackoverflow.com/a/32158604 */
static inline Int32 ToDaysGregorian(Int32 iYear, UInt32 uMonth, UInt32 uDay)
{
	iYear -= uMonth <= 2;
	Int32 const iEra = (iYear >= 0 ? iYear : (iYear - 399)) / 400;
	UInt32 const uYoe = (UInt32)(iYear - (iEra * 400)); // [0, 399]
	UInt32 const uDoy = ((153 * (uMonth + (uMonth > 2 ? -3 : 9)) + 2) / 5) + uDay - 1; // [0, 365]
	UInt32 const uDoe = uYoe * 365 + uYoe/4 - uYoe/100 + uDoy; // [0, 146096]
	return iEra * 146097 + ((Int32)uDoe) - 719468;
}

/** From http://stackoverflow.com/a/32158604 */
static inline void FromDaysGregorian(Int32 iDaysSince1970, Int32& riYear, UInt32& ruMonth, UInt32& ruDay)
{
	iDaysSince1970 += 719468;
	Int32 const iEra = (iDaysSince1970 >= 0 ? iDaysSince1970 : (iDaysSince1970 - 146096)) / 146097;
	UInt32 const uDoe = (UInt32)(iDaysSince1970 - (iEra * 146097)); // [0, 146096]
	UInt32 const uYoe = (uDoe - (uDoe / 1460) + (uDoe / 36524) - (uDoe / 146096)) / 365; // [0, 399]
	Int32 const iY = ((Int32)uYoe) + (iEra * 400);
	UInt32 const uDoy = uDoe - ((365 * uYoe) + (uYoe / 4) - (uYoe / 100)); // [0, 365]
	UInt32 const uMp = ((5 * uDoy) + 2) / 153; // [0, 11]
	UInt32 const uD = uDoy - (((153 * uMp) + 2) / 5) + 1; // [1, 31]
	UInt32 const uM = uMp + (uMp < 10 ? 3 : -9); // [1, 12]

	riYear = (iY + (uM <= 2));
	ruMonth = uM;
	ruDay = uD;
}

/**
 * Set the start of "game" time. All calls to GetGameTimeInMilliseconds()
 * will be relative to this time.
 */
void SeoulTime::MarkGameStartTick()
{
	SeoulTimeInternal::MarkGameStartTick(GetImpl());
}

/**
 * @return The elapsed time in ticks since a call to MarkGameStartTick().
 *
 * If MarkGameStartTick() has not been called, this value will
 * be relative to a platform dependent start point when the global system
 * clock was 0.
 */
Int64 SeoulTime::GetGameTimeInTicks()
{
	return SeoulTimeInternal::GetCurrentTick() -
		SeoulTimeInternal::GetGameStartTick(GetImpl());
}

/**
 * @return The elapsed time in microseconds since a call to MarkGameStartTick().
 *
 * If MarkGameStartTick() has not been called, this value will
 * be relative to a platform dependent start point when the global system
 * clock was 0.
 */
Double SeoulTime::GetGameTimeInMicroseconds()
{
	return ConvertTicksToMicroseconds(SeoulTimeInternal::GetCurrentTick() - SeoulTimeInternal::GetGameStartTick(GetImpl()));
}

/**
 * @return The elapsed time in milliseconds since a call to MarkGameStartTick().
 *
 * If MarkGameStartTick() has not been called, this value will
 * be relative to a platform dependent start point when the global system
 * clock was 0.
 */
Double SeoulTime::GetGameTimeInMilliseconds()
{
	return SeoulTimeInternal::ConvertTicksToMilliseconds(GetImpl(),
		SeoulTimeInternal::GetCurrentTick() -
		SeoulTimeInternal::GetGameStartTick(GetImpl()));
}

/**
 * @return The time in milliseconds fMilliseconds converted
 * to a value in ticks.
 */
Int64 SeoulTime::ConvertMillisecondsToTicks(Double fMilliseconds)
{
	return SeoulTimeInternal::ConvertMillisecondsToTicks(GetImpl(), fMilliseconds);
}

/**
 * @return The time in ticks iTicks converted
 * to milliseconds.
 */
Double SeoulTime::ConvertTicksToMilliseconds(Int64 iTicks)
{
	return SeoulTimeInternal::ConvertTicksToMilliseconds(GetImpl(), iTicks);
}

SeoulTime::SeoulTime()
	: m_iStartTick(0)
	, m_iStopTick(0)
{
}

SeoulTime::~SeoulTime()
{
}

/**
 * Start this SeoulTime object, marking a start tick
 * and resetting the stop tick to the start tick.
 */
void SeoulTime::StartTimer()
{
	m_iStartTick = SeoulTimeInternal::GetCurrentTick();
	m_iStopTick = m_iStartTick;
}

/**
 * Stop this SeoulTime object, marking a stop tick.
 *
 * This method should be called after a call to StartTimer()
 * for methods which return elapsed time values to be valid.
 */
void SeoulTime::StopTimer()
{
	m_iStopTick = SeoulTimeInternal::GetCurrentTick();
}

/**
 * Stops the timer, records the split time, and restarts the timer
 *
 * This is equivalent to calling StartTimer(), GetElapsedTimeInSeconds(), and
 * StartTimer() but is more efficient.
 */
Double SeoulTime::SplitTimerSeconds()
{
	return SplitTimerMilliseconds() / 1000.0;
}

/**
 * Stops the timer, records the split time, and restarts the timer
 *
 * This is equivalent to calling StartTimer(), GetElapsedTimeInMilliseconds(),
 * and StartTimer() but is more efficient.
 */
Double SeoulTime::SplitTimerMilliseconds()
{
	StopTimer();
	Double dElapsedMs = GetElapsedTimeInMilliseconds();
	m_iStartTick = m_iStopTick;

	return dElapsedMs;
}

/**
 * Time that elapsed between calls to StartTimer() and StopTimer()
 * in seconds.
 */
Double SeoulTime::GetElapsedTimeInSeconds() const
{
	return GetElapsedTimeInMilliseconds() / 1000.0;
}

/**
 * Time that elapsed between calls to StartTimer() and StopTimer()
 * in milliseconds.
 */
Double SeoulTime::GetElapsedTimeInMilliseconds() const
{
	return SeoulTimeInternal::ConvertTicksToMilliseconds(GetImpl(), GetElapsedTicks());
}

/**
 * Platform dependent internal data used by all timers.
 */
SeoulTimeImpl& SeoulTime::GetImpl()
{
	static SeoulTimeImpl s_Impl;
	return s_Impl;
}

/**
 * Get the current UTC time (accurate to 1 microsecond)
 */
WorldTime WorldTime::GetUTCTime()
{
	WorldTime ret;
	ret.m_Time = SeoulTimeInternal::GetCurrentTimeOfDay();
	return ret;
}

// Returns the length of the value, and writes the result into *pResult
UInt ParseInt32(const Byte* pStr, UInt uFieldMaxWidth, UInt uStrLen, Int32* pResult)
{
	// If uFieldMaxWidth is not UIntMax, stop consuming characters after min(uStrLen, uFieldMaxWidth)
	const Byte* pMaxEndPtr = pStr + uStrLen;
	if (uFieldMaxWidth != UIntMax && uFieldMaxWidth < uStrLen)
	{
		pMaxEndPtr = pStr + uFieldMaxWidth;
	}

	const Byte* pEndPtr;
	Int32 iValue = 0;
	// While less than the defined limit for the field (if there is no limit don't check) and the end pointer doesn't go past the end of the string.
	for (pEndPtr = pStr; pEndPtr < pMaxEndPtr; pEndPtr++)
	{
		SEOUL_STATIC_ASSERT('9' == ('0' + 9));
		Int32 digit = *pEndPtr - '0';
		if (digit < 0 || digit > 9)
		{
			// Not a digit.
			break;
		}

		// Update the field's value
		iValue = 10 * iValue + digit;
	}

	if (pEndPtr != pStr)
	{
		*pResult = iValue;
	}

	return (UInt)(pEndPtr - pStr);
}

/**
 * Parses an ISO 8601-formatted date+time string into a WorldTime instance.
 * The allowable input strings match the following pattern:
 *
 * YYYY-MM-DDTHH:MM:SS+ZZ:ZZ
 * YYYY-MM-DDTHH:MM:SS.s+ZZ:ZZ
 * YYYY-MM-DDTHH:MM:SS,s+ZZ:ZZ
 *
 * If an error occurs in parsing, WorldTime() is returned.
 */
WorldTime WorldTime::ParseISO8601DateTime(const String& sDateTime)
{
	Int32 nYear = 0;
	Int32 nMonth = 0;
	Int32 nDay = 0;
	Int32 nHours = 0;
	Int32 nMinutes = 0;
	Int32 nSeconds = 0;
	Int32 nMicroseconds = 0;
	Int32 nTimeZoneHours = 0;
	Int32 nTimeZoneMinutes = 0;

	// Data for parsing each field
	const struct FieldInfo
	{
		Int32* m_pOutField;
		UInt m_uWidth;
		Byte m_cSeparators[5];
		Bool m_bSeparatorRequired;
	} kaFieldInfo[] =
	{
		{ &nYear,            4,       {'-', 0, 0, 0, 0},         false },
		{ &nMonth,           2,       {'-', 0, 0, 0, 0},         false },
		{ &nDay,             2,       {'T', ' ', 0, 0, 0},         false },
		{ &nHours,           2,       {':', 0, 0, 0, 0},         false },
		{ &nMinutes,         2,       {':', 0, 0, 0, 0},         false },
		{ &nSeconds,         2,       {'.', ',', '+', '-', 'Z'}, true  },
		// We use -1 here since the number of digits in the decimal fraction for seconds can be unlimited.
		{ &nMicroseconds,    UIntMax, {'+', '-', 'Z', 0, 0},    true  },
		{ &nTimeZoneHours,   2,       {':', 0, 0, 0, 0},         false  },
		{ &nTimeZoneMinutes, 2,       {0, 0, 0, 0, 0},           false  },
	};

	const Byte* pStr = sDateTime.CStr();
	UInt uLength = sDateTime.GetSize();

	size_t uOffset = 0;
	Bool bNegateTimeZone = false;
	for (size_t uFieldNum = 0; uFieldNum < SEOUL_ARRAY_COUNT(kaFieldInfo) && uOffset < uLength; ++uFieldNum)
	{
		// Parse the field value
		Int32* pField = kaFieldInfo[uFieldNum].m_pOutField;
		UInt uFieldLen = ParseInt32(pStr + uOffset, kaFieldInfo[uFieldNum].m_uWidth, uLength, pField);
		if (0 == uFieldLen)
		{
			// If the field couldn't be parsed as an integer, return an error
			return WorldTime();
		}

		if (pField == &nMicroseconds)
		{
			// convert the decimal section of the seconds into microseconds
			if (uFieldLen < 1)
			{
				// Something went wrong with the parsing, probably looks like SS.+ZZ:ZZ
				return WorldTime();
			}
			else if (uFieldLen < 7)
			{
				// Converts something like .00123 into 1230
				// Since microseconds are 10^-6 seconds, S.xxxxxx would be xxxxxx microseconds
				// and anything with less digits like, S.yyyy would be yyyy00
				// We also have to consider that strtol() will turn both .00123 and .123 into 123
				nMicroseconds *= (Int32)Pow(10, 6 - (Float)uFieldLen);
			}
			else
			{
				// Converts something like .0034567 into 3456 (always rounds down because that's simple and fairly irrelevant)
				nMicroseconds /= (Int32)Pow(10, (Float)uFieldLen - 6);
			}
		}

		// Set the offset to point to the next section (might be a separator)
		uOffset += uFieldLen;
		if (uOffset >= uLength)
		{
			break;
		}

		// Skip past the separator, if present
		if (pStr[uOffset] == kaFieldInfo[uFieldNum].m_cSeparators[0] ||
			pStr[uOffset] == kaFieldInfo[uFieldNum].m_cSeparators[1] ||
			pStr[uOffset] == kaFieldInfo[uFieldNum].m_cSeparators[2] ||
			pStr[uOffset] == kaFieldInfo[uFieldNum].m_cSeparators[3] ||
			pStr[uOffset] == kaFieldInfo[uFieldNum].m_cSeparators[4])
		{
			Byte cSeparator = pStr[uOffset];
			uOffset++;

			// Skip microseconds if the time didn't include decimal seconds
			if (pField == &nSeconds && cSeparator != '.' && cSeparator != ',')
			{
				++uFieldNum;
			}

			// If this is the end of the seconds field, check if the separator after the seconds is negative
			if ((pField == &nSeconds || pField == &nMicroseconds) && cSeparator == '-')
			{
				bNegateTimeZone = true;
			}
		}
		else if (kaFieldInfo[uFieldNum].m_bSeparatorRequired)
		{
			// If separator is required but missing, return an error
			return WorldTime();
		}
	}

	// Invalid time if iMonth or iDay is < 0.
	if (nMonth < 0 || nDay < 0)
	{
		return WorldTime();
	}

	// Success, convert the time and return it
	Int32 const nDaysSinceJan1_1970 = ToDaysGregorian(nYear, (UInt32)nMonth, (UInt32)nDay);

	// Adjust the sign of the time zone
	if (bNegateTimeZone)
	{
		nTimeZoneHours = -nTimeZoneHours;
		nTimeZoneMinutes = -nTimeZoneMinutes;
	}

	Int64 nSecondsSinceEpoch =
		(Int64)(nDaysSinceJan1_1970) * (Int64)86400 +
		(Int64)(nHours - nTimeZoneHours) * (Int64)3600 +
		(Int64)(nMinutes - nTimeZoneMinutes) * (Int64)60 +
		(Int64)nSeconds;

	WorldTime ret;
	ret.m_Time.tv_sec = nSecondsSinceEpoch;
	ret.m_Time.tv_usec = nMicroseconds;
	return ret;
}

Bool WorldTime::ConvertToLocalTime(tm& rLocalTime) const
{
	memset(&rLocalTime, 0, sizeof(tm));
	Bool bSuccess = false;
#if SEOUL_PLATFORM_WINDOWS
	bSuccess = (0 == localtime_s(&rLocalTime, &m_Time.tv_sec));
#else
	// FIXME: Support dates after 2038-01-19 03:14:07 UTC
	time_t tv_sec = (time_t)m_Time.tv_sec;
	tm* pLocalTime = localtime(&tv_sec);
	bSuccess = (nullptr != pLocalTime);
	if (bSuccess)
	{
		memcpy(&rLocalTime, pLocalTime, sizeof(tm));
	}
#endif

	return bSuccess;
}

String WorldTime::ToISO8601DateTimeUTCString() const
{
	// Days from Unix 1970 epoch.
	Int64 const iDaysFromEpochUTC = (m_Time.tv_sec / (Int64)86400);

	// Get year, month, day.
	Int32 iYear = 0;
	UInt32 uMonth = 0u;
	UInt32 uDay = 0u;
	FromDaysGregorian((Int32)iDaysFromEpochUTC, iYear, uMonth, uDay);

	// Compute hours, minutes, seconds from the remaining time.
	Int32 iSeconds = (Int32)(m_Time.tv_sec - (Int64)(iDaysFromEpochUTC * 86400));
	Int32 const iHours = (iSeconds / 3600);
	iSeconds -= (iHours * 3600);
	Int32 const iMinutes = (iSeconds / 60);
	iSeconds -= (iMinutes * 60);

	// Done, return a UTC based ISO 8601 string.
	return String::Printf("%04d-%02u-%02uT%02d:%02d:%02dZ",
		iYear,
		uMonth,
		uDay,
		iHours,
		iMinutes,
		iSeconds);
}

String WorldTime::ToLocalTimeString(Bool bIncludeYearMonthDay /*= true*/) const
{
	tm localTime;
	memset(&localTime, 0, sizeof(localTime));

	Bool bSuccess = ConvertToLocalTime(localTime);

	if (bSuccess)
	{
		if (bIncludeYearMonthDay)
		{
			return String::Printf(
				"%04d-%02d-%02d-%02d_%02d_%02d",
				localTime.tm_year + 1900,
				localTime.tm_mon + 1,
				localTime.tm_mday,
				localTime.tm_hour,
				localTime.tm_min,
				localTime.tm_sec);
		}
		else
		{
			return String::Printf(
				"%02d_%02d_%02d_%03d",
				localTime.tm_hour,
				localTime.tm_min,
				localTime.tm_sec,
				(Int32)((Int64)m_Time.tv_usec / kMillisecondsToMicroseconds));
		}
	}
	else
	{
		return String();
	}
}

String WorldTime::ToGMTString(Bool bIncludeYearMonthDay /*= true*/) const
{
	tm gmTime;
	memset(&gmTime, 0, sizeof(gmTime));

	Bool bSuccess = ConvertToGMTime(gmTime);

	if (bSuccess)
	{
		if (bIncludeYearMonthDay)
		{
			return String::Printf(
				"%04d-%02d-%02d %02d:%02d:%02d+0000",
				gmTime.tm_year + 1900,
				gmTime.tm_mon + 1,
				gmTime.tm_mday,
				gmTime.tm_hour,
				gmTime.tm_min,
				gmTime.tm_sec);
		}
		else
		{
			return String::Printf(
				"%02d_%02d_%02d_%03d",
				gmTime.tm_hour,
				gmTime.tm_min,
				gmTime.tm_sec,
				(Int32)((Int64)m_Time.tv_usec / kMillisecondsToMicroseconds));
		}
	}
	else
	{
		return String();
	}
}

Bool WorldTime::ConvertToGMTime(tm& rGMTime) const
{
	memset(&rGMTime, 0, sizeof(tm));
	Bool bSuccess = false;
#if SEOUL_PLATFORM_WINDOWS
	bSuccess = (0 == gmtime_s(&rGMTime, &m_Time.tv_sec));
#else
	// FIXME: Support dates after 2038-01-19 03:14:07 UTC
	time_t tv_sec = (time_t)m_Time.tv_sec;
	tm* pGMTime = gmtime(&tv_sec);
	bSuccess = (nullptr != pGMTime);
	if (bSuccess)
	{
		memcpy(&rGMTime, pGMTime, sizeof(tm));
	}
#endif

	return bSuccess;
}

WorldTime WorldTime::FromYearMonthDayUTC(Int32 year, UInt32 month, UInt32 day)
{
	Int32 const iDaysSinceJan1_1970 = ToDaysGregorian(year, month, day);
	Int64 nSecondsSinceEpoch = (Int64)(iDaysSinceJan1_1970) * (Int64)86400;

	WorldTime ret;
	ret.m_Time.tv_sec = nSecondsSinceEpoch;
	ret.m_Time.tv_usec = 0;
	return ret;
}

void WorldTime::GetYearMonthDay(Int32& rYear, UInt32& rMonth, UInt32& rDay) const
{
	Int64 const iDaysFromEpochUTC = (m_Time.tv_sec / (Int64)86400);

	FromDaysGregorian((Int32)iDaysFromEpochUTC, rYear, rMonth, rDay);
}

/**
 * Adds the specified number of seconds to the time
 */
void WorldTime::AddSeconds(Int64 iSeconds)
{
	m_Time.tv_sec += iSeconds;
}

/**
 * Adds the specified number of seconds to the time
 */
void WorldTime::AddSecondsDouble(Double fSeconds)
{
	AddMicroseconds((Int64)(fSeconds * (Double)kSecondsToMicroseconds));
}

/**
 * Adds the specified number of milliseconds to the time
 */
void WorldTime::AddMilliseconds(Int64 iMilliseconds)
{
	AddMicroseconds(iMilliseconds * kMillisecondsToMicroseconds);
}

/**
 * Adds the specified number of microseconds to the time
 */
void WorldTime::AddMicroseconds(Int64 iMicroseconds)
{
	SetMicroseconds(GetMicroseconds() + iMicroseconds);
}

/**
 * Adds a time interval to this time
 */
WorldTime WorldTime::operator+(const TimeInterval& delta) const
{
	WorldTime result(*this);
	result.AddMicroseconds(delta.GetMicroseconds());
	return result;
}

WorldTime& WorldTime::operator+=(const TimeInterval& delta)
{
	AddMicroseconds(delta.GetMicroseconds());
	return *this;
}

/**
 * Subtracts a time interval from this time
 */
WorldTime WorldTime::operator-(const TimeInterval& delta) const
{
	WorldTime result(*this);
	result.AddMicroseconds(-delta.GetMicroseconds());
	return result;
}

/**
 * Subtracts two times to get a time interval
 */
TimeInterval WorldTime::operator-(const WorldTime& otherTime) const
{
	return SubtractWorldTime(otherTime);
}

/**
 * Subtracts two times to get a time interval
 */
TimeInterval WorldTime::SubtractWorldTime(const WorldTime& otherTime) const
{
	return TimeInterval::FromMicroseconds(GetMicroseconds() - otherTime.GetMicroseconds());
}

/**
* Gets the start of day for the current day, with UTC offset (hours)
*/
WorldTime WorldTime::GetDayStartTime(Int64 iOffsetHoursUTC) const
{
	auto iDayNumber = (GetSeconds() - (iOffsetHoursUTC * kHoursToSeconds)) / kDaysToSeconds;
	auto iDayStartSeconds = iDayNumber * kDaysToSeconds + iOffsetHoursUTC * kHoursToSeconds;
	return FromSecondsInt64(iDayStartSeconds);
}

WorldTime WorldTime::GetNextDayStartTime(Int64 iOffsetHoursUTC) const
{
	auto start = GetDayStartTime(iOffsetHoursUTC);
	if (start < *this)
	{
		start.AddDays(1);
	}

	return start;
}

UInt32 GetHash(const WorldTime& worldTime)
{
	return GetHash(worldTime.GetMicroseconds());
}

/* Static */ IsCurrentlyDSTResult WorldTime::IsCurrentlyDST()
{
	auto const now = GetUTCTime();

	tm local;
	if (now.ConvertToLocalTime(local))
	{
		if (local.tm_isdst > 0)
		{
			return IsCurrentlyDSTResult::KnownTrue;
		}
		else if (local.tm_isdst == 0)
		{
			return IsCurrentlyDSTResult::KnownFalse;
		}
	}

	return IsCurrentlyDSTResult::Unknown;
}
} // namespace Seoul
