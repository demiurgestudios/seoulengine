/**
 * \file ScriptEngineHTTPRequestReflection.cpp
 * \brief Reflection specific implementation of
 * the HTTP::Request script binder.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "HTTPManager.h"
#include "ReflectionDefine.h"
#include "ScriptEngineHTTPRequest.h"
#include "ScriptFunctionInterface.h"

namespace Seoul
{

SEOUL_BEGIN_TYPE(ScriptEngineHTTPRequest, TypeFlags::kDisableCopy)
	SEOUL_METHOD(AddHeader)
	SEOUL_METHOD(AddPostData)
	SEOUL_METHOD(SetLanesMask)
	SEOUL_METHOD(Start)
	SEOUL_METHOD(StartWithPlatformSignInIdToken)
	SEOUL_METHOD(Cancel)
SEOUL_END_TYPE()

/** Serialization path in all cases, set headers to a table. */
static Bool Serialize(
	const ScriptEngineHTTPHeaderTable& headerTable,
	DataStore& rDataStore,
	const DataNode& table)
{
	// Iterate over headers.
	const auto& tHeaders = headerTable.m_pTable->GetKeyValues();
	auto const iBegin = tHeaders.Begin();
	auto const iEnd = tHeaders.End();
	for (auto i = iBegin; iEnd != i; ++i)
	{
		// Cache the key and value.
		HString const key = i->First;
		HTTP::HeaderTable::ValueType const value = i->Second;

		// First try to parse the value - if this succeeds, we're done,
		// otherwise we just set it straight as a string.
		if (!DataStoreParser::FromString(value.m_sValue, value.m_zValueSizeInBytes, rDataStore))
		{
			// Set the value as a string.
			if (!rDataStore.SetStringToTable(
				table,
				key,
				value.m_sValue,
				value.m_zValueSizeInBytes))
			{
				// This should never happen, but we handle it gracefully
				// in case that changes.
				return false;
			}
		}
	}

	return true;
}

Bool DataNodeHandler<ScriptEngineHTTPHeaderTable, false>::FromDataNode(Reflection::SerializeContext& context, const DataStore& dataStore, const DataNode& dataNode, ScriptEngineHTTPHeaderTable& rvalue)
{
	return false;
}

Bool DataNodeHandler<ScriptEngineHTTPHeaderTable, false>::ToArray(Reflection::SerializeContext&, DataStore& rDataStore, const DataNode& array, UInt32 uIndex, const ScriptEngineHTTPHeaderTable& value)
{
	// Insert a table.
	if (!rDataStore.SetTableToArray(array, uIndex))
	{
		return false;
	}

	// Get the table and finish serializing.
	DataNode toTable;
	SEOUL_VERIFY(rDataStore.GetValueFromArray(array, uIndex, toTable));

	return Serialize(value, rDataStore, toTable);
}

Bool DataNodeHandler<ScriptEngineHTTPHeaderTable, false>::ToTable(Reflection::SerializeContext&, DataStore& rDataStore, const DataNode& table, HString key, const ScriptEngineHTTPHeaderTable& value)
{
	// Insert a table.
	if (!rDataStore.SetTableToTable(table, key))
	{
		return false;
	}

	// Get the table and finish serializing.
	DataNode toTable;
	SEOUL_VERIFY(rDataStore.GetValueFromTable(table, key, toTable));

	return Serialize(value, rDataStore, toTable);
}

void DataNodeHandler<ScriptEngineHTTPHeaderTable, false>::FromScript(lua_State* pVm, Int iOffset, ScriptEngineHTTPHeaderTable& r)
{
	SEOUL_FAIL("TODO: Implement.");
}

void DataNodeHandler<ScriptEngineHTTPHeaderTable, false>::ToScript(lua_State* pVm, const ScriptEngineHTTPHeaderTable& v)
{
	DataStore dataStore;
	dataStore.MakeTable();
	Script::FunctionInterface scriptFunctionInterface(pVm);
	if(!Serialize(v, dataStore, dataStore.GetRootNode()))
	{
		scriptFunctionInterface.PushReturnNil();
		return;
	}

	scriptFunctionInterface.PushReturnDataNode(dataStore, dataStore.GetRootNode());
}

SEOUL_BEGIN_TYPE(ScriptEngineHTTPHeaderTable, TypeFlags::kDisableNew)
SEOUL_END_TYPE()

} // namespace Seoul
