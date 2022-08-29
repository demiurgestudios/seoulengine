--[[
	VmCheck.lua
	Applies a check against Vm stats and performs reporting. Can be used in
	multiple automated tests.

	Copyright (c) Demiurge Studios, Inc.
	
	This source code is licensed under the MIT license.
	Full license details can be found in the LICENSE file
	in the root directory of this source tree.
--]]

-- Max values above which a warning is issued.
local ktMaxValues = {
	UIBindingUserData = 60000,
}

-- Track whether we've issued a warning or not.
local s_tIssuedWarning = {}

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
	return function(bShutdown)
		local tStats = udNativeAutomationFunctions:GetVmStats()
		local bNeedStats = false

		-- Different checking on shutdown - leak checking,
		-- since all Vm elements should have been cleaned
		-- up by this point.
		if bShutdown then
			for k,v in pairs(tStats) do
				if v > 0 then
					udNativeAutomationFunctions:Warn(
						"Vm stat '" .. k .. "' is > 0 at " .. v .. ", this indicates a leak.")
					bNeedStats = true
				end
			end
		else
			for k,v in pairs(tStats) do
				if ktMaxValues[k] and ktMaxValues[k] < v then
					if not s_tIssuedWarning[k] or (v > (s_tIssuedWarning[k] + s_tIssuedWarning[k] * 0.1)) then
						s_tIssuedWarning[k] = v
						udNativeAutomationFunctions:Warn(
							"Vm stat '" .. k .. "' has exceeded the max value of " ..
							ktMaxValues[k] .. " at " .. v .. ".")
						bNeedStats = true
					end
				end
			end
		end

		if bNeedStats then
			udNativeAutomationFunctions:Warn("Vm stats:")
			udNativeAutomationFunctions:Warn(TableToString(tStats))
			udNativeAutomationFunctions:Warn("Instance counts per movie:")
			udNativeAutomationFunctions:LogInstanceCountsPerMovie()
			if bShutdown then
				udNativeAutomationFunctions:Warn("Dangling nodes:")
				udNativeAutomationFunctions:LogGlobalUIScriptNodes(true)
			else
				udNativeAutomationFunctions:Warn("See log for node list.")
				udNativeAutomationFunctions:LogGlobalUIScriptNodes(false)
			end
		end
	end
end
