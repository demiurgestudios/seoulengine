/**
 * \file ReflectionDataStoreUtil.h
 * \brief Utilities for convert data in a DataStore to/from concrete
 * C++ types using Reflection.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef REFLECTION_DATA_STORE_UTIL_H
#define REFLECTION_DATA_STORE_UTIL_H

#include "Color.h"
#include "DataStore.h"
#include "DataStoreParser.h"
#include "FilePath.h"
#include "Geometry.h"
#include "HashSet.h"
#include "Matrix4D.h"
#include "Quaternion.h"
#include "ReflectionDeserialize.h"
#include "ReflectionScript.h"
#include "ReflectionSerialize.h"
#include "ReflectionUtil.h"
#include "SeoulHString.h"
#include "SeoulString.h"
#include "SeoulTime.h"
#include "SeoulUUID.h"
#include "Vector2D.h"
#include "Vector3D.h"
#include "Vector4D.h"

namespace Seoul
{

template <typename T>
struct DataNodeHandler<T, true>
{
	static const Bool Value = true;
	static Bool FromDataNode(Reflection::SerializeContext&, const DataStore& dataStore, const DataNode& dataNode, T& reValue)
	{
		Byte const* s = nullptr;
		UInt32 u = 0u;
		HString identifier;
		Int iValue = -1;
		if (dataStore.AsString(dataNode, s, u) &&
			HString::Get(identifier, s, u))
		{
			if (EnumOf<T>().TryGetValue(identifier, iValue))
			{
				reValue = (T)iValue;
				return true;
			}
		}

		return false;
	}

	static Bool ToArray(Reflection::SerializeContext&, DataStore& rDataStore, const DataNode& array, UInt32 uIndex, T eValue)
	{
		HString identifier;
		if (EnumOf<T>().TryGetName(eValue, identifier))
		{
			return rDataStore.SetStringToArray(array, uIndex, identifier);
		}

		return false;
	}

	static Bool ToTable(Reflection::SerializeContext&, DataStore& rDataStore, const DataNode& table, HString key, T eValue)
	{
		HString identifier;
		if (EnumOf<T>().TryGetName(eValue, identifier))
		{
			return rDataStore.SetStringToTable(table, key, identifier);
		}

		return false;
	}

	static void FromScript(lua_State* pVm, Int iOffset, T& r)
	{
		if (lua_type(pVm, iOffset) == LUA_TSTRING)
		{
			size_t z = 0;
			Byte const* s = lua_tolstring(pVm, iOffset, &z);

			HString name;
			if (!HString::Get(name, s, (UInt32)z))
			{
				r = (T)0;
				return;
			}

			Int iValue;
			if (!EnumOf<T>().TryGetValue(name, iValue))
			{
				r = (T)0;
				return;
			}

			r = (T)iValue;
			return;
		}
		else
		{
			r = (T)lua_tointeger(pVm, iOffset);
		}
	}

	static void ToScript(lua_State* pVm, T v)
	{
		lua_pushinteger(pVm, (lua_Integer)v);
	}
};

template <typename T>
inline Bool FromDataNode(Reflection::SerializeContext& context, const DataStore& dataStore, const DataNode& dataNode, T& rValue)
{
	return DataNodeHandler<T, IsEnum<T>::Value>::FromDataNode(context, dataStore, dataNode, rValue);
}

template <typename T>
inline Bool ToDataStoreArray(Reflection::SerializeContext& context, DataStore& rDataStore, const DataNode& array, UInt32 uIndex, const T& value)
{
	return DataNodeHandler<T, IsEnum<T>::Value>::ToArray(context, rDataStore, array, uIndex, value);
}

template <typename T>
inline Bool ToDataStoreTable(Reflection::SerializeContext& context, DataStore& rDataStore, const DataNode& table, HString key, const T& value)
{
	return DataNodeHandler<T, IsEnum<T>::Value>::ToTable(context, rDataStore, table, key, value);
}

#define SEOUL_DEFINE_FROM_DATA_NODE_HANDLER(to_type, inter_type, get_func, set_array_func, set_table_func, script_type, from_script_func, to_script_func) \
	template <> struct DataNodeHandler<to_type, false> \
	{ \
		static const Bool Value = true; \
		static Bool FromDataNode(Reflection::SerializeContext&, const DataStore& dataStore, const DataNode& dataNode, to_type& rValue) \
		{ \
			inter_type interValue; \
			if (dataStore.get_func(dataNode, interValue)) \
			{ \
				rValue = (to_type)interValue; \
				return true; \
			} \
			return false; \
		} \
		static Bool ToArray(Reflection::SerializeContext&, DataStore& rDataStore, const DataNode& array, UInt32 uIndex, const to_type& value) \
		{ \
			return rDataStore.set_array_func(array, uIndex, (inter_type)value); \
		} \
		static Bool ToTable(Reflection::SerializeContext&, DataStore& rDataStore, const DataNode& table, HString key, const to_type& value) \
		{ \
			return rDataStore.set_table_func(table, key, (inter_type)value); \
		} \
		static void FromScript(lua_State* pVm, Int iOffset, to_type& r) \
		{ \
			r = (to_type)from_script_func(pVm, iOffset); \
		} \
		static void ToScript(lua_State* pVm, const to_type& v) \
		{ \
			to_script_func(pVm, (script_type)v); \
		} \
	}

SEOUL_DEFINE_FROM_DATA_NODE_HANDLER(Atomic32, Int32, AsInt32, SetInt32ValueToArray, SetInt32ValueToTable, lua_Integer, (Atomic32Type)lua_tointeger, lua_pushinteger);
SEOUL_DEFINE_FROM_DATA_NODE_HANDLER(Bool, Bool, AsBoolean, SetBooleanValueToArray, SetBooleanValueToTable, int, !!lua_toboolean, lua_pushboolean);
SEOUL_DEFINE_FROM_DATA_NODE_HANDLER(char, Int, AsInt32, SetInt32ValueToArray, SetInt32ValueToTable, lua_Integer, lua_tointeger, lua_pushinteger);
SEOUL_DEFINE_FROM_DATA_NODE_HANDLER(Int8, Int, AsInt32, SetInt32ValueToArray, SetInt32ValueToTable, lua_Integer, lua_tointeger, lua_pushinteger);
SEOUL_DEFINE_FROM_DATA_NODE_HANDLER(UInt8, Int, AsInt32, SetInt32ValueToArray, SetInt32ValueToTable, lua_Integer, lua_tointeger, lua_pushinteger);
SEOUL_DEFINE_FROM_DATA_NODE_HANDLER(Int16, Int, AsInt32, SetInt32ValueToArray, SetInt32ValueToTable, lua_Integer, lua_tointeger, lua_pushinteger);
SEOUL_DEFINE_FROM_DATA_NODE_HANDLER(UInt16, Int, AsInt32, SetInt32ValueToArray, SetInt32ValueToTable, lua_Integer, lua_tointeger, lua_pushinteger);
SEOUL_DEFINE_FROM_DATA_NODE_HANDLER(Int32, Int32, AsInt32, SetInt32ValueToArray, SetInt32ValueToTable, lua_Integer, lua_tointeger, lua_pushinteger);
SEOUL_DEFINE_FROM_DATA_NODE_HANDLER(UInt32, UInt32, AsUInt32, SetUInt32ValueToArray, SetUInt32ValueToTable, lua_Number, lua_tonumber, lua_pushnumber);
SEOUL_DEFINE_FROM_DATA_NODE_HANDLER(Int64, Int64, AsInt64, SetInt64ValueToArray, SetInt64ValueToTable, lua_Number, lua_tonumber, lua_pushnumber);
SEOUL_DEFINE_FROM_DATA_NODE_HANDLER(UInt64, UInt64, AsUInt64, SetUInt64ValueToArray, SetUInt64ValueToTable, lua_Number, lua_tonumber, lua_pushnumber);
SEOUL_DEFINE_FROM_DATA_NODE_HANDLER(Float32, Float32, AsFloat32, SetFloat32ValueToArray, SetFloat32ValueToTable, lua_Number, lua_tonumber, lua_pushnumber);
SEOUL_DEFINE_FROM_DATA_NODE_HANDLER(Float64, Float32, AsFloat32, SetFloat32ValueToArray, SetFloat32ValueToTable, lua_Number, lua_tonumber, lua_pushnumber);
#if !SEOUL_PLATFORM_LINUX && (SEOUL_PLATFORM_32 || !SEOUL_PLATFORM_ANDROID)
SEOUL_DEFINE_FROM_DATA_NODE_HANDLER(long, Int64, AsInt64, SetInt64ValueToArray, SetInt64ValueToTable, lua_Number, lua_tonumber, lua_pushnumber);
SEOUL_DEFINE_FROM_DATA_NODE_HANDLER(unsigned long, UInt64, AsUInt64, SetUInt64ValueToArray, SetUInt64ValueToTable, lua_Number, lua_tonumber, lua_pushnumber);
#else
SEOUL_DEFINE_FROM_DATA_NODE_HANDLER(long long, Int64, AsInt64, SetInt64ValueToArray, SetInt64ValueToTable, lua_Number, lua_tonumber, lua_pushnumber);
#endif

#undef SEOUL_DEFINE_FROM_DATA_NODE_HANDLER

template <typename T, int MEMORY_BUDGETS, typename TRAITS>
struct DataNodeHandler< HashSet<T, MEMORY_BUDGETS, TRAITS> , false>
{
	typedef HashSet<T, MEMORY_BUDGETS, TRAITS> HashSetType;
	static const Bool Value = true;

	static Bool FromDataNode(Reflection::SerializeContext& context, const DataStore& dataStore, const DataNode& dataNode, HashSetType& rValue)
	{
		using namespace Reflection;

		UInt32 uSize = 0u;
		if (!dataStore.GetArrayCount(dataNode, uSize))
		{
			return false;
		}

		auto const& typeInfo = TypeId<T>();

		HashSetType hashSet;
		for (UInt32 i = 0u; i < uSize; ++i)
		{
			DataNode value;
			if (dataStore.GetValueFromArray(dataNode, i, value))
			{
				SerializeContextScope scope(context, value, typeInfo, i);

				T key;
				if (!Reflection::DeserializeObject(context, dataStore, value, &key))
				{
					return false;
				}

				// TODO: I prefer being strict, but generally speaking,
				// a return false on insertion fail is pedantic. It becomes
				// a problem if key is a complex type where (a == b) is not
				// literally (memcmp(&a, &b, sizeof(a)) == 0) and an insertion
				// failure can have ramifications.
				//
				// If that latter case comes up, we should revisit ignoring insertion
				// failures here.
				(void)hashSet.Insert(key);
			}
		}

		rValue.Swap(hashSet);
		return true;
	}

	static Bool PopulateArrayFromHashSet(Reflection::SerializeContext& context, DataStore& rDataStore, const DataNode& array, const HashSetType& value)
	{
		using namespace Reflection;

		auto const& typeInfo = TypeId<T>();

		UInt32 uOut = 0u;
		auto const iBegin = value.Begin();
		auto const iEnd = value.End();
		for (auto i = iBegin; iEnd != i; ++i)
		{
			SerializeContextScope scope(context, DataNode(), typeInfo, uOut);

			const T& key = *i;
			if (!Reflection::SerializeObjectToArray(context, rDataStore, array, uOut, &key))
			{
				return false;
			}

			++uOut;
		}

		return true;
	}

	static Bool ToArray(Reflection::SerializeContext& context, DataStore& rDataStore, const DataNode& array, UInt32 uIndex, const HashSetType& value)
	{
		DataNode outArray;
		if (!rDataStore.SetArrayToArray(array, uIndex))
		{
			return false;
		}
		if (!rDataStore.GetValueFromArray(array, uIndex, outArray))
		{
			return false;
		}

		return PopulateArrayFromHashSet(context, rDataStore, outArray, value);
	}

	static Bool ToTable(Reflection::SerializeContext& context, DataStore& rDataStore, const DataNode& table, HString key, const HashSetType& rValue)
	{
		DataNode outArray;
		if (!rDataStore.SetArrayToTable(table, key))
		{
			return false;
		}
		if (!rDataStore.GetValueFromTable(table, key, outArray))
		{
			return false;
		}

		return PopulateArrayFromHashSet(context, rDataStore, outArray, rValue);
	}

	static inline void FromScript(lua_State* pVm, Int iOffset, HashSetType& r)
	{
		HashSetType t;

		T key;
		auto const& type = TypeOf<T>();
		Int const iTable = (iOffset < 0 ? iOffset - 1 : iOffset);
		lua_pushnil(pVm);
		while (0 != lua_next(pVm, iTable))
		{
			type.FromScript(pVm, -1, &key);

			(void)t.Insert(key);
			lua_pop(pVm, 1);
		}

		r.Swap(t);
	}

	static inline void ToScript(lua_State* pVm, const HashSetType& v)
	{
		lua_createtable(pVm, v.GetSize(), 0);
		Int iOut = 1;

		T key;
		auto const& type = TypeOf<T>();
		auto const iBegin = v.Begin();
		auto const iEnd = v.End();
		for (auto i = iBegin; iEnd != i; ++i)
		{
			key = *i;
			type.ToScript(pVm, &key);
			lua_rawseti(pVm, -2, iOut++);
		}
	}
}; // struct struct DataNodeHandler< HashSet<T, MEMORY_BUDGETS> , false>

template <typename T, int MEMORY_BUDGETS>
struct DataNodeHandler< List<T, MEMORY_BUDGETS> , false>
{
	typedef List<T, MEMORY_BUDGETS> ListType;
	static const Bool Value = true;

	static Bool FromDataNode(Reflection::SerializeContext& context, const DataStore& dataStore, const DataNode& dataNode, ListType& rValue)
	{
		using namespace Reflection;

		auto const& typeInfo = TypeId<T>();

		UInt32 uSize = 0u;
		if (!dataStore.GetArrayCount(dataNode, uSize))
		{
			return false;
		}

		ListType list;
		for (UInt32 i = 0u; i < uSize; ++i)
		{
			DataNode value;
			if (!dataStore.GetValueFromArray(dataNode, i, value))
			{
				return false;
			}

			SerializeContextScope scope(context, value, typeInfo, i);

			T listValue;
			if (!Reflection::DeserializeObject(context, dataStore, value, &listValue))
			{
				return false;
			}

			list.PushBack(listValue);
		}

		rValue.Swap(list);
		return true;
	}

	static Bool PopulateArrayFromList(Reflection::SerializeContext& context, DataStore& rDataStore, const DataNode& vectorArray, const ListType& v)
	{
		using namespace Reflection;

		auto const& typeInfo = TypeId<T>();

		UInt32 i = 0;
		auto iter = v.Begin();
		while (iter != v.End())
		{
			SerializeContextScope scope(context, DataNode(), typeInfo, i);

			const T* p = &(*iter);
			if (!Reflection::SerializeObjectToArray(context, rDataStore, vectorArray, i, p))
			{
				return false;
			}
			++iter;
			++i;
		}

		return true;
	}

	static Bool ToArray(Reflection::SerializeContext& context, DataStore& rDataStore, const DataNode& array, UInt32 uIndex, const ListType& v)
	{
		DataNode vectorArray;
		if (!rDataStore.SetArrayToArray(array, uIndex))
		{
			return false;
		}
		if (!rDataStore.GetValueFromArray(array, uIndex, vectorArray))
		{
			return false;
		}

		return PopulateArrayFromList(context, rDataStore, vectorArray, v);
	}

	static Bool ToTable(Reflection::SerializeContext& context, DataStore& rDataStore, const DataNode& table, HString key, const ListType& v)
	{
		DataNode vectorArray;
		if (!rDataStore.SetArrayToTable(table, key))
		{
			return false;
		}
		if (!rDataStore.GetValueFromTable(table, key, vectorArray))
		{
			return false;
		}

		return PopulateArrayFromList(context, rDataStore, vectorArray, v);
	}

	static inline void FromScript(lua_State* pVm, Int iOffset, ListType& r)
	{
		ListType l;

		T value;
		auto const& type = TypeOf<T>();
		Int const iCount = (Int)lua_rawlen(pVm, iOffset);
		for (Int i = 0; i < iCount; ++i)
		{
			lua_rawgeti(pVm, iOffset, (i+1));
			type.FromScript(pVm, -1, &value);

			l.PushBack(value);
			lua_pop(pVm, 1);
		}

		r.Swap(l);
	}

	static inline void ToScript(lua_State* pVm, const ListType& v)
	{
		lua_createtable(pVm, v.GetSize(), 0);
		Int iOut = 1;

		auto const& type = TypeOf<T>();
		auto const iBegin = v.Begin();
		auto const iEnd = v.End();
		for (auto i = iBegin; iEnd != i; ++i)
		{
			type.ToScript(pVm, &(*i));
			lua_rawseti(pVm, -2, iOut++);
		}
	}
};

template <>
struct DataNodeHandler< Color4 , false>
{
	static const Bool Value = true;
	static Bool FromDataNode(Reflection::SerializeContext&, const DataStore& dataStore, const DataNode& dataNode, Color4& rcValue)
	{
		DataNode value;
		return (
			dataStore.GetValueFromArray(dataNode, 0u, value) &&
			dataStore.AsFloat32(value, rcValue.R) &&
			dataStore.GetValueFromArray(dataNode, 1u, value) &&
			dataStore.AsFloat32(value, rcValue.G) &&
			dataStore.GetValueFromArray(dataNode, 2u, value) &&
			dataStore.AsFloat32(value, rcValue.B) &&
			dataStore.GetValueFromArray(dataNode, 3u, value) &&
			dataStore.AsFloat32(value, rcValue.A));
	}

	static Bool ToArray(Reflection::SerializeContext&, DataStore& rDataStore, const DataNode& array, UInt32 uIndex, const Color4& cValue)
	{
		DataNode vectorArray;
		return (
			rDataStore.SetArrayToArray(array, uIndex, 4u) &&
			rDataStore.GetValueFromArray(array, uIndex, vectorArray) &&
			rDataStore.SetFloat32ValueToArray(vectorArray, 0u, cValue.R) &&
			rDataStore.SetFloat32ValueToArray(vectorArray, 1u, cValue.G) &&
			rDataStore.SetFloat32ValueToArray(vectorArray, 2u, cValue.B) &&
			rDataStore.SetFloat32ValueToArray(vectorArray, 3u, cValue.A));
	}

	static Bool ToTable(Reflection::SerializeContext&, DataStore& rDataStore, const DataNode& table, HString key, const Color4& cValue)
	{
		DataNode vectorArray;
		return (
			rDataStore.SetArrayToTable(table, key, 4u) &&
			rDataStore.GetValueFromTable(table, key, vectorArray) &&
			rDataStore.SetFloat32ValueToArray(vectorArray, 0u, cValue.R) &&
			rDataStore.SetFloat32ValueToArray(vectorArray, 1u, cValue.G) &&
			rDataStore.SetFloat32ValueToArray(vectorArray, 2u, cValue.B) &&
			rDataStore.SetFloat32ValueToArray(vectorArray, 3u, cValue.A));
	}

	static void FromScript(lua_State* pVm, Int iOffset, Color4& r)
	{
		lua_rawgeti(pVm, iOffset, 1);
		r.R = (Float)lua_tonumber(pVm, -1);
		lua_pop(pVm, 1);

		lua_rawgeti(pVm, iOffset, 2);
		r.G = (Float)lua_tonumber(pVm, -1);
		lua_pop(pVm, 1);

		lua_rawgeti(pVm, iOffset, 3);
		r.B = (Float)lua_tonumber(pVm, -1);
		lua_pop(pVm, 1);

		lua_rawgeti(pVm, iOffset, 4);
		r.A = (Float)lua_tonumber(pVm, -1);
		lua_pop(pVm, 1);
	}

	static void ToScript(lua_State* pVm, const Color4& v)
	{
		lua_createtable(pVm, 4, 0);

		lua_pushnumber(pVm, v.R);
		lua_rawseti(pVm, -2, 1);

		lua_pushnumber(pVm, v.G);
		lua_rawseti(pVm, -2, 2);

		lua_pushnumber(pVm, v.B);
		lua_rawseti(pVm, -2, 3);

		lua_pushnumber(pVm, v.A);
		lua_rawseti(pVm, -2, 4);
	}
};

template <>
struct DataNodeHandler< ColorARGBu8 , false>
{
	static const Bool Value = true;

	typedef DataNodeHandler< Color4, false> Util;

	static Bool FromDataNode(Reflection::SerializeContext& context, const DataStore& dataStore, const DataNode& dataNode, ColorARGBu8& rcValue)
	{
		Color4 c;
		if (!Util::FromDataNode(context, dataStore, dataNode, c))
		{
			return false;
		}

		rcValue = c.ToColorARGBu8();
		return true;
	}

	static Bool ToArray(Reflection::SerializeContext& context, DataStore& rDataStore, const DataNode& array, UInt32 uIndex, ColorARGBu8 cValue)
	{
		return Util::ToArray(context, rDataStore, array, uIndex, Color4(cValue));
	}

	static Bool ToTable(Reflection::SerializeContext& context, DataStore& rDataStore, const DataNode& table, HString key, ColorARGBu8 cValue)
	{
		return Util::ToTable(context, rDataStore, table, key, Color4(cValue));
	}

	static void FromScript(lua_State* pVm, Int iOffset, ColorARGBu8 r)
	{
		Color4 c;
		Util::FromScript(pVm, iOffset, c);
		r = c.ToColorARGBu8();
	}

	static void ToScript(lua_State* pVm, ColorARGBu8 v)
	{
		Util::ToScript(pVm, Color4(v));
	}
};

template <>
struct DataNodeHandler<RGBA, false>
{
	static const Bool Value = true;
	static Bool FromDataNode(Reflection::SerializeContext&, const DataStore& dataStore, const DataNode& dataNode, RGBA& rValue);
	static Bool ToArray(Reflection::SerializeContext&, DataStore& rDataStore, const DataNode& array, UInt32 uIndex, RGBA value);
	static Bool ToTable(Reflection::SerializeContext&, DataStore& rDataStore, const DataNode& table, HString key, RGBA value);
	static Bool FromScript(lua_State* pVm, Int iOffset, RGBA& r);
	static void ToScript(lua_State* pVm, const RGBA& v);
};

template <>
struct DataNodeHandler<FilePath, false>
{
	static const Bool Value = true;
	static Bool FromDataNode(Reflection::SerializeContext&, const DataStore& dataStore, const DataNode& dataNode, FilePath& rValue)
	{
		if (dataStore.AsFilePath(dataNode, rValue))
		{
			return true;
		}

		// Handle String as a valid type on the DataStore side, to support JSON populated DataStores.
		Byte const* sString = nullptr;
		UInt32 zStringLengthInBytes = 0u;
		if (dataStore.AsString(dataNode, sString, zStringLengthInBytes))
		{
			return DataStoreParser::StringAsFilePath(sString, zStringLengthInBytes, rValue);
		}
		else
		{
			return false;
		}
	}

	static Bool ToArray(Reflection::SerializeContext&, DataStore& rDataStore, const DataNode& array, UInt32 uIndex, FilePath value)
	{
		return rDataStore.SetFilePathToArray(array, uIndex, value);
	}

	static Bool ToTable(Reflection::SerializeContext&, DataStore& rDataStore, const DataNode& table, HString key, FilePath value)
	{
		return rDataStore.SetFilePathToTable(table, key, value);
	}

	static inline void FromScript(lua_State* pVm, Int iOffset, FilePath& r)
	{
		// TODO: Need to verify the the type of the user data here.

		FilePath* p = (FilePath*)lua_touserdata(pVm, iOffset);
		if (nullptr == p)
		{
			r.Reset();
		}
		else
		{
			r = *p;
		}
	}
	static inline void ToScript(lua_State* pVm, const FilePath& filePath)
	{
		void* pFilePath = lua_newuserdata(pVm, sizeof(filePath));
		new (pFilePath) FilePath(filePath);
		LuaGetMetatable(pVm, TypeOf<FilePath>(), false);
		lua_setmetatable(pVm, -2);
	}
};

template <>
struct DataNodeHandler<FilePathRelativeFilename, false>
{
	static const Bool Value = true;
	static Bool FromDataNode(Reflection::SerializeContext&, const DataStore& dataStore, const DataNode& dataNode, FilePathRelativeFilename& rValue)
	{
		Byte const* sString = nullptr;
		UInt32 zStringLengthInBytes = 0u;
		if (dataStore.AsString(dataNode, sString, zStringLengthInBytes))
		{
			rValue = FilePathRelativeFilename(sString, zStringLengthInBytes);
			return true;
		}

		return false;
	}

	static Bool ToArray(Reflection::SerializeContext&, DataStore& rDataStore, const DataNode& array, UInt32 uIndex, FilePathRelativeFilename value)
	{
		return rDataStore.SetStringToArray(array, uIndex, value.ToString());
	}

	static Bool ToTable(Reflection::SerializeContext&, DataStore& rDataStore, const DataNode& table, HString key, FilePathRelativeFilename value)
	{
		return rDataStore.SetStringToTable(table, key, value.ToString());
	}

	static inline void FromScript(lua_State* pVm, Int iOffset, FilePathRelativeFilename& r)
	{
		size_t z = 0;
		Byte const* s = lua_tolstring(pVm, iOffset, &z);
		r = FilePathRelativeFilename(s, (UInt32)z);
	}

	static inline void ToScript(lua_State* pVm, const FilePathRelativeFilename& s)
	{
		lua_pushlstring(pVm, s.CStr(), (size_t)s.GetSizeInBytes());
	}
};

template <>
struct DataNodeHandler<HString, false>
{
	static const Bool Value = true;
	static Bool FromDataNode(Reflection::SerializeContext&, const DataStore& dataStore, const DataNode& dataNode, HString& rValue)
	{
		Byte const* sString = nullptr;
		UInt32 zStringLengthInBytes = 0u;
		if (dataStore.AsString(dataNode, sString, zStringLengthInBytes))
		{
			rValue = HString(sString, zStringLengthInBytes);
			return true;
		}

		return false;
	}

	static Bool ToArray(Reflection::SerializeContext&, DataStore& rDataStore, const DataNode& array, UInt32 uIndex, HString value)
	{
		return rDataStore.SetStringToArray(array, uIndex, value);
	}

	static Bool ToTable(Reflection::SerializeContext&, DataStore& rDataStore, const DataNode& table, HString key, HString value)
	{
		return rDataStore.SetStringToTable(table, key, value);
	}

	static inline void FromScript(lua_State* pVm, Int iOffset, HString& r)
	{
		size_t z = 0;
		Byte const* s = lua_tolstring(pVm, iOffset, &z);
		r = HString(s, (UInt32)z);
	}

	static inline void ToScript(lua_State* pVm, const HString& s)
	{
		lua_pushlstring(pVm, s.CStr(), (size_t)s.GetSizeInBytes());
	}
};

template <>
struct DataNodeHandler<String, false>
{
	static const Bool Value = true;
	static Bool FromDataNode(Reflection::SerializeContext&, const DataStore& dataStore, const DataNode& dataNode, String& rsValue)
	{
		return dataStore.AsString(dataNode, rsValue);
	}

	static Bool ToArray(Reflection::SerializeContext&, DataStore& rDataStore, const DataNode& array, UInt32 uIndex, const String& sValue)
	{
		return rDataStore.SetStringToArray(array, uIndex, sValue);
	}

	static Bool ToTable(Reflection::SerializeContext&, DataStore& rDataStore, const DataNode& table, HString key, const String& sValue)
	{
		return rDataStore.SetStringToTable(table, key, sValue);
	}

	static inline void FromScript(lua_State* pVm, Int iOffset, String& r)
	{
		size_t z = 0;
		Byte const* s = lua_tolstring(pVm, iOffset, &z);
		r.Assign(s, (UInt32)z);
	}
	static inline void ToScript(lua_State* pVm, const String& s)
	{
		lua_pushlstring(pVm, s.CStr(), (size_t)s.GetSize());
	}
};

template <typename T, typename U>
struct DataNodeHandler< Pair<T, U>, false>
{
	static const Bool Value = true;
	static Bool FromDataNode(Reflection::SerializeContext& context, const DataStore& dataStore, const DataNode& dataNode, Pair<T,U>& rvValue)
	{
		DataNode value;
		return (
			dataStore.GetValueFromArray(dataNode, 0u, value) &&
			Reflection::DeserializeObject(context, dataStore, value, &rvValue.First) &&
			dataStore.GetValueFromArray(dataNode, 1u, value) &&
			Reflection::DeserializeObject(context, dataStore, value, &rvValue.Second));
	}

	static Bool ToArray(Reflection::SerializeContext& context, DataStore& rDataStore, const DataNode& array, UInt32 uIndex, const Pair<T,U>& vValue)
	{
		DataNode vectorArray;
		return (
			rDataStore.SetArrayToArray(array, uIndex, 2u) &&
			rDataStore.GetValueFromArray(array, uIndex, vectorArray) &&
			Reflection::SerializeObjectToArray(context, rDataStore, vectorArray, 0u, &vValue.First) &&
			Reflection::SerializeObjectToArray(context, rDataStore, vectorArray, 1u, &vValue.Second));
	}

	static Bool ToTable(Reflection::SerializeContext& context, DataStore& rDataStore, const DataNode& table, HString key, const Pair<T,U>& vValue)
	{
		DataNode vectorArray;
		return (
			rDataStore.SetArrayToTable(table, key, 2u) &&
			rDataStore.GetValueFromTable(table, key, vectorArray) &&
			Reflection::SerializeObjectToArray(context, rDataStore, vectorArray, 0u, &vValue.First) &&
			Reflection::SerializeObjectToArray(context, rDataStore, vectorArray, 1u, &vValue.Second));
	}

	static void FromScript(lua_State* pVm, Int iOffset, Pair<T,U>& r)
	{
		// First of the pair.
		lua_rawgeti(pVm, iOffset, 1);
		TypeOf<T>().FromScript(pVm, -1, &r.First);
		lua_pop(pVm, 1);

		// Second of the pair.
		lua_rawgeti(pVm, iOffset, 2);
		TypeOf<U>().FromScript(pVm, -1, &r.Second);
		lua_pop(pVm, 1);
	}

	static void ToScript(lua_State* pVm, const Pair<T,U>& v)
	{
		lua_createtable(pVm, 2, 0);

		// First of the pair.
		TypeOf<T>().ToScript(pVm, &v.First);
		lua_rawseti(pVm, -2, 1);

		// Second of the pair.
		TypeOf<U>().ToScript(pVm, &v.Second);
		lua_rawseti(pVm, -2, 2);
	}
};

template <>
struct DataNodeHandler<Point2DInt, false>
{
	static const Bool Value = true;
	static Bool FromDataNode(Reflection::SerializeContext&, const DataStore& dataStore, const DataNode& dataNode, Point2DInt& rValue)
	{
		DataNode value;
		return (
			dataStore.GetValueFromArray(dataNode, 0u, value) &&
			dataStore.AsInt32(value, rValue.X) &&
			dataStore.GetValueFromArray(dataNode, 1u, value) &&
			dataStore.AsInt32(value, rValue.Y));
	}

	static Bool ToArray(Reflection::SerializeContext&, DataStore& rDataStore, const DataNode& array, UInt32 uIndex, const Point2DInt& value)
	{
		DataNode vectorArray;
		return (
			rDataStore.SetArrayToArray(array, uIndex, 2u) &&
			rDataStore.GetValueFromArray(array, uIndex, vectorArray) &&
			rDataStore.SetInt32ValueToArray(vectorArray, 0u, value.X) &&
			rDataStore.SetInt32ValueToArray(vectorArray, 1u, value.Y));
	}

	static Bool ToTable(Reflection::SerializeContext&, DataStore& rDataStore, const DataNode& table, HString key, const Point2DInt& value)
	{
		DataNode vectorArray;
		return (
			rDataStore.SetArrayToTable(table, key, 2u) &&
			rDataStore.GetValueFromTable(table, key, vectorArray) &&
			rDataStore.SetInt32ValueToArray(vectorArray, 0u, value.X) &&
			rDataStore.SetInt32ValueToArray(vectorArray, 1u, value.Y));
	}

	static void FromScript(lua_State* pVm, Int iOffset, Point2DInt& r)
	{
		lua_rawgeti(pVm, iOffset, 1);
		r.X = (Int)lua_tointeger(pVm, -1);
		lua_pop(pVm, 1);

		lua_rawgeti(pVm, iOffset, 2);
		r.Y = (Int)lua_tointeger(pVm, -1);
		lua_pop(pVm, 1);
	}

	static void ToScript(lua_State* pVm, const Point2DInt& v)
	{
		lua_createtable(pVm, 2, 0);

		lua_pushinteger(pVm, v.X);
		lua_rawseti(pVm, -2, 1);

		lua_pushinteger(pVm, v.Y);
		lua_rawseti(pVm, -2, 2);
	}
};

template <>
struct DataNodeHandler<Quaternion, false>
{
	static const Bool Value = true;
	static Bool FromDataNode(Reflection::SerializeContext&, const DataStore& dataStore, const DataNode& dataNode, Quaternion& rvValue)
	{
		DataNode value;
		if (
			dataStore.GetValueFromArray(dataNode, 0u, value) &&
			dataStore.AsFloat32(value, rvValue.X) &&
			dataStore.GetValueFromArray(dataNode, 1u, value) &&
			dataStore.AsFloat32(value, rvValue.Y) &&
			dataStore.GetValueFromArray(dataNode, 2u, value) &&
			dataStore.AsFloat32(value, rvValue.Z) &&
			dataStore.GetValueFromArray(dataNode, 3u, value) &&
			dataStore.AsFloat32(value, rvValue.W))
		{
			return true;
		}

		return false;
	}

	static Bool ToArray(Reflection::SerializeContext&, DataStore& rDataStore, const DataNode& array, UInt32 uIndex, const Quaternion& vValue)
	{
		DataNode vectorArray;
		return (
			rDataStore.SetArrayToArray(array, uIndex, 4u) &&
			rDataStore.GetValueFromArray(array, uIndex, vectorArray) &&
			rDataStore.SetFloat32ValueToArray(vectorArray, 0u, vValue.X) &&
			rDataStore.SetFloat32ValueToArray(vectorArray, 1u, vValue.Y) &&
			rDataStore.SetFloat32ValueToArray(vectorArray, 2u, vValue.Z) &&
			rDataStore.SetFloat32ValueToArray(vectorArray, 3u, vValue.W));
	}

	static Bool ToTable(Reflection::SerializeContext&, DataStore& rDataStore, const DataNode& table, HString key, const Quaternion& vValue)
	{
		DataNode vectorArray;
		return (
			rDataStore.SetArrayToTable(table, key, 4u) &&
			rDataStore.GetValueFromTable(table, key, vectorArray) &&
			rDataStore.SetFloat32ValueToArray(vectorArray, 0u, vValue.X) &&
			rDataStore.SetFloat32ValueToArray(vectorArray, 1u, vValue.Y) &&
			rDataStore.SetFloat32ValueToArray(vectorArray, 2u, vValue.Z) &&
			rDataStore.SetFloat32ValueToArray(vectorArray, 3u, vValue.W));
	}

	static void FromScript(lua_State* pVm, Int iOffset, Quaternion& r)
	{
		lua_rawgeti(pVm, iOffset, 1);
		r.X = (Float)lua_tonumber(pVm, -1);
		lua_pop(pVm, 1);

		lua_rawgeti(pVm, iOffset, 2);
		r.Y = (Float)lua_tonumber(pVm, -1);
		lua_pop(pVm, 1);

		lua_rawgeti(pVm, iOffset, 3);
		r.Z = (Float)lua_tonumber(pVm, -1);
		lua_pop(pVm, 1);

		lua_rawgeti(pVm, iOffset, 4);
		r.W = (Float)lua_tonumber(pVm, -1);
		lua_pop(pVm, 1);
	}

	static void ToScript(lua_State* pVm, const Quaternion& v)
	{
		lua_createtable(pVm, 4, 0);

		lua_pushnumber(pVm, v.X);
		lua_rawseti(pVm, -2, 1);

		lua_pushnumber(pVm, v.Y);
		lua_rawseti(pVm, -2, 2);

		lua_pushnumber(pVm, v.Z);
		lua_rawseti(pVm, -2, 3);

		lua_pushnumber(pVm, v.W);
		lua_rawseti(pVm, -2, 4);
	}
};

template <>
struct DataNodeHandler<TimeInterval, false>
{
	static const Bool Value = true;
	static Bool FromDataNode(Reflection::SerializeContext&, const DataStore& dataStore, const DataNode& dataNode, TimeInterval& rTimeInterval)
	{
		Int64 iMicroseconds = 0;
		if (dataStore.AsInt64(dataNode, iMicroseconds))
		{
			rTimeInterval = TimeInterval::FromMicroseconds(iMicroseconds);
			return true;
		}
		else
		{
			return false;
		}
	}

	static Bool ToArray(Reflection::SerializeContext&, DataStore& rDataStore, const DataNode& array, UInt32 uIndex, const TimeInterval& timeInterval)
	{
		return rDataStore.SetInt64ValueToArray(array, uIndex, timeInterval.GetMicroseconds());
	}

	static Bool ToTable(Reflection::SerializeContext&, DataStore& rDataStore, const DataNode& table, HString key, const TimeInterval& timeInterval)
	{
		return rDataStore.SetInt64ValueToTable(table, key, timeInterval.GetMicroseconds());
	}

	static inline void FromScript(lua_State* pVm, Int iOffset, TimeInterval& r)
	{
		// TODO: Need to verify the the type of the user data here.

		TimeInterval* p = (TimeInterval*)lua_touserdata(pVm, iOffset);
		if (nullptr == p)
		{
			r = TimeInterval();
		}
		else
		{
			r = *p;
		}
	}
	static inline void ToScript(lua_State* pVm, const TimeInterval& timeInterval)
	{
		void* p = lua_newuserdata(pVm, sizeof(timeInterval));
		new (p) TimeInterval(timeInterval);
		LuaGetMetatable(pVm, TypeOf<TimeInterval>(), false);
		lua_setmetatable(pVm, -2);
	}
};

template <>
struct DataNodeHandler<Vector2D, false>
{
	static const Bool Value = true;
	static Bool FromDataNode(Reflection::SerializeContext&, const DataStore& dataStore, const DataNode& dataNode, Vector2D& rvValue)
	{
		DataNode value;
		return (
			dataStore.GetValueFromArray(dataNode, 0u, value) &&
			dataStore.AsFloat32(value, rvValue.X) &&
			dataStore.GetValueFromArray(dataNode, 1u, value) &&
			dataStore.AsFloat32(value, rvValue.Y));
	}

	static Bool ToArray(Reflection::SerializeContext&, DataStore& rDataStore, const DataNode& array, UInt32 uIndex, const Vector2D& vValue)
	{
		DataNode vectorArray;
		return (
			rDataStore.SetArrayToArray(array, uIndex, 2u) &&
			rDataStore.GetValueFromArray(array, uIndex, vectorArray) &&
			rDataStore.SetFloat32ValueToArray(vectorArray, 0u, vValue.X) &&
			rDataStore.SetFloat32ValueToArray(vectorArray, 1u, vValue.Y));
	}

	static Bool ToTable(Reflection::SerializeContext&, DataStore& rDataStore, const DataNode& table, HString key, const Vector2D& vValue)
	{
		DataNode vectorArray;
		return (
			rDataStore.SetArrayToTable(table, key, 2u) &&
			rDataStore.GetValueFromTable(table, key, vectorArray) &&
			rDataStore.SetFloat32ValueToArray(vectorArray, 0u, vValue.X) &&
			rDataStore.SetFloat32ValueToArray(vectorArray, 1u, vValue.Y));
	}

	static void FromScript(lua_State* pVm, Int iOffset, Vector2D& r)
	{
		lua_rawgeti(pVm, iOffset, 1);
		r.X = (Float)lua_tonumber(pVm, -1);
		lua_pop(pVm, 1);

		lua_rawgeti(pVm, iOffset, 2);
		r.Y = (Float)lua_tonumber(pVm, -1);
		lua_pop(pVm, 1);
	}

	static void ToScript(lua_State* pVm, const Vector2D& v)
	{
		lua_createtable(pVm, 2, 0);

		lua_pushnumber(pVm, v.X);
		lua_rawseti(pVm, -2, 1);

		lua_pushnumber(pVm, v.Y);
		lua_rawseti(pVm, -2, 2);
	}
};

template <>
struct DataNodeHandler<Vector3D, false>
{
	static const Bool Value = true;
	static Bool FromDataNode(Reflection::SerializeContext&, const DataStore& dataStore, const DataNode& dataNode, Vector3D& rvValue)
	{
		DataNode value;
		return (
			dataStore.GetValueFromArray(dataNode, 0u, value) &&
			dataStore.AsFloat32(value, rvValue.X) &&
			dataStore.GetValueFromArray(dataNode, 1u, value) &&
			dataStore.AsFloat32(value, rvValue.Y) &&
			dataStore.GetValueFromArray(dataNode, 2u, value) &&
			dataStore.AsFloat32(value, rvValue.Z));
	}

	static Bool ToArray(Reflection::SerializeContext&, DataStore& rDataStore, const DataNode& array, UInt32 uIndex, const Vector3D& vValue)
	{
		DataNode vectorArray;
		return (
			rDataStore.SetArrayToArray(array, uIndex, 3u) &&
			rDataStore.GetValueFromArray(array, uIndex, vectorArray) &&
			rDataStore.SetFloat32ValueToArray(vectorArray, 0u, vValue.X) &&
			rDataStore.SetFloat32ValueToArray(vectorArray, 1u, vValue.Y) &&
			rDataStore.SetFloat32ValueToArray(vectorArray, 2u, vValue.Z));
	}

	static Bool ToTable(Reflection::SerializeContext&, DataStore& rDataStore, const DataNode& table, HString key, const Vector3D& vValue)
	{
		DataNode vectorArray;
		return (
			rDataStore.SetArrayToTable(table, key, 3u) &&
			rDataStore.GetValueFromTable(table, key, vectorArray) &&
			rDataStore.SetFloat32ValueToArray(vectorArray, 0u, vValue.X) &&
			rDataStore.SetFloat32ValueToArray(vectorArray, 1u, vValue.Y) &&
			rDataStore.SetFloat32ValueToArray(vectorArray, 2u, vValue.Z));
	}

	static void FromScript(lua_State* pVm, Int iOffset, Vector3D& r)
	{
		lua_rawgeti(pVm, iOffset, 1);
		r.X = (Float)lua_tonumber(pVm, -1);
		lua_pop(pVm, 1);

		lua_rawgeti(pVm, iOffset, 2);
		r.Y = (Float)lua_tonumber(pVm, -1);
		lua_pop(pVm, 1);

		lua_rawgeti(pVm, iOffset, 3);
		r.Z = (Float)lua_tonumber(pVm, -1);
		lua_pop(pVm, 1);
	}

	static void ToScript(lua_State* pVm, const Vector3D& v)
	{
		lua_createtable(pVm, 3, 0);

		lua_pushnumber(pVm, v.X);
		lua_rawseti(pVm, -2, 1);

		lua_pushnumber(pVm, v.Y);
		lua_rawseti(pVm, -2, 2);

		lua_pushnumber(pVm, v.Z);
		lua_rawseti(pVm, -2, 3);
	}
};

template <>
struct DataNodeHandler<Vector4D, false>
{
	static const Bool Value = true;
	static Bool FromDataNode(Reflection::SerializeContext&, const DataStore& dataStore, const DataNode& dataNode, Vector4D& rvValue)
	{
		DataNode value;
		return (
			dataStore.GetValueFromArray(dataNode, 0u, value) &&
			dataStore.AsFloat32(value, rvValue.X) &&
			dataStore.GetValueFromArray(dataNode, 1u, value) &&
			dataStore.AsFloat32(value, rvValue.Y) &&
			dataStore.GetValueFromArray(dataNode, 2u, value) &&
			dataStore.AsFloat32(value, rvValue.Z) &&
			dataStore.GetValueFromArray(dataNode, 3u, value) &&
			dataStore.AsFloat32(value, rvValue.W));
	}

	static Bool ToArray(Reflection::SerializeContext&, DataStore& rDataStore, const DataNode& array, UInt32 uIndex, const Vector4D& vValue)
	{
		DataNode vectorArray;
		return (
			rDataStore.SetArrayToArray(array, uIndex, 4u) &&
			rDataStore.GetValueFromArray(array, uIndex, vectorArray) &&
			rDataStore.SetFloat32ValueToArray(vectorArray, 0u, vValue.X) &&
			rDataStore.SetFloat32ValueToArray(vectorArray, 1u, vValue.Y) &&
			rDataStore.SetFloat32ValueToArray(vectorArray, 2u, vValue.Z) &&
			rDataStore.SetFloat32ValueToArray(vectorArray, 3u, vValue.W));
	}

	static Bool ToTable(Reflection::SerializeContext&, DataStore& rDataStore, const DataNode& table, HString key, const Vector4D& vValue)
	{
		DataNode vectorArray;
		return (
			rDataStore.SetArrayToTable(table, key, 4u) &&
			rDataStore.GetValueFromTable(table, key, vectorArray) &&
			rDataStore.SetFloat32ValueToArray(vectorArray, 0u, vValue.X) &&
			rDataStore.SetFloat32ValueToArray(vectorArray, 1u, vValue.Y) &&
			rDataStore.SetFloat32ValueToArray(vectorArray, 2u, vValue.Z) &&
			rDataStore.SetFloat32ValueToArray(vectorArray, 3u, vValue.W));
	}

	static void FromScript(lua_State* pVm, Int iOffset, Vector4D& r)
	{
		lua_rawgeti(pVm, iOffset, 1);
		r.X = (Float)lua_tonumber(pVm, -1);
		lua_pop(pVm, 1);

		lua_rawgeti(pVm, iOffset, 2);
		r.Y = (Float)lua_tonumber(pVm, -1);
		lua_pop(pVm, 1);

		lua_rawgeti(pVm, iOffset, 3);
		r.Z = (Float)lua_tonumber(pVm, -1);
		lua_pop(pVm, 1);

		lua_rawgeti(pVm, iOffset, 4);
		r.W = (Float)lua_tonumber(pVm, -1);
		lua_pop(pVm, 1);
	}

	static void ToScript(lua_State* pVm, const Vector4D& v)
	{
		lua_createtable(pVm, 4, 0);

		lua_pushnumber(pVm, v.X);
		lua_rawseti(pVm, -2, 1);

		lua_pushnumber(pVm, v.Y);
		lua_rawseti(pVm, -2, 2);

		lua_pushnumber(pVm, v.Z);
		lua_rawseti(pVm, -2, 3);

		lua_pushnumber(pVm, v.W);
		lua_rawseti(pVm, -2, 4);
	}
};

template <>
struct DataNodeHandler<WorldTime, false>
{
	static const Bool Value = true;
	static Bool FromDataNode(Reflection::SerializeContext&, const DataStore& dataStore, const DataNode& dataNode, WorldTime& rWorldTime)
	{
		Int64 iMicroseconds = 0;
		if (dataStore.AsInt64(dataNode, iMicroseconds))
		{
			rWorldTime.SetMicroseconds(iMicroseconds);
			return true;
		}
		else
		{
			String sWorldTimeISO8601;
			if (dataStore.AsString(dataNode, sWorldTimeISO8601))
			{
				WorldTime const worldTime = WorldTime::ParseISO8601DateTime(sWorldTimeISO8601);
				if (WorldTime() != worldTime)
				{
					rWorldTime = worldTime;
					return true;
				}
			}
		}

		return false;
	}

	static Bool ToArray(Reflection::SerializeContext&, DataStore& rDataStore, const DataNode& array, UInt32 uIndex, const WorldTime& worldTime)
	{
		return rDataStore.SetInt64ValueToArray(array, uIndex, worldTime.GetMicroseconds());
	}

	static Bool ToTable(Reflection::SerializeContext&, DataStore& rDataStore, const DataNode& table, HString key, const WorldTime& worldTime)
	{
		return rDataStore.SetInt64ValueToTable(table, key, worldTime.GetMicroseconds());
	}

	static inline void FromScript(lua_State* pVm, Int iOffset, WorldTime& r)
	{
		// TODO: Need to verify the the type of the user data here.

		WorldTime* p = (WorldTime*)lua_touserdata(pVm, iOffset);
		if (nullptr == p)
		{
			r.Reset();
		}
		else
		{
			r = *p;
		}
	}
	static inline void ToScript(lua_State* pVm, const WorldTime& worldTime)
	{
		void* p = lua_newuserdata(pVm, sizeof(worldTime));
		new (p) WorldTime(worldTime);
		LuaGetMetatable(pVm, TypeOf<WorldTime>(), false);
		lua_setmetatable(pVm, -2);
	}
};

template <>
struct DataNodeHandler<ContentKey, false>
{
	static const Bool Value = true;

	static Bool FromDataNode(Reflection::SerializeContext&, const DataStore& dataStore, const DataNode& dataNode, ContentKey& rValue);
	static Bool ToArray(Reflection::SerializeContext&, DataStore& rDataStore, const DataNode& array, UInt32 uIndex, const ContentKey& value);
	static Bool ToTable(Reflection::SerializeContext&, DataStore& rDataStore, const DataNode& table, HString key, const ContentKey& value);
	static void FromScript(lua_State* pVm, Int iOffset, ContentKey& r);
	static void ToScript(lua_State* pVm, const ContentKey& v);
}; // struct DataNodeHandler<ContentKey, false>

template <>
struct DataNodeHandler<Matrix4D, false>
{
	static const Bool Value = true;

	static Bool FromDataNode(Reflection::SerializeContext&, const DataStore& dataStore, const DataNode& dataNode, Matrix4D& rmValue);
	static Bool ToArray(Reflection::SerializeContext&, DataStore& rDataStore, const DataNode& array, UInt32 uIndex, const Matrix4D& mValue);
	static Bool ToTable(Reflection::SerializeContext&, DataStore& rDataStore, const DataNode& table, HString key, const Matrix4D& mValue);
	static void FromScript(lua_State* pVm, Int iOffset, Matrix4D& r);
	static void ToScript(lua_State* pVm, const Matrix4D& v);
}; // struct DataNodeHandler<Matrix4D, false>

// By default, pointers cannot be serialized from data or script. Certain types (string and void*) have special handling.
template <typename T>
struct DataNodeHandler<T*, false>
{
	static const Bool Value = true;

	static Bool FromDataNode(Reflection::SerializeContext&, const DataStore& dataStore, const DataNode& dataNode, T*& rp)
	{
		SEOUL_FAIL("Serialization and script bindings cannot be defined on arbitrary pointers. If you are using Script::FunctionInterface, the required signature is void(Script::FunctionInterface*).");
		return false;
	}

	static Bool ToArray(Reflection::SerializeContext&, DataStore& rDataStore, const DataNode& array, UInt32 uIndex, T* p)
	{
		SEOUL_FAIL("Serialization and script bindings cannot be defined on arbitrary pointers. If you are using Script::FunctionInterface, the required signature is void(Script::FunctionInterface*).");
		return false;
	}

	static Bool ToTable(Reflection::SerializeContext&, DataStore& rDataStore, const DataNode& table, HString key, T* p)
	{
		SEOUL_FAIL("Serialization and script bindings cannot be defined on arbitrary pointers. If you are using Script::FunctionInterface, the required signature is void(Script::FunctionInterface*).");
		return false;
	}

	static void FromScript(lua_State* pVm, Int iOffset, T*& rp)
	{
		SEOUL_FAIL("Serialization and script bindings cannot be defined on arbitrary pointers. If you are using Script::FunctionInterface, the required signature is void(Script::FunctionInterface*).");
		rp = nullptr;
	}

	static void ToScript(lua_State* pVm, T* p)
	{
		SEOUL_FAIL("Serialization and script bindings cannot be defined on arbitrary pointers. If you are using Script::FunctionInterface, the required signature is void(Script::FunctionInterface*).");
	}
}; // struct DataNodeHandler<void*, false>

template <>
struct DataNodeHandler<void*, false>
{
	static const Bool Value = true;

	static Bool FromDataNode(Reflection::SerializeContext&, const DataStore& dataStore, const DataNode& dataNode, void*& rp) { return false; }
	static Bool ToArray(Reflection::SerializeContext&, DataStore& rDataStore, const DataNode& array, UInt32 uIndex, void* p) { return false; }
	static Bool ToTable(Reflection::SerializeContext&, DataStore& rDataStore, const DataNode& table, HString key, void* p) { return false; }
	static void FromScript(lua_State* pVm, Int iOffset, void*& rp)
	{
		rp = lua_touserdata(pVm, iOffset);
	}

	static void ToScript(lua_State* pVm, void* p)
	{
		lua_pushlightuserdata(pVm, p);
	}
}; // struct DataNodeHandler<void*, false>

template <>
struct DataNodeHandler<Byte const*, false>
{
	static const Bool Value = true;

	// Although we could (safely) serialize from a cstring, we cannot deserialize to a cstring,
	// so we leave both directions unimplemented to minimize surprises.
	static Bool FromDataNode(Reflection::SerializeContext&, const DataStore& dataStore, const DataNode& dataNode, Byte const*& rp) { return false; }
	static Bool ToArray(Reflection::SerializeContext&, DataStore& rDataStore, const DataNode& array, UInt32 uIndex, Byte const* p) { return false; }
	static Bool ToTable(Reflection::SerializeContext&, DataStore& rDataStore, const DataNode& table, HString key, Byte const* p) { return false; }
	static void FromScript(lua_State* pVm, Int iOffset, Byte const*& rp)
	{
		rp = lua_tolstring(pVm, iOffset, nullptr);
	}

	static void ToScript(lua_State* pVm, Byte const* p)
	{
		lua_pushstring(pVm, p);
	}
}; // struct DataNodeHandler<Byte const*, false>

template <typename T>
struct DataNodeHandler< CheckedPtr<T>, false >
{
	static const Bool Value = true;

	static Bool FromDataNode(Reflection::SerializeContext& context, const DataStore& dataStore, const DataNode& dataNode, CheckedPtr<T>& rp)
	{
		using namespace Reflection;

		// Support null values.
		if (dataNode.IsNull())
		{
			SafeDelete(rp);
			return true;
		}

		typedef typename RemoveConstVolatile<T>::Type NonCvType;
		auto const& type(TypeOf<NonCvType>());
		auto weakAny(PolymorphicNew(type, dataStore, dataNode));
		if (!weakAny.IsValid())
		{
			return false;
		}

		if (!DeserializeObject(
			context,
			dataStore,
			dataNode,
			weakAny))
		{
			weakAny.GetType().Delete(weakAny);
			return false;
		}
		else
		{
			// Release existing.
			SafeDelete(rp);

			// Get new - must always succeed given what we've done here.
			{
				NonCvType* p = nullptr;
				SEOUL_VERIFY(weakAny.GetType().CastTo(weakAny, p));
				rp.Reset(p);
			}
			return true;
		}
	}

	static Bool ToArray(Reflection::SerializeContext& context, DataStore& rDataStore, const DataNode& array, UInt32 uIndex, const CheckedPtr<T>& p)
	{
		using namespace Reflection;

		// Support null values.
		if (!p.IsValid())
		{
			return rDataStore.SetNullValueToArray(array, uIndex);
		}

		return SerializeObjectToArray(
			context,
			rDataStore,
			array,
			uIndex,
			GetPolymorphicThis(p.Get()));
	}

	static Bool ToTable(Reflection::SerializeContext& context, DataStore& rDataStore, const DataNode& table, HString key, const CheckedPtr<T>& p)
	{
		using namespace Reflection;

		// Support null values.
		if (!p.IsValid())
		{
			return rDataStore.SetNullValueToTable(table, key);
		}

		return SerializeObjectToTable(
			context,
			rDataStore,
			table,
			key,
			GetPolymorphicThis(p.Get()));
	}

	static void FromScript(lua_State* pVm, Int iOffset, CheckedPtr<T>& rp)
	{
		using namespace Reflection;

		// Support null values.
		if (lua_isnil(pVm, iOffset))
		{
			SafeDelete(rp);
		}
		else
		{
			typedef typename RemoveConstVolatile<T>::Type NonCvType;
			auto const& type = TypeOf<NonCvType>();
			auto weakAny(PolymorphicNew(type, pVm, iOffset));
			if (!weakAny.IsValid())
			{
				// Return, polymorphic new warned.
				SafeDelete(rp);
				return;
			}

			weakAny.GetType().FromScript(pVm, iOffset, weakAny);

			// Release existing.
			SafeDelete(rp);

			// Get new - must always succeed given what we've done here.
			{
				NonCvType* p = nullptr;
				SEOUL_VERIFY(weakAny.GetType().CastTo(weakAny, p));
				rp.Reset(p);
			}
		}
	}

	static void ToScript(lua_State* pVm, const CheckedPtr<T>& p)
	{
		using namespace Reflection;

		// Support null values.
		if (!p.IsValid())
		{
			lua_pushnil(pVm);
		}
		// Otherwise, just push the type we point to.
		else
		{
			auto const weakAny(GetPolymorphicThis(p.Get()));
			weakAny.GetType().ToScript(pVm, weakAny);
		}
	}
}; // struct DataNodeHandler<CheckedPtr, false>

template <typename T>
struct DataNodeHandler< SharedPtr<T>, false >
{
	static const Bool Value = true;

	static Bool FromDataNode(Reflection::SerializeContext& context, const DataStore& dataStore, const DataNode& dataNode, SharedPtr<T>& rp)
	{
		using namespace Reflection;

		// Support null values.
		if (dataNode.IsNull())
		{
			rp.Reset();
			return true;
		}

		typedef typename RemoveConstVolatile<T>::Type NonCvType;
		auto const& type(TypeOf<NonCvType>());
		SharedPtr<T> pShared;
		auto weakAny(PolymorphicNew(type, dataStore, dataNode));
		if (!weakAny.IsValid())
		{
			return false;
		}

		// Get new - must always succeed given what we've done here.
		{
			NonCvType* p = nullptr;
			SEOUL_VERIFY(weakAny.GetType().CastTo(weakAny, p));
			pShared.Reset(p);
		}

		if (!DeserializeObject(
			context,
			dataStore,
			dataNode,
			weakAny))
		{
			return false;
		}
		else
		{
			// Release existing.
			rp = pShared;
			return true;
		}
	}

	static Bool ToArray(Reflection::SerializeContext& context, DataStore& rDataStore, const DataNode& array, UInt32 uIndex, const SharedPtr<T>& p)
	{
		using namespace Reflection;

		// Support null values.
		if (!p.IsValid())
		{
			return rDataStore.SetNullValueToArray(array, uIndex);
		}

		return SerializeObjectToArray(
			context,
			rDataStore,
			array,
			uIndex,
			GetPolymorphicThis(p.GetPtr()));
	}

	static Bool ToTable(Reflection::SerializeContext& context, DataStore& rDataStore, const DataNode& table, HString key, const SharedPtr<T>& p)
	{
		using namespace Reflection;

		// Support null values.
		if (!p.IsValid())
		{
			return rDataStore.SetNullValueToTable(table, key);
		}

		return SerializeObjectToTable(
			context,
			rDataStore,
			table,
			key,
			GetPolymorphicThis(p.GetPtr()));
	}

	static void FromScript(lua_State* pVm, Int iOffset, SharedPtr<T>& rp)
	{
		using namespace Reflection;

		// Support null values.
		if (lua_isnil(pVm, iOffset))
		{
			rp.Reset();
		}
		else
		{
			typedef typename RemoveConstVolatile<T>::Type NonCvType;
			auto const& type = TypeOf<NonCvType>();
			auto weakAny(PolymorphicNew(type, pVm, iOffset));
			if (!weakAny.IsValid())
			{
				// Return, polymorphic new warned.
				rp.Reset();
				return;
			}

			weakAny.GetType().FromScript(pVm, iOffset, weakAny);

			// Release existing.
			rp.Reset();

			// Get new - must always succeed given what we've done here.
			{
				NonCvType* p = nullptr;
				SEOUL_VERIFY(weakAny.GetType().CastTo(weakAny, p));
				rp.Reset(p);
			}
		}
	}

	static void ToScript(lua_State* pVm, const SharedPtr<T>& p)
	{
		using namespace Reflection;

		// Support null values.
		if (!p.IsValid())
		{
			lua_pushnil(pVm);
		}
		// Otherwise, just push the type we point to.
		else
		{
			auto const weakAny(GetPolymorphicThis(p.GetPtr()));
			weakAny.GetType().ToScript(pVm, weakAny);
		}
	}
}; // struct DataNodeHandler<SharedPtr, false>

template <>
struct DataNodeHandler<UUID, false>
{
	static const Bool Value = true;

	static Bool FromDataNode(Reflection::SerializeContext&, const DataStore& dataStore, const DataNode& dataNode, UUID& r);
	static Bool ToArray(Reflection::SerializeContext&, DataStore& rDataStore, const DataNode& array, UInt32 uIndex, const UUID& v);
	static Bool ToTable(Reflection::SerializeContext&, DataStore& rDataStore, const DataNode& table, HString key, const UUID& v);
	static void FromScript(lua_State* pVm, Int iOffset, UUID& r);
	static void ToScript(lua_State* pVm, const UUID& v);
}; // struct DataNodeHandler<Matrix4D, false>

} // namespace Seoul

#endif // include guard
