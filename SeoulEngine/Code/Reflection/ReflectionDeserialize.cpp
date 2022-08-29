/**
 * \file ReflectionDeserialize.cpp
 * \brief Functions for deserializing data from a generic DataStore into
 * a concrete C++ object using Reflection.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "DataStoreParser.h"
#include "Lexer.h"
#include "ReflectionArray.h"
#include "ReflectionAttributes.h"
#include "ReflectionDeserialize.h"
#include "ReflectionType.h"
#include "DataStoreParser.h"
#include "Logger.h"

namespace Seoul::Reflection
{

void DefaultSerializeErrorMessaging(const SerializeContext& context, SerializeError eError, HString additionalData, String& rsMessage)
{
	switch (eError)
	{
	case SerializeError::kNone:
		break;
	case SerializeError::kFailedGettingValue:
		rsMessage.Assign("Programmer error: Could not get value for serialization. Check for type incompatible with serialization (e.g. a raw pointer).");
		break;

	case SerializeError::kFailedGettingTableKeyString:
		rsMessage.Assign("Programmer error: Could not convert table key to string for serialization. Check for missing reflection definitions for new enum values.");
		break;

	case SerializeError::kObjectIsNotAnArray: // fall-through
	case SerializeError::kDataNodeIsNotArray: // fall-through
	case SerializeError::kFailedSettingValue: // fall-through
	case SerializeError::kFailedSettingValueToArray:
	case SerializeError::kFailedSettingValueToTable:
		{
			Reflection::Enum const* pEnum = context.GetCurrentObjectTypeInfo()->GetType().TryGetEnum();
			if (nullptr != pEnum && context.GetCurrentValue().IsString())
			{
				String sName;
				SEOUL_VERIFY(context.GetDataStore().AsString(context.GetCurrentValue(), sName));

				rsMessage.Assign(String::Printf(
					"Invalid enum value '%s', valid values:",
					sName.CStr()));

				UInt32 const nEnumValues = pEnum->GetNames().GetSize();
				for (UInt32 i = 0u; i < nEnumValues; ++i)
				{
					rsMessage.Append(String::Printf("\n    - %s", pEnum->GetNames()[i].CStr()));
				}
			}
			else
			{
				// Handle "array" and "table" types generally, since users getting
				// this message won't care if the type is a Vector<UInt32, 10> or
				// a Vector<Int32, 11>.
				const Type& type = context.GetCurrentObjectTypeInfo()->GetType();
				Byte const* sTypeName = type.GetName().CStr();
				if (type.TryGetArray())
				{
					sTypeName = "Array";
				}
				else if (type.TryGetTable())
				{
					sTypeName = "Table";
				}

				if (SerializeError::kFailedSettingValueToTable == eError)
				{
					rsMessage.Assign(String::Printf(
						"Type mismatch, expected '%s', got '%s'. Also verify that the key '%s' is a valid table key.",
						sTypeName,
						EnumToString<DataNode::Type>(context.GetCurrentValue().GetType()),
						context.Top().m_Name.CStr()));
				}
				else
				{
					rsMessage.Assign(String::Printf(
						"Type mismatch, expected '%s', got '%s'.",
						sTypeName,
						EnumToString<DataNode::Type>(context.GetCurrentValue().GetType())));
				}
			}
		}
		break;

	case SerializeError::kFailedSizingObjectArray:
		rsMessage.Assign("Programmer error: Array size cannot be set.");
		break;

	case SerializeError::kFailedGettingPointer:
		rsMessage.Assign("Programmer error: Target must be accessible as a pointer to be deserialized.");
		break;

	case SerializeError::kUnsupportedDataNodeType:
		rsMessage.Assign(String::Printf(
			"Programmer error: Input value is of unknown type '%s'.",
			EnumToString<DataNode::Type>(context.GetCurrentValue().GetType())));
		break;

	case SerializeError::kRequiredPropertyHasNoCorrespondingValue:
		// If the additional value is not empty, include the similar match that
		// may have been intended as the required property.
		if (!additionalData.IsEmpty())
		{
			rsMessage.Assign(String::Printf(
				"Missing required value. Similar, unexpected value '%s' is defined.\n\nIs this a typo or a capitalization mismatch (properties are case-sensitive)?",
				additionalData.CStr()));
		}
		// Otherwise, just report that the property was not defined.
		else
		{
			rsMessage.Assign("Missing required value.");
		}
		break;

	case SerializeError::kCustomSerializePropertyDelegateNotFound:
		rsMessage.Assign("Programmer error: Property is defined with a custom deserializer, but deserializer method was not found, check reflection definition.");
		break;

	case SerializeError::kCustomSerializePropertyDelegateFailedInvocation:
		rsMessage.Assign("Programmer error: Property is defined with a custom deserializer, but method invocation failed, check method signature.");
		break;

	case SerializeError::kIfDeserializedSetTruePropertyNotFound:
		rsMessage.Assign("Programmer error: Property is defined with another property to set on deserialize, but it was not found. Make sure it's declared for reflection.");
		break;

	case SerializeError::kIfDeserializedSetTruePropertyNotBool:
		rsMessage.Assign("Programmer error: Property is defined with another property to set on deserialize, but it was not a Bool.");
		break;

	case SerializeError::kIfDeserializedSetTruePropertyNotSet:
		rsMessage.Assign("Programmer error: Property is defined with another property to set on deserialize, but it could not be assigned.");
		break;

	case SerializeError::kPostSerializeDelegateNotFound:
		rsMessage.Assign("Programmer error: Type is defined with a custom post deserializer step, but deserializer method was not found, check reflection definition.");
		break;
	case SerializeError::kPostSerializeDelegateFailedInvocation:
		rsMessage.Assign("Programmer error: Type is defined with a custom post deserializer step, but method invocation failed, check method signature.");
		break;

	case SerializeError::kGenericSerializedTypeHasNoProperties:
		rsMessage.Assign(String::Printf("Programmer error: Type '%s' is being (de)serialized with a generic (de)serializer, but has no properties.",
			context.GetCurrentObjectTypeInfo()->GetType().GetName().CStr()));
		break;

	case SerializeError::kDataStoreContainsUndefinedProperty:
		{
			rsMessage.Assign(String::Printf(
				"Unexpected value '%s' is defined.\n\nIs this a typo, capitalization mismatch (properties are case-sensitive), or deprecated field?",
				additionalData.CStr()));
		}
		break;

	case SerializeError::kFailedInstantiatingInstanceForMemberPointer:
		{
			Attributes::PolymorphicKey const* pKey = context.GetCurrentObjectTypeInfo()->GetType().GetAttributes().GetAttribute<Attributes::PolymorphicKey>();
			if (nullptr != pKey)
			{
				rsMessage.Assign(String::Printf(
					"Missing required value. The type of this property must be defined with key '%s'.",
					pKey->m_Key.CStr()));
			}
			else if (context.GetCurrentObjectTypeInfo()->GetType().GetTypeInfo().IsReflectionPolymorphic())
			{
				rsMessage.Assign("Programmer error: A pointer is being (de)serialized for a polymorphic type that does not have the "
					"PolymorphicKey attribute. Add the PolymorphicKey attribute to the reflection definition of this type.");
			}
			else
			{
				rsMessage.Assign("Programmer error: A pointer is being (de)serialized for a type for which new has been disabled (kDisableNew).");
			}
		}
		break;
	case SerializeError::kDoNotSerializeIfEqualToSimpleTypeTypeMismatch:
		rsMessage.Assign("Programmer error : Property is defined with attribute DoNotSerializeIfEqualToSimpleType, but types being compared are mismatched.");
		break;
	case SerializeError::kDoNotSerializeIfEqualToSimpleTypeComplexTypeGiven:
		rsMessage.Assign("Programmer error : Property is defined with attribute DoNotSerializeIfEqualToSimpleType, but a complex type was given.");
		break;
	case SerializeError::kDoNotSerializeIfPropertyDelegateNotFound:
		rsMessage.Assign("Programmer error: Property is defined with a custom DoNotSerializeIf method, but the method was not found, check reflection definition.");
		break;
	case SerializeError::kDoNotSerializeIfPropertyDelegateFailedInvocation:
		rsMessage.Assign("Programmer error: Property is defined with a custom DoNotSerializeIf method, but method invocation failed, check method signature.");
		break;
	case SerializeError::kUnknown: // fall-through
	default:
		rsMessage.Assign("Programmer error: Unknown error.");
		break;
	};
}

DefaultSerializeContext::DefaultSerializeContext(
	const ContentKey& key,
	const DataStore& dataStore,
	const DataNode& value,
	const TypeInfo& objectTypeInfo,
	HString name /*= HString()*/)
	: m_Key(key)
	, m_DataStore(dataStore)
	, m_CurrentValue(value)
	, m_pCurrentObjectTypeInfo(&objectTypeInfo)
	, m_UserData()
	, m_uFlags(DataStoreParserFlags::kLogParseErrors)
	, m_aScope()
	, m_uScope(0)
{
	Push(objectTypeInfo.GetType().GetName());
	if (!name.IsEmpty())
	{
		Push(name);
	}
}

DefaultSerializeContext::DefaultSerializeContext(const DefaultSerializeContext& existingContext)
	: m_Key(existingContext.m_Key)
	, m_DataStore(existingContext.m_DataStore)
	, m_CurrentValue(existingContext.m_CurrentValue)
	, m_pCurrentObjectTypeInfo(existingContext.m_pCurrentObjectTypeInfo)
	, m_UserData(existingContext.m_UserData)
	, m_uFlags(existingContext.m_uFlags)
{
	m_aScope = existingContext.m_aScope;
	m_uScope = existingContext.m_uScope;
}

DefaultSerializeContext::~DefaultSerializeContext()
{
}

Bool DefaultSerializeContext::HandleError(
	SerializeError eError,
	HString additionalData /* = HString() */)
{
#if SEOUL_LOGGING_ENABLED
	if (DataStoreParserFlags::kLogParseErrors == (DataStoreParserFlags::kLogParseErrors & m_uFlags))
	{
		String sMessage;
		DefaultSerializeErrorMessaging(*this, eError, additionalData, sMessage);
		SEOUL_WARN("%s\n%s\n%s", m_Key.GetFilePath().GetRelativeFilenameInSource().CStr(), ScopeToString().CStr(), sMessage.CStr());
	}
#endif

	// By default, only the "missing property" error returns true, since we consider a warning
	// sufficient in this case.
	return (SerializeError::kDataStoreContainsUndefinedProperty == eError);
}

String DefaultSerializeContext::ScopeToString() const
{
	String sReturn;

	UInt32 i = 0u;
	if (m_uScope > i)
	{
		sReturn.Append('[');
		sReturn.Append(m_aScope[0].ToString());
		sReturn.Append(']');
		sReturn.Append('\n');
	}
	++i;

	if (m_uScope > i)
	{
		sReturn.Append(m_aScope[1].ToString());
	}
	++i;

	for (; i + 1u < m_uScope; ++i)
	{
		sReturn.Append('[');
		sReturn.Append(m_aScope[i].ToString());
		sReturn.Append(']');
	}

	if (m_uScope > i)
	{
		sReturn.Append(':');
		sReturn.Append(' ');

		if (m_aScope[i].m_Name.IsEmpty())
		{
			sReturn.Append(String::Printf("Array Index '%hu'", m_aScope[i].m_uIndex));
		}
		else
		{
			sReturn.Append(String::Printf("Property '%s'", m_aScope[i].m_Name.CStr()));
		}
	}

	return sReturn;
}

String DefaultSerializeContext::ScopeToStringAsPath() const
{
	String sReturn;

	UInt32 i = 0u;
	if (m_uScope > i)
	{
		sReturn.Append(m_aScope[0].ToString());
	}
	++i;

	for (; i < m_uScope; ++i)
	{
		sReturn.Append('.');
		sReturn.Append(m_aScope[i].ToString());
	}

	return sReturn;
}

#if !SEOUL_SHIP
namespace
{

typedef HashSet<HString, MemoryBudgets::Reflection> ExclusionSet;
static ExclusionSet s_ExclusionSet;
static Mutex s_ExclusionSetMutex;
static Atomic32Value<Bool> s_bExclusionSetLoaded;

static const ExclusionSet& GetExclusionSet()
{
	// Need to loack to check for loaded - otherwise,
	// once we've loaded once we never change the exclusion
	// set (it cannot be hot loaded), so we can release
	// the lock and return a read-only reference to the
	// set.
	{
		Lock lock(s_ExclusionSetMutex);
		if (!s_bExclusionSetLoaded)
		{
			// Always loaded now, even if this fails. We must never
			// mutate the exclusion set after the first attempt to
			// retrieve/load it, or we might be mutation a structure
			// while it is in use.
			s_bExclusionSetLoaded = true;

			DataStore ds;
			(void)DataStoreParser::FromFile(FilePath::CreateConfigFilePath("ReflectionCheckExclusions.json"), ds);

			// Manually "deserialize" - who excludes the exclusion set?
			ExclusionSet set;

			auto const root = ds.GetRootNode();
			UInt32 uCount = 0u;
			(void)ds.GetArrayCount(root, uCount);
			for (UInt32 i = 0u; i < uCount; ++i)
			{
				DataNode value;
				HString exclusion;
				if (ds.GetValueFromArray(root, i, value) &&
					ds.AsString(value, exclusion))
				{
					(void)set.Insert(exclusion);
				}
			}

			s_ExclusionSet.Swap(set);
		}
	}

	return s_ExclusionSet;
}

} // namespace anonymous
#endif

/**
 * Utility function, deserialize the contents of a DataStore table table into
 * the object objectThis.
 */
Bool Type::DoGenericDeserialize(
	UInt32& ruProperties,
	SerializeContext& rContext,
	const DataStore& dataStore,
	const DataNode& table,
	const WeakAny& objectThis,
	const Type& mostDerivedType,
	Bool bSkipPostSerialize /* = false */,
	Bool bInParent /*= false */)
{
	const Type& type = objectThis.GetType();

	// First, Deserialize parents.
	UInt32 const nParents = type.GetParentCount();
	for (UInt32 i = 0u; i < nParents; ++i)
	{
		const TypePair& pair = type.GetParentPair(i);

		WeakAny parent(objectThis);
		SEOUL_VERIFY(pair.Second(parent));

		// Deserialize the parent.
		if (!DoGenericDeserialize(ruProperties, rContext, dataStore, table, parent, mostDerivedType, bSkipPostSerialize, true))
		{
			return false;
		}
	}

	// Now deserialize the current *this* object.
	UInt32 const nProperties = type.GetPropertyCount();

	// If type is being processed with the generic deserialize but has no properties,
	// flag this as an error - it is likely that a specialized deserialize was
	// not properly invoked
	if (!bInParent && (0u == (nProperties + ruProperties)))
	{
		const Attributes::AllowNoProperties* pAllowNoProperties = type.GetAttribute<Attributes::AllowNoProperties>(true);
		if (!pAllowNoProperties && !rContext.HandleError(SerializeError::kGenericSerializedTypeHasNoProperties))
		{
			return false;
		}
	}

	// Add our local property count to the running total of the object graph.
	ruProperties += nProperties;

#if !SEOUL_SHIP
	// Look at the current table for properties that are not in reflection.
	//
	// Only run this if the current context has a FilePath (we're deserializing
	// data on disk), since this check is capable of false positives and
	// we want to reduce the risk of confusion/noise (when e.g. deserializing
	// server data or various in-memory utility JSON data).
	if (nProperties > 0 &&
		!bInParent &&
		rContext.GetKey().GetFilePath().IsValid() &&
		!type.HasAttribute<Attributes::DisableReflectionCheck>())
	{
		// Cache the exclusion set.
		auto const& set = GetExclusionSet();

		// Grab the PolymorphicKey if one exists
		Attributes::PolymorphicKey const* pKey = type.GetAttribute<Attributes::PolymorphicKey>(true);

		auto const iBegin = dataStore.TableBegin(table);
		auto const iEnd = dataStore.TableEnd(table);
		for (auto i = iBegin; iEnd != i; ++i)
		{
			// Skip if in key exclusion set
			if (set.HasKey(i->First))
			{
				continue;
			}

			// This might be a polymorphic type, so check the poly key first
			// and move along if that's it
			if (pKey && pKey->m_Key == i->First)
			{
				continue;
			}

			const Property *pProp = type.GetProperty(i->First);
			if (!pProp)
			{
				(void)rContext.HandleError(SerializeError::kDataStoreContainsUndefinedProperty, i->First);
			}
		}
	}
#endif

	for (UInt32 i = 0u; i < nProperties; ++i)
	{
		const Property& property = *type.GetProperty(i);

		// If the property is marked as DoNotSerialize, skip it
		if (property.GetAttributes().HasAttribute<Attributes::DoNotSerialize>())
		{
			continue;
		}

		// If we fail to get a value for the property from the table, nothing
		// to deserialize - skip it.
		DataNode value;
		if (!dataStore.GetValueFromTable(table, property.GetName(), value))
		{
			SerializeContextScope scope(rContext, DataNode(), property.GetMemberTypeInfo(), property.GetName());

			// If the property does not have the "NotRequired" attribute, fail the deserialization.
			if (!type.HasAttribute<Attributes::NotRequired>() &&
				!property.GetAttributes().HasAttribute<Attributes::NotRequired>() &&
				!rContext.HandleError(SerializeError::kRequiredPropertyHasNoCorrespondingValue, property.GetName()))
			{
				return false;
			}

			// Otherwise, continue to the next property.
			continue;
		}

		// If the property has the "CustomSerialize" attribute, invoke the associated method to serialize this property.
		Attributes::CustomSerializeProperty const* pCustomSerialize = property.GetAttributes().GetAttribute<Attributes::CustomSerializeProperty>();
		if (nullptr != pCustomSerialize && !pCustomSerialize->m_DeserializeMethodName.IsEmpty())
		{
			// Now attempt to assign the value to the property.
			SerializeContextScope scope(rContext, value, property.GetMemberTypeInfo(), property.GetName());

			CheckedPtr<Method const> pMethod = type.GetMethod(pCustomSerialize->m_DeserializeMethodName);
			if (!pMethod.IsValid())
			{
				if (!rContext.HandleError(SerializeError::kCustomSerializePropertyDelegateNotFound))
				{
					return false;
				}
				// Otherwise, continue to the next property.
				else
				{
					continue;
				}
			}

			Any returnValue;
			MethodArguments aArguments;
			aArguments[0] = &rContext;
			aArguments[1] = &dataStore;
			aArguments[2] = value;
			if (4 == pMethod->GetTypeInfo().m_uArgumentCount)
			{
				aArguments[3] = table;
			}
			Bool bReturnValue = false;
			if (!pMethod->TryInvoke(returnValue, objectThis, aArguments) ||
				!TypeConstruct(returnValue, bReturnValue))
			{
				if (!rContext.HandleError(SerializeError::kCustomSerializePropertyDelegateFailedInvocation))
				{
					return false;
				}
				// Otherwise, continue to the next property.
				else
				{
					continue;
				}
			}

			// If the custom serializer returned false, fail.
			if (!bReturnValue)
			{
				return false;
			}
		}
		else
		{
			// Get a pointer to the property.
			WeakAny valuePointer;
			if (!property.TryGetPtr(objectThis, valuePointer))
			{
				SerializeContextScope scope(rContext, value, property.GetMemberTypeInfo(), property.GetName());

				// Track if any of the complex forms of deserialization succeed.
				Bool bSuccess = false;

				// Try direct set deserialization.
				Any anyValue;

				// If we can set the value directly to an Any, process
				// it that way (this is the cheapest of the complex options.
				if (ToAny(dataStore, value, anyValue) &&
					property.TrySet(objectThis, anyValue))
				{
					// Done.
					bSuccess = true;
				}
				// Otherwise, try to get the existing value, deserialize
				// onto it, and then set it back.
				else if (
					property.TryGet(objectThis, anyValue) &&
					TryDeserialize(rContext, dataStore, value, anyValue.GetWeakAnyPointerToValue(), bSkipPostSerialize) &&
					property.TrySet(objectThis, anyValue))
				{
					// Done.
					bSuccess = true;
				}

				// If all complex deserialization actions failed, then
				// we need to get this value by pointer.
				if (!bSuccess)
				{
					if (!rContext.HandleError(SerializeError::kFailedGettingPointer))
					{
						return false;
					}
					// If this error was suppressed, continue onto the next property.
					else
					{
						continue;
					}
				}
			}
			else
			{
				SerializeContextScope scope(rContext, value, valuePointer.GetTypeInfo(), property.GetName());
				if (!TryDeserialize(rContext, dataStore, value, valuePointer, bSkipPostSerialize))
				{
					return false;
				}
			}
		}

		// If there's a IfDeserializedSetTrue field, set the property it points at to true (because we just deserialized)
		Attributes::IfDeserializedSetTrue const* pIfDeserializedSetTrue = property.GetAttributes().GetAttribute<Attributes::IfDeserializedSetTrue>();
		if (nullptr != pIfDeserializedSetTrue && !pIfDeserializedSetTrue->m_FieldToSetName.IsEmpty())
		{
			// Now attempt to assign the value to the property.
			SerializeContextScope scope(rContext, value, property.GetMemberTypeInfo(), property.GetName());

			CheckedPtr<Property const> pProperty = type.GetProperty(pIfDeserializedSetTrue->m_FieldToSetName);
			if (!pProperty.IsValid())
			{
				if (!rContext.HandleError(SerializeError::kIfDeserializedSetTruePropertyNotFound))
				{
					return false;
				}
				// Otherwise, continue to the next property.
				else
				{
					continue;
				}
			}

			if (pProperty->GetMemberTypeInfo().GetSimpleTypeInfo() != SimpleTypeInfo::kBoolean)
			{
				if (!rContext.HandleError(SerializeError::kIfDeserializedSetTruePropertyNotBool))
				{
					return false;
				}
				// Otherwise, continue to the next property.
				else
				{
					continue;
				}
			}

			if (!pProperty->TrySet(objectThis, true))
			{
				if (!rContext.HandleError(SerializeError::kIfDeserializedSetTruePropertyNotSet))
				{
					return false;
				}
				// Otherwise, continue to the next property.
				else
				{
					continue;
				}
			}
		}
	}

	// If we're performing a post serialize step, find the method and invoke it.
	if (!bSkipPostSerialize && !bInParent)
	{
		Attributes::PostSerializeType const* pAttribute = type.GetAttribute<Attributes::PostSerializeType>(true);

		// If no post serialize attribute, we're done - return true.
		if (nullptr == pAttribute || pAttribute->m_DeserializeMethodName.IsEmpty())
		{
			return true;
		}

		// The rest of this block is post serialize processing.

		Method const* pMethod = type.GetMethod(pAttribute->m_DeserializeMethodName);
		if (nullptr == pMethod)
		{
			if (!rContext.HandleError(SerializeError::kPostSerializeDelegateNotFound))
			{
				return false;
			}
			// Otherwise, if the error was suppressed, we're done, return true.
			else
			{
				return true;
			}
		}

		Any returnValue;
		Bool bReturnValue = false;
		// TODO: make this argument dependent on attribute parameters
		MethodArguments aArguments;
		aArguments[0] = &rContext;
		if ((!pMethod->TryInvoke(returnValue, objectThis)
				&& !pMethod->TryInvoke(returnValue, objectThis, aArguments))
			|| !TypeConstruct(returnValue, bReturnValue))
		{
			if (!rContext.HandleError(SerializeError::kPostSerializeDelegateFailedInvocation))
			{
				return false;
			}
			// Otherwise, if the error was suppressed, we're done, return true.
			else
			{
				return true;
			}
		}

		return bReturnValue;
	}

	return true;
}

/**
 * Deserializes a string into an arbitary object
 *
 * @param[in] s string to deserialize
 * @param[in] zSizeInBytes Length of the given string, in bytes
 * @param[out] outObject Receives the deserialized object
 * @param[in] uFlags DataStoreParser flags to control parsing.
 * @param[in] filePath Optional file identity, used in error reporting only.
 *
 * @return True if the deserialization succeeded, or false if an error occurred
 */
Bool DeserializeFromString(
	Byte const* s,
	UInt32 zSizeInBytes,
	const WeakAny& outObject,
	UInt32 uFlags /*= 0u*/,
	FilePath filePath /*= FilePath()*/)
{
	// Parse the string into a DataStore.
	DataStore dataStore;
	if (!DataStoreParser::FromString(s, zSizeInBytes, dataStore, uFlags, filePath))
	{
		return false;
	}
	// TODO: Shouldn't always assume we want the
	// not-required context.

	// Deserialize and return results.
	DefaultNotRequiredSerializeContext context(ContentKey(), dataStore, dataStore.GetRootNode(), outObject.GetTypeInfo());
	context.m_uFlags = uFlags;
	return DeserializeObject(context, dataStore, dataStore.GetRootNode(), outObject);
}

} // namespace Seoul::Reflection
