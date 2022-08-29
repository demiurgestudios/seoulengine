/**
 * \file ScriptFunctionInterface.h
 * \brief Special type for C++ methods to be bound into Lua that require
 * support for optional/variable arguments and/or multiple return values.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCRIPT_FUNCTION_INTERFACE_H
#define SCRIPT_FUNCTION_INTERFACE_H

#include "CheckedPtr.h"
#include "Prereqs.h"
#include "ReflectionType.h"
#include "ScriptArrayIndex.h"
#include "ScriptPrereqs.h"
#include "ScriptUtils.h"
#include "SeoulHString.h"
#include "SeoulString.h"
#include "SharedPtr.h"
namespace Seoul { namespace Script { struct ByteBuffer; } }
namespace Seoul { namespace Script { class Vm; } }
namespace Seoul { namespace Script { class VmObject; } }

namespace Seoul::Script
{

class FunctionInterface SEOUL_SEALED
{
public:
	explicit FunctionInterface(lua_State* pVm);

	Bool GetAny(Int32 i, const Reflection::TypeInfo& expectedTypeId, Reflection::Any& rAny) const;

	Int GetArgumentCount() const
	{
		return m_iArgumentCount;
	}

	Bool GetArrayIndex(Int32 i, ArrayIndex& r) const
	{
		Double f = 0.0;
		if (GetNumber(i, f))
		{
			if (f <= 0)
			{
				r = ArrayIndex(UIntMax);
			}
			else
			{
				r = ArrayIndex((UInt32)(f- 1));
			}
			return true;
		}
		return false;
	}

	Bool GetBoolean(Int32 i, Bool& rb) const
	{
		if (IsBoolean(i))
		{
			rb = (0 != lua_toboolean(m_pLuaVm, GetArgLuaIndex(i)));
			return true;
		}
		return false;
	}

	template <typename T>
	Bool GetEnum(Int32 i, T& re) const
	{
		SEOUL_STATIC_ASSERT(IsEnum<T>::Value);

		Int32 iValue = 0;
		if (InternalGetEnum(i, iValue, TypeOf<T>()))
		{
			re = (T)iValue;
			return true;
		}

		return false;
	}

	Bool GetFilePath(Int32 i, FilePath& rFilePath) const
	{
		return GetUserDataValue(i, rFilePath);
	}

	Bool GetFunction(Int32 i, SharedPtr<VmObject>& rp) const
	{
		if (IsFunction(i))
		{
			return GetObject(i, rp);
		}
		return false;
	}

	Bool GetInteger(Int32 i, Int32& ri) const
	{
		if (IsNumberCoercible(i))
		{
			ri = (Int32)lua_tointeger(m_pLuaVm, GetArgLuaIndex(i));
			return true;
		}

		return false;
	}

	Int32 GetInvalidArgument() const
	{
		return (0 == m_iInvalidArgument ? -1 : m_iInvalidArgument);
	}

	Bool GetLightUserData(Int32 i, void*& rpLightUserData) const
	{
		if (IsLightUserData(i))
		{
			rpLightUserData = lua_touserdata(m_pLuaVm, GetArgLuaIndex(i));
			return true;
		}

		return false;
	}

	Bool GetNumber(Int32 i, Float32& rfValue) const
	{
		Double fValue = 0.0;
		if (GetNumber(i, fValue))
		{
			rfValue = (Float)fValue;
			return true;
		}

		return false;
	}

	Bool GetNumber(Int32 i, Double& rfValue) const
	{
		if (IsNumberCoercible(i))
		{
			rfValue = (Double)lua_tonumber(m_pLuaVm, GetArgLuaIndex(i));
			return true;
		}
		return false;
	}

	Bool GetObject(Int32 i, SharedPtr<VmObject>& rp) const;

	CheckedPtr<lua_State> GetLowLevelVm() const { return m_pLuaVm; }
	Vm* GetScriptVm() const;

	Int GetReturnCount() const
	{
		return (lua_gettop(m_pLuaVm) - m_iTopStart);
	}

	Bool GetString(Int32 i, Byte const*& rs, UInt32& ru) const
	{
		if (IsStringCoercible(i))
		{
			size_t z = 0;
			rs = lua_tolstring(m_pLuaVm, GetArgLuaIndex(i), &z);
			ru = (UInt32)z;
			return true;
		}

		return false;
	}

	Bool GetString(Int32 i, HString& r) const
	{
		Byte const* s = nullptr;
		UInt32 u = 0;
		if (GetString(i, s, u))
		{
			r = HString(s, u);
			return true;
		}

		return false;
	}

	Bool GetString(Int32 i, String& rs) const
	{
		Byte const* s = nullptr;
		UInt32 u = 0;
		if (GetString(i, s, u))
		{
			rs.Assign(s, u);
			return true;
		}

		return false;
	}

	Bool GetByteBuffer(Int32 i, ByteBuffer& rb) const;

	Bool GetTable(Int32 i, DataStore& r) const;

	template <typename T>
	void GetTableAsComplex(Int32 i, T& r) const
	{
		InternalGetTableAsComplex(i, &r);
	}

	Bool GetUInt32(Int32 i, UInt32& ruValue) const
	{
		Double fValue = 0;
		if (GetNumber(i, fValue))
		{
			ruValue = (UInt32)fValue;
			return true;
		}

		return false;
	}

	template <typename T>
	T* GetUserData(Int32 i) const
	{
		Reflection::WeakAny weakAnyToPointer;
		if (GetUserData(i, weakAnyToPointer))
		{
			T* p = nullptr;
			if (Reflection::PointerCast(weakAnyToPointer, p))
			{
				return p;
			}
		}

		return nullptr;
	}

	template <typename T>
	Bool GetUserDataValue(Int32 i, T& rUserData) const
	{
		T const* pUserData = GetUserData<T>(i);
		if (nullptr == pUserData)
		{
			return false;
		}

		rUserData = *pUserData;
		return true;
	}

	Bool GetUserData(Int32 i, Reflection::WeakAny& rWeakAnyOfPointer) const;

	Bool GetWorldTime(Int32 i, WorldTime& rWorldTime) const
	{
		return GetUserDataValue(i, rWorldTime);
	}

	/**
	 * @return true if an error was raised by the invoked function.
	 */
	Bool HasError() const
	{
		return (m_iInvalidArgument >= 0);
	}

	Bool IsBoolean(Int32 i) const { return (LUA_TBOOLEAN == lua_type(m_pLuaVm, i + 1)); }
	Bool IsFunction(Int32 i) const { return (LUA_TFUNCTION == lua_type(m_pLuaVm, i + 1)); }
	Bool IsLightUserData(Int32 i) const { return (LUA_TLIGHTUSERDATA == lua_type(m_pLuaVm, i + 1)); }
	Bool IsNil(Int32 i) const { return (LUA_TNIL == lua_type(m_pLuaVm, i + 1)); }
	Bool IsNilOrNone(Int32 i) const { return IsNil(i) || IsNone(i); }
	Bool IsNone(Int32 i) const { return (LUA_TNONE == lua_type(m_pLuaVm, i + 1)); }
	Bool IsNumberCoercible(Int32 i) const { return (0 != lua_isnumber(m_pLuaVm, i + 1)); }
	Bool IsNumberExact(Int32 i) const { return (LUA_TNUMBER == lua_type(m_pLuaVm, i + 1)); }
	Bool IsStringCoercible(Int32 i) const { return (0 != lua_isstring(m_pLuaVm, i + 1)); }
	Bool IsStringExact(Int32 i) const { return (LUA_TSTRING == lua_type(m_pLuaVm, i + 1)); }
	Bool IsTable(Int32 i) const { return (LUA_TTABLE == lua_type(m_pLuaVm, i + 1)); }
	Bool IsUserData(Int32 i) const { return (LUA_TUSERDATA == lua_type(m_pLuaVm, i + 1)); }

	void PushReturnAny(const Reflection::Any& any);
	void PushReturnArrayIndex(const ArrayIndex& index) { PushReturnUInt32((UInt32)index + 1); }
	Bool PushReturnBinderFromWeakRegistry(void* pNativeInstanceKey);
	void PushReturnBoolean(Bool b) { lua_pushboolean(m_pLuaVm, (b ? 1 : 0)); }
	void PushReturnByteBuffer(const ByteBuffer& byteBuffer);
	template <typename T>
	void PushReturnEnumAsNumber(T v) { SEOUL_STATIC_ASSERT(IsEnum<T>::Value); PushReturnInteger((Int32)v); }
	void PushReturnFilePath(FilePath filePath);
	void PushReturnInteger(Int32 i)  { lua_pushinteger(m_pLuaVm, (lua_Integer)i); }
	void PushReturnLightUserData(void* p) { lua_pushlightuserdata(m_pLuaVm, p); }
	void PushReturnNil() { lua_pushnil(m_pLuaVm); }
	void PushReturnNumber(Double f) { lua_pushnumber(m_pLuaVm, f); }
	void PushReturnObject(const SharedPtr<VmObject>& p);
	void PushReturnString(HString s) { lua_pushlstring(m_pLuaVm, s.CStr(), (size_t)s.GetSizeInBytes()); }
	void PushReturnString(const String& s) { lua_pushlstring(m_pLuaVm, s.CStr(), (size_t)s.GetSize()); }
	void PushReturnString(Byte const* s) { lua_pushstring(m_pLuaVm, s); }
	void PushReturnString(Byte const* s, UInt32 u) { lua_pushlstring(m_pLuaVm, s, (size_t)u); }
	void PushReturnUInt32(UInt32 u) { lua_pushnumber(m_pLuaVm, (lua_Number)u); }

	template <typename T>
	void PushReturnAsTable(const T& v)
	{
		// Sanity check - can only be called on values that are not simple types.
		SEOUL_STATIC_ASSERT(Reflection::TypeInfoDetail::SimpleTypeInfoT<T>::Value == Reflection::SimpleTypeInfo::kComplex);

		InternalPushReturnAnyAsTable(&v);
	}

	Bool PushReturnDataNode(
		const DataStore& dataStore,
		const DataNode& dataNode,
		Bool bConvertNilToEmptyTable = false,
		Bool bPrefetchAssets = false);

	template <typename T>
	T* PushReturnUserData()
	{
		return (T*)InternalPushReturnUserData(TypeOf<T>());
	}

	Bool PushReturnUserData(const Reflection::Type& type)
	{
		return (nullptr != InternalPushReturnUserData(type));
	}

	Bool PushReturnUserDataType(const Reflection::Type& type);

	void RaiseError(Int iInvalidArgument)
	{
		m_iInvalidArgument = Max(iInvalidArgument + 1, 0);
	}

	// NOTE: W.R.T format attribute, member functions have an implicit
	// 'this' so counting starts at 2 for explicit arguments.
	void RaiseError(const char* sFormat, ...) SEOUL_PRINTF_FORMAT_ATTRIBUTE(2, 3)
	{
		m_iInvalidArgument = 0;
		{
			String sError;
			va_list args;
			va_start(args, sFormat);
			sError = String::VPrintf(sFormat, args);
			va_end(args);

			lua_pushlightuserdata(m_pLuaVm, kpScriptErrorMessageKey);
			lua_pushlstring(m_pLuaVm, sError.CStr(), (UInt32)sError.GetSize());
			lua_settable(m_pLuaVm, LUA_REGISTRYINDEX);
		}
	}

	// NOTE: W.R.T format attribute, member functions have an implicit
	// 'this' so counting starts at 2 for explicit arguments.
	void RaiseError(Int iInvalidArgument, const char* sFormat, ...) SEOUL_PRINTF_FORMAT_ATTRIBUTE(3, 4)
	{
		m_iInvalidArgument = Max(iInvalidArgument + 1, 0);
		{
			String sError;
			va_list args;
			va_start(args, sFormat);
			sError = String::VPrintf(sFormat, args);
			va_end(args);

			lua_pushlightuserdata(m_pLuaVm, kpScriptErrorMessageKey);
			lua_pushlstring(m_pLuaVm, sError.CStr(), (UInt32)sError.GetSize());
			lua_settable(m_pLuaVm, LUA_REGISTRYINDEX);
		}
	}

	// This hook is called by the cfunction wrapper when it is about to return.
	// Normal client code should never call it (this function may also trigger
	// a longjmp for script error handling).
	Int OnCFuncExit() const;

private:
	// WARNING: The destructor of Script::FunctionInterface() may not be invoked
	// if OnCFuncExit() triggers a longjmp. As a result, don't put
	// any complex variables here (CheckedPtr<> is an exception, we assume its
	// destructor is a nop) which require their destructor to be invoked.
	Int const m_iTopStart;
	CheckedPtr<lua_State> const m_pLuaVm;
	Int const m_iArgumentCount;
	Int m_iInvalidArgument;

	Int GetArgLuaIndex(Int i) const { return (i + 1); }

	Bool InternalGetEnum(Int32 i, Int32& ri, const Reflection::Type& type) const;
	void InternalGetTableAsComplex(Int32 i, const Reflection::WeakAny& objectPtr) const;
	void InternalPushReturnAnyAsTable(const Reflection::WeakAny& objectPtr);
	void* InternalPushReturnUserData(const Reflection::Type& type);

	SEOUL_DISABLE_COPY(FunctionInterface);
}; // class Script::FunctionInterface

} // namespace Seoul::Script

#endif // include guard
