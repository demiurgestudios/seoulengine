// Root MovieClip that implements general purpose music playback behavior.
// Add to a UI state and set self.Settings field to a table with a
// configuration to tie music to a particular UI state/context.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using static SlimCS;

public class MusicPlayer : RootMovieClip
{
	#region Private members
	readonly static Table s_tLastPlayedState = new Table();
	readonly Table m_tSettings;
	readonly string m_sStateKey;
	double m_iSoundEventId = -1;

	void StopMusic()
	{
		var udNative = GetNative();

		// Stop the music when the root clip is released.
		if (m_iSoundEventId >= 0)
		{
			udNative.StopTrackedSoundEvent((int)m_iSoundEventId);
			m_iSoundEventId = -1;
		}
	}
	#endregion

	public MusicPlayer(Table tSettings)
	{
		m_tSettings = tSettings;
		m_sStateKey = GetType().Name;

		// Start a piece of game music.
		PlayRandomMusic();
	}

	public override void Destructor()
	{
		StopMusic();
		base.Destructor();
	}

	public virtual void PlayRandomMusic()
	{
		// Grab configuration.
		var tMusicPlayerSettings = m_tSettings;
		var aMusicSet = (Table<double, Table>)tMusicPlayerSettings["MusicSet"];
		bool bNoRepeat = true == tMusicPlayerSettings["m_bNoRepeat"];

		// Nothing to do if no entries.
		if (null == next(aMusicSet))
		{
			return;
		}

		// Cache lastMusicPlayer if we don't want to ever play
		// the same piece twice in a row (unless there is only
		// 1 piece).
		string sLastMusicPlayed = null;
		if (bNoRepeat)
		{
			sLastMusicPlayed = s_tLastPlayedState[m_sStateKey];
		}

		// Sum all playback weights.
		double fTotalWeight = 0;
		foreach ((var _, var v) in ipairs(aMusicSet))
		{
			// Skip the music we played last time.
			if (v["Key"] != sLastMusicPlayed)
			{
				fTotalWeight += (double)v["Weight"];
			}
		}

		// Nothing to do if total weight is about 0.0
		if (fTotalWeight < 0.001)
		{
			return;
		}

		var udNative = GetNative();
		var fRandomRoll = math.random();
		double fAccumulated = 0.0;
		Table tMusicToPlay = null;

		// Iterate until random roll surpases accumulation, then
		// pick that entry.
		foreach ((var _, var v) in ipairs(aMusicSet))
		{
			// Skip if no repeat.
			if (v["Key"] == sLastMusicPlayed)
			{
				continue;
			}

			tMusicToPlay = v;
			fAccumulated += (double)v["Weight"] / fTotalWeight;

			// Found the piece to use
			if (fAccumulated >= fRandomRoll)
			{
				break;
			}
		}

		// If we ended up with no music to player, fallback to the first.
		if (null == tMusicToPlay)
		{
			tMusicToPlay = aMusicSet[1];
		}

		// Track last played if specified.
		if (bNoRepeat)
		{
			s_tLastPlayedState[m_sStateKey] = tMusicToPlay["Key"];
		}

		// Play the selected music.
		StopMusic();
		m_iSoundEventId = udNative.StartTrackedSoundEvent(tMusicToPlay["Key"]);
	}
}
