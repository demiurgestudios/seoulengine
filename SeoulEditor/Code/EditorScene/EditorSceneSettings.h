/**
 * \file EditorSceneSettings.h
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef EDITOR_SCENE_SETTINGS_H
#define EDITOR_SCENE_SETTINGS_H

#include "Delegate.h"
#include "FilePath.h"
#include "SceneRenderer.h"
namespace Seoul { struct CustomCrashErrorState; }

#if SEOUL_WITH_SCENE

namespace Seoul::EditorScene
{

struct Settings SEOUL_SEALED
{
	typedef Delegate<void(const CustomCrashErrorState&)> ScriptErrorHandler;

	Settings()
		: m_RootScenePrefabFilePath()
	{
	}

	/** FilePath of the prefab to load as the root of the scene. Required. */
	FilePath m_RootScenePrefabFilePath;
}; // struct Settings

} // namespace Seoul::EditorScene

#endif // /#if SEOUL_WITH_SCENE

#endif // include guard
