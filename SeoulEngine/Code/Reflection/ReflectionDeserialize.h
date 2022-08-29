/**
 * \file ReflectionDeserialize.h
 * \brief Functions for deserializing data from a generic DataStore into
 * a concrete C++ object using Reflection.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef REFLECTION_DESERIALIZE_H
#define REFLECTION_DESERIALIZE_H

#include "HashSet.h"
#include "Lexer.h"
#include "ReflectionUtil.h"

namespace Seoul::Reflection
{

// Utility function, Deserialize the contents of table into an object instance in objectThis.
inline Bool DeserializeObject(
	SerializeContext& rContext,
	const DataStore& dataStore,
	const DataNode& table,
	const WeakAny& objectThis,
	Bool bSkipPostSerialize = false,
	Bool bDisableRootCustomDeserializeType = false)
{
	return Type::TryDeserialize(rContext, dataStore, table, objectThis, bSkipPostSerialize, bDisableRootCustomDeserializeType);
}

inline Bool DeserializeObject(
	const ContentKey& contentKey,
	const DataStore& dataStore,
	const DataNode& table,
	const WeakAny& objectThis,
	Bool bSkipPostSerialize = false,
	Bool bDisableRootCustomDeserializeType = false)
{
	DefaultSerializeContext context(contentKey, dataStore, table, objectThis.GetTypeInfo());
	return DeserializeObject(context, dataStore, table, objectThis, bSkipPostSerialize, bDisableRootCustomDeserializeType);
}

// Deserializes a string into an arbitary object
Bool DeserializeFromString(
	Byte const* s,
	UInt32 zSizeInBytes,
	const WeakAny& outObject,
	UInt32 uFlags = 0u,
	FilePath filePath = FilePath());

// Deserializes a string into an arbitary object
static inline Bool DeserializeFromString(
	const String& s,
	const WeakAny& outObject,
	UInt32 uFlags = 0u,
	FilePath filePath = FilePath())
{
	return DeserializeFromString(s.CStr(), s.GetSize(), outObject, uFlags, filePath);
}

} // namespace Seoul::Reflection

#endif // include guard
