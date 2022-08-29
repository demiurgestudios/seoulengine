/**
 * \file ReflectionTable.h
 * \brief Reflection::Table is an addendum source of reflection information, supplemental
 * to Reflection::Type. It provides operations to allow manipulations on a type that fulfills
 * the generic contract of a table, including:
 * - access by key
 * - (optional) erase by key
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef REFLECTION_TABLE_H
#define REFLECTION_TABLE_H

#include "ReflectionAny.h"
#include "ReflectionPrereqs.h"
extern "C" { struct lua_State; }

namespace Seoul::Reflection
{

namespace TableFlags
{
	/**
	 * Describe various features of the table.
	 */
	enum Enum
	{
		kNone = 0,

		/** If set, TryErase() can succeed, otherwise it will always return false. */
		kErase = (1 << 0),
	};
}

class TableEnumerator SEOUL_ABSTRACT
{
public:
	virtual ~TableEnumerator();

	virtual Bool TryGetNext(Any& rKey, Any& rValue) = 0;

protected:
	TableEnumerator();

private:
	SEOUL_DISABLE_COPY(TableEnumerator);
}; // class TableEnumerator

class Table SEOUL_ABSTRACT
{
public:
	virtual ~Table();

	// Return the TypeInfo of keys of this Table.
	virtual const TypeInfo& GetKeyTypeInfo() const = 0;

	// Return the TypeInfo of values of this Table.
	virtual const TypeInfo& GetValueTypeInfo() const = 0;

	// Allocator a forward enumerator for the table. Returns
	// nullptr on error. If non-null, the returned enumerator
	// must be deallocated with SEOUL_DELETE/SafeDelete().
	virtual TableEnumerator* NewEnumerator(const WeakAny& tablePointer) const = 0;

	// Attempt to assign a read-write pointer to the value at key to rValue. If
	// bInsert is true, a new element with a default value will be inserted if
	// it does not already exist in the table, and a pointer to that value will
	// be returned.
	virtual Bool TryGetValuePtr(const WeakAny& tablePointer, const WeakAny& key, WeakAny& rValue, Bool bInsert = false) const = 0;

	// Attempt to assign a read-only pointer to the value at key to rValue.
	virtual Bool TryGetValueConstPtr(const WeakAny& tablePointer, const WeakAny& key, WeakAny& rValue) const = 0;

	// Attempt to erase the value at key from the table.
	virtual Bool TryErase(const WeakAny& tablePointer, const WeakAny& key) const = 0;

	// Call this method to attempt to set data to this Table, into the instance in thisPointer.
	virtual Bool TryOverwrite(const WeakAny& thisPointer, const WeakAny& key, const WeakAny& value) const = 0;

	// Populate the table in objectThis with the script table at iOffset.
	virtual void FromScript(lua_State* pVm, Int iOffset, const WeakAny& objectThis) const = 0;

	// Push a table into script that matches the content of the table objectThis.
	virtual void ToScript(lua_State* pVm, const WeakAny& objectThis) const = 0;

	/** @return True if TryErase() can be used, false otherwise. */
	Bool CanErase() const { return (0u != (TableFlags::kErase & m_uFlags)); }

	// Call to attempt to deserialize table data in dataNode into the table in objectThis.
	virtual Bool TryDeserialize(
		SerializeContext& rContext,
		const DataStore& dataStore,
		const DataNode& table,
		const WeakAny& objectThis,
		Bool bSkipPostSerialize = false) const = 0;

	// Call to attempt to serialize a table object pointed at by objectThis into the DataStore table.
	virtual Bool TrySerialize(
		SerializeContext& rContext,
		DataStore& rDataStore,
		const DataNode& table,
		const WeakAny& objectThis,
		Bool bSkipPostSerialize = false) const = 0;

protected:
	Table(UInt32 uFlags);

private:
	// Not default constructable
	Table();

	UInt32 m_uFlags;

	SEOUL_DISABLE_COPY(Table);
}; // class Table

} // namespace Seoul::Reflection

#endif // include guard
