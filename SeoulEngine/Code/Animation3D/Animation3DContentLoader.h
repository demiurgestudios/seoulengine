/**
 * \file Animation3DContentLoader.h
 * \brief Specialization of Content::LoaderBase for loading animation
 * clip and skeleton data.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ANIMATION3D_CONTENT_LOADER_H
#define ANIMATION3D_CONTENT_LOADER_H

#include "Animation3DDataDefinition.h"
#include "ContentLoaderBase.h"

#if SEOUL_WITH_ANIMATION_3D

namespace Seoul::Animation3D
{

class ContentLoader SEOUL_SEALED : public Content::LoaderBase
{
public:
	ContentLoader(FilePath filePath, const Animation3DDataContentHandle& hEntry);
	~ContentLoader();

protected:
	virtual Content::LoadState InternalExecuteContentLoadOp() SEOUL_OVERRIDE;

private:
	void InternalFreeData();
	void InternalReleaseEntry();

	Animation3DDataContentHandle m_hEntry;
	void* m_pRawData;
	UInt32 m_uDataSizeInBytes;
	Bool m_bNetworkPrefetched;
}; // class ContentLoader

} // namespace Seoul::Animation3D

#endif // /#if SEOUL_WITH_ANIMATION_3D

#endif // include guard
