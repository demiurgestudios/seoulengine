/**
 * \file Script::FunctionInterface.cpp
 * \brief Special type for C++ methods to be bound into Lua that require
 * support for optional/variable arguments and/or multiple return values.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ReflectionDefine.h"
#include "ScriptFunctionInterface.h"
#include "ScriptLua.h"
#include "ScriptUtils.h"
#include "ScriptVm.h"

namespace Seoul
{

SEOUL_TYPE(Script::FunctionInterface, TypeFlags::kDisableNew);

namespace Script
{

FunctionInterface::FunctionInterface(lua_State* pVm)
	: m_iTopStart(lua_gettop(pVm))
	, m_pLuaVm(pVm)
	, m_iArgumentCount(m_iTopStart)
	, m_iInvalidArgument(-1)
{
}

Bool FunctionInterface::GetAny(Int32 i, const Reflection::TypeInfo& expectedTypeId, Reflection::Any& rAny) const
{
	return ToAny(m_pLuaVm, GetArgLuaIndex(i), rAny, expectedTypeId);
}

Bool FunctionInterface::GetObject(Int32 i, SharedPtr<VmObject>& rp) const
{
	if (IsNone(i))
	{
		return false;
	}

	if (IsNil(i))
	{
		rp.Reset();
		return true;
	}

	lua_pushvalue(m_pLuaVm, GetArgLuaIndex(i));
	Int const iObject = luaL_ref(m_pLuaVm, LUA_REGISTRYINDEX);
	rp.Reset(SEOUL_NEW(MemoryBudgets::Scripting) VmObject(
		Script::GetScriptVm(m_pLuaVm)->GetHandle(),
		iObject));

	return true;
}

Vm* FunctionInterface::GetScriptVm() const
{
	return Script::GetScriptVm(m_pLuaVm);
}


Bool FunctionInterface::GetByteBuffer(Int32 i, ByteBuffer& rb) const
{
	if (IsStringCoercible(i))
	{
		size_t z = 0;
		rb.m_pData = (void*)lua_tolstring(m_pLuaVm, GetArgLuaIndex(i), &z);
		rb.m_zDataSizeInBytes = (UInt32)z;
		return true;
	}

	return false;
}

Bool FunctionInterface::GetTable(Int32 i, DataStore& r) const
{
	return TableToDataStore(m_pLuaVm, GetArgLuaIndex(i), r);
}

Bool FunctionInterface::GetUserData(Int32 i, Reflection::WeakAny& rWeakAnyOfPointer) const
{
	return Script::GetUserData(m_pLuaVm, GetArgLuaIndex(i), rWeakAnyOfPointer);
}

Bool FunctionInterface::PushReturnBinderFromWeakRegistry(void* pNativeInstanceKey)
{
	// Get the weak registry.
	lua_pushlightuserdata(m_pLuaVm, kpScriptWeakRegistryKey);
	lua_rawget(m_pLuaVm, LUA_REGISTRYINDEX);

	// Lookup by instance pointer.
	lua_pushlightuserdata(m_pLuaVm, pNativeInstanceKey);
	lua_rawget(m_pLuaVm, -2);

	// If nil, pop both and return false.
	if (lua_isnil(m_pLuaVm, -1))
	{
		// Pop nil and the weak registry.
		lua_pop(m_pLuaVm, 2);

		// Done.
		return false;
	}
	// Otherwise, remove the weak registry and return true.
	else
	{
		lua_remove(m_pLuaVm, -2);
		return true;
	}
}

void FunctionInterface::PushReturnByteBuffer(const ByteBuffer& byteBuffer)
{
	lua_pushlstring(m_pLuaVm, (const char*)byteBuffer.m_pData, (size_t)byteBuffer.m_zDataSizeInBytes);
}

void FunctionInterface::PushReturnFilePath(FilePath filePath)
{
	PushReturnAny(filePath);
}

void FunctionInterface::PushReturnObject(const SharedPtr<VmObject>& p)
{
	SEOUL_ASSERT(m_pLuaVm.IsValid());

	if (!p.IsValid())
	{
		lua_pushnil(m_pLuaVm);
	}
	else
	{
		p->PushOntoVmStack(m_pLuaVm);
	}
}

void FunctionInterface::PushReturnAny(const Reflection::Any& any)
{
	PushAny(m_pLuaVm, any);
}

Bool FunctionInterface::PushReturnDataNode(
	const DataStore& dataStore,
	const DataNode& dataNode,
	Bool bConvertNilToEmptyTable /*= false*/,
	Bool bPrefetchAssets /*= false*/)
{
	return PushDataNode(m_pLuaVm, dataStore, dataNode, bConvertNilToEmptyTable, bPrefetchAssets);
}

Bool FunctionInterface::PushReturnUserDataType(const Reflection::Type& type)
{
	Script::GetScriptVm(m_pLuaVm)->InsideLockBindType(type, false);
	LuaGetMetatable(m_pLuaVm, type, false);
	return true;
}

/**
 * This hook is called by the cfunction wrapper used for reflection method
 * invocation when it is about to return.
 *
 * Normal client code should never call it (this function may also trigger
 * a longjmp for script error handling).
 */
Int FunctionInterface::OnCFuncExit() const
{
	// Early out if everythingi s ok.
	if (!HasError())
	{
		return GetReturnCount();
	}

	// Apply error handling - this will longjmp.

	// Get the error message.
	lua_pushlightuserdata(m_pLuaVm, kpScriptErrorMessageKey);
	lua_rawget(m_pLuaVm, LUA_REGISTRYINDEX);
	const char* sErrorMessage = lua_tostring(m_pLuaVm, -1);
	lua_pop(m_pLuaVm, 1);

	// Apply handling based on the data available.
	if (GetInvalidArgument() < 0)
	{
		if (nullptr == sErrorMessage || '\0' == sErrorMessage[0])
		{
			luaL_error(m_pLuaVm, "invocation error");
		}
		else
		{
			luaL_error(m_pLuaVm, "invocation error: %s", sErrorMessage);
		}
	}
	else
	{
		if (nullptr == sErrorMessage || '\0' == sErrorMessage[0])
		{
			luaL_error(m_pLuaVm, "invalid argument %d", (Int)m_iInvalidArgument);
		}
		else
		{
			luaL_error(m_pLuaVm, "invalid argument %d: %s", (Int)m_iInvalidArgument, sErrorMessage);
		}
	}

	return 0;
}

Bool FunctionInterface::InternalGetEnum(
	Int32 i,
	Int32& ri,
	const Reflection::Type& type) const
{
	Int32 const iLuaIndex = (GetArgLuaIndex(i));
	switch (lua_type(m_pLuaVm, iLuaIndex))
	{
	case LUA_TNUMBER:
		{
			lua_Number const fValue = lua_tonumber(m_pLuaVm, iLuaIndex);
			ri = (Int32)fValue;
			return true;
		}
	case LUA_TSTRING:
		{
			Reflection::Enum const* pEnum = type.TryGetEnum();
			if (nullptr == pEnum)
			{
				return false;
			}

			size_t zStringLengthInBytes = 0;
			const char* sString = lua_tolstring(m_pLuaVm, iLuaIndex, &zStringLengthInBytes);

			HString name;
			if (!HString::Get(name, sString, (UInt32)zStringLengthInBytes))
			{
				return false;
			}

			Int iValue = 0;
			if (!pEnum->TryGetValue(name, iValue))
			{
				return false;
			}

			ri = iValue;
			return true;
		}
	default:
		return false;
	};
}

void FunctionInterface::InternalGetTableAsComplex(Int32 i, const Reflection::WeakAny& objectPtr) const
{
	objectPtr.GetType().FromScript(m_pLuaVm, GetArgLuaIndex(i), objectPtr);
}

void FunctionInterface::InternalPushReturnAnyAsTable(const Reflection::WeakAny& objectPtr)
{
	objectPtr.GetType().ToScript(m_pLuaVm, objectPtr);
}

void* FunctionInterface::InternalPushReturnUserData(const Reflection::Type& type)
{
	using namespace Reflection;

	void* pInstance = nullptr;
	{
		// TODO: This is boilerplate in several places,
		// and there are also places that use just lua_newuserdata
		// and rely on the fact that what they are creating
		// has a trivial destructor. Need to wrap all this
		// in a Script::NewUserData<>.
		//
		// Destructor invocation requires registering the type
		// as private user data on creation.
		auto const uIndex = (type.HasDestructorDelegate() ? type.GetRegistryIndex() + 1u : 0u);
		size_t zSizeInBytes = (size_t)type.GetTypeInfo().GetSizeInBytes();
		pInstance = lua_newuserdataex(m_pLuaVm, zSizeInBytes, uIndex);
		if (nullptr == pInstance)
		{
			return nullptr;
		}

		if (!type.InPlaceNew(pInstance, zSizeInBytes).IsValid())
		{
			// pop the instance.
			lua_pop(m_pLuaVm, 1);
			return nullptr;
		}

		Script::GetScriptVm(m_pLuaVm)->InsideLockBindType(type, false);
		LuaGetMetatable(m_pLuaVm, type, false);
		lua_setmetatable(m_pLuaVm, -2);
	}

	return pInstance;
}

} // namespace Script

} // namespace Seoul
