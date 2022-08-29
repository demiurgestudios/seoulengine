/**
 * \file UIMovieContent.h
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

#pragma once
#ifndef UI_MOVIE_CONTENT_H
#define UI_MOVIE_CONTENT_H

#include "FxFactory.h"
#include "UIData.h"
#include "Prereqs.h"
#include "Settings.h"
#include "SoundEventFactory.h"

namespace Seoul::UI
{

class MovieContent SEOUL_SEALED
{
public:
	typedef HashTable<HString, SettingsContentHandle, MemoryBudgets::UIRuntime> Settings;

	MovieContent();
	~MovieContent();

	FxFactory& GetFx()
	{
		return m_Fx;
	}

	const FxFactory& GetFx() const
	{
		return m_Fx;
	}

	Sound::EventFactory& GetSoundEvents()
	{
		return m_SoundEvents;
	}

	const Sound::EventFactory& GetSoundEvents() const
	{
		return m_SoundEvents;
	}

	// Setup all the content in this UI::MovieContent
	// from a DataStore node (expected to be a table).
	Bool Configure(
		const ContentKey& key,
		const DataStore& dataStore,
		const DataNode& table,
		Bool bAppend = false,
		HString sMovieTypeName = HString());

	/**
	 * Poll sound events.
	 */
	void Poll()
	{
		m_SoundEvents.Poll();
	}

private:
	// Fx factory
	FxFactory m_Fx;

	// Sound event factory
	Sound::EventFactory m_SoundEvents;

	SEOUL_DISABLE_COPY(MovieContent);
}; // class UI::MovieContent

} // namespace Seoul::UI

#endif // include guard
