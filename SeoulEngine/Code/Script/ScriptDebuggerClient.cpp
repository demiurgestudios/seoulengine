/**
 * \file ScriptDebuggerClient.cpp
 * Debug client for SlimCS, implements the protocol
 * for talking to SlimCS enabled hosts (debuggers).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FileChangeNotifier.h"
#include "FileManager.h"
#include "FromString.h"
#include "Logger.h"
#include "ReflectionDefine.h"
#include "ScriptDebuggerClient.h"
#include "ScriptLua.h"
#include "ScriptUtils.h"
#include "Thread.h"

namespace Seoul
{

// Conditional class - only define the debugger client in
// limited builds.
#if SEOUL_ENABLE_DEBUGGER_CLIENT

///////////////////////////////////////////////////////////////////////////////
// BEGIN REFLECTION HOOKS
///////////////////////////////////////////////////////////////////////////////
SEOUL_BEGIN_ENUM(Script::DebuggerClientTag)
	SEOUL_ENUM_N("Unknown", Script::DebuggerClientTag::kUnknown)
	SEOUL_ENUM_N("AskBreakpoints", Script::DebuggerClientTag::kAskBreakpoints)
	SEOUL_ENUM_N("BreakAt", Script::DebuggerClientTag::kBreakAt)
	SEOUL_ENUM_N("Frame", Script::DebuggerClientTag::kFrame)
	SEOUL_ENUM_N("GetChildren", Script::DebuggerClientTag::kGetChildren)
	SEOUL_ENUM_N("Heartbeat", Script::DebuggerClientTag::kHeartbeat)
	SEOUL_ENUM_N("SetVariable,", Script::DebuggerClientTag::kSetVariable)
	SEOUL_ENUM_N("Sync", Script::DebuggerClientTag::kSync)
	SEOUL_ENUM_N("Version", Script::DebuggerClientTag::kVersion)
	SEOUL_ENUM_N("Watch", Script::DebuggerClientTag::kWatch)
SEOUL_END_ENUM()

SEOUL_BEGIN_ENUM(Script::DebuggerServerTag)
	SEOUL_ENUM_N("Unknown", Script::DebuggerServerTag::kUnknown)
	SEOUL_ENUM_N("AddWatch", Script::DebuggerServerTag::kAddWatch)
	SEOUL_ENUM_N("Break", Script::DebuggerServerTag::kBreak)
	SEOUL_ENUM_N("Continue", Script::DebuggerServerTag::kContinue)
	SEOUL_ENUM_N("GetFrame", Script::DebuggerServerTag::kGetFrame)
	SEOUL_ENUM_N("GetChildren", Script::DebuggerServerTag::kGetChildren)
	SEOUL_ENUM_N("RemoveWatch", Script::DebuggerServerTag::kRemoveWatch)
	SEOUL_ENUM_N("SetBreakpoints", Script::DebuggerServerTag::kSetBreakpoints)
	SEOUL_ENUM_N("SetVariable", Script::DebuggerServerTag::kSetVariable)
	SEOUL_ENUM_N("StepInto", Script::DebuggerServerTag::kStepInto)
	SEOUL_ENUM_N("StepOut", Script::DebuggerServerTag::kStepOut)
	SEOUL_ENUM_N("StepOver", Script::DebuggerServerTag::kStepOver)
SEOUL_END_ENUM()

SEOUL_BEGIN_ENUM(Script::DebuggerVariableType)
	SEOUL_ENUM_N("nil", Script::DebuggerVariableType::kNil)
	SEOUL_ENUM_N("boolean", Script::DebuggerVariableType::kBoolean)
	SEOUL_ENUM_N("lightuserdata", Script::DebuggerVariableType::kLightUserData)
	SEOUL_ENUM_N("number", Script::DebuggerVariableType::kNumber)
	SEOUL_ENUM_N("string", Script::DebuggerVariableType::kString)
	SEOUL_ENUM_N("table", Script::DebuggerVariableType::kTable)
	SEOUL_ENUM_N("function", Script::DebuggerVariableType::kFunction)
	SEOUL_ENUM_N("userdata", Script::DebuggerVariableType::kUserData)
	SEOUL_ENUM_N("thread", Script::DebuggerVariableType::kThread)
	SEOUL_ENUM_N("emptytable", Script::DebuggerVariableType::kEmptyTable)
SEOUL_END_ENUM()

///////////////////////////////////////////////////////////////////////////////
// END REFLECTION HOOKS
///////////////////////////////////////////////////////////////////////////////

namespace Script
{

///////////////////////////////////////////////////////////////////////////////
// BEGIN LOCAL UTILITY FUNCTIONS
///////////////////////////////////////////////////////////////////////////////
static inline DebuggerVariableType ToVariableType(
	CheckedPtr<lua_State> pLuaVm,
	Int32 iIndex,
	String& rsExtendedTypeInfo)
{
	rsExtendedTypeInfo = String();
	auto const iLuaType = lua_type(pLuaVm, iIndex);
	switch (iLuaType)
	{
	case LUA_TNIL: return DebuggerVariableType::kNil;
	case LUA_TBOOLEAN: return DebuggerVariableType::kBoolean;
	case LUA_TLIGHTUSERDATA: return DebuggerVariableType::kLightUserData;
	case LUA_TNUMBER: return DebuggerVariableType::kNumber;
	case LUA_TSTRING: return DebuggerVariableType::kString;
	case LUA_TTABLE:
	{
		// Look for extended type info if a table.
		if (0 != lua_getmetatable(pLuaVm, iIndex))
		{
			lua_pushstring(pLuaVm, "m_sClassName");
			lua_rawget(pLuaVm, -2);
			if (lua_isstring(pLuaVm, -1))
			{
				rsExtendedTypeInfo.Assign(lua_tostring(pLuaVm, -1));
			}

			lua_pop(pLuaVm, 2);
		}

		return DebuggerVariableType::kTable;
	}
	case LUA_TFUNCTION: return DebuggerVariableType::kFunction;
	case LUA_TUSERDATA: return DebuggerVariableType::kUserData;
	case LUA_TTHREAD: return DebuggerVariableType::kThread;
	default:
		SEOUL_FAIL("Out-of-sync enum.");
		return DebuggerVariableType::kNil;
	};
}

// NOTE: Necessary because LuaJIT has a bug/bad behavior
// that prevents it from properly calling CALL and RET
// debugger hooks in perfect pairs. As a result, we
// must derive the call stack depth as needed.
//
// Unfortunately, lua_getstack() is linear with the size
// of the stack, so this function is O(nlogn) where
// n is the size of the stack we're trying to compute.
//
// This could be reduced to O(n) if we added a function
// to LuaJIT for this purpose specifically, that just
// returned the stack depth.
static inline Int32 GetStackDepth(CheckedPtr<lua_State> pLuaVm)
{
	lua_Debug ar;
	
	Int iLevel = 0;
	while (0 != lua_getstack(pLuaVm, iLevel, &ar))
	{
		++iLevel;
	}

	return iLevel;
}

/**
 * Attempts to find a value for the given variable
 * as seen from the current context. Resolves:
 * - local first
 * - then up values.
 * - finally global.
 *
 * Due to Lua semantics, always resolves to (at least) nil,
 * as a missing global is still nil at runtime.
 */
static void LuaPushValueFromLocalContext(CheckedPtr<lua_State> pLuaVm, CheckedPtr<lua_Debug> pDebugInfo, const String& sName)
{
	SEOUL_SCRIPT_CHECK_VM_STACK(pLuaVm, 1);

	Bool bFound = false;

	// Locals
	{
		Byte const* sLocal = nullptr;
		Int iLocal = 1;
		while ((sLocal = lua_getlocal(pLuaVm, pDebugInfo, iLocal++)) != nullptr)
		{
			// Found a match - later matches win, so we need to pop any existing
			// value and replace with this one if found.
			if (0 == strcmp(sName.CStr(), sLocal))
			{
				if (bFound)
				{
					lua_remove(pLuaVm, -2);
				}
				bFound = true;
			}
			else
			{
				lua_pop(pLuaVm, 1);
			}
		}
	}

	// Done if we found a local.
	if (bFound)
	{
		return;
	}

	// Up values.
	{
		// Get the current function's data onto the stack.
		if (0 != lua_getinfo(pLuaVm, "fu", pDebugInfo))
		{
			auto const iFunction = lua_gettop(pLuaVm);

			Byte const* sUpValue = nullptr;
			Int iUpValue = 1;
			while ((sUpValue = lua_getupvalue(pLuaVm, iFunction, iUpValue++)) != nullptr)
			{
				// Found a match - later matches win, so we need to pop any existing
				// value and replace with this one if found.
				if (0 == strcmp(sName.CStr(), sUpValue))
				{
					if (bFound)
					{
						lua_remove(pLuaVm, -2);
					}
					bFound = true;
				}
				else
				{
					lua_pop(pLuaVm, 1);
				}
			}

			// Pop the function from the stack.
			lua_remove(pLuaVm, iFunction);
		}
	}

	// Done if found.
	if (bFound)
	{
		return;
	}

	// Finally, global.
	lua_getglobal(pLuaVm, sName.CStr());
}

/** Shared utility, attempts to push a lua value parsed from sValue based on its expected type. */
static Bool LuaPushValue(CheckedPtr<lua_State> pLuaVm, DebuggerVariableType eType, const String& sValue)
{
	switch (eType)
	{
	case DebuggerVariableType::kBoolean:
	{
		Bool bValue = false;
		if (FromString(sValue, bValue))
		{
			lua_pushboolean(pLuaVm, bValue);
			return true;
		}
		break;
	}
	case DebuggerVariableType::kNumber:
	{
		Double fValue = false;
		if (FromString(sValue, fValue))
		{
			lua_pushnumber(pLuaVm, fValue);
			return true;
		}
		break;
	}
	case DebuggerVariableType::kString:
	{
		lua_pushlstring(pLuaVm, sValue.CStr(), sValue.GetSize());
		return true;
	}
	default:
		break;
	};

	return false;
}

/**
 * Attempts to find a slot to set for the given variable
 * as seen from the current context. Resolves:
 * - local first
 * - then up values.
 * - finally global.
 */
static Bool LuaSetValueFromLocalContext(
	CheckedPtr<lua_State> pLuaVm,
	CheckedPtr<lua_Debug> pDebugInfo,
	const String& sName,
	DebuggerVariableType eType,
	const String& sValue)
{
	SEOUL_SCRIPT_CHECK_VM_STACK(pLuaVm);

	Int32 iFound = -1;

	// Locals
	{
		Byte const* sLocal = nullptr;
		Int iLocal = 1;
		while ((sLocal = lua_getlocal(pLuaVm, pDebugInfo, iLocal++)) != nullptr)
		{
			// Found a match - later matches win, so we need to pop any existing
			// value and replace with this one if found.
			if (0 == strcmp(sName.CStr(), sLocal))
			{
				iFound = iLocal-1;
			}

			lua_pop(pLuaVm, 1);
		}
	}

	// Done if we found a local.
	if (iFound >= 1)
	{
		if (!LuaPushValue(pLuaVm, eType, sValue))
		{
			return false;
		}

		if (nullptr == lua_setlocal(pLuaVm, pDebugInfo, iFound))
		{
			lua_pop(pLuaVm, 1);
			return false;
		}

		return true;
	}

	// Up values.
	{
		// Get the current function's data onto the stack.
		if (0 != lua_getinfo(pLuaVm, "fu", pDebugInfo))
		{
			auto const iFunction = lua_gettop(pLuaVm);

			Byte const* sUpValue = nullptr;
			Int iUpValue = 1;
			while ((sUpValue = lua_getupvalue(pLuaVm, iFunction, iUpValue++)) != nullptr)
			{
				// Found a match - later matches win, so we need to pop any existing
				// value and replace with this one if found.
				if (0 == strcmp(sName.CStr(), sUpValue))
				{
					iFound = iUpValue-1;
				}
				lua_pop(pLuaVm, 1);
			}

			// Done if found.
			if (iFound >= 1)
			{
				if (!LuaPushValue(pLuaVm, eType, sValue))
				{
					iFound = -1;
				}
				else if (nullptr == lua_setupvalue(pLuaVm, iFunction, iFound))
				{
					lua_pop(pLuaVm, 1);
					iFound = -1;
				}
			}

			// Pop the function from the stack.
			lua_remove(pLuaVm, iFunction);
		}
	}

	// Done if found - will have been set successfully above.
	if (iFound >= 1)
	{
		return true;
	}

	// Finally, set a global.
	if (LuaPushValue(pLuaVm, eType, sValue))
	{
		lua_setglobal(pLuaVm, sName.CStr());
		return true;
	}

	return false;
}

/** Utility to convert a local variable value into a VariableInfo structure. Value of the variable is already expected to be on the stack at the given index. */
static Bool LocalToVariableInfo(
	CheckedPtr<lua_State> pLuaVm, 
	Int32 iIndex, 
	const String& sName, 
	DebuggerClient::VariableInfo& rVariableInfo)
{
	SEOUL_SCRIPT_CHECK_VM_STACK(pLuaVm);

	rVariableInfo.m_eType = ToVariableType(pLuaVm, iIndex, rVariableInfo.m_sExtendedType);
	rVariableInfo.m_sName.Assign(sName);
	
	switch (rVariableInfo.m_eType)
	{
	case DebuggerVariableType::kNil: rVariableInfo.m_sValue.Assign("null"); return true;
	case DebuggerVariableType::kBoolean:
	{
		auto const iValue = lua_toboolean(pLuaVm, iIndex);
		rVariableInfo.m_sValue.Assign(0 == iValue ? "false" : "true");
		return true;
	}
	case DebuggerVariableType::kLightUserData: rVariableInfo.m_sValue.Assign("<lightuserdata>"); return true;
	case DebuggerVariableType::kNumber: // fall-through
	case DebuggerVariableType::kString: rVariableInfo.m_sValue.Assign(lua_tostring(pLuaVm, iIndex)); return true;
	case DebuggerVariableType::kTable:
	{
		// Check if the table element count. If it is empty, use the kEmptyTable
		// type instead of kTable.
		lua_pushnil(pLuaVm);
		Int const iTable = (iIndex < 0 ? iIndex - 1 : iIndex);

		// Zero indicates empty table.
		if (0 == lua_next(pLuaVm, iTable))
		{
			rVariableInfo.m_eType = DebuggerVariableType::kEmptyTable;
			rVariableInfo.m_sValue.Assign("<empty-table>");
		}
		// Success means table with elements, so pop the key and value and assign a regular table.
		else
		{
			lua_pop(pLuaVm, 2);
			rVariableInfo.m_sValue.Assign("<table>");
		}
		return true;
	}
	case DebuggerVariableType::kFunction: rVariableInfo.m_sValue.Assign("<function>"); return true;
	case DebuggerVariableType::kUserData: rVariableInfo.m_sValue.Assign("<userdata>"); return true;
	case DebuggerVariableType::kThread: rVariableInfo.m_sValue.Assign("<thread>"); return true;
	default:
		SEOUL_FAIL("Out-of-sync enum.");
		return false;
	};
}

// Set a max local vars for extremely large local frames.
static const int kiMaxVars = 128;
static Byte const* ksSelf = "self";
static Byte const* ksThis = "this";
static Byte const* ksVararg = "(*vararg)";

/** Utilitay used by ToFrameInfo). */
static Bool ProcessFrameVar(
	CheckedPtr<lua_State> pLuaVm,
	Int32 iVars,
	Byte const* sName,
	UInt32& ruOutVars,
	DebuggerClient::FrameInfo& rFrameInfo,
	Bool bSkipFunctions)
{
	Bool bIsVararg = false;
	// Skip temporaries - these will start with a '('.
	if (sName[0] == '(')
	{
		if (0 == strcmp(sName, ksVararg))
		{
			bIsVararg = true;
		}
		else
		{
			lua_pop(pLuaVm, 1);
			return true;
		}
	}

	if (iVars > kiMaxVars)
	{
		lua_pop(pLuaVm, 1);
		return true;
	}

	// If named 'self', rename to 'this'.
	if (0 == strcmp(sName, ksSelf))
	{
		sName = ksThis;
	}

	rFrameInfo.m_vVariables.Resize(++ruOutVars);
	auto& var = rFrameInfo.m_vVariables.Back();
	Bool bReturn = false;
	if (bIsVararg)
	{
		bReturn = LocalToVariableInfo(pLuaVm, -1, String::Printf("vararg%d", iVars), var);
	}
	else
	{
		bReturn = LocalToVariableInfo(pLuaVm, -1, sName, var);
	}
	lua_pop(pLuaVm, 1);

	if (!bReturn)
	{
		return false;
	}

	// Remove if asked.
	if (bSkipFunctions && rFrameInfo.m_vVariables.Back().m_eType == DebuggerVariableType::kFunction)
	{
		--ruOutVars;
		rFrameInfo.m_vVariables.PopBack();
	}

	return true;
}

/** Given a debug context, convert data in a particular frame into output frame info. */
static Bool ToFrameInfo(
	CheckedPtr<lua_State> pLuaVm,
	Int32 iLevel,
	DebuggerClient::FrameInfo& rFrameInfo)
{
	SEOUL_SCRIPT_CHECK_VM_STACK(pLuaVm);

	// Initialize the activation record.
	lua_Debug ar;
	memset(&ar, 0, sizeof(ar));

	// Get the stack at the specified level.
	if (0 == lua_getstack(pLuaVm, iLevel, &ar))
	{
		return false;
	}

	UInt32 uOutVars = 0u;

	// Enumerate up values.
	Int32 iVars = 1; // NOTE: Unlike getstack, this index is 1 based.
	Byte const* sName = nullptr;
	if (0 != lua_getinfo(pLuaVm, "fu", &ar))
	{
		auto const iFunction = lua_gettop(pLuaVm);
		while ((sName = lua_getupvalue(pLuaVm, iFunction, iVars++)) != nullptr)
		{
			if (!ProcessFrameVar(pLuaVm, iVars, sName, uOutVars, rFrameInfo, true))
			{
				// Pop the function from the stack.
				lua_remove(pLuaVm, iFunction);
				return false;
			}
		}

		// Pop the function from the stack.
		lua_remove(pLuaVm, iFunction);
	}

	// Enumerate local variables.
	iVars = 1; // NOTE: Unlike getstack, this index is 1 based.
	sName = nullptr;
	while ((sName = lua_getlocal(pLuaVm, &ar, iVars++)) != nullptr)
	{
		if (!ProcessFrameVar(pLuaVm, iVars, sName, uOutVars, rFrameInfo, false))
		{
			return false;
		}
	}

	// Enumerate varargs.
	iVars = -1; // NOTE: For varargs we start at -1 and go backwards.
	sName = nullptr;
	while ((sName = lua_getlocal(pLuaVm, &ar, iVars--)) != nullptr)
	{
		if (!ProcessFrameVar(pLuaVm, iVars, sName, uOutVars, rFrameInfo, false))
		{
			return false;
		}
	}

	return true;
}

/** 
 * @return A VM execute state from a server message - defaults to kRunning if
 * the specified tag does not directly after execute state.
 */
static DebuggerExecuteState ToExecuteState(DebuggerServerTag e)
{
	switch (e)
	{
	case DebuggerServerTag::kBreak:
		return DebuggerExecuteState::kStepInto;

	case DebuggerServerTag::kContinue:
		return DebuggerExecuteState::kRunning;
	
	case DebuggerServerTag::kStepInto: // fall-through
		return DebuggerExecuteState::kStepInto;

	case DebuggerServerTag::kStepOut: // fall-through
		return DebuggerExecuteState::kStepOut;

	case DebuggerServerTag::kStepOver:
		return DebuggerExecuteState::kStepOver;

	default:
		return DebuggerExecuteState::kRunning;
	};
}

///////////////////////////////////////////////////////////////////////////////
// END LOCAL UTILITY FUNCTIONS
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// BEGIN MESSAGE CREATE FUNCTIONS
///////////////////////////////////////////////////////////////////////////////
DebuggerClient::Message* DebuggerClient::Message::Create(DebuggerClientTag eTag)
{
	auto pMessage = SEOUL_NEW(MemoryBudgets::UIDebug) Message;
	pMessage->m_uTag = (UInt32)eTag;
	return pMessage;
}

DebuggerClient::Message* DebuggerClient::Message::Create(DebuggerServerTag eTag)
{
	auto pMessage = SEOUL_NEW(MemoryBudgets::UIDebug) Message;
	pMessage->m_uTag = (UInt32)eTag;
	return pMessage;
}

/** Reads a message from rStream - returns nullptr if the message is not complete or a read error occurs. */
DebuggerClient::Message* DebuggerClient::Message::Create(SocketStream& rStream)
{
	UInt32 uMessageSize = 0u;
	UInt32 uTag = 0u;

	// Our debugger protocol is little endian, so we
	// can't use Read32(), etc. here, since that assumes
	// "network order" (which is big endian).
	if (!rStream.Read(&uMessageSize, sizeof(uMessageSize)))
	{
		return nullptr;
	}
	if (!rStream.Read(&uTag, sizeof(uTag)))
	{
		return nullptr;
	}

	// Big endian platforms need to size and tag data now - the blob of
	// data will be endian swapped automatically during read from the StreamBuffer.
#if SEOUL_BIG_ENDIAN
	uMessageSize = EndianSwap32(uMessageSize);
	uTag = EndianSwap32(uTag);
#endif

	// Message is too big, likely invalid or corrupt data.
	if (uMessageSize > kMaxMessageSize)
	{
		return nullptr;
	}

	// Read the data blob if the message has data.
	StreamBuffer data;
	if (uMessageSize > 0)
	{
		data.PadTo(uMessageSize, false);
		if (!rStream.Read(data.GetBuffer(), uMessageSize))
		{
			return nullptr;
		}

		// Reset the stream offset to 0 so it is ready for read.
		data.SeekToOffset(0);
	}

	// Instatiate the message with the specified tag and swap in the data block.
	auto pMessage = SEOUL_NEW(MemoryBudgets::UIDebug) Message;
	pMessage->m_uTag = uTag;
	pMessage->m_Data.Swap(data);

	return pMessage;
}

///////////////////////////////////////////////////////////////////////////////
// Client to server message create functions.
///////////////////////////////////////////////////////////////////////////////

/** Send a message to the server to request any user defined breakpoints. */
DebuggerClient::Message* DebuggerClient::Message::CreateClientAskBreakpoints()
{
	auto pMessage = Message::Create(DebuggerClientTag::kAskBreakpoints);
	return pMessage;
}

/** Send stack frame info to the server at depth uDepth. */
DebuggerClient::Message* DebuggerClient::Message::CreateClientFrame(
	CheckedPtr<lua_State> pLuaVm,
	UInt32 uDepth)
{
	auto pMessage = Message::Create(DebuggerClientTag::kFrame);
	pMessage->Write(uDepth); // Depth we're sending.

	// Locally cache the frame info.
	DebuggerClient::FrameInfo frameInfo;
	if (!ToFrameInfo(pLuaVm, (Int32)uDepth, frameInfo))
	{
		return pMessage;
	}

	// Send each variable one-by-one.
	for (auto const& info : frameInfo.m_vVariables)
	{
		pMessage->Write(info); // Write the actual variable info.
	}

	return pMessage;
}

/** Sends information about a variable to the server in response to a server-to-client GetChildren message. */
DebuggerClient::Message* DebuggerClient::Message::CreateClientGetChildren(
	CheckedPtr<lua_State> pLuaVm, 
	UInt32 uStackDepth,
	const String& sPath)
{
	SEOUL_SCRIPT_CHECK_VM_STACK(pLuaVm);

	// Start creation of the message.
	auto pMessage = Message::Create(DebuggerClientTag::kGetChildren);
	pMessage->Write(uStackDepth);
	pMessage->Write(sPath);

	// Resolution works as follows:
	// - split the path string on a dot separator.
	// - for level 0:
	//   - check local variables that match.
	//   - check up variables that match.
	//   - look in the global table.
	// - if we have a variable from step 0, now resolve each
	//   additional step.
	Vector<String, MemoryBudgets::Scripting> vs;
	SplitString(sPath, '.', vs);
	if (vs.IsEmpty())
	{
		// Early out, no parts.
		return pMessage;
	}

	// Filter - if the first entry is 'this', convert to 'self'.
	if (vs.Front() == ksThis)
	{
		vs.Front().Assign(ksSelf);
	}

	// Get stack level uStackDepth, early out if fail.
	lua_Debug ar;
	memset(&ar, 0, sizeof(ar));
	if (0 == lua_getstack(pLuaVm, uStackDepth, &ar))
	{
		return pMessage;
	}

	// Level 0, apply the above technique.
	LuaPushValueFromLocalContext(pLuaVm, &ar, vs.Front());

	// Now iterate and lookup.
	for (UInt32 i = 1u; i < vs.GetSize(); ++i)
	{
		// Early out if no table at the given index.
		if (!lua_istable(pLuaVm, -1))
		{
			lua_pop(pLuaVm, 1);
			return pMessage;
		}

		// TODO: We need to include proper type info
		// with each part of the path, since this will
		// break in multiple cases (e.g. a table has
		// both a key "1" and a key 1.

		// Try as a string first.
		lua_pushstring(pLuaVm, vs[i].CStr());
		lua_rawget(pLuaVm, -2);
		
		// If that failed, try converting to a number.
		if (lua_isnil(pLuaVm, -1))
		{
			Double f = 0.0;
			Bool b = false;
			if (FromString(vs[i], f))
			{
				lua_pop(pLuaVm, 1);
				lua_pushnumber(pLuaVm, f);
				lua_gettable(pLuaVm, -2);
			}
			else if (FromString(vs[i], b))
			{
				lua_pop(pLuaVm, 1);
				lua_pushboolean(pLuaVm, (b ? 1 : 0));
				lua_gettable(pLuaVm, -2);
			}
		}

		// Remove the previous table.
		lua_remove(pLuaVm, -2);
	}

	// If no table on the top, early out.
	if (!lua_istable(pLuaVm, -1))
	{
		lua_pop(pLuaVm, 1);
		return pMessage;
	}

	// Iterate the table.
	VariableInfo info;
	lua_pushnil(pLuaVm);
	while (0 != lua_next(pLuaVm, -2))
	{
		// Key is now at -2, value at -1.
		String sKey;
		switch (lua_type(pLuaVm, -2))
		{
		case LUA_TNIL:
			break; // Should never happen.
		case LUA_TBOOLEAN:
			sKey.Assign(lua_toboolean(pLuaVm, -2) == 0 ? "false" : "true");
			break;
		case LUA_TNUMBER:
			sKey.Assign(String::Printf("%g", lua_tonumber(pLuaVm, -2)));
			break;
		case LUA_TSTRING:
			sKey.Assign(lua_tostring(pLuaVm, -2));
			break;
		
		case LUA_TFUNCTION:
		case LUA_TLIGHTUSERDATA:
		case LUA_TTABLE:
		case LUA_TTHREAD:
		case LUA_TUSERDATA:
			sKey.Assign(String::Printf("%p", lua_topointer(pLuaVm, -2)));
			break;
		default:
			SEOUL_FAIL("Out-of-sync enum.");
			break;
		};
		
		// Skip empty keys or values.
		if (sKey.IsEmpty())
		{
			lua_pop(pLuaVm, 1);
			continue;
		}

		// Compute variable.
		if (!LocalToVariableInfo(pLuaVm, -1, sKey, info))
		{
			lua_pop(pLuaVm, 1);
			continue;
		}

		// Pop the value.
		lua_pop(pLuaVm, 1);

		// Write the info.
		pMessage->Write(info);
	}

	// Pop the table.
	lua_pop(pLuaVm, 1);

	return pMessage;
}

/** Sends information about a variable to the server in response to a server-to-client SetVariable message. */
DebuggerClient::Message* DebuggerClient::Message::CreateClientSetVariable(
	CheckedPtr<lua_State> pLuaVm,
	UInt32 uStackDepth,
	const String& sPath,
	DebuggerVariableType eType,
	const String& sValue)
{
	auto pMessage = Message::Create(DebuggerClientTag::kSetVariable);
	
	// Stack and path are always included.
	pMessage->Write(uStackDepth);
	pMessage->Write(sPath);

	// Resolution works as follows:
	// - split the path string on a dot separate.
	// - for level 0:
	//   - check local variables that match.
	//   - check up variables that match.
	//   - look in the global table.
	// - if we have a variable from step 0, now resolve each
	//   additional step.
	Vector<String, MemoryBudgets::Scripting> vs;
	SplitString(sPath, '.', vs);
	if (vs.IsEmpty())
	{
		// Early out, no parts.
		pMessage->Write(false);
		return pMessage;
	}

	// Filter - if the first entry is 'this', convert to 'self'.
	if (vs.Front() == ksThis)
	{
		vs.Front().Assign(ksSelf);
	}

	// Get stack level uStackDepth, early out if fail.
	lua_Debug ar;
	memset(&ar, 0, sizeof(ar));
	if (0 == lua_getstack(pLuaVm, uStackDepth, &ar))
	{
		// Done, failure.
		pMessage->Write(false);
		return pMessage;
	}

	// Special handling, if vs is 1 element, it means we're setting a local variable, an up variable,
	// or a global.
	if (vs.GetSize() == 1)
	{
		// Done, success or failure.
		pMessage->Write(LuaSetValueFromLocalContext(pLuaVm, &ar, vs.Front(), eType, sValue));
		return pMessage;
	}

	// Otherwise, we're writing a table member.

	// Level 0, apply the above technique.
	LuaPushValueFromLocalContext(pLuaVm, &ar, vs.Front());

	// Now iterate and lookup - stop prior to the last element,
	// as that is the name of the variable to set.
	for (UInt32 i = 1u; i + 1u < vs.GetSize(); ++i)
	{
		// Early out if no table at the given index.
		if (!lua_istable(pLuaVm, -1))
		{
			lua_pop(pLuaVm, 1);

			// Done, failure.
			pMessage->Write(false);
			return pMessage;
		}

		lua_getfield(pLuaVm, -1, vs[i].CStr());
		lua_remove(pLuaVm, -2);
	}

	// If no table on the top, early out.
	if (!lua_istable(pLuaVm, -1))
	{
		lua_pop(pLuaVm, 1);

		// Done failure.
		pMessage->Write(false);
		return pMessage;
	}

	// Attempt to push the value onto the stack.
	Bool bResult = LuaPushValue(pLuaVm, eType, sValue);

	// Now set the variable based on type.
	if (bResult)
	{
		lua_setfield(pLuaVm, -2, vs.Back().CStr());
	}

	// Done, success or failure.
	lua_pop(pLuaVm, 1);

	// Done, success or failure.
	pMessage->Write(bResult);
	return pMessage;
}

/** Sent at startup to report the Flash Player version. */
DebuggerClient::Message* DebuggerClient::Message::CreateClientVersion()
{
	auto pMessage = Message::Create(DebuggerClientTag::kVersion);
	pMessage->Write(kuDebuggerVersion); // Debugger version.
	pMessage->Write(kuConnectMagic);    // Connection signature/magic.
	return pMessage;
}

///////////////////////////////////////////////////////////////////////////////
// BEGIN MESSAGE READ-WRITE FUNCTIONS
///////////////////////////////////////////////////////////////////////////////

/** Read string data in this Message body into rh - debugger protocol string data is just a null terminated c-style string, UTF8. */
Bool DebuggerClient::Message::Read(HString& rh)
{
	UInt32 uSize = 0u;
	if (!m_Data.ReadLittleEndian32(uSize))
	{
		return false;
	}

	rh = HString(m_Data.GetBuffer() + m_Data.GetOffset(), uSize);
	m_Data.SeekToOffset(m_Data.GetOffset() + uSize);
	return true;
}

/** Read string data in this Message body into rs - debugger protocol string data is just a null terminated c-style string, UTF8. */
Bool DebuggerClient::Message::Read(String& rs)
{
	UInt32 uSize = 0u;
	if (!m_Data.ReadLittleEndian32(uSize))
	{
		return false;
	}

	rs.Assign(m_Data.GetBuffer() + m_Data.GetOffset(), uSize);
	m_Data.SeekToOffset(m_Data.GetOffset() + uSize);
	return true;
}

/** Write cstring style UTF8 string data into this message. */
void DebuggerClient::Message::Write(Byte const* s, UInt32 uSize)
{
	m_Data.WriteLittleEndian32((Int32)uSize);
	m_Data.Write(s, uSize);
}

/** Write variable info into this message. */
void DebuggerClient::Message::Write(const VariableInfo& info)
{
	Write(info.m_sName);
	Write((Int32)info.m_eType);
	Write(info.m_sExtendedType);
	Write(info.m_sValue);
}

/** Sends this message over the wire - false is returned if an error occurs during the send. */
Bool DebuggerClient::Message::Send(SocketStream& r) const
{
	// Cache the message size and tag locally.
	auto uMessageSize = m_Data.GetTotalDataSizeInBytes();
	UInt32 uTag = m_uTag;

	// Swap to little endian if we're on a big endian platform.
#if SEOUL_BIG_ENDIAN
	uMessageSize = EndianSwap32(uMessageSize);
	uTag = EndianSwap32(uTag);
#endif

	// The Flash debugger protocol is little endian, don't use SocketStream::Write32 here,
	// since it assumes network order (big endian).
	if (!r.Write(&uMessageSize, sizeof(uMessageSize)) ||
		!r.Write(&uTag, sizeof(uTag)))
	{
		return false;
	}

	// If we have a data blob to send, get the buffer and send it.
	if (m_Data.GetTotalDataSizeInBytes() > 0)
	{
		SEOUL_ASSERT(m_Data.GetTotalDataSizeInBytes() < UIntMax);
		if (!r.Write(m_Data.GetBuffer(), (SocketStream::SizeType) m_Data.GetTotalDataSizeInBytes()))
		{
			return false;
		}
	}

	// Success or failure is determined on successful flush of the socket.
	return r.Flush();
}

///////////////////////////////////////////////////////////////////////////////
// END MESSAGE READ-WRITE FUNCTIONS
///////////////////////////////////////////////////////////////////////////////

DebuggerClient::WorkerThread::WorkerThread()
	: m_pThread()
	, m_ThreadId()
	, m_Signal()
	, m_Buffer()
	, m_bShuttingDown(false)
{
}

DebuggerClient::WorkerThread::~WorkerThread()
{
}

/** Must be called before destructing this WorkerThread. */
void DebuggerClient::WorkerThread::Shutdown()
{
	m_bShuttingDown = true;
	SeoulMemoryBarrier();
	Message* p = m_Buffer.Pop();
	while (nullptr != p)
	{
		SafeDelete(p);
		p = m_Buffer.Pop();
	}
	m_Signal.Activate();
}

/** Wait for this worker thread to finish - it is up to the caller to ensure the worker thread will terminate in finite time. */
void DebuggerClient::WorkerThread::WaitForThread()
{
	if (m_pThread.IsValid())
	{
		m_pThread->WaitUntilThreadIsNotRunning();
		m_pThread.Reset();
	}
}

namespace
{

void OnDebuggerListenerChange(
	const String& /*sOldPath*/,
	const String& /*sNewPath*/,
	FileChangeNotifier::FileEvent /*eEvent*/)
{
	if (DebuggerClient::Get())
	{
		DebuggerClient::Get()->RefreshDebuggerServerListening();
	}
}

} // namespace anonymous

DebuggerClient::DebuggerClient(
	FilePath appScriptProjectPath,
	const String& sServerHostname)
	: m_AppScriptProjectPath(appScriptProjectPath)
	, m_sScriptsPath(Path::GetDirectoryName(appScriptProjectPath.GetAbsoluteFilenameInSource()))
	, m_pNotifier()
	, m_sServerHostname(sServerHostname)
	, m_PublicMutex()
	, m_State()
	, m_Receive()
	, m_Send()
	, m_BreakSignal()
	, m_Socket()
	, m_Stream(m_Socket)
	, m_bCanSend(false)
	, m_bDebuggerServerListening(false)
	, m_bVerboseLogging(false)
{
	RefreshDebuggerServerListening();
	m_pNotifier.Reset(SEOUL_NEW(MemoryBudgets::Scripting) FileChangeNotifier(
		Path::GetDirectoryName(m_AppScriptProjectPath.GetAbsoluteFilenameInSource()),
		SEOUL_BIND_DELEGATE(OnDebuggerListenerChange),
		FileChangeNotifier::kAll,
		false));

	m_Receive.m_pThread.Reset(SEOUL_NEW(MemoryBudgets::UIDebug) Thread(SEOUL_BIND_DELEGATE(&DebuggerClient::ReceiveThreadBody, this)));
	m_Receive.m_pThread->Start("ScriptDebuggerClient Receive Thread");

	m_Send.m_pThread.Reset(SEOUL_NEW(MemoryBudgets::UIDebug) Thread(SEOUL_BIND_DELEGATE(&DebuggerClient::SendThreadBody, this)));
	m_Send.m_pThread->Start("ScriptDebuggerClient Send Thread");
}

DebuggerClient::~DebuggerClient()
{
	// Shutdown the sender.
	m_Send.Shutdown();
	m_Send.WaitForThread();

	// Shutdown the receiver.
	m_Receive.Shutdown();
	m_Socket.Shutdown();
	m_Receive.WaitForThread();

	// Flush all buffers.
	InternalSafeDeleteAllBufferContents();
}

/** Hard check to refresh listener status. Slow. */
void DebuggerClient::RefreshDebuggerServerListening()
{
	// A file will be present next to the app script project
	// solution with the extension "debugger_listener" if
	// the debugger is listening.
	//
	// We also try deleting the file, to check if
	// the Visual Studio instance that created it crashed
	// (we will not be able to delete it if the debugger
	// session still has a lock on it).
	auto const sLockFile(Path::ReplaceExtension(m_AppScriptProjectPath.GetAbsoluteFilenameInSource(), ".debugger_listener"));
	if (!FileManager::Get()->Exists(sLockFile))
	{
		m_bDebuggerServerListening = false;
		return;
	}

	// If we can delete it, it's just stale. No debugger is active.
	if (FileManager::Get()->Delete(sLockFile))
	{
		m_bDebuggerServerListening = false;
		return;
	}

	// Debugger is listening.
	m_bDebuggerServerListening = true;
}

/** Thread-safe queue a message for send - succeeds unless the send buffer is full. The message will be placed on wire by the send thread. */
void DebuggerClient::EnqueueSend(Message* pMessage)
{
	m_Send.m_Buffer.Push(pMessage);
	m_Send.m_Signal.Activate();
}

/** Called by a script VM when a code step occurs. */
void DebuggerClient::OnStep(CheckedPtr<lua_State> pLuaVm, CheckedPtr<lua_Debug> pDebug)
{
	// Set the active movie and dispatch handling of the leave event to StateLock.
	StateLock lock(m_State);
	lock.OnStep(*this, pLuaVm, pDebug);
}

/** Called by a script VM when it is about to be destroyed. */
void DebuggerClient::OnVmDestroy(CheckedPtr<lua_State> pLuaVm)
{
	// Set the active movie and dispatch handling of the leave event to StateLock.
	StateLock lock(m_State);
	lock.OnVmDestroy(*this, pLuaVm);
}

/** @return A client BreakAt message to report a breakpoint hit to the server. */
DebuggerClient::Message* DebuggerClient::State::CreateClientBreakAt(
	DebuggerClient& r,
	VmEntry& rVmEntry,
	CheckedPtr<lua_State> pLuaVm,
	SuspendReason eSuspendReason)
{
	// Set a max level for extremely large stacks.
	static const int kiMaxStackLevel = 32;

	auto pMessage = Message::Create(DebuggerClientTag::kBreakAt);

	// Include the break reason.
	pMessage->Write((Int32)eSuspendReason);

	// Initialize the activation record.
	lua_Debug ar;
	memset(&ar, 0, sizeof(ar));

	// Iterate over all levels of the stack - lua_getstack() returns
	// 0 on error/end of stack.
	Int32 iLevel = 0;
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
			BreakInfo info;
			ToBreakpointPopulated(r, rVmEntry, pLuaVm, &ar, info);

			Byte const* sFunctionName = (nullptr != ar.name ? ar.name : "");
			if (nullptr == sFunctionName || sFunctionName[0] == '\0')
			{
				sFunctionName = "<anonymous>";
			}
			pMessage->Write(sFunctionName, StrLen(sFunctionName));
			pMessage->Write(info.m_uBreakpoint);

			// The server checks info.m_uBreakpoint file id == 0
			// so we must write the filename if that's true.
			if ((info.m_uBreakpoint & 0x0000FFFF) == 0u)
			{
				pMessage->Write(info.m_FileName);
			}
		}

		// Clear the activation record before getting the next
		// stack frame.
		memset(&ar, 0, sizeof(ar));
	}

	return pMessage;
}

/** Compute the breakpoint data at the current stack frame. */
void DebuggerClient::State::GetCurrentBreakInfo(
	DebuggerClient& r,
	VmEntry& rVmEntry,
	CheckedPtr<lua_State> pLuaVm,
	BreakInfo& rInfo)
{
	lua_Debug ar;
	if (0 != lua_getstack(pLuaVm, 0, &ar))
	{
		ToBreakpointUnpopulated(r, rVmEntry, pLuaVm, &ar, rInfo);
		return;
	}

	rInfo = BreakInfo();
}

/** @return breakpoint token corresponding to the given context. */
void DebuggerClient::State::ToBreakpointUnpopulated(
	DebuggerClient& r,
	VmEntry& rVmEntry,
	CheckedPtr<lua_State> pLuaVm,
	CheckedPtr<lua_Debug> pDebugInfo,
	BreakInfo& rInfo)
{
	if (0 == lua_getinfo(pLuaVm, "Sl", pDebugInfo))
	{
		rInfo.m_FileName = HString();
		rInfo.m_uBreakpoint = 0u;
		return;
	}

	ToBreakpointPopulated(r, rVmEntry, pLuaVm, pDebugInfo, rInfo);
}

/** @return breakpoint token corresponding to the given context. */
void DebuggerClient::State::ToBreakpointPopulated(
	DebuggerClient& r,
	VmEntry& rVmEntry,
	CheckedPtr<lua_State> pLuaVm,
	CheckedPtr<lua_Debug> pDebugInfo,
	BreakInfo& rInfo)
{
	UInt16 const uLineNumber = (pDebugInfo->currentline < 0 ? (UInt16)0 : (UInt16)pDebugInfo->currentline);
	UInt16 uFileId = 0;
	auto const sSource = pDebugInfo->source;
	HString outFileName;
	if (!rVmEntry.m_tLookup.GetValue(sSource, uFileId))
	{
		// Handling - only source starting with @ is a filename.
		if (nullptr != sSource && sSource[0] == '@')
		{
			Bool bRetain = false;
			HString fileName;
			if (!rVmEntry.m_tFileLookup.GetValue(sSource, fileName))
			{
				auto uLength = StrLen(sSource + 1);
				uLength = (uLength >= 4u ? (uLength - 4u) : uLength);

				fileName = HString(sSource + 1, uLength);
				SEOUL_VERIFY(rVmEntry.m_tFileLookup.Insert(sSource, fileName).Second);
				bRetain = true;
			}

			auto puId = m_tScripts.Find(fileName);
			if (nullptr != puId)
			{
				uFileId = *puId;
				SEOUL_VERIFY(rVmEntry.m_tLookup.Insert(sSource, uFileId).Second);
				bRetain = true;
			}
			else
			{
				outFileName = fileName;
			}

			// TODO: Need to investigate why this is necessary - for some developers,
			// the chunk name pointers are changing, so we need to explicitly retain
			// it. This action depends on the global unification of Lua strings (strings that
			// are equal are always the same string).
			// TODO: Should be fine in practice but may collide if anyone decides to
			// use chunk name strings for something else in the global lua registry.
			if (bRetain)
			{
				lua_pushstring(pLuaVm, sSource);
				lua_pushstring(pLuaVm, sSource);
				lua_rawset(pLuaVm, LUA_REGISTRYINDEX);
			}
		}
	}

	rInfo.m_uBreakpoint = (UInt32)((((UInt32)uLineNumber << 16) & 0xFFFF0000) | ((UInt32)uFileId & 0x0000FFFF));
	rInfo.m_FileName = outFileName;
}

/** Used at destruction and in a few other contexts when we want to flush and and delete the contents of all AtomicRingBuffers. */
void DebuggerClient::InternalSafeDeleteAllBufferContents()
{
	Message* p = m_Receive.m_Buffer.Pop();
	while (nullptr != p)
	{
		SafeDelete(p);
		p = m_Receive.m_Buffer.Pop();
	}

	p = m_Send.m_Buffer.Pop();
	while (nullptr != p)
	{
		SafeDelete(p);
		p = m_Send.m_Buffer.Pop();
	}
}

/** Process messages that have been pushed onto the receive queue. */
void DebuggerClient::InternalPollReceive()
{
	// Must only be called from the receive thread.
	SEOUL_ASSERT(m_Receive.m_ThreadId == Thread::GetThisThreadId());

	// Get the next message, loop until we've processed the entire queue.
	auto pMessage = m_Receive.m_Buffer.Pop();
	while (nullptr != pMessage)
	{
		switch ((DebuggerServerTag)pMessage->m_uTag)
		{
			// These are handled by the received thread directly.
		case DebuggerServerTag::kBreak:
		case DebuggerServerTag::kContinue:
		case DebuggerServerTag::kStepInto:
		case DebuggerServerTag::kStepOut:
		case DebuggerServerTag::kStepOver:
			break;

			// The server wants frame info of the current stack at a particular depth - retrieve
			// the active stack and send the info.
		case DebuggerServerTag::kGetFrame:
			{
				UInt32 uDepth = 0u;
				if (pMessage->Read(uDepth))
				{
					(void)StateLock(m_State).RequestGetStackFrame(*this, uDepth);
				}
			}
			break;

			// The server wants more information about a particular variable - this is called
			// when you expand a Lua object to reveal its children in the debugger.
		case DebuggerServerTag::kGetChildren:
			{
				UInt32 uStackDepth = 0;
				String sPath;
				if (pMessage->Read(uStackDepth) &&
					pMessage->Read(sPath))
				{
					(void)StateLock(m_State).RequestGetChildren(*this, uStackDepth, sPath);
				}
			}
			break;

			// Set new user set breakpoints.
		case DebuggerServerTag::kSetBreakpoints:
			{
				StateLock lock(m_State);

				// Refresh script lookups.
				UInt32 uFiles = 0u;
				if (pMessage->Read(uFiles))
				{
					HString fileName;
					UInt16 uValue;
					for (UInt32 i = 0u; i < uFiles; ++i)
					{
						if (pMessage->Read(fileName) &&
							pMessage->Read(uValue))
						{
							lock.SetFileAssociation(fileName, uValue);
						}
					}
				}

				while (pMessage->HasData())
				{
					UInt32 uBreakpoint = 0u;
					Bool bEnable = false;
					if (pMessage->Read(uBreakpoint) &&
						pMessage->Read(bEnable))
					{
						if (bEnable)
						{
							lock.SetBreakpoint(uBreakpoint);
						}
						else
						{
							lock.UnsetBreakpoint(uBreakpoint);
						}
					}
				}
			}
			break;

			// The server wants to update a particular variable - this is called
			// if you edit a writeable field in the debugger stack view.
		case DebuggerServerTag::kSetVariable:
			{
				UInt32 uStackDepth = 0;
				String sPath;
				DebuggerVariableType eType = DebuggerVariableType::kNil;
				String sValue;
				if (pMessage->Read(uStackDepth) &&
					pMessage->Read(sPath) &&
					pMessage->Read(eType) &&
					pMessage->Read(sValue))
				{
					(void)StateLock(m_State).RequestSetVariable(*this, uStackDepth, sPath, eType, sValue);
				}
			}
			break;

			// Unimplemented server-to-client message received. Hitting this line will likely result in
			// a debugger exit or crash, as it will timeout waiting for a response that we never send.
		default:
			SEOUL_WARN("[SwfDebugger]: Unsupported command: %s (%u)\n", EnumToString<DebuggerServerTag>(pMessage->m_uTag), pMessage->m_uTag);
			break;
		};

		SafeDelete(pMessage);
		pMessage = m_Receive.m_Buffer.Pop();
	}
}

/** Called by the receive thread when the connection state needs to be set back to the startup state. */
void DebuggerClient::InternalReceiveThreadResetConnectionState()
{
	// Must only be called from the receive thread.
	SEOUL_ASSERT(m_Receive.m_ThreadId == Thread::GetThisThreadId());

	StateLock lock(m_State);

	// Refresh server listener state before
	// processing disconnect handling.
	RefreshDebuggerServerListening();

	// Reset SwfInfo and Script send states.
	lock.OnDisconnect();

	// Connection state reset is equivalent to a disconnect event, so release
	// any active break.
	m_BreakSignal.Activate();
}

/** Called by the receive thread to get the next message out of the network stream, returns false on a failure. */
Bool DebuggerClient::ThreadReceive()
{
	// Must only be called from the receive thread.
	SEOUL_ASSERT(m_Receive.m_ThreadId == Thread::GetThisThreadId());

	// Read the next message - if this times out, we'll get a nullptr value from Message::Create().
	auto pMessage = Message::Create(m_Stream);
	if (nullptr == pMessage)
	{
		return false;
	}

	// If verbose logging is enabled, output the type of message received.
	if (m_bVerboseLogging)
	{
		SEOUL_LOG("[SwfDebugger]: Receive Message: %s\n", EnumToString<DebuggerServerTag>(pMessage->m_uTag));
	}

	// These message from the server affect the break state of the client - if any of them are received,
	// update the execute state and activate the break signal.
	if (pMessage->m_uTag == (UInt32)DebuggerServerTag::kBreak ||
		pMessage->m_uTag == (UInt32)DebuggerServerTag::kContinue ||
		pMessage->m_uTag == (UInt32)DebuggerServerTag::kStepInto ||
		pMessage->m_uTag == (UInt32)DebuggerServerTag::kStepOut ||
		pMessage->m_uTag == (UInt32)DebuggerServerTag::kStepOver)
	{
		StateLock(m_State).SetActiveVmExecuteState(ToExecuteState((DebuggerServerTag)pMessage->m_uTag));
		m_BreakSignal.Activate();
	}

	// If we fail pushing the message onto the queue, fail the operation.
	m_Receive.m_Buffer.Push(pMessage);

	// Process received messages.
	InternalPollReceive();
	SEOUL_ASSERT(m_Receive.m_Buffer.IsEmpty()); // Required for proper processing.

	return true;
}

/** Called by the send thread to push messages from the send queue to the wire. */
Bool DebuggerClient::ThreadSend(UInt32& ruSentCount)
{
	// Must only be called from the send thread.
	SEOUL_ASSERT(m_Send.m_ThreadId == Thread::GetThisThreadId());

	// Get the next message - if this returns nullptr, that's ok - it just means there are no
	// messages to send. Return true.
	auto pMessage = m_Send.m_Buffer.Pop();
	while (true)
	{
		if (nullptr == pMessage)
		{
			return true;
		}

		// If verbose logging is enabled, output the type of message sent.
		if (m_bVerboseLogging)
		{
			SEOUL_LOG("[SwfDebugger]: Send Message: %s\n", EnumToString<DebuggerClientTag>(pMessage->m_uTag));
		}

		// Send the message, free the associated memory, and return success or failure depending on the result
		// of the network operation.
		Bool const bReturn = pMessage->Send(m_Stream);
		SafeDelete(pMessage);
		if (!bReturn)
		{
			return false;
		}

		++ruSentCount;
		pMessage = m_Send.m_Buffer.Pop();
	}
}

/** Receive thread body - loops forever until shutdown. Network handshake is also managed by this thread. */
Int DebuggerClient::ReceiveThreadBody(const Thread& thread)
{
	m_Receive.m_ThreadId = Thread::GetThisThreadId();

	// Loop forever until shutdown occurs.
	while (!m_Receive.m_bShuttingDown)
	{
		// If we haven't opened a socket yet, try now.
		while (!m_Receive.m_bShuttingDown && !m_bCanSend)
		{
			// Wait on the receive signal - will be activated when debugging has started and we
			// need to try to establish a connection.
			m_Receive.m_Signal.Wait();

			// If we're still running, try to open the socket.
			if (!m_Receive.m_bShuttingDown)
			{
				// Attempt to open the connection, if the debug server is listening.
				if (!IsDebuggerServerListening() ||
					!m_Socket.Connect(Socket::kTCP, m_sServerHostname, kDebuggerPort))
				{
					// Make sure we put the connection state back to the default.
					InternalReceiveThreadResetConnectionState();
					continue;
				}

				// Success - setup the socket and let the send thread do some work.
				m_Socket.SetTCPNoDelay(true);
				m_bCanSend = true;
				m_Send.m_Signal.Activate();
			}
		}

		// Receive loop - just get receive messages until a failure or until shutdown.
		while (!m_Receive.m_bShuttingDown && ThreadReceive()) ;

		// Socket cleanup - this is also a disconnect event, so release the break signal.
		m_bCanSend = false;

		m_Socket.Shutdown();
		m_Socket.Close();
		m_Stream.Clear();
		
		// Reset state so that handshaking and the like happens again.
		InternalReceiveThreadResetConnectionState();
	}

	m_Socket.Close();
	m_Stream.Clear();

	m_Receive.m_ThreadId = ThreadId();

	return 0;
}

/** Body of the send thread - loops forever putting messages on the wire until shutdown. */
Int DebuggerClient::SendThreadBody(const Thread& thread)
{
	// We ping the server every so often to check for disconnect.
	static const UInt32 kuHeartbeatTimeInMilliseconds = 1000u;

	m_Send.m_ThreadId = Thread::GetThisThreadId();

	// Time tracking.
	auto iLastSendInTicks = SeoulTime::GetGameTimeInTicks();

	// While we're still running.
	while (!m_Send.m_bShuttingDown)
	{
		// Wait for messages to queue and/or the connection to be established.
		m_Send.m_Signal.Wait(kuHeartbeatTimeInMilliseconds);

		// Spurious wake-up, do nothing (m_bCanSend is still false). Otherwise,
		// keep sending until shutdown, or until a send failure.
		if (!m_Send.m_bShuttingDown && m_bCanSend)
		{
			UInt32 uCount = 0u;
			if (!ThreadSend(uCount))
			{
				break;
			}

			// Ping if no messages were sent and
			// we're over the heartbeat time.
			if (0u == uCount &&
				SeoulTime::ConvertTicksToMilliseconds(SeoulTime::GetGameTimeInTicks() - iLastSendInTicks) >= (Int64)kuHeartbeatTimeInMilliseconds)
			{
				EnqueueSend(Message::Create(DebuggerClientTag::kHeartbeat));
				if (!ThreadSend(uCount))
				{
					break;
				}
			}

			// Update tick.
			if (uCount > 0u)
			{
				iLastSendInTicks = SeoulTime::GetGameTimeInTicks();
			}
		}
	}

	m_Send.m_ThreadId = ThreadId();

	return 0;
}

DebuggerClient::State::State()
	: m_Mutex()
	, m_tBreakpoints()
	, m_vVms()
	, m_tScripts()
	, m_pActiveVm()
	, m_bConnectionHandshake(false)
	, m_bPendingHandleDisconnect(false)
{
}

DebuggerClient::State::~State()
{
	Lock lock(m_Mutex);
	InsideLockDestroy();
}

void DebuggerClient::State::InsideLockDestroy()
{
	// We no longer have an active VM.
	m_pActiveVm.Reset();

	// Clear lookups and breakpoint data.
	m_tScripts.Clear();
	m_tBreakpoints.Clear();

	// Delete all VM data.
	SafeDeleteVector(m_vVms);
}

/** Called by the receive thread whe we lose connection to the debugger server. */
void DebuggerClient::StateLock::OnDisconnect()
{
	m_rState.m_bPendingHandleDisconnect = true;
}

/** Called by an IggyUIMovie when a debug step event occurs - triggered when OP_debugline opcodes are evaluated. */
void DebuggerClient::StateLock::OnStep(
	DebuggerClient& r, 
	CheckedPtr<lua_State> pLuaVm,
	CheckedPtr<lua_Debug> pDebugInfo)
{
	// If a disconnect is pending, check all movies stack. If all are at 0, perform disconnect handling.
	if (m_rState.m_bPendingHandleDisconnect)
	{
		// If we get here, we can perform disconnect handling.
		InternalDoDisconnectCleanup(r);
		return;
	}

	// Early out if we're not in a send state and the debugger server is not listening.
	if (!r.m_bCanSend)
	{
		if (!r.IsDebuggerServerListening())
		{
			return;
		}
	}

	// Set the active VM based on pLuaVm - this must succeed, or something crazy happened with this VM.
	SetActiveVm(r, pLuaVm);
	SEOUL_ASSERT(m_rState.m_pActiveVm);
	auto pActiveVm = m_rState.m_pActiveVm;

	// Get break info for the current stack frame.
	BreakInfo info;
	m_rState.ToBreakpointUnpopulated(r, *pActiveVm, pLuaVm, pDebugInfo, info);

	// Check if the current breakpoint is set as a user-defined breakpoint.
	Bool bHasBreakpoint = m_rState.m_tBreakpoints.HasValue(info.m_uBreakpoint);

	// Filter - we don't want to hit the same breakpoint twice if the breakpoint
	// is at a function call.
	if (bHasBreakpoint)
	{
		// If we haven't clear the last breakpoint, check if it's the same - if so,
		// don't hit it a second time.
		if (pActiveVm->m_iStepStackFrames >= 0)
		{
			auto const iStackFrames = GetStackDepth(pLuaVm);
			if (iStackFrames == pActiveVm->m_iStepStackFrames &&
				pActiveVm->m_StepBreakInfo.m_uBreakpoint == info.m_uBreakpoint)
			{
				// Filter, same breakpoint.
				bHasBreakpoint = false;
			}
		}
	}
	// Clear recorded break info once we hit a lower stack depth,
	// or we're at the same but a different breakpoint.
	else if (pActiveVm->m_iStepStackFrames >= 0)
	{
		auto const iStackFrames = GetStackDepth(pLuaVm);
		if (iStackFrames < pActiveVm->m_iStepStackFrames ||
			(iStackFrames == pActiveVm->m_iStepStackFrames &&
				pActiveVm->m_StepBreakInfo.m_uBreakpoint != info.m_uBreakpoint))
		{
			pActiveVm->m_StepBreakInfo = BreakInfo();
		}
	}

	// NOTE that InternalBreak() internally and temporarily releases the state lock, so
	// don't make assumptions about the state of State after this method
	// returns.

	// Check if we need to break on step mode.

	// Cache variables to compute whether break is necessary or not.
	auto const eExecuteState = pActiveVm->m_eExecuteState;
	Int const iStepStackFrames = pActiveVm->m_iStepStackFrames;
	auto const& stepBreakInfo = pActiveVm->m_StepBreakInfo;

	switch (eExecuteState)
	{
		// In step into, always break on the next file-line event.
	case DebuggerExecuteState::kStepInto:
		InternalBreak(r, *pActiveVm, pLuaVm, SuspendReason::kStep);
		break;

		// On step out, break on the first file-line event at a stack frame index lower than the index
		// that the step started at.
	case DebuggerExecuteState::kStepOut:
	{
		auto const iCurrentStackFrames = GetStackDepth(pLuaVm);
		if (iStepStackFrames > iCurrentStackFrames)
		{
			InternalBreak(r, *pActiveVm, pLuaVm, SuspendReason::kStep);
		}
		break;
	}

		// On step over, break at the next file-line event at or beyond the frame index that the step
		// started at.
	case DebuggerExecuteState::kStepOver:
	{
		// The second comparison (stepBreakpoint != current)
		// is to filter double hits on exit functions.
		auto const iCurrentStackFrames = GetStackDepth(pLuaVm);
		if (iStepStackFrames >= iCurrentStackFrames)
		{
			BreakInfo currentBreakInfo;
			m_rState.GetCurrentBreakInfo(r, *pActiveVm, pLuaVm, currentBreakInfo);
			if (currentBreakInfo.m_uBreakpoint != stepBreakInfo.m_uBreakpoint)
			{
				InternalBreak(r, *pActiveVm, pLuaVm, SuspendReason::kStep);
			}
		}
		break;
	}

		// No break if we're in running mode.
	case DebuggerExecuteState::kRunning: // fall-through
	default:
		// If we have a user-defined breakpoint, break on it.
		if (bHasBreakpoint)
		{
			InternalBreak(r, *pActiveVm, pLuaVm, SuspendReason::kBreakpoint);
		}
		break;
	};
}

/** Does the work triggered by Script::DebuggerClient::OnVmDestroy(). */
void DebuggerClient::StateLock::OnVmDestroy(DebuggerClient& r, CheckedPtr<lua_State> pLuaVm)
{
	for (auto i = m_rState.m_vVms.Begin(), iEnd = m_rState.m_vVms.End(); iEnd != i; ++i)
	{
		auto p = *i;
		if (p->m_pVm == pLuaVm)
		{
			if (p == m_rState.m_pActiveVm)
			{
				m_rState.m_pActiveVm.Reset();
			}

			i = m_rState.m_vVms.Erase(i);
			SafeDelete(p);
			return;
		}
	}
}

/** Called to send the client-to-server response to the server-to-client GetFrame message. */
Bool DebuggerClient::StateLock::RequestGetStackFrame(DebuggerClient& r, UInt32 uDepth)
{
	if (!m_rState.m_pActiveVm)
	{
		return false;
	}

	m_rState.m_pActiveVm->m_iPendingGetStackFrame = (Int)uDepth;
	r.m_BreakSignal.Activate();
	return true;
}

/** Called by the receive thread to schedule a client-to-server GetChildren - this must be handled on the thread with actively running Lua. */
Bool DebuggerClient::StateLock::RequestGetChildren(DebuggerClient& r, UInt32 uStackDepth, const String& sPath)
{
	if (!m_rState.m_pActiveVm.IsValid())
	{
		return false;
	}

	// Populate the m_PendingGetChildren member of the active movie and then activate the break signal -
	// this will let the thread that is running Lua to release its break, send the variable information,
	// and then reacquire the break.
	m_rState.m_pActiveVm->m_PendingGetChildren.m_uStackDepth = uStackDepth;
	m_rState.m_pActiveVm->m_PendingGetChildren.m_sPath = sPath;
	r.m_BreakSignal.Activate();
	return true;
}

/** Called by the receive thread to schedule a client-to-server SetVariable - this must be handled on the thread with actively running Lua. */
Bool DebuggerClient::StateLock::RequestSetVariable(DebuggerClient& r, UInt32 uStackDepth, const String& sPath, DebuggerVariableType eType, const String& sValue)
{
	if (!m_rState.m_pActiveVm.IsValid())
	{
		return false;
	}

	// Populate the m_PendingSetVariable member of the active movie and then activate the break signal -
	// this will let the thread that is running Lua to release its break, send the variable information,
	// and then reacquire the break.
	m_rState.m_pActiveVm->m_PendingSetVariable.m_uStackDepth = uStackDepth;
	m_rState.m_pActiveVm->m_PendingSetVariable.m_sPath = sPath;
	m_rState.m_pActiveVm->m_PendingSetVariable.m_eType = eType;
	m_rState.m_pActiveVm->m_PendingSetVariable.m_sValue = sValue;
	r.m_BreakSignal.Activate();
	return true;
}

/** Get or lazily create active VM data. */
void DebuggerClient::StateLock::SetActiveVm(DebuggerClient& r, CheckedPtr<lua_State> pLuaVm)
{
	// Create or get an existing entry.
	auto pEntry = m_rState.FindVm(pLuaVm);
	if (nullptr == pEntry)
	{
		pEntry.Reset(SEOUL_NEW(MemoryBudgets::UIDebug) VmEntry);
		pEntry->m_pVm = pLuaVm;
		m_rState.m_vVms.PushBack(pEntry);
	}

	// Update the active.
	m_rState.m_pActiveVm = pEntry;

	// Connection handshake is delayed until we set the first active VM. Connect now
	// if necessary.
	if (!m_rState.m_bConnectionHandshake)
	{
		// Track.
		m_rState.m_bConnectionHandshake = true;

		// Startup handshake
		(void)r.EnqueueSend(Message::CreateClientVersion());               // Report the debugger protocol version.
		(void)r.EnqueueSend(Message::CreateClientAskBreakpoints());        // Ask the server for any currently set breakpoints.

		// Synchronize with the server.
		InternalSync(r);
	}
}

/** Called to break at a breakpoint (either user defined, due to a step, or other reason, such as a halt). */
void DebuggerClient::StateLock::InternalBreak(
	DebuggerClient& r,
	VmEntry& rVmEntry,
	CheckedPtr<lua_State> pLuaVm,
	SuspendReason eSuspendReason)
{
	// Get the movie - don't break if we don't have one.
	CheckedPtr<VmEntry> pActiveVm = m_rState.m_pActiveVm;
	if (!pActiveVm.IsValid() || pActiveVm->m_pVm != pLuaVm)
	{
		return;
	}

	// Wait with a 0 timeout to clear any "dangling" activate,
	// this can happen if the server sends two messages that
	// both clear the break.
	(void)r.m_BreakSignal.Wait(0);

	// Breaks involve a client-to-server send of BreakAt.
	r.EnqueueSend(m_rState.CreateClientBreakAt(r, rVmEntry, pLuaVm, eSuspendReason));

	// Prior to entering break for the first time, set the break execution state.
	pActiveVm->m_eExecuteState = pActiveVm->m_ePendingExecuteState = DebuggerExecuteState::kBreak;
	pActiveVm->m_iStepStackFrames = GetStackDepth(pLuaVm);
	m_rState.GetCurrentBreakInfo(r, *pActiveVm, pLuaVm, pActiveVm->m_StepBreakInfo);

	// A break can be released in order to allow this thread to respond to GetChildren() requests
	// from the debugger server - we only want to release the break for real if the break
	// signal was activated and no pending GetChildren was set.
	do
	{
		// If there is a pending GetStackFrame request, process it and then clear it.
		if (pActiveVm->m_iPendingGetStackFrame >= 0)
		{
			(void)r.EnqueueSend(Message::CreateClientFrame(
				pLuaVm,
				(UInt32)pActiveVm->m_iPendingGetStackFrame));
			pActiveVm->m_iPendingGetStackFrame = -1;
		}

		// If there is a pending GetChildren request, process it and then clear it.
		if (pActiveVm->m_PendingGetChildren.IsValid())
		{
			(void)r.EnqueueSend(Message::CreateClientGetChildren(
				pActiveVm->m_pVm,
				pActiveVm->m_PendingGetChildren.m_uStackDepth,
				pActiveVm->m_PendingGetChildren.m_sPath));
			pActiveVm->m_PendingGetChildren.Reset();
		}

		// If there is a pending SetVariable request, process it and then clear it.
		if (pActiveVm->m_PendingSetVariable.IsValid())
		{
			(void)r.EnqueueSend(Message::CreateClientSetVariable(
				pLuaVm,
				pActiveVm->m_PendingSetVariable.m_uStackDepth,
				pActiveVm->m_PendingSetVariable.m_sPath, 
				pActiveVm->m_PendingSetVariable.m_eType,
				pActiveVm->m_PendingSetVariable.m_sValue));

			pActiveVm->m_PendingSetVariable.Reset();
		}

		// If we need to update the execution state, do so now.
		if (pActiveVm->m_ePendingExecuteState != pActiveVm->m_eExecuteState)
		{
			pActiveVm->m_eExecuteState = pActiveVm->m_ePendingExecuteState;
			pActiveVm->m_iStepStackFrames = GetStackDepth(pLuaVm);
			m_rState.GetCurrentBreakInfo(r, *pActiveVm, pLuaVm, pActiveVm->m_StepBreakInfo);
		}

		// Release the state lock while we're waiting on the break.
		m_rState.m_Mutex.Unlock();

		// Only break here if we're (still or originally) in the break state.
		if (pActiveVm->m_eExecuteState == DebuggerExecuteState::kBreak)
		{
			// Wake up the receive thread if it is not already running.
			r.m_Receive.m_Signal.Activate();

			// Break - receiver thread will activate this signal if something releases the break.
			r.m_BreakSignal.Wait();
		}

		// Reacquire the state lock before leaving the function.
		m_rState.m_Mutex.Lock();

		// Keep looping as long as there is a pending GetChildren or SetVariable request, and as
		// long as a disconnect did not occur.
	} while (!m_rState.m_bPendingHandleDisconnect && (
		(pActiveVm->m_iPendingGetStackFrame >= 0) ||
		pActiveVm->m_PendingGetChildren.IsValid() ||
		pActiveVm->m_PendingSetVariable.IsValid() ||
		pActiveVm->m_ePendingExecuteState != pActiveVm->m_eExecuteState));
}

/** Called when execution leaves the debugger to actually perform disconnect handling. */
void DebuggerClient::StateLock::InternalDoDisconnectCleanup(DebuggerClient& r)
{
	// Nop if a disconnect is not pending.
	if (!m_rState.m_bPendingHandleDisconnect)
	{
		return;
	}

	// No longer a pending disconnect.
	m_rState.m_bPendingHandleDisconnect = false;

	// Flush receive and send buffers.
	r.InternalSafeDeleteAllBufferContents();

	// State is entirely flushed on disconnect (need to connect
	// again to establish breakpoints and lookups).
	m_rState.InsideLockDestroy();

	// No longer have a handshake.
	m_rState.m_bConnectionHandshake = false;
}

/**
 * Similar to break, but specifically to give the client a chance to synchronize with the server
 * (client break's until server sends a continue message).
 */
void DebuggerClient::StateLock::InternalSync(DebuggerClient& r)
{
	// Wait with a 0 timeout to clear any "dangling" activate,
	// this can happen if the server sends two messages that
	// both clear the break.
	(void)r.m_BreakSignal.Wait(0);

	// Sync the client to the server.
	r.EnqueueSend(Message::Create(DebuggerClientTag::kSync));

	// Release the state lock while we're waiting on the break.
	m_rState.m_Mutex.Unlock();

	// Wake up the receive thread if it is not already running.
	r.m_Receive.m_Signal.Activate();

	// Break - receiver thread will activate this signal if something releases the break.
	r.m_BreakSignal.Wait();
	
	// Reacquire the state lock before leaving the function.
	m_rState.m_Mutex.Lock();
}

} // namespace Script

#endif // /#if SEOUL_ENABLE_DEBUGGER_CLIENT

} // namespace Seoul
