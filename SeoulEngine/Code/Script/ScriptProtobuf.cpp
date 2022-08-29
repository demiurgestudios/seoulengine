/**
 * \file ScriptProtobuf.cpp
 * \brief Encapsulates cooked Google Protocol Buffer data. Wrapper around
 * a byte buffer used to bind the data into script.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ContentLoadManager.h"
#include "ScriptProtobuf.h"
#include "ScriptProtobufContentLoader.h"

namespace Seoul
{

namespace Script
{

Protobuf::Protobuf(void*& rpPbData, UInt32 zPbDataSizeInBytes)
	: m_pData(rpPbData)
	, m_zDataSizeInBytes(zPbDataSizeInBytes)
{
	rpPbData = nullptr;
}

Protobuf::~Protobuf()
{
	void* pData = m_pData;
	m_pData = nullptr;
	MemoryManager::Deallocate(pData);
}

} // namespace Script

SharedPtr<Script::Protobuf> Content::Traits<Script::Protobuf>::GetPlaceholder(const KeyType& key)
{
	return SharedPtr<Script::Protobuf>();
}

Bool Content::Traits<Script::Protobuf>::FileChange(const KeyType& key, const Content::Handle<Script::Protobuf>& pEntry)
{
	// Only react to FileChange events if the key is a Proto type file.
	if (FileType::kProtobuf == key.GetType())
	{
		Load(key, pEntry);
		return true;
	}

	return false;
}

void Content::Traits<Script::Protobuf>::Load(const KeyType& key, const Content::Handle<Script::Protobuf>& pEntry)
{
	// Only load if the key a Proto type file.
	if (FileType::kProtobuf == key.GetType())
	{
		Content::LoadManager::Get()->Queue(
			SharedPtr<Content::LoaderBase>(SEOUL_NEW(MemoryBudgets::Content) Script::ProtobufContentLoader(key, pEntry)));
	}
}

Bool Content::Traits<Script::Protobuf>::PrepareDelete(
	const KeyType& key,
	Content::Entry<Script::Protobuf, KeyType>& entry)
{
	return true;
}

} // namespace Seoul
