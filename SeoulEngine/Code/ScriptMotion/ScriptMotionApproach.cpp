/**
 * \file ScriptMotionApproach.cpp
 * \brief Bound scriptable instance tied to
 * a SceneObject that provides "approach" motion
 * behavior.
 *
 * Approach motion behavior causes an Object to move with linear
 * velocity towards a point. The Object will overshoot the point
 * unless otherwise constrained. This results in a narrow,
 * orbital motion.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ReflectionDefine.h"
#include "SceneObject.h"
#include "ScriptFunctionInterface.h"
#include "ScriptFunctionInvoker.h"
#include "ScriptMotion.h"
#include "ScriptSceneObject.h"
#include "ScriptVm.h"

#if SEOUL_WITH_SCENE

namespace Seoul
{

/** Minimum acceptable approach range - 1 meter. */
static const Float kfMinimumApproachRange = 1.0f;

class ScriptMotionApproach SEOUL_SEALED : public ScriptMotion
{
public:
	ScriptMotionApproach();
	~ScriptMotionApproach();

	// Script hooks.

	// Return whether we've reached the target or not.
	Float GetDistanceToTarget() const;

	// Set the acceleration of our approach.
	void SetAccelerationMag(Float fAccelerationMag)
	{
		m_fAccelerationMag = fAccelerationMag;
	}

	// Distance from the approach target within which we are considered in range of the target.
	void SetApproachRange(Float fApproachRange)
	{
		m_fApproachRange = Max(kfMinimumApproachRange, fApproachRange);
	}

	// Set the world space position that we're approaching.
	void SetApproachTarget(Float fX, Float fY, Float fZ)
	{
		m_vApproachTarget = Vector3D(fX, fY, fZ);
	}

	/**
	 * Sets the reverse acceleration.
	 *
	 * A dot product between the desired direction and the current
	 * velocity is computed and converted to a lerp alpha
	 * [-1, 1] -> [0, 1], and that lerp is used to interpolate
	 * between forward acceleration and this value.
	 */
	void SetReverseAccelerationMag(Float fReverseAccelerationMag)
	{
		m_fReverseAccelerationMag = fReverseAccelerationMag;
	}

	// Update the callback when the object enters approach range.
	void SetEnterApproachRangeCallback(Script::FunctionInterface* pInterface);

	// Update the callback when the object leaves approach range.
	void SetLeaveApproachRangeCallback(Script::FunctionInterface* pInterface);

	// Instantaneous velocity set. Uses the object's current facing direction
	// and the provided velocity magnitude.
	void SetVelocityToFacing(Float fVelocityMag);

	// Call once per frame to apply motion.
	virtual void Tick(Scene::Interface& rInterface, Float fDeltaTimeInSeconds) SEOUL_OVERRIDE;

	// /Script hooks.

private:
	SEOUL_DISABLE_COPY(ScriptMotionApproach);

	Float m_fAccelerationMag;
	Float m_fApproachRange;
	Float m_fReverseAccelerationMag;
	Vector3D m_vApproachTarget;
	SharedPtr<Script::VmObject> m_pEnterCallback;
	SharedPtr<Script::VmObject> m_pLeaveCallback;
}; // class ScriptMotionApproach

SEOUL_BEGIN_TYPE(ScriptMotionApproach, TypeFlags::kDisableCopy)
	SEOUL_PARENT(ScriptMotion)
	SEOUL_METHOD(GetDistanceToTarget)
	SEOUL_METHOD(SetAccelerationMag)
	SEOUL_METHOD(SetApproachRange)
	SEOUL_METHOD(SetApproachTarget)
	SEOUL_METHOD(SetEnterApproachRangeCallback)
	SEOUL_METHOD(SetLeaveApproachRangeCallback)
	SEOUL_METHOD(SetReverseAccelerationMag)
	SEOUL_METHOD(SetVelocityToFacing)
SEOUL_END_TYPE()

ScriptMotionApproach::ScriptMotionApproach()
	: m_fAccelerationMag(0.0f)
	, m_fApproachRange(1.0f)
	, m_fReverseAccelerationMag(0.0f)
	, m_vApproachTarget(Vector3D::Zero())
	, m_pEnterCallback()
	, m_pLeaveCallback()
{
}

ScriptMotionApproach::~ScriptMotionApproach()
{
}

/** @return Our distance to the current target. */
Float ScriptMotionApproach::GetDistanceToTarget() const
{
	Vector3D const vCurrentPosition(m_pSceneObject->GetPosition());
	return (vCurrentPosition - m_vApproachTarget).Length();
}

/** Update the callback when the object enters approach range. */
void ScriptMotionApproach::SetEnterApproachRangeCallback(Script::FunctionInterface* pInterface)
{
	if (!pInterface->GetFunction(1u, m_pEnterCallback))
	{
		pInterface->RaiseError(1u, "expected function.");
		return;
	}
}

/** Update the callback when the object leaves approach range. */
void ScriptMotionApproach::SetLeaveApproachRangeCallback(Script::FunctionInterface* pInterface)
{
	if (!pInterface->GetFunction(1u, m_pLeaveCallback))
	{
		pInterface->RaiseError(1u, "expected function.");
		return;
	}
}

/**
 * Instantaneous velocity set. Uses the object's current facing direction
 * and the provided velocity magnitude.
 *
 * fVelocityMag is calmped to the previously set max velocity.
 */
void ScriptMotionApproach::SetVelocityToFacing(Float fVelocityMag)
{
	Vector3D const vFacing = Quaternion::Transform(
		m_pSceneObject->GetRotation(),
		-Vector3D::UnitZ());
	m_vVelocity = Min(fVelocityMag, m_fMaxVelocityMag) * vFacing;
}

/** Per-frame poll of motion util. */
void ScriptMotionApproach::Tick(Scene::Interface& rInterface, Float fDeltaTimeInSeconds)
{
	// Current to final position.
	Vector3D vPosition = m_pSceneObject->GetPosition();

	// Compute starting distance.
	Float const fStartingDistance = GetDistanceToTarget();

	// Compute desired motion direction.
	Vector3D const vDirection = Vector3D::Normalize(m_vApproachTarget - vPosition);

	// Compute acceleration magnitude - dot product between
	// current velocity and desired direction re-ranged. to to [0, 1].
	Float const fAccelerationMag = Lerp(
		m_fReverseAccelerationMag,
		m_fAccelerationMag,
		Clamp(Vector3D::Dot(vDirection, Vector3D::Normalize(m_vVelocity)) * 0.5f + 0.5f, 0.0f, 1.0f));

	// Accelerate towards target.
	m_vAcceleration = vDirection * fAccelerationMag;

	// Call base to apply physics.
	ScriptMotion::Tick(rInterface, fDeltaTimeInSeconds);

	// Compute ending distance.
	Float const fEndingDistance = GetDistanceToTarget();

	// Invoke callbacks if defined and conditions are met.
	if (fStartingDistance <= m_fApproachRange &&
		fEndingDistance > m_fApproachRange &&
		m_pLeaveCallback.IsValid())
	{
		Script::FunctionInvoker invoker(m_pLeaveCallback);
		if (invoker.IsValid())
		{
			(void)invoker.TryInvoke();
		}
	}
	else if (
		fStartingDistance > m_fApproachRange &&
		fEndingDistance <= m_fApproachRange &&
		m_pEnterCallback.IsValid())
	{
		Script::FunctionInvoker invoker(m_pEnterCallback);
		if (invoker.IsValid())
		{
			(void)invoker.TryInvoke();
		}
	}
}

} // namespace Seoul

#endif // /#if SEOUL_WITH_SCENE
