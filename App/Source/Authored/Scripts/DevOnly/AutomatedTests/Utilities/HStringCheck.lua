--[[
	HStringCheck.lua
	Applies a check against HString stats and performs reporting. Can be used in
	multiple automated tests.

	Copyright (c) Demiurge Studios, Inc.
	
	This source code is licensed under the MIT license.
	Full license details can be found in the LICENSE file
	in the root directory of this source tree.
--]]

-- Max HStrings above which a warning is issued.
local kiHStringTableCapacity = 524288
local kiMaxHStrings = math.floor(kiHStringTableCapacity * 0.20)

-- Track whether we've issued an HString warning or not.
local s_bIssuedHStringWarning = false

local function TableToString(t)
	local tSorted = {}
	for k,v in pairs(t) do
		table.insert(tSorted, tostring(k) .. ": " .. tostring(v))
	end
	table.sort(tSorted)

	return table.concat(tSorted, "\n")
end

return function(udNativeAutomationFunctions)
	-- Check and potentially issue a memory warning.
	return function()
		local tHStringStats = udNativeAutomationFunctions:GetHStringStats()
		local iTotalHStrings = tHStringStats.TotalHStrings

		if iTotalHStrings > kiMaxHStrings then
			if not s_bIssuedHStringWarning then
				s_bIssuedHStringWarning = true
				udNativeAutomationFunctions:Warn(
					"HStrings have exceeded threshold of " .. kiMaxHStrings .. " with " ..
					iTotalHStrings .. " total HStrings.")
				udNativeAutomationFunctions:Warn(
					"HString stats:")
				udNativeAutomationFunctions:Warn(TableToString(tHStringStats))
				udNativeAutomationFunctions:LogAllHStrings()
			end
		end
	end
end
