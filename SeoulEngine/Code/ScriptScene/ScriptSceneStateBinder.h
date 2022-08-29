/**
 * \file ScriptSceneStateBinder.h
 * \brief Middleman instance that handles communication between
 * the script VM and the internal state of a scriptable scene.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCRIPT_SCENE_STATE_BINDER_H
#define SCRIPT_SCENE_STATE_BINDER_H

#include "ScriptSceneStateHandle.h"
namespace Seoul { namespace Script { class FunctionInterface; } }
namespace Seoul { class ScriptSceneState; }

#if SEOUL_WITH_SCENE

namespace Seoul
{

struct ScriptSceneStateBinder SEOUL_SEALED
{
	ScriptSceneStateBinder();
	void Construct(const ScriptSceneStateHandle& hScriptSceneState);
	~ScriptSceneStateBinder();

	// Script bindings.
	void AsyncAddPrefab(Script::FunctionInterface* pInterface) const;
	void GetCamera(Script::FunctionInterface* pInterface) const;
	void GetObjectById(Script::FunctionInterface* pInterface) const;
	void GetObjectIdFromHandle(Script::FunctionInterface* pInterface) const;
	void SetScriptInterface(Script::FunctionInterface* pInterface);
	// /Script bindings.

	// Accessor to the ScriptSceneState handle binding.
	CheckedPtr<ScriptSceneState> GetSceneStatePtr() const;

private:
	ScriptSceneStateHandle m_hScriptSceneState;
}; // struct ScriptSceneStateBinder

} // namespace Seoul

#endif // /#if SEOUL_WITH_SCENE

#endif // include guard
