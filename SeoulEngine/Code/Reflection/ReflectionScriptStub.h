/**
 * \file ReflectionScriptStub.h
 * \brief Defines nop script bodies for projects that
 * don't use SeoulEngine's script VMs. Include this
 * file in 1 .cpp file in your root application project.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef REFLECTION_SCRIPT_STUB_H
#define REFLECTION_SCRIPT_STUB_H

#include "ReflectionScript.h"
extern "C" { void lua_call(lua_State*, int, int) {} }
extern "C" { void lua_createtable(lua_State*, int, int) {} }
extern "C" { extern int luaL_error(lua_State*, const char*,...) { return 0; } }
extern "C" { int lua_toboolean(lua_State*, int) { return 0; } }
extern "C" { lua_Integer lua_tointeger(lua_State*, int) { return 0; } }
extern "C" { lua_Number lua_tonumber(lua_State*, int) { return (lua_Number)0; } }
extern "C" { const char* lua_tolstring(lua_State*, int, size_t*) { return ""; } }
extern "C" { void lua_getfield(lua_State*,int,const char*) {} }
extern "C" { int lua_gettop(lua_State*) { return 0; } }
extern "C" { void lua_insert(lua_State*, int) {} }
extern "C" { int lua_isnumber(lua_State*, int) { return 0; } }
extern "C" { int lua_isstring(lua_State*, int) { return 0; } }
extern "C" { void* lua_newuserdata(lua_State*, size_t) { return nullptr; } }
extern "C" { int lua_next(lua_State*, int) { return 0; } }
extern "C" { int lua_pcall(lua_State*, int, int, int) { return 0; } }
extern "C" { void lua_pushboolean(lua_State*, int) {} }
extern "C" { void lua_pushcclosure(lua_State*, lua_CFunction, int) {} }
extern "C" { void lua_pushinteger(lua_State*, lua_Integer) {} }
extern "C" { void lua_pushlightuserdata(lua_State*, void*) {} }
extern "C" { void lua_pushlstring(lua_State*, const char*, size_t) {} }
extern "C" { void lua_pushnil(lua_State*) {} }
extern "C" { void lua_pushstring(lua_State*, const char*) {} }
extern "C" { void lua_pushnumber(lua_State*, lua_Number) {} }
extern "C" { void lua_pushvalue(lua_State*, int) {} }
extern "C" { size_t lua_objlen(lua_State*, int) { return 0; } }
extern "C" { void lua_rawget(lua_State*, int) {} }
extern "C" { void lua_rawgeti(lua_State*, int, int) {} }
extern "C" { void lua_rawset(lua_State*, int) {} }
extern "C" { void lua_rawseti(lua_State*, int, int) {} }
extern "C" { void lua_remove(lua_State*, int) {} }
extern "C" { void lua_setfield(lua_State*, int, const char*) {} }
extern "C" { int lua_setmetatable(lua_State*,int) { return 1; } }
extern "C" { int lua_setfenv(lua_State*, int) { return 1; } }
extern "C" { void lua_settable(lua_State*, int) {} }
extern "C" { void lua_settop(lua_State*, int) {} }
extern "C" { void* lua_touserdata(lua_State*, int) { return nullptr; } }
extern "C" { int lua_type(lua_State*, int) { return 0; } }

extern "C" { extern int luaL_ref(lua_State*, int) { return 0; } }

namespace Seoul
{

void LuaGetMetatable(lua_State* pLuaVm, const Reflection::Type& type, Bool bWeak)
{
}

void seoul_lua_createclasstable(lua_State*, const char*, int, int)
{
}

} // namespace Seoul

#endif // include guard
