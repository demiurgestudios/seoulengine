-- Access to the native PlatformSignInManager singleton.
--
-- Copyright (c) Demiurge Studios, Inc.
-- 
-- This source code is licensed under the MIT license.
-- Full license details can be found in the LICENSE file
-- in the root directory of this source tree.



local PlatformSignInManager = class_static 'PlatformSignInManager'

local udPlatformSignInManager = nil

function PlatformSignInManager.cctor()

	udPlatformSignInManager = CoreUtilities.NewNativeUserData(ScriptEnginePlatformSignInManager)
	if nil == udPlatformSignInManager then

		error 'Failed instantiating ScriptEnginePlatformSignInManager.'
	end
end

function PlatformSignInManager.GetStateChangeCount()

	return udPlatformSignInManager:GetStateChangeCount()
end

function PlatformSignInManager.IsSignedIn()

	return udPlatformSignInManager:IsSignedIn()
end

function PlatformSignInManager.IsSigningIn()

	return udPlatformSignInManager:IsSigningIn()
end

function PlatformSignInManager.IsSignInSupported()

	return udPlatformSignInManager:IsSignInSupported()
end

function PlatformSignInManager.SignIn()

	udPlatformSignInManager:SignIn()
end

function PlatformSignInManager.SignOut()

	udPlatformSignInManager:SignOut()
end
return PlatformSignInManager
