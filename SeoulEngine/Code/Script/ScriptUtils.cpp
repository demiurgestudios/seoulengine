/**
 * \file ScriptUtils.cpp
 * \brief Collection of miscellaneous global functions and utilities per
 * the integration of Lua into SeoulEngine.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "CrashManager.h"
#include "FxManager.h"
#include "Logger.h"
#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionDefine.h"
#include "ScriptFunctionInterface.h"
#include "ScriptLua.h"
#include "ScriptUtils.h"
#include "ScriptVm.h"

extern "C"
{

static int LuaGetErrorString(lua_State* pLuaVm)
{
	if (!lua_isstring(pLuaVm, 1))
	{
		if (lua_isnoneornil(pLuaVm, 1) || !luaL_callmeta(pLuaVm, 1, "__tostring") || !lua_isstring(pLuaVm, -1))
		{
			// Return what we're left with on the stack.
			return 1;
		}

		// Replace the object by result of __tostring.
		lua_remove(pLuaVm, 1);
	}

	return 1;
}

/**
 * Custom lua_pcall() error handler, expands returned string with a traceback,
 * also handles invocation of a user specified error handler.
 */
int LuaErrorHandler(lua_State* pLuaVm)
{
	// Set a max level for extremely large stacks.
	static const int kiMaxStackLevel = 32;

	using namespace Seoul;

	SEOUL_SCRIPT_CHECK_VM_STACK(pLuaVm, 1);

	// Push the debug traceback onto the stack and invoke it.
	lua_getglobal(pLuaVm, "debug");
	lua_getfield(pLuaVm, -1, "traceback");
	lua_remove(pLuaVm, -2);

	// Push the message argument to the error handler as the
	// first argument to the debug traceback.
	lua_pushvalue(pLuaVm, 1);
	Bool const bSuccess = (0 == lua_pcall(pLuaVm, 1, 1, 0));

	// Cache, used in either success or failure.
	Script::Vm* pVm = Script::GetScriptVm(pLuaVm);
	CustomCrashErrorState state;

	// If the debug traceback fails, just return the message string
	// unmodified.
	if (!bSuccess)
	{
		// Pop the error message.
		lua_pop(pLuaVm, 1);

		// Push the error handler message as the return value.
		lua_pushvalue(pLuaVm, 1);
	}

	// Get the original error message as the "reason" string.
	{
		// Push the function.
		lua_pushcfunction(pLuaVm, LuaGetErrorString);
		// Push the message object.
		lua_pushvalue(pLuaVm, 1);

		Bool const bGotString = (0 == lua_pcall(pLuaVm, 1, 1, 0));
		if (bGotString)
		{
			size_t zSizeInBytes = 0;
			Byte const* sReason = lua_tolstring(pLuaVm, -1, &zSizeInBytes);
			if (nullptr != sReason)
			{
				state.m_sReason.Assign(sReason, (UInt32)zSizeInBytes);
			}

			// Now pop the reason string off the stack.
			// We want to leave the results from above as the return value.
			lua_pop(pLuaVm, 1);
		}
	}

	// Populate stack information.
	{
		// Initialize the activation record.
		lua_Debug ar;
		memset(&ar, 0, sizeof(ar));

		// Iterate over all levels of the stack - lua_getstack() returns
		// 0 on error/end of stack.
		int iLevel = 0;
		while (0 != lua_getstack(pLuaVm, iLevel++, &ar))
		{
			// Early out if we've hit the max stack level.
			if (iLevel > kiMaxStackLevel)
			{
				break;
			}

			// Process this stack entry if we successfully get info about it.
			// The string "nSl" is:
			// - n: fills in name and namewhat
			// - S: fills in source, short_src, linedefined, lastlinedefined, and what
			// - l: fills in currentline.
			if (0 != lua_getinfo(pLuaVm, "nSl", &ar))
			{
				// Populate the frame entry.
				CustomCrashErrorState::Frame frame;

				// "Lua" - lua function.
				if (0 == strcmp(ar.what, "Lua"))
				{
					frame.m_iLine = ar.currentline;
					frame.m_sFilename = String(ar.short_src).ReplaceAll("\\", "/");

					// If all parts are available, include "namewhat" as a "namespace",
					// otherwise just use name.
					if (ar.namewhat &&
						ar.namewhat[0] &&
						ar.name &&
						ar.name[0])
					{
						frame.m_sFunction = String::Printf("%s.%s", ar.namewhat, ar.name);
					}
					else
					{
						frame.m_sFunction = ar.name;
					}
				}
				// "main" - the main file closure.
				else if (0 == strcmp(ar.what, "main"))
				{
					frame.m_iLine = ar.currentline;
					frame.m_sFilename = String(ar.short_src).ReplaceAll("\\", "/");
					frame.m_sFunction = "main.Invoke";
				}
				// Other - often "C", but a few other possibilities. In all cases, we
				// assume line and file info is not useful and only include the function name.
				else
				{
					// If all parts are available, include "namewhat" as a "namespace",
					// otherwise just use name.
					if (ar.namewhat &&
						ar.namewhat[0] &&
						ar.name &&
						ar.name[0])
					{
						frame.m_sFunction = String::Printf("%s.%s", ar.namewhat, ar.name);
					}
					else
					{
						frame.m_sFunction = ar.name;
					}
				}

				// Give frames without a function name "anonymous.Invoke".
				if (frame.m_sFunction.IsEmpty())
				{
					frame.m_sFunction = "anonymous.Invoke";
				}

				// Add this stack frame.
				state.m_vStack.PushBack(frame);
			}

			// Clear the activation record before getting the next
			// stack frame.
			memset(&ar, 0, sizeof(ar));
		}
	}

	// Report the error, unless the VM has been interrupted.
	if (!pVm->Interrupted())
	{
		pVm->GetSettings().m_ErrorHandler(state);
	}
	return 1;
}

} // extern "C"

namespace Seoul
{

namespace Script
{

#if defined(_MSC_VER)
static const Int64 kiLargestPossibleLuaInteger = 9007199254740992i64;
#else
static const Int64 kiLargestPossibleLuaInteger = 9007199254740992LL;
#endif

#if !SEOUL_SHIP
ScopedVmChecker::ScopedVmChecker(CheckedPtr<lua_State> pLuaVm, Int iTopDelta /*= 0*/)
	: m_pLuaVm(pLuaVm)
	, m_iTop(lua_gettop(m_pLuaVm))
	, m_iTopDelta(iTopDelta)
{
}

ScopedVmChecker::~ScopedVmChecker()
{
	SEOUL_ASSERT(m_iTop + m_iTopDelta == lua_gettop(m_pLuaVm));
}
#endif // /#if !SEOUL_SHIP

Vm* GetScriptVm(lua_State* pLuaVm)
{
	SEOUL_SCRIPT_CHECK_VM_STACK(pLuaVm);

	lua_pushlightuserdata(pLuaVm, kpScriptVmKey);
	lua_rawget(pLuaVm, LUA_REGISTRYINDEX);

	Vm* pReturn = (Vm*)lua_touserdata(pLuaVm, -1);

	lua_pop(pLuaVm, 1);

	return pReturn;
}

Bool GetUserData(lua_State* pLuaVm, Int32 iIndex, Reflection::WeakAny& rUserData)
{
	SEOUL_SCRIPT_CHECK_VM_STACK(pLuaVm);

	if (0 == lua_isuserdata(pLuaVm, iIndex))
	{
		return false;
	}

	if (0 == lua_getmetatable(pLuaVm, iIndex))
	{
		return false;
	}

	lua_pushlightuserdata(pLuaVm, kpScriptTypeKey);
	lua_rawget(pLuaVm, -2);
	void* pScriptTypeUserData = lua_touserdata(pLuaVm, -1);
	if (nullptr == pScriptTypeUserData)
	{
		// nil and metatable.
		lua_pop(pLuaVm, 2);
		return false;
	}

	// pop the Script::TypeUserData entry and the metatable, now that we
	// have a pointer to the userdata.
	lua_pop(pLuaVm, 2);

	TypeUserData const& luaScriptTypeUserData = *((TypeUserData*)pScriptTypeUserData);
	void* pSeoulEngineUserData = lua_touserdata(pLuaVm, iIndex);
	if (nullptr == pSeoulEngineUserData)
	{
		return false;
	}

	// Weak entries, the lua userdata is actually a pointer to the SeoulEngine object.
	if (luaScriptTypeUserData.m_bWeak)
	{
		rUserData = luaScriptTypeUserData.m_Type.GetPtrUnsafe(*((void**)pSeoulEngineUserData));
	}
	// Strong entries, the user data is the SeoulEngine object itself.
	else
	{
		rUserData = luaScriptTypeUserData.m_Type.GetPtrUnsafe(pSeoulEngineUserData);
	}

	return true;
}

static Bool InnerScriptPushDataNode(
	lua_State* pLuaVm,
	const DataStore& dataStore,
	const DataNode& dataNode,
	Bool bConvertNilToEmptyTable,
	Bool bPrefetchAssets)
{
	SEOUL_SCRIPT_CHECK_VM_STACK(pLuaVm);

	if (dataNode.IsArray())
	{
		UInt32 uArrayCount = 0;
		SEOUL_VERIFY(dataStore.GetArrayCount(dataNode, uArrayCount));

		for (UInt32 i = 0; i < uArrayCount; ++i)
		{
			int const iLuaArrayIndex = (int)(i + 1);
			DataNode valueNode;
			SEOUL_VERIFY(dataStore.GetValueFromArray(dataNode, i, valueNode));

			switch (valueNode.GetType())
			{
			case DataNode::kArray: // fall-through.
			case DataNode::kTable:
				lua_newtable(pLuaVm);
				lua_rawseti(pLuaVm, -2, iLuaArrayIndex);
				lua_rawgeti(pLuaVm, -1, iLuaArrayIndex);
				if (!InnerScriptPushDataNode(pLuaVm, dataStore, valueNode, bConvertNilToEmptyTable, bPrefetchAssets))
				{
					// Pop the table and the field.
					lua_pop(pLuaVm, 2);
					return false;
				}

				// Pop the field, leave the table for return.
				lua_pop(pLuaVm, 1);
				break;

			case DataNode::kBoolean:
				lua_pushboolean(pLuaVm, dataStore.AssumeBoolean(valueNode) ? 1 : 0);
				lua_rawseti(pLuaVm, -2, iLuaArrayIndex);
				break;

			case DataNode::kFilePath:
				{
					FilePath filePath;
					SEOUL_VERIFY(dataStore.AsFilePath(valueNode, filePath));

					// TODO: Eliminate this boilerplate.

					void* pFilePath = lua_newuserdata(pLuaVm, sizeof(filePath));
					new (pFilePath) FilePath(filePath);
					LuaGetMetatable(pLuaVm, TypeOf<FilePath>(), false);
					lua_setmetatable(pLuaVm, -2);
					lua_rawseti(pLuaVm, -2, iLuaArrayIndex);
				}
				break;

			case DataNode::kFloat31:
				lua_pushnumber(pLuaVm, (lua_Number)dataStore.AssumeFloat31(valueNode));
				lua_rawseti(pLuaVm, -2, iLuaArrayIndex);
				break;

			case DataNode::kFloat32:
				lua_pushnumber(pLuaVm, (lua_Number)dataStore.AssumeFloat32(valueNode));
				lua_rawseti(pLuaVm, -2, iLuaArrayIndex);
				break;

			case DataNode::kInt32Big:
				lua_pushnumber(pLuaVm, (lua_Number)dataStore.AssumeInt32Big(valueNode));
				lua_rawseti(pLuaVm, -2, iLuaArrayIndex);
				break;

			case DataNode::kInt32Small:
				lua_pushnumber(pLuaVm, (lua_Number)dataStore.AssumeInt32Small(valueNode));
				lua_rawseti(pLuaVm, -2, iLuaArrayIndex);
				break;

			case DataNode::kInt64:
				{
					Int64 const iValue = dataStore.AssumeInt64(valueNode);
					if (iValue > kiLargestPossibleLuaInteger)
					{
						// TODO: Add support for a BigInt type?
						return false;
					}

					lua_pushnumber(pLuaVm, (lua_Number)iValue);
					lua_rawseti(pLuaVm, -2, iLuaArrayIndex);
				}
				break;

			case DataNode::kNull:
				if (bConvertNilToEmptyTable)
				{
					lua_createtable(pLuaVm, 0, 0);
				}
				else
				{
					lua_pushnil(pLuaVm);
				}
				lua_rawseti(pLuaVm, -2, iLuaArrayIndex);
				break;

			case DataNode::kString:
				{
					Byte const* s = nullptr;
					UInt32 zSizeInBytes = 0;
					SEOUL_VERIFY(dataStore.AsString(valueNode, s, zSizeInBytes));
					lua_pushlstring(pLuaVm, s, zSizeInBytes);
					lua_rawseti(pLuaVm, -2, iLuaArrayIndex);
				}
				break;

			case DataNode::kUInt32:
				lua_pushnumber(pLuaVm, (lua_Number)dataStore.AssumeUInt32(valueNode));
				lua_rawseti(pLuaVm, -2, iLuaArrayIndex);
				break;

			case DataNode::kUInt64:
				// TODO: Add support for a BigInt type?
				return false;

			default:
				SEOUL_FAIL("Out-of-sync enum.");
				return false;
			};
		}
	}
	else if (dataNode.IsTable())
	{
		auto const iBegin = dataStore.TableBegin(dataNode);
		auto const iEnd = dataStore.TableEnd(dataNode);
		for (auto i = iBegin; iEnd != i; ++i)
		{
			HString const key = i->First;
			DataNode const valueNode = i->Second;
			switch (valueNode.GetType())
			{
			case DataNode::kArray: // fall-through.
			case DataNode::kTable:
				lua_newtable(pLuaVm);
				lua_setfield(pLuaVm, -2, key.CStr());
				lua_getfield(pLuaVm, -1, key.CStr());
				if (!InnerScriptPushDataNode(pLuaVm, dataStore, valueNode, bConvertNilToEmptyTable, bPrefetchAssets))
				{
					// Pop the table and the field.
					lua_pop(pLuaVm, 2);
					return false;
				}

				// Pop the field, leave the table for return.
				lua_pop(pLuaVm, 1);
				break;

			case DataNode::kBoolean:
				lua_pushboolean(pLuaVm, dataStore.AssumeBoolean(valueNode) ? 1 : 0);
				lua_setfield(pLuaVm, -2, key.CStr());
				break;

			case DataNode::kFilePath:
				{
					FilePath filePath;
					SEOUL_VERIFY(dataStore.AsFilePath(valueNode, filePath));

					void* pFilePath = lua_newuserdata(pLuaVm, sizeof(filePath));
					new (pFilePath) FilePath(filePath);
					LuaGetMetatable(pLuaVm, TypeOf<FilePath>(), false);
					lua_setmetatable(pLuaVm, -2);
					lua_setfield(pLuaVm, -2, key.CStr());
				}
				break;

			case DataNode::kFloat31:
				lua_pushnumber(pLuaVm, (lua_Number)dataStore.AssumeFloat31(valueNode));
				lua_setfield(pLuaVm, -2, key.CStr());
				break;

			case DataNode::kFloat32:
				lua_pushnumber(pLuaVm, (lua_Number)dataStore.AssumeFloat32(valueNode));
				lua_setfield(pLuaVm, -2, key.CStr());
				break;

			case DataNode::kInt32Big:
				lua_pushnumber(pLuaVm, (lua_Number)dataStore.AssumeInt32Big(valueNode));
				lua_setfield(pLuaVm, -2, key.CStr());
				break;

			case DataNode::kInt32Small:
				lua_pushnumber(pLuaVm, (lua_Number)dataStore.AssumeInt32Small(valueNode));
				lua_setfield(pLuaVm, -2, key.CStr());
				break;

			case DataNode::kInt64:
				{
					Int64 const iValue = dataStore.AssumeInt64(valueNode);
					if (iValue > kiLargestPossibleLuaInteger)
					{
						// TODO: Add support for a BigInt type?
						return false;
					}

					lua_pushnumber(pLuaVm, (lua_Number)iValue);
					lua_setfield(pLuaVm, -2, key.CStr());
				}
				break;

			case DataNode::kNull:
				if (bConvertNilToEmptyTable)
				{
					lua_createtable(pLuaVm, 0, 0);
				}
				else
				{
					lua_pushnil(pLuaVm);
				}
				lua_setfield(pLuaVm, -2, key.CStr());
				break;

			case DataNode::kString:
				{
					Byte const* s = nullptr;
					UInt32 zSizeInBytes = 0;
					SEOUL_VERIFY(dataStore.AsString(valueNode, s, zSizeInBytes));
					lua_pushlstring(pLuaVm, s, zSizeInBytes);
					lua_setfield(pLuaVm, -2, key.CStr());
				}
				break;

			case DataNode::kUInt32:
				lua_pushnumber(pLuaVm, (lua_Number)dataStore.AssumeUInt32(valueNode));
				lua_setfield(pLuaVm, -2, key.CStr());
				break;

			case DataNode::kUInt64:
				// TODO: Add support for a BigInt type?
				return false;

			default:
				SEOUL_FAIL("Out-of-sync enum.");
				return false;
			};
		}
	}

	return true;
}

void PushAny(lua_State* pLuaVm, const Reflection::Any& any)
{
	any.GetType().ToScript(pLuaVm, any.GetPointerToObject());
}

void PushClone(lua_State* pToVm, lua_State* pFromVm, Int iFromIndex /*= -1*/)
{
	SEOUL_SCRIPT_CHECK_VM_STACK(pToVm, 1);
	SEOUL_SCRIPT_CHECK_VM_STACK(pFromVm);

	auto const iType = lua_type(pFromVm, iFromIndex);
	switch (iType)
	{
	case LUA_TNIL: lua_pushnil(pToVm); break;
	case LUA_TBOOLEAN:
	{
		auto const iBoolean = lua_toboolean(pFromVm, iFromIndex);
		lua_pushboolean(pToVm, iBoolean);
	}
	break;
	case LUA_TFUNCTION: // fall-through
	case LUA_TTHREAD:
		lua_pushnil(pToVm); // TODO: Push a placeholder or nothing instead?
		break;
	case LUA_TLIGHTUSERDATA:
	{
		auto const pUserData = lua_touserdata(pFromVm, iFromIndex);
		lua_pushlightuserdata(pToVm, pUserData);
	}
	break;
	case LUA_TNUMBER:
	{
		auto const fNumber = lua_tonumber(pFromVm, iFromIndex);
		lua_pushnumber(pToVm, fNumber);
	}
	break;
	case LUA_TSTRING:
	{
		size_t zSize = 0;
		auto const sString = lua_tolstring(pFromVm, iFromIndex, &zSize);
		lua_pushlstring(pToVm, sString, zSize);
	}
	break;
	case LUA_TTABLE:
	{
		// Populate to with a table.
		lua_newtable(pToVm);

		Int const iIndex = (iFromIndex < 0 ? iFromIndex - 1 : iFromIndex);
		lua_pushnil(pFromVm);
		while (0 != lua_next(pFromVm, iIndex))
		{
			PushClone(pToVm, pFromVm, -2);
			PushClone(pToVm, pFromVm, -1);

			lua_rawset(pToVm, -3);

			// Remove the value from the from stack, in
			// accordance with the semantics
			// of lua_next.
			lua_pop(pFromVm, 1);
		}
	}
	break;
	case LUA_TUSERDATA:
	{
		Reflection::WeakAny weakAny;
		if (GetUserData(pFromVm, iFromIndex, weakAny) &&
			weakAny.IsOfType<FilePath*>())
		{
			auto const fromFilePath = *weakAny.Cast<FilePath*>();

			// TODO: Eliminate this boilerplate.

			void* pFilePath = lua_newuserdata(pToVm, sizeof(fromFilePath));
			new (pFilePath) FilePath(fromFilePath);
			LuaGetMetatable(pToVm, TypeOf<FilePath>(), false);
			lua_setmetatable(pToVm, -2);
		}
		else
		{
			lua_pushnil(pToVm); // TODO: Push a placeholder or nothing instead?
		}
	}
	break;
	default:
		lua_pushnil(pToVm); // TODO: Push a placeholder or nothing instead?
		SEOUL_FAIL("Out-of-sync enum.");
		break;
	};
}

Bool PushDataNode(
	lua_State* pLuaVm,
	const DataStore& dataStore,
	const DataNode& dataNode,
	Bool bConvertNilToEmptyTable,
	Bool bPrefetchAssets)
{
	// Commit the DataStore.
	switch (dataNode.GetType())
	{
	case DataNode::kArray:
	case DataNode::kTable:
		lua_newtable(pLuaVm);
		if (!InnerScriptPushDataNode(pLuaVm, dataStore, dataNode, bConvertNilToEmptyTable, bPrefetchAssets))
		{
			// Done with the table.
			lua_pop(pLuaVm, 1);

			return false;
		}
		return true;

	case DataNode::kBoolean:
		lua_pushboolean(pLuaVm, dataStore.AssumeBoolean(dataNode) ? 1 : 0);
		return true;

	case DataNode::kFilePath:
	{
		FilePath filePath;
		SEOUL_VERIFY(dataStore.AsFilePath(dataNode, filePath));

		void* pFilePath = lua_newuserdata(pLuaVm, sizeof(filePath));
		new (pFilePath) FilePath(filePath);
		LuaGetMetatable(pLuaVm, TypeOf<FilePath>(), false);
		lua_setmetatable(pLuaVm, -2);
		return true;
	}

	case DataNode::kFloat31:
		lua_pushnumber(pLuaVm, (lua_Number)dataStore.AssumeFloat31(dataNode));
		return true;

	case DataNode::kFloat32:
		lua_pushnumber(pLuaVm, (lua_Number)dataStore.AssumeFloat32(dataNode));
		return true;

	case DataNode::kInt32Big:
		lua_pushnumber(pLuaVm, (lua_Number)dataStore.AssumeInt32Big(dataNode));
		return true;

	case DataNode::kInt32Small:
		lua_pushnumber(pLuaVm, (lua_Number)dataStore.AssumeInt32Small(dataNode));
		return true;

	case DataNode::kInt64:
	{
		Int64 const iValue = dataStore.AssumeInt64(dataNode);
		if (iValue > kiLargestPossibleLuaInteger)
		{
			// TODO: Add support for a BigInt type?
			return false;
		}

		lua_pushnumber(pLuaVm, (lua_Number)iValue);
		return true;
	}

	case DataNode::kNull:
		if (bConvertNilToEmptyTable)
		{
			lua_createtable(pLuaVm, 0, 0);
		}
		else
		{
			lua_pushnil(pLuaVm);
		}
		return true;

	case DataNode::kString:
	{
		Byte const* s = nullptr;
		UInt32 zSizeInBytes = 0;
		SEOUL_VERIFY(dataStore.AsString(dataNode, s, zSizeInBytes));
		lua_pushlstring(pLuaVm, s, zSizeInBytes);
		return true;
	}

	case DataNode::kUInt32:
		lua_pushnumber(pLuaVm, (lua_Number)dataStore.AssumeUInt32(dataNode));
		return true;

	case DataNode::kUInt64:
		// TODO: Add support for a BigInt type?
		return false;

	default:
		return false;
	};
}

/**
 * Simple utility, returns true if a lua table can be considered an array for purposes
 * conversion to a DataStore.
 *
 * \pre Lua object at iIndex must be a lua table.
 */
static inline Bool IsEffectivelyArray(lua_State* pLuaVm, Int32 iIndex)
{
	SEOUL_SCRIPT_CHECK_VM_STACK(pLuaVm);

	// Sanity check.
	SEOUL_ASSERT(0 != lua_istable(pLuaVm, iIndex));

	// TODO: Not a great metric, but likely good enough for our needs.
	// If t[1] is not nil, assume the table is an array. Otherwise, assume
	// it is a table.
	lua_rawgeti(pLuaVm, iIndex, 1);
	Bool const bIsNil = lua_isnil(pLuaVm, -1);
	lua_pop(pLuaVm, 1);

	// If IsNil, table is a table.
	return !bIsNil;
}

/** Utility, copy a Lua table into dataNode, which must be an array or table. */
static Bool TableToDataNode(
	lua_State* pLuaVm,
	DataStore& rDataStore,
	const DataNode& dataNode)
{
	SEOUL_SCRIPT_CHECK_VM_STACK(pLuaVm);

	// Filling array.
	if (dataNode.IsArray())
	{
		lua_pushnil(pLuaVm);
		while (0 != lua_next(pLuaVm, -2))
		{
			// Index.
			lua_Integer const iArrayIndex = lua_tointeger(pLuaVm, -2);

			// Skip if not a member of the array.
			if (iArrayIndex <= 0)
			{
				// Remove the value from the stack.
				lua_pop(pLuaVm, 1);
				continue;
			}

			// DataStore index.
			UInt32 const uIndex = (UInt32)(iArrayIndex - 1);

			// Value.
			Bool bSuccess = false;
			switch (lua_type(pLuaVm, -1))
			{
			case LUA_TBOOLEAN:
				bSuccess = rDataStore.SetBooleanValueToArray(dataNode, uIndex, (0 != lua_toboolean(pLuaVm, -1)));
				break;
			case LUA_TNIL:
				bSuccess = rDataStore.SetNullValueToArray(dataNode, uIndex);
				break;
			case LUA_TNUMBER:
				{
					lua_Number const fNumber = lua_tonumber(pLuaVm, -1);
					Int64 const i64Number = (Int64)fNumber;
					if ((lua_Number)i64Number == fNumber)
					{
						bSuccess = rDataStore.SetInt64ValueToArray(dataNode, uIndex, i64Number);
					}
					else
					{
						bSuccess = rDataStore.SetFloat32ValueToArray(dataNode, uIndex, (Float32)fNumber);
					}
				}
				break;
			case LUA_TSTRING:
				{
					size_t zSizeInBytes = 0u;
					Byte const* s = lua_tolstring(pLuaVm, -1, &zSizeInBytes);
					bSuccess = rDataStore.SetStringToArray(dataNode, uIndex, s, (UInt32)zSizeInBytes);
				}
				break;
			case LUA_TTABLE:
				{
					// Push an array or table as appropriate.
					if (IsEffectivelyArray(pLuaVm, -1))
					{
						bSuccess = rDataStore.SetArrayToArray(dataNode, uIndex);
					}
					else
					{
						bSuccess = rDataStore.SetTableToArray(dataNode, uIndex);
					}

					DataNode subNode;
					bSuccess = bSuccess && rDataStore.GetValueFromArray(dataNode, uIndex, subNode);
					bSuccess = bSuccess && TableToDataNode(
						pLuaVm,
						rDataStore,
						subNode);
				}
				break;
			case LUA_TUSERDATA:
				{
					Reflection::WeakAny weakAny;
					if (!GetUserData(pLuaVm, -1, weakAny))
					{
						bSuccess = false;
					}

					// TODO: Eliminate the need for special cases like this.

					if (weakAny.IsOfType<FilePath*>())
					{
						bSuccess = rDataStore.SetFilePathToArray(dataNode, uIndex, *weakAny.Cast<FilePath*>());
					}
					else
					{
						bSuccess = rDataStore.SetTableToArray(dataNode, uIndex);

						DataNode subNode;
						bSuccess = bSuccess && rDataStore.GetValueFromArray(dataNode, uIndex, subNode);

						Reflection::DefaultSerializeContext context(ContentKey(), rDataStore, dataNode, weakAny.GetTypeInfo());
						bSuccess = bSuccess && weakAny.GetType().TrySerializeToArray(
							context,
							rDataStore,
							subNode,
							uIndex,
							weakAny);
					}
				}
				break;
			default:
				// TODO: Should we silently skip unsupported values like this,
				// or return false so it yells about it?
				break;
			};

			// Remove the value from the stack.
			lua_pop(pLuaVm, 1);

			// Done with failure if not successful.
			if (!bSuccess)
			{
				// Remove the marker prior to return.
				lua_pop(pLuaVm, 1);
				return false;
			}
		}

		return true;
	}
	// Else, filling table.
	else
	{
		lua_pushnil(pLuaVm);
		while (0 != lua_next(pLuaVm, -2))
		{
			// Key.
			size_t zSizeInBytes = 0u;
			Byte const* sKey = lua_tolstring(pLuaVm, -2, &zSizeInBytes);

			// Skip if not a string key.
			if (nullptr == sKey)
			{
				// Remove the value from the stack.
				lua_pop(pLuaVm, 1);
				continue;
			}

			// DataStore key.
			HString const key(sKey, (UInt32)zSizeInBytes);

			// Value.
			Bool bSuccess = false;
			switch (lua_type(pLuaVm, -1))
			{
			case LUA_TBOOLEAN:
				bSuccess = rDataStore.SetBooleanValueToTable(dataNode, key, (0 != lua_toboolean(pLuaVm, -1)));
				break;
			case LUA_TNIL:
				bSuccess = rDataStore.SetNullValueToTable(dataNode, key);
				break;
			case LUA_TNUMBER:
				{
					lua_Number const fNumber = lua_tonumber(pLuaVm, -1);
					Int64 const i64Number = (Int64)fNumber;
					if ((lua_Number)i64Number == fNumber)
					{
						bSuccess = rDataStore.SetInt64ValueToTable(dataNode, key, i64Number);
					}
					else
					{
						bSuccess = rDataStore.SetFloat32ValueToTable(dataNode, key, (Float32)fNumber);
					}
				}
				break;
			case LUA_TSTRING:
				{
					size_t zSizeInBytes = 0u;
					Byte const* s = lua_tolstring(pLuaVm, -1, &zSizeInBytes);
					bSuccess = rDataStore.SetStringToTable(dataNode, key, s, (UInt32)zSizeInBytes);
				}
				break;
			case LUA_TTABLE:
				{
					// Push an array or table as appropriate.
					if (IsEffectivelyArray(pLuaVm, -1))
					{
						bSuccess = rDataStore.SetArrayToTable(dataNode, key);
					}
					else
					{
						bSuccess = rDataStore.SetTableToTable(dataNode, key);
					}

					DataNode subNode;
					bSuccess = bSuccess && rDataStore.GetValueFromTable(dataNode, key, subNode);
					bSuccess = bSuccess && TableToDataNode(
						pLuaVm,
						rDataStore,
						subNode);
				}
				break;
			case LUA_TUSERDATA:
				{
					Reflection::WeakAny weakAny;
					if (!GetUserData(pLuaVm, -1, weakAny))
					{
						bSuccess = false;
					}

					// TODO: Eliminate the need for special cases like this.

					if (weakAny.IsOfType<FilePath*>())
					{
						bSuccess = rDataStore.SetFilePathToTable(dataNode, key, *weakAny.Cast<FilePath*>());
					}
					else
					{
						bSuccess = rDataStore.SetTableToTable(dataNode, key);

						DataNode subNode;
						bSuccess = bSuccess && rDataStore.GetValueFromTable(dataNode, key, subNode);

						Reflection::DefaultSerializeContext context(ContentKey(), rDataStore, dataNode, weakAny.GetTypeInfo());
						bSuccess = bSuccess && weakAny.GetType().TrySerializeToTable(
							context,
							rDataStore,
							subNode,
							key,
							weakAny);
					}
				}
				break;
			default:
				// TODO: Should we silently skip unsupported values like this,
				// or return false so it yells about it?
				break;
			};

			// Remove the value from the stack.
			lua_pop(pLuaVm, 1);

			// Done with failure if not successful.
			if (!bSuccess)
			{
				// Remove the marker prior to return.
				lua_pop(pLuaVm, 1);
				return false;
			}
		}

		return true;
	}
}

/**
 * Utility, convert the lua table at iIndex into a DataStore,
 * either an array or table based on a simple heuristic.
 */
Bool TableToDataStore(
	lua_State* pLuaVm,
	Int32 iIndex,
	DataStore& rDataStore)
{
	SEOUL_SCRIPT_CHECK_VM_STACK(pLuaVm);

	// Only for tables.
	if (lua_istable(pLuaVm, iIndex))
	{
		// Make table or array.
		if (IsEffectivelyArray(pLuaVm, iIndex))
		{
			// Array.
			rDataStore.MakeArray();
		}
		// Else, table.
		else
		{
			rDataStore.MakeTable();
		}
	}
	// Unsupported for all other types.
	else
	{
		// Unsupported type.
		return false;
	}

	// Push the table onto the stack.
	lua_pushvalue(pLuaVm, iIndex);
	Bool const bResult = TableToDataNode(pLuaVm, rDataStore, rDataStore.GetRootNode());
	lua_pop(pLuaVm, 1);
	return bResult;
}

Bool ToAny(
	lua_State* pLuaVm,
	Int32 iIndex,
	Reflection::Any& rAny,
	const Reflection::TypeInfo& targetTypeInfo /* = TypeId<void>() */)
{
	using namespace Reflection;

	if (!targetTypeInfo.IsVoid())
	{
		// Special handling for cstrings.
		if (SEOUL_UNLIKELY(targetTypeInfo == TypeId<Byte const*>()))
		{
			rAny = Any(lua_tostring(pLuaVm, iIndex));
			return true;
		}
		// Special handling for light user data.
		else if (SEOUL_UNLIKELY(targetTypeInfo == TypeId<void*>()))
		{
			rAny = Any(lua_touserdata(pLuaVm, iIndex));
			return true;
		}
		else
		{
			auto const& type = targetTypeInfo.GetType();
			type.DefaultCopy(rAny);
			type.FromScript(pLuaVm, iIndex, rAny.GetPointerToObject());
			return true;
		}
	}
	else
	{
		SEOUL_SCRIPT_CHECK_VM_STACK(pLuaVm);

		Int32 const eType = (Int32)lua_type(pLuaVm, iIndex);
		switch (eType)
		{
		case LUA_TBOOLEAN:
			rAny = Any((0 != lua_toboolean(pLuaVm, iIndex)));
			return true;
		case LUA_TLIGHTUSERDATA:
			rAny = Any(lua_touserdata(pLuaVm, iIndex));
			return true;
		case LUA_TNIL:
			rAny.Reset();
			return true;
		case LUA_TNUMBER:
			rAny = Any(lua_tonumber(pLuaVm, iIndex));
			return true;
		case LUA_TSTRING:
			rAny = Any(lua_tostring(pLuaVm, iIndex));
			return true;
		case LUA_TUSERDATA:
			{
				Reflection::WeakAny weakAny;
				if (!GetUserData(pLuaVm, iIndex, weakAny))
				{
					return false;
				}

				if (!weakAny.IsOfType<FilePath*>())
				{
					SEOUL_WARN("UserData of type '%s' cannot be converted to Any. "
						"You are likely seeing this warning due to a UserData passed to "
						"a native method bound with Reflection binding, or an invocation "
						"of BroadcastEvent. UserData are not supported to BroadcastEvent. "
						"Update the Reflection binding to use a (Script::FunctionInterface*) "
						"signature.", weakAny.GetType().GetName().CStr());
					return false;
				}

				rAny = *weakAny.Cast<FilePath*>();
			}
			return true;

		case LUA_TFUNCTION: // fall-through
		case LUA_TTABLE: // fall-through
			 // If a Lua function, or if the target type is void, use a Script::VmObject.
			if (LUA_TFUNCTION == eType ||
				targetTypeInfo.IsVoid() ||
				targetTypeInfo == TypeId< SharedPtr<VmObject> >())
			{
				lua_pushvalue(pLuaVm, iIndex);
				Int32 const iObject = lua_ref(pLuaVm, LUA_REGISTRYINDEX);
				SharedPtr<VmObject> pObject(SEOUL_NEW(MemoryBudgets::Scripting) VmObject(
					GetScriptVm(pLuaVm)->GetHandle(),
					iObject));

				rAny = Any(pObject);
				return true;
			}
			else
			{
				// TODO: This branch should manually serialize
				// into a new instance of the type defined by
				// targetTypeInfo(), and only return false
				// if that fails.

				return false;
			}

		case LUA_TTHREAD: // fall-through
		default:
			return false;
		};
	}
}

} // namespace Script

void LuaGetMetatable(
	lua_State* pLuaVm,
	const Reflection::Type& type,
	Bool bWeak)
{
	lua_pushlightuserdata(pLuaVm, Script::LuaGetMetatableKey(type, bWeak));
	lua_rawget(pLuaVm, LUA_REGISTRYINDEX);
}

namespace Script
{

static Int GetCommandLineArgProperty(lua_State* pLuaVm)
{
	auto const pProperty = (Reflection::Property const*)lua_touserdata(pLuaVm, lua_upvalueindex(1));

	Reflection::Any any;
	if (pProperty->TryGet(Reflection::WeakAny(), any))
	{
		FunctionInterface interf(pLuaVm);
		interf.PushReturnAny(any);
		return 1;
	}

	return 0;
}

static void BindCommandLineArgProperty(lua_State* pLuaVm, Reflection::Property const* pProperty)
{
	lua_pushlightuserdata(pLuaVm, (void*)pProperty);
	lua_pushcclosure(pLuaVm, GetCommandLineArgProperty, 1);
}

void BindMethods(lua_State* pLuaVm, const Reflection::Type& type, Bool bWeak)
{
	using namespace Reflection;

	SEOUL_SCRIPT_CHECK_VM_STACK(pLuaVm);

	// First bind parents.
	UInt32 const nParents = type.GetParentCount();
	for (UInt32 i = 0; i < nParents; ++i)
	{
		Type const* pParent = type.GetParent(i);
		BindMethods(pLuaVm, *pParent, bWeak);
	}

	// TODO: We probably want to do this generally, but for now, we only need
	// a getter for static properties tagged as CommandLineArgs.
	auto const uProps = type.GetPropertyCount();
	for (UInt32 i = 0u; i < uProps; ++i)
	{
		auto const pProperty = type.GetProperty(i);
		if (!pProperty->GetAttributes().HasAttribute<Attributes::CommandLineArg>())
		{
			continue;
		}
		if (!pProperty->IsStatic()) // Sanity, although should be enforced by CommandLineArg processing.
		{
			continue;
		}

		// Push the binder.
		BindCommandLineArgProperty(pLuaVm, pProperty);

		// Set the closure to the table that is at index -2
		// on the stack (expected to be the type table).
		lua_setfield(pLuaVm, -2, pProperty->GetName().CStr());
	}

	// Now bind members.
	UInt32 const nMethods = type.GetMethodCount();
	for (UInt32 i = 0; i < nMethods; ++i)
	{
		Method const* pMethod = type.GetMethod(i);

		// Push the method binding onto the stack.
		pMethod->ScriptBind(pLuaVm, bWeak);

		// Set the closure to the table that is at index -2
		// on the stack (expected to be the type table).
		lua_setfield(pLuaVm, -2, pMethod->GetName().CStr());
	}
}

} // namespace Script

} // namespace Seoul
