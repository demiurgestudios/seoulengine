-- Utility used to give a network time to load asynchronously. Calling
-- .Network prior to IsReady() will return a null pointer.
--
-- Copyright (c) Demiurge Studios, Inc.
-- 
-- This source code is licensed under the MIT license.
-- Full license details can be found in the LICENSE file
-- in the root directory of this source tree.







local Animation2DNetworkAsync = class 'Animation2DNetworkAsync'






local function ProcessDeferred(self)

	-- Conditions to fulfill to process the deferred list.
	if nil == self.m_aDefer or not self.m_Animation:get_IsReady() then

		-- Good to go.
		if nil == self.m_aDefer and self.m_Animation:get_IsReady() then

			self.m_bReady = true
		end

		return self.m_bReady
	end

	-- If we get here, animation is ready. Need to set this
	-- to true *before* calling defer operations so that
	-- any operations that might use Call() will invoke
	-- immediately.
	self.m_bReady = true

	-- Walk the list and pcall each deferred operation.
	local aErrors = nil
	for _, v in ipairs(self.m_aDefer) do

		local bSuccess, sErrorMessage = pcall(v, self.m_Animation)
		if not bSuccess then

			if nil == aErrors then

				aErrors = {}
			end
			table.insert(aErrors, sErrorMessage)
		end
	end

	-- Done with defers.
	self.m_aDefer = nil

	-- If any errors, concat and report.
	if nil ~= aErrors then

		error(table.concat(aErrors, ''))
	end

	-- Otherwise, success.
	return self.m_bReady
end


function Animation2DNetworkAsync:Constructor(
	parent,
	animation) self.m_bReady = false;

	self.m_Animation = animation

	-- Schedule a task to run process deferred.

	local function deferredProcess(_, _)

		if ProcessDeferred(self) then

			parent:RemoveEventListener('tick'--[[Event.TICK]], deferredProcess)
		end
	end
	parent:AddEventListener('tick'--[[Event.TICK]], deferredProcess)
end

--- <summary>
--- Get the underlying network. Returns null until the network is ready.
--- </summary>


function Animation2DNetworkAsync:get_Network()

	if self.m_bReady then

		return self.m_Animation

	else

		return nil
	end
end




--- <summary>
--- Utility, used to defer actions until the underlying network is ready.
--- </summary>
--- <param name="func">Action to run</param>
--- <remarks>
--- If the network IsReady then the passed in callback will be invoked immediately.
---
--- Animation networks load asynchronously. This utility can be useful when you want
--- to ensure a change will be applied to an animation network as soon as it is ready.
---
--- If you need to query animation state, you'll need to access the network via
--- the Network property, which will return null until the Network is ready.
--- </remarks>
function Animation2DNetworkAsync:Call(func)

	if self.m_bReady then

		func(self.m_Animation)
		return
	end

	-- Defer.
	if nil == self.m_aDefer then

		self.m_aDefer = {}
	end

	-- Add the defer.
	table.insert(self.m_aDefer, func)
end
return Animation2DNetworkAsync

-- /#if SEOUL_WITH_ANIMATION_2D
