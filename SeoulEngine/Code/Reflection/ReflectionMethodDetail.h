/**
 * \file ReflectionMethodDetail.h
 * \brief Internal header file, used as part of a series of
 * recursive header file includes to generate concrete definitions
 * of a generic subclass of Reflection::Method.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef REFLECTION_METHOD_DETAIL_H
#define REFLECTION_METHOD_DETAIL_H

#include "ReflectionAny.h"
#include "ReflectionDataStoreUtil.h"
#include "ReflectionMethod.h"
#include "ReflectionMethodTypeInfo.h"
#include "ReflectionPrereqs.h"
#include "ReflectionScript.h"
#include "ReflectionType.h"
#include "ReflectionWeakAny.h"
namespace Seoul { namespace Script { class FunctionInterface; } }

namespace Seoul::Reflection
{

namespace MethodDetail
{

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Utility used by the shared invoker to get a conrete value to the this pointer.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template <typename C>
struct MethodInvokerPointerCast
{
	static Bool Cast(const WeakAny& thisPointer, C*& rp)
	{
		return PointerCast(thisPointer, rp);
	}
}; // struct MethodInvokerPointerCast

template <>
struct MethodInvokerPointerCast<void>
{
	static Bool Cast(const WeakAny&, void*& rp)
	{
		rp = nullptr;
		return true;
	}
}; // struct MethodInvokerPointerCast

template <typename T, Bool HAS_HANDLER>
struct ScriptUtil SEOUL_SEALED
{
	static inline void From(lua_State* pVm, Int iOffset, T& r)
	{
		TypeOf<T>().FromScript(pVm, iOffset, &r);
	}
	static inline void To(lua_State* pVm, const T& v)
	{
		TypeOf<T>().ToScript(pVm, &v);
	}
};

template <typename T>
struct ScriptUtil<T, true> SEOUL_SEALED
{
	static inline void From(lua_State* pVm, Int iOffset, T& r)
	{
		DataNodeHandler<T, IsEnum<T>::Value>::FromScript(pVm, iOffset, r);
	}
	static inline void To(lua_State* pVm, const T& v)
	{
		DataNodeHandler<T, IsEnum<T>::Value>::ToScript(pVm, v);
	}
};

template <typename T>
static inline void FromScriptVm(lua_State* pVm, Int iOffset, T& r)
{
	ScriptUtil<T, DataNodeHandler<T, IsEnum<T>::Value>::Value>::From(pVm, iOffset, r);
}

template <typename T>
static inline void ToScriptVm(lua_State* pVm, const T& v)
{
	ScriptUtil<T, DataNodeHandler<T, IsEnum<T>::Value>::Value>::To(pVm, v);
}
	
#define SEOUL_BIND_SCRIPT_CLOSURE() \
	lua_pushcclosure(pVm, InvokerType::ScriptInvoke, 0)

// Implementation Method classes for each variation - see
// ReflectionMethodVariations.h for the variations currently implemented,
// and ReflectionMethodDetailInternal.h for the template that is used
// to implement the Method class for each variation.
#define SEOUL_METHOD_VARIATION_IMPL_FILENAME "ReflectionMethodDetailInternal.h"
#include "ReflectionMethodVariations.h"

} // namespace MethodDetail

} // namespace Seoul::Reflection

#endif // include guard
