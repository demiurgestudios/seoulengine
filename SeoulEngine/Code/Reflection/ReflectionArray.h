/**
 * \file ReflectionArray.h
 * \brief Reflection::Array is an addendum source of reflection information, supplemental
 * to Reflection::Type. It provides operations to allow manipulations on a type that fulfills
 * the generic contract of an array, including:
 * - access by element
 * - resize of the array (optional, can be excluded for fixed size arrays)
 * - length query of the array
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef REFLECTION_ARRAY_H
#define REFLECTION_ARRAY_H

#include "ReflectionAny.h"
#include "ReflectionPrereqs.h"
extern "C" { struct lua_State; }

namespace Seoul::Reflection
{

namespace ArrayFlags
{
	/**
	 * Describe various features of the array.
	 */
	enum Enum
	{
		kNone = 0,

		/** If set, TryResize() can succeed, otherwise it will always return false. */
		kResizable = (1 << 0),
	};
}

class Array SEOUL_ABSTRACT
{
public:
	virtual ~Array();

	// Return the TypeInfo of elements of this Array.
	virtual const TypeInfo& GetElementTypeInfo() const = 0;

	// Attempt to get a copy of the element at uIndex.
	virtual Bool TryGet(const WeakAny& arrayPointer, UInt32 uIndex, Any& rValue) const = 0;

	// Attempt to assign a read-write pointer to the element at uIndex to rValue.
	virtual Bool TryGetElementPtr(const WeakAny& arrayPointer, UInt32 uIndex, WeakAny& rValue) const = 0;

	// Attempt to assign a read-only pointer to the element at uIndex to rValue.
	virtual Bool TryGetElementConstPtr(const WeakAny& arrayPointer, UInt32 uIndex, WeakAny& rValue) const = 0;

	// Attempt to retrieve the size of arrayPointer.
	virtual Bool TryGetSize(const WeakAny& arrayPointer, UInt32& rzSize) const = 0;

	// Attempt to resize arrayPointer to zSize, return success or failure.
	virtual Bool TryResize(const WeakAny& arrayPointer, UInt32 zNewSize) const = 0;

	// Attempt to update the element at uIndex to value
	virtual Bool TrySet(const WeakAny& arrayPointer, UInt32 uIndex, const WeakAny& value) const = 0;

	// Populate the array in objectThis with the script table at iOffset.
	virtual void FromScript(lua_State* pVm, Int iOffset, const WeakAny& objectThis) const = 0;

	// Push a table into script that matches the content of the array objectThis.
	virtual void ToScript(lua_State* pVm, const WeakAny& objectThis) const = 0;

	/** @return True if TryResize() can be used, false otherwise. */
	Bool CanResize() const { return (0u != (ArrayFlags::kResizable & m_uFlags)); }

	// Attempt to deserialize the data in dataNode into objectThis, assuming objectThis is an array.
	virtual Bool TryDeserialize(
		SerializeContext& rContext,
		const DataStore& dataStore,
		const DataNode& array,
		const WeakAny& objectThis,
		Bool bSkipPostSerialize = false) const = 0;

	// Attempt to serialize the state of an array pointed at by objectThis into the DataStore array.
	virtual Bool TrySerialize(
		SerializeContext& rContext,
		DataStore& rDataStore,
		const DataNode& array,
		const WeakAny& objectThis,
		Bool bSkipPostSerialize = false) const = 0;

protected:
	Array(UInt32 uFlags);

private:
	// Not default constructable
	Array();

	UInt32 m_uFlags;

	SEOUL_DISABLE_COPY(Array);
}; // class Array

} // namespace Seoul::Reflection

#endif // include guard
