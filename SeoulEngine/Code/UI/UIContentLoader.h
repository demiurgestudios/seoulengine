/**
 * \file UIContentLoader.h
 * \brief Specialization of Content::LoaderBase for loading Falcon FCN files.
 *
 * UI::ContentLoader loads cooked, compressed Falcon FCN files and generates
 * a template scene graph for later instantiation, typically into a UI::Movie
 * instance.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef UI_CONTENT_LOADER_H
#define UI_CONTENT_LOADER_H

#include "ContentLoaderBase.h"
#include "ContentKey.h"
#include "UIData.h"

namespace Seoul::UI
{

// Forward declarations
class FCNFileData;

class ContentLoader SEOUL_SEALED : public Content::LoaderBase
{
public:
	ContentLoader(FilePath filePath, const Content::Handle<FCNFileData>& hEntry);

protected:
	virtual Content::LoadState InternalExecuteContentLoadOp() SEOUL_OVERRIDE;

	virtual ~ContentLoader();
	SEOUL_REFERENCE_COUNTED_SUBCLASS(ContentLoader);

private:
	void InternalReleaseEntry();

	Content::Handle<FCNFileData> m_hFCNFileEntry;
	void* m_pTotalFileData;
	UInt32 m_zTotalDataSizeInBytes;

	SEOUL_DISABLE_COPY(ContentLoader);
}; // class UI::ContentLoader

} // namespace Seoul::UI

#endif // include guard
