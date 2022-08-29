/**
 * \file ScriptContentLoader.cpp
 * \brief Handles loading and uncompressiong cooked (bytecode) Lua
 * script data.
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
#include "ScriptFileBody.h"
#include "ScriptContentLoader.h"
#include "ScriptManager.h"
#include "Path.h"

namespace Seoul::Script
{

/**
 * Handles platform specific processing of script data
 * into the final format expected by LuaJIT.
 *
 * Nop on platforms other than iOS. On iOS, GC64 bytecode
 * must be used on ARM64 devices. Standard bytecode must be used
 * on all other architectures.
 */
static inline Bool InternalParsePlatformSpecificScriptData(
	void*& rp,
	UInt32& ru)
{
	static const UInt32 kuUniversalScriptSignature = 0xA3C882F3;
	static const Int32 kiUniversalVersion = 1;

	// Invalid data, header is 24 bytes.
	if (ru < 24u)
	{
		return false;
	}

	Byte* pIn = (Byte*)rp;

	// Signature field is first.
	UInt32 uSignature = 0u;
	memcpy(&uSignature, pIn + 0, sizeof(uSignature));
#if SEOUL_BIG_ENDIAN
	EndianSwap32(uSignature);
#endif // /#if SEOUL_BIG_ENDIAN

	if (uSignature != kuUniversalScriptSignature)
	{
		return false;
	}

	// Followed by version code.
	Int32 iVersion = 0;
	memcpy(&iVersion, pIn + 4, sizeof(iVersion));
#if SEOUL_BIG_ENDIAN
	EndianSwap32(iVersion);
#endif // /#if SEOUL_BIG_ENDIAN

	if (iVersion != kiUniversalVersion)
	{
		return false;
	}

	// Get offsets.
	UInt32 uOffset = 0u;
	UInt32 uSize = 0u;

	// We want GC64 bytecode under 64-bit, otherwise use the standard.
#if SEOUL_PLATFORM_64
	memcpy(&uOffset, pIn + 8, sizeof(uOffset));
	memcpy(&uSize, pIn + 12, sizeof(uSize));
#else
	memcpy(&uOffset, pIn + 16, sizeof(uOffset));
	memcpy(&uSize, pIn + 20, sizeof(uSize));
#endif

	// Failure if offset or size are invalid.
	if (uOffset < 24u || uOffset + uSize > ru)
	{
		return false;
	}

	// Now allocate a new buffer for the size, copy,
	// then free the original.
	void* pScript = MemoryManager::Allocate(uSize, MemoryBudgets::Scripting);
	memcpy(pScript, pIn + uOffset, uSize);

	// Free the input.
	pIn = nullptr;
	MemoryManager::Deallocate(rp);

	// Assign.
	rp = pScript;
	ru = uSize;
	return true;
}

/** Entry point for synchronous load, special case for WaitOnContent() cases. */
Content::LoadState ContentLoader::SyncLoad(FilePath filePath, const Content::Handle<FileBody>& hEntry)
{
	// Wait for script project to finish loading if it is still loading.
	while (Manager::Get()->GetAppScriptProject().IsLoading())
	{
		Jobs::Manager::Get()->YieldThreadTime();
	}

#if !SEOUL_SHIP
	// Conditionally cook if the cooked file is not up to date with the source file.
	CookManager::Get()->CookIfOutOfDate(filePath);
#endif

	// Read the data into a buffer - if this succeeds, cache the data
	// and switch to a worker thread to perform decompression.
	void* pCompressed = nullptr;
	UInt32 uCompressed = 0u;
	if (!FileManager::Get()->ReadAll(
		filePath,
		pCompressed,
		uCompressed,
		kLZ4MinimumAlignment,
		MemoryBudgets::Scripting))
	{
		return Content::LoadState::kError;
	}

	// Deobfuscate the data.
	DeObfuscate((Byte*)pCompressed, uCompressed, filePath);

	// Decompress the data.
	void* pUncompressedFileData = nullptr;
	UInt32 zUncompressedFileDataSizeInBytes = 0u;
	if (!LZ4Decompress(
		pCompressed,
		uCompressed,
		pUncompressedFileData,
		zUncompressedFileDataSizeInBytes,
		MemoryBudgets::Scripting))
	{
		// If failed, return with an error.
		MemoryManager::Deallocate(pCompressed);
		return Content::LoadState::kError;
	}

	// Done with compressed data.
	MemoryManager::Deallocate(pCompressed);
	pCompressed = nullptr;

	// Apply platform specific processing to the script data. On 64bit
	// platforms, this extracts the GC64 or standard bytecode chunk as appropriate.
	InternalParsePlatformSpecificScriptData(pUncompressedFileData, zUncompressedFileDataSizeInBytes);

	// Otherwise, initialize the script object.
	SharedPtr<FileBody> pScript(SEOUL_NEW(MemoryBudgets::Scripting) FileBody(pUncompressedFileData, zUncompressedFileDataSizeInBytes));
	SEOUL_ASSERT(nullptr == pUncompressedFileData);
	SharedPtr< Content::Entry<FileBody, FilePath> > pEntry(hEntry.GetContentEntry());
	if (!pEntry.IsValid())
	{
		return Content::LoadState::kError;
	}

	pEntry->AtomicReplace(pScript);
	pScript.Reset();
	return Content::LoadState::kLoaded;
}

ContentLoader::ContentLoader(
	FilePath filePath,
	const Content::Handle<FileBody>& hEntry)
	: Content::LoaderBase(filePath)
	, m_hEntry(hEntry)
	, m_pScript(nullptr)
	, m_pCompressedFileData(nullptr)
	, m_zCompressedFileDataSizeInBytes(0u)
{
	m_hEntry.GetContentEntry()->IncrementLoaderCount();
}

ContentLoader::~ContentLoader()
{
	// Block until this Script::ContentLoader is in a non-loading state.
	WaitUntilContentIsNotLoading();

	// If a failure happened after we loaded but before the script
	// could take ownership of the data, free it.
	InternalFreeCompressedData();

	// Release the content populate entry if it is still valid.
	InternalReleaseEntry();
}

Content::LoadState ContentLoader::InternalExecuteContentLoadOp()
{
	// Must be on the file IO thread to load the script byte code.
	if (Content::LoadState::kLoadingOnFileIOThread == GetContentLoadState())
	{
		// If we're the only reference to the content, "cancel" the load.
		if (m_hEntry.IsUnique())
		{
			m_hEntry.GetContentEntry()->CancelLoad();
			InternalReleaseEntry();
			return Content::LoadState::kLoaded;
		}

		// Wait for script project to finish loading if it is still loading.
		if (Manager::Get()->GetAppScriptProject().IsLoading())
		{
			return Content::LoadState::kLoadingOnFileIOThread;
		}

		// Cache the filePath.
		FilePath const filePath(GetFilePath());

#if !SEOUL_SHIP
		// Conditionally cook if the cooked file is not up to date with the source file.
		(void)CookManager::Get()->CookIfOutOfDate(filePath);
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

		// Apply platform specific processing to the script data. On iOS,
		// this extras the GC64 or standard bytecode chunk as appropriate.
		InternalParsePlatformSpecificScriptData(pUncompressedFileData, zUncompressedFileDataSizeInBytes);

		// Otherwise, initialize the script object.
		m_pScript.Reset(SEOUL_NEW(MemoryBudgets::Scripting) FileBody(pUncompressedFileData, zUncompressedFileDataSizeInBytes));
		SEOUL_ASSERT(nullptr == pUncompressedFileData);
		SharedPtr< Content::Entry<FileBody, FilePath> > pEntry(m_hEntry.GetContentEntry());
		if (!pEntry.IsValid())
		{
			return Content::LoadState::kError;
		}

		pEntry->AtomicReplace(m_pScript);
		m_pScript.Reset();
		InternalReleaseEntry();
		return Content::LoadState::kLoaded;
	}

	return Content::LoadState::kError;
}

/**
 * Frees loaded compressed data if non-null.
 */
void ContentLoader::InternalFreeCompressedData()
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
void ContentLoader::InternalReleaseEntry()
{
	if (m_hEntry.IsInternalPtrValid())
	{
		// NOTE: We need to release our reference before decrementing the loader count.
		// This is safe, because a Content::Entry's Content::Store always maintains 1 reference,
		// and does not release it until the content is done loading.
		Content::Handle<FileBody>::EntryType* pEntry = m_hEntry.GetContentEntry().GetPtr();
		m_hEntry.Reset();
		pEntry->DecrementLoaderCount();
	}
}

} // namespace Seoul::Script
