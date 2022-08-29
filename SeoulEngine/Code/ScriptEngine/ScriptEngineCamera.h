/**
 * \file ScriptEngineCamera.h
 * \brief Binder instance for exposing a Camera instance into
 * a script VM.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCRIPT_ENGINE_CAMERA_H
#define SCRIPT_ENGINE_CAMERA_H

#include "Prereqs.h"
namespace Seoul { class Camera; }
namespace Seoul { namespace Script { class FunctionInterface; } }

namespace Seoul
{

/** Binder, wraps a Camera instance and exposes functionality to a script VM. */
class ScriptEngineCamera SEOUL_SEALED
{
public:
	ScriptEngineCamera();
	void Construct(const SharedPtr<Camera>& pCamera);
	~ScriptEngineCamera();

	// Smooth animation over time, to target point at speed, with current delta t.
	void Animate(
		Float fDeltaTimeInSeconds,
		Float fPX, Float fPY, Float fPZ,
		Float fQX, Float fQY, Float fQZ, Float fQW,
		Float fMetersPerSecond,
		Float fSlerpFactor);

	// Enable/disable state.
	Bool GetEnabled() const;
	void SetEnabled(Bool bEnabled);

	// Get/set relative viewport.
	void GetRelativeViewport(Script::FunctionInterface* pInterface) const;
	void SetRelativeViewport(Float fLeft, Float fTop, Float fRight, Float fBottom);

	// Position and rotation get and set.
	void GetRotation(Script::FunctionInterface* pInterface) const;
	void SetRotation(Float fX, Float fY, Float fZ, Float fW);
	void GetPosition(Script::FunctionInterface* pInterface) const;
	void SetPosition(Float fX, Float fY, Float fZ);

	// Sets the camera to a perspective projection camera.
	void SetPerspective(
		Float fFieldOfViewInRadians,
		Float fAspectRatio,
		Float fNearPlane,
		Float fFarPlane);

	// Set this camera as the audio listener camera.
	void SetAsAudioListenerCamera();

private:
	SharedPtr<Camera> m_pCamera;

	SEOUL_DISABLE_COPY(ScriptEngineCamera);
}; // class ScriptEngineCamera

} // namespace Seoul

#endif // include guard
