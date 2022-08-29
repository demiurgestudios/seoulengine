/**
 * \file ScriptProtobufContentLoader.h
 * \brief Handles loading and uncompressiong cooked (bytecode) Google
 * Protocol Buffer data, typically for later bind into a script
 * virtual machine.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCRIPT_PROTOBUF_CONTENT_LOADER_H
#define SCRIPT_PROTOBUF_CONTENT_LOADER_H

#include "Atomic32.h"
#include "ContentLoaderBase.h"
#include "ContentKey.h"
#include "ContentHandle.h"
#include "ScriptProtobuf.h"

namespace Seoul::Script
{

class ProtobufContentLoader SEOUL_SEALED : public Content::LoaderBase
{
public:
	ProtobufContentLoader(FilePath filePath, const Content::Handle<Protobuf>& hEntry);
	virtual ~ProtobufContentLoader();

private:
	virtual Content::LoadState InternalExecuteContentLoadOp() SEOUL_OVERRIDE;
	void InternalFreeCompressedData();
	void InternalReleaseEntry();

	Content::Handle<Protobuf> m_hEntry;
	SharedPtr<Protobuf> m_pScriptProtobuf;
	void* m_pCompressedFileData;
	UInt32 m_zCompressedFileDataSizeInBytes;

	SEOUL_DISABLE_COPY(ProtobufContentLoader);
}; // class Script::ProtobufContentLoader

} // namespace Seoul::Script

#endif // include guard
