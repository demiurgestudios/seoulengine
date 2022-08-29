-- Game-specific Engine singleton.
--
-- Copyright (c) Demiurge Studios, Inc.
-- 
-- This source code is licensed under the MIT license.
-- Full license details can be found in the LICENSE file
-- in the root directory of this source tree.



local GameMain = class_static 'GameMain'

local udGameMain = nil

function GameMain.cctor()

	udGameMain = CoreUtilities.NewNativeUserData(Game_ScriptMain)
	if nil == udGameMain then

		error 'Failed instantiating Game_ScriptMain.'
	end
end

function GameMain.GetConfigUpdateCl()

	return udGameMain:GetConfigUpdateCl()
end

function GameMain.GetServerBaseURL()

	local sRet = udGameMain:GetServerBaseURL()
	return sRet
end

--- <summary>
--- Submit an update of auth manager refresh data to the AuthManager.
--- </summary>
--- <returns>true if the update data will require a reboot, false otherwise.</returns>
function GameMain.ManulUpdateRefreshData(tRefreshData)

	return udGameMain:ManulUpdateRefreshData(tRefreshData)
end

function GameMain.SetAutomationValue(sKey, oValue)

	udGameMain:SetAutomationValue(sKey, oValue)
end
return GameMain
