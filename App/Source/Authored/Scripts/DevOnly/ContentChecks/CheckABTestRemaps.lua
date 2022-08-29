--[[
	CheckABTestRemaps.lua
	Applies checking beyond basic .json file testing that is specific
	to A/B test remap files (remap files allow retargeting of content
	or config files for the purpose of A/B testing - they are a dependency
	of A/B test config files).

	Copyright (c) Demiurge Studios, Inc.
	
	This source code is licensed under the MIT license.
	Full license details can be found in the LICENSE file
	in the root directory of this source tree.
--]]

-- Early out if no directory.
if not SettingsManager.Exists 'ABTestConfigs/Remaps' then
	return true
end

-- Get all remap files.
local aFiles = table.pack(SettingsManager.GetSettingsInDirectory('ABTestConfigs/Remaps'))

-- Tracking - two files can never contain the same remap.
local tChecking = {}

local bReturn = true
for i=1,#aFiles,2 do
	local file = aFiles[i]
	local source = aFiles[i+1]
	for _,e in ipairs(file) do
		local from = e.From
		local to = e.To

		if from:GetType() ~= to:GetType() then
			bReturn = false
			CoreNative.Warn('Type mismatch in A/B test config "' .. source:ToString() .. '", ' ..
				'source file "' .. from:ToString() .. '" and target file "' .. to:ToString() .. '" have ' ..
				'different types.')
		end

		-- Update
		tChecking[from] = source
	end
end

return bReturn
