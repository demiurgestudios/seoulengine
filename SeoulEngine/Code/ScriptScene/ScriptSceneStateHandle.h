/**
 * \file ScriptSceneStateHandle.h
 * \brief AtomicHandle specialization of scriptable scene,
 * handles weak reference of the scene from script, to
 * avoid circular referencing.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#ifndef SCRIPT_SCENE_STATE_HANDLE_H
#define SCRIPT_SCENE_STATE_HANDLE_H

#include "AtomicHandle.h"
#include "CheckedPtr.h"
namespace Seoul { class ScriptSceneState; }

#if SEOUL_WITH_SCENE

namespace Seoul
{

typedef AtomicHandle<ScriptSceneState> ScriptSceneStateHandle;
typedef AtomicHandleTable<ScriptSceneState> ScriptSceneStateHandleTable;

/** Conversion to pointer convenience functions. */
template <typename T>
inline CheckedPtr<T> GetPtr(ScriptSceneStateHandle h)
{
	CheckedPtr<T> pReturn((T*)((ScriptSceneState*)ScriptSceneStateHandleTable::Get(h)));
	return pReturn;
}

static inline CheckedPtr<ScriptSceneState> GetPtr(ScriptSceneStateHandle h)
{
	CheckedPtr<ScriptSceneState> pReturn((ScriptSceneState*)ScriptSceneStateHandleTable::Get(h));
	return pReturn;
}

} // namespace Seoul

#endif // /#if SEOUL_WITH_SCENE

#endif  // include guard
