/**
 * \file ReflectionTypeDetail.h
 * \brief Templated subclasses of Type that define certain type specific methods.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef REFLECTION_TYPE_DETAIL_H
#define REFLECTION_TYPE_DETAIL_H

#include "ReflectionAttributes.h"
#include "ReflectionDataStoreUtil.h"
#include "ReflectionType.h"

namespace Seoul::Reflection
{

/**
 * FulfillsArrayContract::Value is equal to true if the type T
 * meets the requirements to be treated like an array by the reflection system. Those
 * requirements are:
 * - defines T::ValueType, which is the type of elements in the array.
 * - defines UInt32 T::GetSize() const, which returns the number of elements in the array.
 * - defines T::ValueType* Get(UInt32 uIndex), which returns a pointer to the element at uIndex in the array.
 * - defines T::ValueType const* Get(UInt32 uIndex) const, which returns a const-pointer to the element at uIndex in the array.
 */
template <typename T>
struct FulfillsArrayContract
{
private:
	typedef Byte True[1];
	typedef Byte False[2];

	template <typename U, U MEMBER_FUNC> struct Tester {};

	template <typename U>
	static True& TestForValueType(typename U::ValueType*);

	template <typename U>
	static False& TestForValueType(...);

	template <typename U>
	static True& TestForGet(Tester< typename U::ValueType* (U::*)(UInt32), &U::Get >*);

	template <typename U>
	static False& TestForGet(...);

	template <typename U>
	static True& TestForGetConst(Tester< typename U::ValueType const* (U::*)(UInt32) const, &U::Get >*);

	template <typename U>
	static False& TestForGetConst(...);

	template <typename U>
	static True& TestForGetSize(Tester< UInt32 (U::*)() const, &U::GetSize >*);

	template <typename U>
	static False& TestForGetSize(...);

public:
	static const Bool Value = (
		sizeof(TestForValueType<T>(nullptr)) == sizeof(True) &&
		sizeof(TestForGet<T>(nullptr)) == sizeof(True) &&
		sizeof(TestForGetConst<T>(nullptr)) == sizeof(True) &&
		sizeof(TestForGetSize<T>(nullptr)) == sizeof(True));
}; // struct FulfillsArrayContract

/**
 * FulfillsTableContract::Value is equal to true if the type T
 * meets the requirements to be treated like a table by the reflection system. Those
 * requirements are:
 * - defines T::KeyType, which is the type of keys in the table.
 * - defines T::ValueType, which is the type of values in the table.
 * - defines T::Iterator, which is the type of an iterator on the table.
 * - defines T::ValueType* Find(const T::KeyType& key), which returns a pointer to the value associated with key, or nullptr.
 * - defines T::ValueType const* Find(const T::KeyType& key) const, which returns a const-pointer to the value associated with key, or nullptr.
 */
template <typename T>
struct FulfillsTableContract
{
private:
	typedef Byte True[1];
	typedef Byte False[2];

	template <typename U, U MEMBER_FUNC> struct Tester {};

	template <typename U>
	static True& TestForKeyType(typename U::KeyType*);

	template <typename U>
	static False& TestForKeyType(...);

	template <typename U>
	static True& TestForValueType(typename U::ValueType*);

	template <typename U>
	static False& TestForValueType(...);

	template <typename U>
	static True& TestForConstIterator(typename U::ConstIterator*);

	template <typename U>
	static False& TestForConstIterator(...);

	template <typename U>
	static True& TestForIterator(typename U::Iterator*);

	template <typename U>
	static False& TestForIterator(...);

	template <typename U>
	static True& TestForFindValue(Tester< typename U::ValueType* (U::*)(const typename U::KeyType&), &U::Find >*);

	template <typename U>
	static False& TestForFindValue(...);

	template <typename U>
	static True& TestForFindValueConst(Tester< typename U::ValueType const* (U::*)(const typename U::KeyType&) const, &U::Find >*);

	template <typename U>
	static False& TestForFindValueConst(...);

	template <typename U>
	static True& TestForInsert(Tester< Pair<typename U::Iterator, Bool> (U::*)(const typename U::KeyType&, const typename U::ValueType&), &U::Insert >*);

	template <typename U>
	static False& TestForInsert(...);

public:
	static const Bool Value = (
		sizeof(TestForKeyType<T>(nullptr)) == sizeof(True) &&
		sizeof(TestForValueType<T>(nullptr)) == sizeof(True) &&
		sizeof(TestForConstIterator<T>(nullptr)) == sizeof(True) &&
		sizeof(TestForIterator<T>(nullptr)) == sizeof(True) &&
		sizeof(TestForFindValue<T>(nullptr)) == sizeof(True) &&
		sizeof(TestForFindValueConst<T>(nullptr)) == sizeof(True) &&
		sizeof(TestForInsert<T>(nullptr)) == sizeof(True));
}; // struct FulfillsTableContract

template <typename T, Bool B_IS_ENUM>
struct EnumSelector
{
	static Enum const* TryGetEnum()
	{
		return nullptr;
	}
};

template <typename T>
struct EnumSelector<T, true>
{
	static Enum const* TryGetEnum()
	{
		return &EnumOf<T>();
	}
};

template <typename T, Bool B_IS_ARRAY, Bool B_IS_TABLE, Bool B_HAS_DATA_NODE_HANDLER>
class TypeT SEOUL_SEALED : public Type
{
public:
	TypeT(const MethodBuilder& methodBuilder)
		: Type(methodBuilder)
	{
		m_pCustomSerializeType.Reset(GetAttribute<Attributes::CustomSerializeType>());
	}

	TypeT(const PropertyBuilder& propertyBuilder)
		: Type(propertyBuilder)
	{
		m_pCustomSerializeType.Reset(GetAttribute<Attributes::CustomSerializeType>());
	}

	TypeT(const TypeBuilder& builder)
		: Type(builder)
	{
		m_pCustomSerializeType.Reset(GetAttribute<Attributes::CustomSerializeType>());
	}

	~TypeT()
	{
	}

	virtual WeakAny GetPtrUnsafe(void* pUnsafePointer) const SEOUL_OVERRIDE
	{
		return WeakAny(((T*)pUnsafePointer));
	}

	virtual WeakAny GetPtrUnsafe(void const* pUnsafePointer) const SEOUL_OVERRIDE
	{
		return WeakAny(((T const*)pUnsafePointer));
	}

	inline static Bool DirectDeserialize(
		SerializeContext& rContext,
		const DataStore& dataStore,
		const DataNode& dataNode,
		T& rObject,
		Bool bSkipPostSerialize)
	{
		return Type::TryDeserialize(rContext, dataStore, dataNode, &rObject, bSkipPostSerialize);
	}

	inline static Bool DirectSerializeToArray(
		SerializeContext& rContext,
		DataStore& rDataStore,
		const DataNode& array,
		UInt32 uIndex,
		const T& object,
		Bool bSkipPostSerialize)
	{
		return Type::TrySerializeToArray(rContext, rDataStore, array, uIndex, &object, bSkipPostSerialize);
	}

	inline static Bool DirectSerializeToTable(
		SerializeContext& rContext,
		DataStore& rDataStore,
		const DataNode& table,
		HString key,
		const T& object,
		Bool bSkipPostSerialize)
	{
		return Type::TrySerializeToTable(rContext, rDataStore, table, key, &object, bSkipPostSerialize);
	}

	virtual Bool DoDeserialize(
		SerializeContext& rContext,
		const DataStore& dataStore,
		const DataNode& dataNode,
		const WeakAny& objectThis,
		Bool bSkipPostSerialize = false,
		Bool bDisableRootCustomDeserializeType = false) const SEOUL_OVERRIDE
	{
		// Invoke the custom deserializer for the type, unless bDisableRootCustomDeserializeType
		// is true (this is typically used to allow custom deserializers to invoke the
		// default deserializer after performing some prep).
		if (!bDisableRootCustomDeserializeType &&
			m_pCustomSerializeType.IsValid() &&
			m_pCustomSerializeType->m_CustomDeserialize)
		{
			return m_pCustomSerializeType->m_CustomDeserialize(rContext, dataStore, dataNode, objectThis, bSkipPostSerialize);
		}
		else
		{
			UInt32 uProperties = 0u;
			return DoGenericDeserialize(uProperties, rContext, dataStore, dataNode, objectThis, objectThis.GetType(), bSkipPostSerialize);
		}
	}

	virtual Bool DoSerializeToArray(
		SerializeContext& rContext,
		DataStore& rDataStore,
		const DataNode& array,
		UInt32 uIndex,
		const WeakAny& objectThis,
		Bool bSkipPostSerialize = false,
		Bool bDisableRootCustomSerializeType = false) const SEOUL_OVERRIDE
	{
		// Invoke the custom serializer for the type, unless bDisableRootCustomSerializeType
		// is true (this is typically used to allow custom serializers to invoke the
		// default serializer after performing some prep).
		if (!bDisableRootCustomSerializeType &&
			m_pCustomSerializeType.IsValid() &&
			m_pCustomSerializeType->m_CustomSerializeToArray)
		{
			return m_pCustomSerializeType->m_CustomSerializeToArray(rContext, rDataStore, array, uIndex, objectThis, bSkipPostSerialize);
		}
		else
		{
			// First, insert a table, since a generic deserialize always stores the object
			// as a table of its properties.
			if (!rDataStore.SetTableToArray(array, uIndex, objectThis.GetType().GetPropertyCount()))
			{
				return false;
			}

			DataNode dataNode;
			SEOUL_VERIFY(rDataStore.GetValueFromArray(array, uIndex, dataNode));

			UInt32 uProperties = 0u;
			return DoGenericSerialize(uProperties, rContext, rDataStore, dataNode, objectThis, bSkipPostSerialize);
		}
	}

	virtual Bool DoSerializeToTable(
		SerializeContext& rContext,
		DataStore& rDataStore,
		const DataNode& table,
		HString key,
		const WeakAny& objectThis,
		Bool bSkipPostSerialize = false,
		Bool bDisableRootCustomSerializeType = false) const SEOUL_OVERRIDE
	{
		// Invoke the custom serializer for the type, unless bDisableRootCustomSerializeType
		// is true (this is typically used to allow custom serializers to invoke the
		// default serializer after performing some prep).
		if (!bDisableRootCustomSerializeType &&
			m_pCustomSerializeType.IsValid() &&
			m_pCustomSerializeType->m_CustomSerializeToTable)
		{
			return m_pCustomSerializeType->m_CustomSerializeToTable(rContext, rDataStore, table, key, objectThis, bSkipPostSerialize);
		}
		else
		{
			// First, insert a table, since a generic deserialize always stores the object
			// as a table of its properties.
			if (!rDataStore.SetTableToTable(table, key, objectThis.GetType().GetPropertyCount()))
			{
				return false;
			}

			DataNode dataNode;
			SEOUL_VERIFY(rDataStore.GetValueFromTable(table, key, dataNode));

			UInt32 uProperties = 0u;
			return DoGenericSerialize(uProperties, rContext, rDataStore, dataNode, objectThis, bSkipPostSerialize);
		}
	}

	virtual Reflection::Array const* TryGetArray() const SEOUL_OVERRIDE
	{
		return nullptr;
	}

	virtual Reflection::Enum const* TryGetEnum() const SEOUL_OVERRIDE
	{
		SEOUL_STATIC_ASSERT(!IsEnum<T>::Value);
		return nullptr;
	}

	virtual Reflection::Table const* TryGetTable() const SEOUL_OVERRIDE
	{
		return nullptr;
	}

private:
	CheckedPtr<Attributes::CustomSerializeType const> m_pCustomSerializeType;
}; // class TypeT

template <typename T, Bool B_IS_ARRAY, Bool B_IS_TABLE>
class TypeT<T, B_IS_ARRAY, B_IS_TABLE, true> SEOUL_SEALED : public Type
{
public:
	TypeT(const MethodBuilder& methodBuilder)
		: Type(methodBuilder)
	{
	}

	TypeT(const PropertyBuilder& propertyBuilder)
		: Type(propertyBuilder)
	{
	}

	TypeT(const TypeBuilder& builder)
		: Type(builder)
	{
	}

	~TypeT()
	{
	}

	virtual WeakAny GetPtrUnsafe(void* pUnsafePointer) const SEOUL_OVERRIDE
	{
		return WeakAny(((T*)pUnsafePointer));
	}

	virtual WeakAny GetPtrUnsafe(void const* pUnsafePointer) const SEOUL_OVERRIDE
	{
		return WeakAny(((T const*)pUnsafePointer));
	}

	inline static Bool DirectDeserialize(
		SerializeContext& rContext,
		const DataStore& dataStore,
		const DataNode& dataNode,
		T& rObject,
		Bool bSkipPostSerialize)
	{
		if (!Seoul::FromDataNode(rContext, dataStore, dataNode, rObject))
		{
			if (!rContext.HandleError(SerializeError::kFailedSettingValue))
			{
				return false;
			}
		}

		return true;
	}

	inline static Bool DirectSerializeToArray(
		SerializeContext& rContext,
		DataStore& rDataStore,
		const DataNode& array,
		UInt32 uIndex,
		const T& object,
		Bool bSkipPostSerialize)
	{
		if (!Seoul::ToDataStoreArray(rContext, rDataStore, array, uIndex, object))
		{
			if (!rContext.HandleError(SerializeError::kFailedGettingValue))
			{
				return false;
			}
		}

		return true;
	}

	inline static Bool DirectSerializeToTable(
		SerializeContext& rContext,
		DataStore& rDataStore,
		const DataNode& table,
		HString key,
		const T& object,
		Bool bSkipPostSerialize)
	{
		if (!Seoul::ToDataStoreTable(rContext, rDataStore, table, key, object))
		{
			if (!rContext.HandleError(SerializeError::kFailedGettingValue))
			{
				return false;
			}
		}

		return true;
	}

	virtual Bool DoDeserialize(
		SerializeContext& rContext,
		const DataStore& dataStore,
		const DataNode& dataNode,
		const WeakAny& objectThis,
		Bool bSkipPostSerialize = false,
		Bool bDisableRootCustomDeserializeType = false) const SEOUL_OVERRIDE
	{
		// Get the object - if this fails, we must fail, as there is nothing more to do.
		T* p = nullptr;
		if (!PointerCast(objectThis, p))
		{
			return false;
		}
		SEOUL_ASSERT(nullptr != p);

		if (!Seoul::FromDataNode(rContext, dataStore, dataNode, *p))
		{
			if (!rContext.HandleError(SerializeError::kFailedSettingValue))
			{
				return false;
			}
		}

		return true;
	}

	virtual Bool DoSerializeToArray(
		SerializeContext& rContext,
		DataStore& rDataStore,
		const DataNode& array,
		UInt32 uIndex,
		const WeakAny& objectThis,
		Bool bSkipPostSerialize = false,
		Bool bDisableRootCustomSerializeType = false) const SEOUL_OVERRIDE
	{
		// Get the object - if this fails, we must fail, as there is nothing more to do.
		T const* p = nullptr;
		if (!PointerCast(objectThis, p))
		{
			return false;
		}
		SEOUL_ASSERT(nullptr != p);

		if (!Seoul::ToDataStoreArray(rContext, rDataStore, array, uIndex, *p))
		{
			if (!rContext.HandleError(SerializeError::kFailedGettingValue))
			{
				return false;
			}
		}

		return true;
	}

	virtual Bool DoSerializeToTable(
		SerializeContext& rContext,
		DataStore& rDataStore,
		const DataNode& table,
		HString key,
		const WeakAny& objectThis,
		Bool bSkipPostSerialize = false,
		Bool bDisableRootCustomSerializeType = false) const SEOUL_OVERRIDE
	{
		// Get the object - if this fails, we must fail, as there is nothing more to do.
		T const* p = nullptr;
		if (!PointerCast(objectThis, p))
		{
			return false;
		}
		SEOUL_ASSERT(nullptr != p);

		if (!Seoul::ToDataStoreTable(rContext, rDataStore, table, key, *p))
		{
			if (!rContext.HandleError(SerializeError::kFailedGettingValue))
			{
				return false;
			}
		}

		return true;
	}

	virtual void FromScript(lua_State* pVm, Int iOffset, const WeakAny& objectThis) const SEOUL_OVERRIDE
	{
		// Get the object - if this fails, we must fail, as there is nothing more to do.
		T* p = nullptr;
		if (!PointerCast(objectThis, p))
		{
			// TODO: I think I can just assert here, as all
			// callers of this context will have enforced this to be true.
			return;
		}
		SEOUL_ASSERT(nullptr != p);

		// Pass handling off to the type's DataNodeHandler.
		DataNodeHandler<T, IsEnum<T>::Value>::FromScript(pVm, iOffset, *p);
	}

	virtual void ToScript(lua_State* pVm, const WeakAny& objectThis, Bool bCreateTable = true) const SEOUL_OVERRIDE
	{
		// Get the object - if this fails, we must fail, as there is nothing more to do.
		T const* p = nullptr;
		if (!PointerCast(objectThis, p))
		{
			// TODO: I think I can just assert here, as all
			// callers of this context will have enforced this to be true.
			lua_pushnil(pVm);
			return;
		}
		SEOUL_ASSERT(nullptr != p);

		// Pass handling off to the type's DataNodeHandler.
		DataNodeHandler<T, IsEnum<T>::Value>::ToScript(pVm, *p);
	}

	virtual Reflection::Array const* TryGetArray() const SEOUL_OVERRIDE
	{
		return nullptr;
	}

	virtual Reflection::Enum const* TryGetEnum() const SEOUL_OVERRIDE
	{
		return EnumSelector<T, IsEnum<T>::Value>::TryGetEnum();
	}

	virtual Reflection::Table const* TryGetTable() const SEOUL_OVERRIDE
	{
		return nullptr;
	}
}; // class TypeT

template <typename T>
class TypeT<T, true, false, false> SEOUL_SEALED : public Type
{
public:
	TypeT(const MethodBuilder& methodBuilder)
		: Type(methodBuilder)
	{
	}

	TypeT(const PropertyBuilder& propertyBuilder)
		: Type(propertyBuilder)
	{
	}

	TypeT(const TypeBuilder& builder)
		: Type(builder)
	{
	}

	~TypeT()
	{
	}

	virtual WeakAny GetPtrUnsafe(void* pUnsafePointer) const SEOUL_OVERRIDE
	{
		return WeakAny(((T*)pUnsafePointer));
	}

	virtual WeakAny GetPtrUnsafe(void const* pUnsafePointer) const SEOUL_OVERRIDE
	{
		return WeakAny(((T const*)pUnsafePointer));
	}

	inline static Bool DirectDeserialize(
		SerializeContext& rContext,
		const DataStore& dataStore,
		const DataNode& dataNode,
		T& rObject,
		Bool bSkipPostSerialize)
	{
		return ArrayOf<T>().TryDeserialize(rContext, dataStore, dataNode, &rObject, bSkipPostSerialize);
	}

	inline static Bool DirectSerializeToArray(
		SerializeContext& rContext,
		DataStore& rDataStore,
		const DataNode& array,
		UInt32 uIndex,
		const T& object,
		Bool bSkipPostSerialize)
	{
		// Create an array to serialize into.
		if (!rDataStore.SetArrayToArray(array, uIndex))
		{
			return false;
		}

		DataNode dataNode;
		SEOUL_VERIFY(rDataStore.GetValueFromArray(array, uIndex, dataNode));

		return ArrayOf<T>().TrySerialize(rContext, rDataStore, dataNode, &object, bSkipPostSerialize);
	}

	inline static Bool DirectSerializeToTable(
		SerializeContext& rContext,
		DataStore& rDataStore,
		const DataNode& table,
		HString key,
		const T& object,
		Bool bSkipPostSerialize)
	{
		// Create an array to serialize into.
		if (!rDataStore.SetArrayToTable(table, key))
		{
			return false;
		}

		DataNode dataNode;
		SEOUL_VERIFY(rDataStore.GetValueFromTable(table, key, dataNode));

		return ArrayOf<T>().TrySerialize(rContext, rDataStore, dataNode, &object, bSkipPostSerialize);
	}

	virtual Bool DoDeserialize(
		SerializeContext& rContext,
		const DataStore& dataStore,
		const DataNode& dataNode,
		const WeakAny& objectThis,
		Bool bSkipPostSerialize = false,
		Bool bDisableRootCustomDeserializeType = false) const SEOUL_OVERRIDE
	{
		return ArrayOf<T>().TryDeserialize(rContext, dataStore, dataNode, objectThis, bSkipPostSerialize);
	}

	virtual Bool DoSerializeToArray(
		SerializeContext& rContext,
		DataStore& rDataStore,
		const DataNode& array,
		UInt32 uIndex,
		const WeakAny& objectThis,
		Bool bSkipPostSerialize = false,
		Bool bDisableRootCustomSerializeType = false) const SEOUL_OVERRIDE
	{
		// Create an array to serialize into.
		if (!rDataStore.SetArrayToArray(array, uIndex))
		{
			return false;
		}

		DataNode dataNode;
		SEOUL_VERIFY(rDataStore.GetValueFromArray(array, uIndex, dataNode));

		return ArrayOf<T>().TrySerialize(rContext, rDataStore, dataNode, objectThis, bSkipPostSerialize);
	}

	virtual Bool DoSerializeToTable(
		SerializeContext& rContext,
		DataStore& rDataStore,
		const DataNode& table,
		HString key,
		const WeakAny& objectThis,
		Bool bSkipPostSerialize = false,
		Bool bDisableRootCustomSerializeType = false) const SEOUL_OVERRIDE
	{
		// Create an array to serialize into.
		if (!rDataStore.SetArrayToTable(table, key))
		{
			return false;
		}

		DataNode dataNode;
		SEOUL_VERIFY(rDataStore.GetValueFromTable(table, key, dataNode));

		return ArrayOf<T>().TrySerialize(rContext, rDataStore, dataNode, objectThis, bSkipPostSerialize);
	}

	virtual void FromScript(lua_State* pVm, Int iOffset, const WeakAny& objectThis) const SEOUL_OVERRIDE
	{
		ArrayOf<T>().FromScript(pVm, iOffset, objectThis);
	}

	virtual void ToScript(lua_State* pVm, const WeakAny& objectThis, Bool bCreateTable = true) const SEOUL_OVERRIDE
	{
		ArrayOf<T>().ToScript(pVm, objectThis);
	}

	virtual Reflection::Array const* TryGetArray() const SEOUL_OVERRIDE
	{
		return &ArrayOf<T>();
	}

	virtual Reflection::Enum const* TryGetEnum() const SEOUL_OVERRIDE
	{
		SEOUL_STATIC_ASSERT(!IsEnum<T>::Value);
		return nullptr;
	}

	virtual Reflection::Table const* TryGetTable() const SEOUL_OVERRIDE
	{
		return nullptr;
	}
}; // class TypeT

template <typename T>
class TypeT<T, false, true, false> SEOUL_SEALED : public Type
{
public:
	TypeT(const MethodBuilder& methodBuilder)
		: Type(methodBuilder)
	{
	}

	TypeT(const PropertyBuilder& propertyBuilder)
		: Type(propertyBuilder)
	{
	}

	TypeT(const TypeBuilder& builder)
		: Type(builder)
	{
	}

	~TypeT()
	{
	}

	virtual WeakAny GetPtrUnsafe(void* pUnsafePointer) const SEOUL_OVERRIDE
	{
		return WeakAny(((T*)pUnsafePointer));
	}

	virtual WeakAny GetPtrUnsafe(void const* pUnsafePointer) const SEOUL_OVERRIDE
	{
		return WeakAny(((T const*)pUnsafePointer));
	}

	inline static Bool DirectDeserialize(
		SerializeContext& rContext,
		const DataStore& dataStore,
		const DataNode& dataNode,
		T& rObject,
		Bool bSkipPostSerialize)
	{
		return TableOf<T>().TryDeserialize(rContext, dataStore, dataNode, &rObject, bSkipPostSerialize);
	}

	inline static Bool DirectSerializeToArray(
		SerializeContext& rContext,
		DataStore& rDataStore,
		const DataNode& array,
		UInt32 uIndex,
		const T& object,
		Bool bSkipPostSerialize)
	{
		// Create a table to serialize into.
		if (!rDataStore.SetTableToArray(array, uIndex))
		{
			return false;
		}

		DataNode dataNode;
		SEOUL_VERIFY(rDataStore.GetValueFromArray(array, uIndex, dataNode));

		return TableOf<T>().TrySerialize(rContext, rDataStore, dataNode, &object, bSkipPostSerialize);
	}

	inline static Bool DirectSerializeToTable(
		SerializeContext& rContext,
		DataStore& rDataStore,
		const DataNode& table,
		HString key,
		const T& object,
		Bool bSkipPostSerialize)
	{
		// Create a table to serialize into.
		if (!rDataStore.SetTableToTable(table, key))
		{
			return false;
		}

		DataNode dataNode;
		SEOUL_VERIFY(rDataStore.GetValueFromTable(table, key, dataNode));

		return TableOf<T>().TrySerialize(rContext, rDataStore, dataNode, &object, bSkipPostSerialize);
	}

	virtual Bool DoDeserialize(
		SerializeContext& rContext,
		const DataStore& dataStore,
		const DataNode& dataNode,
		const WeakAny& objectThis,
		Bool bSkipPostSerialize = false,
		Bool bDisableRootCustomDeserializeType = false) const SEOUL_OVERRIDE
	{
		return TableOf<T>().TryDeserialize(rContext, dataStore, dataNode, objectThis, bSkipPostSerialize);
	}

	virtual Bool DoSerializeToArray(
		SerializeContext& rContext,
		DataStore& rDataStore,
		const DataNode& array,
		UInt32 uIndex,
		const WeakAny& objectThis,
		Bool bSkipPostSerialize = false,
		Bool bDisableRootCustomSerializeType = false) const SEOUL_OVERRIDE
	{
		// Create a table to serialize into.
		if (!rDataStore.SetTableToArray(array, uIndex))
		{
			return false;
		}

		DataNode dataNode;
		SEOUL_VERIFY(rDataStore.GetValueFromArray(array, uIndex, dataNode));

		return TableOf<T>().TrySerialize(rContext, rDataStore, dataNode, objectThis, bSkipPostSerialize);
	}

	virtual Bool DoSerializeToTable(
		SerializeContext& rContext,
		DataStore& rDataStore,
		const DataNode& table,
		HString key,
		const WeakAny& objectThis,
		Bool bSkipPostSerialize = false,
		Bool bDisableRootCustomSerializeType = false) const SEOUL_OVERRIDE
	{
		// Create a table to serialize into.
		if (!rDataStore.SetTableToTable(table, key))
		{
			return false;
		}

		DataNode dataNode;
		SEOUL_VERIFY(rDataStore.GetValueFromTable(table, key, dataNode));

		return TableOf<T>().TrySerialize(rContext, rDataStore, dataNode, objectThis, bSkipPostSerialize);
	}

	virtual void FromScript(lua_State* pVm, Int iOffset, const WeakAny& objectThis) const SEOUL_OVERRIDE
	{
		TableOf<T>().FromScript(pVm, iOffset, objectThis);
	}

	virtual void ToScript(lua_State* pVm, const WeakAny& objectThis, Bool bCreateTable = true) const SEOUL_OVERRIDE
	{
		TableOf<T>().ToScript(pVm, objectThis);
	}

	virtual Reflection::Array const* TryGetArray() const SEOUL_OVERRIDE
	{
		return nullptr;
	}

	virtual Reflection::Enum const* TryGetEnum() const SEOUL_OVERRIDE
	{
		SEOUL_STATIC_ASSERT(!IsEnum<T>::Value);
		return nullptr;
	}

	virtual Reflection::Table const* TryGetTable() const SEOUL_OVERRIDE
	{
		return &TableOf<T>();
	}
}; // class TypeT

template <>
class TypeT<char, false, false, true> SEOUL_SEALED : public Type
{
public:
	TypeT(const MethodBuilder& methodBuilder)
		: Type(methodBuilder)
	{
	}

	TypeT(const PropertyBuilder& propertyBuilder)
		: Type(propertyBuilder)
	{
	}

	TypeT(const TypeBuilder& builder)
		: Type(builder)
	{
	}

	~TypeT()
	{
	}

	virtual WeakAny GetPtrUnsafe(void* pUnsafePointer) const SEOUL_OVERRIDE
	{
		return WeakAny(((char*)pUnsafePointer));
	}

	virtual WeakAny GetPtrUnsafe(void const* pUnsafePointer) const SEOUL_OVERRIDE
	{
		return WeakAny(((char const*)pUnsafePointer));
	}

	inline static Bool DirectDeserialize(
		SerializeContext& rContext,
		const DataStore& dataStore,
		const DataNode& dataNode,
		char& rObject,
		Bool bSkipPostSerialize)
	{
		if (!Seoul::FromDataNode(rContext, dataStore, dataNode, rObject))
		{
			if (!rContext.HandleError(SerializeError::kFailedSettingValue))
			{
				return false;
			}
		}

		return true;
	}

	inline static Bool DirectSerializeToArray(
		SerializeContext& rContext,
		DataStore& rDataStore,
		const DataNode& array,
		UInt32 uIndex,
		const char& object,
		Bool bSkipPostSerialize)
	{
		if (!Seoul::ToDataStoreArray(rContext, rDataStore, array, uIndex, object))
		{
			if (!rContext.HandleError(SerializeError::kFailedGettingValue))
			{
				return false;
			}
		}

		return true;
	}

	inline static Bool DirectSerializeToTable(
		SerializeContext& rContext,
		DataStore& rDataStore,
		const DataNode& table,
		HString key,
		const char& object,
		Bool bSkipPostSerialize)
	{
		if (!Seoul::ToDataStoreTable(rContext, rDataStore, table, key, object))
		{
			if (!rContext.HandleError(SerializeError::kFailedGettingValue))
			{
				return false;
			}
		}

		return true;
	}

	virtual Bool DoDeserialize(
		SerializeContext& rContext,
		const DataStore& dataStore,
		const DataNode& dataNode,
		const WeakAny& objectThis,
		Bool bSkipPostSerialize = false,
		Bool bDisableRootCustomDeserializeType = false) const SEOUL_OVERRIDE
	{
		// Get the object - if this fails, we must fail, as there is nothing more to do.
		char* p = nullptr;
		if (!PointerCast(objectThis, p))
		{
			return false;
		}
		SEOUL_ASSERT(nullptr != p);

		if (!Seoul::FromDataNode(rContext, dataStore, dataNode, *p))
		{
			if (!rContext.HandleError(SerializeError::kFailedSettingValue))
			{
				return false;
			}
		}

		return true;
	}

	virtual Bool DoSerializeToArray(
		SerializeContext& rContext,
		DataStore& rDataStore,
		const DataNode& array,
		UInt32 uIndex,
		const WeakAny& objectThis,
		Bool bSkipPostSerialize = false,
		Bool bDisableRootCustomSerializeType = false) const SEOUL_OVERRIDE
	{
		// Get the object - if this fails, we must fail, as there is nothing more to do.
		char const* p = nullptr;
		if (!PointerCast(objectThis, p))
		{
			return false;
		}
		SEOUL_ASSERT(nullptr != p);

		if (!Seoul::ToDataStoreArray(rContext, rDataStore, array, uIndex, *p))
		{
			if (!rContext.HandleError(SerializeError::kFailedGettingValue))
			{
				return false;
			}
		}

		return true;
	}

	virtual Bool DoSerializeToTable(
		SerializeContext& rContext,
		DataStore& rDataStore,
		const DataNode& table,
		HString key,
		const WeakAny& objectThis,
		Bool bSkipPostSerialize = false,
		Bool bDisableRootCustomSerializeType = false) const SEOUL_OVERRIDE
	{
		// Get the object - if this fails, we must fail, as there is nothing more to do.
		char const* p = nullptr;
		if (!PointerCast(objectThis, p))
		{
			return false;
		}
		SEOUL_ASSERT(nullptr != p);

		if (!Seoul::ToDataStoreTable(rContext, rDataStore, table, key, *p))
		{
			if (!rContext.HandleError(SerializeError::kFailedGettingValue))
			{
				return false;
			}
		}

		return true;
	}

	virtual void FromScript(lua_State* pVm, Int iOffset, const WeakAny& objectThis) const SEOUL_OVERRIDE
	{
		// Special handling for cstrings.
		if (SEOUL_LIKELY(objectThis.GetTypeInfo() == TypeId<Byte const**>()))
		{
			Byte const* s = nullptr;
			DataNodeHandler<Byte const*, false>::FromScript(pVm, iOffset, s);
			*objectThis.Cast<Byte const**>() = s;
		}
		else
		{
			// Get the object - if this fails, we must fail, as there is nothing more to do.
			char* p = nullptr;
			if (!PointerCast(objectThis, p))
			{
				// TODO: I think I can just assert here, as all
				// callers of this context will have enforced this to be true.
				return;
			}
			SEOUL_ASSERT(nullptr != p);

			// Pass handling off to the type's DataNodeHandler.
			DataNodeHandler<char, false>::FromScript(pVm, iOffset, *p);
		}
	}

	virtual void ToScript(lua_State* pVm, const WeakAny& objectThis, Bool bCreateTable = true) const SEOUL_OVERRIDE
	{
		// Special handling for cstrings.
		if (SEOUL_LIKELY(objectThis.GetTypeInfo() == TypeId<Byte const**>()))
		{
			DataNodeHandler<Byte const*, false>::ToScript(pVm, *objectThis.Cast<Byte const**>());
		}
		else if (objectThis.GetTypeInfo() == TypeId<Byte const** const>())
		{
			DataNodeHandler<Byte const*, false>::ToScript(pVm, *objectThis.Cast<Byte const** const>());
		}
		else
		{
			// Get the object - if this fails, we must fail, as there is nothing more to do.
			char const* p = nullptr;
			if (!PointerCast(objectThis, p))
			{
				// TODO: I think I can just assert here, as all
				// callers of this context will have enforced this to be true.
				lua_pushnil(pVm);
				return;
			}
			SEOUL_ASSERT(nullptr != p);

			// Pass handling off to the type's DataNodeHandler.
			DataNodeHandler<char, false>::ToScript(pVm, *p);
		}
	}

	virtual Reflection::Array const* TryGetArray() const SEOUL_OVERRIDE
	{
		return nullptr;
	}

	virtual Reflection::Enum const* TryGetEnum() const SEOUL_OVERRIDE
	{
		return nullptr;
	}

	virtual Reflection::Table const* TryGetTable() const SEOUL_OVERRIDE
	{
		return nullptr;
	}
}; // class TypeT

template <>
class TypeT<void, false, false, false> SEOUL_SEALED : public Type
{
public:
	TypeT(const MethodBuilder& methodBuilder)
		: Type(methodBuilder)
	{
	}

	TypeT(const PropertyBuilder& propertyBuilder)
		: Type(propertyBuilder)
	{
	}

	TypeT(const TypeBuilder& builder)
		: Type(builder)
	{
	}

	~TypeT()
	{
	}

	virtual WeakAny GetPtrUnsafe(void* pUnsafePointer) const SEOUL_OVERRIDE
	{
		return WeakAny(pUnsafePointer);
	}

	virtual WeakAny GetPtrUnsafe(void const* pUnsafePointer) const SEOUL_OVERRIDE
	{
		return WeakAny(pUnsafePointer);
	}

	virtual Bool DoDeserialize(
		SerializeContext& rContext,
		const DataStore& dataStore,
		const DataNode& dataNode,
		const WeakAny& objectThis,
		Bool bSkipPostSerialize = false,
		Bool bDisableRootCustomDeserializeType = false) const SEOUL_OVERRIDE
	{
		return false;
	}

	virtual Bool DoSerializeToArray(
		SerializeContext& rContext,
		DataStore& rDataStore,
		const DataNode& array,
		UInt32 uIndex,
		const WeakAny& objectThis,
		Bool bSkipPostSerialize = false,
		Bool bDisableRootCustomSerializeType = false) const SEOUL_OVERRIDE
	{
		return false;
	}

	virtual Bool DoSerializeToTable(
		SerializeContext& rContext,
		DataStore& rDataStore,
		const DataNode& table,
		HString key,
		const WeakAny& objectThis,
		Bool bSkipPostSerialize = false,
		Bool bDisableRootCustomSerializeType = false) const SEOUL_OVERRIDE
	{
		return false;
	}

	virtual void FromScript(lua_State* pVm, Int iOffset, const WeakAny& objectThis) const SEOUL_OVERRIDE
	{
		// Special handling for light user data.
		if (SEOUL_LIKELY(objectThis.GetTypeInfo() == TypeId<void**>()))
		{
			void* p = nullptr;
			DataNodeHandler<void*, false>::FromScript(pVm, iOffset, p);
			*objectThis.Cast<void**>() = p;
		}
	}

	virtual void ToScript(lua_State* pVm, const WeakAny& objectThis, Bool bCreateTable = true) const SEOUL_OVERRIDE
	{
		// Special handling for light user data.
		if (SEOUL_LIKELY(objectThis.GetTypeInfo() == TypeId<void**>()))
		{
			DataNodeHandler<void*, false>::ToScript(pVm, *objectThis.Cast<void**>());
		}
		else if (SEOUL_LIKELY(objectThis.GetTypeInfo() == TypeId<void** const>()))
		{
			DataNodeHandler<void*, false>::ToScript(pVm, *objectThis.Cast<void** const>());
		}
		else
		{
			lua_pushnil(pVm);
		}
	}

	virtual Reflection::Array const* TryGetArray() const SEOUL_OVERRIDE
	{
		return nullptr;
	}

	virtual Reflection::Enum const* TryGetEnum() const SEOUL_OVERRIDE
	{
		return nullptr;
	}

	virtual Reflection::Table const* TryGetTable() const SEOUL_OVERRIDE
	{
		return nullptr;
	}
}; // class TypeT

template <typename T>
struct TypeTDiscovery
{
	typedef TypeT<
		T,
		FulfillsArrayContract<T>::Value,
		FulfillsTableContract<T>::Value,
		DataNodeHandler<T, IsEnum<T>::Value>::Value> Type;
}; // struct TypeTDiscovery

} // namespace Seoul::Reflection

#endif // include guard
