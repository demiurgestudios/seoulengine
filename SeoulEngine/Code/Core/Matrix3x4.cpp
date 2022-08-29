/**
 * \file Matrix3x4.cpp
 * \brief Matrix3x4 represents the upper 3 rows of a 4x4 square matrix.
 *
 * Matrix3x4 does *not* represent an actual 3x4 matrix, it is
 * a structure for efficient packing of 4x4 matrices with an implicit
 * 4th row of [0, 0, 0, 1].
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Matrix3x4.h"
#include "Matrix4D.h"

namespace Seoul
{

Matrix3x4::Matrix3x4(const Matrix4D& mMatrix)
	: M00(mMatrix.M00), M01(mMatrix.M01), M02(mMatrix.M02), M03(mMatrix.M03)
	, M10(mMatrix.M10), M11(mMatrix.M11), M12(mMatrix.M12), M13(mMatrix.M13)
	, M20(mMatrix.M20), M21(mMatrix.M21), M22(mMatrix.M22), M23(mMatrix.M23)
{
}

} // namespace Seoul
