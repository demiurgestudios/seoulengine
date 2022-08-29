--[[
 BasicAutomation.lua
 Starting point/simple automation test.

 Copyright (c) Demiurge Studios, Inc.
 
 This source code is licensed under the MIT license.
 Full license details can be found in the LICENSE file
 in the root directory of this source tree.
--]]

-- Manually initialize some SlimCS managed dependencies.
require 'SlimCSMain'
require 'Engine/Core/Native'
require 'Engine/Core/Utilities'
require 'Engine/Native/ScriptEngine'
require 'Engine/Native/ScriptEngineCore'
require 'Engine/Engine/Engine'
require 'Engine/Native/ScriptEngineInputManager'
require 'Engine/Engine/InputManager'
require 'Engine/Native/ScriptEngineSettingsManager'
require 'Engine/Engine/SettingsManager'
FinishClassTableInit(CoreUtilities)
FinishClassTableInit(CoreNative)
FinishClassTableInit(Engine)
FinishClassTableInit(InputManager)
FinishClassTableInit(SettingsManager)
CoreNative.cctor()
Engine.cctor()
InputManager.cctor()
SettingsManager.cctor()

-- Convenience, recursively print a table.
local function PrintTable(value, printFunc)
	if printFunc == nil then
		printFunc = print
	end

	local tCache={}
	local function nestedPrint(t, indent)
		if (tCache[tostring(t)]) then
			printFunc(indent .. 'ref ' .. tostring(t))
		else
			tCache[tostring(t)] = true
			printFunc(indent .. '{')
			indent = indent .. '  '
			for k,v in pairs(t) do
				if 'table' == type(v) then
					printFunc(indent .. tostring(k) .. ' =')
					nestedPrint(v, indent)
				else
					printFunc(indent .. tostring(k) .. ' = ' .. tostring(v))
				end
			end
			printFunc(indent .. '}')
		end
	end

	if 'table' == type(value) then
		nestedPrint(value, '')
	else
		printFunc(value)
	end
end

-- Min time test will run.
local kfMinTimeSeconds = 5.0
-- Max time allocated to complete the automation sequence.
local kfMaxTimeInSeconds = 60.0

-- Screens that we care about reaching during the automated test interval.
-- The value is the count of times we need to reach the screen for the
-- test to be considered successful.
local tMustReachScreens = {}
local tScreenAliases = {}

-- Raises an error if any listed target screen is not defined
-- in the corresponding UI config.
local function VerifyTargetScreens()
	for machineName,stateMachine in pairs(tMustReachScreens) do
		local cfg = SettingsManager.GetSettings('UI/' .. machineName)
		if not cfg then
			error('config .json for state machine "' .. machineName .. '" is not defined.')
		end

		for stateName,_ in pairs(stateMachine) do
			if not cfg[stateName] then
				error('state "' .. stateName .. '" is not defined in the config for state machine "' .. machineName .. '"')
			end
		end
	end
end

-- Function to check tMustReachScreens
local function ReachedAllTargetScreens()
	for _,v in pairs(tMustReachScreens) do
		-- If the sub table is not empty, still screens remaining.
		if next(v) then
			return false
		end
	end

	return true
end

-- Local variable to cache the native function userdata
local s_udNativeAutomationFunctions = nil
local s_fUptimeBase = nil

-- Local variable to cache the table of automators.
local s_tAutomators = {}
local s_FallbackAutomator = nil

-- Table to track current UI state.
local s_tUIState = {}

-- Table to track global game state.
local s_tGameState = {}

-- Utility check functions
local s_tChecks = {}

-- Filter table, don't print these state machines.
local ktUIStateFilter = {
	Empty = true,
}
local ktUIStateMachineFilter = {
	FxPreview = true,
	GlobalBroadcastHandler = true,
	VideoCapture = true,
}

-- Convenience, builds a string with the current state machine state.
local function GetStateString()
	local sState = "{"
	local iCount = 0
	for k,v in pairs(s_tUIState) do
		if not ktUIStateMachineFilter[k] and not ktUIStateFilter[v] then
			if iCount > 0 then
				sState = sState .. ", "
			end
			iCount = iCount + 1

			sState = sState .. k .. ": " .. v
		end
	end
	sState = sState .. "}"
	return sState
end
s_tGameState.GetStateString = GetStateString

-- Run all checks
local function RunChecks(bShutdown)
	for _,v in ipairs(s_tChecks) do
		v(bShutdown)
	end
end

-- Utility, selects the best automator given current game state.
local function GetAutomator()
	-- Fallback is initially the best.
	local best = s_FallbackAutomator
	for k,v in pairs(s_tUIState) do
		local tAutomators = s_tAutomators[k]
		if tAutomators then
			local automator = tAutomators[v]

			-- Higher priority automators win, so
			-- use this one if it's higher priority.
			if automator and automator.m_iPriority > best.m_iPriority then
				best = automator
			end
		end
	end

	return best.m_Func
end

-- Convenience wrapper, generates a table for an automator.
local function CreateAutomator(func, iPriority)
	return { m_Func = func, m_iPriority = iPriority }
end

-- Global Initialize function, called at startup. Passed
-- the userdata wrapper to global native functions that
-- can be accessed from automation scripts.
function _G.Initialize(udNativeAutomationFunctions)
	-- Turn on server/client time checking.
	udNativeAutomationFunctions:EnableServerTimeChecking(true)

	-- Verify that target screens are all defined.
	VerifyTargetScreens()

	-- Cache native automator functions.
	s_udNativeAutomationFunctions = udNativeAutomationFunctions
	s_fUptimeBase = s_udNativeAutomationFunctions:GetUptimeInSeconds()

	-- Initialize general check utilities.
	table.insert(s_tChecks, require("DevOnly/AutomatedTests/Utilities/VmCheck")(udNativeAutomationFunctions))
	table.insert(s_tChecks, require("DevOnly/AutomatedTests/Utilities/HStringCheck")(udNativeAutomationFunctions))
	table.insert(s_tChecks, require("DevOnly/AutomatedTests/Utilities/MemoryCheck")(udNativeAutomationFunctions))

	-- Now set a fallback.
	s_FallbackAutomator = CreateAutomator(
		(require("DevOnly/AutomatedTests/AutoPlay/DefaultAutoPlay"))(udNativeAutomationFunctions, s_tGameState, s_tUIState),
		1)

	s_udNativeAutomationFunctions:Log("Initialize.")
end

function _G.OnUIStateChange(sStateMachineId, _--[[sPreviousStateId]], sNextStateId)
	-- Cache the state ID for the state machine that just changed.
	s_tUIState[sStateMachineId] = sNextStateId

	-- Check and mark this state as reached
	-- by decrementing and then potentially clearing it from
	-- the table.
	local tStates = tMustReachScreens[sStateMachineId]
	if type(tStates) == 'table' then
		sNextStateId = tScreenAliases[sNextStateId] and tScreenAliases[sNextStateId] or sNextStateId
		if tStates[sNextStateId] then
			tStates[sNextStateId] = tStates[sNextStateId] - 1
			if tStates[sNextStateId] <= 0 then
				tStates[sNextStateId] = nil
			end
		end
	end
end

-- Global PreTick function, called once per frame. Must
-- return true or false - true to signal that the game
-- should keep running, false otherwise.
function _G.PreTick()
	-- Done if we hit all our target screens or hit the timeout interval.
	local fUptimeSeconds = s_udNativeAutomationFunctions:GetUptimeInSeconds() - s_fUptimeBase
	local bReachedAllTargetScreens = ReachedAllTargetScreens()
	if (bReachedAllTargetScreens and fUptimeSeconds >= kfMinTimeSeconds) or fUptimeSeconds >= kfMaxTimeInSeconds then
		-- Completion time.
		s_udNativeAutomationFunctions:Log("Done in " .. fUptimeSeconds .. " seconds.")

		-- Warn about failure.
		if not bReachedAllTargetScreens then
			s_udNativeAutomationFunctions:Log("FAILURE.")
			s_udNativeAutomationFunctions:Warn(
				"AUTOMATED TEST FAILURE: Did not reach all target screens in " .. kfMaxTimeInSeconds .. " seconds, see log.")
			s_udNativeAutomationFunctions:Warn('REMAINING TARGET SCREENS:')
			PrintTable(tMustReachScreens, function(s) s_udNativeAutomationFunctions:Warn(s) end)
		else
			s_udNativeAutomationFunctions:Log("SUCCESS.")
		end

		return false
	end

	-- Run an automator based on current game state.
	local automator = GetAutomator()
	automator()

	-- General utility checkers (HStrings and Memory, etc.)
	RunChecks(false)

	return true
end

-- Global PostTick function, called once per frame. Must
-- return true or false - true to signal that the game
-- should keep running, false otherwise.
function _G.PostTick()
	return true
end

-- Global PreShutdown function, called once, immediately
-- before game script termination, after the UI has
-- been cleared.
function _G.PreShutdown()
	RunChecks(true)
	return true
end

-- Global SetGlobalState function, called by the environment
-- to update various game state.
function _G.SetGlobalState(key, value)
	s_tGameState[key] = value
end
