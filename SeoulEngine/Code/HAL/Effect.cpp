/**
 * \file Effect.cpp
 * \brief Represents a set of shaders, grouped into passes and techniques, as well
 * as render states that control how geometry is rendered on the GPU.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ContentLoadManager.h"
#include "Effect.h"
#include "EffectContentLoader.h"
#include "EffectPass.h"
#include "SharedPtr.h"
#include "RenderDevice.h"

namespace Seoul
{

Effect::Effect(
	FilePath filePath,
	void* pRawEffectFileData,
	UInt32 zFileSizeInBytes)
	: m_tParametersBySemantic()
	, m_tTechniquesByName()
	, m_hHandle()
	, m_pRawEffectFileData(pRawEffectFileData)
	, m_zFileSizeInBytes(zFileSizeInBytes)
	, m_FilePath(filePath)
{
}

Effect::~Effect()
{
	SEOUL_ASSERT(IsRenderThread());

	SEOUL_ASSERT(!m_hHandle.IsValid());

	InternalFreeFileData();
}

SharedPtr<Effect> Content::Traits<Effect>::GetPlaceholder(FilePath filePath)
{
	return SharedPtr<Effect>();
}

Bool Content::Traits<Effect>::FileChange(FilePath filePath, const EffectContentHandle& pEntry)
{
	// Sanity check that the type is an effect type - Content::Store<> should have
	// done most of the filtering for us, making sure that the target already
	// exists in our store.
	if (FileType::kEffect == filePath.GetType())
	{
		Load(filePath, pEntry);
		return true;
	}

	return false;
}

void Content::Traits<Effect>::Load(FilePath filePath, const EffectContentHandle& pEntry)
{
	Content::LoadManager::Get()->Queue(
		SharedPtr<Content::LoaderBase>(SEOUL_NEW(MemoryBudgets::Content) EffectContentLoader(filePath, pEntry)));
}

Bool Content::Traits<Effect>::PrepareDelete(FilePath filePath, Content::Entry<Effect, KeyType>& entry)
{
	return true;
}

/**
 * If still valid, releases any buffers specified on creation to
 * generate a texture.
 */
void Effect::InternalFreeFileData()
{
	MemoryManager::Deallocate(m_pRawEffectFileData);
	m_pRawEffectFileData = nullptr;
}

} // namespace Seoul
