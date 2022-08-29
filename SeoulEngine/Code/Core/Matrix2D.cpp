/**
 * \file Matrix2D.cpp
 * \brief Matrix2D represents a 2x2 square m.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Matrix2D.h"
#include "Matrix2x3.h"
namespace Seoul
{

Matrix2D::Matrix2D(const Matrix2x3& m)
	: M00(m.M00), M10(m.M10)
	, M01(m.M01), M11(m.M11)
{
}

} // namespace Seoul
