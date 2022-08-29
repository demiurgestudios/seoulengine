/**
 * \file EditorSceneCameraMode.h
 * \brief Camera display modes supported by the Editor.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef EDITOR_SCENE_CAMERA_MODE_H
#define EDITOR_SCENE_CAMERA_MODE_H

#include "Prereqs.h"

#if SEOUL_WITH_SCENE

namespace Seoul::EditorScene
{

enum class CameraMode
{
	kPerspective,
	kTop,
	kBottom,
	kLeft,
	kRight,
	kFront,
	kBack,
	COUNT,
};

} // namespace Seoul::EditorScene

#endif // /#if SEOUL_WITH_SCENE

#endif // include guard
