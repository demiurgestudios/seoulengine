/**
 * \file ScriptMotion.h
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

#pragma once
#ifndef SCRIPT_MOTION_H
#define SCRIPT_MOTION_H

#include "ScriptSceneTicker.h"
namespace Seoul { namespace Scene { class Object; } }
namespace Seoul { namespace Script { class FunctionInterface; } }

#if SEOUL_WITH_SCENE

namespace Seoul
{

class ScriptMotion SEOUL_ABSTRACT : public ScriptSceneTicker
{
public:
	virtual ~ScriptMotion();

	/** Acquire the physical properties (acceleration and velocity) from b. */
	void CopyPhysicalProps(const ScriptMotion& b)
	{
		m_vAcceleration = b.m_vAcceleration;
		m_vVelocity = b.m_vVelocity;
	}

	/** Base Tick(), applies acceleration to velocity and velocity to the object's position. */
	virtual void Tick(Scene::Interface& rInterface, Float fDeltaTimeInSeconds) SEOUL_OVERRIDE;

	// Script hooks
	// Object construction hook - can be override in derived classes, but
	// base method must always be called for correct behavior.
	virtual void Construct(Script::FunctionInterface* pInterface);

	// Script variation of CopyPhysicalProps(ScriptMotion)
	void CopyPhysicalProps(Script::FunctionInterface* pInterface);

	// Set the maximum travel velocity during approach.
	void SetMaxVelocityMag(Float fVelocity);

	// If true, orient to velocity.
	void SetOrientToVelocity(Bool bOrient);
	// /Script hooks

protected:
	ScriptMotion();

	SharedPtr<Scene::Object> m_pSceneObject;
	Vector3D m_vAcceleration;
	Vector3D m_vVelocity;
	Float m_fMaxVelocityMag;
	Bool m_bOrientToVelocity;

private:
	SEOUL_DISABLE_COPY(ScriptMotion);
}; // class ScriptMotion

} // namespace Seoul

#endif // /#if SEOUL_WITH_SCENE

#endif // include guard
