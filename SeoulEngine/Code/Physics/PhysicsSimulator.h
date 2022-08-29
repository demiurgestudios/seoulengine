/**
 * \file PhysicsSimulator.h
 * \brief Defines the root physical world and handles
 * body management and per-frame updates.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef PHYSICS_SIMULATOR_H
#define PHYSICS_SIMULATOR_H

#include "Delegate.h"
#include "Prereqs.h"
#include "Ray3D.h"
#include "ScopedPtr.h"
#include "SharedPtr.h"
#include "Vector.h"
namespace Seoul { namespace Physics { class Body; } }
namespace Seoul { namespace Physics { class ContactListener; } }
namespace Seoul { namespace Physics { struct BodyDef; } }
struct b3Hull;
class b3World;

#if SEOUL_WITH_PHYSICS

namespace Seoul::Physics
{

struct SimulatorSettings SEOUL_SEALED
{
	SimulatorSettings()
		: m_uPositionIterations(2u)
		, m_uVelocityIterations(8u)
		, m_bDisableSleeping(false)
		, m_bDisableWarmStart(false)
	{
	}

	/** Number of iterations used to solve position constraints. */
	Int32 m_uPositionIterations;

	/** Number of iterations used to solve velocity constraints. */
	Int32 m_uVelocityIterations;

	/**
	 * If true, still bodies will not go to sleep. This will adversely affect performance
	 * and is typically only used when testing or debugging simulation stability.
	 */
	Bool m_bDisableSleeping;

	/**
	 * If true, contacts will not be warm started. This tends to adversely
	 * affect solving stability and performance.
	 */
	Bool m_bDisableWarmStart;
}; // struct SimulatorSettings

/**
 * Base class of an instance to be invoked by the physics simulator
 * when a ray cast has completed.
 */
class IRayCastCallback SEOUL_ABSTRACT
{
public:
	virtual ~IRayCastCallback()
	{
	}

	virtual void OnRayCast(Bool bHit, const Vector3D& vPoint) = 0;

protected:
	IRayCastCallback()
	{
	}

	SEOUL_REFERENCE_COUNTED(IRayCastCallback);

private:
	SEOUL_DISABLE_COPY(IRayCastCallback);
}; // class IRayCastCallback

enum class ContactEvent
{
	/** Contact event when entering a sensor. */
	kSensorEnter,

	/** Contact event when leaving a sensor. */
	kSensorLeave,
};

struct SensorEntry SEOUL_SEALED
{
	void* m_pSensor{};
	void* m_pBody{};
	ContactEvent m_eEvent{};
}; // TriggerEntry
typedef Vector<SensorEntry, MemoryBudgets::Physics> SensorEvents;

class Simulator SEOUL_SEALED
{
public:
	Simulator(const SimulatorSettings& settings = SimulatorSettings());
	~Simulator();

	SharedPtr<Body> CreateBody(const BodyDef& bodyDef, const Vector3D& vInitialScale = Vector3D::One(), void* pUserData = nullptr);

	const SimulatorSettings& GetSettings() const { return m_Settings; }
	SimulatorSettings& GetSettings() { return m_Settings; }

	// Get the current queue of sensor events. Reset and updated with each call to Step(),
	// so you'll want to check and respond to this after invoking Step() to avoid missing
	// sensor events.
	const SensorEvents& GetSensorEvents() const;

	void RayCast(
		const Vector3D& v0,
		const Vector3D& v1,
		const SharedPtr<IRayCastCallback>& pCallback);

	void Step(Float fDeltaTimeInSeconds);

private:
	SimulatorSettings m_Settings;
	ScopedPtr<ContactListener> m_pContactListener;
	ScopedPtr<b3World> m_pWorld;

	void Destroy(SharedPtr<Body>& rp);
	void PruneBodies();

	typedef Vector<SharedPtr<Body>, MemoryBudgets::Physics> Bodies;
	Bodies m_vBodies;

	typedef Vector<CheckedPtr<b3Hull>, MemoryBudgets::Physics> Hulls;
	Hulls m_vHullScratch;

	struct RayCastEntry SEOUL_SEALED
	{
		Vector3D m_v0;
		Vector3D m_v1;
		SharedPtr<IRayCastCallback> m_pCallback;
	};
	typedef Vector<RayCastEntry, MemoryBudgets::Physics> RayCasts;
	RayCasts m_vRayCasts;

	SEOUL_DISABLE_COPY(Simulator);
}; // class Simulator

} // namespace Seoul::Physics

#endif // /#if SEOUL_WITH_PHYSICS

#endif // include guard
