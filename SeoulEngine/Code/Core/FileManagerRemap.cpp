/**
 * \file FileManagerRemap.cpp
 * \brief FileManager internal utility. Used to remap FilePaths to other
 * FilePaths.
 *
 * The intended use case of the FileManagerRemap is A/B testing. It can
 * be (re)-configured with mapping tables that re-route file requests.
 *
 * For example, a request for Data/Config/Test.json will resolve to
 * Data/Config/TestA.json or Data/Config/TestB.json depending on the
 * definition of remap tables.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "DataStore.h"
#include "FileManagerRemap.h"

namespace Seoul
{

// From/to keys of remap entries.
static const HString kFrom("From");
static const HString kTo("To");

/** Utility, generates the has for a list of remap file paths. */
UInt32 FileManagerRemap::ComputeHash(FilePath const* iBegin, FilePath const* iEnd)
{
	UInt32 uHash = 0u;
	for (auto i = iBegin; iEnd != i; ++i)
	{
		IncrementalHash(uHash, i->GetHash());
	}
	return uHash;
}

/** Utility. Merges remap entries defined in DataStore into rtRemap */
Bool FileManagerRemap::Merge(const DataStore& dataStore, const DataNode& dataNode, RemapTable& rtRemap)
{
	UInt32 uArrayCount = 0u;
	if (!dataStore.GetArrayCount(dataNode, uArrayCount))
	{
		return false;
	}

	for (UInt32 i = 0u; i < uArrayCount; ++i)
	{
		DataNode entry;
		SEOUL_VERIFY(dataStore.GetValueFromArray(dataNode, i, entry));

		DataNode fromNode;
		FilePath fromPath;
		if (!dataStore.GetValueFromTable(entry, kFrom, fromNode) ||
			!dataStore.AsFilePath(fromNode, fromPath))
		{
			return false;
		}

		DataNode toNode;
		FilePath toPath;
		if (!dataStore.GetValueFromTable(entry, kTo, toNode) ||
			!dataStore.AsFilePath(toNode, toPath))
		{
			return false;
		}

		rtRemap.Overwrite(fromPath, toPath);
	}

	return true;
}

FileManagerRemap::FileManagerRemap()
	: m_tRemap()
	, m_uHash(0u)
	, m_Mutex()
{
}

FileManagerRemap::~FileManagerRemap()
{
}

/** Configure or reconfigure this remap table. */
void FileManagerRemap::Configure(const RemapTable& t, UInt32 uHash)
{
	// Copy first, outside the lock.
	auto tCopy = t;

	// Handle multiple remap types (e.g. textures).
	// We don't do an overwrite and we let the
	// insert fail, in case explicit remaps
	// were specified per mip level
	// (which would be usunusal but is still
	// supported).
	for (auto const& e : t)
	{
		auto const eFromType = e.First.GetType();
		if (IsTextureFileType(eFromType))
		{
			auto from = e.First;
			auto to = e.Second;
			for (auto iType = (Int32)FileType::FIRST_TEXTURE_TYPE; iType <= (Int32)FileType::LAST_TEXTURE_TYPE; ++iType)
			{
				from.SetType((FileType)iType);

				// Only change the to type if it's also a texture.
				if (IsTextureFileType(to.GetType()))
				{
					to.SetType((FileType)iType);
				}

				// Insert updated remapping. Allowed to fail.
				(void)tCopy.Insert(from, to);
			}
		}
	}

	// Now swap inside the lock.
	{
		Lock lock(m_Mutex);
		m_tRemap.Swap(tCopy);
		m_uHash = uHash;
	}
}

} // namespace Seoul
