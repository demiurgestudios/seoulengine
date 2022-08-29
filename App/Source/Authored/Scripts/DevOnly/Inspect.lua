--[[
 Inspect.lua
 Runs luainspect on the given file.

 Copyright (c) Demiurge Studios, Inc.
 
 This source code is licensed under the MIT license.
 Full license details can be found in the LICENSE file
 in the root directory of this source tree.
--]]

-- Setup paths as luainspect and metalua expect
package.path = package.path .. ';DevOnly/External/?.lua'

-- Disable checks library to speed up metalua
package.loaded.checks = {}

-- Assign to _G directly to tell the lint checker we intend
-- to do this, workaround for deviation from naming conventions.
_G.checks = function() end

local tValidGlobal = {
	ArgumentException = true,
	arraycreate = true,
	as = true,
	asnum = true,
	bool = true,
	char = true,
	Char = true,
	class = true,
	class_static = true,
	Delegate = true,
	double = true,
	genericlookup = true,
	initlist = true,
	int = true,
	Int32 = true,
	is = true,
	object = true,
	try = true,
	tryfinally = true,
	using = true,
}

-- Get the luainspect.ast and luainspect.init modules
local la = require 'luainspect.ast'
local li = require 'luainspect.init'

-- Monkey patch away slow parts of luainspect
li.eval_comments = function() end
li.infer_values = function() end

return function (sFilename, sSource)
	local aErrors = {}

	local function ErrorWriteLine(sFilename, iLine, iColumn, sMessage)
		local sOutput =
			tostring(sFilename) .. '(' ..
			tostring(iLine) .. ', ' ..
			tostring(iColumn) .. '): '

		if sMessage then
			sOutput = sOutput .. sMessage
		else
			sOutput = sOutput .. 'unknown error'
		end

		aErrors[#aErrors + 1] = sOutput
	end

	local function ErrorWriteLineInfo(tLineInfo, sMessage)
		local li = tLineInfo.first

		ErrorWriteLine(
			tostring(li.source),
			tonumber(li.line),
			tonumber(li.column),
			sMessage)
	end

	local function ErrorReport(...)
		local aArgs = { ... }
		if #aArgs == 2 then
			if aArgs[1] then
				ErrorWriteLineInfo(...)
			else
				io.stderr:write(sFilename .. '(): ' .. aArgs[2], '\n')
			end
		else
			ErrorWriteLine(...)
		end
	end

	local function IsValidGlobalName(sName)
		-- Valid if our exclusion set.
		if tValidGlobal[sName] then
			return true
		end

		-- Valid if the first letter is uppercase.
		local l1 = sName:sub(1,1)
		if l1 == l1:upper() then
			return true
		end

		-- Valid if the name starts with "g_".
		local l2 = sName:sub(1, 2)
		if l2 == 'g_' then
			return true
		end

		-- All other unknown globals are invalid.
		return false
	end

	local function ToLineColumn(tLineInfo)
		local li = tLineInfo.first

		return '(' .. tostring(li.line) .. ', ' .. tostring(li.column) .. ')'
	end

	-- Generate the AST.
	local tAst, sError, iLine, iColumn = la.ast_from_string(sSource, sFilename)
	if not tAst then
		ErrorReport(sFilename, iLine, iColumn, sError)
		return aErrors
	end

	-- Process the AST further.
	li.inspect(tAst, nil, sSource)
	la.ensure_parents_marked(tAst)

	-- Walk the AST looking for additional problems.
	la.walk(tAst, function(tAstNode)
		local sName = tostring(tAstNode[1])
		local tLineInfo = tAstNode.lineinfo

		-- check if we're masking a variable in the same scope
		if tAstNode.localmasking and
			sName ~= '_' and
			tAstNode.level == tAstNode.localmasking.level then

			local tMaskedLineInfo = tAstNode.localmasking.lineinfo
			ErrorReport(
				tLineInfo,
				"local '" .. sName .. "' masks earlier declaration at " .. ToLineColumn(tMaskedLineInfo) .. ".")
		end

		-- check for unused variables - don't warn about 'self',
		-- since it's implicit with a ':' definition.
		if sName ~= 'self' and
			tAstNode.localdefinition == tAstNode and
			not tAstNode.isused and
			not tAstNode.isignore then

			ErrorReport(
				tLineInfo,
				"local '" .. sName .. "' is unused.")
		end

		-- check for multiple assignment mismatch
		if tAstNode.tag == 'Set' or tAstNode.tag == 'Local' then
			local iLength1 = #(tAstNode[1])
			local iLength2 = #(tAstNode[2])
			if iLength2 > iLength1 then
				ErrorReport(
					tLineInfo,
					"value discarded in multiple assignment: " ..
					tostring(iLength2) .. " values assigned to " ..
					tostring(iLength1) .. " variable(s).")
			end
		end

		-- check for incorrect global accesses
		if tAstNode.tag == 'Id' and
			not tAstNode.localdefinition and
			not tAstNode.definedglobal and
			not IsValidGlobalName(sName) then

			ErrorReport(
				tLineInfo,
				"access of implicit global '" .. sName .. "'.")
		end

		-- check for incorrect global assignment
		if tAstNode.tag == 'Id' and
			not tAstNode.localdefinition and
			tAstNode.definedglobal and
			not IsValidGlobalName(sName) then

			local tParent = tAstNode.parent
			local tParentParent = tParent and tParent.parent
			if tParentParent and
				tParentParent.tag == 'Set' and
				tParentParent[1] == tParent then

				ErrorReport(
					tLineInfo,
					"assignment to implicit global '" .. sName .. "'.")
			end
		end
	end)

	-- Return the errors array.
	return aErrors
end
