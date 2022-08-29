/**
 * \file EditorUICamera.cpp
 * \brief Camera behavior with input/movement logic specific
 * to the editor.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Camera.h"
#include "EditorSceneContainer.h"
#include "EditorSceneState.h"
#include "EditorUICamera.h"
#include "EditorUIControllerScene.h"
#include "Matrix4D.h"
#include "ReflectionDefine.h"
#include "SeoulMath.h"
#include "Vector3D.h"
#include "Viewport.h"

#if SEOUL_WITH_SCENE

namespace Seoul::EditorUI
{

Camera::Camera()
{
}

Camera::~Camera()
{
}

/** Utility, extract camera state from scene. */
static inline Bool GetState(
	const IControllerSceneRoot& scene,
	EditorScene::CameraModeState*& rp)
{
	// Early out if no state.
	if (!scene.GetScene().GetState().IsValid())
	{
		return false;
	}

	// Only apply affects of rotation if rotation is not locked.
	auto& rState = scene.GetScene().GetState()->GetEditState().m_CameraState;
	auto& eMode = rState.m_eMode;
	rp = &rState.m_aStates[(UInt32)eMode];
	return true;
}

/** Update target position of easing. */
void Camera::SetTargetPosition(
	const IControllerSceneRoot& scene,
	const Vector3D& v)
{
	EditorScene::CameraModeState* p = nullptr;
	if (GetState(scene, p))
	{
		p->SetEasePosition(v);
	}
}

/** Update target zoom of easing. */
void Camera::SetTargetZoom(
	const IControllerSceneRoot& scene,
	Float f)
{
	EditorScene::CameraModeState* p = nullptr;
	if (GetState(scene, p))
	{
		p->SetEaseZoom(f);
	}
}

/** Apply movement to a Camera based on our settings and state. */
void Camera::Apply(
	const IControllerSceneRoot& scene,
	const CameraMovement& movement,
	const Viewport& viewport,
	Seoul::Camera& rCamera)
{
	static const Float kfZoomScale = 0.2f; // TODO: Configure.
	static const Float kfEaseSpeed = 10.0f; // TODO: Configure.

	// Early out if no state.
	if (!scene.GetScene().GetState().IsValid())
	{
		return;
	}

	// Only apply affects of rotation if rotation is not locked.
	auto& rState = scene.GetScene().GetState()->GetEditState().m_CameraState;
	auto& eMode = rState.m_eMode;
	auto& state = rState.m_aStates[(UInt32)eMode];
	if (EditorScene::CameraMode::kPerspective == eMode)
	{
		state.m_fPitchInRadians += movement.m_fDeltaPitchInRadians;
		state.m_fYawInRadians += movement.m_fDeltaYawInRadians;

		// Don't allow pitch to exceed 90 degrees or -90 degrees.
		state.m_fPitchInRadians = Clamp(
			state.m_fPitchInRadians,
			-(fPi / 2.0f),
			(fPi / 2.0f));
	}

	Vector3D vLocalDelta = Vector3D::Zero();

	// Perspective uses a FPS style fly camera.
	if (EditorScene::CameraMode::kPerspective == eMode)
	{
		// Forward motion is along the Z axis in perspective
		// mode. Also a zoom change when not in perspective mode.
		vLocalDelta.Z -= (movement.m_bForward ? 1.0f : 0.0f);
		vLocalDelta.Z += (movement.m_bBackward ? 1.0f : 0.0f);

		// Horizontal motion is along the X axis.
		vLocalDelta.X -= (movement.m_bLeft ? 1.0f : 0.0f);
		vLocalDelta.X += (movement.m_bRight ? 1.0f : 0.0f);

		// Vertical motion is along the Y axis.
		vLocalDelta.Y -= (movement.m_bDown ? 1.0f : 0.0f);
		vLocalDelta.Y += (movement.m_bUp ? 1.0f : 0.0f);

		// Normalize and rescale movement.
		vLocalDelta =
			Vector3D::Normalize(vLocalDelta)
			* state.m_fUnitsPerSecond
			* movement.m_fDeltaTimeInSeconds;
	}
	// Orthographic uses mouse dragging.
	else
	{
		state.m_fZoom -= (state.m_fZoom * movement.m_fMouseWheelDelta * kfZoomScale);
		state.m_fZoom = Max(state.m_fZoom, 1.0f);

		vLocalDelta.X = (-movement.m_vMouseDelta.X / (Float)viewport.m_iViewportWidth) * (2.0f * state.m_fZoom * viewport.GetViewportAspectRatio());
		vLocalDelta.Y = (movement.m_vMouseDelta.Y / (Float)viewport.m_iViewportHeight) * (2.0f * state.m_fZoom);

		vLocalDelta.Z -= (movement.m_bForward ? 1.0f : 0.0f);
		vLocalDelta.Z += (movement.m_bBackward ? 1.0f : 0.0f);
	}

	// Compute the translation in world space to apply.
	Vector3D vWorldDelta = Matrix4D::TransformDirection(rCamera.GetViewMatrix().OrthonormalInverse(), vLocalDelta);

	// Update position based on mode.
	state.m_vPosition += vWorldDelta;

	// Final step (since we want it to overwrite any other motion),
	// apply ease.
	state.EaseAdvance(kfEaseSpeed * movement.m_fDeltaTimeInSeconds);

	// If orthographic modes are active, apply scene fitting.
	Float fNear = 1.0f; // TODO: Configure from data.
	Float fFar = 2000.0f; // TODO: Configure from data.
	if (EditorScene::CameraMode::kPerspective != eMode)
	{
		scene.ApplyFittingCameraProperties(
			eMode,
			fNear,
			fFar,
			state.m_vPosition);
	}

	// Update projection based on mode.
	if (EditorScene::CameraMode::kPerspective == eMode)
	{
		rCamera.SetPerspective(
			DegreesToRadians(60.0f),
			viewport.GetViewportAspectRatio(),
			1.0f,
			2000.0f); // TODO: Expose configuration.
	}
	else
	{
		auto const fHalfHeight = state.m_fZoom;
		auto const fHalfWidth = fHalfHeight * viewport.GetViewportAspectRatio();
		rCamera.SetOrthographic(
			-fHalfWidth,
			fHalfWidth,
			-fHalfHeight,
			fHalfHeight,
			1.0f,
			2000.0f); // TODO: Expose configuration.
	}

	// Commit position.
	rCamera.SetPosition(state.m_vPosition);

	// Update rotation based on mode.
	switch (eMode)
	{
	case EditorScene::CameraMode::kPerspective:
		rCamera.SetRotation(Quaternion::CreateFromRotationY(state.m_fYawInRadians) * Quaternion::CreateFromRotationX(state.m_fPitchInRadians));
		break;
	case EditorScene::CameraMode::kTop:
		rCamera.SetRotation(Quaternion::CreateFromRotationX(DegreesToRadians(-90.0f))); // Fixed rotation aiming up.
		break;
	case EditorScene::CameraMode::kBottom:
		rCamera.SetRotation(Quaternion::CreateFromRotationX(DegreesToRadians(90.0f))); // Fixed rotation aiming down.
		break;
	case EditorScene::CameraMode::kLeft:
		rCamera.SetRotation(Quaternion::CreateFromRotationY(DegreesToRadians(-90.0f))); // Fixed rotation to the left.
		break;
	case EditorScene::CameraMode::kRight:
		rCamera.SetRotation(Quaternion::CreateFromRotationY(DegreesToRadians(90.0f))); // Fixed rotation to the right.
		break;
	case EditorScene::CameraMode::kFront:
		rCamera.SetRotation(Quaternion::CreateFromRotationY(DegreesToRadians(0.0f))); // Fixed rotation to the front.
		break;
	case EditorScene::CameraMode::kBack:
		rCamera.SetRotation(Quaternion::CreateFromRotationY(DegreesToRadians(-180.0f))); // Fixed rotation to the back.
		break;
	default:
		break;
	};
}

} // namespace Seoul::EditorUI

#endif // /#if SEOUL_WITH_SCENE
