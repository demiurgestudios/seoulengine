/**
 * \file SceneObjectHandle.cpp
 * \brief Specialization of AtomicHandle<> for SceneObject, allows thread-
 * safe, weak referencing of SceneObject instances.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "SceneObjectHandle.h"

#if SEOUL_WITH_SCENE

namespace Seoul
{

// NOTE: Assignment here is necessary to convince the linker in our GCC toolchain
// to include this definition. Otherwise, it strips it.
template <>
AtomicHandleTableCommon::Data SceneObjectHandleTable::s_Data = AtomicHandleTableCommon::Data();

} // namespace Seoul

#endif // /#if SEOUL_WITH_SCENE
