/**
 * \file SceneGetTransformComponent.cpp
 * \brief Subclass of components that provide at least read access to transform data.
 *
 * GetTransformComponent is inherited by any Component that provides
 * at least read access to GetRotation() and GetPosition(), defining the
 * spatial location of the owner Object in 3D space.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ReflectionDefine.h"
#include "SceneGetTransformComponent.h"

#if SEOUL_WITH_SCENE

namespace Seoul
{

SEOUL_BEGIN_TYPE(Scene::GetTransformComponent, TypeFlags::kDisableNew)
	SEOUL_PARENT(Scene::Component)
SEOUL_END_TYPE()

} // namespace Seoul

#endif // /#if SEOUL_WITH_SCENE
