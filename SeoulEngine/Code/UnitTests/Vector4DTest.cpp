/**
 * \file Vector4DTest.cpp
 * \brief Unit test implementations for 4D vector class
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
#include "Vector4D.h"
#include "Vector4DTest.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(Vector4DTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestBasic)
	SEOUL_METHOD(TestOperators)
	SEOUL_METHOD(TestMiscMethods)
SEOUL_END_TYPE()

/**
 * Tests the basic functionality of the Vector4D class, such as the constructors,
 * getting and setting the components, and equality testing.
 *
 * Note: This uses exact floating point operations.  We're not doing any math
 * here, so this is OK.
 */
void Vector4DTest::TestBasic(void)
{
	Vector4D v0(0.0f, 0.0f, 0.0f, 0.0f),
			 v1(3.0f, 4.0f, 5.0f, 6.0f);
	const Vector4D& cv1 = v1;
	Vector4D v2(v1);
	Vector4D v3(3.001f, 4.001f, 4.999f, 5.999f);
	Vector4D v4;
	const Vector4D& cv4 = v4;

	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, v4.X);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, v4.Y);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, v4.Z);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, v4.W);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, v4[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, v4[1]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, v4[2]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, v4[3]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, v4.GetData()[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, v4.GetData()[1]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, v4.GetData()[2]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, v4.GetData()[3]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, cv4[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, cv4[1]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, cv4[2]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, cv4[3]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, cv4.GetData()[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, cv4.GetData()[1]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, cv4.GetData()[2]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, cv4.GetData()[3]);

	SEOUL_UNITTESTING_ASSERT(v0.X == 0.0f && v0.Y == 0.0f && v0.Z == 0.0f && v0.W == 0.0f);

	SEOUL_UNITTESTING_ASSERT(v1.X == 3.0f && v1.Y == 4.0f && v1.Z == 5.0f && v1.W == 6.0f);
	SEOUL_UNITTESTING_ASSERT(v1[0] == 3.0f && v1[1] == 4.0f && v1[2] == 5.0f && v1[3] == 6.0f);
	SEOUL_UNITTESTING_ASSERT(v1.GetData()[0] == 3.0f && v1.GetData()[1] == 4.0f && v1.GetData()[2] == 5.0f && v1.GetData()[3] == 6.0f);
	SEOUL_UNITTESTING_ASSERT(v1[0] == 3.0f && v1[1] == 4.0f && v1[2] == 5.0f && v1[3] == 6.0f);
	SEOUL_UNITTESTING_ASSERT(cv1.GetData()[0] == 3.0f && cv1.GetData()[1] == 4.0f && cv1.GetData()[2] == 5.0f && cv1.GetData()[3] == 6.0f);

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
 * Tests the functionality of all of the overloaded operators of Vector4D.
 *
 * Note: This uses exact floating point comparisons.  Since all of the numbers
 * chosen here are exactly representable in binary, there is no risk of epsilon
 * errors.  All comparisons should be exact.
 */
void Vector4DTest::TestOperators(void)
{
	Vector4D v0(1.0f, 2.0f, 3.0f, 4.0f),
		     v1(3.0f, 4.0f, 4.0f, 5.0f),
			 v2(4.0f, 6.0f, 7.0f, 9.0f),
			 v3(-2.0f, -2.0f, -1.0f, -1.0f),
			 v4(-1.0f, -2.0f, -3.0f, -4.0f),
			 v5(4.0f, 8.0f, 12.0f, 16.0f),
			 v6(0.25f, 0.5f, 0.75f, 1.0f);
	Vector4D v7(v0), v8;

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
 * Tests the miscellaneous methods of Vector4D: Length(), LengthSquared(),
 * Dot(), Cross(), IsZero(), and Normalize().
 *
 * This method contains some exact and some inexact floating-point math, so
 * exact assertions are used when possible.
 */
void Vector4DTest::TestMiscMethods(void)
{
	Vector4D v0(1.0f, 2.0f, 2.0f, -4.0f),
		     v1(2.0f, 3.0f, 4.0f, 5.0f),
			 v2(0.0f, 0.0f, 0.0f, 0.0f),
			 v3(1e-6f, -1e-6f, 1e-6f, -1e-6f),
			 v4(1.0f/5.0f, 2.0f/5.0f, 2.0f/5.0f, -4.0f/5.0f),
			 v6(-27.0f, -13.0f, -4.0f, -7.0f);

	// Static methods
	SEOUL_UNITTESTING_ASSERT_EQUAL(v3, Vector4D::Clamp(v2, v3, v3));
	SEOUL_UNITTESTING_ASSERT_EQUAL(v0, Vector4D::Clamp(v0, v6, v1));
	SEOUL_UNITTESTING_ASSERT_EQUAL(v2, Vector4D::ComponentwiseMultiply(v2, v3));
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Vector4D(-0.07407407407f, -0.23076923076f, -1.0f, -0.71428571428f), Vector4D::ComponentwiseDivide(v1, v6), 1e-5f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(Vector4D(-54, -39, -16, -35), Vector4D::ComponentwiseMultiply(v1, v6));
	SEOUL_UNITTESTING_ASSERT_EQUAL(-4.0f, Vector4D::Dot(v0, v1));
	SEOUL_UNITTESTING_ASSERT_EQUAL(-4.0f, Vector4D::Dot(v1, v0));

	SEOUL_UNITTESTING_ASSERT_EQUAL(Vector4D(1.5f, 2.5f, 3.0f, 0.5f), Vector4D::Lerp(v0, v1, 0.5f));
	SEOUL_UNITTESTING_ASSERT_EQUAL(Vector4D(29, 39, 45, 63), Vector4D::Max(Vector4D(29, 15, 3, 1), Vector4D(-13, 39, 45, 63)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(Vector4D(63, 45, 29, 39), Vector4D::Max(Vector4D(1, 3, 29, 15), Vector4D(63, 45, -13, 39)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(Vector4D(39, 63, 45, 29), Vector4D::Max(Vector4D(15, 1, 3, 29), Vector4D(39, 63, 45, -13)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(Vector4D(29, 39, 63, 45), Vector4D::Max(Vector4D(29, 15, 1, 3), Vector4D(-13, 39, 63, 45)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(Vector4D(-13, 15, 3, 1), Vector4D::Min(Vector4D(29, 15, 3, 1), Vector4D(-13, 39, 45, 63)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(Vector4D(1, 3, -13, 15), Vector4D::Min(Vector4D(1, 3, 29, 15), Vector4D(63, 45, -13, 39)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(Vector4D(15, 1, 3, -13), Vector4D::Min(Vector4D(15, 1, 3, 29), Vector4D(39, 63, 45, -13)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(Vector4D(-13, 15, 1, 3), Vector4D::Min(Vector4D(29, 15, 1, 3), Vector4D(-13, 39, 63, 45)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(Vector4D(0, 0, 0, 0), Vector4D::Normalize(Vector4D(0, 0, 0, 0)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(Vector4D(1, 0, 0, 0), Vector4D::Normalize(Vector4D(5, 0, 0, 0)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(Vector4D(0, 1, 0, 0), Vector4D::Normalize(Vector4D(0, 7, 0, 0)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(Vector4D(0, 0, 1, 0), Vector4D::Normalize(Vector4D(0, 0, 59, 0)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(Vector4D(0, 0, 0, 1), Vector4D::Normalize(Vector4D(0, 0, 0, 71)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(Vector4D(-1, 1, -1, 1), Vector4D::Round(Vector4D(-0.5f, 0.5f, -0.5f, 0.5f)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(Vector4D(0, 0, 0, 0), Vector4D::Round(Vector4D(-0.49f, 0.4f, -0.49f, 0.4f)));

	SEOUL_UNITTESTING_ASSERT_EQUAL(27.0f, v6.Abs().X);
	SEOUL_UNITTESTING_ASSERT_EQUAL(13.0f, v6.Abs().Y);
	SEOUL_UNITTESTING_ASSERT_EQUAL(4.0f, v6.Abs().Z);
	SEOUL_UNITTESTING_ASSERT_EQUAL(7.0f, v6.Abs().W);
	SEOUL_UNITTESTING_ASSERT_EQUAL(5.0f, v1.GetMaxComponent());
	SEOUL_UNITTESTING_ASSERT_EQUAL(2.0f, v1.GetMinComponent());
	SEOUL_UNITTESTING_ASSERT_EQUAL(-4.0f, v6.GetMaxComponent());
	SEOUL_UNITTESTING_ASSERT_EQUAL(-27.0f, v6.GetMinComponent());
	SEOUL_UNITTESTING_ASSERT_EQUAL(-27.0f, v6.GetXY().X);
	SEOUL_UNITTESTING_ASSERT_EQUAL(-13.0f, v6.GetXY().Y);
	SEOUL_UNITTESTING_ASSERT_EQUAL(-4.0f, v6.GetZW().X);
	SEOUL_UNITTESTING_ASSERT_EQUAL(-7.0f, v6.GetZW().Y);
	SEOUL_UNITTESTING_ASSERT_EQUAL(-27.0f, v6.GetXYZ().X);
	SEOUL_UNITTESTING_ASSERT_EQUAL(-13.0f, v6.GetXYZ().Y);
	SEOUL_UNITTESTING_ASSERT_EQUAL(-4.0f, v6.GetXYZ().Z);
	SEOUL_UNITTESTING_ASSERT_EQUAL(25.0f, v0.LengthSquared());
	SEOUL_UNITTESTING_ASSERT_EQUAL(5.0f, v0.Length());
	SEOUL_UNITTESTING_ASSERT(v2.IsZero());
	SEOUL_UNITTESTING_ASSERT(v2.IsZero(1e-30f));
	SEOUL_UNITTESTING_ASSERT(!v3.IsZero());
	SEOUL_UNITTESTING_ASSERT(v3.IsZero(1e-5f));
	SEOUL_UNITTESTING_ASSERT(v0.Normalize());
	SEOUL_UNITTESTING_ASSERT(v0.Equals(v4, 1e-5f));
	SEOUL_UNITTESTING_ASSERT(!v2.Normalize());
	SEOUL_UNITTESTING_ASSERT(!v3.Normalize(1e-11f));
	SEOUL_UNITTESTING_ASSERT(v3.Normalize(1e-12f));
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(1.0f, v0.Length(), 1e-5f);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(1.0f, v0.LengthSquared(), 1e-5f);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(1.0f, v3.Length(), 1e-5f);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(1.0f, v3.LengthSquared(), 1e-5f);

	// Constants
	SEOUL_UNITTESTING_ASSERT_EQUAL(Vector4D(1, 1, 1, 1), Vector4D::One());
	SEOUL_UNITTESTING_ASSERT_EQUAL(Vector4D(1, 0, 0, 0), Vector4D::UnitX());
	SEOUL_UNITTESTING_ASSERT_EQUAL(Vector4D(0, 1, 0, 0), Vector4D::UnitY());
	SEOUL_UNITTESTING_ASSERT_EQUAL(Vector4D(0, 0, 1, 0), Vector4D::UnitZ());
	SEOUL_UNITTESTING_ASSERT_EQUAL(Vector4D(0, 0, 0, 1), Vector4D::UnitW());
	SEOUL_UNITTESTING_ASSERT_EQUAL(Vector4D(0, 0, 0, 0), Vector4D::Zero());
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
