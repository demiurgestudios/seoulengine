/**
 * \file ReflectionCoreTemplateTypes.h
 * \brief Defines signatures of reflection info for core types that are templated - templated
 * reflection types must be described using a generic description that will be specialized
 * whenever the templated type is specialized and accessed through the reflection system
 * (i.e. TypeOf< Vector<Float> >())
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef REFLECTION_CORE_TEMPLATE_TYPES_H
#define REFLECTION_CORE_TEMPLATE_TYPES_H

#include "CheckedPtr.h"
#include "DataStore.h"
#include "FixedArray.h"
#include "HashSet.h"
#include "HashTable.h"
#include "List.h"
#include "NamedType.h"
#include "SharedPtr.h"
#include "ReflectionAttributes.h"
#include "ReflectionDefine.h"
#include "ReflectionPolymorphic.h"
#include "ReflectionRegistry.h"
#include "Vector.h"

namespace Seoul
{

template <typename T>
inline Reflection::WeakAny PointerLikeCheckedPtr(const Reflection::WeakAny& objectThis)
{
	T* p = nullptr;
	if (objectThis.IsOfType< CheckedPtr<T>* >())
	{
		p = objectThis.Cast< CheckedPtr<T>* >()->Get();
	}
	else if (objectThis.IsOfType< CheckedPtr<T> const* >())
	{
		p = objectThis.Cast< CheckedPtr<T> const* >()->Get();
	}
	return (nullptr != p ? Reflection::GetPolymorphicThis(p) : Reflection::WeakAny());
}

template <typename T>
inline Reflection::WeakAny PointerLikeSharedPtr(const Reflection::WeakAny& objectThis)
{
	T* p = nullptr;
	if (objectThis.IsOfType< SharedPtr<T>* >())
	{
		p = objectThis.Cast< SharedPtr<T>* >()->GetPtr();
	}
	else if (objectThis.IsOfType< SharedPtr<T> const* >())
	{
		p = objectThis.Cast< SharedPtr<T> const* >()->GetPtr();
	}
	return (nullptr != p ? Reflection::GetPolymorphicThis(p) : Reflection::WeakAny());
}

template <typename T, typename Tag>
inline Bool CustomDeserializeNamedType(
	Reflection::SerializeContext& rContext,
	const DataStore& dataStore,
	const DataNode& table,
	const Reflection::WeakAny& objectThis,
	Bool bSkipPostSerialize)
{
	NamedType<T, Tag>* p = objectThis.Cast< NamedType<T, Tag>* >();
	if (p == nullptr)
	{
		return false;
	}
	return Reflection::DeserializeObject(
		rContext,
		dataStore,
		table,
		&(p->GetValue()),
		bSkipPostSerialize);
}

SEOUL_BEGIN_TEMPLATE_TYPE(
	CheckedPtr,													// base type name
	(T),														// type template signature - CheckedPtr<T>
	(typename T),												// template declaration signature - template <typename T>
	("CheckedPtr<%s>", SEOUL_GET_FULLY_QUALIFIED_TYPE_NAME(T))) // expression used to generate a unique name for specializations of this type
	SEOUL_ATTRIBUTE(PointerLike, &PointerLikeCheckedPtr<T>)
SEOUL_END_TYPE()

SEOUL_TEMPLATE_TYPE(
	DefaultHashTableKeyTraits,
	(T),
	(typename T),
	("DefaultHashTableKeyTraits<%s>",
		SEOUL_GET_FULLY_QUALIFIED_TYPE_NAME(T)), TypeFlags::kDisableNew)

SEOUL_BEGIN_TEMPLATE_TYPE(
	HashSet,                                             // base type name
	(KEY, MEMORY_BUDGETS, TRAITS),                       // type template signature - HashSet<KEY, MEMORY_BUDGETS, TRAITS>
	(typename KEY, Int MEMORY_BUDGETS, typename TRAITS), // template declaration signature - template <typename KEY, Int MEMORY_BUDGETS, typename TRAITS>
	("HashSet<%s, %d, %s>",
		SEOUL_GET_FULLY_QUALIFIED_TYPE_NAME(KEY),
		MEMORY_BUDGETS,
		SEOUL_GET_FULLY_QUALIFIED_TYPE_NAME(TRAITS)))    // expression used to generate a unique name for specializations of this type

SEOUL_END_TYPE()

SEOUL_BEGIN_TEMPLATE_TYPE(
	HashTable,                                                           // base type name
	(KEY, VALUE, MEMORY_BUDGETS, TRAITS),                                // type template signature - HashTable<KEY, VALUE, MEMORY_BUDGETS, TRAITS>
	(typename KEY, typename VALUE, Int MEMORY_BUDGETS, typename TRAITS), // template declaration signature - template <typename KEY, typename VALUE, Int MEMORY_BUDGETS, typename TRAITS>
	("HashTable<%s, %s, %d, %s>",
		SEOUL_GET_FULLY_QUALIFIED_TYPE_NAME(KEY),
		SEOUL_GET_FULLY_QUALIFIED_TYPE_NAME(VALUE),
		MEMORY_BUDGETS,
		SEOUL_GET_FULLY_QUALIFIED_TYPE_NAME(TRAITS)))                    // expression used to generate a unique name for specializations of this type

SEOUL_END_TYPE()

SEOUL_BEGIN_TEMPLATE_TYPE(
	FixedArray,                                                           // base type name
	(T, SIZE),                                                            // type template signature - FixedArray<T, SIZE>
	(typename T, UInt32 SIZE),                                            // template declaration signature - template <typename T, UInt32 SIZE>
	("FixedArray<%s, %d>", SEOUL_GET_FULLY_QUALIFIED_TYPE_NAME(T), SIZE)) // expression used to generate a unique name for specializations of this type

SEOUL_END_TYPE()

SEOUL_BEGIN_TEMPLATE_TYPE(
	List,                                                                     // base type name
	(T, MEMORY_BUDGETS),                                                      // type template signature - List<T, MEMORY_BUDGETS>
	(typename T, Int MEMORY_BUDGETS),                                         // template declaration signature - template <typename T, Int MEMORY_BUDGETS>
	("List<%s, %d>", SEOUL_GET_FULLY_QUALIFIED_TYPE_NAME(T), MEMORY_BUDGETS)) // expression used to generate a unique name for specializations of this type

SEOUL_END_TYPE()

SEOUL_BEGIN_TEMPLATE_TYPE(
	SharedPtr,                                                 // base type name
	(T),                                                        // type template signature - SharedPtr<T>
	(typename T),                                               // template declaration signature - template <typename T>
	("SharedPtr<%s>", SEOUL_GET_FULLY_QUALIFIED_TYPE_NAME(T))) // expression used to generate a unique name for specializations of this type
	SEOUL_ATTRIBUTE(PointerLike, &PointerLikeSharedPtr<T>)

SEOUL_END_TYPE()

SEOUL_BEGIN_TEMPLATE_TYPE(
	Pair,                                                                                             // base type name
	(T, U),                                                                                           // type template signature - Pair<T, U>
	(typename T, typename U),                                                                         // template declaration signature - template <typename T, typename U>
	("Pair<%s, %s>", SEOUL_GET_FULLY_QUALIFIED_TYPE_NAME(T), SEOUL_GET_FULLY_QUALIFIED_TYPE_NAME(U))) // expression used to generate a unique name for specializations of this type

SEOUL_END_TYPE()

SEOUL_BEGIN_TEMPLATE_TYPE(
	Vector,                                                                     // base type name
	(T, MEMORY_BUDGETS),                                                        // type template signature - Vector<T, MEMORY_BUDGETS>
	(typename T, Int MEMORY_BUDGETS),                                           // template declaration signature - template <typename T, Int MEMORY_BUDGETS>
	("Vector<%s, %d>", SEOUL_GET_FULLY_QUALIFIED_TYPE_NAME(T), MEMORY_BUDGETS)) // expression used to generate a unique name for specializations of this type

SEOUL_END_TYPE()

SEOUL_BEGIN_TEMPLATE_TYPE(
	NamedType, // base type name
	(T, Tag), // type template signature - NamedType<T, Tag>
	(typename T, typename Tag), // template declaration signature - template <typename T, typename Tag>
	("NamedType<%s, %s>", SEOUL_GET_FULLY_QUALIFIED_TYPE_NAME(T), SEOUL_GET_FULLY_QUALIFIED_TYPE_NAME(Tag))) // expression used to generate a unique name for specializations of this type
	SEOUL_ATTRIBUTE(CustomSerializeType, &CustomDeserializeNamedType<T, Tag>)
SEOUL_END_TYPE()

} // namespace Seoul

#endif // include guard
