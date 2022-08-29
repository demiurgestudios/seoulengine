-- Miscellaneous WorldTime utilities.
--
-- Copyright (c) Demiurge Studios, Inc.
-- 
-- This source code is licensed under the MIT license.
-- Full license details can be found in the LICENSE file
-- in the root directory of this source tree.





local WorldTimeUtilities = class_static 'WorldTimeUtilities'

IsCurrentlyDSTResult = {

KnownTrue = 0,
KnownFalse = 1,
Unknown = 2
}

WorldTimeUtilities.k_SecondsPerDay = 86400--[[60 * 60 * 24]]
WorldTimeUtilities.k_SecondsPerHour = 3600--[[60 * 60]]
WorldTimeUtilities.k_SecondsPerYearWithoutLeapDays = 31536000--[[60 * 60 * 24 * 365]]





function WorldTimeUtilities.cctor()

	WorldTimeUtilities.s_udWorldTimeZero = CoreUtilities.NewNativeUserData(WorldTime)
	WorldTimeUtilities.s_udTimeIntervalZero = CoreUtilities.NewNativeUserData(TimeInterval)
end

function WorldTimeUtilities.ParseServerTime(sTime, required)

	if SlimCSStringLib.IsNullOrEmpty(sTime) then

		if required then

			CoreNative.Warn 'Missing required time from server'
		end
		return nil
	end

	return WorldTime.ParseISO8601DateTime(sTime)
end

function WorldTimeUtilities.IsInTimeRange(currentTime, startAt, endAt)

	if currentTime == nil then

		return false
	end

	if startAt ~= nil and startAt > currentTime then

		return false
	end

	if endAt ~= nil and endAt < currentTime then

		return false
	end

	return true
end

--- <summary>
--- Returns an integer offset for the day of week. Sunday = 0.
--- For an unrecognized weekday, returns null.
--- </summary>
function WorldTimeUtilities.ParseDayOfWeekOffset(dayOfWeek)

	repeat local _ = dayOfWeek; if _ == 'Sunday' then goto case elseif _ == 'Monday' then goto case0 elseif _ == 'Tuesday' then goto case1 elseif _ == 'Wednesday' then goto case2 elseif _ == 'Thursday' then goto case3 elseif _ == 'Friday' then goto case4 elseif _ == 'Saturday' then goto case5 else break end

		::case::
			do return 0 end

		::case0::
			do return 1 end

		::case1::
			do return 2 end

		::case2::
			do return 3 end

		::case3::
			do return 4 end

		::case4::
			do return 5 end

		::case5::
			do return 6 end
	until true

	return nil
end

function WorldTimeUtilities.NextDayOfWeek(dayOfWeek, hour, minute, second)

	-- A Sunday
	local dateStr = string.format('2018-01-07T%02d:%02d:%02dZ', hour, minute, second)

	local result = WorldTime.ParseISO8601DateTime(dateStr)
	result:AddDays(dayOfWeek)

	-- Shift result forward to be in the future
	local now = Engine.GetCurrentServerTime()
	local daysElapsed = (now - result):GetSecondsAsDouble() / 86400--[[WorldTimeUtilities.k_SecondsPerDay]]
	daysElapsed = 7 * math.floor(daysElapsed / 7)
	result:AddDays(__castint__(daysElapsed))

	-- If result is still in the past, shift up 1 week.
	-- We use math.floor to avoid bumping ahead by an extra
	-- week in some cases, so this will be hit fairly often.
	if now > result then

		result:AddDays(7)
	end

	return result
end


function WorldTimeUtilities.ToWorldtime(month, day, year, hour, minute, hoursUTCOffset)

	local dateTime = WorldTime.FromYearMonthDayUTC(year, month, day)
	dateTime:AddHours(hour)
	dateTime:AddMinutes(minute)
	dateTime:AddHours(hoursUTCOffset)
	return dateTime
end

--- <summary>
--- Convert a WorldTime's day into an integer representation (i.e. num days since epoch)
--- </summary>
--- <param name="time">WorldTime to convert.</param>
--- <returns>Days since app configured epoch.</returns>
function WorldTimeUtilities.GetDayNumForWorldTime(time, iOffsetHoursUTC)

	local output = time:GetDayStartTime(iOffsetHoursUTC)
	return __i32narrow__(__castint__((output:GetSecondsAsDouble() / 86400--[[WorldTimeUtilities.k_SecondsPerDay]])) + 1)
end

--- <summary>
--- Given a day (relative to app's default utc offset), generate a WorldTime instance.
--- </summary>
--- <param name="dayInd">Days since epoch.</param>
--- <returns>WorldTime equivalent of the days value.</returns>
function WorldTimeUtilities.WorldTimeFromDayNum(dayInd, iOffsetHoursUTC)

	return WorldTime.FromSecondsInt64(__i32mul__(dayInd, 86400--[[WorldTimeUtilities.k_SecondsPerDay]])):GetDayStartTime(iOffsetHoursUTC)
end

function WorldTimeUtilities.IsCurrentlyDST()

	return WorldTime.IsCurrentlyDST()
end
return WorldTimeUtilities
