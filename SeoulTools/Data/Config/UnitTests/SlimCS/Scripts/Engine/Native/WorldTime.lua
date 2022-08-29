--[[
	WorldTime.lua
	AUTO GENERATED - DO NOT MODIFY
	API FOR NATIVE CLASS INSTANCE

	Run GenerateScriptBindings.bat in the Utilities folder to re-generate bindings.

	Copyright (c) Demiurge Studios, Inc.
	
	This source code is licensed under the MIT license.
	Full license details can be found in the LICENSE file
	in the root directory of this source tree.
]]





local WorldTime = class('WorldTime', nil, 'Native.WorldTime')

interface('IStatic00000', 'Native.WorldTime+IStatic', 'IStatic')










local s_udStaticApi = nil

function WorldTime.cctor()

	s_udStaticApi = CoreUtilities.DescribeNativeUserData 'WorldTime'
end








function WorldTime.FromMicroseconds(iMicroseconds)

	return s_udStaticApi:FromMicroseconds(iMicroseconds)
end

function WorldTime.FromSecondsInt64(iSeconds)

	return s_udStaticApi:FromSecondsInt64(iSeconds)
end

function WorldTime.FromSecondsDouble(fSeconds)

	return s_udStaticApi:FromSecondsDouble(fSeconds)
end

function WorldTime.FromYearMonthDayUTC(iYear, uMonth, iDay)

	return s_udStaticApi:FromYearMonthDayUTC(iYear, uMonth, iDay)
end

function WorldTime.IsCurrentlyDST()

	return s_udStaticApi:IsCurrentlyDST()
end





function WorldTime.ParseISO8601DateTime(sDateTime)

	return s_udStaticApi:ParseISO8601DateTime(sDateTime)
end









function WorldTime.GetUTCTime()

	return s_udStaticApi:GetUTCTime()
end
return WorldTime

