/**
 * \file ReflectionUtil.cpp
 * \brief Collection of miscellaneous utilities for conversion
 * and manipulation of data with reflection.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "DataStoreParser.h"
#include "FileManager.h"
#include "Logger.h"
#include "ReflectionAttributes.h"
#include "ReflectionDeserialize.h"
#include "ReflectionRegistry.h"
#include "ReflectionSerialize.h"
#include "ReflectionUtil.h"
#include "ScopedAction.h"

namespace Seoul
{

namespace Reflection
{

/**
 * Utiliy function, given a DataStore and a table defining a generic object,
 * instantiates a new instnace of the object based on the table.
 *
 * @return non-null if instantiating the type was successful, false otherwise.
 */
WeakAny PolymorphicNew(const Type& type, const DataStore& dataStore, const DataNode& table)
{
	using namespace Attributes;

	// If the table has a 'Type' key, assume we're instantiating
	// a polymorphic type.
	PolymorphicKey const* pKey = type.GetAttributes().GetAttribute<PolymorphicKey>();
	if (pKey != nullptr)
	{
		DataNode typeValue;
		Byte const* s = nullptr;
		UInt32 u = 0u;
		HString type;

		// If the key is not defined and the attribute does not
		// specify a default, consider this an error.
		if (!dataStore.GetValueFromTable(table, pKey->m_Key, typeValue))
		{
			if (pKey->m_Default.IsEmpty())
			{
				return WeakAny();
			}
			else
			{
				type = pKey->m_Default;
			}
		}
		// If the key is not an already existing identifier,
		// consider this an error.
		else if (!dataStore.AsString(typeValue, s, u) || !HString::Get(type, s, u))
		{
			return WeakAny();
		}

		// If we failed getting a Type object for the associated value, consider
		// this an error.
		Type const* pType = Reflection::Registry::GetRegistry().GetType(type);
		if (nullptr == pType)
		{
			return WeakAny();
		}

		// Otherwise, instantiate the type.
		return pType->New(pKey->m_eMemoryBudgetsType);
	}
	else
	{
		return type.New(MemoryBudgets::Game);
	}
}

/**
 * Utiliy function, given a VM context,
 * instantiates a new instnace of the object based on the table.
 *
 * @return non-null if instantiating the type was successful, false otherwise.
 */
WeakAny PolymorphicNew(const Type& type, lua_State* pVm, Int iOffset)
{
	using namespace Attributes;

	// If the table has a 'Type' key, assume we're instantiating
	// a polymorphic type.
	PolymorphicKey const* pKey = type.GetAttributes().GetAttribute<PolymorphicKey>();
	if (pKey != nullptr)
	{
		HString type;

		// If the key is not defined and the attribute does not
		// specify a default, consider this an error.
		lua_getfield(pVm, iOffset, pKey->m_Key.CStr());
		if (lua_isnil(pVm, -1))
		{
			lua_pop(pVm, 1);

			// If the attribute defines a default, use it. Otherwise,
			// the operation fails.
			if (pKey->m_Default.IsEmpty())
			{
				SEOUL_WARN("Script->Native is attempting to convert a script table "
					"into a polymorphic runtime type but the polymorphic key '%s' is not defined "
					"in the table.", pKey->m_Key.CStr());

				return WeakAny();
			}
			else
			{
				type = pKey->m_Default;
			}
		}
		else
		{
			// Get the type name from the script stack.
			size_t z = 0;
			Byte const* s = lua_tolstring(pVm, -1, &z);

			// If the key is not an already existing identifier,
			// consider this an error.
			Bool const bOk = HString::Get(type, s, (UInt32)z);

			// Remove the value from the stack.
			lua_pop(pVm, 1);

			// Failure to populate an existing HString from
			// the type data on the script stack is an error,
			// as it must exist for the name to potentially be
			// used to instantiate the type.
			if (!bOk)
			{
				return WeakAny();
			}
		}

		// If we failed getting a Type object for the associated value, consider
		// this an error.
		Type const* pType = Reflection::Registry::GetRegistry().GetType(type);
		if (nullptr == pType)
		{
			SEOUL_WARN("Script->Native is attempting to convert a script table "
				"into a polymorphic runtime type but the type '%s' could not be instantiated, "
				"check for typos or misconfigured native reflection.",
				type.CStr());
			return WeakAny();
		}

		// Otherwise, instantiate the type.
		return pType->New(pKey->m_eMemoryBudgetsType);
	}
	else
	{
		return type.New(MemoryBudgets::Reflection);
	}
}

/** Simple utility, loads an object from a JSON file on disk using reflection. */
Bool LoadObject(FilePath filePath, const Reflection::WeakAny& objectThis)
{
	DataStore dataStore;
	if (!DataStoreParser::FromFile(filePath, dataStore))
	{
		return false;
	}

	Reflection::DefaultSerializeContext context(filePath, dataStore, dataStore.GetRootNode(), objectThis.GetTypeInfo());
	return Seoul::Reflection::DeserializeObject(context, dataStore, dataStore.GetRootNode(), objectThis);
}

/** Simple utility, saves an object to a JSON file on disk using reflection. */
Bool SaveObject(const Reflection::WeakAny& objectThis, const String& sFileName)
{
	// First serialize - report if this fails.
	DataStore dataStore;
	if (!SerializeToDataStore(objectThis, dataStore))
	{
		SEOUL_LOG("%s: save failed, could not serialize.",
			sFileName.CStr());
		return false;
	}

	return SaveDataStore(dataStore, dataStore.GetRootNode(), sFileName);
}

/** Simple utility, saves a DataStore node to a JSON file on disk . */
Bool SaveDataStore(
	const DataStore& dataStore,
	const DataNode& dataNode,
	const String& sFileName)
{
	// If the file exists, extract hinting data from that file.
	void* pExisting = nullptr;
	UInt32 uExisting = 0u;
	auto const deferred(MakeDeferredAction([&]() { MemoryManager::Deallocate(pExisting); }));

	SharedPtr<DataStoreHint> pHint;
	if (FileManager::Get()->ReadAll(sFileName, pExisting, uExisting, 0u, MemoryBudgets::DataStore))
	{
		if (!DataStorePrinter::ParseHintsNoCopy((Byte const*)pExisting, uExisting, pHint))
		{
			pHint.Reset();
		}
	}

	// Placeholder.
	if (!pHint.IsValid())
	{
		pHint.Reset(SEOUL_NEW(MemoryBudgets::DataStore) DataStoreHintNone);
	}

	// Print.
	String sOut;
	DataStorePrinter::PrintWithHints(dataStore, dataNode, pHint, sOut);

	// Setup are intermediate and output file names.
	String const sTemporary(sFileName + ".bak");
	String const sOld(sFileName + ".old");

	// If an old file exists, try to restore from it before continuing
	// (our atomic write approach means we must assume that .old is
	// the valid file, since it would have been removed if the writing
	// succeeded).
	//
	// NOTE: We can require that, if sOld exists, we can successfully
	// move it into place (on disk), since we never expect a ".old"
	// file in any of our read-only virtual file systems, only on disk.
	if (FileManager::Get()->Exists(sOld))
	{
		(void)FileManager::Get()->Delete(sFileName);
		if (!FileManager::Get()->Rename(sOld, sFileName))
		{
			SEOUL_LOG("%s: a .old file exists, but it could not be restored. Likely, this "
				"means a previous session generated some invalid state that we don't know "
				"how to recover from. Please restore the file '%s' manually.",
				sFileName.CStr(),
				sFileName.CStr());
			return false;
		}
	}

	// Make sure we have a directory to write to.
	if (!FileManager::Get()->CreateDirPath(Path::GetDirectoryName(sFileName)))
	{
		SEOUL_LOG("%s: save failed, could not create output directory structure.", sFileName.CStr());
		return false;
	}

	// Save to the temporary file.
	if (!FileManager::Get()->WriteAll(sTemporary, (void const*)sOut.CStr(), sOut.GetSize()))
	{
		(void)FileManager::Get()->Delete(sTemporary);
		SEOUL_LOG("%s: save failed, data write to temporary failed.",
			sFileName.CStr());
		return false;
	}

	// Backup the existing output file if it exists on disk.
	(void)FileManager::Get()->Rename(sFileName, sOld);

	// Now move the temporary file into the output slot, and delete
	// the temporary file.
	Bool const bReturn = FileManager::Get()->Rename(sTemporary, sFileName);
	(void)FileManager::Get()->Delete(sTemporary);

	// On failure, try to restore the old file.
	if (!bReturn)
	{
		(void)FileManager::Get()->Rename(sOld, sFileName);
	}
	// Otherwise, delete the old file.
	else
	{
		(void)FileManager::Get()->Delete(sOld);
	}

	return bReturn;
}

} // namespace Reflection

/**
 * Outputs a string representation of the type described by typeInfo.
 */
void AppendTypeString(const Reflection::TypeInfo& typeInfo, String& rsOutput)
{
	using namespace Reflection;

	const Type& type = typeInfo.GetType();

	rsOutput.Append(type.GetName().CStr());

	if (typeInfo.IsInnerConstant())
	{
		rsOutput.Append(" const");
	}

	if (typeInfo.IsPointer())
	{
		rsOutput.Append('*');
	}

	if (typeInfo.IsConstant())
	{
		rsOutput.Append(" const");
	}

	if (typeInfo.IsReference())
	{
		rsOutput.Append("&");
	}
}

/**
 * @return A human readable String representation of typeInfo.
 */
String GetTypeString(const Reflection::TypeInfo& typeInfo)
{
	String sReturn;
	AppendTypeString(typeInfo, sReturn);
	return sReturn;
}

/**
 * @return For debug output, a string of the format "(arg0_type, arg1_type, ..., argn_type)"
 */
String GetSignatureString(const Reflection::MethodTypeInfo& methodTypeInfo)
{
	using namespace Reflection;

	String ret("(");
	if (0u == methodTypeInfo.m_uArgumentCount)
	{
		ret.Append("void");
	}
	else
	{
		for (UInt32 i = 0u; i < methodTypeInfo.m_uArgumentCount; ++i)
		{
			const TypeInfo& typeInfo = methodTypeInfo.GetArgumentTypeInfo(i);

			if (i > 0u)
			{
				ret.Append(", ");
			}

			AppendTypeString(typeInfo, ret);
		}
	}
	ret.Append(")");
	return ret;
}

/**
 * @return For debug output, a string of the format "(arg0_type, arg1_type, ..., argn_type)"
 */
String GetSignatureString(const Reflection::MethodArguments& aArguments)
{
	using namespace Reflection;

	String ret("(");
	if (0u == aArguments.GetSize())
	{
		ret.Append("void");
	}
	else
	{
		for (UInt32 i = 0u; i < aArguments.GetSize(); ++i)
		{
			if (aArguments[i].GetTypeInfo().IsVoid())
			{
				break;
			}

			const TypeInfo& typeInfo = aArguments[i].GetTypeInfo();

			if (i > 0u)
			{
				ret.Append(", ");
			}

			AppendTypeString(typeInfo, ret);
		}
	}
	ret.Append(")");
	return ret;
}

/**
 * @return For debug output, a string of the format "return_type method_name(arg0_type, arg1_type, ..., argn_type)"
 */
String GetSignatureString(const Reflection::Method& method)
{
	using namespace Reflection;

	String ret;
	AppendTypeString(method.GetTypeInfo().m_rReturnValueTypeInfo, ret);
	ret.Append(' ');
	ret.Append(String(method.GetName()));
	ret.Append(GetSignatureString(method.GetTypeInfo()));

	return ret;
}

/**
 * Attempt to assign the data in value into the array array in rDataStore,
 * at index uIndex.
 *
 * This method will succeed if value has SimpleTypeInfo other than kComplex.
 *
 * @return True if rAny was modified, false otherwise.
 */
Bool FromAnyToArray(const Reflection::Any& value, DataStore& rDataStore, const DataNode& array, UInt32 uIndex)
{
	using namespace Reflection;

	// Get the simple type from the root type.
	const TypeInfo& typeInfo = value.GetTypeInfo();
	SimpleTypeInfo const eSimpleTypeInfo = typeInfo.GetSimpleTypeInfo();
	switch (eSimpleTypeInfo)
	{
	case SimpleTypeInfo::kBoolean:
		{
			Bool bValue = false;
			return (TypeConstruct(value, bValue) && rDataStore.SetBooleanValueToArray(array, uIndex, bValue));
		}
		break;

	case SimpleTypeInfo::kCString:
		{
			Byte const* sString = nullptr;
			return (TypeConstruct(value, sString) && rDataStore.SetStringToArray(array, uIndex, sString));
		}
		break;

	case SimpleTypeInfo::kEnum:
		{
			HString enumName;
			return (value.GetType().TryGetEnum()->TryGetName(value, enumName) &&
				rDataStore.SetStringToArray(array, uIndex, enumName));
		}
		break;

	case SimpleTypeInfo::kFloat32: // fall-through
	case SimpleTypeInfo::kFloat64:
		{
			Float32 fValue = 0.0;
			return (TypeConstruct(value, fValue) && rDataStore.SetFloat32ValueToArray(array, uIndex, fValue));
		}
		break;

	case SimpleTypeInfo::kHString:
		{
			HString identifier;
			return (TypeConstruct(value, identifier) && rDataStore.SetStringToArray(array, uIndex, identifier));
		}
		break;

	case SimpleTypeInfo::kString:
		{
			String sString;
			return (TypeConstruct(value, sString) && rDataStore.SetStringToArray(array, uIndex, sString));
		}
		break;

		// Integral types except for UInt64 can be treated as an Int64.
	case SimpleTypeInfo::kInt8: // fall-through
	case SimpleTypeInfo::kInt16: // fall-through
	case SimpleTypeInfo::kInt32: // fall-through
	case SimpleTypeInfo::kInt64: // fall-through
	case SimpleTypeInfo::kUInt8: // fall-through
	case SimpleTypeInfo::kUInt16: // fall-through
	case SimpleTypeInfo::kUInt32:
		{
			Int64 iValue = 0;
			return (TypeConstruct(value, iValue) && rDataStore.SetInt64ValueToArray(array, uIndex, iValue));
		}
		break;

	case SimpleTypeInfo::kUInt64:
		{
			UInt64 uValue = 0u;
			return (TypeConstruct(value, uValue) && rDataStore.SetUInt64ValueToArray(array, uIndex, uValue));
		}
		break;

	case SimpleTypeInfo::kComplex: // fall-through
	default:
		return false;
	};
}

/**
 * Attempt to assign the data in value into the table table in rDataStore,
 * at key key.
 *
 * This method will succeed if value has SimpleTypeInfo other than kComplex.
 *
 * @return True if rAny was modified, false otherwise.
 */
Bool FromAnyToTable(const Reflection::Any& value, DataStore& rDataStore, const DataNode& table, HString key)
{
	using namespace Reflection;

	// Get the simple type from the root type.
	const TypeInfo& typeInfo = value.GetTypeInfo();
	SimpleTypeInfo const eSimpleTypeInfo = typeInfo.GetSimpleTypeInfo();
	switch (eSimpleTypeInfo)
	{
	case SimpleTypeInfo::kBoolean:
		{
			Bool bValue = false;
			return (TypeConstruct(value, bValue) && rDataStore.SetBooleanValueToTable(table, key, bValue));
		}
		break;

	case SimpleTypeInfo::kCString:
		{
			Byte const* sString = nullptr;
			return (TypeConstruct(value, sString) && rDataStore.SetStringToTable(table, key, sString));
		}
		break;

	case SimpleTypeInfo::kEnum:
		{
			HString enumName;
			return (value.GetType().TryGetEnum()->TryGetName(value, enumName) &&
				rDataStore.SetStringToTable(table, key, enumName));
		}
		break;

	case SimpleTypeInfo::kFloat32: // fall-through
	case SimpleTypeInfo::kFloat64:
		{
			Float32 fValue = 0.0;
			return (TypeConstruct(value, fValue) && rDataStore.SetFloat32ValueToTable(table, key, fValue));
		}
		break;

	case SimpleTypeInfo::kHString:
		{
			HString identifier;
			return (TypeConstruct(value, identifier) && rDataStore.SetStringToTable(table, key, identifier));
		}
		break;

	case SimpleTypeInfo::kString:
		{
			String sString;
			return (TypeConstruct(value, sString) && rDataStore.SetStringToTable(table, key, sString));
		}
		break;

		// Integral types except for UInt64 can be treated as an Int64.
	case SimpleTypeInfo::kInt8: // fall-through
	case SimpleTypeInfo::kInt16: // fall-through
	case SimpleTypeInfo::kInt32: // fall-through
	case SimpleTypeInfo::kInt64: // fall-through
	case SimpleTypeInfo::kUInt8: // fall-through
	case SimpleTypeInfo::kUInt16: // fall-through
	case SimpleTypeInfo::kUInt32:
		{
			Int64 iValue = 0;
			return (TypeConstruct(value, iValue) && rDataStore.SetInt64ValueToTable(table, key, iValue));
		}
		break;

	case SimpleTypeInfo::kUInt64:
		{
			UInt64 uValue = 0u;
			return (TypeConstruct(value, uValue) && rDataStore.SetUInt64ValueToTable(table, key, uValue));
		}
		break;

	case SimpleTypeInfo::kComplex: // fall-through
	default:
		return false;
	};
}

/**
 * Assign rAny with the value of value, if it has an external
 * concrete type that can be stored in an Any.
 *
 * This method will succeed for all types except kArray and kTable
 *
 * @return True if rAny was modified, false otherwise.
 */
Bool ToAny(const DataStore& dataStore, const DataNode& value, Reflection::Any& rAny)
{
	switch (value.GetType())
	{
	case DataNode::kNull: rAny.Reset(); return true;
	case DataNode::kBoolean: rAny = dataStore.AssumeBoolean(value); return true;
	case DataNode::kUInt32: rAny = dataStore.AssumeUInt32(value); return true;
	case DataNode::kInt32Big: rAny = dataStore.AssumeInt32Big(value); return true;
	case DataNode::kInt32Small: rAny = dataStore.AssumeInt32Small(value); return true;
	case DataNode::kFloat31: rAny = dataStore.AssumeFloat31(value); return true;
	case DataNode::kFloat32: rAny = dataStore.AssumeFloat32(value); return true;
	case DataNode::kFilePath:
		{
			FilePath filePath;
			SEOUL_VERIFY(dataStore.AsFilePath(value, filePath));
			rAny = filePath;
			return true;
		}
	case DataNode::kString:
		{
			String s;
			SEOUL_VERIFY(dataStore.AsString(value, s));
			rAny = s;
			return true;
		}
	case DataNode::kInt64: rAny = dataStore.AssumeInt64(value); return true;
	case DataNode::kUInt64: rAny = dataStore.AssumeUInt64(value); return true;
	default:
		return false;
	};
}

/**
 * Utility function that copies classes with reflection.
 *
 * @return True if the value was successfully copied, false otherwise.
 */
Bool ReflectionShallowCopy(Reflection::WeakAny const& source, Reflection::WeakAny const& dest)
{
	using namespace Reflection;

	SEOUL_ASSERT(source.GetType() == dest.GetType());

	const Type& type =source.GetType();
	UInt32 const nProperties = type.GetPropertyCount();
	for(UInt32 i = 0u; i < nProperties; ++i)
	{
		SEOUL_ASSERT(nullptr != type.GetProperty(i));
		const Property& prop = *type.GetProperty(i);
		Any value;
		if (prop.TryGet(source, value))
		{
			if(!prop.TrySet(dest, value))
			{
				return false;
			}
		}
		else
		{
			return false;
		}
	}

	return true;
}

} // namespace Seoul
