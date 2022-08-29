/**
 * \file ScriptFunctionInvoker.h
 * \brief Type for interacting with Script function contexts. Used
 * to invoke functions and get return values.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCRIPT_FUNCTION_INVOKER_H
#define SCRIPT_FUNCTION_INVOKER_H

#include "CheckedPtr.h"
#include "Prereqs.h"
#include "ReflectionType.h"
#include "ScriptArrayIndex.h"
#include "ScriptPrereqs.h"
#include "ScriptUtils.h"
#include "SeoulHString.h"
#include "SeoulProfiler.h"
#include "SeoulString.h"
#include "SharedPtr.h"
namespace Seoul { namespace Script { struct ByteBuffer; } }
namespace Seoul { namespace Script { class Vm; } }
namespace Seoul { namespace Script { class VmObject; } }

namespace Seoul::Script
{

class FunctionInvoker SEOUL_SEALED
{
public:
	FunctionInvoker(Vm& rVm, HString globalFunctionName);
	explicit FunctionInvoker(const SharedPtr<VmObject>& pObject, HString optionalName = HString());
	~FunctionInvoker();

	Bool GetAny(Int32 i, const Reflection::TypeInfo& expectedTypeId, Reflection::Any& rAny) const;

	Int GetArgumentCount() const
	{
		return Max(lua_gettop(m_pLuaVm) - m_iTopStart - 1, 0);
	}

	Bool GetArrayIndex(Int32 i, ArrayIndex& r) const
	{
		SEOUL_ASSERT(m_pLuaVm.IsValid());

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
		SEOUL_ASSERT(m_pLuaVm.IsValid());

		if (IsBoolean(i))
		{
			rb = (0 != lua_toboolean(m_pLuaVm, GetReturnLuaIndex(i)));
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
		SEOUL_ASSERT(m_pLuaVm.IsValid());

		FilePath const* pFilePath = GetUserData<FilePath>(i);
		if (nullptr == pFilePath)
		{
			return false;
		}

		rFilePath = *pFilePath;
		return true;
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
		SEOUL_ASSERT(m_pLuaVm.IsValid());

		if (IsNumberCoercible(i))
		{
			ri = (Int32)lua_tointeger(m_pLuaVm, GetReturnLuaIndex(i));
			return true;
		}

		return false;
	}

	Bool GetLightUserData(Int32 i, void*& rpLightUserData) const
	{
		SEOUL_ASSERT(m_pLuaVm.IsValid());

		if (IsLightUserData(i))
		{
			rpLightUserData = lua_touserdata(m_pLuaVm, GetReturnLuaIndex(i));
			return true;
		}

		return false;
	}

	Bool GetNumber(Int32 i, Float& rfValue) const
	{
		Double fValue = 0.0;
		if (GetNumber(i, fValue))
		{
			rfValue = (Float)fValue;
			return true;
		}

		return false;
	}

	Bool GetNumber(Int32 i, Double& rf) const
	{
		SEOUL_ASSERT(m_pLuaVm.IsValid());

		if (IsNumberCoercible(i))
		{
			rf = (Double)lua_tonumber(m_pLuaVm, GetReturnLuaIndex(i));
			return true;
		}
		return false;
	}

	Bool GetObject(Int32 i, SharedPtr<VmObject>& rp) const;

	Int GetReturnCount() const
	{
		return m_iReturnCount;
	}

	Bool GetString(Int32 i, Byte const*& rs, UInt32& ru) const
	{
		SEOUL_ASSERT(m_pLuaVm.IsValid());

		if (IsStringCoercible(i))
		{
			size_t z = 0;
			rs = lua_tolstring(m_pLuaVm, GetReturnLuaIndex(i), &z);
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

	Bool GetUserData(Int32 i, Reflection::WeakAny& rWeakAnyOfPointer) const;

	/**
	 * @return True if the specified target function can be invoked, false otherwise.
	 * Caller must check IsValid() before calling any other methods.
	 */
	Bool IsValid() const
	{
		return m_pLuaVm.IsValid();
	}

	Bool IsBoolean(Int32 i) const { SEOUL_ASSERT(m_pLuaVm.IsValid()); return (LUA_TBOOLEAN == lua_type(m_pLuaVm, GetReturnLuaIndex(i))); }
	Bool IsFunction(Int32 i) const { SEOUL_ASSERT(m_pLuaVm.IsValid()); return (LUA_TFUNCTION == lua_type(m_pLuaVm, GetReturnLuaIndex(i))); }
	Bool IsLightUserData(Int32 i) const { SEOUL_ASSERT(m_pLuaVm.IsValid()); return (LUA_TLIGHTUSERDATA == lua_type(m_pLuaVm, GetReturnLuaIndex(i))); }
	Bool IsNil(Int32 i) const { SEOUL_ASSERT(m_pLuaVm.IsValid()); return (LUA_TNIL == lua_type(m_pLuaVm, GetReturnLuaIndex(i))); }
	Bool IsNilOrNone(Int32 i) const { return IsNil(i) || IsNone(i); }
	Bool IsNone(Int32 i) const { SEOUL_ASSERT(m_pLuaVm.IsValid()); return (LUA_TNONE == lua_type(m_pLuaVm, GetReturnLuaIndex(i))); }
	Bool IsNumberCoercible(Int32 i) const { SEOUL_ASSERT(m_pLuaVm.IsValid()); return (0 != lua_isnumber(m_pLuaVm, GetReturnLuaIndex(i))); }
	Bool IsNumberExact(Int32 i) const { SEOUL_ASSERT(m_pLuaVm.IsValid()); return (LUA_TNUMBER == lua_type(m_pLuaVm, GetReturnLuaIndex(i))); }
	Bool IsStringCoercible(Int32 i) const { SEOUL_ASSERT(m_pLuaVm.IsValid()); return (0 != lua_isstring(m_pLuaVm, GetReturnLuaIndex(i))); }
	Bool IsStringExact(Int32 i) const { SEOUL_ASSERT(m_pLuaVm.IsValid()); return (LUA_TSTRING == lua_type(m_pLuaVm, GetReturnLuaIndex(i))); }
	Bool IsTable(Int32 i) const { SEOUL_ASSERT(m_pLuaVm.IsValid()); return (LUA_TTABLE == lua_type(m_pLuaVm, GetReturnLuaIndex(i))); }
	Bool IsUserData(Int32 i) const { SEOUL_ASSERT(m_pLuaVm.IsValid()); return (LUA_TUSERDATA == lua_type(m_pLuaVm, GetReturnLuaIndex(i))); }

	void PushAny(const Reflection::Any& any);
	void PushArrayIndex(const ArrayIndex& index) { PushUInt32((UInt32)index + 1); }
	Bool PushBinderFromWeakRegistry(void* pNativeInstanceKey);
	void PushBoolean(Bool b) { SEOUL_ASSERT(m_pLuaVm.IsValid()); lua_pushboolean(m_pLuaVm, (b ? 1 : 0)); }
	void PushByteBuffer(const ByteBuffer& byteBuffer);
	template <typename T>
	void PushEnumAsNumber(T v) { SEOUL_STATIC_ASSERT(IsEnum<T>::Value); PushInteger((Int32)v); }
	void PushFilePath(FilePath filePath);
	void PushInteger(Int32 i)  { SEOUL_ASSERT(m_pLuaVm.IsValid()); lua_pushinteger(m_pLuaVm, (lua_Integer)i); }
	void PushLightUserData(void* p) { SEOUL_ASSERT(m_pLuaVm.IsValid()); lua_pushlightuserdata(m_pLuaVm, p); }
	void PushNil() { SEOUL_ASSERT(m_pLuaVm.IsValid()); lua_pushnil(m_pLuaVm); }
	void PushNumber(Double f) { SEOUL_ASSERT(m_pLuaVm.IsValid()); lua_pushnumber(m_pLuaVm, f); }
	void PushObject(const SharedPtr<VmObject>& p);
	void PushString(HString s) { SEOUL_ASSERT(m_pLuaVm.IsValid()); lua_pushlstring(m_pLuaVm, s.CStr(), (size_t)s.GetSizeInBytes()); }
	void PushString(const String& s) { SEOUL_ASSERT(m_pLuaVm.IsValid()); lua_pushlstring(m_pLuaVm, s.CStr(), (size_t)s.GetSize()); }
	void PushString(Byte const* s) { SEOUL_ASSERT(m_pLuaVm.IsValid()); lua_pushstring(m_pLuaVm, s); }
	void PushString(Byte const* s, UInt32 u) { SEOUL_ASSERT(m_pLuaVm.IsValid()); lua_pushlstring(m_pLuaVm, s, (size_t)u); }
	void PushUInt32(UInt32 u) { SEOUL_ASSERT(m_pLuaVm.IsValid()); lua_pushnumber(m_pLuaVm, (lua_Number)u); }

	template <typename T>
	void PushAsTable(const T& v)
	{
		// Sanity check - can only be called on values that are not simple types.
		SEOUL_STATIC_ASSERT(Reflection::TypeInfoDetail::SimpleTypeInfoT<T>::Value == Reflection::SimpleTypeInfo::kComplex);

		SEOUL_ASSERT(m_pLuaVm.IsValid());

		InternalPushAnyAsTable(&v);
	}

	Bool PushDataNode(
		const DataStore& dataStore,
		const DataNode& dataNode,
		Bool bConvertNilToEmptyTable = false,
		Bool bPrefetchAssets = false);

	template <typename T>
	T* PushUserData()
	{
		SEOUL_ASSERT(m_pLuaVm.IsValid());

		return (T*)InternalPushUserData(TypeOf<T>());
	}

	Bool PushUserData(const Reflection::Type& type)
	{
		return (nullptr != InternalPushUserData(type));
	}

	Bool PushUserDataType(const Reflection::Type& type);

	Bool TryInvoke();

private:
	CheckedPtr<Vm> const m_p;
	Int const m_iTopStart;
	CheckedPtr<lua_State> const m_pLuaVm;
	Int m_iReturnCount;

	SEOUL_PROF_DEF_VAR(m_ProfName);

	Int GetReturnLuaIndex(Int i) const { return (m_iTopStart + i + 1); }

	Bool InternalGetEnum(Int32 i, Int32& ri, const Reflection::Type& type) const;
	void InternalGetTableAsComplex(Int32 i, const Reflection::WeakAny& objectPtr) const;
	void InternalPushAnyAsTable(const Reflection::WeakAny& objectPtr);
	void* InternalPushUserData(const Reflection::Type& type);

	static CheckedPtr<Vm> AcquireVm(Vm& rVm);
	static CheckedPtr<Vm> AcquireVm(const SharedPtr<VmObject>& pObject);

	SEOUL_DISABLE_COPY(FunctionInvoker);
}; // class Script::FunctionInvoker

} // namespace Seoul::Script

#endif // include guard
