/**
 * \file ReflectionRegistry.h
 * \brief The Registry is the one and only global collection of runtime reflection
 * Type objects. It is used primarily to get Type objects by identifier name.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef REFLECTION_REGISTRY_H
#define REFLECTION_REGISTRY_H

#include "ReflectionPrereqs.h"
#include "SeoulHString.h"

namespace Seoul::Reflection
{

// Forward declarations
class Method;
class Property;
class Type;

/**
 * The global registry of Type, allows Type objects to be
 * returns by index or name.
 *
 * Type indices are stable within one run of an application,
 * but are not guaranteed to remain stable across runs and should
 * not be serialized. Instead, serialize the Type name, and
 * then use the index at runtime to reduce the cost of Type
 * lookups (or cache a const refernece to the Type object itself).
 */
class Registry SEOUL_SEALED
{
public:
	typedef HashTable<HString, UInt16, MemoryBudgets::Reflection> TypeTable;
	typedef Vector<Type const*, MemoryBudgets::Reflection> TypeVector;

	~Registry();

	static const Registry& GetRegistry();

	/**
	 * @return The total number of global Type objects - does not change
	 * after global initialization (once the main function has been entered).
	 */
	UInt32 GetTypeCount() const
	{
		return m_vTypes.GetSize();
	}

	/**
	 * @return Type type at index uIndex, or nullptr if
	 * uIndex is out-of-range.
	 */
	Type const* GetType(UInt32 uIndex) const
	{
		if (uIndex < m_vTypes.GetSize())
		{
			return m_vTypes[uIndex];
		}

		return nullptr;
	}

	/**
	 * @return The index of Type name, or UInt16Max if
	 * a Type name is not registered with the Reflection::Registry.
	 */
	UInt16 GetTypeIndex(HString name) const
	{
		UInt16 uReturn = UInt16Max;

		HString aliased;
		if (!m_tTypes.GetValue(name, uReturn) && m_tAliases.GetValue(name, aliased))
		{
			(void)m_tTypes.GetValue(aliased, uReturn);
		}

		return uReturn;
	}

	/**
	 * @return The Type name, or nullptr if no Type is
	 * registered with the Reflection::Registry.
	 */
	Type const* GetType(HString name) const
	{
		return GetType(GetTypeIndex(name));
	}

private:
	TypeTable m_tTypes;
	TypeVector m_vTypes;
	AliasTable m_tAliases;

	static Registry& GetSingleton();

	friend class Type;

	Bool AddType(Type const& rType, UInt32& ruRegistryIndex);
	Bool AddTypeAlias(HString fromName, HString toName)
	{
		return m_tAliases.Insert(fromName, toName).Second;
	}

private:
	// Not constructable outside of the global Meyer singleton.
	Registry();

	SEOUL_DISABLE_COPY(Registry);
}; // class Registry

} // namespace Seoul::Reflection

#endif // include guard
