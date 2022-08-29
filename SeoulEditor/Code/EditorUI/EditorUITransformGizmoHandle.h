/**
 * \file EditorUITransformGizmoHandle.h
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef EDITOR_UI_TRANSFORM_GIZMO_HANDLE_H
#define EDITOR_UI_TRANSFORM_GIZMO_HANDLE_H

#include "Prereqs.h"

#if SEOUL_WITH_SCENE

namespace Seoul::EditorUI
{

enum class TransformGizmoHandle
{
	kNone,

	// NOTE: Order of these matters. Code
	// expects kX, kY, kZ to be consecutive,
	// and kYZ, kXZ, and kXY to be consecutive.

	/** Transform gizmo affects only the X axis. */
	kX,

	/** Transform gizmo affects only the Y axis. */
	kY,

	/** Transform gizmo affects only the Z axis. */
	kZ,

	/** Transform gizmo affects the YZ plane, plane corresponding to the X axis. */
	kYZ,

	/** Transform gizmo affects the XZ plane, plane corresponding to the Y axis. */
	kXZ,

	/** Transform gizmo affects the XY plane, plane corresponding to the Z axis. */
	kXY,

	/** Transform gizmo affects all axes. */
	kAll,
};

} // namespace Seoul::EditorUI

#endif // /#if SEOUL_WITH_SCENE

#endif // include guard
