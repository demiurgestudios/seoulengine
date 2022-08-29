/**
 * \file FalconTextureCache.cpp
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FalconRendererInterface.h"
#include "FalconTextureCache.h"
#include "Logger.h"

namespace Seoul::Falcon
{

static inline UInt8* CreateOnePixelWhiteData()
{
	auto pReturn = (UInt8*)MemoryManager::Allocate(4u, MemoryBudgets::Falcon);
	pReturn[0] = 255;
	pReturn[1] = 255;
	pReturn[2] = 255;
	pReturn[3] = 255;
	return pReturn;
}

TextureCache::TextureCache(
	RendererInterface* pRendererInterface,
	const TextureCacheSettings& settings)
	: m_pRendererInterface(pRendererInterface)
	, m_pSolidFillBitmap(SEOUL_NEW(MemoryBudgets::Falcon) BitmapDefinition(1u, 1u, CreateOnePixelWhiteData(), true))
	, m_Packer(pRendererInterface, settings.m_iTexturePackerWidth, settings.m_iTexturePackerHeight)
	, m_Settings(settings)
	, m_pPackerTexture()
	, m_tTextures()
	, m_iTotalTextureMemoryUsageInBytes(0)
	, m_tFonts()
	, m_List()
{
	m_pRendererInterface->ResolvePackerTexture(m_Packer, m_pPackerTexture);
}

TextureCache::~TextureCache()
{
	Destroy();
}

void TextureCache::Destroy()
{
	// Entire list is purged.
	m_List.RemoveAll();

	// Cleanup fonts.
	{
		auto const iBeginFonts = m_tFonts.Begin();
		auto const iEndFonts = m_tFonts.End();
		for (auto i = iBeginFonts; iEndFonts != i; ++i)
		{
			SafeDeleteTable(*i->Second);
		}
	}
	SafeDeleteTable(m_tFonts);

	// Cleanup textures.
	m_iTotalTextureMemoryUsageInBytes = 0;
	m_vLoadingTextures.Clear();
	m_tTextureLoadingData.Clear();
	SafeDeleteTable(m_tTextures);

	// Cleanup the packer.
	m_Packer.Clear();
}

Bool TextureCache::Prefetch(
	Float fRenderThreshold,
	FilePath filePath)
{
	// Resolve the bitmap identifier - this picks the identifier based on mip level and
	// target render size.
	if (!ResolveBitmapFilePath(fRenderThreshold, filePath))
	{
		return false;
	}

	// For Prefetch, don't kick off a load of the target resolution
	// unless loading is empty. So, check if we have the entry - if so,
	// we're done. Otherwise, only resolve if no loads are currently
	// active.
	//
	// Simple case - entry already exists.
	{
		TextureCacheTextureEntry* pEntry = nullptr;
		if (m_tTextures.GetValue(filePath, pEntry))
		{
			return (!pEntry->m_pOriginalTexture->IsLoading());
		}
	}

	// Loads active, early out.
	if (!m_vLoadingTextures.IsEmpty())
	{
		ProcessLoading();
		if (!m_vLoadingTextures.IsEmpty())
		{
			return false;
		}
	}

	// Prefetch.
	auto p = Resolve(filePath);
	return (!p->m_pOriginalTexture->IsLoading());
}

void TextureCache::Purge()
{
	// Prior to cleanup, check for textures currently in use.
	// We want to maintain their loading data to avoid a down res.
	// hiccup.
	TextureLoadingDataTable tKeep;
	{
		auto uKeepFrame = m_pRendererInterface->GetRenderFrameCount();
		if (uKeepFrame > 0u) { --uKeepFrame; }
		for (auto pEntry = m_List.GetHeadGlobal(); nullptr != pEntry; pEntry = pEntry->GetNextGlobal())
		{
			// Asset in use for the last render frame, keep it.
			if (pEntry->GetLastDrawFrameCount() >= uKeepFrame)
			{
				FilePath highest(pEntry->m_ID);
				highest.SetType(FileType::LAST_TEXTURE_TYPE);

				TextureLoadingData data;
				if (m_tTextureLoadingData.GetValue(highest, data))
				{
					// Can fail, since we're mapping potentially multiple
					// mip levels to a single identifier. Also, mark
					// this entry as needing a refresh.
					data.m_bNeedsRefresh = true;
					(void)tKeep.Insert(highest, data);
				}
			}
			// Once we hit older entries, we're done, since we're dealing with an LRU
			// list.
			else
			{
				break;
			}
		}
	}

	// Cleanup everything.
	Destroy();

	// Restore loading data.
	m_tTextureLoadingData.Swap(tKeep);
}

Bool TextureCache::ResolveBitmapFilePath(
	Float fRenderThreshold,
	FilePath& rFilePath)
{
	// Indirect image resolve - an indirect image will
	// have only a relative filename, no directory or type.
	if (rFilePath.GetDirectory() == GameDirectory::kUnknown &&
		rFilePath.GetType() == FileType::kUnknown)
	{
		Lock lock(m_IndirectTextureMutex);
		if (!m_tIndirectTextureLookup.GetValue(rFilePath.GetRelativeFilenameWithoutExtension(), rFilePath))
		{
			return false;
		}
	}

	TextureLoadingData* pData = nullptr;

	// Retrieve loading data. If this fails, we need to load the highest
	// mip level to acquire it.
	{
		FilePath highest(rFilePath);
		highest.SetType(FileType::LAST_TEXTURE_TYPE);
		pData = m_tTextureLoadingData.Find(highest);
		if (nullptr == pData || pData->m_bNeedsRefresh)
		{
			TextureCacheTextureEntry* pEntry = nullptr;
			TextureLoadingData newData;
			if (m_tTextures.GetValue(highest, pEntry) &&
				pEntry->m_pOriginalTexture->ResolveLoadingData(highest, newData))
			{
				auto const e = m_tTextureLoadingData.Overwrite(highest, newData);
				SEOUL_ASSERT(e.Second);
				pData = &e.First->Second;
			}
			// Otherwise, in the fallback case, just use mip 4. Don't
			// fall through if we were just asked to refresh.
			else if (nullptr == pData)
			{
				rFilePath = highest;
				return true;
			}
		}
	}

	// Perform selection based on size.
	{
		FilePath ret(rFilePath);
		for (auto const& entry : pData->m_aEntries)
		{
			if (fRenderThreshold <= entry.m_fThreshold)
			{
				ret.SetType(entry.m_eType);
				break;
			}
		}

		rFilePath = ret;
		return true;
	}
}

TextureCacheGlyphEntry const* TextureCache::ResolveGlyph(
	const TextChunk& textChunk,
	GlyphsTable& rtGlyphs,
	UniChar uCodePoint)
{
	// Invalid code point, return immediately.
	if (0 == uCodePoint)
	{
		return nullptr;
	}

	// This block handles retrieving already cached glyphs.
	{
		TextureCacheGlyphEntry* pEntry = nullptr;
		if (rtGlyphs.GetValue(uCodePoint, pEntry))
		{
			// Glyph packing is not ready yet - either not started,
			// or still in progress of being asynchronously packed.
			if (!pEntry->IsPackReady())
			{
				// No node ID yet, need to pack the node.
				if (0 == pEntry->GetPackedNodeID())
				{
					(void)Pack(
						textChunk.m_Format.m_Font,
						(Int32)uCodePoint, *pEntry);
				}
			}

			// If we get here, we have a cached glyph - mark it as in-use
			// and return the glyph data.
			pEntry->Use(m_List, m_pRendererInterface->GetRenderFrameCount());
			return pEntry;
		}
	}

	// This block handles inserting a new glyph into
	// the glyph table.
	{
		TextureCacheGlyphEntry* pEntry = SEOUL_NEW(MemoryBudgets::Falcon) TextureCacheGlyphEntry();
		(void)Pack(textChunk.m_Format.m_Font, uCodePoint, *pEntry);
		SEOUL_VERIFY(rtGlyphs.Insert(uCodePoint, pEntry).Second);

		// Otherwise, use the glyph and return it.
		pEntry->Use(m_List, m_pRendererInterface->GetRenderFrameCount());
		return pEntry;
	}
}

Bool TextureCache::ResolveTextureReference(
	Float fRenderThreshold,
	const SharedPtr<BitmapDefinition const>& p,
	TextureReference& rReference,
	Bool bUsePacked)
{
	if (p.IsValid())
	{
		return DoResolveTextureReference(
			fRenderThreshold,
			p->GetFilePath(),
			p->GetData(),
			p->GetWidth(),
			p->GetHeight(),
			p->IsFullOccluder(),
			bUsePacked && p->CanPack(),
			rReference);
	}
	else
	{
		return DoResolveTextureReference(
			fRenderThreshold,
			FilePath(),
			nullptr,
			0u,
			0u,
			false,
			true,
			rReference);
	}
}

#if SEOUL_ENABLE_CHEATS
// Global shadow for reporting to developer via Reflection.
Mutex g_DevOnlyIndirectTextureShadowMutex;
HashTable<FilePathRelativeFilename, FilePath, MemoryBudgets::Falcon> g_tDevOnlyIndirectTextureLookup;

FilePath DevOnlyIndirectTextureLookup(FilePathRelativeFilename id)
{
	Lock lock(g_DevOnlyIndirectTextureShadowMutex);

	FilePath ret;
	(void)g_tDevOnlyIndirectTextureLookup.GetValue(id, ret);
	return ret;
}
#endif // /SEOUL_ENABLE_CHEATS

/**
 * Configure an indirect texture - this establishes
 * a key-to-FilePath association that can be accessed
 * by more than one BitmapInstance.
 */
void TextureCache::UpdateIndirectTexture(FilePathRelativeFilename name, FilePath filePath)
{
	{
		Lock lock(m_IndirectTextureMutex);
		if (!filePath.IsValid())
		{
			m_tIndirectTextureLookup.Erase(name);
		}
		else
		{
			m_tIndirectTextureLookup.Overwrite(name, filePath);
		}
	}

#if SEOUL_ENABLE_CHEATS
	// Global shadow for reporting to developer via Reflection.
	{
		Lock lock(g_DevOnlyIndirectTextureShadowMutex);
		if (!filePath.IsValid())
		{
			g_tDevOnlyIndirectTextureLookup.Erase(name);
		}
		else
		{
			g_tDevOnlyIndirectTextureLookup.Overwrite(name, filePath);
		}
	}
#endif // /SEOUL_ENABLE_CHEATS
}

Bool TextureCache::DoResolveTextureReference(
	Float fRenderThreshold,
	FilePath filePath,
	UInt8 const* pData,
	UInt32 uDataWidth,
	UInt32 uDataHeight,
	Bool bIsFullOccluder,
	Bool bCanPack,
	TextureReference& rReference)
{
	// Solid fill, use our local bitmap in this case.
	if (!filePath.IsValid())
	{
		filePath = m_pSolidFillBitmap->GetFilePath();
		pData = m_pSolidFillBitmap->GetData();
		uDataWidth = m_pSolidFillBitmap->GetWidth();
		uDataHeight = m_pSolidFillBitmap->GetHeight();
		bIsFullOccluder = m_pSolidFillBitmap->IsFullOccluder();
		bCanPack = true;
	}

	UInt32 const uRenderFrameCount = m_pRendererInterface->GetRenderFrameCount();
	TextureCacheTextureEntry* pEntry = nullptr;

	if (nullptr != pData)
	{
		// Just resolve based on the starting identifier and data.
		pEntry = Resolve(filePath, pData, uDataWidth, uDataHeight, bIsFullOccluder, bCanPack);

		// Sanity check, and mark the entry in use.
		SEOUL_ASSERT(nullptr != pEntry);
		pEntry->Use(m_List, uRenderFrameCount);
	}
	// Otherwise, more complex resolve.
	else
	{
		// Resolve the bitmap identifier - this picks the identifier based on mip level and
		// target render size.
		if (!ResolveBitmapFilePath(fRenderThreshold, filePath))
		{
			return false;
		}

		// Always resolve the desired texture.
		pEntry = Resolve(filePath, pData, uDataWidth, uDataHeight, bIsFullOccluder, bCanPack);

		// Sanity check, and mark the entry in use.
		SEOUL_ASSERT(nullptr != pEntry);
		pEntry->Use(m_List, uRenderFrameCount);

		// TODO: Reevaluate and revise - this is quite a bit
		// of looping and checking per bitmap resolve.
		if (pEntry->m_pOriginalTexture->IsLoading())
		{
			// Get loading status for queries.
			auto newFilePath(filePath);
			newFilePath.SetType(FileType::LAST_TEXTURE_TYPE);
			auto pLoadingData = m_tTextureLoadingData.Find(newFilePath);
			if (nullptr != pLoadingData)
			{
				// Reverse order since the order in the loading data
				// is smallest resolution to largest.
				for (Int i = (Int32)pLoadingData->m_aEntries.GetSize() - 1; i >= 0; --i)
				{
					newFilePath.SetType(pLoadingData->m_aEntries[i].m_eType);
					if (filePath == newFilePath)
					{
						continue;
					}

					TextureCacheTextureEntry* pNewEntry = nullptr;
					if (m_tTextures.GetValue(newFilePath, pNewEntry) &&
						!pNewEntry->m_pOriginalTexture->IsLoading())
					{
						// Sanity check, and mark the entry in use.
						pNewEntry->Use(m_List, uRenderFrameCount);
						filePath = newFilePath;
						pEntry = pNewEntry;
						break;
					}
				}
			}
		}
	}

	// Sanity check.
	SEOUL_ASSERT(nullptr != pEntry);

	// Prepare the found entry for render.
	if (!Prepare(*pEntry))
	{
		return false;
	}

	// Always return unpacked if requested.
	if (!bCanPack)
	{
		rReference = pEntry->m_UnpackedReference;
	}
	else
	{
		rReference = pEntry->m_Reference;
	}
	return true;
}

Bool TextureCache::MakeRoomInPacker()
{
	Bool bCollectGarbage = false;
	UInt32 const uCurrentFrameCount = m_pRendererInterface->GetRenderFrameCount();

	for (TextureCacheListEntry* pEntry = m_List.GetTailPacked(); nullptr != pEntry; )
	{
		if (pEntry->GetLastDrawFrameCount() + m_Settings.m_uTexturePackerPurgeThresholdInFrames < uCurrentFrameCount)
		{
			TextureCacheListEntry* pEntryToUnpack = pEntry;
			pEntry = pEntry->GetPrevPacked();

			if (m_Packer.UnPack(pEntryToUnpack->GetPackedNodeID()))
			{
				pEntryToUnpack->UnPack(m_List);
				bCollectGarbage = true;
			}
		}
		// List is sorted by last draw frame, so once we've hit an entry above our threshold, we're done.
		else
		{
			break;
		}
	}

	if (bCollectGarbage)
	{
		m_Packer.CollectGarbage();
		return true;
	}

	return false;
}

Bool TextureCache::Pack(
	const Font& font,
	UniChar uCodePoint,
	TextureCacheGlyphEntry& rGlyphEntry)
{
	PackerTree2D::NodeID nodeID = 0;
	if (m_Packer.Pack(font, uCodePoint, nodeID, rGlyphEntry.m_Glyph, rGlyphEntry.m_pTexture))
	{
		rGlyphEntry.Pack(m_List, nodeID);
		rGlyphEntry.Use(m_List, m_pRendererInterface->GetRenderFrameCount());
		return true;
	}

	if (!MakeRoomInPacker())
	{
		return false;
	}

	if (m_Packer.Pack(font, uCodePoint, nodeID, rGlyphEntry.m_Glyph, rGlyphEntry.m_pTexture))
	{
		rGlyphEntry.Pack(m_List, nodeID);
		rGlyphEntry.Use(m_List, m_pRendererInterface->GetRenderFrameCount());
		return true;
	}

	return false;
}

Bool TextureCache::Pack(
	const SharedPtr<Texture>& pSource,
	const Rectangle2DInt& source,
	PackerTree2D::NodeID& rNodeID,
	Int32& riX,
	Int32& riY)
{
	if (m_Packer.Pack(
		pSource,
		source,
		rNodeID,
		riX,
		riY))
	{
		return true;
	}

	if (!MakeRoomInPacker())
	{
		return false;
	}

	return m_Packer.Pack(
		pSource,
		source,
		rNodeID,
		riX,
		riY);
}

Bool TextureCache::Prepare(TextureCacheTextureEntry& rEntry)
{
	// Already packed and ready. Also considered ready
	// if packing is not supported and we've setup the unpacked
	// reference.
	if (rEntry.IsPackReady() || (!rEntry.m_bSupportsPacking && rEntry.m_UnpackedReference.m_pTexture.IsValid()))
	{
		return true;
	}

	// Not ready, but packed, so just wait for async packing
	// to be completed.
	if (0 != rEntry.GetPackedNodeID())
	{
		return true;
	}

	// Texture is not ready and can't be used for render.
	TextureMetrics metrics;
	if (!rEntry.m_pOriginalTexture->ResolveTextureMetrics(metrics))
	{
		return false;
	}

	// Setup the default configuration for the reference.
	rEntry.m_Reference.m_vAtlasOffset = metrics.m_vAtlasOffset;
	rEntry.m_Reference.m_vAtlasScale = metrics.m_vAtlasScale;
	rEntry.m_Reference.m_vOcclusionOffset = metrics.m_vOcclusionOffset;
	rEntry.m_Reference.m_vOcclusionScale = metrics.m_vOcclusionScale;
	rEntry.m_Reference.m_vVisibleOffset = metrics.m_vVisibleOffset;
	rEntry.m_Reference.m_vVisibleScale = metrics.m_vVisibleScale;
	rEntry.m_PackedReference = rEntry.m_Reference;
	rEntry.m_UnpackedReference = rEntry.m_Reference;

	// Compute the bounding rectangle.
	Int32 const iTextureWidth = metrics.m_iWidth;
	Int32 const iTextureHeight = metrics.m_iHeight;

	// Derive the rescale portion.
	auto const& vScale = rEntry.m_Reference.m_vAtlasScale;
	Vector4D const vVisible(rEntry.m_Reference.m_vVisibleScale, rEntry.m_Reference.m_vVisibleOffset);
	Float const fX0 = vVisible.Z * (vScale.X * (Float)iTextureWidth);
	Float const fY0 = vVisible.W * (vScale.Y * (Float)iTextureHeight);
	Float const fWidth = vVisible.X * (vScale.X * (Float)iTextureWidth);
	Float const fHeight = vVisible.Y * (vScale.Y * (Float)iTextureHeight);

	// Apply the rescale to get the actual visible window.
	Int32 const iX0 = (Int32)fX0;
	Int32 const iY0 = (Int32)fY0;
	Int32 const iWidth = (Int32)fWidth;
	Int32 const iHeight = (Int32)fHeight;

	// Check - if the dimensions are bigger than our max, don't pack it.
	if ((iWidth * iHeight) > m_Settings.m_iTexturePackerSubImageMaxDimensionSquare)
	{
		if (rEntry.m_pOriginalTexture->HasDimensions())
		{
			rEntry.m_bSupportsPacking = false;
		}

		return true;
	}

	// Attempt to pack the texture - if this fails, just
	// return true (to use the texture directly). We don't
	// mark rEntry.m_bSupportsPacking = false here, since
	// we want to try again in case space frees up in the packer.
	PackerTree2D::NodeID nodeID = 0;
	Int32 iX = 0;
	Int32 iY = 0;
	Rectangle2DInt const source(iX0, iY0, (iX0 + iWidth), (iY0 + iHeight));
	if (!Pack(rEntry.m_pOriginalTexture, source, nodeID, iX, iY))
	{
		return true;
	}

	Vector2D const vPackedScale(Vector2D((Float)iWidth / (Float)m_Packer.GetWidth(), (Float)iHeight / (Float)m_Packer.GetHeight()));
	Vector2D const vPackedOffset(Vector2D((Float)iX / (Float)m_Packer.GetWidth(), (Float)iY / (Float)m_Packer.GetHeight()));

	// When pack into the global atlas texture, we need
	// to undo the visible shift and scale, since
	// we've tightly packed the texture data so that the visible
	// edges of the data map to the atlas region.
	//
	// Note that we are effectively "throwing away" the atlas scale and offset in the original data.
	// This is because that is an atlas remap, and we've replaced it with our combined
	// atlas remap.
	Vector2D const vInverseVisibleScale(
		IsZero(rEntry.m_PackedReference.m_vVisibleScale.X) ? 0.0f : (1.0f / rEntry.m_PackedReference.m_vVisibleScale.X),
		IsZero(rEntry.m_PackedReference.m_vVisibleScale.Y) ? 0.0f : (1.0f / rEntry.m_PackedReference.m_vVisibleScale.Y));
	rEntry.Pack(m_List, nodeID);
	rEntry.m_PackedReference = rEntry.m_Reference;
	rEntry.m_PackedReference.m_pTexture = m_pPackerTexture;
	rEntry.m_PackedReference.m_vAtlasOffset =
		Vector2D::ComponentwiseMultiply(
			Vector2D::ComponentwiseMultiply(vPackedScale, -rEntry.m_PackedReference.m_vVisibleOffset),
			vInverseVisibleScale) + vPackedOffset;
	rEntry.m_PackedReference.m_vAtlasScale =
		Vector2D::ComponentwiseMultiply(vPackedScale, vInverseVisibleScale);
	rEntry.m_PackedReference.m_vAtlasMin = vPackedOffset;
	rEntry.m_PackedReference.m_vAtlasMax = (vPackedOffset + vPackedScale);
	return true;
}

void TextureCache::ProcessLoading()
{
	// Process the loading queue.
	Int32 iLoadingTextures = (Int32)m_vLoadingTextures.GetSize();
	for (Int32 i = 0; i < iLoadingTextures; ++i)
	{
		Int32 iMemoryUsageInBytes = 0;
		if (m_vLoadingTextures[i]->ResolveMemoryUsageInBytes(iMemoryUsageInBytes))
		{
			m_iTotalTextureMemoryUsageInBytes += iMemoryUsageInBytes;
			Swap(m_vLoadingTextures[i], m_vLoadingTextures.Back());
			m_vLoadingTextures.PopBack();
			--i;
			--iLoadingTextures;
		}
	}
}

void TextureCache::PurgeTextures(UInt32 uFramesThreshold)
{
	Bool bCollectGarbage = false;
	UInt32 const uCurrentFrameCount = m_pRendererInterface->GetRenderFrameCount();

	for (TextureCacheTextureEntry* pEntry = m_List.GetTailGlobal(); nullptr != pEntry; )
	{
		if (pEntry->GetLastDrawFrameCount() + uFramesThreshold <= uCurrentFrameCount)
		{
			if (0 != pEntry->GetPackedNodeID())
			{
				if (m_Packer.UnPack(pEntry->GetPackedNodeID()))
				{
					pEntry->UnPack(m_List);
					bCollectGarbage = true;
				}
			}

			SEOUL_VERIFY(m_tTextures.Erase(pEntry->m_ID));

			m_iTotalTextureMemoryUsageInBytes -= pEntry->m_pOriginalTexture->GetMemoryUsageInBytes();
			SEOUL_ASSERT(m_iTotalTextureMemoryUsageInBytes >= 0);

			TextureCacheTextureEntry* pEntryToRemove = pEntry;
			pEntry = pEntry->GetPrevGlobal();
			pEntryToRemove->Remove(m_List);
			SafeDelete(pEntryToRemove);
		}
		// List is sorted by last draw frame, so once we've hit an entry above our threshold, we're done.
		else
		{
			break;
		}
	}

	if (bCollectGarbage)
	{
		m_Packer.CollectGarbage();
	}
}

TextureCacheTextureEntry* TextureCache::Resolve(
	const FilePath& filePath,
	UInt8 const* pData,
	UInt32 uDataWidth,
	UInt32 uDataHeight,
	Bool bIsFullOccluder,
	Bool bCanPack)
{
	// Simple case - entry already exists.
	{
		TextureCacheTextureEntry* pReturn = nullptr;
		if (m_tTextures.GetValue(filePath, pReturn))
		{
			return pReturn;
		}
	}

	// Insert a new entry.
	TextureCacheTextureEntry* pEntry = SEOUL_NEW(MemoryBudgets::Falcon) TextureCacheTextureEntry(m_List);
	if (!bCanPack) { pEntry->m_bSupportsPacking = false; }
	pEntry->m_ID = filePath;
	pEntry->Use(m_List, m_pRendererInterface->GetRenderFrameCount());
	if (nullptr != pData)
	{
		m_pRendererInterface->ResolveTexture(
			pData,
			uDataWidth,
			uDataHeight,
			4u,
			bIsFullOccluder,
			pEntry->m_pOriginalTexture);
	}
	else
	{
		m_pRendererInterface->ResolveTexture(
			filePath,
			pEntry->m_pOriginalTexture);
	}
	pEntry->m_Reference.m_pTexture = pEntry->m_pOriginalTexture;
#if SEOUL_ENABLE_CHEATS
	pEntry->m_Reference.m_eTextureType = filePath.GetType();
#endif // /SEOUL_ENABLE_CHEATS
	SEOUL_VERIFY(m_tTextures.Insert(filePath, pEntry).Second);
	m_vLoadingTextures.PushBack(pEntry->m_pOriginalTexture);

	// Process the loading queue.
	ProcessLoading();

	// Prune if necessary.
	if (m_iTotalTextureMemoryUsageInBytes > m_Settings.m_iTextureMemoryHardPurgeThresholdInBytes)
	{
		PurgeTextures(1);
	}
	else if (m_iTotalTextureMemoryUsageInBytes > m_Settings.m_iTextureMemorySoftPurgeThresholdInBytes)
	{
		PurgeTextures(Max(m_Settings.m_uTextureMemorySoftPurgeThresholdInFrames, 1));
	}

	// Simple case - entry already exists.
	{
		TextureCacheTextureEntry* pReturn = nullptr;
		if (m_tTextures.GetValue(filePath, pReturn))
		{
			return pReturn;
		}
	}

	// Failure case.
	return nullptr;
}

} // namespace Seoul::Falcon
