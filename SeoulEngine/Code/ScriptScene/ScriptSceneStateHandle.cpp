/**
 * \file ScriptSceneStateHandle.cpp
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

#include "ScriptSceneStateHandle.h"

#if SEOUL_WITH_SCENE

namespace Seoul
{

// NOTE: Assignment here is necessary to convince the linker in our GCC toolchain
// to include this definition. Otherwise, it strips it.
template <>
AtomicHandleTableCommon::Data ScriptSceneStateHandleTable::s_Data = AtomicHandleTableCommon::Data();

} // namespace Seoul

#endif // /#if SEOUL_WITH_SCENE
