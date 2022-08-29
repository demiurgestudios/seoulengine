--[[
 DefaultAutoPlayBehavior.lua
 The implementation of the default auto play behavior, so it can
 be used in other behaviors.

 Copyright (c) Demiurge Studios, Inc.
 
 This source code is licensed under the MIT license.
 Full license details can be found in the LICENSE file
 in the root directory of this source tree.
--]]

local k_tInputMask = {
	m_iOff = 0,
	m_iClickInputOnly = 1, -- (1 << 0)
	m_iDragInputOnly = 2, -- (1 << 1)
	m_iClickAndDragInput = 3, -- (1 << 0) | (1 << 1)
	m_iCardContainer = 4, -- (1 << 2)
	m_iMaskAll = 255, -- (0xFF)
}

local tCounts = {}
local prevPointId = nil

local function round(f)
	return f > 0 and math.floor(f + 0.5) or math.ceil(f - 0.5)
end

-- We quantize positions as a form of bucketing, so that slight
-- animations are not detected as new hit points.
local function quantizePos(f)
	return round(f / 10.0) * 10.0
end

local function GetPoint(v)
	local tapPoint = v.TapPoint
	local centerPoint = v.CenterPoint
	local stateMachine = v.StateMachine
	local state = v.State
	local devOnlyInternalStateId = v.DevOnlyInternalStateId
	local movie = v.Movie
	local id = v.Id
	local iCenterX = quantizePos(centerPoint[1])
	local iCenterY = quantizePos(centerPoint[2])
	local pointId = stateMachine .. '|' .. state .. '|' .. iCenterX .. '|' .. iCenterY
	if devOnlyInternalStateId and '' ~= devOnlyInternalStateId then
		pointId = pointId .. '|' .. devOnlyInternalStateId
	end

	return id, stateMachine, state, movie, tapPoint[1], tapPoint[2], pointId
end

local function PickBestPoint(a)
	local best = nil
	local bestCount = nil

	for _,v in ipairs(a) do
		local _, _, _, _, _, _, pointId = GetPoint(v)

		local count = tCounts[pointId] or 0
		if not bestCount or count <= bestCount then
			best = v
			bestCount = count
		end
	end
	return best
end

local iFramesNoPoints = 0

return function(udNativeAutomationFunctions, tGameState, tUIState, fFilterX, fFilterY)
	-- If specified, wait a bit between actions.
	if tGameState.InsertDelaysBetweenActions then
		for _=1,10 do
			coroutine.yield()
		end
	end

	-- Special handling if we're in the player registration screen.
	if tUIState.PlayerName == 'PlayerNameInput' or tUIState.Popups == 'NameChange' then
		local sPlayerName = string.sub(
			"AT " .. udNativeAutomationFunctions:GetCurrentClientWorldTimeInMilliseconds(),
			1,
			14)

		-- 7175 is a bad word in leet speak, so filter it.
		sPlayerName = sPlayerName:gsub('7175', '7157')
		-- 8008 is also a bad wor din leet speak, so also filter it.
		sPlayerName = sPlayerName:gsub('8008', '8080')

		udNativeAutomationFunctions:Log(
			"[DefaultAutoPlay]: Setting player name text field to '" .. sPlayerName .. "', this name " ..
			"may or may not be submitted depending on what I decide to tap next.")
		udNativeAutomationFunctions:BroadcastEventTo(
			'PlayerNameInput',
			'GameAutomationSetPlayerNameField',
			sPlayerName)
	end

	local aPoints = udNativeAutomationFunctions:GetHitPoints(k_tInputMask.m_iClickInputOnly)
	local v = PickBestPoint(aPoints)
	if not v then
		iFramesNoPoints = iFramesNoPoints + 1
		if iFramesNoPoints % 1200 == 0 then
			udNativeAutomationFunctions:Log("[DefaultAutoPlay]: " .. tGameState.GetStateString())
			udNativeAutomationFunctions:Log("[DefaultAutoPlay]: Went " .. tostring(iFramesNoPoints) .. " frames with no points to tap.")
			udNativeAutomationFunctions:Log("[DefaultAutoPlay]: SystemBindingLock active: " .. tostring(InputManager.HasSystemBindingLock()))

			local vWhitelist = udNativeAutomationFunctions:GetUIInputWhitelist()
			if #vWhitelist == 0 then
				udNativeAutomationFunctions:Log("[DefaultAutoPlay]: Input whitelist is empty.")
			else
				for _, v in ipairs(vWhitelist) do
					local name = tostring(v)
					if '' == name then
						udNativeAutomationFunctions:Log("[DefaultAutoPlay]: Whitelist: '' (block all input)")
					else
						udNativeAutomationFunctions:Log("[DefaultAutoPlay]: Whitelist: '" .. name .. "'")
					end
				end
			end

			local tConditions = udNativeAutomationFunctions:GetUIConditions()
			for k, v in pairs(tConditions) do
				udNativeAutomationFunctions:Log("[DefaultAutoPlay]: Condition Var: '" .. k .. "' = " .. tostring(v))
			end
		end
		return
	end

	-- Reset
	iFramesNoPoints = 0

	-- Get state.
	local _, stateMachine, state, movie, iX, iY, pointId = GetPoint(v)

	-- Track changes.
	local count = tCounts[pointId]
	count = count and count + 1 or 1
	tCounts[pointId] = count

	-- backpropogate count to the previous point, if it's lower
	-- we do this to try and help drive the UI to unexplored state
	-- which may be one or more transitions away
	if prevPointId and tCounts[prevPointId] > count then
		tCounts[prevPointId] = count
	end
	prevPointId = pointId

	-- Exclude tapping on filter areas (starting in the upper left corner).
	if (not fFilterX and not fFilterY) or (iX > fFilterX and iY > fFilterY) then
		-- Tap and hold every other time we hit a button.
		local bTapAndHold = count % 2 == 0

		-- Activate
		local sLongName = udNativeAutomationFunctions:GetHitPointLongName(v)
		udNativeAutomationFunctions:Log("[DefaultAutoPlay]: Tapping '" .. stateMachine .. ":" .. state .. ":" .. movie .. ":" .. sLongName .. "' at (" .. iX .. ", " .. iY .. ")" .. " points: " .. #aPoints .. " count: " .. count .. " tap-and-hold: " .. tostring(bTapAndHold) .. " point-id: " .. tostring(pointId))
		udNativeAutomationFunctions:QueueMouseMoveEvent(iX, iY)
		udNativeAutomationFunctions:QueueLeftMouseButtonEvent(true)
		coroutine.yield()

		-- If need to tap and hold, yield longer.
		if bTapAndHold then
			for _=1,20 do
				coroutine.yield()
			end
		end

		udNativeAutomationFunctions:QueueLeftMouseButtonEvent(false)

		-- Always yield a bit, pace the auto test so it is not executing
		-- completely unrealistic behaviors.
		for _=1,3 do
			coroutine.yield()
		end
	end
end
