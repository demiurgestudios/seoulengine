--[[
	CheckABTestConfigs.lua
	Enforces constraints on A/B test config files.

	Copyright (c) Demiurge Studios, Inc.
	
	This source code is licensed under the MIT license.
	Full license details can be found in the LICENSE file
	in the root directory of this source tree.
--]]

-- Early out if no directory.
if not SettingsManager.Exists 'ABTestConfigs' then
	return true
end

-- Get all remap files.
local aFiles = table.pack(SettingsManager.GetSettingsInDirectory('ABTestConfigs'))

local bReturn = true
for i=1,#aFiles,2 do
	local file = aFiles[i]
	local source = aFiles[i+1]

	-- Need to have at least 2 entries.
	if #file < 2 then
		CoreNative.Warn('A/B test config "' .. source:ToString() .. '" must have at least 2 groups.')
	end

	-- Entry 1 must have an empty remap. A is always the control.
	if file[1] then
		if file[1].Remaps and #file[1].Remaps > 0 then
			CoreNative.Warn('A/B test config "' .. source:ToString() .. '" has an "A" group with remaps. "A" must always be the control group (empty remap list).')
		end
	end

	-- Now check weight total. Must be > 0
	local fTotalWeight = 0
	for _,e in ipairs(file) do
		fTotalWeight = fTotalWeight + e.Weight
	end

	if fTotalWeight <= 1e-5 then
		CoreNative.Warn('A/B test config "' .. source:ToString() .. '" has invalid weights. Total weight must be > 0 not "' .. tostring(fTotalWeight) .. '".')
	end
end

return bReturn
