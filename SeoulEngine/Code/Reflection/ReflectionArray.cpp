/**
 * \file ReflectionArray.cpp
 * \brief Reflection::Array is an addendum source of reflection information, supplemental
 * to Reflection::Type. It provides operations to allow manipulations on a type that fulfills
 * the generic contract of an array, including:
 * - access by element
 * - resize of the array (optional, can be excluded for fixed size arrays)
 * - length query of the array
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ReflectionArray.h"

namespace Seoul::Reflection
{

Array::Array(UInt32 uFlags)
	: m_uFlags(uFlags)
{
}

Array::~Array()
{
}

} // namespace Seoul::Reflection
