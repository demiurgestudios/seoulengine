/**
 * \file ScriptArrayIndex.cpp
  * \brief Provides automatic array index conversion back and forth between
 * C++ (0 based) and Lua (1 based)
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ReflectionDefine.h"
#include "ScriptArrayIndex.h"

namespace Seoul
{

SEOUL_TYPE(Script::ArrayIndex);

Bool DataNodeHandler<Script::ArrayIndex, false>::FromDataNode(Reflection::SerializeContext& context, const DataStore& dataStore, const DataNode& dataNode, Script::ArrayIndex& rIndex)
{
	UInt32 u = 0u;
	if (DataNodeHandler<UInt32, false>::FromDataNode(context, dataStore, dataNode, u))
	{
		rIndex = Script::ArrayIndex(u);
	}

	return false;
}

Bool DataNodeHandler<Script::ArrayIndex, false>::ToArray(Reflection::SerializeContext& context, DataStore& rDataStore, const DataNode& array, UInt32 uIndex, const Script::ArrayIndex& index)
{
	return DataNodeHandler<UInt32, false>::ToArray(context, rDataStore, array, uIndex, (UInt32)index);
}

Bool DataNodeHandler<Script::ArrayIndex, false>::ToTable(Reflection::SerializeContext& context, DataStore& rDataStore, const DataNode& table, HString key, const Script::ArrayIndex& index)
{
	return DataNodeHandler<UInt32, false>::ToTable(context, rDataStore, table, key, (UInt32)index);
}

void DataNodeHandler<Script::ArrayIndex, false>::FromScript(lua_State* pVm, Int iOffset, Script::ArrayIndex& rIndex)
{
	lua_Number const fIn = lua_tonumber(pVm, iOffset);
	if (fIn <= 0)
	{
		rIndex = Script::ArrayIndex(UIntMax);
	}
	else
	{
		rIndex = Script::ArrayIndex((UInt32)(fIn - 1));
	}
}

void DataNodeHandler<Script::ArrayIndex, false>::ToScript(lua_State* pVm, const Script::ArrayIndex& index)
{
	lua_pushnumber(pVm, (lua_Number)((UInt32)index + 1u));
}

} // namespace Seoul
