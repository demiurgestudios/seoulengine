-- Root MovieClip that implements general purpose music playback behavior.
-- Add to a UI state and set self.Settings field to a table with a
-- configuration to tie music to a particular UI state/context.
--
-- Copyright (c) Demiurge Studios, Inc.
-- 
-- This source code is licensed under the MIT license.
-- Full license details can be found in the LICENSE file
-- in the root directory of this source tree.



local MusicPlayer = class('MusicPlayer', 'RootMovieClip')


local s_tLastPlayedState = {}




local function StopMusic(self)

	local udNative = self:GetNative()

	-- Stop the music when the root clip is released.
	if self.m_iSoundEventId >= 0 then

		udNative:StopTrackedSoundEvent(__castint__(self.m_iSoundEventId))
		self.m_iSoundEventId = -1
	end
end


function MusicPlayer:Constructor(tSettings) self.m_iSoundEventId = -1; RootMovieClip.Constructor(self)

	self.m_tSettings = tSettings
	self.m_sStateKey = GetType(self):get_Name()

	-- Start a piece of game music.
	self:PlayRandomMusic()
end

function MusicPlayer:Destructor()

	StopMusic(self)
	RootMovieClip.Destructor(self)
end

function MusicPlayer:PlayRandomMusic()

	-- Grab configuration.
	local tMusicPlayerSettings = self.m_tSettings
	local aMusicSet = tMusicPlayerSettings.MusicSet
	local bNoRepeat = true == tMusicPlayerSettings.m_bNoRepeat

	-- Nothing to do if no entries.
	if nil == next(aMusicSet) then

		return
	end

	-- Cache lastMusicPlayer if we don't want to ever play
	-- the same piece twice in a row (unless there is only
	-- 1 piece).
	local sLastMusicPlayed = nil
	if bNoRepeat then

		sLastMusicPlayed = s_tLastPlayedState[self.m_sStateKey]
	end

	-- Sum all playback weights.
	local fTotalWeight = 0
	for _, v in ipairs(aMusicSet) do

		-- Skip the music we played last time.
		if v.Key ~= sLastMusicPlayed then

			fTotalWeight = (fTotalWeight) +(__cast__(v.Weight, double))
		end
	end

	-- Nothing to do if total weight is about 0.0
	if fTotalWeight < 0.001 then

		return
	end

	local udNative = self:GetNative()
	local fRandomRoll = math.random()
	local fAccumulated = 0
	local tMusicToPlay = nil

	-- Iterate until random roll surpases accumulation, then
	-- pick that entry.
	for _, v in ipairs(aMusicSet) do

		-- Skip if no repeat.
		if v.Key == sLastMusicPlayed then

			goto continue
		end

		tMusicToPlay = v
		fAccumulated = (fAccumulated) +(__cast__(v.Weight, double) / fTotalWeight)

		-- Found the piece to use
		if fAccumulated >= fRandomRoll then

			break
		end ::continue::
	end

	-- If we ended up with no music to player, fallback to the first.
	if nil == tMusicToPlay then

		tMusicToPlay = aMusicSet[1]
	end

	-- Track last played if specified.
	if bNoRepeat then

		s_tLastPlayedState[self.m_sStateKey] = tMusicToPlay.Key
	end

	-- Play the selected music.
	StopMusic(self)
	self.m_iSoundEventId = udNative:StartTrackedSoundEvent(tMusicToPlay.Key)
end
return MusicPlayer
