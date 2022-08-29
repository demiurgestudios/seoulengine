/**
 * \file ReflectionSerialize.cpp
 * \brief Functions for serializing data from a concrete C++ object
 * into a generic DataStore using Reflection.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ReflectionArray.h"
#include "ReflectionAttributes.h"
#include "ReflectionSerialize.h"
#include "ReflectionType.h"

namespace Seoul::Reflection
{


/**
 * Utility function, serialize the contents of an object into a DataStore table.
 */
Bool Type::DoGenericSerialize(
	UInt32& ruProperties,
	SerializeContext& rContext,
	DataStore& rDataStore,
	const DataNode& table,
	const WeakAny& objectThis,
	Bool bSkipPostSerialize /* = false */,
	Bool bInParent /* = false */)
{
	const Type& type = objectThis.GetType();

	// First, serialize parents.
	UInt32 const nParents = type.GetParentCount();
	for (UInt32 i = 0u; i < nParents; ++i)
	{
		const TypePair& pair = type.GetParentPair(i);

		WeakAny parent(objectThis);
		SEOUL_VERIFY(pair.Second(parent));

		// Serialize the parent - skip the post serialize step, since that will
		// be invoked (if specified) at the current level.
		if (!DoGenericSerialize(ruProperties, rContext, rDataStore, table, parent, bSkipPostSerialize, true))
		{
			return false;
		}
	}

	// Now serialize the current *this* object.
	UInt32 const nProperties = type.GetPropertyCount();

	// If type is being processed with the generic deserialize but has no properties,
	// flag this as an error - it is likely that a specialized deserialize was
	// not properly invoked
	if (!bInParent && 0u == (nProperties + ruProperties))
	{
		if (!rContext.HandleError(SerializeError::kGenericSerializedTypeHasNoProperties))
		{
			return false;
		}
	}

	// Add our local property count to the running total of the object graph.
	ruProperties += nProperties;

	for (UInt32 i = 0u; i < nProperties; ++i)
	{
		SEOUL_ASSERT(nullptr != type.GetProperty(i));
		const Property& property = *type.GetProperty(i);

		// If the property is marked as DoNotSerialize or Deprecated skip it
		if (property.GetAttributes().HasAttribute<Attributes::DoNotSerialize>()
			 || property.GetAttributes().HasAttribute<Attributes::Deprecated>())
		{
			continue;
		}

		// Check for skipping serialization via a simple type value check
		auto const* pDoNotSerializeIfEqualToSimpleType = property.GetAttributes().GetAttribute<Attributes::DoNotSerializeIfEqualToSimpleType>();
		if (nullptr != pDoNotSerializeIfEqualToSimpleType)
		{
			SerializeError error = SerializeError::kNone;
			auto bSkip = pDoNotSerializeIfEqualToSimpleType->Equals(property, objectThis, error);
			if (error != SerializeError::kNone && !rContext.HandleError(error))
			{
				return false;
			}
			if (bSkip)
			{
				continue;
			}
		}

		// Check for skipping serialization via a custom method
		auto const* pDoNotSerializeIf = property.GetAttributes().GetAttribute<Attributes::DoNotSerializeIf>();
		if (nullptr != pDoNotSerializeIf && !pDoNotSerializeIf->m_DoNotSerializeIfMethodName.IsEmpty())
		{
			SerializeContextScope scope(rContext, DataNode(), property.GetMemberTypeInfo(), property.GetName());
			CheckedPtr<Method const> pMethod = type.GetMethod(pDoNotSerializeIf->m_DoNotSerializeIfMethodName);
			if (!pMethod.IsValid())
			{
				if (!rContext.HandleError(SerializeError::kDoNotSerializeIfPropertyDelegateNotFound))
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
			Bool bReturnValue = false;
			if (!pMethod->TryInvoke(returnValue, objectThis) ||
				!TypeConstruct(returnValue, bReturnValue))
			{
				if (!rContext.HandleError(SerializeError::kDoNotSerializeIfPropertyDelegateFailedInvocation))
				{
					return false;
				}
				// Otherwise, continue to the next property.
				else
				{
					continue;
				}
			}

			// If the method reported to not serialize this property, continue.
			if (bReturnValue)
			{
				continue;
			}
		}

		// If the property has the "CustomSerialize" attribute, invoke the associated method to serialize this property.
		Attributes::CustomSerializeProperty const* pCustomSerialize = property.GetAttributes().GetAttribute<Attributes::CustomSerializeProperty>();
		if (nullptr != pCustomSerialize && !pCustomSerialize->m_SerializeMethodName.IsEmpty())
		{
			// Now attempt to assign the value to the property.
			SerializeContextScope scope(rContext, DataNode(), property.GetMemberTypeInfo(), property.GetName());

			CheckedPtr<Method const> pMethod = type.GetMethod(pCustomSerialize->m_SerializeMethodName);
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
			aArguments[1] = property.GetName();
			aArguments[2] = &rDataStore;
			aArguments[3] = table;
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
			// Get a const pointer to the property.
			WeakAny valueConstPointer;
			if (!property.TryGetConstPtr(objectThis, valueConstPointer))
			{
				SerializeContextScope scope(rContext, DataNode(), property.GetMemberTypeInfo(), property.GetName());

				// Track if any of the complex forms of serialization succeed.
				Bool bSuccess = false;

				// Try direct get serialization.
				Any anyValue;

				// If we can get the value, we can attempt to serialize it.
				if (property.TryGet(objectThis, anyValue))
				{
					// First attempt non-complex type serialization - this
					// is cheaper and faster if it succeeds.
					if (FromAnyToTable(anyValue, rDataStore, table, property.GetName()))
					{
						// Done.
						bSuccess = true;
					}
					else
					{
						// Otherwise, grab a const pointer to the value and
						// perform the standard full serialization path.
						valueConstPointer = anyValue.GetWeakAnyConstPointerToValue();
						if (TrySerializeToTable(rContext, rDataStore, table, property.GetName(), valueConstPointer, bSkipPostSerialize))
						{
							// Done.
							bSuccess = true;
						}
					}
				}

				// If all complex serialization actions failed, then
				// we need to get this value by pointer.
				if (!bSuccess)
				{
					if (!rContext.HandleError(SerializeError::kFailedGettingValue))
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
				SerializeContextScope scope(rContext, DataNode(), valueConstPointer.GetTypeInfo(), property.GetName());
				if (!TrySerializeToTable(rContext, rDataStore, table, property.GetName(), valueConstPointer, bSkipPostSerialize))
				{
					return false;
				}
			}
		}
	}

	// If we're performing a post serialize step, find the method and invoke it.
	if (!bSkipPostSerialize && !bInParent)
	{
		// Polymorphic key handle.
		{
			Attributes::PolymorphicKey const* pPolymorphicKey = type.GetAttribute<Attributes::PolymorphicKey>(true);

			// If the type has a polymorphic key, make sure type info is written.
			if (nullptr != pPolymorphicKey)
			{
				if (!rDataStore.SetStringToTable(table, pPolymorphicKey->m_Key, type.GetName()))
				{
					if (!rContext.HandleError(SerializeError::kFailedSettingValueToTable))
					{
						return false;
					}
				}
			}
		}

		// Custom post serialize attribute.
		{
			Attributes::PostSerializeType const* pAttribute = type.GetAttribute<Attributes::PostSerializeType>(true);

			// If no post serialize attribute, we're done - return true.
			if (nullptr == pAttribute || pAttribute->m_SerializeMethodName.IsEmpty())
			{
				return true;
			}

			// The rest of this block is post serialize processing.

			Method const* pMethod = type.GetMethod(pAttribute->m_SerializeMethodName);
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
			MethodArguments aArguments;
			aArguments[0] = &rContext;
			Bool bReturnValue = false;
			if ((!pMethod->TryInvoke(returnValue, objectThis) &&
				!pMethod->TryInvoke(returnValue, objectThis, aArguments)) ||
				!TypeConstruct(returnValue, bReturnValue))
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
	}

	return true;
}

/**
 * Serializes an arbitrary object into a DataStore.
 *
 * @param[in] inObject Pointer to input object to serialize.
 * @param[in] rOut DataStore to populate with the object.
 *
 * @return true if the serialization succeeded, or false if an error occurred.
 */
Bool SerializeToDataStore(
	const WeakAny& inObject,
	DataStore& rOut)
{
	DataStore dataStore;
	dataStore.MakeArray();

	// Serialize the object to the DataStore
	if (SerializeObjectToArray(
		ContentKey(),
		dataStore,
		dataStore.GetRootNode(),
		0u,
		inObject) &&
		// Replace the root with the array element.
		dataStore.ReplaceRootWithArrayElement(dataStore.GetRootNode(), 0u))
	{
		// Swap and return success.
		rOut.Swap(dataStore);
		return true;
	}

	// Failure.
	return false;
}

/**
 * Serializes an arbitrary object into a string.
 *
 * @param[in] inObject Pointer to input object to serialize.
 * @param[out] rsOut String to write the object to.
 * @param[in] bMultiline true to format multiline, false otherwise.
 * @param[in] iIdentationLevel The starting indentation level of multiline formatting.
 * @param[in] bSortTableKeysAlphabetical true to lexographically sort table keys in the output, false otherwise.
 *
 * @return true if the serialization succeeded, or false if an error occurred.
 */
Bool SerializeToString(
	const WeakAny& inObject,
	String& rsOut,
	Bool bMultiline /*= false*/,
	Int iIndentationLevel /*= 0*/,
	Bool bSortTableKeysAlphabetical /*= false*/)
{
	DataStore dataStore;

	dataStore.MakeArray();

	// Serialize the object to the DataStore
	if (!SerializeObjectToArray(
		ContentKey(),
		dataStore,
		dataStore.GetRootNode(),
		0u,
		inObject))
	{
		return false;
	}

	DataNode dataNode;
	if (!dataStore.GetValueFromArray(dataStore.GetRootNode(), 0, dataNode))
	{
		return false;
	}

	// Commit to a string and return.
	dataStore.ToString(dataNode, rsOut, bMultiline, iIndentationLevel, bSortTableKeysAlphabetical);
	return true;
}

} // namespace Seoul::Reflection
