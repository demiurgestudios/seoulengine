/**
 * \file AABBTest.cpp
 * \brief Unit test header file for the Seoul AABB struct
 *
 * This file contains the unit test implementations for the Seoul AABB struct
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "AABBTest.h"
#include "Geometry.h"
#include "Matrix4D.h"
#include "Logger.h"
#include "ReflectionDefine.h"
#include "UnitTesting.h"
#include <ctime>

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(AABBTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestBasic)
	SEOUL_METHOD(TestIntersect)
	SEOUL_METHOD(TestTransform)
	SEOUL_METHOD(TestUtilities)
SEOUL_END_TYPE()

// Helper functions
static const Int32 kiMax = 0x7FFF;
static Float GetSignedRand()
{
	return (Float)((Int32)GlobalRandom::UniformRandomUInt32n(kiMax+1) - (kiMax / 2));
}

static Float GetUnsignedRand()
{
	return (Float)((Int32)GlobalRandom::UniformRandomUInt32n(kiMax+1));
}

static AABB GetRandomAABB()
{
	const Vector3D min(GetSignedRand(), GetSignedRand(), GetSignedRand());
	const Vector3D max(
		min.X + GetUnsignedRand(),
		min.Y + GetUnsignedRand(),
		min.Z + GetUnsignedRand());

	return AABB::CreateFromMinAndMax(min, max);
}

static Bool TestExactlyEqual(const AABB& a, const AABB& b)
{
	return (
		a == b &&
		a.m_vMin == b.m_vMin &&
		a.m_vMax == b.m_vMax);
}
// /Helper functions

void AABBTest::TestBasic()
{
	// assignment and equality tests.
	AABB aabb1 = GetRandomAABB();
	AABB aabb2 = AABB::CreateFromMinAndMax(aabb1.m_vMin, aabb1.m_vMax);
	SEOUL_UNITTESTING_ASSERT(TestExactlyEqual(aabb1, aabb2));

	aabb2 = AABB::CreateFromCenterAndExtents(aabb1.GetCenter(), aabb1.GetExtents());
	SEOUL_UNITTESTING_ASSERT(TestExactlyEqual(aabb1, aabb2));

	// basic query tests
	SEOUL_UNITTESTING_ASSERT(aabb1.GetDimensions() == (aabb1.m_vMax - aabb1.m_vMin));
	SEOUL_UNITTESTING_ASSERT(aabb1.GetExtents() == 0.5f * aabb1.GetDimensions());
	SEOUL_UNITTESTING_ASSERT(aabb1.GetDiagonalLength() == aabb1.GetDimensions().Length());
	SEOUL_UNITTESTING_ASSERT(aabb1.GetCenter() == (0.5f * (aabb1.m_vMax + aabb1.m_vMin)));

	// complex query tests
	if (aabb1.GetSurfaceArea() >= fEpsilon)
	{
		SEOUL_UNITTESTING_ASSERT(
			::Seoul::Equals(aabb1.GetInverseSurfaceArea(), (1.0f / aabb1.GetSurfaceArea())));
	}

	// axis test
	Int iMaxAxis = (Int)aabb1.GetMaxAxis();
	Vector3D dimensions = aabb1.GetDimensions();
	SEOUL_UNITTESTING_ASSERT(
		dimensions[iMaxAxis] >= dimensions[(iMaxAxis + 1) % 3] &&
		dimensions[iMaxAxis] >= dimensions[(iMaxAxis + 2) % 3]);
}

void AABBTest::TestIntersect()
{
	// intersection test
	{
		AABB aabb1 = GetRandomAABB();
		AABB aabb2 = AABB::CreateFromMinAndMax(
			aabb1.GetCenter(),
			aabb1.GetCenter() + aabb1.GetDimensions());

		SEOUL_UNITTESTING_ASSERT(!aabb1.Contains(aabb2));
		SEOUL_UNITTESTING_ASSERT(aabb1.Intersects(aabb2));
	}

	// containment test
	{
		AABB aabb1 = GetRandomAABB();
		AABB aabb2 = GetRandomAABB();

		aabb1.AbsorbPoint(aabb2.m_vMin);
		aabb1.AbsorbPoint(aabb2.m_vMax);

		SEOUL_UNITTESTING_ASSERT(aabb1.Contains(aabb2));
		SEOUL_UNITTESTING_ASSERT(aabb1.Intersects(aabb2));
	}
}

void AABBTest::TestTransform()
{
	AABB aabb = GetRandomAABB();
	Matrix4D mTransform =
		Matrix4D::CreateRotationY(GetSignedRand()) *
		Matrix4D::CreateTranslation(GetSignedRand(), GetSignedRand(), GetSignedRand());

	Vector3D cornerBuffer[8];

	cornerBuffer[0] = Matrix4D::TransformPosition(mTransform, Vector3D(aabb.m_vMin.X, aabb.m_vMin.Y, aabb.m_vMin.Z));
	cornerBuffer[1] = Matrix4D::TransformPosition(mTransform, Vector3D(aabb.m_vMin.X, aabb.m_vMin.Y, aabb.m_vMax.Z));
	cornerBuffer[2] = Matrix4D::TransformPosition(mTransform, Vector3D(aabb.m_vMin.X, aabb.m_vMax.Y, aabb.m_vMin.Z));
	cornerBuffer[3] = Matrix4D::TransformPosition(mTransform, Vector3D(aabb.m_vMin.X, aabb.m_vMax.Y, aabb.m_vMax.Z));
	cornerBuffer[4] = Matrix4D::TransformPosition(mTransform, Vector3D(aabb.m_vMax.X, aabb.m_vMin.Y, aabb.m_vMin.Z));
	cornerBuffer[5] = Matrix4D::TransformPosition(mTransform, Vector3D(aabb.m_vMax.X, aabb.m_vMin.Y, aabb.m_vMax.Z));
	cornerBuffer[6] = Matrix4D::TransformPosition(mTransform, Vector3D(aabb.m_vMax.X, aabb.m_vMax.Y, aabb.m_vMin.Z));
	cornerBuffer[7] = Matrix4D::TransformPosition(mTransform, Vector3D(aabb.m_vMax.X, aabb.m_vMax.Y, aabb.m_vMax.Z));

	AABB transformedAABB = AABB::InverseMaxAABB();
	for (int i = 0; i < 8; i++)
	{
		transformedAABB.m_vMin = Vector3D::Min(transformedAABB.m_vMin, cornerBuffer[i]);
		transformedAABB.m_vMax = Vector3D::Max(transformedAABB.m_vMax, cornerBuffer[i]);
	}

	AABB testAABB = AABB::Transform(mTransform, aabb);
	SEOUL_UNITTESTING_ASSERT(
		testAABB.m_vMin.Equals(transformedAABB.m_vMin, 1e-2f) &&
		testAABB.m_vMax.Equals(transformedAABB.m_vMax, 1e-2f));
}

void AABBTest::TestUtilities()
{
	// Max and InverseMax
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			AABB::CreateFromMinAndMax(0.5f * Vector3D(FloatMax, FloatMax, FloatMax), 0.5f * Vector3D(-FloatMax, -FloatMax, -FloatMax)),
			AABB::InverseMaxAABB());

		SEOUL_UNITTESTING_ASSERT_EQUAL(
			AABB::CreateFromMinAndMax(0.5f * Vector3D(-FloatMax, -FloatMax, -FloatMax), 0.5f * Vector3D(FloatMax, FloatMax, FloatMax)),
			AABB::MaxAABB());
	}

	// Equal and not equal.
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(AABB(), AABB());
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(AABB(), AABB::MaxAABB());
	}

	// Tolerance equals.
	{
		SEOUL_UNITTESTING_ASSERT(Equals(AABB(), AABB()));
		SEOUL_UNITTESTING_ASSERT(AABB().Equals(AABB()));
	}

	// Effective radius.
	{
		auto const aabb(AABB::CreateFromMinAndMax(-Vector3D::One(), Vector3D::One()));

		SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, aabb.GetEffectiveRadius(Vector3D::UnitX()));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, aabb.GetEffectiveRadius(-Vector3D::UnitX()));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, aabb.GetEffectiveRadius(Vector3D::UnitY()));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, aabb.GetEffectiveRadius(-Vector3D::UnitY()));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, aabb.GetEffectiveRadius(Vector3D::UnitZ()));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, aabb.GetEffectiveRadius(-Vector3D::UnitZ()));

		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Vector3D(1, 1, 1).Length(), aabb.GetEffectiveRadius(Vector3D::Normalize(Vector3D(1, 1, 1))), 1e-4f);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Vector3D(1, 1, 1).Length(), aabb.GetEffectiveRadius(Vector3D::Normalize(Vector3D(-1, -1, -1))), 1e-4f);
	}

	// Expand.
	{
		AABB aabb;
		aabb.Expand(2.0f);
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			AABB::CreateFromMinAndMax(-Vector3D::One(), Vector3D::One()),
			aabb);
	}

	// IsHuge.
	{
		Float f = kfUnitTestMaxConstant;
		SEOUL_UNITTESTING_ASSERT(!AABB().IsHuge());
		SEOUL_UNITTESTING_ASSERT(AABB::CreateFromMinAndMax(-Vector3D(f),  Vector3D(f)).IsHuge());
		SEOUL_UNITTESTING_ASSERT(AABB::CreateFromMinAndMax( Vector3D(f), -Vector3D(f)).IsHuge());
	}

	// IsValid.
	{
		SEOUL_UNITTESTING_ASSERT(AABB().IsValid());
		SEOUL_UNITTESTING_ASSERT(AABB::MaxAABB().IsValid());
		SEOUL_UNITTESTING_ASSERT(!AABB::InverseMaxAABB().IsValid());
	}

	// CalculateMerged.
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(AABB(), AABB::CalculateMerged(AABB(), AABB()));
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			AABB::CreateFromCenterAndExtents(Vector3D::Zero(), Vector3D::One()),
			AABB::CalculateMerged(
				AABB::CreateFromMinAndMax(-Vector3D::One(), Vector3D::Zero()),
				AABB::CreateFromMinAndMax(Vector3D::Zero(), Vector3D::One())));
	}

	// CalculateFromSphere
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(AABB(), AABB::CalculateFromSphere(Sphere(Vector3D::Zero(), 0.0f)));
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			AABB::CreateFromCenterAndExtents(Vector3D::Zero(), Vector3D::One()),
			AABB::CalculateFromSphere(Sphere(Vector3D::Zero(), 1.0f)));
	}

	// Clamp
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(Vector3D::Zero(), AABB::Clamp(Vector3D(FloatMax), AABB()));

		SEOUL_UNITTESTING_ASSERT_EQUAL(Vector3D::One(), AABB::Clamp(Vector3D(FloatMax), AABB::CreateFromCenterAndExtents(Vector3D::Zero(), Vector3D::One())));
		SEOUL_UNITTESTING_ASSERT_EQUAL(-Vector3D::One(), AABB::Clamp(Vector3D(-FloatMax), AABB::CreateFromCenterAndExtents(Vector3D::Zero(), Vector3D::One())));
	}

	// Intersects
	{
		SEOUL_UNITTESTING_ASSERT(AABB().Intersects(AABB()));
		SEOUL_UNITTESTING_ASSERT(AABB().Intersects(Vector3D::Zero()));

		SEOUL_UNITTESTING_ASSERT(
			AABB::CreateFromCenterAndExtents(Vector3D::Zero(), Vector3D::One()).Intersects(
			AABB::CreateFromCenterAndExtents(Vector3D(2.0f), Vector3D::One())));
		SEOUL_UNITTESTING_ASSERT(
			AABB::CreateFromCenterAndExtents(Vector3D::Zero(), Vector3D::One()).Intersects(
			AABB::CreateFromCenterAndExtents(Vector3D(-2.0f), Vector3D::One())));

		SEOUL_UNITTESTING_ASSERT(
			!AABB::CreateFromCenterAndExtents(Vector3D::Zero(), Vector3D::One()).Intersects(
			AABB::CreateFromCenterAndExtents(Vector3D(2.0f + 1e-6f), Vector3D::One())));
		SEOUL_UNITTESTING_ASSERT(
			!AABB::CreateFromCenterAndExtents(Vector3D::Zero(), Vector3D::One()).Intersects(
			AABB::CreateFromCenterAndExtents(Vector3D(-2.0f - 1e-6f), Vector3D::One())));
	}

	// CalculateFromAABBs
	{
		FixedArray<AABB, 2u> a;
		SEOUL_UNITTESTING_ASSERT_EQUAL(AABB(), AABB::CalculateFromAABBs(a.Begin(), a.End()));

		a[0] = AABB::CreateFromMinAndMax(-Vector3D::One(), Vector3D::Zero());
		a[1] = AABB::CreateFromMinAndMax(Vector3D::Zero(), Vector3D::One());
		SEOUL_UNITTESTING_ASSERT_EQUAL(AABB::CreateFromCenterAndExtents(Vector3D::Zero(), Vector3D::One()), AABB::CalculateFromAABBs(a.Begin(), a.End()));
	}

	// CalculateFromPoints
	{
		FixedArray<Vector3D, 2u> a;
		SEOUL_UNITTESTING_ASSERT_EQUAL(AABB(), AABB::CalculateFromPoints(a.Begin(), a.End()));

		a[0] = Vector3D::One();
		a[1] = -Vector3D::One();
		SEOUL_UNITTESTING_ASSERT_EQUAL(AABB::CreateFromCenterAndExtents(Vector3D::Zero(), Vector3D::One()), AABB::CalculateFromPoints(a.Begin(), a.End()));
	}
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
