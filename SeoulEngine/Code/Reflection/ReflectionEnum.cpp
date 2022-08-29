/**
 * \file ReflectionEnum.cpp
 * \brief Reflection::Enum is an addendum source of reflection information, supplemental
 * to Reflection::Type. It provides a fixed set of valid enum values for the associated enum,
 * in both their string and integer representations, for robust serialization and easy
 * debugging.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ReflectionBuilders.h"
#include "ReflectionEnum.h"

namespace Seoul::Reflection
{

Enum::Enum(const EnumBuilder& enumBuilder)
	: m_TypeInfo(enumBuilder.m_TypeInfo)
	, m_tAliases(enumBuilder.m_tAliases)
	, m_vAttributes(enumBuilder.m_vAttributes)
	, m_vNames(enumBuilder.m_vNames)
	, m_vValues(enumBuilder.m_vValues)
	, m_Name(enumBuilder.m_Name)
{
}

Enum::~Enum()
{
}

} // namespace Seoul::Reflection
