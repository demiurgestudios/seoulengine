/**
 * \file EditorSceneCameraMode.cpp
 * \brief Camera display modes supported by the Editor.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "EditorSceneCameraMode.h"
#include "ReflectionDefine.h"

#if SEOUL_WITH_SCENE

namespace Seoul
{

SEOUL_BEGIN_ENUM(EditorScene::CameraMode)
	SEOUL_ENUM_N("Perspective", EditorScene::CameraMode::kPerspective)
	SEOUL_ENUM_N("Top", EditorScene::CameraMode::kTop)
	SEOUL_ENUM_N("Bottom", EditorScene::CameraMode::kBottom)
	SEOUL_ENUM_N("Left", EditorScene::CameraMode::kLeft)
	SEOUL_ENUM_N("Right", EditorScene::CameraMode::kRight)
	SEOUL_ENUM_N("Front", EditorScene::CameraMode::kFront)
	SEOUL_ENUM_N("Back", EditorScene::CameraMode::kBack)
SEOUL_END_ENUM()

} // namespace Seoul

#endif // /#if SEOUL_WITH_SCENE
