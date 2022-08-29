/**
 * \file ContentTraits.h
 * \brief Content::Traits<> is a generic template class with types and
 * static functions which are used to operate on loadable content
 * of type T. Various classes in the content system (ContentStore, Content::Entry)
 * use Content::Traits to perform operations that differ by type. You must
 * create a full specialization of Content::Traits<> for types that will
 * be managed by the content system.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef CONTENT_TRAITS_H
#define CONTENT_TRAITS_H

#include "FilePath.h"
#include "Prereqs.h"

namespace Seoul
{

namespace Content
{

// Forward declarations
template <typename T, typename KEY_TYPE> class Entry;
template <typename T> class Handle;

/**
 * Declaration of a class that must be specialized for all content that will
 * be managed by a Content::Store<>
 */
template <typename T> struct Traits;

// Content::Traits must fulfill the following generic contract.
/*
template <>
struct Traits<Type>
{
	Specializations must define this to the correct key type.
	typedef <key-type> KeyType;

	Specializations should define this to return a SharedPtr<> that is either
	nullptr, or points to a placeholder that can be used until a piece of content
	can be loaded.
	inline static SharedPtr<T> GetPlaceholder(KeyType key);

	Specializations should define this function so that, if file changes
	to key can be handled, handle those changes and then return true.
	inline static Bool FileChange(KeyType key, const Content::Handle<T>& pEntry);

	Specialization should define this so that it starts a content loading
	operation for pEntry.
	inline static void Load(KeyType key, const Content::Handle<T>& pEntry);

	Specializations should define this so that it returns true if entry
	can be destroyed, false otherwise.
	inline static Bool PrepareDelete(KeyType key, Content::Entry<T>& entry);

	Specializations should define this to return an estimate of the content's
	current memory usage. Return 0 to indicate "unsupported".
	inline static UInt32 GetMemoryUsage(const SharedPtr<T>& p);
};
*/

} // namespace Content

// Declaration of FilePathToContentKey
template <typename T>
inline T FilePathToContentKey(FilePath filePath);

// Declaration of Content::KeyToFilePath
template <typename T>
inline FilePath ContentKeyToFilePath(const T& key);

/**
 * Specialization of FilePathToContentKey for FilePath, just return the input value.
 */
template <>
inline FilePath FilePathToContentKey(FilePath filePath)
{
	return filePath;
}

/**
 * Specialization of Content::KeyToFilePath for FilePath, just return the input value.
 */
template <>
inline FilePath ContentKeyToFilePath(const FilePath& filePath)
{
	return filePath;
}

} // namespace Seoul

#endif // include guard
