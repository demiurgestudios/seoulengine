/**
 * \file PhysicsTest.cpp
 * \brief Navigation unit test .
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
#include "PhysicsTest.h"
#include "ReflectionDefine.h"
#include "UnitTesting.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

#if SEOUL_WITH_PHYSICS

static const Float kfFixedTimeStep = (1.0f / 60.0f);

SEOUL_BEGIN_TYPE(PhysicsTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestDynamicsSingleBoxStack)
	SEOUL_METHOD(TestDynamicsSmallBoxStack)
	SEOUL_METHOD(TestDynamicsLargeBoxStack)
SEOUL_END_TYPE()

namespace
{

struct State
{
	Vector3D m_vA;
	Quaternion m_qO;
	Vector3D m_vP;
	Vector3D m_vL;
};

} // namespace

static void TestBoxStack(
	UInt32 uDynamicBoxes,
	Float32 fDynamicBoxDensity,
	UInt32 uInitialSolverFrameCount,
	Float fFriction,
	Float fE0, Float fE1, Float fE2, Float fE3)
{
	using namespace Physics;

	Simulator simulator;

	Vector<SharedPtr<Physics::Body>, MemoryBudgets::Developer> vBodies;

	// Static base.
	{
		BodyDef def;
		def.m_eType = BodyType::kStatic;
		def.m_Shape.SetType(ShapeType::kBox);
		def.m_Shape.GetData<Physics::BoxShapeData>()->m_vExtents = Vector3D::One();
		def.m_Shape.m_fFriction = fFriction;
		vBodies.PushBack(simulator.CreateBody(def));
	}

	// Dynamic stack.
	for (UInt32 i = 0u; i < uDynamicBoxes; ++i)
	{
		Float32 const fY = 2.0f * ((Float)i + 1.0f);

		BodyDef def;
		def.m_eType = BodyType::kDynamic;
		def.m_Shape.SetType(ShapeType::kBox);
		def.m_Shape.m_fDensity = fDynamicBoxDensity;
		def.m_Shape.GetData<Physics::BoxShapeData>()->m_vExtents = Vector3D::One();
		def.m_Shape.m_fFriction = fFriction;
		def.m_vPosition = Vector3D(0, fY, 0);
		vBodies.PushBack(simulator.CreateBody(def));
	}

	// Disable sleeping for stability testing. Then do an explicit sleeping test
	// at the end.
	simulator.GetSettings().m_bDisableSleeping = true;
	for (UInt32 i = 0u; i < uInitialSolverFrameCount; ++i)
	{
		simulator.Step(kfFixedTimeStep);

		for (auto p : vBodies)
		{
			if (p->GetType() == BodyType::kStatic)
			{
				SEOUL_UNITTESTING_ASSERT_EQUAL(Quaternion::Identity(), p->GetOrientation());
				SEOUL_UNITTESTING_ASSERT_EQUAL(Vector3D::Zero(), p->GetPosition());
			}
		}
	}

	// Static shape must remain unmoved.
	SEOUL_UNITTESTING_ASSERT_EQUAL(Quaternion::Identity(), vBodies[0]->GetOrientation());
	SEOUL_UNITTESTING_ASSERT_EQUAL(Vector3D::Zero(), vBodies[0]->GetPosition());
	SEOUL_UNITTESTING_ASSERT_EQUAL(Vector3D::Zero(), vBodies[0]->GetAngularVelocity());
	SEOUL_UNITTESTING_ASSERT_EQUAL(Vector3D::Zero(), vBodies[0]->GetLinearVelocity());

	// Dynamic shapes should have settled on top of the static box.
	for (UInt32 i = 0u; i < uDynamicBoxes; ++i)
	{
		auto const qO = vBodies[i + 1]->GetOrientation();
		auto const vP = vBodies[i + 1]->GetPosition();
		auto const vA = vBodies[i + 1]->GetAngularVelocity();
		auto const vL = vBodies[i + 1]->GetLinearVelocity();
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(
			Quaternion::Identity(),
			qO,
			fE0);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(
			Vector3D(0, (Float)(2 * (i + 1)), 0),
			vP,
			fE1);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(
			Vector3D::Zero(),
			vA,
			fE2);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(
			Vector3D::Zero(),
			vL,
			fE3);
	}

	// Now, capture current state and test that it stays the same (within kfEpsilon) for 10000 frames.
	{
		Vector<State> vStates;
		for (UInt32 i = 0u; i < uDynamicBoxes; ++i)
		{
			State state;
			state.m_qO = vBodies[i + 1]->GetOrientation();
			state.m_vA = vBodies[i + 1]->GetAngularVelocity();
			state.m_vL = vBodies[i + 1]->GetLinearVelocity();
			state.m_vP = vBodies[i + 1]->GetPosition();
			vStates.PushBack(state);
		}

		for (UInt32 i = 0u; i < 10000u; ++i)
		{
			simulator.Step(kfFixedTimeStep);

			for (UInt32 j = 0u; j < uDynamicBoxes; ++j)
			{
				const State& state = vStates[j];
				Quaternion const qDifference = Quaternion::Normalize(state.m_qO * vBodies[j + 1]->GetOrientation().Inverse());
				Vector3D const vDifference = (state.m_vP - vBodies[j + 1]->GetPosition());
				Vector3D const vAngular = vBodies[j + 1]->GetAngularVelocity();
				Vector3D const vLinear = vBodies[j + 1]->GetLinearVelocity();

				SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(
					Quaternion::Identity(),
					qDifference,
					fE0);
				SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(
					Vector3D::Zero(),
					vDifference,
					fE1);
				SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(
					state.m_vA,
					vAngular,
					fE2);
				SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(
					state.m_vL,
					vLinear,
					fE3);
			}

			for (auto p : vBodies)
			{
				if (p->GetType() == BodyType::kStatic)
				{
					SEOUL_UNITTESTING_ASSERT_EQUAL(Quaternion::Identity(), p->GetOrientation());
					SEOUL_UNITTESTING_ASSERT_EQUAL(Vector3D::Zero(), p->GetPosition());
				}
			}
		}
	}

	// Finally, check for sleeping. We need to allow the minimum sleep time to pass,
	// then verify sleeping.
	simulator.GetSettings().m_bDisableSleeping = false;

	Float fTime = 0.0f;
	while (fTime < 0.2f)
	{
		fTime += kfFixedTimeStep;
		simulator.Step(kfFixedTimeStep);
	}

	// Now verify all shapes are asleep.
	for (auto p : vBodies)
	{
		SEOUL_UNITTESTING_ASSERT(
			BodyType::kStatic == p->GetType() ||
			p->IsSleeping());
	}
}

void PhysicsTest::TestDynamicsSingleBoxStack()
{
	TestBoxStack(1, 1.0f, 10u, 0.0f, 1e-4f, 1e-4f, 1e-4f, 1e-4f);
}

void PhysicsTest::TestDynamicsSmallBoxStack()
{
	TestBoxStack(5, 1.0f, 50u, 0.1f, 1e-3f, 1e-2f, 1e-2f, 2e-2f);
}

void PhysicsTest::TestDynamicsLargeBoxStack()
{
	TestBoxStack(10, 1.0f, 250u, 0.3f, 1e-2f, 0.1f, 2e-2f, 0.1f);
}

#endif // /#if SEOUL_WITH_PHYSICS

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
