/**
 * \file ReflectionUtil.h
 * \brief Collection of miscellaneous utilities for conversion
 * and manipulation of data with reflection.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef REFLECTION_UTIL_H
#define REFLECTION_UTIL_H

#include "DataStore.h"
#include "Delegate.h"
#include "ReflectionAny.h"
#include "ReflectionEnum.h"
#include "ReflectionMethod.h"
#include "ReflectionMethodTypeInfo.h"
#include "ReflectionPolymorphic.h"
#include "ReflectionType.h"
#include "ReflectionWeakAny.h"

namespace Seoul
{

namespace Reflection
{

// Simple utility, loads an object from a JSON file on disk using reflection.
Bool LoadObject(FilePath filePath, const Reflection::WeakAny& objectThis);

Bool SaveDataStore(const DataStore& dataStore, const DataNode& dataNode, const String& sFileName);

// Simple utility, saves a DataStore node to a JSON file on disk.
static inline Bool SaveDataStore(const DataStore& dataStore, const DataNode& dataNode, FilePath filePath)
{
	return SaveDataStore(dataStore, dataNode, filePath.GetAbsoluteFilenameInSource());
}

Bool SaveObject(const Reflection::WeakAny& objectThis, const String& sFileName);

// Simple utility, saves an object to a JSON file on disk using reflection.
static inline Bool SaveObject(const Reflection::WeakAny& objectThis, FilePath filePath)
{
	return SaveObject(objectThis, filePath.GetAbsoluteFilenameInSource());
}

} // namespace Reflection

// Outputs a string representation of the type described by typeInfo.
void AppendTypeString(const Reflection::TypeInfo& typeInfo, String& rsOutput);

// Return a human readable String representation of typeInfo.
String GetTypeString(const Reflection::TypeInfo& typeInfo);

// For debug output, a string of the format "(arg0_type, arg1_type, ..., argn_type)"
String GetSignatureString(const Reflection::MethodTypeInfo& methodTypeInfo);

// For debug output, a string of the format "(arg0_type, arg1_type, ..., argn_type)"
String GetSignatureString(const Reflection::MethodArguments& aArguments);

// For debug output, a string of the format "return_type method_name(arg0_type, arg1_type, ..., argn_type)"
String GetSignatureString(const Reflection::Method& method);

// Attempt to assign the data in value into the array in rDataStore,
// at index uIndex.
Bool FromAnyToArray(const Reflection::Any& value, DataStore& rDataStore, const DataNode& array, UInt32 uIndex);

// Attempt to assign the data in value into the table in rDataStore,
// at key.
Bool FromAnyToTable(const Reflection::Any& value, DataStore& rDataStore, const DataNode& table, HString key);

// Assign rAny with the value of value, if it has an external
// concrete type that can be stored in an Any.
Bool ToAny(const DataStore& dataStore, const DataNode& value, Reflection::Any& rAny);

/**
 * Simpler helper function that uses reflection to convert an enum
 * value into a cstring - resolves to "<invalid enum>" if:
 * - the value was not a registered value of the enum.
 */
template <typename T, typename U>
inline Byte const* EnumToString(U value)
{
	const Reflection::Enum& e = EnumOf<T>();
	HString name;
	if (e.TryGetName(value, name))
	{
		return name.CStr();
	}

	return "<invalid enum>";
}

/**
 * Utility function, attempt to convert a DataNode value in a DataStore
 * to an enum type.
 *
 * @return True if the value was retrieved successfully, false otherwise.
 */
template <typename T>
inline Bool AsEnum(const DataStore& dataStore, const DataNode& value, T& reEnum)
{
	SEOUL_STATIC_ASSERT_MESSAGE(IsEnum<T>::Value, "Cannot convert DataNode value to an enum value if type T is not an enum.");

	Byte const* s = nullptr;
	UInt32 u = 0u;
	HString name;
	if (dataStore.AsString(value, s, u) &&
		HString::Get(name, s, u))
	{
		const Reflection::Enum& e = EnumOf<T>();

		Int iValue = -1;
		if (e.TryGetValue(name, iValue))
		{
			reEnum = (T)iValue;
			return true;
		}
	}

	return false;
}

/**
 * Utility function, attempt to store an enum value in a DataStore array.
 *
 * @return True if the value was successfully stored, false otherwise.
 */
template <typename T>
inline Bool SetEnumToArray(DataStore& rDataStore, const DataNode& array, UInt32 uIndex, T eEnum)
{
	STATIC_ASERT(IsEnum<T>::Value);

	const Reflection::Enum& e = EnumOf<T>();

	HString name;
	if (e.TryGetName(eEnum, name))
	{
		return rDataStore.SetStringToArray(array, uIndex, name);
	}

	return false;
}

/**
 * Utility function, attempt to store an enum value in a DataStore table.
 *
 * @return True if the value was successfully stored, false otherwise.
 */
template <typename T>
inline Bool SetEnumToTable(DataStore& rDataStore, const DataNode& table, HString key, T eEnum)
{
	STATIC_ASERT(IsEnum<T>::Value);

	const Reflection::Enum& e = EnumOf<T>();

	HString name;
	if (e.TryGetName(eEnum, name))
	{
		return rDataStore.SetStringToTable(table, key, name);
	}

	return false;
}

// Utility function that copies classes with reflection.
Bool ReflectionShallowCopy(Reflection::WeakAny const& source, Reflection::WeakAny const& dest);

} // namespace Seoul

#endif // include guard
