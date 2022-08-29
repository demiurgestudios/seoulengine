/**
 * \file ReflectionSerialize.h
 * \brief Functions for serializing data from a concrete C++ object
 * into a generic DataStore using Reflection.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef REFLECTION_SERIALIZE_H
#define REFLECTION_SERIALIZE_H

#include "ReflectionUtil.h"

namespace Seoul::Reflection
{

// Utility function, Deserialize the contents of table into an object instance in objectThis.
inline Bool SerializeObjectToArray(
	SerializeContext& rContext,
	DataStore& rDataStore,
	const DataNode& dataNode,
	UInt32 uIndex,
	const WeakAny& objectThis,
	Bool bSkipPostSerialize = false,
	Bool bDisableRootCustomSerializeType = false)
{
	return Type::TrySerializeToArray(rContext, rDataStore, dataNode, uIndex, objectThis, bSkipPostSerialize, bDisableRootCustomSerializeType);
}

inline Bool SerializeObjectToTable(
	SerializeContext& rContext,
	DataStore& rDataStore,
	const DataNode& dataNode,
	HString key,
	const WeakAny& objectThis,
	Bool bSkipPostSerialize = false,
	Bool bDisableRootCustomSerializeType = false)
{
	return Type::TrySerializeToTable(rContext, rDataStore, dataNode, key, objectThis, bSkipPostSerialize, bDisableRootCustomSerializeType);
}

inline Bool SerializeObjectToArray(
	const ContentKey& contentKey,
	DataStore& rDataStore,
	const DataNode& dataNode,
	UInt32 uIndex,
	const WeakAny& objectThis,
	Bool bSkipPostSerialize = false,
	Bool bDisableRootCustomSerializeType = false)
{
	DefaultSerializeContext context(contentKey, rDataStore, DataNode(), objectThis.GetTypeInfo());
	return SerializeObjectToArray(context, rDataStore, dataNode, uIndex, objectThis, bSkipPostSerialize, bDisableRootCustomSerializeType);
}

inline Bool SerializeObjectToTable(
	const ContentKey& contentKey,
	DataStore& rDataStore,
	const DataNode& dataNode,
	HString key,
	const WeakAny& objectThis,
	Bool bSkipPostSerialize = false,
	Bool bDisableRootCustomSerializeType = false)
{
	DefaultSerializeContext context(contentKey, rDataStore, DataNode(), objectThis.GetTypeInfo());
	return SerializeObjectToTable(context, rDataStore, dataNode, key, objectThis, bSkipPostSerialize, bDisableRootCustomSerializeType);
}

// Serializes an arbitrary object into a DataStore.
Bool SerializeToDataStore(
	const WeakAny& inObject,
	DataStore& rOut);

// Serializes an arbitrary object into a string.
Bool SerializeToString(
	const WeakAny& inObject,
	String& rsOut,
	Bool bMultiline = false,
	Int iIndentationLevel = 0,
	Bool bSortTableKeysAlphabetical = false);

} // namespace Seoul::Reflection

#endif // include guard
