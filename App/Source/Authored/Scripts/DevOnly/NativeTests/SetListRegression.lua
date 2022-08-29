-- Regression for a bug for a body that
-- generates a very large table from
-- a returned stack
local settingsManager = SeoulNativeNewNativeUserData 'ScriptEngineSettingsManager'
function TestFunction()
	local a = { settingsManager:GetSettingsInDirectory('Currency/', false) }
	print('a is ' .. #a)
	assert(#a > 200)
end

TestFunction()
