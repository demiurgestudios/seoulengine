/**
 * \file PhysicsSimulator.cpp
 * \brief Defines the root physical world and handles
 * body management and per-frame updates.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Logger.h"
#include "PhysicsBody.h"
#include "PhysicsBodyDef.h"
#include "PhysicsSimulator.h"
#include "PhysicsUtil.h"
#include "SeoulProfiler.h"

#if SEOUL_WITH_PHYSICS

#include <bounce/collision/shapes/box_hull.h>
#include <bounce/dynamics/body.h>
#include <bounce/dynamics/contacts/contact.h>
#include <bounce/dynamics/shapes/capsule_shape.h>
#include <bounce/dynamics/shapes/hull_shape.h>
#include <bounce/dynamics/shapes/shape.h>
#include <bounce/dynamics/shapes/sphere_shape.h>
#include <bounce/dynamics/world.h>
#include <bounce/dynamics/world_listeners.h>

// Start of up definitions - not a fan of this.
void* b3Alloc(u32 size)
{
	return Seoul::MemoryManager::Allocate(size, Seoul::MemoryBudgets::Physics);
}

void b3Free(void* block)
{
	Seoul::MemoryManager::Deallocate(block);
}

#if SEOUL_LOGGING_ENABLED
void b3Log(const char* text, ...)
{
	va_list args;
	va_start(args, text);
	Seoul::Logger::GetSingleton().LogMessage(Seoul::LoggerChannel::Physics, text, args);
	va_end(args);
}
#endif // /#if SEOUL_LOGGING_ENABLED

#if B3_ENABLE_PROFILING
int b3GetProfileId(const char* name)
{
	return (int)Seoul::HString(name).GetHandleValue();
}

void b3PushProfileScope(int i)
{
	Seoul::HString name;
	name.SetHandleValue((Seoul::UInt32)i);
	Seoul::Profiler::BeginSample(name);
}

void b3PopProfileScope(int i)
{
	Seoul::Profiler::EndSample();
}
#endif // /#if SEOUL_PROFILING_BUILD
// End of up definitions.

namespace Seoul::Physics
{

static inline b3BodyType Convert(BodyType eType)
{
	switch (eType)
	{
	case BodyType::kStatic: return e_staticBody;
	case BodyType::kKinematic: return e_kinematicBody;
	case BodyType::kDynamic: return e_dynamicBody;
	default:
		SEOUL_FAIL("Out-of-sync enum.");
		return e_staticBody;
	};
}

static inline void Convert(const BodyDef& def, b3BodyDef& defB3)
{
	defB3.type = Convert(def.m_eType);
	defB3.orientation = Convert(def.m_qOrientation);
	defB3.position = Convert(def.m_vPosition);
}

namespace
{

struct ConvexHullB3 SEOUL_SEALED : b3Hull
{
	ConvexHullShapeData m_Data;

	void Finalize()
	{
		// Sanity checks to make sure we can safely reinterpret several
		// SeoulEngine types as bounce types.
		SEOUL_STATIC_ASSERT(sizeof(b3HalfEdge) == sizeof(ConvexHullEdge));
		SEOUL_STATIC_ASSERT(offsetof(b3HalfEdge, face) == offsetof(ConvexHullEdge, m_uFace));
		SEOUL_STATIC_ASSERT(offsetof(b3HalfEdge, next) == offsetof(ConvexHullEdge, m_uNext));
		SEOUL_STATIC_ASSERT(offsetof(b3HalfEdge, origin) == offsetof(ConvexHullEdge, m_uOrigin));
		SEOUL_STATIC_ASSERT(offsetof(b3HalfEdge, twin) == offsetof(ConvexHullEdge, m_uTwin));
		SEOUL_STATIC_ASSERT(sizeof(b3Face) == sizeof(UInt8));
		SEOUL_STATIC_ASSERT(sizeof(b3Plane) == sizeof(Plane));
		SEOUL_STATIC_ASSERT(offsetof(b3Plane, normal) == offsetof(Plane, A));
		SEOUL_STATIC_ASSERT(offsetof(b3Plane, offset) == offsetof(Plane, D));
		SEOUL_STATIC_ASSERT(sizeof(b3Vec3) == sizeof(Vector3D));
		SEOUL_STATIC_ASSERT(offsetof(b3Vec3, x) == offsetof(Vector3D, X));
		SEOUL_STATIC_ASSERT(offsetof(b3Vec3, y) == offsetof(Vector3D, Y));
		SEOUL_STATIC_ASSERT(offsetof(b3Vec3, z) == offsetof(Vector3D, Z));

		centroid = Convert(m_Data.GetCenterOfMass());
		edgeCount = m_Data.GetEdges().GetSize();
		edges = (b3HalfEdge*)m_Data.GetEdges().Data();
		faceCount = m_Data.GetFaces().GetSize();
		faces = (b3Face*)m_Data.GetFaces().Data();
		planes = (b3Plane*)m_Data.GetPlanes().Data();
		vertexCount = m_Data.GetPoints().GetSize();
		vertices = (b3Vec3*)m_Data.GetPoints().Data();
	}
};

} // namespace anonymous

static inline CheckedPtr<b3Shape> Add(const ShapeDef& def, b3Body& rBody, const Vector3D& vScale)
{
	b3ShapeDef defB3;
	defB3.density = def.m_fDensity;
	defB3.friction = def.m_fFriction;
	defB3.restitution = def.m_fRestitution;
	defB3.isSensor = def.m_bSensor;

	switch (def.GetType())
	{
	case ShapeType::kBox:
		{
			// TODO: Cache these and avoid too many allocations
			// when the boxes can be shared.
			auto pBox = SEOUL_NEW(MemoryBudgets::Physics) b3BoxHull;

			// Input.
			auto pInBox = def.GetData<BoxShapeData>();

			// Scaled.
			BoxShapeData scaled;
			pInBox->ComputeScaled(vScale, scaled);

			// Output.
			b3Transform xf;
			xf.position = Convert(scaled.m_vCenter);
			xf.rotation = b3Diagonal(scaled.m_vExtents.X, scaled.m_vExtents.Y, scaled.m_vExtents.Z);
			pBox->SetTransform(xf);

			b3HullShape shape;
			shape.m_hull = (b3Hull*)pBox;
			defB3.shape = &shape;

			return rBody.CreateShape(defB3);
		}
		break;
	case ShapeType::kCapsule:
		{
			// Input.
			auto pInCapsule = def.GetData<CapsuleShapeData>();

			// Scale.
			CapsuleShapeData scaled;
			pInCapsule->ComputeScaled(vScale, scaled);

			// Output.
			b3CapsuleShape shape;
			shape.m_centers[0] = Convert(scaled.m_vP0);
			shape.m_centers[1] = Convert(scaled.m_vP1);
			shape.m_radius = scaled.m_fRadius;
			defB3.shape = &shape;

			return rBody.CreateShape(defB3);
		}
		break;
	case ShapeType::kConvexHull:
		{
			// Input.
			auto pInConvexHull = def.GetData<ConvexHullShapeData>();

			// TODO: Cache these and avoid too many allocations
			// when the boxes can be shared.
			auto pHull = SEOUL_NEW(MemoryBudgets::Physics) ConvexHullB3;

			// Scale.
			pInConvexHull->ComputeScaled(vScale, pHull->m_Data);

			// Complete.
			pHull->Finalize();

			// Output.
			b3HullShape shape;
			shape.m_hull = (b3Hull*)pHull;
			defB3.shape = &shape;

			return rBody.CreateShape(defB3);
		}
		break;
	case ShapeType::kSphere:
		{
			// Input.
			auto pInSphere = def.GetData<SphereShapeData>();

			// Scale.
			SphereShapeData scaled;
			pInSphere->ComputeScaled(vScale, scaled);

			// Output.
			b3SphereShape shape;
			shape.m_center = Convert(scaled.m_vCenter);
			shape.m_radius = scaled.m_fRadius;
			defB3.shape = &shape;

			return rBody.CreateShape(defB3);
		}
		break;
	case ShapeType::kNone:
	default:
		return nullptr;
	};
}

class ContactListener SEOUL_SEALED : public b3ContactListener
{
public:
	ContactListener()
	{
	}

	~ContactListener()
	{
	}

	virtual void BeginContact(b3Contact* pContact) SEOUL_OVERRIDE
	{
		OnContact(pContact, ContactEvent::kSensorEnter);
	}

	virtual void EndContact(b3Contact* pContact) SEOUL_OVERRIDE
	{
		OnContact(pContact, ContactEvent::kSensorLeave);
	}

	virtual void PreSolve(b3Contact* pContact) SEOUL_OVERRIDE
	{
		// Nop
	}

	SensorEvents m_vEvents;

private:
	void OnContact(b3Contact* pContact, ContactEvent eEvent)
	{
		// Extract parts.
		auto pShapeA = pContact->GetShapeA();
		auto pShapeB = pContact->GetShapeB();
		auto pA = pShapeA->GetBody();
		auto pB = pShapeB->GetBody();

		// Can't report if no user data.
		if (pA->GetUserData() == nullptr || pB->GetUserData() == nullptr)
		{
			return;
		}

		// If A is a sensor, report.
		if (pShapeA->IsSensor())
		{
			SensorEntry entry;
			entry.m_eEvent = eEvent;
			entry.m_pBody = pB->GetUserData();
			entry.m_pSensor = pA->GetUserData();
			m_vEvents.PushBack(entry);
		}
		// Or if B is a sensor, report.
		else if (pShapeB->IsSensor())
		{
			SensorEntry entry;
			entry.m_eEvent = eEvent;
			entry.m_pBody = pA->GetUserData();
			entry.m_pSensor = pB->GetUserData();
			m_vEvents.PushBack(entry);
		}
	}

	SEOUL_DISABLE_COPY(ContactListener);
}; // class ContactListener

Simulator::Simulator(const SimulatorSettings& settings /* = SimulatorSettings() */)
	: m_Settings(settings)
	, m_pContactListener(SEOUL_NEW(MemoryBudgets::Physics) ContactListener)
	, m_pWorld(SEOUL_NEW(MemoryBudgets::Physics) b3World)
	, m_vBodies()
	, m_vHullScratch()
	, m_vRayCasts()
{
	m_pWorld->SetContactListener(m_pContactListener.Get());
}

Simulator::~Simulator()
{
	// Complete ray casts, no hit.
	for (auto const& e : m_vRayCasts)
	{
		e.m_pCallback->OnRayCast(false, Vector3D::Zero());
	}
	m_vRayCasts.Clear();

	// Destroy all bodies.
	for (Int32 i = (Int32)m_vBodies.GetSize() - 1; i >= 0; --i)
	{
		Destroy(m_vBodies[i]);
	}
	m_vBodies.Clear();

	// Cleanup scratch if there are any.
	SafeDeleteVector(m_vHullScratch);

	// Finally, release the world. Sanity check that nothing is dangling.
	SEOUL_ASSERT(0 == m_pWorld->GetBodyList().m_count);
	SEOUL_ASSERT(0 == m_pWorld->GetContactList().m_count);
	SEOUL_ASSERT(0 == m_pWorld->GetJointList().m_count);
	m_pWorld.Reset();

	// Last step, clear the contact listener.
	m_pContactListener.Reset();
}

SharedPtr<Body> Simulator::CreateBody(const BodyDef& bodyDef, const Vector3D& vInitialScale /* = Vector3D::One() */, void* pUserData /* = nullptr */)
{
	// Get a body def for bounce.
	b3BodyDef bodyDefB3;
	Convert(bodyDef, bodyDefB3);

	// Create the body.
	auto pBody(m_pWorld->CreateBody(bodyDefB3));
	pBody->SetUserData(pUserData);

	// TODO: Multiple shape support.
	// Add the single shape to the body.
	(void)Add(bodyDef.m_Shape, *pBody, vInitialScale);

	// Create the body.
	SharedPtr<Body> pReturn(SEOUL_NEW(MemoryBudgets::Physics) Body(pBody));

	// Register the body.
	m_vBodies.PushBack(pReturn);

	return pReturn;
}

const SensorEvents& Simulator::GetSensorEvents() const
{
	return m_pContactListener->m_vEvents;
}

void Simulator::RayCast(
	const Vector3D& v0,
	const Vector3D& v1,
	const SharedPtr<IRayCastCallback>& pCallback)
{
	RayCastEntry e;
	e.m_v0 = v0;
	e.m_v1 = v1;
	e.m_pCallback = pCallback;
	m_vRayCasts.PushBack(e);
}

void Simulator::Step(Float fDeltaTimeInSeconds)
{
	// Prior to simulating, clear the sensor events queue.
	m_pContactListener->m_vEvents.Clear();

	// Prior to simulating, eliminate any unreferenced bodies.
	PruneBodies();

	// Apply settings.
	m_pWorld->SetSleeping(!m_Settings.m_bDisableSleeping);
	m_pWorld->SetWarmStart(!m_Settings.m_bDisableWarmStart);

	// Now step the simulation.
	m_pWorld->Step(fDeltaTimeInSeconds, m_Settings.m_uVelocityIterations, m_Settings.m_uPositionIterations);

	// Finally, process ray casts.
	b3RayCastSingleOutput output;
	for (auto const& e : m_vRayCasts)
	{
		Bool const bHit = m_pWorld->RayCastSingle(&output, Convert(e.m_v0), Convert(e.m_v1));
		Vector3D const vHit = (bHit ? Vector3D::Lerp(e.m_v0, e.m_v1, Clamp(output.fraction, 0.0f, 1.0f)) : Vector3D::Zero());
		e.m_pCallback->OnRayCast(bHit, vHit);
	}
	m_vRayCasts.Clear();
}

void Simulator::Destroy(SharedPtr<Body>& rp)
{
	// Grab the body and clear it.
	auto pBodyB3 = rp->m_pImpl;
	rp->m_pImpl.Reset();

	// Track any hull shapes so we destroy the dependent data.
	auto const& v = pBodyB3->GetShapeList();
	for (auto pShape = v.m_head; nullptr != pShape; pShape = pShape->GetNext())
	{
		if (pShape->GetType() == e_hullShape)
		{
			auto pHullShape = (b3HullShape*)pShape;
			m_vHullScratch.PushBack((b3Hull*)pHullShape->m_hull);
		}
	}


	// Destroy the bounce body.
	m_pWorld->DestroyBody(pBodyB3);
	pBodyB3.Reset();

	// Release our body.
	rp.Reset();
}

void Simulator::PruneBodies()
{
	Int32 iCount = (Int32)m_vBodies.GetSize();
	for (Int32 i = 0; i < iCount; ++i)
	{
		if (m_vBodies[i].IsUnique())
		{
			// Cleanup hull data of any corresponding shapes of the body.
			Destroy(m_vBodies[i]);

			// Use the "swap trick" to delete this item.
			Swap(m_vBodies[i], m_vBodies[iCount - 1]);
			--i;
			--iCount;
		}
	}

	// Done, drop any prunes.
	m_vBodies.Resize(iCount);

	// Cleanup scratch if there are any.
	SafeDeleteVector(m_vHullScratch);
}

} // namespace Seoul::Physics

#endif // /#if SEOUL_WITH_PHYSICS
