/**
 * \file SeoulUUID.cpp
 * \brief Structure and utility functions for generating and storing
 * a UUID (unique universal identifier).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Lexer.h"
#include "SecureRandom.h"
#include "SeoulUUID.h"
#include "StringUtil.h"

namespace Seoul
{

/**
 * Populate a new UUID structure from a String. Always succeeds - invalid input
 * results in a 0 UUID.
 */
UUID UUID::FromString(const String& s)
{
	UInt32 const uSize = s.GetSize();

	// Wrong size, early out.
	if (36u != uSize)
	{
		return UUID::Zero();
	}

	UUID ret;
	UInt32 uOut = 0u;
	for (UInt32 i = 1u; i < uSize; )
	{
		// Done, invalid UUID.
		if (uOut >= ret.m_auBytes.GetSize())
		{
			return UUID::Zero();
		}

		// Cache next two characters.
		Byte const ch0 = s[i-1];
		Byte const ch1 = s[i-0];

		// Conversion of first char.
		UInt32 u0;
		if (!HexCharToUInt32(ch0, u0))
		{
			// Invalid UUID.
			if ('-' != ch0)
			{
				return UUID::Zero();
			}
			else
			{
				++i;
				continue;
			}
		}

		// Conversio of second char.
		UInt32 u1;
		if (!HexCharToUInt32(ch1, u1))
		{
			// Invalid UUID.
			if ('-' != ch1)
			{
				return UUID::Zero();
			}
			else
			{
				++i;
				continue;
			}
		}

		ret.m_auBytes[uOut++] = (UInt8)(((u0 & 0xF) << 4) | ((u1 & 0xF) << 0));
		i += 2u;
	}

	// Invalid.
	if (uOut != ret.m_auBytes.GetSize())
	{
		return UUID::Zero();
	}

	return ret;
}

/** Generate a new UUID using v4 of RFC-4122: http://www.ietf.org/rfc/rfc4122.txt */
void UUID::GenerateV4(UUID& rOut)
{
	// Generate the bytes.
	SecureRandom::GetBytes(rOut.m_auBytes.Data(), rOut.m_auBytes.GetSizeInBytes());

	// Adjust bytes according to section 4.4 of RFC 4122
	rOut.m_auBytes[6u] = 0x40 | (rOut.m_auBytes[6u] & 0xF);
	rOut.m_auBytes[8u] = 0x80 | (rOut.m_auBytes[8u] & 0x3F);
}

/** @return The state of this UUID as a canonical string representation. */
String UUID::ToString() const
{
	// Convert to hexadecimal.
	String const sHex(HexDump((void const*)m_auBytes.Data(), m_auBytes.GetSizeInBytes(), false));

	// Add separators: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
	String const sReturn(String::Printf("%s-%s-%s-%s-%s",
		sHex.Substring(0, 8).CStr(), // 0-7
		sHex.Substring(8, 4).CStr(), // 8-11
		sHex.Substring(12, 4).CStr(), // 12-15
		sHex.Substring(16, 4).CStr(), // 16-19
		sHex.Substring(20, 12).CStr())); // 20-32

	return sReturn;
}

} // namespace Seoul
