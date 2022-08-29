/**
 * \file ScriptArrayIndex.h
 * \brief Provides automatic array index conversion back and forth between
 * C++ (0 based) and Lua (1 based)
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCRIPT_ARRAY_INDEX_H
#define SCRIPT_ARRAY_INDEX_H

#include "Prereqs.h"
#include "ReflectionUtil.h"

namespace Seoul
{

namespace Script
{

class ArrayIndex SEOUL_SEALED
{
public:
	ArrayIndex()
		: m_u(0)
	{
	}

	ArrayIndex(const ArrayIndex& b)
		: m_u(b.m_u)
	{
	}

	explicit ArrayIndex(UInt32 u)
		: m_u(u)
	{
	}

	operator UInt32() const
	{
		return m_u;
	}

	ArrayIndex operator+(const ArrayIndex& b) const
	{
		return ArrayIndex(m_u + b.m_u);
	}

	ArrayIndex operator-(const ArrayIndex& b) const
	{
		return ArrayIndex(m_u - b.m_u);
	}

	ArrayIndex& operator+=(const ArrayIndex& b)
	{
		m_u += b.m_u;
		return *this;
	}

	ArrayIndex& operator-=(const ArrayIndex& b)
	{
		m_u -= b.m_u;
		return *this;
	}

	// pre-increment
	ArrayIndex& operator++()
	{
		++m_u;
		return *this;
	}

	// post-increment
	ArrayIndex operator++(int)
	{
		ArrayIndex temp(m_u);
		++m_u;
		return temp;
	}

	// pre-decrement
	ArrayIndex& operator--()
	{
		--m_u;
		return *this;
	}

	// post-decrement
	ArrayIndex operator--(int)
	{
		ArrayIndex temp(m_u);
		--m_u;
		return temp;
	}

private:
	UInt32 m_u;
}; // class Script::ArrayIndex

} // namespace Script

/**
 * Special DataNodeHandler for array indicies that auto converts from 0 based indices in C++
 * to 1 based indices in Lua
 */
template <>
struct DataNodeHandler<Script::ArrayIndex, false>
{
	static const Bool Value = true;

	static Bool FromDataNode(Reflection::SerializeContext& context, const DataStore& dataStore, const DataNode& dataNode, Script::ArrayIndex& rIndex);
	static Bool ToArray(Reflection::SerializeContext& context, DataStore& rDataStore, const DataNode& array, UInt32 uIndex, const Script::ArrayIndex& index);
	static Bool ToTable(Reflection::SerializeContext& context, DataStore& rDataStore, const DataNode& table, HString key, const Script::ArrayIndex& index);
	static void FromScript(lua_State* pVm, Int iOffset, Script::ArrayIndex& rIndex);
	static void ToScript(lua_State* pVm, const Script::ArrayIndex& index);
};

} // namespace Seoul

#endif // include guard
