-- Access to the native Renderer singleton.
--
-- Copyright (c) Demiurge Studios, Inc.
-- 
-- This source code is licensed under the MIT license.
-- Full license details can be found in the LICENSE file
-- in the root directory of this source tree.



local Renderer = class_static 'Renderer'

local udRenderer = nil

function Renderer.cctor()

	udRenderer = CoreUtilities.NewNativeUserData(ScriptEngineRenderer)
	if nil == udRenderer then

		error 'Failed instantiating ScriptEngineRenderer.'
	end
end

function Renderer.GetViewportAspectRatio()

	local fRet = udRenderer:GetViewportAspectRatio()
	return fRet
end

function Renderer.GetViewportDimensions()

	local varX, varY = udRenderer:GetViewportDimensions()
	return varX, varY
end

function Renderer.PrefetchFx(filePath)

	udRenderer:PrefetchFx(filePath)
end
return Renderer
