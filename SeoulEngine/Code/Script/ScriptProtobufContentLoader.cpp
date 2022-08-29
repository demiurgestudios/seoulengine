/**
 * \file ScriptProtobufContentLoader.cpp
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

#include "Compress.h"
#include "CookManager.h"
#include "Engine.h"
#include "FileManager.h"
#include "Path.h"
#include "ScriptFileBody.h"
#include "ScriptProtobufContentLoader.h"

namespace Seoul::Script
{

ProtobufContentLoader::ProtobufContentLoader(
	FilePath filePath,
	const Content::Handle<Protobuf>& hEntry)
	: Content::LoaderBase(filePath)
	, m_hEntry(hEntry)
	, m_pScriptProtobuf(nullptr)
	, m_pCompressedFileData(nullptr)
	, m_zCompressedFileDataSizeInBytes(0u)
{
	m_hEntry.GetContentEntry()->IncrementLoaderCount();
}

ProtobufContentLoader::~ProtobufContentLoader()
{
	// Block until this Content::Loaded is in a non-loading state.
	WaitUntilContentIsNotLoading();

	// If a failure happened after we loaded but before the Script::Protobuf object
	// could take ownership of the data, free it.
	InternalFreeCompressedData();

	// Release the content populate entry if it is still valid.
	InternalReleaseEntry();
}

Content::LoadState ProtobufContentLoader::InternalExecuteContentLoadOp()
{
	// Must be on the file IO thread to load the compressed proto data.
	if (Content::LoadState::kLoadingOnFileIOThread == GetContentLoadState())
	{
		// If we're the only reference to the content, "cancel" the load.
		if (m_hEntry.IsUnique())
		{
			m_hEntry.GetContentEntry()->CancelLoad();
			InternalReleaseEntry();
			return Content::LoadState::kLoaded;
		}

		// Cache the filePath.
		FilePath const filePath(GetFilePath());

#if !SEOUL_SHIP
		// Conditionally cook if the cooked file is not up to date with the source file.
		CookManager::Get()->CookIfOutOfDate(filePath);
#endif

		// Read the data into a buffer - if this succeeds, cache the data
		// and switch to a worker thread to perform decompression.
		if (FileManager::Get()->ReadAll(
			filePath,
			m_pCompressedFileData,
			m_zCompressedFileDataSizeInBytes,
			kLZ4MinimumAlignment,
			MemoryBudgets::Scripting))
		{
			// Finish loading off the file IO thread.
			return Content::LoadState::kLoadingOnWorkerThread;
		}
	}
	// We get here to decompress the Lua bytecode.
	else if (Content::LoadState::kLoadingOnWorkerThread == GetContentLoadState())
	{
		// Sanity check, should have been insured by the previous step.
		SEOUL_ASSERT(nullptr != m_pCompressedFileData);

		// Deobfuscate the data.
		DeObfuscate((Byte*)m_pCompressedFileData, m_zCompressedFileDataSizeInBytes, GetFilePath());

		// Decompress the data.
		void* pUncompressedFileData = nullptr;
		UInt32 zUncompressedFileDataSizeInBytes = 0u;

		// If failed, return with an error.
		if (!LZ4Decompress(
			m_pCompressedFileData,
			m_zCompressedFileDataSizeInBytes,
			pUncompressedFileData,
			zUncompressedFileDataSizeInBytes,
			MemoryBudgets::Scripting))
		{
			return Content::LoadState::kError;
		}

		// Done with compressed data, free it.
		InternalFreeCompressedData();

		// Otherwise, initialize the Script::Protobuf object.
		m_pScriptProtobuf.Reset(SEOUL_NEW(MemoryBudgets::Scripting) Protobuf(pUncompressedFileData, zUncompressedFileDataSizeInBytes));
		SEOUL_ASSERT(nullptr == pUncompressedFileData);
		SharedPtr< Content::Entry<Protobuf, FilePath> > pEntry(m_hEntry.GetContentEntry());
		if (!pEntry.IsValid())
		{
			return Content::LoadState::kError;
		}

		pEntry->AtomicReplace(m_pScriptProtobuf);
		m_pScriptProtobuf.Reset();
		InternalReleaseEntry();
		return Content::LoadState::kLoaded;
	}

	return Content::LoadState::kError;
}

/**
 * Frees loaded compressed data if non-nullptr.
 */
void ProtobufContentLoader::InternalFreeCompressedData()
{
	if (nullptr != m_pCompressedFileData)
	{
		MemoryManager::Deallocate(m_pCompressedFileData);
		m_pCompressedFileData = nullptr;
	}
	m_zCompressedFileDataSizeInBytes = 0u;
}

/**
 * Release the loader's reference on its content entry - doing this as
 * soon as loading completes allows anything waiting for the load to react
 * as soon as possible.
 */
void ProtobufContentLoader::InternalReleaseEntry()
{
	if (m_hEntry.IsInternalPtrValid())
	{
		// NOTE: We need to release our reference before decrementing the loader count.
		// This is safe, because a Content::Entry's Content::Store always maintains 1 reference,
		// and does not release it until the content is done loading.
		Content::Handle<Protobuf>::EntryType* pEntry = m_hEntry.GetContentEntry().GetPtr();
		m_hEntry.Reset();
		pEntry->DecrementLoaderCount();
	}
}

} // namespace Seoul::Script
