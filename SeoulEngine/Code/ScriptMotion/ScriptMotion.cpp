/**
 * \file ScriptMotion.cpp
 * \brief Base class for ScriptMotion utilities. ScriptMotion utilities
 * provides ScriptSceneTicker specializations that are focused
 * on object motion simulation.
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
#include "ScriptMotion.h"
#include "ScriptSceneObject.h"

#if SEOUL_WITH_SCENE

namespace Seoul
{

/** Small value for minimum max velocity to avoid divide-by-zero. */
static const Float kfMinimumMaxVelocity = 1e-5f;

SEOUL_BEGIN_TYPE(ScriptMotion, TypeFlags::kDisableNew)
	SEOUL_PARENT(ScriptSceneTicker)
	SEOUL_METHOD(Construct)
	SEOUL_METHOD(SetMaxVelocityMag)
	SEOUL_METHOD(SetOrientToVelocity)
SEOUL_END_TYPE()

ScriptMotion::ScriptMotion()
	: m_pSceneObject()
	, m_vAcceleration(Vector3D::Zero())
	, m_vVelocity(Vector3D::Zero())
	, m_fMaxVelocityMag(kfMinimumMaxVelocity)
	, m_bOrientToVelocity(false)
{
}

ScriptMotion::~ScriptMotion()
{
}

void ScriptMotion::Construct(Script::FunctionInterface* pInterface)
{
	// Grab our scene object - error out if no specified.
	ScriptSceneObject* pObject = pInterface->GetUserData<ScriptSceneObject>(1u);
	if (nullptr == pObject)
	{
		pInterface->RaiseError(1u, "expected scene object user data");
		return;
	}

	m_pSceneObject = pObject->m_pSceneObject;
}

void ScriptMotion::CopyPhysicalProps(Script::FunctionInterface* pInterface)
{
	// Grab the ScriptMotion user data to copy from.
	ScriptMotion* pMotion = pInterface->GetUserData<ScriptMotion>(1u);
	if (nullptr == pMotion)
	{
		pInterface->RaiseError(1u, "expected scene user data");
		return;
	}

	CopyPhysicalProps(*pMotion);
}

/** Set the maximum travel velocity during approach. */
void ScriptMotion::SetMaxVelocityMag(Float fVelocity)
{
	// Clamp max velocity mag to avoid divide-by-zero.
	m_fMaxVelocityMag = Max(fVelocity, kfMinimumMaxVelocity);
}

/** If true, orient to velocity. */
void ScriptMotion::SetOrientToVelocity(Bool bOrient)
{
	m_bOrientToVelocity = bOrient;
}

/** Base Tick(), applies acceleration to velocity and velocity to the object's position. */
void ScriptMotion::Tick(Scene::Interface& rInterface, Float fDeltaTimeInSeconds)
{
	// Apply acceleration.
	m_vVelocity += m_vAcceleration * fDeltaTimeInSeconds;

	// Clamp velocity.
	Float const fMagVelocity = m_vVelocity.Length();
	if (fMagVelocity > m_fMaxVelocityMag)
	{
		m_vVelocity *= (m_fMaxVelocityMag / fMagVelocity);
	}

	// Compute delta position.
	Vector3D vMotion = (m_vVelocity * fDeltaTimeInSeconds);

	// Apply orientation.
	if (m_bOrientToVelocity)
	{
		Quaternion const qRotation(Quaternion::CreateFromDirection(Vector3D::Normalize(vMotion)));
		m_pSceneObject->SetRotation(qRotation);
	}

	// Apply position delta and update.
	m_pSceneObject->SetPosition(m_pSceneObject->GetPosition() + vMotion);
}

} // namespace Seoul

#endif // /#if SEOUL_WITH_SCENE
