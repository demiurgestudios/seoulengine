--[[
 RunInspect.lua
 Command-line utility, runs inspect on all files
 in the passed in project directory. Calls os.exit()
 with 0 or <#-of-failures> depending on success or failure, and
 writes useful output to standard out.

 Copyright (c) Demiurge Studios, Inc.
 
 This source code is licensed under the MIT license.
 Full license details can be found in the LICENSE file
 in the root directory of this source tree.
--]]

-- Import the inspect function
local inspect = require('DevOnly/Inspect')

-- List of path patterns to apply to sFilename to ignore -
-- allows us to filter out external libraries from lint checks.
--
-- The windows shell is terminally awful and "dir *.lua" also returns "*.lua*"
-- so backup files need to be excluded as well.
local aIgnorePatterns =
{
	[1] = 'DevOnly.External',
	[2] = '~',
	[3] = '#',
}

-- Local utility for file testing.
function FileExists(name)
	local f = io.open(name, 'r')
	if f then
		io.close(f)
		return true
	else
		return false
	end
end

-- Local utility for file reading.
local function ReadAll(sFilename)
	local oFile = assert(io.open(sFilename, 'rb'))
	local sData = oFile:read('*all')
	oFile:close()

	-- TODO: This should probably be an explicit error,
	-- since UTF-8 files that contain a BOM will not parse in a
	-- stock version of Lua.

	-- Check for and strip the UTF-8 BOM if present.
	local sUTF8BOM = "\239\187\191"
	if (sData:sub(1, 3) == sUTF8BOM) then
		sData = sData:sub(4)
	end

	return sData
end

-- Set stdout to line-buffered.
io.stdout:setvbuf("line")

local aArguments = {...}
local sProjectDirectory = aArguments and aArguments[1]
if not sProjectDirectory then
	io.stderr:write('missing required argument <project directory>', '\n')
	os.exit(1)
end

-- May be nil: specific filename override
local sProjectFile = aArguments and aArguments[2]

-- Make sure the project directory ends with a slash.
local sProjectDirectoryEnd = sProjectDirectory:sub(sProjectDirectory:len())
if sProjectDirectoryEnd ~= '/' and sProjectDirectoryEnd ~= '\\' then
	sProjectDirectory = sProjectDirectory .. '\\'
end

-- Get a list of files
local aFiles = nil

if sProjectFile then
	-- If a single .lua file was passed in, use that
	local file = sProjectFile
	aFiles = function()
		-- Imitate an iterator: return sProjectFile, then nil on future calls
		local f = file
		file = nil
		return f
	end
else
	-- Otherwise, read the whole folder
	if FileExists('/usr/bin/find') then
		aFiles = io.popen('/usr/bin/find ' .. sProjectDirectory ..  ' -name \\*.lua | cat'):lines()
	else
		-- If not Linux, assume Windows
		aFiles = io.popen('dir /s/b ' .. sProjectDirectory .. '*.lua'):lines()
	end
end


-- Results array and counts
local aResults = {}
local iOk = 0
local iFail = 0

-- Now process each file.
for sFilename in aFiles do
	-- Clean the filename - remove the absolute part of the path.
	sFilename = sFilename:sub(sProjectDirectory:len() + 1)

	-- Skip files in the include set.
	local bSkip = false
	for _,v in ipairs(aIgnorePatterns) do
		if sFilename:find(v) then
			bSkip = true
			break
		end
	end

	-- Process the file if we're not skipping it
	if not bSkip then
		-- Read the file
		local sSource = ReadAll(sFilename)

		-- Run inspect
		local aErrors = inspect(sFilename, sSource)

		-- Success or failure
		local bOk = (not aErrors or (#aErrors == 0))

		-- Track
		if not bOk then
			iFail = iFail + 1
			-- Output errors immediately
			for _,sError in ipairs(aErrors) do
				io.stdout:write(sError, '\n')
			end
		else
			iOk = iOk + 1
		end

		-- Status output
		if not bOk then
			io.stdout:write('. Inspecting ' .. sFilename .. ': ' .. 'FAIL', '\n')
		end

		-- Append results
		aResults[#aResults + 1] = { m_sFilename = sFilename, m_aErrors = aErrors }
	end
end

-- Done
io.stdout:write(
	(iFail > 0 and 'FAIL' or 'OK') .. ' (PASS: ' .. tostring(iOk) ..
	', FAIL: ' .. tostring(iFail) ..
	', TOTAL: ' .. tostring(iOk + iFail) .. ')', '\n\n')
os.exit(iFail)
