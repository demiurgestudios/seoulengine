-- Placeholder API for the native ScriptUIAnimation2DNetworkInstance.
--
-- Copyright (c) Demiurge Studios, Inc.
-- 
-- This source code is licensed under the MIT license.
-- Full license details can be found in the LICENSE file
-- in the root directory of this source tree.







local Animation2DNetwork = class('Animation2DNetwork', 'DisplayObject')


-- Utility to check for ready state - all methods in this instance
-- depend on IsReady() and we raise an error if they're called
-- otherwise (the Animation2DNetworkAsync utility is used
-- to enforce this in practice).
local function CheckReady(self)


	if not self.m_udNativeInstance:IsReady() then

		error("Animation2DNetwork instance '" .. tostring(self:GetName()) .. "' is not ready.", 1)
	end
end




function Animation2DNetwork:get_ActivePalette()

	--[[CheckReady(self)]]
	return self.m_udNativeInstance:GetActivePalette()
end

function Animation2DNetwork:set_ActivePalette(value)

	--[[CheckReady(self)]]
	self.m_udNativeInstance:SetActivePalette(value)
end




function Animation2DNetwork:get_ActiveSkin()

	--[[CheckReady(self)]]
	return self.m_udNativeInstance:GetActiveSkin()
end

function Animation2DNetwork:set_ActiveSkin(value)

	--[[CheckReady(self)]]
	self.m_udNativeInstance:SetActiveSkin(value)
end


function Animation2DNetwork:GetActiveStatePath()

	--[[CheckReady(self)]]
	return self.m_udNativeInstance:GetActiveStatePath()
end



function Animation2DNetwork:get_IsReady()

	return self.m_udNativeInstance:IsReady()
end


function Animation2DNetwork:AddBoneAttachment(iIndex, attachment)

	--[[CheckReady(self)]]
	self.m_udNativeInstance:AddBoneAttachment(
		iIndex,
		attachment:get_NativeInstance())
end

function Animation2DNetwork:AddTimestepOffset(fTimestepOffset)

	--[[CheckReady(self)]]
	self.m_udNativeInstance:AddTimestepOffset(fTimestepOffset)
end

function Animation2DNetwork:AllDonePlaying()

	--[[CheckReady(self)]]
	return self.m_udNativeInstance:AllDonePlaying()
end



function Animation2DNetwork:get_CurrentMaxTime()

	--[[CheckReady(self)]]
	return self.m_udNativeInstance:GetCurrentMaxTime()
end


function Animation2DNetwork:GetBoneIndex(sName)

	--[[CheckReady(self)]]
	return self.m_udNativeInstance:GetBoneIndex(sName)
end

function Animation2DNetwork:GetBonePositionByName(sName)

	--[[CheckReady(self)]]
	return self.m_udNativeInstance:GetBonePositionByName(sName)
end

function Animation2DNetwork:GetBonePositionByIndex(iIndex)

	--[[CheckReady(self)]]
	return self:GetBonePositionByIndex(iIndex)
end

function Animation2DNetwork:GetLocalBonePositionByName(sName)

	--[[CheckReady(self)]]
	return self.m_udNativeInstance:GetLocalBonePositionByName(sName)
end

function Animation2DNetwork:GetLocalBonePositionByIndex(iIndex)

	--[[CheckReady(self)]]
	return self.m_udNativeInstance:GetLocalBonePositionByIndex(iIndex)
end

function Animation2DNetwork:GetLocalBoneScaleByIndex(iIndex)

	--[[CheckReady(self)]]
	return self.m_udNativeInstance:GetLocalBoneScaleByIndex(iIndex)
end

function Animation2DNetwork:GetLocalBoneScaleByName(sName)

	--[[CheckReady(self)]]
	return self.m_udNativeInstance:GetLocalBoneScaleByName(sName)
end

function Animation2DNetwork:GetWorldSpaceBonePositionByIndex(iIndex)

	--[[CheckReady(self)]]
	return self.m_udNativeInstance:GetWorldSpaceBonePositionByIndex(iIndex)
end

function Animation2DNetwork:GetWorldSpaceBonePositionByName(sName)

	--[[CheckReady(self)]]
	return self.m_udNativeInstance:GetWorldSpaceBonePositionByName(sName)
end

function Animation2DNetwork:GetTimeToEvent(sEventName)

	--[[CheckReady(self)]]
	return self.m_udNativeInstance:GetTimeToEvent(sEventName)
end



function Animation2DNetwork:get_IsInStateTransition()

	--[[CheckReady(self)]]
	return self.m_udNativeInstance:IsInStateTransition()
end


function Animation2DNetwork:SetCastShadow(bCast)

	--[[CheckReady(self)]]
	self.m_udNativeInstance:SetCastShadow(bCast)
end

function Animation2DNetwork:SetCondition(sName, bValue)

	--[[CheckReady(self)]]
	self.m_udNativeInstance:SetCondition(sName, bValue)
end

function Animation2DNetwork:SetShadowOffset(fX, fY)

	--[[CheckReady(self)]]
	self.m_udNativeInstance:SetShadowOffset(fX, fY)
end

function Animation2DNetwork:SetParameter(sName, fValue)

	--[[CheckReady(self)]]
	self.m_udNativeInstance:SetParameter(sName, fValue)
end

function Animation2DNetwork:TriggerTransition(sName)

	--[[CheckReady(self)]]
	self.m_udNativeInstance:TriggerTransition(sName)
end



function Animation2DNetwork:set_VariableTimeStep(value)

	--[[CheckReady(self)]]
	self.m_udNativeInstance:SetVariableTimeStep(value)
end

return Animation2DNetwork

-- /#if SEOUL_WITH_ANIMATION_2D
