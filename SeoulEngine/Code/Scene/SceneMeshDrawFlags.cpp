/**
 * \file SceneMeshDrawFlags.cpp
 * \brief Rendering options when drawing a mesh.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ReflectionDefine.h"
#include "SceneMeshDrawFlags.h"

#if SEOUL_WITH_SCENE

namespace Seoul
{

SEOUL_BEGIN_ENUM(Scene::MeshDrawFlags::Enum)
	SEOUL_ENUM_N("None", Scene::MeshDrawFlags::kNone)
	SEOUL_ENUM_N("Sky", Scene::MeshDrawFlags::kSky)
	SEOUL_ENUM_N("InfiniteDepth", Scene::MeshDrawFlags::kInfiniteDepth)
SEOUL_END_ENUM()

} // namespace Seoul

#endif // /#if SEOUL_WITH_SCENE
