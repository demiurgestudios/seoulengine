/**
 * \file HTTPHeaderTable.h
 * \brief Data structure to unpack headers returned with HTTP
 * responses into key-value query pairs.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef	HTTP_HEADER_TABLE_H
#define HTTP_HEADER_TABLE_H

#include "HashTable.h"
#include "Prereqs.h"
#include "SeoulHString.h"
namespace Seoul { class String; }

namespace Seoul::HTTP
{

class HeaderTable SEOUL_SEALED
{
public:
	struct ValueEntry
	{
		ValueEntry()
			: m_sValue(nullptr)
			, m_zValueSizeInBytes(0)
		{
		}

		Byte const* m_sValue;
		UInt32 m_zValueSizeInBytes;
	};

	typedef Int32 DifferenceType;
	typedef HString KeyType;
	typedef UInt32 SizeType;
	typedef ValueEntry ValueType;
	typedef HashTable<KeyType, ValueType, MemoryBudgets::Network> TableType;

	HeaderTable();
	~HeaderTable();

	/** Manually add a key-value header pair without parsing. */
	void AddKeyValue(
		Byte const* sKey,
		UInt32 zKeyLengthInBytes,
		Byte const* sValue,
		UInt32 zValueLengthInBytes);

	/** Replace the contents of this header table with an exact copy of b. */
	void CloneFrom(const HeaderTable& b);

	/** @return The inner table of key-value pairs, read-only. */
	const TableType& GetKeyValues() const
	{
		return m_tHeaders;
	}

	/** Populate the value associated with key or return false if the header is not defined. */
	Bool GetValue(KeyType key, Byte const*& rsValue, UInt32& rzValueSizeInBytes) const;
	Bool GetValue(KeyType key, Byte const*& rsValue) const
	{
		UInt32 zUnusedValueSizeInBytes = 0u;
		return GetValue(key, rsValue, zUnusedValueSizeInBytes);
	}
	Bool GetValue(KeyType key, String& rsValue) const;

	/** Parse a header into parts and add it to the table. */
	Bool ParseAndAddHeader(Byte const* sHeader, UInt32 zHeaderSizeInBytes);

	/** Parse a header into parts and add it to the table. */
	Bool ParseAndAddHeader(Byte const* sHeader)
	{
		return ParseAndAddHeader(sHeader, StrLen(sHeader));
	}

	/** Replace the contents of this header table with b and vice versa. */
	void Swap(HeaderTable& r)
	{
		m_tHeaders.Swap(r.m_tHeaders);
	}

private:
	TableType m_tHeaders;

	void AddKeyValue(
		HString key,
		const ValueType& inValue);

	SEOUL_DISABLE_COPY(HeaderTable);
}; // class HTTP::HeaderTable

} // namespace Seoul::HTTP

#endif // include guard
