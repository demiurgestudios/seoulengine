/**
 * \file ScriptScene.h
 * \brief A Scene container (tree of ScenePrefabs) with
 * a Script VM.
 *
 * Creates a scriptable 3D scene.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCRIPT_SCENE_H
#define SCRIPT_SCENE_H

#include "Delegate.h"
#include "FilePath.h"
#include "FixedArray.h"
#include "ScriptSceneSettings.h"
#include "SeoulHString.h"
namespace Seoul { class RenderCommandStreamBuilder; }
namespace Seoul { class RenderPass; }
namespace Seoul { namespace Scene { class Renderer; } }
namespace Seoul { namespace Scene { class Ticker; } }
namespace Seoul { class ScriptSceneState; }
namespace Seoul { class ScriptSceneStateLoadJob; }

#if SEOUL_WITH_SCENE

namespace Seoul
{

/**
 * ScriptScene is a Scene container oriented for a Script VM.
 *
 * ScriptScene binds all functionality into an owned Script VM.
 * Most scene interactions are in fact only available in the VM
 * and are not exposed in the public native API.
 */
class ScriptScene SEOUL_SEALED
{
public:
	SEOUL_DELEGATE_TARGET(ScriptScene);

	ScriptScene(const ScriptSceneSettings& settings);
	~ScriptScene();

	// Return true if a scene load is active, false otherwise.
	Bool IsLoading() const;

	// Entry point, called per frame to render the current scene state.
	void Render(
		RenderPass& rPass,
		RenderCommandStreamBuilder& rBuilder);

	// Utility, pass events (expected at least 1 event name
	// argument with additional optional data arguments)
	// to script. Invokes any registered event callbacks.
	void SendEvent(
		const Reflection::MethodArguments& aArguments,
		Int iArgumentCount);

	// Entry point, called per frame to advance/simulate the current scene state.
	void Tick(Float fDeltaTimeInSeconds);

private:
	ScriptSceneSettings const m_Settings;
	SharedPtr<ScriptSceneStateLoadJob> m_pScriptSceneStateLoadJob;
	ScopedPtr<Scene::Renderer> m_pSceneRenderer;
	ScopedPtr<Scene::Ticker> m_pSceneTicker;
	ScopedPtr<ScriptSceneState> m_pState;
#if SEOUL_HOT_LOADING
	Bool m_bPendingHotLoad;
	void OnFileLoadComplete(FilePath filePath);
#endif // /#if SEOUL_HOT_LOADING

	Bool InternalCheckState();

	SEOUL_DISABLE_COPY(ScriptScene);
}; // class ScriptScene

} // namespace Seoul

#endif // /#if SEOUL_WITH_SCENE

#endif // include guard
