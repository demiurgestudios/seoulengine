_G.GlobalBase = function()
	return "Base"
end

local ud = _G.SeoulNativeNewNativeUserData('ScriptEngineSettingsManager')
local settings = ud:GetSettings('gui')

_G.GlobalSetting = function()
	return settings.ScriptSettingTest
end
