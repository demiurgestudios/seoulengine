/**
 * \file SeoulUUID.h
 * \brief Structure and utility functions for generating and storing
 * a UUID (unique universal identifier).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SEOUL_UUID_H
#define SEOUL_UUID_H

#include "FixedArray.h"
#include "HashFunctions.h"
#include "Prereqs.h"
#include "SeoulString.h"

namespace Seoul
{

class UUID SEOUL_SEALED
{
public:
	typedef FixedArray<UInt8, 16> Bytes;

	// Populate a new UUID structure from a String. Always succeeds - invalid input
	// results in a 0 UUID.
	static UUID FromString(const String& s);

	// Generate a new UUID using v4 of RFC-4122: http://www.ietf.org/rfc/rfc4122.txt
	//
	// WARNING: Can be expensive/block, since UUID generation uses a cryptographically
	// secure random source.
	static void GenerateV4(UUID& rOut);
	static inline UUID GenerateV4()
	{
		UUID ret;
		GenerateV4(ret);
		return ret;
	}

	// Equality.
	Bool operator==(const UUID& b) const
	{
		return (0 == memcmp(m_auBytes.Data(), b.m_auBytes.Data(), b.m_auBytes.GetSizeInBytes()));
	}
	Bool operator!=(const UUID& b) const
	{
		return !(*this == b);
	}

	/** @return The internal bytes of this UUID. */
	const Bytes& GetBytes() const
	{
		return m_auBytes;
	}

	// Produce a 36 character standardized UUID string.
	String ToString() const;

	/** @return An all zero UUID. */
	static inline UUID Zero()
	{
		UUID ret;
		ret.m_auBytes.Fill((Byte)0);
		return ret;
	}

private:
	// UUID consists of 16 bytes.
	Bytes m_auBytes;
}; // class UUID
template <> struct CanMemCpy<UUID> { static const Bool Value = true; };
template <> struct CanZeroInit<UUID> { static const Bool Value = true; };

SEOUL_STATIC_ASSERT_MESSAGE(sizeof(UUID) == 16, "UUID is not the expected size of 16 bytes.");

static inline UInt32 GetHash(const UUID& uuid)
{
	return Seoul::GetHash((Byte const*)uuid.GetBytes().Data(), UUID::Bytes::StaticSize);
}

template <>
struct DefaultHashTableKeyTraits<UUID>
{
	inline static Float GetLoadFactor()
	{
		return 0.75f;
	}

	inline static UUID GetNullKey()
	{
		return UUID::Zero();
	}

	static const Bool kCheckHashBeforeEquals = false;
};

} // namespace Seoul

#endif // include guard
