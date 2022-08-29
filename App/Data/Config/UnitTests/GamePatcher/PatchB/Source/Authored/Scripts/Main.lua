_G.GlobalPatchB = function()
	return "PatchB"
end

local ud = _G.SeoulNativeNewNativeUserData('ScriptEngineSettingsManager')
local settings = ud:GetSettings('gui')

_G.GlobalSetting = function()
	return settings.ScriptSettingTest
end
