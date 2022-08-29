/**
 * \file ReflectionPrereqs.h
 * \brief Miscellaneous constants, declarations, and utility functions used throughout the Reflection project -
 * equivalent to the Core Prereqs.h header.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef REFLECTION_PREREQS_H
#define REFLECTION_PREREQS_H

#include "HashTable.h"
#include "ContentKey.h"
#include "DataStoreParser.h"
#include "DataStore.h"
#include "Delegate.h"
#include "FixedArray.h"
#include "Pair.h"
#include "Prereqs.h"
#include "ReflectionWeakAny.h"
#include "Vector.h"

namespace Seoul::Reflection
{

// Forward declarations
class Attribute;
class Enum;
class Method;
class Property;
class Type;

// Containers and helpers used throughout the Reflection namespace.
typedef const Type& (*TypeDelegate)();
typedef Bool (*ReflectionCastDelegate)(WeakAny& rCastTarget);
typedef Vector<HString, MemoryBudgets::Reflection> AliasVector;
typedef HashTable<HString, HString, MemoryBudgets::Reflection> AliasTable;
typedef Vector<Attribute*, MemoryBudgets::Reflection> AttributeVector;
typedef Vector<HString, MemoryBudgets::Reflection> EnumNameVector;
typedef Vector<Int, MemoryBudgets::Reflection> EnumValueVector;
typedef Vector<Method*, MemoryBudgets::Reflection> MethodVector;
typedef Vector<Property*, MemoryBudgets::Reflection> PropertyVector;
typedef Pair<TypeDelegate, ReflectionCastDelegate> TypePair;
typedef Vector<TypePair, MemoryBudgets::Reflection> TypeVector;

/**
 * Safe cast that uses Seoul::Reflection features to attempt to cast a pointer stored in
 * rCastTarget to/from another type, either its child or its parent.
 */
template <typename T, typename PARENT>
Bool ReflectionCast(WeakAny& rCastTarget)
{
	if (rCastTarget.IsOfType<T const*>())
	{
		rCastTarget = static_cast<PARENT const*>(
			rCastTarget.Cast<T const*>());
		return true;
	}
	else if (rCastTarget.IsOfType<T*>())
	{
		rCastTarget = static_cast<PARENT*>(
			rCastTarget.Cast<T*>());
		return true;
	}

	return false;
}

enum class SerializeError
{
	kNone,
	kUnknown,

	/**
	 * (De)serializeArray was called on a C++ object that is not an array
	 * (Reflection::TypeInfo::TryGetArray() returned nullptr).
	 */
	kObjectIsNotAnArray,

	/**
	 * (De)serializeArray was called with a DataNode value that is not an array.
	 */
	kDataNodeIsNotArray,

	/**
	 * (De)serializeArray could not size the C++ object array to match the
	 * size of the array in the DataStore.
	 */
	kFailedSizingObjectArray,

	/**
	 * To (de)serialize arrays and objects, a pointer to the destination must
	 * be acquired (through either a property or array interface). This is not always
	 * possible (the array or property is read-only, the property does not support
	 * "get ptr").
	 */
	kFailedGettingPointer,

	/**
	 * This error is signaled if a DataNode value is encountered that is of
	 * an unsupported type.
	 */
	kUnsupportedDataNodeType,

	/**
	 * If a Reflection::Property has the Reflection::Attributes::Required attributes, and a
	 * value does not exist in the DataStore for that property, this error will be signaled.
	 */
	kRequiredPropertyHasNoCorrespondingValue,

	/**
	 * If a serialization context, a TryGetValue() failed on an array or a property.
	 */
	kFailedGettingValue,

	/**
	 * If a TrySetValue() failed, either setting to an array or a property, this error
	 * will be signaled.
	 */
	kFailedSettingValue,

	/**
	 * If a Deserialize fails on an array element.
	 */
	kFailedSettingValueToArray,

	/**
	 * If a TrySetValue() fails on a table element.
	 */
	kFailedSettingValueToTable,

	/**
	 * The property currently being (de)serialized has a Reflection::Attributes::Custom(De)serializeProperty
	 * attribute specified, but the (de)serialize function failed to acquire the specified method that
	 * is supposed to be used for custom deserialization.
	 */
	kCustomSerializePropertyDelegateNotFound,

	/**
	 * The property currently being (de)serialized has a Reflection::Attributes::Custom(De)serializeProperty
	 * but invocation of the method specified for custom deserialization failed (likely due to parameter
	 * mismatch - check that your custom method fulfills the expected method signature).
	 */
	kCustomSerializePropertyDelegateFailedInvocation,

	/**
	 * The property currently being deserialized has a Reflection::Attributes::IfDeserializedSetTrue,
	 * but a property with the given identifier was not found.
	 */
	kIfDeserializedSetTruePropertyNotFound,

	/**
	 * The property currently being deserialized has a Reflection::Attributes::IfDeserializedSetTrue,
	 * but the named property had the wrong type. (It should be a Bool.)
	 */
	kIfDeserializedSetTruePropertyNotBool,

	/**
	 * The property currently being deserialized has a Reflection::Attributes::IfDeserializedSetTrue,
	 * but the named property could not be assigned to.
	 */
	kIfDeserializedSetTruePropertyNotSet,

	/**
	 * The object currently being (de)serialized has a Reflection::Attributes::PostSerialize
	 * attribute specified, but the (de)serialize function failed to acquire the specified method that
	 * is supposed to be used for custom deserialization.
	 */
	kPostSerializeDelegateNotFound,

	/**
	 * The object currently being (de)serialized has a Reflection::Attributes::PostSerialize
	 * but invocation of the method specified for custom deserialization failed (likely due to parameter
	 * mismatch - check that your custom method fulfills the expected method signature).
	 */
	kPostSerializeDelegateFailedInvocation,

	/**
	 * An object is being serialized or deserialized with a generic serialized/deserialize
	 * function, but it has no properties. This is likely an error, because a generic serialize
	 * function becomes a nop in this case.
	 */
	kGenericSerializedTypeHasNoProperties,

	/**
	 * In a deserialization context, the input DataStore contains a table key
	 * that does not correspond to any properties. This indicates a likely typo, or
	 * a deprecated property that should be removed from the configuration data.
	 */
	kDataStoreContainsUndefinedProperty,

	/**
	 * In a deserialization context, a type T could not be instantiated for
	 * a member pointer property. This can occur if the type T is not constructable
	 * (was defined with an explicit kDisableNew) or if the type is polymorphic
	 * and no PolymorphicKey attribute was defined for the type.
	 */
	kFailedInstantiatingInstanceForMemberPointer,

	/**
	 * In a serialization context, a property with attribute
	 * DoNotSerializeIfEqualToSimpleType is present, but the Value we're comparing against
	 * doesn't match of the type of the data given.
	 */
	kDoNotSerializeIfEqualToSimpleTypeTypeMismatch,

	/**
	 * In a serialization context, a property with attribute
	 * DoNotSerializeIfEqualToSimpleType is present, but the Value we're comparing against
	 * or the value given are a complex type.
	 */
	kDoNotSerializeIfEqualToSimpleTypeComplexTypeGiven,


	/**
	 * The property currently being serialized has a Reflection::Attributes::DoNotSerializeIf
	 * attribute specified, but the serialize function failed to acquire the specified method that
	 * is supposed to be used for custom checking.
	 */
	kDoNotSerializeIfPropertyDelegateNotFound,

	/**
	 * The property currently being serialized has a Reflection::Attributes::DoNotSerializeIf
	 * attribute specified, but invocation of the method specified for custom checking failed (likely due to parameter
	 * mismatch - check that your custom method fulfills the expected method signature).
	 */
	kDoNotSerializeIfPropertyDelegateFailedInvocation,

	/**
	 * In a serialization context, when writing a table we were unable to convert a key value
	 * into an HString. The most obvious cause would be an incomplete Enum definition.
	 */
	kFailedGettingTableKeyString,
};

/**
 * Structure used to track the current (de)serialize context when invoking an error handler.
 */
struct SerializeContext
{
	/**
	 * Wraps a name or index dereference.
	 */
	struct NameOrIndex
	{
		NameOrIndex()
			: m_Name()
			, m_uIndex(0u)
		{
		}

		NameOrIndex(HString name)
			: m_Name(name)
			, m_uIndex(0u)
		{
		}

		NameOrIndex(UInt16 uIndex)
			: m_Name()
			, m_uIndex(uIndex)
		{
		}

		/**
		 * @return A Seoul::String representation of this property name or array index.
		 */
		String ToString() const
		{
			return (m_Name.IsEmpty() ? String::Printf("%hu", m_uIndex) : String(m_Name));
		}

		HString m_Name;
		UInt16 m_uIndex;
	};

	virtual ~SerializeContext()
	{
	}

	virtual const DataNode& GetCurrentValue() const = 0;
	virtual void SetCurrentValue(const DataNode& value) = 0;
	virtual CheckedPtr<TypeInfo const> GetCurrentObjectTypeInfo() const = 0;
	virtual void SetCurrentObjectTypeInfo(CheckedPtr<TypeInfo const> p) = 0;
	virtual const ContentKey& GetKey() const = 0;
	virtual const DataStore& GetDataStore() const = 0;
	virtual Bool HandleError(SerializeError eError, HString additionalData = HString()) = 0;
	virtual String ScopeToString() const = 0;
	virtual String ScopeToStringAsPath() const = 0;
	virtual void Pop() = 0;
	virtual void Push(HString name) = 0;
	virtual void Push(UInt32 uIndex) = 0;
	virtual NameOrIndex Top(UInt32 uOffset = 0u) const = 0;
	virtual const WeakAny& GetUserData() const = 0;
	virtual void SetUserData(const WeakAny& userData) = 0;
}; // struct SerializeContext

/**
 * Structure used to track the current (de)serialize context when invoking an error handler.
 */
struct DefaultSerializeContext : public SerializeContext
{
	DefaultSerializeContext(
		const ContentKey& key,
		const DataStore& dataStore,
		const DataNode& value,
		const TypeInfo& objectTypeInfo,
		HString name = HString());
	DefaultSerializeContext(const DefaultSerializeContext& existingContext);
	virtual ~DefaultSerializeContext();

	virtual const DataNode& GetCurrentValue() const SEOUL_OVERRIDE
	{
		return m_CurrentValue;
	}

	virtual void SetCurrentValue(const DataNode& value) SEOUL_OVERRIDE
	{
		m_CurrentValue = value;
	}

	virtual CheckedPtr<TypeInfo const> GetCurrentObjectTypeInfo() const SEOUL_OVERRIDE
	{
		return m_pCurrentObjectTypeInfo;
	}

	virtual void SetCurrentObjectTypeInfo(CheckedPtr<TypeInfo const> p) SEOUL_OVERRIDE
	{
		m_pCurrentObjectTypeInfo = p;
	}

	virtual const ContentKey& GetKey() const SEOUL_OVERRIDE
	{
		return m_Key;
	}

	virtual const DataStore& GetDataStore() const SEOUL_OVERRIDE
	{
		return m_DataStore;
	}

	// Called by the serializer or deserializer when an error occurs - if
	// this method returns true, the serializer will continue (if possible),
	// otherwise, it will immediately return false and fail the (de)serialization.
	virtual Bool HandleError(SerializeError eError, HString additionalData = HString()) SEOUL_OVERRIDE;
	virtual String ScopeToString() const SEOUL_OVERRIDE;

	virtual String ScopeToStringAsPath() const SEOUL_OVERRIDE;

	ContentKey m_Key;
	const DataStore& m_DataStore;
	DataNode m_CurrentValue;
	CheckedPtr<TypeInfo const> m_pCurrentObjectTypeInfo;
	WeakAny m_UserData;
	UInt32 m_uFlags;

	/**
	 * Pop the top entry off the scope array, or a nop
	 * if the array is empty.
	 */
	virtual void Pop() SEOUL_OVERRIDE
	{
		if (m_uScope > 0u)
		{
			--m_uScope;
		}
	}

	/**
	 * Insert a new entry on the scope array, or a nop if
	 * the array is full.
	 */
	virtual void Push(HString name) SEOUL_OVERRIDE
	{
		if (m_uScope < m_aScope.GetSize())
		{
			m_aScope[m_uScope++] = NameOrIndex(name);
		}
	}

	/**
	 * Insert a new entry on the scope array, or a nop if
	 * the array is full.
	 */
	virtual void Push(UInt32 uIndex) SEOUL_OVERRIDE
	{
		if (m_uScope < m_aScope.GetSize())
		{
			m_aScope[m_uScope++] = NameOrIndex((UInt16)uIndex);
		}
	}

	/**
	 * @return The top name or index on the scope stack, or the the empty
	 * entry if the stack is empty.
	 */
	virtual NameOrIndex Top(UInt32 uOffset = 0u) const SEOUL_OVERRIDE
	{
		if (m_uScope > uOffset)
		{
			return m_aScope[m_uScope - 1u - uOffset];
		}

		return NameOrIndex();
	}

	/** User data is stored in the context for client use, not accessed by the Reflection system. */
	virtual const WeakAny& GetUserData() const SEOUL_OVERRIDE
	{
		return m_UserData;
	}
	virtual void SetUserData(const WeakAny& userData) SEOUL_OVERRIDE
	{
		m_UserData = userData;
	}

protected:
	static const UInt32 kCapacity = 32u;
	FixedArray<NameOrIndex, kCapacity> m_aScope;
	UInt32 m_uScope;
}; // struct SerializeContext

/**
 * Common variation of SerializeContext, suppresses the
 * kGenericSerializedTypeHasNoProperties error.
 *
 * The kGenericSerializedTypeHasNoProperties occurs if a deserialize
 * path attempts to treat an object as "generic" and deserialize it by walking its
 * property table and deserializing each property. If this path is reached but
 * an object has no properties throughout its entire object hierarchy, this, by default,
 * is viewed as an error, because it is likely a template definition was not picked
 * up and a special handler was not called when it should have been.
 *
 * However, there are cases where this behavior is expected, for example, when the
 * deserializer is specifically being used to handle instantiation of a polymorphic type.
 * This class can be used to suppress the error in those cases.
 */
struct SuppressTypeHasNoPropertiesSerializeContext SEOUL_SEALED : public SerializeContext
{
	SuppressTypeHasNoPropertiesSerializeContext(SerializeContext& existingContext)
		: SerializeContext(existingContext)
		, m_SavedContext(existingContext)
	{
	}

	virtual Bool HandleError(
		SerializeError eError,
		HString additionalData = HString()) SEOUL_OVERRIDE
	{
		if (SerializeError::kGenericSerializedTypeHasNoProperties != eError)
		{
			return m_SavedContext.HandleError(eError, additionalData);
		}
		else
		{
			return true;
		}
	}

	virtual const DataNode& GetCurrentValue() const SEOUL_OVERRIDE
	{
		return m_SavedContext.GetCurrentValue();
	}

	virtual void SetCurrentValue(const DataNode& value) SEOUL_OVERRIDE
	{
		return m_SavedContext.SetCurrentValue(value);
	}

	virtual CheckedPtr<TypeInfo const> GetCurrentObjectTypeInfo() const SEOUL_OVERRIDE
	{
		return m_SavedContext.GetCurrentObjectTypeInfo();
	}

	virtual void SetCurrentObjectTypeInfo(CheckedPtr<TypeInfo const> p) SEOUL_OVERRIDE
	{
		m_SavedContext.SetCurrentObjectTypeInfo(p);
	}

	virtual const ContentKey& GetKey() const SEOUL_OVERRIDE
	{
		return m_SavedContext.GetKey();
	}

	virtual const DataStore& GetDataStore() const SEOUL_OVERRIDE
	{
		return m_SavedContext.GetDataStore();
	}

	virtual String ScopeToString() const SEOUL_OVERRIDE
	{
		return m_SavedContext.ScopeToString();
	}

	virtual String ScopeToStringAsPath() const SEOUL_OVERRIDE
	{
		return m_SavedContext.ScopeToStringAsPath();
	}

	virtual void Pop() SEOUL_OVERRIDE
	{
		m_SavedContext.Pop();
	}

	virtual void Push(HString name) SEOUL_OVERRIDE
	{
		m_SavedContext.Push(name);
	}

	virtual void Push(UInt32 uIndex) SEOUL_OVERRIDE
	{
		m_SavedContext.Push(uIndex);
	}

	virtual NameOrIndex Top(UInt32 uOffset = 0u) const SEOUL_OVERRIDE
	{
		return m_SavedContext.Top(uOffset);
	}

	virtual const WeakAny& GetUserData() const SEOUL_OVERRIDE
	{
		return m_SavedContext.GetUserData();
	}

	virtual void SetUserData(const WeakAny& userData) SEOUL_OVERRIDE
	{
		m_SavedContext.SetUserData(userData);
	}

	SerializeContext& m_SavedContext;
}; // struct SuppressTypeHasNoPropertiesSerializeContext

/**
 * Subclass of SerializeContext that is identical to the behavior of
 * SerializeContext, except properties are assumed to not be required by default.
 */
class DefaultNotRequiredSerializeContext SEOUL_SEALED : public DefaultSerializeContext
{
public:
	DefaultNotRequiredSerializeContext(
		const ContentKey& contentKey,
		const DataStore& dataStore,
		const DataNode& table,
		const TypeInfo& typeInfo)
		: DefaultSerializeContext(contentKey, dataStore, table, typeInfo)
	{
	}

	virtual Bool HandleError(SerializeError eError, HString additionalData = HString()) SEOUL_OVERRIDE
	{
		using namespace Reflection;

		if (SerializeError::kRequiredPropertyHasNoCorrespondingValue != eError)
		{
			return DefaultSerializeContext::HandleError(eError, additionalData);
		}
		else
		{
			return true;
		}
	}
}; // class DefaultNotRequiredSerializeContext

/**
 * Utility class for keeping SerializeContext in sync with the actual
 * function context.
 */
class SerializeContextScope
{
public:
	SerializeContextScope(
		SerializeContext& rContext,
		const DataNode& newValue,
		const Reflection::TypeInfo& newObjectTypeInfo,
		Int iNewArrayIndex)
		: m_rContext(rContext)
		, m_OriginalValue(m_rContext.GetCurrentValue())
		, m_pOriginalObjectTypeInfo(m_rContext.GetCurrentObjectTypeInfo())
	{
		m_rContext.SetCurrentValue(newValue);
		m_rContext.SetCurrentObjectTypeInfo(&newObjectTypeInfo);
		m_rContext.Push(iNewArrayIndex);
	}

	SerializeContextScope(
		SerializeContext& rContext,
		const DataNode& newValue,
		const Reflection::TypeInfo& newObjectTypeInfo,
		HString newName)
		: m_rContext(rContext)
		, m_OriginalValue(m_rContext.GetCurrentValue())
		, m_pOriginalObjectTypeInfo(m_rContext.GetCurrentObjectTypeInfo())
	{
		m_rContext.SetCurrentValue(newValue);
		m_rContext.SetCurrentObjectTypeInfo(&newObjectTypeInfo);
		m_rContext.Push(newName);
	}

	~SerializeContextScope()
	{
		m_rContext.Pop();
		m_rContext.SetCurrentObjectTypeInfo(m_pOriginalObjectTypeInfo);
		m_rContext.SetCurrentValue(m_OriginalValue);
	}

private:
	SerializeContext& m_rContext;
	DataNode m_OriginalValue;
	CheckedPtr<Reflection::TypeInfo const> m_pOriginalObjectTypeInfo;
}; // struct SerializeContextScope

// Can be used to generate the stock error message for a given reflection deserialization error.
void DefaultSerializeErrorMessaging(const SerializeContext& context, SerializeError eError, HString additionalData, String& rsMessage);

} // namespace Seoul::Reflection

#endif // include guard
