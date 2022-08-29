/**
 * \file ReflectionType.h
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

#pragma once
#ifndef REFLECTION_TYPE_H
#define REFLECTION_TYPE_H

#include "HashTable.h"
#include "Prereqs.h"
#include "ReflectionAttribute.h"
#include "ReflectionEnum.h"
#include "ReflectionPrereqs.h"
#include "ReflectionProperty.h"
#include "ReflectionScript.h"
#include "ReflectionSimpleCast.h"
#include "ReflectionWeakAny.h"
#include "SeoulHString.h"
#include "SeoulTime.h"
#include "StreamBuffer.h"
#include "Vector.h"

namespace Seoul
{

// Forward declarations
class StreamBuffer;

namespace Reflection
{

// Forward declarations
class Method;
struct MethodBuilder;
struct PropertyBuilder;
struct TypeBuilder;

namespace MethodDetail
{
	struct BasePlaceholder;
}

namespace TypeFlags
{
	/**
	 * Flags that can be used to control Type behavior when defining reflection Types.
	 */
	enum Enum
	{
		kNone = 0,

		/**
		 * If set, Type::New<> will return an invalid WeakAny for the Type - this
		 * can be used if a Type does not have a default constructor, or if you simply
		 * don't want the Type to be created via reflection.
		 */
		kDisableNew = (1 << 0),

		/**
		 * If set, Type::Delete<> will become a nop.
		 */
		kDisableDelete = (1 << 1),

		/**
		 * If set, Type::DefaultCopy() will become a nop.
		 */
		kDisableCopy = (1 << 2),
	};
}

struct NewUtil SEOUL_SEALED
{
	/**
	 * @return A new instance of type T - used to generate a delegate that
	 * can be stored with a Type to allow new instances of that Type to
	 * be create at runtime.
	 */
	template <typename T>
	static WeakAny NewHandler(MemoryBudgets::Type eType)
	{
		T* pReturn = SEOUL_NEW(eType) T;
		WeakAny weakAny(pReturn);

		return weakAny;
	}

	template <typename T>
	static void DeleteHandler(WeakAny& rAny)
	{
		if (rAny.IsValid())
		{
			T* p = rAny.Cast<T*>();
			rAny.Reset();

			delete p;
		}
	}

	template <typename T>
	static WeakAny InPlaceNewHandler(void* pData, size_t zDataSizeInBytes)
	{
		if ((size_t)pData % SEOUL_ALIGN_OF(T) != 0 ||
			zDataSizeInBytes < sizeof(T))
		{
			return WeakAny();
		}

		new (pData) T;
		WeakAny weakAny((T*)pData);
		return weakAny;
	}

	template <typename T>
	static void DestructorHandler(const WeakAny& weakAny)
	{
		T* p = weakAny.Cast<T*>();
		if (nullptr != p)
		{
			p->~T();
		}
	}

	template <typename T>
	static void DefaultCopyHandler(Any& rAny)
	{
		rAny = T();
	}
}; // struct NewUtil

namespace TypeDetail
{

/** New delegate, typically implemented with NullHandler and NullNewHandler. */
typedef WeakAny (*NewDelegate)(MemoryBudgets::Type eType);
typedef void (*DeleteDelegate)(WeakAny& rAny);
typedef WeakAny (*InPlaceNewDelegate)(void* pData, size_t zDataSizeInBytes);
typedef void (*DestructorDelegate)(const WeakAny& any);
typedef void (*DefaultCopyDelegate)(Any& rAny);

/** Template used to select the appropriate delegate to instantiate instances of type T. */
template <Bool B_NEW_DISABLE, typename T> struct NewDeleteHelper;

/**
 * Specialization for void or abstract types - instantiating these types returns an invalid WeakAny,
 * effectively a nullptr.
 */
template <typename T>
struct NewDeleteHelper<true, T>
{
	static NewDelegate GetNewDelegate()
	{
		return nullptr;
	}

	static DeleteDelegate GetDeleteDelegate()
	{
		return nullptr;
	}

	static InPlaceNewDelegate GetInPlaceNewDelegate()
	{
		return nullptr;
	}

	static DestructorDelegate GetDestructorDelegate()
	{
		return nullptr;
	}
};

/**
 * Specialization for types that are not void or abstract - returns a delegate
 * that when called, will return a new instance of type T.
 */
template <typename T>
struct NewDeleteHelper<false, T>
{
	static NewDelegate GetNewDelegate()
	{
		return NewUtil::NewHandler<T>;
	}

	static DeleteDelegate GetDeleteDelegate()
	{
		return NewUtil::DeleteHandler<T>;
	}

	static InPlaceNewDelegate GetInPlaceNewDelegate()
	{
		return NewUtil::InPlaceNewHandler<T>;
	}

	static DestructorDelegate GetDestructorDelegate()
	{
		return NewUtil::DestructorHandler<T>;
	}
};

/** Template used to select the appropriate delegate to instantiate instances of type T. */
template <Bool B_DISABLE_DEFAULT_COPY, typename T> struct DefaultCopyHelper;

/**
 * Specialization for non-copyable types.
 */
template <typename T>
struct DefaultCopyHelper<true, T>
{
	static DefaultCopyDelegate GetDefaultCopyDelegate()
	{
		return nullptr;
	}
};

/**
 * Specialization for copyable types.
 */
template <typename T>
struct DefaultCopyHelper<false, T>
{
	static DefaultCopyDelegate GetDefaultCopyDelegate()
	{
		return NewUtil::DefaultCopyHandler<T>;
	}
};

/**
 * @return A function pointer to a function that when called will return a defaul constructed instance of the type T.
 */
template <typename T, int FLAGS = 0>
struct NewDelegateBind
{
	static NewDelegate GetNewDelegate()
	{
		const Bool kbDisableNew =
			(0 != (TypeFlags::kDisableNew & FLAGS)) ||
			IsAbstract<T>::Value;
		return NewDeleteHelper<kbDisableNew, T>::GetNewDelegate();
	}
};

template <typename T, int FLAGS = 0>
struct DeleteDelegateBind
{
	static DeleteDelegate GetDeleteDelegate()
	{
		return NewDeleteHelper<(0 != (TypeFlags::kDisableDelete & FLAGS)), T>::GetDeleteDelegate();
	}
};

template <typename T, int FLAGS = 0>
struct InPlaceNewDelegateBind
{
	static InPlaceNewDelegate GetInPlaceNewDelegate()
	{
		const Bool kbDisableNew =
			(0 != (TypeFlags::kDisableNew & FLAGS)) ||
			IsAbstract<T>::Value;
		return NewDeleteHelper<kbDisableNew, T>::GetInPlaceNewDelegate();
	}
};

template <typename T, int FLAGS = 0>
struct DestructorDelegateBind
{
	static DestructorDelegate GetDestructorDelegate()
	{
		return NewDeleteHelper<(0 != (TypeFlags::kDisableDelete & FLAGS)) || IsTriviallyDestructible<T>::Value, T>::GetDestructorDelegate();
	}
};

template <typename T, int FLAGS = 0>
struct DefaultCopyDelegateBind
{
	static const Bool kbDisableCopy =
		(0 != (TypeFlags::kDisableNew & FLAGS)) ||
		(0 != (TypeFlags::kDisableDelete & FLAGS)) ||
		(0 != (TypeFlags::kDisableCopy & FLAGS)) ||
		IsAbstract<T>::Value;
	typedef T Type;
};

template <typename BINDER>
static DefaultCopyDelegate GetDefaultCopyDelegate(void*)
{
	return DefaultCopyHelper< BINDER::kbDisableCopy, typename BINDER::Type >::GetDefaultCopyDelegate();
}

} // namespace TypeDetail

/**
 * Type is the main class used for reflection. It contains members to reflect
 * on constructors and properties of a type, generated new instances of a type at runtime,
 * and query for attributes of a type, which can be used to add metadata to a class definition.
 */
class Type SEOUL_ABSTRACT
{
public:
	/**
	 * @return A new instance of the type described by this Type, cast
	 * to T, or nullptr if the cast is not valid, or the type described by this
	 * Type cannot be instantiated.
	 */
	template <typename T>
	T* New(MemoryBudgets::Type eType = MemoryBudgets::Reflection) const
	{
		typedef typename RemoveConstVolatile<T>::Type Type;

		if (m_NewDelegate)
		{
			WeakAny weakAny = m_NewDelegate(eType);

			Type* pReturn = nullptr;
			if (CastTo<Type>(weakAny, pReturn))
			{
				return pReturn;
			}
			else
			{
				m_DeleteDelegate(weakAny);
			}
		}

		return nullptr;
	}

	WeakAny New(MemoryBudgets::Type eType = MemoryBudgets::Reflection) const
	{
		if (m_NewDelegate)
		{
			return m_NewDelegate(eType);
		}
		else
		{
			return WeakAny();
		}
	}

	/**
	 * Delete the instance of this Type in rWeakAny, or
	 * reset rWeakAny to its invalid state of this Type does not
	 * have New/Delete handlers registered.
	 */
	void Delete(WeakAny& rWeakAny) const
	{
		if (m_DeleteDelegate)
		{
			m_DeleteDelegate(rWeakAny);
		}
		else
		{
			rWeakAny.Reset();
		}
	}

	WeakAny InPlaceNew(void* pData, size_t zDataSizeInBytes) const
	{
		if (m_InPlaceNewDelegate)
		{
			return m_InPlaceNewDelegate(pData, zDataSizeInBytes);
		}
		else
		{
			return WeakAny();
		}
	}

	Bool HasDestructorDelegate() const
	{
		return (nullptr != m_DestructorDelegate);
	}

	void InvokeDestructor(WeakAny& rWeakAny) const
	{
		if (m_DestructorDelegate)
		{
			auto weakAny = rWeakAny;
			rWeakAny.Reset();
			m_DestructorDelegate(weakAny);
		}
	}

	void DefaultCopy(Any& rAny) const
	{
		if (m_DefaultCopyDelegate)
		{
			m_DefaultCopyDelegate(rAny);
		}
		else
		{
			rAny.Reset();
		}
	}

	virtual ~Type();

	/**
	 * @return A WeakAny that wraps a read-write pointer pUnsafePointer to the Type represented
	 * by this Reflection::Type object.
	 *
	 * \pre pUnsafePointer must point to an object that exactly matches the Type
	 * represented by this Reflection::Type object, or the value of the returned
	 * pointer is undefined.
	 */
	virtual WeakAny GetPtrUnsafe(void* pUnsafePointer) const = 0;

	/**
	 * @return A WeakAny that wraps a read-only pointer pUnsafePointer to the Type represented
	 * by this Reflection::Type object.
	 *
	 * \pre pUnsafePointer must point to an object that exactly matches the Type
	 * represented by this Reflection::Type object, or the value of the returned
	 * pointer is undefined.
	 */
	virtual WeakAny GetPtrUnsafe(void const* pUnsafePointer) const = 0;

	/**
	 * @return True if this Type is equal to b, false otherwise.
	 *
	 * Two Types are only equal if they have the same memory address (they
	 * are the same object).
	 */
	Bool operator==(const Type& b) const
	{
		return (this == &b);
	}

	/**
	 * @return True if this Type is *not* equal to b, false otherwise.
	 *
	 * Two Types are only equal if they have the same memory address (they
	 * are the same object).
	 */
	Bool operator!=(const Type& b) const
	{
		return !(*this == b);
	}

	/** @return True if this type can be instantiated via New(), false otherwise. */
	Bool CanNew() const
	{
		return (nullptr != m_NewDelegate);
	}

	/**
	 * @return The unique name that identifies this Type - typically equal to
	 * the name of the class/struct of the C++ type that this Type object
	 * describes.
	 */
	HString GetName() const
	{
		return m_Name;
	}

	/**
	 * @return The TypeInfo of the type described by this Type.
	 */
	const TypeInfo& GetTypeInfo() const
	{
		return m_TypeInfo;
	}

	/**
	 * @return Type level attributes of the type described by this Type.
	 */
	const AttributeCollection& GetAttributes() const
	{
		return m_Attributes;
	}

	/**
	 * @return A non-null pointer to type T if this Type or any of its
	 * parents contain an attribute of type T, or nullptr if the attribute
	 * was not found.
	 *
	 * @param[in] bCheckParents - if true, parents will be traversed for
	 * the attribute, otherwise only this Type will be checked.
	 */
	template <typename T>
	T const* GetAttribute(Bool bCheckParents = false) const
	{
		T const* pReturn = GetAttributes().GetAttribute<T>();
		if (nullptr != pReturn)
		{
			return pReturn;
		}

		if (bCheckParents)
		{
			UInt32 const nParents = m_vParents.GetSize();
			for (UInt32 i = 0u; i < nParents; ++i)
			{
				pReturn = m_vParents[i].First().GetAttribute<T>(bCheckParents);
				if (nullptr != pReturn)
				{
					return pReturn;
				}
			}
		}

		return nullptr;
	}

	/**
	 * @return True if this Type has attribute T, false otherwise. If
	 * bCheckParents is true, also recursively checks parents for the
	 * attribute.
	 */
	template <typename T>
	Bool HasAttribute(Bool bCheckParents = false) const
	{
		if (GetAttributes().HasAttribute<T>())
		{
			return true;
		}

		if (bCheckParents)
		{
			UInt32 const nParents = m_vParents.GetSize();
			for (UInt32 i = 0u; i < nParents; ++i)
			{
				if (m_vParents[i].First().HasAttribute<T>(bCheckParents))
				{
					return true;
				}
			}
		}

		return false;
	}

	/**
	 * Attempt to cast the object in in to
	 * the concrete type C, where in contains a pointer
	 * to an object of this Type's type.
	 */
	template <typename C>
	Bool CastTo(const WeakAny& in, C const*& rpOut) const
	{
		if (in.IsOfType<C*>())
		{
			rpOut = in.Cast<C*>();
			return true;
		}
		else if (in.IsOfType<C const*>())
		{
			rpOut = in.Cast<C const*>();
			return true;
		}

		UInt32 const nImmediateParents = m_vParents.GetSize();
		for (UInt32 i = 0u; i < nImmediateParents; ++i)
		{
			WeakAny downCast(in);
			if (m_vParents[i].Second(downCast))
			{
				if (m_vParents[i].First().CastTo(downCast, rpOut))
				{
					return true;
				}
			}
		}

		return false;
	}

	/**
	 * Attempt to cast the object in in to
	 * the concrete type C, where in contains a pointer
	 * to an object of this Type's type.
	 */
	template <typename C>
	Bool CastTo(const WeakAny& in, C*& rpOut) const
	{
		if (in.IsOfType<C*>())
		{
			rpOut = in.Cast<C*>();
			return true;
		}

		UInt32 const nImmediateParents = m_vParents.GetSize();
		for (UInt32 i = 0u; i < nImmediateParents; ++i)
		{
			WeakAny downCast(in);
			if (m_vParents[i].Second(downCast))
			{
				if (m_vParents[i].First().CastTo(downCast, rpOut))
				{
					return true;
				}
			}
		}

		return false;
	}

	/**
	 * @return The method i of this Type.
	 *
	 * \pre i must be < GetMethodCount() or this method will SEOUL_FAIL().
	 */
	Method const* GetMethod(UInt32 i) const
	{
		return m_vMethods[i];
	}

	Method const* GetMethod(HString name) const;

	/**
	 * @return The number of methods that types of this Type have.
	 */
	UInt32 GetMethodCount() const
	{
		return m_vMethods.GetSize();
	}

	/**
	 * @return The number of parents that types of this Type have.
	 */
	UInt32 GetParentCount() const
	{
		return m_vParents.GetSize();
	}

	/**
	 * @return Type of parent i of types of this Type.
	 *
	 * \pre i must be < GetParentCount() or this method will SEOUL_FAIL().
	 */
	Type const* GetParent(UInt32 i) const
	{
		return &(m_vParents[i].First());
	}

	/**
	 * @return A pair in which the First element is a delegate that will resolve
	 * to a Type which is a parent of this Type, and the Second element is a delegate
	 * that when invoked on a WeakAny value, will cast that WeakAny to the type of the First
	 * element.
	 */
	const TypePair& GetParentPair(UInt32 i) const
	{
		return m_vParents[i];
	}

	// Attempt to get a parent with name, or nullptr if types of this Type
	// do not have a parent name.
	//
	// Note that this funciton traverses the inheritance graph of the
	// type and will return immediate as well as distance parents
	// of the type.
	Type const* GetParent(HString name) const;

	/**
	 * @return The number of properties of types of this Type.
	 */
	UInt32 GetPropertyCount() const
	{
		return m_vProperties.GetSize();
	}

	/**
	 * @return The i property of this Type.
	 *
	 * i must be < GetPropertyCount() or this method will SEOUL_FAIL().
	 */
	Property const* GetProperty(UInt32 i) const
	{
		return m_vProperties[i];
	}

	// Get a property of this type, or nullptr if no such property exists.
	// Note that this function will traverse the parent hierarchy of
	// this Type, so it will return Properties of parent classes
	// of this type.
	Property const* GetProperty(HString name) const;

	/**
	 * Return true if the Reflection system can cast instances of this
	 * Type to the concrete type, false otherwise.
	 */
	Bool IsSubclassOf(const Type& type) const
	{
		UInt32 const nImmediateParents = m_vParents.GetSize();
		for (UInt32 i = 0u; i < nImmediateParents; ++i)
		{
			const Type& parentType = m_vParents[i].First();
	 		if (type == parentType)
			{
				return true;
			}

			if (parentType.IsSubclassOf(type))
			{
				return true;
	 		}
		}

		return false;
	}

	/**
	 * Try to deserialize data in dataNode into objectThis,
	 * where objectThis is a read-write pointer to the object to
	 * be deserialized.
	 *
	 * @return True if deserialization was successful, false otherwise. If
	 * this function returns false, the object in objectThis may be
	 * in an incomplete deserialized state.
	 */
	static Bool TryDeserialize(
		SerializeContext& rContext,
		const DataStore& dataStore,
		const DataNode& dataNode,
		const WeakAny& objectThis,
		Bool bSkipPostSerialize = false,
		Bool bDisableRootCustomDeserializeType = false)
	{
		return objectThis.GetType().DoDeserialize(
			rContext,
			dataStore,
			dataNode,
			objectThis,
			bSkipPostSerialize,
			bDisableRootCustomDeserializeType);
	}

	/**
	 * Try to serialize the state of objectThis into an
	 * array element at uIndex in array, where objectThis
	 * is a read pointer to the object to be serialized.
	 *
	 * @return True if serialization was successful, false otherwise. If
	 * this function returns false, the DataStore may be in a partially
	 * serialized, modified state.
	 */
	static Bool TrySerializeToArray(
		SerializeContext& rContext,
		DataStore& rDataStore,
		const DataNode& array,
		UInt32 uIndex,
		const WeakAny& objectThis,
		Bool bSkipPostSerialize = false,
		Bool bDisableRootCustomSerializeType = false)
	{
		return objectThis.GetType().DoSerializeToArray(
			rContext,
			rDataStore,
			array,
			uIndex,
			objectThis,
			bSkipPostSerialize,
			bDisableRootCustomSerializeType);
	}

	/**
	 * Try to serialize the state of objectThis into a
	 * table element at key in table, where objectThis
	 * is a read pointer to the object to be serialized.
	 *
	 * @return True if serialization was successful, false otherwise. If
	 * this function returns false, the DataStore may be in a partially
	 * serialized, modified state.
	 */
	static Bool TrySerializeToTable(
		SerializeContext& rContext,
		DataStore& rDataStore,
		const DataNode& table,
		HString key,
		const WeakAny& objectThis,
		Bool bSkipPostSerialize = false,
		Bool bDisableRootCustomSerializeType = false)
	{
		return objectThis.GetType().DoSerializeToTable(
			rContext,
			rDataStore,
			table,
			key,
			objectThis,
			bSkipPostSerialize,
			bDisableRootCustomSerializeType);
	}

	// Get a Reflection::Array object that can be used
	// to manipulate an instance of this Type as an array,
	// or nullptr if this type does not fulfill the contract
	// of an array.
	virtual Reflection::Array const* TryGetArray() const = 0;

	// Get a Reflection::Enum object that can be used
	// to get more information about this type's enumeration,
	// or nullptr if this type is not an enumeration.
	virtual Reflection::Enum const* TryGetEnum() const = 0;

	// Get a Reflection::Table object that can be used
	// to manipulate an instance of this Type as a table,
	// or nullptr if this type does not fulfill the contract
	// of a table.
	virtual Reflection::Table const* TryGetTable() const = 0;

	// Populate the type pointer in objectThis with data from script at iOffset.
	virtual void FromScript(lua_State* pVm, Int iOffset, const WeakAny& objectThis) const;

	// Push the state of the object in objectThis onto the script stack.
	virtual void ToScript(lua_State* pVm, const WeakAny& objectThis, Bool bCreateTable = true) const;

	/**
	 * Call Registry::GetType(UInt32) with this value to retrieve
	 * this type data. Can be used in contexts as an effective
	 * handle to the type info when the handle data can be
	 * only 32-bits.
	 */
	UInt32 GetRegistryIndex() const
	{
		return m_uRegistryIndex;
	}

protected:
	Type(const MethodBuilder& methodBuilder);
	Type(const PropertyBuilder& propertyBuilder);
	Type(const TypeBuilder& builder);

	virtual Bool DoDeserialize(
		SerializeContext& rContext,
		const DataStore& dataStore,
		const DataNode& table,
		const WeakAny& objectThis,
		Bool bSkipPostSerialize = false,
		Bool bDisableRootCustomDeserializeType = false) const = 0;

	virtual Bool DoSerializeToArray(
		SerializeContext& rContext,
		DataStore& rDataStore,
		const DataNode& array,
		UInt32 uIndex,
		const WeakAny& objectThis,
		Bool bSkipPostSerialize = false,
		Bool bDisableRootCustomSerializeType = false) const = 0;

	virtual Bool DoSerializeToTable(
		SerializeContext& rContext,
		DataStore& rDataStore,
		const DataNode& table,
		HString key,
		const WeakAny& objectThis,
		Bool bSkipPostSerialize = false,
		Bool bDisableRootCustomSerializeType = false) const = 0;

	static Bool DoGenericDeserialize(
		UInt32& ruProperties,
		SerializeContext& rContext,
		const DataStore& dataStore,
		const DataNode& table,
		const WeakAny& objectThis,
		const Type& mostDerivedType,
		Bool bSkipPostSerialize = false,
		Bool bInParent = false);

	static Bool DoGenericSerialize(
		UInt32& ruProperties,
		SerializeContext& rContext,
		DataStore& rDataStore,
		const DataNode& table,
		const WeakAny& objectThis,
		Bool bSkipPostSerialize = false,
		Bool bInParent = false);

private:
	void InternalConstruct(const TypeBuilder& rBuilder);

	typedef HashTable<HString, Method const*, MemoryBudgets::Reflection> MethodTable;
	typedef HashTable<HString, Property const*, MemoryBudgets::Reflection> PropertyTable;

	AliasTable m_tAliases;
	AttributeCollection m_Attributes;
	MethodVector m_vMethods;
	TypeVector m_vParents;
	PropertyVector m_vProperties;
	MethodTable m_tMethods;
	PropertyTable m_tProperties;

	const TypeInfo& m_TypeInfo;
	TypeDetail::NewDelegate m_NewDelegate;
	TypeDetail::DeleteDelegate m_DeleteDelegate;
	TypeDetail::InPlaceNewDelegate m_InPlaceNewDelegate;
	TypeDetail::DestructorDelegate m_DestructorDelegate;
	TypeDetail::DefaultCopyDelegate m_DefaultCopyDelegate;
	UInt32 m_uRegistryIndex;
	HString m_Name;

	SEOUL_DISABLE_COPY(Type);
}; // class Type

/**
 * Assign a pointer of type T to rpValue on success, or leave rpValue
 * unchanged otherwise.
 *
 * @return True if a pointer can be assigned to rpValue, false otherwise.
 */
template <typename T>
inline Bool Property::TryGetPtr(const WeakAny& thisPointer, T*& rpValue) const
{
	WeakAny value;
	if (TryGetPtr(thisPointer, value))
	{
		return value.GetType().CastTo(value, rpValue);
	}

	return false;
}

/**
 * Assign a read-only pointer of type T to rpValue on success, or leave rpValue
 * unchanged otherwise.
 *
 * @return True if a pointer can be assigned to rpValue, false otherwise.
 */
template <typename T>
inline Bool Property::TryGetConstPtr(const WeakAny& thisPointer, T const*& rpValue) const
{
	WeakAny value;
	if (TryGetConstPtr(thisPointer, value))
	{
		return value.GetType().CastTo(value, rpValue);
	}

	return false;
}

/**
 * PointerCast is a utility function that, given a pointer stored in in,
 * attempts to cast that pointer to the concrete type C, and then assign it to rpOut.
 */
template <typename C>
inline Bool PointerCast(const WeakAny& inPointer, C const*& rpOut)
{
	if (inPointer.IsOfType<C const*>())
	{
		rpOut = inPointer.Cast<C const*>();
		return true;
	}
	else if (inPointer.IsOfType<C*>())
	{
		rpOut = inPointer.Cast<C*>();
		return true;
	}
	else
	{
		const Type& derivedType = inPointer.GetTypeInfo().GetType();

		return derivedType.CastTo(inPointer, rpOut);
	}
}

template <typename C>
inline Bool PointerCast(const WeakAny& inPointer, C*& rpOut)
{
	if (inPointer.IsOfType<C*>())
	{
		rpOut = inPointer.Cast<C*>();
		return true;
	}
	else
	{
		const Type& derivedType = inPointer.GetTypeInfo().GetType();

		return derivedType.CastTo(inPointer, rpOut);
	}
}

template <typename A, Bool IS_ENUM> struct TypeConstructHelper;
template <typename A>
struct TypeConstructHelper<A, false>
{
	// Specialized when A is specialized on
	// a type that is not an enum.
	static Bool TypeConstruct(const WeakAny& in, A& rOut)
	{
		if (in.IsOfType<A>())
		{
			rOut = in.Cast<A>();
			return true;
		}

#if defined(__GNUC__)
		// TODO: Workaround for erroneous -Wmaybe-uninitialized
		// warning-as-error in GCC, revisit.
		rOut = A();
#endif // /#if defined(__GNUC__)

		return false;
	}
}; // struct TypeConstructHelper<A, false>

template <typename A>
struct TypeConstructHelper<A, true>
{
	// Specialized implementation when A is an enum.
	static Bool TypeConstruct(const WeakAny& in, A& rOut)
	{
		// Special handling for <string type> -> Enum.
		const Enum& e = EnumOf<A>();
		const TypeInfo& typeInfo = in.GetTypeInfo();
		switch (typeInfo.GetSimpleTypeInfo())
		{
		case SimpleTypeInfo::kCString:
			{
				Int iValue = -1;
				if (e.TryGetValue(HString(in.Cast<Byte const*>()), iValue))
				{
					rOut = (A)iValue;
					return true;
				}
			}
			return false;
		case SimpleTypeInfo::kHString:
			{
				Int iValue = -1;
				if (e.TryGetValue(HString(in.Cast<HString>()), iValue))
				{
					rOut = (A)iValue;
					return true;
				}
			}
			return false;
		case SimpleTypeInfo::kString:
			{
				Int iValue = -1;
				if (e.TryGetValue(HString(in.Cast<String>()), iValue))
				{
					rOut = (A)iValue;
					return true;
				}
			}
			return false;
		default:
			// Default handling - if the type converts to an Int32, cast
			// to an Enum.
			{
				Int32 iValue = -1;
				if (SimpleCast(in, iValue))
				{
					rOut = (A)iValue;
					return true;
				}
			}
			return false;
		};
	}
}; // struct TypeConstructHelper<A, true>

/**
 * Utility function that, given any arbitrary value in in, attempts to copy
 * construct it to a value of type A and assign it to rOut.
 */
template <typename A>
inline Bool TypeConstruct(const WeakAny& in, A& rOut)
{
	return TypeConstructHelper<A, IsEnum<A>::Value>::TypeConstruct(in, rOut);
}

template <>
inline Bool TypeConstruct<String>(const WeakAny& in, String& rOut)
{
	const TypeInfo& typeInfo = in.GetTypeInfo();
	switch (typeInfo.GetSimpleTypeInfo())
	{
	case SimpleTypeInfo::kCString:
		rOut = String(in.Cast<Byte const*>());
		return true;
	case SimpleTypeInfo::kHString:
		rOut = String(in.Cast<HString>());
		return true;
	case SimpleTypeInfo::kString:
		rOut = in.Cast<String>();
		return true;
	default:
		return false;
	};
}

template <>
inline Bool TypeConstruct<HString>(const WeakAny& in, HString& rOut)
{
	const TypeInfo& typeInfo = in.GetTypeInfo();
	switch (typeInfo.GetSimpleTypeInfo())
	{
	case SimpleTypeInfo::kCString:
		rOut = HString(in.Cast<Byte const*>());
		return true;
	case SimpleTypeInfo::kHString:
		rOut = in.Cast<HString>();
		return true;
	case SimpleTypeInfo::kString:
		rOut = HString(in.Cast<String>());
		return true;
	default:
		return false;
	};
}

template <>
inline Bool TypeConstruct<WorldTime>(const WeakAny& in, WorldTime& rOut)
{
	const TypeInfo& typeInfo = in.GetTypeInfo();
	switch (typeInfo.GetSimpleTypeInfo())
	{
	case SimpleTypeInfo::kCString:
		rOut = WorldTime::ParseISO8601DateTime(String(in.Cast<Byte const*>()));
		return true;
	case SimpleTypeInfo::kHString:
		rOut = WorldTime::ParseISO8601DateTime(String(in.Cast<HString>()));
		return true;
	case SimpleTypeInfo::kString:
		rOut = WorldTime::ParseISO8601DateTime(in.Cast<String>());
		return true;
	default:
		if (TypeConstructHelper<WorldTime, IsEnum<WorldTime>::Value>::TypeConstruct(in, rOut))
		{
			return true;
		}
		else
		{
			Int64 iMicroseconds = 0;
			if (SimpleCast(in, iMicroseconds))
			{
				rOut.SetMicroseconds(iMicroseconds);
				return true;
			}
		}
		return false;
	};
}

template <> inline Bool TypeConstruct(const WeakAny& in, Bool& rOut) { return SimpleCast(in, rOut); }
template <> inline Bool TypeConstruct(const WeakAny& in, Float32& rOut) { return SimpleCast(in, rOut); }
template <> inline Bool TypeConstruct(const WeakAny& in, Float64& rOut) { return SimpleCast(in, rOut); }
template <> inline Bool TypeConstruct(const WeakAny& in, Int8& rOut) { return SimpleCast(in, rOut); }
template <> inline Bool TypeConstruct(const WeakAny& in, Int16& rOut) { return SimpleCast(in, rOut); }
template <> inline Bool TypeConstruct(const WeakAny& in, Int32& rOut) { return SimpleCast(in, rOut); }
template <> inline Bool TypeConstruct(const WeakAny& in, Int64& rOut) { return SimpleCast(in, rOut); }
template <> inline Bool TypeConstruct(const WeakAny& in, UInt8& rOut) { return SimpleCast(in, rOut); }
template <> inline Bool TypeConstruct(const WeakAny& in, UInt16& rOut) { return SimpleCast(in, rOut); }
template <> inline Bool TypeConstruct(const WeakAny& in, UInt32& rOut) { return SimpleCast(in, rOut); }
template <> inline Bool TypeConstruct(const WeakAny& in, UInt64& rOut) { return SimpleCast(in, rOut); }

} // namespace Reflection

// TODO: Wrap the implementation in a helper structure
// so it can support by reference casts.

template <typename TO, typename FROM>
inline static TO DynamicCast(FROM p)
{
	using namespace Reflection;

	if (nullptr == p)
	{
		return nullptr;
	}

	WeakAny const weakAny = p->GetReflectionThis();

	TO pReturn = nullptr;
	if (!PointerCast(weakAny, pReturn))
	{
		return nullptr;
	}

	return pReturn;
}

/**
 * This macro can be used to encourage the linker to include a class that is not referenced outside its
 * compiland.
 *
 * This is most likely to happen when a class is completely isolated (bound into the app only via reflection,
 * e.g. commonly the case with classes that implement the CommandLineInstance attribute).
 *
 * However, it can also happen for classes for which the reflection infrastructure is not otherwise
 * accessed. e.g. if TypeOf<Foo>() is never called, it is possible for the reflection infrastructure
 * of Foo to be optimized out and then elided by the compiler.
 *
 * The method calls below exist to force the function to be a hint to the compiler. We must call
 * at least one method that is defined in a CPP file (so it cannot be inlined and potentially
 * optimized away). This of course only works because we don't use link time code generation.
 *
 * In short, it would be really nice if there was a standard way to define a function as
 * "always link me", but there is not, so we have to fallback on tricks. In future,
 * we might try attribute(used) (for compilers that support it) on the reflection
 * infrastructure global variables, but this has not been reliable in past attempts.
 */
#define SEOUL_LINK_ME_COMMON(type) \
	namespace Reflection\
	{\
		namespace TypeDetailDummyFunctions_DO_NOT_CALL\
		{\
			void SEOUL_CONCAT(DummyFunction, __LINE__)(type&)\
			{\
				auto const& t = Seoul::TypeOf<type>();\
				(void)t.TryGetArray();\
				(void)t.TryGetEnum();\
				(void)t.TryGetTable();\
			}\
		}\
	}
#define SEOUL_LINK_ME(type_class, type)\
	type_class type; \
	SEOUL_LINK_ME_COMMON(type)
#define SEOUL_LINK_ME_NS(type_class, ns, type) \
	namespace ns { type_class type; } \
	SEOUL_LINK_ME_COMMON(ns::type)

} // namespace Seoul

#endif // include guard
