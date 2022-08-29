--[[
 RunUnitTests.lua
 Command-line utility, runs lunatest on all files
 in the passed in tests directory.

 Copyright (c) Demiurge Studios, Inc.
 
 This source code is licensed under the MIT license.
 Full license details can be found in the LICENSE file
 in the root directory of this source tree.
--]]

-- Manually initialize some SlimCS managed dependencies.
require 'SlimCSMain'
require 'Engine/Native/ScriptEnginePath'
require 'Engine/Core/Native'
require 'Engine/Core/Path'
require 'Engine/Core/Utilities'
require 'Engine/Native/ScriptEngine'
require 'Engine/Native/ScriptEngineCore'
require 'Engine/Engine/Engine'
require 'Engine/Native/ScriptEngineFileManager'
require 'Engine/Engine/FileManager'
require 'Engine/Native/ScriptEngineInputManager'
require 'Engine/Engine/InputManager'
FinishClassTableInit(CoreNative)
FinishClassTableInit(CoreUtilities)
FinishClassTableInit(Engine)
FinishClassTableInit(FileManager)
FinishClassTableInit(InputManager)
FinishClassTableInit(Path)
CoreNative.cctor()
Engine.cctor()
FileManager.cctor()
InputManager.cctor()
Path.cctor()

-- Bind os.exit for reporting.
_G.os = _G.os or {}
_G.os.exit = _G.os.exit or function(code)
	if code ~= 0 then
		error('Exit code ' .. tostring(code))
	end
end
_G.os.time = _G.os.time or function()
	return Engine.GetCurrentServerTime():GetSecondsAsDouble()
end

-- Get the list of test files.
local sProjectDirectory = Path.Combine(FileManager.GetSourceDir(), 'Authored/Scripts/')
local sTestsDir = Path.Combine(sProjectDirectory, 'DevOnly/UnitTests')
local aFiles = FileManager.GetDirectoryListing(sTestsDir, true, ".lua")

-- Import luna test
local lunatest = require('DevOnly/External/lunatest/lunatest')

-- Now process each file.
for _,sFilename in ipairs(aFiles) do
	-- Clean the filename - remove the absolute part of the path.
	sFilename = sFilename:sub(sProjectDirectory:len() + 1)

	-- Clean the filename - reformat to be a valid argument to require()
	sFilename = sFilename:gsub('.lua', '')
	sFilename = sFilename:gsub('\\', '/')

	-- Add the file to lunatest as a suite.
	lunatest.suite(sFilename)
end

-- Run lunatest, it will handle output and os.exit()
local sLastSuiteName = ""
lunatest.run(
	{
		begin = function(_--[[res]], _--[[aSuites]])
			end,
		begin_suite = function(tSuite, _--[[aTests]])
			sLastSuiteName = tSuite.name
			end,
		end_suite = function(_--[[tSuite]])
			end,
		pre_test = false,
		post_test = function(sName, tResult)
			local sType = tResult:type()

			-- Log an entry for the test, with suite name
			-- and test name.
			io.stdout:write('. Running test ' .. sLastSuiteName .. '.' .. sName .. ': ' ..
				sType:upper(), '\n')

			-- Log error details on fail or error.
			if sType == 'fail' or sType == 'error' then
				io.stdout:write(tResult:tostring(sName), '\n')
			end
		end,
		done = function(tResults)
			-- Utility function to count test result buckets.
			local function Count(t)
				local iCount = 0
				for _ in pairs(t) do
					iCount = iCount + 1
				end
				return iCount
			end

			-- Count the 4 types (pass, fail, skip, and error).
			local iPass = Count(tResults.pass)
			local iFail = Count(tResults.fail)
			local iSkip = Count(tResults.skip)
			local iError = Count(tResults.err)
			local iTotalFail = (iFail + iError)

			local sResult = iTotalFail > 0 and 'FAIL' or 'OK'

			-- Final results.
			io.stdout:write(sResult .. ' (PASS: ' .. tostring(iPass) ..
				', FAIL: ' .. tostring(iTotalFail) ..
				', TOTAL: ' .. tostring(iPass + iFail + iSkip + iError) ..
				')', '\n\n')
		end,
	},
	{
		verbose=true
	})
