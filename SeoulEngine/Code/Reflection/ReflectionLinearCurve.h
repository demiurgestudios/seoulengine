/**
 * \file ReflectionLinearCurve.h
 * \brief Defines signatures of reflection info fo the LinearCurve templated type.
 *
 * Whenever an instance of LinearCurve is used in a reflection definition, this
 * header file should be included in the .cpp file that defines that reflection
 * body.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef REFLECTION_LINEAR_CURVE_H
#define REFLECTION_LINEAR_CURVE_H

#include "LinearCurve.h"
#include "ReflectionDefine.h"

namespace Seoul
{

SEOUL_BEGIN_TEMPLATE_TYPE(
	LinearCurve,                                                                     // base type name
	(T, MEMORY_BUDGETS),                                                             // type template signature - LinearCurve<T, MEMORY_BUDGETS>
	(typename T, Int MEMORY_BUDGETS),                                                // template declaration signature - template <typename T, Int MEMORY_BUDGETS>
	("LinearCurve<%s, %d>", SEOUL_GET_FULLY_QUALIFIED_TYPE_NAME(T), MEMORY_BUDGETS)) // expression used to generate a unique name for specializations of this type

	SEOUL_PROPERTY_N("Times", m_vTimes)
	SEOUL_PROPERTY_N("Values", m_vValues)

SEOUL_END_TYPE()

} // namespace Seoul

#endif // include guard
