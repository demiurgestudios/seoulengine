/**
 * \file ReflectionBuilders.h
 * \brief Internal header of the Reflection system, defines
 * "builders", which are used as part of the mechanism that derives
 * concrete reflection types from (e.g.) a SEOUL_METHOD*() macro.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef REFLECTION_BUILDERS_H
#define REFLECTION_BUILDERS_H

#include "ReflectionAttribute.h"
#include "ReflectionAttributes.h"
#include "ReflectionEnum.h"
#include "ReflectionMethod.h"
#include "ReflectionPrereqs.h"
#include "ReflectionProperty.h"
#include "ReflectionPropertyDetail.h"
#include "ReflectionType.h"

namespace Seoul::Reflection
{

// Forward declarations
struct MethodBuilder;
struct PropertyBuilder;
struct TypeBuilder;

struct MethodBuilder
{
	MethodBuilder(Method& rMethod, TypeBuilder& rBuilder)
		: m_rMethod(rMethod)
		, m_rBuilder(rBuilder)
	{
	}

	Seoul::Reflection::Method& m_rMethod;
	TypeBuilder& m_rBuilder;

	template <size_t zFromStringArrayLengthInBytes, size_t zToStringArrayLengthInBytes>
	MethodBuilder& AddAlias(Byte const (&sFromName)[zFromStringArrayLengthInBytes], Byte const (&sToName)[zToStringArrayLengthInBytes]);

	MethodBuilder& AddAttribute(Attribute* pAttribute)
	{
		SEOUL_ASSERT(nullptr != pAttribute);
		m_rMethod.m_Attributes.AddAttribute(pAttribute);

		return *this;
	}

	MethodBuilder AddMethod(Method* pMethod);
	PropertyBuilder AddProperty(Property* pProperty);
};

struct PropertyBuilder
{
	PropertyBuilder(Property& rProperty, TypeBuilder& rBuilder)
		: m_rProperty(rProperty)
		, m_rBuilder(rBuilder)
	{
	}

	Seoul::Reflection::Property& m_rProperty;
	TypeBuilder& m_rBuilder;

	template <size_t zFromStringArrayLengthInBytes, size_t zToStringArrayLengthInBytes>
	PropertyBuilder& AddAlias(Byte const (&sFromName)[zFromStringArrayLengthInBytes], Byte const (&sToName)[zToStringArrayLengthInBytes]);

	PropertyBuilder& AddAttribute(Attribute* pAttribute)
	{
		SEOUL_ASSERT(nullptr != pAttribute);
		m_rProperty.m_Attributes.AddAttribute(pAttribute);

		return *this;
	}

	MethodBuilder AddMethod(Method* pMethod);
	PropertyBuilder AddProperty(Property* pProperty);
};

struct EnumBuilder
{
	template <size_t zStringArrayLengthInBytes>
	EnumBuilder(
		const TypeInfo& typeInfo,
		Byte const (&sName)[zStringArrayLengthInBytes])
		: m_TypeInfo(typeInfo)
		, m_vNames()
		, m_vValues()
		, m_Name(CStringLiteral(sName))
	{
	}

	template <size_t zFromStringArrayLengthInBytes, size_t zToStringArrayLengthInBytes>
	EnumBuilder& AddAlias(Byte const (&sFromName)[zFromStringArrayLengthInBytes], Byte const (&sToName)[zToStringArrayLengthInBytes])
	{
		m_tAliases.Insert(CStringLiteral(sFromName), CStringLiteral(sToName));

		return *this;
	}

	EnumBuilder& AddAttribute(Attribute* pAttribute)
	{
		SEOUL_ASSERT(nullptr != pAttribute);
		m_vAttributes.Back().AddAttribute(pAttribute);

		return *this;
	}

	template <typename T, size_t zStringArrayLengthInBytes>
	EnumBuilder& AddEnum(Byte const (&sName)[zStringArrayLengthInBytes], T value)
	{
		m_vAttributes.PushBack(AttributeCollection{});
		m_vNames.PushBack(CStringLiteral(sName));
		m_vValues.PushBack((Int)value);

		return *this;
	}

	AliasTable m_tAliases;
	const TypeInfo& m_TypeInfo;
	EnumAttributeVector m_vAttributes;
	EnumNameVector m_vNames;
	EnumValueVector m_vValues;
	HString m_Name;
}; // struct EnumBuilder

struct TypeBuilder
{
	template <size_t zStringArrayLengthInBytes>
	TypeBuilder(
		const TypeInfo& typeInfo,
		Byte const (&sName)[zStringArrayLengthInBytes],
		TypeDetail::NewDelegate newDelegate,
		TypeDetail::DeleteDelegate deleteDelegate,
		TypeDetail::InPlaceNewDelegate inPlaceNewDelegate,
		TypeDetail::DestructorDelegate destructorDelegate,
		TypeDetail::DefaultCopyDelegate defaultCopyDelegate)
		: m_tAliases()
		, m_Attributes()
		, m_vParents()
		, m_vProperties()
		, m_TypeInfo(typeInfo)
		, m_Name(CStringLiteral(sName))
		, m_NewDelegate(newDelegate)
		, m_DeleteDelegate(deleteDelegate)
		, m_InPlaceNewDelegate(inPlaceNewDelegate)
		, m_DestructorDelegate(destructorDelegate)
		, m_DefaultCopyDelegate(defaultCopyDelegate)
	{
	}

	TypeBuilder(
		const TypeInfo& typeInfo,
		const String& sName,
		TypeDetail::NewDelegate newDelegate,
		TypeDetail::DeleteDelegate deleteDelegate,
		TypeDetail::InPlaceNewDelegate inPlaceNewDelegate,
		TypeDetail::DestructorDelegate destructorDelegate,
		TypeDetail::DefaultCopyDelegate defaultCopyDelegate)
		: m_tAliases()
		, m_Attributes()
		, m_vParents()
		, m_vProperties()
		, m_TypeInfo(typeInfo)
		, m_Name(sName)
		, m_NewDelegate(newDelegate)
		, m_DeleteDelegate(deleteDelegate)
		, m_InPlaceNewDelegate(inPlaceNewDelegate)
		, m_DestructorDelegate(destructorDelegate)
		, m_DefaultCopyDelegate(defaultCopyDelegate)
	{
	}

	AliasVector m_vTypeAliases;
	AliasTable m_tAliases;
	AttributeCollection m_Attributes;
	MethodVector m_vMethods;
	TypeVector m_vParents;
	PropertyVector m_vProperties;
	const TypeInfo& m_TypeInfo;
	HString m_Name;
	TypeDetail::NewDelegate m_NewDelegate;
	TypeDetail::DeleteDelegate m_DeleteDelegate;
	TypeDetail::InPlaceNewDelegate m_InPlaceNewDelegate;
	TypeDetail::DestructorDelegate m_DestructorDelegate;
	TypeDetail::DefaultCopyDelegate m_DefaultCopyDelegate;

	template <size_t zFromStringArrayLengthInBytes, size_t zToStringArrayLengthInBytes>
	TypeBuilder& AddAlias(Byte const (&sFromName)[zFromStringArrayLengthInBytes], Byte const (&sToName)[zToStringArrayLengthInBytes])
	{
		m_tAliases.Insert(CStringLiteral(sFromName), CStringLiteral(sToName));

		return *this;
	}

	template <size_t zFromStringArrayLengthInBytes>
	TypeBuilder& AddTypeAlias(Byte const (&sFromName)[zFromStringArrayLengthInBytes])
	{
		m_vTypeAliases.PushBack(CStringLiteral(sFromName));

		return *this;
	}

	TypeBuilder& AddAttribute(Attribute* pAttribute)
	{
		SEOUL_ASSERT(nullptr != pAttribute);
		m_Attributes.AddAttribute(pAttribute);

		return *this;
	}

	MethodBuilder AddMethod(Method* pMethod)
	{
		SEOUL_ASSERT(nullptr != pMethod);
		m_vMethods.PushBack(pMethod);

		return MethodBuilder(*pMethod, *this);
	}

	template <typename T, typename PARENT>
	TypeBuilder& AddParent()
	{
		typedef IsBaseOf<PARENT, T> Tester;
		SEOUL_STATIC_ASSERT_MESSAGE(Tester::Value, "Parent specified in SEOUL_PARENT() macro for reflection definition of T is not a parent class of T.");

		m_vParents.PushBack(MakePair(
			TypeOf<PARENT>,
			&ReflectionCast<T, PARENT>));
		return *this;
	}

	PropertyBuilder AddProperty(Property* pProperty)
	{
		SEOUL_ASSERT(nullptr != pProperty);
		pProperty->m_pClassTypeInfo = &m_TypeInfo;
		m_vProperties.PushBack(pProperty);

		return PropertyBuilder(*pProperty, *this);
	}
}; // struct TypeBuilder

template <size_t zFromStringArrayLengthInBytes, size_t zToStringArrayLengthInBytes>
inline MethodBuilder& MethodBuilder::AddAlias(Byte const (&sFromName)[zFromStringArrayLengthInBytes], Byte const (&sToName)[zToStringArrayLengthInBytes])
{
	(void)m_rBuilder.AddAlias(sFromName, sToName);

	return *this;
}

inline MethodBuilder MethodBuilder::AddMethod(Method* pMethod)
{
	SEOUL_ASSERT(nullptr != pMethod);
	m_rBuilder.m_vMethods.PushBack(pMethod);

	return MethodBuilder(*pMethod, m_rBuilder);
}

inline PropertyBuilder MethodBuilder::AddProperty(Property* pProperty)
{
	SEOUL_ASSERT(nullptr != pProperty);
	pProperty->m_pClassTypeInfo = &m_rBuilder.m_TypeInfo;
	m_rBuilder.m_vProperties.PushBack(pProperty);

	return PropertyBuilder(*pProperty, m_rBuilder);
}

template <size_t zFromStringArrayLengthInBytes, size_t zToStringArrayLengthInBytes>
inline PropertyBuilder& PropertyBuilder::AddAlias(Byte const (&sFromName)[zFromStringArrayLengthInBytes], Byte const (&sToName)[zToStringArrayLengthInBytes])
{
	(void)m_rBuilder.AddAlias(sFromName, sToName);

	return *this;
}

inline MethodBuilder PropertyBuilder::AddMethod(Method* pMethod)
{
	SEOUL_ASSERT(nullptr != pMethod);
	m_rBuilder.m_vMethods.PushBack(pMethod);

	return MethodBuilder(*pMethod, m_rBuilder);
}

inline PropertyBuilder PropertyBuilder::AddProperty(Property* pProperty)
{
	SEOUL_ASSERT(nullptr != pProperty);
	pProperty->m_pClassTypeInfo = &m_rBuilder.m_TypeInfo;
	m_rBuilder.m_vProperties.PushBack(pProperty);

	return PropertyBuilder(*pProperty, m_rBuilder);
}

} // namespace Seoul::Reflection

#endif // include guard
