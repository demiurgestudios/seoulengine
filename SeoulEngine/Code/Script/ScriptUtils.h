/**
 * \file ScriptUtils.h
 * \brief Collection of miscellaneous global functions and utilities per
 * the integration of Lua into SeoulEngine.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCRIPT_UTILS_H
#define SCRIPT_UTILS_H

#include "CheckedPtr.h"
#include "DataStore.h"
#include "Logger.h"
#include "Prereqs.h"
#include "ReflectionTypeInfo.h"
#include "ScriptPrereqs.h"
namespace Seoul { class DataNode; }
namespace Seoul { class DataStore; }
namespace Seoul { namespace Script { class Vm; } }
namespace Seoul { namespace Script { class VmObject; } }
namespace Seoul { namespace Reflection { class Any; } }
namespace Seoul { namespace Reflection { class Method; } }
namespace Seoul { namespace Reflection { class Type; } }
namespace Seoul { namespace Reflection { class WeakAny; } }
extern "C" { struct lua_State; } // extern "C"
extern "C" { extern int LuaErrorHandler(lua_State* pLuaVm); }

namespace Seoul
{

namespace Script
{

struct TypeUserData SEOUL_SEALED
{
	TypeUserData(const Reflection::Type& type, Bool bWeak)
		: m_Type(type)
		, m_bWeak(bWeak)
	{
	}

	const Reflection::Type& m_Type;
	Bool const m_bWeak;
}; // struct Script::TypeUserData;

static void* const kpScriptVmKey = (void*)2;
static void* const kpScriptVmHotLoadDataKey = (void*)3;
static void* const kpScriptTypeKey = (void*)4;
static void* const kpScriptErrorMessageKey = (void*)5;
static void* const kpScriptWeakRegistryKey = (void*)6;

inline static void* LuaGetMetatableKey(const Reflection::Type& type, Bool bWeak)
{
	// Relies on the assertion that the lower bit of &type is always zero, due
	// to alignment. Sanity check because reasons.
	SEOUL_ASSERT(0 == (((size_t)((void*)(&type))) & 0x1));

	void* pKey = (void*)&type;
	if (bWeak)
	{
		pKey = (void*)(((size_t)pKey) | 0x1);
	}

	return pKey;
}

} // namespace Script

void LuaGetMetatable(lua_State* pLuaVm, const Reflection::Type& type, Bool bWeak);

namespace Script
{

void BindMethod(lua_State* pLuaVm, const Reflection::Method& method, Bool bWeak);
void BindMethods(lua_State* pLuaVm, const Reflection::Type& type, Bool bWeak);
Vm* GetScriptVm(lua_State* pLuaVm);
Bool GetUserData(lua_State* pLuaVm, Int32 iIndex, Reflection::WeakAny& rUserData);

inline Bool Pcall(lua_State* pLuaVm, Int32 iArguments, Int iReturnValues = -1 /* LUA_MULTRET */)
{
	// Get the position for the error handler.
	Int const iErrorHandler = lua_gettop(pLuaVm) - iArguments;

	// Push the error handler onto the stack.
	lua_pushcclosure(pLuaVm, LuaErrorHandler, 0);

	// Reposition it to before the function and arguments we're about to pcall.
	lua_insert(pLuaVm, iErrorHandler);

	// Perform the pcall.
	auto const iResult = lua_pcall(pLuaVm, iArguments, iReturnValues, iErrorHandler);
	Bool const bSuccess = (0 == iResult);

	// Remove the error handler.
	lua_remove(pLuaVm, iErrorHandler);

	// LUA_ERRRUN is the only "expected" error case - explicitly warn about the others.
#if SEOUL_LOGGING_ENABLED
	if (!bSuccess && 2/*LUA_ERRRUN*/ != iResult)
	{
		SEOUL_WARN("ScriptPcall returned unexpected error result: %d\n", iResult);
	}
#endif // /#if SEOUL_LOGGING_ENABLED

	return bSuccess;
}

void PushAny(lua_State* pLuaVm, const Reflection::Any& any);
void PushClone(lua_State* pToVm, lua_State* pFromVm, Int iFromIndex = -1);
Bool PushDataNode(lua_State* pLuaVm, const DataStore& dataStore, const DataNode& dataNode, Bool bConvertNilToEmptyTable, Bool bPrefetchAssets);
inline Bool PushDataStore(lua_State* pLuaVm, const DataStore& dataStore, Bool bConvertNilToEmptyTable, Bool bPrefetchAssets)
{
	return PushDataNode(pLuaVm, dataStore, dataStore.GetRootNode(), bConvertNilToEmptyTable, bPrefetchAssets);
}
Bool TableToDataStore(
	lua_State* pLuaVm,
	Int32 iIndex,
	DataStore& rDataStore);
Bool TableToDataStore(
	lua_State* pLuaVm,
	Int32 iIndex,
	DataStore& rDataStore,
	const DataNode& dataNode);
Bool ToAny(
	lua_State* pLuaVm,
	Int32 iIndex,
	Reflection::Any& rAny,
	const Reflection::TypeInfo& targetTypeInfo = TypeId<void>());

#if !SEOUL_SHIP
class ScopedVmChecker SEOUL_SEALED
{
public:
	ScopedVmChecker(CheckedPtr<lua_State> pLuaVm, Int iTopDelta = 0);
	~ScopedVmChecker();

private:
	CheckedPtr<lua_State> m_pLuaVm;
	Int const m_iTop;
	Int const m_iTopDelta;
}; // class Script::ScopedVmChecker

#	define SEOUL_SCRIPT_CHECK_VM_STACK(vm, ...) ::Seoul::Script::ScopedVmChecker SEOUL_MACRO_JOIN(seoul__Vm_Checker_0x58AB34712__, __LINE__)(vm, ##__VA_ARGS__)

#else // #if SEOUL_SHIP

#	define SEOUL_SCRIPT_CHECK_VM_STACK(vm, ...) ((void)0)

#endif // /#if SEOUL_SHIP

} // namespace Script

} // namespace Seoul

#endif // include guard
