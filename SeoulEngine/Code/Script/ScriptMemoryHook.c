/**
 * \file ScriptMemoryHook.c
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include <stdint.h> /* for bit width detection. */

/* common settings for our integration of LuaJIT. */
#define LUAJIT_USE_SYSMALLOC 1
#define LUAJIT_ENABLE_LUA52COMPAT 1
#define LUAJIT_DISABLE_JIT 1
#define LUAJIT_DISABLE_FFI 1
#if UINTPTR_MAX == 0xFFFFFFFFFFFFFFFF /* 64-bit */
#	define LUAJIT_ENABLE_GC64 1 /* always true with SeoulEngine integration on 64-bit platforms. */
#endif
#include <lj_obj.h>

/* conditionals must match curr_funcisL() */
#if LJ_GC64
#define seoul_isfunc(L)     tvisfunc(L->base-2)
#elif LJ_FR2
#define seoul_isfunc(L)     tvisfunc(L->base-2)
#else
#define seoul_isfunc(L)     tvisfunc(L->base-1)
#endif

/** @return the current Lua function pointer or NULL if not in a Lua function context (immediate). */
void* SeoulLuaHookGetFuncPtr(lua_State* L)
{
	return (seoul_isfunc(L) && curr_funcisL(L)) ? curr_proto(L) : NULL;
}

/** Output data about the given opaque Lua function pointer - return 0 on failure, 1 on success. */
int SeoulLuaHookGetFuncInfo(lua_State* L, void* pFunc, const char** psName, int* piLine)
{
	GCproto* pProto = pFunc;
	if (pProto->numline < 1)
	{
		return 0;
	}

	if (NULL != psName)
	{
		*psName = proto_chunknamestr(pProto);
	}
	if (NULL != piLine)
	{
		*piLine = pProto->firstline;
	}

	return 1;
}
