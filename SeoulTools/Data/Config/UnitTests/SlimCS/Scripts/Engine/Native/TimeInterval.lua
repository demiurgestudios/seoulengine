--[[
	TimeInterval.lua
	AUTO GENERATED - DO NOT MODIFY
	API FOR NATIVE CLASS INSTANCE

	Run GenerateScriptBindings.bat in the Utilities folder to re-generate bindings.

	Copyright (c) Demiurge Studios, Inc.
	
	This source code is licensed under the MIT license.
	Full license details can be found in the LICENSE file
	in the root directory of this source tree.
]]





local TimeInterval = class('TimeInterval', nil, 'Native.TimeInterval')

interface('IStatic0000', 'Native.TimeInterval+IStatic', 'IStatic')








local s_udStaticApi = nil

function TimeInterval.cctor()

	s_udStaticApi = CoreUtilities.DescribeNativeUserData 'TimeInterval'
end









function TimeInterval.FromMicroseconds(a0)

	return s_udStaticApi:FromMicroseconds(a0)
end

function TimeInterval.FromSecondsInt64(a0)

	return s_udStaticApi:FromSecondsInt64(a0)
end

function TimeInterval.FromSecondsDouble(a0)

	return s_udStaticApi:FromSecondsDouble(a0)
end

function TimeInterval.FromHoursInt64(a0)

	return s_udStaticApi:FromHoursInt64(a0)
end

function TimeInterval.FromDaysInt64(a0)

	return s_udStaticApi:FromDaysInt64(a0)
end




return TimeInterval

