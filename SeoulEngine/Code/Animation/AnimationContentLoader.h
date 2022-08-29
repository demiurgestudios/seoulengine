/**
 * \file AnimationContentLoader.h
 * \brief Specialization of Content::LoaderBase for loading animation
 * data and animation network data.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ANIMATION_CONTENT_LOADER_H
#define ANIMATION_CONTENT_LOADER_H

#include "AnimationNetworkDefinition.h"
#include "ContentLoaderBase.h"

#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)

namespace Seoul::Animation
{

class NetworkContentLoader SEOUL_SEALED : public Content::LoaderBase
{
public:
	NetworkContentLoader(FilePath filePath, const AnimationNetworkContentHandle& hEntry);
	~NetworkContentLoader();

protected:
	virtual Content::LoadState InternalExecuteContentLoadOp() SEOUL_OVERRIDE;

private:
	void InternalReleaseEntry();

	AnimationNetworkContentHandle m_hEntry;
}; // class NetworkContentLoader

} // namespace Seoul::Animation

#endif // /#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)

#endif // include guard
