/**
 * \file ScriptEngineCamera.cpp
 * \brief Binder instance for exposing a Camera instance into
 * a script VM.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Camera.h"
#include "ReflectionDefine.h"
#include "ScriptEngineCamera.h"
#include "ScriptFunctionInterface.h"
#include "SoundManager.h"

namespace Seoul
{

SEOUL_BEGIN_TYPE(ScriptEngineCamera, TypeFlags::kDisableCopy)
	SEOUL_METHOD(Animate)
	SEOUL_METHOD(GetEnabled)
	SEOUL_METHOD(SetEnabled)
	SEOUL_METHOD(GetRelativeViewport)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "(double, double, double, double)")
	SEOUL_METHOD(SetRelativeViewport)
	SEOUL_METHOD(GetRotation)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "(double, double, double, double)")
	SEOUL_METHOD(SetRotation)
	SEOUL_METHOD(GetPosition)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "(double, double, double)")
	SEOUL_METHOD(SetPosition)
	SEOUL_METHOD(SetPerspective)
	SEOUL_METHOD(SetAsAudioListenerCamera)
SEOUL_END_TYPE()

ScriptEngineCamera::ScriptEngineCamera()
	: m_pCamera()
{
}

/**
 * To be called immediately after ScriptEngineCamera().
 *
 * Required. Exists to workaround the fact that Reflection
 * only supports instantiation via a default constructor.
 */
void ScriptEngineCamera::Construct(const SharedPtr<Camera>& pCamera)
{
	m_pCamera = pCamera;
}

ScriptEngineCamera::~ScriptEngineCamera()
{
}

/** Smooth animation over time, to target point at speed, with current delta t. */
void ScriptEngineCamera::Animate(
	Float fDeltaTimeInSeconds,
	Float fPX, Float fPY, Float fPZ,
	Float fQX, Float fQY, Float fQZ, Float fQW,
	Float fMetersPerSecond,
	Float fSlerpFactor)
{
	Vector3D vCameraPosition = m_pCamera->GetPosition();
	Vector3D const vTargetPosition(fPX, fPY, fPZ);

	Vector3D vDelta(vTargetPosition - vCameraPosition);
	Float32 const fLength = vDelta.Length();
	if (fLength > 0.5f)
	{
		Float const fMovement = Min(fLength, fMetersPerSecond);
		vCameraPosition += (vDelta / fLength) * fDeltaTimeInSeconds * fMovement;
		m_pCamera->SetPosition(vCameraPosition);
	}

	Quaternion qCameraRotation = m_pCamera->GetRotation();
	Quaternion qTargetRotation(fQX, fQY, fQZ, fQW);
	qCameraRotation = Quaternion::Slerp(qCameraRotation, qTargetRotation, fSlerpFactor);
	m_pCamera->SetRotation(qCameraRotation);
}

/** @return True if this Camera is currently enabled. */
Bool ScriptEngineCamera::GetEnabled() const
{
	return m_pCamera->GetEnabled();
}

/** Update the current enabled state of this Camera. */
void ScriptEngineCamera::SetEnabled(Bool bEnabled)
{
	m_pCamera->SetEnabled(bEnabled);
}

/** Get the Camera's relative viewport rectangle. */
void ScriptEngineCamera::GetRelativeViewport(Script::FunctionInterface* pInterface) const
{
	Rectangle2D const relativeViewport(m_pCamera->GetRelativeViewport());
	pInterface->PushReturnNumber(relativeViewport.m_fLeft);
	pInterface->PushReturnNumber(relativeViewport.m_fTop);
	pInterface->PushReturnNumber(relativeViewport.m_fRight);
	pInterface->PushReturnNumber(relativeViewport.m_fBottom);
}

/** Update the Camera's relative viewport rectangle. */
void ScriptEngineCamera::SetRelativeViewport(Float fLeft, Float fTop, Float fRight, Float fBottom)
{
	m_pCamera->SetRelativeViewport(Rectangle2D(
		fLeft,
		fTop,
		fRight,
		fBottom));
}

/** @return Current camera rotation as a quaternion. */
void ScriptEngineCamera::GetRotation(Script::FunctionInterface* pInterface) const
{
	Quaternion const qRotation(m_pCamera->GetRotation());
	pInterface->PushReturnNumber(qRotation.X);
	pInterface->PushReturnNumber(qRotation.Y);
	pInterface->PushReturnNumber(qRotation.Z);
	pInterface->PushReturnNumber(qRotation.W);
}

/** Update the current camera rotation - 4 values specify a Quaternion. */
void ScriptEngineCamera::SetRotation(Float fX, Float fY, Float fZ, Float fW)
{
	m_pCamera->SetRotation(Quaternion(fX, fY, fZ, fW));
}

/** @return The current camera position - 3 values 3D vector. */
void ScriptEngineCamera::GetPosition(Script::FunctionInterface* pInterface) const
{
	Vector3D const vPosition(m_pCamera->GetPosition());
	pInterface->PushReturnNumber(vPosition.X);
	pInterface->PushReturnNumber(vPosition.Y);
	pInterface->PushReturnNumber(vPosition.Z);
}

/** Update the current camera position - 3 values specify a 3D position. */
void ScriptEngineCamera::SetPosition(Float fX, Float fY, Float fZ)
{
	m_pCamera->SetPosition(Vector3D(fX, fY, fZ));
}

/** Set the camera to a perspective projection - 4 values fully define the projection transform. */
void ScriptEngineCamera::SetPerspective(
	Float fFieldOfViewInRadians,
	Float fAspectRatio,
	Float fNearPlane,
	Float fFarPlane)
{
	m_pCamera->SetPerspective(fFieldOfViewInRadians, fAspectRatio, fNearPlane, fFarPlane);
}

/** Commit this camera to the Sound::Manager as the overall 3D audio listener camera. */
void ScriptEngineCamera::SetAsAudioListenerCamera()
{
	Sound::Manager::Get()->SetListenerCamera(m_pCamera);
}

} // namespace Seoul
