/**
 * \file ContentKey.h
 * \brief Generic representation of content. Implements a very
 * common pattern of FilePath + HString data.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef CONTENT_KEY_H
#define CONTENT_KEY_H

#include "DataStore.h"
#include "FilePath.h"
#include "HashFunctions.h"
#include "Prereqs.h"

namespace Seoul
{

/**
 * Generic key for representing any loadable content.
 */
class ContentKey SEOUL_SEALED
{
public:
	ContentKey()
		: m_FilePath()
		, m_Data()
	{
	}

	ContentKey(FilePath filePath, HString data = HString())
		: m_FilePath(filePath)
		, m_Data(data)
	{
	}

	ContentKey(const ContentKey& b)
		: m_FilePath(b.m_FilePath)
		, m_Data(b.m_Data)
	{
	}

	ContentKey& operator=(const ContentKey& b)
	{
		m_FilePath = b.m_FilePath;
		m_Data = b.m_Data;

		return *this;
	}

	Bool operator==(const ContentKey& b) const
	{
		return (
			m_FilePath == b.m_FilePath &&
			m_Data == b.m_Data);
	}

	Bool operator!=(const ContentKey& b) const
	{
		return (
			m_FilePath != b.m_FilePath ||
			m_Data != b.m_Data);
	}

	FilePath GetFilePath() const
	{
		return m_FilePath;
	}

	void SetFilePath(FilePath filePath)
	{
		m_FilePath = filePath;
	}

	/**
	 * ContentKeys has an HString "data" entry
	 * that can be used to constrain the key in addition to
	 * the FilePath (i.e. sound event name)
	 */
	HString GetData() const
	{
		return m_Data;
	}

	/**
	 * Sets the data attribute for this ContentKey.
	 */
	void SetData(HString data)
	{
		m_Data = data;
	}

	/**
	 * Calculates a hash value for this ContentKey, so that 
	 * this ContentKey can be used as a key in a HashTable or other
	 * key-value structure that uses hash values.
	 */
	UInt32 GetHash() const
	{
		UInt32 hash = 0u;
		IncrementalHash(hash, Seoul::GetHash(m_FilePath));
		IncrementalHash(hash, Seoul::GetHash(m_Data));
		return hash;
	}

	/**
	 * Sets this ContentKey such that GetFilePath().IsValid() is false,
	 * and GetData().IsEmpty() is true.
	 */
	void Reset()
	{
		m_FilePath.Reset();
		m_Data = HString();
	}

	// Construct this content key from an entry in a DataStore.
	Bool SetFromDataStore(const DataStore& dataStore, const DataNode& value);

	// Commit this content key to an entry in a DataStore.
	Bool SetToDataStoreArray(DataStore& rDataStore, const DataNode& array, UInt32 uIndex) const;
	Bool SetToDataStoreTable(DataStore& rDataStore, const DataNode& table, HString key) const;

	String ToString() const;

private:
	FilePath m_FilePath;
	HString m_Data;
}; // class ContentKey

/**
 * Helper function to allow ContentKeys to be used as
 * keys in key-value data structures that use hash values on
 * the key.
 */
inline UInt32 GetHash(const ContentKey& key)
{
	return key.GetHash();
}

// Declaration of FilePathToContentKey
template <typename T>
inline T FilePathToContentKey(FilePath filePath);

// Declaration of ContentKeyToFilePath
template <typename T>
inline FilePath ContentKeyToFilePath(const T& key);

/**
 * Specialization of DefaultHashTableKeyTraits<>, allows
 * ContentKey to be used as a key in a HashTable<>.
 */
template<>
struct DefaultHashTableKeyTraits<ContentKey>
{
	inline static Float GetLoadFactor()
	{
		return 0.75f;
	}

	inline static ContentKey GetNullKey()
	{
		return ContentKey();
	}

	static const Bool kCheckHashBeforeEquals = false;
};

/**
 * Specializaiton of FilePathToContentKey for the ContentKey type.
 */
template <>
inline ContentKey FilePathToContentKey(FilePath filePath)
{
	return ContentKey(filePath, HString());
}

/**
 * Specializaiton of ContentKeyToFilePath for the ContentKey type.
 */
template <>
inline FilePath ContentKeyToFilePath<ContentKey>(const ContentKey& key)
{
	return key.GetFilePath();
}

} // namespace Seoul

#endif // include guard
