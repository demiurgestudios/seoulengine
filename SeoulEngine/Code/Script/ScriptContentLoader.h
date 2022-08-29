/**
 * \file ScriptContentLoader.h
 * \brief Handles loading and uncompressiong cooked (bytecode) Lua
 * script data.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCRIPT_CONTENT_LOADER_H
#define SCRIPT_CONTENT_LOADER_H

#include "Atomic32.h"
#include "ContentLoaderBase.h"
#include "ContentKey.h"
#include "ContentHandle.h"
#include "ScriptFileBody.h"

namespace Seoul::Script
{

class ContentLoader SEOUL_SEALED : public Content::LoaderBase
{
public:
	// Entry point for synchronous load, special case for WaitOnContent() cases.
	static Content::LoadState SyncLoad(FilePath filePath, const Content::Handle<FileBody>& hEntry);

	ContentLoader(FilePath filePath, const Content::Handle<FileBody>& hEntry);
	virtual ~ContentLoader();

private:
	virtual Content::LoadState InternalExecuteContentLoadOp() SEOUL_OVERRIDE;
	void InternalFreeCompressedData();
	void InternalReleaseEntry();

	Content::Handle<FileBody> m_hEntry;
	SharedPtr<FileBody> m_pScript;
	void* m_pCompressedFileData;
	UInt32 m_zCompressedFileDataSizeInBytes;

	SEOUL_DISABLE_COPY(ContentLoader);
}; // class Script::ContentLoader

} // namespace Seoul::Script

#endif // include guard
