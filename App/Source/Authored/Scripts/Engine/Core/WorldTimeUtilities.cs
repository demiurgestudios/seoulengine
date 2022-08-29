// Miscellaneous WorldTime utilities.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using Native;

using static SlimCS;

public static class WorldTimeUtilities
{
	public enum IsCurrentlyDSTResult
	{
		KnownTrue,
		KnownFalse,
		Unknown
	};

	public const int k_SecondsPerDay = 60 * 60 * 24;
	public const int k_SecondsPerHour = 60 * 60;
	public const int k_SecondsPerYearWithoutLeapDays = 60 * 60 * 24 * 365;


	public static Native.WorldTime s_udWorldTimeZero = null;
	public static Native.TimeInterval s_udTimeIntervalZero = null;

	static WorldTimeUtilities()
	{
		s_udWorldTimeZero = CoreUtilities.NewNativeUserData<Native.WorldTime>();
		s_udTimeIntervalZero = CoreUtilities.NewNativeUserData<Native.TimeInterval>();
	}

	public static Native.WorldTime ParseServerTime(string sTime, bool required = false)
	{
		if (string.IsNullOrEmpty(sTime))
		{
			if (required)
			{
				CoreNative.Warn("Missing required time from server");
			}
			return null;
		}

		return Native.WorldTime.ParseISO8601DateTime(sTime);
	}

	public static bool IsInTimeRange(Native.WorldTime currentTime, Native.WorldTime startAt, Native.WorldTime endAt)
	{
		if (currentTime == null)
		{
			return false;
		}

		if (startAt != null && startAt > currentTime)
		{
			return false;
		}

		if (endAt != null && endAt < currentTime)
		{
			return false;
		}

		return true;
	}

	/// <summary>
	/// Returns an integer offset for the day of week. Sunday = 0.
	/// For an unrecognized weekday, returns null.
	/// </summary>
	public static int? ParseDayOfWeekOffset(string dayOfWeek)
	{
		switch (dayOfWeek)
		{
			case "Sunday":
				return 0;

			case "Monday":
				return 1;

			case "Tuesday":
				return 2;

			case "Wednesday":
				return 3;

			case "Thursday":
				return 4;

			case "Friday":
				return 5;

			case "Saturday":
				return 6;
		}

		return null;
	}

	public static Native.WorldTime NextDayOfWeek(int dayOfWeek, int hour = 0, int minute = 0, int second = 0)
	{
		// A Sunday
		var dateStr = @string.format("2018-01-07T%02d:%02d:%02dZ", hour, minute, second);

		var result = Native.WorldTime.ParseISO8601DateTime(dateStr);
		result.AddDays(dayOfWeek);

		// Shift result forward to be in the future
		var now = Engine.GetCurrentServerTime();
		var daysElapsed = (now - result).GetSecondsAsDouble() / k_SecondsPerDay;
		daysElapsed = 7 * math.floor(daysElapsed / 7);
		result.AddDays((int)daysElapsed);

		// If result is still in the past, shift up 1 week.
		// We use math.floor to avoid bumping ahead by an extra
		// week in some cases, so this will be hit fairly often.
		if (now > result)
		{
			result.AddDays(7);
		}

		return result;
	}


	public static Native.WorldTime ToWorldtime(int month, int day, int year, int hour, int minute, int hoursUTCOffset)
	{
		var dateTime = Native.WorldTime.FromYearMonthDayUTC(year, month, day);
		dateTime.AddHours(hour);
		dateTime.AddMinutes(minute);
		dateTime.AddHours(hoursUTCOffset);
		return dateTime;
	}

	/// <summary>
	/// Convert a WorldTime's day into an integer representation (i.e. num days since epoch)
	/// </summary>
	/// <param name="time">WorldTime to convert.</param>
	/// <returns>Days since app configured epoch.</returns>
	public static int GetDayNumForWorldTime(Native.WorldTime time, double iOffsetHoursUTC)
	{
		Native.WorldTime output = time.GetDayStartTime(iOffsetHoursUTC);
		return (int)(output.GetSecondsAsDouble() / WorldTimeUtilities.k_SecondsPerDay) + 1;
	}

	/// <summary>
	/// Given a day (relative to app's default utc offset), generate a WorldTime instance.
	/// </summary>
	/// <param name="dayInd">Days since epoch.</param>
	/// <returns>WorldTime equivalent of the days value.</returns>
	public static Native.WorldTime WorldTimeFromDayNum(int dayInd, double iOffsetHoursUTC)
	{
		return Native.WorldTime.FromSecondsInt64(dayInd * WorldTimeUtilities.k_SecondsPerDay).GetDayStartTime(iOffsetHoursUTC);
	}

	public static IsCurrentlyDSTResult IsCurrentlyDST()
	{
		return (IsCurrentlyDSTResult)Native.WorldTime.IsCurrentlyDST();
	}
}
