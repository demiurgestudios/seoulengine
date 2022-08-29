/**
 * \file Matrix2DTest.cpp
 * \brief Unit tests for the Matrix2D class. Matrix2D is the work horse
 * of our linear algebra classes, and is used to represet a variety
 * of 3D transformations.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Logger.h"
#include "Matrix2x3.h"
#include "Matrix2DTest.h"
#include "Matrix2D.h"
#include "ReflectionDefine.h"
#include "UnitTesting.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(Matrix2DTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestMethods)
	SEOUL_METHOD(TestTransformation)
	SEOUL_METHOD(TestUtilities)
SEOUL_END_TYPE()

static const Matrix2D k90Degrees(
	0.0f, -1.0f,
	1.0f,  0.0f);

/**
 * Test most of the member methods of the Matrix2D class.
 */
void Matrix2DTest::TestMethods()
{
	// zero
	{
		SEOUL_UNITTESTING_ASSERT(
			// row 0
			0.0f == Matrix2D::Zero().M00 &&
			0.0f == Matrix2D::Zero().M01 &&

			// row 1
			0.0f == Matrix2D::Zero().M10 &&
			0.0f == Matrix2D::Zero().M11);
	}

	// identity
	{
		SEOUL_UNITTESTING_ASSERT(
			// row 0
			1.0f == Matrix2D::Identity().M00 &&
			0.0f == Matrix2D::Identity().M01 &&

			// row 1
			0.0f == Matrix2D::Identity().M10 &&
			1.0f == Matrix2D::Identity().M11);
	}

	// tolerant equality
	{
		Matrix2D m0(
			3.0f,  4.0f,
			7.0f,  8.0f);
		Matrix2D m1(
			3.000999f,  4.000999f,
			7.000999f,  8.000999f);

		SEOUL_UNITTESTING_ASSERT(!m0.Equals(m1, 1e-4f));
		SEOUL_UNITTESTING_ASSERT(m0.Equals(m1, 1e-3f));
	}

	// tolerant zero
	{
		Matrix2D m(
			0.000999f, 0.000999f,
			0.000999f, 0.000999f);

		SEOUL_UNITTESTING_ASSERT(!m.IsZero(1e-4f));
		SEOUL_UNITTESTING_ASSERT(m.IsZero(1e-3f));
	}

	// default constructor
	{
		Matrix2D m;

		// Per element verification
		SEOUL_UNITTESTING_ASSERT(
			0.0f == m.M00 && 0.0f == m.M01 &&
			0.0f == m.M10 && 0.0f == m.M11);
	}

	// single value constructor
	{
		Matrix2D m(13.0f);

		// Per element verification
		SEOUL_UNITTESTING_ASSERT(
			13.0f == m.M00 && 13.0f == m.M01 &&
			13.0f == m.M10 && 13.0f == m.M11);
	}

	// componentwise constructor
	{
		Matrix2D m(
			3.0f,  4.0f,
			7.0f,  8.0f);

		// Per element verification
		SEOUL_UNITTESTING_ASSERT(
			3.0f  == m.M00 && 4.0f  == m.M01 &&
			7.0f  == m.M10 && 8.0f  == m.M11);

		// Array verification - ensure column major storage
		SEOUL_UNITTESTING_ASSERT(
			3.0f  == m.GetData()[0] &&
			7.0f  == m.GetData()[1] &&
			4.0f == m.GetData()[2] &&
			8.0f == m.GetData()[3]);

		// Per-column verification
		SEOUL_UNITTESTING_ASSERT(
			3.0f  == m.GetColumn(0).X && 4.0f  == m.GetColumn(1).X &&
			7.0f  == m.GetColumn(0).Y && 8.0f  == m.GetColumn(1).Y);
	}

	// copy constructor
	{
		Matrix2D m(Matrix2D(
			3.0f,  4.0f,
			7.0f,  8.0f));

		SEOUL_UNITTESTING_ASSERT(
			3.0f  == m.M00 && 4.0f  == m.M01 &&
			7.0f  == m.M10 && 8.0f  == m.M11);
	}

	// matrix3x4 constructor
	{
		Matrix2D m(Matrix2x3(
			3.0f, 4.0f, 5.0f,
			7.0f, 8.0f, 9.0f));

		SEOUL_UNITTESTING_ASSERT(
			3.0f  == m.M00 && 4.0f  == m.M01 &&
			7.0f  == m.M10 && 8.0f  == m.M11);
	}

	// assignment
	{
		Matrix2D m = Matrix2D(
			3.0f,  4.0f,
			7.0f,  8.0f);

		SEOUL_UNITTESTING_ASSERT(
			3.0f  == m.M00 && 4.0f  == m.M01 &&
			7.0f  == m.M10 && 8.0f  == m.M11);
	}

	// self-addition
	{
		Matrix2D m0(
			3.0f,  4.0f,
			7.0f,  8.0f);

		{
			Matrix2D m = (m0 + m0);
			SEOUL_UNITTESTING_ASSERT(
				6.0f  == m.M00 && 8.0f  == m.M01 &&
				14.0f == m.M10 && 16.0f == m.M11);
		}

		{
			Matrix2D m = m0;
			m += m;
			SEOUL_UNITTESTING_ASSERT(
				6.0f  == m.M00 && 8.0f  == m.M01 &&
				14.0f == m.M10 && 16.0f == m.M11);
		}
	}

	// addition
	{
		Matrix2D m0(
			3.0f,  4.0f,
			7.0f,  8.0f);
		Matrix2D m1(
			-4.0f,  -5.0f,
			-8.0f,  -9.0f);

		{
			Matrix2D m = (m0 + m1);
			SEOUL_UNITTESTING_ASSERT(
				-1.0f == m.M00 && -1.0f == m.M01 &&
				-1.0f == m.M10 && -1.0f == m.M11);
		}

		{
			Matrix2D m = m0;
			m += m1;
			SEOUL_UNITTESTING_ASSERT(
				-1.0f == m.M00 && -1.0f == m.M01 &&
				-1.0f == m.M10 && -1.0f == m.M11);
		}
	}

	// self-subtraction
	{
		Matrix2D m0(
			3.0f,  4.0f,
			7.0f,  8.0f);

		{
			Matrix2D m = (m0 - m0);
			SEOUL_UNITTESTING_ASSERT(
				0.0f == m.M00 && 0.0f == m.M01 &&
				0.0f == m.M10 && 0.0f == m.M11);
		}

		{
			Matrix2D m = m0;
			m -= m;
			SEOUL_UNITTESTING_ASSERT(
				0.0f == m.M00 && 0.0f == m.M01 &&
				0.0f == m.M10 && 0.0f == m.M11);
		}
	}

	// subtraction
	{
		Matrix2D m0(
			3.0f,  4.0f,
			7.0f,  8.0f);
		Matrix2D m1(
			4.0f,  5.0f,
			8.0f,  9.0f);

		{
			Matrix2D m = (m0 - m1);
			SEOUL_UNITTESTING_ASSERT(
				-1.0f == m.M00 && -1.0f == m.M01 &&
				-1.0f == m.M10 && -1.0f == m.M11);
		}

		{
			Matrix2D m = m0;
			m -= m1;
			SEOUL_UNITTESTING_ASSERT(
				-1.0f == m.M00 && -1.0f == m.M01 &&
				-1.0f == m.M10 && -1.0f == m.M11);
		}
	}

	// negation
	{
		Matrix2D m0(
			3.0f,  4.0f,
			7.0f,  8.0f);
		Matrix2D m = (-m0);

		SEOUL_UNITTESTING_ASSERT(
			-3.0f  == m.M00 && -4.0f  == m.M01 &&
			-7.0f  == m.M10 && -8.0f  == m.M11);
	}

	// matrix self-multiplication
	{
		Matrix2D m0(
			3.0f,  4.0f,
			7.0f,  8.0f);

		{
			Matrix2D m = (m0 * m0);
			SEOUL_UNITTESTING_ASSERT(
				37.0f == m.M00 && 44.0f == m.M01 &&
				77.0f == m.M10 && 92.0f == m.M11);
		}

		{
			Matrix2D m = m0;
			m *= m;
			SEOUL_UNITTESTING_ASSERT(
				37.0f == m.M00 && 44.0f == m.M01 &&
				77.0f == m.M10 && 92.0f == m.M11);
		}
	}

	// matrix multiplication
	{
		Matrix2D m0(
			3.0f,  4.0f,
			7.0f,  8.0f);
		Matrix2D m1(
			4.0f,  5.0f,
			8.0f,  9.0f);

		{
			Matrix2D m = (m0 * m1);
			SEOUL_UNITTESTING_ASSERT(
				44.0f == m.M00 && 51.0f == m.M01 &&
				92.0f == m.M10 && 107.0f == m.M11);
		}

		{
			Matrix2D m = m0;
			m *= m1;
			SEOUL_UNITTESTING_ASSERT(
				44.0f == m.M00 && 51.0f == m.M01 &&
				92.0f == m.M10 && 107.0f == m.M11);
		}

		{
			Matrix2D m = (m1 * m0);
			SEOUL_UNITTESTING_ASSERT(
				47.0f == m.M00 && 56.0f == m.M01 &&
				87.0f == m.M10 && 104.0f == m.M11);
		}

		{
			Matrix2D m = m1;
			m *= m0;
			SEOUL_UNITTESTING_ASSERT(
				47.0f == m.M00 && 56.0f == m.M01 &&
				87.0f == m.M10 && 104.0f == m.M11);
		}
	}

	// scalar multiplication
	{
		Matrix2D m0(
			3.0f,  4.0f,
			7.0f,  8.0f);

		{
			Matrix2D m = (m0 * 3.0f);
			SEOUL_UNITTESTING_ASSERT(
				9.0f  == m.M00 && 12.0f == m.M01 &&
				21.0f == m.M10 && 24.0f == m.M11);
		}

		{
			Matrix2D m = (3.0f * m0);
			SEOUL_UNITTESTING_ASSERT(
				9.0f  == m.M00 && 12.0f == m.M01 &&
				21.0f == m.M10 && 24.0f == m.M11);
		}

		{
			Matrix2D m = m0;
			m *= 3.0f;
			SEOUL_UNITTESTING_ASSERT(
				9.0f  == m.M00 && 12.0f == m.M01 &&
				21.0f == m.M10 && 24.0f == m.M11);
		}
	}

	// scalar division
	{
		Matrix2D m0(
			3.0f,  9.0f,
			18.0f, 21.0f);

		{
			Matrix2D m = (m0 / 3.0f);
			SEOUL_UNITTESTING_ASSERT(
				1.0f  == m.M00 && 3.0f  == m.M01 &&
				6.0f  == m.M10 && 7.0f  == m.M11);
		}

		{
			Matrix2D m = m0;
			m /= 3.0f;
			SEOUL_UNITTESTING_ASSERT(
				1.0f  == m.M00 && 3.0f  == m.M01 &&
				6.0f  == m.M10 && 7.0f  == m.M11);
		}
	}

	// self-equality
	{
		Matrix2D m(
			3.0f,  4.0f,
			7.0f,  8.0f);

		SEOUL_UNITTESTING_ASSERT(m == m);
	}

	// equality
	{
		Matrix2D m0(
			3.0f,  4.0f,
			7.0f,  8.0f);

		Matrix2D m1(
			3.0f,  4.0f,
			7.0f,  8.0f);

		SEOUL_UNITTESTING_ASSERT(m0 == m1);
	}

	// inequality
	{
		Matrix2D m0(
			3.0f,  4.0f,
			7.0f,  8.0f);

		Matrix2D m1(
			4.0f,  5.0f,
			8.0f,  9.0f);

		SEOUL_UNITTESTING_ASSERT(m0 != m1);
	}

	// diagonal
	{
		Matrix2D m(
			3.0f,  4.0f,
			7.0f,  8.0f);

		Vector2D v = m.GetDiagonal();

		SEOUL_UNITTESTING_ASSERT(
			3.0f == v.X && 8.0f == v.Y);
	}

	// transpose
	{
		Matrix2D m(
			3.0f,  4.0f,
			7.0f,  8.0f);

		Matrix2D m1 = m.Transpose();

		// Verify the source is unchanged
		SEOUL_UNITTESTING_ASSERT(
			3.0f  == m.M00 && 4.0f  == m.M01 &&
			7.0f  == m.M10 && 8.0f  == m.M11);

		SEOUL_UNITTESTING_ASSERT(
			3.0f == m1.M00 && 7.0f  == m1.M01 &&
			4.0f == m1.M10 && 8.0f  == m1.M11);
	}

	// determinant
	{
		Matrix2D m(
			3.0f,  4.0f,
			7.0f,  3.0f);

		SEOUL_UNITTESTING_ASSERT(-19 == m.Determinant());
	}

	// inverse
	{
		Matrix2D m(
			1.0f,  4.0f,
			7.0f,  1.0f);

		Matrix2D m0 = m.Inverse();

		SEOUL_UNITTESTING_ASSERT((m * m0).Equals(Matrix2D::Identity(), 1e-3f));
		SEOUL_UNITTESTING_ASSERT((m0 * m).Equals(Matrix2D::Identity(), 1e-3f));
	}

	// orthonormal tests
	{
		SEOUL_UNITTESTING_ASSERT(k90Degrees.IsOrthonormal());

		SEOUL_UNITTESTING_ASSERT(
			k90Degrees.OrthonormalInverse() == k90Degrees.Transpose());
	}

	// accessors
	{
		Matrix2D m(
			3.0f,  4.0f,
			7.0f,  8.0f);

		// Per element accessor
		SEOUL_UNITTESTING_ASSERT(
			3.0f  == m(0, 0) && 4.0f  == m(0, 1) &&
			7.0f  == m(1, 0) && 8.0f  == m(1, 1));

		// Row accessor
		SEOUL_UNITTESTING_ASSERT(
			3.0f  == m.GetRow(0).X && 4.0f  == m.GetRow(0).Y &&
			7.0f  == m.GetRow(1).X && 8.0f  == m.GetRow(1).Y);

		// Column accessor
		SEOUL_UNITTESTING_ASSERT(
			3.0f  == m.GetColumn(0).X && 4.0f  == m.GetColumn(1).X &&
			7.0f  == m.GetColumn(0).Y && 8.0f  == m.GetColumn(1).Y);
	}

	// unit axis tests
	{
		Matrix2D m(
			3.0f,  0.0f,
			0.0f,  8.0f);

		SEOUL_UNITTESTING_ASSERT(
			Vector2D::UnitX() == m.GetUnitAxis(0));

		SEOUL_UNITTESTING_ASSERT(
			Vector2D::UnitY() == m.GetUnitAxis(1));

		Vector2D vUnitX;
		Vector2D vUnitY;
		m.GetUnitAxes(vUnitX, vUnitY);

		SEOUL_UNITTESTING_ASSERT(Vector2D::UnitX() == vUnitX);
		SEOUL_UNITTESTING_ASSERT(Vector2D::UnitY() == vUnitY);
	}

	// rotation get
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(DegreesToRadians(90.0f), k90Degrees.DecomposeRotation());
	}

	// rotation set
	{
		Matrix2D m = Matrix2D::Identity();

		m.SetRotation(DegreesToRadians(90.0f));
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(k90Degrees, m, 1e-6f);
	}
}

/**
 * Test Matrix2D Transformation methods.
 */
void Matrix2DTest::TestTransformation()
{
	// identity
	{
		// Direction transformation
		SEOUL_UNITTESTING_ASSERT(Vector2D::UnitX() == Matrix2D::Transform(Matrix2D::Identity(), Vector2D::UnitX()));
		SEOUL_UNITTESTING_ASSERT(Vector2D::UnitY() == Matrix2D::Transform(Matrix2D::Identity(), Vector2D::UnitY()));
	}

	// 90-degree rotation.
	{
		SEOUL_UNITTESTING_ASSERT(( Vector2D::UnitY()).Equals(Matrix2D::Transform(k90Degrees, Vector2D::UnitX()), 1e-6f));
		SEOUL_UNITTESTING_ASSERT((-Vector2D::UnitX()).Equals(Matrix2D::Transform(k90Degrees, Vector2D::UnitY()), 1e-6f));
	}
}

/**
 * Test utility functions, mostly for creating new Matrix2Ds
 * of various types from various data.
 */
void Matrix2DTest::TestUtilities()
{
	// rotation
	{
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(k90Degrees, Matrix2D::CreateRotationFromDegrees(90.0f), 1e-6f);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(k90Degrees, Matrix2D::CreateRotation(DegreesToRadians(90.0f)), 1e-6f);
	}

	// scale
	{
		SEOUL_UNITTESTING_ASSERT(Matrix2D::CreateScale(4.0f) ==
			Matrix2D(
				4.0f, 0.0f,
				0.0f, 4.0f));

		SEOUL_UNITTESTING_ASSERT(Matrix2D::CreateScale(3.0f, 4.0f) ==
			Matrix2D(
				3.0f, 0.0f,
				0.0f, 4.0f));

		SEOUL_UNITTESTING_ASSERT(Matrix2D::CreateScale(Vector2D(3.0f, 4.0f)) ==
			Matrix2D(
				3.0f, 0.0f,
				0.0f, 4.0f));
	}

	// decompose
	{
		Matrix2D mPreRotation;
		Matrix2D mRotation;

		SEOUL_UNITTESTING_ASSERT(Matrix2D::Decompose(
			Matrix2D::CreateRotationFromDegrees(90.0f) *
			Matrix2D::CreateScale(3.0f, 4.0f), mPreRotation, mRotation));

		SEOUL_UNITTESTING_ASSERT_EQUAL(
			Matrix2D::CreateRotationFromDegrees(90.0f) *
			Matrix2D::CreateScale(3.0f, 4.0f),
			mRotation * mPreRotation);

		SEOUL_UNITTESTING_ASSERT(mPreRotation.Equals(
			Matrix2D::CreateScale(3.0f, 4.0f), 1e-6f));

		SEOUL_UNITTESTING_ASSERT(mRotation.Equals(
			Matrix2D::CreateRotationFromDegrees(90.0f)));
	}

	// decompose (many rotations)
	{
		Matrix2D mPreRotation;
		Matrix2D mRotation;

		for (Int i = -180; i <= 180; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(Matrix2D::Decompose(
				Matrix2D::CreateRotationFromDegrees((Float)i) *
				Matrix2D::CreateScale(3.0f, 4.0f), mPreRotation, mRotation));

			SEOUL_UNITTESTING_ASSERT(EqualDegrees(
				(Float)i,
				RadiansToDegrees(Matrix2D::CreateRotationFromDegrees((Float)i).DecomposeRotation()),
				1e-4f));

			SEOUL_UNITTESTING_ASSERT(EqualRadians(
				DegreesToRadians((Float)i),
				Matrix2D::CreateRotationFromDegrees((Float)i).DecomposeRotation(),
				1e-6f));

			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(
				Matrix2D::CreateRotationFromDegrees((Float)i) *
				Matrix2D::CreateScale(3.0f, 4.0f),
				mRotation * mPreRotation, 1e-6f);

			SEOUL_UNITTESTING_ASSERT(mPreRotation.Equals(
				Matrix2D::CreateScale(3.0f, 4.0f), 1e-6f));

			SEOUL_UNITTESTING_ASSERT(mRotation.Equals(
				Matrix2D::CreateRotationFromDegrees((Float)i), 1e-6f));
		}
	}

	// decompose (negative scale X)
	{
		Matrix2D mPreRotation;
		Matrix2D mRotation;

		SEOUL_UNITTESTING_ASSERT(Matrix2D::Decompose(
			Matrix2D::CreateRotationFromDegrees(90.0f) *
			Matrix2D::CreateScale(-3.0f, 4.0f), mPreRotation, mRotation));

		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(
			Matrix2D::CreateRotationFromDegrees(90.0f) *
			Matrix2D::CreateScale(-3.0f, 4.0f),
			mRotation * mPreRotation, 1e-6f);

		SEOUL_UNITTESTING_ASSERT(mPreRotation.Equals(
			Matrix2D::CreateScale(-3.0f, 4.0f), 1e-6f));

		SEOUL_UNITTESTING_ASSERT(mRotation.Equals(
			Matrix2D::CreateRotationFromDegrees(90.0f)));
	}

	// decompose (negative scale Y)
	{
		Matrix2D mPreRotation;
		Matrix2D mRotation;

		SEOUL_UNITTESTING_ASSERT(Matrix2D::Decompose(
			Matrix2D::CreateRotationFromDegrees(90.0f) *
			Matrix2D::CreateScale(3.0f, -4.0f), mPreRotation, mRotation));

		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(
			Matrix2D::CreateRotationFromDegrees(90.0f) *
			Matrix2D::CreateScale(3.0f, -4.0f),
			mRotation * mPreRotation, 1e-6f);

		// It is impossible to tell the difference between reflection on
		// a particular axis and reflection on a different axis with
		// a corrective rotation, so Matrix2D::Decompose() is
		// expected to always apply reflection to -X (note the
		// sign differences below).

		SEOUL_UNITTESTING_ASSERT(mPreRotation.Equals(
			Matrix2D::CreateScale(-3.0f, 4.0f), 1e-6f));

		SEOUL_UNITTESTING_ASSERT(mRotation.Equals(
			Matrix2D::CreateRotationFromDegrees(-90.0f)));
	}

	// decompose (negative scale X, many rotations.)
	{
		Matrix2D mPreRotation;
		Matrix2D mRotation;

		for (Int i = -180; i <= 180; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(Matrix2D::Decompose(
				Matrix2D::CreateRotationFromDegrees((Float)i) *
				Matrix2D::CreateScale(-3.0f, 4.0f), mPreRotation, mRotation));

			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(
				Matrix2D::CreateRotationFromDegrees((Float)i) *
				Matrix2D::CreateScale(-3.0f, 4.0f),
				mRotation * mPreRotation, 1e-6f);

			SEOUL_UNITTESTING_ASSERT(mPreRotation.Equals(
				Matrix2D::CreateScale(-3.0f, 4.0f), 1e-6f));

			SEOUL_UNITTESTING_ASSERT(mRotation.Equals(
				Matrix2D::CreateRotationFromDegrees((Float)i)));
		}
	}

	// decompose (negative scale Y, many rotations.)
	{
		Matrix2D mPreRotation;
		Matrix2D mRotation;

		for (Int i = -180; i <= 180; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(Matrix2D::Decompose(
				Matrix2D::CreateRotationFromDegrees((Float)i) *
				Matrix2D::CreateScale(3.0f, -4.0f), mPreRotation, mRotation));

			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(
				Matrix2D::CreateRotationFromDegrees((Float)i) *
				Matrix2D::CreateScale(3.0f, -4.0f),
				mRotation * mPreRotation, 1e-6f);

			// It is impossible to tell the difference between reflection on
			// a particular axis and reflection on a different axis with
			// a corrective rotation, so Matrix2D::Decompose() is
			// expected to always apply reflection to -X (note the
			// sign differences below).

			SEOUL_UNITTESTING_ASSERT(mPreRotation.Equals(
				Matrix2D::CreateScale(-3.0f, 4.0f), 1e-6f));

			SEOUL_UNITTESTING_ASSERT(mRotation.Equals(
				Matrix2D::CreateRotationFromDegrees((Float)i - 180.0f), 1e-6f));
		}
	}

	// lerp
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			Matrix2D(
				1, 2,
				5, 6),
			Matrix2D::Lerp(
				Matrix2D(
					0, 1,
					4, 5),
				Matrix2D(
					2, 3,
					6, 7),
				0.5f));
	}
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
