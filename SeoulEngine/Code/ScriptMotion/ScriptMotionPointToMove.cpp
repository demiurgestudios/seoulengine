/**
 * \file ScriptMotionPointToMove.cpp
 * \brief Bound scriptable instance tied to
 * a SceneObject that provides "point-to-move"
 * behavior.
 *
 * There are three main pieces to this behavior:
 * - physics raycasts to determine target position.
 * - navgrid queries to find the navigation path.
 * - path following behavior.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "PhysicsSimulator.h"
#include "Ray3D.h"
#include "ReflectionDefine.h"
#include "SceneNavigationGridComponent.h"
#include "SceneInterface.h"
#include "SceneObject.h"
#include "ScriptMotion.h"

#if SEOUL_WITH_SCENE

namespace Seoul
{

namespace
{

enum class State
{
	kNoRefresh, // We have a target
	kNewRay, // A new ray has been picked, but we don't have a pending raycast.
	kPendingRayCast, // A raycast request has been issued to the physics simulator.
	kPendingPath, // A path generation has been issued to the navigation grid.
};

class RayCastCallback SEOUL_SEALED : public Physics::IRayCastCallback
{
public:
	RayCastCallback()
		: m_vPoint()
		, m_bHit(false)
		, m_bDone(false)
	{
	}

	~RayCastCallback()
	{
	}

	virtual void OnRayCast(Bool bHit, const Vector3D& vPoint) SEOUL_OVERRIDE
	{
		m_vPoint = vPoint;
		m_bHit = bHit;
		SeoulMemoryBarrier();
		m_bDone = true;
	}

	Vector3D m_vPoint;
	Bool m_bHit;
	Atomic32Value<Bool> m_bDone;

private:
	SEOUL_DISABLE_COPY(RayCastCallback);
	SEOUL_REFERENCE_COUNTED_SUBCLASS(RayCastCallback);
}; // class RayCastCallback

} // namespace anonymous

class ScriptMotionPointToMove SEOUL_SEALED : public ScriptMotion
{
public:
	ScriptMotionPointToMove();
	~ScriptMotionPointToMove();

	// Script hooks.

	// Set the acceleration of our approach.
	void SetAccelerationMag(Float fAccelerationMag)
	{
		m_fAccelerationMag = fAccelerationMag;
	}

	// Update the id of the navigation grid we will use for motion.
	void SetNavigationGrid(const String& sId);

	// Update the screen space pick position for motion.
	void SetScreenSpacePoint(Int32 iX, Int32 iY)
	{
		m_ScreenSpacePoint.X = iX;
		m_ScreenSpacePoint.Y = iY;
		m_eState = State::kNewRay;
	}

	// Call once per frame to apply motion.
	virtual void Tick(Scene::Interface& rInterface, Float fDeltaTimeInSeconds) SEOUL_OVERRIDE;

	// /Script hooks.

private:
	Float m_fAccelerationMag;
	String m_sNavigationGridId;
	SharedPtr<Scene::NavigationGridComponent> m_pNavigationGrid;
	Point2DInt m_ScreenSpacePoint;
	State m_eState;
	SharedPtr<RayCastCallback> m_pRayCast;
	SharedPtr<Scene::NavigationGridQuery> m_pPathFind;
	SharedPtr<Scene::NavigationGridQuery> m_pPathFollow;
	UInt32 m_uProgress;

	void CheckTarget(Scene::Interface& rInterface);
	SharedPtr<Scene::NavigationGridComponent> ResolveNavigationGrid(Scene::Interface& rInterface);

	SEOUL_DISABLE_COPY(ScriptMotionPointToMove);
}; // class ScriptMotionPointToMove

SEOUL_BEGIN_TYPE(ScriptMotionPointToMove, TypeFlags::kDisableCopy)
	SEOUL_PARENT(ScriptMotion)
	SEOUL_METHOD(SetAccelerationMag)
	SEOUL_METHOD(SetNavigationGrid)
	SEOUL_METHOD(SetScreenSpacePoint)
SEOUL_END_TYPE()

ScriptMotionPointToMove::ScriptMotionPointToMove()
	: m_fAccelerationMag(0.0f)
	, m_sNavigationGridId()
	, m_pNavigationGrid()
	, m_ScreenSpacePoint()
	, m_eState(State::kNoRefresh)
	, m_pRayCast()
	, m_pPathFind()
	, m_pPathFollow()
	, m_uProgress(0u)
{
}

ScriptMotionPointToMove::~ScriptMotionPointToMove()
{
}

/** Update the id of the navigation grid used for path finding. */
void ScriptMotionPointToMove::SetNavigationGrid(const String& sId)
{
	m_sNavigationGridId = sId;
	if (!m_pSceneObject.IsValid())
	{
		return;
	}

	String s(m_pSceneObject->GetId());
	Scene::Object::RemoveLeafId(s);
	Scene::Object::QualifyId(s, m_sNavigationGridId);
	m_pNavigationGrid.Reset();
}

/** Per-frame poll of motion util. */
void ScriptMotionPointToMove::Tick(Scene::Interface& rInterface, Float fDeltaTimeInSeconds)
{
	CheckTarget(rInterface);

	// Nothing to do if no path to follow.
	if (!m_pPathFollow.IsValid() || !m_pPathFollow->IsDone())
	{
		return;
	}

	// Nothing to do if no point.
	Vector3D vNext;
	if (!m_pPathFollow->GetPoint(m_uProgress, vNext))
	{
		m_vAcceleration = Vector3D::Zero();
		m_vVelocity = Vector3D::Zero();
		return;
	}

	// Current to final position.
	Vector3D vPosition = m_pSceneObject->GetPosition();

	// Compute desired motion direction.
	Vector3D const vDirection = Vector3D::Normalize(vNext - vPosition);

	m_vVelocity = (vDirection * m_fAccelerationMag);

	// Call base to apply physics.
	ScriptMotion::Tick(rInterface, fDeltaTimeInSeconds);

	// Check if we should update the next progress.
	if ((vNext - m_pSceneObject->GetPosition()).LengthSquared() <= (0.1f * 0.1f) /* TODO: Configure. */)
	{
		m_uProgress++;
	}
}

void ScriptMotionPointToMove::CheckTarget(Scene::Interface& rInterface)
{
	// Action depending on state.
	switch (m_eState)
	{
		// No action.
	case State::kNoRefresh:
		break;

		// A new point has been picked, need to issue a ray cast with the physics
		// simulator.
	case State::kNewRay:
	{
		auto pSimulator = rInterface.GetPhysicsSimulator();

		// No simulator, immediately return to no refresh.
		if (nullptr == pSimulator)
		{
			m_eState = State::kNoRefresh;
			return;
		}

		// Derive v0 and v1 for the ray cast request.
		Vector3D v0, v1;
		if (!rInterface.ConvertScreenSpaceToWorldSpace(Vector3D((Float)m_ScreenSpacePoint.X, (Float)m_ScreenSpacePoint.Y, 0.0f), v0) ||
			!rInterface.ConvertScreenSpaceToWorldSpace(Vector3D((Float)m_ScreenSpacePoint.X, (Float)m_ScreenSpacePoint.Y, 1.0f), v1))
		{
			m_eState = State::kNoRefresh;
			return;
		}

		// Start a ray cast request.
		m_eState = State::kPendingRayCast;
		m_pRayCast.Reset(SEOUL_NEW(MemoryBudgets::Physics) RayCastCallback);
		pSimulator->RayCast(v0, v1, m_pRayCast);
	}
	break;

		// Waiting for a ray cast request to be returned.
	case State::kPendingRayCast:
	{
		// Check for completion.
		if (!m_pRayCast->m_bDone)
		{
			return;
		}

		// No hit, done.
		if (!m_pRayCast->m_bHit)
		{
			m_eState = State::kNoRefresh;
			return;
		}

		// Now issue the nav query.
		auto pNavGrid(ResolveNavigationGrid(rInterface));

		// Done if couldn't get the grid.
		if (!pNavGrid.IsValid())
		{
			m_eState = State::kNoRefresh;
			return;
		}

		// Issue the path query.
		auto const vStart(m_pSceneObject->GetPosition());
		auto const vEnd(m_pRayCast->m_vPoint);
		if (!pNavGrid->RobustFindStraightPath(
			vStart,
			vEnd,
			1u, // TODO: Configure
			1u, // TODO: Configure
			m_pPathFind))
		{
			m_eState = State::kNoRefresh;
			return;
		}
		else
		{
			m_eState = State::kPendingPath;
		}
	}
	break;

		// Waiting for a navigation query request to be returend.
	case State::kPendingPath:
	{
		if (!m_pPathFind->IsDone())
		{
			return;
		}

		// On failure, done immediately.
		if (!m_pPathFind->WasSuccessful())
		{
			m_eState = State::kNoRefresh;
			return;
		}

		// Complete and set new target.
		m_eState = State::kNoRefresh;
		m_pPathFollow.Swap(m_pPathFind);
		m_pPathFind.Reset();
		m_uProgress = 0u;
	}
	break;

		// Fallback.
	default:
		break;
	};
}

SharedPtr<Scene::NavigationGridComponent> ScriptMotionPointToMove::ResolveNavigationGrid(Scene::Interface& rInterface)
{
	if (!m_pNavigationGrid.IsValid())
	{
		SharedPtr<Scene::Object> pObject;
		if (!rInterface.GetObjectById(m_sNavigationGridId, pObject))
		{
			return m_pNavigationGrid;
		}

		m_pNavigationGrid = pObject->GetComponent<Scene::NavigationGridComponent>();
	}

	return m_pNavigationGrid;
}

} // namespace Seoul

#endif // /#if SEOUL_WITH_SCENE
