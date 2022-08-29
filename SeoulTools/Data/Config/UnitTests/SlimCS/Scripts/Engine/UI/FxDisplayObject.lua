-- Placeholder API for the native ScriptUIFxInstance.
--
-- Copyright (c) Demiurge Studios, Inc.
-- 
-- This source code is licensed under the MIT license.
-- Full license details can be found in the LICENSE file
-- in the root directory of this source tree.



local FxDisplayObject = class('FxDisplayObject', 'DisplayObject')

-- TODO: Remove the need for this hook. It's low-level
-- and unexpected in SlimCS.
--
-- __cinit when defined is run on the class table immediately
-- before super propagation occurs. Meaning, the class is
-- not yet constructed (the static constructor has not run yet)
-- but all members defined below will have already been added
function FxDisplayObject.__cinit()

	-- Get the native ScriptUIFxInstance description and bind
	-- wrapper methods into the FxDisplayObject class description.
	do
		local tUserDataDesc = CoreUtilities.DescribeNativeUserData 'ScriptUIFxInstance'
		for name, func in pairs(tUserDataDesc) do

			if type(func) == 'function' then

				rawset(
					FxDisplayObject,
					name,
					function(self, ...)

						local udNativeInstance = rawget(self, 'm_udNativeInstance')
						return func(udNativeInstance, ...)
					end)
			end
		end
	end
end

function FxDisplayObject:SetDepth3DSource(source)

	local udSourceInstance = rawget(source, 'm_udNativeInstance')
	self:SetDepth3DNativeSource(udSourceInstance)
end

-- These functions are defined as placeholders to allow concrete
-- definitions in script. They will be replaced with their native
-- hookups.
--
-- PLACEHOLDERS
function FxDisplayObject:GetDepth3D()

	error 'placeholder not replaced'
	return 0
end

function FxDisplayObject:GetProperties()

	error 'placeholder not replaced'
	return nil
end

function FxDisplayObject:SetDepth3D(fDepth3D)

	local _ = fDepth3D
	error 'placeholder not replaced'
end

function FxDisplayObject:SetDepth3DBias(fDepth3DBias)

	local _ = fDepth3DBias
	error 'placeholder not replaced'
end

function FxDisplayObject:SetDepth3DNativeSource(source)

	local _ = source
	error 'placeholder not replaced'
end

function FxDisplayObject:SetRallyPoint(fX, fY)

	local _ = fX, fY
	error 'placeholder not replaced'
	return false
end

function FxDisplayObject:SetTreatAsLooping(b)

	local _ = b
	error 'placeholder not replaced'
end

function FxDisplayObject:Stop(bStopImmediately)

	local _ = bStopImmediately
	error 'placeholder not replaced'
end

-- END PLACEHOLDERS
return FxDisplayObject
