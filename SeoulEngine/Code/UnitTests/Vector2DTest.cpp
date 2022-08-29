/**
 * \file Vector2DTest.cpp
 * \brief Unit test implementations for 2D vector class
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Logger.h"
#include "ReflectionDefine.h"
#include "UnitTesting.h"
#include "Vector2DTest.h"
#include "Vector2D.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(Vector2DTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestBasic)
	SEOUL_METHOD(TestOperators)
	SEOUL_METHOD(TestMiscMethods)
SEOUL_END_TYPE()

/**
 * Tests the basic functionality of the Vector2D class, such as the constructors,
 * getting and setting the components, and equality testing.
 *
 * Note: This uses exact floating point operations.  We're not doing any math
 * here, so this is OK.
 */
void Vector2DTest::TestBasic()
{
	Vector2D v0(0.0f, 0.0f),
			 v1(3.0f, 4.0f);
	const Vector2D& cv1 = v1;
	Vector2D v2(v1);
	Vector2D v3(3.001f, 4.001f);
	Vector2D v4;
	const Vector2D& cv4 = v4;

	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, v4.X);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, v4.Y);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, v4[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, v4[1]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, v4.GetData()[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, v4.GetData()[1]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, cv4[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, cv4[1]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, cv4.GetData()[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, cv4.GetData()[1]);

	SEOUL_UNITTESTING_ASSERT(v0.X == 0.0f && v0.Y == 0.0f);

	SEOUL_UNITTESTING_ASSERT(v1.X == 3.0f && v1.Y == 4.0f);
	SEOUL_UNITTESTING_ASSERT(v1[0] == 3.0f && v1[1] == 4.0f);
	SEOUL_UNITTESTING_ASSERT(v1.GetData()[0] == 3.0f && v1.GetData()[1] == 4.0f);
	SEOUL_UNITTESTING_ASSERT(cv1[0] == 3.0f && cv1[1] == 4.0f);
	SEOUL_UNITTESTING_ASSERT(cv1.GetData()[0] == 3.0f && cv1.GetData()[1] == 4.0f);

	SEOUL_UNITTESTING_ASSERT(v1 == v1);
	SEOUL_UNITTESTING_ASSERT(v1 == v2);
	SEOUL_UNITTESTING_ASSERT(v2 == v1);
	SEOUL_UNITTESTING_ASSERT(v0 != v1);
	SEOUL_UNITTESTING_ASSERT(v1 != v0);
	SEOUL_UNITTESTING_ASSERT(v1 != v3);
	SEOUL_UNITTESTING_ASSERT(!v1.Equals(v3, 0.0f));
	SEOUL_UNITTESTING_ASSERT(!v1.Equals(v3, 0.0009f));
	SEOUL_UNITTESTING_ASSERT(v1.Equals(v3, 0.0011f));

	v1[1] += 2.0f;

	SEOUL_UNITTESTING_ASSERT_EQUAL(6.0f, v1.Y);
	SEOUL_UNITTESTING_ASSERT(!(v1 == v2));
}

/**
 * Tests the functionality of all of the overloaded operators of Vector2D.
 *
 * Note: This uses exact floating point comparisons.  Since all of the numbers
 * chosen here are exactly representable in binary, there is no risk of epsilon
 * errors.  All comparisons should be exact.
 */
void Vector2DTest::TestOperators()
{
	Vector2D v0(1.0f, 2.0f),
		     v1(3.0f, 4.0f),
			 v2(4.0f, 6.0f),
			 v3(-2.0f, -2.0f),
			 v4(-1.0f, -2.0f),
			 v5(4.0f, 8.0f),
			 v6(0.25f, 0.5f);
	Vector2D v7(v0), v8;

	SEOUL_UNITTESTING_ASSERT(v0 + v1 == v2);
	SEOUL_UNITTESTING_ASSERT(v0 - v1 == v3);
	SEOUL_UNITTESTING_ASSERT(-v0 == v4);
	SEOUL_UNITTESTING_ASSERT(-v4 == v0);
	SEOUL_UNITTESTING_ASSERT(v1 - v0 == -v3);
	SEOUL_UNITTESTING_ASSERT(v0 * 4.0f == v5);
	SEOUL_UNITTESTING_ASSERT(4.0f * v0 == v5);
	SEOUL_UNITTESTING_ASSERT(v0 / 4.0f == v6);

	v0 = v1;
	SEOUL_UNITTESTING_ASSERT(v0 == v1);

	v0 = v7;
	v8 = (v0 += v1);
	SEOUL_UNITTESTING_ASSERT(v0 == v2);
	SEOUL_UNITTESTING_ASSERT(v0 == v8);

	v0 = v7;
	v8 = (v0 -= v1);
	SEOUL_UNITTESTING_ASSERT(v0 == v3);
	SEOUL_UNITTESTING_ASSERT(v0 == v8);

	v0 = v7;
	v8 = (v0 *= 4.0f);
	SEOUL_UNITTESTING_ASSERT(v0 == v5);
	SEOUL_UNITTESTING_ASSERT(v0 == v8);

	v0 = v7;
	v8 = (v0 /= 4.0f);
	SEOUL_UNITTESTING_ASSERT(v0 == v6);
	SEOUL_UNITTESTING_ASSERT(v0 == v8);
}

/**
 * Tests the miscellaneous methods of Vector2D: Length(), LengthSquared(),
 * Dot(), Cross(), IsZero(), and Normalize().
 *
 * This method contains some exact and some inexact floating-point math, so
 * exact assertions are used when possible.
 */
void Vector2DTest::TestMiscMethods()
{
	Vector2D v0(0.0f, 2.0f),
		     v1(2.0f, 3.0f),
			 v3(0.0f, 0.0f),
			 v4(1e-6f, -1e-6f),
			 v5(0.0f/2.0f, 2.0f/2.0f),
			 v6(-27.0f, -13.0f);

	// Static methods
	SEOUL_UNITTESTING_ASSERT_EQUAL(v4, Vector2D::Clamp(v3, v4, v4));
	SEOUL_UNITTESTING_ASSERT_EQUAL(v0, Vector2D::Clamp(v0, v6, v1));
	SEOUL_UNITTESTING_ASSERT_EQUAL(v3, Vector2D::ComponentwiseMultiply(v3, v4));
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Vector2D(-0.07407407407f, -0.23076923076f), Vector2D::ComponentwiseDivide(v1, v6), 1e-5f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(Vector2D(-54, -39), Vector2D::ComponentwiseMultiply(v1, v6));
	SEOUL_UNITTESTING_ASSERT_EQUAL(6.0f, Vector2D::Dot(v0, v1));
	SEOUL_UNITTESTING_ASSERT_EQUAL(6.0f, Vector2D::Dot(v1, v0));
	SEOUL_UNITTESTING_ASSERT(Vector2D::Cross(v0, v1) == -4.0f);
	SEOUL_UNITTESTING_ASSERT(Vector2D::Cross(v1, v0) == 4.0f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(Vector2D(2, 2), Vector2D::GramSchmidtProjectionOperator(Vector2D(1, 1), Vector2D(2, 2)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(Vector2D(1, 2.5f), Vector2D::Lerp(v0, v1, 0.5f));
	SEOUL_UNITTESTING_ASSERT_EQUAL(Vector2D(29, 39), Vector2D::Max(Vector2D(29, 15), Vector2D(-13, 39)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(Vector2D(39, 29), Vector2D::Max(Vector2D(15, 29), Vector2D(39, -13)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(Vector2D(-13, 15), Vector2D::Min(Vector2D(29, 15), Vector2D(-13, 39)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(Vector2D(15, -13), Vector2D::Min(Vector2D(15, 29), Vector2D(39, -13)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(Vector2D(0, 0), Vector2D::Normalize(Vector2D(0, 0)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(Vector2D(1, 0), Vector2D::Normalize(Vector2D(5, 0)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(Vector2D(0, 1), Vector2D::Normalize(Vector2D(0, 7)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(Vector2D(-1, 0), Vector2D::Perpendicular(Vector2D(0, -1)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(Vector2D(0, -1), Vector2D::Perpendicular(Vector2D(1, 0)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(Vector2D(-1, 1), Vector2D::Round(Vector2D(-0.5f, 0.5f)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(Vector2D(0, 0), Vector2D::Round(Vector2D(-0.49f, 0.4f)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(-1.0f, Vector2D::UnitCross(v0, v1));
	SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, Vector2D::UnitCross(v1, v0));

	SEOUL_UNITTESTING_ASSERT_EQUAL(27.0f, v6.Abs().X);
	SEOUL_UNITTESTING_ASSERT_EQUAL(13.0f, v6.Abs().Y);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(0.58800256f, v1.GetAngle(), 1e-6f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, v3.GetAngle());
	SEOUL_UNITTESTING_ASSERT_EQUAL(3.0f, v1.GetMaxComponent());
	SEOUL_UNITTESTING_ASSERT_EQUAL(2.0f, v1.GetMinComponent());
	SEOUL_UNITTESTING_ASSERT_EQUAL(-13.0f, v6.GetMaxComponent());
	SEOUL_UNITTESTING_ASSERT_EQUAL(-27.0f, v6.GetMinComponent());
	SEOUL_UNITTESTING_ASSERT_EQUAL(4.0f, v0.LengthSquared());
	SEOUL_UNITTESTING_ASSERT_EQUAL(2.0f, v0.Length());
	SEOUL_UNITTESTING_ASSERT(v3.IsZero());
	SEOUL_UNITTESTING_ASSERT(v3.IsZero(1e-30f));
	SEOUL_UNITTESTING_ASSERT(!v4.IsZero());
	SEOUL_UNITTESTING_ASSERT(v4.IsZero(1e-5f));
	SEOUL_UNITTESTING_ASSERT(v0.Normalize());
	SEOUL_UNITTESTING_ASSERT(v0.Equals(v5, 1e-5f));
	SEOUL_UNITTESTING_ASSERT(!v3.Normalize());
	SEOUL_UNITTESTING_ASSERT(!v4.Normalize(1e-11f));
	SEOUL_UNITTESTING_ASSERT(v4.Normalize(1e-12f));
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(1.0f, v0.Length(), 1e-5f);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(1.0f, v0.LengthSquared(), 1e-5f);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(1.0f, v4.Length(), 1e-5f);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(1.0f, v4.LengthSquared(), 1e-5f);

	// Constants
	SEOUL_UNITTESTING_ASSERT_EQUAL(Vector2D(1, 1), Vector2D::One());
	SEOUL_UNITTESTING_ASSERT_EQUAL(Vector2D(1, 0), Vector2D::UnitX());
	SEOUL_UNITTESTING_ASSERT_EQUAL(Vector2D(0, 1), Vector2D::UnitY());
	SEOUL_UNITTESTING_ASSERT_EQUAL(Vector2D(0, 0), Vector2D::Zero());
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
