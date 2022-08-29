/**
 * \file EffectContentLoader.h
 * \brief Specialization of Content::LoaderBase for loading effects.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef EFFECT_CONTENT_LOADER_H
#define EFFECT_CONTENT_LOADER_H

#include "ContentLoaderBase.h"
#include "ContentKey.h"
#include "Effect.h"

namespace Seoul
{

/**
 * Specialization of Content::LoaderBase for loading effects.
 */
class EffectContentLoader SEOUL_SEALED : public Content::LoaderBase
{
public:
	EffectContentLoader(FilePath filePath, const EffectContentHandle& pEntry);
	virtual ~EffectContentLoader();

protected:
	virtual Content::LoadState InternalExecuteContentLoadOp() SEOUL_OVERRIDE;

private:
	void InternalFreeEffectData();
	void InternalReleaseEntry();

	EffectContentHandle m_hEntry;
	SharedPtr<Effect> m_pEffect;
	void* m_pRawEffectFileData;
	UInt32 m_zFileSizeInBytes;
	Bool m_bNetworkPrefetched;
}; // class EffectContentLoader

} // namespace Seoul

#endif // include guard
