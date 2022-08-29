--[[
	CheckJson.lua
	Enumerates all .json files in the App. Loads them to
	apply basic schema checking (inherently part of file
	loading) and can optionally apply additional checking.

	Copyright (c) Demiurge Studios, Inc.
	
	This source code is licensed under the MIT license.
	Full license details can be found in the LICENSE file
	in the root directory of this source tree.
--]]

-- Load all .json settings - this will also apply schema checks.
-- For command-line checks, we skip checking the existence of referenced
-- content (second argument false)
local bReturn = SettingsManager.ValidateSettings('UnitTests/*', false)

-- TODO: Apply additional checks that are specific to certain
-- files.

return bReturn
