/**
 * \file EditorUICamera.h
 * \brief Camera behavior with input/movement logic specific
 * to the editor.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef EDITOR_UI_CAMERA_H
#define EDITOR_UI_CAMERA_H

#include "EditorSceneEditState.h"
#include "Prereqs.h"
namespace Seoul { class Camera; }
namespace Seoul { namespace EditorUI { class IControllerSceneRoot; } }
namespace Seoul { struct Viewport; }

#if SEOUL_WITH_SCENE

namespace Seoul::EditorUI
{

/** Movement state applied to the fly camera each frame. */
struct CameraMovement SEOUL_SEALED
{
	CameraMovement()
		: m_fDeltaTimeInSeconds(0.0f)
		, m_bForward(false)
		, m_bBackward(false)
		, m_bRight(false)
		, m_bLeft(false)
		, m_bUp(false)
		, m_bDown(false)
		, m_fDeltaPitchInRadians(0.0f)
		, m_fDeltaYawInRadians(0.0f)
		, m_vMouseDelta(0, 0)
		, m_fMouseWheelDelta(0.0f)
	{
	}

	Float32 m_fDeltaTimeInSeconds;
	Bool m_bForward;
	Bool m_bBackward;
	Bool m_bRight;
	Bool m_bLeft;
	Bool m_bUp;
	Bool m_bDown;
	Float32 m_fDeltaPitchInRadians;
	Float32 m_fDeltaYawInRadians;
	Vector2D m_vMouseDelta;
	Float32 m_fMouseWheelDelta;
}; // struct CameraMovement

class Camera SEOUL_SEALED
{
public:
	Camera();
	~Camera();

	// Update a target tracking position for the camera.
	void SetTargetPosition(const IControllerSceneRoot& scene, const Vector3D& v);
	void SetTargetZoom(const IControllerSceneRoot& scene, Float f);

	// Apply movement to a Camera based on our settings and state.
	void Apply(
		const IControllerSceneRoot& scene,
		const CameraMovement& movement,
		const Viewport& viewport,
		Seoul::Camera& rCamera);

private:
	SEOUL_DISABLE_COPY(Camera);
}; // class Camera

} // namespace Seoul::EditorUI

#endif // /#if SEOUL_WITH_SCENE

#endif // include guard
