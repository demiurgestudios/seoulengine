/**
 * \file ScriptSceneSettings.h
 * \brief Configuration of a Script::Scene instance.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCRIPT_SCENE_SETTINGS_H
#define SCRIPT_SCENE_SETTINGS_H

#include "CrashManager.h"
#include "Delegate.h"
#include "FilePath.h"

#if SEOUL_WITH_SCENE

namespace Seoul
{

struct ScriptSceneSettings SEOUL_SEALED
{
	typedef Delegate<void()> CustomHotLoadHandler;
	typedef Delegate<void(const CustomCrashErrorState&)> ScriptErrorHandler;

	ScriptSceneSettings()
		: m_sScriptMainRelativeFilename()
		, m_ScriptErrorHandler(SEOUL_BIND_DELEGATE(&CrashManager::DefaultErrorHandler))
		, m_RootScenePrefabFilePath()
		, m_FxEffectFilePath()
		, m_MeshEffectFilePath()
		, m_CustomHotLoadHandler()
	{
	}

	/** (Optional) Script to run in the Lua VM to run its main function. */
	String m_sScriptMainRelativeFilename;

	/** (Optional) If defined, Lua script errors will be handled by this implementation. */
	ScriptErrorHandler m_ScriptErrorHandler;

	/** FilePath of the prefab to load as the root of the scene. Required. */
	FilePath m_RootScenePrefabFilePath;

	/** FilePath of the Microsoft FX to use for particle FX rendering. */
	FilePath m_FxEffectFilePath;

	/** FilePath of the Microsoft FX to use for mesh rendering. */
	FilePath m_MeshEffectFilePath;

	/**
	 * By default, scene hot loading is handled internally by reloading
	 * the scene state. Custom support can be handled, in which case
	 * the called function is expected to queue up an action that results
	 * in the recreation of the scene.
	 */
	CustomHotLoadHandler m_CustomHotLoadHandler;
}; // struct ScriptSceneSettings

} // namespace Seoul

#endif // /#if SEOUL_WITH_SCENE

#endif // include guard
