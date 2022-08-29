/**
 * \file TextureContentLoader.cpp
 * \brief Specialization of Content::LoaderBase for loading textures.
 *
 * \warning Don't instantiate this class to load a texture
 * directly unless you know what you are doing. Loading textures
 * this way prevents the texture from being managed by ContentLoadManager.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Compress.h"
#include "CookManager.h"
#include "DDS.h"
#include "FileManager.h"
#include "JobsFunction.h"
#include "RenderDevice.h"
#include "ScopedAction.h"
#include "SeoulETC1.h"
#include "SeoulFile.h"
#include "Texture.h"
#include "TextureContentLoader.h"
#include "TextureFooter.h"
#include "TextureManager.h"

#ifdef _WIN32
#define WIN32
#endif
#define crnd seoul_crnd // Deal with the bad include header design of crn_decomp.h (it's included in the crn library itself, so here and there can result in duplicate definition failures at link time).
#include <crn_decomp.h>

namespace Seoul
{

namespace
{

void* CrnRealloc(void* p, size_t zSize, size_t* pzActualSize, Bool bMovable, void*)
{
	if (nullptr != pzActualSize) { *pzActualSize = zSize; }

	if (0u == zSize)
	{
		MemoryManager::Deallocate(p);
		return nullptr;
	}
	else
	{
		return MemoryManager::ReallocateAligned(p, zSize, CRNLIB_MIN_ALLOC_ALIGNMENT, MemoryBudgets::Cooking);
	}
}

size_t CrnMsize(void* p, void*)
{
	return MemoryManager::GetAllocationSizeInBytes(p);
}

struct CrnHooks
{
	CrnHooks()
	{
		crnd::crnd_set_memory_callbacks(CrnRealloc, CrnMsize, nullptr);
	}
};
static CrnHooks s_CrnHooks;

}

// TODO: Mipped loading in this file can be conditionally
// optimized:
// - on platforms that support MIP LOD clamping and do not
//   support asynchronous texture creation, we can
//   directly load each mip level into the same texture object,
//   instead of creating an entirely new object each time. This
//   would also allow us to immediately deallocate CPU data
//   for each texture level instead of holding onto it until
//   all mips have been loaded.

static Bool PostProcessPossibleCrnData(void*& rp, UInt32& ru)
{
	// Early out if not a cern block.
	using namespace crnd;

	// Early out if not enough room for the texture footer.
	if (ru < sizeof(TextureFooter))
	{
		return true;
	}

	// Not a crn file, return immediately.
	crn_texture_info info;
	if (!crnd_get_texture_info(rp, ru, &info))
	{
		return true;
	}

	// Validate file data.
	crn_file_info fileInfo;
	if (!crnd_validate_file(rp, ru, &fileInfo))
	{
		return false;
	}

	// TODO: Support .crn data after first - cooker never emits this
	// so we don't worry about it for now (alpha texture is encoded as DDS,
	// so we only check the first header for crn, not any additional headers).
	// TODO: Multiple level support and other cases (cube maps).
	if (info.m_faces != 1 || info.m_levels != 1)
	{
		return false;
	}

	// Convert the fourCC into a pixel format.
	auto const ePixelFormat = DDS::ToPixelFormat(
		crnd_crn_format_to_fourcc(info.m_format));

	// Invalid format, corrupt file.
	if (PixelFormat::kInvalid == ePixelFormat)
	{
		return false;
	}

	// Additional data size is any size not accounted for
	// in the texture footer, crn header, or crn data blob.
	// Sanity check that first.
	auto const uCrnSize = fileInfo.m_actual_data_size;
	if (uCrnSize + sizeof(TextureFooter) > ru)
	{
		return false;
	}

	// Zero if no alpha channel, otherwise the total size of
	// the additional secondary texture in an ETC1 compressed image
	// with an alpha channel.
	auto const uAdditionalDataSize = (ru - (uCrnSize + sizeof(TextureFooter)));

	// Copy the footer for use later.
	TextureFooter footer;
	memcpy(&footer, (Byte*)rp + ru - sizeof(TextureFooter), sizeof(TextureFooter));

	// Allocate a block big enough for the output (DDSHeader + data + uAdditionalDataSize + TextureFooter).
	auto const uDataSize = GetDataSizeForPixelFormat(info.m_width, info.m_height, ePixelFormat);
	auto const uNewSize = (UInt32)(sizeof(DdsHeader) + uDataSize + uAdditionalDataSize + sizeof(TextureFooter));
	void* pNew = MemoryManager::Allocate(uNewSize, MemoryBudgets::Content);

	// Release pNew one way or the other - either we'll swap it with rp or leave it
	// alone in the case of error.
	auto const scope(MakeScopedAction([](){}, [&]()
	{
		MemoryManager::Deallocate(pNew);
		pNew = nullptr;
	}));

	// Begin decompression.
	auto pContext = crnd_unpack_begin(rp, ru - sizeof(TextureFooter));
	if (nullptr == pContext)
	{
		return false;
	}

	// Extract mip level 0.
	void* pDataOut = (Byte*)pNew + sizeof(DdsHeader);
	auto const bReturn = crnd_unpack_level(pContext, &pDataOut, uDataSize, 0, 0);
	(void)crnd_unpack_end(pContext);

	if (!bReturn)
	{
		return false;
	}

	// Now fill in the header and copy through the footer and any additional data.
	DdsHeader header;
	memset(&header, 0, sizeof(header));

	// Get the pitch if possible.
	UInt32 uPitch = 0u;
	(void)GetPitchForPixelFormat(info.m_width, ePixelFormat, uPitch);

	// Populate relevant fields for an DXT image.
	header.m_MagicNumber = kDdsMagicValue;
	header.m_Size = (sizeof(header) - sizeof(header.m_MagicNumber));
	header.m_HeaderFlags = kDdsHeaderFlagsTexture | kDdsHeaderFlagsLinearSize;
	header.m_Height = info.m_height;
	header.m_Width = info.m_width;
	header.m_PitchOrLinearSize = (IsCompressedPixelFormat(ePixelFormat) ? uDataSize : uPitch);
	header.m_Depth = 1;
	header.m_MipMapCount = 1;
	header.m_PixelFormat = DDS::ToDdsPixelFormat(ePixelFormat);
	header.m_SurfaceFlags = kDdsSurfaceFlagsTexture;

	// Fill in.
	memcpy(pNew, &header, sizeof(header)); // Header.
	memcpy((Byte*)pNew + sizeof(DdsHeader) + uDataSize, (Byte*)rp + uCrnSize, uAdditionalDataSize); // Additional data - alpha texture/secondary texture if present.
	memcpy((Byte*)pNew + uNewSize - sizeof(footer), &footer, sizeof(footer)); // Footer

	// Swap and return - scoped action above will handle cleanup of the replaced buffer.
	Swap(pNew, rp);
	ru = uNewSize;
	return true;
}

/** Apply various handling to texture data after it has been decompressed. */
static Bool PostProcessTextureData(void*& rp, UInt32& ru)
{
	// If a crn encoded blob, convert to DDS.
	if (!PostProcessPossibleCrnData(rp, ru))
	{
		return false;
	}

	// Get the pixel format from the stream. If this fails, can't post process.
	PixelFormat eFormat = PixelFormat::kInvalid;
	if (!DDS::ReadPixelFormat(rp, ru, eFormat) || PixelFormat::kInvalid == eFormat)
	{
		return false;
	}

	// Now check for and apply various handling.
	auto& r = *RenderDevice::Get();
	auto const& caps = r.GetCaps();

	// Data is an ETC1 texture that we don't support.
	if (PixelFormat::kETC1_RGB8 == eFormat && !caps.m_bETC1)
	{
		// Software decompress the data.
		void* p = nullptr;
		UInt32 u = 0u;
		if (!ETC1Decompress(
			rp,
			ru,
			p,
			u))
		{
			return false;
		}

		// Replace.
		MemoryManager::Deallocate(rp);
		rp = p;
		ru = u;

		// Recompute pixel format.
		if (!DDS::ReadPixelFormat(rp, ru, eFormat) || PixelFormat::kInvalid == eFormat)
		{
			return false;
		}
	}

	// Data is in BGRA8888 format that we don't support.
	if (PixelFormat::kA8R8G8B8 == eFormat && !caps.m_bBGRA)
	{
		// Swap the RB channels of the data.
		if (!DDS::SwapChannelsRB(rp, ru))
		{
			return false;
		}

		// Recompute pixel format.
		if (!DDS::ReadPixelFormat(rp, ru, eFormat) || PixelFormat::kInvalid == eFormat)
		{
			return false;
		}
	}

	return true;
}

TextureContentLoader::TextureContentLoader(FilePath filePath, const TextureContentHandle& pEntry)
	: Content::LoaderBase(filePath)
	, m_CurrentLevelFilePath(filePath)
	, m_hEntry(pEntry)
	, m_TextureConfig(TextureManager::Get()->GetTextureConfig(filePath))
	, m_uWidth(0u)
	, m_uHeight(0u)
	, m_eFormat(PixelFormat::kInvalid)
	, m_CurrentLevelData()
	, m_Data()
	, m_bNetworkPrefetched(false)
#if !SEOUL_SHIP
	, m_bTriedRecook(false)
#endif
{
	m_hEntry.GetContentEntry()->IncrementLoaderCount();

	// If we're loading mips, we start with the highest mip (smallest)
	// and load towards the desired. This requires incomplete mip
	// chain support under the current rendering backend.
	if (m_TextureConfig.m_bMipped &&
		RenderDevice::Get()->GetCaps().m_bIncompleteMipChain)
	{
		m_CurrentLevelFilePath.SetType(FileType::LAST_TEXTURE_TYPE);
	}

	// Kick off prefetching of the asset (this will be a nop for local files)
	m_bNetworkPrefetched = FileManager::Get()->NetworkPrefetch(m_CurrentLevelFilePath);
}

TextureContentLoader::~TextureContentLoader()
{
	// Block until this Content::LoaderBase is in a non-loading state.
	WaitUntilContentIsNotLoading();

	// Release the tracking entry.
	InternalReleaseEntry();
}

/**
 * Method which handles actual loading of texture data - can
 * perform a variety of ops depending on the platform and type
 * of texture data.
 */
Content::LoadState TextureContentLoader::InternalExecuteContentLoadOp()
{
	auto eResult = InternalExecuteContentLoadOpBody();

	// Start the next step of the loading process if applicable. This
	// only occurs if m_bMipped was true for the original
	// texture file, in which case we're progressively loading
	// each mip level, from mip4 up to the original
	// specified mip.
	if (Content::LoadState::kLoaded == eResult && m_CurrentLevelFilePath != GetFilePath())
	{
		m_CurrentLevelFilePath.SetType((FileType)((Int)m_CurrentLevelFilePath.GetType() - 1));
		eResult = Content::LoadState::kLoadingOnFileIOThread;
		m_bNetworkPrefetched = FileManager::Get()->NetworkPrefetch(m_CurrentLevelFilePath);
#if !SEOUL_SHIP
		m_bTriedRecook = false;
#endif // /#if !SEOUL_SHIP
	}

	// In non-ship builds, trigger a recook of the file, in case a developer has old (locally referenced)
	// images, when a cooking error occurs.
#if !SEOUL_SHIP
	if (Content::LoadState::kError == eResult && !m_bTriedRecook)
	{
		m_bTriedRecook = true;

		auto const filePath(m_CurrentLevelFilePath);

		// A recook is only possible if we can cook the file in the first place.
		if (CookManager::Get()->SupportsCooking(filePath.GetType()))
		{
			// Don't attempt a delete if the source doesn't exist.
			Bool const bCanDelete = FileManager::Get()->ExistsInSource(filePath);

			// If the file doesn't exist or we successfully delete it,
			// flush data and try again.
			if (!FileManager::Get()->Exists(filePath) || (bCanDelete && FileManager::Get()->Delete(filePath)))
			{
				InternalFreeCurrentLevelData();
				return Content::LoadState::kLoadingOnFileIOThread;
			}
		}

		// If we get here, it means we couldn't attempt a recook, so immediately swap in the error texture.
		m_hEntry.GetContentEntry()->AtomicReplace(TextureManager::Get()->GetErrorTexture());
	}
#endif

	return eResult;
}

Content::LoadState TextureContentLoader::InternalExecuteContentLoadOpBody()
{
	// Default quantum by default - certain cases may switch scheduling quantums.
	SetJobQuantum(Min(GetJobQuantum(), Jobs::Quantum::kDefault));

	// Final stage, swap in and complete.
	if (m_pTexture.IsValid())
	{
		// Sanity check - must have been moved to the render thread for this step.
		SEOUL_ASSERT(IsRenderThread());

		// Swap in.
		auto pTexture(m_pTexture);
		m_pTexture.Reset();

		m_hEntry.GetContentEntry()->AtomicReplace(pTexture);
		if (GetFilePath() == m_CurrentLevelFilePath) { InternalReleaseEntry(); }

		// Done with loading body, decrement the loading count.
		return Content::LoadState::kLoaded;
	}

	// Used for immediate creation off the render thread.
	Bool bAsyncCreate = false;

	// First step, load the data.
	if (Content::LoadState::kLoadingOnFileIOThread == GetContentLoadState())
	{
		// If we're the only reference to the content, "cancel" the load.
		if (m_hEntry.IsUnique())
		{
			m_hEntry.GetContentEntry()->CancelLoad();
			if (GetFilePath() == m_CurrentLevelFilePath) { InternalReleaseEntry(); }
			return Content::LoadState::kLoaded;
		}

		// If network file systems are still pending, check if the texture exists. If it does
		// not, wait until network file systems are no longer pending before failing
		if (FileManager::Get()->IsAnyFileSystemStillInitializing())
		{
			if (!FileManager::Get()->Exists(m_CurrentLevelFilePath))
			{
				SetJobQuantum(Jobs::Quantum::kWaitingForDependency);
				return Content::LoadState::kLoadingOnFileIOThread;
			}
		}

		// Only try to read from disk. Let the prefetch finish the download.
		if (FileManager::Get()->IsServicedByNetwork(m_CurrentLevelFilePath))
		{
			if (FileManager::Get()->IsNetworkFileIOEnabled())
			{
				// Kick off a prefetch if we have not yet done so.
				if (!m_bNetworkPrefetched)
				{
					m_bNetworkPrefetched = FileManager::Get()->NetworkPrefetch(m_CurrentLevelFilePath);
				}

				return Content::LoadState::kLoadingOnFileIOThread;
			}
			else // This is a network download, but the network system isn't enabled so it will never complete
			{
				InternalFreeCurrentLevelData();

				// Immediately set m_bTriedRecook to true, don't want a reattempt in this case.
#if !SEOUL_SHIP
				m_bTriedRecook = true;
#endif // /#if !SEOUL_SHIP

				// Swap the placeholder texture into the slot.
				m_hEntry.GetContentEntry()->AtomicReplace(TextureManager::Get()->GetPlaceholderTexture());

				// Done - don't treat this case as an error since it indicates shutdown and we don't
				// want to spuriously flag the shutdown case.
				return Content::LoadState::kLoaded;
			}
		}

#if !SEOUL_SHIP
		// Conditionally cook if the cooked file is not up to date with the source file.
		CookManager::Get()->Cook(m_CurrentLevelFilePath, !m_bTriedRecook);
#endif

		UInt32 const zMaxReadSize = kDefaultMaxReadSize;

		// If reading succeeds, check if we can immediately create
		// the texture object.
		if (FileManager::Get()->ReadAll(
			m_CurrentLevelFilePath,
			m_CurrentLevelData.m_pFileData,
			m_CurrentLevelData.m_uFileSizeInBytes,
			kLZ4MinimumAlignment,
			MemoryBudgets::Content,
			zMaxReadSize))
		{
			// Decompress on a worker thread.
			return Content::LoadState::kLoadingOnWorkerThread;
		}
		// If reading fails, we have an error, so clear state
		// data and return an error code.
		else
		{
			InternalFreeCurrentLevelData();

			// Don't swap in the error texture if we're going to attempt a recook.
#if !SEOUL_SHIP
			if (m_bTriedRecook)
#endif // /#if !SEOUL_SHIP
			{
				// Swap the error texture into the slot.
				m_hEntry.GetContentEntry()->AtomicReplace(TextureManager::Get()->GetErrorTexture());
			}

			// Done with loading body, decrement the loading count.
			return Content::LoadState::kError;
		}
	}
	// Second step, decompress the data.
	else if (Content::LoadState::kLoadingOnWorkerThread == GetContentLoadState())
	{
		void* pUncompressedFileData = nullptr;
		UInt32 zUncompressedFileDataInBytes = 0u;

		if (ZSTDDecompress(
			m_CurrentLevelData.m_pFileData,
			m_CurrentLevelData.m_uFileSizeInBytes,
			pUncompressedFileData,
			zUncompressedFileDataInBytes,
			MemoryBudgets::Content))
		{
			InternalFreeCurrentLevelData();
			Swap(pUncompressedFileData, m_CurrentLevelData.m_pFileData);
			Swap(zUncompressedFileDataInBytes, m_CurrentLevelData.m_uFileSizeInBytes);

			// Handles additional processing, based on the format
			// of m_pRawTextureFileData and any optional capabilities
			// of the current graphics hardware.
			if (!PostProcessTextureData(m_CurrentLevelData.m_pFileData, m_CurrentLevelData.m_uFileSizeInBytes))
			{
				InternalFreeCurrentLevelData();

				// Don't swap in the error texture if we're going to attempt a recook.
#if !SEOUL_SHIP
				if (m_bTriedRecook)
#endif // /#if !SEOUL_SHIP
				{
					// Swap the error texture into the slot.
					m_hEntry.GetContentEntry()->AtomicReplace(TextureManager::Get()->GetErrorTexture());
				}

				// Failure means loading fails.
				return Content::LoadState::kError;
			}
			else
			{
				// Decode the texture data.
				if (!InternalDecodeTexture())
				{
					return Content::LoadState::kError;
				}

				// If async is supported, try it now. Otherwise, switch to the render
				// thread.
				if (RenderDevice::Get()->SupportsAsyncCreateTexture())
				{
					bAsyncCreate = true;

					// Fall-through to attempt creation off the render thread.
				}
				else
				{
					return Content::LoadState::kLoadingOnRenderThread;
				}
			}
		}
		else
		{
			InternalFreeCurrentLevelData();

			// Don't swap in the error texture if we're going to attempt a recook.
#if !SEOUL_SHIP
			if (m_bTriedRecook)
#endif // /#if !SEOUL_SHIP
			{
				// Swap the error texture into the slot.
				m_hEntry.GetContentEntry()->AtomicReplace(TextureManager::Get()->GetErrorTexture());
			}

			// Done with loading body, decrement the loading count.
			return Content::LoadState::kError;
		}
	}

	// Sanity check - we should only fall through to this context if we're creating
	// a texture asynchronously, or we've switched to the render thread.
	SEOUL_ASSERT(bAsyncCreate || IsRenderThread());

	SharedPtr<BaseTexture> pTexture;

	// Get the footer - if it's invalid, don't create the texture.
	if (m_CurrentLevelData.m_uFileSizeInBytes >= sizeof(TextureFooter))
	{
		TextureFooter footer;
		memcpy(&footer, (Byte const*)(m_CurrentLevelData.m_pFileData) + m_CurrentLevelData.m_uFileSizeInBytes - sizeof(footer), sizeof(footer));

		// If the signature and version are valid, create the texture.
		if (footer.m_uSignature == kTextureFooterSignature &&
			footer.m_uVersion == kTextureFooterVersion &&
			footer.m_fTexcoordsScaleU >= 0.0f && footer.m_fTexcoordsScaleU <= 1.0f &&
			footer.m_fTexcoordsScaleV >= 0.0f && footer.m_fTexcoordsScaleV <= 1.0f &&
			footer.m_fVisibleRegionScaleU >= 0.0f && footer.m_fVisibleRegionScaleU <= 1.0f &&
			footer.m_fVisibleRegionScaleV >= 0.0f && footer.m_fVisibleRegionScaleV <= 1.0f &&
			footer.m_fVisibleRegionOffsetU >= 0.0f && footer.m_fVisibleRegionOffsetU <= 1.0f &&
			footer.m_fVisibleRegionOffsetV >= 0.0f && footer.m_fVisibleRegionOffsetV <= 1.0f &&
			footer.m_fOcclusionRegionScaleU >= 0.0f && footer.m_fOcclusionRegionScaleU <= 1.0f &&
			footer.m_fOcclusionRegionScaleV >= 0.0f && footer.m_fOcclusionRegionScaleV <= 1.0f &&
			footer.m_fOcclusionRegionOffsetU >= 0.0f && footer.m_fOcclusionRegionOffsetU <= 1.0f &&
			footer.m_fOcclusionRegionOffsetV >= 0.0f && footer.m_fOcclusionRegionOffsetV <= 1.0f)
		{
			// Commit the current level to the running total data.
			m_Data = TextureData::PushFrontLevel(m_Data, m_CurrentLevelData.ReleaseAsTextureLevelData());

			// Now do the create.
			pTexture.Reset();
			InternalCreateTextureUtil(bAsyncCreate, &pTexture);

			// If creation failed and this was an async creation attempt, retry
			// with a render thread attempt.
			if (!pTexture.IsValid() && bAsyncCreate)
			{
				// Update.
				bAsyncCreate = false;

				// Synchronous execute, block and wait on render thread.
				//
				// IMPORTANT: If you decide to make this async (for some reason),
				// note that we're passing a stack pointer here. That needs to be
				// changed before this can be anything other than a synchronous
				// call.
				pTexture.Reset();
				Jobs::AwaitFunction(
					GetRenderThreadId(),
					SEOUL_BIND_DELEGATE(&TextureContentLoader::InternalCreateTextureUtil, this),
					bAsyncCreate,
					&pTexture);
			}

			// If we have a valid texture, assign the texcoords scale and the visible region.
			if (pTexture.IsValid())
			{
				pTexture->SetTexcoordsScale(Vector2D(footer.m_fTexcoordsScaleU, footer.m_fTexcoordsScaleV));
				pTexture->SetOcclusionRegionScaleAndOffset(Vector4D(
					footer.m_fOcclusionRegionScaleU,
					footer.m_fOcclusionRegionScaleV,
					footer.m_fOcclusionRegionOffsetU,
					footer.m_fOcclusionRegionOffsetV));
				pTexture->SetVisibleRegionScaleAndOffset(Vector4D(
					footer.m_fVisibleRegionScaleU,
					footer.m_fVisibleRegionScaleV,
					footer.m_fVisibleRegionOffsetU,
					footer.m_fVisibleRegionOffsetV));
			}
		}
	}

	// If we have a texture object, loading succeeded.
	if (pTexture.IsValid())
	{
		// With an async create, wait for the texture instance to enter
		// the ready state when not on the render thread.
		if (bAsyncCreate && !IsRenderThread() && !IsMainThread())
		{
			// Swap and switch to the render thread.
			m_pTexture.Swap(pTexture);
			return Content::LoadState::kLoadingOnRenderThread;
		}

		m_hEntry.GetContentEntry()->AtomicReplace(pTexture);
		if (GetFilePath() == m_CurrentLevelFilePath) { InternalReleaseEntry(); }
		// Done with loading body, decrement the loading count.
		return Content::LoadState::kLoaded;
	}
	// If loading failed, place the error texture in the slot for this texture.
	else
	{
		// Don't swap in the error texture if we're going to attempt a recook.
#if !SEOUL_SHIP
		if (m_bTriedRecook)
#endif // /#if !SEOUL_SHIP
		{
			m_hEntry.GetContentEntry()->AtomicReplace(TextureManager::Get()->GetErrorTexture());
		}

		// Done with loading body, decrement the loading count.
		return Content::LoadState::kError;
	}
}

/**
 * Utility shared and used by the creation body
 * to instantiate a texture object from a context
 * and configuration.
 */
void TextureContentLoader::InternalCreateTextureUtil(
	Bool bAsyncCreate,
	SharedPtr<BaseTexture>* ppTexture) const
{
	// Initially reset - always non-null, it's
	// a pointer only to workaround the semantics
	// of Jobs::AwaitFunction().
	ppTexture->Reset();

	// The *CreateTextureFromFileInMemory doesn't know about (or care about)
	// the footer, so exclude it from the file size.
	if (bAsyncCreate)
	{
		// Immediate creation off the render thread.
		*ppTexture = RenderDevice::Get()->AsyncCreateTexture(
			m_TextureConfig,
			m_Data,
			m_uWidth,
			m_uHeight,
			m_eFormat);
	}
	else
	{
		// Render thread creation.
		*ppTexture = RenderDevice::Get()->CreateTexture(
			m_TextureConfig,
			m_Data,
			m_uWidth,
			m_uHeight,
			m_eFormat);
	}
}

/**
 * Decodes the texture file data into a surface pointer and size data.
 */
Bool TextureContentLoader::InternalDecodeTexture()
{
	// Not even big enough to extract the footer, fail immediately.
	if (m_CurrentLevelData.m_uFileSizeInBytes < sizeof(TextureFooter))
	{
		return false;
	}

	// Extract parameters of the current level.
	UInt32 uWidth = 0u;
	UInt32 uHeight = 0u;
	PixelFormat eFormat = PixelFormat::kInvalid;
	void const* pTexture = nullptr;
	void const* pTextureSecondary = nullptr;
	if (!DDS::Decode(
		m_CurrentLevelData.m_pFileData,
		m_CurrentLevelData.m_uFileSizeInBytes - sizeof(TextureFooter),
		uWidth,
		uHeight,
		eFormat,
		pTexture,
		pTextureSecondary))
	{
		return false;
	}

	// Either this is the only level, or we must fulfill certain requirements
	// compared to the existing levels.
	if (m_Data.HasLevels())
	{
		// Must have the same format.
		if (m_eFormat != eFormat)
		{
			return false;
		}

		// Must be 2x each existing dimension.
		if (uWidth != (m_uWidth << 1) ||
			uHeight != (m_uHeight << 1))
		{
			return false;
		}
	}

	// Good to go, track through. This is now mip0.
	m_uWidth = uWidth;
	m_uHeight = uHeight;
	m_eFormat = eFormat;
	m_CurrentLevelData.m_pTextureData = pTexture;
	m_CurrentLevelData.m_pTextureDataSecondary = pTextureSecondary;
	return true;
}

/**
 * Frees loaded texture data if still owned by this TextureContentLoader.
 */
void TextureContentLoader::InternalFreeCurrentLevelData()
{
	m_CurrentLevelData.Free();
}

/**
 * Release the loader's reference on its content entry - doing this as
 * soon as loading completes allows anything waiting for the load to react
 * as soon as possible.
 */
void TextureContentLoader::InternalReleaseEntry()
{
	if (m_hEntry.IsInternalPtrValid())
	{
		// NOTE: We need to release our reference before decrementing the loader count.
		// This is safe, because a Content::Entry's Content::Store always maintains 1 reference,
		// and does not release it until the content is done loading.
		TextureContentHandle::EntryType* pEntry = m_hEntry.GetContentEntry().GetPtr();
		m_hEntry.Reset();
		pEntry->DecrementLoaderCount();
	}
}

TextureContentLoader::CurrentLevelData::CurrentLevelData()
{
}

TextureContentLoader::CurrentLevelData::~CurrentLevelData()
{
	Free();
}

void TextureContentLoader::CurrentLevelData::Free()
{
	m_pTextureDataSecondary = nullptr;
	m_pTextureData = nullptr;
	m_uFileSizeInBytes = 0u;
	MemoryManager::Deallocate(m_pFileData);
	m_pFileData = nullptr;
}

SharedPtr<TextureLevelData> TextureContentLoader::CurrentLevelData::ReleaseAsTextureLevelData()
{
	auto pFileData = m_pFileData;
	m_pFileData = nullptr;
	auto uFileSizeInBytes = m_uFileSizeInBytes;
	m_uFileSizeInBytes = 0u;
	auto pTextureData = m_pTextureData;
	m_pTextureData = nullptr;
	auto pTextureDataSecondary = m_pTextureDataSecondary;
	m_pTextureDataSecondary = nullptr;

	return SharedPtr<TextureLevelData>(SEOUL_NEW(MemoryBudgets::Content) TextureLevelData(
		pFileData,
		uFileSizeInBytes,
		pTextureData,
		pTextureDataSecondary));
}

} // namespace Seoul
