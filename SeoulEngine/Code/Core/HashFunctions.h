/**
 * \file HashFunctions.h
 * \brief Provides hash functions for common data types.
 *
 * Seoul's Hash table implementation lets users specify the types
 * of keys and values to use in any instance. It looks here for
 * hash functions specific to the key types being used.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef HASH_FUNCTIONS_H
#define HASH_FUNCTIONS_H

#include "SharedPtr.h"
#include "Pair.h"
#include "SeoulAssert.h"
#include "SeoulHString.h"
#include "SeoulString.h"

#include <ctype.h>
#include <limits.h>

namespace Seoul
{

/**
 * Calculate the hash value of the provided integer.
 *
 * This method uses Bob Jenkins' 32-bit integer hash function.
 * \sa http://burtleburtle.net/bob/c/lookup3.c
 *
 * @param[in]	key		The key to hash
 * @return 	hash code for provided key
 */
inline UInt32 GetHash(UInt32 key)
{
	UInt32 hash = key;
	hash = (hash + 0x7ed55d16) + (hash << 12);
	hash = (hash ^ 0xc761c23c) ^ (hash >> 19);
	hash = (hash + 0x165667b1) + (hash << 5);
	hash = (hash + 0xd3a2646c) ^ (hash << 9);
	hash = (hash + 0xfd7046c5) + (hash << 3);
	hash = (hash ^ 0xb55a4f09) ^ (hash >> 16);

	return hash;
}

/**
 * Calculate the hash value of the provided integer.
 *
 * This method uses Bob Jenkins' 32-bit integer hash function.
 * \sa http://burtleburtle.net/bob/c/lookup3.c
 *
 * @param[in]	key		The key to hash
 * @return 	hash code for provided key
 */
inline UInt32 GetHash(Int32 key)
{
	return GetHash((UInt32)key);
}

inline UInt32 GetHash(HString key)
{
	return key.GetHash();
}

/**
 * Calculate the hash value of the provided nullptr-terminated C-string
 *
 * This method uses Bob Jenkins' "One-at-a-Time" hash function.
 * \sa http://www.burtleburtle.net/bob/hash/doobs.html
 *
 * @param[in]	key		The key to hash
 * @return 	hash code for provided key
 */
inline UInt32 GetHash(const Byte* key, UInt32 zStringLengthInBytes)
{
	UInt32 hash = 0;
	for (UInt32 i = 0u; i < zStringLengthInBytes; ++i)
	{
		hash += key[i];
		hash += (hash << 10);
		hash ^= (hash >> 6);
	}
	hash += (hash << 3);
	hash ^= (hash >> 11);
	hash += (hash << 15);
	return hash;
}

inline UInt32 GetHash(const Byte* key)
{
	return GetHash(key, StrLen(key));
}

inline UInt32 GetHash(const String& s)
{
	return GetHash(s.CStr(), s.GetSize());
}

/**
 * Calculate the hash value of the provided nullptr-terminated C-string
 *
 * This method uses Bob Jenkins' "One-at-a-Time" hash function.
 * \sa http://www.burtleburtle.net/bob/hash/doobs.html
 *
 * @param[in]	key		The key to hash
 * @return 	hash code for provided key
 */
inline UInt64 GetHash64(const Byte* key, UInt32 zStringLengthInBytes)
{
	UInt64 hash = 0;
	for (UInt32 i = 0u; i < zStringLengthInBytes; ++i)
	{
		hash += key[i];
		hash += (hash << 10);
		hash ^= (hash >> 6);
	}
	hash += (hash << 3);
	hash ^= (hash >> 11);
	hash += (hash << 15);
	return hash;
}

inline UInt64 GetHash64(const Byte* key)
{
	return GetHash64(key, StrLen(key));
}

inline UInt64 GetHash64(const String& s)
{
	return GetHash64(s.CStr(), s.GetSize());
}

/**
 * Calculate the hash value of the provided nullptr-terminated C-string.
 * Converts the characters to lowercase before calculating the hash
 * so the hash is case insensitive.
 *
 * This method uses Bob Jenkins' "One-at-a-Time" hash function.
 * \sa http://www.burtleburtle.net/bob/hash/doobs.html
 *
 * @param[in]	key		The key to hash
 * @return 	hash code for provided key
 */
inline UInt32 GetCaseInsensitiveHash(const Byte* key, UInt32 zStringLengthInBytes)
{
	UInt32 hash = 0;
	for (UInt32 i = 0u; i < zStringLengthInBytes; ++i)
	{
		hash += tolower(key[i]);
		hash += (hash << 10);
		hash ^= (hash >> 6);
	}
	hash += (hash << 3);
	hash ^= (hash >> 11);
	hash += (hash << 15);
	return hash;
}

inline UInt32 GetCaseInsensitiveHash(const Byte* key)
{
	return GetCaseInsensitiveHash(key, StrLen(key));
}

/**
 * Calculate the 32-bit hash value of the provided signed 64-bit integer.
 *
 * This method uses Thomas Wang's 64-bit algorithm.
 * \sa http://www.concentric.net/~Ttwang/tech/inthash.htm
 *
 * @param[in]	key		The key to hash
 * @return 	hash code for provided key
 */
inline UInt32 GetHash(const Int64 &key)
{
	long long hash = key;
	hash = ~hash + (hash << 18);
	hash = hash  ^ (hash >> 31);
	hash = hash  * 21;
	hash = hash  ^ (hash >> 11);
	hash = hash  + (hash << 6);
	hash = hash  ^ (hash >> 22);
	return (UInt32) hash;
}

/**
 * Calculate the 32-bit hash value of the provided unsigned 64-bit integer.
 *
 * This method uses Thomas Wang's 64-bit algorithm.
 * \sa http://www.concentric.net/~Ttwang/tech/inthash.htm
 *
 * @param[in]	key		The key to hash
 * @return 	hash code for provided key
 */
inline UInt32 GetHash(UInt64 key)
{
	UInt64 hash = key;
	hash = ~hash + (hash << 18);
	hash = hash  ^ (hash >> 31);
	hash = hash  * 21;
	hash = hash  ^ (hash >> 11);
	hash = hash  + (hash << 6);
	hash = hash  ^ (hash >> 22);
	return (UInt32)hash;
}

/**
 * Calculate the hash of a pointer by treating it as a size_t.
 *
 * @param[in]	key		The key to hash
 * @return 	hash code for provided key
 */
inline UInt32 GetHash(const void *key)
{
	return GetHash((UInt64)reinterpret_cast<size_t>(key));
}

template <typename T>
UInt32 GetHash(const SharedPtr<T>& key)
{
	return GetHash(key.GetPtr());
}

inline UInt32 GetHash(Float32 f)
{
	union
	{
		Float32 fIn;
		UInt32 uOut;
	};
	fIn = f;
	return GetHash(uOut);
}

/**
 * Incrementally builds a final UInt32 hash value by mixing in hash values of
 * subparts.
 *
 * @param[inout] arHash The previous or initial hash and the resulting hash after mixin.
 * @param[in] aHashValueMixIn A subpart hash value to be combined with the current running hash.
 */
inline void IncrementalHash(UInt32& rHash, UInt32 HashValueMixIn)
{
	rHash ^= HashValueMixIn
		+ 0x9e3779b9
		+ (rHash << 6)
		+ (rHash >> 2);
}

/**
 * Mixes two hash values to produce a final hash value
 *
 * @param[in] uHash1 First hash value to mix
 * @param[in] uHash2 Second hash value to mix
 *
 * @return The mixed hash value
 */
inline UInt32 MixHashes(UInt32 uHash1, UInt32 uHash2)
{
	UInt32 uResult = uHash1;
	IncrementalHash(uResult, uHash2);
	return uResult;
}

/**
 * Mixes three hash values to produce a final hash value
 *
 * @param[in] uHash1 First hash value to mix
 * @param[in] uHash2 Second hash value to mix
 * @param[in] uHash3 Third hash value to mix
 *
 * @return The mixed hash value
 */
inline UInt32 MixHashes(UInt32 uHash1, UInt32 uHash2, UInt32 uHash3)
{
	UInt32 uResult = uHash1;
	IncrementalHash(uResult, uHash2);
	IncrementalHash(uResult, uHash3);
	return uResult;
}

/**
 * Mixes four hash values to produce a final hash value
 *
 * @param[in] uHash1 First hash value to mix
 * @param[in] uHash2 Second hash value to mix
 * @param[in] uHash3 Third hash value to mix
 * @param[in] uHash4 Fourth hash value to mix
 *
 * @return The mixed hash value
 */
inline UInt32 MixHashes(UInt32 uHash1, UInt32 uHash2, UInt32 uHash3, UInt32 uHash4)
{
	UInt32 uResult = uHash1;
	IncrementalHash(uResult, uHash2);
	IncrementalHash(uResult, uHash3);
	IncrementalHash(uResult, uHash4);
	return uResult;
}

template <typename T1, typename T2>
UInt32 GetHash(const Pair<T1, T2>& key)
{
	UInt32 uHash = 0u;
	IncrementalHash(uHash, GetHash(key.First));
	IncrementalHash(uHash, GetHash(key.Second));
	return uHash;
}

} // namespace Seoul

#endif // include guard
