/**
 * \file ReflectionType.cpp
 * \brief Type is the main class used for reflecting on
 * engine types at runtime. Type can be thought of as
 * the runtime equivalent of a C++ class definition - it allows
 * runtime code to discover and interact with various class
 * elements (properties, constructors).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ReflectionAttribute.h"
#include "ReflectionBuilders.h"
#include "ReflectionMethod.h"
#include "ReflectionProperty.h"
#include "ReflectionRegistry.h"
#include "ReflectionScript.h"
#include "ReflectionType.h"
#include "StreamBuffer.h"

namespace Seoul::Reflection
{

Type::Type(const MethodBuilder& methodBuilder)
	: m_tAliases()
	, m_Attributes()
	, m_vMethods()
	, m_vParents()
	, m_vProperties()
	, m_tMethods()
	, m_tProperties()
	, m_TypeInfo(methodBuilder.m_rBuilder.m_TypeInfo)
	, m_NewDelegate(methodBuilder.m_rBuilder.m_NewDelegate)
	, m_DeleteDelegate(methodBuilder.m_rBuilder.m_DeleteDelegate)
	, m_InPlaceNewDelegate(methodBuilder.m_rBuilder.m_InPlaceNewDelegate)
	, m_DestructorDelegate(methodBuilder.m_rBuilder.m_DestructorDelegate)
	, m_DefaultCopyDelegate(methodBuilder.m_rBuilder.m_DefaultCopyDelegate)
	, m_uRegistryIndex(0u)
	, m_Name(methodBuilder.m_rBuilder.m_Name)
{
	InternalConstruct(methodBuilder.m_rBuilder);
}

Type::Type(const PropertyBuilder& propertyBuilder)
	: m_tAliases()
	, m_Attributes()
	, m_vMethods()
	, m_vParents()
	, m_vProperties()
	, m_tMethods()
	, m_tProperties()
	, m_TypeInfo(propertyBuilder.m_rBuilder.m_TypeInfo)
	, m_NewDelegate(propertyBuilder.m_rBuilder.m_NewDelegate)
	, m_DeleteDelegate(propertyBuilder.m_rBuilder.m_DeleteDelegate)
	, m_InPlaceNewDelegate(propertyBuilder.m_rBuilder.m_InPlaceNewDelegate)
	, m_DestructorDelegate(propertyBuilder.m_rBuilder.m_DestructorDelegate)
	, m_DefaultCopyDelegate(propertyBuilder.m_rBuilder.m_DefaultCopyDelegate)
	, m_uRegistryIndex(0u)
	, m_Name(propertyBuilder.m_rBuilder.m_Name)
{
	InternalConstruct(propertyBuilder.m_rBuilder);
}

Type::Type(const TypeBuilder& typeBuilder)
	: m_tAliases()
	, m_Attributes()
	, m_vMethods()
	, m_vParents()
	, m_vProperties()
	, m_tMethods()
	, m_tProperties()
	, m_TypeInfo(typeBuilder.m_TypeInfo)
	, m_NewDelegate(typeBuilder.m_NewDelegate)
	, m_DeleteDelegate(typeBuilder.m_DeleteDelegate)
	, m_InPlaceNewDelegate(typeBuilder.m_InPlaceNewDelegate)
	, m_DestructorDelegate(typeBuilder.m_DestructorDelegate)
	, m_DefaultCopyDelegate(typeBuilder.m_DefaultCopyDelegate)
	, m_uRegistryIndex(0u)
	, m_Name(typeBuilder.m_Name)
{
	InternalConstruct(typeBuilder);
}

Type::~Type()
{
	SafeDeleteVector(m_vProperties);
	// Do not delete m_vParents - it contains pointers to global functions.
	SafeDeleteVector(m_vMethods);
	m_Attributes.DestroyAttributes();
}

Method const* Type::GetMethod(HString name) const
{
	// Check immediate methods first.
	{
		Method const* pReturn = nullptr;
		if (m_tMethods.GetValue(name, pReturn))
		{
			return pReturn;
		}
	}

	// Next check parents.
	UInt32 const nImmediateParents = m_vParents.GetSize();
	for (UInt32 i = 0u; i < nImmediateParents; ++i)
	{
		Method const* pReturn = m_vParents[i].First().GetMethod(name);
		if (nullptr != pReturn)
		{
			return pReturn;
		}
	}

	// Finally check alises and retry if found.
	HString aliasedName;
	if (m_tAliases.GetValue(name, aliasedName))
	{
		return GetMethod(aliasedName);
	}

	return nullptr;
}

/**
 * @return Type of a parent of this Type with name name,
 * or nullptr if no such parent exists.
 */
Type const* Type::GetParent(HString name) const
{
	// First traverse immediate parents.
	UInt32 const nImmediateParents = m_vParents.GetSize();
	for (UInt32 i = 0u; i < nImmediateParents; ++i)
	{
		if (m_vParents[i].First().GetName() == name)
		{
			return &(m_vParents[i].First());
		}
	}

	// Next, try to get a parent from each of our immediate parents.
	for (UInt32 i = 0u; i < nImmediateParents; ++i)
	{
		Type const* pReturn = m_vParents[i].First().GetParent(name);
		if (nullptr != pReturn)
		{
			return pReturn;
		}
	}

	// Finally check alises and retry if found.
	HString aliasedName;
	if (m_tAliases.GetValue(name, aliasedName))
	{
		return GetParent(aliasedName);
	}

	// No parent 'name'.
	return nullptr;
}

/**
 * @return A Property with name name, or nullptr if no such property
 * exists.
 */
Property const* Type::GetProperty(HString name) const
{
	// Check immediate properties first.
	{
		Property const* pReturn = nullptr;
		if (m_tProperties.GetValue(name, pReturn))
		{
			return pReturn;
		}
	}

	// Next check parents.
	UInt32 const nImmediateParents = m_vParents.GetSize();
	for (UInt32 i = 0u; i < nImmediateParents; ++i)
	{
		Property const* pReturn = m_vParents[i].First().GetProperty(name);
		if (nullptr != pReturn)
		{
			return pReturn;
		}
	}

	// Finally check alises and retry if found.
	HString aliasedName;
	if (m_tAliases.GetValue(name, aliasedName))
	{
		return GetProperty(aliasedName);
	}

	return nullptr;
}

void Type::FromScript(lua_State* pVm, Int iOffset, const WeakAny& objectThis) const
{
	// Parents first.
	UInt32 const uParents = GetParentCount();
	for (UInt32 i = 0u; i < uParents; ++i)
	{
		auto const& pair = GetParentPair(i);
		WeakAny parent(objectThis);
		SEOUL_VERIFY(pair.Second(parent));

		// Process a parent.
		pair.First().FromScript(pVm, iOffset, parent);
	}

	// Now properties.
	WeakAny pointer;
	UInt32 const uProperties = GetPropertyCount();
	for (UInt32 i = 0u; i < uProperties; ++i)
	{
		auto const p = GetProperty(i);

		// Lookup the property by name.
		lua_getfield(pVm, iOffset, p->GetName().CStr());

		// If the property is not defined, skip it.
		if (lua_isnil(pVm, -1))
		{
			// Remove the nil from the stack.
			lua_pop(pVm, 1);
			continue;
		}

		// Attempt to get the property as an opaque pointer.
		if (p->TryGetPtr(objectThis, pointer))
		{
			// If success, process the value on the top of the stack
			// into the property.
			p->GetMemberTypeInfo().GetType().FromScript(pVm, -1, pointer);
		}
		// We need to get the value, write into it, and then set
		// it back.
		else
		{
			Any value;
			if (p->TryGet(objectThis, value))
			{
				p->GetMemberTypeInfo().GetType().FromScript(pVm, -1, value.GetPointerToObject());
				(void)p->TrySet(objectThis, value);
			}
		}

		// Remove the property from the script stack
		// before processing additional properties.
		lua_pop(pVm, 1);
	}
}

void Type::ToScript(lua_State* pVm, const WeakAny& objectThis, Bool bCreateTable /* = true*/) const
{
	UInt32 const uProperties = GetPropertyCount();

	// When requested, push a new table onto the stack with allocated
	// space. This will be false when TryToScript is called for parents.
	if (bCreateTable)
	{
		if (auto attr = GetAttribute<Attributes::ScriptClass>())
		{ 
			seoul_lua_createclasstable(pVm, attr->m_ClassName.IsEmpty() ? GetName().CStr() : attr->m_ClassName.CStr(), (int)uProperties, (int)uProperties);
		}
		else
		{
			lua_createtable(pVm, (int)uProperties, (int)uProperties);
		}
	}

	// Parents first.
	UInt32 const uParents = GetParentCount();
	for (UInt32 i = 0u; i < uParents; ++i)
	{
		auto const& pair = GetParentPair(i);
		WeakAny parent(objectThis);
		SEOUL_VERIFY(pair.Second(parent));

		// Process the parent.
		pair.First().ToScript(pVm, parent, false);
	}

	// Now properties.
	WeakAny pointer;
	for (UInt32 i = 0u; i < uProperties; ++i)
	{
		auto const p = GetProperty(i);

		// Get an opaque pointer to the property for reading.
		if (p->TryGetConstPtr(objectThis, pointer))
		{
			// Process the property directly.
			p->GetMemberTypeInfo().GetType().ToScript(pVm, pointer);
		}
		// If we couldn't get an opaque pointer to the property,
		// we must get the property's value and then use
		// that for further processing.
		else
		{
			Any value;
			if (p->TryGet(objectThis, value))
			{
				// Process the value we acquired.
				p->GetMemberTypeInfo().GetType().ToScript(pVm, value.GetPointerToObject());
			}
		}

		// Commit the value to script. This also pops
		// the property from the stack.
		lua_setfield(pVm, -2, p->GetName().CStr());
	}
}

void Type::InternalConstruct(const TypeBuilder& rBuilder)
{
	m_tAliases = rBuilder.m_tAliases;
	m_Attributes = rBuilder.m_Attributes;
	m_vMethods = rBuilder.m_vMethods;
	m_vParents = rBuilder.m_vParents;
	m_vProperties = rBuilder.m_vProperties;

	// Sanity check - must not be generating types once main has been entered for the Reflection system
	// to be thread safe
	SEOUL_ASSERT(!IsInMainFunction());

	// Setup shadowed lookup tables for methods and properties
	// and in so doing, check for duplicates.
	{
		UInt32 const uSize = m_vMethods.GetSize();
		for (UInt32 i = 0u; i < uSize; ++i)
		{
			Method const* pMethod = m_vMethods[i];
			SEOUL_VERIFY(m_tMethods.Insert(pMethod->GetName(), pMethod).Second);
		}
	}
	{
		UInt32 const uSize = m_vProperties.GetSize();
		for (UInt32 i = 0u; i < uSize; ++i)
		{
			Property const* pProperty = m_vProperties[i];
			SEOUL_VERIFY(m_tProperties.Insert(pProperty->GetName(), pProperty).Second);
		}
	}

	// TODO: We can't setup a table for parents here, due to static
	// initialization order (parents are not guaranteed to be constructed
	// and queryable prior to children in the graph). We'd need a post
	// fixup step, possibly part of entering main.

	// Setup type aliases
	UInt32 const nTypeAliases = rBuilder.m_vTypeAliases.GetSize();
	for (UInt32 i = 0u; i < nTypeAliases; ++i)
	{
		SEOUL_VERIFY(Registry::GetSingleton().AddTypeAlias(rBuilder.m_vTypeAliases[i], m_Name));
	}

	// Complete, add the type to the registry.
	SEOUL_VERIFY(Registry::GetSingleton().AddType(*this, m_uRegistryIndex));
}

} // namespace Seoul::Reflection
