/**
 * \file QuaternionTest.cpp
 * \brief Unit tests for the Quaternion class. Quaternions are
 * used to represent 3D rotations in a way that does not
 * suffer from gimbal lock.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Logger.h"
#include "Matrix3D.h"
#include "Matrix3x4.h"
#include "Matrix4D.h"
#include "ReflectionDefine.h"
#include "Quaternion.h"
#include "QuaternionTest.h"
#include "UnitTesting.h"
#include "Vector3D.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(QuaternionTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestMethods)
	SEOUL_METHOD(TestToMatrix)
	SEOUL_METHOD(TestFromMatrix)
	SEOUL_METHOD(TestTransformation)
	SEOUL_METHOD(TestUtilities)
	SEOUL_METHOD(TestTransformationRegressions)
SEOUL_END_TYPE();

/**
 * Test most of the member methods of the Quaternion class.
 *
 * This test excludes matrix conversion and transformation methods.
 */
void QuaternionTest::TestMethods()
{
	// identity
	{
		SEOUL_UNITTESTING_ASSERT(
			1.0f == Quaternion::Identity().W &&
			0.0f == Quaternion::Identity().X &&
			0.0f == Quaternion::Identity().Y &&
			0.0f == Quaternion::Identity().Z);
	}

	// tolerant equality
	{
		Quaternion q0(3.0f, 4.0f, 5.0f, 6.0f);
		Quaternion q1(3.000999f, 4.000999f, 5.000999f, 6.000999f);

		SEOUL_UNITTESTING_ASSERT(!q0.Equals(q1, 1e-4f));
		SEOUL_UNITTESTING_ASSERT(q0.Equals(q1, 1e-3f));
	}

	// tolerant zero
	{
		Quaternion q(0.000999f, 0.000999f, 0.000999f, 0.000999f);

		SEOUL_UNITTESTING_ASSERT(!q.IsZero(1e-4f));
		SEOUL_UNITTESTING_ASSERT(q.IsZero(1e-3f));
	}

	// componentwise constructor
	{
		Quaternion q(4.0f, 5.0f, 6.0f, 3.0f);
		SEOUL_UNITTESTING_ASSERT(3.0f == q.W && 4.0f == q.X && 5.0f == q.Y && 6.0f == q.Z);
	}

	// Vector3D and w constructor
	{
		Quaternion q(Vector3D(4.0f, 5.0f, 6.0f), 3.0f);
		SEOUL_UNITTESTING_ASSERT(3.0f == q.W && 4.0f == q.X && 5.0f == q.Y && 6.0f == q.Z);
	}

	// copy constructor
	{
		Quaternion q(Quaternion(4.0f, 5.0f, 6.0f, 3.0f));
		SEOUL_UNITTESTING_ASSERT(3.0f == q.W && 4.0f == q.X && 5.0f == q.Y && 6.0f == q.Z);
	}

	// assignment
	{
		Quaternion q = Quaternion(4.0f, 5.0f, 6.0, 3.0f);
		SEOUL_UNITTESTING_ASSERT(3.0f == q.W && 4.0f == q.X && 5.0f == q.Y && 6.0f == q.Z);
	}

	// self-addition
	{
		Quaternion q0(4.0f, 5.0f, 6.0f, 3.0f);

		{
			Quaternion q = (q0 + q0);
			SEOUL_UNITTESTING_ASSERT(6.0f == q.W && 8.0f == q.X && 10.0f == q.Y && 12.0f == q.Z);
		}

		{
			Quaternion q = q0;
			q += q;
			SEOUL_UNITTESTING_ASSERT(6.0f == q.W && 8.0f == q.X && 10.0f == q.Y && 12.0f == q.Z);
		}
	}

	// addition
	{
		Quaternion q0(4.0f, 5.0f, 6.0f, 3.0f);
		Quaternion q1(-5.0f, -6.0f, -7.0f, -4.0f);

		{
			Quaternion q = (q0 + q1);
			SEOUL_UNITTESTING_ASSERT(-1.0f == q.W && -1.0f == q.X && -1.0f == q.Y && -1.0f == q.Z);
		}

		{
			Quaternion q = q0;
			q += q1;
			SEOUL_UNITTESTING_ASSERT(-1.0f == q.W && -1.0f == q.X && -1.0f == q.Y && -1.0f == q.Z);
		}
	}

	// self-subtraction
	{
		Quaternion q0(4.0f, 5.0f, 6.0f, 3.0f);

		{
			Quaternion q = (q0 - q0);
			SEOUL_UNITTESTING_ASSERT(0.0f == q.W && 0.0f == q.X && 0.0f == q.Y && 0.0f == q.Z);
		}

		{
			Quaternion q = q0;
			q -= q;
			SEOUL_UNITTESTING_ASSERT(0.0f == q.W && 0.0f == q.X && 0.0f == q.Y && 0.0f == q.Z);
		}
	}

	// subtraction
	{
		Quaternion q0(4.0f, 5.0f, 6.0f, 3.0f);
		Quaternion q1(5.0f, 6.0f, 7.0f, 4.0f);

		{
			Quaternion q = (q0 - q1);
			SEOUL_UNITTESTING_ASSERT(-1.0f == q.W && -1.0f == q.X && -1.0f == q.Y && -1.0f == q.Z);
		}

		{
			Quaternion q = q0;
			q -= q1;
			SEOUL_UNITTESTING_ASSERT(-1.0f == q.W && -1.0f == q.X && -1.0f == q.Y && -1.0f == q.Z);
		}
	}

	// negation
	{
		Quaternion q0(4.0f, 5.0f, 6.0f, 3.0f);
		Quaternion q = (-q0);

		SEOUL_UNITTESTING_ASSERT(-3.0f == q.W && -4.0f == q.X && -5.0f == q.Y && -6.0f == q.Z);
	}

	// quaternion self-multiplication
	{
		Quaternion q0(4.0f, 5.0f, 6.0f, 3.0f);

		{
			Quaternion q = (q0 * q0);
			// W should equal: (W * W) - (X * X) - (Y * Y) - (Z * Z)
			// X should equal: (W * X) + (X * W) + (Y * Z) - (Z * Y)
			// Y should equal: (W * Y) + (Y * W) + (Z * X) - (X * Z)
			// Z should equal: (W * Z) + (Z * W) + (X * Y) - (Y * X)
			SEOUL_UNITTESTING_ASSERT(-68.0f == q.W && 24.0f == q.X && 30.0f == q.Y && 36.0f == q.Z);
		}

		{
			Quaternion q = q0;
			q *= q;
			// W should equal: (W * W) - (X * X) - (Y * Y) - (Z * Z)
			// X should equal: (W * X) + (X * W) + (Y * Z) - (Z * Y)
			// Y should equal: (W * Y) + (Y * W) + (Z * X) - (X * Z)
			// Z should equal: (W * Z) + (Z * W) + (X * Y) - (Y * X)
			SEOUL_UNITTESTING_ASSERT(-68.0f == q.W && 24.0f == q.X && 30.0f == q.Y && 36.0f == q.Z);
		}
	}

	// quaternion multiplication
	{
		Quaternion q0(4.0f, 5.0f, 6.0f, 3.0f);
		Quaternion q1(5.0f, 6.0f, 7.0f, 4.0f);

		{
			Quaternion q = (q0 * q1);

			// W should equal: (q0.W * q1.W) - (q0.X * q1.X) - (q0.Y * q1.Y) - (q0.Z * q1.Z)
			// X should equal: (q0.W * q1.X) + (q0.X * q1.W) + (q0.Y * q1.Z) - (q0.Z * q1.Y)
			// Y should equal: (q0.W * q1.Y) + (q0.Y * q1.W) + (q0.Z * q1.X) - (q0.X * q1.Z)
			// Z should equal: (q0.W * q1.Z) + (q0.Z * q1.W) + (q0.X * q1.Y) - (q0.Y * q1.X)
			SEOUL_UNITTESTING_ASSERT(-80.0f == q.W && 30.0f == q.X && 40.0f == q.Y && 44.0f == q.Z);
		}

		{
			Quaternion q = q0;
			q *= q1;

			// W should equal: (q0.W * q1.W) - (q0.X * q1.X) - (q0.Y * q1.Y) - (q0.Z * q1.Z)
			// X should equal: (q0.W * q1.X) + (q0.X * q1.W) + (q0.Y * q1.Z) - (q0.Z * q1.Y)
			// Y should equal: (q0.W * q1.Y) + (q0.Y * q1.W) + (q0.Z * q1.X) - (q0.X * q1.Z)
			// Z should equal: (q0.W * q1.Z) + (q0.Z * q1.W) + (q0.X * q1.Y) - (q0.Y * q1.X)
			SEOUL_UNITTESTING_ASSERT(-80.0f == q.W && 30.0f == q.X && 40.0f == q.Y && 44.0f == q.Z);
		}

		{
			Quaternion q = (q1 * q0);

			// W should equal: (q1.W * q0.W) - (q1.X * q0.X) - (q1.Y * q0.Y) - (q1.Z * q0.Z)
			// X should equal: (q1.W * q0.X) + (q1.X * q0.W) + (q1.Y * q0.Z) - (q1.Z * q0.Y)
			// Y should equal: (q1.W * q0.Y) + (q1.Y * q0.W) + (q1.Z * q0.X) - (q1.X * q0.Z)
			// Z should equal: (q1.W * q0.Z) + (q1.Z * q0.W) + (q1.X * q0.Y) - (q1.Y * q0.X)
			SEOUL_UNITTESTING_ASSERT(-80.0f == q.W && 32.0f == q.X && 36.0f == q.Y && 46.0f == q.Z);
		}

		{
			Quaternion q = q1;
			q *= q0;

			// W should equal: (q1.W * q0.W) - (q1.X * q0.X) - (q1.Y * q0.Y) - (q1.Z * q0.Z)
			// X should equal: (q1.W * q0.X) + (q1.X * q0.W) + (q1.Y * q0.Z) - (q1.Z * q0.Y)
			// Y should equal: (q1.W * q0.Y) + (q1.Y * q0.W) + (q1.Z * q0.X) - (q1.X * q0.Z)
			// Z should equal: (q1.W * q0.Z) + (q1.Z * q0.W) + (q1.X * q0.Y) - (q1.Y * q0.X)
			SEOUL_UNITTESTING_ASSERT(-80.0f == q.W && 32.0f == q.X && 36.0f == q.Y && 46.0f == q.Z);
		}
	}

	// scalar multiplication
	{
		Quaternion q0(4.0f, 5.0f, 6.0, 3.0f);

		{
			Quaternion q = (q0 * 3.0f);
			SEOUL_UNITTESTING_ASSERT(9.0f == q.W && 12.0f == q.X && 15.0f == q.Y && 18.0f == q.Z);
		}

		{
			Quaternion q = (3.0f * q0);
			SEOUL_UNITTESTING_ASSERT(9.0f == q.W && 12.0f == q.X && 15.0f == q.Y && 18.0f == q.Z);
		}

		{
			Quaternion q = q0;
			q *= 3.0f;
			SEOUL_UNITTESTING_ASSERT(9.0f == q.W && 12.0f == q.X && 15.0f == q.Y && 18.0f == q.Z);
		}
	}

	// scalar division
	{
		Quaternion q0(9.0f, 12.0f, 15.0, 3.0f);

		{
			Quaternion q = (q0 / 3.0f);
			SEOUL_UNITTESTING_ASSERT(1.0f == q.W && 3.0f == q.X && 4.0f == q.Y && 5.0f == q.Z);
		}

		{
			Quaternion q = q0;
			q /= 3.0f;
			SEOUL_UNITTESTING_ASSERT(1.0f == q.W && 3.0f == q.X && 4.0f == q.Y && 5.0f == q.Z);
		}
	}

	// self-equality
	{
		Quaternion q(3.0f, 4.0f, 5.0f, 6.0f);
		SEOUL_UNITTESTING_ASSERT(q == q);
	}

	// equality
	{
		Quaternion q0(3.0f, 4.0f, 5.0f, 6.0f);
		Quaternion q1(3.0f, 4.0f, 5.0f, 6.0f);
		SEOUL_UNITTESTING_ASSERT(q0 == q1);
	}

	// inequality
	{
		Quaternion q0(3.0f, 4.0f, 5.0f, 6.0f);
		Quaternion q1(4.0f, 5.0f, 6.0f, 7.0f);
		SEOUL_UNITTESTING_ASSERT(q0 != q1);
	}

	// conjugate
	{
		{
			Quaternion q0(4.0f, 5.0f, 6.0f, 3.0f);
			Quaternion q = q0.Conjugate();

			// Conjugate should be equal to -q0.X, -q0.Y, -q0.Z, q0.W
			SEOUL_UNITTESTING_ASSERT(3.0f == q.W && -4.0f == q.X && -5.0f == q.Y && -6.0f == q.Z);
		}

		{
			Quaternion q0(4.0f, 5.0f, 6.0f, 3.0f);
			Quaternion q = Quaternion::Conjugate(q0);

			// Conjugate should be equal to -q0.X, -q0.Y, -q0.Z, q0.W
			SEOUL_UNITTESTING_ASSERT(3.0f == q.W && -4.0f == q.X && -5.0f == q.Y && -6.0f == q.Z);
		}
	}

	// dot product
	{
		Quaternion q(3.0f, 4.0f, 5.0f, 6.0f);

		// Should equal to (q.W * q.W + q.X * q.X + q.Y * q.Y + q.Z * q.Z)
		SEOUL_UNITTESTING_ASSERT(86.0f == Quaternion::Dot(q, q));
	}

	// length squared
	{
		Quaternion q(3.0f, 4.0f, 5.0f, 6.0f);

		// Should equal to (q.W * q.W + q.X * q.X + q.Y * q.Y + q.Z * q.Z)
		SEOUL_UNITTESTING_ASSERT(86.0f == q.LengthSquared());
	}

	// length
	{
		Quaternion q(2.0f, 4.0f, 5.0f, 6.0f);

		// Should equal to Sqrt(q.W * q.W + q.X * q.X + q.Y * q.Y + q.Z * q.Z)
		SEOUL_UNITTESTING_ASSERT(9.0f == q.Length());
	}

	// normalize
	{
		{
			Quaternion q(3.0f, 4.0f, 5.0f, 6.0f);
			SEOUL_UNITTESTING_ASSERT(q.Normalize());
			SEOUL_UNITTESTING_ASSERT(Equals(1.0f, q.Length(), 1e-3f));
		}

		{
			Quaternion q(3.0f, 4.0f, 5.0f, 6.0f);
			SEOUL_UNITTESTING_ASSERT(Equals(1.0f, Quaternion::Normalize(q).Length(), 1e-3f));
		}

		{
			Quaternion q(0.0f, 0.0f, 0.0f, 0.0f);
			SEOUL_UNITTESTING_ASSERT(!q.Normalize());
			SEOUL_UNITTESTING_ASSERT_EQUAL(q, Quaternion::Normalize(q));
		}
	}

	// inverse
	{
		Quaternion q0(3.0f, 4.0f, 5.0f, 6.0f);

		{
			Quaternion q = q0.Inverse();

			// A quaternion multiplied by its inverse or its inverse multiplied
			// by the quaternion should be the identity quaternion.
			SEOUL_UNITTESTING_ASSERT((q0 * q).Equals(Quaternion::Identity(), 1e-3f));
			SEOUL_UNITTESTING_ASSERT((q * q0).Equals(Quaternion::Identity(), 1e-3f));
		}

		{
			Quaternion q = Quaternion::Inverse(q0);

			// A quaternion multiplied by its inverse or its inverse multiplied
			// by the quaternion should be the identity quaternion.
			SEOUL_UNITTESTING_ASSERT((q0 * q).Equals(Quaternion::Identity(), 1e-3f));
			SEOUL_UNITTESTING_ASSERT((q * q0).Equals(Quaternion::Identity(), 1e-3f));
		}
	}
}

static const Quaternion k90DegreesX(0.707106781f, 0.0f, 0.0f, 0.707106781f);
static const Quaternion k90DegreesY(0.0f, 0.707106781f, 0.0f, 0.707106781f);
static const Quaternion k90DegreesZ(0.0f, 0.0f, 0.707106781f, 0.707106781f);

// Empirically tuned on current Seoul Engine platforms for unit tests currently
// defined in this file.
static const Float kEqualityTolerance = 8e-6f;

/**
 * Test conversion from a Quaternion to a Matrix3D and Matrix4D.
 */
void QuaternionTest::TestToMatrix()
{
	// identity quaternion, should equal identity matrix.
	{
		Matrix3D m3(Quaternion::Identity().GetMatrix3D());
		SEOUL_UNITTESTING_ASSERT(Matrix3D::Identity() == m3);

		Matrix4D m4(Quaternion::Identity().GetMatrix4D());
		SEOUL_UNITTESTING_ASSERT(Matrix4D::Identity() == m4);
	}

	// 90-degree rotation around X.
	{
		Matrix3D m3(k90DegreesX.GetMatrix3D());
		SEOUL_UNITTESTING_ASSERT(
			Matrix3D(
				1.0f, 0.0f, 0.0f,
				0.0f, 0.0f, -1.0f,
				0.0f, 1.0f, 0.0f).Equals(m3, kEqualityTolerance));

		Matrix4D m4(k90DegreesX.GetMatrix4D());
		SEOUL_UNITTESTING_ASSERT(
			Matrix4D(
				1.0f, 0.0f, 0.0f, 0.0f,
				0.0f, 0.0f, -1.0f, 0.0f,
				0.0f, 1.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 0.0f, 1.0f).Equals(m4, kEqualityTolerance));
	}

	// 90-degree rotation around Y.
	{
		Matrix3D m3(k90DegreesY.GetMatrix3D());
		SEOUL_UNITTESTING_ASSERT(
			Matrix3D(
				 0.0f, 0.0f, 1.0f,
				 0.0f, 1.0f, 0.0f,
				-1.0f, 0.0f, 0.0f).Equals(m3, kEqualityTolerance));

		Matrix4D m4(k90DegreesY.GetMatrix4D());
		SEOUL_UNITTESTING_ASSERT(
			Matrix4D(
				 0.0f, 0.0f, 1.0f, 0.0f,
				 0.0f, 1.0f, 0.0f, 0.0f,
				-1.0f, 0.0f, 0.0f, 0.0f,
				 0.0f, 0.0f, 0.0f, 1.0f).Equals(m4, kEqualityTolerance));
	}

	// 90-degree rotation around Z.
	{
		Matrix3D m3(k90DegreesZ.GetMatrix3D());
		SEOUL_UNITTESTING_ASSERT(
			Matrix3D(
				0.0f, -1.0f, 0.0f,
				1.0f,  0.0f, 0.0f,
				0.0f,  0.0f, 1.0f).Equals(m3, kEqualityTolerance));

		Matrix4D m4(k90DegreesZ.GetMatrix4D());
		SEOUL_UNITTESTING_ASSERT(
			Matrix4D(
				0.0f, -1.0f, 0.0f, 0.0f,
				1.0f,  0.0f, 0.0f, 0.0f,
				0.0f,  0.0f, 1.0f, 0.0f,
				0.0f,  0.0f, 0.0f, 1.0f).Equals(m4, kEqualityTolerance));
	}
}

/**
 * Test conversion from a Matrix3D, Matrix4D, or Matrix3x4 to
 * a Quaternion.
 */
void QuaternionTest::TestFromMatrix()
{
	// identity matrix, should equal to identity quaternion
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			Quaternion::Identity(),
			Quaternion::CreateFromRotationMatrix(Matrix3D::Identity()));

		SEOUL_UNITTESTING_ASSERT_EQUAL(
			Quaternion::Identity(),
			Quaternion::CreateFromRotationMatrix(Matrix3x4::Identity()));

		SEOUL_UNITTESTING_ASSERT_EQUAL(
			Quaternion::Identity(),
			Quaternion::CreateFromRotationMatrix(Matrix4D::Identity()));
	}

	// 90-degree rotation around X.
	{
		{
			Matrix3D m3(
				1.0f, 0.0f, 0.0f,
				0.0f, 0.0f, -1.0f,
				0.0f, 1.0f, 0.0f);

			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(
				k90DegreesX,
				Quaternion::CreateFromRotationMatrix(m3),
				kEqualityTolerance);
		}

		{
			Matrix4D m4(
				1.0f, 0.0f, 0.0f, 0.0f,
				0.0f, 0.0f, -1.0f, 0.0f,
				0.0f, 1.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 0.0f, 1.0f);

			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(
				k90DegreesX,
				Quaternion::CreateFromRotationMatrix(m4),
				kEqualityTolerance);
		}

		{
			Matrix3x4 m3x4(
				1.0f, 0.0f, 0.0f, 0.0f,
				0.0f, 0.0f, -1.0f, 0.0f,
				0.0f, 1.0f, 0.0f, 0.0f);

			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(
				k90DegreesX,
				Quaternion::CreateFromRotationMatrix(m3x4),
				kEqualityTolerance);
		}
	}

	// 90-degree rotation around Y.
	{
		{
			Matrix3D m3(
				 0.0f, 0.0f, 1.0f,
				 0.0f, 1.0f, 0.0f,
				-1.0f, 0.0f, 0.0f);

			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(
				k90DegreesY,
				Quaternion::CreateFromRotationMatrix(m3),
				kEqualityTolerance);
		}

		{
			Matrix4D m4(
				 0.0f, 0.0f, 1.0f, 0.0f,
				 0.0f, 1.0f, 0.0f, 0.0f,
				-1.0f, 0.0f, 0.0f, 0.0f,
				 0.0f, 0.0f, 0.0f, 1.0f);

			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(
					k90DegreesY,
					Quaternion::CreateFromRotationMatrix(m4),
					kEqualityTolerance);
		}

		{
			Matrix3x4 m3x4(
				 0.0f, 0.0f, 1.0f, 0.0f,
				 0.0f, 1.0f, 0.0f, 0.0f,
				-1.0f, 0.0f, 0.0f, 0.0f);

			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(
					k90DegreesY,
					Quaternion::CreateFromRotationMatrix(m3x4),
					kEqualityTolerance);
		}
	}

	// 90-degree rotation around Z.
	{
		{
			Matrix3D m3(
				0.0f, -1.0f, 0.0f,
				1.0f,  0.0f, 0.0f,
				0.0f,  0.0f, 1.0f);

			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(
				k90DegreesZ,
				Quaternion::CreateFromRotationMatrix(m3),
				kEqualityTolerance);
		}

		{
			Matrix4D m4(
				 0.0f, -1.0f, 0.0f, 0.0f,
				 1.0f,  0.0f, 0.0f, 0.0f,
				 0.0f,  0.0f, 1.0f, 0.0f,
				 0.0f,  0.0f, 0.0f, 1.0f);

			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(
				k90DegreesZ,
				Quaternion::CreateFromRotationMatrix(m4),
				kEqualityTolerance);
		}

		{
			Matrix3x4 m3x4(
				 0.0f, -1.0f, 0.0f, 0.0f,
				 1.0f,  0.0f, 0.0f, 0.0f,
				 0.0f,  0.0f, 1.0f, 0.0f);

			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(
				k90DegreesZ,
				Quaternion::CreateFromRotationMatrix(m3x4),
				kEqualityTolerance);
		}
	}
}

/**
 * Test Quaternion::Transform, which transforms a Vector3D direction
 * vector by a Quaternion.
 */
void QuaternionTest::TestTransformation()
{
	// Identity
	{
		SEOUL_UNITTESTING_ASSERT(Vector3D::UnitX() == Quaternion::Transform(Quaternion::Identity(), Vector3D::UnitX()));
		SEOUL_UNITTESTING_ASSERT(Vector3D::UnitY() == Quaternion::Transform(Quaternion::Identity(), Vector3D::UnitY()));
		SEOUL_UNITTESTING_ASSERT(Vector3D::UnitZ() == Quaternion::Transform(Quaternion::Identity(), Vector3D::UnitZ()));
	}

	// 90-degree rotation around X.
	{
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Quaternion::Normalize(k90DegreesX), k90DegreesX, kEqualityTolerance);
		SEOUL_UNITTESTING_ASSERT(( Vector3D::UnitX()).Equals(Quaternion::Transform(k90DegreesX, Vector3D::UnitX()), kEqualityTolerance));
		SEOUL_UNITTESTING_ASSERT(( Vector3D::UnitZ()).Equals(Quaternion::Transform(k90DegreesX, Vector3D::UnitY()), kEqualityTolerance));
		SEOUL_UNITTESTING_ASSERT((-Vector3D::UnitY()).Equals(Quaternion::Transform(k90DegreesX, Vector3D::UnitZ()), kEqualityTolerance));
	}

	// 90-degree rotation around Y.
	{
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Quaternion::Normalize(k90DegreesY), k90DegreesY, kEqualityTolerance);
		SEOUL_UNITTESTING_ASSERT((-Vector3D::UnitZ()).Equals(Quaternion::Transform(k90DegreesY, Vector3D::UnitX()), kEqualityTolerance));
		SEOUL_UNITTESTING_ASSERT(( Vector3D::UnitY()).Equals(Quaternion::Transform(k90DegreesY, Vector3D::UnitY()), kEqualityTolerance));
		SEOUL_UNITTESTING_ASSERT(( Vector3D::UnitX()).Equals(Quaternion::Transform(k90DegreesY, Vector3D::UnitZ()), kEqualityTolerance));
	}

	// 90-degree rotation around Z.
	{
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Quaternion::Normalize(k90DegreesZ), k90DegreesZ, kEqualityTolerance);
		SEOUL_UNITTESTING_ASSERT(( Vector3D::UnitY()).Equals(Quaternion::Transform(k90DegreesZ, Vector3D::UnitX()), kEqualityTolerance));
		SEOUL_UNITTESTING_ASSERT((-Vector3D::UnitX()).Equals(Quaternion::Transform(k90DegreesZ, Vector3D::UnitY()), kEqualityTolerance));
		SEOUL_UNITTESTING_ASSERT(( Vector3D::UnitZ()).Equals(Quaternion::Transform(k90DegreesZ, Vector3D::UnitZ()), kEqualityTolerance));
	}
}

/**
 * Test utility functions, mostly for creating new Quaternions
 * from various angle configurations.
 */
void QuaternionTest::TestUtilities()
{
	// x rotation test
	{
		Quaternion q(Quaternion::CreateFromRotationX(DegreesToRadians(90.0f)));
		SEOUL_UNITTESTING_ASSERT(k90DegreesX.Equals(q));
	}

	// y rotation test
	{
		Quaternion q(Quaternion::CreateFromRotationY(DegreesToRadians(90.0f)));
		SEOUL_UNITTESTING_ASSERT(k90DegreesY.Equals(q));
	}

	// z rotation test
	{
		Quaternion q(Quaternion::CreateFromRotationZ(DegreesToRadians(90.0f)));
		SEOUL_UNITTESTING_ASSERT(k90DegreesZ.Equals(q));
	}

	// axis angle rotation test.
	{
		// identity
		{
			Quaternion q(Quaternion::CreateFromAxisAngle(Vector3D::UnitX(), 0.0f));
			SEOUL_UNITTESTING_ASSERT(Quaternion::Identity().Equals(q));
		}

		// 90-degree x rotation
		{
			Quaternion q(Quaternion::CreateFromAxisAngle(Vector3D::UnitX(), DegreesToRadians(90.0f)));
			SEOUL_UNITTESTING_ASSERT(k90DegreesX.Equals(q));
		}

		// 90-degree y rotation
		{
			Quaternion q(Quaternion::CreateFromAxisAngle(Vector3D::UnitY(), DegreesToRadians(90.0f)));
			SEOUL_UNITTESTING_ASSERT(k90DegreesY.Equals(q));
		}

		// 90-degree z rotation
		{
			Quaternion q(Quaternion::CreateFromAxisAngle(Vector3D::UnitZ(), DegreesToRadians(90.0f)));
			SEOUL_UNITTESTING_ASSERT(k90DegreesZ.Equals(q));
		}
	}

	// euler angle test
	{
		// identity
		{
			{
				Quaternion q(Quaternion::CreateFromYawPitchRollYXZ(0.0f, 0.0f, 0.0f));
				SEOUL_UNITTESTING_ASSERT(Quaternion::Identity().Equals(q, kEqualityTolerance));
			}

			{
				Quaternion q(Quaternion::CreateFromYawPitchRollZXY(0.0f, 0.0f, 0.0f));
				SEOUL_UNITTESTING_ASSERT(Quaternion::Identity().Equals(q, kEqualityTolerance));
			}
		}

		// 90-degree yaw, 90-degree pitch
		{
			{
				Quaternion q(Quaternion::CreateFromYawPitchRollYXZ(DegreesToRadians(90.0f), DegreesToRadians(90.0f), 0.0f));
				SEOUL_UNITTESTING_ASSERT((Vector3D::UnitY()).Equals(Quaternion::Transform(q, Vector3D::UnitX()), kEqualityTolerance));
				SEOUL_UNITTESTING_ASSERT((Vector3D::UnitZ()).Equals(Quaternion::Transform(q, Vector3D::UnitY()), kEqualityTolerance));
				SEOUL_UNITTESTING_ASSERT((Vector3D::UnitX()).Equals(Quaternion::Transform(q, Vector3D::UnitZ()), kEqualityTolerance));
			}

			{
				Quaternion q(Quaternion::CreateFromYawPitchRollZXY(DegreesToRadians(90.0f), DegreesToRadians(90.0f), 0.0f));
				SEOUL_UNITTESTING_ASSERT(( Vector3D::UnitZ()).Equals(Quaternion::Transform(q, Vector3D::UnitX()), kEqualityTolerance));
				SEOUL_UNITTESTING_ASSERT((-Vector3D::UnitX()).Equals(Quaternion::Transform(q, Vector3D::UnitY()), kEqualityTolerance));
				SEOUL_UNITTESTING_ASSERT((-Vector3D::UnitY()).Equals(Quaternion::Transform(q, Vector3D::UnitZ()), kEqualityTolerance));
			}
		}

		// 90-degree pitch, 90-degree roll
		{
			{
				Quaternion q(Quaternion::CreateFromYawPitchRollYXZ(0.0f, DegreesToRadians(90.0f), DegreesToRadians(90.0f)));
				SEOUL_UNITTESTING_ASSERT((Vector3D::UnitY()).Equals(Quaternion::Transform(q, Vector3D::UnitX()), kEqualityTolerance));
				SEOUL_UNITTESTING_ASSERT((Vector3D::UnitZ()).Equals(Quaternion::Transform(q, Vector3D::UnitY()), kEqualityTolerance));
				SEOUL_UNITTESTING_ASSERT((Vector3D::UnitX()).Equals(Quaternion::Transform(q, Vector3D::UnitZ()), kEqualityTolerance));
			}

			{
				Quaternion q(Quaternion::CreateFromYawPitchRollZXY(0.0f, DegreesToRadians(90.0f), DegreesToRadians(90.0f)));
				SEOUL_UNITTESTING_ASSERT((-Vector3D::UnitZ()).Equals(Quaternion::Transform(q, Vector3D::UnitX()), kEqualityTolerance));
				SEOUL_UNITTESTING_ASSERT(( Vector3D::UnitX()).Equals(Quaternion::Transform(q, Vector3D::UnitY()), kEqualityTolerance));
				SEOUL_UNITTESTING_ASSERT((-Vector3D::UnitY()).Equals(Quaternion::Transform(q, Vector3D::UnitZ()), kEqualityTolerance));
			}
		}

		// 90-degree yaw, 90-degree roll
		{
			{
				Quaternion q(Quaternion::CreateFromYawPitchRollYXZ(DegreesToRadians(90.0f), 0.0f, DegreesToRadians(90.0f)));
				SEOUL_UNITTESTING_ASSERT((-Vector3D::UnitZ()).Equals(Quaternion::Transform(q, Vector3D::UnitX()), kEqualityTolerance));
				SEOUL_UNITTESTING_ASSERT((-Vector3D::UnitX()).Equals(Quaternion::Transform(q, Vector3D::UnitY()), kEqualityTolerance));
				SEOUL_UNITTESTING_ASSERT(( Vector3D::UnitY()).Equals(Quaternion::Transform(q, Vector3D::UnitZ()), kEqualityTolerance));
			}

			{
				Quaternion q(Quaternion::CreateFromYawPitchRollZXY(DegreesToRadians(90.0f), 0.0f, DegreesToRadians(90.0f)));
				SEOUL_UNITTESTING_ASSERT(( Vector3D::UnitY()).Equals(Quaternion::Transform(q, Vector3D::UnitX()), kEqualityTolerance));
				SEOUL_UNITTESTING_ASSERT(( Vector3D::UnitZ()).Equals(Quaternion::Transform(q, Vector3D::UnitY()), kEqualityTolerance));
				SEOUL_UNITTESTING_ASSERT(( Vector3D::UnitX()).Equals(Quaternion::Transform(q, Vector3D::UnitZ()), kEqualityTolerance));
			}
		}
	}

	// lerp test
	{
		// lerp produces less accurate results in most cases, so we need a larger tolerance.
		static const Float kfLerpTolerance = 0.01f;

		// identity test
		{
			Quaternion q = Quaternion::Lerp(Quaternion::Identity(), Quaternion::Identity(), 0.5f);
			SEOUL_UNITTESTING_ASSERT(Quaternion::Identity() == q);
		}

		// lerp of 30 and 60 degrees around X
		{
			Quaternion q0 = Quaternion::CreateFromRotationX(DegreesToRadians(30.0f));
			Quaternion q1 = Quaternion::CreateFromRotationX(DegreesToRadians(60.0f));

			// result should be a 45 degree rotation around X.
			SEOUL_UNITTESTING_ASSERT(
				Quaternion::CreateFromRotationX(DegreesToRadians(45.0f)).Equals(
					Quaternion::Lerp(q0, q1, 0.5f), kfLerpTolerance));
		}

		// lerp of 30 and 60 degrees around Y
		{
			Quaternion q0 = Quaternion::CreateFromRotationY(DegreesToRadians(30.0f));
			Quaternion q1 = Quaternion::CreateFromRotationY(DegreesToRadians(60.0f));

			// result should be a 45 degree rotation around Y.
			SEOUL_UNITTESTING_ASSERT(
				Quaternion::CreateFromRotationY(DegreesToRadians(45.0f)).Equals(
					Quaternion::Lerp(q0, q1, 0.5f), kfLerpTolerance));
		}

		// lerp of 30 and 60 degrees around Z
		{
			Quaternion q0 = Quaternion::CreateFromRotationZ(DegreesToRadians(30.0f));
			Quaternion q1 = Quaternion::CreateFromRotationZ(DegreesToRadians(60.0f));

			// result should be a 45 degree rotation around Z.
			SEOUL_UNITTESTING_ASSERT(
				Quaternion::CreateFromRotationZ(DegreesToRadians(45.0f)).Equals(
					Quaternion::Lerp(q0, q1, 0.5f), kfLerpTolerance));
		}

		// lerp of 30 and 330 degrees around X
		{
			Quaternion q0 = Quaternion::CreateFromRotationX(DegreesToRadians(30.0f));
			Quaternion q1 = Quaternion::CreateFromRotationX(DegreesToRadians(330.0f));

			// result should be a 180 degree rotation around X.
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(
				180.0f,
				RadiansToDegrees(Quaternion::Lerp(q0, q1, 0.5f).GetRotationX()),
				kEqualityTolerance);
		}

		// lerp of 30 and 330 degrees around Y
		{
			Quaternion q0 = Quaternion::CreateFromRotationY(DegreesToRadians(30.0f));
			Quaternion q1 = Quaternion::CreateFromRotationY(DegreesToRadians(330.0f));

			// result should be a 180 degree rotation around Y.
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(
				180.0f,
				RadiansToDegrees(Quaternion::Lerp(q0, q1, 0.5f).GetRotationY()),
				kEqualityTolerance);
		}

		// lerp of 30 and 330 degrees around Z
		{
			Quaternion q0 = Quaternion::CreateFromRotationZ(DegreesToRadians(30.0f));
			Quaternion q1 = Quaternion::CreateFromRotationZ(DegreesToRadians(330.0f));

			// result should be a 180 degree rotation around Z.
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(
				180.0f,
				RadiansToDegrees(Quaternion::Lerp(q0, q1, 0.5f).GetRotationZ()),
				kEqualityTolerance);
		}
	}

	// slerp test
	{
		// identity test
		{
			Quaternion q = Quaternion::Slerp(Quaternion::Identity(), Quaternion::Identity(), 0.5f);
			SEOUL_UNITTESTING_ASSERT(Quaternion::Identity() == q);
		}

		// slerp of 30 and 60 degrees around X
		{
			Quaternion q0 = Quaternion::CreateFromRotationX(DegreesToRadians(30.0f));
			Quaternion q1 = Quaternion::CreateFromRotationX(DegreesToRadians(60.0f));

			// result should be a 45 degree rotation around X.
			SEOUL_UNITTESTING_ASSERT(
				Quaternion::CreateFromRotationX(DegreesToRadians(45.0f)).Equals(
					Quaternion::Slerp(q0, q1, 0.5f), kEqualityTolerance));
		}

		// slerp of 30 and 60 degrees around Y
		{
			Quaternion q0 = Quaternion::CreateFromRotationY(DegreesToRadians(30.0f));
			Quaternion q1 = Quaternion::CreateFromRotationY(DegreesToRadians(60.0f));

			// result should be a 45 degree rotation around Y.
			SEOUL_UNITTESTING_ASSERT(
				Quaternion::CreateFromRotationY(DegreesToRadians(45.0f)).Equals(
					Quaternion::Slerp(q0, q1, 0.5f), kEqualityTolerance));
		}

		// slerp of 30 and 60 degrees around Z
		{
			Quaternion q0 = Quaternion::CreateFromRotationZ(DegreesToRadians(30.0f));
			Quaternion q1 = Quaternion::CreateFromRotationZ(DegreesToRadians(60.0f));

			// result should be a 45 degree rotation around Z.
			SEOUL_UNITTESTING_ASSERT(
				Quaternion::CreateFromRotationZ(DegreesToRadians(45.0f)).Equals(
					Quaternion::Slerp(q0, q1, 0.5f), kEqualityTolerance));
		}

		// slerp of 30 and 330 degrees around X
		{
			Quaternion q0 = Quaternion::CreateFromRotationX(DegreesToRadians(30.0f));
			Quaternion q1 = Quaternion::CreateFromRotationX(DegreesToRadians(330.0f));

			// result should be a 0 degree rotation around X.
			SEOUL_UNITTESTING_ASSERT(
				Quaternion::CreateFromRotationX(DegreesToRadians(0.0f)).Equals(
					Quaternion::Slerp(q0, q1, 0.5f), kEqualityTolerance));
		}

		// slerp of 30 and 330 degrees around Y
		{
			Quaternion q0 = Quaternion::CreateFromRotationY(DegreesToRadians(30.0f));
			Quaternion q1 = Quaternion::CreateFromRotationY(DegreesToRadians(330.0f));

			// result should be a 0 degree rotation around Y.
			SEOUL_UNITTESTING_ASSERT(
				Quaternion::CreateFromRotationY(DegreesToRadians(0.0f)).Equals(
					Quaternion::Slerp(q0, q1, 0.5f), kEqualityTolerance));
		}

		// slerp of 30 and 330 degrees around Z
		{
			Quaternion q0 = Quaternion::CreateFromRotationZ(DegreesToRadians(30.0f));
			Quaternion q1 = Quaternion::CreateFromRotationZ(DegreesToRadians(330.0f));

			// result should be a 0 degree rotation around Z.
			SEOUL_UNITTESTING_ASSERT(
				Quaternion::CreateFromRotationZ(DegreesToRadians(0.0f)).Equals(
					Quaternion::Slerp(q0, q1, 0.5f), kEqualityTolerance));
		}
	}

	// rotation from direction test
	{
		// identity test
		{
			Quaternion q = Quaternion::CreateFromDirection(
				-Vector3D::UnitZ());
			SEOUL_UNITTESTING_ASSERT(Quaternion::Identity().Equals(q));
		}

		// unit x to unit y -> 90-degree rotation around Z.
		{
			Quaternion q = Quaternion::CreateFromDirection(
				Vector3D::UnitY(),
				Vector3D::UnitX());
			SEOUL_UNITTESTING_ASSERT(
				Quaternion::CreateFromRotationZ(DegreesToRadians(90.0f)).Equals(q));
		}

		// unit y to unit z -> 90-degree rotation around X.
		{
			Quaternion q = Quaternion::CreateFromDirection(
				Vector3D::UnitZ(),
				Vector3D::UnitY());
			SEOUL_UNITTESTING_ASSERT(
				Quaternion::CreateFromRotationX(DegreesToRadians(90.0f)).Equals(q));
		}

		// unit z to unit x -> 90-degree rotation around Y.
		{
			Quaternion q = Quaternion::CreateFromDirection(
				Vector3D::UnitX(),
				Vector3D::UnitZ());
			SEOUL_UNITTESTING_ASSERT(
				Quaternion::CreateFromRotationY(DegreesToRadians(90.0f)).Equals(q));
		}
	}

	// angle from quaternion test
	{
		// identity test
		{
			SEOUL_UNITTESTING_ASSERT(0.0f == Quaternion::Identity().GetRotationX());
			SEOUL_UNITTESTING_ASSERT(0.0f == Quaternion::Identity().GetRotationY());
			SEOUL_UNITTESTING_ASSERT(0.0f == Quaternion::Identity().GetRotationZ());
		}

		// 90 degrees around x
		{
			Quaternion q = Quaternion::CreateFromRotationX(DegreesToRadians(90.0f));
			SEOUL_UNITTESTING_ASSERT(Equals(DegreesToRadians(90.0f), q.GetRotationX(), kEqualityTolerance));
			SEOUL_UNITTESTING_ASSERT(0.0f == q.GetRotationY());
			SEOUL_UNITTESTING_ASSERT(0.0f == q.GetRotationZ());
		}

		// 90 degrees around y
		{
			Quaternion q = Quaternion::CreateFromRotationY(DegreesToRadians(90.0f));
			SEOUL_UNITTESTING_ASSERT(0.0f == q.GetRotationX());
			SEOUL_UNITTESTING_ASSERT(Equals(DegreesToRadians(90.0f), q.GetRotationY(), kEqualityTolerance));
			SEOUL_UNITTESTING_ASSERT(0.0f == q.GetRotationZ());

		}

		// 90 degrees around z
		{
			Quaternion q = Quaternion::CreateFromRotationZ(DegreesToRadians(90.0f));
			SEOUL_UNITTESTING_ASSERT(0.0f == q.GetRotationX());
			SEOUL_UNITTESTING_ASSERT(0.0f == q.GetRotationY());
			SEOUL_UNITTESTING_ASSERT(Equals(DegreesToRadians(90.0f), q.GetRotationZ(), kEqualityTolerance));
		}
	}
}

/** Previous failures of Quaternion::Transform() to catch regressions. */
void QuaternionTest::TestTransformationRegressions()
{
	{
		// Test Quaternion - stored as UInt32 to ensure exact value reproduction.
		Quaternion q;
		UInt32 u = 0xbf39a452;
		memcpy(&q.X, &u, sizeof(u));
		u = 0xbefce962;
		memcpy(&q.Y, &u, sizeof(u));
		u = 0xbed1f3ee;
		memcpy(&q.Z, &u, sizeof(u));
		u = 0x3e7ef330;
		memcpy(&q.W, &u, sizeof(u));

		// Affirm that q is unit length.
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Quaternion::Normalize(q), q, kEqualityTolerance);

		// Transform and verify still unit length.
		auto const vAxis(Vector3D::UnitZ());

		// {X=0.348756254 Y=0.766211748 Z=-0.539733410 }
		auto const vAxisT(Quaternion::Transform(q, vAxis));
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Vector3D(0.348756254f, 0.766211748f, -0.539725780f), vAxisT, kEqualityTolerance);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(vAxis.Length(), Quaternion::Transform(q, vAxis).Length(), kEqualityTolerance);
	}
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
