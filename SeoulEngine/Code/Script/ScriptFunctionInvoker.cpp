/**
 * \file ScriptFunctionInvoker.cpp
 * \brief Type for interacting with Script function contexts. Used
 * to invoke functions and get return values.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ScriptLua.h"
#include "ScriptFunctionInvoker.h"
#include "ScriptUtils.h"
#include "ScriptVm.h"

namespace Seoul::Script
{

static CheckedPtr<lua_State> PrepareVm(CheckedPtr<lua_State> pVm, HString globalFunctionName)
{
	// Global function lookup. Name is required in this case.
	lua_getglobal(pVm, globalFunctionName.CStr());

	// Doesn't exist, mark as such.
	if (lua_isnil(pVm, -1))
	{
		// Release the nil value, return failure.
		lua_pop(pVm, 1);
		return nullptr;
	}

	return pVm;
}

static CheckedPtr<lua_State> PrepareVm(CheckedPtr<lua_State> pVm, const SharedPtr<VmObject>& p, HString optionalName)
{
	// May have no VM, so early out in this case.
	if (!pVm.IsValid())
	{
		return nullptr;
	}

	// Check the handle.
	if (!p.IsValid() || p->IsNil())
	{
		return nullptr;
	}

	// Get the object via the indirect reference.
	p->PushOntoVmStack(pVm);

	// If name is empty, then we want to just invoke iObject.
	if (!optionalName.IsEmpty())
	{
		// Invalid, mark as such.
		if (!lua_istable(pVm, -1) && !lua_isuserdata(pVm, -1))
		{
			// Release the value, return failure.
			lua_pop(pVm, 1);
			return nullptr;
		}

		// Get the member.
		lua_getfield(pVm, -1, optionalName.CStr());

		// On failure, cleanup and return immediately.
		if (lua_isnil(pVm, -1))
		{
			// Cleanup the stack and return failure.
			lua_pop(pVm, 2);
			return nullptr;
		}

		// Swap -1 with -2 (this is a func(self, ...) invocation.
		lua_insert(pVm, -2);
	}
	else if (lua_isnil(pVm, -1))
	{
		// Release the nil value, return failure.
		lua_pop(pVm, 1);
		return nullptr;
	}

	return pVm;
}

#if SEOUL_PROF_ENABLED
static const HString kProfAnonymous("<anonymous-script>");
#endif // /#if SEOUL_PROF_ENABLED

// NOTE: Order of initializer list is very precise. Must acquire the VM first (to lock the mutex),
// then grab top (so it is marked prior to getting the function to invoke), then finally
// prepare the Lua VM to get the actual function object to invoke.
FunctionInvoker::FunctionInvoker(Vm& rVm, HString globalFunctionName)
	: m_p(AcquireVm(rVm))
	, m_iTopStart(lua_gettop(rVm.m_pLuaVm))
	, m_pLuaVm(PrepareVm(rVm.m_pLuaVm, globalFunctionName))
	, m_iReturnCount(0)
{
	SEOUL_PROF_INIT_VAR(m_ProfName, globalFunctionName.IsEmpty() ? kProfAnonymous : globalFunctionName);
}

// NOTE: Order of initializer list is very precise. Must acquire the VM first (to lock the mutex),
// then grab top (so it is marked prior to getting the function to invoke), then finally
// prepare the Lua VM to get the actual function object to invoke.
FunctionInvoker::FunctionInvoker(const SharedPtr<VmObject>& pObject, HString optionalName /*= HString()*/)
	: m_p(AcquireVm(pObject))
	, m_iTopStart(m_p.IsValid() ? lua_gettop(m_p->m_pLuaVm) : 0)
	, m_pLuaVm(PrepareVm(m_p.IsValid() ? m_p->m_pLuaVm : nullptr, pObject, optionalName))
	, m_iReturnCount(0)
{
	SEOUL_PROF_INIT_VAR(m_ProfName, optionalName.IsEmpty() ? kProfAnonymous : optionalName);
}

FunctionInvoker::~FunctionInvoker()
{
	// Cleanup the stack as specified.
	if (m_pLuaVm.IsValid())
	{
		lua_settop(m_pLuaVm, m_iTopStart);
	}

	// Release our exclusive access.
	if (m_p.IsValid())
	{
		m_p->m_Mutex.Unlock();
	}
}

Bool FunctionInvoker::TryInvoke()
{
	SEOUL_PROF_VAR(m_ProfName);
	SEOUL_ASSERT(m_pLuaVm.IsValid());

	// vmSettings 
	// The stack has all arguments, plus the function object itself, so the
	// argument count is the push count - 1.
	Int32 const iArgs = GetArgumentCount();
	SEOUL_ASSERT(iArgs >= 0);

	auto const bSuccess = Pcall(m_pLuaVm, iArgs);

	// After invocation, update the return count.
	m_iReturnCount = (lua_gettop(m_pLuaVm) - m_iTopStart);
	return bSuccess;
}

Bool FunctionInvoker::GetAny(Int32 i, const Reflection::TypeInfo& expectedTypeId, Reflection::Any& rAny) const
{
	SEOUL_ASSERT(m_pLuaVm.IsValid());

	return ToAny(m_pLuaVm, GetReturnLuaIndex(i), rAny, expectedTypeId);
}

Bool FunctionInvoker::GetObject(Int32 i, SharedPtr<VmObject>& rp) const
{
	SEOUL_ASSERT(m_pLuaVm.IsValid());

	if (IsNone(i))
	{
		return false;
	}

	if (IsNil(i))
	{
		rp.Reset();
		return true;
	}

	lua_pushvalue(m_pLuaVm, GetReturnLuaIndex(i));
	Int const iObject = luaL_ref(m_pLuaVm, LUA_REGISTRYINDEX);
	rp.Reset(SEOUL_NEW(MemoryBudgets::Scripting) VmObject(
		GetScriptVm(m_pLuaVm)->GetHandle(),
		iObject));

	return true;
}

Bool FunctionInvoker::GetTable(Int32 i, DataStore& r) const
{
	SEOUL_ASSERT(m_pLuaVm.IsValid());

	return TableToDataStore(m_pLuaVm, GetReturnLuaIndex(i), r);
}

Bool FunctionInvoker::GetUserData(Int32 i, Reflection::WeakAny& rWeakAnyOfPointer) const
{
	SEOUL_ASSERT(m_pLuaVm.IsValid());

	return Script::GetUserData(m_pLuaVm, GetReturnLuaIndex(i), rWeakAnyOfPointer);
}

Bool FunctionInvoker::PushBinderFromWeakRegistry(void* pNativeInstanceKey)
{
	SEOUL_ASSERT(m_pLuaVm.IsValid());

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

void FunctionInvoker::PushByteBuffer(const ByteBuffer& byteBuffer)
{
	SEOUL_ASSERT(m_pLuaVm.IsValid());

	lua_pushlstring(m_pLuaVm, (const char*)byteBuffer.m_pData, (size_t)byteBuffer.m_zDataSizeInBytes);
}

void FunctionInvoker::PushFilePath(FilePath filePath)
{
	SEOUL_ASSERT(m_pLuaVm.IsValid());

	Script::PushAny(m_pLuaVm, filePath);
}

void FunctionInvoker::PushObject(const SharedPtr<VmObject>& p)
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

void FunctionInvoker::PushAny(const Reflection::Any& any)
{
	SEOUL_ASSERT(m_pLuaVm.IsValid());

	Script::PushAny(m_pLuaVm, any);
}

Bool FunctionInvoker::PushDataNode(
	const DataStore& dataStore,
	const DataNode& dataNode,
	Bool bConvertNilToEmptyTable /* = false*/,
	Bool bPrefetchAssets /* = false*/)
{
	return Script::PushDataNode(m_pLuaVm, dataStore, dataNode, bConvertNilToEmptyTable, bPrefetchAssets);
}

Bool FunctionInvoker::PushUserDataType(const Reflection::Type& type)
{
	SEOUL_ASSERT(m_pLuaVm.IsValid());

	GetScriptVm(m_pLuaVm)->InsideLockBindType(type, false);
	LuaGetMetatable(m_pLuaVm, type, false);
	return true;
}

Bool FunctionInvoker::InternalGetEnum(
	Int32 i,
	Int32& ri,
	const Reflection::Type& type) const
{
	Int32 const iLuaIndex = (GetReturnLuaIndex(i));
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

void FunctionInvoker::InternalGetTableAsComplex(Int32 i, const Reflection::WeakAny& objectPtr) const
{
	objectPtr.GetType().FromScript(m_pLuaVm, GetReturnLuaIndex(i), objectPtr);
}

void FunctionInvoker::InternalPushAnyAsTable(const Reflection::WeakAny& objectPtr)
{
	objectPtr.GetType().ToScript(m_pLuaVm, objectPtr);
}

void* FunctionInvoker::InternalPushUserData(const Reflection::Type& type)
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

		GetScriptVm(m_pLuaVm)->InsideLockBindType(type, false);
		LuaGetMetatable(m_pLuaVm, type, false);
		lua_setmetatable(m_pLuaVm, -2);
	}

	return pInstance;
}

CheckedPtr<Vm> FunctionInvoker::AcquireVm(Vm& rVm)
{
	// Mark exclusive access.
	rVm.m_Mutex.Lock();
	return &rVm;
}

CheckedPtr<Vm> FunctionInvoker::AcquireVm(const SharedPtr<VmObject>& pObject)
{
	if (!pObject.IsValid())
	{
		return nullptr;
	}

	auto pVm(GetPtr(pObject->GetVm()));
	if (pVm.IsValid())
	{
		pVm->m_Mutex.Lock();
		return pVm;
	}
	else
	{
		return nullptr;
	}
}

} // namespace Seoul::Script
