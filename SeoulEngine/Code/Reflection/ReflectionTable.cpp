/**
 * \file ReflectionTable.cpp
 * \brief Reflection::Table is an addendum source of reflection information, supplemental
 * to Reflection::Type. It provides operations to allow manipulations on a type that fulfills
 * the generic contract of a table, including:
 * - access by key
 * - (optional) erase by key
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ReflectionTable.h"

namespace Seoul::Reflection
{

TableEnumerator::TableEnumerator()
{
}

TableEnumerator::~TableEnumerator()
{
}

Table::Table(UInt32 uFlags)
	: m_uFlags(uFlags)
{
}

Table::~Table()
{
}

} // namespace Seoul::Reflection
