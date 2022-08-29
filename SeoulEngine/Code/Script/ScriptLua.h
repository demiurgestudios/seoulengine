/**
 * \file ScriptLua.h
 * \brief Wrapper around the standard lua.h header.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCRIPT_LUA_H
#define SCRIPT_LUA_H

extern "C"
{

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

// Remove from luajit, redefined here.
#define lua_ref(L,lock) ((lock) ? luaL_ref(L, LUA_REGISTRYINDEX) : \
      (lua_pushstring(L, "unlocked references are obsolete"), lua_error(L), 0))
#define lua_unref(L,ref)        luaL_unref(L, LUA_REGISTRYINDEX, (ref))
#define lua_getref(L,ref)       lua_rawgeti(L, LUA_REGISTRYINDEX, (ref))
#define luaL_reg	luaL_Reg

} // extern "C"

#endif // include guard
