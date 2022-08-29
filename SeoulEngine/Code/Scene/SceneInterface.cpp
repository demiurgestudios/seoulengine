/**
 * \file SceneInterface.cpp
 * \brief Interface for a scene, provides scene-wide access.
 *
 * Different contexts can implement different forms of scene management.
 * Interface provides a full abstract, base interface for Component
 * and Object to access common, scene-wide functionality (e.g.
 * GetObjectById()).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ReflectionDefine.h"
#include "SceneInterface.h"

#if SEOUL_WITH_SCENE

namespace Seoul
{

SEOUL_TYPE(Scene::Interface, TypeFlags::kDisableNew);

} // namespace Seoul

#endif // /#if SEOUL_WITH_SCENE
