/**
 * \file PlaneTest.cpp
 * \brief Unit test header file for the Plane struct.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Logger.h"
#include "Plane.h"
#include "PlaneTest.h"
#include "ReflectionDefine.h"
#include "UnitTesting.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(PlaneTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestBasic)
	SEOUL_METHOD(TestIntersectsAABB)
	SEOUL_METHOD(TestIntersectsPoint)
	SEOUL_METHOD(TestIntersectsSphere)
	SEOUL_METHOD(TestMiscMethods)
SEOUL_END_TYPE()

void PlaneTest::TestBasic()
{
	// Default test.
	{
		Plane plane;
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, plane.A);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, plane.B);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, plane.C);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, plane.D);
		SEOUL_UNITTESTING_ASSERT_EQUAL(Vector3D(0, 0, 0), plane.GetNormal());
	}

	// Create test.
	{
		Plane plane(Plane::Create(0, 1, 2, 5));
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, plane.A);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, plane.B);
		SEOUL_UNITTESTING_ASSERT_EQUAL(2, plane.C);
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, plane.D);
		SEOUL_UNITTESTING_ASSERT_EQUAL(Vector3D(0, 1, 2), plane.GetNormal());
	}

	// CreateFromPositionAndNormal test.
	{
		Vector3D const vNormal(Vector3D::Normalize(Vector3D(5, 4, 3)));
		Vector3D const vPosition(Vector3D(23, 15.3f, 123.2f));
		Plane plane(Plane::CreateFromPositionAndNormal(vPosition, vNormal));

		SEOUL_UNITTESTING_ASSERT_EQUAL(vNormal.X, plane.A);
		SEOUL_UNITTESTING_ASSERT_EQUAL(vNormal.Y, plane.B);
		SEOUL_UNITTESTING_ASSERT_EQUAL(vNormal.Z, plane.C);
		SEOUL_UNITTESTING_ASSERT_EQUAL(vNormal, plane.GetNormal());
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(-77.187776519f, plane.D, 1e-5f);
	}

	// CreateFromCorners tests.
	{
		// Free axis.
		{
			Vector3D const vP0(27, -5, 25);
			Vector3D const vP1(-42, 1, 31);
			Vector3D const vP2(-32, 223, 90);

			Vector4D const vExpected(-0.0613043f, 0.2589449f, -0.9639447f, 27.0485578f);

			Plane plane(Plane::CreateFromCorners(vP0, vP1, vP2));
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(vExpected.X, plane.A, 1e-5f);
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(vExpected.Y, plane.B, 1e-5f);
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(vExpected.Z, plane.C, 1e-5f);
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(vExpected.W, plane.D, 1e-5f);
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(vExpected.GetXYZ(), plane.GetNormal(), 1e-5f);
		}

		// XY plane.
		{
			Vector3D const vP0(0, 1, 0);
			Vector3D const vP1(0, 0, 0);
			Vector3D const vP2(1, 0, 0);

			Plane plane(Plane::CreateFromCorners(vP0, vP1, vP2));
			SEOUL_UNITTESTING_ASSERT_EQUAL(Vector3D::UnitZ().X, plane.A);
			SEOUL_UNITTESTING_ASSERT_EQUAL(Vector3D::UnitZ().Y, plane.B);
			SEOUL_UNITTESTING_ASSERT_EQUAL(Vector3D::UnitZ().Z, plane.C);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, plane.D);
			SEOUL_UNITTESTING_ASSERT_EQUAL(Vector3D::UnitZ(), plane.GetNormal());
		}

		// XZ plane.
		{
			Vector3D const vP0(1, 0, 0);
			Vector3D const vP1(0, 0, 0);
			Vector3D const vP2(0, 0, 1);

			Plane plane(Plane::CreateFromCorners(vP0, vP1, vP2));
			SEOUL_UNITTESTING_ASSERT_EQUAL(Vector3D::UnitY().X, plane.A);
			SEOUL_UNITTESTING_ASSERT_EQUAL(Vector3D::UnitY().Y, plane.B);
			SEOUL_UNITTESTING_ASSERT_EQUAL(Vector3D::UnitY().Z, plane.C);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, plane.D);
			SEOUL_UNITTESTING_ASSERT_EQUAL(Vector3D::UnitY(), plane.GetNormal());
		}

		// YZ plane.
		{
			Vector3D const vP0(0, 0, 1);
			Vector3D const vP1(0, 0, 0);
			Vector3D const vP2(0, 1, 0);

			Plane plane(Plane::CreateFromCorners(vP0, vP1, vP2));
			SEOUL_UNITTESTING_ASSERT_EQUAL(Vector3D::UnitX().X, plane.A);
			SEOUL_UNITTESTING_ASSERT_EQUAL(Vector3D::UnitX().Y, plane.B);
			SEOUL_UNITTESTING_ASSERT_EQUAL(Vector3D::UnitX().Z, plane.C);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, plane.D);
			SEOUL_UNITTESTING_ASSERT_EQUAL(Vector3D::UnitX(), plane.GetNormal());
		}
	}

	// Set test.
	{
		// Components
		{
			Plane plane;
			plane.Set(0, 1, 2, 5);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, plane.A);
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, plane.B);
			SEOUL_UNITTESTING_ASSERT_EQUAL(2, plane.C);
			SEOUL_UNITTESTING_ASSERT_EQUAL(5, plane.D);
			SEOUL_UNITTESTING_ASSERT_EQUAL(Vector3D(0, 1, 2), plane.GetNormal());
		}

		// Vector
		{
			Plane plane;
			plane.Set(Vector4D(0, 1, 2, 5));
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, plane.A);
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, plane.B);
			SEOUL_UNITTESTING_ASSERT_EQUAL(2, plane.C);
			SEOUL_UNITTESTING_ASSERT_EQUAL(5, plane.D);
			SEOUL_UNITTESTING_ASSERT_EQUAL(Vector3D(0, 1, 2), plane.GetNormal());
		}
	}

	// Equality test
	{
		Plane planeA;
		Plane planeB;
		SEOUL_UNITTESTING_ASSERT_EQUAL(planeA, planeB);

		planeA.Set(1, 2, 3, 4);
		planeB.Set(1, 2, 3, 4);
		SEOUL_UNITTESTING_ASSERT_EQUAL(planeA, planeB);
	}

	// Tolerance equality test
	{
		Plane planeA(Plane::Create(1, 2, 3, 4));
		Plane planeB(Plane::Create(1.0f + fEpsilon, 2.0f + fEpsilon, 3.0f + fEpsilon, 4.0f + fEpsilon));
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(planeA, planeB, fEpsilon);
	}

	// Inequality test
	{
		Plane planeA;
		Plane planeB(Plane::Create(1, 2, 3, 4));
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(planeA, planeB);

		planeA.Set(5, 6, 7, 8);
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(planeA, planeB);
	}
}

void PlaneTest::TestIntersectsAABB()
{
	// XY plane.
	{
		Vector3D const vP0(0, 1, 0);
		Vector3D const vP1(0, 0, 0);
		Vector3D const vP2(1, 0, 0);

		Plane plane(Plane::CreateFromCorners(vP0, vP1, vP2));

		// Front
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			PlaneTestResult::kFront,
			plane.Intersects(AABB::CreateFromMinAndMax(
				Vector3D::Zero(),
				Vector3D::One())));

		// Back
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			PlaneTestResult::kBack,
			plane.Intersects(AABB::CreateFromMinAndMax(
				-Vector3D::One(),
				Vector3D::Zero())));

		// Intersects
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			PlaneTestResult::kIntersects,
			plane.Intersects(AABB::CreateFromMinAndMax(
				-Vector3D::One(),
				Vector3D::One())));
	}

	// XZ plane.
	{
		Vector3D const vP0(0, 1, 0);
		Vector3D const vP1(0, 0, 0);
		Vector3D const vP2(1, 0, 0);

		Plane plane(Plane::CreateFromCorners(vP0, vP1, vP2));

		// Front
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			PlaneTestResult::kFront,
			plane.Intersects(AABB::CreateFromMinAndMax(
				Vector3D::Zero(),
				Vector3D::One())));

		// Back
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			PlaneTestResult::kBack,
			plane.Intersects(AABB::CreateFromMinAndMax(
				-Vector3D::One(),
				Vector3D::Zero())));

		// Intersects
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			PlaneTestResult::kIntersects,
			plane.Intersects(AABB::CreateFromMinAndMax(
				-Vector3D::One(),
				Vector3D::One())));
	}

	// YZ plane.
	{
		Vector3D const vP0(0, 0, 1);
		Vector3D const vP1(0, 0, 0);
		Vector3D const vP2(0, 1, 0);

		Plane plane(Plane::CreateFromCorners(vP0, vP1, vP2));

		// Front
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			PlaneTestResult::kFront,
			plane.Intersects(AABB::CreateFromMinAndMax(
				Vector3D::Zero(),
				Vector3D::One())));

		// Back
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			PlaneTestResult::kBack,
			plane.Intersects(AABB::CreateFromMinAndMax(
				-Vector3D::One(),
				Vector3D::Zero())));

		// Intersects
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			PlaneTestResult::kIntersects,
			plane.Intersects(AABB::CreateFromMinAndMax(
				-Vector3D::One(),
				Vector3D::One())));
	}
}

void PlaneTest::TestIntersectsPoint()
{
	// XY plane.
	{
		Vector3D const vP0(0, 1, 0);
		Vector3D const vP1(0, 0, 0);
		Vector3D const vP2(1, 0, 0);

		Plane plane(Plane::CreateFromCorners(vP0, vP1, vP2));

		// Front
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			PlaneTestResult::kFront,
			plane.Intersects(Vector3D(fEpsilon + fEpsilon)));

		// Back
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			PlaneTestResult::kBack,
			plane.Intersects(Vector3D(-fEpsilon - fEpsilon)));

		// Intersects
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			PlaneTestResult::kIntersects,
			plane.Intersects(Vector3D::Zero()));
	}

	// XZ plane.
	{
		Vector3D const vP0(0, 1, 0);
		Vector3D const vP1(0, 0, 0);
		Vector3D const vP2(1, 0, 0);

		Plane plane(Plane::CreateFromCorners(vP0, vP1, vP2));

		// Front
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			PlaneTestResult::kFront,
			plane.Intersects(Vector3D(fEpsilon + fEpsilon)));

		// Back
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			PlaneTestResult::kBack,
			plane.Intersects(Vector3D(-fEpsilon - fEpsilon)));

		// Intersects
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			PlaneTestResult::kIntersects,
			plane.Intersects(Vector3D::Zero()));
	}

	// YZ plane.
	{
		Vector3D const vP0(0, 0, 1);
		Vector3D const vP1(0, 0, 0);
		Vector3D const vP2(0, 1, 0);

		Plane plane(Plane::CreateFromCorners(vP0, vP1, vP2));

		// Front
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			PlaneTestResult::kFront,
			plane.Intersects(Vector3D(fEpsilon + fEpsilon)));

		// Back
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			PlaneTestResult::kBack,
			plane.Intersects(Vector3D(-fEpsilon - fEpsilon)));

		// Intersects
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			PlaneTestResult::kIntersects,
			plane.Intersects(Vector3D::Zero()));
	}
}

void PlaneTest::TestIntersectsSphere()
{
	// XY plane.
	{
		Vector3D const vP0(0, 1, 0);
		Vector3D const vP1(0, 0, 0);
		Vector3D const vP2(1, 0, 0);

		Plane plane(Plane::CreateFromCorners(vP0, vP1, vP2));

		// Front
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			PlaneTestResult::kFront,
			plane.Intersects(Sphere(Vector3D(0.5f, 0.5f, 0.5f), 0.5f)));

		// Back
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			PlaneTestResult::kBack,
			plane.Intersects(Sphere(Vector3D(-0.5f, -0.5f, -0.5f), 0.5f)));

		// Intersects
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			PlaneTestResult::kIntersects,
			plane.Intersects(Sphere(Vector3D(0.0f, 0.0f, 0.0f), 0.5f)));
	}

	// XZ plane.
	{
		Vector3D const vP0(0, 1, 0);
		Vector3D const vP1(0, 0, 0);
		Vector3D const vP2(1, 0, 0);

		Plane plane(Plane::CreateFromCorners(vP0, vP1, vP2));

		// Front
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			PlaneTestResult::kFront,
			plane.Intersects(Sphere(Vector3D(0.5f, 0.5f, 0.5f), 0.5f)));

		// Back
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			PlaneTestResult::kBack,
			plane.Intersects(Sphere(Vector3D(-0.5f, -0.5f, -0.5f), 0.5f)));

		// Intersects
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			PlaneTestResult::kIntersects,
			plane.Intersects(Sphere(Vector3D(0.0f, 0.0f, 0.0f), 0.5f)));
	}

	// YZ plane.
	{
		Vector3D const vP0(0, 0, 1);
		Vector3D const vP1(0, 0, 0);
		Vector3D const vP2(0, 1, 0);

		Plane plane(Plane::CreateFromCorners(vP0, vP1, vP2));

		// Front
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			PlaneTestResult::kFront,
			plane.Intersects(Sphere(Vector3D(0.5f, 0.5f, 0.5f), 0.5f)));

		// Back
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			PlaneTestResult::kBack,
			plane.Intersects(Sphere(Vector3D(-0.5f, -0.5f, -0.5f), 0.5f)));

		// Intersects
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			PlaneTestResult::kIntersects,
			plane.Intersects(Sphere(Vector3D(0.0f, 0.0f, 0.0f), 0.5f)));
	}
}

void PlaneTest::TestMiscMethods()
{
	// DotCoordinate
	{
		// XY plane.
		{
			Vector3D const vP0(0, 1, 5);
			Vector3D const vP1(0, 0, 5);
			Vector3D const vP2(1, 0, 5);

			Plane plane(Plane::CreateFromCorners(vP0, vP1, vP2));
			SEOUL_UNITTESTING_ASSERT_EQUAL(22.0f, plane.DotCoordinate(Vector3D(0, 0, 27)));
			SEOUL_UNITTESTING_ASSERT_EQUAL(-40.0f, plane.DotCoordinate(Vector3D(0, 0, -35)));
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, plane.DotCoordinate(Vector3D(25, 98, 5)));
		}

		// XZ plane.
		{
			Vector3D const vP0(1, 5, 0);
			Vector3D const vP1(0, 5, 0);
			Vector3D const vP2(0, 5, 1);

			Plane plane(Plane::CreateFromCorners(vP0, vP1, vP2));
			SEOUL_UNITTESTING_ASSERT_EQUAL(22.0f, plane.DotCoordinate(Vector3D(0, 27, 0)));
			SEOUL_UNITTESTING_ASSERT_EQUAL(-40.0f, plane.DotCoordinate(Vector3D(0, -35, 0)));
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, plane.DotCoordinate(Vector3D(25, 5, 98)));
		}

		// YZ plane.
		{
			Vector3D const vP0(5, 0, 1);
			Vector3D const vP1(5, 0, 0);
			Vector3D const vP2(5, 1, 0);

			Plane plane(Plane::CreateFromCorners(vP0, vP1, vP2));
			SEOUL_UNITTESTING_ASSERT_EQUAL(22.0f, plane.DotCoordinate(Vector3D(27, 0, 0)));
			SEOUL_UNITTESTING_ASSERT_EQUAL(-40.0f, plane.DotCoordinate(Vector3D(-35, 0, 0)));
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, plane.DotCoordinate(Vector3D(5, 25, 98)));
		}
	}

	// DotNormal
	{
		// XY plane.
		{
			Vector3D const vP0(0, 1, 0);
			Vector3D const vP1(0, 0, 0);
			Vector3D const vP2(1, 0, 0);

			Plane plane(Plane::CreateFromCorners(vP0, vP1, vP2));
			SEOUL_UNITTESTING_ASSERT_EQUAL(27.0f, plane.DotNormal(Vector3D(0, 0, 27)));
			SEOUL_UNITTESTING_ASSERT_EQUAL(-35.0f, plane.DotNormal(Vector3D(0, 0, -35)));
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, plane.DotNormal(Vector3D(25, 98, 0)));
		}

		// XZ plane.
		{
			Vector3D const vP0(1, 0, 0);
			Vector3D const vP1(0, 0, 0);
			Vector3D const vP2(0, 0, 1);

			Plane plane(Plane::CreateFromCorners(vP0, vP1, vP2));
			SEOUL_UNITTESTING_ASSERT_EQUAL(27.0f, plane.DotNormal(Vector3D(0, 27, 0)));
			SEOUL_UNITTESTING_ASSERT_EQUAL(-35.0f, plane.DotNormal(Vector3D(0, -35, 0)));
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, plane.DotNormal(Vector3D(25, 0, 98)));
		}

		// YZ plane.
		{
			Vector3D const vP0(0, 0, 1);
			Vector3D const vP1(0, 0, 0);
			Vector3D const vP2(0, 1, 0);

			Plane plane(Plane::CreateFromCorners(vP0, vP1, vP2));
			SEOUL_UNITTESTING_ASSERT_EQUAL(27.0f, plane.DotNormal(Vector3D(27, 0, 0)));
			SEOUL_UNITTESTING_ASSERT_EQUAL(-35.0f, plane.DotNormal(Vector3D(-35, 0, 0)));
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, plane.DotNormal(Vector3D(0, 25, 98)));
		}
	}

	// ProjectOnto
	{
		// XY plane.
		{
			Vector3D const vP0(0, 1, 0);
			Vector3D const vP1(0, 0, 0);
			Vector3D const vP2(1, 0, 0);

			Plane plane(Plane::CreateFromCorners(vP0, vP1, vP2));
			SEOUL_UNITTESTING_ASSERT_EQUAL(Vector3D::Zero(), plane.ProjectOnto(Vector3D(0, 0, 27)));
			SEOUL_UNITTESTING_ASSERT_EQUAL(Vector3D::Zero(), plane.ProjectOnto(Vector3D(0, 0, -35)));
			SEOUL_UNITTESTING_ASSERT_EQUAL(Vector3D(25, 98, 0), plane.ProjectOnto(Vector3D(25, 98, 0)));
			SEOUL_UNITTESTING_ASSERT_EQUAL(Vector3D(25, 98, 0), plane.ProjectOnto(Vector3D(25, 98, 93)));
			SEOUL_UNITTESTING_ASSERT_EQUAL(Vector3D(25, 98, 0), plane.ProjectOnto(Vector3D(25, 98, -93)));
		}

		// XZ plane.
		{
			Vector3D const vP0(1, 0, 0);
			Vector3D const vP1(0, 0, 0);
			Vector3D const vP2(0, 0, 1);

			Plane plane(Plane::CreateFromCorners(vP0, vP1, vP2));
			SEOUL_UNITTESTING_ASSERT_EQUAL(Vector3D::Zero(), plane.ProjectOnto(Vector3D(0, 27, 0)));
			SEOUL_UNITTESTING_ASSERT_EQUAL(Vector3D::Zero(), plane.ProjectOnto(Vector3D(0, -35, 0)));
			SEOUL_UNITTESTING_ASSERT_EQUAL(Vector3D(25, 0, 98), plane.ProjectOnto(Vector3D(25, 0, 98)));
			SEOUL_UNITTESTING_ASSERT_EQUAL(Vector3D(25, 0, 98), plane.ProjectOnto(Vector3D(25, 31, 98)));
			SEOUL_UNITTESTING_ASSERT_EQUAL(Vector3D(25, 0, 98), plane.ProjectOnto(Vector3D(25, -251, 98)));
		}

		// YZ plane.
		{
			Vector3D const vP0(0, 0, 1);
			Vector3D const vP1(0, 0, 0);
			Vector3D const vP2(0, 1, 0);

			Plane plane(Plane::CreateFromCorners(vP0, vP1, vP2));
			SEOUL_UNITTESTING_ASSERT_EQUAL(Vector3D::Zero(), plane.ProjectOnto(Vector3D(27, 0, 0)));
			SEOUL_UNITTESTING_ASSERT_EQUAL(Vector3D::Zero(), plane.ProjectOnto(Vector3D(-35, 0, 0)));
			SEOUL_UNITTESTING_ASSERT_EQUAL(Vector3D(0, 25, 98), plane.ProjectOnto(Vector3D(0, 25, 98)));
			SEOUL_UNITTESTING_ASSERT_EQUAL(Vector3D(0, 25, 98), plane.ProjectOnto(Vector3D(98, 25, 98)));
			SEOUL_UNITTESTING_ASSERT_EQUAL(Vector3D(0, 25, 98), plane.ProjectOnto(Vector3D(-71, 25, 98)));
		}
	}
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
