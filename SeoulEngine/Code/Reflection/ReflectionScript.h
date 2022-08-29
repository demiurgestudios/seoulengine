/**
 * \file ReflectionScript.h
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef REFLECTION_SCRIPT_H
#define REFLECTION_SCRIPT_H

#include "Prereqs.h"
namespace Seoul { namespace Reflection { class Type; } }

extern "C" { struct lua_State; }
extern "C" { typedef int (*lua_CFunction)(lua_State*); }
extern "C" { typedef double lua_Number; }
extern "C" { typedef ptrdiff_t lua_Integer; }

extern "C" { extern void lua_call(lua_State*,int,int); }
extern "C" { extern void lua_createtable(lua_State*, int, int); }
extern "C" { extern int luaL_error(lua_State*, const char*,...); }
extern "C" { extern int lua_toboolean(lua_State*, int); }
extern "C" { extern lua_Integer lua_tointeger(lua_State*, int); }
extern "C" { extern lua_Number lua_tonumber(lua_State*, int); }
extern "C" { extern const char* lua_tolstring(lua_State*, int, size_t*); }
extern "C" { extern void lua_getfield(lua_State*,int,const char*); }
extern "C" { extern int lua_gettop(lua_State*); }
extern "C" { extern void lua_insert(lua_State*, int); }
extern "C" { extern int lua_isnumber(lua_State*, int); }
extern "C" { extern int lua_isstring(lua_State*, int); }
extern "C" { extern void* lua_newuserdata(lua_State*, size_t); }
extern "C" { extern int lua_next(lua_State*, int); }
extern "C" { extern size_t lua_objlen(lua_State*, int); }
extern "C" { extern int lua_pcall(lua_State*, int, int, int); }
extern "C" { extern void lua_pushboolean(lua_State*, int); }
extern "C" { extern void lua_pushcclosure(lua_State*, lua_CFunction, int); }
extern "C" { extern void lua_pushinteger(lua_State*, lua_Integer); }
extern "C" { extern void lua_pushlightuserdata(lua_State*, void*); }
extern "C" { extern void lua_pushlstring(lua_State*, const char*, size_t); }
extern "C" { extern void lua_pushnil(lua_State*); }
extern "C" { extern void lua_pushstring(lua_State*, const char*); }
extern "C" { extern void lua_pushnumber(lua_State*, lua_Number); }
extern "C" { extern void lua_pushvalue(lua_State*, int); }
extern "C" { inline size_t lua_rawlen(lua_State* L, int i) { return lua_objlen(L, i); } }
extern "C" { extern void lua_rawget(lua_State*, int); }
extern "C" { extern void lua_rawgeti(lua_State*, int, int); }
extern "C" { extern void lua_rawset(lua_State*, int); }
extern "C" { extern void lua_rawseti(lua_State*, int, int); }
extern "C" { extern void lua_remove(lua_State*, int); }
extern "C" { extern void lua_setfield(lua_State*, int, const char*); }
extern "C" { extern int lua_setmetatable(lua_State*,int); }
extern "C" { extern int lua_setfenv(lua_State*, int); }
extern "C" { extern void lua_settable(lua_State*, int); }
extern "C" { extern void lua_settop(lua_State*, int); }
extern "C" { extern void* lua_touserdata(lua_State*, int); }
extern "C" { extern int lua_type(lua_State*, int); }

extern "C" { extern int luaL_ref(lua_State*, int); }

#ifndef LUAI_MAXSTACK
#define LUAI_MAXSTACK 65500
#else
SEOUL_STATIC_ASSERT(65500 == LUAI_MAXSTACK);
#endif

#ifndef LUAI_FIRSTPSEUDOIDX
#define LUAI_FIRSTPSEUDOIDX	(-LUAI_MAXSTACK - 1000)
#else
SEOUL_STATIC_ASSERT((-LUAI_MAXSTACK - 1000) == LUAI_FIRSTPSEUDOIDX);
#endif

#ifndef LUA_GLOBALSINDEX
#define LUA_GLOBALSINDEX (-10002)
#else
SEOUL_STATIC_ASSERT((-10002) == LUA_GLOBALSINDEX);
#endif

#ifndef LUA_REGISTRYINDEX
#define LUA_REGISTRYINDEX (-10000)
#else
SEOUL_STATIC_ASSERT((-10000) == LUA_REGISTRYINDEX);
#endif

#ifndef LUA_TNONE
#define LUA_TNONE (-1)
#else
SEOUL_STATIC_ASSERT(-1 == LUA_TNONE);
#endif

#ifndef LUA_TNIL
#define LUA_TNIL 0
#else
SEOUL_STATIC_ASSERT(0 == LUA_TNIL);
#endif

#ifndef LUA_TBOOLEAN
#define LUA_TBOOLEAN 1
#else
SEOUL_STATIC_ASSERT(1 == LUA_TBOOLEAN);
#endif

#ifndef LUA_TLIGHTUSERDATA
#define LUA_TLIGHTUSERDATA 2
#else
SEOUL_STATIC_ASSERT(2 == LUA_TLIGHTUSERDATA);
#endif

#ifndef LUA_TNUMBER
#define LUA_TNUMBER 3
#else
SEOUL_STATIC_ASSERT(3 == LUA_TNUMBER);
#endif

#ifndef LUA_TSTRING
#define LUA_TSTRING 4
#else
SEOUL_STATIC_ASSERT(4 == LUA_TSTRING);
#endif

#ifndef LUA_TTABLE
#define LUA_TTABLE 5
#else
SEOUL_STATIC_ASSERT(5 == LUA_TTABLE);
#endif

#ifndef LUA_TFUNCTION
#define LUA_TFUNCTION 6
#else
SEOUL_STATIC_ASSERT(6 == LUA_TFUNCTION);
#endif

#ifndef LUA_TUSERDATA
#define LUA_TUSERDATA 7
#else
SEOUL_STATIC_ASSERT(7 == LUA_TUSERDATA);
#endif

#ifndef LUA_TTHREAD
#define LUA_TTHREAD 8
#else
SEOUL_STATIC_ASSERT(8 == LUA_TTHREAD);
#endif

#ifndef lua_isnil
#define lua_isnil(L,n) (lua_type(L, (n)) == LUA_TNIL)
#endif

#ifndef lua_pop
#define lua_pop(L,n) lua_settop(L, -(n)-1)
#endif

namespace Seoul
{

void LuaGetMetatable(lua_State* pLuaVm, const Reflection::Type& type, Bool bWeak);
void seoul_lua_createclasstable(lua_State* L, const char*, int, int);

} // namespace Seoul

#endif // include guard
