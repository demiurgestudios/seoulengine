/**
 * \file ScriptFileBody.cpp
 * \brief Encapsulates cooked script data. Wrapper around a byte buffer of
 * uncompressed Lua bytecode data.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ContentLoadManager.h"
#include "ScriptFileBody.h"
#include "ScriptContentLoader.h"

namespace Seoul
{

namespace Script
{

FileBody::FileBody(void*& rpScriptData, UInt32 zScriptDataSizeInBytes)
	: m_pData(rpScriptData)
	, m_zDataSizeInBytes(zScriptDataSizeInBytes)
{
	rpScriptData = nullptr;
}

FileBody::~FileBody()
{
	void* pData = m_pData;
	m_pData = nullptr;
	MemoryManager::Deallocate(pData);
}

} // namespace Script

SharedPtr<Script::FileBody> Content::Traits<Script::FileBody>::GetPlaceholder(const KeyType& key)
{
	return SharedPtr<Script::FileBody>();
}

Bool Content::Traits<Script::FileBody>::FileChange(const KeyType& key, const Content::Handle<Script::FileBody>& pEntry)
{
	// Only react to FileChange events if the key is a Lua script file.
	if (FileType::kScript == key.GetType())
	{
		Load(key, pEntry);
		return true;
	}

	return false;
}

void Content::Traits<Script::FileBody>::Load(const KeyType& key, const Content::Handle<Script::FileBody>& hEntry)
{
	// Only load if the key a Lua script file.
	if (FileType::kScript == key.GetType())
	{
		Content::LoadManager::Get()->Queue(
			SharedPtr<Content::LoaderBase>(SEOUL_NEW(MemoryBudgets::Content) Script::ContentLoader(key, hEntry)));
	}
}

Bool Content::Traits<Script::FileBody>::PrepareDelete(
	const KeyType& key,
	Content::Entry<Script::FileBody, KeyType>& entry)
{
	return true;
}

void Content::Traits<Script::FileBody>::SyncLoad(FilePath filePath, const Content::Handle<Script::FileBody>& hEntry)
{
	(void)Script::ContentLoader::SyncLoad(filePath, hEntry);
}

} // namespace Seoul
