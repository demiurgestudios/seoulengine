/**
 * \file ContentKey.cpp
 * \brief Generic representation of content. Implements a very
 * common pattern of FilePath + HString data.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ContentKey.h"

namespace Seoul
{

/**
 * Set the values of this ContentKey from a DataStore value value.
 *
 * The DataStore value is expected to be an array with 2 elements, example:
 * (config://foo.json, "My Data")
 * (config://foo.json, My_Data)
 *
 * Either a string or identifier type is valid in the second position.
 */
Bool ContentKey::SetFromDataStore(const DataStore& dataStore, const DataNode& value)
{
	DataNode file;
	DataNode dataValue;
	FilePath filePath;
	HString data;

	// If value is just a file path, then treat this as a content key with no data.
	if (dataStore.AsFilePath(value, filePath))
	{
		m_FilePath = filePath;
		m_Data = HString();
		return true;
	}

	// Otherwise, value must be an array of 2 values, the first is the file path,
	// the second is either a String or an Identifier that is the data of the ContentKey.
	if (!dataStore.GetValueFromArray(value, 0u, file) ||
		!dataStore.GetValueFromArray(value, 1u, dataValue) ||
		!dataStore.AsFilePath(file, filePath))
	{
		return false;
	}

	Byte const* sData = nullptr;
	UInt32 zDataLengthInBytes = 0u;
	if (dataStore.AsString(dataValue, sData, zDataLengthInBytes))
	{
		// Content key data is case insensitive.
		data = HString(sData, zDataLengthInBytes, true);
	}
	else
	{
		return false;
	}

	m_FilePath = filePath;
	m_Data = data;
	return true;
}

/**
 * Set the values of this ContentKey to a DataStore node, contained
 * in an array at index uIndex.
 */
Bool ContentKey::SetToDataStoreArray(
	DataStore& rDataStore,
	const DataNode& array,
	UInt32 uIndex) const
{
	if (!rDataStore.SetArrayToArray(array, uIndex, 2u))
	{
		return false;
	}

	DataNode node;
	if (!rDataStore.GetValueFromArray(array, uIndex, node))
	{
		return false;
	}

	if (!rDataStore.SetFilePathToArray(node, 0u, m_FilePath) ||
		!rDataStore.SetStringToArray(node, 1u, m_Data))
	{
		return false;
	}

	return true;
}

/**
 * Set the values of this ContentKey to a DataStore node, contained
 * in a table at key.
 */
Bool ContentKey::SetToDataStoreTable(
	DataStore& rDataStore,
	const DataNode& table,
	HString key) const
{
	if (!rDataStore.SetArrayToTable(table, key, 2u))
	{
		return false;
	}

	DataNode node;
	if (!rDataStore.GetValueFromTable(table, key, node))
	{
		return false;
	}

	if (!rDataStore.SetFilePathToArray(node, 0u, m_FilePath) ||
		!rDataStore.SetStringToArray(node, 1u, m_Data))
	{
		return false;
	}

	return true;
}

/**
 * Returns a human readable representation of this
 * ContentKey.
 */
String ContentKey::ToString() const
{
	if (m_Data.IsEmpty())
	{
		return m_FilePath.GetRelativeFilename();
	}
	else
	{
		return m_FilePath.GetRelativeFilename() + "(" + String(m_Data) + ")";
	}
}

} // namespace Seoul
