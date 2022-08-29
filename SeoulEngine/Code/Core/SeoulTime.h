/**
 * \file SeoulTime.h
 * \brief High resolution timing functionality.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SEOUL_TIME_H
#define SEOUL_TIME_H

#include "Prereqs.h"

// Forward declaration.
struct tm;

namespace Seoul
{

/**
 * Platform-agnostic variant of the POSIX 'struct timeval' which always uses a
 * 64-bit number of seconds to avoid the Y2038 problem
 */
struct TimeValue
{
	/** Seconds */
	Int64 tv_sec;
	/** Microseconds */
	Int32 tv_usec;
};

// Forward declarations
struct SeoulTimeImpl;

/**
 * SeoulTime provides high resolution timing functionality. Time
 * can be both be measured globally, relative to a "game start time" that
 * is marked by the application, or in individual instances of SeoulTime.
 * In the latter usage, SeoulTime can be viewed as timer object.
 */
class SeoulTime SEOUL_SEALED
{
public:
	// Should be called at the point in an app's startup which
	// is the desired mark point for the game start time. All
	// calls to GetGameTimeInMilliseconds() will be relative
	// to this time.
	static void MarkGameStartTick();

	// Return the elapsed time since a call to MarkGameStartTick()
	// in ticks.
	static Int64 GetGameTimeInTicks();

	// Return the elapsed time in microseconds since a call to MarkGameStartTick().
	static Double GetGameTimeInMicroseconds();

	// Return the elapsed time in milliseconds since a call to MarkGameStartTick().
	static Double GetGameTimeInMilliseconds();

	// Converts a time in microseconds into ticks.
	static Int64 ConvertMicrosecondsToTicks(Double fMicroseconds)
	{
		return ConvertMillisecondsToTicks(fMicroseconds / 1000.0);
	}

	// Converts a time in milliseconds into ticks.
	static Int64 ConvertMillisecondsToTicks(Double fMilliseconds);

	// Converts a time in seconds into ticks.
	static Int64 ConvertSecondsToTicks(Double fSeconds)
	{
		return ConvertMillisecondsToTicks(fSeconds * 1000.0);
	}

	// Converts a time in ticks into microseconds.
	static Double ConvertTicksToMicroseconds(Int64 iTicks)
	{
		return ConvertTicksToMilliseconds(iTicks) * 1000.0;
	}

	// Converts a time in ticks into milliseconds.
	static Double ConvertTicksToMilliseconds(Int64 iTicks);

	// Converts a time in ticks into seconds.
	static Double ConvertTicksToSeconds(Int64 iTicks)
	{
		return ConvertTicksToMilliseconds(iTicks) / 1000.0;
	}

	SeoulTime();
	~SeoulTime();

	// Start this SeoulTime object's timer, marking a start time.
	void StartTimer();

	// Stop this SeoulTime object's timer, marking a stop time.
	void StopTimer();

	// Equivalent to StopTimer(), GetElapsedTimeInSeconds(), StartTimer()
	Double SplitTimerSeconds();

	// Equivalent to StopTimer(), GetElapsedTimeInMilliseconds(), StartTimer()
	Double SplitTimerMilliseconds();

	// Time elapsed in seconds between calls to StartTimer() and StopTimer().
	// Return value is undefined if StartTimer() and StopTimer()
	// have not been called.
	Double GetElapsedTimeInSeconds() const;

	// Time elapsed in milliseconds between calls to StartTimer() and StopTimer().
	// Return value is undefined if StartTimer() and StopTimer()
	// have not been called.
	Double GetElapsedTimeInMilliseconds() const;

	/**
	 * @return The time in ticks at which StartTimer() was called.
	 *
	 * Value is undefined if StartTimer() has not been called.
	 *
	 * "Ticks" has no absolute meaning - it can change from platform
	 * to platform. Only use ticks as input to ConvertTicksToMilliseconds()
	 * to determine the actual time elapsed.
	 */
	Int64 GetStartTick() const
	{
		return m_iStartTick;
	}

	/**
	 * @return The time in ticks at which StopTimer() was called.
	 *
	 * Value is undefined if StopTimer() has not been called.
	 *
	 * "Ticks" has no absolute meaning - it can change from platform
	 * to platform. Only use ticks as input to ConvertTicksToMilliseconds()
	 * to determine the actual time elapsed.
	 */
	Int64 GetStopTick() const
	{
		return m_iStopTick;
	}

	/**
	 * @return Delta time elapsed in ticks between calls to StopTimer()
	 * and StartTimer().
	 *
	 * Value is undefined if StopTimer() and StartTimer() have
	 * not been called.
	 */
	Int64 GetElapsedTicks() const
	{
		return (m_iStopTick - m_iStartTick);
	}

private:
	static SeoulTimeImpl& GetImpl();

	Int64 m_iStartTick;
	Int64 m_iStopTick;
}; // class SeoulTime

class TimeInterval;

enum class IsCurrentlyDSTResult
{
	KnownTrue,
	KnownFalse,
	Unknown
};

/**
 * WorldTime provides low resolution time-of-day functionality.
 */
class WorldTime SEOUL_SEALED
{
public:
	/** Conversion factor when converting to/from milliseconds/microseconds. */
	static const Int64 kMillisecondsToMicroseconds = 1000;

	/** Conversion factor when converting to/from milliseconds/microseconds. */
	static const Int64 kSecondsToMilliseconds = 1000;

	/** Conversion factor when converting to/from seconds/microseconds. */
	static const Int64 kSecondsToMicroseconds = 1000000;

	/** Conversion factor when converting to/from minutes/seconds. */
	static const Int64 kMinutesToSeconds = 60;

	/** Conversion factor when converting to/from hours/minutes. */
	static const Int64 kHoursToMinutes = 60;

	/** Conversion factor when converting to/from days/hours. */
	static const Int64 kDaysToHours = 24;

	/** Conversion factor when converting to/from seconds/nanoseconds. */
	static const Int64 kSecondsToNanoseconds = 1000000000;

	/** Conversion factor when converting to/from days/seconds. */
	static const Int64 kDaysToSeconds = 86400;

	/** Conversion factor when converting to/from hours/seconds. */
	static const Int64 kHoursToSeconds = 3600;

	static IsCurrentlyDSTResult IsCurrentlyDST();

	/**
	 * @return The value fMicroseconds converted to seconds.
	 */
	static inline Double ConvertMicrosecondsToSeconds(Double fMicroseconds)
	{
		return (fMicroseconds / (Double)kSecondsToMicroseconds);
	}

	/**
	 * @return The value iMicroseconds converted to seconds.
	 */
	static inline Double ConvertInt64MicrosecondsToSeconds(Int64 iMicroseconds)
	{
		return ((Double)iMicroseconds / (Double)kSecondsToMicroseconds);
	}

	/**
	 * @return The value iNanoseconds converted to seconds.
	 */
	static inline Double ConvertInt64NanosecondsToSeconds(Int64 iNanoseconds)
	{
		return ((Double)iNanoseconds / (Double)kSecondsToNanoseconds);
	}

	/**
	 * @return The value fSeconds converted to microseconds.
	 */
	static inline Int64 ConvertSecondsToInt64Microseconds(Double fSeconds)
	{
		return (Int64)(fSeconds * (Double)kSecondsToMicroseconds);
	}

	/**
	 * Factory method to construct a WorldTime from the given total number
	 * of microseconds. Only valid for values of iMicroseconds >= 0.
	 */
	static inline WorldTime FromMicroseconds(Int64 iMicroseconds)
	{
		WorldTime ret;
		ret.SetMicroseconds(iMicroseconds);
		return ret;
	}

	/** @return A WorldTime instantiated from a UTC epoch in seconds. */
	static inline WorldTime FromSecondsInt64(Int64 iSeconds)
	{
		WorldTime ret;
		ret.m_Time.tv_sec = iSeconds;
		ret.m_Time.tv_usec = 0;
		return ret;
	}

	static inline WorldTime FromSecondsDouble(Double fSeconds)
	{
		WorldTime ret;
		Int64 iMicroseconds = ConvertSecondsToInt64Microseconds(fSeconds);
		ret.SetMicroseconds(iMicroseconds);
		return ret;
	}

	/** @return A WorldTime instantiated from a UTC epoch defined in a TimeValue structure. */
	static inline WorldTime FromTimeValue(const TimeValue& timeValue)
	{
		WorldTime ret;
		ret.m_Time = timeValue;
		return ret;
	}
	static WorldTime FromYearMonthDayUTC(Int32 year, UInt32 month, UInt32 day);

	/**
	 * Constructs a default WorldTime at the Unix epoch
	 * (1970-01-01 00:00:00 UTC)
	 */
	WorldTime()
	{
		Reset();
	}

	/**
	 * Constructs a WorldTime with the given number of seconds since the Unix
	 * epoch
	 */
	explicit WorldTime(Int64 iSeconds)
	{
		m_Time.tv_sec = iSeconds;
		m_Time.tv_usec = 0;
	}

	/**
	 * Constructs a WorldTime with the given number of seconds and microseconds
	 * since the Unix epoch
	 */
	WorldTime(Int64 iSeconds, Int32 iMicroseconds)
	{
		m_Time.tv_sec = iSeconds;
		m_Time.tv_usec = iMicroseconds;
	}

	static WorldTime GetUTCTime();

	// Parses an ISO 8601 date+time into a WorldTime object
	static WorldTime ParseISO8601DateTime(const String& sDateTime);

	String ToISO8601DateTimeUTCString() const;
	String ToLocalTimeString(Bool bIncludeYearMonthDay = true) const;
	String ToGMTString(Bool bIncludeYearMonthDay = true) const;

	// Helper functions to add/subtract varying time quantities
	void AddSeconds(Int64 iSeconds);
	void AddSecondsDouble(Double fSeconds);

	void AddMinutes(Int64 iMinutes) { AddSeconds(60 * iMinutes); }
	void AddHours(Int64 iHours) { AddSeconds(3600 * iHours); }
	void AddDays(Int64 iDays) { AddSeconds(86400 * iDays); }

	void AddMinutesDouble(Double fMinutes) { AddSecondsDouble(60.0f * fMinutes); }
	void AddHoursDouble(Double fHours) { AddSecondsDouble(3600.0f * fHours); }
	void AddDaysDouble(Double fDays) { AddSecondsDouble(86400.0f * fDays); }

	void AddMilliseconds(Int64 iMilliseconds);
	void AddMicroseconds(Int64 iMicroseconds);

	Bool operator<(const WorldTime& b) const
	{
		return (GetMicroseconds() < b.GetMicroseconds());
	}

	Bool operator<=(const WorldTime& b) const
	{
		return (GetMicroseconds() <= b.GetMicroseconds());
	}

	Bool operator>(const WorldTime& b) const
	{
		return (GetMicroseconds() > b.GetMicroseconds());
	}

	Bool operator>=(const WorldTime& b) const
	{
		return (GetMicroseconds() >= b.GetMicroseconds());
	}

	Bool operator==(const WorldTime& b) const
	{
		return (GetMicroseconds() == b.GetMicroseconds());
	}

	Bool operator!=(const WorldTime& b) const
	{
		return (GetMicroseconds() != b.GetMicroseconds());
	}

	// Adds a time interval to this time
	WorldTime operator+(const TimeInterval& delta) const;
	WorldTime& operator+=(const TimeInterval& delta);

	// Subtracts a time interval from this time
	WorldTime operator-(const TimeInterval& delta) const;

	// Subtracts two times to get a time interval
	TimeInterval operator-(const WorldTime& otherTime) const;

	/** Convenience wrapper for Reflection binding. */
	TimeInterval SubtractWorldTime(const WorldTime& otherTime) const;

	/**
	 * Gets the number of seconds since the Unix epoch (the microseconds
	 * are not returned)
	 */
	Int64 GetSeconds() const
	{
		return m_Time.tv_sec;
	}

	/**
	 * Gets the number of seconds since the Unix epoch as a double value
	 * which includes the number of microseconds
	 */
	Double GetSecondsAsDouble() const
	{
		return (Double)m_Time.tv_sec + ConvertMicrosecondsToSeconds(m_Time.tv_usec);
	}

	/**
	 * @return this WorldTime value in absolute microseconds.
	 */
	Int64 GetMicroseconds() const
	{
		return (m_Time.tv_sec * kSecondsToMicroseconds + (Int64)m_Time.tv_usec);
	}

	/**
	 * Set this WorldTime value from an absolute microseconds value
	 * (measured from the Unix epoch).
	 */
	void SetMicroseconds(Int64 iMicroseconds)
	{
		m_Time.tv_sec = (iMicroseconds / kSecondsToMicroseconds);
		m_Time.tv_usec = (iMicroseconds % kSecondsToMicroseconds);
	}

	/**
	 * Reset this time value back to the Unix epoch.
	 */
	void Reset()
	{
		memset(&m_Time, 0, sizeof(m_Time));
	}

	/**
	 * Is the WorldTime uninitialized?
	 */
	Bool IsZero() const
	{
		return m_Time.tv_sec == 0 && m_Time.tv_usec == 0;
	}

	void GetYearMonthDay(Int32& rYear, UInt32& rMonth, UInt32& rDay) const;

	Bool ConvertToLocalTime(tm& rLocalTime) const;
	Bool ConvertToGMTime(tm& rLocalTime) const;

	WorldTime GetDayStartTime(Int64 iOffsetHoursUTC) const;
	/**
	 * Gets the day start time based on the given UTC offset, but always
	 * returns a WorldTime after itself.
	 */
	WorldTime GetNextDayStartTime(Int64 iOffsetHoursUTC) const;

private:
	TimeValue m_Time;
}; // class WorldTime

/**
 * Class representing a time interval between two WorldTime instances
 */
class TimeInterval SEOUL_SEALED
{
public:
	/**
	 * Default-constructs a time interval of 0 seconds
	 */
	TimeInterval()
	{
		Reset();
	}

	/**
	 * Constructs a time interval from a low-level TimeValue.
	 */
	explicit TimeInterval(const TimeValue& timeValue)
		: m_Delta(timeValue)
	{
	}

	/**
	 * Constructs a time interval with the given number of seconds
	 */
	explicit TimeInterval(Int64 iSeconds)
	{
		m_Delta.tv_sec = iSeconds;
		m_Delta.tv_usec = 0;
	}

	/**
	 * Constructs a time interval with the given number of seconds and
	 * microseconds
	 */
	TimeInterval(Int64 iSeconds, Int32 iMicroseconds)
	{
		m_Delta.tv_sec = iSeconds;
		m_Delta.tv_usec = iMicroseconds;

		// Ensure that iMicroseconds is always in [0, 999999]
		if (m_Delta.tv_usec >= 1000000)
		{
			m_Delta.tv_sec += (Int64)(m_Delta.tv_usec / 1000000);
			m_Delta.tv_usec %= 1000000;
		}
		else if (m_Delta.tv_usec < 0)
		{
			m_Delta.tv_usec = 999999 - (-m_Delta.tv_usec - 1) % 1000000;
			m_Delta.tv_sec += (Int64)((iMicroseconds - m_Delta.tv_usec) / 1000000);
		}
	}

	/**
	 * Factory method to construct a time interval from the given total number
	 * of microseconds.  Note that even if iMicroseconds is negative, the
	 * result will be correct, even though the sign of (negative) % (positive)
	 * is implementation-defined -- for all non-zero b, it's always true that
	 * a == (a/b)*b + (a%b).
	 */
	static TimeInterval FromMicroseconds(Int64 iMicroseconds)
	{
		return TimeInterval(iMicroseconds / WorldTime::kSecondsToMicroseconds,
							iMicroseconds % WorldTime::kSecondsToMicroseconds);
	}

	/**
	 * Factory method to construct a time interval from the given total number
	 * of minutes.
	 */
	static TimeInterval FromMinutes(Int64 iMinutes)
	{
		return TimeInterval(iMinutes * WorldTime::kMinutesToSeconds);
	}

	/**
	* Factory method to construct a time interval from the given total number
	* of hours.
	*/
	static TimeInterval FromHours(Int64 iHours)
	{
		return TimeInterval(iHours * WorldTime::kHoursToMinutes * WorldTime::kMinutesToSeconds);
	}

	/**
	 * Factory method to construct a time interval from the given total number
	 * of days.
	 */
	static TimeInterval FromDays(Int64 iDays)
	{
		return TimeInterval(iDays * WorldTime::kDaysToHours * WorldTime::kHoursToMinutes * WorldTime::kMinutesToSeconds);
	}

	/**
	 * Factory method to construct a time interval from the given total
	 * number of seconds. Valid for positive or negative values of iSeconds.
	 */
	static inline TimeInterval FromSecondsInt64(Int64 iSeconds)
	{
		TimeInterval ret;
		ret.m_Delta.tv_sec = iSeconds;
		ret.m_Delta.tv_usec = 0;
		return ret;
	}

	/** Equivalent to FromSecondsInt64 for sub-integer precision values of seconds. */
	static inline TimeInterval FromSecondsDouble(Double fSeconds)
	{
		return FromMicroseconds(WorldTime::ConvertSecondsToInt64Microseconds(fSeconds));
	}

	/**
	* Factory method to construct a time interval from the given total
	* number of hours. Valid for positive or negative values of iHours.
	*/
	static inline TimeInterval FromHoursInt64(Int64 iHours)
	{
		return FromSecondsInt64(iHours * WorldTime::kHoursToSeconds);
	}

	/**
	* Factory method to construct a time interval from the given total
	* number of days. Valid for positive or negative values of iDays.
	*/
	static inline TimeInterval FromDaysInt64(Int64 iDays)
	{
		return FromSecondsInt64(iDays * WorldTime::kDaysToSeconds);
	}

	/**
	 * Gets the number of seconds in this time interval (the microseconds are
	 * not returned)
	 */
	Int64 GetSeconds() const
	{
		return m_Delta.tv_sec;
	}

	/**
	 * Gets the number of seconds in this time interval as a Double value,
	 * which includes the microseconds
	 */
	Double GetSecondsAsDouble() const
	{
		return (Double)m_Delta.tv_sec + WorldTime::ConvertMicrosecondsToSeconds(m_Delta.tv_usec);
	}

	/**
	 * Gets the total number of microseconds in this time interval
	 */
	Int64 GetMicroseconds() const
	{
		return m_Delta.tv_sec * (Int64)1000000 + (Int64)m_Delta.tv_usec;
	}

	/**
	 * Reset this time value back to 0.
	 */
	void Reset()
	{
		m_Delta.tv_sec = 0;
		m_Delta.tv_usec = 0;
	}

	/**
	 * Is the WorldTime uninitialized?
	 */
	Bool IsZero() const
	{
		return (m_Delta.tv_sec == 0 && m_Delta.tv_usec == 0);
	}

	Bool operator<(const TimeInterval& b) const
	{
		return (GetMicroseconds() < b.GetMicroseconds());
	}

	Bool operator<=(const TimeInterval& b) const
	{
		return (GetMicroseconds() <= b.GetMicroseconds());
	}

	Bool operator>(const TimeInterval& b) const
	{
		return (GetMicroseconds() > b.GetMicroseconds());
	}

	Bool operator>=(const TimeInterval& b) const
	{
		return (GetMicroseconds() >= b.GetMicroseconds());
	}

	Bool operator==(const TimeInterval& b) const
	{
		return (GetMicroseconds() == b.GetMicroseconds());
	}

	Bool operator!=(const TimeInterval& b) const
	{
		return (GetMicroseconds() != b.GetMicroseconds());
	}

	/**
	 * Returns the negative of this time interval
	 */
	TimeInterval operator-() const
	{
		return Negate();
	}

	/**
	 * Ads two TimeIntervals to produce a new TimeInterval.
	 */
	TimeInterval operator+(const TimeInterval& b) const
	{
		return Add(b);
	}

	/**
	 * Subtracts two TimeIntervals to produce a new TimeInterval.
	 */
	TimeInterval operator-(const TimeInterval& b) const
	{
		return Subtract(b);
	}

	/** Convenience wrapper for Reflection binding. */
	TimeInterval Add(const TimeInterval& b) const
	{
		return TimeInterval::FromMicroseconds(
			GetMicroseconds() + b.GetMicroseconds());
	}

	/** Convenience wrapper for Reflection binding. */
	TimeInterval Negate() const
	{
		return TimeInterval(-m_Delta.tv_sec, -m_Delta.tv_usec);
	}

	/** Convenience wrapper for Reflection binding. */
	TimeInterval Subtract(const TimeInterval& b) const
	{
		return TimeInterval::FromMicroseconds(
			GetMicroseconds() - b.GetMicroseconds());
	}

	/** Return the internal TimeValue structure for read. */
	const TimeValue& GetTimeValue() const { return m_Delta; }

private:
	// Interval size in seconds + microseconds
	TimeValue m_Delta;
};

UInt32 GetHash(const WorldTime& worldTime);

} // namespace Seoul

#endif // include guard
