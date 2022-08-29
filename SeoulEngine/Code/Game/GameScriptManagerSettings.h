/**
 * \file GameScriptManagerSettings.h
 * \brief Configuration for the global Game::ScriptManager singleton.
 *
 * Defines the main entry point of the Game::ScriptManager VM, error handling
 * behavior, and other application specific settings.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef GAME_SCRIPT_MANAGER_SETTINGS_H
#define GAME_SCRIPT_MANAGER_SETTINGS_H

#include "CrashManager.h"
#include "Delegate.h"
#include "FilePath.h"
#include "SeoulHString.h"
namespace Seoul { struct CustomCrashErrorState; }
namespace Seoul { namespace UI { class Movie; } }

namespace Seoul
{

/** Utility structure, describes global settings to configure ScriptUI. */
namespace Game
{

struct ScriptManagerSettings SEOUL_SEALED
{
	typedef Delegate<void(const CustomCrashErrorState&)> ScriptErrorHandler;
	typedef Delegate<UI::Movie*(HString typeName)> CustomUIMovieInstantiator;

	ScriptManagerSettings()
		: m_sMainScriptFileName()
		, m_InstantiatorOverride()
		, m_ScriptErrorHandler(SEOUL_BIND_DELEGATE(&CrashManager::DefaultErrorHandler))
	{
	}

	/** Root script file that contains the applications "main" function. Relative to the Scripts folder. */
	String m_sMainScriptFileName;

	/**
	 * Optional - if specified, uses an app specific instantiator method for
	 * fallback UI::Movie instantiation, instead of the ScriptUI
	 * default (see UI::Manager::SetCustomUIMovieInstantiator()).
	 */
	CustomUIMovieInstantiator m_InstantiatorOverride;

	/**
	 * Optional - if specified, Lua errors will be passed to this delegate
	 * for application specific handling.
	 */
	ScriptErrorHandler m_ScriptErrorHandler;
}; // struct Game::ScriptManagerSettings

} // namespace Game

} // namespace Seoul

#endif // include guard
