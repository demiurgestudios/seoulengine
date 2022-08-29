/**
 * \file ReflectionPolymorphic.h
 * \brief Utilities for interacting with polymorphic types via and
 * in the Reflection system.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef REFLECTION_POLYMORPHIC_H
#define REFLECTION_POLYMORPHIC_H

#include "Prereqs.h"
#include "ReflectionWeakAny.h"
namespace Seoul { class DataNode; }
namespace Seoul { class DataStore; }
extern "C" { struct lua_State; }

namespace Seoul::Reflection
{

// Utility function for instantiating polymorphic types from
// DataStore definitions.
WeakAny PolymorphicNew(const Type& type, const DataStore& dataStore, const DataNode& table);

// Utility function for instantiating polymorphic types from
// script VM state.
WeakAny PolymorphicNew(const Type& type, lua_State* pVm, Int iOffset);

/**
* Utility structure - calls GetReflectionThis() on a type
* if it is polymorphic, otherwise just wraps the type in a WeakAny.
*/
template <typename T, Bool B_FULFILLS_CONTRACT>
struct PolymorphicThis
{
	static WeakAny Get(T* p)
	{
		SEOUL_ASSERT(nullptr != p);
		return p->GetReflectionThis();
	}
};
template <typename T>
struct PolymorphicThis<T, false>
{
	static WeakAny Get(T* p)
	{
		SEOUL_ASSERT(nullptr != p);
		return WeakAny(p);
	}
};

/**
* @return p wrapped in a WeakAny. If p is polymorphic, the returned
* WeakAny will contain a most derived pointer of p, as returned by p->GetReflectionThis().
*/
template <typename T>
inline WeakAny GetPolymorphicThis(T* p)
{
	return PolymorphicThis<T, IsReflectionPolymorphic<T>::Value>::Get(p);
}

} // namespace Seoul::Reflection

#endif // include guard
