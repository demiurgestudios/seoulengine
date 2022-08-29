--[[
 DefaultAutoPlay.lua
 Auto play used for all contexts that do not have a specialized automator.
 Simply tries a tap (with no drag) at every screen location using a grid.

 Copyright (c) Demiurge Studios, Inc.
 
 This source code is licensed under the MIT license.
 Full license details can be found in the LICENSE file
 in the root directory of this source tree.
--]]

-- return a function that generates a default auto play
-- function.
return function(udNativeAutomationFunctions, tGameState, tUIState)
	local behavior = (require("DevOnly/AutomatedTests/AutoPlay/DefaultAutoPlayBehavior"))
	local co = coroutine.create(function()
		while true do
			behavior(udNativeAutomationFunctions, tGameState, tUIState)
			coroutine.yield()
		end
	end)

	return function()
		local bOk, sErrorMessage = coroutine.resume(co)
		if not bOk then
			error(sErrorMessage)
		end
	end
end
