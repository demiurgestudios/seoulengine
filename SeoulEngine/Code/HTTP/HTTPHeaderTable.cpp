/**
 * \file HTTPHeaderTable.cpp
 * \brief Data structure to unpack headers returned with HTTP
 * responses into key-value query pairs.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "HTTPHeaderTable.h"
#include "Lexer.h"
#include "SeoulHString.h"
#include "SeoulString.h"

namespace Seoul::HTTP
{

HeaderTable::HeaderTable()
	: m_tHeaders()
{
}

HeaderTable::~HeaderTable()
{
	{
		auto const iBegin = m_tHeaders.Begin();
		auto const iEnd = m_tHeaders.End();
		for (auto i = iBegin; iEnd != i; ++i)
		{
			MemoryManager::Deallocate((void*)(i->Second.m_sValue));
			i->Second.m_sValue = nullptr;
			i->Second.m_zValueSizeInBytes = 0;
		}
	}
	m_tHeaders.Clear();
}

void HeaderTable::AddKeyValue(
	Byte const* sKey,
	UInt32 zKeyLengthInBytes,
	Byte const* sValue,
	UInt32 zValueLengthInBytes)
{
	// Initialize the HString key.
	String unknownCaseKey(sKey, zKeyLengthInBytes);
	HString const key(unknownCaseKey.ToLowerASCII());

	// Initialize a ValueType value.
	ValueType value;
	value.m_sValue = (Byte const*)MemoryManager::Allocate(
		zValueLengthInBytes + 1,
		MemoryBudgets::Network);
	if (0 != zValueLengthInBytes)
	{
		memcpy((void*)value.m_sValue, (void const*)sValue, zValueLengthInBytes);
	}
	((Byte*)value.m_sValue)[zValueLengthInBytes] = '\0';
	value.m_zValueSizeInBytes = zValueLengthInBytes;

	// Insert the key.
	AddKeyValue(key, value);
}

/** Replace the contents of this header table with an exact copy of b. */
void HeaderTable::CloneFrom(const HeaderTable& b)
{
	HeaderTable t;
	auto const iBegin = b.m_tHeaders.Begin();
	auto const iEnd = b.m_tHeaders.End();
	for (auto i = iBegin; iEnd != i; ++i)
	{
		t.AddKeyValue(i->First.CStr(), i->First.GetSizeInBytes(), i->Second.m_sValue, i->Second.m_zValueSizeInBytes);
	}

	Swap(t);
}

Bool HeaderTable::GetValue(
	KeyType key,
	Byte const*& rsValue,
	UInt32& rzValueSizeInBytes) const
{
	ValueType value;
	if (m_tHeaders.GetValue(key, value))
	{
		rsValue = value.m_sValue;
		rzValueSizeInBytes = value.m_zValueSizeInBytes;
		return true;
	}

	return false;
}

Bool HeaderTable::GetValue(
	KeyType key,
	String& rsValue) const
{
	Byte const* sValue = nullptr;
	UInt32 zValueSizeInBytes = 0u;
	if (GetValue(key, sValue, zValueSizeInBytes))
	{
		rsValue.Assign(sValue, zValueSizeInBytes);
		return true;
	}

	return false;
}

/** Parse a header into parts and add it to the table. */
Bool HeaderTable::ParseAndAddHeader(
	Byte const* sHeader,
	UInt32 zHeaderSizeInBytes)
{
	LexerContext context;
	context.SetStream(sHeader, zHeaderSizeInBytes);

	// Find key and value parts.
	Byte const* sKeyFirst = nullptr;
	Byte const* sKeyLast = nullptr;
	Byte const* sValueFirst = nullptr;
	Byte const* sValueLast = nullptr;
	Bool bInValue = false;

	while (context.IsStreamValid())
	{
		UniChar const ch = context.GetCurrent();

		// Looking for value start/end.
		if (bInValue)
		{
			// If we have a non-whitespace character, update sValueFirst
			// and sValueLast.
			if (!IsSpace(ch))
			{
				if (nullptr == sValueFirst)
				{
					sValueFirst = context.GetStream();
				}
				sValueLast = context.GetStream();
			}
		}
		// Looking for key start/end.
		else
		{
			// Key end, check for error.
			if (':' == ch)
			{
				// No key, not a key-value header, return immediately.
				if (nullptr == sKeyFirst)
				{
					return false;
				}

				// Now parsing the value part.
				bInValue = true;
			}
			else
			{
				// If we have a non-whitespace character, update sKeyFirst
				// and sKeyLast.
				if (!IsSpace(ch))
				{
					if (nullptr == sKeyFirst)
					{
						sKeyFirst = context.GetStream();
					}
					sKeyLast = context.GetStream();
				}
			}
		}

		context.Advance();
	}

	// No key, or we never encountered the ':' (bInValue is false),
	// this is not a key-value header, return immediately.
	if (nullptr == sKeyFirst ||
		!bInValue)
	{
		return false;
	}

	AddKeyValue(
		sKeyFirst,
		(UInt32)(sKeyLast - sKeyFirst) + 1,
		(nullptr == sValueFirst ? "" : sValueFirst),
		(nullptr == sValueFirst ? 0u : ((UInt32)(sValueLast - sValueFirst) + 1)));
	return true;
}

void HeaderTable::AddKeyValue(
	HString key,
	const ValueType& inValue)
{
	ValueType newValue(inValue);

	// RFC 2616 Section 4: http://www.w3.org/Protocols/rfc2616/rfc2616-sec4.html#sec4.2
	// states that duplicate keys can appear and the values can be concatenated with a ','
	// to generate a single key-value pair.
	{
		ValueType existingValue;
		if (m_tHeaders.GetValue(key, existingValue))
		{
			// Concatenate the new value onto the existing value.
			Byte* sData = (Byte*)existingValue.m_sValue;
			sData = (Byte*)MemoryManager::Reallocate(
				(void*)sData,
				// +1 for the comma, +1 for the null terminator.
				(size_t)(existingValue.m_zValueSizeInBytes + newValue.m_zValueSizeInBytes + 2),
				MemoryBudgets::Network);

			// Add a comma.
			UInt32 uOffset = existingValue.m_zValueSizeInBytes;
			sData[uOffset++] = ',';

			// Copy the new data
			memcpy((void*)(sData + uOffset), (void const*)newValue.m_sValue, newValue.m_zValueSizeInBytes);
			uOffset += newValue.m_zValueSizeInBytes;

			// Add the null terminator.
			sData[uOffset] = '\0';

			// Deallocate the newValue buffer.
			MemoryManager::Deallocate((void*)newValue.m_sValue);

			// Update the new value with the new size and pointer.
			newValue.m_sValue = (Byte const*)sData;
			newValue.m_zValueSizeInBytes = uOffset;
		}
	}

	// Insert newValue - always overwrite. Either an entirely
	// new value or a concatenation of an existing value that
	// was already done above.
	SEOUL_VERIFY(m_tHeaders.Overwrite(key, newValue).Second);
}

} // namespace Seoul::HTTP
