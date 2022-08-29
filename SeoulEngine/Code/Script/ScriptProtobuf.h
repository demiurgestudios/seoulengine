/**
 * \file ScriptProtobuf.h
 * \brief Encapsulates cooked Google Protocol Buffer data. Wrapper around
 * a byte buffer used to bind the data into script.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCRIPT_PROTOBUF_H
#define SCRIPT_PROTOBUF_H

#include "ContentKey.h"
#include "ContentTraits.h"
#include "SharedPtr.h"
#include "Prereqs.h"

namespace Seoul
{

namespace Script
{

class Protobuf SEOUL_SEALED
{
public:
	Protobuf(void*& rpPbData, UInt32 zPbDataSizeInBytes);
	~Protobuf();

	/**
	 * @return The raw binary data of this ScriptProtobuf.
	 */
	void const* GetDataPtr() const
	{
		return m_pData;
	}

	/**
	 * @return The size of the binary data of this Script::Protobuf in bytes.
	 */
	UInt32 GetDataSizeInBytes() const
	{
		return m_zDataSizeInBytes;
	}

private:
	void* m_pData;
	UInt32 m_zDataSizeInBytes;

	SEOUL_REFERENCE_COUNTED(Protobuf);

	SEOUL_DISABLE_COPY(Protobuf);
}; // class Script::Protobuf

} // namespace Script

namespace Content
{

template <>
struct Traits<Script::Protobuf>
{
	typedef FilePath KeyType;
	static const Bool CanSyncLoad = false;

	static SharedPtr<Script::Protobuf> GetPlaceholder(const KeyType& key);
	static Bool FileChange(const KeyType& key, const Handle<Script::Protobuf>& hEntry);
	static void Load(const KeyType& key, const Handle<Script::Protobuf>& hEntry);
	static Bool PrepareDelete(const KeyType& key, Entry<Script::Protobuf, KeyType>& entry);
	static void SyncLoad(FilePath filePath, const Handle<Script::Protobuf>& hEntry) {}
	static UInt32 GetMemoryUsage(const SharedPtr<Script::Protobuf>& p) { return 0u; }
}; // struct Content::Traits<ScriptProtobuf>

} // namespace Content

} // namespace Seoul

#endif // include guard
