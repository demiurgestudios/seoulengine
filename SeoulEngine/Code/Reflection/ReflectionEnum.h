/**
 * \file ReflectionEnum.h
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

#pragma once
#ifndef REFLECTION_ENUM_H
#define REFLECTION_ENUM_H

#include "ReflectionAny.h"
#include "ReflectionAttribute.h"
#include "ReflectionPrereqs.h"
#include "ReflectionSimpleCast.h"
#include "SeoulHString.h"

namespace Seoul::Reflection
{

// Forward declarations
struct EnumBuilder;

class Enum SEOUL_SEALED
{
public:
	Enum(const EnumBuilder& enumBuilder);
	~Enum();

	/**
	 * @return The array of AttributeCollection instances for each enum value.
	 */
	const EnumAttributeVector& GetAttributes() const
	{
		return m_vAttributes;
	}

	/**
	 * @return The set of all enum values, as their string name representation. Each value
	 * corresponds to the integrate value at the corresponding index in GetValues().
	 */
	const EnumNameVector& GetNames() const
	{
		return m_vNames;
	}

	/**
	 * @return The set of all enum values, as their integer representation. Each value
	 * corresponds to the string value at the corresponding index in GetNames().
	 */
	const EnumValueVector& GetValues() const
	{
		return m_vValues;
	}

	/**
	 * @return An HString name of this Enum.
	 */
	HString GetName() const
	{
		return m_Name;
	}

	/**
	 * @return The TypeInfo associated with the C++ enum associated with this Enum.
	 */
	const TypeInfo& GetTypeInfo() const
	{
		return m_TypeInfo;
	}

	template <typename T>
	Bool TryGetName(T value, HString& rOut) const
	{
		Int const iValue = (Int)value;
		UInt32 const nCount = m_vValues.GetSize();
		for (UInt32 i = 0u; i < nCount; ++i)
		{
			if (iValue == m_vValues[i])
			{
				rOut = m_vNames[i];
				return true;
			}
		}

		return false;
	}

	Bool TryGetName(const Any& any, HString& rOut) const
	{
		Int iValue = 0;
		if (SimpleCast(any, iValue))
		{
			return TryGetName(iValue, rOut);
		}

		return false;
	}

	Bool TryGetName(const WeakAny& any, HString& rOut) const
	{
		Int iValue = 0;
		if (SimpleCast(any, iValue))
		{
			return TryGetName(iValue, rOut);
		}

		return false;
	}

	/**
	 * Set the enum value to riOut that corresponds to name. If
	 * name is not a valid enum value, riOut is left unchanged.
	 *
	 * @return True if name is a valid enum value, false otherwise. If this method
	 * returns false, riOut is left unchanged, otherwise it contains the integer
	 * enum value that corresponds to name.
	 */
	Bool TryGetValue(HString name, Int& riOut) const
	{
		UInt32 const nCount = m_vNames.GetSize();
		for (UInt32 i = 0u; i < nCount; ++i)
		{
			if (name == m_vNames[i])
			{
				riOut = m_vValues[i];
				return true;
			}
		}

		// Check for an alias and try again.
		{
			HString alias;
			if (m_tAliases.GetValue(name, alias))
			{
				return TryGetValue(alias, riOut);
			}
		}

		return false;
	}

	/**
	 * Convenience variation of TryGetValue(HString, Int&) const
	 */
	template <typename T>
	Bool TryGetValue(HString name, T& rOut) const
	{
		Int i = -1;
		if (TryGetValue(name, i))
		{
			rOut = (T)i;
			return true;
		}

		return false;
	}

private:
	const TypeInfo& m_TypeInfo;
	AliasTable m_tAliases;
	EnumAttributeVector m_vAttributes;
	EnumNameVector m_vNames;
	EnumValueVector m_vValues;
	UInt32 m_uFlags;
	HString m_Name;

	// Not default constructable
	Enum();

	SEOUL_DISABLE_COPY(Enum);
}; // class Enum

} // namespace Seoul::Reflection

#endif // include guard
