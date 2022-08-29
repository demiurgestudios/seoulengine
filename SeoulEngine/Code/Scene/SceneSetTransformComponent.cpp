/**
 * \file SceneSetTransformComponent.cpp
 * \brief Subclass of components that provide both read and write access to transform data.
 *
 * SetTransformComponent is inherited by any Component that provides
 * read-write access to *Rotation() and *Position(), defining a mutable
 * spatial location of the owner Object in 3D space.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ReflectionDefine.h"
#include "SceneSetTransformComponent.h"

#if SEOUL_WITH_SCENE

namespace Seoul
{

SEOUL_BEGIN_TYPE(Scene::SetTransformComponent, TypeFlags::kDisableNew)
	SEOUL_PARENT(Scene::GetTransformComponent)
SEOUL_END_TYPE()

} // namespace Seoul

#endif // /#if SEOUL_WITH_SCENE
