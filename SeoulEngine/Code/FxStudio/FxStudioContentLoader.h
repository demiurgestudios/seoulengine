/**
 * \file FxStudioContentLoader.h
 * \brief Specialization of Content::LoaderBase for loading FxStudio banks
 * (.FXB files).
 *
 * \warning Don't instantiate this class to load a FxStudio banks
 * directly unless you know what you are doing. Loading FxStudio banks
 * this way prevents the bank from being managed by ContentLoadManager.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FX_STUDIO_CONTENT_LOADER_H
#define FX_STUDIO_CONTENT_LOADER_H

#include "ContentLoaderBase.h"
#include "ContentHandle.h"
#include "FxStudioBankFile.h"

#if SEOUL_WITH_FX_STUDIO

namespace Seoul::FxStudio
{

// Forward declarations
class BankFile;

/**
 * Specialization of Content::LoaderBase for loading FxStudio
 * bank files.
 */
class ContentLoader SEOUL_SEALED : public Content::LoaderBase
{
public:
	ContentLoader(
		FilePath filePath,
		const Content::Handle<BankFile>& pEntry);
	virtual ~ContentLoader();

private:
	virtual Content::LoadState InternalExecuteContentLoadOp() SEOUL_OVERRIDE;
	void InternalFreeCompressedData();
	void InternalReleaseEntry();

	Content::Handle<BankFile> m_hEntry;
	void* m_pCompressedFileData;
	UInt32 m_zCompressedFileDataSizeInBytes;
	BankFileData m_Data;

	SEOUL_DISABLE_COPY(ContentLoader);
}; // class FxStudio::ContentLoader

} // namespace Seoul::FxStudio

#endif // /#if SEOUL_WITH_FX_STUDIO

#endif // include guard
