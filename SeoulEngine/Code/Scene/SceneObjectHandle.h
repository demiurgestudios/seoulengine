/**
 * \file SceneObjectHandle.h
 * \brief Specialization of AtomicHandle<> for SceneObject, allows thread-
 * safe, weak referencing of SceneObject instances.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCENE_OBJECT_HANDLE_H
#define SCENE_OBJECT_HANDLE_H

#include "AtomicHandle.h"
#include "CheckedPtr.h"
namespace Seoul { namespace Scene { class Object; } }

#if SEOUL_WITH_SCENE

namespace Seoul
{

typedef AtomicHandle<Scene::Object> SceneObjectHandle;
typedef AtomicHandleTable<Scene::Object> SceneObjectHandleTable;

/** Conversion to pointer convenience functions. */
template <typename T>
inline CheckedPtr<T> GetPtr(SceneObjectHandle h)
{
	CheckedPtr<T> pReturn((T*)((Scene::Object*)SceneObjectHandleTable::Get(h)));
	return pReturn;
}

static inline CheckedPtr<Scene::Object> GetPtr(SceneObjectHandle h)
{
	CheckedPtr<Scene::Object> pReturn((Scene::Object*)SceneObjectHandleTable::Get(h));
	return pReturn;
}

} // namespace Seoul

#endif // /#if SEOUL_WITH_SCENE

#endif  // include guard
