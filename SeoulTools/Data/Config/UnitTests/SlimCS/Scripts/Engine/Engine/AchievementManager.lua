-- Access to the native AchievementManager singleton.
--
-- Copyright (c) Demiurge Studios, Inc.
-- 
-- This source code is licensed under the MIT license.
-- Full license details can be found in the LICENSE file
-- in the root directory of this source tree.



local AchievementManager = class_static 'AchievementManager'

local udAchievementManager = nil

function AchievementManager.cctor()

	udAchievementManager = CoreUtilities.NewNativeUserData(ScriptEngineAchievementManager)
	if nil == udAchievementManager then

		error 'Failed instantiating ScriptEngineAchievementManager.'
	end
end

function AchievementManager.UnlockAchievement(sAchievementID)

	udAchievementManager:UnlockAchievement(sAchievementID)
end
return AchievementManager
