/**
 * \file ScriptFileBody.h
 * \brief Encapsulates cooked script data. Wrapper around a byte buffer of
 * uncompressed Lua bytecode data.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCRIPT_FILE_BODY_H
#define SCRIPT_FILE_BODY_H

#include "ContentKey.h"
#include "ContentTraits.h"
#include "Prereqs.h"
#include "SharedPtr.h"

namespace Seoul
{

namespace Script
{

class FileBody SEOUL_SEALED
{
public:
	FileBody(void*& rpScriptData, UInt32 zScriptDataSizeInBytes);
	~FileBody();

	/**
	 * @return The raw binary data of this Script.
	 */
	void const* GetDataPtr() const
	{
		return m_pData;
	}

	/**
	 * @return The size of the binary data of this Script in bytes.
	 */
	UInt32 GetDataSizeInBytes() const
	{
		return m_zDataSizeInBytes;
	}

private:
	void* m_pData;
	UInt32 m_zDataSizeInBytes;

	SEOUL_REFERENCE_COUNTED(FileBody);

	SEOUL_DISABLE_COPY(FileBody);
}; // class FileBody

} // namespace Script

namespace Content
{

template <>
struct Traits<Script::FileBody>
{
	typedef FilePath KeyType;
	static const Bool CanSyncLoad = true;

	static SharedPtr<Script::FileBody> GetPlaceholder(const KeyType& key);
	static Bool FileChange(const KeyType& key, const Handle<Script::FileBody>& hEntry);
	static void Load(const KeyType& key, const Handle<Script::FileBody>& hEntry);
	static Bool PrepareDelete(const KeyType& key, Entry<Script::FileBody, KeyType>& entry);
	static void SyncLoad(FilePath filePath, const Handle<Script::FileBody>& pEntry);
	static UInt32 GetMemoryUsage(const SharedPtr<Script::FileBody>& p) { return 0u; }
}; // struct Content::Traits<Script::FileBody>

} // namespace Content

namespace Script
{

inline static void DeObfuscate(
	Byte* pData,
	UInt32 zDataSizeInBytes,
	FilePath filePath)
{
	// Get the base filename.
	String const s(Path::GetFileNameWithoutExtension(filePath.GetRelativeFilenameWithoutExtension().ToString()));

	UInt32 uXorKey = 0xB29F8D49;
	for (UInt32 i = 0u; i < s.GetSize(); ++i)
	{
		uXorKey = uXorKey * 33 + tolower(s[i]);
	}

	for (UInt32 i = 0u; i < zDataSizeInBytes; ++i)
	{
		// Mix in the file offset
		pData[i] ^= (Byte)((uXorKey >> ((i % 4) << 3)) + (i / 4) * 101);
	}
}

} // namespace Script

} // namespace Seoul

#endif // include guard
