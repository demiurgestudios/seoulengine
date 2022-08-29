/**
 * \file Script::Vm.cpp
 * \brief SeoulEngine wrapper around a Lua scripting language virtual machine.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "BuildChangelistPublic.h"
#include "BuildDistroPublic.h"
#include "BuildVersion.h"
#include "CompilerSettings.h"
#include "Compress.h"
#include "ContentHandle.h"
#include "ContentLoadManager.h"
#include "CookManager.h"
#include "CrashManager.h"
#include "DataStoreParser.h"
#include "Engine.h"
#include "FileManager.h"
#include "FromString.h"
#include "EventsManager.h"
#include "GamePaths.h"
#include "LocManager.h"
#include "Logger.h"
#include "Path.h"
#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionDefine.h"
#include "ReflectionMethod.h"
#include "ReflectionMethodTypeInfo.h"
#include "ReflectionRegistry.h"
#include "ReflectionType.h"
#include "ScriptContentLoader.h"
#include "ScriptDebuggerClient.h"
#include "ScriptFunctionInterface.h"
#include "ScriptFunctionInvoker.h"
#include "ScriptLua.h"
#include "ScriptManager.h"
#include "ScriptProtobuf.h"
#include "ScriptUtils.h"
#include "ScriptVm.h"
#include "SeoulFile.h"
#include "SeoulMath.h"
#include "SeoulProfiler.h"
#include "SettingsManager.h"

extern "C"
{

LUALIB_API int luaopen_pb_conv(lua_State *L);
LUALIB_API int luaopen_pb_io(lua_State *L);
LUALIB_API int luaopen_pb_buffer(lua_State *L);
LUALIB_API int luaopen_pb_slice(lua_State *L);

void lua_pushglobaltable(lua_State* L)
{
	lua_getglobal(L, "_G");
}

#if SEOUL_ENABLE_MEMORY_TOOLING
void* SeoulLuaHookGetFuncPtr(lua_State* L);
int SeoulLuaHookGetFuncInfo(lua_State* L, void* pFunc, const char** psName, int* piLine);
#endif // /SEOUL_ENABLE_MEMORY_TOOLING

} // extern "C"

namespace Seoul
{

SEOUL_TYPE(Script::VmObject, TypeFlags::kDisableNew);

SEOUL_TYPE(Script::Vm, TypeFlags::kDisableNew);

SEOUL_TYPE(Script::ByteBuffer)

SEOUL_SPEC_TEMPLATE_TYPE(SharedPtr<Script::VmObject>)

namespace Script
{

static const size_t kLuaReadSize = 8192;
static const HString kFunctionSeoulDispose("SeoulDispose");
static const HString kFunctionSeoulOnHotload("SeoulOnHotload");
static const HString kFunctionSeoulPostHotload("SeoulPostHotload");
static const HString kFunctionRestoreDynamicGameStateData("RestoreDynamicGameStateData");

#if SEOUL_HOT_LOADING
static Vm::HotLoadData* GetScriptVmHotLoadData(lua_State* pLuaVm)
{
	lua_pushlightuserdata(pLuaVm, kpScriptVmHotLoadDataKey);
	lua_rawget(pLuaVm, LUA_REGISTRYINDEX);

	Vm::HotLoadData* pReturn = (Vm::HotLoadData*)lua_touserdata(pLuaVm, -1);

	lua_pop(pLuaVm, 1);

	return pReturn;
}
#endif // /#if SEOUL_HOT_LOADING

static void ReportError(lua_State* pLuaVm, char const* sErrorMessage)
{
	SEOUL_SCRIPT_CHECK_VM_STACK(pLuaVm);

	lua_getglobal(pLuaVm, "print");
	lua_pushstring(pLuaVm, sErrorMessage);
	Bool const bResult = (0 == lua_pcall(pLuaVm, 1, 0, 0));
	if (!bResult)
	{
		// pop the error message.
		lua_pop(pLuaVm, 1);
	}
}

/** Convert a FilePath to a string to specify a Lua chunk name - used for debugging and stack traces. */
inline static String ToLuaChunkName(lua_State* pLuaVm, FilePath filePath)
{
	// Resolve the filePath to an absolute filename, then
	// test it against all base paths.
	String const sAbsoluteFilenameInSource(filePath.GetAbsoluteFilenameInSource());
	auto pVm = GetScriptVm(pLuaVm);

	// Iterate and search.
	UInt32 uBasePathOffset = String::NPos;
	String const* psBasePath = nullptr;
	for (auto const& sBasePath : pVm->GetSettings().m_vBasePaths)
	{
		uBasePathOffset = sAbsoluteFilenameInSource.Find(sBasePath);
		if (String::NPos != uBasePathOffset)
		{
			psBasePath = &sBasePath;
			break;
		}
	}

	// If for some reason the base path is not contained within
	// the absolute filename, just use the absolute filename.
	if (uBasePathOffset == String::NPos ||
		nullptr == psBasePath)
	{
		return "@" + sAbsoluteFilenameInSource;
	}
	// Otherwise, make the absolute filename relative to the base path part.
	else
	{
		// Substring on the base path part, and then remove a leading directory
		// separator if present.
		String sRelativePath(sAbsoluteFilenameInSource.Substring(uBasePathOffset + psBasePath->GetSize()));
		if (sRelativePath.StartsWith(Path::DirectorySeparatorChar()))
		{
			sRelativePath = sRelativePath.Substring(1u);
		}

		return "@" + sRelativePath;
	}
}

struct LuaReadContext
{
	LuaReadContext(FileBody& rScript)
		: m_pData(rScript.GetDataPtr())
		, m_zDataSizeInBytes(rScript.GetDataSizeInBytes())
		, m_zCurrentOffset(0)
	{
	}

	LuaReadContext(const String& sCode)
		: m_pData((void const*)sCode.CStr())
		, m_zDataSizeInBytes((UInt32)sCode.GetSize())
		, m_zCurrentOffset(0)
	{
	}

	void const* const m_pData;
	UInt32 const m_zDataSizeInBytes;
	size_t m_zCurrentOffset;
}; // struct LuaReadContext

} // namespace Script

} // namespace Seoul

extern "C"
{

#if SEOUL_ENABLE_MEMORY_TOOLING
void* Seoul::Script::Vm::LuaMemoryAllocWithTooling(void* ud, void* ptr, size_t osize, size_t nsize)
{
	using namespace Seoul;

	// State.
	void* pFunc = nullptr;
	auto& r = *((Script::Vm*)ud);
	auto pLuaVm = r.m_pLuaVm;
	auto& rt = r.m_tMemory;

	// Deallocation.
	if (0 == nsize)
	{
		if (nullptr != ptr)
		{
			// Retrieve existing data.
			pFunc = *((void**)((Byte*)ptr + osize));
			// Also, in case this deallocation is itself a function prototype,
			// erase it.
			(void)rt.Erase(ptr);
		}

		// Free the actual memory.
		MemoryManager::Deallocate(ptr);

		// Now correct for the memory just deallocated.
		if (nullptr != ptr && nullptr != pFunc)
		{
			auto pi = rt.Find(pFunc);
			if (nullptr != pi)
			{
				*pi -= (ptrdiff_t)osize;
			}
		}

		return nullptr;
	}
	// Allocation.
	else
	{
		// Either query the entry from existing or create new.
		if (0 != osize)
		{
			pFunc = *((void**)((Byte*)ptr + osize));
		}
		else if (nullptr != pLuaVm)
		{
			pFunc = SeoulLuaHookGetFuncPtr(pLuaVm);
		}

		// Allocate, and then store the reference at the end of the block.
		auto p = MemoryManager::Reallocate(ptr, nsize + sizeof(void*), MemoryBudgets::Scripting);
		*((void**)((Byte*)p + nsize)) = pFunc;

		// Query.
		auto pi = rt.Find(pFunc);

		// Existing entry, update.
		if (nullptr == pi && nullptr != pFunc)
		{
			// Otherwise, create a new entry.
			pi = &rt.Insert(pFunc, 0).First->Second;
		}
		// If we found an entry to update, apply the delta of the new size vs. the old size.
		if (nullptr != pi)
		{
			*pi += ((ptrdiff_t)nsize - (ptrdiff_t)osize);
		}

		return p;
	}
}
#endif // /SEOUL_ENABLE_MEMORY_TOOLING

void* Seoul::Script::Vm::LuaMemoryAlloc(void* ud, void* ptr, size_t osize, size_t nsize)
{
	using namespace Seoul;

	if (0 == nsize)
	{
		MemoryManager::Deallocate(ptr);
		return nullptr;
	}
	else
	{
		return MemoryManager::Reallocate(ptr, nsize, MemoryBudgets::Scripting);
	}
}

static int LuaAtPanic(lua_State* pLuaVm)
{
#if !SEOUL_ASSERTIONS_DISABLED
	SEOUL_FAIL("Critical Lua failure.");
#else
	// Trigger a segmentation fault explicitly.
	(*((unsigned int volatile*)nullptr)) = 1;
#endif

	return 0;
}

static const char* LuaRead(lua_State* pLuaVm, void* pUserData, size_t* rzSizeInBytes)
{
	using namespace Seoul;

	Script::LuaReadContext* pContext = (Script::LuaReadContext*)pUserData;

	size_t zSize = Min((size_t)pContext->m_zDataSizeInBytes - pContext->m_zCurrentOffset, Script::kLuaReadSize);
	const char* sReturn = (0 == zSize ? nullptr : ((const char*)pContext->m_pData + pContext->m_zCurrentOffset));
	pContext->m_zCurrentOffset += zSize;

	*rzSizeInBytes = zSize;
	return sReturn;
}
// TODO: Lazy binding here should not be necessary, revisit.
static const luaL_Reg s_kaLuaLazyBindModules[] =
{
	// Google protocol buffer in Lua support.
	{ "pb.conv", luaopen_pb_conv },
	{ "pb.io", luaopen_pb_io },
	{ "pb.buffer", luaopen_pb_buffer },
	{ "pb.slice", luaopen_pb_slice },
};

int LuaLoaderSeoul(lua_State* pLuaVm)
{
	using namespace Seoul;

	// luaL_checkstring() will trigger a longjmp on error, don't
	// use C++ types with destructors prior to this call.
	size_t zNameLengthInBytes = 0;
	const char* sName = luaL_checklstring(pLuaVm, 1, &zNameLengthInBytes);

	// Early out for "os" module.
	if (0 == strcmp("os", sName))
	{
		luaL_error(
			pLuaVm,
			"error loading module " LUA_QS ":\n\tfile not found or error reading file.",
			lua_tostring(pLuaVm, 1));
		return 0;
	}

	for (size_t i = 0u; i < SEOUL_ARRAY_COUNT(s_kaLuaLazyBindModules); ++i)
	{
		luaL_Reg const reg = s_kaLuaLazyBindModules[i];
		if (0 == strcmp(sName, reg.name))
		{
			lua_pushcfunction(pLuaVm, reg.func);
			return 1;
		}
	}

	auto pScriptVm = Script::GetScriptVm(pLuaVm);

	// Check now for interruption.
	if (pScriptVm->Interrupted())
	{
		luaL_error(pLuaVm, "interrupted");
		return 0;
	}

	// Wrap this block in a scope so destructors are wrapped prior
	// to a potential longjmp at the end.
	Bool bSuccess = false;
	{
		// Attempt to construct a FilePath from the script reference.
		FilePath filePath;
		if (pScriptVm->ResolveFilePathFromRelativeFilename(
			String(sName, (UInt32)zNameLengthInBytes),
			filePath))
		{
			// Get the script.
			SharedPtr<Script::FileBody> pScript(Script::Manager::Get()->WaitForScript(filePath));

#if SEOUL_HOT_LOADING
			// Before running the script, add it to the list of scripts we've run.
			(void)Script::GetScriptVmHotLoadData(pLuaVm)->m_tScripts.Insert(filePath, true);
#endif // /#if SEOUL_HOT_LOADING

			// Run if defined.
			if (pScript.IsValid())
			{
				// Load the lua chunk.
				Script::LuaReadContext context(*pScript);
				bSuccess = (0 == lua_load(
					pLuaVm,
					LuaRead,
					&context,
					Script::ToLuaChunkName(pLuaVm, filePath).CStr()));
			}
			// Otherwise, add an error string.
			else
			{
				lua_pushstring(pLuaVm, "file not found or error reading file.");
			}
		}
	}

	// Report any error - this will trigger a longjmp.
	if (!bSuccess)
	{
		luaL_error(
			pLuaVm,
			"error loading module " LUA_QS ":\nstack:\n\t" LUA_QS,
			lua_tostring(pLuaVm, 1),
			lua_tostring(pLuaVm, -1));
	}

	// Done.
	return 1;
}

static int LuaOpenPackage(lua_State* pLuaVm)
{
	// Register standard package handling functionality.
	int const iReturn = luaopen_package(pLuaVm);

	// Now replace the loaders entries with only our custom SeoulEngine loader.
	lua_getfield(pLuaVm, LUA_ENVIRONINDEX, "loaders");
	if (!lua_istable(pLuaVm, -1))
	{
		luaL_error(pLuaVm, LUA_QL("package.loaders") " must be a table");
	}

	// Entry 1 is the preloader, which we want to leave in place.

	// Replace entry 2 with the SeoulEngine loader.
	lua_pushcfunction(pLuaVm, LuaLoaderSeoul);
	lua_rawseti(pLuaVm, -2, 2);

	// Replace all additional entries with nil.
	for (int i = 3; ; ++i)
	{
		// Get the entry.
		lua_rawgeti(pLuaVm, -1, i);

		// If the entry is nil, we're done. Pop the entry and the table.
		if (lua_isnil(pLuaVm, -1))
		{
			lua_pop(pLuaVm, 2);

			// If i is != 5, then the builtin loader table was
			// not registered in the expected manner (it should have had 4
			// non-nil entries).
			if (i != 5)
			{
				luaL_error(pLuaVm, LUA_QL("package.loaders") " did not have 4 existing entries");
			}

			return iReturn;
		}
		// Otherwise, replace the entry with nil.
		else
		{
			lua_pop(pLuaVm, 1);
			lua_pushnil(pLuaVm);
			lua_rawseti(pLuaVm, -2, i);
		}
	}
}

/** SeoulEngine implementation of math.random. */
static int LuaMathRandom(lua_State* pLuaVm)
{
	using namespace Seoul;

	auto const iCount = lua_gettop(pLuaVm);
	switch (iCount)
	{
	case 0:
		// This case, return a random number between [0, 1).
		lua_pushnumber(pLuaVm, GlobalRandom::UniformRandomFloat64());
		return 1;
	case 1:
	{
		// This case, return a random number on [1, upper].
		auto const iUpper = luaL_checkinteger(pLuaVm, 1);
		luaL_argcheck(pLuaVm, 1 <= iUpper, 1, "interval is empty");

		// UniformRandomUInt64n() is on [0, n), so we get the desired
		// output just be adding 1.
		lua_pushnumber(pLuaVm, (lua_Number)((Int64)GlobalRandom::UniformRandomUInt64n((UInt64)iUpper) + 1));
		return 1;
	}
	case 2:
	{
		// This case, return a random number on [lower, upper].
		auto const iLower = luaL_checkinteger(pLuaVm, 1);
		auto const iUpper = luaL_checkinteger(pLuaVm, 2);
		luaL_argcheck(pLuaVm, iLower <= iUpper, 2, "interval is empty");

		// Compute n - since our generator function generates a range that excludes
		// n, we need to +1 the delta.
		auto const iDelta = ((Int64)iUpper - (Int64)iLower) + (Int64)1;

		// UniformRandomUInt64() is on [0, n), so we get the desired
		// output by adding the result to iLower.
		lua_pushnumber(pLuaVm, (lua_Number)((Int64)GlobalRandom::UniformRandomUInt64n((UInt64)iDelta) + (Int64)iLower));
		return 1;
	}
	default:
		return luaL_error(pLuaVm, "wrong number of arguments");
	};
}

/** SeoulEngine implementation of math.random. */
static int LuaMathRandomSeed(lua_State* pLuaVm)
{
	using namespace Seoul;

	auto const iSeed = (Int64)luaL_checknumber(pLuaVm, 1);
	GlobalRandom::SetSeed((UInt64)iSeed, 0xEDC11D7A3A01D1E8 /* Same as Y default, though we only want non-zero to allow iSeed to be any value without requiring sanitizing. */);
	return 0;
}

/**
 * SeoulEngine specific math override. Uses standard
 * math package, but replaces math.random() and math.randomseed()
 * with hooks into SeoulEngine global random.
 */
static int LuaOpenMath(lua_State* pLuaVm)
{
	int iReturn = luaopen_math(pLuaVm);
	if (iReturn != 1)
	{
		return iReturn;
	}
	else
	{
		lua_pushcfunction(pLuaVm, LuaMathRandom);
		lua_setfield(pLuaVm, -2, "random");
		lua_pushcfunction(pLuaVm, LuaMathRandomSeed);
		lua_setfield(pLuaVm, -2, "randomseed");
		return 1;
	}
}

// TODO: Replace IO library with a SeoulEngine safe implementation.

static const luaL_Reg s_kaLuaBuiltinModules[] =
{
	{ "", luaopen_base },
	{ LUA_LOADLIBNAME, LuaOpenPackage },
	{ LUA_TABLIBNAME, luaopen_table },
	{ LUA_IOLIBNAME, luaopen_io },
	// We deliberately don't expose the os modules.
	// { LUA_OSLIBNAME, luaopen_os },
	{ LUA_STRLIBNAME, luaopen_string },
	{ LUA_MATHLIBNAME, LuaOpenMath },
	{ LUA_DBLIBNAME, luaopen_debug },
	{ LUA_BITLIBNAME, luaopen_bit },
};

static int LuaPrint(lua_State* pLuaVm)
{
	using namespace Seoul;

	int nargs = lua_gettop(pLuaVm);

	Script::Vm* pScriptVm = Script::GetScriptVm(pLuaVm);
	if (nullptr != pScriptVm && pScriptVm->GetSettings().m_StandardOutput.IsValid())
	{
		for (int i = 1; i <= nargs; ++i)
		{
			if (lua_isstring(pLuaVm, i))
			{
				pScriptVm->GetSettings().m_StandardOutput(lua_tostring(pLuaVm, i));
			}
		}
	}

	return 0;
}

static int LuaCreateTable(lua_State* pLuaVm)
{
	lua_createtable(pLuaVm, (int)luaL_optinteger(pLuaVm, 1, 0), (int)luaL_optinteger(pLuaVm, 1, 0));
	return 1;
}

static int LuaDescribeNativeEnum(lua_State* pLuaVm)
{
	using namespace Seoul;
	using namespace Reflection;

	int nargs = lua_gettop(pLuaVm);

	if (1 != nargs)
	{
		luaL_error(pLuaVm, "Incorrect number of arguments to SeoulDescribeNativeEnum - expected 1 string argument.");
		return 0;
	}

	if (!lua_isstring(pLuaVm, 1))
	{
		luaL_error(pLuaVm, "Incorrect argument type 1 in SeoulDescribeNativeEnum - expected string.");
		return 0;
	}

	Bool bSuccess = false;
	{
		size_t zTypeNameLength = 0;
		Byte const* sTypeName = lua_tolstring(pLuaVm, 1, &zTypeNameLength);

		// Check the HString first - avoid spurious HString creation.
		HString typeName;
		(void)HString::Get(typeName, sTypeName, (UInt32)zTypeNameLength);

		// Continue on if we have a type name.
		Reflection::Type const* pType = nullptr;
		Reflection::Enum const* pEnum = nullptr;
		if (!typeName.IsEmpty())
		{
			// Get the reflection data.
			pType = Registry::GetRegistry().GetType(typeName);
			pEnum = pType->TryGetEnum();
		}

		// If we have a type, and it is an enum, bind its name -> value mapping
		// and its value -> name mapping.
		if (nullptr != pType && nullptr != pEnum)
		{
			auto const& vNames = pEnum->GetNames();
			auto const& vValues = pEnum->GetValues();

			// Sanity check, must always be true.
			SEOUL_ASSERT(vNames.GetSize() == vValues.GetSize());

			// Into the table, bind name -> value and value -> name.
			bSuccess = true;
			lua_newtable(pLuaVm);

			// Value to name mapping, goes into the "Names" sub-table.
			lua_newtable(pLuaVm);
			for (UInt32 i = 0u; i < vValues.GetSize(); ++i)
			{
				HString const name = vNames[i];
				Int const iValue = vValues[i];

				// Value to name mapping.
				lua_pushlstring(pLuaVm, name.CStr(), (size_t)name.GetSizeInBytes());
				lua_rawseti(pLuaVm, -2, iValue);
			}
			lua_setfield(pLuaVm, -2, "Names");

			// Name to value mapping, goes into the "Values" sub-table.
			lua_newtable(pLuaVm);
			for (UInt32 i = 0u; i < vNames.GetSize(); ++i)
			{
				HString const name = vNames[i];
				Int const iValue = vValues[i];

				// Name to value mapping.
				lua_pushinteger(pLuaVm, iValue);
				lua_setfield(pLuaVm, -2, name.CStr());
			}
			lua_setfield(pLuaVm, -2, "Values");
		}
	}

	if (!bSuccess)
	{
		luaL_error(pLuaVm, "Failed binding native enum, probably invalid type name.");
		return 0;
	}
	else
	{
		return 1;
	}
}

static int LuaDescribeNativeUserData(lua_State* pLuaVm)
{
	using namespace Seoul;
	using namespace Reflection;

	int nargs = lua_gettop(pLuaVm);

	if (1 != nargs)
	{
		luaL_error(pLuaVm, "Incorrect number of arguments to SeoulDescribeNativeUserData - expected 1 string argument.");
		return 0;
	}

	if (!lua_isstring(pLuaVm, 1))
	{
		luaL_error(pLuaVm, "Incorrect argument type 1 in SeoulDescribeNativeUserData - expected string.");
		return 0;
	}

	Bool bSuccess = false;
	{
		size_t zTypeNameLength = 0;
		Byte const* sTypeName = lua_tolstring(pLuaVm, 1, &zTypeNameLength);

		// Check the HString first - avoid spurious HString creation.
		HString typeName;
		(void)HString::Get(typeName, sTypeName, (UInt32)zTypeNameLength);

		// NOTE: if you are seeing typeName be empty here in a Ship build,
		// then it is possible that your type was just not linked.
		// Check to make sure, and add SEOUL_LINK_ME in the appropriate
		// place if not.

		// Continue on if we have a type name.
		Reflection::Type const* pType = nullptr;
		if (!typeName.IsEmpty())
		{
			// Get the reflection data.
			pType = Registry::GetRegistry().GetType(typeName);
		}

		// If we have a type, bind its description.
		if (nullptr != pType)
		{
			Script::FunctionInterface scriptFunctionInterface(pLuaVm);
			bSuccess = scriptFunctionInterface.PushReturnUserDataType(*pType);
		}
	}

	if (!bSuccess)
	{
		luaL_error(pLuaVm, "Failed binding native type description, probably invalid type name.");
		return 0;
	}
	else
	{
		return 1;
	}
}

/**
 * This is a custom hook we've added to lua. It will be defined
 * whenever a user data is about to be freed (not finalized - completely
 * freed. It is called right before lua_memfree).
 *
 * For types that require it, this is where we invoke the type's
 * destructor. Doing this as a __gc hook creates the unresolvable
 * situation where a type may be finalized but then resurrected,
 * leaving it to be in an accessible and (must be) usable state
 * after its destructor has been invoked, which is invalid.
 */
static void PreFreeUserData(void* p, uint32_t data)
{
	using namespace Seoul;

	if (0u == data) { return; }

	auto pType = Reflection::Registry::GetRegistry().GetType(data - 1u);
	if (nullptr == pType) { return; }

	auto weakAny = pType->GetPtrUnsafe(p);
	pType->InvokeDestructor(weakAny);
}

static int LuaIsWeakUserDataValid(lua_State* pLuaVm)
{
	using namespace Seoul::Reflection;
	using namespace Seoul;

	SEOUL_SCRIPT_CHECK_VM_STACK(pLuaVm, 1);

	if (0 == lua_getmetatable(pLuaVm, 1))
	{
		lua_pushboolean(pLuaVm, 0);
		return 1;
	}

	lua_pushlightuserdata(pLuaVm, Script::kpScriptTypeKey);
	lua_rawget(pLuaVm, -2);

	void* pScriptTypeUserData = lua_touserdata(pLuaVm, -1);
	if (nullptr == pScriptTypeUserData)
	{
		lua_pop(pLuaVm, 1); // Pop the metatable
		lua_pushboolean(pLuaVm, 0);
		return 1;
	}

	// Check to see if we have a valid pointer in our weak user data
	{
		Script::TypeUserData const& luaScriptTypeUserData = *((Script::TypeUserData*)pScriptTypeUserData);

		if (luaScriptTypeUserData.m_bWeak)
		{
			void** pSeoulEngineUserData = (void**)lua_touserdata(pLuaVm, 1);
			if (nullptr != pSeoulEngineUserData)
			{
				void* pInner = *pSeoulEngineUserData;

				if(pInner)
				{
					lua_pop(pLuaVm, 2); // Pop the user data and the meta table
					lua_pushboolean(pLuaVm, 1);
					return 1;
				}
			}
		}
	}

	lua_pop(pLuaVm, 2); // Pop the user data and the meta table
	lua_pushboolean(pLuaVm, 0);
	return 1;
}

static int LuaInitSetProgressTotal(lua_State* pLuaVm)
{
	// Prior to any C++ invocations, since this will longjmp on error.
	auto const iTotal = luaL_checkinteger(pLuaVm, 1);
	if (iTotal < 0)
	{
		luaL_error(pLuaVm, "expected integer >= 0.");
		return 0;
	}

	using namespace Seoul;
	Script::Vm* pScriptVm = Script::GetScriptVm(pLuaVm);
	pScriptVm->InitIncreaseProgressTotal((Atomic32Type)iTotal);
	return 0;
}

static int LuaInitOnProgress(lua_State* pLuaVm)
{
	using namespace Seoul;
	Script::Vm* pScriptVm = Script::GetScriptVm(pLuaVm);
	pScriptVm->InitOnProgress();
	return 0;
}

static int LuaHasNativeUserData(lua_State* pLuaVm)
{
	using namespace Seoul;
	using namespace Reflection;

	int nargs = lua_gettop(pLuaVm);

	if (1 != nargs)
	{
		luaL_error(pLuaVm, "Incorrect number of arguments to SeoulNewNativeUserData - expected 1 string argument.");
		return 0;
	}

	if (!lua_isstring(pLuaVm, 1))
	{
		luaL_error(pLuaVm, "Incorrect argument type 1 in SeoulNewNativeUserData - expected string.");
		return 0;
	}

	size_t zTypeNameLength = 0;
	Byte const* sTypeName = "";
	{
		sTypeName = lua_tolstring(pLuaVm, 1, &zTypeNameLength);

		// Check the HString first - avoid spurious HString creation.
		HString typeName;
		(void)HString::Get(typeName, sTypeName, (UInt32)zTypeNameLength);

		// Continue on if we have a type name.
		Reflection::Type const* pType = nullptr;
		if (!typeName.IsEmpty())
		{
			// Get the reflection data.
			pType = Registry::GetRegistry().GetType(typeName);
		}

		// Push the return value.
		lua_pushboolean(pLuaVm, (nullptr != pType));
		return 1;
	}
}

static int LuaNewNativeUserData(lua_State* pLuaVm)
{
	using namespace Seoul;
	using namespace Reflection;

	int nargs = lua_gettop(pLuaVm);

	if (1 != nargs)
	{
		luaL_error(pLuaVm, "Incorrect number of arguments to SeoulNewNativeUserData - expected 1 string argument.");
		return 0;
	}

	if (!lua_isstring(pLuaVm, 1))
	{
		luaL_error(pLuaVm, "Incorrect argument type 1 in SeoulNewNativeUserData - expected string.");
		return 0;
	}

	size_t zTypeNameLength = 0;
	Byte const* sTypeName = "";
	Bool bSuccess = false;
	{
		sTypeName = lua_tolstring(pLuaVm, 1, &zTypeNameLength);

		// Check the HString first - avoid spurious HString creation.
		HString typeName;
		(void)HString::Get(typeName, sTypeName, (UInt32)zTypeNameLength);

		// Continue on if we have a type name.
		Reflection::Type const* pType = nullptr;
		if (!typeName.IsEmpty())
		{
			// Get the reflection data.
			pType = Registry::GetRegistry().GetType(typeName);
		}

		// If we have a type, instantiate it.
		if (nullptr != pType)
		{
			Script::FunctionInterface scriptFunctionInterface(pLuaVm);
			bSuccess = scriptFunctionInterface.PushReturnUserData(*pType);
		}
	}

	if (!bSuccess)
	{
		luaL_error(pLuaVm, "Failed instantiating native type, invalid type name: \"%s\"", sTypeName);
		return 0;
	}
	else
	{
		return 1;
	}
}

static int LuaReadPb(lua_State* pLuaVm)
{
	using namespace Seoul;

	Reflection::WeakAny weakAny;
	int nargs = lua_gettop(pLuaVm);

	if (1 != nargs)
	{
		luaL_error(pLuaVm, "Incorrect number of arguments to SeoulLuaReadPb - expected 1 FilePath argument.");
		return 0;
	}

	if (!Script::GetUserData(pLuaVm, 1, weakAny) ||
		!weakAny.IsOfType<FilePath*>())
	{
		luaL_error(pLuaVm, "Incorrect type to SeoulLuaReadPb - expected 1 FilePath argument.");
		return 0;
	}

	FilePath const filePath = *weakAny.Cast<FilePath*>();

	SharedPtr<Script::Protobuf> pScriptProtobuf(Script::Manager::Get()->WaitForPb(filePath));
	if (!pScriptProtobuf.IsValid())
	{
		luaL_error(pLuaVm, "Failed loading binary Protocol Buffer file \"%s\"", filePath.CStr());
		return 0;
	}

	lua_pushlstring(pLuaVm, (const char*)pScriptProtobuf->GetDataPtr(), (size_t)pScriptProtobuf->GetDataSizeInBytes());
	return 1;
}

// Called when a type is bound with its own __index method. This
// function has 2 upvalues (the first is the user __index, the
// second is the metatable of the type). We attempt to resolve
// the index with the metatable and if that fails, finish by
// invoking the user __index function.
static int LuaUserIndexWrapper(lua_State* pLuaVm)
{
	using namespace Seoul::Reflection;
	using namespace Seoul;

	SEOUL_SCRIPT_CHECK_VM_STACK(pLuaVm, 1);

	// Argument 1 is the userdata instance, Argument 2 is the key.
	lua_pushvalue(pLuaVm, 2);
	lua_rawget(pLuaVm, lua_upvalueindex(2));

	// If nil, try the user function.
	if (0 != lua_isnil(pLuaVm, -1))
	{
		// Pop the nil.
		lua_pop(pLuaVm, 1);

		// Now push the user __index function.
		lua_pushvalue(pLuaVm, lua_upvalueindex(1));

		// Push arguments - the user data and then the key.
		lua_pushvalue(pLuaVm, 1);
		lua_pushvalue(pLuaVm, 2);

		// Invoke the user function.
		lua_call(pLuaVm, 2, 1);
	}

	// Always one return value.
	return 1;
}

} // extern "C"

namespace Seoul
{

static Bool InternalStaticRunScript(lua_State* pLuaVm, FilePath filePath, const SharedPtr<Script::FileBody>& pScript)
{
	// Make sure we're handling VM state properly.
	SEOUL_SCRIPT_CHECK_VM_STACK(pLuaVm);

	// Load the lua chunk.
	Script::LuaReadContext context(*pScript);
	Bool bSuccess = (0 == lua_load(
		pLuaVm,
		LuaRead,
		&context,
		Script::ToLuaChunkName(pLuaVm, filePath).CStr()));

	// If the read succeeded, call the chunk.
	if (bSuccess)
	{
		bSuccess = Script::Pcall(pLuaVm, 0, 0);
		if (!bSuccess)
		{
			const char* sErrorMessage = lua_tostring(pLuaVm, -1);
			if (nullptr != sErrorMessage)
			{
				Script::ReportError(pLuaVm, sErrorMessage);
			}
			lua_pop(pLuaVm, 1);
		}
	}
	else if (lua_isstring(pLuaVm, -1))
	{
		const char* sErrorMessage = lua_tostring(pLuaVm, -1);
		Script::ReportError(pLuaVm, sErrorMessage);
		lua_pop(pLuaVm, 1);
	}

	return bSuccess;
}

namespace Script
{

void VmSettings::SetStandardBasePaths()
{
	BasePaths vsPaths;

	// If a script project, use it to derive the paths.
	auto const projectPath(CompilerSettings::GetApplicationScriptProjectFilePath());
	if (projectPath.IsValid())
	{
		String sRootCS;
		String sRootLua;
		String sRootLuaDebug;
		CompilerSettings::GetRootPaths(keCurrentPlatform, projectPath, sRootCS, sRootLua, sRootLuaDebug);

		// Add the root CS and the appropriate root Lua paths.
		if (CompilerSettings::ApplicationIsUsingDebug()) { vsPaths.PushBack(sRootLuaDebug); }
		else { vsPaths.PushBack(sRootLua); }
		vsPaths.PushBack(sRootCS);
	}
	else
	{
		// Fallback to the standardized script path.
		auto const& sContentDir(GamePaths::Get()->GetSourceDir());
		vsPaths.PushBack(Path::Combine(sContentDir, "Authored/Scripts/"));
	}

	// Done.
	m_vBasePaths.Swap(vsPaths);
}

#if !SEOUL_ASSERTIONS_DISABLED
Atomic32 Vm::s_InVmDestroy(0);
#endif // /#if !SEOUL_ASSERTIONS_DISABLED

Vm::Vm(const VmSettings& luaScriptVmSettings)
	: m_Settings(luaScriptVmSettings)
	, m_hThis()
	, m_pLuaVm()
	, m_pDefaultAtPanic(nullptr)
	, m_Mutex()
	, m_uGcStepSize(luaScriptVmSettings.m_uInitialGcStepSize)
	, m_InitProgress(0)
	, m_InitTotalSteps(0)
#if SEOUL_HOT_LOADING
	, m_HotLoadData()
#endif // /#if SEOUL_HOT_LOADING
{
	{
		// Keep access to the VM exclusive.
		Lock lock(m_Mutex);

		// Create the VM.
		InsideLockCreateVM();

#if SEOUL_ENABLE_DEBUGGER_CLIENT
		if (m_Settings.m_bEnableDebuggerHooks)
		{
			// Hook into the debugger client if present.
			InsideLockSetDebuggerHooks();
		}
#endif // /SEOUL_ENABLE_DEBUGGER_CLIENT

		// Make sure we're handling the lua stack properly.
		SEOUL_SCRIPT_CHECK_VM_STACK(m_pLuaVm);

		// Bind builtin functions and global variables.
		InsideLockBindBuiltins();
	}

	// Allocate a handle for this.
	m_hThis = VmHandleTable::Allocate(this);
}

Vm::~Vm()
{
	// Free our handle.
	VmHandleTable::Free(m_hThis);

	{
		// Keep access to the VM exclusive.
		Lock lock(m_Mutex);

		InsideLockDestroyVM();
	}
}

#if SEOUL_HOT_LOADING
void Vm::RegisterForHotLoading()
{
	SEOUL_ASSERT(IsMainThread());

	if (!m_HotLoadData.m_bRegisteredForHotLoading)
	{
		// Register for appropriate callbacks with ContentLoadManager.
		Events::Manager::Get()->RegisterCallback(
			Content::FileChangeEventId,
			SEOUL_BIND_DELEGATE(&Vm::OnFileChange, this));
		// Make sure we're first in line for the file change event,
		// so we come before the Content::Store that will actually handle the
		// change event.
		Events::Manager::Get()->MoveLastCallbackToFirst(Content::FileChangeEventId);

		Events::Manager::Get()->RegisterCallback(
			Content::FileIsLoadedEventId,
			SEOUL_BIND_DELEGATE(&Vm::OnIsFileLoaded, this));

		Events::Manager::Get()->RegisterCallback(
			Content::FileLoadCompleteEventId,
			SEOUL_BIND_DELEGATE(&Vm::OnFileLoadComplete, this));

		m_HotLoadData.m_bRegisteredForHotLoading = true;
	}
}

void Vm::UnregisterFromHotLoading()
{
	SEOUL_ASSERT(IsMainThread());

	if (m_HotLoadData.m_bRegisteredForHotLoading)
	{
		// Unregister from appropriate callbacks with ContentLoadManager.
		Events::Manager::Get()->UnregisterCallback(
			Content::FileLoadCompleteEventId,
			SEOUL_BIND_DELEGATE(&Vm::OnFileLoadComplete, this));

		Events::Manager::Get()->UnregisterCallback(
			Content::FileIsLoadedEventId,
			SEOUL_BIND_DELEGATE(&Vm::OnIsFileLoaded, this));

		Events::Manager::Get()->UnregisterCallback(
			Content::FileChangeEventId,
			SEOUL_BIND_DELEGATE(&Vm::OnFileChange, this));

		m_HotLoadData.m_bRegisteredForHotLoading = false;
	}
}
#endif // /#if SEOUL_HOT_LOADING

void Vm::BindType(const Reflection::Type& type)
{
	// Keep access to the VM exclusive.
	Lock lock(m_Mutex);

	SEOUL_SCRIPT_CHECK_VM_STACK(m_pLuaVm);

	InsideLockBindType(type, false);
	InsideLockBindType(type, true);
}

Bool Vm::InternalBindStrongInstance(
	const Reflection::Type& type,
	SharedPtr<VmObject>& rpBinding,
	void*& rpInstance)
{
	using namespace Reflection;

	// Keep access to the VM exclusive.
	Lock lock(m_Mutex);

	SEOUL_SCRIPT_CHECK_VM_STACK(m_pLuaVm);

	void* pInstance = nullptr;

	// Instantiate the native instance.
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
			ReportError(m_pLuaVm, String::Printf("%s: failed allocating "
				"memory for strong instance of type %s",
				__FUNCTION__,
				type.GetName().CStr()).CStr());

			return false;
		}

		if (!type.InPlaceNew(pInstance, zSizeInBytes).IsValid())
		{
			ReportError(m_pLuaVm, String::Printf("%s: failed instantiating "
				"native instance of type '%s' for bind.",
				__FUNCTION__,
				type.GetName().CStr()).CStr());

			// pop the instance.
			lua_pop(m_pLuaVm, 1);
			return false;
		}
	}

	// Conditionally setup the metatable for the userdata and associate it.
	InsideLockBindType(type, false);
	LuaGetMetatable(m_pLuaVm, type, false);
	lua_setmetatable(m_pLuaVm, -2);

	// Get the object.
	Int32 const iObject = luaL_ref(m_pLuaVm, LUA_REGISTRYINDEX);

	// Done - output the wrapper and the userdata.
	rpBinding = SharedPtr<VmObject>(SEOUL_NEW(MemoryBudgets::Scripting) VmObject(
		m_hThis,
		iObject));
	rpInstance = pInstance;

	return true;
}

/**
 * Construct a Lua table from dataStore at tableNode and
 * bind as a strong instance, assigned to rpBinding.
 */
Bool Vm::BindStrongTable(
	SharedPtr<VmObject>& rpBinding,
	const DataStore& dataStore,
	const DataNode& tableNode)
{
	// Keep access to the VM exclusive.
	Lock lock(m_Mutex);

	SEOUL_SCRIPT_CHECK_VM_STACK(m_pLuaVm);

	// Commit the DataStore.
	if (!PushDataNode(m_pLuaVm, dataStore, tableNode, false, false))
	{
		return false;
	}

	// Get the object.
	Int32 const iObject = luaL_ref(m_pLuaVm, LUA_REGISTRYINDEX);

	// Done - output the wrapper and the userdata.
	rpBinding = SharedPtr<VmObject>(SEOUL_NEW(MemoryBudgets::Scripting) VmObject(
		m_hThis,
		iObject));

	return true;
}

Bool Vm::BindWeakInstance(
	const Reflection::WeakAny& nativeInstance,
	SharedPtr<VmObject>& rpBinding)
{
	using namespace Reflection;

	// Cache the reflection type.
	const Reflection::Type& type = nativeInstance.GetType();

	// Keep access to the VM exclusive.
	Lock lock(m_Mutex);

	SEOUL_SCRIPT_CHECK_VM_STACK(m_pLuaVm);

	// Instantiate a Lua userdata to contain the pointer to the native instance.
	// Instantiate the native instance.
	{
		auto const& typeInfo = nativeInstance.GetTypeInfo();
		if (!typeInfo.IsPointer())
		{
			ReportError(m_pLuaVm, String::Printf("%s: failed binding "
				"weak instance of type %s, instance is not a pointer.",
				__FUNCTION__,
				type.GetName().CStr()).CStr());

			return false;
		}

		void** ppInstance = (void**)nativeInstance.GetConstVoidStarPointerToObject();
		if (nullptr == ppInstance || nullptr == *ppInstance)
		{
			ReportError(m_pLuaVm, String::Printf("%s: failed binding "
				"weak instance of type %s, instance is a null pointer.",
				__FUNCTION__,
				type.GetName().CStr()).CStr());

			return false;
		}

		void** ppOutInstance = (void**)lua_newuserdata(m_pLuaVm, sizeof(void*));
		if (nullptr == ppOutInstance)
		{
			ReportError(m_pLuaVm, String::Printf("%s: failed allocating "
				"memory for weak instance of type %s",
				__FUNCTION__,
				type.GetName().CStr()).CStr());

			return false;
		}

		*ppOutInstance = *ppInstance;
	}

	// Conditionally setup the metatable for the userdata and associate it.
	InsideLockBindType(type, true);
	LuaGetMetatable(m_pLuaVm, type, true);
	lua_setmetatable(m_pLuaVm, -2);

	// Get the object.
	Int32 const iObject = luaL_ref(m_pLuaVm, LUA_REGISTRYINDEX);

	// Done - output the wrapper and the userdata.
	rpBinding = SharedPtr<VmObject>(SEOUL_NEW(MemoryBudgets::Scripting) VmObject(
		m_hThis,
		iObject));

	return true;
}

// Convenience, return true if a global member is not nil.
Bool Vm::HasGlobal(HString name) const
{
	// Keep access to the VM exclusive.
	Lock lock(m_Mutex);

	SEOUL_SCRIPT_CHECK_VM_STACK(m_pLuaVm);

	// Get the global.
	lua_getglobal(m_pLuaVm, name.CStr());
	auto const bHasGlobal = !lua_isnil(m_pLuaVm, -1);
	lua_pop(m_pLuaVm, 1);

	return bHasGlobal;
}

Bool Vm::RunCode(const String& sCode)
{
	// Keep access to the VM exclusive.
	Lock lock(m_Mutex);

	SEOUL_SCRIPT_CHECK_VM_STACK(m_pLuaVm);

	return InsideLockRunCode(sCode);
}

Bool Vm::RunScript(const String& sRelativeFilename, Bool bAddToHotLoadSet /*= true*/)
{
	// Keep access to the VM exclusive.
	Lock lock(m_Mutex);

	SEOUL_SCRIPT_CHECK_VM_STACK(m_pLuaVm);

	if (InsideLockRunScript(sRelativeFilename, bAddToHotLoadSet))
	{
		if (bAddToHotLoadSet)
		{
			// Capture for tracking.
#if SEOUL_HOT_LOADING
			m_HotLoadData.m_ScriptProjectLoadCount = Manager::Get()->GetAppScriptProject().GetTotalLoadsCount();
#endif
		}

		return true;
	}

	return false;
}

void Vm::GcFull()
{
	// Keep access to the VM exclusive.
	Lock lock(m_Mutex);

	SEOUL_SCRIPT_CHECK_VM_STACK(m_pLuaVm);

	(void)lua_gc(m_pLuaVm, LUA_GCCOLLECT, 0);
}

void Vm::StepGarbageCollector()
{
	SEOUL_PROF("Script::Vm.StepGC");

	// Keep access to the VM exclusive.
	Lock lock(m_Mutex);

	// TODO: Not ideal but also a convenient place to perform this check.
#if SEOUL_HOT_LOADING
	if (m_HotLoadData.m_ScriptProjectLoadCount != Manager::Get()->GetAppScriptProject().GetTotalLoadsCount())
	{
		if (m_HotLoadData.m_ScriptProjectLoadCount > 0)
		{
			m_HotLoadData.m_bOutOfDate = true;
		}

		// Make sure we don't get stuck in a loading loop.
		m_HotLoadData.m_ScriptProjectLoadCount = Manager::Get()->GetAppScriptProject().GetTotalLoadsCount();
	}
#endif // /#if SEOUL_HOT_LOADING

	SEOUL_SCRIPT_CHECK_VM_STACK(m_pLuaVm);

	// Step Lua's garbage collect.
	Int64 const iStartTimeInTicks = SeoulTime::GetGameTimeInTicks();
	(void)lua_gc(m_pLuaVm, LUA_GCSTEP, m_uGcStepSize);
	Int64 const iEndTimeInTicks = SeoulTime::GetGameTimeInTicks();

	// Compute the total time spent in the Gc step in milliseconds, and
	// use this to adjust the GC step size.
	Double const fGcTimeInMilliseconds = SeoulTime::ConvertTicksToMilliseconds(iEndTimeInTicks - iStartTimeInTicks);
	if (fGcTimeInMilliseconds > m_Settings.m_fTargetIncrementalGcTimeInMilliseconds)
	{
		m_uGcStepSize >>= 1;
	}
	else if (fGcTimeInMilliseconds <= m_Settings.m_fTargetIncrementalGcTimeInMilliseconds * 0.5)
	{
		m_uGcStepSize <<= 1;
	}
	m_uGcStepSize = Clamp(m_uGcStepSize, m_Settings.m_uMinGcStepSize, m_Settings.m_uMaxGcStepSize);
}

/** Attempt to set rpObject to an existing object in the global table. */
Bool Vm::TryGetGlobal(HString name, SharedPtr<VmObject>& rpObject)
{
	// Keep access to the VM exclusive.
	Lock lock(m_Mutex);

	SEOUL_SCRIPT_CHECK_VM_STACK(m_pLuaVm);

	// Perform the lookup.
	lua_getglobal(m_pLuaVm, name.CStr());

	// Invalid if nil.
	if (lua_isnil(m_pLuaVm, -1))
	{
		lua_pop(m_pLuaVm, 1);
		return false;
	}
	// Otherwise, bind the object and return.
	else
	{
		// Get the object.
		Int32 const iObject = luaL_ref(m_pLuaVm, LUA_REGISTRYINDEX);

		// Done - output the wrapper and the userdata.
		rpObject = SharedPtr<VmObject>(SEOUL_NEW(MemoryBudgets::Scripting) VmObject(
			m_hThis,
			iObject));
		return true;
	}
}

/**
 * Attempt to set a Script::VmObject to the global table.
 * On false, global table is left unchanged.
 */
Bool Vm::TrySetGlobal(HString name, const SharedPtr<VmObject>& pObject)
{
	// Early out if pObject is invalid.
	if (!pObject.IsValid() || pObject->IsNil())
	{
		return false;
	}

	// Keep access to the VM exclusive.
	Lock lock(m_Mutex);

	SEOUL_SCRIPT_CHECK_VM_STACK(m_pLuaVm);

	// Push the object.
	pObject->PushOntoVmStack(m_pLuaVm);

	// Commit to the global table.
	lua_setglobal(m_pLuaVm, name.CStr());

	return true;
}

/** Utility - public for script hook utilities, not meant to be used by client code. */
Bool Vm::ResolveFilePathFromRelativeFilename(
	const String& sRelativeFilename,
	FilePath& rFilePath) const
{
	// Matches resolution behavior of default Lua and most languages.
	// We search each base path for the file until we find a match
	// or until we run out of paths to search.
	for (auto const& sBase : m_Settings.m_vBasePaths)
	{
		// Construct a FilePath to test.
		auto filePath(FilePath::CreateContentFilePath(
			Path::Combine(sBase, sRelativeFilename)));
		filePath.SetType(FileType::kScript);

		// If that file exists, we've found the match.
		if (FileManager::Get()->Exists(filePath))
		{
			rFilePath = filePath;
			return true;
		}
		// In non-ship builds and when cooking is enabled,
		// also check the source folder.
#if !SEOUL_SHIP
		else if (
			CookManager::Get()->IsCookingEnabled() &&
			FileManager::Get()->ExistsInSource(filePath))
		{
			rFilePath = filePath;
			return true;
		}
#endif // /#if !SEOUL_SHIP
	}

	return false;
}

void Vm::InsideLockBindBuiltins()
{
	// Make sure we are managing the lua stack correctly.
	SEOUL_SCRIPT_CHECK_VM_STACK(m_pLuaVm);

	// Register all our modules.
	for (size_t i = 0; i < SEOUL_ARRAY_COUNT(s_kaLuaBuiltinModules); ++i)
	{
		const luaL_Reg& module = s_kaLuaBuiltinModules[i];
		lua_pushcfunction(m_pLuaVm, module.func);
		lua_pushstring(m_pLuaVm, module.name);
		lua_call(m_pLuaVm, 1, 0);
	}

	// Add or replace a minimum set of global hooks.
	{
		lua_pushglobaltable(m_pLuaVm);

		// Replace the default print function.
		lua_pushcfunction(m_pLuaVm, LuaPrint); lua_setfield(m_pLuaVm, -2, "print");

		// Add the global function used to preallocate tables.
		lua_pushcfunction(m_pLuaVm, LuaCreateTable); lua_setfield(m_pLuaVm, -2, "SeoulCreateTable");

		// Add the global function used to describe native enums.
		lua_pushcfunction(m_pLuaVm, LuaDescribeNativeEnum); lua_setfield(m_pLuaVm, -2, "SeoulDescribeNativeEnum");

		// Add the global function used to describe native types.
		lua_pushcfunction(m_pLuaVm, LuaDescribeNativeUserData); lua_setfield(m_pLuaVm, -2, "SeoulDescribeNativeUserData");

		// Add the global function used to query the existence of native types.
		lua_pushcfunction(m_pLuaVm, LuaHasNativeUserData); lua_setfield(m_pLuaVm, -2, "SeoulHasNativeUserData");

		// Add the global function used to instantiate instances of native types.
		lua_pushcfunction(m_pLuaVm, LuaNewNativeUserData); lua_setfield(m_pLuaVm, -2, "SeoulNativeNewNativeUserData");

		// Add the global function to test if a native instance is still valid
		lua_pushcfunction(m_pLuaVm, LuaIsWeakUserDataValid); lua_setfield(m_pLuaVm, -2, "SeoulIsNativeValid");

		// Add the global functions used for progress tracking.
		lua_pushcfunction(m_pLuaVm, LuaInitSetProgressTotal); lua_setfield(m_pLuaVm, -2, "__initprogresssteps__");
		lua_pushcfunction(m_pLuaVm, LuaInitOnProgress); lua_setfield(m_pLuaVm, -2, "__oninitprogress__");

		// Add the global hook for protocol buffer files.
		lua_pushcfunction(m_pLuaVm, LuaReadPb); lua_setfield(m_pLuaVm, -2, "SeoulLuaReadPb");

		// Done with the global table.
		lua_pop(m_pLuaVm, 1);
	}

	// Set build config variables.
	{
		lua_pushglobaltable(m_pLuaVm);

		// SEOUL_DEBUG
		lua_pushboolean(m_pLuaVm, SEOUL_DEBUG); lua_setfield(m_pLuaVm, -2, "g_bBuildConfigDebug");

		// SEOUL_DEVELOPER
		lua_pushboolean(m_pLuaVm, SEOUL_DEVELOPER); lua_setfield(m_pLuaVm, -2, "g_bBuildConfigDeveloper");

		// SEOUL_SHIP
		lua_pushboolean(m_pLuaVm, SEOUL_SHIP); lua_setfield(m_pLuaVm, -2, "g_bBuildConfigShip");

		// g_kbBuildForDistribution
		lua_pushboolean(m_pLuaVm, g_kbBuildForDistribution ? 1 : 0); lua_setfield(m_pLuaVm, -2, "g_bBuildForDistribution");

		// Build version
		lua_pushstring(m_pLuaVm, BUILD_VERSION_STR); lua_setfield(m_pLuaVm, -2, "g_sBuildVersion");

		// Build changelist.
		lua_pushinteger(m_pLuaVm, (lua_Integer)g_iBuildChangelist); lua_setfield(m_pLuaVm, -2, "g_iBuildChangelist");

		// Platform
		lua_pushinteger(m_pLuaVm, (lua_Integer)keCurrentPlatform); lua_setfield(m_pLuaVm, -2, "g_iPlatform");

		// Platform name
		lua_pushstring(m_pLuaVm, GetCurrentPlatformName()); lua_setfield(m_pLuaVm, -2, "g_sPlatformName");

		// Pop the global table.
		lua_pop(m_pLuaVm, 1);
	}

	// Register some basic types used by Script::Vm.
	InsideLockBindType(TypeOf<FilePath>(), true);
	InsideLockBindType(TypeOf<FilePath>(), false);
	InsideLockBindType(TypeOf<TimeInterval>(), true);
	InsideLockBindType(TypeOf<TimeInterval>(), false);
	InsideLockBindType(TypeOf<WorldTime>(), true);
	InsideLockBindType(TypeOf<WorldTime>(), false);
}

void Vm::InsideLockBindType(const Reflection::Type& type, Bool bWeak)
{
	SEOUL_SCRIPT_CHECK_VM_STACK(m_pLuaVm);

	// If a metatable exists of the type, we're done.
	lua_pushlightuserdata(m_pLuaVm, LuaGetMetatableKey(type, bWeak));
	lua_rawget(m_pLuaVm, LUA_REGISTRYINDEX);
	if (0 == lua_isnil(m_pLuaVm, -1))
	{
		lua_pop(m_pLuaVm, 1);
		return;
	}
	// Otherwise, create the table and populate it.
	else
	{
		// First pop the nil value.
		lua_pop(m_pLuaVm, 1);

		// Now create a new table.
		lua_newtable(m_pLuaVm);

		// Insert the table into the registry with the type pointer as the key.
		lua_pushlightuserdata(m_pLuaVm, LuaGetMetatableKey(type, bWeak));
		lua_pushvalue(m_pLuaVm, -2);
		lua_settable(m_pLuaVm, LUA_REGISTRYINDEX);
	}

	// Bind the methods of the type.
	BindMethods(m_pLuaVm, type, bWeak);

	// Set the type as a userdata in the table.
	lua_pushlightuserdata(m_pLuaVm, kpScriptTypeKey);
	TypeUserData* pScriptTypeUserData = (TypeUserData*)lua_newuserdata(m_pLuaVm, sizeof(TypeUserData));
	new (pScriptTypeUserData) TypeUserData(type, bWeak);
	lua_settable(m_pLuaVm, -3);

	// Set a "typeName" text field to describe the userdata type.
	lua_pushlstring(m_pLuaVm, type.GetName().CStr(), type.GetName().GetSizeInBytes());
	lua_setfield(m_pLuaVm, -2, "typeName");

	// Check for an existing __index field - if it exists,
	// we need to wrap it. Otherwise, we can just use
	// the table itself as the __index field.
	lua_getfield(m_pLuaVm, -1, "__index");
	if (0 == lua_isnil(m_pLuaVm, -1))
	{
		// __index is a closure with the metatable and the user __index function as upvalues.
		lua_pushvalue(m_pLuaVm, -2);
		lua_pushcclosure(m_pLuaVm, LuaUserIndexWrapper, 2);

		// Set the function and then pop the table.
		lua_setfield(m_pLuaVm, -2, "__index");
		lua_pop(m_pLuaVm, 1);
	}
	else
	{
		// Pop the nil.
		lua_pop(m_pLuaVm, 1);

		// Set the __index metamethod to the metatable instance,
		// so that methods are resolved if they are not overriden in the
		// instance. This will also pop the table, so we're done.
		lua_setfield(m_pLuaVm, -1, "__index");
	}
}

void Vm::InsideLockCreateVM()
{
	// Make sure we don't already have a VM.
	InsideLockDestroyVM();

	// Create a new VM.
	auto alloc = (m_Settings.m_CustomMemoryAllocatorHook != nullptr
		? m_Settings.m_CustomMemoryAllocatorHook
		: LuaMemoryAlloc);
	void* pUserData = nullptr;

#if SEOUL_ENABLE_MEMORY_TOOLING
	// Override with tooling hook.
	if (m_Settings.m_bEnableMemoryProfiling)
	{
		alloc = LuaMemoryAllocWithTooling;
		pUserData = this;
	}
#endif // /SEOUL_ENABLE_MEMORY_TOOLING

	m_pLuaVm.Reset(lua_newstateex(alloc, pUserData, PreFreeUserData, m_Settings.m_pPreCollectionHook));

	// If this fails, and the current build is x86_64 for OSX,
	// the following linker options must be used:
	//   -pagezero_size 10000 -image_base 100000000
	//
	// See: http://luajit.org/install.html, Embedding LuaJIT.
	SEOUL_ASSERT(m_pLuaVm.IsValid());

	// Set our atpanic handlier.
	m_pDefaultAtPanic = lua_atpanic(m_pLuaVm, LuaAtPanic);

	// Make sure we're cleaning up the lua stack properly.
	SEOUL_SCRIPT_CHECK_VM_STACK(m_pLuaVm);

	// Set a self pointer.
	lua_pushlightuserdata(m_pLuaVm, kpScriptVmKey);
	lua_pushlightuserdata(m_pLuaVm, this);
	lua_settable(m_pLuaVm, LUA_REGISTRYINDEX);

#if SEOUL_HOT_LOADING
	// Set a hot load data pointer.
	lua_pushlightuserdata(m_pLuaVm, kpScriptVmHotLoadDataKey);
	lua_pushlightuserdata(m_pLuaVm, &(m_HotLoadData));
	lua_settable(m_pLuaVm, LUA_REGISTRYINDEX);
#endif // /#if SEOUL_HOT_LOADING

	// Setup the weak registry.
	{
		// Key for the table goes first.
		lua_pushlightuserdata(m_pLuaVm, kpScriptWeakRegistryKey);

		// Weak table for mapping native instances to their
		// native script bindings. Used generally/globally
		// for cases where the two must be tightly bound but
		// are separate instances (one bound directly into
		// script, the other typically a referenced counted
		// smart pointer).
		lua_newtable(m_pLuaVm);

		// Setup a metatable making the container table weak.
		lua_newtable(m_pLuaVm);
		lua_pushstring(m_pLuaVm, "kv");
		lua_setfield(m_pLuaVm, -2, "__mode");

		// Now commit the metatable.
		lua_setmetatable(m_pLuaVm, -2);

		// Now commit the weak table to the registry.
		lua_rawset(m_pLuaVm, LUA_REGISTRYINDEX);
	}
}

#if SEOUL_ENABLE_DEBUGGER_CLIENT
extern "C"
{

// NOTE: LuaJIT has a bug / bad behavior where it does
// not call HOOKCALL and HOOKRET/HOOKTAILRET consistently
// enough for us to use them for stack depth tracking.
// As a result, we only invoke the Step hook, and
// the body of that method in the debugger must compute
// the stack depth when needed.
static void LuaDebugHook(lua_State* pLuaVm, lua_Debug* pDebug)
{
	switch (pDebug->event)
	{
	case LUA_HOOKLINE:
	{
		DebuggerClientLock lock(DebuggerClient::Get());
		lock.OnStep(pLuaVm, pDebug);
		break;
	}

	case LUA_HOOKCALL:
	case LUA_HOOKCOUNT:
	case LUA_HOOKRET:
	case LUA_HOOKTAILRET:
		break;

	default:
		break;
	};
}

} // extern "C"

void Vm::InsideLockSetDebuggerHooks()
{
	// Doesn't exist, early out.
	if (!DebuggerClient::Get())
	{
		return;
	}

	// Set hook.
	(void)lua_sethook(m_pLuaVm, LuaDebugHook, LUA_MASKCALL | LUA_MASKLINE | LUA_MASKRET, 0);
}
#endif // /SEOUL_ENABLE_DEBUGGER_CLIENT

void Vm::InsideLockDestroyVM()
{
	if (m_pLuaVm.IsValid())
	{
#if !SEOUL_ASSERTIONS_DISABLED
		++s_InVmDestroy;
#endif // /#if !SEOUL_ASSERTIONS_DISABLED

		lua_State* pLuaVm = m_pLuaVm;
		m_pLuaVm.Reset();

#if SEOUL_ENABLE_DEBUGGER_CLIENT
		if (m_Settings.m_bEnableDebuggerHooks)
		{
			// Tell the script debugger that we're going away.
			if (DebuggerClient::Get())
			{
				DebuggerClientLock lock(DebuggerClient::Get());
				lock.OnVmDestroy(pLuaVm);
			}
		}
#endif // /SEOUL_ENABLE_DEBUGGER_CLIENT

		// Close the VM.
		lua_close(pLuaVm);

#if !SEOUL_ASSERTIONS_DISABLED
		--s_InVmDestroy;
#endif // /#if !SEOUL_ASSERTIONS_DISABLED
	}

	m_pDefaultAtPanic = nullptr;
}

Bool Vm::InsideLockRunCode(const String& sCode)
{
	SEOUL_SCRIPT_CHECK_VM_STACK(m_pLuaVm);

	// Load the lua chunk.
	LuaReadContext context(sCode);
	Bool bSuccess = (0 == lua_load(
		m_pLuaVm,
		LuaRead,
		&context,
		"[code]"));

	// If the read succeeded, call the chunk.
	if (bSuccess)
	{
		bSuccess = Pcall(m_pLuaVm, 0, 0);
		if (!bSuccess)
		{
			const char* sErrorMessage = lua_tostring(m_pLuaVm, -1);
			if (nullptr != sErrorMessage)
			{
				ReportError(m_pLuaVm, sErrorMessage);
			}
			lua_pop(m_pLuaVm, 1);
		}
	}
	else if (lua_isstring(m_pLuaVm, -1))
	{
		const char* sErrorMessage = lua_tostring(m_pLuaVm, -1);
		ReportError(m_pLuaVm, sErrorMessage);
		lua_pop(m_pLuaVm, 1);
	}

	return bSuccess;
}

Bool Vm::InsideLockRunScript(
	const String& sRelativeFilename,
	Bool bAddToHotLoadSet /*= true*/)
{
	SEOUL_SCRIPT_CHECK_VM_STACK(m_pLuaVm);

	// If we can't resolve the path, fail immediately.
	FilePath filePath;
	if (!ResolveFilePathFromRelativeFilename(sRelativeFilename, filePath))
	{
		SEOUL_WARN("ScriptVM::RunScript: Error, attempting to run non-existent script: '%s'. Make sure you've cooked.", sRelativeFilename.CStr());
		return false;
	}

#if SEOUL_HOT_LOADING
	if (bAddToHotLoadSet)
	{
		// Before running the script, add it to the list of scripts we've run.
		(void)m_HotLoadData.m_tScripts.Insert(filePath, true);
	}
#endif // /#if SEOUL_HOT_LOADING

	SharedPtr<Script::FileBody> pScript(Manager::Get()->WaitForScript(filePath));
	if (!pScript.IsValid())
	{
		return false;
	}

	// Run the script
	return InternalStaticRunScript(m_pLuaVm, filePath, pScript);
}

#if SEOUL_HOT_LOADING
Bool Vm::OnFileChange(Content::ChangeEvent* pFileChangeEvent)
{
	// Don't insert entries if hot loading is suppressed.
	if (Content::LoadManager::Get()->IsHotLoadingSuppressed())
	{
		return false;
	}

	// Exclusive access to data structures.
	Lock lock(m_Mutex);

	// If the changed file has been run in this VM, schedule it to rerun.
	FilePath const filePath = pFileChangeEvent->m_New;
	if (m_HotLoadData.m_tData.HasValue(filePath) || LocManager::Get()->IsLocManagerFilePath(filePath))
	{
		(void)m_HotLoadData.m_tDataToMonitor.Insert(filePath, SettingsManager::Get()->GetSettings(filePath));
	}

	if (m_HotLoadData.m_tScripts.HasValue(filePath))
	{
		(void)m_HotLoadData.m_tScriptsToMonitor.Insert(filePath, Manager::Get()->GetScript(filePath));
	}

	// We never want to handle this event, we're only using it to handle reloads.
	return false;
}

Bool Vm::OnIsFileLoaded(FilePath filePath)
{
	// Exclusive access to data structures.
	Lock lock(m_Mutex);

	// Always report files that have been run in this VM.
	return m_HotLoadData.m_tScripts.HasValue(filePath);
}

Bool Vm::OnFileLoadComplete(FilePath filePath)
{
	// Project change - always out-of-date.
	if (filePath.GetType() == FileType::kScriptProject ||
		m_HotLoadData.m_tGeneral.HasValue(filePath))
	{
		m_HotLoadData.m_bOutOfDate = true;
	}
	// Check if we should run the script.
	else if (
		filePath.GetType() == FileType::kJson ||
		filePath.GetType() == FileType::kScript)
	{
		// Exclusive access to data structures.
		Lock lock(m_Mutex);

		// If the data file is being monitored.
		if (m_HotLoadData.m_tDataToMonitor.HasValue(filePath))
		{
			SEOUL_VERIFY(m_HotLoadData.m_tDataToMonitor.Erase(filePath));

			// Mark out of date once all monitored scripts and data have been loaded.
			if (m_HotLoadData.m_tDataToMonitor.IsEmpty() &&
				m_HotLoadData.m_tScriptsToMonitor.IsEmpty())
			{
				m_HotLoadData.m_bOutOfDate = true;
			}
		}

		// If the script file is being monitored.
		if (m_HotLoadData.m_tScriptsToMonitor.HasValue(filePath))
		{
			SEOUL_VERIFY(m_HotLoadData.m_tScriptsToMonitor.Erase(filePath));

			// Mark out of date once all monitored scripts and data have been loaded.
			if (m_HotLoadData.m_tDataToMonitor.IsEmpty() &&
				m_HotLoadData.m_tScriptsToMonitor.IsEmpty())
			{
				m_HotLoadData.m_bOutOfDate = true;
			}
		}
	}

	// We never want to handle this event, allow other callbacks to receive it.
	return false;
}

Bool Vm::TryInvokeGlobalOnHotload()
{
	FunctionInvoker invoker(*this, kFunctionSeoulOnHotload);

	if (invoker.IsValid())
	{
		(void)invoker.TryInvoke();
		return true;
	}

	return false;
}

Bool Vm::TryInvokeGlobalPostHotload()
{
	FunctionInvoker invoker(*this, kFunctionSeoulPostHotload);

	if (invoker.IsValid())
	{
		(void)invoker.TryInvoke();
		return true;
	}

	return false;
}

Bool Vm::TryInvokeGlobalRestoreDynamicGameStateData()
{
	FunctionInvoker invoker(*this, kFunctionRestoreDynamicGameStateData);

	if (invoker.IsValid())
	{
		(void)invoker.TryInvoke();
		return true;
	}

	return false;
}

#endif // /#if SEOUL_HOT_LOADING

#if SEOUL_ENABLE_MEMORY_TOOLING
namespace
{

struct MemoryEntry SEOUL_SEALED
{
	ptrdiff_t m_i{};
	void* m_p{};

	Bool operator<(const MemoryEntry& b) const
	{
		return m_i > b.m_i;
	}
}; // struct MemoryEntry

} // namespace anonymous

void Vm::QueryMemoryProfilingData(const MemoryCallback& callback)
{
	Vector<MemoryEntry, MemoryBudgets::Scripting> v;

	// Must remain locked for the duration since we query for data
	// after the sort.
	Lock lock(m_Mutex);
	for (auto const& pair : m_tMemory)
	{
		// Skip entries with no current allocations.
		if (pair.Second <= 0)
		{
			continue;
		}

		MemoryEntry entry;
		entry.m_p = pair.First;
		entry.m_i = pair.Second;
		v.PushBack(entry);
	}

	// Sort.
	QuickSort(v.Begin(), v.End());

	// Report.
	for (auto const& e : v)
	{
		Byte const* sName = nullptr;
		Int iLine = 0;
		Int iNumLines = 0;
		if (0 != SeoulLuaHookGetFuncInfo(m_pLuaVm, e.m_p, &sName, &iLine))
		{
			callback(sName, e.m_i, iLine);
		}
	}
}
#endif // /SEOUL_ENABLE_MEMORY_TOOLING

VmObject::~VmObject()
{
	ReleaseRef();
}

/**
 * Push onto the VM stack the referenced script object. Will push
 * nil if invalid.
 */
void VmObject::PushOntoVmStack(lua_State* pVm)
{
	SEOUL_SCRIPT_CHECK_VM_STACK(pVm, 1);

	// In all cases, if the object has a nil reference, push nil.
	if (LUA_REFNIL == m_iRef)
	{
		lua_pushnil(pVm);
	}
	else
	{
		// Check for mismatched VMs - we need to copy through the tables
		// if the object is from a different VM.
		if (m_hVm != GetScriptVm(pVm)->GetHandle())
		{
			auto pScriptVm(GetPtr(m_hVm));
			if (!pScriptVm.IsValid())
			{
				lua_pushnil(pVm);
			}
			else
			{
				InternalGetRef(pScriptVm->m_pLuaVm);
				PushClone(pVm, pScriptVm->m_pLuaVm);
				lua_pop(pScriptVm->m_pLuaVm, 1);
			}
		}
		else
		{
			// Otherwise, acquire the ref.
			InternalGetRef(pVm);
		}
	}
}

/**
 * Sets the reference state of this VM object to nil. This sets
 * the reference to nil.
 */
void VmObject::ReleaseRef()
{
	// Nothing more to do if m_iObject is nil.
	if (LUA_REFNIL == m_iRef)
	{
		return;
	}

	// Nothing to do if our Vm is gone.
	SharedPtr<Vm> pVm(GetPtr(m_hVm));
	if (!pVm.IsValid())
	{
		m_iRef = LUA_REFNIL;
		return;
	}

	// Keep access to the VM exclusive.
	Lock lock(pVm->m_Mutex);
	CheckedPtr<lua_State> vm(pVm->m_pLuaVm);

	SEOUL_SCRIPT_CHECK_VM_STACK(vm);

	// Release the reference.
	InternalUnref(vm);
}

/**
 * Used for management of Script::VmObject's created with
 * Script::Vm::BindWeakInstance(). Calling this method sets
 * the internal binding to nil, which is useful if the
 * bound native instance is destroyed prior to Lua's ownership
 * of the binding.
 */
void VmObject::SetWeakBindingToNil()
{
	// Nothing to do if we're an invalid object.
	if (LUA_REFNIL == m_iRef)
	{
		return;
	}

	// Nothing to do if our Vm is gone.
	SharedPtr<Vm> pVm(GetPtr(m_hVm));
	if (!pVm.IsValid())
	{
		return;
	}

	Lock lock(pVm->m_Mutex);
	CheckedPtr<lua_State> vm(pVm->m_pLuaVm);

	SEOUL_SCRIPT_CHECK_VM_STACK(vm);

	InternalGetRef(vm);
	if (0 == lua_isuserdata(vm, -1))
	{
		// Pop the instance before returning.
		lua_pop(vm, 1);
		return;
	}

	if (0 == lua_getmetatable(vm, -1))
	{
		// Pop the instance before returning.
		lua_pop(vm, 1);
		return;
	}

	lua_pushlightuserdata(vm, kpScriptTypeKey);
	lua_rawget(vm, -2);
	void* pScriptTypeUserData = lua_touserdata(vm, -1);
	if (nullptr == pScriptTypeUserData)
	{
		// Pop the type entry, the metatable, and the instance.
		lua_pop(vm, 3);
		return;
	}

	// pop the Script::TypeUserData entry and the metatable, now that we
	// have a pointer to the type information.
	lua_pop(vm, 2);

	// Check if the type is weak, and if so, clear it.
	TypeUserData const& luaScriptTypeUserData = *((TypeUserData*)pScriptTypeUserData);
	if (luaScriptTypeUserData.m_bWeak)
	{
		void** ppSeoulEngineUserData = (void**)lua_touserdata(vm, -1);
		if (nullptr != ppSeoulEngineUserData)
		{
			*ppSeoulEngineUserData = nullptr;
		}
	}

	// Pop the user data.
	lua_pop(vm, 1);
}

/**
 * If a compatible type, serializes the script object
 * to rDataStore.
 */
Bool VmObject::TryToDataStore(DataStore& rDataStore) const
{
	// Invoke on Nil.
	if (LUA_REFNIL == m_iRef)
	{
		return false;
	}

	// Dangling, invoke on released Vm.
	SharedPtr<Vm> pVm(GetPtr(m_hVm));
	if (!pVm.IsValid())
	{
		return false;
	}

	// Keep access to the VM exclusive.
	Lock lock(pVm->m_Mutex);
	CheckedPtr<lua_State> vm(pVm->m_pLuaVm);

	SEOUL_SCRIPT_CHECK_VM_STACK(vm);

	InternalGetRef(vm);
	Bool const bReturn = TableToDataStore(vm, -1, rDataStore);
	lua_pop(vm, 1);

	return bReturn;
}

VmObject::VmObject(const VmHandle& hVm, Int32 iRef)
	: m_hVm(hVm)
	, m_iRef(iRef)
{
}

void VmObject::InternalGetRef(lua_State* pVm) const
{
	// Make sure we're manipulating the stack properly.
	SEOUL_SCRIPT_CHECK_VM_STACK(pVm, 1);

	lua_getref(pVm, m_iRef);
}

void VmObject::InternalRef(lua_State* pVm)
{
	SEOUL_SCRIPT_CHECK_VM_STACK(pVm, -1);

	m_iRef = lua_ref(pVm, 1);
}

void VmObject::InternalUnref(lua_State* pVm)
{
	SEOUL_SCRIPT_CHECK_VM_STACK(pVm);

	lua_unref(pVm, m_iRef);
	m_iRef = LUA_REFNIL;
}

} // namespace Script

Bool DataNodeHandler< SharedPtr<Script::VmObject>, false>::FromDataNode(Reflection::SerializeContext&, const DataStore& dataStore, const DataNode& dataNode, SharedPtr<Script::VmObject>& rp)
{
	// Serialization not supported.
	return false;
}

Bool DataNodeHandler< SharedPtr<Script::VmObject>, false>::ToArray(Reflection::SerializeContext&, DataStore& rDataStore, const DataNode& array, UInt32 uIndex, const SharedPtr<Script::VmObject>& p)
{
	// Serialization not supported.
	return false;
}

Bool DataNodeHandler< SharedPtr<Script::VmObject>, false>::ToTable(Reflection::SerializeContext&, DataStore& rDataStore, const DataNode& table, HString key, const SharedPtr<Script::VmObject>& p)
{
	// Serialization not supported.
	return false;
}

void DataNodeHandler< SharedPtr<Script::VmObject>, false>::FromScript(lua_State* pVm, Int iOffset, SharedPtr<Script::VmObject>& rp)
{
	// Support nil values.
	if (lua_isnil(pVm, iOffset))
	{
		rp.Reset();
	}
	// Otherwise, instantiate a Script::VmObject, which is
	// a strong reference to the value in Lua (via
	// the Lua registry table).
	else
	{
		lua_pushvalue(pVm, iOffset);
		auto const iObject = lua_ref(pVm, LUA_REGISTRYINDEX);
		rp.Reset(SEOUL_NEW(MemoryBudgets::Scripting) Script::VmObject(
			Script::GetScriptVm(pVm)->GetHandle(),
			iObject));
	}
}

void DataNodeHandler< SharedPtr<Script::VmObject>, false>::ToScript(lua_State* pVm, const SharedPtr<Script::VmObject>& p)
{
	// Support null values.
	if (!p.IsValid())
	{
		lua_pushnil(pVm);
	}
	else
	{
		p->PushOntoVmStack(pVm);
	}
}

Bool DataNodeHandler< Script::ByteBuffer, false>::FromDataNode(Reflection::SerializeContext&, const DataStore& dataStore, const DataNode& dataNode, Script::ByteBuffer& rv)
{
	// Serialization not supported.
	return false;
}

Bool DataNodeHandler< Script::ByteBuffer, false>::ToArray(Reflection::SerializeContext&, DataStore& rDataStore, const DataNode& array, UInt32 uIndex, const Script::ByteBuffer& v)
{
	// Serialization not supported.
	return false;
}

Bool DataNodeHandler< Script::ByteBuffer, false>::ToTable(Reflection::SerializeContext&, DataStore& rDataStore, const DataNode& table, HString key, const Script::ByteBuffer& v)
{
	// Serialization not supported.
	return false;
}

void DataNodeHandler< Script::ByteBuffer, false>::FromScript(lua_State* pVm, Int iOffset, Script::ByteBuffer& rv)
{
	size_t z = 0;
	Byte const* s = lua_tolstring(pVm, iOffset, &z);

	rv.m_pData = (void*)s;
	rv.m_zDataSizeInBytes = (UInt32)z;
}

void DataNodeHandler< Script::ByteBuffer, false>::ToScript(lua_State* pVm, const Script::ByteBuffer& v)
{
	lua_pushlstring(pVm, (Byte const*)v.m_pData, (size_t)v.m_zDataSizeInBytes);
}

void seoul_lua_createclasstable(lua_State* pVm, const char* sClassName, int iArraySize, int iRecordSize)
{
	lua_createtable(pVm, iArraySize, iRecordSize);
	lua_getglobal(pVm, sClassName);
	lua_setmetatable(pVm, -2);
}

} // namespace Seoul
