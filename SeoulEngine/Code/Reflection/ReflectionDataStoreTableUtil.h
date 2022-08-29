/**
 * \file ReflectionDataStoreTableUtil.h
 * \brief A convenience wrapper for a DataStore. Exposes common operations
 * on a DataStore sub table with a simpler API than direct usage of
 * a DataStore.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef REFLECTION_DATA_STORE_TABLE_UTIL_H
#define REFLECTION_DATA_STORE_TABLE_UTIL_H

#include "DataStore.h"
#include "ReflectionDataStoreUtil.h"
#include "ReflectionDeserialize.h"
#include "ReflectionSerialize.h"

namespace Seoul
{

namespace DataStoreUtilCommon
{

template <typename T, Bool B_HAS_DATA_STORE_UTIL> struct Handler;

template <typename T>
struct Handler<T, true>
{
	static Bool GetValue(
		const DataStore& dataStore,
		const DataNode& dataNode,
		T& rValue)
	{
		Reflection::DefaultSerializeContext context(ContentKey(), dataStore, dataNode, TypeId<T>());
		return DataNodeHandler< T, IsEnum<T>::Value >::FromDataNode(
			context,
			dataStore,
			dataNode,
			rValue);
	}

	static Bool SetValue(
		DataStore& dataStore,
		const DataNode& dataNode,
		HString key,
		const T& value)
	{
		Reflection::DefaultSerializeContext context(ContentKey(), dataStore, dataNode, TypeId<T>());
		return DataNodeHandler< T, IsEnum<T>::Value >::ToTable(
			context,
			dataStore,
			dataNode,
			key,
			value);
	}

	static Bool SetValue(
		DataStore& dataStore,
		const DataNode& dataNode,
		UInt32 uIndex,
		const T& value)
	{
		Reflection::DefaultSerializeContext context(ContentKey(), dataStore, dataNode, TypeId<T>());
		return DataNodeHandler< T, IsEnum<T>::Value >::ToArray(
			context,
			dataStore,
			dataNode,
			uIndex,
			value);
	}
}; // struct Handler<T, true>

template <typename T>
struct Handler<T, false>
{
	static Bool GetValue(
		const DataStore& dataStore,
		const DataNode& dataNode,
		T& rValue)
	{
		return Reflection::DeserializeObject(
			ContentKey(),
			dataStore,
			dataNode,
			&rValue);
	}

	static Bool SetValue(
		DataStore& dataStore,
		const DataNode& dataNode,
		HString key,
		const T& value)
	{
		return Reflection::SerializeObjectToTable(
			ContentKey(),
			dataStore,
			dataNode,
			key,
			&value);
	}

	static Bool SetValue(
		DataStore& dataStore,
		const DataNode& dataNode,
		UInt32 uIndex,
		const T& value)
	{
		return Reflection::SerializeObjectToTable(
			ContentKey(),
			dataStore,
			dataNode,
			uIndex,
			&value);
	}
}; // struct Handler<T, false>

} // namespace DataStoreUtilCommon

class DataStoreArrayUtil
{
public:
	typedef DataStore::TableIterator Iterator;

	DataStoreArrayUtil(const DataStore& dataStore, const DataNode& dataNode)
		: m_DataStore(dataStore)
		, m_DataNode(dataNode)
	{
	}

	UInt32 GetCount() const
	{
		UInt32 uReturn = 0u;
		(void)m_DataStore.GetArrayCount(m_DataNode, uReturn);
		return uReturn;
	}

	template <typename T>
	Bool GetValue(UInt32 u, T& rValue) const
	{
		DataNode valueNode;
		if (!m_DataStore.GetValueFromArray(m_DataNode, u, valueNode))
		{
			return false;
		}

		return DataStoreUtilCommon::Handler< T, DataNodeHandler< T, IsEnum<T>::Value >::Value >::GetValue(
			m_DataStore,
			valueNode,
			rValue);
	}

protected:
	const DataStore& m_DataStore;
	const DataNode m_DataNode;

private:
	SEOUL_DISABLE_COPY(DataStoreArrayUtil);
}; // class DataStoreArrayUtil

class MutableDataStoreArrayUtil SEOUL_SEALED : public DataStoreArrayUtil
{
public:
	MutableDataStoreArrayUtil(DataStore& dataStore, const DataNode& dataNode)
		: DataStoreArrayUtil(dataStore, dataNode)
		, m_rDataStore(dataStore)
	{
	}

	template <typename T>
	Bool SetValue(UInt32 uIndex, const T& value)
	{
		return DataStoreUtilCommon::Handler< T, DataNodeHandler< T, IsEnum<T>::Value >::Value >::SetValue(
			m_rDataStore,
			m_DataNode,
			uIndex,
			value);
	}

	Bool SetValue(UInt32 uIndex, Byte const* sValue)
	{
		return m_rDataStore.SetStringToArray(m_DataNode, uIndex, sValue);
	}

private:
	DataStore& m_rDataStore;

	SEOUL_DISABLE_COPY(MutableDataStoreArrayUtil);
}; // class MutableDataStoreArrayUtil

class DataStoreTableUtil
{
public:
	typedef DataStore::TableIterator Iterator;

	DataStoreTableUtil(const DataStore& dataStore, const DataNode& dataNode, HString tableKey)
		: m_DataStore(dataStore)
		, m_DataNode(dataNode)
		, m_Name(tableKey)
	{
	}

	DataStoreTableUtil(const DataStore& dataStore, HString tableKey)
		: m_DataStore(dataStore)
		, m_DataNode(GetTableDataNode(dataStore, tableKey))
		, m_Name(tableKey)
	{
	}

	virtual ~DataStoreTableUtil()
	{
	}

	Iterator Begin() const
	{
		return m_DataStore.TableBegin(m_DataNode);
	}

	Iterator End() const
	{
		return m_DataStore.TableEnd(m_DataNode);
	}

	Iterator begin() const
	{
		return Begin();
	}

	Iterator end() const
	{
		return End();
	}

	HString GetName() const
	{
		return m_Name;
	}

	template <typename T>
	Bool GetValue(HString key, T& rValue) const
	{
		DataNode valueNode;
		if (!m_DataStore.GetValueFromTable(m_DataNode, key, valueNode))
		{
			return false;
		}

		return DataStoreUtilCommon::Handler< T, DataNodeHandler< T, IsEnum<T>::Value >::Value >::GetValue(
			m_DataStore,
			valueNode,
			rValue);
	}

protected:
	const DataStore& m_DataStore;
	const DataNode m_DataNode;
	const HString m_Name;

	static DataNode GetTableDataNode(const DataStore& dataStore, HString tableKey)
	{
		DataNode node;
		(void)dataStore.GetValueFromTable(dataStore.GetRootNode(), tableKey, node);
		return node;
	}

private:
	SEOUL_DISABLE_COPY(DataStoreTableUtil);
}; // class DataStoreTableUtil

class MutableDataStoreTableUtil SEOUL_SEALED : public DataStoreTableUtil
{
public:
	typedef DataStore::TableIterator Iterator;

	MutableDataStoreTableUtil(DataStore& dataStore, const DataNode& dataNode, HString tableKey)
		: DataStoreTableUtil(dataStore, dataNode, tableKey)
		, m_rDataStore(dataStore)
	{
	}

	MutableDataStoreTableUtil(DataStore& dataStore, HString tableKey)
		: DataStoreTableUtil(dataStore, tableKey)
		, m_rDataStore(dataStore)
	{
	}

	template <typename T>
	Bool SetValue(HString key, const T& value)
	{
		return DataStoreUtilCommon::Handler< T, DataNodeHandler< T, IsEnum<T>::Value >::Value >::SetValue(
			m_rDataStore,
			m_DataNode,
			key,
			value);
	}

	template <typename T>
	Bool SetValue(HString key, const std::initializer_list<T>& list)
	{
		if (!m_rDataStore.SetArrayToTable(m_DataNode, key)) { return false; }

		DataNode arr;
		if (!m_rDataStore.GetValueFromTable(m_DataNode, key, arr)) { return false; }

		MutableDataStoreArrayUtil arrUtil(m_rDataStore, arr);

		UInt32 uOut = 0u;
		for (auto const& e : list)
		{
			if (!arrUtil.SetValue(uOut++, e)) { return false; }
		}

		return true;
	}

	Bool SetValue(HString key, Byte const* sValue)
	{
		return m_rDataStore.SetStringToTable(m_DataNode, key, sValue);
	}

private:
	DataStore& m_rDataStore;

	SEOUL_DISABLE_COPY(MutableDataStoreTableUtil);
}; // class MutableDataStoreTableUtil

} // namespace Seoul

#endif // include guard
