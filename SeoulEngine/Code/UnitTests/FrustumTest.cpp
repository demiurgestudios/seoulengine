/**
 * \file FrustumTest.cpp
 * \brief Unit test header file for the Frustum struct.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Frustum.h"
#include "FrustumTest.h"
#include "Logger.h"
#include "ReflectionDefine.h"
#include "UnitTesting.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(FrustumTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestBasic)
	SEOUL_METHOD(TestIntersectsAABB)
	SEOUL_METHOD(TestIntersectsPoint)
	SEOUL_METHOD(TestIntersectsSphere)
	SEOUL_METHOD(TestMiscMethods)
SEOUL_END_TYPE()

void FrustumTest::TestBasic()
{
	// Default test.
	{
		Frustum frustum;
		for (Int i = 0; i < Frustum::PLANE_COUNT; ++i)
		{
			const Plane& plane = frustum.GetPlane(i);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, plane.A);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, plane.B);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, plane.C);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, plane.D);
			SEOUL_UNITTESTING_ASSERT_EQUAL(Vector3D(0, 0, 0), plane.GetNormal());
		}
	}

	// CreateFromPlanes test.
	{
		Frustum frustum(Frustum::CreateFromPlanes(
			Plane::Create(1, 2, 3, 4),
			Plane::Create(5, 6, 7, 8),
			Plane::Create(9, 10, 11, 12),
			Plane::Create(13, 14, 15, 16),
			Plane::Create(17, 18, 19, 20),
			Plane::Create(21, 22, 23, 24)));

		// Near
		{
			const Plane& plane = frustum.GetNearPlane();
			SEOUL_UNITTESTING_ASSERT_EQUAL(&plane, &frustum.GetPlane(Frustum::kNear));
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, plane.A);
			SEOUL_UNITTESTING_ASSERT_EQUAL(2, plane.B);
			SEOUL_UNITTESTING_ASSERT_EQUAL(3, plane.C);
			SEOUL_UNITTESTING_ASSERT_EQUAL(4, plane.D);
			SEOUL_UNITTESTING_ASSERT_EQUAL(Vector3D(1, 2, 3), plane.GetNormal());
		}

		// Far
		{
			const Plane& plane = frustum.GetFarPlane();
			SEOUL_UNITTESTING_ASSERT_EQUAL(&plane, &frustum.GetPlane(Frustum::kFar));
			SEOUL_UNITTESTING_ASSERT_EQUAL(5, plane.A);
			SEOUL_UNITTESTING_ASSERT_EQUAL(6, plane.B);
			SEOUL_UNITTESTING_ASSERT_EQUAL(7, plane.C);
			SEOUL_UNITTESTING_ASSERT_EQUAL(8, plane.D);
			SEOUL_UNITTESTING_ASSERT_EQUAL(Vector3D(5, 6, 7), plane.GetNormal());
		}

		// Left
		{
			const Plane& plane = frustum.GetLeftPlane();
			SEOUL_UNITTESTING_ASSERT_EQUAL(&plane, &frustum.GetPlane(Frustum::kLeft));
			SEOUL_UNITTESTING_ASSERT_EQUAL(9, plane.A);
			SEOUL_UNITTESTING_ASSERT_EQUAL(10, plane.B);
			SEOUL_UNITTESTING_ASSERT_EQUAL(11, plane.C);
			SEOUL_UNITTESTING_ASSERT_EQUAL(12, plane.D);
			SEOUL_UNITTESTING_ASSERT_EQUAL(Vector3D(9, 10, 11), plane.GetNormal());
		}

		// Right
		{
			const Plane& plane = frustum.GetRightPlane();
			SEOUL_UNITTESTING_ASSERT_EQUAL(&plane, &frustum.GetPlane(Frustum::kRight));
			SEOUL_UNITTESTING_ASSERT_EQUAL(13, plane.A);
			SEOUL_UNITTESTING_ASSERT_EQUAL(14, plane.B);
			SEOUL_UNITTESTING_ASSERT_EQUAL(15, plane.C);
			SEOUL_UNITTESTING_ASSERT_EQUAL(16, plane.D);
			SEOUL_UNITTESTING_ASSERT_EQUAL(Vector3D(13, 14, 15), plane.GetNormal());
		}

		// Top
		{
			const Plane& plane = frustum.GetTopPlane();
			SEOUL_UNITTESTING_ASSERT_EQUAL(&plane, &frustum.GetPlane(Frustum::kTop));
			SEOUL_UNITTESTING_ASSERT_EQUAL(17, plane.A);
			SEOUL_UNITTESTING_ASSERT_EQUAL(18, plane.B);
			SEOUL_UNITTESTING_ASSERT_EQUAL(19, plane.C);
			SEOUL_UNITTESTING_ASSERT_EQUAL(20, plane.D);
			SEOUL_UNITTESTING_ASSERT_EQUAL(Vector3D(17, 18, 19), plane.GetNormal());
		}

		// Bottom
		{
			const Plane& plane = frustum.GetBottomPlane();
			SEOUL_UNITTESTING_ASSERT_EQUAL(&plane, &frustum.GetPlane(Frustum::kBottom));
			SEOUL_UNITTESTING_ASSERT_EQUAL(21, plane.A);
			SEOUL_UNITTESTING_ASSERT_EQUAL(22, plane.B);
			SEOUL_UNITTESTING_ASSERT_EQUAL(23, plane.C);
			SEOUL_UNITTESTING_ASSERT_EQUAL(24, plane.D);
			SEOUL_UNITTESTING_ASSERT_EQUAL(Vector3D(21, 22, 23), plane.GetNormal());
		}
	}

	// CreateFromViewProjection tests.
	{
		// Orthographic, identity view.
		{
			Frustum frustum(Frustum::CreateFromViewProjection(
				Matrix4D::CreateOrthographic(-1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f),
				Matrix4D::Identity()));

			SEOUL_UNITTESTING_ASSERT_EQUAL(Plane::Create( 0,  0, -1, 0), frustum.GetNearPlane());
			SEOUL_UNITTESTING_ASSERT_EQUAL(Plane::Create( 0,  0,  1, 1), frustum.GetFarPlane());
			SEOUL_UNITTESTING_ASSERT_EQUAL(Plane::Create( 1,  0,  0, 1), frustum.GetLeftPlane());
			SEOUL_UNITTESTING_ASSERT_EQUAL(Plane::Create(-1,  0,  0, 1), frustum.GetRightPlane());
			SEOUL_UNITTESTING_ASSERT_EQUAL(Plane::Create( 0, -1,  0, 1), frustum.GetTopPlane());
			SEOUL_UNITTESTING_ASSERT_EQUAL(Plane::Create( 0,  1,  0, 1), frustum.GetBottomPlane());
		}

		// Orthographic, view.
		{
			Frustum frustum(Frustum::CreateFromViewProjection(
				Matrix4D::CreateOrthographic(-1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f),
				Matrix4D::CreateTranslation(Vector3D(5, 23, -13))));

			SEOUL_UNITTESTING_ASSERT_EQUAL(Plane::Create( 0,  0, -1,  13), frustum.GetNearPlane());
			SEOUL_UNITTESTING_ASSERT_EQUAL(Plane::Create( 0,  0,  1, -12), frustum.GetFarPlane());
			SEOUL_UNITTESTING_ASSERT_EQUAL(Plane::Create( 1,  0,  0,   6), frustum.GetLeftPlane());
			SEOUL_UNITTESTING_ASSERT_EQUAL(Plane::Create(-1,  0,  0,  -4), frustum.GetRightPlane());
			SEOUL_UNITTESTING_ASSERT_EQUAL(Plane::Create( 0, -1,  0, -22), frustum.GetTopPlane());
			SEOUL_UNITTESTING_ASSERT_EQUAL(Plane::Create( 0,  1,  0,  24), frustum.GetBottomPlane());
		}

		// Perspective, identity view.
		{
			Frustum frustum(Frustum::CreateFromViewProjection(
				Matrix4D::CreatePerspectiveFromVerticalFieldOfView(DegreesToRadians(60.0f), 1.0f, 0.1f, 1.0f),
				Matrix4D::Identity()));

			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Plane::Create(        0.0f,         0.0f, - 1.0f, -0.1f), frustum.GetNearPlane(), 1e-5f);
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Plane::Create(        0.0f,         0.0f,   1.0f,  1.0f), frustum.GetFarPlane(), 1e-5f);
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Plane::Create( 0.86602545f,         0.0f,  -0.5f,  0.0f), frustum.GetLeftPlane(), 1e-5f);
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Plane::Create(-0.86602545f,         0.0f,  -0.5f,  0.0f), frustum.GetRightPlane(), 1e-5f);
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Plane::Create(        0.0f, -0.86602545f,  -0.5f,  0.0f), frustum.GetTopPlane(), 1e-5f);
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Plane::Create(        0.0f,  0.86602545f,  -0.5f,  0.0f), frustum.GetBottomPlane(), 1e-5f);
		}

		// Perspective, view.
		{
			Frustum frustum(Frustum::CreateFromViewProjection(
				Matrix4D::CreatePerspectiveFromVerticalFieldOfView(DegreesToRadians(60.0f), 1.0f, 0.1f, 1.0f),
				Matrix4D::CreateTranslation(Vector3D(5, 23, -13))));

			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Plane::Create(        0.0f,         0.0f, - 1.0f,       12.9f), frustum.GetNearPlane(), 1e-5f);
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Plane::Create(        0.0f,         0.0f,   1.0f,      -12.0f), frustum.GetFarPlane(), 1e-5f);
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Plane::Create( 0.86602545f,         0.0f,  -0.5f,  10.830128f), frustum.GetLeftPlane(), 1e-5f);
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Plane::Create(-0.86602545f,         0.0f,  -0.5f,  2.1698728f), frustum.GetRightPlane(), 1e-5f);
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Plane::Create(        0.0f, -0.86602545f,  -0.5f, -13.418585f), frustum.GetTopPlane(), 1e-5f);
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Plane::Create(        0.0f,  0.86602545f,  -0.5f,  26.418585f), frustum.GetBottomPlane(), 1e-5f);
		}
	}

	// Set test.
	{
		// Orthographic, identity view.
		{
			Frustum frustum;
			frustum.Set(
				Matrix4D::CreateOrthographic(-1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f),
				Matrix4D::Identity());

			SEOUL_UNITTESTING_ASSERT_EQUAL(Plane::Create( 0,  0, -1, 0), frustum.GetNearPlane());
			SEOUL_UNITTESTING_ASSERT_EQUAL(Plane::Create( 0,  0,  1, 1), frustum.GetFarPlane());
			SEOUL_UNITTESTING_ASSERT_EQUAL(Plane::Create( 1,  0,  0, 1), frustum.GetLeftPlane());
			SEOUL_UNITTESTING_ASSERT_EQUAL(Plane::Create(-1,  0,  0, 1), frustum.GetRightPlane());
			SEOUL_UNITTESTING_ASSERT_EQUAL(Plane::Create( 0, -1,  0, 1), frustum.GetTopPlane());
			SEOUL_UNITTESTING_ASSERT_EQUAL(Plane::Create( 0,  1,  0, 1), frustum.GetBottomPlane());
		}

		// Orthographic, view.
		{
			Frustum frustum;
			frustum.Set(
				Matrix4D::CreateOrthographic(-1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f),
				Matrix4D::CreateTranslation(Vector3D(5, 23, -13)));

			SEOUL_UNITTESTING_ASSERT_EQUAL(Plane::Create( 0,  0, -1,  13), frustum.GetNearPlane());
			SEOUL_UNITTESTING_ASSERT_EQUAL(Plane::Create( 0,  0,  1, -12), frustum.GetFarPlane());
			SEOUL_UNITTESTING_ASSERT_EQUAL(Plane::Create( 1,  0,  0,   6), frustum.GetLeftPlane());
			SEOUL_UNITTESTING_ASSERT_EQUAL(Plane::Create(-1,  0,  0,  -4), frustum.GetRightPlane());
			SEOUL_UNITTESTING_ASSERT_EQUAL(Plane::Create( 0, -1,  0, -22), frustum.GetTopPlane());
			SEOUL_UNITTESTING_ASSERT_EQUAL(Plane::Create( 0,  1,  0,  24), frustum.GetBottomPlane());
		}

		// Perspective, identity view.
		{
			Frustum frustum;
			frustum.Set(
				Matrix4D::CreatePerspectiveFromVerticalFieldOfView(DegreesToRadians(60.0f), 1.0f, 0.1f, 1.0f),
				Matrix4D::Identity());

			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Plane::Create(        0.0f,         0.0f, - 1.0f, -0.1f), frustum.GetNearPlane(), 1e-5f);
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Plane::Create(        0.0f,         0.0f,   1.0f,  1.0f), frustum.GetFarPlane(), 1e-5f);
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Plane::Create( 0.86602545f,         0.0f,  -0.5f,  0.0f), frustum.GetLeftPlane(), 1e-5f);
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Plane::Create(-0.86602545f,         0.0f,  -0.5f,  0.0f), frustum.GetRightPlane(), 1e-5f);
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Plane::Create(        0.0f, -0.86602545f,  -0.5f,  0.0f), frustum.GetTopPlane(), 1e-5f);
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Plane::Create(        0.0f,  0.86602545f,  -0.5f,  0.0f), frustum.GetBottomPlane(), 1e-5f);
		}

		// Perspective, view.
		{
			Frustum frustum;
			frustum.Set(
				Matrix4D::CreatePerspectiveFromVerticalFieldOfView(DegreesToRadians(60.0f), 1.0f, 0.1f, 1.0f),
				Matrix4D::CreateTranslation(Vector3D(5, 23, -13)));

			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Plane::Create(        0.0f,         0.0f, - 1.0f,       12.9f), frustum.GetNearPlane(), 1e-5f);
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Plane::Create(        0.0f,         0.0f,   1.0f,      -12.0f), frustum.GetFarPlane(), 1e-5f);
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Plane::Create( 0.86602545f,         0.0f,  -0.5f,  10.830128f), frustum.GetLeftPlane(), 1e-5f);
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Plane::Create(-0.86602545f,         0.0f,  -0.5f,  2.1698728f), frustum.GetRightPlane(), 1e-5f);
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Plane::Create(        0.0f, -0.86602545f,  -0.5f, -13.418585f), frustum.GetTopPlane(), 1e-5f);
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Plane::Create(        0.0f,  0.86602545f,  -0.5f,  26.418585f), frustum.GetBottomPlane(), 1e-5f);
		}
	}
}

void FrustumTest::TestIntersectsAABB()
{
	Frustum frustum(Frustum::CreateFromPlanes(
		Plane::CreateFromPositionAndNormal(Vector3D( 0,  0,  1), -Vector3D::UnitZ()),
		Plane::CreateFromPositionAndNormal(Vector3D( 0,  0, -1),  Vector3D::UnitZ()),
		Plane::CreateFromPositionAndNormal(Vector3D(-1,  0,  0),  Vector3D::UnitX()),
		Plane::CreateFromPositionAndNormal(Vector3D( 1,  0,  0), -Vector3D::UnitX()),
		Plane::CreateFromPositionAndNormal(Vector3D( 0,  1,  0), -Vector3D::UnitY()),
		Plane::CreateFromPositionAndNormal(Vector3D( 1, -1,  0),  Vector3D::UnitY())));

	// Contains
	SEOUL_UNITTESTING_ASSERT_EQUAL(
		FrustumTestResult::kContains,
		frustum.Intersects(AABB::CreateFromMinAndMax(
			-Vector3D::One(),
			Vector3D::One())));

	// Disjoint
	SEOUL_UNITTESTING_ASSERT_EQUAL(
		FrustumTestResult::kDisjoint,
		frustum.Intersects(AABB::CreateFromMinAndMax(
			Vector3D::One(),
			2.0f * Vector3D::One())));

	// Intersects
	SEOUL_UNITTESTING_ASSERT_EQUAL(
		FrustumTestResult::kIntersects,
		frustum.Intersects(AABB::CreateFromMinAndMax(
			-2.0f * Vector3D::One(),
			2.0f * Vector3D::One())));
}

void FrustumTest::TestIntersectsPoint()
{
	Frustum frustum(Frustum::CreateFromPlanes(
		Plane::CreateFromPositionAndNormal(Vector3D( 0,  0,  1), -Vector3D::UnitZ()),
		Plane::CreateFromPositionAndNormal(Vector3D( 0,  0, -1),  Vector3D::UnitZ()),
		Plane::CreateFromPositionAndNormal(Vector3D(-1,  0,  0),  Vector3D::UnitX()),
		Plane::CreateFromPositionAndNormal(Vector3D( 1,  0,  0), -Vector3D::UnitX()),
		Plane::CreateFromPositionAndNormal(Vector3D( 0,  1,  0), -Vector3D::UnitY()),
		Plane::CreateFromPositionAndNormal(Vector3D( 1, -1,  0),  Vector3D::UnitY())));

	// Contains
	SEOUL_UNITTESTING_ASSERT_EQUAL(
		FrustumTestResult::kContains,
		frustum.Intersects(Vector3D::Zero()));

	// Disjoint
	SEOUL_UNITTESTING_ASSERT_EQUAL(
		FrustumTestResult::kDisjoint,
		frustum.Intersects(2.0f * Vector3D::One()));

	// Intersects
	SEOUL_UNITTESTING_ASSERT_EQUAL(
		FrustumTestResult::kIntersects,
		frustum.Intersects(Vector3D::One()));
}

void FrustumTest::TestIntersectsSphere()
{
	Frustum frustum(Frustum::CreateFromPlanes(
		Plane::CreateFromPositionAndNormal(Vector3D( 0,  0,  1), -Vector3D::UnitZ()),
		Plane::CreateFromPositionAndNormal(Vector3D( 0,  0, -1),  Vector3D::UnitZ()),
		Plane::CreateFromPositionAndNormal(Vector3D(-1,  0,  0),  Vector3D::UnitX()),
		Plane::CreateFromPositionAndNormal(Vector3D( 1,  0,  0), -Vector3D::UnitX()),
		Plane::CreateFromPositionAndNormal(Vector3D( 0,  1,  0), -Vector3D::UnitY()),
		Plane::CreateFromPositionAndNormal(Vector3D( 1, -1,  0),  Vector3D::UnitY())));

	// Contains
	SEOUL_UNITTESTING_ASSERT_EQUAL(
		FrustumTestResult::kContains,
		frustum.Intersects(Sphere(Vector3D::Zero(), 1.0f)));

	// Disjoint
	SEOUL_UNITTESTING_ASSERT_EQUAL(
		FrustumTestResult::kDisjoint,
		frustum.Intersects(Sphere(2.0f * Vector3D::One(), 1.0f)));

	// Intersects
	SEOUL_UNITTESTING_ASSERT_EQUAL(
		FrustumTestResult::kIntersects,
		frustum.Intersects(Sphere(Vector3D::One(), 1.0f)));
}

void FrustumTest::TestMiscMethods()
{
	// GetAABB
	{
		Frustum frustum(Frustum::CreateFromPlanes(
			Plane::CreateFromPositionAndNormal(-Vector3D::One(), -Vector3D::UnitZ()),
			Plane::CreateFromPositionAndNormal( Vector3D::One(),  Vector3D::UnitZ()),
			Plane::CreateFromPositionAndNormal(-Vector3D::One(),  Vector3D::UnitX()),
			Plane::CreateFromPositionAndNormal( Vector3D::One(), -Vector3D::UnitX()),
			Plane::CreateFromPositionAndNormal( Vector3D::One(), -Vector3D::UnitY()),
			Plane::CreateFromPositionAndNormal(-Vector3D::One(),  Vector3D::UnitY())));

		AABB const aabb = frustum.GetAABB();

		SEOUL_UNITTESTING_ASSERT_EQUAL(-Vector3D::One(), aabb.m_vMin);
		SEOUL_UNITTESTING_ASSERT_EQUAL( Vector3D::One(), aabb.m_vMax);
	}

	// GetCornerVertices
	{
		Frustum frustum(Frustum::CreateFromPlanes(
			Plane::CreateFromPositionAndNormal(-Vector3D::One(), -Vector3D::UnitZ()),
			Plane::CreateFromPositionAndNormal( Vector3D::One(),  Vector3D::UnitZ()),
			Plane::CreateFromPositionAndNormal(-Vector3D::One(),  Vector3D::UnitX()),
			Plane::CreateFromPositionAndNormal( Vector3D::One(), -Vector3D::UnitX()),
			Plane::CreateFromPositionAndNormal( Vector3D::One(), -Vector3D::UnitY()),
			Plane::CreateFromPositionAndNormal(-Vector3D::One(),  Vector3D::UnitY())));

		Vector3D avCorners[8];
		frustum.GetCornerVertices(avCorners);

		SEOUL_UNITTESTING_ASSERT_EQUAL(Vector3D(-1,  1, -1), avCorners[0]);
		SEOUL_UNITTESTING_ASSERT_EQUAL(Vector3D( 1,  1, -1), avCorners[1]);
		SEOUL_UNITTESTING_ASSERT_EQUAL(Vector3D( 1, -1, -1), avCorners[2]);
		SEOUL_UNITTESTING_ASSERT_EQUAL(Vector3D(-1, -1, -1), avCorners[3]);
		SEOUL_UNITTESTING_ASSERT_EQUAL(Vector3D(-1,  1,  1), avCorners[4]);
		SEOUL_UNITTESTING_ASSERT_EQUAL(Vector3D( 1,  1,  1), avCorners[5]);
		SEOUL_UNITTESTING_ASSERT_EQUAL(Vector3D( 1, -1,  1), avCorners[6]);
		SEOUL_UNITTESTING_ASSERT_EQUAL(Vector3D(-1, -1,  1), avCorners[7]);
	}
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
