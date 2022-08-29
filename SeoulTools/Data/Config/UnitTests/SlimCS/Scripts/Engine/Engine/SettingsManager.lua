-- Wrapper and access to JSON files from script.
--
-- Copyright (c) Demiurge Studios, Inc.
-- 
-- This source code is licensed under the MIT license.
-- Full license details can be found in the LICENSE file
-- in the root directory of this source tree.



local SettingsManager = class_static 'SettingsManager'; local s_udSettings; local s_tCache



function SettingsManager:cctor() s_udSettings = CoreUtilities.NewNativeUserData(ScriptEngineSettingsManager); s_tCache = CoreUtilities.MakeWeakTable(); end


function SettingsManager.Exists(sPath)

	local bRet = s_udSettings:Exists(sPath)
	return bRet
end

function SettingsManager.GetSettings(sPath)

	-- Get from cache.
	local t = s_tCache[sPath]
	if nil ~= t then

		return t
	end

	-- Otherwise, try to get settings from the native user data.
	t = s_udSettings:GetSettings(sPath)

	-- If successful, cache it in the s_tCache table.
	if nil ~= t then

		s_tCache[sPath] = t
	end

	-- Either way, return the value of t.
	return t
end

function SettingsManager.IsQAOrLocal()

	local clientSettings = SettingsManager.GetSettings 'ClientSettings'
	local env = clientSettings.ServerType

	return env == nil or env == 'LOCAL' or env == 'QA'
end

function SettingsManager.GetSettingsAsJsonString(sPath)

	return s_udSettings:GetSettingsAsJsonString(sPath)
end

function SettingsManager.GetSettingsInDirectory(sPath, bRecursive)

	return s_udSettings:GetSettingsInDirectory(sPath, bRecursive)
end

function SettingsManager.SetSettings(fPath, data)

	s_udSettings:SetSettings(fPath, data)
end

function SettingsManager.ValidateSettings(sWildcard, bCheckDependencies)

	return s_udSettings:ValidateSettings(sWildcard, bCheckDependencies)
end
return SettingsManager
