-- Access to the native SoundManager singleton.
--
-- Copyright (c) Demiurge Studios, Inc.
-- 
-- This source code is licensed under the MIT license.
-- Full license details can be found in the LICENSE file
-- in the root directory of this source tree.



local SoundManager = class_static 'SoundManager'

local udSoundManager = nil

-- Note: These values from Engine\SoundManager.h
--- <summary>
--- Standard category for the overall mix
--- </summary>
SoundManager.ksSoundCategoryMaster = 'bus:/'

--- <summary>
--- Standard category for the music sub mix
--- </summary>
SoundManager.ksSoundCategoryMusic = 'bus:/music'

--- <summary>
--- Standard category for the sound sub mix
--- </summary>
SoundManager.ksSoundCategorySFX = 'bus:/SFX'

function SoundManager.cctor()

	-- Before further action, capture any methods that are not safe to be called
	-- from off the main thread (static initialization occurs on a worker thread).
	CoreUtilities.MainThreadOnly(
		SoundManager,
		'IsCategoryPlaying'--[['IsCategoryPlaying']],
		'SetCategoryMute'--[['SetCategoryMute']],
		'SetCategoryVolume'--[['SetCategoryVolume']])

	udSoundManager = CoreUtilities.NewNativeUserData(ScriptEngineSoundManager)
	if nil == udSoundManager then

		error 'Failed instantiating ScriptEngineSoundManager.'
	end
end

function SoundManager.IsCategoryPlaying(sCategoryName, bIncludeLoopingSounds)

	local bRet = udSoundManager:IsCategoryPlaying(sCategoryName, bIncludeLoopingSounds)
	return bRet
end

function SoundManager.SetCategoryMute(sCategoryName, bMute, bAllowPending, bSuppressLogging)

	local bRet = udSoundManager:SetCategoryMute(sCategoryName, bMute, bAllowPending, bSuppressLogging)
	return bRet
end

function SoundManager.SetCategoryVolume(sCategoryName, fVolume, fFadeTimeInSeconds, bAllowPending, bSuppressLogging)

	local bRet = udSoundManager:SetCategoryVolume(sCategoryName, fVolume, fFadeTimeInSeconds, bAllowPending, bSuppressLogging)
	return bRet
end
return SoundManager
