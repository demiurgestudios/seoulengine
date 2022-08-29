-- Access to the native UIManager singleton.
--
-- Copyright (c) Demiurge Studios, Inc.
-- 
-- This source code is licensed under the MIT license.
-- Full license details can be found in the LICENSE file
-- in the root directory of this source tree.





local UIManager = class_static 'UIManager'

local udUIManager = nil

function UIManager.cctor()

	-- Before further action, capture any methods that are not safe to be called
	-- from off the main thread (static initialization occurs on a worker thread).
	CoreUtilities.MainThreadOnly(
		UIManager,
		'DebugLogEntireUIState'--[['DebugLogEntireUIState']],
		'GetRootMovieClip'--[['GetRootMovieClip']],
		'GetStateMachineCurrentStateId'--[['GetStateMachineCurrentStateId']],


		'GotoState'--[['GotoState']],
		'ValidateUiFiles'--[['ValidateUiFiles']])



	udUIManager = CoreUtilities.NewNativeUserData(ScriptUIManager)
	if nil == udUIManager then

		error 'Failed instantiating ScriptUIManager.'
	end
end

function UIManager.BroadcastEvent(sEvent, ...)

	local bRet = udUIManager:BroadcastEvent(sEvent, ...)
	return bRet
end

function UIManager.BroadcastEventTo(sTargetType, sEvent, ...)

	local bRet = udUIManager:BroadcastEventTo(sTargetType, sEvent, ...)
	return bRet
end

function UIManager.GetRootMovieClip(sStateMachine, sTarget)

	return udUIManager:GetRootMovieClip(sStateMachine, sTarget)
end

function UIManager.GetCondition(sName)

	local bCondition = udUIManager:GetCondition(sName)
	return bCondition
end

function UIManager.GetPerspectiveFactorAdjustment()

	local fFactor = udUIManager:GetPerspectiveFactorAdjustment()
	return fFactor
end

function UIManager.PersistentBroadcastEvent(sEvent, ...)

	local bRet = udUIManager:PersistentBroadcastEvent(sEvent, ...)
	return bRet
end

function UIManager.PersistentBroadcastEventTo(sTargetType, sEvent, ...)

	local bRet = udUIManager:PersistentBroadcastEventTo(sTargetType, sEvent, ...)
	return bRet
end

function UIManager.SetCondition(sName, bValue)

	udUIManager:SetCondition(sName, bValue)
end

function UIManager.SetPerspectiveFactorAdjustment(fFactor)

	udUIManager:SetPerspectiveFactorAdjustment(fFactor)
end

function UIManager.SetStage3DSettings(sName)

	udUIManager:SetStage3DSettings(sName)
end

function UIManager.TriggerTransition(sName)

	udUIManager:TriggerTransition(sName)

	if CoreNative.IsLogChannelEnabled('UIDebug'--[[LoggerChannel.UIDebug]]) then

		-- Complementary to similar logging that will occur in native,
		-- completes the stack.
		CoreNative.LogChannel('UIDebug'--[[LoggerChannel.UIDebug]], debug.traceback())
	end

end

function UIManager.GetViewportAspectRatio()

	return udUIManager:GetViewportAspectRatio()
end

function UIManager.DebugLogEntireUIState()


	udUIManager:DebugLogEntireUIState()
end

function UIManager.ComputeWorldSpaceDepthProjection(posX, posY, worldDepth)

	return udUIManager:ComputeWorldSpaceDepthProjection(posX, posY, worldDepth)
end

function UIManager.ComputeInverseWorldSpaceDepthProjection(posX, posY, worldDepth)

	return udUIManager:ComputeInverseWorldSpaceDepthProjection(posX, posY, worldDepth)
end

function UIManager.GetStateMachineCurrentStateId(sStateName)

	return udUIManager:GetStateMachineCurrentStateId(sStateName)
end

function UIManager.AddToInputWhitelist(movieClip)

	udUIManager:AddToInputWhitelist(rawget(movieClip, 'm_udNativeInstance'))
end

function UIManager.ClearInputWhitelist()

	udUIManager:ClearInputWhitelist()
end

function UIManager.GetPlainTextString(text)

	return udUIManager:GetPlainTextString(text)
end

function UIManager.HasPendingTransitions()

	return udUIManager:HasPendingTransitions()
end

function UIManager.RemoveFromInputWhitelist(movieClip)

	udUIManager:RemoveFromInputWhitelist(rawget(movieClip, 'm_udNativeInstance'))
end

function UIManager.SetInputActionsEnabled(bEnabled)

	udUIManager:SetInputActionsEnabled(bEnabled)
end


function UIManager.GotoState(stateMachineName, stateName)

	udUIManager:GotoState(stateMachineName, stateName)
end

function UIManager.ValidateUiFiles(sExcludeWildcard)

	return udUIManager:ValidateUiFiles(sExcludeWildcard)
end

function UIManager.ShelveDataForHotLoad(sId, data)

	udUIManager:ShelveDataForHotLoad(sId, data)
end

function UIManager.UnshelveDataFromHotLoad(T, sId)

	return udUIManager:UnshelveDataFromHotLoad(sId)
end
-- /#if DEBUG

--- <summary>
--- Special handling around condition variables used to control transition from
--- patching to full game state in a game application.
--- </summary>
--- <param name="bForceImmediate">If true, restart occurs without delay. Otherwise, it can be gated by game state.</param>
function UIManager.TriggerRestart(bForceImmediate)

	udUIManager:TriggerRestart(bForceImmediate)
end
return UIManager
