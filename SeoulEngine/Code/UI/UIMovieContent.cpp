/**
 * \file UIMovieContent.cpp
 * \brief Internal aggregate class of UI::Movie, manages content that
 * has been associated with the movie, including Fx, Sound::Events,
 * and any settings data.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Logger.h"
#include "UIManager.h"
#include "UIMovieContent.h"
#include "UIUtil.h"
#include "SettingsManager.h"

namespace Seoul::UI
{

MovieContent::MovieContent()
	: m_Fx()
	, m_SoundEvents()
{
}

MovieContent::~MovieContent()
{
}

/**
 * Setup this UI::Movie content from a table describing it. The table
 * is expected to have the following entries (all are optional)
 *
 * MovieFilePath= - the value must be a file path to the SWF associated with the movie.
 * FX= - a table of FX (particle) definitions.
 * Sound::Events= - a table of Sound::Event definitions.
 * Settings= a table of JSON files that will be used as settings to configure this movie.
 */
Bool MovieContent::Configure(
	const ContentKey& key,
	const DataStore& dataStore,
	const DataNode& table,
	Bool bAppend, /*= false*/
	HString sMovieTypeName /*=HString()*/)
{
	DataNode movieFile;
	FilePath movieFilePath;
	if (dataStore.GetValueFromTable(table, FalconConstants::kMovieFilePath, movieFile) &&
		!dataStore.AsFilePath(movieFile, movieFilePath))
	{
		SEOUL_WARN("Malformed file path in %s for movie %s",
			key.GetFilePath().CStr(), sMovieTypeName.CStr());
		return false;
	}

	// Fetch sound events - do this first, want to queue up Sound::Events for load before
	// FX data.
	{
		// Fetch shared sound events, if any.  Do this first so they will be overwritten appropriately
		DataNode sharedSoundEventsTable;
		if (dataStore.GetValueFromTable(dataStore.GetRootNode(), FalconConstants::kSharedMovieSoundEvents, sharedSoundEventsTable))
		{
			if (!m_SoundEvents.ConfigureSoundEvents(
					key,
					dataStore,
					sharedSoundEventsTable,
					bAppend,
					sMovieTypeName)
				)
			{
				return false;
			}
		}

		// Now fetch the sound events for this movie clip.  Note that the append flag is always set true in this case.
		DataNode soundEventsTable;
		if (dataStore.GetValueFromTable(table, FalconConstants::kSoundEvents, soundEventsTable))
		{
			// Get any (optional) sound duckers - this value is allowed to be null even if
			// a table of sound events is defined.
			DataNode soundDuckersArray;
			(void)dataStore.GetValueFromTable(table, FalconConstants::kSoundDuckers, soundDuckersArray);

			if (!m_SoundEvents.Configure(
					key,
					dataStore,
					soundEventsTable,
					soundDuckersArray,
					true, // bAppend = true
					sMovieTypeName))
			{
				return false;
			}
		}
	}

	// Overload FX preload for this movie - must happen before Configure is called.
	Bool bPreloadFx = true;
	{
		DataNode value;
		if (dataStore.GetValueFromTable(table, FalconConstants::kPreloadFx, value))
		{
			(void)dataStore.AsBoolean(value, bPreloadFx);
		}
	}
	// Configure.
	m_Fx.SetPreloadAllFx(bPreloadFx);

	// Fetch fx.
	DataNode fxTable;
	if (dataStore.GetValueFromTable(table, FalconConstants::kFx, fxTable))
	{
		if (!m_Fx.Configure(dataStore, fxTable, bAppend, key.GetFilePath()))
		{
			return false;
		}
	}

	return true;
}

} // namespace Seoul::UI
