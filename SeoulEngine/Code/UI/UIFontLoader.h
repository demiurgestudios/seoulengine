/**
 * \file UIFontLoader.h
 * \brief Specialization of Content::LoaderBase for loading Falcon font data.
 *
 * UI::FontLoader reads cooked, compressed TTF font data and prepares
 * it for consumption by the Falcon project.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef UI_FONT_LOADER_H
#define UI_FONT_LOADER_H

#include "ContentLoaderBase.h"
#include "ContentKey.h"
#include "UIData.h"

namespace Seoul::UI
{

class FontLoader SEOUL_SEALED : public Content::LoaderBase
{
public:
	FontLoader(FilePath filePath, const Content::Handle<Falcon::CookedTrueTypeFontData>& hTrueTypeFontDataEntry);

protected:
	virtual Content::LoadState InternalExecuteContentLoadOp() SEOUL_OVERRIDE;

	virtual ~FontLoader();
	SEOUL_REFERENCE_COUNTED_SUBCLASS(FontLoader);

private:
	void InternalReleaseEntry();

	Content::Handle<Falcon::CookedTrueTypeFontData> m_hTrueTypeFontDataEntry;
	void* m_pTotalFileData;
	UInt32 m_zTotalDataSizeInBytes;

	SEOUL_DISABLE_COPY(FontLoader);
}; // class UI::FontLoader

} // namespace Seoul::UI

#endif // include guard
