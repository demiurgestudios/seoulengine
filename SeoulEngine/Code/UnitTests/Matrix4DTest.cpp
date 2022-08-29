/**
 * \file Matrix4DTest.cpp
 * \brief Unit tests for the Matrix4D class. Matrix4D is the work horse
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
#include "Matrix3x4.h"
#include "Matrix4DTest.h"
#include "Matrix4D.h"
#include "ReflectionDefine.h"
#include "UnitTesting.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(Matrix4DTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestMethods)
	SEOUL_METHOD(TestTransformation)
	SEOUL_METHOD(TestUtilities)
SEOUL_END_TYPE()

static const Matrix4D k90DegreesX(
	1.0f, 0.0f,  0.0f, 0.0f,
	0.0f, 0.0f, -1.0f, 0.0f,
	0.0f, 1.0f,  0.0f, 0.0f,
	0.0f, 0.0f,  0.0f, 1.0f);
static const Matrix4D k90DegreesY(
	 0.0f, 0.0f, 1.0f, 0.0f,
	 0.0f, 1.0f, 0.0f, 0.0f,
	-1.0f, 0.0f, 0.0f, 0.0f,
	 0.0f, 0.0f, 0.0f, 1.0f);
static const Matrix4D k90DegreesZ(
	0.0f, -1.0f, 0.0f, 0.0f,
	1.0f,  0.0f, 0.0f, 0.0f,
	0.0f,  0.0f, 1.0f, 0.0f,
	0.0f,  0.0f, 0.0f, 1.0f);

static const Quaternion kQuaternion90DegreesX(0.707106781f, 0.0f, 0.0f, 0.707106781f);
static const Quaternion kQuaternion90DegreesY(0.0f, 0.707106781f, 0.0f, 0.707106781f);
static const Quaternion kQuaternion90DegreesZ(0.0f, 0.0f, 0.707106781f, 0.707106781f);

/**
 * Test most of the member methods of the Matrix4D class.
 */
void Matrix4DTest::TestMethods()
{
	// zero
	{
		SEOUL_UNITTESTING_ASSERT(
			// row 0
			0.0f == Matrix4D::Zero().M00 &&
			0.0f == Matrix4D::Zero().M01 &&
			0.0f == Matrix4D::Zero().M02 &&
			0.0f == Matrix4D::Zero().M03 &&

			// row 1
			0.0f == Matrix4D::Zero().M10 &&
			0.0f == Matrix4D::Zero().M11 &&
			0.0f == Matrix4D::Zero().M12 &&
			0.0f == Matrix4D::Zero().M13 &&

			// row 2
			0.0f == Matrix4D::Zero().M20 &&
			0.0f == Matrix4D::Zero().M21 &&
			0.0f == Matrix4D::Zero().M22 &&
			0.0f == Matrix4D::Zero().M23 &&

			// row 3
			0.0f == Matrix4D::Zero().M30 &&
			0.0f == Matrix4D::Zero().M31 &&
			0.0f == Matrix4D::Zero().M32 &&
			0.0f == Matrix4D::Zero().M33);
	}

	// identity
	{
		SEOUL_UNITTESTING_ASSERT(
			// row 0
			1.0f == Matrix4D::Identity().M00 &&
			0.0f == Matrix4D::Identity().M01 &&
			0.0f == Matrix4D::Identity().M02 &&
			0.0f == Matrix4D::Identity().M03 &&

			// row 1
			0.0f == Matrix4D::Identity().M10 &&
			1.0f == Matrix4D::Identity().M11 &&
			0.0f == Matrix4D::Identity().M12 &&
			0.0f == Matrix4D::Identity().M13 &&

			// row 2
			0.0f == Matrix4D::Identity().M20 &&
			0.0f == Matrix4D::Identity().M21 &&
			1.0f == Matrix4D::Identity().M22 &&
			0.0f == Matrix4D::Identity().M23 &&

			// row 3
			0.0f == Matrix4D::Identity().M30 &&
			0.0f == Matrix4D::Identity().M31 &&
			0.0f == Matrix4D::Identity().M32 &&
			1.0f == Matrix4D::Identity().M33);
	}

	// tolerant equality
	{
		Matrix4D m0(
			3.0f,  4.0f,  5.0f,  6.0f,
			7.0f,  8.0f,  9.0f,  10.0f,
			11.0f, 12.0f, 13.0f, 14.0f,
			15.0f, 16.0f, 17.0f, 18.0f);
		Matrix4D m1(
			3.000999f,  4.000999f,  5.000999f,  6.000999f,
			7.000999f,  8.000999f,  9.000999f,  10.000999f,
			11.000999f, 12.000999f, 13.000999f, 14.000999f,
			15.000999f, 16.000999f, 17.000999f, 18.000999f);

		SEOUL_UNITTESTING_ASSERT(!m0.Equals(m1, 1e-4f));
		SEOUL_UNITTESTING_ASSERT(m0.Equals(m1, 1e-3f));
	}

	// tolerant zero
	{
		Matrix4D m(
			0.000999f, 0.000999f, 0.000999f, 0.000999f,
			0.000999f, 0.000999f, 0.000999f, 0.000999f,
			0.000999f, 0.000999f, 0.000999f, 0.000999f,
			0.000999f, 0.000999f, 0.000999f, 0.000999f);

		SEOUL_UNITTESTING_ASSERT(!m.IsZero(1e-4f));
		SEOUL_UNITTESTING_ASSERT(m.IsZero(1e-3f));
	}

	// default constructor
	{
		Matrix4D m;

		// Per element verification
		SEOUL_UNITTESTING_ASSERT(
			0.0f == m.M00 && 0.0f == m.M01 && 0.0f == m.M02 && 0.0f == m.M03 &&
			0.0f == m.M10 && 0.0f == m.M11 && 0.0f == m.M12 && 0.0f == m.M13 &&
			0.0f == m.M20 && 0.0f == m.M21 && 0.0f == m.M22 && 0.0f == m.M23 &&
			0.0f == m.M30 && 0.0f == m.M31 && 0.0f == m.M32 && 0.0f == m.M33);
	}

	// single value constructor
	{
		Matrix4D m(13.0f);

		// Per element verification
		SEOUL_UNITTESTING_ASSERT(
			13.0f == m.M00 && 13.0f == m.M01 && 13.0f == m.M02 && 13.0f == m.M03 &&
			13.0f == m.M10 && 13.0f == m.M11 && 13.0f == m.M12 && 13.0f == m.M13 &&
			13.0f == m.M20 && 13.0f == m.M21 && 13.0f == m.M22 && 13.0f == m.M23 &&
			13.0f == m.M30 && 13.0f == m.M31 && 13.0f == m.M32 && 13.0f == m.M33);
	}

	// componentwise constructor
	{
		Matrix4D m(
			3.0f,  4.0f,  5.0f,  6.0f,
			7.0f,  8.0f,  9.0f,  10.0f,
			11.0f, 12.0f, 13.0f, 14.0f,
			15.0f, 16.0f, 17.0f, 18.0f);

		// Per element verification
		SEOUL_UNITTESTING_ASSERT(
			3.0f  == m.M00 && 4.0f  == m.M01 && 5.0f  == m.M02 && 6.0f  == m.M03 &&
			7.0f  == m.M10 && 8.0f  == m.M11 && 9.0f  == m.M12 && 10.0f == m.M13 &&
			11.0f == m.M20 && 12.0f == m.M21 && 13.0f == m.M22 && 14.0f == m.M23 &&
			15.0f == m.M30 && 16.0f == m.M31 && 17.0f == m.M32 && 18.0f == m.M33);

		// Array verification - ensure column major storage
		SEOUL_UNITTESTING_ASSERT(
			3.0f  == m.GetData()[0] && 4.0f  == m.GetData()[4] && 5.0f  == m.GetData()[8]  && 6.0f  == m.GetData()[12] &&
			7.0f  == m.GetData()[1] && 8.0f  == m.GetData()[5] && 9.0f  == m.GetData()[9]  && 10.0f == m.GetData()[13] &&
			11.0f == m.GetData()[2] && 12.0f == m.GetData()[6] && 13.0f == m.GetData()[10] && 14.0f == m.GetData()[14] &&
			15.0f == m.GetData()[3] && 16.0f == m.GetData()[7] && 17.0f == m.GetData()[11] && 18.0f == m.GetData()[15]);

		// Per-column verification
		SEOUL_UNITTESTING_ASSERT(
			3.0f  == m.GetColumn(0).X && 4.0f  == m.GetColumn(1).X && 5.0f  == m.GetColumn(2).X && 6.0f  == m.GetColumn(3).X &&
			7.0f  == m.GetColumn(0).Y && 8.0f  == m.GetColumn(1).Y && 9.0f  == m.GetColumn(2).Y && 10.0f == m.GetColumn(3).Y &&
			11.0f == m.GetColumn(0).Z && 12.0f == m.GetColumn(1).Z && 13.0f == m.GetColumn(2).Z && 14.0f == m.GetColumn(3).Z &&
			15.0f == m.GetColumn(0).W && 16.0f == m.GetColumn(1).W && 17.0f == m.GetColumn(2).W && 18.0f == m.GetColumn(3).W);
	}

	// copy constructor
	{
		Matrix4D m(Matrix4D(
			3.0f,  4.0f,  5.0f,  6.0f,
			7.0f,  8.0f,  9.0f,  10.0f,
			11.0f, 12.0f, 13.0f, 14.0f,
			15.0f, 16.0f, 17.0f, 18.0f));

		SEOUL_UNITTESTING_ASSERT(
			3.0f  == m.M00 && 4.0f  == m.M01 && 5.0f  == m.M02 && 6.0f  == m.M03 &&
			7.0f  == m.M10 && 8.0f  == m.M11 && 9.0f  == m.M12 && 10.0f == m.M13 &&
			11.0f == m.M20 && 12.0f == m.M21 && 13.0f == m.M22 && 14.0f == m.M23 &&
			15.0f == m.M30 && 16.0f == m.M31 && 17.0f == m.M32 && 18.0f == m.M33);
	}

	// matrix3x4 constructor
	{
		Matrix4D m(Matrix3x4(
			3.0f, 4.0f, 5.0f, 6.0f,
			7.0f, 8.0f, 9.0f, 10.0f,
			11.0f, 12.0f, 13.0f, 14.0f));

		SEOUL_UNITTESTING_ASSERT(
			3.0f  == m.M00 && 4.0f  == m.M01 && 5.0f  == m.M02 && 6.0f  == m.M03 &&
			7.0f  == m.M10 && 8.0f  == m.M11 && 9.0f  == m.M12 && 10.0f == m.M13 &&
			11.0f == m.M20 && 12.0f == m.M21 && 13.0f == m.M22 && 14.0f == m.M23 &&
			0.0f  == m.M30 && 0.0f  == m.M31 && 0.0f  == m.M32 && 1.0f  == m.M33);
	}

	// assignment
	{
		Matrix4D m = Matrix4D(
			3.0f,  4.0f,  5.0f,  6.0f,
			7.0f,  8.0f,  9.0f,  10.0f,
			11.0f, 12.0f, 13.0f, 14.0f,
			15.0f, 16.0f, 17.0f, 18.0f);

		SEOUL_UNITTESTING_ASSERT(
			3.0f  == m.M00 && 4.0f  == m.M01 && 5.0f  == m.M02 && 6.0f  == m.M03 &&
			7.0f  == m.M10 && 8.0f  == m.M11 && 9.0f  == m.M12 && 10.0f == m.M13 &&
			11.0f == m.M20 && 12.0f == m.M21 && 13.0f == m.M22 && 14.0f == m.M23 &&
			15.0f == m.M30 && 16.0f == m.M31 && 17.0f == m.M32 && 18.0f == m.M33);
	}

	// self-addition
	{
		Matrix4D m0(
			3.0f,  4.0f,  5.0f,  6.0f,
			7.0f,  8.0f,  9.0f,  10.0f,
			11.0f, 12.0f, 13.0f, 14.0f,
			15.0f, 16.0f, 17.0f, 18.0f);

		{
			Matrix4D m = (m0 + m0);
			SEOUL_UNITTESTING_ASSERT(
				6.0f  == m.M00 && 8.0f  == m.M01 && 10.0f == m.M02 && 12.0f == m.M03 &&
				14.0f == m.M10 && 16.0f == m.M11 && 18.0f == m.M12 && 20.0f == m.M13 &&
				22.0f == m.M20 && 24.0f == m.M21 && 26.0f == m.M22 && 28.0f == m.M23 &&
				30.0f == m.M30 && 32.0f == m.M31 && 34.0f == m.M32 && 36.0f == m.M33);
		}

		{
			Matrix4D m = m0;
			m += m;
			SEOUL_UNITTESTING_ASSERT(
				6.0f  == m.M00 && 8.0f  == m.M01 && 10.0f == m.M02 && 12.0f == m.M03 &&
				14.0f == m.M10 && 16.0f == m.M11 && 18.0f == m.M12 && 20.0f == m.M13 &&
				22.0f == m.M20 && 24.0f == m.M21 && 26.0f == m.M22 && 28.0f == m.M23 &&
				30.0f == m.M30 && 32.0f == m.M31 && 34.0f == m.M32 && 36.0f == m.M33);
		}
	}

	// addition
	{
		Matrix4D m0(
			3.0f,  4.0f,  5.0f,  6.0f,
			7.0f,  8.0f,  9.0f,  10.0f,
			11.0f, 12.0f, 13.0f, 14.0f,
			15.0f, 16.0f, 17.0f, 18.0f);
		Matrix4D m1(
			-4.0f,  -5.0f,  -6.0f,  -7.0f,
			-8.0f,  -9.0f,  -10.0f, -11.0f,
			-12.0f, -13.0f, -14.0f, -15.0f,
			-16.0f, -17.0f, -18.0f, -19.0f);

		{
			Matrix4D m = (m0 + m1);
			SEOUL_UNITTESTING_ASSERT(
				-1.0f == m.M00 && -1.0f == m.M01 && -1.0f == m.M02 && -1.0f == m.M03 &&
				-1.0f == m.M10 && -1.0f == m.M11 && -1.0f == m.M12 && -1.0f == m.M13 &&
				-1.0f == m.M20 && -1.0f == m.M21 && -1.0f == m.M22 && -1.0f == m.M23 &&
				-1.0f == m.M30 && -1.0f == m.M31 && -1.0f == m.M32 && -1.0f == m.M33);
		}

		{
			Matrix4D m = m0;
			m += m1;
			SEOUL_UNITTESTING_ASSERT(
				-1.0f == m.M00 && -1.0f == m.M01 && -1.0f == m.M02 && -1.0f == m.M03 &&
				-1.0f == m.M10 && -1.0f == m.M11 && -1.0f == m.M12 && -1.0f == m.M13 &&
				-1.0f == m.M20 && -1.0f == m.M21 && -1.0f == m.M22 && -1.0f == m.M23 &&
				-1.0f == m.M30 && -1.0f == m.M31 && -1.0f == m.M32 && -1.0f == m.M33);
		}
	}

	// self-subtraction
	{
		Matrix4D m0(
			3.0f,  4.0f,  5.0f,  6.0f,
			7.0f,  8.0f,  9.0f,  10.0f,
			11.0f, 12.0f, 13.0f, 14.0f,
			15.0f, 16.0f, 17.0f, 18.0f);

		{
			Matrix4D m = (m0 - m0);
			SEOUL_UNITTESTING_ASSERT(
				0.0f == m.M00 && 0.0f == m.M01 && 0.0f == m.M02 && 0.0f == m.M03 &&
				0.0f == m.M10 && 0.0f == m.M11 && 0.0f == m.M12 && 0.0f == m.M13 &&
				0.0f == m.M20 && 0.0f == m.M21 && 0.0f == m.M22 && 0.0f == m.M23 &&
				0.0f == m.M30 && 0.0f == m.M31 && 0.0f == m.M32 && 0.0f == m.M33);
		}

		{
			Matrix4D m = m0;
			m -= m;
			SEOUL_UNITTESTING_ASSERT(
				0.0f == m.M00 && 0.0f == m.M01 && 0.0f == m.M02 && 0.0f == m.M03 &&
				0.0f == m.M10 && 0.0f == m.M11 && 0.0f == m.M12 && 0.0f == m.M13 &&
				0.0f == m.M20 && 0.0f == m.M21 && 0.0f == m.M22 && 0.0f == m.M23 &&
				0.0f == m.M30 && 0.0f == m.M31 && 0.0f == m.M32 && 0.0f == m.M33);
		}
	}

	// subtraction
	{
		Matrix4D m0(
			3.0f,  4.0f,  5.0f,  6.0f,
			7.0f,  8.0f,  9.0f,  10.0f,
			11.0f, 12.0f, 13.0f, 14.0f,
			15.0f, 16.0f, 17.0f, 18.0f);
		Matrix4D m1(
			4.0f,  5.0f,  6.0f,  7.0f,
			8.0f,  9.0f,  10.0f, 11.0f,
			12.0f, 13.0f, 14.0f, 15.0f,
			16.0f, 17.0f, 18.0f, 19.0f);

		{
			Matrix4D m = (m0 - m1);
			SEOUL_UNITTESTING_ASSERT(
				-1.0f == m.M00 && -1.0f == m.M01 && -1.0f == m.M02 && -1.0f == m.M03 &&
				-1.0f == m.M10 && -1.0f == m.M11 && -1.0f == m.M12 && -1.0f == m.M13 &&
				-1.0f == m.M20 && -1.0f == m.M21 && -1.0f == m.M22 && -1.0f == m.M23 &&
				-1.0f == m.M30 && -1.0f == m.M31 && -1.0f == m.M32 && -1.0f == m.M33);
		}

		{
			Matrix4D m = m0;
			m -= m1;
			SEOUL_UNITTESTING_ASSERT(
				-1.0f == m.M00 && -1.0f == m.M01 && -1.0f == m.M02 && -1.0f == m.M03 &&
				-1.0f == m.M10 && -1.0f == m.M11 && -1.0f == m.M12 && -1.0f == m.M13 &&
				-1.0f == m.M20 && -1.0f == m.M21 && -1.0f == m.M22 && -1.0f == m.M23 &&
				-1.0f == m.M30 && -1.0f == m.M31 && -1.0f == m.M32 && -1.0f == m.M33);
		}
	}

	// negation
	{
		Matrix4D m0(
			3.0f,  4.0f,  5.0f,  6.0f,
			7.0f,  8.0f,  9.0f,  10.0f,
			11.0f, 12.0f, 13.0f, 14.0f,
			15.0f, 16.0f, 17.0f, 18.0f);
		Matrix4D m = (-m0);

		SEOUL_UNITTESTING_ASSERT(
			-3.0f  == m.M00 && -4.0f  == m.M01 && -5.0f  == m.M02 && -6.0f  == m.M03 &&
			-7.0f  == m.M10 && -8.0f  == m.M11 && -9.0f  == m.M12 && -10.0f == m.M13 &&
			-11.0f == m.M20 && -12.0f == m.M21 && -13.0f == m.M22 && -14.0f == m.M23 &&
			-15.0f == m.M30 && -16.0f == m.M31 && -17.0f == m.M32 && -18.0f == m.M33);
	}

	// matrix self-multiplication
	{
		Matrix4D m0(
			3.0f,  4.0f,  5.0f,  6.0f,
			7.0f,  8.0f,  9.0f,  10.0f,
			11.0f, 12.0f, 13.0f, 14.0f,
			15.0f, 16.0f, 17.0f, 18.0f);

		{
			Matrix4D m = (m0 * m0);
			SEOUL_UNITTESTING_ASSERT(
				182.0f == m.M00 && 200.0f == m.M01 && 218.0f == m.M02 && 236.0f == m.M03 &&
				326.0f == m.M10 && 360.0f == m.M11 && 394.0f == m.M12 && 428.0f == m.M13 &&
				470.0f == m.M20 && 520.0f == m.M21 && 570.0f == m.M22 && 620.0f == m.M23 &&
				614.0f == m.M30 && 680.0f == m.M31 && 746.0f == m.M32 && 812.0f == m.M33);
		}

		{
			Matrix4D m = m0;
			m *= m;
			SEOUL_UNITTESTING_ASSERT(
				182.0f == m.M00 && 200.0f == m.M01 && 218.0f == m.M02 && 236.0f == m.M03 &&
				326.0f == m.M10 && 360.0f == m.M11 && 394.0f == m.M12 && 428.0f == m.M13 &&
				470.0f == m.M20 && 520.0f == m.M21 && 570.0f == m.M22 && 620.0f == m.M23 &&
				614.0f == m.M30 && 680.0f == m.M31 && 746.0f == m.M32 && 812.0f == m.M33);
		}
	}

	// matrix multiplication
	{
		Matrix4D m0(
			3.0f,  4.0f,  5.0f,  6.0f,
			7.0f,  8.0f,  9.0f,  10.0f,
			11.0f, 12.0f, 13.0f, 14.0f,
			15.0f, 16.0f, 17.0f, 18.0f);
		Matrix4D m1(
			4.0f,  5.0f,  6.0f,  7.0f,
			8.0f,  9.0f,  10.0f, 11.0f,
			12.0f, 13.0f, 14.0f, 15.0f,
			16.0f, 17.0f, 18.0f, 19.0f);

		{
			Matrix4D m = (m0 * m1);
			SEOUL_UNITTESTING_ASSERT(
				200.0f == m.M00 && 218.0f == m.M01 && 236.0f == m.M02 && 254.0f == m.M03 &&
				360.0f == m.M10 && 394.0f == m.M11 && 428.0f == m.M12 && 462.0f == m.M13 &&
				520.0f == m.M20 && 570.0f == m.M21 && 620.0f == m.M22 && 670.0f == m.M23 &&
				680.0f == m.M30 && 746.0f == m.M31 && 812.0f == m.M32 && 878.0f == m.M33);
		}

		{
			Matrix4D m = m0;
			m *= m1;
			SEOUL_UNITTESTING_ASSERT(
				200.0f == m.M00 && 218.0f == m.M01 && 236.0f == m.M02 && 254.0f == m.M03 &&
				360.0f == m.M10 && 394.0f == m.M11 && 428.0f == m.M12 && 462.0f == m.M13 &&
				520.0f == m.M20 && 570.0f == m.M21 && 620.0f == m.M22 && 670.0f == m.M23 &&
				680.0f == m.M30 && 746.0f == m.M31 && 812.0f == m.M32 && 878.0f == m.M33);
		}

		{
			Matrix4D m = (m1 * m0);
			SEOUL_UNITTESTING_ASSERT(
				218.0f == m.M00 && 240.0f == m.M01 && 262.0f == m.M02 && 284.0f == m.M03 &&
				362.0f == m.M10 && 400.0f == m.M11 && 438.0f == m.M12 && 476.0f == m.M13 &&
				506.0f == m.M20 && 560.0f == m.M21 && 614.0f == m.M22 && 668.0f == m.M23 &&
				650.0f == m.M30 && 720.0f == m.M31 && 790.0f == m.M32 && 860.0f == m.M33);
		}

		{
			Matrix4D m = m1;
			m *= m0;
			SEOUL_UNITTESTING_ASSERT(
				218.0f == m.M00 && 240.0f == m.M01 && 262.0f == m.M02 && 284.0f == m.M03 &&
				362.0f == m.M10 && 400.0f == m.M11 && 438.0f == m.M12 && 476.0f == m.M13 &&
				506.0f == m.M20 && 560.0f == m.M21 && 614.0f == m.M22 && 668.0f == m.M23 &&
				650.0f == m.M30 && 720.0f == m.M31 && 790.0f == m.M32 && 860.0f == m.M33);
		}
	}

	// scalar multiplication
	{
		Matrix4D m0(
			3.0f,  4.0f,  5.0f,  6.0f,
			7.0f,  8.0f,  9.0f,  10.0f,
			11.0f, 12.0f, 13.0f, 14.0f,
			15.0f, 16.0f, 17.0f, 18.0f);

		{
			Matrix4D m = (m0 * 3.0f);
			SEOUL_UNITTESTING_ASSERT(
				9.0f  == m.M00 && 12.0f == m.M01 && 15.0f == m.M02 && 18.0f == m.M03 &&
				21.0f == m.M10 && 24.0f == m.M11 && 27.0f == m.M12 && 30.0f == m.M13 &&
				33.0f == m.M20 && 36.0f == m.M21 && 39.0f == m.M22 && 42.0f == m.M23 &&
				45.0f == m.M30 && 48.0f == m.M31 && 51.0f == m.M32 && 54.0f == m.M33);
		}

		{
			Matrix4D m = (3.0f * m0);
			SEOUL_UNITTESTING_ASSERT(
				9.0f  == m.M00 && 12.0f == m.M01 && 15.0f == m.M02 && 18.0f == m.M03 &&
				21.0f == m.M10 && 24.0f == m.M11 && 27.0f == m.M12 && 30.0f == m.M13 &&
				33.0f == m.M20 && 36.0f == m.M21 && 39.0f == m.M22 && 42.0f == m.M23 &&
				45.0f == m.M30 && 48.0f == m.M31 && 51.0f == m.M32 && 54.0f == m.M33);
		}

		{
			Matrix4D m = m0;
			m *= 3.0f;
			SEOUL_UNITTESTING_ASSERT(
				9.0f  == m.M00 && 12.0f == m.M01 && 15.0f == m.M02 && 18.0f == m.M03 &&
				21.0f == m.M10 && 24.0f == m.M11 && 27.0f == m.M12 && 30.0f == m.M13 &&
				33.0f == m.M20 && 36.0f == m.M21 && 39.0f == m.M22 && 42.0f == m.M23 &&
				45.0f == m.M30 && 48.0f == m.M31 && 51.0f == m.M32 && 54.0f == m.M33);
		}
	}

	// scalar division
	{
		Matrix4D m0(
			3.0f,  9.0f,  12.0f, 15.0f,
			18.0f, 21.0f, 24.0f, 27.0f,
			30.0f, 33.0f, 36.0f, 39.0f,
			42.0f, 45.0f, 48.0f, 51.0f);

		{
			Matrix4D m = (m0 / 3.0f);
			SEOUL_UNITTESTING_ASSERT(
				1.0f  == m.M00 && 3.0f  == m.M01 && 4.0f  == m.M02 && 5.0f  == m.M03 &&
				6.0f  == m.M10 && 7.0f  == m.M11 && 8.0f  == m.M12 && 9.0f  == m.M13 &&
				10.0f == m.M20 && 11.0f == m.M21 && 12.0f == m.M22 && 13.0f == m.M23 &&
				14.0f == m.M30 && 15.0f == m.M31 && 16.0f == m.M32 && 17.0f == m.M33);
		}

		{
			Matrix4D m = m0;
			m /= 3.0f;
			SEOUL_UNITTESTING_ASSERT(
				1.0f  == m.M00 && 3.0f  == m.M01 && 4.0f  == m.M02 && 5.0f  == m.M03 &&
				6.0f  == m.M10 && 7.0f  == m.M11 && 8.0f  == m.M12 && 9.0f  == m.M13 &&
				10.0f == m.M20 && 11.0f == m.M21 && 12.0f == m.M22 && 13.0f == m.M23 &&
				14.0f == m.M30 && 15.0f == m.M31 && 16.0f == m.M32 && 17.0f == m.M33);
		}
	}

	// self-equality
	{
		Matrix4D m(
			3.0f,  4.0f,  5.0f,  6.0f,
			7.0f,  8.0f,  9.0f,  10.0f,
			11.0f, 12.0f, 13.0f, 14.0f,
			15.0f, 16.0f, 17.0f, 18.0f);

		SEOUL_UNITTESTING_ASSERT(m == m);
	}

	// equality
	{
		Matrix4D m0(
			3.0f,  4.0f,  5.0f,  6.0f,
			7.0f,  8.0f,  9.0f,  10.0f,
			11.0f, 12.0f, 13.0f, 14.0f,
			15.0f, 16.0f, 17.0f, 18.0f);

		Matrix4D m1(
			3.0f,  4.0f,  5.0f,  6.0f,
			7.0f,  8.0f,  9.0f,  10.0f,
			11.0f, 12.0f, 13.0f, 14.0f,
			15.0f, 16.0f, 17.0f, 18.0f);

		SEOUL_UNITTESTING_ASSERT(m0 == m1);
	}

	// inequality
	{
		Matrix4D m0(
			3.0f,  4.0f,  5.0f,  6.0f,
			7.0f,  8.0f,  9.0f,  10.0f,
			11.0f, 12.0f, 13.0f, 14.0f,
			15.0f, 16.0f, 17.0f, 18.0f);

		Matrix4D m1(
			4.0f,  5.0f,  6.0f,  7.0f,
			8.0f,  9.0f,  10.0f, 11.0f,
			12.0f, 13.0f, 14.0f, 15.0f,
			16.0f, 17.0f, 18.0f, 19.0f);

		SEOUL_UNITTESTING_ASSERT(m0 != m1);
	}

	// diagonal
	{
		Matrix4D m(
			3.0f,  4.0f,  5.0f,  6.0f,
			7.0f,  8.0f,  9.0f,  10.0f,
			11.0f, 12.0f, 13.0f, 14.0f,
			15.0f, 16.0f, 17.0f, 18.0f);

		Vector4D v = m.GetDiagonal();

		SEOUL_UNITTESTING_ASSERT(
			3.0f == v.X && 8.0f == v.Y && 13.0f == v.Z && 18.0f == v.W);
	}

	// transpose
	{
		Matrix4D m(
			3.0f,  4.0f,  5.0f,  6.0f,
			7.0f,  8.0f,  9.0f,  10.0f,
			11.0f, 12.0f, 13.0f, 14.0f,
			15.0f, 16.0f, 17.0f, 18.0f);

		Matrix4D m1 = m.Transpose();

		// Verify the source is unchanged
		SEOUL_UNITTESTING_ASSERT(
			3.0f  == m.M00 && 4.0f  == m.M01 && 5.0f  == m.M02 && 6.0f  == m.M03 &&
			7.0f  == m.M10 && 8.0f  == m.M11 && 9.0f  == m.M12 && 10.0f == m.M13 &&
			11.0f == m.M20 && 12.0f == m.M21 && 13.0f == m.M22 && 14.0f == m.M23 &&
			15.0f == m.M30 && 16.0f == m.M31 && 17.0f == m.M32 && 18.0f == m.M33);

		SEOUL_UNITTESTING_ASSERT(
			3.0f == m1.M00 && 7.0f  == m1.M01 && 11.0f == m1.M02 && 15.0f == m1.M03 &&
			4.0f == m1.M10 && 8.0f  == m1.M11 && 12.0f == m1.M12 && 16.0f == m1.M13 &&
			5.0f == m1.M20 && 9.0f  == m1.M21 && 13.0f == m1.M22 && 17.0f == m1.M23 &&
			6.0f == m1.M30 && 10.0f == m1.M31 && 14.0f == m1.M32 && 18.0f == m1.M33);
	}

	// determinant
	{
		Matrix4D m(
			3.0f,  4.0f,  5.0f,  6.0f,
			7.0f,  3.0f,  9.0f,  10.0f,
			11.0f, 12.0f, 6.0f, 14.0f,
			15.0f, 16.0f, 17.0f, 18.0f);

		SEOUL_UNITTESTING_ASSERT(-1260.0f ==
			m.Determinant());
	}

	// inverse
	{
		Matrix4D m(
			1.0f,  4.0f,  5.0f,  6.0f,
			7.0f,  1.0f,  9.0f,  10.0f,
			11.0f, 12.0f, 1.0f, 14.0f,
			15.0f, 16.0f, 17.0f, 1.0f);

		Matrix4D m0 = m.Inverse();

		SEOUL_UNITTESTING_ASSERT((m * m0).Equals(Matrix4D::Identity(), 1e-3f));
		SEOUL_UNITTESTING_ASSERT((m0 * m).Equals(Matrix4D::Identity(), 1e-3f));
	}

	// orthonormal tests
	{
		SEOUL_UNITTESTING_ASSERT(k90DegreesX.IsOrthonormal());
		SEOUL_UNITTESTING_ASSERT(k90DegreesY.IsOrthonormal());
		SEOUL_UNITTESTING_ASSERT(k90DegreesZ.IsOrthonormal());

		SEOUL_UNITTESTING_ASSERT(
			k90DegreesX.OrthonormalInverse() == k90DegreesX.Transpose());
		SEOUL_UNITTESTING_ASSERT(
			k90DegreesY.OrthonormalInverse() == k90DegreesY.Transpose());
		SEOUL_UNITTESTING_ASSERT(
			k90DegreesZ.OrthonormalInverse() == k90DegreesZ.Transpose());

		SEOUL_UNITTESTING_ASSERT(!Matrix4D().IsOrthonormal());
		SEOUL_UNITTESTING_ASSERT(!Matrix4D(
			1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			1, 2, 3, 4).IsOrthonormal());
		SEOUL_UNITTESTING_ASSERT(Matrix4D::CreateTranslation(1, 2, 3).IsOrthonormal());
	}

	// accessors
	{
		Matrix4D m(
			3.0f,  4.0f,  5.0f,  6.0f,
			7.0f,  8.0f,  9.0f,  10.0f,
			11.0f, 12.0f, 13.0f, 14.0f,
			15.0f, 16.0f, 17.0f, 18.0f);

		// Per element accessor
		SEOUL_UNITTESTING_ASSERT(
			3.0f  == m(0, 0) && 4.0f  == m(0, 1) && 5.0f  == m(0, 2) && 6.0f  == m(0, 3) &&
			7.0f  == m(1, 0) && 8.0f  == m(1, 1) && 9.0f  == m(1, 2) && 10.0f == m(1, 3) &&
			11.0f == m(2, 0) && 12.0f == m(2, 1) && 13.0f == m(2, 2) && 14.0f == m(2, 3) &&
			15.0f == m(3, 0) && 16.0f == m(3, 1) && 17.0f == m(3, 2) && 18.0f == m(3, 3));

		// Row accessor
		SEOUL_UNITTESTING_ASSERT(
			3.0f  == m.GetRow(0).X && 4.0f  == m.GetRow(0).Y && 5.0f  == m.GetRow(0).Z && 6.0f  == m.GetRow(0).W &&
			7.0f  == m.GetRow(1).X && 8.0f  == m.GetRow(1).Y && 9.0f  == m.GetRow(1).Z && 10.0f == m.GetRow(1).W &&
			11.0f == m.GetRow(2).X && 12.0f == m.GetRow(2).Y && 13.0f == m.GetRow(2).Z && 14.0f == m.GetRow(2).W &&
			15.0f == m.GetRow(3).X && 16.0f == m.GetRow(3).Y && 17.0f == m.GetRow(3).Z && 18.0f == m.GetRow(3).W);

		// Column accessor
		SEOUL_UNITTESTING_ASSERT(
			3.0f  == m.GetColumn(0).X && 4.0f  == m.GetColumn(1).X && 5.0f  == m.GetColumn(2).X && 6.0f  == m.GetColumn(3).X &&
			7.0f  == m.GetColumn(0).Y && 8.0f  == m.GetColumn(1).Y && 9.0f  == m.GetColumn(2).Y && 10.0f == m.GetColumn(3).Y &&
			11.0f == m.GetColumn(0).Z && 12.0f == m.GetColumn(1).Z && 13.0f == m.GetColumn(2).Z && 14.0f == m.GetColumn(3).Z &&
			15.0f == m.GetColumn(0).W && 16.0f == m.GetColumn(1).W && 17.0f == m.GetColumn(2).W && 18.0f == m.GetColumn(3).W);
	}

	// unit axis tests
	{
		Matrix4D m(
			3.0f,  0.0f,  0.0f,  6.0f,
			0.0f,  8.0f,  0.0f,  10.0f,
			0.0f,  0.0f,  13.0f, 14.0f,
			15.0f, 16.0f, 17.0f, 18.0f);

		SEOUL_UNITTESTING_ASSERT(
			Vector3D::UnitX() == m.GetUnitAxis(0));

		SEOUL_UNITTESTING_ASSERT(
			Vector3D::UnitY() == m.GetUnitAxis(1));

		Vector3D vUnitX;
		Vector3D vUnitY;
		Vector3D vUnitZ;
		m.GetUnitAxes(vUnitX, vUnitY, vUnitZ);

		SEOUL_UNITTESTING_ASSERT(Vector3D::UnitX() == vUnitX);
		SEOUL_UNITTESTING_ASSERT(Vector3D::UnitY() == vUnitY);
		SEOUL_UNITTESTING_ASSERT(Vector3D::UnitZ() == vUnitZ);
	}

	// rotation and translation get
	{
		Matrix4D m(
			3.0f,  4.0f,  5.0f,  6.0f,
			7.0f,  8.0f,  9.0f,  10.0f,
			11.0f, 12.0f, 13.0f, 14.0f,
			15.0f, 16.0f, 17.0f, 18.0f);

		// Get rotation as Matrix3D.
		Matrix3D mTestRotation;
		m.GetRotation(mTestRotation);
		SEOUL_UNITTESTING_ASSERT(
			Matrix3D(
				3.0f, 4.0f, 5.0f,
				7.0f, 8.0f, 9.0f,
				11.0f, 12.0f, 13.0f) == mTestRotation);

		// Get translation.
		SEOUL_UNITTESTING_ASSERT(
			Vector3D(6.0f, 10.0f, 14.0f) == m.GetTranslation());

		// Rotation as quaternion
		SEOUL_UNITTESTING_ASSERT(
			kQuaternion90DegreesX.Equals(k90DegreesX.GetRotation(), 1e-3f));
		SEOUL_UNITTESTING_ASSERT(
			kQuaternion90DegreesY.Equals(k90DegreesY.GetRotation(), 1e-3f));
		SEOUL_UNITTESTING_ASSERT(
			kQuaternion90DegreesZ.Equals(k90DegreesZ.GetRotation(), 1e-3f));
	}

	// rotation and translation set
	{
		Matrix4D m = Matrix4D::Identity();

		m.SetRotation(Matrix3D::CreateRotationX(DegreesToRadians(90.0f)));
		SEOUL_UNITTESTING_ASSERT(Matrix4D::CreateRotationX(DegreesToRadians(90.0f)) == m);

		m.SetRotation(Matrix3D::CreateRotationY(DegreesToRadians(90.0f)));
		SEOUL_UNITTESTING_ASSERT(Matrix4D::CreateRotationY(DegreesToRadians(90.0f)) == m);

		m.SetRotation(Matrix3D::CreateRotationZ(DegreesToRadians(90.0f)));
		SEOUL_UNITTESTING_ASSERT(Matrix4D::CreateRotationZ(DegreesToRadians(90.0f)) == m);

		m.SetRotation(kQuaternion90DegreesX);
		SEOUL_UNITTESTING_ASSERT(Matrix4D::CreateRotationX(DegreesToRadians(90.0f)).Equals(m, 1e-3f));

		m.SetRotation(kQuaternion90DegreesY);
		SEOUL_UNITTESTING_ASSERT(Matrix4D::CreateRotationY(DegreesToRadians(90.0f)).Equals(m, 1e-3f));

		m.SetRotation(kQuaternion90DegreesZ);
		SEOUL_UNITTESTING_ASSERT(Matrix4D::CreateRotationZ(DegreesToRadians(90.0f)).Equals(m, 1e-3f));

		m.SetTranslation(Vector3D(3.0f, 4.0f, 5.0f));
		SEOUL_UNITTESTING_ASSERT(Vector3D(3.0f, 4.0f, 5.0f) == m.GetTranslation());
	}

	// SetColumn
	{
		Matrix4D m;
		SEOUL_UNITTESTING_ASSERT_EQUAL(m,
			Matrix4D(
				0, 0, 0, 0,
				0, 0, 0, 0,
				0, 0, 0, 0,
				0, 0, 0, 0));

		m.SetColumn(0, Vector4D(1, 2, 3, 4));
		SEOUL_UNITTESTING_ASSERT_EQUAL(m,
			Matrix4D(
				1, 2, 3, 4,
				0, 0, 0, 0,
				0, 0, 0, 0,
				0, 0, 0, 0).Transpose());

		m.SetColumn(3, Vector4D(13, 14, 15, 16));
		SEOUL_UNITTESTING_ASSERT_EQUAL(m,
			Matrix4D(
				1, 2, 3, 4,
				0, 0, 0, 0,
				0, 0, 0, 0,
				13, 14, 15, 16).Transpose());

		m.SetColumn(1, Vector4D(5, 6, 7, 8));
		SEOUL_UNITTESTING_ASSERT_EQUAL(m,
			Matrix4D(
				1, 2, 3, 4,
				5, 6, 7, 8,
				0, 0, 0, 0,
				13, 14, 15, 16).Transpose());

		m.SetColumn(2, Vector4D(9, 10, 11, 12));
		SEOUL_UNITTESTING_ASSERT_EQUAL(m,
			Matrix4D(
				1, 2, 3, 4,
				5, 6, 7, 8,
				9, 10, 11, 12,
				13, 14, 15, 16).Transpose());

		m.SetColumn(0, Vector4D(4, 3, 2, 1));
		SEOUL_UNITTESTING_ASSERT_EQUAL(m,
			Matrix4D(
				4, 3, 2, 1,
				5, 6, 7, 8,
				9, 10, 11, 12,
				13, 14, 15, 16).Transpose());
	}

	// SetRow
	{
		Matrix4D m;
		SEOUL_UNITTESTING_ASSERT_EQUAL(m,
			Matrix4D(
				0, 0, 0, 0,
				0, 0, 0, 0,
				0, 0, 0, 0,
				0, 0, 0, 0));

		m.SetRow(0, Vector4D(1, 2, 3, 4));
		SEOUL_UNITTESTING_ASSERT_EQUAL(m,
			Matrix4D(
				1, 2, 3, 4,
				0, 0, 0, 0,
				0, 0, 0, 0,
				0, 0, 0, 0));

		m.SetRow(3, Vector4D(13, 14, 15, 16));
		SEOUL_UNITTESTING_ASSERT_EQUAL(m,
			Matrix4D(
				1, 2, 3, 4,
				0, 0, 0, 0,
				0, 0, 0, 0,
				13, 14, 15, 16));

		m.SetRow(1, Vector4D(5, 6, 7, 8));
		SEOUL_UNITTESTING_ASSERT_EQUAL(m,
			Matrix4D(
				1, 2, 3, 4,
				5, 6, 7, 8,
				0, 0, 0, 0,
				13, 14, 15, 16));

		m.SetRow(2, Vector4D(9, 10, 11, 12));
		SEOUL_UNITTESTING_ASSERT_EQUAL(m,
			Matrix4D(
				1, 2, 3, 4,
				5, 6, 7, 8,
				9, 10, 11, 12,
				13, 14, 15, 16));

		m.SetRow(0, Vector4D(4, 3, 2, 1));
		SEOUL_UNITTESTING_ASSERT_EQUAL(m,
			Matrix4D(
				4, 3, 2, 1,
				5, 6, 7, 8,
				9, 10, 11, 12,
				13, 14, 15, 16));
	}

	// IsPerspective
	{
		for (Int i = 1; i < 180; ++i)
		{
			Matrix4D m = Matrix4D::CreatePerspectiveFromVerticalFieldOfView(
				DegreesToRadians((Float)i),
				1.0f,
				0.1f,
				100.0f);
			SEOUL_UNITTESTING_ASSERT(m.IsPerspective());
		}
		Matrix4D m = Matrix4D::CreateOrthographic(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 100.0f);
		SEOUL_UNITTESTING_ASSERT(!m.IsPerspective());
	}

	// UpdateAspectRatio (orthographic)
	{
		Matrix4D m = Matrix4D::CreateOrthographic(
			-1.0f,
			1.0f,
			-1.0f,
			1.0f,
			0.1f,
			100.0f);

		for (Int i = 1; i <= 10; ++i)
		{
			m.UpdateAspectRatio((Float32)i / 10.0f);
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL((Float32)i / 10.0f, Matrix4D::ExtractAspectRatio(m), 1e-5f);
		}
	}

	// UpdateAspectRatio (perspective)
	{
		Matrix4D m = Matrix4D::CreatePerspectiveFromVerticalFieldOfView(
			DegreesToRadians(45.0f),
			1.0f,
			0.1f,
			100.0f);

		for (Int i = 1; i <= 10; ++i)
		{
			m.UpdateAspectRatio((Float32)i / 10.0f);
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL((Float32)i / 10.0f, Matrix4D::ExtractAspectRatio(m), 1e-5f);
		}
	}
}

/**
 * Test Matrix4D Transformation methods.
 */
void Matrix4DTest::TestTransformation()
{
	// identity
	{
		// Direction transformation
		SEOUL_UNITTESTING_ASSERT(Vector3D::UnitX() == Matrix4D::TransformDirection(Matrix4D::Identity(), Vector3D::UnitX()));
		SEOUL_UNITTESTING_ASSERT(Vector3D::UnitY() == Matrix4D::TransformDirection(Matrix4D::Identity(), Vector3D::UnitY()));
		SEOUL_UNITTESTING_ASSERT(Vector3D::UnitZ() == Matrix4D::TransformDirection(Matrix4D::Identity(), Vector3D::UnitZ()));

		// Position transformation
		SEOUL_UNITTESTING_ASSERT(Vector3D::UnitX() == Matrix4D::TransformPosition(Matrix4D::Identity(), Vector3D::UnitX()));
		SEOUL_UNITTESTING_ASSERT(Vector3D::UnitY() == Matrix4D::TransformPosition(Matrix4D::Identity(), Vector3D::UnitY()));
		SEOUL_UNITTESTING_ASSERT(Vector3D::UnitZ() == Matrix4D::TransformPosition(Matrix4D::Identity(), Vector3D::UnitZ()));

		// 4D transformation
		SEOUL_UNITTESTING_ASSERT(Vector4D::UnitX() == Matrix4D::Transform(Matrix4D::Identity(), Vector4D::UnitX()));
		SEOUL_UNITTESTING_ASSERT(Vector4D::UnitY() == Matrix4D::Transform(Matrix4D::Identity(), Vector4D::UnitY()));
		SEOUL_UNITTESTING_ASSERT(Vector4D::UnitZ() == Matrix4D::Transform(Matrix4D::Identity(), Vector4D::UnitZ()));
		SEOUL_UNITTESTING_ASSERT(Vector4D::UnitW() == Matrix4D::Transform(Matrix4D::Identity(), Vector4D::UnitW()));
	}

	// 90-degree rotation around X.
	{
		SEOUL_UNITTESTING_ASSERT(( Vector3D::UnitX()).Equals(Matrix4D::TransformDirection(k90DegreesX, Vector3D::UnitX()), 1e-6f));
		SEOUL_UNITTESTING_ASSERT(( Vector3D::UnitZ()).Equals(Matrix4D::TransformDirection(k90DegreesX, Vector3D::UnitY()), 1e-6f));
		SEOUL_UNITTESTING_ASSERT((-Vector3D::UnitY()).Equals(Matrix4D::TransformDirection(k90DegreesX, Vector3D::UnitZ()), 1e-6f));
	}

	// 90-degree rotation around Y.
	{
		SEOUL_UNITTESTING_ASSERT((-Vector3D::UnitZ()).Equals(Matrix4D::TransformDirection(k90DegreesY, Vector3D::UnitX()), 1e-6f));
		SEOUL_UNITTESTING_ASSERT(( Vector3D::UnitY()).Equals(Matrix4D::TransformDirection(k90DegreesY, Vector3D::UnitY()), 1e-6f));
		SEOUL_UNITTESTING_ASSERT(( Vector3D::UnitX()).Equals(Matrix4D::TransformDirection(k90DegreesY, Vector3D::UnitZ()), 1e-6f));
	}

	// 90-degree rotation around Z.
	{
		SEOUL_UNITTESTING_ASSERT(( Vector3D::UnitY()).Equals(Matrix4D::TransformDirection(k90DegreesZ, Vector3D::UnitX()), 1e-6f));
		SEOUL_UNITTESTING_ASSERT((-Vector3D::UnitX()).Equals(Matrix4D::TransformDirection(k90DegreesZ, Vector3D::UnitY()), 1e-6f));
		SEOUL_UNITTESTING_ASSERT(( Vector3D::UnitZ()).Equals(Matrix4D::TransformDirection(k90DegreesZ, Vector3D::UnitZ()), 1e-6f));
	}

	// translation
	{
		SEOUL_UNITTESTING_ASSERT(Vector3D::UnitX() == Matrix4D::TransformPosition(Matrix4D::Identity(), Vector3D::UnitX()));
		SEOUL_UNITTESTING_ASSERT(Vector3D::UnitY() == Matrix4D::TransformPosition(Matrix4D::Identity(), Vector3D::UnitY()));
		SEOUL_UNITTESTING_ASSERT(Vector3D::UnitZ() == Matrix4D::TransformPosition(Matrix4D::Identity(), Vector3D::UnitZ()));

		SEOUL_UNITTESTING_ASSERT(Vector3D(3.0f, 4.0f, 5.0f) == Matrix4D::TransformPosition(Matrix4D::CreateTranslation(3.0f, 4.0f, 5.0f), Vector3D::Zero()));

		SEOUL_UNITTESTING_ASSERT(Vector4D(3.0f, 4.0f, 5.0f, 1.0f) == Matrix4D::Transform(Matrix4D::CreateTranslation(3.0f, 4.0f, 5.0f), Vector4D(0.0f, 0.0f, 0.0f, 1.0f)));
		SEOUL_UNITTESTING_ASSERT(Vector4D::Zero() == Matrix4D::Transform(Matrix4D::CreateTranslation(3.0f, 4.0f, 5.0f), Vector4D::Zero()));
	}
}

/**
 * Test utility functions, mostly for creating new Matrix4Ds
 * of various types from various data.
 */
void Matrix4DTest::TestUtilities()
{
	// rotation from direction test
	{
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(
			k90DegreesX,
			Matrix4D::CreateRotationFromDirection(Vector3D::UnitZ(), Vector3D::UnitY()),
			1e-6f);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(
			k90DegreesY,
			Matrix4D::CreateRotationFromDirection(Vector3D::UnitX(), Vector3D::UnitZ()),
			1e-6f);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(
			k90DegreesZ,
			Matrix4D::CreateRotationFromDirection(Vector3D::UnitY(), Vector3D::UnitX()),
			1e-6f);

		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(
			Matrix4D::Identity(),
			Matrix4D::CreateRotationFromDirection(Vector3D::UnitY(), Vector3D::UnitY()),
			1e-6f);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(
			Matrix4D::CreateRotationX(DegreesToRadians(180.0f)),
			Matrix4D::CreateRotationFromDirection(Vector3D::UnitY(), -Vector3D::UnitY()),
			1e-6f);
	}

	// normal transform test
	{
		Matrix4D m(
			2.0f,  1.0f,  5.0f, 0.0f,
			7.0f,  4.0f,  9.0f, 0.0f,
			11.0f, 10.0f, 2.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f);

		SEOUL_UNITTESTING_ASSERT(
			Matrix4D::CreateNormalTransform(m) ==
			m.Inverse().Transpose());
	}

	// rotation-translation test
	{
		SEOUL_UNITTESTING_ASSERT(Matrix4D::CreateRotationTranslation(
			kQuaternion90DegreesX,
			Vector3D::UnitX()) ==
			Matrix4D::CreateTranslation(Vector3D::UnitX()) *
			kQuaternion90DegreesX.GetMatrix4D());

		SEOUL_UNITTESTING_ASSERT(Matrix4D::CreateRotationTranslation(
			kQuaternion90DegreesY,
			Vector3D::UnitY()) ==
			Matrix4D::CreateTranslation(Vector3D::UnitY()) *
			kQuaternion90DegreesY.GetMatrix4D());

		SEOUL_UNITTESTING_ASSERT(Matrix4D::CreateRotationTranslation(
			kQuaternion90DegreesZ,
			Vector3D::UnitZ()) ==
			Matrix4D::CreateTranslation(Vector3D::UnitZ()) *
			kQuaternion90DegreesZ.GetMatrix4D());
	}

	// rotation from axis-angle
	{
		SEOUL_UNITTESTING_ASSERT(Matrix4D::CreateRotationFromAxisAngle(
			Vector3D::UnitX(),
			DegreesToRadians(90.0f)).Equals(
			kQuaternion90DegreesX.GetMatrix4D(), 1e-3f));

		SEOUL_UNITTESTING_ASSERT(Matrix4D::CreateRotationFromAxisAngle(
			Vector3D::UnitY(),
			DegreesToRadians(90.0f)).Equals(
			kQuaternion90DegreesY.GetMatrix4D(), 1e-3f));

		SEOUL_UNITTESTING_ASSERT(Matrix4D::CreateRotationFromAxisAngle(
			Vector3D::UnitZ(),
			DegreesToRadians(90.0f)).Equals(
			kQuaternion90DegreesZ.GetMatrix4D(), 1e-3f));
	}

	// reflection matrix
	{
		SEOUL_UNITTESTING_ASSERT(
			-Vector3D::UnitX() == Matrix4D::TransformDirection(
				Matrix4D::CreateReflection(Plane::Create(1.0f, 0.0f, 0.0f, 0.0f)),
				Vector3D::UnitX()));

		SEOUL_UNITTESTING_ASSERT(
			-Vector3D::UnitY() == Matrix4D::TransformDirection(
				Matrix4D::CreateReflection(Plane::Create(0.0f, 1.0f, 0.0f, 0.0f)),
				Vector3D::UnitY()));

		SEOUL_UNITTESTING_ASSERT(
			-Vector3D::UnitZ() == Matrix4D::TransformDirection(
				Matrix4D::CreateReflection(Plane::Create(0.0f, 0.0f, 1.0f, 0.0f)),
				Vector3D::UnitZ()));
	}

	// rotation around x, y, and z.
	{
		SEOUL_UNITTESTING_ASSERT(Matrix4D::CreateRotationX(DegreesToRadians(90.0f)).Equals(
			kQuaternion90DegreesX.GetMatrix4D(), 1e-3f));

		SEOUL_UNITTESTING_ASSERT(Matrix4D::CreateRotationY(DegreesToRadians(90.0f)).Equals(
			kQuaternion90DegreesY.GetMatrix4D(), 1e-3f));

		SEOUL_UNITTESTING_ASSERT(Matrix4D::CreateRotationZ(DegreesToRadians(90.0f)).Equals(
			kQuaternion90DegreesZ.GetMatrix4D(), 1e-3f));
	}

	// scale
	{
		SEOUL_UNITTESTING_ASSERT(Matrix4D::CreateScale(4.0f) ==
			Matrix4D(
				4.0f, 0.0f, 0.0f, 0.0f,
				0.0f, 4.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 4.0f, 0.0f,
				0.0f, 0.0f, 0.0f, 1.0f));

		SEOUL_UNITTESTING_ASSERT(Matrix4D::CreateScale(3.0f, 4.0f, 5.0f) ==
			Matrix4D(
				3.0f, 0.0f, 0.0f, 0.0f,
				0.0f, 4.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 5.0f, 0.0f,
				0.0f, 0.0f, 0.0f, 1.0f));

		SEOUL_UNITTESTING_ASSERT(Matrix4D::CreateScale(Vector3D(3.0f, 4.0f, 5.0f)) ==
			Matrix4D(
				3.0f, 0.0f, 0.0f, 0.0f,
				0.0f, 4.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 5.0f, 0.0f,
				0.0f, 0.0f, 0.0f, 1.0f));
	}

	// translation
	{
		SEOUL_UNITTESTING_ASSERT(Matrix4D::CreateTranslation(3.0f, 4.0f, 5.0f) ==
			Matrix4D(
				1.0f, 0.0f, 0.0f, 3.0f,
				0.0f, 1.0f, 0.0f, 4.0f,
				0.0f, 0.0f, 1.0f, 5.0f,
				0.0f, 0.0f, 0.0f, 1.0f));

		SEOUL_UNITTESTING_ASSERT(Matrix4D::CreateTranslation(Vector3D(3.0f, 4.0f, 5.0f)) ==
			Matrix4D(
				1.0f, 0.0f, 0.0f, 3.0f,
				0.0f, 1.0f, 0.0f, 4.0f,
				0.0f, 0.0f, 1.0f, 5.0f,
				0.0f, 0.0f, 0.0f, 1.0f));
	}

	// from matrix3D
	{
		SEOUL_UNITTESTING_ASSERT(Matrix4D::CreateFromMatrix3D(Matrix3D::Identity()) ==
			Matrix4D::Identity());

		SEOUL_UNITTESTING_ASSERT(Matrix4D::CreateFromMatrix3D(
			Matrix3D(
				3.0f, 4.0f, 5.0f,
				6.0f, 7.0f, 8.0f,
				9.0f, 10.0f, 11.0f)) ==
			Matrix4D(
				3.0f, 4.0f, 5.0f, 0.0f,
				6.0f, 7.0f, 8.0f, 0.0f,
				9.0f, 10.0f, 11.0f,  0.0f,
				0.0f, 0.0f, 0.0f, 1.0f));
	}

	// decompose
	{
		Matrix3D mPreRotation;
		Matrix3D mRotation;
		Vector3D vTranslation;

		SEOUL_UNITTESTING_ASSERT(Matrix4D::Decompose(
			Matrix4D::CreateTranslation(3.0f, 4.0f, 5.0f) *
			Matrix4D::CreateRotationX(DegreesToRadians(90.0f)) *
			Matrix4D::CreateScale(3.0f, 4.0f, 5.0f), mPreRotation, mRotation, vTranslation));

		SEOUL_UNITTESTING_ASSERT_EQUAL(
			Matrix4D::CreateTranslation(3.0f, 4.0f, 5.0f) *
			Matrix4D::CreateRotationX(DegreesToRadians(90.0f)) *
			Matrix4D::CreateScale(3.0f, 4.0f, 5.0f),
			Matrix4D::CreateTranslation(vTranslation) *
			Matrix4D::CreateFromMatrix3D(mRotation) *
			Matrix4D::CreateFromMatrix3D(mPreRotation));

		SEOUL_UNITTESTING_ASSERT(mPreRotation.Equals(
			Matrix3D::CreateScale(3.0f, 4.0f, 5.0f), 1e-6f));

		SEOUL_UNITTESTING_ASSERT(mRotation.Equals(
			Matrix3D::CreateRotationX(DegreesToRadians(90.0f))));

		SEOUL_UNITTESTING_ASSERT(vTranslation ==
			Vector3D(3.0f, 4.0f, 5.0f));
	}

	// decompose (many rotations X)
	{
		Matrix3D mPreRotation;
		Matrix3D mRotation;
		Vector3D vTranslation;

		for (Int i = -180; i <= 180; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(Matrix4D::Decompose(
				Matrix4D::CreateTranslation(3.0f, 4.0f, 5.0f) *
				Matrix4D::CreateRotationX(DegreesToRadians((Float)i)) *
				Matrix4D::CreateScale(3.0f, 4.0f, 5.0f), mPreRotation, mRotation, vTranslation));

			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(
				Matrix4D::CreateTranslation(3.0f, 4.0f, 5.0f) *
				Matrix4D::CreateRotationX(DegreesToRadians((Float)i)) *
				Matrix4D::CreateScale(3.0f, 4.0f, 5.0f),
				Matrix4D::CreateTranslation(vTranslation) *
				Matrix4D::CreateFromMatrix3D(mRotation) *
				Matrix4D::CreateFromMatrix3D(mPreRotation), 1e-6f);

			SEOUL_UNITTESTING_ASSERT(mPreRotation.Equals(
				Matrix3D::CreateScale(3.0f, 4.0f, 5.0f), 1e-6f));

			SEOUL_UNITTESTING_ASSERT(mRotation.Equals(
				Matrix3D::CreateRotationX(DegreesToRadians((Float)i))));

			SEOUL_UNITTESTING_ASSERT(vTranslation ==
				Vector3D(3.0f, 4.0f, 5.0f));
		}
	}

	// decompose (many rotations Y)
	{
		Matrix3D mPreRotation;
		Matrix3D mRotation;
		Vector3D vTranslation;

		for (Int i = -180; i <= 180; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(Matrix4D::Decompose(
				Matrix4D::CreateTranslation(3.0f, 4.0f, 5.0f) *
				Matrix4D::CreateRotationY(DegreesToRadians((Float)i)) *
				Matrix4D::CreateScale(3.0f, 4.0f, 5.0f), mPreRotation, mRotation, vTranslation));

			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(
				Matrix4D::CreateTranslation(3.0f, 4.0f, 5.0f) *
				Matrix4D::CreateRotationY(DegreesToRadians((Float)i)) *
				Matrix4D::CreateScale(3.0f, 4.0f, 5.0f),
				Matrix4D::CreateTranslation(vTranslation) *
				Matrix4D::CreateFromMatrix3D(mRotation) *
				Matrix4D::CreateFromMatrix3D(mPreRotation), 1e-6f);

			SEOUL_UNITTESTING_ASSERT(mPreRotation.Equals(
				Matrix3D::CreateScale(3.0f, 4.0f, 5.0f), 1e-6f));

			SEOUL_UNITTESTING_ASSERT(mRotation.Equals(
				Matrix3D::CreateRotationY(DegreesToRadians((Float)i))));

			SEOUL_UNITTESTING_ASSERT(vTranslation ==
				Vector3D(3.0f, 4.0f, 5.0f));
		}
	}

	// decompose (many rotations Z)
	{
		Matrix3D mPreRotation;
		Matrix3D mRotation;
		Vector3D vTranslation;

		for (Int i = -180; i <= 180; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(Matrix4D::Decompose(
				Matrix4D::CreateTranslation(3.0f, 4.0f, 5.0f) *
				Matrix4D::CreateRotationZ(DegreesToRadians((Float)i)) *
				Matrix4D::CreateScale(3.0f, 4.0f, 5.0f), mPreRotation, mRotation, vTranslation));

			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(
				Matrix4D::CreateTranslation(3.0f, 4.0f, 5.0f) *
				Matrix4D::CreateRotationZ(DegreesToRadians((Float)i)) *
				Matrix4D::CreateScale(3.0f, 4.0f, 5.0f),
				Matrix4D::CreateTranslation(vTranslation) *
				Matrix4D::CreateFromMatrix3D(mRotation) *
				Matrix4D::CreateFromMatrix3D(mPreRotation), 1e-6f);

			SEOUL_UNITTESTING_ASSERT(mPreRotation.Equals(
				Matrix3D::CreateScale(3.0f, 4.0f, 5.0f), 1e-6f));

			SEOUL_UNITTESTING_ASSERT(mRotation.Equals(
				Matrix3D::CreateRotationZ(DegreesToRadians((Float)i))));

			SEOUL_UNITTESTING_ASSERT(vTranslation ==
				Vector3D(3.0f, 4.0f, 5.0f));
		}
	}

	// decompose (negative scale X)
	{
		Matrix3D mPreRotation;
		Matrix3D mRotation;
		Vector3D vTranslation;

		SEOUL_UNITTESTING_ASSERT(Matrix4D::Decompose(
			Matrix4D::CreateTranslation(3.0f, 4.0f, 5.0f) *
			Matrix4D::CreateRotationZ(DegreesToRadians(90.0f)) *
			Matrix4D::CreateScale(-3.0f, 4.0f, 5.0f), mPreRotation, mRotation, vTranslation));

		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(
			Matrix4D::CreateTranslation(3.0f, 4.0f, 5.0f) *
			Matrix4D::CreateRotationZ(DegreesToRadians(90.0f)) *
			Matrix4D::CreateScale(-3.0f, 4.0f, 5.0f),
			Matrix4D::CreateTranslation(vTranslation) *
			Matrix4D::CreateFromMatrix3D(mRotation) *
			Matrix4D::CreateFromMatrix3D(mPreRotation), 1e-6f);

		SEOUL_UNITTESTING_ASSERT(mPreRotation.Equals(
			Matrix3D::CreateScale(-3.0f, 4.0f, 5.0f), 1e-6f));

		SEOUL_UNITTESTING_ASSERT(mRotation.Equals(
			Matrix3D::CreateRotationZ(DegreesToRadians(90.0f))));

		SEOUL_UNITTESTING_ASSERT(vTranslation ==
			Vector3D(3.0f, 4.0f, 5.0f));
	}

	// decompose (negative scale Y)
	{
		Matrix3D mPreRotation;
		Matrix3D mRotation;
		Vector3D vTranslation;

		SEOUL_UNITTESTING_ASSERT(Matrix4D::Decompose(
			Matrix4D::CreateTranslation(3.0f, 4.0f, 5.0f) *
			Matrix4D::CreateRotationX(DegreesToRadians(90.0f)) *
			Matrix4D::CreateScale(3.0f, -4.0f, 5.0f), mPreRotation, mRotation, vTranslation));

		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(
			Matrix4D::CreateTranslation(3.0f, 4.0f, 5.0f) *
			Matrix4D::CreateRotationX(DegreesToRadians(90.0f)) *
			Matrix4D::CreateScale(3.0f, -4.0f, 5.0f),
			Matrix4D::CreateTranslation(vTranslation) *
			Matrix4D::CreateFromMatrix3D(mRotation) *
			Matrix4D::CreateFromMatrix3D(mPreRotation), 1e-6f);

		// It is impossible to tell the difference between reflection on
		// a particular axis and reflection on a different axis with
		// a corrective rotation, so Matrix4D::Decompose() is
		// expected to always apply reflection to -X (note the
		// sign differences below).

		SEOUL_UNITTESTING_ASSERT(mPreRotation.Equals(
			Matrix3D::CreateScale(-3.0f, 4.0f, 5.0f), 1e-6f));

		SEOUL_UNITTESTING_ASSERT(mRotation.Equals(
			Matrix3D::CreateRotationX(DegreesToRadians(90.0f)) *
			Matrix3D::CreateRotationZ(DegreesToRadians(180.0f))));

		SEOUL_UNITTESTING_ASSERT(vTranslation ==
			Vector3D(3.0f, 4.0f, 5.0f));
	}

	// decompose (negative scale Z)
	{
		Matrix3D mPreRotation;
		Matrix3D mRotation;
		Vector3D vTranslation;

		SEOUL_UNITTESTING_ASSERT(Matrix4D::Decompose(
			Matrix4D::CreateTranslation(3.0f, 4.0f, 5.0f) *
			Matrix4D::CreateRotationY(DegreesToRadians(90.0f)) *
			Matrix4D::CreateScale(3.0f, 4.0f, -5.0f), mPreRotation, mRotation, vTranslation));

		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(
			Matrix4D::CreateTranslation(3.0f, 4.0f, 5.0f) *
			Matrix4D::CreateRotationY(DegreesToRadians(90.0f)) *
			Matrix4D::CreateScale(3.0f, 4.0f, -5.0f),
			Matrix4D::CreateTranslation(vTranslation) *
			Matrix4D::CreateFromMatrix3D(mRotation) *
			Matrix4D::CreateFromMatrix3D(mPreRotation), 1e-6f);

		// It is impossible to tell the difference between reflection on
		// a particular axis and reflection on a different axis with
		// a corrective rotation, so Matrix4D::Decompose() is
		// expected to always apply reflection to -X (note the
		// sign differences below).

		SEOUL_UNITTESTING_ASSERT(mPreRotation.Equals(
			Matrix3D::CreateScale(-3.0f, 4.0f, 5.0f), 1e-6f));

		SEOUL_UNITTESTING_ASSERT(mRotation.Equals(
			Matrix3D::CreateRotationY(DegreesToRadians(-90.0f))));

		SEOUL_UNITTESTING_ASSERT(vTranslation ==
			Vector3D(3.0f, 4.0f, 5.0f));
	}

	// decompose (negative scale X, many rotations)
	{
		Matrix3D mPreRotation;
		Matrix3D mRotation;
		Vector3D vTranslation;

		for (Int i = -180; i <= 180; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(Matrix4D::Decompose(
				Matrix4D::CreateTranslation(3.0f, 4.0f, 5.0f) *
				Matrix4D::CreateRotationZ(DegreesToRadians((Float)i)) *
				Matrix4D::CreateScale(-3.0f, 4.0f, 5.0f), mPreRotation, mRotation, vTranslation));

			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(
				Matrix4D::CreateTranslation(3.0f, 4.0f, 5.0f) *
				Matrix4D::CreateRotationZ(DegreesToRadians((Float)i)) *
				Matrix4D::CreateScale(-3.0f, 4.0f, 5.0f),
				Matrix4D::CreateTranslation(vTranslation) *
				Matrix4D::CreateFromMatrix3D(mRotation) *
				Matrix4D::CreateFromMatrix3D(mPreRotation), 1e-6f);

			SEOUL_UNITTESTING_ASSERT(mPreRotation.Equals(
				Matrix3D::CreateScale(-3.0f, 4.0f, 5.0f), 1e-6f));

			SEOUL_UNITTESTING_ASSERT(mRotation.Equals(
				Matrix3D::CreateRotationZ(DegreesToRadians((Float)i))));

			SEOUL_UNITTESTING_ASSERT(vTranslation ==
				Vector3D(3.0f, 4.0f, 5.0f));
		}
	}

	// decompose (negative scale Y, many rotations)
	{
		Matrix3D mPreRotation;
		Matrix3D mRotation;
		Vector3D vTranslation;

		for (Int i = -180; i <= 180; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(Matrix4D::Decompose(
				Matrix4D::CreateTranslation(3.0f, 4.0f, 5.0f) *
				Matrix4D::CreateRotationX(DegreesToRadians((Float)i)) *
				Matrix4D::CreateScale(3.0f, -4.0f, 5.0f), mPreRotation, mRotation, vTranslation));

			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(
				Matrix4D::CreateTranslation(3.0f, 4.0f, 5.0f) *
				Matrix4D::CreateRotationX(DegreesToRadians((Float)i)) *
				Matrix4D::CreateScale(3.0f, -4.0f, 5.0f),
				Matrix4D::CreateTranslation(vTranslation) *
				Matrix4D::CreateFromMatrix3D(mRotation) *
				Matrix4D::CreateFromMatrix3D(mPreRotation), 1e-6f);

			// It is impossible to tell the difference between reflection on
			// a particular axis and reflection on a different axis with
			// a corrective rotation, so Matrix4D::Decompose() is
			// expected to always apply reflection to -X (note the
			// sign differences below).

			SEOUL_UNITTESTING_ASSERT(mPreRotation.Equals(
				Matrix3D::CreateScale(-3.0f, 4.0f, 5.0f), 1e-6f));

			SEOUL_UNITTESTING_ASSERT(mRotation.Equals(
				Matrix3D::CreateRotationX(DegreesToRadians((Float)i)) *
				Matrix3D::CreateRotationZ(DegreesToRadians(180.0f))));

			SEOUL_UNITTESTING_ASSERT(vTranslation ==
				Vector3D(3.0f, 4.0f, 5.0f));
		}
	}

	// decompose (negative scale Z, many rotations)
	{
		Matrix3D mPreRotation;
		Matrix3D mRotation;
		Vector3D vTranslation;

		for (Int i = -180; i <= 180; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(Matrix4D::Decompose(
				Matrix4D::CreateTranslation(3.0f, 4.0f, 5.0f) *
				Matrix4D::CreateRotationY(DegreesToRadians((Float)i)) *
				Matrix4D::CreateScale(3.0f, 4.0f, -5.0f), mPreRotation, mRotation, vTranslation));

			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(
				Matrix4D::CreateTranslation(3.0f, 4.0f, 5.0f) *
				Matrix4D::CreateRotationY(DegreesToRadians((Float)i)) *
				Matrix4D::CreateScale(3.0f, 4.0f, -5.0f),
				Matrix4D::CreateTranslation(vTranslation) *
				Matrix4D::CreateFromMatrix3D(mRotation) *
				Matrix4D::CreateFromMatrix3D(mPreRotation), 1e-5f);

			// It is impossible to tell the difference between reflection on
			// a particular axis and reflection on a different axis with
			// a corrective rotation, so Matrix4D::Decompose() is
			// expected to always apply reflection to -X (note the
			// sign differences below).

			SEOUL_UNITTESTING_ASSERT(mPreRotation.Equals(
				Matrix3D::CreateScale(-3.0f, 4.0f, 5.0f), 1e-6f));

			SEOUL_UNITTESTING_ASSERT(mRotation.Equals(
				Matrix3D::CreateRotationY(DegreesToRadians((Float)i - 180.0f)), 1e-6f));

			SEOUL_UNITTESTING_ASSERT(vTranslation ==
				Vector3D(3.0f, 4.0f, 5.0f));
		}
	}

	// perspective projection transform parameter extraction
	{
		SEOUL_UNITTESTING_ASSERT(Equals(
			DegreesToRadians(90.0f),
			Matrix4D::ExtractFovInRadians(
				Matrix4D::CreatePerspectiveFromVerticalFieldOfView(
					DegreesToRadians(90.0f),
					1.0f,
					3.0f,
					1000.0f)),
			1e-6f));

		SEOUL_UNITTESTING_ASSERT(
			1.0f ==
			Matrix4D::ExtractAspectRatio(
				Matrix4D::CreatePerspectiveFromVerticalFieldOfView(
					DegreesToRadians(90.0f),
					1.0f,
					3.0f,
					1000.0f)));

		Float fNear;
		Float fFar;
		Matrix4D::ExtractNearFar(
			Matrix4D::CreatePerspectiveFromVerticalFieldOfView(
				DegreesToRadians(90.0f),
				1.0f,
				3.0f,
				1000.0f), fNear, fFar);

		SEOUL_UNITTESTING_ASSERT(Equals(3.0f, fNear, 1e-1f));
		SEOUL_UNITTESTING_ASSERT(Equals(1000.0f, fFar, 1e-1f));
	}

	// orthographic projection transform parameter extraction
	{
		SEOUL_UNITTESTING_ASSERT(Equals(
			DegreesToRadians(90.0f),
			Matrix4D::ExtractFovInRadians(
				Matrix4D::CreateOrthographic(
					-1.0f, 1.0f, -1.0f, 1.0f, 3.0f, 1000.0f)),
			1e-6f));

		SEOUL_UNITTESTING_ASSERT(
			1.0f ==
			Matrix4D::ExtractAspectRatio(
				Matrix4D::CreateOrthographic(
					-1.0f, 1.0f, -1.0f, 1.0f, 3.0f, 1000.0f)));

		Float fNear;
		Float fFar;
		Matrix4D::ExtractNearFar(
			Matrix4D::CreateOrthographic(
					-1.0f, 1.0f, -1.0f, 1.0f, 3.0f, 1000.0f), fNear, fFar);

		SEOUL_UNITTESTING_ASSERT(Equals(3.0f, fNear, 1e-1f));
		SEOUL_UNITTESTING_ASSERT(Equals(1000.0f, fFar, 1e-1f));
	}

	// lerp
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			Matrix4D(
				1, 2, 3, 4,
				5, 6, 7, 8,
				9, 10, 11, 12,
				13, 14, 15, 16),
			Matrix4D::Lerp(
				Matrix4D(
					0, 1, 2, 3,
					4, 5, 6, 7,
					8, 9, 10, 11,
					12, 13, 14, 15),
				Matrix4D(
					2, 3, 4, 5,
					6, 7, 8, 9,
					10, 11, 12, 13,
					14, 15, 16, 17),
				0.5f));
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			Matrix4D(
				1, 2, 3, 4,
				5, 6, 7, 8,
				9, 10, 11, 12,
				13, 14, 15, 16),
			Lerp(
				Matrix4D(
					0, 1, 2, 3,
					4, 5, 6, 7,
					8, 9, 10, 11,
					12, 13, 14, 15),
				Matrix4D(
					2, 3, 4, 5,
					6, 7, 8, 9,
					10, 11, 12, 13,
					14, 15, 16, 17),
				0.5f));
	}

	// rotation-translation (Matrix3D)
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			Matrix4D::CreateTranslation(Vector3D(1, 2, 3)) *
			Matrix4D::CreateRotationX(DegreesToRadians(45.0f)),
			Matrix4D::CreateRotationTranslation(
				Matrix3D::CreateRotationX(DegreesToRadians(45.0f)),
				Vector3D(1, 2, 3)));
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			Matrix4D::CreateTranslation(Vector3D(1, 2, 3)) *
			Matrix4D::CreateRotationY(DegreesToRadians(45.0f)),
			Matrix4D::CreateRotationTranslation(
				Matrix3D::CreateRotationY(DegreesToRadians(45.0f)),
				Vector3D(1, 2, 3)));
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			Matrix4D::CreateTranslation(Vector3D(1, 2, 3)) *
			Matrix4D::CreateRotationZ(DegreesToRadians(45.0f)),
			Matrix4D::CreateRotationTranslation(
				Matrix3D::CreateRotationZ(DegreesToRadians(45.0f)),
				Vector3D(1, 2, 3)));
	}

	// rotation-translation (Quaternion)
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			Matrix4D::CreateTranslation(Vector3D(1, 2, 3)) *
			Quaternion::CreateFromRotationX(DegreesToRadians(45.0f)).GetMatrix4D(),
			Matrix4D::CreateRotationTranslation(
				Quaternion::CreateFromRotationX(DegreesToRadians(45.0f)),
				Vector3D(1, 2, 3)));
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			Matrix4D::CreateTranslation(Vector3D(1, 2, 3)) *
			Quaternion::CreateFromRotationY(DegreesToRadians(45.0f)).GetMatrix4D(),
			Matrix4D::CreateRotationTranslation(
				Quaternion::CreateFromRotationY(DegreesToRadians(45.0f)),
				Vector3D(1, 2, 3)));
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			Matrix4D::CreateTranslation(Vector3D(1, 2, 3)) *
			Quaternion::CreateFromRotationZ(DegreesToRadians(45.0f)).GetMatrix4D(),
			Matrix4D::CreateRotationTranslation(
				Quaternion::CreateFromRotationZ(DegreesToRadians(45.0f)),
				Vector3D(1, 2, 3)));
	}

	// GetHash()
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(3250977765u, GetHash(Matrix4D()));
	}

	// BiasedProjection() (perspective).
	static const Int kiTestNear = 1;
	static const Int kiTestFar = 1000;
	{
		Matrix4D const m = Matrix4D::CreatePerspectiveFromVerticalFieldOfView(
			DegreesToRadians(45.0f),
			1.0f,
			(Float32)kiTestNear,
			(Float32)kiTestFar);
		Matrix4D const mInf = m.BiasedProjection();

		for (Int i = kiTestNear + 1; i < kiTestFar; ++i)
		{
			auto v0 = Matrix4D::Transform(m, Vector4D(1, 1, -(Float)i, 1));
			auto fZ0 = v0.Z / v0.W;
			auto v1 = Matrix4D::Transform(mInf, Vector4D(1, 1, -(Float)i, 1));
			auto fZ1 = v1.Z / v1.W;
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(v0.GetXY(), v1.GetXY(), 1e-4f);
			SEOUL_UNITTESTING_ASSERT_GREATER_THAN(fZ0, fZ1);
		}

		Matrix4D const mInf2 = m.BiasedProjection(2.0f * kfBiasProjectionEpsilon);
		for (Int i = kiTestNear + 1; i < kiTestFar; ++i)
		{
			auto v0 = Matrix4D::Transform(mInf2, Vector4D(1, 1, -(Float)i, 1));
			auto fZ0 = v0.Z / v0.W;
			auto v1 = Matrix4D::Transform(mInf, Vector4D(1, 1, -(Float)i, 1));
			auto fZ1 = v1.Z / v1.W;
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(v0.GetXY(), v1.GetXY(), 1e-4f);
			SEOUL_UNITTESTING_ASSERT_LESS_THAN(fZ0, fZ1);
		}
	}

	// BiasedProjection() (orthographic).
	{
		Matrix4D const m = Matrix4D::CreateOrthographic(
			-10.0f,
			10.0f,
			-10.0f,
			10.0f,
			(Float32)kiTestNear,
			(Float32)kiTestFar);
		Matrix4D const mInf = m.BiasedProjection();

		for (Int i = kiTestNear + 1; i < kiTestFar; ++i)
		{
			auto const v0 = Matrix4D::Transform(m, Vector4D(1, 1, -(Float)i, 1));
			auto const fZ0 = v0.Z / v0.W;
			auto const v1 = Matrix4D::Transform(mInf, Vector4D(1, 1, -(Float)i, 1));
			auto const fZ1 = v1.Z / v1.W;
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(v0.GetXY(), v1.GetXY(), 1e-4f);
			SEOUL_UNITTESTING_ASSERT_GREATER_THAN(fZ0, fZ1);
		}

		Matrix4D const mInf2 = m.BiasedProjection(2.0f * kfBiasProjectionEpsilon);
		for (Int i = kiTestNear + 1; i < kiTestFar; ++i)
		{
			auto const v0 = Matrix4D::Transform(mInf2, Vector4D(1, 1, -(Float)i, 1));
			auto const fZ0 = v0.Z / v0.W;
			auto const v1 = Matrix4D::Transform(mInf, Vector4D(1, 1, -(Float)i, 1));
			auto const fZ1 = v1.Z / v1.W;
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(v0.GetXY(), v1.GetXY(), 1e-4f);
			SEOUL_UNITTESTING_ASSERT_LESS_THAN(fZ0, fZ1);
		}
	}

	// InfiniteProjection() (perspective).
	{
		Matrix4D const m = Matrix4D::CreatePerspectiveFromVerticalFieldOfView(
			DegreesToRadians(45.0f),
			1.0f,
			(Float32)kiTestNear,
			(Float32)kiTestFar);
		Matrix4D const mInf = m.InfiniteProjection();

		for (Int i = kiTestNear + 1; i < kiTestFar; ++i)
		{
			auto v0 = Matrix4D::Transform(m, Vector4D(1, 1, -(Float)i, 1));
			auto fZ0 = v0.Z / v0.W;
			auto v1 = Matrix4D::Transform(mInf, Vector4D(1, 1, -(Float)i, 1));
			auto fZ1 = v1.Z / v1.W;
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(v0.GetXY(), v1.GetXY(), 1e-4f);
			SEOUL_UNITTESTING_ASSERT_LESS_THAN(fZ0, fZ1);
		}

		Matrix4D const mInf2 = m.InfiniteProjection(2.0f * kfInfiniteProjectionEpsilon);
		for (Int i = kiTestNear + 1; i < kiTestFar; ++i)
		{
			auto v0 = Matrix4D::Transform(mInf2, Vector4D(0, 0, -(Float)i, 1));
			auto fZ0 = v0.Z / v0.W;
			auto v1 = Matrix4D::Transform(mInf, Vector4D(0, 0, -(Float)i, 1));
			auto fZ1 = v1.Z / v1.W;
			SEOUL_UNITTESTING_ASSERT_LESS_THAN(fZ0, fZ1);
		}
	}

	// InfiniteProjection() (orthographic).
	{
		Matrix4D const m = Matrix4D::CreateOrthographic(
			-10.0f,
			10.0f,
			-10.0f,
			10.0f,
			(Float32)kiTestNear,
			(Float32)kiTestFar);
		Matrix4D const mInf = m.InfiniteProjection();

		for (Int i = kiTestNear + 1; i < kiTestFar; ++i)
		{
			auto v0 = Matrix4D::Transform(m, Vector4D(1, 1, -(Float)i, 1));
			auto fZ0 = v0.Z / v0.W;
			auto v1 = Matrix4D::Transform(mInf, Vector4D(1, 1, -(Float)i, 1));
			auto fZ1 = v1.Z / v1.W;
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(v0.GetXY(), v1.GetXY(), 1e-4f);
			SEOUL_UNITTESTING_ASSERT_LESS_THAN(fZ0, fZ1);
		}

		Matrix4D const mInf2 = m.InfiniteProjection(2.0f * kfInfiniteProjectionEpsilon);
		for (Int i = kiTestNear + 1; i < kiTestFar; ++i)
		{
			auto v0 = Matrix4D::Transform(mInf2, Vector4D(0, 0, -(Float)i, 1));
			auto fZ0 = v0.Z / v0.W;
			auto v1 = Matrix4D::Transform(mInf, Vector4D(0, 0, -(Float)i, 1));
			auto fZ1 = v1.Z / v1.W;
			SEOUL_UNITTESTING_ASSERT_LESS_THAN(fZ0, fZ1);
		}
	}
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
